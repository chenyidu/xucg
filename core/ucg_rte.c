#include "ucg_rte.h"
#include "ucg_config.h"

#include <config.h>
#include <ucg/api/ucg_dt.h>
#include <ucg/api/ucg.h>
#include <ucg/plan/api.h>
#include <ucs/debug/assert.h>
#include <ucs/sys/module.h>

#include <string.h>

typedef struct ucg_rte {
    ucg_rte_type_t type;
    union {
        ucg_rte_mpi_t mpi;
    };
} ucg_rte_t;
static ucg_rte_t g_rte;

ucg_rte_ops_t ucg_rte_ops[UCG_RTE_RESOURCE_TYPE_MAX];

ucs_status_t ucg_rte_init(ucg_rte_params_t *params)
{
    UCG_CHECK_PARAMS(params != NULL && params->type < UCG_RTE_TYPE_LAST);

    ucs_status_t status;
    if (params->type == UCG_RTE_TYPE_MPI) {
        // TODO: Check the validity of all fields
        memcpy(&g_rte.mpi, &params->mpi, sizeof(ucg_rte_mpi_t));
    } else {
        ucs_assert(params->type == UCG_RTE_TYPE_MOCK);
    }

    UCS_MODULE_FRAMEWORK_DECLARE(ucg);
    UCS_MODULE_FRAMEWORK_LOAD(ucg, 0);

    // Initialize all rte resources.
    for (int i = 0; i < UCG_RTE_RESOURCE_TYPE_MAX; ++i) {
        if (ucg_rte_ops[i].init == NULL) {
            continue;
        }
        status = ucg_rte_ops[i].init();
        if (status != UCS_OK) {
            goto err_cleanup;
        }
    }

    return UCS_OK;
err_cleanup:
    ucg_rte_cleanup();
    return status;
}

void ucg_rte_cleanup()
{
    for (int i = 0; i < UCG_RTE_RESOURCE_TYPE_MAX; ++i) {
        if (ucg_rte_ops[i].cleanup == NULL) {
            continue;
        }
        ucg_rte_ops[i].cleanup();
    }
    return;
}

uint32_t ucg_rte_dt_is_contig(ucg_datatype_t *dt)
{
     ucg_dt_type_id_t id = dt->id;
     ucs_assert(id == UCG_DT_TYPE_USER_DEFINED);
     uint32_t ret = 0;
     switch (g_rte.type) {
        case UCG_RTE_TYPE_MPI:
            ret = g_rte.mpi.dtype.is_contig(dt->dt_ptr);
            break;
        default:
            ucs_assert(0);
            break;
    }
    return ret;
}

uint64_t ucg_rte_dt_size(ucg_datatype_t *dt)
{
    ucg_dt_type_id_t id = dt->id;
    uint64_t size = 0;
    ucs_assert(id == UCG_DT_TYPE_USER_DEFINED);
    switch (g_rte.type) {
        case UCG_RTE_TYPE_MPI:
            size = g_rte.mpi.dtype.size(dt->dt_ptr, &size);
            break;
        default:
            ucs_assert(0);
            break;
    }
    return size;
}

void* ucg_rte_dt_pack_state(ucg_datatype_t *dt, const void *buffer, uint32_t count)
{
    void *dt_state = NULL;
    ucs_assert(dt->id == UCG_DT_TYPE_USER_DEFINED);
    switch (g_rte.type) {
        case UCG_RTE_TYPE_MPI:
            ucs_assert(!g_rte.mpi.dtype.is_contig(dt->dt_ptr));
            dt_state = g_rte.mpi.dtype.start_pack(dt->dt_ptr, buffer, count);
            break;
        default:
            ucs_assert(0);
            break;
    }
    
    return dt_state;
}

void* ucg_rte_dt_unpack_state(ucg_datatype_t *dt, void *buffer, uint32_t count)
{
    void *dt_state = NULL;
    ucs_assert(dt->id == UCG_DT_TYPE_USER_DEFINED);
    switch (g_rte.type) {
        case UCG_RTE_TYPE_MPI:
            ucs_assert(!g_rte.mpi.dtype.is_contig(dt->dt_ptr));
            dt_state = g_rte.mpi.dtype.start_unpack(dt->dt_ptr, buffer, count);
            break;
        default:
            ucs_assert(0);
            break;
    }
    
    return dt_state;
}

void ucg_rte_dt_state_finish(void *state)
{
    switch (g_rte.type) {
        case UCG_RTE_TYPE_MPI:
            g_rte.mpi.dtype.finish(state);
            break;
        default:
            ucs_assert(0);
            break;
    }
    
    return;
}