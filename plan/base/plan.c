#include "plan.h"

#include <ucs/debug/log.h>
#include <ucs/datastruct/khash.h>
#include <ucs/debug/assert.h>
#include <ucs/debug/memtrack.h>
#include <ucs/datastruct/mpool.h>
#include <ucs/arch/cpu.h>

#include <string.h>

#define UCG_PLAN_CLONE_BASIC_PARAMS(_derived_plan_type, _derived_params_type) \
    _derived_plan_type *plan = ucs_derived_of(plan_p, _derived_plan_type); \
    _derived_params_type *src = &plan->params; \
    _derived_params_type *dst = ucs_derived_of(dst_p, _derived_params_type); \
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
    dst->members.count = src->members.count;
    dst->members.self = src->members.self;
    int alloc_size = dst->members.count * sizeof(ucg_mh_t);
    ucg_mh_t *mh = ucs_malloc(alloc_size, "plan members");
    if (mh == NULL) {
        return UCS_ERR_NO_MEMORY;
    }
    memcpy(mh, src->members.mh, alloc_size);
    dst->members.mh = mh; 
    return UCS_OK;
}

void ucg_plan_basic_params_free(ucg_plan_params_t *params)
{
    if (params->members.mh != NULL) {
        ucs_free(params->members.mh);
        params->members.mh = NULL;
    }
    return;
}

static ucs_status_t ucg_plan_bcast_clone_params(ucg_plan_t *plan_p, 
                                                const ucg_plan_params_t *dst_p)
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
                                                    const ucg_plan_params_t *dst_p)
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
                                                  const ucg_plan_params_t *dst_p)
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

ucs_status_t ucg_plan_clone_params(ucg_plan_t *plan, const ucg_plan_params_t *params)
{
    ucg_plan_type_t type = plan->core->id.type;
    ucs_assert(type < sizeof(g_params_methods) / sizeof(ucg_plan_params_method_t));
    return g_params_methods[type].clone(plan, params);
}

void ucg_plan_free_params(ucg_plan_t *plan)
{
    ucg_plan_type_t type = plan->core->id.type;
    ucs_assert(type < sizeof(g_params_methods) / sizeof(ucg_plan_params_method_t));
    return g_params_methods[type].free(plan);
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

KHASH_MAP_INIT_INT64(plan, ucg_plan_t*);
static khash_t(plan) g_plan_hash;
static uint32_t g_max_plan_size;
static ucs_mpool_t g_plan_mp;
static ucs_mpool_ops_t g_plan_mp_ops = {
    .chunk_alloc = ucs_mpool_hugetlb_malloc,
    .chunk_release = ucs_mpool_hugetlb_free,
    .obj_init = ucs_empty_function,
    .obj_cleanup = ucs_empty_function,
};

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
        kh_init_inplace(plan, &g_plan_hash);
        inited = 1;
    }

    int ret = 0;
    khiter_t iter = kh_put(plan, &g_plan_hash, 
                           ucg_plan_id_pack(&plan->core->id), &ret);
    ucs_assert(ret == 1);
    kh_value(&g_plan_hash, iter) = plan;
    ucs_assert(plan->core->plan_size >= sizeof(ucg_plan_t));
    if (g_max_plan_size < plan->core->plan_size) {
        g_max_plan_size =  plan->core->plan_size;
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

    ucg_plan_t *self = (ucg_plan_t*)ucs_mpool_get(&g_plan_mp);
    if (self == NULL) {
        ucs_error("Failed to allocate plan.");
        return NULL;
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
        return NULL;
    }
    
    // Construct derived class object.
    ucs_assert(advice == UCG_PLAN_CLONE_ADVICE_SIMILAR || advice == UCG_PLAN_CLONE_ADVICE_UNEQUAL);
    ucg_plan_t *other = advice == UCG_PLAN_CLONE_ADVICE_SIMILAR ? plan : NULL;
    status = ucg_plan_constructor(self, other, config);
    if (status != UCS_OK) {
        ucs_mpool_put(self);
        return NULL;
    }
    return self;
}

ucg_plan_t* ucg_plan_create(const ucg_plan_id_t *id, const ucg_config_t *config, 
                            const ucg_plan_params_t *params)
{
    // Find plan template.
    khiter_t iter = kh_get(plan, &g_plan_hash, ucg_plan_id_pack(id));
    if (iter == kh_end(&g_plan_hash)) {
        ucs_error("Failed to find the plan(type:%s, algo:%d).", 
                    ucg_plan_type_str(id->type), id->algo);
        return NULL;
    }
    ucg_plan_t *plan = kh_value(&g_plan_hash, iter);  
    return ucg_plan_clone(plan, config, params, UCG_PLAN_CLONE_ADVICE_UNEQUAL);
}

void ucg_plan_free(ucg_plan_t *plan)
{
    if (--plan->refcount > 0) {
        return;
    }

    ucg_plan_destructor(plan);
    ucg_plan_free_params(plan);

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

ucs_status_t ucg_plan_global_init()
{
    ucs_status_t status;
    status = ucg_plan_action_global_init();
    if (status != UCS_OK) {
        ucs_error("Failed to init action, %s", ucs_status_string(status));
        return status;
    }

    status = ucs_mpool_init(&g_plan_mp, 0, g_max_plan_size, 0, UCS_SYS_CACHE_LINE_SIZE, 
                            128, UINT_MAX, &g_plan_mp_ops, "ucg plan mp");
    if (status != UCS_OK) {
        ucs_error("Failed to init plan mpool, %s", ucs_status_string(status));
        return status;
    }

    return UCS_OK;
}

void ucg_plan_global_cleanup()
{
    ucs_mpool_cleanup(&g_plan_mp, 1);
    ucg_plan_action_global_cleanup();
    return;
}