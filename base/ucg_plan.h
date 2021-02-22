/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_PLAN_H_
#define UCG_PLAN_H_

typedef enum ucg_plan_type {
    UCG_PLAN_TYPE_BCAST,
    UCG_PLAN_TYPE_ALLREDUCE,
    UCS_PLAN_TYPE_BARRIER,
    UCG_PLAN_TYPE_MAX,
} ucg_plan_type_t;

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
 * @brief Select a plan.
 *
 * @params [in] params Pointer to a derived class of ucg_plan_params_t.
 * @return A plan pointer, NULL means no available plan.
 */
ucg_plan_t* ucg_plan_select(ucg_plan_params_t *params);

/**
 * @ingroup UCG_PLAN
 * @breif Release a plan
 *
 * @param [in] plan Plan returned by ucg_plan_select().
 */
void ucg_plan_release(ucg_plan_t *plan);

#endif
