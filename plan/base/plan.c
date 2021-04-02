#include "plan.h"

#include <ucg/core/ucg_util.h>
#include <ucg/core/ucg_rte.h>
#include <ucs/debug/log.h>
#include <ucs/datastruct/khash.h>
#include <ucs/debug/assert.h>
#include <ucs/debug/memtrack.h>
#include <ucs/datastruct/mpool.h>
#include <ucs/arch/cpu.h>
#include <ucs/type/spinlock.h>

#include <string.h>

/*****************************************************************************
 *                   Part related to plan params
 *****************************************************************************/
#define UCG_PLAN_CLONE_BASIC_PARAMS(_derived_plan_type, _derived_params_type) \
    _derived_plan_type *plan = ucs_derived_of(plan_p, _derived_plan_type); \
    _derived_params_type *dst = &plan->params; \
    _derived_params_type *src = ucs_derived_of(src_p, _derived_params_type); \
    ucs_status_t status = ucg_plan_basic_params_clone(&src->super, &dst->super); \
    if (status != UCS_OK) { \
        return status; \
    } 

#define UCG_PLAN_FREE_BASIC_PARAMS(_derived_plan_type, _derived_params_type) \
    _derived_plan_type *plan = ucs_derived_of(plan_p, _derived_plan_type); \
    ucg_plan_params_t *params = &plan->params.super; \
    ucg_plan_basic_params_free(params);

typedef ucs_status_t (*ucg_plan_clone_params_func_t)(ucg_plan_t *plan, 
                                                     const ucg_plan_params_t *params);

typedef void (*ucg_plan_free_params_func_t)(ucg_plan_t *plan);

typedef struct ucg_plan_params_method {
    ucg_plan_clone_params_func_t clone;
    ucg_plan_free_params_func_t free;
} ucg_plan_params_method_t;

static ucs_status_t ucg_plan_basic_params_clone(const ucg_plan_params_t *src, 
                                                ucg_plan_params_t *dst)
{
    dst->type = src->type;
    dst->id = src->id;
    return ucg_group_members_clone(&src->members, &dst->members);
}

void ucg_plan_basic_params_free(ucg_plan_params_t *params)
{
    ucg_group_members_free(&params->members);
    return;
}

static ucs_status_t ucg_plan_bcast_clone_params(ucg_plan_t *plan_p, 
                                                const ucg_plan_params_t *src_p)
{
    UCG_PLAN_CLONE_BASIC_PARAMS(ucg_plan_bcast_t, ucg_plan_bcast_params_t);
    dst->buffer = src->buffer;
    dst->count = src->count;
    dst->dtype = src->dtype;
    dst->root = src->root;
    return UCS_OK;
}

static void ucg_plan_bcast_free_params(ucg_plan_t *plan_p)
{
    UCG_PLAN_FREE_BASIC_PARAMS(ucg_plan_bcast_t, ucg_plan_bcast_params_t);
    return;
}

static ucs_status_t ucg_plan_allreduce_clone_params(ucg_plan_t *plan_p, 
                                                    const ucg_plan_params_t *src_p)
{
    UCG_PLAN_CLONE_BASIC_PARAMS(ucg_plan_allreduce_t, ucg_plan_allreduce_params_t);
    dst->sendbuf = src->sendbuf;
    dst->recvbuf = src->recvbuf;
    dst->op = src->op;
    dst->count = src->count;
    dst->dtype = src->dtype;
    return UCS_OK;
}

static void ucg_plan_allreduce_free_params(ucg_plan_t *plan_p)
{
    UCG_PLAN_FREE_BASIC_PARAMS(ucg_plan_allreduce_t, ucg_plan_allreduce_params_t);
    return;
}

static ucs_status_t ucg_plan_barrier_clone_params(ucg_plan_t *plan_p, 
                                                  const ucg_plan_params_t *src_p)
{
    UCG_PLAN_CLONE_BASIC_PARAMS(ucg_plan_barrier_t, ucg_plan_barrier_params_t);
    return UCS_OK;
}

static void ucg_plan_barrier_free_params(ucg_plan_t *plan_p)
{
    UCG_PLAN_FREE_BASIC_PARAMS(ucg_plan_barrier_t, ucg_plan_barrier_params_t);
    return;
}

