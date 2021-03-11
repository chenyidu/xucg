#include <ucg/api/ucg_dt.h>
#include <ucg/api/ucg.h>
#include <ucg/core/ucg_rte.h>
#include <stdint.h>

typedef struct ucg_rte {
    ucg_rte_type_t type;
    union {
        ucg_rte_mpi_t mpi;
    };

} ucg_rte_t;

static ucg_rte_t g_rte;

ucs_status_t ucg_rte_init(ucg_rte_params_t *params)
{
    g_rte.type = params->type;
    switch (g_rte.type) {
        case UCG_RTE_TYPE_MPI:
            memcpy(&g_rte.mpi, &params->mpi, sizeof(ucg_rte_mpi_t));
            break;
        default:
            return UCS_ERR_UNSUPPORTED;
    }
    return UCS_OK;
}

uint32_t ucg_rte_dt_size(ucg_datatype_t *dt)
{
    int id = dt->id;
    uint32_t size = 0;
    if (id == UCG_DT_TYPE_USER_DEFINED) {
        switch (g_rte.type) {
            case UCG_RTE_TYPE_MPI:
                size = g_rte.mpi.dtype.size(dt->dt_ptr);
                break;
            default:
                ucs_assert(0);
                break;
        }
        return size;
    }

    ucs_assert(id < UCG_DT_TYPE_MAX_PREDEFINED);
    switch (id) {
        case UCG_DT_TYPE_UINT32:
            size = sizeof(uint32_t);
            break;
        default:
            ucs_assert(0);
            break;
    }
    return size;
}

void* ucg_rte_dt_state(ucg_datatype_t *dt, void *buffer, uint32_t count, int is_pack)
{
    void *dt_state = NULL;
    int id = dt->id;
    uint32_t size = 0;
    if (id == UCG_DT_TYPE_USER_DEFINED) {
        switch (g_rte.type) {
            case UCG_RTE_TYPE_MPI:
                if (!g_rte.mpi.dtype.is_contig(dt->dt_ptr)) {
                    if (is_pack) {
                        dt_state = g_rte.mpi.dtype.start_pack(dt->dt_ptr, buffer, count);
                    } else {
                        dt_state = g_rte.mpi.dtype.start_unpack(dt->dt_ptr, buffer, count);
                    }
                }
                break;
            default:
                ucs_assert(0);
                break;
        }
    }
    
    return dt_state;
}