/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_PLAN_BASE_H_
#define UCG_PLAN_BASE_H_

#include <ucg/plans/ucg_plan.h>
#include <ucs/type/status.h>
#include <ucs/sys/preprocessor.h>
#include <ucs/config/parser.h>

#include <stdint.h>

/* Assertions which are checked in compile-time. 
 * TODO: move to appropriate file. 
 */
#define UCG_STATIC_ASSERT(_cond) typedef int UCS_PP_APPEND_UNIQUE_ID(assert)[(_cond)?1:-1]

/* Indicates that the buffer can't be read or written. */
#define UCG_BUFFER_INVALID ((void*)0)

/* Indicates that the buffer should be replaced by incoming buffer. */
#define UCG_BUFFER_HOLDER ((void*)1)

/* Default maximum number of peers */
#define UCG_DEFAULT_MAX_PEERS_NUM 64

/* The ID after 10000 is reserved for x plan. */
#define UCG_PLAN_ID_MAX 10000

/* Register a plan */
#define UCG_PLAN_REGISTER(_plan)\
    UCS_STATIC_INIT { \
        ucg_plan_register(&_plan); \
    }

/**
 * @ingroup UCG_PLAN
 * @brief Algorigthm id for broadcast.
 */
typedef enum ucg_plan_bcast_id {
    UCG_PLAN_BCAST_ID_KTREE, /* knomial tree */
    UCG_PLAN_BCAST_ID_MAX,
} ucg_plan_bcast_id_t;
UCG_STATIC_ASSERT(UCG_PLAN_BCAST_ID_MAX <= UCG_PLAN_ID_MAX);

/**
 * @ingroup UCG_PLAN
 * @brief Algorigthm id for broadcast.
 */
typedef enum ucg_plan_allreduce_id {
    UCG_PLAN_ALLREDUCE_ID_RD,  /* recursive doubling */
    UCG_PLAN_ALLREDUCE_ID_MAX,
} ucg_plan_allreduce_id_t;
UCG_STATIC_ASSERT(UCG_PLAN_ALLREDUCE_ID_MAX <= UCG_PLAN_ID_MAX);

/**
 * @ingroup UCG_PLAN_PHASE
 * @brief Phase type.
 *
 * At present, The way of SEND, RECV and REDUCE are performed is very clear. 
 * Keep a generic type for some special actions.
 */        
typedef enum ucg_plan_phase_type {
    UCG_PLAN_PHASE_TYPE_SEND,
    UCG_PLAN_PHASE_TYPE_RECV,
    UCG_PLAN_PHASE_TYPE_REDUCE, 
    UCG_PLAN_PHASE_TYPE_GENERIC,
    UCG_PLAN_PHASE_TYPE_MAX,
} ucg_plan_phase_type_t;

/**
 * @ingroup UCG_PLAN_PHASE
 * @brief Receive and transmit peers in a phase.
 */
typedef struct ucg_plan_phase_peer {
    int count; /* Number of peers */
    union {
        int ranks[UCG_DEFAULT_MAX_PEERS_NUM]; /* Need to be converted to member handle for communication. */ 
        int *dyn_ranks; /* When count is larger than UCG_DEFAULT_MAX_PEERS_NUM, allocate from the heap  */
    };
} ucg_plan_phase_peer_t;

/**
 * @ingroup UCG_PLAN_PHASE
 * @brief Buffers used in a phase.
 *
 * Phase buffer is related to phase peers.In other words, 
 * buffer[i], length[i] are associated with ranks[i].
 */
typedef struct ucg_plan_phase_buf {
    int count; /* Number of peers */
    uint64_t *buffers; 
    uint64_t *lengths;
} ucg_plan_phase_buf_t;

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
struct ucg_plan_phase {
    ucg_plan_action_infra_t *infra;
    union {
       ucg_plan_action_buf_t send;
       ucg_plan_action_buf_t recv;
       ucg_plan_action_buf_t reduce;
    };
    ucg_plan_action_t* next;
};

typedef struct ucg_plan ucg_plan_t;

/**
 * @ingroup UCG_PLAN
 * @brief Clone a plan object.
 *
 * The new plan has same configuration with the origin plan.
 * @param [in] plan Origin plan object.
 * @param [in] params Parameters.
 * @param [out] new_plan New plan object.
 */
typedef ucg_plan_t* (*ucg_plan_clone_cb_t)(ucg_plan_t *plan, 
                                           ucg_plan_params_t *params);

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
