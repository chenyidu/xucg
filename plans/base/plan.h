/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_PLAN_H_
#define UCG_PLAN_H_

#include "action.h"
#include <ucg/plans/ucg_ppool.h>
#include <ucs/sys/preprocessor.h>

/* Replaced by incoming message. */
#define UCG_BUFFER_MSG_HOLDER ((uint8_t*)1)

/* Assertions which are checked in compile-time. 
 * TODO: move to appropriate file. 
 */
#define UCG_STATIC_ASSERT(_cond) typedef int UCS_PP_APPEND_UNIQUE_ID(assert)[(_cond)?1:-1]

/* The ID after 10000 is reserved for x plan. */
#define UCG_PLAN_ID_MAX 10000

/**
 * @ingroup UCG_PLAN
 * @brief Flags of plan's capabilities.
 */
typedef enum ucg_plan_cap_flag {
    UCG_PLAN_CAP_FLAG_TOPO_AWARE = UCS_BIT(0), /* Topo-aware plan. */
    UCG_PLAN_CAP_FLAG_SWITCH_ROOT = UCS_BIT(1), /* Support swicthing root. */
} ucg_plan_cap_flag_t;

/**
 * @ingroup UCG_PLAN
 * @brief The result of the comparison between the two plan parameters. 
 * 
 * Advice comes from 
 * 1. User parameter comparison,
 * 2. Configuration comparison,
 * 3. Agreed rules
 */
typedef enum ucg_plan_clone_advice {
    UCG_PLAN_CLONE_ADVICE_SAME, /* Everything is same, may reuse the origin plan directly. */
    UCG_PLAN_CLONE_ADVICE_SIMILAR, /* Non-critical parts are different. */
    UCG_PLAN_CLONE_ADVICE_UNEQUAL, /* Critical parts are different, not clone but creating a new one. */
} ucg_plan_clone_advice_t;

typedef struct ucg_plan ucg_plan_t;

/**
 * @ingroup UCG_PLAN
 * @brief Clone a plan object.
 *
 * Follow the COW(Copy On Write) principle, reuse the original plan for the
 * part not affected by parameter changes.
 * 
 * @param [in] plan The original plan used for cloning.
 * @param [in] params Parameters for creating a plan.
 * @param [in] advise Advice on how to clone.
 */
typedef ucg_plan_t* (*ucg_plan_clone_cb_t)(ucg_plan_t *plan,
                                           ucg_plan_params_t *params,
                                           ucg_plan_clone_advice_t advice);

/**
 * @ingroup UCG_PLAN
 * @brief Destroy a plan returned by clone()
 * 
 * @note clone() and destroy() need to be used in pairs.
 */
typedef void (*ucg_plan_destroy_cb_t)(ucg_plan_t *plan);

/**
 * @ingroup UCG_PLAN
 * @brief Cleanup a plan.
 */
typedef void (*ucg_plan_cleanup_cb_t)(ucg_plan_t *plan);

/**
 * @ingroup UCG_PLAN
 * @brief IDs of broadcast plans.
 */
typedef enum ucg_plan_bcast_id {
    UCG_PLAN_BCAST_ID_BTREE, /* binomial tree */
    UCG_PLAN_BCAST_ID_KTREE, /* knomial tree */
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
 * @brief The unchangeable part of a plan.
 */
typedef struct ucg_plan_core {
    ucg_plan_type_t type;
    int id;
    const char *desc;
    uint64_t flags;
    ucs_config_global_list_entry_t config_entry;

    ucg_plan_clone_cb_t clone;
    ucg_plan_destroy_cb_t destroy;
} ucg_plan_core_t;

/**
 * @ingroup UCG_PLAN
 * @brief Collective operation execution plan.
 */
typedef struct ucg_plan {
    ucg_plan_core_t *core;
    int refcount;

    struct {
        ucg_dt_state_t *pack_state;
        ucg_dt_state_t *unpack_state;
    } dt;

    ucg_plan_params_t params;
    ucs_list_link_t action_list;
} ucg_plan_t;

/**
 * @ingroup UCG_PLAN
 * @brief Base structure of broadcast plan.
 */
typedef struct ucg_plan_bcast {
    ucg_plan_t super;
    ucg_plan_bcast_params_t params;
} ucg_plan_bcast_t;

/**
 * @ingroup UCG_PLAN
 * @brief Base structure of allreduce plan.
 */
typedef struct ucg_plan_allreduce {
    ucg_plan_t super;
    ucg_plan_allreduce_params_t params;
} ucg_plan_allreduce_t;

/**
 * @ingroup UCG_PLAN
 * @brief Allocate a plan.
 * 
 * @param [in] size Size of the plan space.
 */
ucg_plan_t* ucg_plan_allocate(uint32_t size);

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
 * @brief Obtain plan and increase refcount.
 */
static inline ucg_plan_t* ucg_plan_obtain(ucg_plan_t *plan)
{
    plan->refcount++;
    return plan;
}

/**
 * @ingroup UCG_PLAN
 * @brief Release actions in the plan.
 */
void ucg_plan_release_actions(ucg_plan_t *plan);

/**
 * @ingroup UCG_PLAN
 * @brief Cleanup all resource of the plan.
 */
void ucg_plan_cleanup(ucg_plan_t *plan);

/**
 * @ingroup UCG_PLAN
 * @brief Clone parameters (deep copy).
 */
ucs_status_t ucg_plan_clone_params(ucg_plan_t *plan, ucg_plan_params_t *params);

ucs_status_t ucg_plan_create_and_append_action(ucg_plan_t *plan, 
                                               ucg_plan_action_type_t type, 
                                               ucg_rank_t *peers, int count);

static inline void ucg_plan_append_action(ucg_plan_t *plan, 
                                          ucg_plan_action_t *action)
{
    ucs_list_add_tail(&plan->action_list, &action->list);
    return;
};

/**
 * @ingroup UCG_PLAN
 * @brief Clone a plan.
 */
static inline ucg_plan_t* ucg_plan_clone(ucg_plan_t *plan, 
                                         ucg_plan_params_t *params,
                                         ucg_plan_clone_advice_t advice)
{
    return plan->core->clone(plan, params, advice);
}

/**
 * @ingroup UCG_PLAN
 * @brief Destroy a plan.
 */
static inline void ucg_plan_destroy(ucg_plan_t *plan)
{
    return plan->core->destroy(plan);
}

#endif