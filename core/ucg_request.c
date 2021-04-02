#include "ucg_request.h"
#include "ucg_util.h"
#include "ucg_rte.h"

#include <ucs/datastruct/mpool.h>
#include <ucs/arch/cpu.h>

#include <limits.h>

typedef struct ucg_request_mgr {
    UCG_THREAD_SAFE_DECLARE(lock); /* Protect get/put on mp. */
    ucs_mpool_t mp;
    ucs_mpool_ops_t mp_ops;
} ucg_request_mgr_t;

static ucg_request_mgr_t g_req_mgr = {
    .mp_ops = {
        .chunk_alloc = ucs_mpool_hugetlb_malloc,
        .chunk_release = ucs_mpool_hugetlb_free,
        .obj_init = ucs_empty_function,
        .obj_cleanup = ucs_empty_function,
    }
};

static ucs_status_t ucg_request_global_init()
{
    ucs_status_t status;
    status = UCG_THREAD_SAFE_INIT(&g_req_mgr.lock);
    if (status != UCS_OK) {
        ucs_error("Failed to init thread safe.");
        goto err;
    }

    status = ucs_mpool_init(&g_req_mgr.mp, 0, sizeof(ucg_request_t), 
                            0, UCS_SYS_CACHE_LINE_SIZE, 128,
                            UINT_MAX, &g_req_mgr.mp_ops, "ucg request");
    if (status != UCS_OK) {
        ucs_error("Failed to init request mpool, %s", ucs_status_string(status));
        goto err_destroy_thread_safe;
    }

    return UCS_OK;
err_destroy_thread_safe:
    UCG_THREAD_SAFE_DESTROY(&g_req_mgr.lock);
err:
    return status;
}

static void ucg_request_global_cleanup()
{
    ucs_mpool_cleanup(&g_req_mgr.mp, 1);
    UCG_THREAD_SAFE_DESTROY(&g_req_mgr.lock);
    return;
}

static ucs_status_t ucg_request_init(ucg_group_t *group, 
                                     ucg_plan_params_t *params, 
                                     ucg_request_h *request)
{
    ucs_status_t status = UCS_OK;
    UCG_THREAD_SAFE_ENTER(&g_req_mgr.lock);
    ucg_request_t *req = (ucg_request_t*)ucs_mpool_get(&g_req_mgr.mp);
    UCG_THREAD_SAFE_LEAVE(&g_req_mgr.lock);
    if (req == NULL) {
        status = UCS_ERR_NO_MEMORY;
        goto err;
    }

    ucg_group_members_share(group, &params->members);
    ucg_plan_t *plan = ucg_group_select_plan(group, params);
    if (plan == NULL) {
        ucs_error("Failed to select plan for %s.", ucg_plan_type_str(params->type));
        status = UCS_ERR_NO_RESOURCE;
        goto err_free_req; ;
    }
    req->plan = plan;
    req->is_barrier = params->type == UCG_PLAN_TYPE_BARRIER;
    req->handle = 0;
    req->state = UCG_REQUEST_STATE_INITED;

    return UCS_OK;
err_free_req:
    UCG_THREAD_SAFE_ENTER(&g_req_mgr.lock);
    ucs_mpool_put(req);
    UCG_THREAD_SAFE_LEAVE(&g_req_mgr.lock);
err:
    return status;
}

ucs_status_t ucg_request_bcast_init(ucg_group_h group,
                                    void *buffer, 
                                    int count, 
                                    ucg_datatype_t *dtype, 
                                    ucg_rank_t root,
                                    ucg_request_h *request)
{
    UCG_CHECK_PARAMS(group != NULL && buffer != NULL && dtype != NULL && request != NULL);
    ucg_plan_bcast_params_t params = {
        .super = {
            .type = UCG_PLAN_TYPE_BCAST,
        },
        .buffer = buffer,
        .count = count,
        .dtype = dtype,
        .root = root,
    };
    return ucg_request_init(group, &params.super, request);
}

ucs_status_t ucg_request_allreduce_init(ucg_group_h group,
                                        const void *sendbuf, 
                                        void *recvbuf, 
                                        int count, 
                                        ucg_datatype_t *dtype, 
                                        ucg_op_t *op, 
                                        ucg_request_h *request)
{
    UCG_CHECK_PARAMS(group != NULL && sendbuf != NULL && recvbuf != NULL 
                     && dtype != NULL && op != NULL && request != NULL);
    ucg_plan_allreduce_params_t params = {
        .super = {
            .type = UCG_PLAN_TYPE_ALLREDUCE,
        },
        .sendbuf = sendbuf,
        .recvbuf = recvbuf,
        .count = count,
        .dtype = dtype,
        .op = op,
    };
    return ucg_request_init(group, &params.super, request);
}

