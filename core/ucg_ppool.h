/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_PPOOL_H_
#define UCG_PPOOL_H_

#include <ucg/plan/api.h>

typedef struct ucg_ppool ucg_ppool_t;

/**
 * @ingroup UCG_PPOOL
 * @brief Create a plan pool.
 */
ucs_status_t ucg_ppool_create(ucg_config_t *config, 
                              ucg_ppool_t **ppool);

/**
 * @ingroup UCG_PPOOL
 * @brief Destroy a plan pool.
 */
void ucg_ppool_destroy(ucg_ppool_t *ppool);

/**
 * @ingroup UCG_PPOOL
 * @brief Select a plan.
 */
ucg_plan_t* ucg_ppool_select_plan(ucg_ppool_t *ppool, const ucg_config_t *config, 
                                  const ucg_plan_params_t *params);

/**
 * @ingroup UCG_PPOOL
 * @brief Release a plan.
 */
void ucg_ppool_release_plan(ucg_ppool_t *ppool, ucg_plan_t *plan);

#endif
