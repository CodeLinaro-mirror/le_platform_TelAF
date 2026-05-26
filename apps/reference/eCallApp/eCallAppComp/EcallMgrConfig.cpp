/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EcallMgrConfig.hpp"

extern "C"
{
#include "legato.h"
}
#include "ApiEcall.hpp"
#include "EcallMgrEcall.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace ecall
{

//------------------------------------------------------------------------------
// Singleton access
//------------------------------------------------------------------------------

EcallMgrConfig& EcallMgrConfig::GetInstance()
{
    static EcallMgrConfig inst;
    return inst;
}

EcallMgrConfig::EcallMgrConfig() = default;

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

void EcallMgrConfig::Trim(std::string& s)
{
    const char* ws = " \t\r\n";
    const auto  b  = s.find_first_not_of(ws);
    const auto  e  = s.find_last_not_of(ws);

    if (b == std::string::npos)
    {
        s.clear();
    }
    else
    {
        s = s.substr(b, e - b + 1);
    }
}

bool EcallMgrConfig::IsValidVinChar(char c)
{
    if (c >= '0' && c <= '9')
    {
        return true;
    }

    if (c >= 'A' && c <= 'Z' && c != 'I' && c != 'O' && c != 'Q')
    {
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void EcallMgrConfig::SetVin(const std::string& vinIn)
{
    std::string vin = vinIn;

    // Trim and normalize to upper-case.
    Trim(vin);
    std::transform(
        vin.begin(), vin.end(), vin.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    // Basic VIN validation: 17 characters, allowed character set.
    if (vin.size() != 17 || !std::all_of(vin.begin(), vin.end(), IsValidVinChar))
    {
        LE_ERROR("EcallMgrConfig::SetVin: invalid VIN '%s'", vin.c_str());
        return;
    }

    // No change -> nothing to do.
    if (vin == vin_)
    {
        LE_INFO("EcallMgrConfig::SetVin: VIN unchanged");
        return;
    }

    // Store and propagate.
    vin_ = vin;
    ApplyVinInternal(vin_);

    LE_INFO("EcallMgrConfig::SetVin: VIN updated to '%s'", vin_.c_str());
}

void EcallMgrConfig::SetConfig(const EcConfigData& cfg)
{
    cfg_ = cfg;
    ApplyConfigInternal(cfg_);
}

//------------------------------------------------------------------------------
// Internal propagation to API/manager layers
//------------------------------------------------------------------------------

void EcallMgrConfig::ApplyVinInternal(const std::string& vin)
{
    // Forward VIN to lower eCall API wrapper.
    auto& apiEcall = ApiEcall::GetInstance();
    (void)apiEcall.SetVin(vin.c_str());

    LE_INFO("EcallMgrConfig::ApplyVinInternal: VIN='%s' applied to ApiEcall", vin.c_str());
}

void EcallMgrConfig::ApplyConfigInternal(const EcConfigData& cfg)
{
    LE_INFO("EcallMgrConfig::ApplyConfigInternal: "
            "MSD_Version:%d, dialAttempts:%d, dialDuration:%d, "
            "emergencyNumber:%s, testNumber:%s, t2:%d, t9:%d, t10:%d, vehicleType:%d",
            cfg.msdVersion,
            cfg.dialAttempts,
            cfg.dialDuration,
            cfg.emergencyNumber,
            cfg.testNumber,
            cfg.t2,
            cfg.t9,
            cfg.t10,
            cfg.vehicleType);

    // Normalise phone numbers via le_utf8_Copy.
    EcConfigData mgrCfg = cfg;
    std::memset(mgrCfg.testNumber,      0, sizeof(mgrCfg.testNumber));
    std::memset(mgrCfg.emergencyNumber, 0, sizeof(mgrCfg.emergencyNumber));
    le_utf8_Copy(mgrCfg.testNumber,      cfg.testNumber,
                 sizeof(mgrCfg.testNumber),      nullptr);
    le_utf8_Copy(mgrCfg.emergencyNumber, cfg.emergencyNumber,
                 sizeof(mgrCfg.emergencyNumber), nullptr);

    // 1. Push HLAP timers to taf_ecall.
    auto& api = ApiEcall::GetInstance();
    if (mgrCfg.t2 > 0 || mgrCfg.t9 > 0 || mgrCfg.t10 > 0)
    {
        le_result_t rc = api.SetHlapTimerConfig(
            static_cast<uint16_t>(mgrCfg.t2),
            static_cast<uint16_t>(mgrCfg.t9),
            static_cast<uint16_t>(mgrCfg.t10));
        if (rc != LE_OK)
            LE_ERROR("EcallMgrConfig: SetHlapTimerConfig failed rc=%d", rc);
    }

    // 2. Push redial config.
    if (mgrCfg.dialAttempts > 0 && mgrCfg.dialDuration > 0)
    {
        le_result_t rc = api.SetRedialConfig(
            static_cast<uint16_t>(mgrCfg.dialAttempts),
            static_cast<uint16_t>(mgrCfg.dialDuration));
        if (rc != LE_OK)
            LE_ERROR("EcallMgrConfig: SetRedialConfig failed rc=%d", rc);
    }

    // 3. Push MSD version.
    if (mgrCfg.msdVersion == 2 || mgrCfg.msdVersion == 3)
    {
        le_result_t rc = api.SetMsdVersion(
            static_cast<uint32_t>(mgrCfg.msdVersion));
        if (rc != LE_OK)
            LE_ERROR("EcallMgrConfig: SetMsdVersion failed rc=%d", rc);
    }

    // 4. Push vehicle info.
    {
        const taf_ecall_MsdVehicleType_t vtype =
            EcallMgrEcall::ConvertIntToVehicleType(mgrCfg.vehicleType);
        const taf_ecall_PropulsionStorageType_t pmask =
            EcallMgrEcall::ConvertFuelToPropulsionMask(mgrCfg);
        le_result_t rc = api.SetVehicleInfo(vtype, pmask);
        if (rc != LE_OK)
            LE_ERROR("EcallMgrConfig: SetVehicleInfo failed rc=%d", rc);
    }

    // 5. Store config in EcallMgrEcall for runtime use (number selection, etc.).
    EcallMgrEcall::GetInstance().SetConfig(mgrCfg);
}

} // namespace ecall
