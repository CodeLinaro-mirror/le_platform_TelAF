/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include "EcallDefs.hpp"  // EcNetworkStatus, EcSignalStrength

namespace ecall
{

// -----------------------------------------------------------------------------
// EcallMgrNetwork
//
// eCall-oriented facade over ApiRadio.
//
// Responsibilities:
//   - GetNetworkStatus : query current RAT + PLMN (MCC/MNC) in one call.
//   - GetSignalStrength: dispatch to the correct RAT-specific signal-metrics
//     API and return a normalised EcSignalStrength.
//
// Both methods are called only from EcallMgrEcall::SaveCallData() at the end
// of an eCall session, so they are on-demand (not polled).
//
// EcNetworkStatus and EcSignalStrength are defined in EcallDefs.hpp
// so that EcallMgrEcall and EcallMgrDataLogger can use them without pulling
// in ApiRadio headers.
//
// Singleton; use GetInstance() for access.
// -----------------------------------------------------------------------------
class EcallMgrNetwork
{
public:
    static EcallMgrNetwork& GetInstance();

    // Query current RAT and PLMN for phoneId.
    // Always returns LE_OK; individual field failures are logged internally.
    le_result_t GetNetworkStatus(uint8_t phoneId, EcNetworkStatus& outStatus);

    // Query signal strength for the current RAT on phoneId.
    // Returns LE_OK on success, LE_UNSUPPORTED for unknown RAT,
    // or the underlying ApiRadio error code on failure.
    le_result_t GetSignalStrength(uint8_t phoneId, EcSignalStrength& outSig);

private:
    EcallMgrNetwork()  = default;
    ~EcallMgrNetwork() = default;

    EcallMgrNetwork(const EcallMgrNetwork&)            = delete;
    EcallMgrNetwork& operator=(const EcallMgrNetwork&) = delete;
};

} // namespace ecall


