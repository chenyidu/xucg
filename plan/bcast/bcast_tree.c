#include <ucg/algo/tree/tree.h>
#include <ucg/plan/base/plan.h>
#include <ucg/core/ucg_config.h>
#include <ucs/sys/compiler_def.h>
#include <ucs/debug/log.h>
#include <ucs/debug/assert.h>

typedef struct ucg_plan_bcast_tree ucg_plan_bcast_tree_t;
typedef ucs_status_t (*ucg_calc_tree_node_func_t)(ucg_plan_bcast_tree_t *plan, 
                                                  const ucg_config_t *config,
                                                  ucg_plan_tree_node_t *node);

typedef struct ucg_plan_bcast_ktree_config {
    uint32_t degree;
} ucg_plan_bcast_ktree_config_t;

typedef struct ucg_plan_bcast_tree {
    ucg_plan_bcast_t super;
    ucg_calc_tree_node_func_t calc_tree_node;
} ucg_plan_bcast_tree_t;

static ucs_config_field_t g_bcast_ktree_config_table[] = {
    {"BCAST_KTREE_DEGREE", "8", "knomial tree degree.", 
     ucs_offsetof(ucg_plan_bcast_ktree_config_t, degree), UCS_CONFIG_TYPE_UINT},
    {NULL},
};
UCG_CONFIG_REGISTER_TABLER(g_bcast_ktree_config_table, "BCAST KTREE", NULL, 
                           ucg_plan_bcast_ktree_config_t);


static ucs_status_t ucg_plan_bcast_btree_calc_node(ucg_plan_bcast_tree_t *plan,
                                                   const ucg_config_t *config,
                                                   ucg_plan_tree_node_t *node)
{
    ucg_group_members_t *members = &plan->super.params.super.members;
    ucg_plan_btree_params_t btree_params = {
        .self = members->self,
        .root = plan->super.params.root,
        .size = members->count,
    };
    // For bcast, leftmost tree has better parallelism.
    ucs_status_t status = ucg_algo_btree_left(&btree_params, node);
    if (status != UCS_OK) {
        ucs_error("Fail to calculate binomial tree node, %s.", ucs_status_string(status));
        return status;
    }
    return UCS_OK;
}

static ucs_status_t ucg_plan_bcast_ktree_calc_node(ucg_plan_bcast_tree_t *plan,
                                                   const ucg_config_t *config,
                                                   ucg_plan_tree_node_t *node)
{
    ucg_plan_bcast_ktree_config_t *ktree_config = UCG_CONFIG_CONVERT(config, ucg_plan_bcast_ktree_config_t);
    ucg_group_members_t *members = &plan->super.params.super.members;
    ucg_plan_ktree_params_t btree_params = {
        .self = members->self,
        .root = plan->super.params.root,
        .size = members->count,
        .degree = ktree_config->degree,
    };
    // For bcast, leftmost tree has better parallelism.
    ucs_status_t status = ucg_algo_ktree_left(&btree_params, node);
    if (status != UCS_OK) {
        ucs_error("Fail to calculate knomial tree node, %s.", ucs_status_string(status));
        return status;
    }
    return UCS_OK;
}

static ucs_status_t ucg_plan_bcast_tree_build_actions(ucg_plan_bcast_tree_t *plan,
                                                      ucg_plan_tree_node_t *node)
{
    ucg_plan_t *bplan = &plan->super.super;
    ucs_status_t status = UCS_OK;
    ucs_assert(node->father_cnt == 0 || node->father_cnt == 1);
    if (node->father_cnt == 1) {
        status = ucg_plan_create_and_append_action(bplan, 
                                                   UCG_PLAN_ACTION_TYPE_RECV, 
                                                   &node->father, 1);
        if (status != UCS_OK) {
            goto release_actions;
        }
    }
    
    int child_cnt = node->child_cnt;
    if (child_cnt > 0) {
        ucg_plan_action_type_t type = node->father_cnt == 0 ? UCG_PLAN_ACTION_TYPE_SEND : UCG_PLAN_ACTION_TYPE_FORWARD;
        status = ucg_plan_create_and_append_action(bplan, type, node->child, child_cnt);
        if (status != UCS_OK) {
            goto release_actions;
        }
    }

    if (node->father_cnt == 1) {
        /* Let COPY after FORWARD, so that COPY and FORWARD may be in parallel.
           FORWARD use NIC, COPY use CPU. */
        status = ucg_plan_create_and_append_action(bplan, 
                                                   UCG_PLAN_ACTION_TYPE_RECV, 
                                                   &node->father, 1);
        if (status != UCS_OK) {
            goto release_actions;
        }
    }

    return UCS_OK;
release_actions:
    ucg_plan_release_actions(bplan);
    return status;
}

