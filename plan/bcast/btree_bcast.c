#include <ucg/algo/tree/tree.h>
#include <ucg/plan/base/action.h>
#include <ucg/plan/base/plan.h>
#include <ucs/sys/compiler_def.h>

static ucs_status_t ucg_plan_bcast_calc_btree_node(ucg_rank_t root, 
                                                   ucg_rank_t self, 
                                                   uint32_t size, 
                                                   ucg_plan_tree_node_t *node)
{
    ucg_plan_btree_params_t btree_params = {
        .self = self,
        .root = root,
        .size = size,
    };
    // For bcast, btree_left has better parallelism.
    return ucg_plan_btree_left(&btree_params, &node);
}

static ucs_status_t ucg_plan_bcast_build_actions(ucg_plan_bcast_t *plan,
                                                 ucg_plan_tree_node_t *node)
{
    ucs_status_t status = UCS_OK;
    ucs_assert(node->father_cnt == 0 || node->father_cnt == 1);
    if (node->father_cnt == 1) {
        status = ucg_plan_create_and_append_action(&plan->super, 
                                                   UCG_PLAN_ACTION_TYPE_RECV, 
                                                   &node->father, 1);
        if (status != UCS_OK) {
            goto clear_actions;
        }
    }
    
    int child_cnt = node->child_cnt;
    if (child_cnt > 0) {
        ucg_plan_action_type_t type = node->father_cnt == 0 ? UCG_PLAN_ACTION_TYPE_SEND : UCG_PLAN_ACTION_TYPE_FORWARD;
        status = ucg_plan_create_and_append_action(&plan->super, type, node->child, child_cnt);
        if (status != UCS_OK) {
            goto clear_actions;
        }
    }

    if (node->father_cnt == 1) {
        /* Let COPY after FORWARD, so that COPY and FORWARD may be in parallel.
           FORWARD use NIC, COPY use CPU. */
        status = ucg_plan_create_and_append_action(&plan->super, 
                                                   UCG_PLAN_ACTION_TYPE_RECV, 
                                                   &node->father, 1);
        if (status != UCS_OK) {
            goto clear_actions;
        }
    }

    return UCS_OK;
clear_actions:
    ucg_plan_release_actions(plan);
    return status;
}

static ucs_status_t ucg_plan_bcast_create_actions(ucg_plan_bcast_t *plan)
{
    // Calcuate my father and children in the tree.
    ucg_rank_t child[UCG_PLAN_PHASE_PEERS_MAX_NUM] = {0};
    ucg_plan_tree_node_t node = {
        .child = child,
        .child_cnt = UCG_PLAN_PHASE_PEERS_MAX_NUM,
    };
    ucg_group_members_t *members = &plan->super.params.members;
    ucs_status_t status = ucg_plan_bcast_calc_btree_node(plan->params.root, 
                                                         members->self, 
                                                         members->count, 
                                                         &node);
    if (status != UCS_OK) {
        ucs_error("Fail to calculate binomial tree node, %s.", ucs_status_string(status));
        return status;
    }

    return ucg_plan_bcast_build_actions(plan, &node);
}

static ucs_status_t ucg_plan_bcast_clone_params(ucg_plan_bcast_t *plan, 
                                                ucg_plan_bcast_params_t *params)
{
    ucs_status_t status = UCS_OK;
    status = ucg_plan_clone_params(&plan->super, &plan->params.super, &params->super);
    if (status != UCS_OK) {
        return status;
    }
    // Shallow copy
    plan->params.buffer = params->buffer;
    plan->params.count = params->count;
    plan->params.dtype = params->dtype;
    plan->params.root = params->root;
    return UCS_OK;
}


