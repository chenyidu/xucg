
ucs_status_t ucg_plan_tree_add_fanout_actions(ucg_plan_t *plan,
                                              ucg_plan_tree_node_t *node)
{
    ucs_status_t status = UCS_OK;
    ucs_assert(node->father_cnt == 0 || node->father_cnt == 1);
    if (node->father_cnt == 1) {
        status = ucg_plan_create_and_append_action(UCG_PLAN_ACTION_TYPE_RECV, 1, 
                                     &node->father, action_head);
        if (status != UCS_OK) {
            goto clear_actions;
        }
    }
    
    int child_cnt = node->child_cnt;
    if (child_cnt > 0) {
        ucg_plan_action_type_t type = node->father_cnt == 0 ? UCG_PLAN_ACTION_TYPE_SEND : UCG_PLAN_ACTION_TYPE_FORWARD;
        status = ucg_plan_add_action(type, child_cnt, node->child, action_head);
        if (status != UCS_OK) {
            goto clear_actions;
        }
    }

    if (node->father_cnt == 1) {
        /* Let COPY after FORWARD, so that COPY and FORWARD may be in parallel.
           FORWARD use NIC, COPY use CPU. */
        status = ucg_plan_add_action(UCG_PLAN_ACTION_TYPE_COPY, 1, 
                                     &node->father, action_head);
        if (status != UCS_OK) {
            goto clear_actions;
        }
    }

    return UCS_OK;
clear_actions:
    ucg_plan_release_actions(plan);
    return status;
}