/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_GROUP_H_
#define UCG_GROUP_H_

#include <ucs/datastruct/list.h>

typedef struct ucg_group {
    ucs_list_link_t list; /* group list in context. */
} ucg_group_t;

#endif