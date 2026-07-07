/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ApiAudio.hpp"

#include <algorithm>
#include <new>
#include <cstring>

namespace ecall
{

#define LOG_NAME "ApiAudio"

// -----------------------------------------------------------------------------
// Singleton
// -----------------------------------------------------------------------------
ApiAudio& ApiAudio::GetInstance()
{
    static ApiAudio inst;
    return inst;
}

// -----------------------------------------------------------------------------
// Ctor / Dtor
// -----------------------------------------------------------------------------
ApiAudio::ApiAudio()
{
    stopSem_ = le_sem_Create("ApiAudioStopSem", 0);
    LE_ASSERT(stopSem_ != nullptr);

    workerThreadRef_ = le_thread_Create("ApiAudioWorker",
                                        &ApiAudio::WorkerThreadFn,
                                        this);
    LE_ASSERT(workerThreadRef_ != nullptr);
    le_thread_SetJoinable(workerThreadRef_);
    le_thread_Start(workerThreadRef_);
    LE_INFO("[%s] worker thread started", LOG_NAME);
}

ApiAudio::~ApiAudio()
{
    (void)RunOnWorkerSync(Op::StopLoop);
    if (workerThreadRef_)
    {
        le_thread_Join(workerThreadRef_, nullptr);
        workerThreadRef_ = nullptr;
    }
    if (stopSem_)
    {
        le_sem_Delete(stopSem_);
        stopSem_ = nullptr;
    }
}

// -----------------------------------------------------------------------------
// Worker thread
// -----------------------------------------------------------------------------
void* ApiAudio::WorkerThreadFn(void* ctx)
{
    auto* self = static_cast<ApiAudio*>(ctx);
    if (!self) return nullptr;
    self->WorkerInitOnThisThread();
    le_event_RunLoop();
    LE_INFO("[%s] RunLoop exited", LOG_NAME);
    return nullptr;
}

void ApiAudio::WorkerInitOnThisThread()
{
    le_result_t rc = taf_audio_TryConnectService();
    if (rc == LE_OK)
    {
        audioServiceConnected_ = true;
        taf_audio_SetNonExitServerDisconnectHandler(&ApiAudio::OnAudioServerDisconnected, this);
        LE_INFO("[%s] taf_audio connected; non-exit disconnect handler registered", LOG_NAME);
        return;
    }

    audioServiceConnected_ = false;
    LE_WARN("[%s] initial taf_audio TryConnectService failed rc=%d; start reconnect timer",
            LOG_NAME, rc);
    StartAudioReconnectTimer();
}

// -----------------------------------------------------------------------------
// Internal worker-thread helpers
// -----------------------------------------------------------------------------

// Set mute on both modem RX and TX; update voiceMuted_ flag.
le_result_t ApiAudio::DoSetVoiceMute(bool mute)
{
    if (voiceMuted_ == mute)
    {
        LE_INFO("[%s] voice already %s", LOG_NAME, mute ? "MUTED" : "UNMUTED");
        return LE_OK;
    }

    le_result_t rc = LE_OK;

    if (mdmRxAudioRef_)
    {
        le_result_t rr = taf_audio_SetMute(mdmRxAudioRef_, mute);
        if (rr != LE_OK)
        {
            LE_ERROR("[%s] SetMute(mdmRx, %d) failed rc=%d", LOG_NAME, mute, rr);
            rc = rr;
        }
    }
    if (mdmTxAudioRef_)
    {
        le_result_t rr = taf_audio_SetMute(mdmTxAudioRef_, mute);
        if (rr != LE_OK)
        {
            LE_ERROR("[%s] SetMute(mdmTx, %d) failed rc=%d", LOG_NAME, mute, rr);
            rc = rr;
        }
    }
    if (rc == LE_OK)
    {
        voiceMuted_ = mute;
        LE_INFO("[%s] voice %s", LOG_NAME, mute ? "MUTED" : "UNMUTED");
    }
    return rc;
}

// Stop prompt player and wait for MEDIA_STOPPED/MEDIA_ERROR confirmation.
// Follow tafAudioConsoleApp order:
//   taf_audio_Stop() -> le_sem_WaitWithTimeOut() -> Disconnect/Close later.
le_result_t ApiAudio::DoStopPrompt()
{
    if (!playerRef_)
    {
        promptPlaying_.store(false);
        return LE_OK;
    }

    if (!promptPlaying_.load())
    {
        // Do not return here. After MEDIA_ERROR the local flag may already be
        // false/stale, but tafAudioSvc can still require Stop() to clear the
        // player state before the next playback or before Close().
        LE_INFO("[%s] DoStopPrompt: prompt flag is false; still issue Stop to clear player", LOG_NAME);
    }

    le_result_t rc = taf_audio_Stop(playerRef_);
    if (rc != LE_OK)
    {
        LE_INFO("[%s] taf_audio_Stop(player) rc=%d (already stopped or not active)",
                LOG_NAME, rc);
        promptPlaying_.store(false);
        return LE_OK;
    }

    le_clk_Time_t timeout{2, 0};
    le_result_t waitRc = le_sem_WaitWithTimeOut(stopSem_, timeout);
    if (waitRc == LE_TIMEOUT)
        LE_WARN("[%s] DoStopPrompt: timed out waiting for media stop event", LOG_NAME);
    else
        LE_INFO("[%s] DoStopPrompt: media stop event received", LOG_NAME);

    promptPlaying_.store(false);
    return LE_OK;
}

void ApiAudio::RebuildPromptList()
{
    promptList_.clear();

    // PlayFileList supports multiple entries.  Pass both files in one call:
    //   entry 0: onceFilePath_ repeat=0
    //   entry 1: loopFilePath_ repeat=-1
    // tafAudioSvc handles sequential playback internally.
    if (!onceFilePath_.empty())
    {
        taf_audio_PlayFileConfig_t entry{};
        std::snprintf(entry.srcPath, sizeof(entry.srcPath), "%s", onceFilePath_.c_str());
        entry.repeat = 0;
        promptList_.push_back(entry);
    }

    if (!loopFilePath_.empty())
    {
        taf_audio_PlayFileConfig_t entry{};
        std::snprintf(entry.srcPath, sizeof(entry.srcPath), "%s", loopFilePath_.c_str());
        entry.repeat = -1; // endless loop
        promptList_.push_back(entry);
    }
}

// Start prompt playlist on existing player with one PlayFileList call.
// Do not wait here.  The worker thread must remain available for
// EndMsdTransmission / TeardownAudio.
le_result_t ApiAudio::DoStartPrompt()
{
    if (!audioServiceConnected_)
    {
        LE_WARN("[%s] DoStartPrompt: taf_audio service not connected", LOG_NAME);
        promptPlaying_.store(false);
        return LE_UNAVAILABLE;
    }

    if (promptPlaying_.load())
    {
        LE_INFO("[%s] DoStartPrompt: prompt already playing, skip duplicate PlayFileList", LOG_NAME);
        return LE_OK;
    }

    if (!playerRef_ || (onceFilePath_.empty() && loopFilePath_.empty()))
    {
        LE_DEBUG("[%s] DoStartPrompt: no player or no file, skip", LOG_NAME);
        return LE_OK;
    }

    RebuildPromptList();
    if (promptList_.empty())
    {
        LE_WARN("[%s] DoStartPrompt: prompt list empty", LOG_NAME);
        return LE_FAULT;
    }

    le_result_t rc = taf_audio_PlayFileList(playerRef_,
                                            promptList_.data(),
                                            promptList_.size());
    if (rc != LE_OK)
    {
        LE_ERROR("[%s] PlayFileList failed rc=%d (once='%s' loop='%s')",
                 LOG_NAME, rc, onceFilePath_.c_str(), loopFilePath_.c_str());
        promptPlaying_.store(false);
        return rc;
    }

    promptPlaying_.store(true);
    LE_INFO("[%s] prompt started: once='%s' loop='%s' listSize=%zu",
            LOG_NAME, onceFilePath_.c_str(), loopFilePath_.c_str(), promptList_.size());
    return LE_OK;
}

void ApiAudio::ClearRefsAfterAudioServiceDisconnect()
{
    // The server side disappeared, so local taf_audio refs are no longer valid.
    // Do not call Disconnect/Close on these stale refs; just forget them and let
    // the reconnect path create a fresh route/player chain.
    mediaHandlerRef_ = nullptr;
    audioInputConnectorRef_ = nullptr;
    audioOutputConnectorRef_ = nullptr;
    playerConnectorRef_ = nullptr;
    mdmRxAudioRef_ = nullptr;
    mdmTxAudioRef_ = nullptr;
    playerRef_ = nullptr;
    routeRef_ = nullptr;
    feOutRef_ = nullptr;
    feInRef_ = nullptr;
    routeMode_ = RouteMode::None;
    connected_ = false;
    voiceMuted_ = false;
    teardownInProgress_ = false;
    promptPlaying_.store(false);
    promptList_.clear();
}

void ApiAudio::StartAudioReconnectTimer()
{
    if (audioReconnectTimer_)
    {
        return;
    }

    audioRetryMs_ = 1000;
    audioReconnectTimer_ = le_timer_Create("ApiAudioReconnectTimer");
    if (!audioReconnectTimer_)
    {
        LE_ERROR("[%s] failed to create audio reconnect timer", LOG_NAME);
        return;
    }

    le_clk_Time_t interval{};
    interval.sec = audioRetryMs_ / 1000U;
    interval.usec = static_cast<long>((audioRetryMs_ % 1000U) * 1000U);
    le_timer_SetInterval(audioReconnectTimer_, interval);
    le_timer_SetHandler(audioReconnectTimer_, &ApiAudio::AudioReconnectTimerHandler);
    le_timer_SetContextPtr(audioReconnectTimer_, this);
    le_result_t rc = le_timer_Start(audioReconnectTimer_);
    if (rc != LE_OK)
    {
        LE_ERROR("[%s] failed to start audio reconnect timer rc=%d", LOG_NAME, rc);
        le_timer_Delete(audioReconnectTimer_);
        audioReconnectTimer_ = nullptr;
    }
}

void ApiAudio::OnAudioServerDisconnected(void* ctx)
{
    auto* self = static_cast<ApiAudio*>(ctx);
    if (!self) return;

    LE_WARN("[%s] taf_audio server disconnected (non-exit); starting reconnect timer", LOG_NAME);
    self->audioServiceConnected_ = false;
    self->ClearRefsAfterAudioServiceDisconnect();
    self->StartAudioReconnectTimer();
}

void ApiAudio::AudioReconnectTimerHandler(le_timer_Ref_t timerRef)
{
    auto* self = static_cast<ApiAudio*>(le_timer_GetContextPtr(timerRef));
    if (!self) return;

    le_result_t rc = taf_audio_TryConnectService();
    if (rc == LE_OK)
    {
        LE_WARN("[%s] taf_audio reconnected", LOG_NAME);
        self->audioServiceConnected_ = true;
        taf_audio_SetNonExitServerDisconnectHandler(&ApiAudio::OnAudioServerDisconnected, self);

        le_timer_Stop(timerRef);
        le_timer_Delete(timerRef);
        self->audioReconnectTimer_ = nullptr;
        self->audioRetryMs_ = 1000;
        return;
    }

    self->audioRetryMs_ = (self->audioRetryMs_ < 30000U) ? (self->audioRetryMs_ * 2U) : 30000U;
    le_clk_Time_t interval{};
    interval.sec = self->audioRetryMs_ / 1000U;
    interval.usec = static_cast<long>((self->audioRetryMs_ % 1000U) * 1000U);
    le_timer_SetInterval(timerRef, interval);
    le_timer_Start(timerRef);
}

// -----------------------------------------------------------------------------
// DispatchOp  (runs on worker thread)
// -----------------------------------------------------------------------------
void ApiAudio::DispatchOp(void* ctx, void* /*param2*/)
{
    auto* req = static_cast<Req*>(ctx);
    if (!req || !req->self)
        return;
    auto* self = req->self;
    le_result_t r = LE_OK;

    switch (req->op)
    {
        // -----------------------------------------------------------------
        // PromoteToVoiceCall
        //   Opens VOICE_CALL route, connects modem RX/TX, mutes voice,
        //   and starts prompt playback as IVS alert tone.
        //   Called on MO eCall dialing or MT eCall incoming.
        // -----------------------------------------------------------------
        case Op::PromoteToVoiceCall:
        {
            LE_INFO("[%s][worker] PromoteToVoiceCall", LOG_NAME);

            if (!self->audioServiceConnected_)
            {
                LE_WARN("[%s] PromoteToVoiceCall: taf_audio service not connected", LOG_NAME);
                r = LE_UNAVAILABLE;
                break;
            }

            // Store prompt file paths even if the voice route is already open.
            if (req->onceFilePathLen > 0)
                self->onceFilePath_.assign(req->onceFilePathBuf, req->onceFilePathLen);
            else
                self->onceFilePath_.clear();

            if (req->loopFilePathLen > 0)
                self->loopFilePath_.assign(req->loopFilePathBuf, req->loopFilePathLen);
            else
                self->loopFilePath_.clear();

            self->RebuildPromptList();

            if (self->connected_)
            {
                LE_INFO("[%s] PromoteToVoiceCall: already connected; ensure mute + prompt only", LOG_NAME);
                (void)self->DoSetVoiceMute(true);
                r = self->DoStartPrompt();
                break;
            }

            if (self->teardownInProgress_)
            {
                LE_WARN("[%s] PromoteToVoiceCall: teardown in progress, skip", LOG_NAME);
                r = LE_BUSY;
                break;
            }

            // --- Open VOICE_CALL route ---
            self->routeRef_ = taf_audio_OpenRoute(TAF_AUDIO_ROUTE_1,
                                                  TAF_AUDIO_VOICE_CALL,
                                                  &self->feOutRef_,
                                                  &self->feInRef_);
            if (!self->routeRef_)
            {
                LE_ERROR("[%s] PromoteToVoiceCall: OpenRoute(VOICE_CALL) failed", LOG_NAME);
                r = LE_FAULT;
                break;
            }
            self->routeMode_ = RouteMode::VoiceCall;

            // --- Prompt player chain: playerConnector <- feOut + player ---
            self->playerConnectorRef_ = taf_audio_CreateConnector();
            if (!self->playerConnectorRef_)
            {
                LE_ERROR("[%s] PromoteToVoiceCall: CreateConnector(player) failed", LOG_NAME);
                r = LE_FAULT;
                break;
            }

            self->playerRef_ = taf_audio_OpenPlayer(TAF_AUDIO_RX);
            if (!self->playerRef_)
            {
                LE_ERROR("[%s] PromoteToVoiceCall: OpenPlayer failed", LOG_NAME);
                r = LE_FAULT;
                break;
            }

            self->mediaHandlerRef_ = taf_audio_AddMediaHandler(
                self->playerRef_, &ApiAudio::MediaEventHandler, self);

            r = taf_audio_Connect(self->playerConnectorRef_, self->feOutRef_);
            if (r != LE_OK) { LE_ERROR("[%s] PromoteToVoiceCall: Connect(playerConn,feOut) rc=%d", LOG_NAME, r); break; }
            r = taf_audio_Connect(self->playerConnectorRef_, self->playerRef_);
            if (r != LE_OK) { LE_ERROR("[%s] PromoteToVoiceCall: Connect(playerConn,player) rc=%d", LOG_NAME, r); break; }
            (void)taf_audio_SetVolume(self->playerRef_, 1);

            // --- Modem RX path ---
            self->mdmRxAudioRef_ = taf_audio_OpenModemVoiceRx(1);
            if (!self->mdmRxAudioRef_)
            {
                LE_ERROR("[%s] PromoteToVoiceCall: OpenModemVoiceRx failed", LOG_NAME);
                r = LE_FAULT;
                break;
            }

            self->audioOutputConnectorRef_ = taf_audio_CreateConnector();
            if (!self->audioOutputConnectorRef_)
            {
                LE_ERROR("[%s] PromoteToVoiceCall: CreateConnector(output) failed", LOG_NAME);
                r = LE_FAULT;
                break;
            }
            r = taf_audio_Connect(self->audioOutputConnectorRef_, self->feOutRef_);
            if (r != LE_OK) { LE_ERROR("[%s] PromoteToVoiceCall: Connect(outConn,feOut) rc=%d", LOG_NAME, r); break; }
            r = taf_audio_Connect(self->audioOutputConnectorRef_, self->mdmRxAudioRef_);
            if (r != LE_OK) { LE_ERROR("[%s] PromoteToVoiceCall: Connect(outConn,mdmRx) rc=%d", LOG_NAME, r); break; }

            // --- Modem TX path ---
            self->mdmTxAudioRef_ = taf_audio_OpenModemVoiceTx(1, false);
            if (!self->mdmTxAudioRef_)
            {
                LE_ERROR("[%s] PromoteToVoiceCall: OpenModemVoiceTx failed", LOG_NAME);
                r = LE_FAULT;
                break;
            }

            self->audioInputConnectorRef_ = taf_audio_CreateConnector();
            if (!self->audioInputConnectorRef_)
            {
                LE_ERROR("[%s] PromoteToVoiceCall: CreateConnector(input) failed", LOG_NAME);
                r = LE_FAULT;
                break;
            }
            r = taf_audio_Connect(self->audioInputConnectorRef_, self->feInRef_);
            if (r != LE_OK) { LE_ERROR("[%s] PromoteToVoiceCall: Connect(inConn,feIn) rc=%d", LOG_NAME, r); break; }
            r = taf_audio_Connect(self->audioInputConnectorRef_, self->mdmTxAudioRef_);
            if (r != LE_OK) { LE_ERROR("[%s] PromoteToVoiceCall: Connect(inConn,mdmTx) rc=%d", LOG_NAME, r); break; }

            (void)taf_audio_SetVolume(self->mdmRxAudioRef_, 0.5);

            // Initial MO dialing / MT incoming prompt: mute voice and play alert tone.
            (void)self->DoSetVoiceMute(true);
            r = self->DoStartPrompt();
            if (r != LE_OK) break;

            self->connected_ = true;
            LE_INFO("[%s] PromoteToVoiceCall OK (once=%s loop=%s)",
                    LOG_NAME, self->onceFilePath_.c_str(), self->loopFilePath_.c_str());
            r = LE_OK;
            break;
        }


        // -----------------------------------------------------------------
        // BeginMsdTransmission
        //   Kept for API compatibility. The simplified eCall audio flow does
        //   not change audio on MSD start; playback is stopped and voice is
        //   unmuted only on MSD success/fail.
        // -----------------------------------------------------------------
        case Op::BeginMsdTransmission:
        {
            LE_INFO("[%s][worker] BeginMsdTransmission: no audio change", LOG_NAME);
            r = LE_OK;
            break;
        }

        // -----------------------------------------------------------------
        // EndMsdTransmission
        //   Called on EcPhase::MsdSuccess or EcPhase::MsdFail.
        //   Stops prompt playback and unmutes modem RX/TX for
        //   hands-free conversation.
        // -----------------------------------------------------------------
        case Op::EndMsdTransmission:
        {
            LE_INFO("[%s][worker] EndMsdTransmission", LOG_NAME);

            if (!self->audioServiceConnected_)
            {
                LE_WARN("[%s] EndMsdTransmission: taf_audio service not connected", LOG_NAME);
                r = LE_OK;
                break;
            }

            if (!self->connected_)
            {
                LE_WARN("[%s] EndMsdTransmission: not connected, skip", LOG_NAME);
                r = LE_OK;
                break;
            }
            // Stop prompt first, then unmute voice.
            (void)self->DoStopPrompt();
            r = self->DoSetVoiceMute(false);
            break;
        }

        // -----------------------------------------------------------------
        // TeardownAudio
        //   Disconnect and release all handles.
        // -----------------------------------------------------------------
        case Op::TeardownAudio:
        {
            LE_INFO("[%s][worker] TeardownAudio", LOG_NAME);

            if (!self->audioServiceConnected_)
            {
                LE_WARN("[%s] TeardownAudio: taf_audio service not connected; clearing local refs", LOG_NAME);
                self->ClearRefsAfterAudioServiceDisconnect();
                r = LE_OK;
                break;
            }

            if (self->teardownInProgress_)
            {
                LE_INFO("[%s] TeardownAudio: already in progress, skip duplicate", LOG_NAME);
                r = LE_OK;
                break;
            }
            if (!self->connected_ && !self->routeRef_ && !self->playerRef_)
            {
                LE_INFO("[%s] TeardownAudio: already released", LOG_NAME);
                r = LE_OK;
                break;
            }

            self->teardownInProgress_ = true;

            // Always restore voice before releasing streams.
            (void)self->DoSetVoiceMute(false);

            // Stop player and wait for MEDIA_STOPPED/MEDIA_ERROR before releasing resources.
            (void)self->DoStopPrompt();
            // Disconnect input connector.
            if (self->audioInputConnectorRef_)
            {
                if (self->feInRef_)
                    taf_audio_Disconnect(self->audioInputConnectorRef_, self->feInRef_);
                if (self->mdmTxAudioRef_)
                    taf_audio_Disconnect(self->audioInputConnectorRef_, self->mdmTxAudioRef_);
                taf_audio_DeleteConnector(self->audioInputConnectorRef_);
                self->audioInputConnectorRef_ = nullptr;
            }

            // Disconnect output connector.
            if (self->audioOutputConnectorRef_)
            {
                if (self->feOutRef_)
                    taf_audio_Disconnect(self->audioOutputConnectorRef_, self->feOutRef_);
                if (self->mdmRxAudioRef_)
                    taf_audio_Disconnect(self->audioOutputConnectorRef_, self->mdmRxAudioRef_);
                taf_audio_DeleteConnector(self->audioOutputConnectorRef_);
                self->audioOutputConnectorRef_ = nullptr;
            }

            // Disconnect player connector.
            if (self->playerConnectorRef_)
            {
                if (self->playerRef_)
                    taf_audio_Disconnect(self->playerConnectorRef_, self->playerRef_);
                if (self->feOutRef_)
                    taf_audio_Disconnect(self->playerConnectorRef_, self->feOutRef_);
                taf_audio_DeleteConnector(self->playerConnectorRef_);
                self->playerConnectorRef_ = nullptr;
            }

            // Close streams.
            if (self->mdmRxAudioRef_)
            {
                taf_audio_Close(self->mdmRxAudioRef_);
                self->mdmRxAudioRef_ = nullptr;
            }
            if (self->mdmTxAudioRef_)
            {
                taf_audio_Close(self->mdmTxAudioRef_);
                self->mdmTxAudioRef_ = nullptr;
            }
            if (self->playerRef_)
            {
                taf_audio_Close(self->playerRef_);
                self->playerRef_ = nullptr;
            }

            // Close route (releases feOutRef_ / feInRef_).
            if (self->routeRef_)
            {
                le_result_t rr = taf_audio_CloseRoute(self->routeRef_);
                LE_ERROR_IF((rr != LE_OK),
                            "[%s] CloseRoute failed rc=%d", LOG_NAME, rr);
                self->routeRef_ = nullptr;
            }

            // Deregister media callback last, after Stop wait + Disconnect + Close + CloseRoute.
            if (self->mediaHandlerRef_)
            {
                taf_audio_RemoveMediaHandler(self->mediaHandlerRef_);
                self->mediaHandlerRef_ = nullptr;
            }

            self->feOutRef_       = nullptr;
            self->feInRef_        = nullptr;
            self->routeMode_      = RouteMode::None;
            self->connected_  = false;
            self->voiceMuted_ = false;
            self->teardownInProgress_ = false;
            self->promptPlaying_.store(false);
            self->onceFilePath_.clear();
            self->loopFilePath_.clear();
            self->promptList_.clear();

            LE_INFO("[%s] TeardownAudio OK", LOG_NAME);
            r = LE_OK;
            break;
        }

        case Op::StopLoop:
        {
            LE_INFO("[%s][worker] StopLoop", LOG_NAME);
            if (self->audioReconnectTimer_)
            {
                le_timer_Stop(self->audioReconnectTimer_);
                le_timer_Delete(self->audioReconnectTimer_);
                self->audioReconnectTimer_ = nullptr;
            }
            if (self->mediaHandlerRef_ && self->audioServiceConnected_)
            {
                taf_audio_RemoveMediaHandler(self->mediaHandlerRef_);
                self->mediaHandlerRef_ = nullptr;
            }
            r = LE_OK;
            req->result = r;
            if (req->sem)
                le_sem_Post(req->sem);
            le_thread_Exit(nullptr);
            return;
        }

        default:
            LE_ERROR("[%s][worker] Unknown op=%d", LOG_NAME, static_cast<int>(req->op));
            r = LE_FAULT;
            break;
    }

    req->result = r;
    if (req->sem)
        le_sem_Post(req->sem);
}

// -----------------------------------------------------------------------------
// RunOnWorkerSync helpers
// -----------------------------------------------------------------------------
le_result_t ApiAudio::RunOnWorkerSync(Op op)
{
    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;

    req->op   = op;
    req->self = this;
    req->sem  = le_sem_Create("ApiAudioReqSem", 0);

    le_event_QueueFunctionToThread(workerThreadRef_, &ApiAudio::DispatchOp, req, nullptr);
    le_sem_Wait(req->sem);

    le_result_t rc = req->result;
    le_sem_Delete(req->sem);
    delete req;
    return rc;
}

le_result_t ApiAudio::RunOnWorkerSync(Op op,
                                      const char* onceFilePath,
                                      const char* loopFilePath)
{
    Req* req = new (std::nothrow) Req;
    if (!req) return LE_NO_MEMORY;

    req->op   = op;
    req->self = this;
    req->sem  = le_sem_Create("ApiAudioReqSem", 0);

    auto copyPath = [](const char* src, char* dst, std::size_t dstSize, std::size_t& outLen)
    {
        if (src && src[0] != '\0')
        {
            outLen = std::min<std::size_t>(std::strlen(src), dstSize - 1);
            std::memcpy(dst, src, outLen);
            dst[outLen] = '\0';
        }
        else
        {
            outLen = 0;
        }
    };

    copyPath(onceFilePath, req->onceFilePathBuf, sizeof(req->onceFilePathBuf), req->onceFilePathLen);
    copyPath(loopFilePath, req->loopFilePathBuf, sizeof(req->loopFilePathBuf), req->loopFilePathLen);

    le_event_QueueFunctionToThread(workerThreadRef_, &ApiAudio::DispatchOp, req, nullptr);
    le_sem_Wait(req->sem);

    le_result_t rc = req->result;
    le_sem_Delete(req->sem);
    delete req;
    return rc;
}

// -----------------------------------------------------------------------------
// Public methods
// -----------------------------------------------------------------------------
le_result_t ApiAudio::PromoteToVoiceCall(const char* onceFilePath,
                                         const char* loopFilePath)
{
    return RunOnWorkerSync(Op::PromoteToVoiceCall,
                           onceFilePath ? onceFilePath : "",
                           loopFilePath ? loopFilePath : "");
}

le_result_t ApiAudio::BeginMsdTransmission()
{
    return RunOnWorkerSync(Op::BeginMsdTransmission);
}

le_result_t ApiAudio::EndMsdTransmission()
{
    return RunOnWorkerSync(Op::EndMsdTransmission);
}

le_result_t ApiAudio::TeardownAudio()
{
    return RunOnWorkerSync(Op::TeardownAudio);
}

// -----------------------------------------------------------------------------
// Media event handler
// -----------------------------------------------------------------------------
void ApiAudio::MediaEventHandler(taf_audio_StreamRef_t  /*playerRef*/,
                                 taf_audio_MediaEvent_t event,
                                 void*                  ctx)
{
    auto* self = static_cast<ApiAudio*>(ctx);
    if (!self) return;

    switch (event)
    {
        case TAF_AUDIO_MEDIA_ENDED:
            LE_INFO("[%s] MEDIA_ENDED", LOG_NAME);
            self->promptPlaying_.store(false);
            break;
        case TAF_AUDIO_MEDIA_STOPPED:
            LE_INFO("[%s] MEDIA_STOPPED", LOG_NAME);
            self->promptPlaying_.store(false);
            break;
        case TAF_AUDIO_MEDIA_ERROR:
            LE_ERROR("[%s] MEDIA_ERROR", LOG_NAME);
            // Keep promptPlaying_ true so EndMsd/Teardown will still call
            // taf_audio_Stop() to clear tafAudioSvc's player error state.
            break;
        default: LE_INFO("[%s] MEDIA_EVENT=%d", LOG_NAME, static_cast<int>(event)); break;
    }

    // Always post — Stop/teardown waits for this before releasing resources.
    if (self->stopSem_)
        le_sem_Post(self->stopSem_);
}

} // namespace ecall
