/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
#ifndef UCG_DEF_H_
#define UCG_DEF_H_

#include <stdint.h>

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
 * @brief UCG context configuration descriptor
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
 * @ingroup UCG_GROUP
 * @brief Globally unique member handle.
 * 
 * One member can be in different group, and global uniqueness means that no 
 * matter which group the member is in, its handle is the same. 
 */
typedef uint64_t ucg_mh_t;

/**
 * @ingroup UCG_GROUP
 * @brief The position of the member in the current group.
 * 
 * e.g. 
 * 1. Group members: [a,b,c,d], the rank of a in this group is 0
 * 2. Group members: [e,f,g,a], the rank of a in this group is 3
 * a,b,c,d,e,f,g is ucg_mh_t. 
 */
typedef int ucg_rank_t;

/**
 * @ingroup UCG_GROUP
 * @brief Group ID
 * 
 * Each group has its own unique ID.
 */
typedef uint32_t ucg_group_id_t;

/**
  * @ingroup UCG_REQUEST
  * @brief UCG collective operation request
  *
  * UCG request is an opaque object representing a description of a collective
  * operation. The description holds all the
  * necessary information to perform collectives, so re-starting an operation
  * requires no additional parameters.
  */
typedef struct ucg_request ucg_request_t;
typedef struct ucg_request* ucg_request_h;

#endif