static ucg_plan_params_method_t g_params_methods[] = {
    [UCG_PLAN_TYPE_BCAST] = {
        .clone = ucg_plan_bcast_clone_params,
        .free = ucg_plan_bcast_free_params,
    },
    [UCG_PLAN_TYPE_ALLREDUCE] = {
        .clone = ucg_plan_allreduce_clone_params,
        .free = ucg_plan_allreduce_free_params,
    },
    [UCG_PLAN_TYPE_BARRIER] = {
        .clone = ucg_plan_barrier_clone_params,
        .free = ucg_plan_barrier_free_params,
    },
};

static ucs_status_t ucg_plan_clone_params(ucg_plan_t *plan, const ucg_plan_params_t *params)
{
    ucg_plan_type_t type = plan->core->id.type;
    ucs_assert(type < sizeof(g_params_methods) / sizeof(ucg_plan_params_method_t));
    return g_params_methods[type].clone(plan, params);
}

static void ucg_plan_free_params(ucg_plan_t *plan)
{
    ucg_plan_type_t type = plan->core->id.type;
    ucs_assert(type < sizeof(g_params_methods) / sizeof(ucg_plan_params_method_t));
    return g_params_methods[type].free(plan);
}

/*****************************************************************************
 *                    Part related to plan action
 *****************************************************************************/
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

/*****************************************************************************
 *                    Part related to non-contig datatype
 *****************************************************************************/
static ucs_status_t ucg_plan_empty_prepare_dt_state(ucg_plan_t *plan)
{
    return UCS_OK;
}

static ucs_status_t ucg_plan_bcast_prepare_dt_state(ucg_plan_t *plan)
{
    ucg_plan_bcast_t *bcast_plan = ucs_derived_of(plan, ucg_plan_bcast_t);
    ucg_plan_bcast_params_t *params = &bcast_plan->params;
    if (ucg_dt_is_contig(params->dtype)) {
        return UCS_OK;
    }

    ucs_status_t status;
    if ( params->super.members.self == params->root) {
        // I'm root, I need to pack the data to sendbuf.
        plan->dt.pack_state = ucg_dt_pack_state(params->dtype, params->buffer, params->count);
        status = plan->dt.pack_state != NULL ? UCS_OK : UCS_ERR_NO_RESOURCE;
    } else {
        // I'm not root, I need to unpack the incoming data to recvbuf.
        plan->dt.unpack_state = ucg_dt_unpack_state(params->dtype, params->buffer, params->count);
        status = plan->dt.unpack_state != NULL ? UCS_OK : UCS_ERR_NO_RESOURCE;
    }

    return status;
}

static ucs_status_t ucg_plan_allreduce_prepare_dt_state(ucg_plan_t *plan)
{
    ucg_plan_allreduce_t *allreduce_plan = ucs_derived_of(plan, ucg_plan_allreduce_t);
    ucg_plan_allreduce_params_t *params = &allreduce_plan->params;
    if (ucg_dt_is_contig(params->dtype)) {
        return UCS_OK;
    }
    plan->dt.pack_state = ucg_dt_pack_state(params->dtype, params->sendbuf, params->count);
    if (plan->dt.pack_state == NULL) {
        return UCS_ERR_NO_RESOURCE;
    }
    plan->dt.unpack_state = ucg_dt_unpack_state(params->dtype, params->recvbuf, params->count);
    if (plan->dt.unpack_state == NULL) {
        ucg_dt_state_finish(plan->dt.pack_state);
        return UCS_ERR_NO_RESOURCE;
    }
    return UCS_OK;
}

typedef ucs_status_t (*ucg_plan_prepare_dt_state_func_t)(ucg_plan_t *plan);
static ucg_plan_prepare_dt_state_func_t g_dt_state_prepare[] = {
    [UCG_PLAN_TYPE_BCAST] = ucg_plan_bcast_prepare_dt_state,
    [UCG_PLAN_TYPE_ALLREDUCE] = ucg_plan_allreduce_prepare_dt_state,
    [UCG_PLAN_TYPE_BARRIER] = ucg_plan_empty_prepare_dt_state,
};
static ucs_status_t ucg_plan_prepare_dt_state(ucg_plan_t *plan)
{
    ucg_plan_type_t type = plan->core->id.type;
    ucs_assert(type < sizeof(g_dt_state_prepare) / sizeof(ucg_plan_prepare_dt_state_func_t));
    plan->dt.pack_state = NULL;
    plan->dt.unpack_state = NULL;
    g_dt_state_prepare[type](plan);
    return UCS_OK;
}

