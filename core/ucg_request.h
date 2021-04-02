/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_REQUEST_H_
#define UCG_REQUEST_H_

#include "ucg_group.h"
#include <ucg/plan/api.h>

/**
 * @ingroup UCG_REQUEST
 * @brief Request state.
 */
typedef enum ucg_request_state {
    UCG_REQUEST_STATE_INITED,
    UCG_REQUEST_STATE_PENDING,
    UCG_REQUEST_STATE_OUTSTANDING,
    UCG_REQUEST_STATE_CANCELED,
    UCG_REQUEST_STATE_COMPLETED,
} ucg_request_state_t;

typedef struct ucg_request {
    ucs_status_t comp_status; /* Completion status. */
    ucg_request_state_t state;

    ucg_group_t *group;
    ucg_plan_t *plan;
    uint8_t is_barrier;

    ucg_plan_handle_t handle;
} ucg_request_t;

#endif