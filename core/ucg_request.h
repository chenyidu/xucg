/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_REQUEST_H_
#define UCG_REQUEST_H_

#include "ucg_group.h"
#include <ucg/plan/api.h>

typedef struct ucg_request {
    ucg_group_t *group;
    ucg_plan_t *plan;
    ucs_list_link_t list; /* outstanding list. */
    ucs_queue_elem_t elem; /* pending queue. */
} ucg_request_t;

#endif