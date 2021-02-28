/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_CONTEXT_H_
#define UCG_CONTEXT_H_

#include <ucg/plans/ucg_plan.h>
#include <ucs/config/types.h>

typedef struct ucg_config {
    char *env_prefix;
    ucs_config_names_array_t bcast_plans;
    ucs_config_names_array_t allreduce_plans;
    ucs_config_names_array_t barrier_plans;
} ucg_config_t;

typedef struct ucg_context {
    ucg_config_t *config;
    ucg_ppool_t pp;
} ucg_context_t;

#endif 