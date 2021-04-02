/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_PLAN_API_H_
#define UCG_PLAN_API_H_

#include <ucg/api/ucg.h>
#include <ucg/core/ucg_util.h>
#include <ucs/sys/preprocessor.h>
#include <ucs/sys/compiler_def.h>

/* The ID after 10000 is reserved for x plan. */
#define UCG_PLAN_ID_MAX 10000

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
 * @brief Algorithms of broadcast plans.
 */
typedef enum ucg_plan_bcast_algo {
    UCG_PLAN_BCAST_ALGO_BTREE, /* binomial tree */
    UCG_PLAN_BCAST_ALGO_KTREE, /* knomial tree */
    UCG_PLAN_BCAST_ALGO_MAX,
} ucg_plan_bcast_algo_t;
UCG_STATIC_ASSERT(UCG_PLAN_BCAST_ALGO_MAX <= UCG_PLAN_ID_MAX);

/**
 * @ingroup UCG_PLAN
 * @brief Algorithms of allredece plans.
 */
typedef enum ucg_plan_allreduce_algo {
    UCG_PLAN_ALLREDUCE_ALGO_RD,  /* recursive doubling */
    UCG_PLAN_ALLREDUCE_ALGO_MAX,
} ucg_plan_allreduce_id_t;
UCG_STATIC_ASSERT(UCG_PLAN_ALLREDUCE_ALGO_MAX <= UCG_PLAN_ID_MAX);

/**
 * @ingroup UCG_PLAN
 * @brief Flags of plan's capabilities.
 */
typedef enum ucg_plan_cap_flag {
    UCG_PLAN_CAP_FLAG_TOPO_AWARE = UCS_BIT(0), /* Topo-aware plan. */
    UCG_PLAN_CAP_FLAG_SWITCH_ROOT = UCS_BIT(1), /* Support swicthing root. */
} ucg_plan_cap_flag_t;

/**
 * @ingroup UCG_PLAN
 * @brief The result of the comparison between the two plan parameters. 
 * 
 * Advice comes from 
 * 1. User parameter comparison,
 * 2. Configuration comparison,
 * 3. Agreed rules
 */
typedef enum ucg_plan_clone_advice {
    UCG_PLAN_CLONE_ADVICE_SAME, /* Everything is same, may reuse the origin plan directly. */
    UCG_PLAN_CLONE_ADVICE_SIMILAR, /* Non-critical parts are different. */
    UCG_PLAN_CLONE_ADVICE_UNEQUAL, /* Critical parts are different, not clone but creating a new one. */
} ucg_plan_clone_advice_t;

/**
 * @ingroup UCG_PLAN
 * @brief Plan handle.
 *
 * A plan can have many handles, just like a file can have many file handles.
 * Plan handle is stateful, just like each file handle contains its offset.
 * The value of a valid handle is greater than 0. It is safe to operate different 
 * handles of the same plan in parallel.
 */
typedef uint64_t ucg_plan_handle_t;

typedef int ucg_plan_algo_t;
/**
 * @ingroup UCG_PLAN
 * @brief Unique identification of a plan.
 */
typedef struct ucg_plan_id {
    ucg_plan_type_t type;
    ucg_plan_algo_t algo;
} ucg_plan_id_t;

/**
 * @ingroup UCG_PLAN
 * @brief Base structure of plan parameters.
 */
typedef struct ucg_plan_params {
    ucg_plan_type_t type;
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
 * @brief Structure of allreduce plan parameters.
 */
typedef struct ucg_plan_barrier_params {
    ucg_plan_params_t super;
} ucg_plan_barrier_params_t;

typedef struct ucg_plan ucg_plan_t;

/**
 * @ingroup UCG_PLAN
 * @brief Plan type string.
 */
const char* ucg_plan_type_str(ucg_plan_type_t type);

/**
 * @ingroup UCG_PLAN
 * @brief Create a new plan.
 * 
 * @param [in] id Plan ID.
 * @param [in] config Plan's configuration.
 * @param [in] params Plan's parameters.
*/
ucg_plan_t* ucg_plan_create(const ucg_plan_id_t *id, const ucg_config_t *config, 
                            const ucg_plan_params_t *params);

/**
 * @ingroup UCG_PLAN
 * @brief Clone a plan.
 * 
 * This routine create a new plan according to @b plan. Because some part of the 
 * plan can directly copy from the original plan, it can reduce some overhead of 
 * creating a totally new plan.
 * 
 * @param [in] plan original plan.
 * @param [in] config Plan's configuration.
 * @param [in] params Plan's parameters.
 * @param [in] advice Advice on how to clone.
*/
ucg_plan_t* ucg_plan_clone(ucg_plan_t *plan, const ucg_config_t *config, 
                           const ucg_plan_params_t *params, 
                           ucg_plan_clone_advice_t advice);

/**
 * @ingroup UCG_PLAN
 * @brief Free a plan.
 * 
 * This routine free the plan object returned by @ref ucg_plan_create() and @ref
 * ucg_plan_clone().
 */
void ucg_plan_free(ucg_plan_t *plan);

/**
 * @ingroup UCG_PLAN
 * @brief Open a plan handle.
 * 
 * @param [in] plan 
 * @param [in] gid Group ID.
 * @param [in] rid Request ID.
 * @return > 0 is valid, 0 is invalid.
 */
ucg_plan_handle_t ucg_plan_open(ucg_plan_t *plan, ucg_group_id_t gid, uint32_t rid);

/**
 * @ingroup UCG_PLAN
 * @brief Open a plan to execute.
 * 
 * @return UCS_OK All actions in this plan are done.
 * @return UCS_INPROGRESS Some conditions have not been met, so the execution 
 *                        cannot continue, try it later.
 * @return Otherwise Failed.
 */
ucs_status_t ucg_plan_execute(ucg_plan_handle_t handle);

/**
 * @ingroup UCG_PLAN
 * @brief Close a plan handle.
 *
 * When execution is done or failed, it's need to invoke this routine to close 
 * the handle.
 */
void ucg_plan_close(ucg_plan_handle_t handle);

/**
 * @ingroup UCG_PLAN
 * @brief Free a plan.
 * 
 * Print plan's information, just for debug purpose.
 */
void ucg_plan_print(ucg_plan_t *plan, FILE *stream);

/**
 * @ingroup UCG_PLAN
 * @brief Query plan's type. 
 */
ucg_plan_type_t ucg_plan_type(ucg_plan_t *plan);

#endif
