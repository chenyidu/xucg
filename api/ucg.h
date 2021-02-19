/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
#ifndef UCG_H_
#define UCG_H_

#include <ucg/api/ucg_mpi.h>

/**
 * @ingroup UCG_RTE
 * @brief UCG Runtime Enviroment Type.
 */
typedef enum ucg_rte_type {
    UCG_RTE_TYPE_MPI,
    UCG_RTE_TYPE_LAST,
} ucg_rte_type_t;

typedef enum ucg_group_params_field {
    UCG_GROUP_PARAMS_UCP_WORKER = UCS_BIT(0),
    UCG_GROUP_PARAMS_GROUP = UCS_BIT(1),
} ucg_group_params_field_t;

/**
 * @ingroup UCG_RTE
 * @brief Tuning UCG Runtime Enviroment.
 */
typedef struct ucg_rte_params {
    ucg_rte_type_t type;
    union {
        ucg_rte_mpi_t mpi;
    };
} ucg_rte_params_t;

/**
 * @ingroup UCG_CONTEXT
 * @brief Creation parameters for the UCG context.
 */
typedef struct ucg_context_params {
    uint64_t field_mask;
} ucg_context_params_t;

/**
 * @ingroup UCG_GROUP
 * @brief Creation parameters for the UCG group.
 */
typedef struct ucg_group_params {
    /**
     * Mask of valid fields in this structure, using bits from @ref ucg_group_params_field.
     * Fields not specified in this mask will be ignored.
     * Provides ABI compatibility with respect to adding new fields.
     */
    uint64_t field_mask;
    
    /* Specified worker */
    ucp_worker_h ucp_worker;
    
    struct {
        int count; /* Number of element in the handles */
        uint64_t *handles; /* Array of user-defined process handle */
        int offset; /* My offset position in the handles array */
    } group;
} ucg_group_params_t;

/**
 * @ingroup UCG_RTE
 * @brief UCG runtime enviroment initialization.
 */
ucs_status_t ucg_rte_init(ucg_rte_params_t *params);

/**
 * @ingroup UCG_CONTEXT
 * @brief UCG context initialization.
 */
ucs_status_t ucg_context_init(const ucg_context_params_t *params, 
                              const ucg_context_config_t *config,
                              ucg_context_h *context_p);
/**
 * @ingroup UCG_CONTEXT
 * @brief Release UCG application context.
 */
void ucg_context_cleanup(ucg_context_h context_p);

#endif