static void ucg_plan_bcast_tree_setup_actions_data(ucg_plan_bcast_tree_t *plan)
{
    ucs_list_link_t *action_head = &plan->super.super.action_list;
    ucg_plan_bcast_params_t *params = &plan->super.params;
    ucg_mh_t *mh = params->super.members.mh;
    uint8_t *buffer = params->buffer;
    uint64_t length = ucg_dt_size(params->dtype) * params->count;

    ucg_plan_action_t *action = NULL;
    ucg_plan_action_datav_t *datav = NULL;
    uint8_t *tmp_buffer = NULL;
    int child_cnt = 0;
    ucs_list_for_each(action, action_head, list) {
        action->mh = mh;
        datav = action->datav;
        tmp_buffer = buffer;
        switch (action->core->type) {
            case UCG_PLAN_ACTION_TYPE_RECV:
                ucs_assert(action->core->count == 1);
                datav[0].count = 1;
                datav[0].data[0].length = length;
                break;
            case UCG_PLAN_ACTION_TYPE_FORWARD:
                tmp_buffer = UCG_BUFFER_MSG_HOLDER; // fall through
            case UCG_PLAN_ACTION_TYPE_SEND:
                child_cnt = action->core->count;
                for (int i = 0; i < child_cnt; ++i) {
                    datav[i].count = 1;
                    datav[i].data[0].first = tmp_buffer;
                    datav[i].data[0].length = length;
                }
                break;
            case UCG_PLAN_ACTION_TYPE_COPY:
                ucs_assert(action->core->count == 1);
                datav[0].count = 1;
                datav[0].data[0].first = UCG_BUFFER_MSG_HOLDER;
                datav[0].data[0].second = buffer;
                datav[0].data[0].length = length;
                break;
            default:
                ucs_fatal("Bcast shouldn't have this action(%d).", action->core->type);
                break;
        }
    }    

    return;
}

static ucs_status_t ucg_plan_bcast_tree_clone_actions(ucg_plan_bcast_tree_t *self, 
                                                      ucg_plan_bcast_tree_t *other)
{
    ucs_status_t status = UCS_OK;
    ucs_list_link_t *action_head = &other->super.super.action_list;
    ucg_plan_action_t *action = NULL;
    const int with_core = 0;
    ucs_list_for_each (action, action_head, list) {
        ucg_plan_action_t *new_action = ucg_plan_action_allocate(with_core);
        if (new_action == NULL) {
            status = UCS_ERR_NO_MEMORY;
            goto release_actions;
        }
        new_action->core = ucg_plan_action_obtain_core(action);
        ucg_plan_append_action(&self->super.super, action);
    }
    ucg_plan_bcast_tree_setup_actions_data(self);

    return UCS_OK;
release_actions:
    ucg_plan_release_actions(&self->super.super);
    return status;
}

static ucs_status_t ucg_plan_bcast_tree_create_actions(ucg_plan_bcast_tree_t *plan, 
                                                       const ucg_config_t *config)
{
    // Get my father and children in the tree.
    ucg_rank_t child[UCG_PLAN_PHASE_PEERS_MAX_NUM] = {0};
    ucg_plan_tree_node_t node = {
        .child = child,
        .child_cnt = UCG_PLAN_PHASE_PEERS_MAX_NUM,
    };
    ucs_status_t status = plan->calc_tree_node(plan, config, &node);
    if (status != UCS_OK) {
        ucs_error("Fail to calculate tree node, %s.", ucs_status_string(status));
        return status;
    }

    return ucg_plan_bcast_tree_build_actions(plan, &node);
}

