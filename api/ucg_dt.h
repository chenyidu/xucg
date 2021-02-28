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
    UCG_DT_TYPE_UINT32, 
    UCG_DT_TYPE_MAX_PREDEFINED, 
 
    UCG_DT_TYPE_USER_DEFINED, 
} ucg_dt_type_id_t;

typedef enum ucg_dt_op_id {
    UCG_DT_OP_SUM,
    UCG_DT_OP_MAX_PREDEFINED,
 
    UCG_DT_OP_USER_DEFINED, 
} ucg_dt_op_id_t;

typedef struct ucg_datatype {
    ucg_dt_type_id_t id;
    uint8_t is_contig;
    void *dt_ptr;
} ucg_datatype_t;

typedef struct ucg_op {
    ucg_dt_op_id_t id;
    void *op_ptr;
} ucg_op_t;

extern ucg_datatype_t ucg_dt_uint32;
extern ucg_op_t ucg_dt_op_sum;

#endif
