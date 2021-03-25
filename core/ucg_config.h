/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

#ifndef UCG_CONFIG_H_
#define UCG_CONFIG_H_

#include <ucs/type/status.h>
#include <ucs/config/parser.h>

/**
 * @brief Register config table.
 */
#define UCG_CONFIG_REGISTER_TABLER(_table, _name, _prefix, _type) \
    static uint32_t _type##_num_fields = sizeof(_table) / sizeof(ucs_config_field_t) - 1; \
    static ucs_config_global_list_entry_t _table##_config_entry = { \
        .table = _table, \
        .name = _name, \
        .prefix = _prefix, \
        .size = sizeof(_type), \
    }; \
    static uint32_t _type##_offset = 0; \
    UCS_STATIC_INIT { \
        _type##_offset = ucg_config_register(&_table##_config_entry, _type##_num_fields); \
    }

#define UCG_CONFIG_CONVERT(config, _type) \
    (_type*)(config + _type##_offset); \

/**
 * @ingroup UCG_CONFIG
 * @brief ucg configuration.
 */
typedef struct ucg_config {
    char *env_prefix;
    char data[0];
} ucg_config_t;

/**
 * @ingroup UCG_CONFIG
 * @brief Initialize global configuration resources.
 * 
 * This routine should be invoked before any other config routines.
 */
ucs_status_t ucg_config_global_init();

/**
 * @ingroup UCG_CONFIG
 * @brief Release global configuration resources.
 */
void ucg_config_global_cleanup();

/**
 * @ingroup UCG_CONFIG
 * @brief Register a config entry to global.
 * 
 * Call @ref UCG_CONFIG_REGISTER_TABLER() instead.
 */
uint32_t ucg_config_register(ucs_config_global_list_entry_t *entry, uint32_t num_fields);

/**
 * @ingroup UCG_CONFIG
 * @brief Clone configuration.
 * 
 * @param [in] src source configuration.
 * @param [out] dst destination configuration.
 */
ucs_status_t ucg_config_clone(const ucg_config_t *src, ucg_config_t **dst);

#endif
