/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#include "ucg_ppool_impl.h"
#include <ucs/debug/assert.h>
#include <ucs/debug/log.h>

static ucg_ppool_t g_global_ppool;


static uint64_t ucg_ppool_hash_key(int type, int id)
{
    return (uint64_t)type << 32| id;
}

static void ucg_ppool_init(ucg_ppool_t *ppool)
{
    kh_init(ucg_ppool);

    ppool->caches.max_num = 200;
    ucs_list_head_init(&ppool->caches.global_list);
    for (int i = 0; i < UCG_PLAN_TYPE_MAX; ++i) {
         ucs_list_head_init(&ppool->caches.type_list[i]);
    }
    g_global_ppool.inited = 1;
    return;
}

static uint64_t ucg_plan_hash_key(ucg_plan_t *plan)
{
    return ucg_ppool_hash_key(plan->core->type, plan->core->id);
}

void ucg_ppool_register_plan(ucg_plan_t *plan)
{
    if (!g_global_ppool.inited) {
        ucg_ppool_init(&g_global_ppool);
    }
    int ret = 0;
    khiter_t iter = kh_put(ucg_ppool, &g_global_ppool.plans, ucg_plan_hash_key(plan), &ret);
    ucs_assert(ret == 1);
    kh_value(&g_global_ppool.plans, iter) = plan;
    return;
}

static inline ucg_plan_t* ucg_plan_clone(ucg_plan_t *plan, 
                                         ucg_plan_params_t *params,
                                         ucg_plan_clone_advice_t advice)
{
    return plan->core->clone(plan, params, advice);
}

static inline void ucg_plan_destroy(ucg_plan_t *plan)
{
    return plan->core->destroy(plan);
}

static ucg_plan_t* ucg_ppool_search_plan(ucg_ppool_t *ppool,
                                         ucg_plan_params_t *params)
{
    return NULL;
}

static int ucg_ppool_select_algo(ucg_ppool_t *ppool,
                                 ucg_plan_params_t *params)
{
    return -1;
}

static ucg_plan_t* ucg_ppool_create_plan(ucg_ppool_t *ppool,
                                         ucg_plan_params_t *params)
{
    int id = ucg_ppool_select_algo(ppool, params);
    if (id < 0) {
        ucs_error("No algorithm is available.");
        return NULL;
    }

    khiter_t iter = kh_get(ucg_ppool, &ppool->plans, 
                           ucg_ppool_hash_key(params->type, id));
    ucs_assert(iter != kh_end(&ppool->plans));
    ucg_plan_t *plan = kh_value(&ppool->plans, iter);
    return ucg_plan_clone(plan, params, UCG_PLAN_CLONE_ADVICE_UNEQUAL);
}


ucg_plan_t* ucg_ppool_get_plan(ucg_plan_params_t *params)
{
    ucs_assert(g_global_ppool.inited);
    ucg_plan_t *plan = NULL;
    plan = ucg_ppool_search_plan(&g_global_ppool, params);
    if (plan != NULL) {
        return plan;
    }

    return ucg_ppool_create_plan(&g_global_ppool, params);
}

void ucg_ppool_put_plan(ucg_plan_t *plan)
{
    return;
}