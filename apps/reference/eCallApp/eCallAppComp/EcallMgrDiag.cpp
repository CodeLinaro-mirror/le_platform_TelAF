/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EcallMgrDiag.hpp"
#include "EcallEventBus.hpp"
#include "interfaces.h"

#include <ctime>

namespace ecall
{

#define LOG_NAME "EcallMgrDiag"

// =============================================================================
// Singleton
// =============================================================================
EcallMgrDiag& EcallMgrDiag::GetInstance()
{
    static EcallMgrDiag instance;
    return instance;
}

// =============================================================================
// Constructor: register HMS bus handler + start periodic timer
// =============================================================================
EcallMgrDiag::EcallMgrDiag()
{
    // Subscribe to HMS events on the bus (replaces EcallMgrHms).
    le_event_AddHandler(
        "ec.hms",
        EcallEventBus::GetInstance().HmsChannel(),
        EcallMgrDiag::OnHmsEvt);

    LE_INFO("[%s] HMS handler registered", LOG_NAME);

    // Create and start the periodic self-test timer.
    periodicTimer_ = le_timer_Create("EcallDiagPeriodicTimer");
    if (!periodicTimer_)
    {
        LE_ERROR("[%s] le_timer_Create failed", LOG_NAME);
        return;
    }

    le_timer_SetHandler(periodicTimer_, &EcallMgrDiag::TimerHandlerStatic);

    le_clk_Time_t interval{kHealthCheckPeriodSec, 0};
    le_result_t rc = le_timer_SetInterval(periodicTimer_, interval);
    if (rc != LE_OK)
    {
        LE_ERROR("[%s] le_timer_SetInterval failed rc=%d", LOG_NAME, rc);
        le_timer_Delete(periodicTimer_);
        periodicTimer_ = nullptr;
        return;
    }

    rc = le_timer_SetRepeat(periodicTimer_, 0);  // 0 = repeat forever
    if (rc != LE_OK)
    {
        LE_ERROR("[%s] le_timer_SetRepeat failed rc=%d", LOG_NAME, rc);
        le_timer_Delete(periodicTimer_);
        periodicTimer_ = nullptr;
        return;
    }

    rc = le_timer_Start(periodicTimer_);
    if (rc != LE_OK)
    {
        LE_ERROR("[%s] le_timer_Start failed rc=%d", LOG_NAME, rc);
        le_timer_Delete(periodicTimer_);
        periodicTimer_ = nullptr;
        return;
    }

    LE_INFO("[%s] periodic health-check timer started (interval=%lld s)",
            LOG_NAME, static_cast<long long>(kHealthCheckPeriodSec));
}

// =============================================================================
// HMS bus callback  (merged from EcallMgrHms)
// =============================================================================
void EcallMgrDiag::OnHmsEvt(void* payloadPtr)
{
    if (!payloadPtr)
    {
        LE_WARN("[%s] OnHmsEvt: null payload", LOG_NAME);
        return;
    }

    const auto* e = static_cast<const ModemHmsEvt*>(payloadPtr);

    LE_DEBUG("[%s] OnHmsEvt type=%d severity=%d",
             LOG_NAME,
             static_cast<int>(e->type),
             static_cast<int>(e->severity));

    // Report a modem DTC fault for Low-severity events.
    // (Medium / High severity may warrant different handling in the future.)
    if (e->severity == ModemEvtSeverity::Low)
    {
        GetInstance().ReportFaultByDTC(
            EVENT_Sample_Big,
            DTC_CODE_MODEM_FAULT,
            OPERATION_CYCLE_DC);
    }
}

// =============================================================================
// Periodic timer
// =============================================================================
void EcallMgrDiag::TimerHandlerStatic(le_timer_Ref_t /*timerRef*/)
{
    GetInstance().TimerHandler();
}

void EcallMgrDiag::TimerHandler()
{
    LE_INFO("[%s] periodic timer expired", LOG_NAME);
    RunHealthCheck();
}

// =============================================================================
// Health check helpers
// =============================================================================
bool EcallMgrDiag::IsNetworkOk()
{
    // TODO: implement real network health check via EcallMgrNetwork.
    LE_INFO("[%s] IsNetworkOk: stub -> assume NOT ok", LOG_NAME);
    return false;
}

// =============================================================================
// RunHealthCheck  (was PerformPeriodicSelfTestAndReport)
// =============================================================================
void EcallMgrDiag::RunHealthCheck()
{
    const int64_t nowSec = static_cast<int64_t>(time(nullptr));

    if (lastHealthCheckTimeSec_ != 0)
    {
        const int64_t delta = nowSec - lastHealthCheckTimeSec_;

        if (delta >= 0 && delta < kHealthCheckPeriodSec)
        {
            LE_INFO("[%s] PerformPeriodicSelfTest: skip (delta=%lld < %lld)",
                    LOG_NAME,
                    static_cast<long long>(delta),
                    static_cast<long long>(kHealthCheckPeriodSec));
            return;
        }

        if (delta < 0)
        {
            LE_WARN("[%s] PerformPeriodicSelfTest: system time moved backwards",
                    LOG_NAME);
        }
    }

        LE_INFO("[%s] RunHealthCheck: running", LOG_NAME);

    if (!IsNetworkOk())
    {
        LE_WARN("[%s] network fault detected", LOG_NAME);
        ReportFaultByDTC(EVENT_Sample_Small,
                         DTC_CODE_NETWORK_FAULT,
                         OPERATION_CYCLE_DC);
    }

    LE_INFO("[%s] RunHealthCheck: done", LOG_NAME);
}

// =============================================================================
// DTC reporting core
// =============================================================================
void EcallMgrDiag::ReportFaultByDTC(uint16_t eventId,
                                    uint32_t dtcCode,
                                    uint8_t  opCycleId)
{
    LE_INFO("[%s] ReportFaultByDTC eventId=%u dtc=0x%08x opCycle=%u",
            LOG_NAME,
            static_cast<unsigned>(eventId),
            static_cast<unsigned>(dtcCode),
            static_cast<unsigned>(opCycleId));

    auto& diag = ApiDiag::GetInstance();

    taf_diagEvent_ServiceRef_t  eventSvcRef    = nullptr;
    taf_diagDTC_ServiceRef_t    dtcSvcRef      = nullptr;
    taf_diagEvent_OpCycleRef_t  opCycleRef     = nullptr;

    const le_result_t r1 = diag.GetEventService(eventId,   &eventSvcRef);
    const le_result_t r2 = diag.GetDtcService  (dtcCode,   &dtcSvcRef);
    const le_result_t r3 = diag.GetOpCycle     (opCycleId, &opCycleRef);

    if (r1 != LE_OK || r2 != LE_OK || r3 != LE_OK)
    {
        LE_ERROR("[%s] ReportFaultByDTC: GetService failed r1=%d r2=%d r3=%d",
                 LOG_NAME, r1, r2, r3);
        return;
    }

    le_result_t r = diag.SetEnableCondition(CONDITION_TOOBIG, true);
    if (r != LE_OK)
    {
        LE_ERROR("[%s] SetEnableCondition failed rc=%d", LOG_NAME, r);
        return;
    }

    uint8_t dtcStatus = 0;
    r = diag.ReadDtcStatus(dtcSvcRef, &dtcStatus);
    if (r != LE_OK)
    {
        LE_ERROR("[%s] ReadDtcStatus (before) failed rc=%d", LOG_NAME, r);
        return;
    }
    LE_INFO("[%s] DTC status before: 0x%02x", LOG_NAME, dtcStatus);

    r = diag.SetOpCycleState(opCycleRef, TAF_DIAGEVENT_CYCLE_START);
    if (r != LE_OK)
    {
        LE_ERROR("[%s] SetOpCycleState(START) failed rc=%d", LOG_NAME, r);
        return;
    }

    r = diag.SetEventStatus(eventSvcRef, TAF_DIAGEVENT_FAILED);
    if (r != LE_OK)
    {
        LE_ERROR("[%s] SetEventStatus(FAILED) failed rc=%d", LOG_NAME, r);
        // Fall through to stop the cycle even on failure.
    }
    else
    {
        LE_INFO("[%s] SetEventStatus(FAILED) ok eventId=%u",
                LOG_NAME, static_cast<unsigned>(eventId));
    }

    r = diag.SetOpCycleState(opCycleRef, TAF_DIAGEVENT_CYCLE_STOP);
    if (r != LE_OK)
        LE_ERROR("[%s] SetOpCycleState(STOP) failed rc=%d", LOG_NAME, r);

    r = diag.ReadDtcStatus(dtcSvcRef, &dtcStatus);
    if (r != LE_OK)
        LE_ERROR("[%s] ReadDtcStatus (after) failed rc=%d", LOG_NAME, r);
    else
        LE_INFO("[%s] DTC status after: 0x%02x", LOG_NAME, dtcStatus);

    LE_INFO("[%s] ReportFaultByDTC done", LOG_NAME);
}

} // namespace ecall

