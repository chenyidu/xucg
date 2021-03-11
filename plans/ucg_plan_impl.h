/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_PLAN_IMPL_H_
#define UCG_PLAN_IMPL_H_

#include <ucg/plans/ucg_plan.h>
#include <ucs/type/status.h>
#include <ucs/sys/preprocessor.h>
#include <ucs/datastruct/khash.h>
#include <ucs/datastruct/list.h>

#include <stdint.h>

/* Replaced by incoming message. */
#define UCG_BUFFER_MSG_HOLDER ((uint8_t*)1)

/* Assertions which are checked in compile-time. 
 * TODO: move to appropriate file. 
 */
#define UCG_STATIC_ASSERT(_cond) typedef int UCS_PP_APPEND_UNIQUE_ID(assert)[(_cond)?1:-1]

/* The ID after 10000 is reserved for x plan. */
#define UCG_PLAN_ID_MAX 10000

/* Register a plan */
#define UCG_PLAN_REGISTER(_plan)\
    UCS_STATIC_INIT { \
        ucg_plan_register(&_plan); \
    }

/**
 * @ingroup UCG_PLAN
 * @brief The result of the comparison between the two plan parameters. 
 */
typedef enum ucg_plan_params_cmp {
    UCG_PLAN_PARAMS_CMP_SAME, /* plan-level reuse. */
    UCG_PLAN_PARAMS_CMP_SIMILAR, /* phase-core-level reuse. */
    UCG_PLAN_PARAMS_CMP_UNEQUAL, /* plan-core-level reuse. */
} ucg_plan_params_cmp_t;

/**
 * @ingroup UCG_PLAN
 * @brief IDs of broadcast plans.
 */
typedef enum ucg_plan_bcast_id {
    UCG_PLAN_BCAST_ID_BTREE, /* binomial tree */
    UCG_PLAN_BCAST_ID_MAX,
} ucg_plan_bcast_id_t;
UCG_STATIC_ASSERT(UCG_PLAN_BCAST_ID_MAX <= UCG_PLAN_ID_MAX);

/**
 * @ingroup UCG_PLAN
 * @brief IDs of allredece plans.
 */
typedef enum ucg_plan_allreduce_id {
    UCG_PLAN_ALLREDUCE_ID_RD,  /* recursive doubling */
    UCG_PLAN_ALLREDUCE_ID_MAX,
} ucg_plan_allreduce_id_t;
UCG_STATIC_ASSERT(UCG_PLAN_ALLREDUCE_ID_MAX <= UCG_PLAN_ID_MAX);

/**
 * @ingroup UCG_PLAN
 * @brief Base structure of broadcast plan.
 */
typedef struct ucg_plan_bcast {
    ucg_plan_t super;
    ucg_plan_bcast_params_t params;
} ucg_plan_bcast_t;

typedef struct ucg_plan_lru_node {
    ucg_plan_t *plan;
    int used_cnt;
    ucs_list_link_t global_list;
    ucs_list_link_t type_list;
} ucg_plan_lru_node_t;

typedef struct ucg_plan_lru_cache {
    int max_num;
    ucs_list_link_t global_list;
    ucs_list_link_t type_list[UCG_PLAN_TYPE_MAX];
} ucg_plan_lru_cache_t;

KHASH_MAP_INIT_INT64(ucg_ppool, ucg_plan_t*);
/**
 * @ingroup UCG_PLAN
 * @brief Plan pool.
 */
typedef struct ucg_ppool {
    int inited;
    khash_t(ucg_ppool) plans; /* registered plans */
    ucg_plan_lru_cache_t caches;
} ucg_ppool_t;

/**
 * @ingroup UCG_PLAN
 * @brief Register a plan.
 */
void ucg_plan_register(ucg_plan_t *plan);

/**
 * @ingroup UCG_PLAN
 * @brief Cleanup a plan.
 */
typedef void (*ucg_plan_cleanup_cb_t)(ucg_plan_t *plan);

/**
 * @ingroup UCG_PLAN
 * @brief Allocate a plan space.
 * 
 * @param [in] size Size of the plan space.
 */
ucg_plan_t* ucg_plan_allocate(uint32_t size);

/**
 * @ingroup UCG_PLAN
 * @brief Increase plan refcount.
 */
static inline ucg_plan_t* ucg_plan_obtain(ucg_plan_t *plan)
{
    plan->refcount++;
    return plan;
}

/**
 * @ingroup UCG_PLAN
 * @brief Release a plan.
 * 
 * This routine decreases the refcount of plan, when the refcount becomes 0, 
 * the cleanup is performed.
 */
void ucg_plan_release(ucg_plan_t *plan, ucg_plan_cleanup_cb_t cleanup);

/**
 * @ingroup UCG_PLAN
 * @brief Cleanup a phase.
 */
typedef void (*ucg_plan_phase_cleanup_cb_t)(ucg_plan_phase_t *phase);

/**
 * @ingroup UCG_PLAN
 * @brief Allocate a phase space.
 * 
 * @param [in] with_core Obtain phase core at the same time or not.
 */
ucg_plan_phase_t* ucg_plan_phase_allocate(int with_core);

/**
 * @ingroup UCG_PLAN
 * @brief Increase phase core refcount.
 */
static inline ucg_plan_phase_core_t* ucg_plan_phase_obtain_core(ucg_plan_phase_t* phase)
{
    phase->core->refcount++;
    return phase->core;
}

/**
 * @ingroup UCG_PLAN
 * @brief Release a phase object.
 */
void ucg_plan_phase_release(ucg_plan_phase_t* phase, ucg_plan_phase_cleanup_cb_t cleanup);

/**
 * @ingroup UCG_PLAN
 * @brief Clone a plan.
 */
static inline ucg_plan_t* ucg_plan_clone(ucg_plan_t *plan, 
                                         ucg_plan_params_t *params,
                                         ucg_plan_params_cmp_t cmp)
{
    return plan->core->clone(plan, params, cmp);
}

/**
 * @ingroup UCG_PLAN
 * @brief Release a plan.
 */
static inline void ucg_plan_destroy(ucg_plan_t *plan)
{
    return plan->core->destroy(plan);
}

#endif
