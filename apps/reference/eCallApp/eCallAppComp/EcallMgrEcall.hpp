/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "EcallDefs.hpp"
#include "ApiEcall.hpp"
#include "EcallMgrEventInterpreter.hpp"

namespace ecall
{

// Delta in 100 milliarcseconds (100 mas ≈ 3 m on Earth), clamped to [-512..511].
struct RecentDelta100Mas
{
    int16_t latDelta{0};  // +North / -South (WGS84), unit: 100 mas
    int16_t lonDelta{0};  // +East  / -West  (WGS84), unit: 100 mas
    bool    valid{false};
};

// -----------------------------------------------------------------------------
// EcallMgrEcall
//
// Central eCall manager.  Owns the eCall session state machine and
// coordinates all eCall-related subsystems:
//
//   ApiEcall        – taf_ecall IPC (MSD, dial, config)
//   ApiAudio        – voice path / prompt control
//   EcallMgrPower   – wakeup lock lifecycle
//   EcallMgrDiag    – DTC fault reporting
//   EcallMgrNetwork – RAT / signal snapshot at call end
//   EcallMgrDataLogger – call record persistence
//
// State machine (ETSI EN 16062 / EN 15722):
//
//   Idle ──CallStarted──► CallSetup ──CallConnected──► Connected
//                                                          │
//                                              MsdRequested│
//                                                          ▼
//                                                     SendingMsd
//                                                          │
//                                          MsdInbandSuccess│MsdInbandFailed
//                                                          ▼
//   Callback ◄──CallEnded──────────────────────────────── Voice
//       │
//   CallbackWindowTimeout
//       │
//       ▼
//     Idle
//
// Note: EcallMgrPsap has been merged into this class.  There is no longer
// a separate EcallMgrPsap singleton; all ApiEcall calls are made directly
// from the private helpers below.
//
// Singleton; use GetInstance() for access.
// -----------------------------------------------------------------------------
class EcallMgrEcall
{
public:
    static EcallMgrEcall& GetInstance();

    // -------------------------------------------------------------------------
    // VIN and configuration
    // -------------------------------------------------------------------------
    void        SetVin(const char* vin);
    le_result_t GetVin(char* outBuf, size_t outLen) const;

    static const char*                        VehicleTypeToName(int v);
    static taf_ecall_MsdVehicleType_t         ConvertIntToVehicleType(int v);
    static std::string                        FuelToName(const EcConfigData& c);
    static taf_ecall_PropulsionStorageType_t  ConvertFuelToPropulsionMask(const EcConfigData& c);

    void SetConfig(const EcConfigData& cfg);
    const EcConfigData& GetLastConfig() const { return lastCfg_; }

    // -------------------------------------------------------------------------
    // Location ring buffer and MSD recent position
    // -------------------------------------------------------------------------
    void LocRingBufferHandler(const LocConverted& d);
    std::optional<LocConverted> RingReadAt(size_t relIndex) const;
    bool ComputeAndSetRecentVehicleLocationsFromRing();
    void SetRecentVehicleLocationN1(int16_t latDelta100Mas, int16_t lonDelta100Mas);
    void SetRecentVehicleLocationN2(int16_t latDelta100Mas, int16_t lonDelta100Mas);
    bool PushZeroBaseMsdWithRecent();
    const MsdPosition& GetLastMsdPosition() const { return lastMsdPos_; }

    // -------------------------------------------------------------------------
    // Bus callbacks
    // -------------------------------------------------------------------------
    static void OnPhase(void* payloadPtr);
    static void OnPsapSig(void* payloadPtr);
    static void OnTimer(void* payloadPtr);

    void HandlePhase(const EvPhase* e);
    void HandlePsapSig(const EvPsap* e);
    void HandleTimer(const EvTimer* e);

    static void RegisterBusHandlers();

    // -------------------------------------------------------------------------
    // Start an eCall session of the given type.
    // Drives the state machine to CallSetup and initiates the dial sequence.
    void StartEcall(EcCallType callType);

    void ReportOperationResult(const char* opName, le_result_t rc);

private:
    EcallMgrEcall();

    void LocRingBufferWrite(const LocConverted& d, const char* statusStr);

    void StartLocationWaitTimer();
    void StopLocationWaitTimer();
    static void OnLocationWaitTimeout(le_timer_Ref_t timerRef);

    static bool    IsLatMasValid(int32_t latMas);
    static bool    IsLonMasValid(int32_t lonMas);
    static bool    IsDirectionValid(int32_t direction);
    static int16_t ToDelta100Mas(int32_t refMas, int32_t sampleMas);

    // -------------------------------------------------------------------------
    // eCall state machine
    // -------------------------------------------------------------------------
    EcSessionState state_{EcSessionState::Idle};
    void HandleEcEvent(EcEvent ev);

    // Event translator: owned by this class, not a singleton.
    EcallMgrEventInterpreter interp_;

    // -------------------------------------------------------------------------
    // ApiEcall helpers (merged from EcallMgrPsap)
    // -------------------------------------------------------------------------
    void        DoSetVehicleData();
    void        DoSetLocationData();
    void        DoSendMsd();
    le_result_t DoGetMsdContent(std::vector<uint8_t>& msdContent);
    void        DoAnswerIncomingCall();
    void        DoDialCall(EcCallType callType);

    void DispatchActions(const std::vector<EcAction>& actions);
    void SaveCallData();

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------
    EcPhase lastPhase_{EcPhase::Idle};
    PsapSig lastPsap_{PsapSig::LlAck};

    static constexpr size_t kRingSize = 64;
    LocConverted ring_[kRingSize];
    size_t       head_{0};
    size_t       count_{0};

    RecentDelta100Mas msdRecentN1_{};
    RecentDelta100Mas msdRecentN2_{};

    char         vinCached_[18]{};
    EcConfigData lastCfg_{};
    MsdPosition  lastMsdPos_{};

    // -------------------------------------------------------------------------
    // Pending dial: set when DoDialCall() must wait for the first GNSS sample.
    // If a sample arrives before timeout, LocRingBufferHandler() updates MSD
    // location and dials. If no sample arrives within 3 seconds, the timeout
    // path updates MSD with unknown-position sentinel values and dials anyway.
    // -------------------------------------------------------------------------
    bool          locationReadyPending_{false};
    EcCallType    pendingCallType_{EcCallType::Manual};
    le_timer_Ref_t locationWaitTimer_{nullptr};

    // -------------------------------------------------------------------------
    // Incoming call answer delay.
    // On MT eCall (Callback state), delay Answer() by kAnswerDelayMs so the
    // IVS alert tone plays before the PSAP voice path opens.
    // -------------------------------------------------------------------------
    static constexpr uint32_t kAnswerDelayMs = 5000;
    le_timer_Ref_t            answerDelayTimer_{nullptr};

    void        StartAnswerDelayTimer();
    static void OnAnswerDelayTimeout(le_timer_Ref_t timerRef);
};

} // namespace ecall


