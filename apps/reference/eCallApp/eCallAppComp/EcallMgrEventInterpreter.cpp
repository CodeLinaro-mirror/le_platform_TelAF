/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EcallMgrEventInterpreter.hpp"

extern "C"
{
#include "legato.h"
}

namespace ecall
{

// =============================================================================
// MapPhaseToPsapState
// =============================================================================

EcPowerSessionState MapPhaseToPsapState(EcPhase phase)
{
    switch (phase)
    {
        case EcPhase::Idle:                          return EcPowerSessionState::Idle;
        case EcPhase::Dialing:
        case EcPhase::Alerting:
        case EcPhase::Incoming:                      return EcPowerSessionState::Processing;
        case EcPhase::Active:                        return EcPowerSessionState::Connected;
        case EcPhase::Teardown:                      return EcPowerSessionState::Teardown;
        default:                                     return EcPowerSessionState::Unknown;
    }
}

// =============================================================================
// OnPhase
// =============================================================================

InterpretResult EcallMgrEventInterpreter::OnPhase(const EvPhase& phaseEv)
{
    InterpretResult r;
    const EcPhase   phase = phaseEv.phase;

    // -------------------------------------------------------------------------
    // 1. Audio side-effects
    // -------------------------------------------------------------------------
    switch (phase)
    {
        case EcPhase::Dialing:
            isMt_ = false;
            r.actions.push_back({EcActionKind::AudioPromoteToVoiceCall});
            break;

        case EcPhase::Incoming:
            isMt_ = true;
            r.actions.push_back({EcActionKind::AudioPromoteToVoiceCall});
            break;

        case EcPhase::Active:
            if (isMt_)
            {
                // MT call: no MSD phase → unmute voice immediately.
                r.actions.push_back({EcActionKind::AudioEndMsd});
            }
            break;

        case EcPhase::MsdStart:
        case EcPhase::OutbandMsdStart:
            // Keep audio unchanged. The simple eCall audio flow only starts
            // playback on Dialing/Incoming and stops it on MSD result/end.
            break;

        case EcPhase::MsdSuccess:
        case EcPhase::MsdFail:
            // Success or fail: stop prompt and unmute so operator can speak.
            r.actions.push_back({EcActionKind::AudioEndMsd});
            break;

        case EcPhase::OutbandMsdSuccess:
        case EcPhase::OutbandMsdFail:
            // Stop prompt and keep voice unmuted.
            r.actions.push_back({EcActionKind::AudioEndMsd});
            break;

        case EcPhase::Teardown:
            r.actions.push_back({EcActionKind::AudioTeardown});
            isMt_ = false;
            break;

        default:
            break;
    }

    // -------------------------------------------------------------------------
    // 2. Power-manager hint
    // -------------------------------------------------------------------------
    const EcPowerSessionState mapped = MapPhaseToPsapState(phase);
    if (mapped != EcPowerSessionState::Unknown)
        r.psapState = mapped;

    // -------------------------------------------------------------------------
    // 3. State-machine event
    // -------------------------------------------------------------------------
    switch (phase)
    {
        case EcPhase::Dialing:
        case EcPhase::Alerting:          r.ev = EcEvent::CallStarted;       break;
        case EcPhase::Incoming:          r.ev = EcEvent::CallIncoming;      break;
        case EcPhase::Active:            r.ev = EcEvent::CallConnected;     break;
        case EcPhase::MsdSuccess:        r.ev = EcEvent::MsdInbandSuccess;  break;
        case EcPhase::MsdFail:           r.ev = EcEvent::MsdInbandFailed;   break;
        case EcPhase::OutbandMsdFail:    r.ev = EcEvent::MsdOutbandFailed;  break;
        case EcPhase::OutbandMsdSuccess: r.ev = EcEvent::MsdOutbandSuccess; break;
        case EcPhase::Teardown:          r.ev = EcEvent::CallEnded;         break;
        default:                         break;
    }

    LE_DEBUG("Interpreter::OnPhase phase=%d ev=%d actions=%zu psapState=%d",
             static_cast<int>(phase),
             r.ev.has_value()        ? static_cast<int>(r.ev.value())        : -1,
             r.actions.size(),
             r.psapState.has_value() ? static_cast<int>(r.psapState.value()) : -1);

    return r;
}

// =============================================================================
// OnPsapSig
// =============================================================================

InterpretResult EcallMgrEventInterpreter::OnPsapSig(const EvPsap& psapEv)
{
    InterpretResult r;

    switch (psapEv.sig)
    {
        case PsapSig::MsdPullReq:
            // PSAP requests MSD retransmission.
            r.ev = EcEvent::MsdRequested;
            r.actions.push_back({EcActionKind::PsapExecuteSendMsd});
            break;

        case PsapSig::Start:
            // Simple audio flow: no change on START; wait for MSD success/fail.
            break;

        case PsapSig::AlAckClearDown:
            // PSAP requests call teardown. Audio resources are released when
            // the eCall stack reports the final Teardown/ENDED phase.
            r.ev = EcEvent::CallEnded;
            break;

        default:
            // LlAck, LlNack, AlAckPositive: no SM event or action needed here.
            break;
    }

    LE_DEBUG("Interpreter::OnPsapSig sig=%d ev=%d actions=%zu",
             static_cast<int>(psapEv.sig),
             r.ev.has_value() ? static_cast<int>(r.ev.value()) : -1,
             r.actions.size());

    return r;
}

// =============================================================================
// OnTimer
// =============================================================================

InterpretResult EcallMgrEventInterpreter::OnTimer(const EvTimer& timerEv)
{
    InterpretResult r;

    const EcTimerId     id     = timerEv.timerId;
    const EcTimerAction action = timerEv.action;

    switch (id)
    {
        case EcTimerId::T5:
        case EcTimerId::T6:
        case EcTimerId::T7:
            if (action == EcTimerAction::Expire)
                r.ev = EcEvent::PsapFailed;
            break;

        case EcTimerId::T9:
            if (action == EcTimerAction::Start ||
                action == EcTimerAction::Resume)
            {
                r.actions.push_back({EcActionKind::PowerCallbackWindowStart});
            }
            else if (action == EcTimerAction::Stop)
            {
                // Callback call answered: end wakelock window.
                r.actions.push_back({EcActionKind::PowerCallbackWindowEnd});
            }
            else if (action == EcTimerAction::Expire)
            {
                r.ev = EcEvent::CallbackWindowTimeout;
                r.actions.push_back({EcActionKind::PowerCallbackWindowEnd});
            }
            break;

        default:
            break;
    }

    LE_DEBUG("Interpreter::OnTimer id=%d action=%d ev=%d actions=%zu",
             static_cast<int>(id),
             static_cast<int>(action),
             r.ev.has_value() ? static_cast<int>(r.ev.value()) : -1,
             r.actions.size());

    return r;
}

} // namespace ecall
