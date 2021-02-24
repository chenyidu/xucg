/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_PLAN_DERIVED_H_
#define UCG_PLAN_DERIVED_H_

#include "ucg_plan.h"
#include <ucs/type/status.h>
#include <ucs/sys/preprocessor.h>
#include <ucs/config/parser.h>

#include <stdint.h>

/* Action buffer holder need to replaced by real buffer, must be different from NULL. */
#define UCG_BUFFER_HOLDER ((void*)1)

/* Register a plan template */
#define UCG_PLAN_REGISTER_TEMPLATE(_template)\
    UCS_STATIC_INIT { \
        ucg_plan_register_template(&_template); \
    }

/**
 * @ingroup UCG_PLAN
 * @brief Algorigthm id for broadcast.
 */
typedef enum ucg_plan_bcast_id {
    UCG_PLAN_BCAST_ID_KTREE, 
} ucg_plan_bcast_id_t;

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief  Action type.
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
typedef struct ucg_plan_action_params {
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
typedef ucs_status_t (*ucg_plan_action_generic_cb_t)(ucg_plan_action_t *action, 
                                                     ucg_plan_action_params_t *params);

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
 * buffers[i], lengths[i] are associated with peers[i].
 */
typedef struct ucg_plan_action_buf {
    uint8_t **buffers; 
    int *lengths;
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
    uint16_t id; /* Action ID */
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

/**
 * @ingroup UCG_PLAN
 * @brief Plan's attribution.
 *
 * At present, Selecting plan will use these information.
 */
typedef struct ucg_plan_attr {
    /* TODO: Depending on selection strategy. */
} ucg_plan_attr_t;

typedef struct ucg_plan ucg_plan_t;
/**
 * @ingroup UCG_PLAN
 * @brief Check Whether a plan is available.
 * 
 * A plan template can instantiate different plans based on specific 
 * configuration and parameters. We need to know whether the plan is available.
 * 
 * @param [in] config Configuration to instantiate a plan.
 * @param [in] params Parameters to instantiate a plan.
 * @return 1-is available, 0-not available.
 */
typedef int (*ucg_plan_is_available_cb_t)(ucg_plan_config_t *config, 
                                          ucg_plan_params_t *params);

/**
 * @ingroup UCG_PLAN
 * @brief Get plan's attribution.
 *
 * @param [in] config Configuration to instantiate a plan.
 * @param [in] params Parameters to instantiate a plan. 
 * @param [out] attr Plan's attribution. 
 */
typedef ucs_status_t (*ucg_plan_query_cb_t)(ucg_plan_config_t *config, 
                                            ucg_plan_params_t *params,
                                            ucg_plan_attr_t* attr);

/**
 * @ingroup UCG_PLAN
 * @brief Initialize a plan object.
 *
 * @param [in] config Configuration to instantiate a plan.
 * @param [in] params Parameters to instantiate a plan. 
 * @param [out] plan Initialized plan object.
 */
typedef ucs_status_t (*ucg_plan_init_cb_t)(ucg_plan_config_t *config,
                                           ucg_plan_params_t *params, 
                                           ucg_plan_t **plan);

/**
 * @ingroup UCG_PLAN
 * @brief Clone a plan object.
 *
 * The new plan has same configuration with the origin plan.
 * @param [in] plan Origin plan object.
 * @param [in] params Parameters.
 * @param [out] new_plan New plan object.
 */
typedef ucs_status_t (*ucg_plan_clone_cb_t)(ucg_plan_t *plan, 
                                            ucg_plan_params_t *params, 
                                            ucg_plan_t **new_plan);

/**
 * @ingroup UCG_PLAN
 * @brief Release a plan returned by init() or clone()
 */
typedef void (*ucg_plan_release_cb_t)(ucg_plan_t *plan);

/**
 * @ingroup UCG_PLAN
 * @brief Plan template.
 * 
 * Plan template is used to instantiate plan objects based on different 
 * configurations and parameters.
 */
typedef struct ucg_plan_template {
    int refcount; 
    ucg_plan_type_t type;
    int id;
    const char *description;
    ucs_config_global_list_entry_t config;

    ucg_plan_is_available_cb_t is_available;
    ucg_plan_query_cb_t query;
    ucg_plan_init_cb_t init;
    ucg_plan_clone_cb_t clone;
    ucg_plan_release_cb_t release;
} ucg_plan_template_t;

/**
 * @ingroup UCG_PLAN
 * @brief Base structure of plan.
 */
struct ucg_plan {
    ucg_plan_template_t *based_template;
    int action_cnt;
    ucg_plan_action_t *action; /* The head of an aciton list. */
};

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
 * @brief Register a plan template.
 */
void ucg_plan_register_template(ucg_plan_template_t *plan_template);

#endif
