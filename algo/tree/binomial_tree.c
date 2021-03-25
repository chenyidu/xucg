#include "tree.h"

#include <ucs/debug/assert.h>

#include <stddef.h>

ucs_status_t ucg_algo_btree_left(const ucg_plan_btree_params_t *params, 
                                 ucg_plan_tree_node_t *node)
{
    ucs_assert(params != NULL && node->child != NULL);
    
    ucg_rank_t self = params->self;
    ucg_rank_t root = params->root;
    uint32_t size = params->size;
    ucs_assert(self < size && root < size);

    /* Adjust the position of members and put root at the front, 
       {left, root, right} to {root, right, left} */
    ucg_rank_t vself = (self - root + size) % size;

    uint32_t mask = 1;
    for (uint32_t i = vself; i > 0; i >>= 1) {
        mask <<= 1;
    }

    /* find father on my left hand. */
    if (root == self) {
        node->father_cnt = 0;
    } else {
        node->father = ((vself ^ (mask >> 1)) + root) % size;
        node->father_cnt = 1;
    }

    /* find child on my right hand. */
    ucg_rank_t vrank = 0;
    uint32_t cnt = 0;
    uint32_t max_cnt = node->child_cnt;
    while(mask < size) {
        vrank = vself ^ mask;
        if (vrank >= size) {
            break;
        }
        if (cnt == max_cnt) {
            return UCS_ERR_BUFFER_TOO_SMALL;
        }
        node->child[cnt++] = (vrank + root) % size;
        mask <<= 1;
    }
    node->child_cnt = cnt;
    return UCS_OK;
}


ucs_status_t ucg_algo_btree_right(const ucg_plan_btree_params_t *params, 
                                  ucg_plan_tree_node_t *node)
{
    ucs_assert(params != NULL &&  node->child != NULL);

    ucg_rank_t self = params->self;
    ucg_rank_t root = params->root;
    uint32_t size = params->size;
    ucs_assert(self < size && root < size);

     /* Adjust the position of members and put root at the front, 
       {left, root, right} to {root, right, left} */
    ucg_rank_t vself = (self - root + size) % size;
    if (root == self) {
        node->father_cnt = 0;
    }

    uint32_t cnt = 0;
    uint32_t max_cnt = node->child_cnt;
    uint32_t mask = 1;
    ucg_rank_t vrank = 0;
    while (mask < size) {
        vrank = vself ^ mask;
        if (vrank < vself) {
            // The first rank on my left hand is my father node.
            node->father = (vrank + root) % size;
            node->father_cnt = 1;
            break;
        }
        if (vrank < size) {
            if (cnt == max_cnt) {
                return UCS_ERR_BUFFER_TOO_SMALL;
            }
            node->child[cnt++] = (vrank + root) % size;
        }
        mask <<= 1;
    }
    node->child_cnt = cnt;
    return UCS_OK;
}