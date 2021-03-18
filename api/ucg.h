/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */
#ifndef UCG_H_
#define UCG_H_

#include <ucg/api/ucg_rte_mpi.h>

/**
 * @ingroup UCG_RTE
 * @brief UCG Runtime Enviroment Type.
 */
typedef enum ucg_rte_type {
    UCG_RTE_TYPE_MPI,
    UCG_RTE_TYPE_LAST,
} ucg_rte_type_t;

typedef enum ucg_group_params_field {
    UCG_GROUP_PARAMS_UCP_WORKER = UCS_BIT(0),
    UCG_GROUP_PARAMS_MEMBERS = UCS_BIT(1),
} ucg_group_params_field_t;

/**
 * @ingroup UCG_RTE
 * @brief Tuning UCG runtime enviroment.
 */
typedef struct ucg_rte_params {
    ucg_rte_type_t type;
    union {
        ucg_rte_mpi_t mpi;
    };
} ucg_rte_params_t;

/**
 * @ingroup UCG_CONTEXT
 * @brief Creation parameters for the UCG context.
 */
typedef struct ucg_context_params {
    uint64_t field_mask;
} ucg_context_params_t;

typedef struct ucg_group_members {
    ucg_rank_t self; /* My position in the handles array */
    int count; /* Number of element in the mh array */
    ucg_mh_t *mh; /* Array of user-defined member handle */
} ucg_group_members_t;

/**
 * @ingroup UCG_GROUP
 * @brief Creation parameters for the UCG group.
 */
typedef struct ucg_group_params {
    /**
     * Mask of valid fields in this structure, using bits from @ref ucg_group_params_field.
     * Fields not specified in this mask will be ignored.
     * Provides ABI compatibility with respect to adding new fields.
     */
    uint64_t field_mask;
    
    /* Specified worker, cannot release until the group is destroyed */
    ucp_worker_h ucp_worker;
    ucg_group_members_t members;
} ucg_group_params_t;

/**
 * @ingroup UCG_CONFIG
 * @brief Read UCG configuration descriptor
 *
 * The routine fetches the information about UCG library configuration from
 * the run-time environment. Then, the fetched descriptor is used for
 * UCG library @ref ucg_init "initialization". The Application can print out the
 * descriptor using @ref ucg_config_print "print" routine. In addition
 * the application is responsible for @ref ucg_config_release "releasing" the
 * descriptor back to the UCG library.
 *
 * @param [in]  env_prefix    If non-NULL, the routine searches for the
 *                            environment variables that start with
 *                            @e \<env_prefix\>_UCX_ prefix.
 *                            Otherwise, the routine searches for the
 *                            environment variables that start with
 *                            @e UCX_ prefix.
 * @param [in]  filename      If non-NULL, read configuration from the file
 *                            defined by @e filename. If the file does not
 *                            exist, it will be ignored and no error reported
 *                            to the application.
 * @param [out] config_p      Pointer to configuration descriptor as defined by
 *                            @ref ucg_config_t "ucg_config_t".
 *
 * @return Error code as defined by @ref ucs_status_t
 */
ucs_status_t ucg_config_read(const char *env_prefix, const char *filename,
                             ucg_config_t **config_p);


/**
 * @ingroup UCG_CONFIG
 * @brief Release configuration descriptor
 *
 * The routine releases the configuration descriptor that was allocated through
 * @ref ucg_config_read "ucg_config_read()" routine.
 *
 * @param [out] config        Configuration descriptor as defined by
 *                            @ref ucg_config_t "ucg_config_t".
 */
void ucg_config_release(ucg_config_t *config);


/**
 * @ingroup UCG_CONFIG
 * @brief Modify context configuration.
 *
 * The routine changes one configuration setting stored in @ref ucg_config_t
 * "configuration" descriptor.
 *
 * @param [in]  config        Configuration to modify.
 * @param [in]  name          Configuration variable name.
 * @param [in]  value         Value to set.
 *
 * @return Error code.
 */
ucs_status_t ucg_config_modify(ucg_config_t *config, const char *name,
                               const char *value);


/**
 * @ingroup UCG_CONFIG
 * @brief Print configuration information
 *
 * The routine prints the configuration information that is stored in
 * @ref ucg_config_t "configuration" descriptor.
 *
 * @todo Expose ucs_config_print_flags_t
 *
 * @param [in]  config        @ref ucg_config_t "Configuration descriptor"
 *                            to print.
 * @param [in]  stream        Output stream to print the configuration to.
 * @param [in]  title         Configuration title to print.
 * @param [in]  print_flags   Flags that control various printing options.
 */
void ucg_config_print(const ucg_config_t *config, FILE *stream,
                      const char *title, ucs_config_print_flags_t print_flags);


/**
 * @ingroup UCG_CONTEXT
 * @brief Get UCG library version.
 *
 * This routine returns the UCG library version.
 *
 * @param [out] major_version       Filled with library major version.
 * @param [out] minor_version       Filled with library minor version.
 * @param [out] release_number      Filled with library release number.
 */
