/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#include "ucg_ppool.h"

ucs_status_t ucg_ppool_create(ucg_config_t *config, 
                              ucg_ppool_t **ppool)
{
    return UCS_OK;
}

void ucg_ppool_destroy(ucg_ppool_t *ppool)
{

}

ucg_plan_t* ucg_ppool_select_plan(ucg_ppool_t *ppool, ucg_config_t *config, 
                                  ucg_plan_params_t *params)
{
    return NULL;
} 

void ucg_ppool_release_plan(ucg_ppool_t *ppool, ucg_plan_t *plan)
{
    
}