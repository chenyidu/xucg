#include "binomial_tree.h"
#include <ucs/debug/assert.h>

ucs_status_t ucg_plan_btree_left(const ucg_plan_btree_params_t *params, 
                                 ucg_plan_btree_node_t *node)
{
    ucs_assert(params != NULL && node->child != NULL);
    
    uint32_t self = params->self;
    uint32_t root = params->root;
    uint32_t size = params->size;
    ucs_assert(self < size && root < size);

    /* Adjust the position of members and put root at the front, 
       {left, root, right} to {root, right, left} */
    uint32_t nself = (self - root + size) % size;

    uint32_t mask = 1;
    for (uint32_t i = nself; i > 0; i >>= 1) {
        mask <<= 1;
    }

    /* find father on my left hand. */
    uint32_t remote = 0;
    if (root == self) {
        node->father_cnt = 0;
    } else {
        node->father = (nself ^ (mask >> 1) + root) % size;
        node->father_cnt = 1;
    }

    /* find child on my right hand. */
    uint32_t tmp = 0;
    uint32_t cnt = 0;
    uint32_t max_cnt = node->child_cnt;
    while(mask < size) {
        tmp = nself ^ mask;
        if (tmp >= size) {
            break;
        }
        node->child[cnt++] = (tmp + root) % size;
        if (cnt == max_cnt) {
            return UCS_ERR_BUFFER_TOO_SMALL;
        }
        mask <<= 1;
    }
    node->child_cnt = cnt;
    return UCS_OK;
}


ucs_status_t ucg_plan_btree_right(const ucg_plan_btree_params_t *params, 
                                  ucg_plan_btree_node_t *node)
{
    ucs_assert(params != NULL &&  node->child != NULL);

    uint32_t self = params->self;
    uint32_t root = params->root;
    uint32_t size = params->size;
    ucs_assert(self < size && root < size);

     /* Adjust the position of members and put root at the front, 
       {left, root, right} to {root, right, left} */
    uint32_t nself = (self - root + size) % size;
    if (root == self) {
        node->father_cnt = 0;
    }

    uint32_t cnt = 0;
    uint32_t max_cnt = node->child_cnt;
    uint32_t mask = 1;
    uint32_t tmp = 0;
    while (mask < size) {
        tmp = nself ^ mask;
        if (tmp < nself) {
            node->father = (tmp + root) % size;
            node->father_cnt = 1;
            break;
        }
        if (tmp < size) {
            node->child[cnt++] = (tmp + root) % size;
            if (cnt == max_cnt) {
                return UCS_ERR_BUFFER_TOO_SMALL;
            }
        }
        mask <<= 1;
    }
    node->child_cnt = cnt;
    return UCS_OK;
}