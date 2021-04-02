/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
#ifndef UCG_RTE_MPI_H_
#define UCG_RTE_MPI_H_

#include <ucg/api/ucg_def.h>
#include <ucp/api/ucp_def.h>

/**
 * @ingroup UCG_RTE_MPI
 * @brief Get MPI world rank.
 *
 * @param [in] mh MPI process handle.
 * @return MPI world rank.
 */
typedef int (*ucg_mpi_world_rank_func_t)(ucg_mh_t mh);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Get locations of all MPI processes.
 *
 * @param [out] locations Pointer to locations information.
 * @param [out] size Size of one location.
 * @param [out] count Number of locations.
 * @param [out] id ID of locations.
 * @return 0-Success, Other-Fail.
 */
typedef int (*ucg_mpi_locations_get_func_t)(void **locations, uint64_t *size, 
                                            uint64_t *count, uint64_t *id);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Put back the pointer of all MPI processes's locations.
 *
 * @param [in] locations Pointer to locations information.
 */
typedef void (*ucg_mpi_locations_put_func_t)(void *locations);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Get distance between world_rank1 and world_rank2.
 *
 * @param [in] locations Pointer to locations information.
 * @param [in] size Size of each location.
 * @param [in] count Number of locations.
 * @param [in] world_rank1 MPI process world rank.
 * @param [in] world_rank2 MPI process world rank.
 * @return The distance between world_rank1 and world_rank2.
 */
typedef ucg_distance_t (*ucg_mpi_locations_distance_func_t)(void *locations, 
                                                          uint64_t size,
                                                          uint64_t count,
                                                          int32_t world_rank1, 
                                                          int32_t world_rank2);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Perform a reduction operation.
 *
 * @param [in] op MPI reduction operation.
 * @param [in] source Source buffer.
 * @param [inout] target Target buffer.
 * @param [in] count Number of elements.
 * @param [in] dtype MPI datatype.
 * @return 0-Success, Other-Fail.
 */
typedef int (*ucg_mpi_op_reduce_func_t)(void *op, void *source, void *target, uint64_t count, void *dtype);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Check whether an operation is communative or not.
 *
 * @param [in] op Operation.
 * @return 1-Op is communative, 0-Op is not communative
 */
typedef int (*ucg_mpi_op_is_commute_func_t)(void *op);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Lookup address by world rank.
 *
 * @param [in] mh MPI process handle.
 * @param [out] addr Address of the handle.
 * @papram [out] addr_len Address length.
 * @return 0-Success, Other-Fail.
 */
typedef int (*ucg_mpi_addr_get_func_t)(ucg_mh_t mh, ucp_address_t **addr, uint32_t *addr_len);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Release address pointer.
 *
 * @param [in] addr Address obtained through ucg_mpi_addr_get_cb.
 */
typedef void (*ucg_mpi_addr_put_func_t)(ucp_address_t *addr);


/**
 * @ingroup UCG_RTE_MPI
 * @brief Wether datatype is config.
 *
 * @param [in] dtype MPI datatype.
 * @return 0 Non-contig, 1 contig
 */
typedef int (*ucg_mpi_dt_is_contig_func_t)(void *dtype);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Compare datatype.
 *
 * @param [in] ldtype Left-hand MPI datatype.
 * @param [in] rdtype Right-hand MPI datatype.
 * @return 0-not same, 1-same
 */
typedef int (*ucg_mpi_dt_is_same_func_t)(void *ldtype, void *rdtype);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Get the size of datatype.
 *
 * @param [in] dtype Left-hand MPI datatype.
 * @param [out] size True size of the datatype.
 * @return 0
 */
typedef int (*ucg_mpi_dt_size_func_t)(void *dtype, uint64_t *size);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Start a packing request.
 *
 * @param [in] dtype MPI datatype.
 * @param [in] buffer Buffer to pack.
 * @param [in] count Number of elements to pack into the buffer.
 * @return A state that is passed later to pack().
 */
typedef void* (*ucg_mpi_dt_start_pack_func_t)(void *dtype, const void *buffer, uint64_t count);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Get the total size of packed data.
 *
 * @param [in] state State returned by start_pack().
 * @return The size of the data in a packed form.
 */
typedef int (*ucg_mpi_dt_packed_size_func_t)(void *state);

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
typedef int (*ucg_mpi_dt_pack_func_t)(void *state, uint64_t offset, void *dest, uint64_t max_length);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Start a packing request.
 *
 * @param [in] dtype MPI datatype.
 * @param [in] buffer Buffer to unpack.
 * @param [in] count Number of elements to unpack in the buffer.
 * @return A state that is passed later to unpack().
 */
typedef void* (*ucg_mpi_dt_start_unpack_func_t)(void *dtype, void *buffer, uint64_t count);

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
typedef int (*ucg_mpi_dt_unpack_func_t)(void *state, uint64_t offset, void *src, uint64_t length);

/**
 * @ingroup UCG_RTE_MPI
 * @brief Finish packing/unpacking
 *
 * @param [in] state State returned by start_pack()/start_unpack().
 */
typedef void (*ucg_mpi_dt_finish_func_t)(void *state);

/**
 * @ingroup UCG_RTE_MPI
 * @brief UCG MPI Runtime Enviroment.
 *
 * @note It is very likely that fields are added to the substructure.
 *       It's hard to provide ABI compatibility.
 */
typedef struct ucg_rte_mpi {
    ucg_mpi_world_rank_func_t world_rank;
    
    struct {
        ucg_mpi_locations_get_func_t get;
        ucg_mpi_locations_put_func_t put;
        ucg_mpi_locations_distance_func_t distance;
    } locations;
        
    struct {
        ucg_mpi_op_reduce_func_t reduce;
        ucg_mpi_op_is_commute_func_t is_commute;
    } op;
 
    struct {
        ucg_mpi_addr_get_func_t get;
        ucg_mpi_addr_put_func_t put;
    } address;
 
    struct {
        ucg_mpi_dt_is_contig_func_t is_contig;
        ucg_mpi_dt_is_same_func_t is_same;
        ucg_mpi_dt_size_func_t size;

        ucg_mpi_dt_start_pack_func_t start_pack;
        ucg_mpi_dt_packed_size_func_t packed_size;
        ucg_mpi_dt_pack_func_t pack;
        ucg_mpi_dt_start_unpack_func_t start_unpack;
        ucg_mpi_dt_unpack_func_t unpack;
        ucg_mpi_dt_finish_func_t finish;
    } dtype;
} ucg_rte_mpi_t;

#endif
