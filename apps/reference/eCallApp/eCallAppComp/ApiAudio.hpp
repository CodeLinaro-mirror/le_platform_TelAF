/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <string>
#include <vector>

extern "C"
{
#include "legato.h"
#include "interfaces.h"    // taf_audio_* interfaces
}

namespace ecall
{

// -----------------------------------------------------------------------------
// ApiAudio
//
// eCall audio manager — owns the TelAF audio service connection and all
// audio handles for the duration of an eCall session.
//
// Design:
//   - Single worker thread with its own Legato RunLoop; all taf_audio_* calls
//     are marshalled onto that thread via RunOnWorkerSync().
//   - Exposes high-level, eCall-lifecycle-aware methods so that
//     EcallMgrEcall can drive audio directly without an intermediate layer.
//
// eCall audio lifecycle:
//
//   PromoteToVoiceCall(onceFile, loopFile)  ← MO Dialing / MT Incoming
//       Opens VOICE_CALL route, connects modem RX/TX, mutes voice,
//       and starts prompt playback:
//         - onceFile played once (e.g. announcement)
//         - loopFile played endlessly (e.g. IVS alert tone)
//       Either file may be nullptr/empty to skip.
//
//   BeginMsdTransmission()                  ← EcPhase::MsdStart / OutbandMsdStart
//       No audio change in the simplified flow.
//
//   EndMsdTransmission()                    ← EcPhase::MsdSuccess / EcPhase::MsdFail
//                                             / OutbandMsdSuccess / OutbandMsdFail
//       Stops prompt playback and unmutes modem RX/TX for
//       hands-free conversation.
//
//   TeardownAudio()                         ← Teardown phase
//       Disconnects and releases all handles.
//
// -----------------------------------------------------------------------------
class ApiAudio
{
public:
    // Singleton accessor.
    static ApiAudio& GetInstance();

        // -------------------------------------------------------------------------
    // eCall lifecycle methods (called by EcallMgrEcall::DispatchActions)
    // -------------------------------------------------------------------------

        // Open VOICE_CALL route, connect modem RX/TX, mute voice, and start
    // prompt playback:
    //   - onceFilePath  : played once (e.g. announcement); may be nullptr.
    //   - loopFilePath  : played endlessly (e.g. IVS alert tone); may be nullptr.
    // Called on MO eCall dialing or MT eCall incoming.
    le_result_t PromoteToVoiceCall(const char* onceFilePath,
                                   const char* loopFilePath);

    // Kept for API compatibility. In the simplified flow MSD start does not
    // change audio; only MSD result stops playback and unmutes voice.
    le_result_t BeginMsdTransmission();

    // Called on EcPhase::MsdSuccess / MsdFail / OutbandMsdSuccess / OutbandMsdFail.
    // Stops prompt playback and unmutes modem RX/TX for voice conversation.
    le_result_t EndMsdTransmission();

    // Disconnect and release all audio resources.
    // Must be called on Teardown phase.
    le_result_t TeardownAudio();

private:
    ApiAudio();
    ~ApiAudio();

    ApiAudio(const ApiAudio&)            = delete;
    ApiAudio& operator=(const ApiAudio&) = delete;

    // -------------------------------------------------------------------------
    // Internal worker-thread infrastructure
    // -------------------------------------------------------------------------
        enum class Op
    {
        PromoteToVoiceCall,
        BeginMsdTransmission,
        EndMsdTransmission,
        TeardownAudio,
        StopLoop
    };

        struct Req
    {
        Op            op{};
        ApiAudio*     self{nullptr};
        le_sem_Ref_t  sem{nullptr};
        le_result_t   result{LE_FAULT};

        // For PromoteToVoiceCall(onceFilePath, loopFilePath).
        char          onceFilePathBuf[256]{};
        std::size_t   onceFilePathLen{0};
        char          loopFilePathBuf[256]{};
        std::size_t   loopFilePathLen{0};
    };

    static void* WorkerThreadFn(void* ctx);
    void         WorkerInitOnThisThread();
    static void  DispatchOp(void* ctx, void* param2);
    static void  OnAudioServerDisconnected(void* ctx);
    static void  AudioReconnectTimerHandler(le_timer_Ref_t timerRef);

                le_result_t RunOnWorkerSync(Op op);
    le_result_t RunOnWorkerSync(Op op,
                                const char* onceFilePath,
                                const char* loopFilePath);


    // -------------------------------------------------------------------------
    // Worker-thread state (only accessed from worker thread)
    // -------------------------------------------------------------------------
    le_thread_Ref_t workerThreadRef_{nullptr};

    // taf_audio service non-exit disconnect/reconnect handling.
    le_timer_Ref_t audioReconnectTimer_{nullptr};
    uint32_t       audioRetryMs_{1000};
    bool           audioServiceConnected_{false};

    enum class RouteMode
    {
        None,
        LocalPlayback,
        VoiceCall
    };

        // Route handles.
    RouteMode                    routeMode_{RouteMode::None};
    taf_audio_RouteRef_t         routeRef_{nullptr};
    taf_audio_StreamRef_t        feOutRef_{nullptr};
    taf_audio_StreamRef_t        feInRef_{nullptr};

    // Prompt player chain.
    taf_audio_ConnectorRef_t     playerConnectorRef_{nullptr};
    taf_audio_StreamRef_t        playerRef_{nullptr};
    taf_audio_MediaHandlerRef_t  mediaHandlerRef_{nullptr};

    // Modem voice path.
    taf_audio_ConnectorRef_t     audioOutputConnectorRef_{nullptr};
    taf_audio_ConnectorRef_t     audioInputConnectorRef_{nullptr};
    taf_audio_StreamRef_t        mdmRxAudioRef_{nullptr};
    taf_audio_StreamRef_t        mdmTxAudioRef_{nullptr};

    // Session state flags.
    bool connected_{false};       // VOICE_CALL route is open (worker thread only)
    bool voiceMuted_{false};      // modem RX/TX currently muted (worker thread only)
    bool teardownInProgress_{false}; // resource release is running (worker thread only)

    // Updated by the worker thread and by MediaEventHandler().
    std::atomic_bool promptPlaying_{false};

    // Semaphore posted by MediaEventHandler on media events.
    // Stop/teardown waits on this after taf_audio_Stop() before releasing resources.
    le_sem_Ref_t stopSem_{nullptr};

    // Prompt file paths (worker thread only).
    // Both files are passed to one taf_audio_PlayFileList() call:
    //   onceFilePath_: repeat=0  (play once)
    //   loopFilePath_: repeat=-1 (loop forever)
    std::string                             onceFilePath_;
    std::string                             loopFilePath_;
    std::vector<taf_audio_PlayFileConfig_t> promptList_;


        // Internal helpers (worker thread).
    le_result_t DoSetVoiceMute(bool mute);   // sets mdmRx/Tx mute, updates voiceMuted_
    le_result_t DoStopPrompt();              // stops player and waits for media event
    le_result_t DoStartPrompt();             // builds playlist and starts playback
    void        RebuildPromptList();
    void        ClearRefsAfterAudioServiceDisconnect();
    void        StartAudioReconnectTimer();

    static void MediaEventHandler(taf_audio_StreamRef_t  playerRef,
                                  taf_audio_MediaEvent_t event,
                                  void*                  ctx);
};

} // namespace ecall


