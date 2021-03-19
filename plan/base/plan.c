
#include "plan.h"
#include <ucs/debug/log.h>
#include <string.h>

ucg_plan_t* ucg_plan_allocate_inner(uint32_t size)
{
    // TODO: use memory pool ?
    ucg_plan_t *plan = (ucg_plan_t*)ucs_calloc(1, size, "ucg_plan_t");
    if (plan == NULL) {
        return NULL;
    }
    plan->refcount = 1;
    return plan;
}

void ucg_plan_release_inner(ucg_plan_t *plan, ucg_plan_cleanup_cb_t cleanup)
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
    return;
}

void ucg_plan_cleanup(ucg_plan_t *plan)
{
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

ucs_status_t ucg_plan_clone_params(ucg_plan_t *plan, 
                                   ucg_plan_params_t *src, 
                                   ucg_plan_params_t *dst)
{
    dst->type = src->type;
    if (src->config != NULL) {
        dst->config = ucs_malloc(plan->core->config_entry.size, "plan config");
        if (dst->config == NULL) {
            return UCS_ERR_NO_MEMORY;
        }
        ucs_status_t status = ucs_config_parser_clone_opts(src->config, 
                                                           dst->config, 
                                                           plan->core->config_entry.table);
        if (status != UCS_OK) {
            ucs_free(dst->config);
            ucs_error("Fail to clone opts.");
            return status;
        }
    }

    dst->members.count = src->members.count;
    dst->members.self = src->members.self;
    int alloc_size = dst->members.count * sizeof(uint64_t);
    uint64_t *mh = ucs_malloc(alloc_size, "plan members");
    if (mh == NULL) {
        return UCS_ERR_NO_MEMORY;
    }
    memcpy(mh, src->members.mh, alloc_size);
    dst->members.mh = mh; 
    return UCS_OK;
}

void ucg_plan_cleanup_params(ucg_plan_params_t *params)
{
    if (params->members.mh != NULL) {
        ucs_free(params->members.mh);
        params->members.mh = NULL;
    }

    if (params->config != NULL) {
        ucs_free(params->config);
        params->config = NULL;
    }
    return;
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
    return UCS_OK;
}

ucs_status_t ucg_plan_bcast_clone_params(ucg_plan_bcast_t *plan, 
                                         ucg_plan_bcast_params_t *params)
{
    ucs_status_t status = UCS_OK;
    status = ucg_plan_clone_params(&plan->super, &plan->params.super, &params->super);
    if (status != UCS_OK) {
        return status;
    }

    plan->params.buffer = params->buffer;
    plan->params.count = params->count;
    plan->params.dtype = params->dtype;
    plan->params.root = params->root;
    return UCS_OK;
}