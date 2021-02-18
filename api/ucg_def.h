/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
#ifndef UCG_DEF_H_
#define UCG_DEF_H_

/**
 * @ingroup UCG_TOPOLOGY
 * @brief Define the distance between two processes.
 */
typedef enum ucg_distance {
    UCG_DISTANCE_SELF,
    UCG_DISTANCE_L3CACHE,
    UCG_DISTANCE_SOCKET,
    UCG_DISTANCE_HOST,
    UCG_DISTANCE_NET,
} ucg_distance_t;

#endif
