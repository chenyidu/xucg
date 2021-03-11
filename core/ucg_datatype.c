#include <ucg/core/ucg_datatype.h>
#include <ucg/core/ucg_rte.h>

ucg_datatype_t ucg_dt_uint32 = {
    .id = UCG_DT_TYPE_UINT32,
    .dt_ptr = 0,
};

ucg_op_t ucg_dt_op_sum = {
    .id = UCG_DT_OP_SUM,
    .op_ptr = 0,
};

uint32_t ucg_datatype_size(ucg_datatype_t *dt)
{
    return ucg_rte_dt_size(dt);
}

ucg_dt_state_t* ucg_dt_pack_state(ucg_datatype_t *dt, void *buffer, uint32_t count)
{
    return (ucg_dt_state_t*)ucg_rte_dt_state(dt, buffer, count, 1);
}

ucg_dt_state_t* ucg_dt_unpack_state(ucg_datatype_t *dt, void *buffer, uint32_t count)
{
    return (ucg_dt_state_t*)ucg_rte_dt_state(dt, buffer, count, 0);
}
