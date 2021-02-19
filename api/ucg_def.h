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

/**
 * @ingroup UCG_CONTEXT
 * @brief UCG Application Context
 */
typedef struct ucg_context* ucg_context_h;

/**
 * @ingroup UCG_CONFIG
 * @brief UCG configuration descriptor
 */
typedef struct ucg_config ucg_config_t;

 /**
  * @ingroup UCG_GROUP
  * @brief UCG Group
  *
  * UCG group is used for collective operations. Groups are created with respect to a local
  * worker, and share its endpoints for communication with the remote workers.
  */
typedef struct ucg_group* ucg_group_h;

 /**
  * @ingroup UCG_REQUEST
  * @brief UCG collective operation request
  *
  * UCG request is an opaque object representing a description of a collective
  * operation. The description holds all the
  * necessary information to perform collectives, so re-starting an operation
  * requires no additional parameters.
  */
typedef void* ucg_request_h;

#endif
