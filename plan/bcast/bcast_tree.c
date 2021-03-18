#include <ucg/algo/tree/tree.h>
#include <ucg/plan/base/action.h>
#include <ucg/plan/base/plan.h>
#include <ucg/plan/ucg_ppool.h>
#include <ucs/sys/compiler_def.h>

typedef ucs_status_t (*ucg_calc_tree_node_cb_t)(ucg_plan_bcast_t *plan, 
                                                ucg_plan_tree_node_t *node);

typedef struct ucg_plan_bcast_ktree_config {
    uint32_t degree;
} ucg_plan_bcast_ktree_config_t;

typedef struct ucg_plan_bcast_tree {
    ucg_plan_bcast_t super;
    ucg_calc_tree_node_cb_t calc_tree_node;
} ucg_plan_bcast_tree_t;

static ucs_status_t ucg_plan_bcast_btree_calc_node(ucg_plan_bcast_tree_t *plan,
                                                   ucg_plan_tree_node_t *node)
{
    ucg_group_members_t *members = &plan->super.params.super.members;
    ucg_plan_btree_params_t btree_params = {
        .self = members->self,
        .root = plan->super.params.root,
        .size = members->count,
    };
    // For bcast, leftmost tree has better parallelism.
    ucs_status_t status = ucg_plan_btree_left(&btree_params, &node);
    if (status != UCS_OK) {
        ucs_error("Fail to calculate binomial tree node, %s.", ucs_status_string(status));
        return status;
    }
    return UCS_OK;
}

static ucs_status_t ucg_plan_bcast_ktree_calc_node(ucg_plan_bcast_tree_t* plan,
                                                   ucg_plan_tree_node_t *node)
{
    ucg_plan_bcast_ktree_config_t *config = (ucg_plan_bcast_ktree_config_t*)plan->super.params.super.config;
    ucg_group_members_t *members = &plan->super.params.super.members;
    ucg_plan_ktree_params_t btree_params = {
        .self = members->self,
        .root = plan->super.params.root,
        .size = members->count,
        .degree = config->degree,
    };
    // For bcast, leftmost tree has better parallelism.
    ucs_status_t status = ucg_plan_ktree_left(&btree_params, &node);
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
    ucg_plan_release_actions(plan);
    return status;
}

static ucs_status_t ucg_plan_bcast_tree_create_actions(ucg_plan_bcast_tree_t *plan)
{
    // Get my father and children in the tree.
    ucg_rank_t child[UCG_PLAN_PHASE_PEERS_MAX_NUM] = {0};
    ucg_plan_tree_node_t node = {
        .child = child,
        .child_cnt = UCG_PLAN_PHASE_PEERS_MAX_NUM,
    };
    ucs_status_t status = plan->calc_tree_node(plan, &node);
    if (status != UCS_OK) {
        ucs_error("Fail to calculate tree node, %s.", ucs_status_string(status));
        return status;
    }

    return ucg_plan_bcast_build_actions(plan, &node);
}

