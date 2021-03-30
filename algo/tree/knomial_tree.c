#include "tree.h"

#include <ucs/debug/assert.h>

#include <stddef.h>

ucs_status_t ucg_algo_ktree_left(const ucg_plan_ktree_params_t *params, 
                                 ucg_plan_tree_node_t *node)
{
    ucs_assert(params != NULL && node->child != NULL);
    
    ucg_rank_t self = params->self;
    ucg_rank_t root = params->root;
    uint32_t size = params->size;
    uint32_t degree = params->degree;
    ucs_assert(self < size && root < size);

    ucg_rank_t vself = (self - root + size) % size;
    uint32_t mask = 1;
    if (root == self) {
        node->father_cnt = 0;
    } 

    while (mask < size) {
        if (vself % (degree * mask)) {
            node->father = vself / (degree * mask) * (degree * mask);
            node->father = (node->father + root) % size;
            node->father_cnt = 1;
            break;
        }
        mask *= degree;
    }
    
    mask /= degree;
    uint32_t cnt = 0;
    uint32_t max_cnt = node->child_cnt;
    uint32_t vrank = 0;
    while(mask > 0) {
       for (int i = 1; i < degree; ++i) {
           vrank = vself + mask * i;
           if (vrank < size) {
               vrank = (vrank + root) % size;
               if (cnt == max_cnt) {
                    return UCS_ERR_BUFFER_TOO_SMALL;
                }
               node->child[cnt++] = vrank;
           }
       }
       mask /= degree;
    }
    node->child_cnt = cnt;
    return UCS_OK;
}

ucs_status_t ucg_algo_ktree_right(const ucg_plan_ktree_params_t *params, 
                                  ucg_plan_tree_node_t *node)
{
    ucs_assert(params != NULL && node->child != NULL);
    
    ucg_rank_t self = params->self;
    ucg_rank_t root = params->root;
    uint32_t size = params->size;
    uint32_t degree = params->degree;
    ucs_assert(self < size && root < size);

    ucg_rank_t vself = (self - root + size) % size;
    uint32_t mask = 1;
    if (root == self) {
        node->father_cnt = 0;
    }

    while (mask < size) {
        if (vself % (degree * mask)) {
            node->father = vself / (degree * mask) * (degree * mask);
            node->father = (node->father + root) % size;
            node->father_cnt = 1;
            break;
        }
        mask *= degree;
    } 

    mask /= degree;
    uint32_t cnt = 0;
    uint32_t max_cnt = node->child_cnt;
    uint32_t vrank = 0;
    while(mask > 0) {
       for (int i = 1; i < degree; ++i) {
           vrank = vself + mask * i;
           if (vrank < size) {
               vrank = (vrank + root) % size;
               if (cnt == max_cnt) {
                    return UCS_ERR_BUFFER_TOO_SMALL;
                }
               node->child[cnt++] = vrank;
           }
       }
       mask /= degree;
    }
    node->child_cnt = cnt;
    // change the order of children from leftmost to rightmost
    for (int i = 0; i < cnt/2; ++i) {
        vrank = node->child[i];
        node->child[i] = node->child[cnt-1-i];
        node->child[cnt-1-i] = vrank;
    }
    return UCS_OK;
}