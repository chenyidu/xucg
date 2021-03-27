#include "ucg_config.h"
#include "ucg_rte.h"

#include <ucg/api/ucg.h>
#include <ucs/debug/memtrack.h>
#include <ucs/sys/string.h>
#include <ucs/sys/preprocessor.h>

typedef struct ucg_config_mgr {
    uint32_t num_fields; /* Number of fileds of all config table. */
    ucs_list_link_t list; /* config table list. */
    ucs_config_global_list_entry_t entry;
} ucg_config_mgr_t;

static ucg_config_mgr_t g_config_mgr = {
    .num_fields = 0,
    .list = UCS_LIST_INITIALIZER(&g_config_mgr.list, &g_config_mgr.list),
    .entry = {
        .name = "UCG config",
        .prefix = "GROUP_",
        .table = NULL,
        .size = sizeof(ucg_config_t),
    },
};
UCS_CONFIG_REGISTER_TABLE_ENTRY(&g_config_mgr.entry);

static ucs_status_t ucg_config_global_init()
{
    ucs_config_field_t *field;
    field = (ucs_config_field_t*)ucs_calloc(g_config_mgr.num_fields + 1, 
                                            sizeof(ucs_config_field_t), 
                                            "ucg config");
    if (field == NULL) {
        return UCS_ERR_NO_MEMORY;
    }
    g_config_mgr.entry.table = field;

    // Copy all fields and adjust the offset.
    ucs_config_global_list_entry_t *entry;
    ucs_config_field_t *src_field;
    uint32_t start_offset = sizeof(ucg_config_t);
    ucs_list_for_each (entry, &g_config_mgr.list, list) {
        for(src_field = entry->table; src_field->name != NULL; ++src_field, ++field) {
            memcpy(field, src_field, sizeof(ucs_config_field_t));
            field->offset += start_offset;
        }
        start_offset += entry->size;
    }

    return 0;
}

static void ucg_config_global_cleanup()
{
    ucs_free(g_config_mgr.entry.table);
    return;
}

uint32_t ucg_config_register(ucs_config_global_list_entry_t *entry, uint32_t num_fields)
{
    g_config_mgr.num_fields += num_fields;
    ucs_list_add_tail(&g_config_mgr.list, &entry->list);
    uint32_t offset = g_config_mgr.entry.size;
    g_config_mgr.entry.size += entry->size;
    return offset;
}

ucs_status_t ucg_config_read(const char *env_prefix, const char *filename, 
                             ucg_config_t **config)
{
    unsigned full_prefix_len = sizeof(UCS_DEFAULT_ENV_PREFIX) + 1;
    ucs_status_t status = UCS_OK;

    ucg_config_t *cfg = ucs_malloc(sizeof(ucg_config_t) + g_config_mgr.entry.size, "ucg config");
    if (cfg == NULL) {
        status = UCS_ERR_NO_MEMORY;
        goto err;
    }

    if (env_prefix != NULL) {
        full_prefix_len += strlen(env_prefix);
    }

    cfg->env_prefix = ucs_malloc(full_prefix_len, "ucg config");
    if (cfg->env_prefix == NULL) {
        status = UCS_ERR_NO_MEMORY;
        goto err_free_config;
    }

    if (env_prefix != NULL) {
        ucs_snprintf_zero(cfg->env_prefix, full_prefix_len, "%s_%s", 
                          env_prefix, UCS_DEFAULT_ENV_PREFIX);
    } else {
        ucs_snprintf_zero(cfg->env_prefix, full_prefix_len, "%s", 
                          UCS_DEFAULT_ENV_PREFIX);
    }

    status = ucs_config_parser_fill_opts(cfg, g_config_mgr.entry.table, cfg->env_prefix, NULL, 0);
    if (status != UCS_OK) {
        goto err_free_prefix;
    }

    *config = cfg;
    return UCS_OK;
err_free_prefix:
    ucs_free(cfg->env_prefix);
err_free_config:
    ucs_free(cfg);
err:
    return status;
}

ucs_status_t ucg_config_modify(ucg_config_t *config, const char *name, 
                               const char *value)
{
    return ucs_config_parser_set_value(config, g_config_mgr.entry.table, name, value);
}

void ucg_config_print(const ucg_config_t *config, FILE *stream, 
                      const char *title, ucs_config_print_flags_t print_flag)
{
    ucs_config_parser_print_opts(stream, title, config, g_config_mgr.entry.table,
                                 NULL, UCS_DEFAULT_ENV_PREFIX, print_flag);
    return;
}

void ucg_config_release(ucg_config_t *config)
{
    ucs_config_parser_release_opts(config, g_config_mgr.entry.table);
    ucs_free(config->env_prefix);
    ucs_free(config);
    return;
}

ucs_status_t ucg_config_clone(const ucg_config_t *src, ucg_config_t **dst)
{
    ucg_config_t *cfg = ucs_malloc(sizeof(ucg_config_t) + g_config_mgr.entry.size, "ucg config");
    if (cfg == NULL) {
        return UCS_ERR_NO_MEMORY;
    }
    
    if (src->env_prefix != NULL) {
        cfg->env_prefix = ucs_strdup(src->env_prefix);
        if (cfg->env_prefix == NULL) {
            return UCS_ERR_NO_MEMORY;
        }
    } else {
        cfg->env_prefix = NULL;
    }

    ucs_status_t status = ucs_config_parser_clone_opts(src, cfg, g_config_mgr.entry.table);
    if (status != UCS_OK) {
        ucs_free(cfg->env_prefix);
        return status;
    }

    return UCS_OK;
}

UCG_RTE_INNER_DEFINE(UCG_RTE_RESOURCE_TYPE_CONFIG, ucg_config_global_init, 
                     ucg_config_global_cleanup);