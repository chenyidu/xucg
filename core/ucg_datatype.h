#ifndef UCG_DATATYPE_H_
#define UCG_DATATYPE_H_

#include <ucg/api/ucg_dt.h>
#include <stdint.h>

typedef void ucg_dt_state_t;
uint32_t ucg_datatype_size(ucg_datatype_t *dt);
ucg_dt_state_t* ucg_dt_pack_state(ucg_datatype_t *dt, void *buffer, uint32_t count);
ucg_dt_state_t* ucg_dt_unpack_state(ucg_datatype_t *dt, void *buffer, uint32_t count);

#endif