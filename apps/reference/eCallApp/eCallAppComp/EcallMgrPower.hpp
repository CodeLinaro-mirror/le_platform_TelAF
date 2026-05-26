/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include "ApiMngdPm.hpp"
#include "EcallDefs.hpp"

namespace ecall
{

// -----------------------------------------------------------------------------
// EcallMgrPower
//
// eCall power manager: coordinates with ApiMngdPm to hold / release the
// system wakeup lock across the eCall lifecycle.
//
// Wake-lock state machine:
//
//   Dialing / Connected / Processing
//     -> HoldPowerAsync(EcallActive)
//
//   T9 Start / Resume (callback window opens, call may still be active)
//     -> if already holding: UpdateHoldPowerReasonAsync(* -> CallbackActive)
//     -> if not holding:     HoldPowerAsync(CallbackActive)
//
//   Teardown (while IN callback window)
//     -> UpdateHoldPowerReasonAsync(EcallActive -> CallbackActive)
//
//   Teardown (while NOT in callback window)
//     -> ReleasePowerAsync
//
//   T9 Stop (callback call answered, new call becomes active)
//     -> OnCallbackWindowEnd() called; inCall_=true guard keeps wake lock
//     -> UpdateHoldPowerReasonAsync(CallbackActive -> EcallActive)
//
//   T9 Expire (callback window closed, no active call)
//     -> ReleasePowerAsync
//
// Key invariant:
//   UpdateHoldPowerReasonAsync always does StayAwake(new) before
//   Relax(old)+Delete(old), so wsCount goes from 1 to 2 to 1 — never 0.
//   The only path that takes wsCount to 0 is ReleasePowerAsync, which is
//   called only on Teardown (no callback window) or T9 Expire.
//
// Thread safety:
//   All public methods are called on the Legato main event-loop thread.
//   ApiMngdPm callbacks are posted back to the caller's thread (main loop)
//   by ApiMngdPm itself, so holdId_ is always read and written on the same
//   thread — no mutex required.
//
// Singleton; use GetInstance() for access.
// -----------------------------------------------------------------------------
class EcallMgrPower
{
public:
    static EcallMgrPower& GetInstance();

    // Register a one-shot callback invoked on the main thread once the
    // first wakeup lock is successfully acquired (PM wakeup complete).
    // Used by EcallMgrEcall::StartEcall() to defer GNSS start until
    // the system is fully awake.
    // The callback is cleared automatically after it fires.
    void SetOnWakeupReadyCb(std::function<void()> cb)
    {
        onWakeupReady_ = std::move(cb);
    }

    // Called by EcallMgrEcall when the PSAP session state changes.
    void OnPsapStateChange(EcPowerSessionState state);

    // Called when the T9 callback window opens / closes.
    void OnCallbackWindowStart();
    void OnCallbackWindowEnd();

private:
    EcallMgrPower() = default;

        // Decide whether to acquire, update, or release the wakeup lock.
    void ApplyHoldPolicy(bool                       needHold,
                         bool                       needRelease,
                         ApiMngdPm::StayAwakeReason newReason);

    // Handle call teardown: transition EcallActive->CallbackActive or release.
    // Called internally from OnPsapStateChange(Teardown).
    void HandleCallTeardown();

    // Current wakeup-lock id (-1 = none held).
    // Written only from ApiMngdPm callbacks, which are posted back to the
    // main event-loop thread by ApiMngdPm, so no synchronisation needed.
    int  holdId_{-1};

    bool inCall_{false};           // true while eCall is active
    bool inCallbackWindow_{false}; // true while T9 callback window is open

    // One-shot callback fired on main thread after first wakeup lock acquired.
    std::function<void()> onWakeupReady_;

    ApiMngdPm::StayAwakeReason currentReason_{ApiMngdPm::StayAwakeReason::None};
};

} // namespace ecall

