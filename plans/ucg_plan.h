/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_PLAN_H_
#define UCG_PLAN_H_

#include <ucg/api/ucg_dt.h>
#include <ucs/datastruct/ptr_array.h>
#include <ucs/config/parser.h>

/* Maximum number of elements in buffer vector. */
#define UCG_PLAN_PHASE_BUFFER_VEC_MAX_NUM 10

/* Maximum number of peers in a phase. */
#define UCG_PLAN_PHASE_PEERS_MAX_NUM 64

typedef struct ucg_plan_phase ucg_plan_phase_core_t;
typedef struct ucg_plan ucg_plan_t;

/**
 * @ingroup UCG_PLAN
 * @brief Clone a plan object.
 *
 * The new plan has same configuration with the origin plan.
 * @param [in] plan Origin plan object.
 * @param [in] params Parameters.
 */
typedef ucg_plan_t* (*ucg_plan_clone_cb_t)(ucg_plan_t *plan, 
                                           ucg_plan_params_t *params,
                                           );

/**
 * @ingroup UCG_PLAN
 * @brief Release a plan returned by init() or clone()
 */
typedef void (*ucg_plan_release_cb_t)(ucg_plan_t *plan);

typedef enum ucg_plan_type {
    UCG_PLAN_TYPE_BCAST,
    UCG_PLAN_TYPE_ALLREDUCE,
    UCS_PLAN_TYPE_BARRIER,
    UCG_PLAN_TYPE_MAX,
} ucg_plan_type_t;

/**
 * @ingroup UCG_PLAN
 * @brief Phase action.
 */        
typedef enum ucg_plan_phase_action {
    UCG_PLAN_PHASE_ACTION_NOP = 0,
    UCG_PLAN_PHASE_ACTION_SEND,
    UCG_PLAN_PHASE_ACTION_RECV,
    UCG_PLAN_PHASE_ACTION_REDUCE, 
    UCG_PLAN_PHASE_ACTION_COPY,
    UCG_PLAN_PHASE_ACTION_FORWARD, 
    UCG_PLAN_PHASE_ACTION_MAX,
} ucg_plan_phase_action_t;

typedef struct ucg_plan_phase_buffer {
    uint8_t *first;
    uint8_t *second;
    uint64_t length;
} ucg_plan_phase_buffer_t;

typedef struct ucg_plan_phase_buffer_vec {
    int count;
    ucg_plan_phase_buffer_t buffers[UCG_PLAN_PHASE_BUFFER_VEC_MAX_NUM];
} ucg_plan_phase_buffer_vec_t;

typedef struct ucg_plan_phase_peers {
    int count;
    int ranks[UCG_PLAN_PHASE_PEERS_MAX_NUM]; 
} ucg_plan_phase_peers_t;

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief The little changed part of a phase.
 */
typedef struct ucg_plan_phase_core {
    int refcount; /* Reference count */
    /* actions[i] corresponds to peers[i] */
    ucg_plan_phase_action_t actions[UCG_PLAN_PHASE_ACTION_MAX];
    ucg_plan_phase_peers_t peers[UCG_PLAN_PHASE_ACTION_MAX];
} ucg_plan_phase_core_t;

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Plan phase.
 */
typedef struct ucg_plan_phase {
    ucg_plan_phase_core_t *core;
    ucg_plan_phase_t *next;
    /* actions[i] corresponds to vectors[i] */
    ucg_plan_phase_buffer_vec_t vectors[UCG_PLAN_PHASE_ACTION_MAX];
} ucg_plan_phase_t;

/**
 * @ingroup UCG_PLAN
 * @brief The little changed part of a plan.
 */
typedef struct ucg_plan_core {
    int refcount; 
    ucg_plan_type_t type;
    int id;
    const char *desc;
    ucs_config_global_list_entry_t config;

    ucg_plan_clone_cb_t clone;
    ucg_plan_release_cb_t release;
} ucg_plan_core_t;

/**
 * @ingroup UCG_PLAN
 * @brief Collective operation execution plan.
 */
typedef struct ucg_plan {
    int refcount;
    ucg_plan_core_t *core;
    int phase_cnt;
    ucg_plan_phase_t *phase; /* The head phase. */
} ucg_plan_t;

/**
 * @ingroup UCG_PLAN
 * @brief Base structure of plan parameters.
 */
typedef struct ucg_plan_params {
    ucg_plan_type_t type;
    void *config;
    int count; /* Number of elements in handles. */
    uint64_t *handles; /* Handles of communication members. */
} ucg_plan_params_t;

/**
 * @ingroup UCG_PLAN
 * @brief Structure of broadcast plan parameters.
 */
typedef struct ucg_plan_bcast_params {
    ucg_plan_params_t super;
    void *buffer; 
    int count; 
    ucg_datatype_t *dtype;
    int root;
} ucg_plan_bcast_params_t;

/**
 * @ingroup UCG_PLAN
 * @brief Structure of allreduce plan parameters.
 */
typedef struct ucg_plan_allreduce_params {
    ucg_plan_params_t super;
    const void *sendbuf;
    void *recvbuf;
    int count;
    ucg_datatype_t *dtype;
    ucg_op_t *op;
} ucg_plan_allreduce_params_t;

/**
 * @ingroup UCG_PLAN
 * @brief Select a plan.
 *
 * @param [in] params Parameters.
 */
 ucg_plan_t *ucg_plan_get(ucg_plan_params_t *params);

 ucs_status_t ucg_plan_put(ucg_plan_t *plan);

#endif