void ucg_get_version(unsigned *major_version, unsigned *minor_version,
                     unsigned *release_number);

/**
 * @ingroup UCG_CONTEXT
 * @brief Get UCG library version as a string.
 *
 * This routine returns the UCG library version as a string which consists of:
 * "major.minor.release".
 */
const char* ucg_get_version_string(void);

/**
 * @ingroup UCG_RTE
 * @brief UCG runtime enviroment initialization.
 */
ucs_status_t ucg_rte_init(ucg_rte_params_t *params);

/**
 * @ingroup UCG_CONTEXT
 * @brief UCG context initialization.
 *
 * @param [in] params User-defined parameter.
 * @param [in] config User-defined configuration.
 * @param [out] context Initialized UCG context.
 * @return Error code as defined by @ref ucs_status_t
 */
ucs_status_t ucg_context_init(const ucg_context_params_t *params, 
                              const ucg_config_t *config,
                              ucg_context_h *context);
/**
 * @ingroup UCG_CONTEXT
 * @brief Release UCG application context.
 *
 * @param [in] context Initialized UCG context.
 */
void ucg_context_cleanup(ucg_context_h context);

/**
 * @ingroup UCG_GROUP
 * @brief Create a UCG group.
 *
 * @param [in] context Initialized UCG context.
 * @param [in] params User-defined parameter.
 * @param [out] group Allocated group.
 * @return Error code.
 */
ucs_status_t ucg_group_create(ucg_context_h context, 
                              const ucg_group_params_t *params,
                              ucg_group_h *group);

/**
 * @ingroup UCG_CONTEXT
 * @brief Destroy a UCG group.
 *
 * @param [in] group Allocated group.
 */
void ucg_group_destroy(ucg_group_h group);

/**
 * @ingroup UCG_GROUP
 * @brief Progress all communications in the group.
 *
 * @param [in] group Allocated group.
 */
int ucg_group_progress(ucg_group_h group);

/**
 * @ingroup UCG_REQUEST
 * @brief Initialize a bcast request.
 *
 * Create a request to broadcast a message from the root process to all other processes of the group.
 * @param [in] group UCG group handle.
 * @param [in] buffer Starting address of buffer.
 * @param [in] count Number of elements in buffer.
 * @param [in] dtype Data type of buffer.
 * @param [in] root Position of root handle in handles(ucg_group_params_t::members::handles).
 * @param [out] request Request.
 */
ucs_status_t ucg_request_bcast_init(ucg_group_h group,
                                    void *buffer, 
                                    int count, 
                                    ucg_datatype_t *dtype, 
                                    ucg_rank_t root,
                                    ucg_request_h *request);

/**
 * @ingroup UCG_REQUEST
 * @brief Initialize a allreduce request.
 *
 * Create a request to combine values from all processes of the group and distributes the result back to all processes of the group.
 * @param [in] group UCG group handle.
 * @param [in] sendbuf Send buffer.
 * @param [out] recvbuf Receive buffer.
 * @param [in] count Number of elements in send buffer.
 * @param [in] dtype Datatype of elements of send buffer.
 * @param [in] op Operaton.
 * @param [out] request Request.
 * @note sendbuf shoud be a accessable address, not support "in_place".
 */
ucs_status_t ucg_request_allreduce_init(ucg_group_h group,
                                        const void *sendbuf, 
                                        void *recvbuf, 
                                        int count, 
                                        ucg_datatype_t *dtype, 
                                        ucg_op_t *op, 
                                        ucg_request_h *request);

/**
 * @ingroup UCG_REQUEST
 * @brief Initialize a barrier request.
 *
 * Create a request to block until all processes in the group have reached this routine.
 * @param [in] group UCG group handle.
 * @param [out] request Request.
 */
ucs_status_t ucg_request_barrier_init(ucg_group_h group, ucg_request_h *request);

/**
 * @ingroup UCG_REQUEST
 * @brief Start a initialized request.
 */
ucs_status_t ucg_request_start(ucg_request_h request);

/**
 * @ingroup UCG_REQUEST
 * @brief Progress a started request.
 *
 * @return Non-zero if any communication was progressed, zero otherwise.
 */
int ucg_request_progress(ucg_request_h request);

/**
 * @ingroup UCG_REQUEST
 * @brief Check the status of request.
 *
 * This routine checks the state of the request and returns its current status.
 * Any value different from UCS_INPROGRESS means that request is in a completed
 * state.
 */
ucs_status_t ucg_request_check_status(ucg_request_h request);

/**
 * @ingroup UCG_REQUEST
 * @brief Cancel the request.
 */
ucs_status_t ucg_request_cancel(ucg_request_h request);

/**
 * @ingroup UCG_REQUEST
 * @brief Free the request.
 */
ucs_status_t ucg_request_free(ucg_request_h request);

#endif
