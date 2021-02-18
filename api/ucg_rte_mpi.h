/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
#ifndef UCG_RTE_MPI_H_
#define UCG_RTE_MPI_H_

/**
 * @ingroup UCG_RTE_MPI
 * @brief Get mpi world rank.
 *
 * @param [in] handle process handle.
 */
 typedef int (*ucp_mpi_world_rank)(void *handle);
 
/**
 * @ingroup UCG_RTE_MPI
 * @brief Perform a reduction operation.
 *
 * @param [in] op MPI reduction operation.
 * @param [in] source buffer
 * @param [inout] target buffer
 * @param [in] count Number of elements
 * @param [in] dtype MPI datatype
 */
typedef int (*ucg_mpi_reduce_cb_t)(void *op, void *source, void *target, int count, void *dtype);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Check whether an operation is communative or not.
 */
typedef int (*ucg_mpi_op_is_commute_cb_t)(void *op);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Lookup address by world rank.
 */
typedef int (*ucg_mpi_addr_lookup_cb_t)(int world_rank, ucp_address_t **addr, size_t *addr_len);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Release address returned by loopup().
 */
typedef void (*ucg_mpi_addr_release_cb_t)(ucp_address_t *addr);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Start a packing request.
 *
 * @param [in] dtype MPI datatype.
 * @param [in] buffer Buffer to pack.
 * @param [in] count Number of elements to pack into the buffer.
 * @return A state that is passed later to pack().
 */
typedef void* (*ucg_dt_start_pack_cb_t)(void *dtype, void *buffer, int count);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Get the total size of packed data.
 *
 * @param [in] state State returned by start_pack().
 * @return The size of the data in a packed form.
 */
typedef int (*packed_size)(void *state);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Pack data.
 *
 * @param [in] state State returned by start_pack().
 * @param [in] offset Dest buffer offset.
 * @param [out] dest Destination to pack the data to.
 * @param [in] max_length Maximal length to pack.
 * @return The size of the data that was written to the destination buffer.
 */
typedef int (*ucg_dt_pack_cb_t)(void *state, int offset, void *dest, int max_length);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Start a packing request.
 *
 * @param [in] dtype MPI datatype.
 * @param [in] buffer Buffer to unpack.
 * @param [in] count Number of elements to unpack in the buffer.
 * @return A state that is passed later to unpack().
 */
typedef void* (*ucg_dt_start_unpack_cb_t)(void *dtype, void *buffer, int count);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Pack data.
 *
 * @param [in] state State returned by start_unpack().
 * @param [in] offset Src buffer offset.
 * @param [in] src Source to unpack the data from.
 * @param [in] length Length to unpack.
 * @return UCS_OK or an error.
 */
typedef ucs_status_t (*ucg_dt_unpack_cb_t)(void *state, int offet, void *src, int length);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Finish packing/unpacking
 *
 * @param [in] state State returned by start_pack()/start_unpack().
 */
typedef void (*ucg_dt_finish_cb_t)(void *stat);

/**
 * @ingroup UCG_RTE_MPI
 * @brief UCG MPI Runtime Enviroment.
 */
typedef struct ucg_rte_mpi {
    ucg_mpi_reduce_cb_t reduce;
    ucg_mpi_op_is_commute_cb_t op_is_commute;
    struct {
        ucg_addr_lookup_cb_t lookup;
        ucg_addr_release_cb_t release;
    } address;
    struct {
        ucg_dt_start_pack_cb_t start_pack;
        ucg_dt_packed_size_cb_t packed_size;
        ucg_dt_pack_cb_t pack;
        ucg_dt_start_unpack_cb_t start_unpack;
        ucg_dt_unpack_cb_t unpack;
        ucg_dt_finish_cb_t finish;
    } datatype;
} ucg_rte_mpi_t;

#endif
