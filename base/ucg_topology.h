/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_TOPOLOGY_H_
#define UCG_TOPOLOGY_H_
 
/**
 * @ingroup UCG_TOPOLOGY
 * @brief Get distance between two members.
 *
 * @param [in] group UCG group object
 * @param [in] member_id1 Position of member in the array of group handles.
 * @param [in] member_id2 Position of member in the array of group handles.
 */
ucs_status_t ucg_topo_get_distance(ucg_group_h group, int member_id1, int member_id2);

/**
 * @ingroup UCG_TOPOLOGY
 * @brief Group members by distance.
 *
 * @param [in] group UCG group object.
 * @param [in] distance Members less than this distance are grouped into one group.
 * @param [out] subgroups 
 * @param [out] count Number of elements in subgroups.
 * @note Terminator of subgroup array is UCG_HANDLE_MAX.
 */
ucs_status_t ucg_topo_group_by_distance(ucg_group_h group, ucg_distance_t distance, uint64_t **subgroups, int *count);

#endif
