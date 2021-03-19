#include "ucg_datatype.h"
#include <ucg/core/ucg_rte.h>
#include <stddef.h>

ucg_datatype_t ucg_dt_uint32 = {
    .id = UCG_DT_TYPE_UINT32,
    .dt_ptr = 0,
};

ucg_op_t ucg_dt_op_sum = {
    .id = UCG_DT_OP_SUM,
    .op_ptr = 0,
};

uint64_t g_dt_size[] = {
    sizeof(uint32_t), // UCG_DT_TYPE_UINT32
};

uint64_t ucg_dt_size(ucg_datatype_t *dt)
{
    int id = dt->id;
    if (id == UCG_DT_TYPE_USER_DEFINED) { 
        return ucg_rte_dt_size(dt);
    }

    return g_dt_size[id];
}

ucg_dt_state_t* ucg_dt_pack_state(ucg_datatype_t *dt, void *buffer, uint32_t count)
{
    if (dt->id == UCG_DT_TYPE_USER_DEFINED) { 
        return ucg_rte_dt_state(dt, buffer, count, 1);
    }
    // all builtin datatype is contig
    return NULL;
}

ucg_dt_state_t* ucg_dt_unpack_state(ucg_datatype_t *dt, void *buffer, uint32_t count)
{
    if (dt->id == UCG_DT_TYPE_USER_DEFINED) { 
        return ucg_rte_dt_state(dt, buffer, count, 0);
    }
    // all builtin datatype is contig
    return NULL;
}

void ucg_dt_state_finish(ucg_dt_state_t *state)
{
    if (state != NULL) { 
        return ucg_rte_dt_state_finish(state);
    }
    return;
}