/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EcallEventBus.hpp"

namespace ecall
{

EcallEventBus& EcallEventBus::GetInstance()
{
    static EcallEventBus inst;
    return inst;
}

EcallEventBus::EcallEventBus()
{
    // Create Legato event IDs for all eCall-related channels.
    // Names kept short but descriptive; payload sizes are strongly typed.
    locFixEvt_ = le_event_CreateId("ec.locFix", sizeof(LocFix));
    phaseEvt_  = le_event_CreateId("ec.phase",  sizeof(EvPhase));
    psapEvt_   = le_event_CreateId("ec.psap",   sizeof(EvPsap));
    timerEvt_  = le_event_CreateId("ec.timer",  sizeof(EvTimer));
    hmsEvt_    = le_event_CreateId("ec.hms",    sizeof(ModemHmsEvt));

    LE_INFO("EcallEventBus: event IDs created "
            "(locFix=%p phase=%p psap=%p timer=%p hms=%p)",
            locFixEvt_, phaseEvt_, psapEvt_, timerEvt_, hmsEvt_);
}

// ---------------------------------------------------------------------
// Typed publish helpers
// ---------------------------------------------------------------------

void EcallEventBus::PublishLocFix(const LocFix& fix) const
{
    le_event_Report(locFixEvt_, const_cast<LocFix*>(&fix), sizeof(fix));
}

void EcallEventBus::PublishPhase(const EvPhase& ev) const
{
    le_event_Report(phaseEvt_, const_cast<EvPhase*>(&ev), sizeof(ev));
}

void EcallEventBus::PublishPsap(const EvPsap& ev) const
{
    le_event_Report(psapEvt_, const_cast<EvPsap*>(&ev), sizeof(ev));
}

void EcallEventBus::PublishTimer(const EvTimer& ev) const
{
    le_event_Report(timerEvt_, const_cast<EvTimer*>(&ev), sizeof(ev));
}

void EcallEventBus::PublishHms(const ModemHmsEvt& ev) const
{
    le_event_Report(hmsEvt_, const_cast<ModemHmsEvt*>(&ev), sizeof(ev));
}

// ---------------------------------------------------------------------
// Generic low-level publish
// ---------------------------------------------------------------------

void EcallEventBus::Report(le_event_Id_t evt,
                           const void*   payload,
                           size_t        size) const
{
    le_event_Report(evt, const_cast<void*>(payload), size);
}

} // namespace ecall
