/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_PLAN_IMPL_H_
#define UCG_PLAN_IMPL_H_

#include <ucg/plans/base/plan.h>
#include <ucs/type/status.h>
#include <ucs/sys/preprocessor.h>
#include <ucs/datastruct/khash.h>
#include <ucs/datastruct/list.h>

#include <stdint.h>

typedef struct ucg_plan ucg_plan_t;

/* Register a plan to plan pool. */
#define UCG_PPOOL_REGISTER_PLAN(_plan)\
    UCS_STATIC_INIT { \
        ucg_ppool_register_plan(&_plan); \
    }
    UCS_CONFIG_REGISTER_TABLE_ENTRY(&(_plan).core->config_entry)

typedef struct ucg_plan_lru_node {
    ucg_plan_t *plan;
    int used_cnt;
    ucs_list_link_t global_list;
    ucs_list_link_t type_list;
} ucg_plan_lru_node_t;

typedef struct ucg_plan_lru_cache {
    int max_num;
    ucs_list_link_t global_list;
    ucs_list_link_t type_list[UCG_PLAN_TYPE_MAX];
} ucg_plan_lru_cache_t;

KHASH_MAP_INIT_INT64(ucg_ppool, ucg_plan_t*);
/**
 * @ingroup UCG_PLAN
 * @brief Plan pool.
 */
typedef struct ucg_ppool {
    int inited;
    khash_t(ucg_ppool) plans; /* registered plans */
    ucg_plan_lru_cache_t caches;
} ucg_ppool_t;

/**
 * @ingroup UCG_PLAN
 * @brief Register a plan.
 */
void ucg_ppool_register_plan(ucg_plan_t *plan);


ucs_status_t ucg_plan_bcast_clone_params(ucg_plan_bcast_t *plan, 
                                         ucg_plan_bcast_params_t *params);
                                         


#endif
