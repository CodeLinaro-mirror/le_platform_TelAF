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

#include <cstddef>

#include "EcallDefs.hpp"

namespace ecall
{

/**
 * @brief
 *   Central in-process event bus used by all eCall components.
 *
 * This class wraps Legato's le_event mechanism and provides typed
 * channels for all main eCall-related events:
 *   - Location fixes (LocFix)
 *   - eCall phase changes (EvPhase)
 *   - PSAP call events (EvPsap)
 *   - Internal timers (EvTimer)
 *   - Modem health/monitoring events (ModemHmsEvt)
 *
 * Design:
 *   - Implemented as a process-wide singleton: GetInstance().
 *   - Each event type has a dedicated le_event_Id_t created in the ctor.
 *   - Publisher helpers (PublishXxx) wrap le_event_Report with the
 *     correct payload type and size.
 *
 * Typical usage:
 *
 *   // Register an event handler for location fixes (e.g. in EcallMgrLocation)
  *   le_event_AddHandler("EcallLocHandler",
 *                       EcallEventBus::GetInstance().LocFixChannel(),
 *                       &MyLocFixHandler);
 *
 *   // Later, publish a new location fix (e.g. in ApiLoc)
 *   ecall::LocFix fix = { ... };
 *   EcallEventBus::GetInstance().PublishLocFix(fix);
 */
class EcallEventBus
{
public:
    /// @return Singleton instance of the event bus.
    static EcallEventBus& GetInstance();

    // ---------------------------------------------------------------------
    // Accessors for the underlying Legato event IDs
    // ---------------------------------------------------------------------

        le_event_Id_t LocFixChannel() const { return locFixEvt_; }
    le_event_Id_t PhaseChannel()  const { return phaseEvt_;  }
    le_event_Id_t PsapChannel()   const { return psapEvt_;   }
    le_event_Id_t TimerChannel()  const { return timerEvt_;  }
    le_event_Id_t HmsChannel()    const { return hmsEvt_;    }

    // ---------------------------------------------------------------------
    // Typed publish helpers
    // ---------------------------------------------------------------------

    /// Publish a location fix to all subscribers.
    void PublishLocFix(const LocFix& fix) const;

    /// Publish an eCall phase change event.
    void PublishPhase(const EvPhase& ev) const;

    /// Publish a PSAP event.
    void PublishPsap(const EvPsap& ev) const;

    /// Publish an internal timer event.
    void PublishTimer(const EvTimer& ev) const;

    /// Publish a modem HMS event.
    void PublishHms(const ModemHmsEvt& ev) const;

    /**
     * @brief
     *   Low-level, generic publish helper.
     *
     * This allows publishing events by le_event_Id_t directly, which can
     * be useful for unit tests or less-typed components. For normal
     * production code, prefer the typed PublishXxx() helpers above.
     *
     * @param evt     Event ID to publish on.
     * @param payload Pointer to payload data (must outlive the call).
     * @param size    Payload size in bytes.
     */
    void Report(le_event_Id_t evt, const void* payload, size_t size) const;

private:
    EcallEventBus();
    ~EcallEventBus() = default;

    // Non-copyable singleton
    EcallEventBus(const EcallEventBus&) = delete;
    EcallEventBus& operator=(const EcallEventBus&) = delete;

    // Underlying Legato event IDs for each channel
    le_event_Id_t locFixEvt_{nullptr};
    le_event_Id_t phaseEvt_{nullptr};
    le_event_Id_t psapEvt_{nullptr};
    le_event_Id_t timerEvt_{nullptr};
    le_event_Id_t hmsEvt_{nullptr};
};

} // namespace ecall
