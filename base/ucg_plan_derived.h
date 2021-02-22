/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_PLAN_INNER_H_
#define UCG_PLAN_INNER_H_

#include "ucg_plan.h"

/* Action buffer holder need to replaced by real buffer, must be different from NULL. */
#define UCG_BUFFER_HOLDER ((void*)1)

/* Register a plan pointer */
#define UCG_PLAN_REGISTER(_plan) \
    UCS_STATIC_INIT { \
        ucg_plan_register(_plan); \
    }

/**
 * Define a static object of plan and register to plan pool.
 * Example: UCG_PLAN_STATIC_DEFINE_AND_REGISTER(ktree, bcast, BCAST, "Using k-nonimal tree to broadcast.")
 */
#define UCG_PLAN_STATIC_DEFINE_AND_REGISTER(_name, _type, _uppercase_type, _description, \
                                            _is_available, _query, _clone, _release) \
    static ucg_plan_##_type##_t ucg_plan_##_type##_##_name = {\
        .refcount = 1, /* Static object can not be released. */ \
        .type = UCG_PLAN_TYPE_##_upcase_type, \
        .name = #_name, \
        .description = _description, \
        .action_cnt = 0, \
        .action = NULL, \
        .is_available = _is_available, \
        .query = _query, \
        .clone = _clone, \
        .release = _release, \
    }; \
    UCG_PLAN_REGISTER(&ucg_plan_##_type##_##_name)

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Abstracted action type.
 *
 * At present, The way of SEND, RECV and REDUCE are performed is very clear. 
 * Keep a generic type for some special actions.
 */        
typedef enum ucg_plan_action_type {
    UCG_PLAN_ACTION_TYPE_SEND,
    UCG_PLAN_ACTION_TYPE_RECV,
    UCG_PLAN_ACTION_TYPE_REDUCE, 
    UCG_PLAN_ACTION_TYPE_GENERIC,
    UCG_PLAN_ACTION_TYPE_MAX,
} ucg_plan_action_type_t;

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Parameters for performing action.
 */
typedef ucg_plan_action_params {
    struct {
        uint8_t *data; 
        int length;
    } recv;
} ucg_plan_action_params_t;

typedef struct ucg_plan_action ucg_plan_action_t;

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Generic action routine.
 */
typedef ucs_status_t (*ucg_plan_action_generic_cb_t)(ucg_plan_action_t *action, ucg_plan_action_params_t *params);

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Action peers for sending and receiving.
 */
typedef struct ucg_plan_action_peers {
    int count; /* Number of peers */
    int *peers; /* Relative member ID which needs to be converted to handle 
                   for communication. */
} ucg_plan_action_peers_t;

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Buffer structure for action.
 *
 * Action buffer is related to action peers, so the number of elements in 
 * buffers is equal to ucg_plan_action_peers_t::count.In other words, 
 * buffers[i], lengths[i] and capacitys[i] are associated with peers[i].
 */
typedef struct ucg_plan_action_buf {
    uint8_t **buffers; 
    int *lengths;
    int *capcitys;
} ucg_plan_action_buf_t;

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Infrastructure of an action.
 *
 * When cloning a plan, something in a action has no need to change, we call it 
 * the infrastructure of an action which can be reused directly.
 */
typedef struct ucg_plan_action_infra {
    int refcount; /* Reference count */
    uint8_t id; /* Action ID */
    ucg_plan_action_type_t type;
    union {
        ucg_plan_action_peers_t send;
        ucg_plan_action_peers_t recv;
        ucg_plan_action_peers_t reduce;
        ucg_plan_action_generic_cb_t generic;
    };
} ucg_plan_action_infra_t;

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Base structure of plan action.
 */
struct ucg_plan_action {
    ucg_plan_action_infra_t *infra;
    union {
       ucg_plan_action_buf_t send;
       ucg_plan_action_buf_t recv;
       ucg_plan_action_buf_t reduce;
    };
    ucg_plan_action_t* next;
};

typedef struct ucg_plan_attr {
    
} ucg_plan_attr_t;

/**
 * @ingroup UCG_PLAN
 * @brief Check whether the plan is available.
 * 
 * @return 1-is available, 0-not available.
 */
typedef int (*ucg_plan_is_available_cb_t)(ucg_plan_t *plan, ucg_plan_params_t *params);

/**
 * @ingroup UCG_PLAN
 * @brief Get plan's attribution.
 *
 * This interface is used to obtain some scoring information from plan, so that 
 * we can select the best plan from multiple plans.
 */
typedef ucs_status_t (*ucg_plan_query_cb_t)(ucg_plan_t *plan, ucg_plan_attr_t* attr);

/**
 * @ingroup UCG_PLAN
 * @brief Clone a plan.
 *
 * This interface should return a initialized plan which has generated all actions.
 */
typedef ucg_plan_t* (*ucg_plan_clone_cb_t)(ucg_plan_t *plan, ucg_plan_params_t *params);

/**
 * @ingroup UCG_PLAN
 * @brief Release a plan.
 *
 * This interface should return a initialized plan which has generated all actions.
 */
typedef void (*ucg_plan_release_cb_t)(ucg_plan_t *plan);

/**
 * @ingroup UCG_PLAN
 * @brief Base structure of plan.
 */
typedef struct ucg_plan {
    int refcount; 
    ucg_plan_type_t type;
    const char *name;
    const char *description;
 
    int action_cnt;
    ucg_plan_action_t *action; /* The head of an aciton list. */
 
    ucg_plan_is_available_cb_t is_available;
    ucg_plan_query_cb_t query;
    ucg_plan_clone_cb_t clone;
    ucg_plan_release_cb_t release;
} ucg_plan_t;

/**
 * @ingroup UCG_PLAN
 * @brief Base structure of broadcast plan.
 */
typedef strcut ucg_plan_bcast {
    ucg_plan_t super;
    ucg_plan_bcast_params_t params;
} ucg_plan_bcast_t;

/**
 * @ingroup UCG_PLAN
 * @brief Base structure of allreduce plan.
 */
typedef strcut ucg_plan_allreduce {
    ucg_plan_t super;
    ucg_plan_allreduce_params_t params;
} ucg_plan_allreduce_t;

/**
 * @ingroup UCG_PLAN
 * @brief Base structure of barrier plan.
 */
typedef strcut ucg_plan_barrier {
    ucg_plan_t super;
    ucg_plan_barrier_params_t params;
} ucg_plan_barrier_t;

/**
 * @ingroup UCG_PLAN
 * @brief Register a plan
 */
void ucg_plan_register(ucg_plan_t *plan);

#endif
