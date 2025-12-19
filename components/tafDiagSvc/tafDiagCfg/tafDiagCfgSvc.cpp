/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <openssl/evp.h>
#include <openssl/md5.h>
#include "configuration.hpp"

using namespace tafsvc::cfg;

static le_result_t ComputeFileMD5
(
    const char *filename,
    uint8_t md_value [MD5_DIGEST_LENGTH]
)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        LE_ERROR("File (%s) opening failed: %m", filename);
        return LE_FAULT;
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL)
    {
        LE_ERROR("EVP_MD_CTX_new failed: %m");
        fclose(file);
        return LE_FAULT;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_md5(), NULL) != 1)
    {
        LE_ERROR("EVP_DigestInit_ex failed: %m");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return LE_FAULT;
    }

    unsigned char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        if (EVP_DigestUpdate(mdctx, buffer, bytes_read) != 1)
        {
            LE_ERROR("EVP_DigestUpdate failed: %m");
            EVP_MD_CTX_free(mdctx);
            fclose(file);
            return LE_FAULT;
        }
    }

    if (EVP_DigestFinal_ex(mdctx, md_value, NULL) != 1)
    {
        LE_ERROR("EVP_DigestFinal_ex failed: %m");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return LE_FAULT;
    }

    EVP_MD_CTX_free(mdctx);
    fclose(file);

    return LE_OK;
}

static void ConvertHexToArray
(
    const char *hex_str,
    size_t hex_len,
    unsigned char *byte_array,
    size_t array_len
)
{
    LE_ASSERT(hex_len % 2 == 0);
    LE_ASSERT(array_len * 2 == hex_len); // Without '\0' in tail

    for (size_t i = 0; i < hex_len; i += 2) // Eat 2 chars one shot
    {
        sscanf(hex_str + i, "%2hhx", &byte_array[i / 2]);
    }
}

static void ConvertArrayToHex
(
    const uint8_t *byte_array,
    size_t length,
    char *hex_string,
    size_t hex_len
)
{
    LE_ASSERT(length * 2 + 1 == hex_len);

    for (size_t i = 0; i < length; i++)
    {
        snprintf(hex_string + (i * 2), hex_len, "%02x", byte_array[i]);
    }
    hex_string[length * 2] = '\0';
}

//--------------------------------------------------------------------------------------------------
/**
 * The initialization of TelAF diag configuration component.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    //Init configuration module.
    init();

    try
    {
        diag_config_init("./diag_template.yaml.json");
    }
    catch (const std::exception& e)
    {
        LE_ERROR("json file is not present");
        exit(EXIT_SUCCESS);
    }

    { /* Output all information tafDiagGen tool generated */
        LE_INFO("%s", tafDiagGen_tool_version);
        LE_INFO("%s", tafDiagGen_tool_timestamp);
        LE_INFO("%s", tafDiagGen_tool_json_md5);
        LE_INFO("%s", tafDiagGen_tool_evid_h_md5);
    }

    uint8_t json_md5_runtime[MD5_DIGEST_LENGTH];
    LE_ASSERT(
        ComputeFileMD5("./diag_template.yaml.json", json_md5_runtime)
        == LE_OK);

    LE_ASSERT(strlen(TAFDIAGGEN_JSON_MD5) / 2 == MD5_DIGEST_LENGTH);

    uint8_t json_md5_origin[MD5_DIGEST_LENGTH];
    ConvertHexToArray(TAFDIAGGEN_JSON_MD5,
                      strlen(TAFDIAGGEN_JSON_MD5),
                      json_md5_origin,
                      sizeof(json_md5_origin));

    if (memcmp(json_md5_runtime, json_md5_origin, MD5_DIGEST_LENGTH) != 0)
    {
        char json_md5_runtime_str[MD5_DIGEST_LENGTH * 2 + 1];
        ConvertArrayToHex(json_md5_runtime,
                          MD5_DIGEST_LENGTH,
                          json_md5_runtime_str,
                          sizeof(json_md5_runtime_str));

        LE_INFO("MD5 of JSON file (runtime): %s", json_md5_runtime_str);
        LE_INFO("MD5 of JSON file (tool-gn): %s", TAFDIAGGEN_JSON_MD5);

        LE_ERROR("MD5 of JSON file [Mismatched], FATAL!");
        exit(EXIT_SUCCESS);
    }
    else
    {
        LE_INFO("MD5 of JSON file [Matched], continue ...");
    }

}
