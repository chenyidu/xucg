#include "action.h"
#include <ucs/datastruct/mpool.h>
#include <ucs/arch/cpu.h>
#include <ucs/debug/log.h>
#include <ucs/debug/memtrack.h>

#include <limits.h>

typedef struct ucg_plan_action_mgr {
    int inited;
    ucs_mpool_t action_mp;
    ucs_mpool_t action_core_mp;
} ucg_plan_action_mgr_t;
static ucg_plan_action_mgr_t g_action_mgr = {
    .inited = 0,
};

static ucs_mpool_ops_t g_plan_action_mp_ops = {
    .chunk_alloc = ucs_mpool_chunk_malloc,
    .chunk_release = ucs_mpool_chunk_free,
    .obj_init = NULL,
    .obj_cleanup = NULL,
};

static ucs_mpool_ops_t g_plan_action_mp_core_ops = {
    .chunk_alloc = ucs_mpool_chunk_malloc,
    .chunk_release = ucs_mpool_chunk_free,
    .obj_init = NULL,
    .obj_cleanup = NULL,
};

ucg_plan_action_t* ucg_plan_action_allocate(int with_core)
{
    ucs_assert(g_action_mgr.inited);
    ucg_plan_action_t *action = (ucg_plan_action_t *)ucs_mpool_get(&g_action_mgr.action_mp);
    if (action == NULL) {
        return NULL;
    }
    if (!with_core) {
        return action;
    }
    
    action->core = (ucg_plan_action_core_t *)ucs_mpool_get(&g_action_mgr.action_core_mp);
    if (action->core == NULL) {
        ucs_free(action);
        return NULL;
    }
    action->core->refcount = 1;
    return action;
}

void ucg_plan_action_release(ucg_plan_action_t *action)
{
    ucs_assert(action != NULL && action->core->refcount > 0);
    if (--action->core->refcount == 0) {
        ucs_mpool_put(action->core);
    }
    ucs_mpool_put(action);
    return;
}

ucs_status_t ucg_plan_action_global_init()
{
    if (g_action_mgr.inited) {
        return UCS_OK;
    }

    ucs_status_t status = UCS_OK;
    status = ucs_mpool_init(&g_action_mgr.action_mp, 0, sizeof(ucg_plan_action_t), 0, 
                            UCS_SYS_CACHE_LINE_SIZE, 128, UINT_MAX, 
                            &g_plan_action_mp_ops, "plan aciton");
    if (status != UCS_OK) {
        ucs_error("Fail to init action mpool.");
        return status;
    }

    status = ucs_mpool_init(&g_action_mgr.action_core_mp, 0, sizeof(ucg_plan_action_core_t), 0, 
                            UCS_SYS_CACHE_LINE_SIZE, 128, UINT_MAX, 
                            &g_plan_action_mp_core_ops, "plan aciton core");
    if (status != UCS_OK) {
        ucs_error("Fail to init action core mpool.");
        return status;
    }
    
    return status;
}

void ucg_plan_action_global_cleanup()
{
    ucs_mpool_cleanup(&g_action_mgr.action_core_mp, 1);
    ucs_mpool_cleanup(&g_action_mgr.action_mp, 1);
    return;
}