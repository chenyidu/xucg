#include "ucg_context.h"
#include "ucg_config.h"

#include <ucg/api/ucg_version.h>
#include <ucs/debug/debug.h>
#include <ucs/debug/log.h>
#include <ucs/debug/memtrack.h>

ucs_status_t ucg_init_version(unsigned api_major_version,
                              unsigned api_minor_version,
                              const ucg_params_t *params,
                              const ucg_config_t *config,
                              ucg_context_h *context_p)
{
    ucs_status_t status = UCS_OK;
    unsigned major_version = 0;
    unsigned minor_version = 0;
    unsigned release_version = 0;

    ucg_get_version(&major_version, &minor_version, &release_version);
    if ((api_major_version != major_version) || 
        (api_minor_version != minor_version && 
         api_minor_version > minor_version)) {
        ucs_debug_address_info_t addr_info;
        status = ucs_debug_lookup_address(ucg_init_version, &addr_info);
        ucs_warn("UCG version is incompatible, required: %d.%d, actual: %d.%d (released %d %s).",
                 api_major_version, api_minor_version, 
                 major_version, minor_version, release_version,
                 status == UCS_OK ? addr_info.file.path : "");
    }

    ucg_context_t *context = ucs_calloc(1, sizeof(ucg_context_t), "ucg context");
    if (context == NULL) {
        status = UCS_ERR_NO_MEMORY;
        goto err;
    }
    ucs_list_head_init(&context->group_head);

    if (config == NULL) {
        status = ucg_config_read(NULL, NULL, &context->config);
    } else {
        status = ucg_config_clone(config, &context->config);
    }
    if (status != UCS_OK) {
        ucs_error("Failed to read ucg configuration.");
        goto err_free_ctx;
    }

    status = ucg_ppool_create(context->config, &context->ppool);
    if (status != UCS_OK) {
        ucs_error("Failed to create plan pool.");
        goto err_free_config;
    }

    *context_p = context;
    return UCS_OK;
err_free_config:
    ucg_config_release(context->config);
err_free_ctx:
    ucs_free(context);
err:
    return status;
}

ucs_status_t ucg_init(const ucg_params_t *params,
                      const ucg_config_t *config,
                      ucg_context_h *context)
{
    return ucg_init_version(UCG_API_MAJOR, UCG_API_MINOR, params, config, context);
}

void ucg_cleanup(ucg_context_h context)
{
    if (!ucs_list_is_empty(&context->group_head)) {
        ucs_warn("Some ucg group are not destroyed, there is a problem with "
                 "the order of releasing resources.");
    }
    ucg_config_release(context->config);
    ucg_ppool_destroy(context->ppool);
    return;
}