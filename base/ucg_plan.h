/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_PLAN_H_
#define UCG_PLAN_H_

typedef enum ucg_plan_action_type {
    UCG_PLAN_ACTION_TYPE_SEND,
    UCG_PLAN_ACTION_TYPE_RECV,
    UCG_PLAN_ACTION_TYPE_REDUCE, 
    UCG_PLAN_ACTION_TYPE_GENERIC,
    UCG_PLAN_ACTION_TYPE_MAX,
} ucg_plan_action_type_t;

typedef struct ucg_plan_action ucg_plan_action_t;
typedef ucs_status_t (*ucg_plan_action_generic_cb_t)(ucg_plan_action_t *action);

/**
 * @ingroup UCG_PLAN_ACTION
 * @brief Action peers for sending and receiving.
 */
typedef struct ucg_plan_action_peers_t {
    int count; /* Number of peers */
    int *peers; /* Relative member ID which needs to be converted to handle 
                   for creating communication channel. */
} ucg_plan_action_peers_t_t;

typedef struct ucg_plan_action_buf {
    int count; /* Number of elements in buffers array and lengths array. */
    uint8_t **buffers;
    int *lengths;
} ucg_plan_action_buf_t;

/**
 * @ingroup UCG_PLAN
 * @brief Constant field of an action
 *
 * When do plan clone, something in the action is no need to change. 
 * This structure holds the unchanged field of an action.
 */
typedef struct ucg_plan_action_constant {
    int refcount; /* Reference count */
    uint8_t id; /* Action ID */
    ucg_plan_action_type_t type;
    union {
        ucg_plan_action_peers_t send;
        ucg_plan_action_peers_t recv;
        ucg_plan_action_generic_cb_t generic;
    };
} ucg_plan_action_constant_t;

/**
 * @ingroup UCG_PLAN
 * @brief Base structure of plan action.
 */
struct ucg_plan_action {
    ucg_plan_action_constant_t *constant;
    union {
       /* The size of send array is equal to generic->send.count, recv and reduce are the same. */
       ucg_plan_action_buf_t send;
       ucg_plan_action_buf_t recv;
       ucg_plan_action_buf_t reduce;
    };
    ucg_plan_action_t* next;
};

typedef struct ucg_plan {
    int refcount;
} ucg_plan_t;

inline ucg_plan_t* ucg_plan_clone(ucg_plan_t *plan, ...)
{
    return plan->clone(plan, ...);
}

#define UCG_PLAN_REGISTER(_plan, _name, _desciption,)
 
#endif
