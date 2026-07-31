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
#include <optional>

namespace ecall
{

// ApiHms
// ------
// Thin wrapper around taf_hms_* service.
// - Connects to HMS service and registers a modem event handler.
// - Forwards modem events to Bus.
// - Keeps the last event in memory for synchronous query.
class ApiHms
{
public:
    // Get the singleton instance.
    static ApiHms& GetInstance();

private:
    ApiHms();
    ~ApiHms() = default;

    ApiHms(const ApiHms&) = delete;
    ApiHms& operator=(const ApiHms&) = delete;

    // Static callback registered with taf_hms service.
    // Uses ctx (this pointer) to dispatch to the instance.
    static void ModemEvtCallback(taf_hms_ModemEvtType_t eventType,
                           taf_hms_ModemEvtSeverity_t eventSeverity,
                           taf_hms_ModemEventRef_t eventRef,
                           void* contextPtr);

    // Handle one modem event:
    // - convert to HmsEvt
    // - publish to Bus
    // - store as lastEvt_
    void HandleModemEvt(taf_hms_ModemEvtType_t eventType,
                   taf_hms_ModemEvtSeverity_t eventSeverity);
};

} // namespace ecall
