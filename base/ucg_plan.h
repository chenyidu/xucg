/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_PLAN_H_
#define UCG_PLAN_H_

typedef ucg_plan_action_type {
    UCG_PLAN_ACTION_TYPE_SEND,
    UCG_PLAN_ACTION_TYPE_RECV,
    UCG_PLAN_ACTION_TYPE_REDUCE, /* Receive then reduce */
    UCG_PLAN_ACTION_TYPE_GENERIC,
    UCG_PLAN_ACTION_TYPE_MAX,
} ucg_plan_action_type_t;

typedef struct ucg_plan_action ucg_plan_action_t;

typedef ucs_status_t (*ucg_plan_do_action_cb_t)(ucg_plan_action_t *action);

typedef struct ucg_plan_action_peers {
    int count; /* Number of peers */
    int *peers; /* Relative member ID which needs to be converted to handle for creating channel. */
} ucg_plan_action_peers_t;

typedef struct ucg_plan_action_buf {
    uint8_t *buf;
    int length;
} ucg_plan_action_buf_t;

/**
 * @ingroup UCG_PLAN
 * @brief Generic action
 *
 * When do plan clone, something in the action is no need to change. 
 * This structure holds the unchanged field of an action.
 */
typedef struct ucg_plan_action_generic {
    int refcount; /* Reference count */
    uint8_t id; /* Action ID */
    ucg_plan_action_type_t type;
    union {
        ucg_plan_action_peers_t send;
        ucg_plan_action_peers_t recv;
        ucg_plan_action_peers_t reduce;
        ucg_plan_do_action_cb_t do_action; /* For UCG_PLAN_ACTION_TYPE_GENERIC */
    };
    ucg_plan_action_t* next;
} ucg_plan_action_generic_t;

/**
 * @ingroup UCG_PLAN
 * @brief Base structure of plan action.
 */
struct ucg_plan_action {
    ucg_plan_action_generic_t *generic;
    union {
       /* The size of send array is equal to generic->send.count, recv and reduce are the same. */
       ucg_plan_action_buf_t *send;
       ucg_plan_action_buf_t *recv;
       ucg_plan_action_buf_t *reduce;
    };
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
