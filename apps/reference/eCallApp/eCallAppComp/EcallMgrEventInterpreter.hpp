/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <vector>
#include <optional>

#include "EcallDefs.hpp"

namespace ecall
{

// -----------------------------------------------------------------------------
// EcActionKind / EcAction
//
// Side-effect tokens emitted by EcallMgrEventInterpreter and executed by
// EcallMgrEcall::DispatchActions().
//
// Audio lifecycle (CS in-band MSD path):
//   Dialing/Incoming        -> AudioPromoteToVoiceCall  (open VOICE_CALL route, mute voice, start prompt)
//   MsdStart/OutbandStart   -> no audio action
//   MsdSuccess/Fail         -> AudioEndMsd              (stop prompt, unmute voice)
//   OutbandMsdSuccess/Fail  -> AudioEndMsd              (stop prompt, unmute voice)
//   Teardown                -> AudioTeardown            (release all audio resources)
// -----------------------------------------------------------------------------

enum class EcActionKind
{
    None = 0,

        // Audio path (maps 1-to-1 to ApiAudio methods).
    AudioPromoteToVoiceCall,   // PromoteToVoiceCall(prompt): open VOICE_CALL, mute, start prompt
    AudioBeginMsd,             // Reserved/no-op in the simplified flow
    AudioEndMsd,               // EndMsdTransmission(): stop prompt + unmute
    AudioTeardown,             // TeardownAudio()

    // Power manager.
    PowerCallbackWindowStart,
    PowerCallbackWindowEnd,

    // MSD retransmission triggered by PSAP pull request.
    PsapExecuteSendMsd,
};

struct EcAction
{
    EcActionKind kind{EcActionKind::None};
};

// Map an eCall phase to the power-manager session state.
// Returns EcPowerSessionState::Unknown for phases that carry no power hint.
EcPowerSessionState MapPhaseToPsapState(EcPhase phase);

// -----------------------------------------------------------------------------
// InterpretResult
//
// Value type returned by every EcallMgrEventInterpreter::On*() method.
// Replaces the previous triple-output-parameter style.
// -----------------------------------------------------------------------------
struct InterpretResult
{
    std::optional<EcEvent>             ev;        // SM event to feed into HandleEcEvent()
    std::vector<EcAction>              actions;   // side-effects to execute
    std::optional<EcPowerSessionState> psapState; // power-manager hint (phase/timer only)
};

// -----------------------------------------------------------------------------
// EcallMgrEventInterpreter
//
// Pure translator: converts raw bus events (phase / PSAP signal / timer) into
// an InterpretResult that EcallMgrEcall can act on without knowing the
// translation rules.
//
// Design:
//   - Not a singleton.  EcallMgrEcall owns one instance as a member.
//   - The only mutable state is isMt_ (1 bit), which tracks whether the
//     current session is mobile-terminated so that the Active phase can
//     decide whether to unmute voice immediately.
//   - All On*() methods are pure in the sense that their output depends only
//     on the input event and isMt_; they have no I/O or IPC side-effects.
//     This makes them straightforward to unit-test.
// -----------------------------------------------------------------------------
class EcallMgrEventInterpreter
{
public:
    EcallMgrEventInterpreter()  = default;
    ~EcallMgrEventInterpreter() = default;

    // Non-copyable (owned by EcallMgrEcall).
    EcallMgrEventInterpreter(const EcallMgrEventInterpreter&)            = delete;
    EcallMgrEventInterpreter& operator=(const EcallMgrEventInterpreter&) = delete;

    InterpretResult OnPhase  (const EvPhase&  ev);
    InterpretResult OnPsapSig(const EvPsap&   ev);
    InterpretResult OnTimer  (const EvTimer&  ev);

    // Reset session state (call at the start of each new eCall session).
    void Reset() { isMt_ = false; }

private:
    // true when the current session is mobile-terminated (incoming callback).
    bool isMt_{false};
};

} // namespace ecall


