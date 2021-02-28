/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_PLAN_KTREE_H_
#define UCG_PLAN_KTREE_H_

ucs_status_t ucg_plan_ktree_right_most_actions(uin32_t rank, 
                                               uint32_t root, 
                                               uint32_t member_cnt, 
                                               uint32_t degree, 
                                               ucg_plan_action_t* head);

ucs_status_t ucg_plan_ktree_left_most_actions(uin32_t rank, 
                                              uint32_t root, 
                                              uint32_t member_cnt, 
                                              uint32_t degree, 
                                              ucg_plan_action_t* head);
#endif