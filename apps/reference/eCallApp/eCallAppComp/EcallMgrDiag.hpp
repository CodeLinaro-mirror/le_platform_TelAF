/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <cstdint>

#include "EcallDefs.hpp"
#include "ApiDiag.hpp"

extern "C"
{
#include "legato.h"
}

namespace ecall
{

// -----------------------------------------------------------------------------
// EcallMgrDiag
//
// Diagnostic and health-monitoring manager for the eCall stack.
//
// Responsibilities:
//   1. HMS event subscription:
//        Registers a handler on EcallEventBus for ModemHmsEvt in the
//        constructor (same pattern as EcallMgrLocation).  On each event,
//        OnHmsEvt() decides whether to report a DTC fault.
//
//   2. DTC fault reporting:
//        ReportFaultByDTC() drives the full taf_diagEvent / taf_diagDTC
//        sequence: GetService -> SetEnableCondition -> ReadDtcStatus ->
//        StartCycle -> SetEventStatus(FAILED) -> StopCycle -> ReadDtcStatus.
//
//   3. Periodic self-test:
//        A Legato repeating timer fires every kHealthCheckPeriodSec seconds.
//        PerformPeriodicSelfTestAndReport() is called on each expiry and
//        checks network health (stub: always reports fault for now).
//
// Note: EcallMgrHms has been merged into this class.  There is no longer a
// separate EcallMgrHms singleton; EcallMgrDiag subscribes to the HMS bus
// event directly and handles it inline.
//
// Singleton; use GetInstance() for access.
// -----------------------------------------------------------------------------
class EcallMgrDiag
{
public:
    static EcallMgrDiag& GetInstance();

        // Run the full DTC reporting sequence for the given event / DTC / cycle.
    void ReportFaultByDTC(uint16_t eventId,
                          uint32_t dtcCode,
                          uint8_t  opCycleId);

    // Periodic health check: queries network state and reports any faults.
    // Also called by the internal repeating timer.
    void RunHealthCheck();

private:
    EcallMgrDiag();
    ~EcallMgrDiag() = default;

    EcallMgrDiag(const EcallMgrDiag&)            = delete;
    EcallMgrDiag& operator=(const EcallMgrDiag&) = delete;

    // HMS bus callback (registered in constructor).
    static void OnHmsEvt(void* payloadPtr);

    // Network health check (stub).
    bool IsNetworkOk();

    // Periodic timer callback.
    static void TimerHandlerStatic(le_timer_Ref_t timerRef);
    void        TimerHandler();

    // Timestamp of the last effective periodic health check (UTC seconds).
    int64_t lastHealthCheckTimeSec_{0};

    // Minimum interval between two effective periodic checks.
    // Set to 600 s for development; change to 2 * 24 * 3600 for production.
    static constexpr int64_t kHealthCheckPeriodSec = 600;

    le_timer_Ref_t periodicTimer_{nullptr};
};

} // namespace ecall

