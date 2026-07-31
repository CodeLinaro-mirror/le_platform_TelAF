/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EcallMgrConfigJsonLoader.hpp"
#include "EcallMgrConfig.hpp"

extern "C"
{
#include "legato.h"
}

#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

namespace ecall
{

// =============================================================================
// Internal JSON parsing context
// =============================================================================

struct JsonCfgParseCtx
{
    EcConfigData* cfg{};
    std::string*  vin{};
    char          currentKey[64]{};

    int  fd{-1};
    bool hasError{false};
    bool done{false};
};

// Legato JSON does not provide le_json_GetCallbackContext().
// Use a file-local parse context for the active parsing session.
static JsonCfgParseCtx* CurrentJsonCfgCtx = nullptr;

static void JsonCfgEventHandler(le_json_Event_t event)
{
    auto* ctx = CurrentJsonCfgCtx;
    if (!ctx)
    {
        LE_ERROR("JsonCfgEventHandler: null context");
        return;
    }

    auto& c = *ctx->cfg;

    switch (event)
    {
        case LE_JSON_OBJECT_MEMBER:
        {
            const char* key = le_json_GetString();
            if (!key) key = "";
            le_utf8_Copy(ctx->currentKey, key, sizeof(ctx->currentKey), nullptr);
            break;
        }

        case LE_JSON_NUMBER:
        {
            const int32_t v = static_cast<int32_t>(le_json_GetNumber());
            LE_DEBUG("JSON NUMBER key='%s' value=%d", ctx->currentKey, static_cast<int>(v));

            if      (strcmp(ctx->currentKey, "msdVersion")     == 0) c.msdVersion     = v;
            else if (strcmp(ctx->currentKey, "dialAttempts")   == 0) c.dialAttempts   = v;
            else if (strcmp(ctx->currentKey, "dialDuration")   == 0) c.dialDuration   = v;
            else if (strcmp(ctx->currentKey, "t2")             == 0) c.t2             = v;
            else if (strcmp(ctx->currentKey, "t9")             == 0) c.t9             = v;
            else if (strcmp(ctx->currentKey, "t10")            == 0) c.t10            = v;
            else if (strcmp(ctx->currentKey, "vehicleType")    == 0) c.vehicleType    = v;
            else if (strcmp(ctx->currentKey, "fuelElectric")   == 0) c.fuelElectric   = v;
            else if (strcmp(ctx->currentKey, "fuelDiesel")     == 0) c.fuelDiesel     = v;
            else if (strcmp(ctx->currentKey, "fuelGasoline")   == 0) c.fuelGasoline   = v;
            else if (strcmp(ctx->currentKey, "fuelCompressed") == 0) c.fuelCompressed = v;
            else if (strcmp(ctx->currentKey, "fuelLiquid")     == 0) c.fuelLiquid     = v;
            else if (strcmp(ctx->currentKey, "fuelHydrogen")   == 0) c.fuelHydrogen   = v;
            else if (strcmp(ctx->currentKey, "fuelOther")      == 0) c.fuelOther      = v;
            break;
        }

        case LE_JSON_STRING:
        {
            const char* s = le_json_GetString();
            if (!s) s = "";
            LE_DEBUG("JSON STRING key='%s' value='%s'", ctx->currentKey, s);

            if (strcmp(ctx->currentKey, "testNumber") == 0)
                le_utf8_Copy(c.testNumber, s, sizeof(c.testNumber), nullptr);
            else if (strcmp(ctx->currentKey, "emergencyNumber") == 0)
                le_utf8_Copy(c.emergencyNumber, s, sizeof(c.emergencyNumber), nullptr);
            else if (strcmp(ctx->currentKey, "vin") == 0 && ctx->vin)
                *(ctx->vin) = s;
            break;
        }

        case LE_JSON_DOC_END:
        {
            le_json_ParsingSessionRef_t sess = le_json_GetSession();
            if (sess) le_json_Cleanup(sess);

            if (ctx->fd >= 0) { close(ctx->fd); ctx->fd = -1; }
            ctx->done = true;
            break;
        }

        default:
            break;
    }
}

static void JsonCfgErrorHandler(le_json_Error_t error, const char* msg)
{
    LE_ERROR("Ecall JSON parse error=%d msg=%s",
             static_cast<int>(error), msg ? msg : "");

    auto* ctx = CurrentJsonCfgCtx;
    if (ctx)
    {
        ctx->hasError = true;
        ctx->done     = true;
        if (ctx->fd >= 0) { close(ctx->fd); ctx->fd = -1; }
    }

    le_json_ParsingSessionRef_t sess = le_json_GetSession();
    if (sess) le_json_Cleanup(sess);
}

// =============================================================================
// ParseJson
// =============================================================================

le_result_t EcallMgrConfigJsonLoader::ParseJson(int          fd,
                                                EcConfigData& outCfg,
                                                std::string&  outVin)
{
    JsonCfgParseCtx ctx{};
    ctx.cfg = &outCfg;
    ctx.vin = &outVin;
    ctx.fd  = fd;

        CurrentJsonCfgCtx = &ctx;

    le_json_ParsingSessionRef_t sessRef =
        le_json_Parse(fd, JsonCfgEventHandler, JsonCfgErrorHandler, nullptr);

    if (!sessRef)
    {
        LE_ERROR("EcallMgrConfigJsonLoader::ParseJson: le_json_Parse failed");
        if (ctx.fd >= 0) { close(ctx.fd); ctx.fd = -1; }
        return LE_FAULT;
    }

        while (!ctx.done)
    {
        le_event_ServiceLoop();
    }

    CurrentJsonCfgCtx = nullptr;

    if (ctx.hasError)
    {
        LE_ERROR("ParseJson: finished with error");
        return LE_FAULT;
    }

    LE_INFO("ParseJson OK: msdVersion=%d dialAttempts=%d dialDuration=%d "
            "t2=%d t9=%d t10=%d vehicleType=%d",
            outCfg.msdVersion, outCfg.dialAttempts, outCfg.dialDuration,
            outCfg.t2, outCfg.t9, outCfg.t10, outCfg.vehicleType);
    return LE_OK;
}

// =============================================================================
// Singleton
// =============================================================================

EcallMgrConfigJsonLoader& EcallMgrConfigJsonLoader::GetInstance()
{
    static EcallMgrConfigJsonLoader inst;
    return inst;
}

EcallMgrConfigJsonLoader::EcallMgrConfigJsonLoader()
{
    // Best-effort load of the default config file at construction time.
    // Failure is non-fatal: the system will use compiled-in defaults.
    constexpr const char* kJsonPath = "/data/ecall_config.json";
    LE_INFO("EcallMgrConfigJsonLoader: loading default config '%s'", kJsonPath);
    (void)LoadFromFile(kJsonPath);
}

// =============================================================================
// Public API
// =============================================================================

le_result_t EcallMgrConfigJsonLoader::LoadFromFile(const char* jsonPath)
{
    if (!jsonPath)
    {
        LE_ERROR("EcallMgrConfigJsonLoader::LoadFromFile: null path");
        return LE_BAD_PARAMETER;
    }

    const int fd = open(jsonPath, O_RDONLY);
    if (fd < 0)
    {
        LE_ERROR("EcallMgrConfigJsonLoader: open '%s' failed errno=%d",
                 jsonPath, errno);
        return LE_NOT_FOUND;
    }

    EcConfigData cfg{};
    std::string  vin;

    const le_result_t r = ParseJson(fd, cfg, vin);
    // fd is closed inside ParseJson (in DOC_END or error handler).

    if (r != LE_OK)
    {
        LE_ERROR("EcallMgrConfigJsonLoader::LoadFromFile: parse failed rc=%d", r);
        return r;
    }

    auto& mgr = EcallMgrConfig::GetInstance();
    if (!vin.empty()) mgr.SetVin(vin);
    mgr.SetConfig(cfg);

    LE_INFO("EcallMgrConfigJsonLoader::LoadFromFile('%s') OK", jsonPath);
    return LE_OK;
}

} // namespace ecall

