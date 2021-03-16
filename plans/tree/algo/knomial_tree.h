/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_PLAN_KTREE_H_
#define UCG_PLAN_KTREE_H_

#include "tree.h"
#include <ucg/api/ucg_def.h>
#include <ucs/type/status.h>
#include <stdint.h>

typedef struct ucg_plan_ktree_params {
    ucg_rank_t self;
    ucg_rank_t root;
    uint32_t size;
    uint32_t degree;
} ucg_plan_ktree_params_t;

/**
 * @ingroup UCG_PLAN
 * @brief Get the tree node from a left btree.
 * 
 * For 8 members, the left ktree is as follows
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
ucs_status_t ucg_plan_ktree_left(const ucg_plan_ktree_params_t *params, 
                                 ucg_plan_tree_node_t *node);

/**
 * @ingroup UCG_PLAN
 * @brief Get the tree node from a right btree.
 * 
 * For 8 members, the right ktree is as follows
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
ucs_status_t ucg_plan_ktree_right(const ucg_plan_ktree_params_t *params, 
                                  ucg_plan_tree_node_t *node);
#endif