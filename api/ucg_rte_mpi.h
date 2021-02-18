/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
#ifndef UCG_RTE_MPI_H_
#define UCG_RTE_MPI_H_

#include <ucg/api/ucg_def.h>

/**
 * @ingroup UCG_RTE_MPI
 * @brief Get MPI world rank.
 *
 * @param [in] handle process handle.
 * @return MPI world rank.
 */
typedef int (*ucg_mpi_world_rank_cb_t)(void *handle);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Get distance between world_rank1 and world_rank2.
 */
typedef ucg_distance_t (*ucg_mpi_get_distance_cb_t)(int world_rank1, int world_rank2);

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
typedef int (*ucg_mpi_op_reduce_cb_t)(void *op, void *source, void *target, int count, void *dtype);

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
 * @return 0 non-contig, 1 contig
 */
typedef int (*ucg_mpi_dt_is_contig_cb_t)(void *dtype);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Start a packing request.
 *
 * @param [in] ldtype left-hand MPI datatype.
 * @param [in] rdtype right-hand MPI datatype.
 * @return 0 non-contig, 1 contig
 */
typedef int (*ucg_mpi_dt_is_same_cb_t)(void *ldtype, void *rdtype);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Start a packing request.
 *
 * @param [in] dtype MPI datatype.
 * @param [in] buffer Buffer to pack.
 * @param [in] count Number of elements to pack into the buffer.
 * @return A state that is passed later to pack().
 */
typedef void* (*ucg_mpi_dt_start_pack_cb_t)(void *dtype, void *buffer, int count);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Get the total size of packed data.
 *
 * @param [in] state State returned by start_pack().
 * @return The size of the data in a packed form.
 */
typedef int (*ucg_mpi_dt_packed_size)(void *state);

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
typedef int (*ucg_mpi_dt_pack_cb_t)(void *state, int offset, void *dest, int max_length);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Start a packing request.
 *
 * @param [in] dtype MPI datatype.
 * @param [in] buffer Buffer to unpack.
 * @param [in] count Number of elements to unpack in the buffer.
 * @return A state that is passed later to unpack().
 */
typedef void* (*ucg_mpi_dt_start_unpack_cb_t)(void *dtype, void *buffer, int count);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Pack data.
 *
 * @param [in] state State returned by start_unpack().
 * @param [in] offset Src buffer offset.
 * @param [in] src Source to unpack the data from.
 * @param [in] length Length to unpack.
 * @return 0 for Success, other for Fail
 */
typedef int (*ucg_mpi_dt_unpack_cb_t)(void *state, int offet, void *src, int length);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Finish packing/unpacking
 *
 * @param [in] state State returned by start_pack()/start_unpack().
 */
typedef void (*ucg_mpi_dt_finish_cb_t)(void *state);

/**
 * @ingroup UCG_RTE_MPI
 * @brief UCG MPI Runtime Enviroment.
 */
typedef struct ucg_rte_mpi {
    ucg_mpi_world_rank_cb_t world_rank;
    ucg_mpi_get_distance_cb_t get_distance;
 
    struct {
        ucg_mpi_op_reduce_cb_t reduce;
        ucg_mpi_op_is_commute_cb_t is_commute;
    } op;
 
    struct {
        ucg_mpi_addr_lookup_cb_t lookup;
        ucg_mpi_addr_release_cb_t release;
    } address;
 
    struct {
        ucg_mpi_dt_is_contig_cb_t is_contig;
        ucg_mpi_dt_is_same_cb_t is_same;
     
        ucg_mpi_dt_start_pack_cb_t start_pack;
        ucg_mpi_dt_packed_size_cb_t packed_size;
        ucg_mpi_dt_pack_cb_t pack;
        ucg_mpi_dt_start_unpack_cb_t start_unpack;
        ucg_mpi_dt_unpack_cb_t unpack;
        ucg_mpi_dt_finish_cb_t finish;
    } dtype;
} ucg_rte_mpi_t;

#endif
