/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_PLAN_H_
#define UCG_PLAN_H_

#include <ucg/api/ucg.h>
#include <ucg/core/ucg_datatype.h>
#include <ucs/datastruct/ptr_array.h>
#include <ucs/config/parser.h>

/* Maximum number of elements in buffer vector. */
#define UCG_PLAN_PHASE_BUFFER_VEC_MAX_NUM 10

/* Maximum number of peers in a phase. */
#define UCG_PLAN_PHASE_PEERS_MAX_NUM 64

/**
 * @ingroup UCG_PLAN
 * @brief Flags of plan's capabilities.
 */
typedef enum ucg_plan_cap_flag {
    UCG_PLAN_CAP_FLAG_TOPO_AWARE = UCS_BIT(0), /* Topo-aware plan. */
    UCG_PLAN_CAP_FLAG_SWITCH_ROOT = UCS_BIT(1), /* Support swicthing root. */
} ucg_plan_cap_flag_t;

typedef struct ucg_plan_phase ucg_plan_phase_core_t;
typedef struct ucg_plan ucg_plan_t;

/**
 * @ingroup UCG_PLAN
 * @brief Clone a plan object.
 *
 * The new plan has same configuration with the origin plan.
 * @param [in] plan Origin plan object.
 * @param [in] params Parameters.
 * @param [in] cmp 
 */
typedef ucg_plan_t* (*ucg_plan_clone_cb_t)(ucg_plan_t *plan, 
                                           ucg_plan_params_t *params,
                                           ucg_plan_params_cmp_t cmp);

/**
 * @ingroup UCG_PLAN
 * @brief Destroy a plan returned by clone()
 */
typedef void (*ucg_plan_destroy_cb_t)(ucg_plan_t *plan);

/**
 * @ingroup UCG_PLAN
 * @brief Plan type
 */
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
    UCG_PLAN_PHASE_ACTION_SEND, /* Send local data to peers. */
    UCG_PLAN_PHASE_ACTION_RECV, /* Receive data from peers. */
    UCG_PLAN_PHASE_ACTION_REDUCE, /* Do reduction on two data. */
    UCG_PLAN_PHASE_ACTION_COPY, /* Copy data from src to dst. */
    UCG_PLAN_PHASE_ACTION_FORWARD, /* Forward message data to peers. */
    UCG_PLAN_PHASE_ACTION_MAX,
} ucg_plan_phase_action_t;

/**
 * @ingroup UCG_PLAN
 * @brief Phase buffer.
 * 
 * Conventions on using this structure for actions:
 * 1. SEND: "fisrt" points to send buffer, "second" is not used.
 * 2. RECV: only use the "length".
 * 3. REDUCE: B = A op B, "first" points to A, "second" points to B.
 * 4. COPY: "first" points to src buffer, "second" points to dst buffer.
 * 5. FORWARD: only use the "length".
 * "length" is always used.
 */     
typedef struct ucg_plan_phase_buffer {
    uint8_t *first;
    uint8_t *second;
    uint64_t length;
} ucg_plan_phase_buffer_t;

/**
 * @ingroup UCG_PLAN
 * @brief Phase buffer vector.
 * 
 * Actions may deal with non-contig buffer, buffer vector is needed.
 * As a rule of thumb, there are usually no more than 10 non-contig buffers.
 * UCG_PLAN_PHASE_BUFFER_VEC_MAX_NUM = 10, change it if necessary. However, if 
 * the value is large, it is better to allocate memory from the heap to avoid 
 * space waste.
 */ 
typedef struct ucg_plan_phase_buffer_vec {
    int count;
    ucg_plan_phase_buffer_t buffers[UCG_PLAN_PHASE_BUFFER_VEC_MAX_NUM];
} ucg_plan_phase_buffer_vec_t;

typedef struct ucg_plan_phase_buffer_vecs {
    int count;
    ucg_plan_phase_buffer_vec_t vec[UCG_PLAN_PHASE_PEERS_MAX_NUM];
} ucg_plan_phase_buffer_vecs_t;

/**
 * @ingroup UCG_PLAN
 * @brief Phase peers.
 * 
 * Different algorithms will have different number of peers, but at present, it 
 * usually doesn't exceed 64 when the algorithm is good. 
 * UCG_PLAN_PHASE_PEERS_MAX_NUM = 64.
 */ 
typedef struct ucg_plan_phase_peers {
    int count;
    int ranks[UCG_PLAN_PHASE_PEERS_MAX_NUM]; 
} ucg_plan_phase_peers_t;

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief The unchangeable part of a phase.
 * 
 * When algorithm is fixed, the data in this structure is not changed which means
 * it can be reused.
 */
typedef struct ucg_plan_phase_core {
    int refcount; /* Reference count */
    /* actions[i] corresponds to peers[i] */
    int count;
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
    /* actions[i] corresponds to bvectors[i] */
    ucg_plan_phase_buffer_vecs_t bvectors[UCG_PLAN_PHASE_ACTION_MAX];
} ucg_plan_phase_t;

/**
 * @ingroup UCG_PLAN
 * @brief The unchangeable part of a plan.
 */
typedef struct ucg_plan_core {
    ucg_plan_type_t type;
    int id;
    const char *desc;
    uint64_t flags;
    ucs_config_global_list_entry_t config;

    ucg_plan_clone_cb_t clone;
    ucg_plan_destroy_cb_t destroy;
} ucg_plan_core_t;

/**
 * @ingroup UCG_PLAN
 * @brief Collective operation execution plan.
 */
typedef struct ucg_plan {
    int refcount;
    ucg_plan_core_t *core;
    struct {
        ucg_dt_state_t *pack_state;
        ucg_dt_state_t *unpack_state;
    } dt;
   
    ucg_plan_phase_t *phase; /* The head phase. */
} ucg_plan_t;

typedef ucg_group_members_t ucg_plan_members_t;
/**
 * @ingroup UCG_PLAN
 * @brief Base structure of plan parameters.
 */
typedef struct ucg_plan_params {
    ucg_plan_type_t type;
    void *config;
    ucg_plan_members_t members;
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