static ucs_status_t ucg_plan_bcast_tree_init(ucg_plan_bcast_tree_t *plan, 
                                             const ucg_config_t *config)
{
    ucs_status_t status = UCS_OK;
    // Create actions
    status = ucg_plan_bcast_tree_create_actions(plan, config);
    if (status != UCS_OK) {
        ucs_error("Fail to create actions, %s", ucs_status_string(status));
        return status;
    }
    // Setup actions data
    ucg_plan_bcast_tree_setup_actions_data(plan);
    return UCS_OK;
}

static ucs_status_t ucg_plan_bcast_tree_copy(ucg_plan_bcast_tree_t *self, 
                                             ucg_plan_bcast_tree_t *other,
                                             const ucg_config_t *config)
{
    /* TODO: Check the parameters to determine how to copy
     * 1. changes root rank ? 
     * 2. changes self rank ?
     */
    ucs_status_t status;
    status = ucg_plan_bcast_tree_clone_actions(self, other);
    if (status != UCS_OK) {
        return status;
    }

    return UCS_OK;
}

static ucs_status_t ucg_plan_bcast_tree_constructor(ucg_plan_t *self_p, 
                                                   const ucg_plan_t *other_p,
                                                   const ucg_config_t *config)
{
    ucg_plan_bcast_tree_t *self = ucs_derived_of(self_p, ucg_plan_bcast_tree_t);
    ucs_assert(self->calc_tree_node != NULL);
    ucs_status_t status;
    if (other_p == NULL) {
        status = ucg_plan_bcast_tree_init(self, config);
    } else {
        ucg_plan_bcast_tree_t *other = ucs_derived_of(other_p, ucg_plan_bcast_tree_t);
        status = ucg_plan_bcast_tree_copy(self, other, config);
    }
    return status;
}

static void ucg_plan_bcast_tree_destructor(ucg_plan_t *plan)
{
    ucg_plan_release_actions(plan);
    return;
}

static ucs_status_t ucg_plan_bcast_btree_constructor(ucg_plan_t *self, 
                                                    const ucg_plan_t *other,
                                                    const ucg_config_t *config)
{
    ucg_plan_bcast_tree_t *plan = ucs_derived_of(self, ucg_plan_bcast_tree_t);
    plan->calc_tree_node = ucg_plan_bcast_btree_calc_node;
    return ucg_plan_bcast_tree_constructor(self, other, config);
}

static ucs_status_t ucg_plan_bcast_ktree_constructor(ucg_plan_t *self, 
                                                    const ucg_plan_t *other,
                                                    const ucg_config_t *config)
{
    ucg_plan_bcast_tree_t *plan = ucs_derived_of(self, ucg_plan_bcast_tree_t);
    plan->calc_tree_node = ucg_plan_bcast_ktree_calc_node;
    return ucg_plan_bcast_tree_constructor(self, other, config);
}

static ucg_plan_core_t g_bcast_btree_core = {
    .id = {
        .type = UCG_PLAN_TYPE_BCAST,
        .algo = UCG_PLAN_BCAST_ALGO_BTREE,
    },
    .desc = "Binomial tree broadcast",
    .cap_flags = 0,
    .plan_size = sizeof(ucg_plan_bcast_tree_t),
    .constructor = ucg_plan_bcast_btree_constructor,
    .destructor = ucg_plan_bcast_tree_destructor,
};
UCG_PLAN_REGISTER(bcast_btree, &g_bcast_btree_core);

static ucg_plan_core_t g_bcast_ktree_core = {
    .id = {
        .type = UCG_PLAN_TYPE_BCAST,
        .algo = UCG_PLAN_BCAST_ALGO_KTREE,
    },
    .desc = "Binomial tree broadcast",
    .cap_flags = 0,
    .plan_size = sizeof(ucg_plan_bcast_tree_t),
    .constructor = ucg_plan_bcast_ktree_constructor,
    .destructor = ucg_plan_bcast_tree_destructor,
};
UCG_PLAN_REGISTER(bcast_ktree, &g_bcast_ktree_core);
