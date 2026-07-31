/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EcallMgrPower.hpp"

extern "C"
{
#include "legato.h"
}

namespace ecall
{

#define LOG_NAME "EcallMgrPower"

// =============================================================================
// Singleton
// =============================================================================
EcallMgrPower& EcallMgrPower::GetInstance()
{
    static EcallMgrPower instance;
    return instance;
}

// =============================================================================
// Public interface
// =============================================================================

void EcallMgrPower::OnPsapStateChange(EcPowerSessionState state)
{
    bool needHold    = false;
    bool needRelease = false;
    ApiMngdPm::StayAwakeReason newReason = ApiMngdPm::StayAwakeReason::None;

    switch (state)
    {
                case EcPowerSessionState::Connected:
        case EcPowerSessionState::Processing:
            // A new/active eCall may start while the previous call's T9
            // callback window is still open.  Close the old window so that
            // a later T9 STOP/EXPIRE from the previous session does not
            // release the new call's wake lock.
            if (inCallbackWindow_)
            {
                LE_INFO("[%s] OnPsapStateChange: active eCall closes old callback window",
                        LOG_NAME);
                inCallbackWindow_ = false;
            }
            needHold  = true;
            newReason = ApiMngdPm::StayAwakeReason::EcallActive;
            inCall_   = true;
            break;

        case EcPowerSessionState::Teardown:
            HandleCallTeardown();
            return;

        case EcPowerSessionState::Idle:
            // Idle is reported after Teardown.  Do not release here; the wake
            // lock must stay held through the T9 callback window.  The only
            // release point is T9 EXPIRED -> OnCallbackWindowEnd().
            inCall_ = false;
            LE_INFO("[%s] OnPsapStateChange: Idle — no release, wait for T9 expiry",
                    LOG_NAME);
            return;

        case EcPowerSessionState::Unknown:
        default:
            return;
    }

    ApplyHoldPolicy(needHold, needRelease, newReason);
}

void EcallMgrPower::HandleCallTeardown()
{
    LE_INFO("[%s] HandleCallTeardown inCall=%d cbWin=%d holdId=%d reason=%d",
            LOG_NAME,
            inCall_ ? 1 : 0,
            inCallbackWindow_ ? 1 : 0,
            holdId_,
            static_cast<int>(currentReason_));

    inCall_ = false;

    // Never release the wake lock on Teardown regardless of whether the T9
    // callback window flag is set yet.  T9 START and Teardown arrive
    // simultaneously (same timestamp in the log); inCallbackWindow_ may still
    // be false when Teardown is processed even though T9 is about to start.
    // Releasing here causes wsCount to hit 0 and the NAD to suspend before
    // OnCallbackWindowStart() can re-acquire.
    //
    // Policy:
    //   - If already in callback window: switch reason EcallActive->CallbackActive.
    //   - If NOT yet in callback window: keep the existing wake lock as-is;
    //     OnCallbackWindowStart() will update the reason when T9 fires.
    //   - If no wake lock held at all: nothing to do; OnCallbackWindowStart()
    //     will acquire one.
    //
    // The wake lock is released only by OnCallbackWindowEnd() on T9 EXPIRED.

    if (inCallbackWindow_ &&
        holdId_ >= 0 &&
        currentReason_ != ApiMngdPm::StayAwakeReason::CallbackActive)
    {
        const int oldId = holdId_;
        currentReason_  = ApiMngdPm::StayAwakeReason::CallbackActive;

        LE_INFO("[%s] HandleCallTeardown: switch EcallActive->CallbackActive oldId=%d",
                LOG_NAME, oldId);

        ApiMngdPm::GetInstance().UpdateHoldPowerReasonAsync(
            oldId,
            ApiMngdPm::StayAwakeReason::CallbackActive,
            "ecallmgr",
            [this](le_result_t result, int newId)
            {
                if (result != LE_OK)
                {
                    LE_ERROR("[%s] UpdateHoldPower(Teardown) failed rc=%d",
                             LOG_NAME, result);
                    holdId_ = -1;
                    currentReason_ = ApiMngdPm::StayAwakeReason::None;
                    return;
                }
                holdId_ = newId;
                LE_INFO("[%s] UpdateHoldPower(Teardown) ok newId=%d",
                        LOG_NAME, newId);
            });
    }
    else
    {
        LE_INFO("[%s] HandleCallTeardown: keeping wake lock id=%d; "
                "OnCallbackWindowStart will update reason",
                LOG_NAME, holdId_);
    }
}

void EcallMgrPower::OnCallbackWindowStart()
{
    LE_INFO("[%s] OnCallbackWindowStart holdId=%d", LOG_NAME, holdId_);
    inCallbackWindow_ = true;

    if (holdId_ >= 0)
    {
        // Already holding: switch reason to CallbackActive.
        const int oldId = holdId_;
        currentReason_  = ApiMngdPm::StayAwakeReason::CallbackActive;

        ApiMngdPm::GetInstance().UpdateHoldPowerReasonAsync(
            oldId,
            ApiMngdPm::StayAwakeReason::CallbackActive,
            "ecallmgr",
            [this](le_result_t result, int newId)
            {
                if (result != LE_OK)
                {
                    LE_ERROR("[%s] UpdateHoldPower(CbWinStart) failed rc=%d",
                             LOG_NAME, result);
                    holdId_ = -1;
                    currentReason_ = ApiMngdPm::StayAwakeReason::None;
                    return;
                }
                holdId_ = newId;
                LE_INFO("[%s] UpdateHoldPower(CbWinStart) ok newId=%d",
                        LOG_NAME, newId);
            });
    }
    else
    {
        // No existing hold: acquire a new one.
        currentReason_ = ApiMngdPm::StayAwakeReason::CallbackActive;

        ApiMngdPm::GetInstance().HoldPowerAsync(
            ApiMngdPm::StayAwakeReason::CallbackActive,
            "ecallmgr",
            [this](le_result_t result, int id)
            {
                if (result != LE_OK)
                {
                    LE_ERROR("[%s] HoldPower(CbWinStart) failed rc=%d",
                             LOG_NAME, result);
                    currentReason_ = ApiMngdPm::StayAwakeReason::None;
                    return;
                }
                holdId_ = id;
                LE_INFO("[%s] HoldPower(CbWinStart) ok id=%d", LOG_NAME, id);
            });
    }
}

void EcallMgrPower::OnCallbackWindowEnd()
{
    LE_INFO("[%s] OnCallbackWindowEnd holdId=%d inCall=%d reason=%d",
            LOG_NAME,
            holdId_,
            inCall_ ? 1 : 0,
            static_cast<int>(currentReason_));
    inCallbackWindow_ = false;

    // Guard: if a new call became active before T9 expired (e.g. callback
    // call answered), do not release the wake lock.
    if (inCall_)
    {
        if (holdId_ >= 0 &&
            currentReason_ == ApiMngdPm::StayAwakeReason::CallbackActive)
        {
            // Switch back to EcallActive.
            // UpdateHoldPowerReasonAsync does StayAwake(new) then
            // DeleteWakeupSource(old) without Relax — wsCount stays >= 1.
            const int oldId = holdId_;
            currentReason_  = ApiMngdPm::StayAwakeReason::EcallActive;

            LE_INFO("[%s] OnCallbackWindowEnd: call active, switch CallbackActive->EcallActive oldId=%d",
                    LOG_NAME, oldId);

            ApiMngdPm::GetInstance().UpdateHoldPowerReasonAsync(
                oldId,
                ApiMngdPm::StayAwakeReason::EcallActive,
                "ecallmgr",
                [this](le_result_t result, int newId)
                {
                    if (result != LE_OK)
                    {
                        LE_ERROR("[%s] UpdateHoldPower(CbWinEndActive) failed rc=%d",
                                 LOG_NAME, result);
                        holdId_ = -1;
                        currentReason_ = ApiMngdPm::StayAwakeReason::None;
                        return;
                    }
                    holdId_ = newId;
                    LE_INFO("[%s] UpdateHoldPower(CbWinEndActive) ok newId=%d",
                            LOG_NAME, newId);
                });
        }
        else
        {
            LE_INFO("[%s] OnCallbackWindowEnd: call active, keeping wake lock id=%d",
                    LOG_NAME, holdId_);
        }
        return;
    }

    // Normal path: T9 expired, no active call — release the wake lock.
    if (holdId_ >= 0)
    {
        const int id = holdId_;
        holdId_        = -1;
        currentReason_ = ApiMngdPm::StayAwakeReason::None;

        ApiMngdPm::GetInstance().ReleasePowerAsync(
            id,
            [](le_result_t result, int relId)
            {
                if (result != LE_OK)
                    LE_ERROR("[%s] ReleasePower(CbWinEnd) failed rc=%d id=%d",
                             LOG_NAME, result, relId);
                else
                    LE_INFO("[%s] ReleasePower(CbWinEnd) ok id=%d",
                            LOG_NAME, relId);
            });
    }
}

// =============================================================================
// Internal policy
// =============================================================================

void EcallMgrPower::ApplyHoldPolicy(bool                       needHold,
                                    bool                       needRelease,
                                    ApiMngdPm::StayAwakeReason newReason)
{
    LE_INFO("[%s] ApplyHoldPolicy needHold=%d needRelease=%d "
            "holdId=%d curReason=%d newReason=%d cbWin=%d",
            LOG_NAME,
            needHold, needRelease,
            holdId_,
            static_cast<int>(currentReason_),
            static_cast<int>(newReason),
            inCallbackWindow_ ? 1 : 0);

    if (needHold)
    {
        if (holdId_ < 0)
        {
            // No active hold: acquire a new one.
            currentReason_ = newReason;
            ApiMngdPm::GetInstance().HoldPowerAsync(
                newReason,
                "ecallmgr",
                [this](le_result_t result, int id)
                {
                    if (result != LE_OK)
                    {
                        LE_ERROR("[%s] HoldPower(Apply) failed rc=%d",
                                 LOG_NAME, result);
                        currentReason_ = ApiMngdPm::StayAwakeReason::None;
                        onWakeupReady_ = nullptr; // discard pending GNSS start
                        return;
                    }
                    holdId_ = id;
                    LE_INFO("[%s] HoldPower(Apply) ok id=%d", LOG_NAME, id);

                    // PM wakeup complete � fire the one-shot GNSS start cb.
                    if (onWakeupReady_)
                    {
                        LE_INFO("[%s] HoldPower(Apply): firing onWakeupReady_",
                                LOG_NAME);
                        auto cb = std::move(onWakeupReady_);
                        onWakeupReady_ = nullptr;
                        cb();
                    }
                });
        }
                else if (newReason != currentReason_)
        {
            // Existing hold with different reason: update it.
            // UpdateHoldPowerReasonAsync does StayAwake(new) then
            // DeleteWakeupSource(old) without Relax — wsCount stays >= 1.
            const int oldId = holdId_;
            currentReason_  = newReason;
            ApiMngdPm::GetInstance().UpdateHoldPowerReasonAsync(
                oldId,
                newReason,
                "ecallmgr",
                [this](le_result_t result, int newId)
                {
                    if (result != LE_OK)
                    {
                        LE_ERROR("[%s] UpdateHoldPower(Apply) failed rc=%d",
                                 LOG_NAME, result);
                        holdId_ = -1;
                        currentReason_ = ApiMngdPm::StayAwakeReason::None;
                        onWakeupReady_ = nullptr;
                        return;
                    }
                    holdId_ = newId;
                    LE_INFO("[%s] UpdateHoldPower(Apply) ok newId=%d",
                            LOG_NAME, newId);

                    if (onWakeupReady_)
                    {
                        LE_INFO("[%s] UpdateHoldPower(Apply): firing onWakeupReady_",
                                LOG_NAME);
                        auto cb = std::move(onWakeupReady_);
                        onWakeupReady_ = nullptr;
                        cb();
                    }
                });
        }
        // else: same reason already held, nothing to do.
    }
    else if (needRelease)
    {
        // Don't release during callback window; T9 expiry handles that.
        if (!inCallbackWindow_ && holdId_ >= 0)
        {
            const int id = holdId_;
            holdId_        = -1;
            currentReason_ = ApiMngdPm::StayAwakeReason::None;
            ApiMngdPm::GetInstance().ReleasePowerAsync(
                id,
                [](le_result_t result, int relId)
                {
                    if (result != LE_OK)
                        LE_ERROR("[%s] ReleasePower(Apply) failed rc=%d id=%d",
                                 LOG_NAME, result, relId);
                    else
                        LE_INFO("[%s] ReleasePower(Apply) ok id=%d",
                                LOG_NAME, relId);
                });
        }
    }
}

} // namespace ecall

