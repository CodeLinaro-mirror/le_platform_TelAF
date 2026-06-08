/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EcallMgrEcall.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <inttypes.h>

extern "C"
{
#include "legato.h"
}
#include "interfaces.h"

#include "EcallEventBus.hpp"
#include "EcallMgrConfig.hpp"
#include "EcallMgrDataLogger.hpp"
#include "EcallMgrNetwork.hpp"
#include "EcallMgrPower.hpp"
#include "ApiAudio.hpp"
#include "ApiLoc.hpp"

namespace ecall
{

// -----------------------------------------------------------------------------
// Local constants and helpers
// -----------------------------------------------------------------------------

// Time window for "recent vehicle locations" (N1/N2) in milliseconds.
static constexpr uint64_t kRecentWindowMs = 15000;

static constexpr uint32_t kLocationWaitTimeoutMs = 3000;

// Unit for MSD deltas: 100 milli-arcseconds.
static constexpr int      kUnitMas        = 100;

// Delta range in units of 100 mas (ETSI constraint).
static constexpr int16_t  kDeltaMin       = -512;
static constexpr int16_t  kDeltaMax       =  511;

static const char* ToStr(EcSessionState s)
{
    switch (s)
    {
        case EcSessionState::Idle:       return "IDLE";
        case EcSessionState::CallSetup:  return "CALL_SETUP";
        case EcSessionState::Connected:  return "CONNECTED";
        case EcSessionState::SendingMsd: return "SENDING_MSD";
        case EcSessionState::Voice:      return "VOICE";
        case EcSessionState::Callback:   return "CALLBACK";
    }
    return "UNKNOWN";
}

static const char* ToStr(EcEvent e)
{
    switch (e)
    {
        case EcEvent::OperationError:       return "OPERATION_ERROR";
        case EcEvent::PsapFailed:           return "PSAP_FAILED";

        case EcEvent::CallStarted:          return "CALL_STARTED";
        case EcEvent::CallConnected:        return "CALL_CONNECTED";
        case EcEvent::CallEnded:            return "CALL_ENDED";
        case EcEvent::CallIncoming:         return "CALL_INCOMING"; 

        case EcEvent::MsdRequested:         return "MSD_REQUESTED";
        case EcEvent::MsdInbandSuccess:     return "MSD_INBAND_SUCCESS";
        case EcEvent::MsdOutbandSuccess:    return "MSD_OUTBAND_SUCCESS";
        case EcEvent::MsdOutbandFailed:     return "MSD_OUTBAND_FAILED";
        case EcEvent::MsdInbandFailed:      return "MSD_INBAND_FAILED";

        case EcEvent::CallbackWindowTimeout:return "CALLBACK_WINDOW_TIMEOUT";
    }
    return "UNKNOWN";
}

static const char* ToStr(EcPhase p)
{
    switch (p)
    {
        case EcPhase::Idle:              return "Idle";
        case EcPhase::Dialing:           return "Dialing";
        case EcPhase::Alerting:          return "Alerting";
        case EcPhase::MsdStart:          return "MsdStart";
        case EcPhase::MsdSuccess:        return "MsdSuccess";
        case EcPhase::MsdFail:           return "MsdFail";
        case EcPhase::OutbandMsdStart:   return "OutbandMsdStart";
        case EcPhase::OutbandMsdSuccess: return "OutbandMsdSuccess";
        case EcPhase::OutbandMsdFail:    return "OutbandMsdFail";
        case EcPhase::Active:            return "Active";
        case EcPhase::Teardown:          return "Teardown";
        case EcPhase::Incoming:          return "Incoming";
    }
    return "Unknown";
}

static const char* ToStr(EcTimerId id)
{
    switch (id)
    {
        case EcTimerId::T2:  return "T2";
        case EcTimerId::T5:  return "T5";
        case EcTimerId::T6:  return "T6";
        case EcTimerId::T7:  return "T7";
        case EcTimerId::T9:  return "T9";
        case EcTimerId::T10: return "T10";
    }
    return "T?";
}

static const char* ToStr(EcTimerAction a)
{
    switch (a)
    {
        case EcTimerAction::Start:  return "STARTED";
        case EcTimerAction::Stop:   return "STOPPED";
        case EcTimerAction::Expire: return "EXPIRED";
        case EcTimerAction::Resume: return "RESUMED";
    }
    return "UNKNOWN";
}

static const char* ToStr(PsapSig s)
{
    switch (s)
    {
        case PsapSig::Start:          return "Start";
        case PsapSig::LlAck:          return "LL-ACK";
        case PsapSig::LlNack:         return "LL-NACK";
        case PsapSig::AlAckPositive:  return "AL-ACK-POSITIVE";
        case PsapSig::AlAckClearDown: return "AL-ACK-CLEARDOWN";
        case PsapSig::MsdPullReq:     return "MSD-PULL-REQ";
    }
    return "PSAP-?";
}

// -----------------------------------------------------------------------------
// Singleton
// -----------------------------------------------------------------------------

EcallMgrEcall& EcallMgrEcall::GetInstance()
{
    static EcallMgrEcall inst;
    return inst;
}

EcallMgrEcall::EcallMgrEcall()
{
    RegisterBusHandlers();
}

// -----------------------------------------------------------------------------
// Bus registration
// -----------------------------------------------------------------------------

void EcallMgrEcall::RegisterBusHandlers()
{
    auto& bus = EcallEventBus::GetInstance();

        le_event_AddHandler("ec.phase", bus.PhaseChannel(),  EcallMgrEcall::OnPhase);
    le_event_AddHandler("ec.psap",  bus.PsapChannel(),   EcallMgrEcall::OnPsapSig);
    le_event_AddHandler("ec.timer", bus.TimerChannel(),  EcallMgrEcall::OnTimer);

    LE_INFO("EcallMgrEcall: bus handlers registered (phase/psap/timer/hms)");
}

// -----------------------------------------------------------------------------
// VIN handling
// -----------------------------------------------------------------------------

void EcallMgrEcall::SetVin(const char* vin)
{
    if (!vin)
    {
        LE_ERROR("EcallMgrEcall: SetVin got null");
        return;
    }

    std::memset(vinCached_, 0, sizeof(vinCached_));
    snprintf(vinCached_, sizeof(vinCached_), "%s", vin);

    LE_DEBUG("EcallMgrEcall: CONFIGURATION VIN: %s", vinCached_);
}

le_result_t EcallMgrEcall::GetVin(char* outBuf, size_t outLen) const
{
    if (!outBuf || outLen == 0)
    {
        return LE_BAD_PARAMETER;
    }

    size_t n = sstrnlen(vinCached_, sizeof(vinCached_));
    if (n == 0)
    {
        return LE_NOT_FOUND;
    }

    size_t toCopy = std::min(n, outLen - 1);
    std::memcpy(outBuf, vinCached_, toCopy);
    outBuf[toCopy] = '\0';

    return LE_OK;
}

const char* EcallMgrEcall::VehicleTypeToName(int v)
{
    switch (v)
    {
        case TAF_ECALL_PASSENGER_VEHICLE_CLASS_M1:               return "PASSENGER_VEHICLE_CLASS_M1";
        case TAF_ECALL_BUSES_AND_COACHES_CLASS_M2:               return "BUSES_AND_COACHES_CLASS_M2";
        case TAF_ECALL_BUSES_AND_COACHES_CLASS_M3:               return "BUSES_AND_COACHES_CLASS_M3";
        case TAF_ECALL_LIGHT_COMMERCIAL_VEHICLES_CLASS_N1:       return "LIGHT_COMMERCIAL_VEHICLES_CLASS_N1";
        case TAF_ECALL_HEAVY_DUTY_VEHICLES_CLASS_N2:             return "HEAVY_DUTY_VEHICLES_CLASS_N2";
        case TAF_ECALL_HEAVY_DUTY_VEHICLES_CLASS_N3:             return "HEAVY_DUTY_VEHICLES_CLASS_N3";
        case TAF_ECALL_MOTOR_CYCLES_CLASS_L1E:                   return "MOTOR_CYCLES_CLASS_L1E";
        case TAF_ECALL_MOTOR_CYCLES_CLASS_L2E:                   return "MOTOR_CYCLES_CLASS_L2E";
        case TAF_ECALL_MOTOR_CYCLES_CLASS_L3E:                   return "MOTOR_CYCLES_CLASS_L3E";
        case TAF_ECALL_MOTOR_CYCLES_CLASS_L4E:                   return "MOTOR_CYCLES_CLASS_L4E";
        case TAF_ECALL_MOTOR_CYCLES_CLASS_L5E:                   return "MOTOR_CYCLES_CLASS_L5E";
        case TAF_ECALL_MOTOR_CYCLES_CLASS_L6E:                   return "MOTOR_CYCLES_CLASS_L6E";
        case TAF_ECALL_MOTOR_CYCLES_CLASS_L7E:                   return "MOTOR_CYCLES_CLASS_L7E";
        case TAF_ECALL_TRAILERS_CLASS_O:                         return "TRAILERS_CLASS_O";
        case TAF_ECALL_AGRI_VEHICLES_CLASS_R:                    return "AGRI_VEHICLES_CLASS_R";
        case TAF_ECALL_AGRI_VEHICLES_CLASS_S:                    return "AGRI_VEHICLES_CLASS_S";
        case TAF_ECALL_AGRI_VEHICLES_CLASS_T:                    return "AGRI_VEHICLES_CLASS_T";
        case TAF_ECALL_OFF_ROAD_VEHICLES_G:                      return "OFF_ROAD_VEHICLES_G";
        case TAF_ECALL_SPECIAL_PURPOSE_MOTOR_CARAVAN_CLASS_SA:   return "SPECIAL_PURPOSE_MOTOR_CARAVAN_CLASS_SA";
        case TAF_ECALL_SPECIAL_PURPOSE_ARMOURED_VEHICLE_CLASS_SB:return "SPECIAL_PURPOSE_ARMOURED_VEHICLE_CLASS_SB";
        case TAF_ECALL_SPECIAL_PURPOSE_AMBULANCE_CLASS_SC:       return "SPECIAL_PURPOSE_AMBULANCE_CLASS_SC";
        case TAF_ECALL_SPECIAL_PURPOSE_HEARSE_CLASS_SD:          return "SPECIAL_PURPOSE_HEARSE_CLASS_SD";
        case TAF_ECALL_OTHER_VEHICLE_CLASS:                      return "OTHER_VEHICLE_CLASS";
        default:                                                 return "UNKNOWN";
    }
}

taf_ecall_MsdVehicleType_t EcallMgrEcall::ConvertIntToVehicleType(int v)
{
    if (v >= TAF_ECALL_PASSENGER_VEHICLE_CLASS_M1 &&
        v <= TAF_ECALL_OTHER_VEHICLE_CLASS)
    {
        return static_cast<taf_ecall_MsdVehicleType_t>(v);
    }
    return TAF_ECALL_OTHER_VEHICLE_CLASS;
}

std::string EcallMgrEcall::FuelToName(const EcConfigData& c)
{
    std::vector<std::string> names;

    if (c.fuelGasoline)   names.emplace_back("PROP_TYPE_GASOLINE_TANK");
    if (c.fuelDiesel)     names.emplace_back("PROP_TYPE_DIESEL_TANK");
    if (c.fuelCompressed) names.emplace_back("PROP_TYPE_COMPRESSED_NATURALGAS");
    if (c.fuelLiquid)     names.emplace_back("PROP_TYPE_PROPANE_GAS");
    if (c.fuelElectric)   names.emplace_back("PROP_TYPE_ELECTRIC");
    if (c.fuelHydrogen)   names.emplace_back("PROP_TYPE_HYDROGEN");
    if (c.fuelOther)      names.emplace_back("PROP_TYPE_OTHER");

    if (names.empty())
    {
        names.emplace_back("PROP_TYPE_NONE");
    }

    std::string result;
    for (size_t i = 0; i < names.size(); ++i)
    {
        if (i > 0)
        {
            result += '|';
        }
        result += names[i];
    }
    return result;
}

taf_ecall_PropulsionStorageType_t
EcallMgrEcall::ConvertFuelToPropulsionMask(const EcConfigData& c)
{
    taf_ecall_PropulsionStorageType_t mask = 0;

    auto setFlag = [&mask](taf_ecall_PropulsionStorageType_t flag, const char* name)
    {
        mask |= flag;
        LE_INFO("Propulsion flag set: %s (flag=0x%02X)", name, static_cast<unsigned>(flag));
    };

    if (c.fuelGasoline)   setFlag(TAF_ECALL_PROP_TYPE_GASOLINE_TANK,         "GASOLINE");
    if (c.fuelDiesel)     setFlag(TAF_ECALL_PROP_TYPE_DIESEL_TANK,           "DIESEL");
    if (c.fuelCompressed) setFlag(TAF_ECALL_PROP_TYPE_COMPRESSED_NATURALGAS, "CNG");
    if (c.fuelLiquid)     setFlag(TAF_ECALL_PROP_TYPE_PROPANE_GAS,           "PROPANE");
    if (c.fuelElectric)   setFlag(TAF_ECALL_PROP_TYPE_ELECTRIC,              "ELECTRIC");
    if (c.fuelHydrogen)   setFlag(TAF_ECALL_PROP_TYPE_HYDROGEN,              "HYDROGEN");
    if (c.fuelOther)      setFlag(TAF_ECALL_PROP_TYPE_OTHER,                 "OTHER");

    LE_INFO("Final propulsion mask: 0x%08X", static_cast<unsigned>(mask));
    return mask;
}

void EcallMgrEcall::SetConfig(const EcConfigData& cfg)
{
    lastCfg_ = cfg;

    LE_INFO(
        "EcallMgrEcall: SET_ECALL_CONFIG_REQ [t2=%d t9=%d t10=%d "
        "dialAttempts=%d dialDuration=%d testNumber=%s emergencyNumber=%s "
        "vehicleType=%s fuelType=%s msdVersion=%d]",
        cfg.t2,
        cfg.t9,
        cfg.t10,
        cfg.dialAttempts,
        cfg.dialDuration,
        cfg.testNumber,
        cfg.emergencyNumber,
        VehicleTypeToName(cfg.vehicleType),
        FuelToName(cfg).c_str(),
        cfg.msdVersion);
}

// -----------------------------------------------------------------------------
// Location ring buffer
// -----------------------------------------------------------------------------

void EcallMgrEcall::LocRingBufferHandler(const LocConverted& d)
{
    LE_DEBUG("EcallMgrEcall: LocRingBufferHandler latMas=%d lonMas=%d "
             "trusted=%d direction=%d tsUtcMs=%" PRIu64,
             d.latMas,
             d.lonMas,
             d.posTrusted ? 1 : 0,
             d.direction,
             d.tsUtcMs);

    LocRingBufferWrite(d, "ok");

    // If a dial was deferred waiting for the first valid GNSS fix,
    // fire it now that we have location data in the ring buffer.
    if (locationReadyPending_)
    {
        const bool hasValidPos = IsLatMasValid(d.latMas) && IsLonMasValid(d.lonMas);
        if (hasValidPos)
        {
            locationReadyPending_ = false;
            StopLocationWaitTimer();
            LE_INFO("EcallMgrEcall: valid GNSS position — updating MSD location and dialling");
            DoSetLocationData();
            DoDialCall(pendingCallType_);
        }
        else
        {
            LE_INFO("EcallMgrEcall: GNSS sample received but position invalid; "
                    "continue waiting up to %u ms (latMas=%d lonMas=%d)",
                    static_cast<unsigned>(kLocationWaitTimeoutMs),
                    d.latMas,
                    d.lonMas);
        }
    }
}

void EcallMgrEcall::LocRingBufferWrite(const LocConverted& d, const char* /*statusStr*/)
{
    ring_[head_] = d;
    head_        = (head_ + 1) % kRingSize;

    if (count_ < kRingSize)
    {
        ++count_;
    }
}

void EcallMgrEcall::StartLocationWaitTimer()
{
    StopLocationWaitTimer();

    locationWaitTimer_ = le_timer_Create("ec.locWait");
    if (!locationWaitTimer_)
    {
        LE_ERROR("EcallMgrEcall: failed to create location wait timer");
        return;
    }

    le_clk_Time_t interval{};
    interval.sec  = kLocationWaitTimeoutMs / 1000U;
    interval.usec = static_cast<long>((kLocationWaitTimeoutMs % 1000U) * 1000U);

    le_timer_SetInterval(locationWaitTimer_, interval);
    le_timer_SetRepeat(locationWaitTimer_, 1);
    le_timer_SetHandler(locationWaitTimer_, EcallMgrEcall::OnLocationWaitTimeout);
    le_timer_SetContextPtr(locationWaitTimer_, this);

    le_result_t rc = le_timer_Start(locationWaitTimer_);
    if (rc != LE_OK)
    {
        LE_ERROR("EcallMgrEcall: failed to start location wait timer rc=%d", rc);
        le_timer_Delete(locationWaitTimer_);
        locationWaitTimer_ = nullptr;
    }
    else
    {
        LE_INFO("EcallMgrEcall: location wait timer started (%u ms)",
                static_cast<unsigned>(kLocationWaitTimeoutMs));
    }
}

void EcallMgrEcall::StopLocationWaitTimer()
{
    if (!locationWaitTimer_)
    {
        return;
    }

    (void)le_timer_Stop(locationWaitTimer_);
    le_timer_Delete(locationWaitTimer_);
    locationWaitTimer_ = nullptr;
    LE_INFO("EcallMgrEcall: location wait timer stopped");
}

// static
void EcallMgrEcall::OnLocationWaitTimeout(le_timer_Ref_t timerRef)
{
    auto* self = static_cast<EcallMgrEcall*>(le_timer_GetContextPtr(timerRef));
    if (!self)
    {
        return;
    }

    if (self->locationWaitTimer_ == timerRef)
    {
        self->locationWaitTimer_ = nullptr;
    }

    le_timer_Delete(timerRef);

    if (!self->locationReadyPending_)
    {
        LE_INFO("EcallMgrEcall: location wait timeout ignored (no pending dial)");
        return;
    }

    self->locationReadyPending_ = false;
    LE_WARN("EcallMgrEcall: no GNSS sample within %u ms; continuing with unknown position",
            static_cast<unsigned>(kLocationWaitTimeoutMs));

        // If invalid samples arrived before timeout, the ring buffer contains the
    // latest invalid entry. If no samples arrived, the ring buffer is empty.
    // In both cases DoSetLocationData() encodes ETSI unknown-position sentinel
    // values, then eCall continues.
    self->DoSetLocationData();
    self->DoDialCall(self->pendingCallType_);
}

std::optional<LocConverted> EcallMgrEcall::RingReadAt(size_t relIndex) const
{
    if (count_ == 0 || relIndex >= count_)
    {
        return std::nullopt;
    }

    size_t latestIdx = (head_ + kRingSize - 1) % kRingSize;
    size_t idx       = (latestIdx + kRingSize - relIndex) % kRingSize;
    return ring_[idx];
}

// -----------------------------------------------------------------------------
// MAS range checks and delta computation
// -----------------------------------------------------------------------------

bool EcallMgrEcall::IsLatMasValid(int32_t latMas)
{
    return (latMas >= -324000000) && (latMas <= 324000000);
}

bool EcallMgrEcall::IsLonMasValid(int32_t lonMas)
{
    return (lonMas >= -648000000) && (lonMas <= 648000000);
}

bool EcallMgrEcall::IsDirectionValid(int32_t direction)
{
    return (direction >= 0) && (direction <= 179);
}

int16_t EcallMgrEcall::ToDelta100Mas(int32_t refMas, int32_t sampleMas)
{
    long long diffMas = static_cast<long long>(sampleMas) -
                        static_cast<long long>(refMas);
    long long units   = std::llround(static_cast<double>(diffMas) /
                                     static_cast<double>(kUnitMas));

    if (units < kDeltaMin) units = kDeltaMin;
    if (units > kDeltaMax) units = kDeltaMax;

    return static_cast<int16_t>(units);
}

void EcallMgrEcall::SetRecentVehicleLocationN1(int16_t latDelta100Mas,
                                               int16_t lonDelta100Mas)
{
    msdRecentN1_.latDelta = latDelta100Mas;
    msdRecentN1_.lonDelta = lonDelta100Mas;
    msdRecentN1_.valid    = true;

    LE_DEBUG("EcallMgrEcall: MSD N1 set latDelta(100mas)=%d lonDelta(100mas)=%d",
             static_cast<int>(latDelta100Mas),
             static_cast<int>(lonDelta100Mas));
}

void EcallMgrEcall::SetRecentVehicleLocationN2(int16_t latDelta100Mas,
                                               int16_t lonDelta100Mas)
{
    msdRecentN2_.latDelta = latDelta100Mas;
    msdRecentN2_.lonDelta = lonDelta100Mas;
    msdRecentN2_.valid    = true;

    LE_DEBUG("EcallMgrEcall: MSD N2 set latDelta(100mas)=%d lonDelta(100mas)=%d",
             static_cast<int>(latDelta100Mas),
             static_cast<int>(lonDelta100Mas));
}

bool EcallMgrEcall::ComputeAndSetRecentVehicleLocationsFromRing()
{
    auto baseOpt = RingReadAt(0);
    if (!baseOpt.has_value())
    {
        LE_WARN("EcallMgrEcall: no base location in ring");
        return false;
    }

    const LocConverted base   = baseOpt.value();
    const uint64_t     baseTs = base.tsUtcMs;

    if (!IsLatMasValid(base.latMas) || !IsLonMasValid(base.lonMas))
    {
        LE_WARN("EcallMgrEcall: base lat/lon invalid");
        return false;
    }

    std::optional<LocConverted> n1Opt;
    int                         n1RelIdx = -1;

    // Find the first older fix within window to form N1.
    for (size_t i = 1; i < count_; ++i)
    {
        auto eOpt = RingReadAt(i);
        if (!eOpt.has_value())
            break;

        const LocConverted e = eOpt.value();
        if (e.tsUtcMs == 0 || e.tsUtcMs >= baseTs)
            continue;

        uint64_t age = baseTs - e.tsUtcMs;
        if (age <= kRecentWindowMs &&
            IsLatMasValid(e.latMas) && IsLonMasValid(e.lonMas))
        {
            n1Opt    = e;
            n1RelIdx = static_cast<int>(i);
            break;
        }
    }

    if (!n1Opt.has_value())
    {
        LE_DEBUG("EcallMgrEcall: no N1 within %u ms",
                 static_cast<unsigned>(kRecentWindowMs));
        return false;
    }

    const LocConverted n1 = n1Opt.value();

    int16_t n1LatDelta = ToDelta100Mas(base.latMas, n1.latMas);
    int16_t n1LonDelta = ToDelta100Mas(base.lonMas, n1.lonMas);
    SetRecentVehicleLocationN1(n1LatDelta, n1LonDelta);

    std::optional<LocConverted> n2Opt;
    for (size_t i = static_cast<size_t>(n1RelIdx + 1); i < count_; ++i)
    {
        auto eOpt = RingReadAt(i);
        if (!eOpt.has_value())
            break;

        const LocConverted e = eOpt.value();
        if (e.tsUtcMs == 0 || e.tsUtcMs >= n1.tsUtcMs)
            continue;

        uint64_t age = baseTs - e.tsUtcMs;
        if (age <= kRecentWindowMs &&
            IsLatMasValid(e.latMas) && IsLonMasValid(e.lonMas))
        {
            n2Opt = e;
            break;
        }
    }

    if (n2Opt.has_value())
    {
        const LocConverted n2 = n2Opt.value();
        int16_t n2LatDelta = ToDelta100Mas(n1.latMas, n2.latMas);
        int16_t n2LonDelta = ToDelta100Mas(n1.lonMas, n2.lonMas);
        SetRecentVehicleLocationN2(n2LatDelta, n2LonDelta);
    }
    else
    {
        LE_DEBUG("EcallMgrEcall: no N2 within %u ms (N1 only)",
                 static_cast<unsigned>(kRecentWindowMs));
    }

    LE_DEBUG("EcallMgrEcall: MSD recent locs computed: "
             "N1(lat=%d lon=%d valid=%d) N2(lat=%d lon=%d valid=%d)",
             static_cast<int>(msdRecentN1_.latDelta),
             static_cast<int>(msdRecentN1_.lonDelta),
             msdRecentN1_.valid ? 1 : 0,
             static_cast<int>(msdRecentN2_.latDelta),
             static_cast<int>(msdRecentN2_.lonDelta),
             msdRecentN2_.valid ? 1 : 0);

    return true;
}

// -----------------------------------------------------------------------------
// MSD position push
// -----------------------------------------------------------------------------

bool EcallMgrEcall::PushZeroBaseMsdWithRecent()
{
    // Compute N1/N2; ignore result but keep any deltas found.
    (void)ComputeAndSetRecentVehicleLocationsFromRing();

        MsdPosition pos{};
    // ETSI EN 15722: INT32_MAX (0x7FFFFFFF) is the "unknown" sentinel for
    // lat/lon when no valid fix is available; 255 = unknown direction.
    pos.latitudeMas  = INT32_MAX;
    pos.longitudeMas = INT32_MAX;
    pos.isTrusted    = false;
    pos.direction    = 255;

    if (auto baseOpt = RingReadAt(0); baseOpt.has_value())
    {
        const LocConverted& base = baseOpt.value();
        if (IsLatMasValid(base.latMas) && IsLonMasValid(base.lonMas))
        {
            pos.latitudeMas  = base.latMas;
            pos.longitudeMas = base.lonMas;
            pos.isTrusted    = base.posTrusted;
        }
        else
        {
            LE_WARN("EcallMgrEcall: PushZeroBaseMsdWithRecent - base lat/lon invalid, "
                    "sending MSD with unknown position (ETSI sentinel)");
        }
        if (IsDirectionValid(base.direction))
        {
            pos.direction = base.direction;
        }
    }
    else
    {
        LE_WARN("EcallMgrEcall: PushZeroBaseMsdWithRecent - ring buffer empty, "
                "sending MSD with unknown position (ETSI sentinel)");
    }

    if (msdRecentN1_.valid)
    {
        pos.hasN1      = true;
        pos.latDeltaN1 = static_cast<int32_t>(msdRecentN1_.latDelta);
        pos.lonDeltaN1 = static_cast<int32_t>(msdRecentN1_.lonDelta);
    }
    else
    {
        pos.hasN1 = false;
        LE_DEBUG("EcallMgrEcall: MSD N1 skipped (invalid)");
    }

    if (msdRecentN2_.valid)
    {
        pos.hasN2      = true;
        pos.latDeltaN2 = static_cast<int32_t>(msdRecentN2_.latDelta);
        pos.lonDeltaN2 = static_cast<int32_t>(msdRecentN2_.lonDelta);
    }
    else
    {
        pos.hasN2 = false;
        LE_DEBUG("EcallMgrEcall: MSD N2 skipped (invalid)");
    }

    LE_DEBUG("EcallMgrEcall: MSD Position payload -> trusted=%d latMas=%d lonMas=%d dir=%d | "
             "N1(has=%d) latΔ=%d lonΔ=%d | "
             "N2(has=%d) latΔ=%d lonΔ=%d | Δ unit=100 mas, clamp=[-512..511]",
             pos.isTrusted ? 1 : 0,
             pos.latitudeMas,
             pos.longitudeMas,
             pos.direction,
             pos.hasN1 ? 1 : 0,
             pos.latDeltaN1,
             pos.lonDeltaN1,
             pos.hasN2 ? 1 : 0,
             pos.latDeltaN2,
             pos.lonDeltaN2);

    lastMsdPos_ = pos;

    return true;
}

// -----------------------------------------------------------------------------
// Phase / PSAP / Timer / HMS handling
// -----------------------------------------------------------------------------

void EcallMgrEcall::OnPhase(void* payloadPtr)
{
    if (!payloadPtr)
    {
        LE_WARN("EcallMgrEcall: OnPhase got null payload");
        return;
    }
    const auto* e = static_cast<const EvPhase*>(payloadPtr);
    GetInstance().HandlePhase(e);
}

void EcallMgrEcall::OnPsapSig(void* payloadPtr)
{
    if (!payloadPtr)
    {
        LE_WARN("EcallMgrEcall: OnPsapSig got null payload");
        return;
    }
    const auto* e = static_cast<const EvPsap*>(payloadPtr);
    GetInstance().HandlePsapSig(e);
}

void EcallMgrEcall::OnTimer(void* payloadPtr)
{
    if (!payloadPtr)
    {
        LE_WARN("EcallMgrEcall: OnTimer got null payload");
        return;
    }
    const auto* e = static_cast<const EvTimer*>(payloadPtr);
    GetInstance().HandleTimer(e);
}

void EcallMgrEcall::HandlePhase(const EvPhase* e)
{
    if (!e) { LE_WARN("EcallMgrEcall: HandlePhase null"); return; }

    lastPhase_ = e->phase;
    LE_INFO("EcallMgrEcall: Phase(%s ts=%llu)",
            ToStr(e->phase), static_cast<unsigned long long>(e->tsUtcMs));

    switch (e->phase)
    {
        case EcPhase::Dialing:
            KEY_LOG("  eCall call state   : Dialing");
            break;
        case EcPhase::Incoming:
            KEY_LOG("  eCall call state   : Incoming");
            break;
        case EcPhase::Alerting:
            KEY_LOG("  eCall call state   : Alerting");
            break;
        case EcPhase::Active:
            KEY_LOG("  eCall call state   : Active");
            break;
        case EcPhase::Teardown:
            KEY_LOG("  eCall call state   : Ended");
            break;
        case EcPhase::MsdStart:
            KEY_LOG("  eCall MSD state   :  MSD transmission start");
            break;
        case EcPhase::MsdSuccess:
            KEY_LOG("  eCall MSD state   :  MSD transmission success");
            break;
        case EcPhase::MsdFail:
            KEY_LOG("  Ecall MSD state   :  MSD transmission fail");
            break;
        case EcPhase::OutbandMsdStart:
            KEY_LOG("  eCall MSD state   :  Outband MSD transmission start");
            break;
        case EcPhase::OutbandMsdSuccess:
            KEY_LOG("  eCall MSD state   :  Outband MSD transmission success");
            break;
        case EcPhase::OutbandMsdFail:
            KEY_LOG("  eCall MSD state   :  Outband MSD transmission fail");
            break;
        default:
            break;
    }

    const InterpretResult ir = interp_.OnPhase(*e);

    if (ir.psapState.has_value())
        EcallMgrPower::GetInstance().OnPsapStateChange(ir.psapState.value());

    DispatchActions(ir.actions);

    if (ir.ev.has_value())
        HandleEcEvent(ir.ev.value());
}

void EcallMgrEcall::HandlePsapSig(const EvPsap* e)
{
    if (!e) { LE_WARN("EcallMgrEcall: HandlePsapSig null"); return; }

    lastPsap_ = e->sig;
    LE_INFO("EcallMgrEcall: PsapSig(%s ts=%llu)",
            ToStr(e->sig), static_cast<unsigned long long>(e->tsUtcMs));

    switch (e->sig)
    {
        case PsapSig::Start:
            KEY_LOG("  eCall Msd state   : SEND-MSD(START) received");
            break;
        case PsapSig::LlAck:
            KEY_LOG("  eCall Msd state   : LL-ACK received");
            break;
        case PsapSig::LlNack:
            KEY_LOG("  eCall MSD state   : LL-NACK received");
            break;
        case PsapSig::AlAckPositive:
            KEY_LOG("  eCall MSD state   : AL-ACK positive received");
            break;
        case PsapSig::AlAckClearDown:
            KEY_LOG("  eCall MSD state   : AL-ACK cleardown received");
            break;
        case PsapSig::MsdPullReq:
            KEY_LOG("  eCall MSD state   : MSD Pull request received");
            break;
        default:
            break;
    }

    const InterpretResult ir = interp_.OnPsapSig(*e);

    DispatchActions(ir.actions);


    if (ir.ev.has_value())
        HandleEcEvent(ir.ev.value());
}

void EcallMgrEcall::HandleTimer(const EvTimer* e)
{
    if (!e) { LE_WARN("EcallMgrEcall: HandleTimer null"); return; }

    LE_INFO("EcallMgrEcall: Timer(%s %s ts=%llu)",
            ToStr(e->timerId), ToStr(e->action),
            static_cast<unsigned long long>(e->tsUtcMs));

    KEY_LOG("  eCall timer state : %s %s",  ToStr(e->timerId), ToStr(e->action));


    const InterpretResult ir = interp_.OnTimer(*e);

    DispatchActions(ir.actions);

    if (ir.ev.has_value())
        HandleEcEvent(ir.ev.value());
}

void EcallMgrEcall::DispatchActions(const std::vector<EcAction>& actions)
{
    auto& audio = ApiAudio::GetInstance();
    auto& pwr   = EcallMgrPower::GetInstance();

        // Prompt file paths used for eCall IVS audio playback.
    // oncePromptFile: played once on call setup (e.g. announcement).
    // loopPromptFile: played endlessly as IVS alert tone.
    static constexpr const char* kOncePromptFile = "/data/ecall_ivs_tone1.wav";
    static constexpr const char* kLoopPromptFile = "/data/ecall_ivs_tone2.wav";

    for (const auto& a : actions)
    {
        switch (a.kind)
        {
            // -----------------------------------------------------------------
            // Audio path control — direct ApiAudio calls.
            // -----------------------------------------------------------------

            case EcActionKind::AudioPromoteToVoiceCall:
            {
                // Open VOICE_CALL route, connect modem RX/TX, mute voice,
                // and start prompt playback:
                //   kOncePromptFile: played once (announcement)
                //   kLoopPromptFile: played endlessly (IVS alert tone)
                le_result_t r = audio.PromoteToVoiceCall(kOncePromptFile, kLoopPromptFile);
                if (r != LE_OK)
                {
                    LE_ERROR("EcallMgrEcall: AudioPromoteToVoiceCall failed rc=%d", r);
                    KEY_LOG("  Audio state : Voice call route opened or audio file play failed");
                }
                else
                {
                    KEY_LOG("  Audio state : Playing audio file started");
                    KEY_LOG("  Audio state : Voice call route opened, voice muted");
                }
                break;
            }

            case EcActionKind::AudioBeginMsd:
            {
                // Reserved/no-op in the simplified audio flow.
                le_result_t r = audio.BeginMsdTransmission();
                if (r != LE_OK)
                {
                    LE_ERROR("EcallMgrEcall: AudioBeginMsd failed rc=%d", r);
                    KEY_LOG("  Audio state : Audio operation(MSD start no-op) failed");
                }
                break;
            }

            case EcActionKind::AudioEndMsd:
            {
                // EcPhase::MsdSuccess / MsdFail / OutbandMsdSuccess / OutbandMsdFail:
                // stop prompt and unmute voice for hands-free conversation.
                le_result_t r = audio.EndMsdTransmission();
                if (r != LE_OK)
                {
                    LE_ERROR("EcallMgrEcall: AudioEndMsd failed rc=%d", r);
                    KEY_LOG("  Audio state : Audio operation(Msd transmission end) failed");
                }
                else
                {
                    KEY_LOG("  Audio state : playing stopped");
                    KEY_LOG("  Audio state : voice unmuted");
                }
                break;
            }

            case EcActionKind::AudioTeardown:
            {
                le_result_t r = audio.TeardownAudio();
                if (r != LE_OK)
                {
                    LE_ERROR("EcallMgrEcall: AudioTeardown failed rc=%d", r);
                    KEY_LOG("  Audio state : Audio disconnect failed");
                }
                else
                {
                    KEY_LOG("  Audio state : Playing route closed");
                    KEY_LOG("  Audio state : Voice call route closed");
                }
                break;
            }

            // -----------------------------------------------------------------
            // Power manager
            // -----------------------------------------------------------------

            case EcActionKind::PowerCallbackWindowStart:
                pwr.OnCallbackWindowStart();
                break;

            case EcActionKind::PowerCallbackWindowEnd:
                pwr.OnCallbackWindowEnd();
                break;

            // -----------------------------------------------------------------
            // PSAP / MSD
            // -----------------------------------------------------------------

            case EcActionKind::PsapExecuteSendMsd:
            {
                DoSetVehicleData();
                DoSetLocationData();
                DoSendMsd();
                break;
            }

            case EcActionKind::None:
            default:
                break;
        }
    }
}

void EcallMgrEcall::SaveCallData()
{
    LE_INFO("EcallMgrEcall: SaveCallData");

    auto& logger  = EcallMgrDataLogger::GetInstance();
    auto& network = EcallMgrNetwork::GetInstance();
    constexpr uint8_t phoneId = 1;

    logger.BeginCallRecord();

    // 1. MSD content
    std::vector<uint8_t> msdContent;
    if (DoGetMsdContent(msdContent) == LE_OK)
        logger.SetMsdContent(msdContent);
    else
        LE_WARN("EcallMgrEcall: SaveCallData: GetMsdContent failed");

    // 2. Network status (MCC/MNC)
    EcNetworkStatus netStatus;
    if (network.GetNetworkStatus(phoneId, netStatus) == LE_OK)
        logger.SetNetwork(netStatus.mcc, netStatus.mnc);
    else
        LE_WARN("EcallMgrEcall: SaveCallData: GetNetworkStatus failed");

    // 3. Signal strength (prefer RSRP for LTE/5G, fall back to RSSI)
    EcSignalStrength sigStrength;
    if (network.GetSignalStrength(phoneId, sigStrength) == LE_OK)
    {
        if (sigStrength.rsrp != INT32_MIN)
            logger.SetSignalLevel(sigStrength.rsrp);
        else if (sigStrength.rawRssi != INT32_MIN)
            logger.SetSignalLevel(sigStrength.rawRssi);
    }
    else
    {
        LE_WARN("EcallMgrEcall: SaveCallData: GetSignalStrength failed");
    }

    logger.SaveCallDataToFile();
}

// -----------------------------------------------------------------------------
// State machine implementation
// -----------------------------------------------------------------------------

void EcallMgrEcall::HandleEcEvent(EcEvent ev)
{
    EcSessionState cur  = state_;
    EcSessionState next = cur;

    switch (cur)
    {
        case EcSessionState::Idle:
        {
            switch (ev)
            {
                case EcEvent::CallStarted:
                    next = EcSessionState::CallSetup;
                    DoSetVehicleData();
                    break;

                default:
                    break;
            }
            break;
        }

        case EcSessionState::CallSetup:
        {
            switch (ev)
            {
                case EcEvent::CallEnded:
                    next = EcSessionState::Callback;
                    break;

                case EcEvent::CallConnected:
                    next = EcSessionState::Connected;
                    break;

                case EcEvent::OperationError:
                    next = EcSessionState::Idle;
                    break;

                case EcEvent::MsdOutbandFailed:
                    next = EcSessionState::Voice;  // out-band MSD done -> voice
                    break;

                case EcEvent::MsdOutbandSuccess:
                    SaveCallData();
                    next = EcSessionState::Voice;  // out-band MSD done -> voice
                    break;

                default:
                    break;
            }
            break;
        }

        case EcSessionState::Connected:
        {
            switch (ev)
            {
                case EcEvent::CallEnded:
                    next = EcSessionState::Callback;
                    break;

                case EcEvent::CallbackWindowTimeout:
                    next = EcSessionState::Idle;
                    break;

                case EcEvent::MsdOutbandFailed:
                    next = EcSessionState::Voice;  // out-band MSD done -> voice
                    break;

                case EcEvent::MsdOutbandSuccess:
                    SaveCallData();
                    next = EcSessionState::Voice;  // out-band MSD done -> voice
                    break;

                case EcEvent::MsdInbandSuccess:
                    SaveCallData();
                    next = EcSessionState::Voice;
                    break;

                case EcEvent::MsdInbandFailed:
                    next = EcSessionState::Voice;
                    break;

                default:
                    break;
            }
            break;
        }

        case EcSessionState::SendingMsd:
        {
            switch (ev)
            {
                case EcEvent::CallEnded:
                    next = EcSessionState::Callback;
                    break;

                case EcEvent::MsdOutbandFailed:
                    next = EcSessionState::Voice;  // out-band MSD done -> voice
                    break;

                case EcEvent::MsdOutbandSuccess:
                    SaveCallData();
                    next = EcSessionState::Voice;  // out-band MSD done -> voice
                    break;

                case EcEvent::MsdInbandSuccess:
                    SaveCallData();
                    next = EcSessionState::Voice;
                    break;

                case EcEvent::MsdInbandFailed:
                    next = EcSessionState::Voice;
                    break;

                default:
                    break;
            }
            break;
        }

        case EcSessionState::Voice:
        {
            switch (ev)
            {
                case EcEvent::MsdRequested:
                    next = EcSessionState::SendingMsd;
                    break;

                case EcEvent::CallEnded:
                    next = EcSessionState::Callback;
                    break;

                default:
                    break;
            }
            break;
        }

        case EcSessionState::Callback:
        {
            switch (ev)
            {
                case EcEvent::CallStarted:
                    next = EcSessionState::CallSetup;
                    DoSetVehicleData();
                    break;

                case EcEvent::CallIncoming:
                    next = EcSessionState::CallSetup;
                    DoAnswerIncomingCall();
                    break;

                case EcEvent::CallbackWindowTimeout:
                    next = EcSessionState::Idle;
                    ApiLoc::GetInstance().StopGnss();
                    break;

                default:
                    break;
            }
            break;
        }
    }

    if (next != cur)
    {
        LE_INFO("EcallMgr-Ecall SM: CUR STATE:%s EVENT:%s NEXT STATE:%s",
                ToStr(cur), ToStr(ev), ToStr(next));
        state_ = next;

        // When the SM reaches Idle the eCall session is fully over.
        // Release the wake lock unconditionally so the NAD can suspend.
        // This covers both the normal path (T9 EXPIRED already called
        // OnCallbackWindowEnd) and error paths (OperationError, no T9)
        // where OnCallbackWindowEnd may never have been called.
        // OnCallbackWindowEnd is idempotent when holdId_ is already -1.
        if (next == EcSessionState::Idle)
        {
            EcallMgrPower::GetInstance().OnCallbackWindowEnd();
        }
    }
    else
    {
        LE_DEBUG("EcallMgr-Ecall SM: ignore EVENT:%s in STATE:%s",
                 ToStr(ev), ToStr(cur));
    }
}

void EcallMgrEcall::DoSetVehicleData()
{
    LE_INFO("EcallMgr-Ecall: DoSetVehicleData");

    const EcConfigData& cfg = lastCfg_;
    char vinBuf[18] = {};
    const char* vinCStr = (GetVin(vinBuf, sizeof(vinBuf)) == LE_OK) ? vinBuf : "";

    const taf_ecall_PropulsionStorageType_t propMask =
        ConvertFuelToPropulsionMask(cfg);
    const taf_ecall_MsdVehicleType_t vehicleType =
        ConvertIntToVehicleType(cfg.vehicleType);

    auto& api = ApiEcall::GetInstance();

    if (vinCStr[0] != '\0')
    {
        le_result_t rc = api.SetVin(vinCStr);
        ReportOperationResult("SetVin", rc);
        if (rc != LE_OK) return;
    }

    le_result_t rc = api.SetVehicleInfo(vehicleType, propMask);
    ReportOperationResult("SetVehicleInfo", rc);
}

void EcallMgrEcall::DoSetLocationData()
{
    (void)PushZeroBaseMsdWithRecent();
    LE_INFO("EcallMgr-Ecall: DoSetLocationData");
    le_result_t rc = ApiEcall::GetInstance().UpdateMsdPosition(lastMsdPos_);
    ReportOperationResult("UpdateMsdPosition", rc);
}

void EcallMgrEcall::DoSendMsd()
{
    LE_INFO("EcallMgr-Ecall: DoSendMsd");
    le_result_t rc = ApiEcall::GetInstance().SendMsd();
    ReportOperationResult("SendMsd", rc);
}

le_result_t EcallMgrEcall::DoGetMsdContent(std::vector<uint8_t>& msdContent)
{
    LE_INFO("EcallMgr-Ecall: DoGetMsdContent");
    le_result_t rc = ApiEcall::GetInstance().GetMsdContent(msdContent);
    ReportOperationResult("GetMsdContent", rc);
    return rc;
}

void EcallMgrEcall::DoAnswerIncomingCall()
{
    LE_INFO("EcallMgr-Ecall: DoAnswerIncomingCall — waiting %u ms before answering",
            static_cast<unsigned>(kAnswerDelayMs));
    StartAnswerDelayTimer();
}


void EcallMgrEcall::StartAnswerDelayTimer()
{
    if (answerDelayTimer_)
    {
        LE_WARN("EcallMgr-Ecall: StartAnswerDelayTimer: timer already running");
        return;
    }

    answerDelayTimer_ = le_timer_Create("ec.answerDelay");
    if (!answerDelayTimer_)
    {
        LE_ERROR("EcallMgr-Ecall: failed to create answer delay timer; answering immediately");
        le_result_t rc = ApiEcall::GetInstance().Answer();
        ReportOperationResult("Answer", rc);
        return;
    }

    le_clk_Time_t interval{};
    interval.sec  = kAnswerDelayMs / 1000U;
    interval.usec = static_cast<long>((kAnswerDelayMs % 1000U) * 1000U);

    le_timer_SetInterval(answerDelayTimer_, interval);
    le_timer_SetRepeat(answerDelayTimer_, 1);
    le_timer_SetHandler(answerDelayTimer_, EcallMgrEcall::OnAnswerDelayTimeout);
    le_timer_SetContextPtr(answerDelayTimer_, this);

    le_result_t rc = le_timer_Start(answerDelayTimer_);
    if (rc != LE_OK)
    {
        LE_ERROR("EcallMgr-Ecall: failed to start answer delay timer rc=%d; answering immediately",
                 rc);
        le_timer_Delete(answerDelayTimer_);
        answerDelayTimer_ = nullptr;
        le_result_t arc = ApiEcall::GetInstance().Answer();
        ReportOperationResult("Answer", arc);
        return;
    }

    LE_INFO("EcallMgr-Ecall: answer delay timer started (%u ms)",
            static_cast<unsigned>(kAnswerDelayMs));
}

// static
void EcallMgrEcall::OnAnswerDelayTimeout(le_timer_Ref_t timerRef)
{
    auto* self = static_cast<EcallMgrEcall*>(le_timer_GetContextPtr(timerRef));
    if (!self) return;

    le_timer_Delete(timerRef);
    self->answerDelayTimer_ = nullptr;

    KEY_LOG("  eCall answer delay expired — answering incoming call");
    LE_INFO("EcallMgr-Ecall: answer delay expired; calling Answer()");

    le_result_t rc = ApiEcall::GetInstance().Answer();
    self->ReportOperationResult("Answer", rc);
}

void EcallMgrEcall::DoDialCall(EcCallType callType)
{
    const EcConfigData& cfg = lastCfg_;
    const char* number = "112";

    if (callType == EcCallType::Test)
    {
        if (cfg.testNumber[0] != '\0') number = cfg.testNumber;
    }
    else
    {
        if (cfg.emergencyNumber[0] != '\0') number = cfg.emergencyNumber;
    }

    auto& api = ApiEcall::GetInstance();
    le_result_t rc = api.SetPsapNumber(number);
    ReportOperationResult("SetPsapNumber", rc);
    if (rc != LE_OK)
    {
        HandleEcEvent(EcEvent::OperationError);
        return;
    }

    if (callType == EcCallType::Automatic) rc = api.StartAutomatic();
    else if (callType == EcCallType::Manual)    rc = api.StartManual();
    else if (callType == EcCallType::Test)      rc = api.StartTest();
    else
    {
        LE_ERROR("EcallMgr-Ecall: DoDialCall unknown callType=%d",
                 static_cast<int>(callType));
        HandleEcEvent(EcEvent::OperationError);
        return;
    }
    ReportOperationResult("Start", rc);
    if (rc != LE_OK)
        HandleEcEvent(EcEvent::OperationError);
}

void EcallMgrEcall::StartEcall(EcCallType callType)
{
    LE_INFO("EcallMgr-Ecall: StartEcall callType=%d",
            static_cast<int>(callType));

    locationReadyPending_ = true;
    pendingCallType_      = callType;

    EcallMgrPower::GetInstance().SetOnWakeupReadyCb([]()
    {
        auto& mgr = EcallMgrEcall::GetInstance();
        ApiLoc::GetInstance().StopGnss();
        ApiLoc::GetInstance().StartGnss();
        mgr.StartLocationWaitTimer();
    });

    EcallMgrPower::GetInstance().OnPsapStateChange(EcPowerSessionState::Processing);

    HandleEcEvent(EcEvent::CallStarted);

    LE_INFO("EcallMgrEcall: StartEcall — dial deferred until valid GNSS position or timeout");
}

void EcallMgrEcall::ReportOperationResult(const char* opName, le_result_t rc)
{
    LE_INFO("EcallMgr-Ecall: Operation %s result=%d",
            opName ? opName : "?",
            rc);
}

} // namespace ecall
