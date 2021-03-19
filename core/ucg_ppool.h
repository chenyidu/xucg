/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_PPOOL_H_
#define UCG_PPOOL_H_

#include <ucg/api/ucg.h>
#include <ucg/core/ucg_datatype.h>
#include <ucs/datastruct/ptr_array.h>
#include <ucs/config/parser.h>

typedef struct ucg_plan ucg_plan_t;

/**
 * @ingroup UCG_PLAN
 * @brief Plan type
 */
typedef enum ucg_plan_type {
    UCG_PLAN_TYPE_BCAST,
    UCG_PLAN_TYPE_ALLREDUCE,
    UCG_PLAN_TYPE_BARRIER,
    UCG_PLAN_TYPE_MAX,
} ucg_plan_type_t;

/**
 * @ingroup UCG_PLAN
 * @brief Base structure of plan parameters.
 */
typedef struct ucg_plan_params {
    ucg_plan_type_t type;
    void *config;
    ucg_group_id_t id;
    ucg_group_members_t members;
} ucg_plan_params_t;

/**
 * @ingroup UCG_PLAN
 * @brief Structure of broadcast plan parameters.
 */
typedef struct ucg_plan_bcast_params {
    ucg_plan_params_t super;
    // Here are the user specified parameters.
    void *buffer; 
    int count; 
    ucg_datatype_t *dtype;
    ucg_rank_t root;
} ucg_plan_bcast_params_t;

/**
 * @ingroup UCG_PLAN
 * @brief Structure of allreduce plan parameters.
 */
typedef struct ucg_plan_allreduce_params {
    ucg_plan_params_t super;
    // Here are the user specified parameters.
    const void *sendbuf;
    void *recvbuf;
    int count;
    ucg_datatype_t *dtype;
    ucg_op_t *op;
} ucg_plan_allreduce_params_t;

/**
 * @ingroup UCG_PLAN
 * @brief Select a plan.
 *
 * @param [in] params Parameters.
 */
 ucg_plan_t *ucg_ppool_get_plan(ucg_plan_params_t *params);

/**
 * @ingroup UCG_PLAN
 * @brief Put the plan.
 */
void ucg_ppool_put_plan(ucg_plan_t *plan);

#endif
