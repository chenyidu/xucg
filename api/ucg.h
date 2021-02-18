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
    UCG_RTE_TYPE_INVALID,
} ucg_rte_type_t;

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
 * @ingroup UCG_RTE
 * @brief UCG Runtime Enviroment initialization.
 */
ucs_status_t ucg_rte_init(ucg_rte_params_t *params);

typedef enum ucg_dt_type_id {
    UCG_DT_UINT32,
    UCG_DT_MPI,
    UCG_DT_MAX,
} ucg_dt_type_t;

typedef struct ucg_datatype {
    ucg_dt_type_t id;
    void *dt_ptr;
} ucg_datatype_t;
#endif
