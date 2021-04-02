/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_GROUP_H_
#define UCG_GROUP_H_

#include "ucg_context.h"

#include <ucg/api/ucg.h>
#include <ucs/datastruct/list.h>
#include <ucs/datastruct/queue.h>
#include <ucs/datastruct/callbackq.h>
#include <ucs/arch/atomic.h>

/* Indicate the group is in barrier state. */
#define UCG_GROUP_BARRIER 1
/* Indicate the group is not in barrier state. */
#define UCG_GROUP_NBARRIER 0

/**
 * @ingroup UCG_GROUP
 * @brief Group 
 */
typedef struct ucg_group {
    ucg_context_t *context;

    ucp_worker_h ucp_worker;
    ucg_group_id_t id; /* Unique group id specified by user. */
    ucg_group_members_t members;

    uint8_t is_barrier;
    uint32_t next_req_id; /* Next request id. */
    ucs_callbackq_t progress_q;
} ucg_group_t;

static inline ucg_group_id_t ucg_group_id(ucg_group_t *group)
{
    return group->id;
}

static inline uint32_t ucg_group_next_req_id(ucg_group_t *group)
{
    return group->next_req_id++;
}

static inline uint8_t ucg_group_barrier_cswap(ucg_group_t *group, uint8_t expected, 
                                              uint8_t desired)
{
    ucs_assert(expected == UCG_GROUP_BARRIER || expected == UCG_GROUP_NBARRIER);
    ucs_assert(desired == UCG_GROUP_BARRIER || desired == UCG_GROUP_NBARRIER);
#if ENABLE_MT
    return ucs_atomic_cswap8(&group->is_barrier, expected, desired);
#else
    uint8_t prev = group->is_barrier;
    if (prev == expected) {
        if (prev != desired) {
            group->is_barrier = desired;
        }
    }
    return prev;
#endif 
}

static inline ucg_plan_t* ucg_group_select_plan(ucg_group_t *group, const ucg_plan_params_t *params)
{
    return ucg_context_select_plan(group->context, params);
}

static inline void ucg_group_release_plan(ucg_group_t *group, ucg_plan_t* plan)
{
    return ucg_context_release_plan(group->context, plan);
}

static inline void ucg_group_members_share(ucg_group_t *group, ucg_group_members_t *members)
{
    members->self = group->members.self;
    members->count = group->members.count;
    members->mh = group->members.mh;
    return;
}

static inline int ucg_group_add_progress(ucg_group_t *group, ucs_callback_t cb, void *arg)
{
    return ucs_callbackq_add(&group->progress_q, cb, arg, UCS_CALLBACKQ_FLAG_FAST);
}

static inline void ucg_group_rm_progress(ucg_group_t *group, int id)
{
    return ucs_callbackq_remove(&group->progress_q, id);
}

#endif