static void ucg_plan_release_dt_state(ucg_plan_t *plan)
{
    if (plan->dt.pack_state != NULL) {
        ucg_dt_state_finish(plan->dt.pack_state);
        plan->dt.pack_state = NULL;
    }

    if (plan->dt.unpack_state != NULL) {
        ucg_dt_state_finish(plan->dt.unpack_state);
        plan->dt.unpack_state = NULL;
    }

    return;
}
/*****************************************************************************
 *                    Part related to plan
 *****************************************************************************/
KHASH_MAP_INIT_INT64(plan, ucg_plan_t*);
typedef struct ucg_plan_mgr {
    UCG_THREAD_SAFE_DECLARE(lock); /* Protect get/put on plan_mp, other fields is readonly. */
    khash_t(plan) plan_hash;
    uint32_t max_plan_size; 
    ucs_mpool_t plan_mp;
    ucs_mpool_ops_t mp_ops;
} ucg_plan_mgr_t;

static ucg_plan_mgr_t g_plan_mgr = {
    .mp_ops = {
        .chunk_alloc = ucs_mpool_hugetlb_malloc,
        .chunk_release = ucs_mpool_hugetlb_free,
        .obj_init = ucs_empty_function,
        .obj_cleanup = ucs_empty_function,
    },
};

static ucs_status_t ucg_plan_global_init()
{
    ucs_status_t status;
    status = UCG_THREAD_SAFE_INIT(&g_plan_mgr.lock);
    if (status != UCS_OK) {
        ucs_error("Failed to init thread safe.");
        goto err;
    }

    status = ucs_mpool_init(&g_plan_mgr.plan_mp, 0, g_plan_mgr.max_plan_size, 0, UCS_SYS_CACHE_LINE_SIZE, 
                            128, UINT_MAX, &g_plan_mgr.mp_ops, "ucg plan");
    if (status != UCS_OK) {
        ucs_error("Failed to init plan mpool, %s", ucs_status_string(status));
        goto err_destroy_thread_safe;
    }

    return UCS_OK;
err_destroy_thread_safe:
    UCG_THREAD_SAFE_DESTROY(&g_plan_mgr.lock);
err:
    return status;
}

static void ucg_plan_global_cleanup()
{
    ucs_mpool_cleanup(&g_plan_mgr.plan_mp, 1);
    UCG_THREAD_SAFE_DESTROY(&g_plan_mgr.lock);
    return;
}

static ucs_status_t ucg_plan_constructor(ucg_plan_t *self, ucg_plan_t *other, 
                                         const ucg_config_t *config)
{
    return self->core->constructor(self, other, config);
}

static void ucg_plan_destructor(ucg_plan_t *self)
{
    return self->core->destructor(self);
}

static uint64_t ucg_plan_id_pack(const ucg_plan_id_t *id)
{
    return (uint64_t)id->type << 32 | id->algo;
}

void ucg_plan_register(ucg_plan_t *plan)
{
    static int inited = 0;
    if (!inited) {
        kh_init_inplace(plan, &g_plan_mgr.plan_hash);
        inited = 1;
    }

    int ret = 0;
    khiter_t iter = kh_put(plan, &g_plan_mgr.plan_hash, 
                           ucg_plan_id_pack(&plan->core->id), &ret);
    ucs_assert(ret == 1);
    kh_value(&g_plan_mgr.plan_hash, iter) = plan;
    ucs_assert(plan->core->plan_size >= sizeof(ucg_plan_t));
    if (g_plan_mgr.max_plan_size < plan->core->plan_size) {
        g_plan_mgr.max_plan_size =  plan->core->plan_size;
    }
    return;
}

const char* ucg_plan_type_str(ucg_plan_type_t type)
{
    switch (type) {
        case UCG_PLAN_TYPE_BCAST:
            return "BCAST";
        case UCG_PLAN_TYPE_ALLREDUCE:
            return "ALLREDUCE";
        case UCG_PLAN_TYPE_BARRIER:
            return "BARRIER";
        default:
            return "UNKNOWN";
    }
}

