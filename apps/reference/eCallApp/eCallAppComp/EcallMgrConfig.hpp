/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear 
 */

#pragma once

#include "EcallDefs.hpp"

#include <string>
#include <cstdint>

namespace ecall
{

// -----------------------------------------------------------------------------
// EcallMgrConfig
//
// Singleton responsible for holding the current eCall configuration (EcallConfigData)
// and the VIN, and for propagating updates to lower layers:
//
//   - VIN changes -> forwarded to ApiEcall (setVin())
//   - Config changes -> forwarded to EcallMgrEcall (setConfig())
//
// This class itself is purely C++ and does NOT expose Legato symbols in the header.
// Legato-specific APIs are only used inside the .cpp implementation.
// -----------------------------------------------------------------------------
class EcallMgrConfig
{
public:
    // Get singleton instance.
    static EcallMgrConfig& GetInstance();

    // Current effective configuration (last applied).
    const EcConfigData& GetConfig() const { return cfg_; }

    // Current VIN (upper-case, validated, 17 characters).
    const std::string& GetVin() const { return vin_; }

    // Set VIN string from upper layer (e.g., JSON/config). This function:
    //   - trims whitespace,
    //   - converts to upper-case,
    //   - validates VIN format (17 characters, no I/O/Q),
    //   - if valid and changed, stores it and calls ApplyVinInternal().
    void SetVin(const std::string& vinIn);

    // Set full eCall configuration from upper layers.
    // The configuration is copied and then passed to ApplyConfigInternal().
    void SetConfig(const EcConfigData& cfg);

    // Helpers used by other components (e.g., config loader / tests).
    static void Trim(std::string& s);
    static bool IsValidVinChar(char c);

private:
    EcallMgrConfig();
    ~EcallMgrConfig() = default;

    EcallMgrConfig(const EcallMgrConfig&) = delete;
    EcallMgrConfig& operator=(const EcallMgrConfig&) = delete;

    // Push the VIN down to the eCall API layer.
    void ApplyVinInternal(const std::string& vin);

    // Push configuration down to the eCall manager (EcallMgrEcall).
    void ApplyConfigInternal(const EcConfigData& cfg);

private:
    // Last applied configuration.
    EcConfigData cfg_{};

    // VIN (normalized, upper-case, validated).
    std::string vin_;
};

} // namespace ecall
