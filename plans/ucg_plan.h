/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_PLAN_H_
#define UCG_PLAN_H_

#include <ucg/api/ucg_dt.h>
#include <ucs/datastruct/ptr_array.h>

typedef void ucg_plan_config_t;

typedef enum ucg_plan_type {
    UCG_PLAN_TYPE_BCAST,
    UCG_PLAN_TYPE_ALLREDUCE,
    UCS_PLAN_TYPE_BARRIER,
    UCG_PLAN_TYPE_MAX,
} ucg_plan_type_t;

typedef struct ucg_plan_pool {
    ucs_ptr_array_t plans;
} ucg_plan_pool_t;

/**
 * @ingroup UCG_PLAN
 * @brief Base structure of plan parameters.
 */
typedef struct ucg_plan_params {
    ucg_plan_type_t type;
    int count; /* Number of elements in handles. */
    uint64_t *handles; /* Handles of communication members. */
} ucg_plan_params_t;

/**
 * @ingroup UCG_PLAN
 * @brief Structure of broadcast plan parameters.
 */
typedef struct ucg_plan_bcast_params {
    ucg_plan_params_t super;
    void *buffer; 
    int count; 
    ucg_datatype_t *dtype;
    int root;
} ucg_plan_bcast_params_t;

/**
 * @ingroup UCG_PLAN
 * @brief Structure of allreduce plan parameters.
 */
typedef struct ucg_plan_allreduce_params {
    ucg_plan_params_t super;
    const void *sendbuf;
    void *recvbuf;
    int count;
    ucg_datatype_t *dtype;
    ucg_op_t *op;
} ucg_plan_allreduce_params_t;

/**
 * @ingroup UCG_PLAN
 * @brief Structure of barrier plan parameters.
 */
typedef struct ucg_plan_barrier_params {
    ucg_plan_params_t super;
    /* Barrier has no special parameters. */
} ucg_plan_barrier_params_t;

typedef struct ucg_plan ucg_plan_t;

/**
 * @ingroup UCG_PLAN
 * @brief Select a plan from plan pool.
 *
 * @param [in] plan_pool Plan pool.
 * @param [in] params Parameters.
 * @param [out] plan Selected plan.
 */
 ucs_status_t ucg_plan_select(ucg_plan_pool_t *plan_pool, 
                              ucg_plan_params_t *params, 
                              ucg_plan_t **plan);

/**
 * @ingroup UCG_PLAN
 * @breif Release a plan
 *
 * @param [in] plan Plan returned by ucg_plan_select().
 */
void ucg_plan_release(ucg_plan_t *plan);

ucs_status_t ucg_plan_config_read(const char *env_prefix, 
                                  const char *filename, 
                                  ucg_plan_config_t *config);

#endif
