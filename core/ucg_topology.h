/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_TOPOLOGY_H_
#define UCG_TOPOLOGY_H_

typedef ucg_topo_group {
    ucg_distance_t distance; /* The distance between every two members is less than this value. */
    uin32_t count; /* Number of elements in handles */
    uint64_t *handles; /* member handle */
} ucg_topo_group_t;

/**
 * @ingroup UCG_TOPOLOGY
 * @brief Initilize global topology instance.
 */
ucs_status_t ucg_topo_init();

/**
 * @ingroup UCG_TOPOLOGY
 * @brief Get distance between two members.
 *
 * @param [in] group UCG group object
 * @param [in] handle1 Global member handle.
 * @param [in] handle2 Global member handle.
 */
ucs_status_t ucg_topo_get_distance(uint64_t handle1, uint64_t handle2);

/**
 * @ingroup UCG_TOPOLOGY
 * @brief Group members by distance.
 *
 * @param [in] group UCG group object.
 * @param [in] distance Members less than this distance are grouped into one group.
 * @param [out] topo_groups Topology group.
 * @param [out] count Number of elements in topo_groups.
 */
ucs_status_t ucg_topo_group_by_distance(ucg_group_h group, ucg_distance_t distance, ucg_topo_group_t *topo_groups, uin32_t *count);

/**
 * @ingroup UCG_TOPOLOGY
 * @brief Release topology groups returned by ucg_topo_group_by_distance().
 *
 * @param [in] topo_groups Topology groups.
 */
ucs_status_t ucg_topo_release_group(ucg_topo_group_t *topo_groups);

#endif
