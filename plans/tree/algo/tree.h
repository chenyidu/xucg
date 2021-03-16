/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_PLAN_TREE_H_
#define UCG_PLAN_TREE_H_

typedef struct ucg_plan_tree_node {
    ucg_rank_t father;
    uint32_t father_cnt;
    ucg_rank_t *child;
    uint32_t child_cnt; /* Should be initialized to indicate the max size of down. */
} ucg_plan_tree_node_t;


#endif