ucg_plan_t* ucg_plan_clone(ucg_plan_t *plan, const ucg_config_t *config, 
                           const ucg_plan_params_t *params, 
                           const ucg_plan_clone_advice_t advice)
{
    if (advice == UCG_PLAN_CLONE_ADVICE_SAME) {
        plan->refcount++;
        return plan;
    }

    UCG_THREAD_SAFE_ENTER(&g_plan_mgr.lock);
    ucg_plan_t *self = (ucg_plan_t*)ucs_mpool_get(&g_plan_mgr.plan_mp);
    UCG_THREAD_SAFE_LEAVE(&g_plan_mgr.lock);
    if (self == NULL) {
        ucs_error("Failed to allocate plan.");
        goto err;
    }
    // Initialize super class object.
    self->core = plan->core;
    self->refcount = 1;
    ucs_list_head_init(&self->action_list);
    
    self->dt.pack_state = NULL;
    self->dt.unpack_state = NULL;
    // Save parameters to basic collective plan.
    ucs_status_t status = ucg_plan_clone_params(self, params);
    if (status != UCS_OK) {
        ucs_error("Failed to clone params, %s", ucs_status_string(status));
        goto err_put_plan;
    }

    status = ucg_plan_prepare_dt_state(self);
    if (status != UCS_OK) {
        ucs_error("Failed to prepare dt state, %s", ucs_status_string(status));
        goto err_free_params;
    }

    // Construct derived class object.
    ucs_assert(advice == UCG_PLAN_CLONE_ADVICE_SIMILAR || advice == UCG_PLAN_CLONE_ADVICE_UNEQUAL);
    ucg_plan_t *other = advice == UCG_PLAN_CLONE_ADVICE_SIMILAR ? plan : NULL;
    status = ucg_plan_constructor(self, other, config);
    if (status != UCS_OK) {
        goto err_free_dt_state;
    }
    return self;
err_free_dt_state:
    ucg_plan_release_dt_state(self);
err_free_params:
    ucg_plan_free_params(plan);
err_put_plan:
    UCG_THREAD_SAFE_ENTER(&g_plan_mgr.lock);
    ucs_mpool_put(self);
    UCG_THREAD_SAFE_LEAVE(&g_plan_mgr.lock);
err:
    return NULL;
}

ucg_plan_t* ucg_plan_create(const ucg_plan_id_t *id, const ucg_config_t *config, 
                            const ucg_plan_params_t *params)
{
    // Find plan template.
    khiter_t iter = kh_get(plan, &g_plan_mgr.plan_hash, ucg_plan_id_pack(id));
    if (iter == kh_end(&g_plan_mgr.plan_hash)) {
        ucs_error("Failed to find the plan(type:%s, algo:%d).", 
                    ucg_plan_type_str(id->type), id->algo);
        return NULL;
    }
    ucg_plan_t *plan = kh_value(&g_plan_mgr.plan_hash, iter);  
    return ucg_plan_clone(plan, config, params, UCG_PLAN_CLONE_ADVICE_UNEQUAL);
}

void ucg_plan_free(ucg_plan_t *plan)
{
    if (--plan->refcount > 0) {
        return;
    }

    ucg_plan_destructor(plan);
    ucg_plan_release_dt_state(plan);
    ucg_plan_free_params(plan);

    UCG_THREAD_SAFE_ENTER(&g_plan_mgr.lock);
    ucs_mpool_put(plan);
    UCG_THREAD_SAFE_LEAVE(&g_plan_mgr.lock);

    return;
}

ucg_plan_handle_t ucg_plan_open(ucg_plan_t *plan, ucg_group_id_t gid, uint32_t rid)
{
    ucg_plan_handle_t handle = 0;

    return handle;
}

ucs_status_t ucg_plan_execute(ucg_plan_handle_t handle)
{
    return UCS_INPROGRESS;
}

void ucg_plan_close(ucg_plan_handle_t handle)
{
    return;
}

void ucg_plan_print(ucg_plan_t *plan, FILE *stream)
{
    fprintf(stream, "#\n");
    fprintf(stream, "# %s plan based on algo %d\n", ucg_plan_type_str(plan->core->id.type), plan->core->id.algo);
    fprintf(stream, "#\n");
    ucg_plan_action_t *action;
    uint32_t idx = 0;
    ucs_list_for_each (action, &plan->action_list, list) {
        fprintf(stream, "# Action[%d]\n", idx++);
        ucg_plan_action_print(action, stream);
    }
    fprintf(stream, "\n");
    return;
}

ucg_plan_type_t ucg_plan_type(ucg_plan_t *plan)
{
    return plan->core->id.type;
}

UCG_RTE_INNER_DEFINE(UCG_RTE_RESOURCE_TYPE_PLAN, ucg_plan_global_init, 
                     ucg_plan_global_cleanup);
 