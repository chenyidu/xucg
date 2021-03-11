/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_PLAN_BTREE_H_
#define UCG_PLAN_BTREE_H_

#include <ucs/type/status.h>
#include <stdint.h>

typedef struct ucg_plan_btree_node {
    int father;
    int father_cnt;
    int *child;
    int child_cnt; /* Should be initialized to indicate the max size of down. */
} ucg_plan_btree_node_t;

typedef struct ucg_plan_btree_params {
    int self;
    int root;
    uint32_t size;
} ucg_plan_btree_params_t;

/**
 * @ingroup UCG_PLAN
 * @brief Get the tree node from a left btree.
 * 
 * For 8 members, the left btree is as follows
 *             0
 *           / | \
 *          1  2  4
 *        / |  |
 *       3  5  6
 *       |
 *       7
 * @param [in] params 
 * @param [inout] node
 */
ucs_status_t ucg_plan_btree_left(const ucg_plan_btree_params_t *params, 
                                 ucg_plan_btree_node_t *node);

/**
 * @ingroup UCG_PLAN
 * @brief Get the tree node from a right btree.
 * 
 * For 8 members, the right btree is as follows
 *             0
 *           / | \
 *          1  2  4
 *             |  | \
 *             3  5  6
 *                   |
 *                   7
 * @param [in] params 
 * @param [inout] node
 */
ucs_status_t ucg_plan_btree_right(const ucg_plan_btree_params_t *params, 
                                  ucg_plan_btree_node_t *node);
#endif