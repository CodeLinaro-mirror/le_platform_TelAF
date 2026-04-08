/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EcallMgrLocation.hpp"

#include <cmath>
#include <climits>
#include <inttypes.h>

extern "C"
{
#include "legato.h"
}
#include "interfaces.h"      // TAF_LOCGNSS_HAS_* bitmask constants
#include "EcallEventBus.hpp"
#include "EcallMgrEcall.hpp" // EcallMgrEcall::LocRingBufferHandler

namespace ecall
{

// =============================================================================
// File-local helpers  (ETSI EN 15722 conversion logic)
// =============================================================================
namespace
{

// Horizontal accuracy threshold for "trusted" position (ETSI EN 15722).
constexpr double kHAccTrustThresholdM = 150.0;

// ETSI sentinel values.
constexpr int32_t kMasInvalid        = INT32_MAX;  // unknown lat/lon
constexpr int32_t kVehicleDirInvalid = 255;        // unknown direction

// Convert geographic degrees to milli-arcseconds (MAS).
//
// 1 degree = 3 600 000 milli-arcseconds.
// Direct multiplication is used to avoid sign-handling pitfalls of a manual
// DMS decomposition (negative degrees produce wrong minute/second signs when
// truncating toward zero).
//
// Valid ETSI ranges:
//   latitude  [-90,  +90]  deg  → [-324 000 000, +324 000 000] MAS
//   longitude [-180, +180] deg  → [-648 000 000, +648 000 000] MAS
//
// Returns true when the result is within the valid range.
// outMas is always written (even on range failure) so callers can log it.
bool ToMasDeg(double degrees, bool isLatitude, int32_t& outMas)
{
    if (std::isnan(degrees) || std::isinf(degrees))
    {
        outMas = kMasInvalid;
        return false;
    }

    const double  masDouble = degrees * 3'600'000.0;
    const int32_t mas       = static_cast<int32_t>(std::round(masDouble));

    const int32_t minBound  = isLatitude ? -324'000'000 : -648'000'000;
    const int32_t maxBound  = isLatitude ?  324'000'000 :  648'000'000;

    outMas = mas;
    return (mas >= minBound && mas <= maxBound);
}

// Discretise a true-north heading (degrees) into a 2-degree step (0..179).
// Applies magnetic deviation correction when available.
// Returns kVehicleDirInvalid when heading is not available.
int32_t ToDirectionStep(float headingDeg, float magDevDeg, bool hasMagDev)
{
    double dir = static_cast<double>(headingDeg);
    if (hasMagDev)
    {
        dir -= static_cast<double>(magDevDeg);
    }

    // Normalise to [0, 360).
    dir = std::fmod(dir, 360.0);
    if (dir < 0.0)
    {
        dir += 360.0;
    }

    // Each step covers 2 degrees; valid range 0..179.
    const int step = static_cast<int>(dir / 2.0);
    return (step >= 0 && step <= 179) ? step : kVehicleDirInvalid;
}

} // anonymous namespace

// =============================================================================
// EcallMgrLocation
// =============================================================================

EcallMgrLocation& EcallMgrLocation::GetInstance()
{
    static EcallMgrLocation inst;
    return inst;
}

EcallMgrLocation::EcallMgrLocation()
{
    le_event_AddHandler(
        "ec.loc",
        EcallEventBus::GetInstance().LocFixChannel(),
        EcallMgrLocation::OnLoc);

    LE_INFO("EcallMgr-Location: LocFix handler registered");
}

// -----------------------------------------------------------------------------
// OnLoc  — Bus callback, runs on the Legato main event loop thread.
//
// Converts a raw LocFix (degrees, m, validity mask) into a LocConverted
// (MAS, trust flag, direction step) and pushes it into EcallMgrEcall's
// ring buffer for MSD recent-location (N1/N2) computation.
// -----------------------------------------------------------------------------
void EcallMgrLocation::OnLoc(void* payload)
{
    if (!payload)
    {
        LE_WARN("EcallMgr-Location: OnLoc null payload");
        return;
    }

    const auto* fix = static_cast<const LocFix*>(payload);

    const uint32_t validityMask = fix->locValidMask;
    const uint8_t  svUsed       = fix->satUsed;

    // -------------------------------------------------------------------------
    // 1. Lat / lon conversion
    // -------------------------------------------------------------------------
    const bool hasLatLon = (validityMask & TAF_LOCGNSS_HAS_LAT_LONG_BIT) != 0u;
    const bool hasSats   = (svUsed > 0);

    int32_t latMas = kMasInvalid;
    int32_t lonMas = kMasInvalid;

    if (hasLatLon && hasSats)
    {
        int32_t latTmp = kMasInvalid;
        int32_t lonTmp = kMasInvalid;

        const bool latOk = ToMasDeg(fix->latDeg, /*isLatitude=*/true,  latTmp);
        const bool lonOk = ToMasDeg(fix->lonDeg, /*isLatitude=*/false, lonTmp);

        if (latOk && lonOk)
        {
            // Reject the null-island (0°N, 0°E) — not a valid vehicle position.
            if (latTmp == 0 && lonTmp == 0)
            {
                LE_WARN("EcallMgr-Location: rejecting null-island (0,0) fix");
            }
            else
            {
                latMas = latTmp;
                lonMas = lonTmp;
            }
        }
        else
        {
            LE_WARN("EcallMgr-Location: lat/lon out of ETSI range "
                    "(lat=%.6f lon=%.6f)", fix->latDeg, fix->lonDeg);
        }
    }

    // -------------------------------------------------------------------------
    // 2. Position trust
    //    Requires: valid lat/lon bit + at least one satellite used +
    //              horizontal accuracy reported and <= 150 m.
    // -------------------------------------------------------------------------
    const bool hasHAcc = (validityMask & TAF_LOCGNSS_HAS_HORIZONTAL_ACCURACY_BIT) != 0u;
    const bool posValid = (latMas != kMasInvalid) && (lonMas != kMasInvalid);

    const bool posTrusted = posValid
                         && hasHAcc
                         && (std::fabs(fix->hAccM) <= kHAccTrustThresholdM);

    // -------------------------------------------------------------------------
    // 3. Vehicle direction
    // -------------------------------------------------------------------------
    const bool hasHeading = (validityMask & TAF_LOCGNSS_HAS_HEADING_BIT) != 0u;
    const bool hasMagDev  = (validityMask & TAF_LOCGNSS_HAS_MAGNETIC_DEVIATION) != 0u;

    const int32_t direction = hasHeading
        ? ToDirectionStep(fix->headingDeg, fix->magDevDeg, hasMagDev)
        : kVehicleDirInvalid;

    // -------------------------------------------------------------------------
    // 4. Forward to ring buffer
    // -------------------------------------------------------------------------
    const LocConverted d{
        posTrusted,
        latMas,
        lonMas,
        direction,
        fix->epochMs
    };

    LE_DEBUG("EcallMgr-Location: latMas=%d lonMas=%d trusted=%d dir=%d "
             "hAccM=%.1f svUsed=%u tsMs=%" PRIu64,
             d.latMas, d.lonMas,
             d.posTrusted ? 1 : 0,
             d.direction,
             fix->hAccM,
             static_cast<unsigned>(svUsed),
             d.tsUtcMs);

    EcallMgrEcall::GetInstance().LocRingBufferHandler(d);

}

} // namespace ecall

