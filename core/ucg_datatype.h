#ifndef UCG_DATATYPE_H_
#define UCG_DATATYPE_H_

#include <ucg/api/ucg_dt.h>
#include <stdint.h>

typedef void ucg_dt_state_t;

/**
 * @ingroup UCG_DT
 * @brief Check wether datatype is contig.
 * @return 1 contig, 0 non-contig
 */
uint32_t ucg_dt_is_contig(ucg_datatype_t *dt);

/**
 * @ingroup UCG_DT
 * @brief Get the real size of the datatype.
 */
uint64_t ucg_dt_size(ucg_datatype_t *dt);

/**
 * @ingroup UCG_DT
 * @brief Create a pack state.
 */
ucg_dt_state_t* ucg_dt_pack_state(ucg_datatype_t *dt, const void *buffer, uint32_t count);

/**
 * @ingroup UCG_DT
 * @brief Create a unpack state.
 */
ucg_dt_state_t* ucg_dt_unpack_state(ucg_datatype_t *dt, void *buffer, uint32_t count);

/**
 * @ingroup UCG_DT
 * @brief Release a state.
 */
void ucg_dt_state_finish(ucg_dt_state_t *state);

#endif