ucs_status_t ucg_request_barrier_init(ucg_group_h group, ucg_request_h *request)
{
    UCG_CHECK_PARAMS(group != NULL && request != NULL);
    ucg_plan_barrier_params_t params = {
        .super = {
            .type = UCG_PLAN_TYPE_BARRIER,
        },
    };
    return ucg_request_init(group, &params.super, request);
}

ucs_status_t ucg_request_start(ucg_request_h request)
{
    /* A non-outstanding request can start many times. */
    UCG_CHECK_PARAMS(request != NULL && request->state != UCG_REQUEST_STATE_OUTSTANDING);

    uint8_t is_need_barrier = request->is_barrier ? UCG_GROUP_BARRIER : UCG_GROUP_NBARRIER;
    /* If group is already in barrier state, all later requests need to be suspended. */
    if (UCG_GROUP_BARRIER == ucg_group_barrier_cswap(request->group, UCG_GROUP_NBARRIER, is_need_barrier)) {
        request->state = UCG_REQUEST_STATE_PENDING;
        return UCS_INPROGRESS;
    }

    request->handle = ucg_plan_open(request->plan, ucg_group_id(request->group), 
                                    ucg_group_next_req_id(request->group));
    if (request->handle == 0) {
        ucs_error("Failed to open plan.");
        return UCS_ERR_IO_ERROR;
    }

    ucs_status_t status = ucg_plan_execute(request->handle);
    if (status != UCS_INPROGRESS) {
        request->state = UCG_REQUEST_STATE_COMPLETED;
        request->comp_status = status;
        ucg_plan_close(request->handle);
    } else {
        request->state = UCG_REQUEST_STATE_OUTSTANDING;
    }
    
    return status;
}

int ucg_request_progress(ucg_request_h request)
{
    UCG_CHECK_PARAMS_ZERO(request != NULL);

    if (request->state == UCG_REQUEST_STATE_PENDING) {
        /* Try to start again. */
        (void)ucg_request_start(request);
        return 1;
    }

    if (request->state == UCG_REQUEST_STATE_OUTSTANDING) {
        ucs_status_t status = ucg_plan_execute(request->handle);
        if (status != UCS_INPROGRESS) {
            request->state = UCG_REQUEST_STATE_COMPLETED;
            request->comp_status = status;
            ucg_plan_close(request->handle);
        }
        return 1;
    }

    return 0;
}

ucs_status_t ucg_request_check_status(ucg_request_h request)
{
    UCG_CHECK_PARAMS(request != NULL);

    if (request->state == UCG_REQUEST_STATE_COMPLETED) {
        return request->comp_status;
    }

    if (request->state == UCG_REQUEST_STATE_CANCELED) {
        return UCS_ERR_CANCELED;
    }
    
    return UCS_INPROGRESS;
}

void ucg_request_cancel(ucg_request_h request)
{
    UCG_CHECK_PARAMS_VOID(request != NULL);

    /* Only pending request can be canceled. A outstanding request has many 
     * underlying operations, it's hard to cancel correctly. */
    if (request->state == UCG_REQUEST_STATE_PENDING) {
        request->state = UCG_REQUEST_STATE_CANCELED;
    }

    return;
}

void ucg_request_free(ucg_request_h request)
{
    UCG_CHECK_PARAMS_VOID(request != NULL);
    ucs_assert(request->state == UCG_REQUEST_STATE_COMPLETED || request->state == UCG_REQUEST_STATE_CANCELED);

    if (request->handle > 0) {
        ucg_plan_close(request->handle);
    }
    ucg_group_release_plan(request->group, request->plan);
    UCG_THREAD_SAFE_ENTER(&g_req_mgr.lock);
    ucs_mpool_put(request);
    UCG_THREAD_SAFE_LEAVE(&g_req_mgr.lock);

    return;
}

UCG_RTE_INNER_DEFINE(UCG_RTE_RESOURCE_TYPE_REQUEST, ucg_request_global_init,
                     ucg_request_global_cleanup);