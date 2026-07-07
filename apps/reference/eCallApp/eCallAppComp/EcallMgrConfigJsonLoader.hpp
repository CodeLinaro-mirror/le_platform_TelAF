/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

extern "C"
{
    #include "legato.h"
    #include "interfaces.h"
}

#include "EcallDefs.hpp"

#include <string>

namespace ecall
{

// -----------------------------------------------------------------------------
// EcallMgrConfigJsonLoader
//
// Singleton responsible for loading eCall configuration from a JSON file and
// applying it to EcallMgrConfig.
//
// Typical flow:
//   - At first GetInstance() call, it tries to load a default JSON
//     ("/tmp/ecall_config.json") once.
//   - LoadFromFile(path) can be called explicitly at any time to
//     re-load configuration from a specific path.
//
// JSON fields it understands:
//   - Numbers: msdVersion, dialAttempts, dialDuration, t2, t9, t10, vehicleType,
//              fuelElectric, fuelDiesel, fuelGasoline, fuelCompressed,
//              fuelLiquid, fuelHydrogen, fuelOther
//   - Strings: testNumber, emergencyNumber, vin
//
// VIN and configuration are forwarded to EcallMgrConfig:
//   - EcallMgrConfig::SetVin(vin)
//   - EcallMgrConfig::SetConfig(cfg)
// -----------------------------------------------------------------------------
class EcallMgrConfigJsonLoader
{
public:
    // Get singleton instance.
    static EcallMgrConfigJsonLoader& GetInstance();

    // Load configuration from the specified JSON file.
    //
    // Returns:
    //   - LE_OK on success
    //   - LE_BAD_PARAMETER if jsonPath is null
    //   - LE_NOT_FOUND if file cannot be opened
    //   - other LE_* values depending on JSON parse errors
    le_result_t LoadFromFile(const char* jsonPath);

private:
    EcallMgrConfigJsonLoader();
    ~EcallMgrConfigJsonLoader() = default;

    EcallMgrConfigJsonLoader(const EcallMgrConfigJsonLoader&)            = delete;
    EcallMgrConfigJsonLoader& operator=(const EcallMgrConfigJsonLoader&) = delete;

    // Parse JSON from an open file descriptor.
    // Fills outCfg and outVin. Returns LE_OK unless parsing fails.
    static le_result_t ParseJson(int fd,
                                 EcConfigData& outCfg,
                                 std::string& outVin);
};

} // namespace ecall