static void ucg_plan_bcast_setup_actions_data(ucg_plan_bcast_t *plan)
{
    ucs_list_link_t *action_head = &plan->super.action_list;
    ucg_mh_t *mh = plan->params.super.members.mh;
    uint8_t *buffer = plan->params.buffer;
    uint64_t length = ucg_dt_size(plan->params.dtype) * plan->params.count;

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

static ucs_status_t ucg_plan_btree_bcast_init(ucg_plan_bcast_t *plan, 
                                              ucg_plan_bcast_params_t *params)
{
    ucs_status_t status = UCS_OK;

    plan->super.core = &g_btree_bcast_core;
    ucs_list_head_init(&plan->super.action_list);

    // Save parameters before creating actions.
    status = ucg_plan_bcast_clone_params(plan, params);
    if (status != UCS_OK) {
        ucs_error("Fail to save parameters.", ucs_status_string(status));
        return status;
    }
    // Create actions
    status = ucg_plan_bcast_create_actions(plan);
    if (status != UCS_OK) {
        ucs_error("Fail to craete actions.", ucs_status_string(status));
        return status;
    }
    // Setup actions data
    ucg_plan_bcast_setup_actions_data(plan);
    return UCS_OK;
}

static void ucg_plan_bcast_cleanup(ucg_plan_t *plan)
{
    ucg_plan_bcast_t *bcast_plan = ucs_derived_of(plan, ucg_plan_bcast_t);
    ucg_plan_params_t *params = &bcast_plan->params.super;

    ucg_plan_cleanup(plan);
    return;
}

static ucg_plan_t* ucg_plan_btree_bcast_new(ucg_plan_bcast_t *plan,
                                                      ucg_plan_bcast_params_t *params)
{
    ucs_status_t status = UCS_OK;
    ucg_plan_bcast_t *new_plan = NULL;

    if (ucs_list_is_empty(&plan->super.action_list)) {
        // The plan is not initialized, use its space.
        new_plan = ucg_plan_obtain(&plan->super);
    } else {
        new_plan = ucg_plan_allocate(sizeof(ucg_plan_bcast_t));
        if (new_plan == NULL) {
            ucs_error("Fail to allocate plan object.");
            return NULL;
        }
    }

    status = ucg_plan_btree_bcast_init(new_plan, params);
    if (status != UCS_OK) {
        ucg_plan_release(new_plan, ucg_plan_bcast_cleanup);
        return NULL;
    }
    return &new_plan->super;
}

static ucs_status_t ucg_plan_btree_bcast_clone_actions(ucg_plan_bcast_t* old_plan,
                                                       ucg_plan_bcast_t* new_plan)
{
    ucg_plan_bcast_params_t *old_params = &old_plan->params;
    ucg_plan_bcast_params_t *new_params = &new_plan->params;
    ucg_group_members_t *old_members = &old_params->super.members;
    ucg_group_members_t *new_members = &new_params->super.members;

    ucs_assert(old_members->count == new_members->count);

    int with_core = 0;
    if (old_params->root != new_params->root && !memcmp(old_members->mh, new_members->mh, old_members->count * sizeof(uint64_t))) {
        // Same members, but only root has changed.
        with_core = 1;
    }

    ucg_plan_action_t *old_action = NULL;
    ucg_plan_action_t *new_action = NULL;
    ucs_list_for_each (old_action, &old_plan->super.action_list, list) {
        ucg_plan_action_t *new_action = ucg_plan_action_allocate(with_core);
        if (new_action == NULL) {
            goto clear_actions;
        }
        new_action->core = old_action->core;
    }
     ucg_plan_bcast_setup_actions(&plan->super.action_list, 
                                 plan->params.super.members.mh,
                                 plan->params.buffer,
                                 ucg_dt_size(plan->params.dtype) * plan->params.count);

    return UCS_OK;

clear_actions:
    ucg_plan_clear_actions(&new_plan->super.action_list);
    return UCS_ERR_NO_MEMORY;
}

static ucg_plan_t* ucg_plan_btree_bcast_cow(ucg_plan_bcast_t* plan,
                                            ucg_plan_bcast_params_t *params)
{
    ucs_status_t status = UCS_OK;
    
    ucg_plan_bcast_t *new_plan = ucg_plan_allocate(sizeof(ucg_plan_bcast_t));
    if (new_plan == NULL) {
        ucs_error("Fail to allocate plan object.");
        return NULL;
    }

    // In order to compare when reusing, the parameters are saved.
    status = ucg_plan_bcast_save_params(plan, params);
    if (status != UCS_OK) {
        ucs_error("Fail to save parameters.", ucs_status_string(status));
        return status;
    }

    ucs_status_t status = ucg_plan_btree_bcast_clone_actions(plan, new_plan);
    if (status != UCS_OK) {
        ucg_plan_release(new_plan, ucg_plan_bcast_cleanup);
        return NULL;
    }
    return &new_plan->super;
}

ucg_plan_t* ucg_plan_btree_bcast_clone(ucg_plan_t *bplan, 
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

static void ucg_plan_btree_bcast_destroy(ucg_plan_t *bplan)
{
    ucg_plan_release(bplan, ucg_plan_btree_bcast_cleanup);
    return;
}

static ucg_plan_core_t g_btree_bcast_core = {
    .type = UCG_PLAN_TYPE_BCAST,
    .id = UCG_PLAN_BCAST_ID_BTREE,
    .desc = "Non topo-aware binomial tree broadcast",
    .flags = 0,
    .config_entry = {
        .name = NULL, 
        .prefix = NULL,
        .table = NULL,
        .size = 0,
    },
    .clone = ucg_plan_btree_bcast_clone,
    .destroy = ucg_plan_btree_bcast_destroy,
};

static ucg_plan_bcast_t g_btree_bcast = {
    .super = {
        .refcount = 1, /* Never release. */
        .core = &g_btree_bcast_core,
    },
};
UCG_PPOOL_REGISTER_PLAN(g_btree_bcast);