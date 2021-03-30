/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_REQUEST_H_
#define UCG_REQUEST_H_

#include "ucg_group.h"
#include "ucg_channel.h"
#include <ucg/plan/api.h>

typedef enum {
    UCG_REQUEST_STATE_INIT,
    UCG_REQUEST_STATE_START,
    UCG_REQUEST_STATE_CANNELED,
    UCG_REQUEST_STATE_COMPLETED,
} ucg_request_state_t;

typedef struct ucg_request {
    ucs_status_t status; /* Completion status. */
    ucg_request_state_t state;

    ucg_group_t *group;

    ucg_plan_t *plan;
    ucg_channel_t *channel; 
} ucg_request_t;

#endif