static ucs_status_t ucg_plan_bcast_tree_clone_params(ucg_plan_bcast_tree_t *plan, 
                                                     ucg_plan_bcast_params_t *params)
{
    return ucg_plan_bcast_clone_params(&plan->super, params);
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
                int child_cnt = action->core->count;
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

static ucs_status_t ucg_plan_bcast_tree_init(ucg_plan_bcast_tree_t *plan, 
                                             ucg_plan_bcast_params_t *params)
{
    ucs_status_t status = UCS_OK;

    ucs_list_head_init(&plan->super.super.action_list);

    // Save parameters before creating actions.
    status = ucg_plan_bcast_tree_clone_params(plan, params);
    if (status != UCS_OK) {
        ucs_error("Fail to save parameters.", ucs_status_string(status));
        return status;
    }
    // Create actions
    status = ucg_plan_bcast_tree_create_actions(plan);
    if (status != UCS_OK) {
        ucs_error("Fail to craete actions.", ucs_status_string(status));
        return status;
    }
    // Setup actions data
    ucg_plan_bcast_tree_setup_actions_data(plan);
    return UCS_OK;
}

static void ucg_plan_bcast_tree_cleanup(ucg_plan_t *plan)
{
    ucg_plan_bcast_tree_t *bcast_plan = ucs_derived_of(plan, ucg_plan_bcast_tree_t);
    ucg_plan_params_t *params = &bcast_plan->super.params.super;
    ucg_plan_destroy_params(params);
    ucg_plan_cleanup(plan);
    return;
}

static ucg_plan_t* ucg_plan_bcast_tree_new(ucg_plan_bcast_tree_t *plan,
                                           ucg_plan_bcast_params_t *params)
{
    ucs_status_t status = UCS_OK;
    ucg_plan_bcast_tree_t *new_plan = NULL;

    if (ucs_list_is_empty(&plan->super.super.action_list)) {
        // The plan is not initialized, use its space.
        new_plan = ucg_plan_obtain(&plan->super.super);
    } else {
        new_plan = ucg_plan_allocate(sizeof(ucg_plan_bcast_t));
        if (new_plan == NULL) {
            ucs_error("Fail to allocate plan object.");
            return NULL;
        }
        new_plan->super.super.core = plan->super.super.core;
        new_plan->calc_tree_node = plan->calc_tree_node;
    }

    status = ucg_plan_bcast_tree_init(new_plan, params);
    if (status != UCS_OK) {
        ucg_plan_release(new_plan, ucg_plan_bcast_tree_cleanup);
        return NULL;
    }
    return &new_plan->super;
}

static ucs_status_t ucg_plan_bcast_tree_clone_actions(ucg_plan_bcast_tree_t *plan, 
                                                      ucg_plan_bcast_t *origin_plan)
{
    ucg_plan_bcast_params_t *old_params = &origin_plan->params;
    ucg_plan_bcast_params_t *new_params = &plan->super.params;
    ucg_group_members_t *old_members = &old_params->super.members;
    ucg_group_members_t *new_members = &new_params->super.members;

    ucg_plan_action_t *origin_action = NULL;
    ucg_plan_action_t *action = NULL;
    const int with_core = 0;
    ucs_list_for_each (origin_action, &origin_plan->super.action_list, list) {
        ucg_plan_action_t *new_action = ucg_plan_action_allocate(with_core);
        if (new_action == NULL) {
            goto release_actions;
        }
        new_action->core = ucg_plan_action_obtain_core(origin_action);
        ucg_plan_append_action(&plan->super.super, action);
    }
    ucg_plan_bcast_tree_setup_actions_data(plan);

    return UCS_OK;
release_actions:
    ucg_plan_release_actions(&plan->super.super);
    return UCS_ERR_NO_MEMORY;
}

static ucg_plan_t* ucg_plan_bcast_tree_cow(ucg_plan_bcast_tree_t* plan,
                                           ucg_plan_bcast_params_t *params)
{
    ucs_status_t status = UCS_OK;
    
    ucg_plan_bcast_tree_t *new_plan = ucg_plan_allocate(sizeof(ucg_plan_bcast_t));
    if (new_plan == NULL) {
        ucs_error("Fail to allocate plan object.");
        return NULL;
    }

    status = ucg_plan_bcast_tree_clone_params(new_plan, params);
    if (status != UCS_OK) {
        ucg_plan_release(new_plan, ucg_plan_bcast_tree_cleanup);
        ucs_error("Fail to clone parameters.", ucs_status_string(status));
        return NULL;
    }

    ucs_status_t status = ucg_plan_bcast_tree_clone_actions(new_plan, plan);
    if (status != UCS_OK) {
        ucg_plan_release(new_plan, ucg_plan_bcast_tree_cleanup);
        return NULL;
    }

    return &new_plan->super.super;
}

static ucg_plan_t* ucg_plan_bcast_tree_clone(ucg_plan_t *bplan, 
                                             ucg_plan_params_t *bparams,
                                             ucg_plan_clone_advice_t advice)
{
    if (advice == UCG_PLAN_CLONE_ADVICE_SAME) {
        return ucg_plan_obtain(bplan);
    }
    
    ucg_plan_bcast_t *plan = ucs_container_of(bplan, ucg_plan_bcast_t, super);
    ucg_plan_bcast_params_t *params = ucs_container_of(bparams, ucg_plan_bcast_params_t, super);
    if (advice ==  UCG_PLAN_CLONE_ADVICE_UNEQUAL) {
        return ucg_plan_btree_bcast_new(plan, params);
    }

    if (advice == UCG_PLAN_CLONE_ADVICE_SIMILAR) {
        return ucg_plan_btree_bcast_cow(plan, params);
    }
    // Should never be here.
    ucs_assert(0);
    return NULL;
}

static void ucg_plan_bcast_tree_destroy(ucg_plan_t *bplan)
{
    ucg_plan_release(bplan, ucg_plan_bcast_tree_cleanup);
    return;
}

static ucg_plan_core_t g_bcast_btree_core = {
    .type = UCG_PLAN_TYPE_BCAST,
    .id = UCG_PLAN_BCAST_ID_BTREE,
    .desc = "Binomial tree broadcast",
    .cap_flags = 0,
    .config_entry = UCS_CONFIG_EMPTY_GLOBAL_LIST_ENTRY,
    .clone = ucg_plan_bcast_tree_clone,
    .destroy = ucg_plan_bcast_tree_destroy,
};

static ucg_plan_bcast_tree_t g_bcast_tree = {
    .super = {
        .super = {
            .refcount = 1, /* Never release. */
            .core = &g_bcast_btree_core,
        },
    },
    .calc_tree_node = ucg_plan_bcast_btree_calc_node,
};
UCG_PPOOL_REGISTER_PLAN(g_bcast_tree);

static ucs_config_field_t g_bcast_ktree_config_table[] = {
    {"DEGREE", "knomial tree degree.", 
     ucs_offsetof(ucg_plan_bcast_ktree_config_t, degree), UCS_CONFIG_TYPE_UINT},
    {NULL},
};

static ucg_plan_core_t g_bcast_ktree_core = {
    .type = UCG_PLAN_TYPE_BCAST,
    .id = UCG_PLAN_BCAST_ID_BTREE,
    .desc = "Binomial tree broadcast",
    .cap_flags = 0,
    .config_entry = {
        .table = g_bcast_ktree_config_table,
        .prefix = "BCAST_KTREE",
        .size = sizeof(ucg_plan_bcast_ktree_config_t),
        .name = "Bcast ktree config",
    },
    .clone = ucg_plan_bcast_tree_clone,
    .destroy = ucg_plan_bcast_tree_destroy,
};

static ucg_plan_bcast_tree_t g_bcast_ktree = {
    .super = {
        .super = {
            .refcount = 1, /* Never release. */
            .core = &g_bcast_ktree_core,
        },
    },
    .calc_tree_node = ucg_plan_bcast_ktree_calc_node,
};
UCG_PPOOL_REGISTER_PLAN(g_bcast_ktree);