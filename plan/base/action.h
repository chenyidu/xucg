/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_PLAN_ACTION_H_
#define UCG_PLAN_ACTION_H_

#include <ucg/api/ucg_def.h>
#include <ucs/datastruct/list.h>
#include <ucs/config/parser.h>
#include <stdint.h>

/* Maximum number of elements in buffer vector. */
#define UCG_PLAN_PHASE_BUFFER_VEC_MAX_NUM 10

/* Maximum number of peers in a action. */
#define UCG_PLAN_PHASE_PEERS_MAX_NUM 64

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Plan action type.
 */        
typedef enum ucg_plan_action_type {
    UCG_PLAN_ACTION_TYPE_SEND, /* Send local data to peers. */
    UCG_PLAN_ACTION_TYPE_RECV, /* Receive data from peers. */
    UCG_PLAN_ACTION_TYPE_REDUCE, /* Do reduction on two data. */
    UCG_PLAN_ACTION_TYPE_COPY, /* Copy data from src to dst. */
    UCG_PLAN_ACTION_TYPE_FORWARD, /* Forward message data to peers. */
    UCG_PLAN_ACTION_TYPE_FENCE, /* Block until all prior actions are completed. */
} ucg_plan_action_type_t;

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Action data.
 * 
 * Conventions on using this structure for actions:
 * 1. SEND: "fisrt" points to send buffer, "second" is not used.
 * 2. RECV: only use the "length".
 * 3. REDUCE: B = A op B, "first" points to A, "second" points to B.
 * 4. COPY: "first" points to src buffer, "second" points to dst buffer.
 * 5. FORWARD: only use the "length".
 */     
typedef struct ucg_plan_action_data {
    uint8_t *first;
    uint8_t *second;
    uint64_t length;
} ucg_plan_action_data_t;

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Action data vector.
 * 
 * Actions may deal with non-contig buffer, data vector is needed.
 * As a rule of thumb, there are usually no more than 10 non-contig buffers.
 * UCG_PLAN_PHASE_BUFFER_VEC_MAX_NUM = 10, change it if necessary. However, if 
 * the value is large, it is better to allocate memory from the heap to avoid 
 * space waste.
 */ 
typedef struct ucg_plan_action_datav {
    int count;
    ucg_plan_action_data_t data[UCG_PLAN_PHASE_BUFFER_VEC_MAX_NUM];
} ucg_plan_action_datav_t;

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief The unchangeable part of an action.
 */
typedef struct ucg_plan_action_core {
    int refcount;
    ucg_plan_action_type_t type;
    int count; /* Number of peers. */
    ucg_rank_t peers[UCG_PLAN_PHASE_PEERS_MAX_NUM];
} ucg_plan_action_core_t;

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Plan action.
 */
typedef struct ucg_plan_action {
    ucg_plan_action_core_t *core;

    ucg_mh_t *mh; /* This member handle of this action, mh[peers[i]]. */
    ucg_plan_action_datav_t datav[UCG_PLAN_PHASE_PEERS_MAX_NUM]; /* peers[i] associated with datav[i]. */
    ucs_list_link_t list;
} ucg_plan_action_t;

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Allocate a action object.
 * 
 * @param [in] with_core Obtain action core at the same time or not.
 */
ucg_plan_action_t* ucg_plan_action_allocate(int with_core);

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Release a action object.
 */
void ucg_plan_action_release(ucg_plan_action_t* phase);

static inline void ucg_plan_action_init_core(ucg_plan_action_t *action,
                                             ucg_plan_action_type_t type, 
                                             ucg_rank_t *peers, int count)
{
    action->core->type = type;
    action->core->count = count;
    for (int i = 0; i < count; ++i) {
        action->core->peers[i] = peers[i];
    }
    return;
}

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Increase refcount of action core and return it.
 */
static inline ucg_plan_action_core_t* ucg_plan_action_obtain_core(ucg_plan_action_t* action)
{
    action->core->refcount++;
    return action->core;
}

/**
 * @ingroup UCG_PLAN
 * @brief Plan action type string.
 */
const char* ucg_plan_action_type_str(ucg_plan_action_type_t type);

/**
 * @ingroup UCG_PLAN
 * @brief Free a plan.
 * 
 * Print action's information, just for debug purpose.
 */
void ucg_plan_action_print(ucg_plan_action_t *action, FILE *stream);

#endif