/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#include "ucg_ppool.h"
#include "ucg_config.h"

#include <ucs/config/parser.h>

typedef UCS_CONFIG_STRING_ARRAY_FIELD(strategy) ucg_config_stragety_array_t;

typedef struct ucg_ppool_config {
    ucg_config_stragety_array_t bcast; 
    ucg_config_stragety_array_t allreduce;
    ucg_config_stragety_array_t barrier;  
} ucg_ppool_config_t;

static ucs_config_field_t g_ppool_config_table[] = {
    {"BCAST_PLAN", "", "Control bcast algorithm selection.\n"
     "Syntax: <algo>[@lower:upper][#lower:upper] \n"
     "[@l:u] indecates the range of message size.\n"
     "[#l:u] indecates the range of group size.\n"
     "Example: 1@10:100#0:8 means when message size is in [10, 100) and group "
     "size is in [0, 8), using algorithm 1.", ucs_offsetof(ucg_ppool_config_t, bcast), 
     UCS_CONFIG_TYPE_STRING_ARRAY},
     {"ALLREDUCE_PLAN", "", "Control allreduce algorithm selection.\n"
     "Syntax: <algo>[@lower:upper][#lower:upper] \n"
     "[@l:u] indecates the range of message size.\n"
     "[#l:u] indecates the range of group size.\n"
     "Example: 1@10:100#0:8 means when message size is in [10, 100) and group "
     "size is in [0, 8), using algorithm 1.", ucs_offsetof(ucg_ppool_config_t, bcast), 
     UCS_CONFIG_TYPE_STRING_ARRAY},
     {"BARRIER_PLAN", "", "Control barrier algorithm selection.\n"
     "Syntax: <algo>[#lower:upper] \n"
     "[#l:u] indecates the range of group size.\n"
     "Example: 1#0:8 means when group size is in [0, 8), using algorithm 1.", 
     ucs_offsetof(ucg_ppool_config_t, bcast), UCS_CONFIG_TYPE_STRING_ARRAY},
    {NULL},
};
UCG_CONFIG_REGISTER_TABLE(g_ppool_config_table, "Plan Pool", NULL, ucg_ppool_config_t);

ucs_status_t ucg_ppool_create(ucg_config_t *config, 
                              ucg_ppool_t **ppool)
{

    return UCS_OK;
}

void ucg_ppool_destroy(ucg_ppool_t *ppool)
{

}

ucg_plan_t* ucg_ppool_select_plan(ucg_ppool_t *ppool, const ucg_config_t *config, 
                                  const ucg_plan_params_t *params)
{
    return NULL;
} 

void ucg_ppool_release_plan(ucg_ppool_t *ppool, ucg_plan_t *plan)
{
    
}