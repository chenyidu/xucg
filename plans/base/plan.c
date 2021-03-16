
#include "plan.h"

ucg_plan_t* ucg_plan_allocate(uint32_t size)
{
    // TODO: use memory pool ?
    ucg_plan_t *plan = ucs_calloc(size, "ucg_plan_t");
    if (plan == NULL) {
        return NULL;
    }
    plan->refcount = 1;
    return;
}

void ucg_plan_release(ucg_plan_t *plan, ucg_plan_cleanup_cb_t cleanup)
{
    ucs_assert(plan != NULL && plan->refcount > 0);
    if (--plan->refcount == 0) {
        if (cleanup != NULL) {
            cleanup(plan);
        }
        ucs_free(plan);
    }
    return;
}

void ucg_plan_release_actions(ucg_plan_t *plan)
{
    ucg_plan_action_t *action = NULL;
    ucg_plan_action_t *taction = NULL;
    ucs_list_for_each_safe (action, taction, &plan->action_list, list) {
        ucs_list_del(&action->list);
        ucg_plan_action_release(action);
    }
    return UCS_OK;
}

void ucg_plan_cleanup(ucg_plan_t *plan)
{
    if (plan->params.members.mh != NULL) {
        ucs_free(plan->params.members.mh);
        plan->params.members.mh = NULL;
    }

    if (plan->params.config != NULL) {
        ucs_free(plan->params.config);
        plan->params.config = NULL;
    }

    if (plan->dt.pack_state != NULL) {
        ucg_dt_state_finish(plan->dt.pack_state);
        plan->dt.pack_state = NULL;
    }

    if (plan->dt.unpack_state != NULL) {
        ucg_dt_state_finish(plan->dt.unpack_state);
        plan->dt.unpack_state = NULL;
    }
    ucg_plan_release_actions(plan);

    return;
}

ucs_status_t ucg_plan_clone_params(ucg_plan_t *plan, ucg_plan_params_t *params)
{
    ucg_plan_params_t *plan_params = &plan->params;
    plan_params->type = params->type;
    if (plan_params->config != NULL) {
        plan_params->config = ucs_malloc(plan->core->config_entry.size, "plan config");
        if (plan_params->config == NULL) {
            return UCS_ERR_NO_MEMORY;
        }
        ucs_status_t status = ucs_config_parser_clone_opts(params->config, 
                                                           plan_params->config, 
                                                           plan->core->config_entry.table);
        if (status != UCS_OK) {
            ucs_free(plan_params->config);
            ucs_error("Fail to clone opts.");
            return status;
        }
    }

    plan_params->members.count = params->members.count;
    plan_params->members.self = params->members.self;
    int alloc_size = plan_params->members.count * sizeof(uint64_t);
    uint64_t *mh = ucs_malloc(alloc_size, "plan members");
    if (mh == NULL) {
        return UCS_ERR_NO_MEMORY;
    }
    memcpy(mh, params->members.mh, alloc_size);
    plan_params->members.mh = mh; 
    return UCS_OK;
}

ucs_status_t ucg_plan_create_and_append_action(ucg_plan_t *plan, 
                                               ucg_plan_action_type_t type, 
                                               ucg_rank_t *peers, int count)
{
    const int with_core = 1;
    ucg_plan_action_t *action = ucg_plan_action_allocate(with_core);
    if (action == NULL) {
        return UCS_ERR_NO_MEMORY;
    }
    ucg_plan_action_init_core(action, type, peers, count);
    ucg_plan_append_action(plan, action);
    return;
}

void ucg_plan_bcast_setup_actions(ucs_list_link_t *action_head, uint64_t *mh, 
                                  uint8_t *buffer, int64_t length)
{
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