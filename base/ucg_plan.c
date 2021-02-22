/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#include "ucg_plan.h"
#include "ucg_plan_derived.h"
#include <ucs/debug/assert.h>

typedef struct ucg_plan_pool {
    int initialized;
    ucs_ptr_array_t template_plans[UCG_PLAN_TYPE_MAX];
    ucs_link_list_t instance_plans[UCG_PLAN_TYPE_MAX];
} ucg_plan_pool_t;

static ucg_plan_pool_t g_plan_pool = {
    .initialized = 0;
};

static int ucg_plan_is_available(ucg_plan_t *plan, ucg_plan_params_t *params)
{
    return plan->is_available(plan, params);
}

static ucs_status_t ucg_plan_query(ucg_plan_t *plan, ucg_plan_attr_t* attr)
{
    return plan->query(plan, attr);
}

static ucg_plan_t* ucg_plan_clone(ucg_plan_t *plan, ucg_plan_params_t *params)
{
    return plan->clone(plan, params);
}

void ucg_plan_register(ucg_plan_t *plan)
{
    ucs_assert(plan->type < UCG_PLAN_TYPE_MAX);
    if (!g_plan_pool.initialized) {
        ucg_plan_pool_init(&g_plan_pool);
    }
}

ucg_plan_t* ucg_plan_select_from_cache(ucg_plan_params_t *params)
{
    ucg_plan_t *plan = NULL;
    /*
     for cplan in plan_cache
         if plan->params == params
            plan = ucg_plan_clone(cplan, params)
         else if plan->params and params are different, but within the allowed range
            plan = ucg_plan_clone(cplan, params)
            The latest plan is most likely to be reused, so replace cplan with plan.
      return plan
     */
}

ucg_plan_t* ucg_plan_select_from_template(ucg_plan_params_t *params)
{
    ucg_plan_t *plan = NULL;
    
    /**
      find all available plans
      caculate score, find the best template plan
      plan = ucg_plan_clone(template_plan, params)
      return plan
     */
}

void ucg_plan_add_to_cache(ucg_plan_t *plan)
{

}

ucg_plan_t* ucg_plan_select(ucg_plan_params_t *params)
{
    ucg_plan_t *plan = NULL;
    plan = ucg_plan_select_from_cache(params);
    if (plan != NULL) {
        return plan;
    }
    
    plan = ucg_plan_select_from_template(params);
    if (plan != NULL) {
        ucg_plan_add_to_cache(plan);
    }
    
    return plan;
}

void ucg_plan_release(ucg_plan_t *plan)
{
    return plan->release(plan);
}
