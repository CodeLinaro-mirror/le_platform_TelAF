/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include "EcallDefs.hpp"  // LocConverted, LocFix

namespace ecall
{

// -----------------------------------------------------------------------------
// EcallMgrLocation
//
// Bridges ApiLoc (raw GNSS service) and EcallMgrEcall (MSD ring buffer).
//
// Responsibilities:
//   1. Subscribe to LocFix events published by ApiLoc on EcallEventBus.
//   2. Validate and convert each LocFix:
//        - degrees  → milli-arcseconds (MAS), ETSI EN 15722 range check
//        - hAcc     → position-trust flag (threshold: 150 m)
//        - heading  → 2-degree-step direction (0..179; 255 = unknown)
//   3. Forward the resulting LocConverted to EcallMgrEcall::LocRingBufferHandler.
//
// The conversion logic (ToMasFromDeg) is intentionally kept here rather than
// in ApiLoc, because it encodes ETSI-specific knowledge (MAS ranges, trust
// thresholds, direction discretisation) that belongs to the eCall domain, not
// to the generic GNSS service wrapper.
//
// Singleton; use GetInstance() for access.
// -----------------------------------------------------------------------------
class EcallMgrLocation
{
public:
    static EcallMgrLocation& GetInstance();

private:
    EcallMgrLocation();

    // Bus callback registered in the constructor.
    // Payload: const LocFix* published by ApiLoc.
    static void OnLoc(void* payload);
};

} // namespace ecall

