/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_RTE_H_
#define UCG_RTE_H_

uint32_t ucg_rte_dt_size(ucg_datatype_t *dt);
void* ucg_rte_dt_state(ucg_datatype_t *dt, void *buffer, uint32_t count, int is_pack);

#endif