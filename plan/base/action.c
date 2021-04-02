#include "action.h"

#include <ucg/core/ucg_util.h>
#include <ucg/core/ucg_rte.h>
#include <ucs/datastruct/mpool.h>
#include <ucs/arch/cpu.h>
#include <ucs/debug/log.h>
#include <ucs/debug/memtrack.h>

#include <limits.h>

typedef struct ucg_plan_action_mgr {
    UCG_THREAD_SAFE_DECLARE(lock); /* Protect get/put on action_mp and action_core_mp. */
    ucs_mpool_t action_mp;
    ucs_mpool_t action_core_mp;
    ucs_mpool_ops_t mp_ops;
} ucg_plan_action_mgr_t;

static ucg_plan_action_mgr_t g_action_mgr = {
    .mp_ops = {
        .chunk_alloc = ucs_mpool_chunk_malloc,
        .chunk_release = ucs_mpool_chunk_free,
        .obj_init = ucs_empty_function,
        .obj_cleanup = ucs_empty_function,
    }
};

static ucs_status_t ucg_plan_action_global_init()
{
    ucs_status_t status = UCS_OK;
    status = UCG_THREAD_SAFE_INIT(&g_action_mgr.lock);
    if (status != UCS_OK) {
        ucs_error("Failed to init thread safe.");
        goto err;
    }

    status = ucs_mpool_init(&g_action_mgr.action_mp, 0, sizeof(ucg_plan_action_t), 0, 
                            UCS_SYS_CACHE_LINE_SIZE, 128, UINT_MAX, 
                            &g_action_mgr.mp_ops, "plan aciton");
    if (status != UCS_OK) {
        ucs_error("Failed to init action mpool.");
        goto err_destroy_thread_safe;
    }

    status = ucs_mpool_init(&g_action_mgr.action_core_mp, 0, sizeof(ucg_plan_action_core_t), 0, 
                            UCS_SYS_CACHE_LINE_SIZE, 128, UINT_MAX, 
                            &g_action_mgr.mp_ops, "plan aciton core");
    if (status != UCS_OK) {
        ucs_error("Fail to init action core mpool.");
        goto err_cleanup_mp;
    }

    return UCS_OK;
err_cleanup_mp:
    ucs_mpool_cleanup(&g_action_mgr.action_mp, 1);
err_destroy_thread_safe:
    UCG_THREAD_SAFE_DESTROY(&g_action_mgr.lock);
err:
    return status;
}

static void ucg_plan_action_global_cleanup()
{
    ucs_mpool_cleanup(&g_action_mgr.action_core_mp, 1);
    ucs_mpool_cleanup(&g_action_mgr.action_mp, 1);
    UCG_THREAD_SAFE_DESTROY(&g_action_mgr.lock);
    return;
}

ucg_plan_action_t* ucg_plan_action_allocate(int with_core)
{
    UCG_THREAD_SAFE_ENTER(&g_action_mgr.lock);
    ucg_plan_action_t *action = (ucg_plan_action_t *)ucs_mpool_get(&g_action_mgr.action_mp);
    UCG_THREAD_SAFE_LEAVE(&g_action_mgr.lock);
    if (action == NULL) {
        return NULL;
    }
    memset(action, 0, sizeof(ucg_plan_action_t));
    if (!with_core) {
        return action;
    }
    
    UCG_THREAD_SAFE_ENTER(&g_action_mgr.lock);
    action->core = (ucg_plan_action_core_t *)ucs_mpool_get(&g_action_mgr.action_core_mp);
    UCG_THREAD_SAFE_LEAVE(&g_action_mgr.lock);
    if (action->core == NULL) {
        ucs_free(action);
        return NULL;
    }
    memset(action->core, 0, sizeof(ucg_plan_action_core_t));
    action->core->refcount = 1;
    return action;
}

void ucg_plan_action_release(ucg_plan_action_t *action)
{
    ucs_assert(action != NULL && action->core->refcount > 0);
    if (--action->core->refcount == 0) {
        UCG_THREAD_SAFE_ENTER(&g_action_mgr.lock);
        ucs_mpool_put(action->core);
        UCG_THREAD_SAFE_LEAVE(&g_action_mgr.lock);
    }
    UCG_THREAD_SAFE_ENTER(&g_action_mgr.lock);
    ucs_mpool_put(action);
    UCG_THREAD_SAFE_LEAVE(&g_action_mgr.lock);
    return;
}

const char* ucg_plan_action_type_str(ucg_plan_action_type_t type)
{
    switch (type) {
        case UCG_PLAN_ACTION_TYPE_SEND:
            return "SEND";
        case UCG_PLAN_ACTION_TYPE_RECV:
            return "RECV";
        case UCG_PLAN_ACTION_TYPE_REDUCE:
            return "REDUCE";
        case UCG_PLAN_ACTION_TYPE_COPY:
            return "COPY";
        case UCG_PLAN_ACTION_TYPE_FORWARD:
            return "FORWARD";
        case UCG_PLAN_ACTION_TYPE_FENCE:
            return "FENCE";
        default:
            return "UNKOWN";
    }
}

void ucg_plan_action_print(ucg_plan_action_t *action, FILE *stream)
{
    ucg_plan_action_core_t *core = action->core;
    ucg_rank_t *peers = core->peers;
    fprintf(stream, "#\n");
    fprintf(stream, "Type: %s, peer count: %d\n", ucg_plan_action_type_str(core->type), core->count);
    for (int i = 0; i < core->count; ++i) {
        ucg_plan_action_datav_t *datav = &action->datav[i];
        fprintf(stream, "\t#peer[%d] rank:%d, mh: %lu\n", i, peers[i], action->mh[peers[i]]);
        for (int j = 0; j < datav->count; ++j) {
            fprintf(stream, "\t\t#%d data.first: %p, data.second: %p, data.length: %lu\n", 
                    j, datav->data[j].first, datav->data[j].second, datav->data[j].length);
        }
    }
    fprintf(stream, "#\n");
    return;
}

UCG_RTE_INNER_DEFINE(UCG_RTE_RESOURCE_TYPE_ACTION, ucg_plan_action_global_init, 
                     ucg_plan_action_global_cleanup)