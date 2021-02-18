/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
 
#ifndef UCG_DT_H_
#define UCG_DT_H_
 
 /**
 * @ingroup UCG_DT
 * @brief datatype type
 */
typedef enum ucg_dt_type_id {
    UCG_DT_UINT32,
    UCG_DT_MPI,
    UCG_DT_MAX,
} ucg_dt_type_t;

typedef struct ucg_datatype {
    ucg_dt_type_t id;
    void *dt_ptr;
} ucg_datatype_t;

extern ucg_datatype_t ucg_dt_uint32;

#endif
