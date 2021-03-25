/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_CONTEXT_H_
#define UCG_CONTEXT_H_

#include "ucg_ppool.h"
#include <ucs/datastruct/list.h>

typedef struct ucg_context {
    ucs_list_link_t group_head;
    ucg_ppool_t *ppool;
    ucg_config_t *config;
} ucg_context_t;

#endif