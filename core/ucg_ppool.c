/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#include "ucg_ppool.h"
#include "ucg_config.h"

#include <ucg/plan/api.h>
#include <ucs/config/parser.h>

typedef UCS_CONFIG_STRING_ARRAY_FIELD(policy) ucg_config_policy_array_t;

typedef struct ucg_ppool_config {
    uint32_t plan_cache_size;
    char *policy_file;
    ucg_config_policy_array_t bcast; 
    ucg_config_policy_array_t allreduce;
    ucg_config_policy_array_t barrier;  
} ucg_ppool_config_t;

#define UCG_PPOOL_CONFIG_POLICY_DOC(_type) \
    "Control "#_type" algorithm selection, the order makes sense.\n" \
    "Syntax: <algo>[@l:u][#l:u]\n" \
    "\t[@l:u] indecates the range of message size.\n" \
    "\t[#l:u] indecates the range of group size.\n" \
    "\tAny nagetive 'l' or 'u' stands for infinity, " \
    "[] is optional, two option has 'and' relationship.\n" \
    "Example: \n" \
    "\t - 1@10:100 Using algo 1 when message size is in [10, 100).\n"

static ucs_config_field_t g_ppool_config_table[] = {
    {"PLAN_CACHE_SIZE", "300", "Specify the size of plan cache.", 
     ucs_offsetof(ucg_ppool_config_t, plan_cache_size), UCS_CONFIG_TYPE_UINT},
    {"POLICY_FILE", "", "Specify a file that contains plan selection policy. "
    "Using JSON format.",
     ucs_offsetof(ucg_ppool_config_t, policy_file), UCS_CONFIG_TYPE_STRING},
    {"BCAST_PLAN", "", UCG_PPOOL_CONFIG_POLICY_DOC(bcast),
     ucs_offsetof(ucg_ppool_config_t, bcast), 
     UCS_CONFIG_TYPE_STRING_ARRAY},
     {"ALLREDUCE_PLAN", "", UCG_PPOOL_CONFIG_POLICY_DOC(allreduce),
      ucs_offsetof(ucg_ppool_config_t, allreduce), UCS_CONFIG_TYPE_STRING_ARRAY},
     {"BARRIER_PLAN", "", UCG_PPOOL_CONFIG_POLICY_DOC(barrier),
     ucs_offsetof(ucg_ppool_config_t, barrier), UCS_CONFIG_TYPE_STRING_ARRAY},
    {NULL},
};
UCG_CONFIG_REGISTER_TABLE(g_ppool_config_table, "Plan Pool", NULL, ucg_ppool_config_t);

static ucs_status_t ucg_ppool_init_policy(ucg_ppool_t *ppool, 
                                          ucg_ppool_config_t *config)
{
    // TODO

    /* Initialize the selection policy for a specific collection operation that
     * has higer priority. */
    

    /* Initialize a decisiton tree for selecting plan. */

    return UCS_OK;
}

static void ucg_ppool_cleanup_policy(ucg_ppool_t *ppool)
{
    // TODO
}

static ucs_status_t ucg_ppool_init_cache(ucg_ppool_t *ppool, 
                                         ucg_ppool_config_t *config)
{
    // TODO
    return UCS_OK;
}

static void ucg_ppool_cleanup_cache()
{
    // TODO
}

static ucg_plan_t *ucg_ppool_lookup_cache(ucg_ppool_t *ppool, 
                                          const ucg_plan_params_t *params)
{
    // TODO
    return NULL;
}

static ucs_status_t ucg_ppool_select_algo(ucg_ppool_t *ppool, 
                                          const ucg_plan_params_t *params,
                                          ucg_plan_id_t *id)
{
    // TODO
    id->type = params->type;
    id->algo = UCG_PLAN_BCAST_ALGO_KTREE;
    return UCS_OK;
}

static ucs_status_t ucg_ppool_add2cache(ucg_ppool_t *ppool, ucg_plan_t *plan)
{
    // TODO
    return UCS_OK;
}

ucs_status_t ucg_ppool_create(ucg_config_t *config, 
                              ucg_ppool_t **ppool_p)
{
    ucs_status_t status = UCS_OK;
    ucg_ppool_config_t *ppool_config = UCG_CONFIG_CONVERT(config, ucg_ppool_config_t);
    ucg_ppool_t *ppool = ucs_malloc(sizeof(ucg_ppool_t), "ucg ppool");
    if (ppool == NULL) {
        return UCS_ERR_NO_MEMORY;
    }
    
    status = ucg_ppool_init_policy(ppool, ppool_config);
    if (status != UCS_OK) {
        goto err_free_ppool;
    }

    status = ucg_ppool_init_cache(ppool, ppool_config);
    if (status != UCS_OK) {
        goto err_cleanup_policy;
    }

    *ppool_p = ppool;
    return UCS_OK;
err_cleanup_policy:
    ucg_ppool_cleanup_policy(ppool);
err_free_ppool:
    ucs_free(ppool);
    return status;
}

void ucg_ppool_destroy(ucg_ppool_t *ppool)
{
    ucg_ppool_cleanup_cache(ppool);
    ucg_ppool_cleanup_policy(ppool);
    ucs_free(ppool);
    return;
}

ucg_plan_t* ucg_ppool_select_plan(ucg_ppool_t *ppool, const ucg_config_t *config, 
                                  const ucg_plan_params_t *params)
{
    ucg_plan_t *plan = NULL;
    plan = ucg_ppool_lookup_cache(ppool, params);
    if (plan != NULL) {
        return plan;
    }

    ucg_plan_id_t id;
    ucs_status_t status = ucg_ppool_select_algo(ppool, params, &id);
    if (status != UCS_OK) {
        ucs_error("Failed to select plan, %s.", ucs_status_string(status));
        return NULL;
    }

    plan = ucg_plan_create(&id, config, params);
    if (plan == NULL) {
        ucs_error("Failed to create plan.");
        return NULL;
    }
    ucg_ppool_add2cache(ppool, plan);
    return plan;
} 

void ucg_ppool_release_plan(ucg_ppool_t *ppool, ucg_plan_t *plan)
{
    // TODO
    return;
}