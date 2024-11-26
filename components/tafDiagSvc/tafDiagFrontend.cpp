/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDataIDSvr.hpp"
#include "tafSecuritySvr.hpp"
#include "tafUpdateSvr.hpp"
#include "tafRoutineCtrlSvr.hpp"
#include "tafResetSvr.hpp"
#include "tafIOCtrlSvr.hpp"
#include "configuration.hpp"
#include "tafDiagBackend.hpp"
#include "tafDiagSvr.hpp"

#include <openssl/evp.h>
#include <openssl/md5.h>

#ifndef LE_CONFIG_DIAG_VSTACK
#include "tafEventSvr.hpp"
#include "tafSnapshotSvc.hpp"
#include "tafDTCInf.hpp"
#include "tafDTCSvr.hpp"
#include "tafDiagDoIPSvr.hpp"
#include "tafAuthSvr.hpp"
#endif

using namespace telux::tafsvc;

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
 * The initialization of TelAF diag service component.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{

    try
    {
        cfg::diag_config_init("./diag_template.yaml.json");
    }
    catch (const std::exception& e)
    {
        LE_FATAL("json file is not present");
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

        LE_FATAL("MD5 of JSON file [Mismatched], FATAL!");
    }
    else
    {
        LE_INFO("MD5 of JSON file [Matched], continue ...");
    }

    LE_INFO("TelAF Diag service initialization start...");
    auto& diag = taf_DiagSvr::GetInstance();
    diag.Init();
    LE_INFO("TelAF Diag service initialization end...");

    LE_INFO("TelAF UDS DataID service initialization start...");
    auto& did = taf_DataIDSvr::GetInstance();
    did.Init();
    LE_INFO("TelAF UDS DataID service initialization end...");

    LE_INFO("TelAF UDS Security service initialization start...");
    auto& tafSecurity = taf_SecuritySvr::GetInstance();
    tafSecurity.Init();
    LE_INFO("TelAF UDS Security service initialization end...");

    LE_INFO("TelAF UDS update service initialization start...");
    auto& tafUpdateSvr = taf_UpdateSvr::GetInstance();
    tafUpdateSvr.Init();
    LE_INFO("TelAF UDS update service initialization end...");

    LE_INFO("TelAF UDS routine conctrol service initialization start...");
    auto& tafRCS = taf_RoutinCtrlSvr::GetInstance();
    tafRCS.Init();
    LE_INFO("TelAF UDS routine conctrol service initialization end...");

    auto &reset = taf_ResetSvr::GetInstance();
    reset.Init();
    LE_INFO("TelAF UDS update service initialization end...");

    LE_INFO("TelAF IOCtrl service initialization start...");
    auto &ioCtrl = taf_IOCtrlSvr::GetInstance();
    ioCtrl.Init();
    LE_INFO("TelAF IOCtrl service initialization end...");

#ifndef LE_CONFIG_DIAG_VSTACK
    LE_INFO("TelAF Event Management service initialization start...");
    auto& event = taf_EventSvr::GetInstance();
    event.Init();
    LE_INFO("TelAF Event Management service initialization end...");

    LE_INFO("TelAF DTC service initialization start...");
    auto& dtcSvc = taf_DTCSvr::GetInstance();
    dtcSvc.Init();
    LE_INFO("TelAF DTC service initialization end...");

    taf_DataAccess_Init();

    LE_INFO("TelAF UDS DTC interface initialization start...");
    auto& dtcInf = taf_DTCInf::GetInstance();
    dtcInf.Init();
    LE_INFO("TelAF UDS DTC interface initialization end...");

    LE_INFO("TelAF Snapshot service initialization start...");
    auto& snapshot = taf_SnapshotSvr::GetInstance();
    snapshot.Init();
    LE_INFO("TelAF Snapshot service initialization end...");

    LE_INFO("TelAF DoIP service initialization start...");
    auto& doipSvc = taf_DiagDoIPSvr::GetInstance();
    doipSvc.Init();
    LE_INFO("TelAF DoIP service initialization end...");

    LE_INFO("TelAF Authentication service initialization start...");
    auto& authSvc = taf_AuthSvr::GetInstance();
    authSvc.Init();
    LE_INFO("TelAF Authentication service initialization end...");

    LE_INFO("TelAF Diag Backend initialization start...");
    auto& tafBackend = taf_DiagBackend::GetInstance();
    tafBackend.Init();
    LE_INFO("TelAF Diag Backend initialization end...");

#endif
    // Add boot KPI marker
    const char *kpi_file = "/sys/kernel/boot_kpi/kpi_values";
    const char *kpi_marker = "L - TelAF diagnostic service is ready";
    FILE *file = fopen(kpi_file, "w");
    if (file == NULL)
    {
        LE_ERROR("%s does not exist", kpi_file);
        return;
    }
    if (fwrite(kpi_marker, sizeof(char), strlen(kpi_marker), file) != strlen(kpi_marker))
    {
        LE_ERROR("failed to write %s to %s", kpi_marker, kpi_file);
    }
    fclose(file);
}
