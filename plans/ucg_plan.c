/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#include "ucg_plan.h"
#include "ucg_plan_derived.h"
#include <ucs/debug/assert.h>

typedef struct ucg_plan_template_mgr {
    int initialized;
    ucs_ptr_array_t plan_templates[UCG_PLAN_TYPE_MAX];
} ucg_plan_template_mgr_t;

typedef struct ucg_plan_pool {
    ucs_ptr_array_t plans;
} ucg_plan_pool_t;

static ucg_plan_template_mgr_t g_pt_mgr = {.initialized = 0};

void ucg_plan_register_template(ucg_plan_template_t *pt)
{
    if (!g_pt_mgr.initialized) {
        int i;
        for (i = 0; i < UCG_PLAN_TYPE_MAX; ++i) {
            ucs_ptr_array_init(&g_pt_mgr.plan_templates[i], "plan template");
        }
        g_pt_mgr.initialized = 1;
    }
    ucs_ptr_array_insert(&g_pt_mgr.plan_templates[pt->type], pt);
    return;
}

ucs_status_t ucg_plan_config_read((const char *env_prefix, 
                                  const char *filename,
                                  void **config)
{


}

ucs_status_t ucg_plan_config_read(const char *env_prefix, 
                                  const char *filename,
                                  ucs_plan_config_t **config)
{
    ucs_ptr_array_t *config_array = ucs_malloc(sizeof(ucs_ptr_array_t));

    int i;
    int j;
    ucg_plan_template_t *pt;
    for (i = 0; i < UCG_PLAN_TYPE_MAX; ++i) {
        ucs_ptr_array_for_each (pt, j, g_pt_mgr.plan_templates[i]) {
            ucs_plan_config_t *config;
            pt->config.read(env_prefix, filename, &config);
        }
    }
    
}

ucs_status_t ucg_plan_select(ucg_plan_pool_t *plan_pool, 
                             ucg_plan_params_t *params, 
                             ucg_plan_t **plan)
{

}
