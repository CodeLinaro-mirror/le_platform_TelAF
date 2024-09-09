/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "TafPiCfgStor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jansson.h"

le_result_t process_json_file(const char *output_file, const char *json_file)
{
    json_t *data, *merged_data, *group;
    json_error_t error;

    // Load the new JSON file
    data = json_load_file(json_file, 0, &error);
    if (!data) {
        LE_ERROR("Error loading JSON file: %s", error.text);
        return LE_BAD_PARAMETER;
    }

    // Load or initialize the merged data
    merged_data = json_load_file(output_file, 0, &error);
    if (!merged_data) {
        merged_data = json_object();
        json_object_set_new(merged_data, "name", json_string("MSS data config"));
        json_object_set_new(merged_data, "type", json_string("stem"));
        json_object_set_new(merged_data, "children", json_array());
    }

    // Extract version information from the new file
    const char *version =
        json_string_value(json_object_get(json_object_get(data, "configuration_header"),
                                                            "configuration_version"));
    int major_version, minor_version, patch_version;
    sscanf(version, "%d.%d.%d", &major_version, &minor_version, &patch_version);

    // Check if version information is already added
    json_t *children = json_object_get(merged_data, "children");
    size_t index;
    json_t *value;
    int version_exists = 0;
    json_array_foreach(children, index, value) {
        const char *name = json_string_value(json_object_get(value, "name"));
        if (strcmp(name, "MajorVersion") == 0) {
            version_exists = 1;
            break;
        }
    }

    if (!version_exists) {
        json_array_append_new(children, json_pack("{s:s, s:s, s:i}", "name", "MajorVersion", "type", "int", "value", major_version));
        json_array_append_new(children, json_pack("{s:s, s:s, s:i}", "name", "MinorVersion", "type", "int", "value", minor_version));
        json_array_append_new(children, json_pack("{s:s, s:s, s:i}", "name", "PatchVersion", "type", "int", "value", patch_version));
    } else {
        // Compare version information
        int existing_major_version = json_integer_value(json_object_get(json_array_get(children, 0), "value"));
        int existing_minor_version = json_integer_value(json_object_get(json_array_get(children, 1), "value"));
        int existing_patch_version = json_integer_value(json_object_get(json_array_get(children, 2), "value"));
        if (major_version != existing_major_version ||
            minor_version != existing_minor_version ||
            patch_version != existing_patch_version) {
            LE_ERROR("Error: Version mismatch with %s", json_file);
            return LE_BAD_PARAMETER;
        }
    }

    // Get configuration_id and convert to data_group_id
    const char *data_group_id =
        json_string_value(json_object_get(json_object_get(data, "configuration_header"),
                                                            "configuration_id"));

    // Check if data_group_id already exists
    json_array_foreach(children, index, value) {
        const char *name = json_string_value(json_object_get(value, "name"));
        if (strcmp(name, data_group_id) == 0) {
            LE_ERROR("Error: data_group_id '%s' already exists.", data_group_id);
            return LE_BAD_PARAMETER;
        }
    }

    group = json_pack("{s:s, s:s, s:[]}", "name", data_group_id, "type", "stem", "children");

    // Scan "configuration_list" and add "key" and "value" under the data_group_id
    json_t *config_list = json_object_get(data, "configuration_list");
    json_array_foreach(config_list, index, value) {
        const char *key = json_string_value(json_object_get(value, "configuration_key"));
        const char *val = json_string_value(json_object_get(value, "configuration_value"));
        if (key && val) {
            json_array_append_new(json_object_get(group, "children"),
                                    json_pack("{s:s, s:s, s:s}",
                                    "name", key, "type", "string", "value", val));
        }
    }

    json_array_append_new(children, group);

    // Save the updated merged data
    if (json_dump_file(merged_data, output_file, JSON_INDENT(4)) != 0) {
        LE_ERROR("Error saving merged JSON file.\n");
        return LE_FAULT;
    }

    json_decref(data);
    json_decref(merged_data);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get Config storage information table.
 *
 * @return
 * - NULL   -- Failed.
 * - Others -- DID Storage module information.
 */
//--------------------------------------------------------------------------------------------------
void* taf_hal_GetModInf()
{
    LE_INFO("Get Config storage module information table.");

    return &(TAF_HAL_INFO_TAB.cfgStorInf);
}

//--------------------------------------------------------------------------------------------------
/**
 * Authenticate file.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t taf_pi_CfgStor_Auth
(
    const char* filePath
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Merge file.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t taf_pi_CfgStor_Merge
(
    const char* outputFilePath,
    const char* configFilePath
)
{
    return process_json_file(outputFilePath, configFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize Config Storage plug-in module.
 */
//--------------------------------------------------------------------------------------------------
void taf_pi_CfgStor_Init
(
    void
)
{
    LE_INFO("Config storage plug-in module Init...");

    // Initialization

    LE_INFO("Config storage Plug-In Ready...");
}


//--------------------------------------------------------------------------------------------------
/**
 * Did Storage information table.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED pi_cfgStor_InfoTab_t TAF_HAL_INFO_TAB =
{
    .mgrInf =
    {
        .name = TAF_CFGSTOR_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .moduleType = TAF_MODULETYPE_PLUG_IN,
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    },

    .cfgStorInf =
    {
        .init = taf_pi_CfgStor_Init,
        .auth = taf_pi_CfgStor_Auth,
        .merge = taf_pi_CfgStor_Merge
    },
};

// DOT NOT USE COMPONET_INIT define module init for your service/apps
COMPONENT_INIT
{
    LE_INFO("Plugin is loading");
}
