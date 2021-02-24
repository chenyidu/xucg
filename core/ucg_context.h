/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_CONTEXT_H_
#define UCG_CONTEXT_H_

#include <ucg/base/ucg_plan.h>
#include <ucs/config/types.h>

typedef struct ucg_config {
    char *env_prefix;
    UCS_CONFIG_ARRAY_FIELD(int, value) bcast_plans;
    UCS_CONFIG_ARRAY_FIELD(int, value) allreduce_plans;
    UCS_CONFIG_ARRAY_FIELD(int, value) barrier_plans;
} ucg_config_t;

typedef struct ucg_context {
    ucg_plan_pool_t plan_pool;
} ucg_context_t;

#endif 