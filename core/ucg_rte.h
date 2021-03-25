/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_RTE_H_
#define UCG_RTE_H_

#include <ucg/api/ucg_dt.h>
#include <stdint.h>

uint32_t ucg_rte_dt_is_contig(ucg_datatype_t *dt);

uint64_t ucg_rte_dt_size(ucg_datatype_t *dt);

void* ucg_rte_dt_state(ucg_datatype_t *dt, void *buffer, uint32_t count, int is_pack);

void ucg_rte_dt_state_finish(void *state);

#endif