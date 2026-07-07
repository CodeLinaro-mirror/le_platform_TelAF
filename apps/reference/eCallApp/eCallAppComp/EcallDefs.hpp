/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

// EcallDefs.hpp
//
// Central header for the eCall component.  Defines all shared types,
// enumerations, structs, constants and small inline utilities used across
// every layer of the stack (Api*, EcallMgr*, EcallEventBus, …).
//
// Previously named EcallCommonTypes.hpp.

#pragma once

#include <cstdint>
#include <ctime>

extern "C"
{
#include "legato.h"
#include "interfaces.h"   // taf_radio_Rat_t, TAF_RADIO_RAT_UNKNOWN
}

// -----------------------------------------------------------------------------
// KEY_LOG - always-on diagnostic macro
//
// Writes timestamp + event message directly to stderr, bypassing the Legato
// log level system. Visible in logread / serial console without any log
// configuration.
// Usage:  KEY_LOG("AudioPromoteToVoiceCall once=%s loop=%s", once, loop);
// Remove or gate with #ifdef ECALL_KEY_LOG before release.
// -----------------------------------------------------------------------------
#define KEY_LOG(fmt, ...)                                              \
    do {                                                               \
        struct timespec _ts;                                           \
        clock_gettime(CLOCK_REALTIME, &_ts);                           \
        struct tm _tm;                                                 \
        gmtime_r(&_ts.tv_sec, &_tm);                                   \
        fprintf(stderr,                                                \
                "[KEY] %02d:%02d:%02d.%03d " fmt "\n",                \
                _tm.tm_hour, _tm.tm_min, _tm.tm_sec,                  \
                (int)(_ts.tv_nsec / 1000000),                         \
                ##__VA_ARGS__);                                        \
    } while (0)

namespace ecall
{

// -----------------------------------------------------------------------------
// GNSS / position related constants
// -----------------------------------------------------------------------------

// Milli-arcseconds per degree. 1 degree = 3,600,000 milli-arcseconds.
constexpr int32_t MAS_PER_DEGREE = 3'600'000;

// Maximum age for a GNSS fix to be considered "fresh", in milliseconds.
constexpr int32_t AGED_THR_MS = 10'000;

// Error horizontal position estimate threshold, in centimeters.
constexpr int32_t EHPE_THR_CM = 3'000;

// Horizontal accuracy threshold, in centimeters.
constexpr int32_t HACC_THR_CM = 30'000;

// Minimum satellite count to consider a fix trustworthy.
constexpr int SAT_MIN_TRUST = 4;

// -----------------------------------------------------------------------------
// DTC / diagnostic related constants
// -----------------------------------------------------------------------------
constexpr std::uint16_t EVENT_Sample_Big       = 1;
constexpr std::uint32_t DTC_CODE_MODEM_FAULT    = 0x00AB'0000u;

constexpr std::uint16_t EVENT_Sample_Small     = 2;
constexpr std::uint32_t DTC_CODE_NETWORK_FAULT  = 0x00AB'0001u;

constexpr std::uint8_t  OPERATION_CYCLE_DC      = 0x01u;

constexpr std::uint8_t CONDITION_TOOBIG = 3;

// -----------------------------------------------------------------------------
// eCall state machine for EcallMgrEcall
// -----------------------------------------------------------------------------

// eCall session state machine.
// Flow: Idle -> CallSetup -> Connected -> SendingMsd -> Voice -> Callback -> Idle
enum class EcSessionState
{
    Idle = 0,       // No active eCall.
    CallSetup,      // Dialing / alerting / incoming.
    Connected,      // PSAP connected.
    SendingMsd,     // MSD sending phase after PSAP MSD request.
    Voice,          // Voice phase after MSD done or if no MSD.
    Callback        // Callback window (T9) after PSAP hangup.
};

enum class EcEvent
{
    OperationError,         // ApiEcall operation failed (non-timer)
    PsapFailed,             // PSAP failure (T5/T7 timeout etc.)

    CallStarted,            // Outgoing call is dialing/alerting
    CallConnected,          // Voice connected and active
    CallEnded,              // PSAP hang up or T2 timeout or AL-ACK cleardown
    CallIncoming,           // Incoming call (callback)

    MsdRequested,           // PSAP requested MSD (pull)
    MsdInbandSuccess,       // In-band MSD delivered (LL+AL ACK)
    MsdOutbandFailed,       // Out-band MSD failed
    MsdOutbandSuccess,      // Out-band MSD delivered
    MsdInbandFailed,        // In-band MSD failed -> go to Voice

    CallbackWindowTimeout   // T9 timeout: end callback window and go Idle
};

// eCall call type
enum class EcCallType : std::int32_t
{
    Automatic,
    Manual,
    Test
};

// -----------------------------------------------------------------------------
// eCall phase / state events
// -----------------------------------------------------------------------------

// eCall call phases as seen by upper layers.
enum class EcPhase : std::int32_t
{
    Idle = 0,
    Dialing,
    Alerting,
    MsdStart,
    MsdSuccess,
    MsdFail,
    OutbandMsdStart,
    OutbandMsdSuccess,
    OutbandMsdFail,
    Active,
    Teardown,
    Incoming
};

// Phase transition event with timestamp.
struct EvPhase
{
    EcPhase       phase{};    // New eCall phase.
    std::uint64_t tsUtcMs{};  // UTC timestamp in milliseconds.
};

// -----------------------------------------------------------------------------
// eCall timer events
// -----------------------------------------------------------------------------

// eCall timers used by the call state machine.
enum class EcTimerId : std::int32_t
{
    T2,
    T5,
    T6,
    T7,
    T9,
    T10
};

// Timer actions (start/stop/expire/resume).
enum class EcTimerAction : std::int32_t
{
    Start,
    Stop,
    Expire,
    Resume
};

// Timer event carrying which timer and which action occurred.
struct EvTimer
{
    EcTimerId     timerId{};
    EcTimerAction action{};
    std::uint64_t tsUtcMs{}; // UTC timestamp in milliseconds.
};

// -----------------------------------------------------------------------------
// PSAP signalling
// -----------------------------------------------------------------------------

// PSAP (Public Safety Answering Point) signalling received by the modem.
enum class PsapSig : std::int32_t
{
    Start,          // SEND MSD
    LlAck,          // Lower layer ACK.
    LlNack,         // Lower layer NACK.
    AlAckPositive,  // Application-layer ACK, continue call.
    AlAckClearDown, // Application-layer ACK, clear down call.
    MsdPullReq      // MSD pull request from PSAP.
};

// PSAP signal event with timestamp.
struct EvPsap
{
    PsapSig       sig{};
    std::uint64_t tsUtcMs{}; // UTC timestamp in milliseconds.
};

// -----------------------------------------------------------------------------
// PSAP connection state (for power manager)
// -----------------------------------------------------------------------------

enum class EcPowerSessionState : std::int32_t
{
    Unknown  = -1,
    Idle     = 0,
    Connected,
    Processing,
    Teardown,
};

// -----------------------------------------------------------------------------
// Location / GNSS fix
// -----------------------------------------------------------------------------

// Raw GNSS fix as used by eCall logic.
struct LocFix
{
    double        latDeg{};        // Latitude in degrees.
    double        lonDeg{};        // Longitude in degrees.
    double        hAccM{};         // Horizontal accuracy in meters.
    float         headingDeg{};    // Heading in degrees.
    float         magDevDeg{};     // Magnetic deviation in degrees.
    std::uint32_t locValidMask{};  // Project-specific validity bitmask.
    std::uint8_t  satUsed{};       // Number of satellites used.
    std::uint64_t epochMs{};       // Epoch timestamp in milliseconds.
};

// Position format used in the eCall MSD (EN 15722).
struct MsdPosition
{
    bool          isTrusted{false};

    // Latitude/longitude in milli-arcseconds; INT32_MAX = unknown (ETSI sentinel).
    std::int32_t  latitudeMas{2'147'483'647};
    std::int32_t  longitudeMas{2'147'483'647};

    // Vehicle direction: 0..179 valid, 255 = invalid (ETSI EN 15722).
    std::int32_t  direction{255};

    // Optional recent-location deltas (100 milli-arcsecond units).
    bool          hasN1{false};
    std::int32_t  latDeltaN1{0};
    std::int32_t  lonDeltaN1{0};

    bool          hasN2{false};
    std::int32_t  latDeltaN2{0};
    std::int32_t  lonDeltaN2{0};
};

// Post-processed GNSS fix ready for MSD consumption.
// Derived from LocFix by EcallMgrLocation; stored in EcallMgrEcall's ring buffer.
struct LocConverted
{
    bool     posTrusted{false};  // true if position meets ETSI trust criteria
    int32_t  latMas{INT32_MAX};  // latitude  in MAS; INT32_MAX = unknown
    int32_t  lonMas{INT32_MAX};  // longitude in MAS; INT32_MAX = unknown
    int32_t  direction{255};     // 0..179 (2-deg steps); 255 = unknown
    uint64_t tsUtcMs{0};         // UTC epoch timestamp in milliseconds
};

// -----------------------------------------------------------------------------
// Network / radio types used by eCall logging
// -----------------------------------------------------------------------------

// Simplified network status snapshot used by EcallMgrEcall / EcallMgrDataLogger.
struct EcNetworkStatus
{
    taf_radio_Rat_t rat{TAF_RADIO_RAT_UNKNOWN};
    char            mcc[8]{};   // null-terminated MCC string (e.g. "460")
    char            mnc[8]{};   // null-terminated MNC string (e.g. "00")
};

// Normalised signal strength for the current RAT.
// Fields not applicable to the current RAT are left at INT32_MIN.
struct EcSignalStrength
{
    taf_radio_Rat_t rat{TAF_RADIO_RAT_UNKNOWN};

    int32_t rawRssi{INT32_MIN};   // GSM RSSI or UMTS SS (dBm)
    int32_t rsrp{INT32_MIN};      // LTE / NR5G RSRP (dBm)
    int32_t rsrq{INT32_MIN};      // LTE / NR5G RSRQ (dB)
    int32_t sinr{INT32_MIN};      // LTE / NR5G SNR  (dB)
    int32_t umtsRscp{INT32_MIN};  // UMTS RSCP (dBm)
};

// -----------------------------------------------------------------------------
// eCall configuration data
// -----------------------------------------------------------------------------

// Static eCall configuration parameters loaded from config/JSON.
struct EcConfigData
{
    int msdVersion{};    // MSD protocol version.
    int dialAttempts{};  // Maximum dialing attempts.
    int dialDuration{};  // Each dialing attempt duration, seconds.
    int t2{};            // T2 timer, seconds.
    int t9{};            // T9 timer, seconds.
    int t10{};           // T10 timer, seconds.
    int vehicleType{};   // Vehicle type, e.g., passenger car.

    // Fuel presence flags.
    int fuelElectric{};
    int fuelDiesel{};
    int fuelGasoline{};
    int fuelCompressed{};
    int fuelLiquid{};
    int fuelHydrogen{};
    int fuelOther{};

    // Test/emergency numbers as configured (null-terminated).
    char testNumber[64]{};
    char emergencyNumber[64]{};
};

// -----------------------------------------------------------------------------
// Modem HMS events used by eCall
// -----------------------------------------------------------------------------

// Modem event type (subset relevant to eCall).
enum class ModemEvtType : std::int32_t
{
    ContinueReboot   = 0,
    ConnectionLost   = 1,
    ConnectionAvail  = 2
};

// Event severity from HMS / modem health monitor.
enum class ModemEvtSeverity : std::int32_t
{
    Low    = 0,
    Medium = 1,
    High   = 2
};

// Modem HMS event as consumed by eCall logic.
struct ModemHmsEvt
{
    ModemEvtType     type{};
    ModemEvtSeverity severity{};
};

// -----------------------------------------------------------------------------
// Time helpers
// -----------------------------------------------------------------------------

// Get current UTC time in milliseconds since the Unix epoch.
inline std::uint64_t NowUtcMs()
{
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1000ULL +
           static_cast<std::uint64_t>(ts.tv_nsec) / 1'000'000ULL;
}

// Safe strnlen: returns 0 for nullptr, does not scan beyond maxn bytes.
inline size_t sstrnlen(const char* s, size_t maxn)
{
    if (!s) return 0;
    size_t i = 0;
    for (; i < maxn; ++i)
        if (s[i] == '\0') break;
    return i;
}

} // namespace ecall
