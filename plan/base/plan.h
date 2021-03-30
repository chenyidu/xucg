/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_PLAN_BASE_H_
#define UCG_PLAN_BASE_H_

#include "action.h"
#include <ucg/plan/api.h>
#include <ucg/api/ucg.h>
#include <ucg/core/ucg_datatype.h>
#include <ucs/sys/preprocessor.h>

/* Will be replaced by incoming message. */
#define UCG_BUFFER_MSG_HOLDER ((uint8_t*)1)

/**
 * @brief Register a plan
 * @param _name Plan name.
 * @param _plan_core_ptr Pointer to plan core.
*/
#define UCG_PLAN_REGISTER(_name, _plan_core_ptr)\
    static ucg_plan_t _name##_plan = { \
        .core = _plan_core_ptr, \
        .refcount = 1, /* Never release. */ \
    }; \
    UCS_STATIC_INIT { \
        ucg_plan_register(&_name##_plan); \
    }

typedef struct ucg_plan ucg_plan_t;
typedef struct ucg_plan_params ucg_plan_params_t;
/**
 * @ingroup UCG_PLAN
 * @brief Plan's constructor.
 *
 * If @b other is NULL, it's a normal constrcutor. Otherwise, it's a copy constrcutor. 
 * Constructor only needs to care about creating actions, and the rest have been 
 * set during super class initialization, such as parameters.
 * 
 * @param [in] self Plan object waiting to be constructed.
 * @param [in] other For copy constructor. 
 * @param [in] config Configuration for constructing the plan.
 * @param [in] params Parameters for constructing the plan.
 */
typedef ucs_status_t (*ucg_plan_constructor_func_t)(ucg_plan_t *self,
                                                    const ucg_plan_t *other,
                                                    const ucg_config_t *config);

/**
 * @ingroup UCG_PLAN
 * @brief Plan's destructor.
 * 
 * Destructor only needs to care about destroying actions created in constructor, 
 * and other resources allocated in constructor.
 */
typedef void (*ucg_plan_destructor_func_t)(ucg_plan_t *self);

/**
 * @ingroup UCG_PLAN
 * @brief The unchangeable part of a plan.
 */
typedef struct ucg_plan_core {
    ucg_plan_id_t id;
    const char *desc;
    uint64_t cap_flags; /**< see @ref ucg_plan_cap_flag_t. */

    uint32_t plan_size;
    ucg_plan_constructor_func_t constructor;
    ucg_plan_destructor_func_t destructor;
} ucg_plan_core_t;

/**
 * @ingroup UCG_PLAN
 * @brief Collective operation execution plan.
 */
typedef struct ucg_plan {
    ucg_plan_core_t *core;
    int refcount;
    ucs_list_link_t action_list;

    struct {
        ucg_dt_state_t *pack_state;
        ucg_dt_state_t *unpack_state;
    } dt;
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
 * @brief Base structure of barrier plan.
 */
typedef struct ucg_plan_barrier {
    ucg_plan_t super;
    ucg_plan_barrier_params_t params;
} ucg_plan_barrier_t;

/**
 * @ingroup UCG_PLAN
 * @brief Append an action to the plan.
 */
static inline void ucg_plan_append_action(ucg_plan_t *plan, 
                                          ucg_plan_action_t *action)
{
    ucs_list_add_tail(&plan->action_list, &action->list);
    return;
};

/**
 * @ingroup UCG_PLAN
 * @brief Create an action then append to the plan.
 */
ucs_status_t ucg_plan_create_and_append_action(ucg_plan_t *plan, 
                                               ucg_plan_action_type_t type, 
                                               ucg_rank_t *peers, int count);

/**
 * @ingroup UCG_PLAN
 * @brief Free all actions in this plan.
 */
void ucg_plan_release_actions(ucg_plan_t *plan);

/**
 * @ingroup UCG_PLAN
 * @brief Register a plan object.
 */
void ucg_plan_register(ucg_plan_t *plan);

#endif