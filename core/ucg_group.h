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

/**
 * @ingroup UCG_GROUP
 * @brief Group 
 */
typedef struct ucg_group {
    ucg_context_t *context;

    ucp_worker_h ucp_worker;
    ucg_group_id_t id; /* Unique group id specified by user. */
    ucg_group_members_t members;

    uint32_t next_req_id; /* next request id. */
    ucs_list_link_t outstanding_req;
    ucs_queue_head_t pending_req;
    uint8_t is_barrier;
} ucg_group_t;

static inline ucg_group_id_t ucg_group_id(ucg_group_t *group)
{
    return group->id;
}

static inline uint32_t ucg_group_next_req_id(ucg_group_t *group)
{
    return group->next_req_id++;
}

static inline uint8_t ucg_group_is_barrier(ucg_group_t *group)
{   
    return group->is_barrier;
}

static inline void ucg_group_set_barrier(ucg_group_t *group)
{
    group->is_barrier = 1;
    return;
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

#endif