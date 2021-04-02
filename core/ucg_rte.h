/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_RTE_H_
#define UCG_RTE_H_

#include <ucg/api/ucg_dt.h>
#include <ucs/type/status.h>
#include <ucs/sys/compiler_def.h>
#include <stdint.h>

/**
 * @ingroup UCG_RTE
 * @brief Define ucg rte operations.
 * 
 * @param _type see @ref ucg_rte_resource_type_t.
 * @param _init Initialization function.
 * @param _cleanup Cleanup function.
*/
#define UCG_RTE_INNER_DEFINE(_type, _init, _cleanup) \
    UCS_STATIC_INIT { \
        ucg_rte_ops[_type].init = _init; \
        ucg_rte_ops[_type].cleanup = _cleanup; \
    }

/**
 * @ingroup UCG_RTE
 * @brief Inner runtime resource type.
 * @note Start from 0, and MAKE SURE the value is continuous.
 */
typedef enum ucg_rte_resource_type {
    UCG_RTE_RESOURCE_TYPE_ACTION,
    UCG_RTE_RESOURCE_TYPE_PLAN,
    UCG_RTE_RESOURCE_TYPE_REQUEST,
    UCG_RTE_RESOURCE_TYPE_CONFIG,
    UCG_RTE_RESOURCE_TYPE_MAX,
} ucg_rte_resource_type_t;

/**
 * @ingroup UCG_RTE
 * @brief Function pointer to initialize the runtime resource.
 */
typedef ucs_status_t (*ucg_rte_init_func_t)();

/**
 * @ingroup UCG_RTE
 * @brief Function pointer to cleanup the runtime resource.
 */
typedef void (*ucg_rte_cleanup_func_t)();

/**
 * @ingroup UCG_RTE
 * @brief Runtime resource operations.
 */
typedef struct ucg_rte_ops {
    ucg_rte_init_func_t init;
    ucg_rte_cleanup_func_t cleanup;
} ucg_rte_ops_t;

extern ucg_rte_ops_t ucg_rte_ops[UCG_RTE_RESOURCE_TYPE_MAX];

uint32_t ucg_rte_dt_is_contig(ucg_datatype_t *dt);

uint64_t ucg_rte_dt_size(ucg_datatype_t *dt);

void* ucg_rte_dt_pack_state(ucg_datatype_t *dt, const void *buffer, uint32_t count);

void* ucg_rte_dt_unpack_state(ucg_datatype_t *dt, void *buffer, uint32_t count);

void ucg_rte_dt_state_finish(void *state);

#endif