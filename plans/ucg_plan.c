/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#include <ucg/plans/ucg_plan.h>
#include <ucg/plans/ucg_plan_impl.h>
#include <ucs/debug/assert.h>

static ucg_ppool_t global_ppool;

static void ucg_ppool_init(ucg_ppool_t *ppool)
{
    kh_init(ucg_ppool);

    ppool->caches.max_num = 200;
    ucs_list_head_init(&ppool->caches.global_list);
    for (int i = 0; i < UCG_PLAN_TYPE_MAX; ++i) {
         ucs_list_head_init(&ppool->caches.type_list[i]);
    }
    global_ppool.inited = 1;
    return;
}

static ucg_plan_t* ucg_ppool_search_plan(ucg_ppool_t *ppool,
                                         ucg_plan_params_t *params)
{

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
    return ucg_plan_clone(plan, params);
}

static uint64_t ucg_ppool_hash_key(int type, int id)
{
    return (uint64_t)type << 32| id;
}

static uint64_t ucg_plan_hash_key(ucg_plan_t *plan)
{
    return ucg_ppool_hash_key(plan->core->type, plan->core->id);
}

void ucg_plan_register(ucg_plan_t *plan)
{
    if (!global_ppool.inited) {
        ucg_ppool_init(&global_ppool);
    }
    ucs_assert(kh_end(&global_ppool.plans) == kh_get(ucg_ppool, 
                                                     &global_ppool.plans, 
                                                     ucg_plan_hash_key(plan)));
    kh_put(ucg_ppool, &global_ppool.plans, ucg_plan_hash_key(plan), plan);
    return;
}

ucg_plan_t* ucg_plan_get(ucg_plan_params_t *params)
{
    ucs_assert(global_ppool.inited);
    ucg_plan_t *plan = NULL;
    plan = ucg_ppool_search_plan(&global_ppool, params);
    if (plan != NULL) {
        return plan;
    }

    return ucg_ppool_create_plan(&global_ppool, params);
}

ucs_status_t ucg_plan_put(ucg_plan_t *plan)
{
    ucs_assert(global_ppool.inited);
    ucs_assert(plan->refcount > 0);
    --plan->refcount;
    return;
}