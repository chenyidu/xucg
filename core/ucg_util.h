/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_UTIL_H_
#define UCG_UTIL_H_

#include <config.h>
#include <ucg/api/ucg.h>
#include <ucs/debug/log.h>
#include <ucs/sys/string.h>
#include <ucs/debug/memtrack.h>

/* Assertions which are checked in compile-time. */
#define UCG_STATIC_ASSERT(_cond) typedef int UCS_PP_APPEND_UNIQUE_ID(assert)[(_cond)?1:-1]

#if ENABLE_PARAMS_CHECK 
    #define UCG_CHECK_PARAMS(_condition) \
        if (!(_condition)) { \
            ucs_error("Paramters don't meet the requirmemnt(%s)", #_condition); \
            return UCS_ERR_INVALID_PARAM; \
        }
    #define UCG_CHECK_PARAMS_VOID(_condition) \
        if (!(_condition)) { \
            return; \
        }
#else 
     #define UCG_CHECK_PARAMS(_condition, _err_message, ...)
     #define UCG_CHECK_PARAMS_VOID(_condition)
#endif 

#define UCG_CHECK_REQUIRED_FIELD(_mask,_field, _lable) \
    if (!(_mask & _field)) { \
        ucs_error("The field \"%s\" is required.", #_field); \
        goto _lable; \
    }

static inline ucs_status_t ucg_group_members_clone(const ucg_group_members_t *src, 
                                                   ucg_group_members_t *dst)
{
    uint32_t alloc_size = src->count * sizeof(ucg_mh_t);
    ucg_mh_t *mh = (ucg_mh_t*)ucs_malloc(alloc_size, "ucg members");
    if (mh == NULL) {
        return UCS_ERR_NO_MEMORY;
    }
    memcpy(mh, src->mh, alloc_size);
    dst->self = src->self;
    dst->count = src->count;
    dst->mh = mh;
    return UCS_OK;
}

static inline void ucg_group_members_free(ucg_group_members_t *members)
{
    ucs_free(members->mh);
    return;
}

#endif