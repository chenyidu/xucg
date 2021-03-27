/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_CONTEXT_H_
#define UCG_CONTEXT_H_

#include "ucg_ppool.h"
#include <ucs/datastruct/list.h>
#include <ucs/datastruct/ptr_array.h>

/**
 * @ingroup UCG_CONTEXT
 * @brief UCG context. 
 */
typedef struct ucg_context {
    ucg_ppool_t *ppool;
    ucg_config_t *config;
} ucg_context_t;

static inline ucg_plan_t* ucg_context_select_plan(ucg_context_t *context, const ucg_plan_params_t *params)
{
    return ucg_ppool_select_plan(context->ppool, context->config, params);
}

static inline void ucg_context_release_plan(ucg_context_t *context, ucg_plan_t* plan)
{
    return ucg_ppool_release_plan(context->ppool, plan);
}

#endif