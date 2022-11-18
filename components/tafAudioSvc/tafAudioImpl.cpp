/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <telux/audio/AudioFactory.hpp>
#include <telux/audio/AudioManager.hpp>
#include "tafAudio.hpp"
#include <unistd.h>
#include <future>

using namespace telux::common;
using namespace telux::audio;
using namespace telux::tafsvc;
using namespace std;


LE_MEM_DEFINE_STATIC_POOL(tafAudio,MAX_STREAM,sizeof(taf_audio_Stream_t));
LE_MEM_DEFINE_STATIC_POOL(tafAudioConnector,MAX_CONNECTOR,sizeof(taf_audio_Connector_t));
LE_MEM_DEFINE_STATIC_POOL(tafAudioHashmap,HASHMAP_SIZE,sizeof(struct hashMapList));
LE_MEM_DEFINE_STATIC_POOL(tafSessionRef,MAX_STREAM,sizeof(taf_SessionRef_t));
LE_MEM_DEFINE_STATIC_POOL(tafEventIdPool,MAX_STREAM,sizeof(struct tafEventIdList));
LE_MEM_DEFINE_STATIC_POOL(tafEventHandlerRef,MAX_CONNECTOR,sizeof(EventHandlerRefNode_t));

#define DEVICE_TYPE_HEADSET_SPEAKER 3

static taf_audio_StreamRef_t DtmfAudioRef = NULL;
// Resets the global callback promise variable
static inline void resetCallbackPromise(void) {
    auto &audio = taf_Audio::GetInstance();
    audio.gCallbackPromise = promise<ErrorCode>();
}

void taf_Audio::StartAudioCallback(ErrorCode error)
{
    auto &audio = taf_Audio::GetInstance();
    if (ErrorCode::SUCCESS == error) {
        if (audio.mSlotId == SLOT_ID_1) {
            audio.mVoiceEnabled1 = true;
        } else  if (audio.mSlotId == SLOT_ID_2) {
            audio.mVoiceEnabled2 = true;
        }
        LE_DEBUG("audio started successfully");
    }
    audio.gCallbackPromise.set_value(error);
    return;
}

void taf_Audio::DeleteVoiceCallback(ErrorCode error) {
    auto &audio = taf_Audio::GetInstance();
    if (ErrorCode::SUCCESS == error) {
        LE_DEBUG("deleteStream() succeeded.");
        audio.mAudioVoiceStream.reset();
        audio.mAudioVoiceStream = nullptr;
    }
    audio.gCallbackPromise.set_value(error);
    return;
}

void taf_Audio::DeleteVoiceCallback2(ErrorCode error) {
    auto &audio = taf_Audio::GetInstance();
    if (ErrorCode::SUCCESS == error) {
        LE_DEBUG("deleteStream() succeeded.");
        audio.mAudioVoiceStream2.reset();
        audio.mAudioVoiceStream2 = nullptr;
    }
    audio.gCallbackPromise.set_value(error);
    return;
}

void taf_Audio::StopAudioCallback(ErrorCode error)
{
    auto &audio = taf_Audio::GetInstance();
    if (ErrorCode::SUCCESS == error) {
        if (audio.mSlotId == SLOT_ID_1) {
            audio.mVoiceEnabled1 = false;
        } else  if (audio.mSlotId == SLOT_ID_2) {
            audio.mVoiceEnabled2 = false;
        }
        LE_DEBUG("audio stopped successfully");
    }
    audio.gCallbackPromise.set_value(error);
    return;
}

void taf_Audio::PlayDtmfCallback(ErrorCode error)
{
    auto &audio = taf_Audio::GetInstance();
    if (ErrorCode::SUCCESS == error) {
        LE_DEBUG("Dtmf tone played !!");
    }
    audio.gCallbackPromise.set_value(error);
    return;
}

void taf_Audio::StreamMuteUnmuteCallback(ErrorCode error)
{
    auto &audio = taf_Audio::GetInstance();
    if (error != ErrorCode::SUCCESS) {
        LE_DEBUG("returned with error %d ", uint32_t(error));
    }
    LE_DEBUG("Mute/Unmute succeeded.");
    audio.gCallbackPromise.set_value(error);
    return;
}

void taf_Audio::WriteCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer, uint32_t bytes,
        telux::common::ErrorCode error)
{
    auto &audio = taf_Audio::GetInstance();
    if (ErrorCode::SUCCESS != error || buffer->getDataSize() != bytes) {
        audio.mEmptyPipeline = false;
        LE_DEBUG("Bytes Requested %d: Bytes Written: %d", buffer->getDataSize(), bytes);
        long offset = -1 * (long)(buffer->getDataSize() - bytes);
        fseek(audio.mFile, offset, SEEK_CUR);
        LE_DEBUG( "write failed with error code %d", int(error) );
    }
    buffer->reset();
    audio.mFreeBuffers.push(buffer);
    le_sem_Post(audio.mSemRef);
    return;
}

void taf_Audio::DeletePlayCallback(ErrorCode error) {
    auto &audio = taf_Audio::GetInstance();
    if (ErrorCode::SUCCESS == error) {
        LE_DEBUG("DeletePlayStream() succeeded.");
        audio.mAudioPlayStream.reset();
        audio.mAudioPlayStream = nullptr;
    } else {
        LE_DEBUG("Delete PlayStream error: %d", int (error));
    }
    audio.gCallbackPromise.set_value(error);
    return;
}

void taf_Audio::setStreamVolumeCallback(ErrorCode error)
{
    auto &audio = taf_Audio::GetInstance();
    if (error != ErrorCode::SUCCESS) {
        LE_DEBUG("setVolume() returned with error %d" , int(error));
    }
    audio.gCallbackPromise.set_value(error);

    LE_DEBUG("setVolume() succeeded.");
    return;
}

void taf_Audio::getStreamVolumeCallback(StreamVolume volume, ErrorCode error)
{
    auto &audio = taf_Audio::GetInstance();
    if (error != ErrorCode::SUCCESS) {
        LE_DEBUG("getVolume() returned with error %d", int(error));
    }

    LE_DEBUG("Volume direction: %d",uint32_t(volume.dir));

    int i = 0;
    for (auto channel_volume : volume.volume) {
        LE_DEBUG("ChannelVolume [%d] channeltype:%d Volume: %d ",i
                ,uint32_t(channel_volume.channelType), uint32_t(channel_volume.vol));
    }
    audio.mVol = volume;
    audio.gCallbackPromise.set_value(error);
    return;
}

/**
 * Creates the stream for audio operation and Starts audio stream
 * @param [in] streamConfig    stream configuration.
 * Callback methods used w.r.t the stream config for create/start stream
 * StreamType::VOICE_CALL
 * StreamType::PLAY
 * StreamType::CAPTURE
 * @returns Status of request i.e. success or suitable status code.
 */
le_result_t taf_Audio::StartAudio
(
 StreamConfig config
)
{
    // SA415M does not support slotId. Comment this function as a workaround.
#ifdef TARGET_SA515M
    LE_DEBUG("Create and Start audio\n");
    resetCallbackPromise();
    auto status = Status::FAILED;
    std::promise<bool> p;
    std::shared_ptr<telux::audio::IAudioStream> tafAudioStream;
    if (!mAudioManager){
        LE_ERROR("Invalid Audio Manager ");
        return LE_FAULT;
    }

    Status audioStatus = mAudioManager->createStream(config,
            [&p,&tafAudioStream,this](std::shared_ptr<telux::audio::IAudioStream> &stream,
                telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
            tafAudioStream = stream;
            p.set_value(true);
            } else {
            p.set_value(false);
            LE_INFO("failed to Create a stream");
            }
            });
    if(audioStatus == Status::SUCCESS) {
        LE_DEBUG("Request to create stream sent" );
    } else {
        LE_ERROR("Request to create stream failed: %d", int(audioStatus));
        return LE_FAULT;
    }

    if (p.get_future().get()) {
        LE_DEBUG("Audio Stream is Created" );
        if(tafAudioStream->getType() == StreamType::VOICE_CALL) {
            if (config.slotId == SLOT_ID_1) {
                mAudioVoiceStream = std::dynamic_pointer_cast<
                    telux::audio::IAudioVoiceStream>(tafAudioStream);
            }
            if (config.slotId == SLOT_ID_2) {
                mAudioVoiceStream2 = std::dynamic_pointer_cast<
                    telux::audio::IAudioVoiceStream>(tafAudioStream);
            }
            LE_DEBUG("Voice Stream is Created on slot id %d ",config.slotId );

        } else if(tafAudioStream->getType() == StreamType::PLAY) {
            mAudioPlayStream = std::dynamic_pointer_cast<
                telux::audio::IAudioPlayStream>(tafAudioStream);
            LE_DEBUG("Audio Play Stream is Created" );
            if(mAudioPlayStream) {
                Status status = mAudioPlayStream->registerListener(mPlayListener);
                if(status == Status::SUCCESS) {
                    LE_DEBUG("Request to register Play Listener Sent" );
                } else {
                    LE_DEBUG("Request to register Play Listener failed %d", int (status));
                }
            }
        } else if(tafAudioStream->getType() == StreamType::CAPTURE) {
            mAudioCaptureStream = std::dynamic_pointer_cast<
                telux::audio::IAudioCaptureStream>(tafAudioStream);
            LE_DEBUG("Audio Capture Stream is Created" );
        } else {
            LE_DEBUG("Unkonw Stream Created" );
        }
    }

    if(mAudioVoiceStream && (config.slotId == SLOT_ID_1) && !mVoiceEnabled1) {
        mCallStarted = true;
        status = mAudioVoiceStream->startAudio(StartAudioCallback);
        Status st = mAudioVoiceStream->registerListener(mVoiceListener);
          if(st == Status::SUCCESS) {
          LE_DEBUG("Request to register Voice Listener Sent" );
          }
    }

    if(mAudioVoiceStream2 && (config.slotId == SLOT_ID_2) && !mVoiceEnabled2) {
        status = mAudioVoiceStream2->startAudio(StartAudioCallback);
    }

    if (mAudioVoiceStream || mAudioVoiceStream2) {
        if (status == Status::SUCCESS) {
            LE_DEBUG("Request to start voice stream sent");
            ErrorCode error = gCallbackPromise.get_future().get();
            if (ErrorCode::SUCCESS != error) {
                LE_ERROR("Request to start failed");
                return LE_FAULT;
            }
        } else {
            LE_ERROR("Request to start voice stream failed.\n");
            return LE_FAULT;
        }
    }
#endif
    return LE_OK;
}

/**
 * Stop Audio stream
 * Callback methods used w.r.t the stream for stop
 * StreamType::VOICE_CALL
 * StreamType::PLAY
 * StreamType::CAPTURE
 * @returns Status of request i.e. success or suitable status code.
 */
le_result_t taf_Audio::StopAudio
(
)
{
    LE_DEBUG("Stop audio stream\n");
    resetCallbackPromise();
    auto status = Status::FAILED;

    if (mAudioVoiceStream && (mSlotId == SLOT_ID_1) && mVoiceEnabled1) {
        Status st = mAudioVoiceStream->deRegisterListener(mVoiceListener);
        if(st == Status::SUCCESS) {
            LE_DEBUG("Request to Deregister Voice Listener Sent" );
        }
        status = mAudioVoiceStream->stopAudio(StopAudioCallback);
    }

    if (mAudioVoiceStream2 && (mSlotId == SLOT_ID_2)&& mVoiceEnabled2) {
        status = mAudioVoiceStream2->stopAudio(StopAudioCallback);
    }

    if (mAudioPlayStream && mIsPlaying) {
        mIsPlaying = false;
        auto &audio = taf_Audio::GetInstance();
        if (audio.mFileFormat == AudioFormat::PCM_16BIT_SIGNED)
        {
            LE_DEBUG("Stop WAV file successful");
            return LE_OK;
        }
        status = mAudioPlayStream->stopAudio(StopType::FORCE_STOP, StopAudioCallback);
    }

    if (mAudioCaptureStream) {
        LE_DEBUG("mAudioCaptureStream no API for Stop ");
    }

    if (status == Status::SUCCESS) {
        LE_DEBUG("Stop successful");
        ErrorCode error = gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS != error) {
            LE_ERROR("Request to Stop stream failed error: %d", int (error));
            return LE_FAULT;
        }
    }

    return LE_OK;
}

/**
 * Delete Audio stream
 * Callback methods used w.r.t the stream for delete stream
 * Sets null to the stream created
 * StreamType::VOICE_CALL
 * StreamType::PLAY
 * StreamType::CAPTURE
 * @returns Status of request i.e. success or suitable status code.
 */
le_result_t taf_Audio::DeleteAudio
(
)
{
    LE_DEBUG(" Delete stream\n");
    resetCallbackPromise();
    auto status = Status::FAILED;

    if (mAudioVoiceStream && (mSlotId == SLOT_ID_1)) {
        status = mAudioManager->deleteStream(mAudioVoiceStream, DeleteVoiceCallback);
    }

    if (mAudioVoiceStream2 && (mSlotId == SLOT_ID_2)) {
        status = mAudioManager->deleteStream(mAudioVoiceStream2, DeleteVoiceCallback2);
    }

    if (mAudioPlayStream) {
        Status st = mAudioPlayStream->deRegisterListener(mPlayListener);
        if(st == Status::SUCCESS) {
            LE_DEBUG("Request to deregister Play Listener Sent" );
        }
        mIsPlayStreamCreated = false;
        status = mAudioManager->deleteStream(mAudioPlayStream, DeletePlayCallback);
    }

    if (status == Status::SUCCESS) {
        ErrorCode error = gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS != error) {
            LE_ERROR("Request to delet stream failed error: %d", int (error));
            return LE_FAULT;
        }
    } else {
        LE_ERROR("Error in disabling audio ");
        return LE_FAULT;
    }

    return LE_OK;
}

void tafVoiceListener::onDtmfToneDetection(telux::audio::DtmfTone dtmfTone) {
    LE_DEBUG("Dtmf Tone Detected");
    LE_DEBUG("Direction is %d",uint32_t (dtmfTone.direction));
    LE_DEBUG("Low Frequency is %d",uint32_t(dtmfTone.lowFreq));
    LE_DEBUG("High Frequency is %d",uint32_t(dtmfTone.highFreq));
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)DtmfAudioRef;
    taf_audio_StreamEvent_t streamEvent;
    streamEvent.streamPtr = streamPtr;
    streamEvent.streamEvent = TAF_AUDIO_BITMASK_DTMF_DETECTION;
    le_event_Report(streamPtr->eventId, &streamEvent,
            sizeof(taf_audio_StreamEvent_t));
}

void tafPlayListener::onReadyForWrite() {
    auto &audio = taf_Audio::GetInstance();
    audio.mEmptyPipeline = true;
    le_sem_Post(audio.mSemRef);
}

void tafPlayListener::onPlayStopped() {
    LE_DEBUG("onPlayStopped");
}

size_t HashRef
(
 const void* safeRefPtr
)
{
    return (size_t) safeRefPtr;
}

static bool EqualsRef
(
 const void* firstRef,
 const void* secondRef
)
{
    return firstRef == secondRef;
}

le_hashmap_Ref_t taf_Audio::GetHashMap
(
 void
)
{
    struct hashMapList* currentPtr=NULL;
    le_dls_Link_t* lPtr = le_dls_Peek(&HashMapList);

    while (lPtr!=NULL)
    {
        currentPtr = CONTAINER_OF(lPtr,
                struct hashMapList,
                hashMapLink);

        if (!currentPtr->isUsed)
        {
            LE_DEBUG("Found one HashMap unused (%p)", currentPtr->hashMapRef);
            currentPtr->isUsed = true;
            return currentPtr->hashMapRef;
        }
        lPtr = le_dls_PeekNext(&HashMapList,lPtr);
    }

    currentPtr = (hashMapList*)le_mem_ForceAlloc(HashMapPool);

    char ConnMapName[20];

    snprintf( ConnMapName,20,"ConnMap%d", (int) (le_dls_NumLinks(&HashMapList)+1) );

    currentPtr->hashMapRef = le_hashmap_Create(ConnMapName,
            HASHMAP_SIZE,
            HashRef,
            EqualsRef);
    currentPtr->isUsed = true;
    currentPtr->hashMapLink = LE_DLS_LINK_INIT;

    le_dls_Queue(&HashMapList,&(currentPtr->hashMapLink));

    LE_DEBUG("Create a new HashMap (%p)", currentPtr->hashMapRef);

    return currentPtr->hashMapRef;
}

void taf_Audio::FirstLayerEventHandler( void* reportPtr, void* secondLayerHandlerFunc ) {
    taf_audio_StreamEvent_t* streamEventPtr = (taf_audio_StreamEvent_t*)reportPtr;
    EventHandlerRefNode_t* streamRefNodePtr = (EventHandlerRefNode_t*)le_event_GetContextPtr();
    LE_DEBUG("FirstLayerEventHandler");

    TAF_ERROR_IF_RET_NIL((!streamRefNodePtr) || (!streamEventPtr) || (!streamEventPtr->streamPtr), "Invalid reference");

    taf_audio_Stream_t* streamPtr = streamEventPtr->streamPtr;

    switch ( streamEventPtr->streamEvent )
    {
        case TAF_AUDIO_BITMASK_MEDIA_EVENT:
            {
                taf_audio_MediaEvent_t mediaEvent = streamEventPtr->event.mediaEvent;

                LE_DEBUG("MediaEvent %d", mediaEvent);

                if (streamPtr->playFile)
                {
                    if (mediaEvent == TAF_AUDIO_MEDIA_NO_MORE_SAMPLES)
                    {
                        mediaEvent = TAF_AUDIO_MEDIA_ENDED;
                    }
                }

                taf_audio_MediaHandlerFunc_t clientHandlerFunc = (taf_audio_MediaHandlerFunc_t)secondLayerHandlerFunc;
                clientHandlerFunc(streamPtr->streamRef, mediaEvent, streamRefNodePtr->userCtx);
            }
            break;

        case TAF_AUDIO_BITMASK_DTMF_DETECTION:
            {
                taf_audio_DtmfDetectorHandlerFunc_t clientHandlerFunc = (taf_audio_DtmfDetectorHandlerFunc_t)secondLayerHandlerFunc;
                clientHandlerFunc(streamPtr->streamRef,
                        streamEventPtr->event.dtmf,
                        streamRefNodePtr->userCtx);
            }
            break;
    }
}


void taf_Audio::ClearHashMap
(
 le_hashmap_Ref_t hashMapRef
)
{
    LE_ASSERT(hashMapRef);

    le_dls_Link_t* lPtr = le_dls_Peek(&HashMapList);
    struct hashMapList* currentPtr;

    while (lPtr!=NULL)
    {
        currentPtr = CONTAINER_OF(lPtr, struct hashMapList, hashMapLink);

        if (currentPtr->hashMapRef == hashMapRef)
        {
            LE_DEBUG("Release HashMap (%p)", currentPtr->hashMapRef);
            currentPtr->isUsed = false;
            return;
        }
        lPtr = le_dls_PeekNext(&HashMapList,lPtr);
    }

    LE_DEBUG("Nothing to HashMap to release");
    return;
}

void taf_Audio::DeleteHashMap
(
 taf_audio_Connector_t* connPtr
)
{

    TAF_KILL_CLIENT_IF_RET_NIL(connPtr == NULL,  "connPtr is nullptr!");
    le_hashmap_It_Ref_t Iterator;
    taf_audio_Stream_t const * currentStreamPtr;

    Iterator = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(connPtr->audioInList);
    while (le_hashmap_NextNode(Iterator)==LE_OK)
    {
        currentStreamPtr = (taf_audio_Stream_t const *)le_hashmap_GetValue(Iterator);

        le_hashmap_Remove(currentStreamPtr->connList,connPtr);
    }

    Iterator = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(connPtr->audioOutList);
    while (le_hashmap_NextNode(Iterator)==LE_OK)
    {
        currentStreamPtr = (taf_audio_Stream_t const *)le_hashmap_GetValue(Iterator);

        le_hashmap_Remove(currentStreamPtr->connList,connPtr);
    }

    le_hashmap_RemoveAll(connPtr->audioInList);
    le_hashmap_RemoveAll(connPtr->audioOutList);

    ClearHashMap(connPtr->audioInList);
    ClearHashMap(connPtr->audioOutList);
}

/**
 * create and start the stream
 */
le_result_t taf_Audio::CreateandStart
(
 taf_audio_Stream_t*      streamPtr,
 le_hashmap_Ref_t        streamListPtr
)
{
    TAF_ERROR_IF_RET_VAL( streamPtr == NULL, LE_BAD_PARAMETER,"streamPtr is nullptr!");
    taf_audio_Stream_t* inputPtr;
    taf_audio_Stream_t* outputPtr;
    taf_audio_Stream_t* currentPtr;
    le_result_t res = LE_FAULT;

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(streamListPtr);

    while (le_hashmap_NextNode(streamIterator) == LE_OK)
    {
        currentPtr = (taf_audio_Stream_t*)le_hashmap_GetValue(streamIterator);

        LE_DEBUG("CurrentStream %p",currentPtr);

        if (streamPtr->device)
        {
            inputPtr  = streamPtr;
            outputPtr = currentPtr;
        }
        else
        {
            inputPtr  = currentPtr;
            outputPtr = streamPtr;
        }

        LE_DEBUG("Input [%d] and Output [%d] are tied together.",
                inputPtr->interface,
                outputPtr->interface);

        StreamConfig config = {};
        if (mModemRx && mSpeaker && !mCallStarted)
        {
            config.type = StreamType::VOICE_CALL;
#ifdef TARGET_SA515M
            config.slotId = (SlotId)mSlotId;
#endif
            if (outputPtr->samplePcmConfig.sampleRate != 0)
            {
                config.sampleRate = outputPtr->samplePcmConfig.sampleRate;
            }
            else
            {
                config.sampleRate = 16000;
                LE_INFO("setting default sampling rate as 16000");
            }
            config.format = AudioFormat::PCM_16BIT_SIGNED;
            config.channelTypeMask = ChannelType::LEFT | ChannelType::RIGHT;

            // Set the config device type based on output device
            if (outputPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER) {
                config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
                LE_DEBUG("set config with device type speaker");
            }
            else if (outputPtr->interface == TAF_AUDIO_IF_PCM_SPEAKER) {
                config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_HEADSET_SPEAKER);
                LE_DEBUG("set config with device type headset speaker");
            }
            else if (outputPtr->interface == TAF_AUDIO_IF_I2S_SPEAKER) {
                config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
                LE_DEBUG("set config with device type speaker");
                taf_audio_I2SChannel_t channel = outputPtr->channelMode;
                switch(channel)
                {
                    case TAF_AUDIO_I2S_LEFT:
                        config.channelTypeMask = ChannelType::LEFT;
                        break;
                    case TAF_AUDIO_I2S_RIGHT:
                        config.channelTypeMask = ChannelType::RIGHT;
                        break;
                    default:
                        config.channelTypeMask = ChannelType::LEFT | ChannelType::RIGHT;
                        break;
                }
            }
#ifdef TARGET_SA515M
            if (streamPtr->echoCancellerEnabled) {
                config.ecnrMode = EcnrMode::ENABLE;
            } else {
                config.ecnrMode = EcnrMode::DISABLE;
            }
#endif
            DtmfAudioRef = (taf_audio_StreamRef_t)streamPtr;
            res = StartAudio(config);
        } else {
            res = LE_OK;
        }

    }

    return res;
}

StreamEventHandlerRef_t taf_Audio::AddStreamEventHandler
(
 taf_audio_Stream_t* sPtr,
 le_event_HandlerFunc_t handlerPtr,
 taf_audio_StreamEventBitMask_t streamEventBitMask,
 void* contextPtr
)
{
    auto &audio = taf_Audio::GetInstance();
    le_event_HandlerRef_t handlerRef;

    TAF_KILL_CLIENT_IF_RET_VAL((sPtr == NULL) || (handlerPtr == NULL), NULL, "Invalid reference/handle");

    LE_DEBUG("Add handler on interface %d.", sPtr->interface);

    EventHandlerRefNode_t* streamRefNodePtr = (EventHandlerRefNode_t*)le_mem_ForceAlloc(
            audio.EventHandlerRefNodePool);
    streamRefNodePtr->streamEventMask = streamEventBitMask;

    streamRefNodePtr->next = LE_DLS_LINK_INIT;

    streamRefNodePtr->streamPtr = sPtr;

    handlerRef = le_event_AddLayeredHandler("StreamEventHandler", sPtr->eventId,
                                       FirstLayerEventHandler, (void*)handlerPtr);

    streamRefNodePtr->userCtx = contextPtr;

    le_event_SetContextPtr(handlerRef, streamRefNodePtr);

    streamRefNodePtr->handlerRef = handlerRef;

    le_dls_Queue(&(sPtr->streamRefWithEventHdlrList),&(streamRefNodePtr->next));

    streamRefNodePtr->streamHandlerRef = (StreamEventHandlerRef*)le_ref_CreateRef(audio.EventHandlerRefMap,
            streamRefNodePtr );

    return streamRefNodePtr->streamHandlerRef;
}

static le_event_Id_t CreateEventId
(
)
{
    auto &audio = taf_Audio::GetInstance();
    struct tafEventIdList* curPtr = NULL;
    le_dls_Link_t*      linkPtr = le_dls_Peek(&audio.EventIdList);
    char                eventName[25];
    int32_t             eventId = 1;

    while (linkPtr!=NULL)
    {
        curPtr = CONTAINER_OF(linkPtr, struct tafEventIdList, next);

        if (!curPtr->inUse)
        {
            LE_DEBUG("unused eventId (%p)", curPtr->eventId);
            curPtr->inUse = true;
            return curPtr->eventId;
        }
        linkPtr = le_dls_PeekNext(&audio.EventIdList,linkPtr);

        eventId++;
    }

    snprintf(eventName, sizeof(eventName), "eventId-%d", eventId);

    curPtr = (tafEventIdList*)le_mem_ForceAlloc(audio.EventIdPool);
    curPtr->eventId = le_event_CreateId(eventName, sizeof(taf_audio_StreamEvent_t));
    curPtr->inUse = true;
    curPtr->next = LE_DLS_LINK_INIT;

    le_dls_Queue(&audio.EventIdList, &(curPtr->next));

    LE_DEBUG("Create a new eventId (%p)", curPtr->eventId);

    return curPtr->eventId;
}

taf_audio_DtmfDetectorHandlerRef_t taf_Audio::AddDtmfDetectorHandler
(
 taf_audio_StreamRef_t               streamRef,
 taf_audio_DtmfDetectorHandlerFunc_t handlerPtr,
 void*                              ctxPtr
 )
{
    auto &audio = taf_Audio::GetInstance();
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(audio.AudioRefMap, streamRef);

    TAF_KILL_CLIENT_IF_RET_VAL(streamPtr == NULL, NULL, "Invalid reference");
    LE_DEBUG("AddDtmfDetectorHandler");

    return (taf_audio_DtmfDetectorHandlerRef_t) audio.AddStreamEventHandler(
            streamPtr,
            (le_event_HandlerFunc_t) handlerPtr,
            TAF_AUDIO_BITMASK_DTMF_DETECTION,
            ctxPtr
            );
}

/**
 * Stop and delete the current stream
 */
le_result_t taf_Audio::StopandDelete
(
 taf_audio_Stream_t*    streamPtr,
 le_hashmap_Ref_t      streamListPtr
)
{
    TAF_ERROR_IF_RET_VAL( streamPtr == NULL, LE_BAD_PARAMETER,"streamPtr is nullptr!");
    le_result_t   res = LE_OK;
    taf_audio_Stream_t* inputPtr;
    taf_audio_Stream_t* outputPtr;
    taf_audio_Stream_t* currentPtr;

    LE_DEBUG("StopandDelete stream.%p", streamPtr);

    le_hashmap_It_Ref_t streamIterator = le_hashmap_GetIterator(streamListPtr);

    while (le_hashmap_NextNode(streamIterator)==LE_OK)
    {
        currentPtr=(taf_audio_Stream_t*)le_hashmap_GetValue(streamIterator);

        if (streamPtr->device)
        {
            inputPtr  = streamPtr;
            outputPtr = currentPtr;
        }
        else
        {
            inputPtr  = currentPtr;
            outputPtr = streamPtr;
        }

        LE_DEBUG("inputInterface.%d with outputInterface.%d",
                inputPtr->interface, outputPtr->interface);
        StopAudio();
        DeleteAudio();
        res = LE_OK;

    }

    return res;
}

/**
 * Close the connectors
 */
void taf_Audio::CloseConnector
(
 taf_audio_Connector_t*   connPtr
)
{
    TAF_ERROR_IF_RET_NIL( connPtr == NULL, "connPtr is nullptr!");
    LE_DEBUG("CloseConnector %p", connPtr);

    le_hashmap_It_Ref_t Iterator = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(connPtr->audioInList);
    taf_audio_Stream_t* currentStreamPtr;
    while (le_hashmap_NextNode(Iterator)==LE_OK)
    {
        currentStreamPtr=(taf_audio_Stream_t*)le_hashmap_GetValue(Iterator);
        StopandDelete(currentStreamPtr,connPtr->audioOutList);
    }
}

/**
 * Disconnect the connectors
 */
void taf_Audio::DisconnectConnectors
(
 taf_audio_Stream_t*     streamPtr
)
{
    TAF_ERROR_IF_RET_NIL( streamPtr == NULL, "streamPtr is nullptr!");
    LE_DEBUG("DisconnectConnectors streamPtr.%p", streamPtr);

    le_hashmap_It_Ref_t Iterator = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
    taf_audio_Connector_t const * currentconnPtr;

    while (le_hashmap_NextNode(Iterator)==LE_OK)
    {
        currentconnPtr = (taf_audio_Connector_t const *)le_hashmap_GetValue(Iterator);

        Disconnect(currentconnPtr->connRef, streamPtr->streamRef);
    }
}

/**
 * Initialize stream reference
 */
void taf_Audio::InitAudio
(
 taf_audio_Stream_t* streamPtr
)
{
    TAF_ERROR_IF_RET_NIL( streamPtr == NULL, "streamPtr is nullptr!");

    memset(streamPtr,0,sizeof(taf_audio_Stream_t));
    streamPtr->fd=-1;
    streamPtr->connList = GetHashMap();
}

/**
 * Create stream for input and output stream reference
 * if not opened already
 */
taf_audio_StreamRef_t taf_Audio::CreateStream
(
 CreateStream_t* streamPtr
)
{
    LE_DEBUG("Create audio stream (%d)", streamPtr->interface);
    bool isOpened = false;
    taf_audio_Stream_t* StreamPtr=NULL;
    le_ref_IterRef_t iterRef;
    taf_SessionRefNode_t* newSessionRefPtr;

    if (streamPtr->HwDevice)
    {
        iterRef = (le_ref_IterRef_t)le_ref_GetIterator(AudioRefMap);

        while (!isOpened && (le_ref_NextNode(iterRef) == LE_OK))
        {
            StreamPtr = (taf_audio_Stream_t*) le_ref_GetValue(iterRef);

            if (StreamPtr->interface == streamPtr->interface)
            {
                isOpened = true;
            }
        }
    }

    if ( !isOpened )
    {
        StreamPtr = (taf_audio_Stream_t*)le_mem_ForceAlloc(AudioPool);

        InitAudio(StreamPtr);

        StreamPtr->interface = streamPtr->interface;
        StreamPtr->device = !CHECK_OUTPUT_IF(streamPtr->interface);
        StreamPtr->sessionRefList=LE_DLS_LIST_INIT;

        switch ( streamPtr->interface )
        {
            case TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY:
                 StreamPtr->eventId = CreateEventId();
                break;
            case TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
                 StreamPtr->eventId = CreateEventId();
            case TAF_AUDIO_IF_CODEC_MIC:
            case TAF_AUDIO_IF_CODEC_SPEAKER:
                break;
            case TAF_AUDIO_IF_PCM_MIC:
            case TAF_AUDIO_IF_PCM_SPEAKER:
                StreamPtr->timeSlot = streamPtr->timeSlot;
                break;
            case TAF_AUDIO_IF_I2S_MIC:
            case TAF_AUDIO_IF_I2S_SPEAKER:
                StreamPtr->channelMode = streamPtr->channelMode;
                break;
            default:
                break;
        }

        StreamPtr->streamRef = (taf_audio_StreamRef_t)le_ref_CreateRef(AudioRefMap, StreamPtr);

        LE_DEBUG("Create streamRef %p of interface.%d",
                StreamPtr->streamRef, StreamPtr->interface);
    }
    else
    {
        le_mem_AddRef(StreamPtr);

        LE_DEBUG("AddRef for streamRef %p of interface.%d",
                StreamPtr->streamRef, StreamPtr->interface);
    }

    newSessionRefPtr = (taf_SessionRefNode_t*)le_mem_ForceAlloc(SessionRefPool);
    newSessionRefPtr->sessionRef = taf_audio_GetClientSessionRef();
    newSessionRefPtr->refNodeLink = LE_DLS_LINK_INIT;

    le_dls_Queue(&StreamPtr->sessionRefList, &(newSessionRefPtr->refNodeLink));

    return StreamPtr->streamRef;
}


/**
 * Create new connector for input and output stream reference
 */
taf_audio_ConnectorRef_t taf_Audio::CreateConnector
(
 void
)
{
    taf_audio_Connector_t* newconnPtr = (taf_audio_Connector_t*)le_mem_ForceAlloc(AudioConnPool);

    newconnPtr->audioInList   = GetHashMap();
    newconnPtr->audioOutList  = GetHashMap();
    newconnPtr->sessionRef = taf_audio_GetClientSessionRef();
    newconnPtr->connRef = (taf_audio_Connector_t*)le_ref_CreateRef(AudioConnRefMap, newconnPtr);

    newconnPtr->connLink = LE_DLS_LINK_INIT;

    le_dls_Queue(&ConnList,&(newconnPtr->connLink));

    return newconnPtr->connRef;
}

/**
 * Connect the available input and output stream reference
 */
le_result_t taf_Audio::Connect
(
 taf_audio_ConnectorRef_t connRef,
 taf_audio_StreamRef_t    sRef
)
{
    le_hashmap_Ref_t      lPtr=NULL;
    le_result_t res;
    taf_audio_Stream_t*    sPtr = (taf_audio_Stream_t*)le_ref_Lookup(AudioRefMap, sRef);
    taf_audio_Connector_t* connPtr = (taf_audio_Connector_t*)le_ref_Lookup(AudioConnRefMap, connRef);

    TAF_ERROR_IF_RET_VAL( connPtr == NULL, LE_BAD_PARAMETER, "connPtr is nullptr!");
    TAF_ERROR_IF_RET_VAL( sPtr == NULL, LE_BAD_PARAMETER,"streamPtr is nullptr!");

    LE_DEBUG("StreamRef.%p (@%p) Connect [%d] '%s' to connRef.%p",
            sRef, sPtr, sPtr->interface,
            (sPtr->device)?"input":"output", connRef);

    if ( sPtr->device )
    {
        if (le_hashmap_ContainsKey(connPtr->audioInList, sPtr))
        {
            LE_ERROR("Already connected");
            return LE_BUSY;
        }
        else
        {
            le_hashmap_Put(connPtr->audioInList, sPtr, sPtr);
            lPtr = connPtr->audioOutList;
        }
    }
    else
    {
        if (le_hashmap_ContainsKey(connPtr->audioOutList, sPtr))
        {
            LE_ERROR("Already connected");
            return LE_BUSY;
        }
        else
        {
            le_hashmap_Put(connPtr->audioOutList, sPtr, sPtr);
            lPtr = connPtr->audioInList;
        }
    }

    le_hashmap_Put(sPtr->connList, connPtr, connPtr);

    LE_DEBUG("le_hashmap_Size(lPtr) %d", (int) le_hashmap_Size(lPtr));

    if (le_hashmap_Size(lPtr) >= 1)
    {
        if ((res = CreateandStart (sPtr, lPtr)) != LE_OK)
        {
            return res;
        }
    }

    return LE_OK;
}

taf_audio_MediaHandlerRef_t taf_Audio::AddMediaHandler
(
    taf_audio_StreamRef_t streamRef,
    taf_audio_MediaHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &audio = taf_Audio::GetInstance();
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(audio.AudioRefMap, streamRef);
    LE_DEBUG("AddMediaHandler");

    TAF_KILL_CLIENT_IF_RET_VAL((streamPtr == NULL), NULL, "Invalid reference");
    if ((streamPtr->interface != TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY))
    {
        LE_ERROR("Bad Interface!");
        return NULL;
    }

    return (taf_audio_MediaHandlerRef_t) audio.AddStreamEventHandler(streamPtr, (le_event_HandlerFunc_t)handlerPtr,
                                         TAF_AUDIO_BITMASK_MEDIA_EVENT, (void*)contextPtr);
}

/**
 * Delete the connecter
 */
void taf_Audio::DeleteConnector
(
 taf_audio_ConnectorRef_t connRef
)
{
    taf_audio_Connector_t* connPtr = (taf_audio_Connector_t*)le_ref_Lookup(AudioConnRefMap, connRef);

    TAF_ERROR_IF_RET_NIL( connPtr == NULL, "connPtr is nullptr!");

    CloseConnector (connPtr);

    DeleteHashMap (connPtr);


    le_dls_Remove(&ConnList,&(connPtr->connLink));

    le_ref_DeleteRef(AudioConnRefMap, connRef);

    le_mem_Release(connPtr);
}

static void DeleteEventId
(
 le_event_Id_t eventId
)
{
    auto &audio = taf_Audio::GetInstance();
    le_dls_Link_t* linkPtr = le_dls_Peek(&audio.EventIdList);

    while (linkPtr!=NULL)
    {
        struct tafEventIdList* curPtr = CONTAINER_OF(linkPtr,
                struct tafEventIdList,
                next);

        if (curPtr->eventId == eventId)
        {
            LE_DEBUG("Found eventId to release (%p)", curPtr->eventId);
            curPtr->inUse = false;
            return;
        }
        linkPtr = le_dls_PeekNext(&audio.EventIdList,linkPtr);
    }

    LE_DEBUG("Nothing to delete");
    return;
}

static void RemoveAudioEventHandler
(
 StreamEventHandlerRef_t addHandlerRef
)
{
    auto &audio = taf_Audio::GetInstance();
    EventHandlerRefNode_t* streamRefNodePtr = (EventHandlerRefNode_t*)le_ref_Lookup(audio.EventHandlerRefMap,
            addHandlerRef);
    TAF_ERROR_IF_RET_NIL( streamRefNodePtr == NULL, "Cannot find stream reference");

    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)streamRefNodePtr->streamPtr;

    le_event_RemoveHandler((le_event_HandlerRef_t)streamRefNodePtr->handlerRef);

    le_ref_DeleteRef(audio.EventHandlerRefMap, streamRefNodePtr->streamHandlerRef);

    if (streamRefNodePtr->streamEventMask == TAF_AUDIO_BITMASK_DTMF_DETECTION)
    {
        uint32_t handlerCount = 0;

        le_dls_Link_t* linkPtr = le_dls_Peek(&(streamPtr->streamRefWithEventHdlrList));

        while (linkPtr != NULL)
        {
            EventHandlerRefNode_t* nodePtr;

            nodePtr = CONTAINER_OF(linkPtr, EventHandlerRefNode_t, next);

            linkPtr = le_dls_PeekNext(&streamPtr->streamRefWithEventHdlrList, linkPtr);

            if (nodePtr->streamEventMask == TAF_AUDIO_BITMASK_DTMF_DETECTION)
            {
                handlerCount++;
            }
        }

        LE_DEBUG("handlerCount %d", handlerCount);
        if (handlerCount == 1)
        {
            streamPtr->dtmfEventHandler = NULL;
        }
    }

    le_dls_Remove(&streamPtr->streamRefWithEventHdlrList,
            &streamRefNodePtr->next);

    le_mem_Release(streamRefNodePtr);
}

static void RemoveAllHandlers
(
    taf_audio_Stream_t* streamPtr
)
{
    le_dls_Link_t* linkPtr;

    if (streamPtr->streamRefWithEventHdlrList.headLinkPtr != NULL)
    {
        EventHandlerRefNode_t* nodePtr;

        linkPtr = le_dls_Peek(&(streamPtr->streamRefWithEventHdlrList));
        while (linkPtr != NULL)
        {
            nodePtr = CONTAINER_OF(linkPtr, EventHandlerRefNode_t, next);

            linkPtr = le_dls_PeekNext(&streamPtr->streamRefWithEventHdlrList, linkPtr);

            RemoveAudioEventHandler(nodePtr->streamHandlerRef);
        }
    }
}

/**
 * Release the connecter
 */
void taf_Audio::ReleaseStream( void *objPtr )
{
    LE_DEBUG("ReleaseStream");
    LE_ASSERT(objPtr);
    auto &audio = taf_Audio::GetInstance();

    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)objPtr;

    RemoveAllHandlers(streamPtr);
    audio.DisconnectConnectors (streamPtr);

    le_hashmap_RemoveAll(streamPtr->connList);
    audio.ClearHashMap(streamPtr->connList);
    DeleteEventId(streamPtr->eventId);
    le_ref_DeleteRef(audio.AudioRefMap, streamPtr->streamRef);
}

/**
 * Delete all stream reference
 */
void taf_Audio::DeleteStream
(
 taf_audio_Stream_t*  streamPtr,
 le_msg_SessionRef_t sessionRef,
 bool                deleteReferences
)
{
    taf_SessionRefNode_t* sessionRefNodePtr;
    le_dls_Link_t* lPtr;

    LE_DEBUG("interface %d, sessionRef %p", streamPtr->interface, sessionRef);

    lPtr = le_dls_Peek(&(streamPtr->sessionRefList));
    while (lPtr != NULL)
    {
        sessionRefNodePtr = CONTAINER_OF(lPtr, taf_SessionRefNode_t, refNodeLink);

        lPtr = le_dls_PeekNext(&(streamPtr->sessionRefList), lPtr);

        LE_DEBUG("sessionRef %p", sessionRefNodePtr->sessionRef);

        if ( sessionRefNodePtr->sessionRef == sessionRef )
        {
            le_dls_Remove(&(streamPtr->sessionRefList),
                    &(sessionRefNodePtr->refNodeLink));

            le_mem_Release(sessionRefNodePtr);

            LE_DEBUG("Release stream %d", streamPtr->interface);
            le_mem_Release(streamPtr);

            if (!deleteReferences)
            {
                return;
            }
        }
    }
}

/**
 * Disconnect the connecter and stream reference
 */
void taf_Audio::Disconnect
(
 taf_audio_ConnectorRef_t connRef,
 taf_audio_StreamRef_t    streamRef
)
{
    taf_audio_Stream_t*    streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(AudioRefMap, streamRef);
    taf_audio_Connector_t* connPtr = (taf_audio_Connector_t*)le_ref_Lookup(AudioConnRefMap, connRef);

    TAF_ERROR_IF_RET_NIL( connPtr == NULL, "connPtr is nullptr!");
    TAF_ERROR_IF_RET_NIL( streamPtr == NULL, "streamPtr is nullptr!");

    LE_DEBUG("Disconnect stream.%p from connector.%p", streamRef, connRef);
    if (streamPtr->device)
    {
        if (le_hashmap_ContainsKey(connPtr->audioInList, streamPtr))
        {
            StopandDelete (streamPtr,connPtr->audioOutList);
            le_hashmap_Remove(connPtr->audioInList,streamPtr);
            le_hashmap_Remove(streamPtr->connList,connPtr);
        }
        else
        {
            LE_ERROR("Nothing connected");
        }
    }
    else
    {
        if (le_hashmap_ContainsKey(connPtr->audioOutList, streamPtr))
        {
            StopandDelete (streamPtr,connPtr->audioInList);
            le_hashmap_Remove(connPtr->audioOutList,streamPtr);
            le_hashmap_Remove(streamPtr->connList,connPtr);
        }
        else
        {
            LE_ERROR("Not linked to the connector");
        }
    }
}

/**
 * Close the Stream Reference
 */
void taf_Audio::Close
(
 taf_audio_StreamRef_t streamRef
)
{
    taf_audio_Stream_t*  streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(AudioRefMap, streamRef);


    TAF_ERROR_IF_RET_NIL( streamPtr == NULL, "streamPtr is nullptr!");
    mSpeaker = false;
    mMic = false;
    mModemRx = false;
    mModemTx = false;
    mSlotId = -1;
    mPlayer = false;
    mCallStarted = false;
    DeleteStream(streamPtr, taf_audio_GetClientSessionRef(), false);
}

/**
 * Remove handler function for EVENT
 */
void taf_audio_RemoveMediaHandler
(
    taf_audio_MediaHandlerRef_t addHandlerRef
)
{
    RemoveAudioEventHandler( (StreamEventHandlerRef_t) addHandlerRef );
}

void taf_audio_RemoveDtmfDetectorHandler
(
 taf_audio_DtmfDetectorHandlerRef_t addHandlerRef
)
{
    RemoveAudioEventHandler( (StreamEventHandlerRef_t) addHandlerRef );
}

/**
 * Setup for Speaker
 */
taf_audio_StreamRef_t taf_Audio::OpenSpeaker
(
 void
)
{
    CreateStream_t createAudio;

    mSpeaker = true;
    createAudio.HwDevice = true;
    createAudio.interface = TAF_AUDIO_IF_CODEC_SPEAKER;

    return CreateStream(&createAudio);
}

/**
 * Setup for voice outstream
 */
taf_audio_StreamRef_t taf_Audio::OpenModemVoiceRx
(
 uint32_t slotId
)
{
    CreateStream_t createAudio;

    mModemRx = true;
    mSlotId = slotId;
    createAudio.HwDevice = true;
    createAudio.interface = TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX;

    return CreateStream(&createAudio);
}

taf_audio_StreamRef_t taf_Audio::OpenMic
(
 void
)
{
    LE_DEBUG("Feature yet to Implement");
    return NULL;

}

taf_audio_StreamRef_t taf_Audio::OpenModemVoiceTx
(
 uint32_t slotId
)
{
    mSlotId = slotId;
    LE_DEBUG("Feature yet to Implement");
    return NULL;
}

/**
 * Setup for I2S Rx
 */
taf_audio_StreamRef_t taf_Audio::OpenI2sRx
(
    taf_audio_I2SChannel_t mode
)
{
    TAF_ERROR_IF_RET_VAL((mode == TAF_AUDIO_I2S_MONO || mode == TAF_AUDIO_I2S_REVERSE),
            NULL, "Channel mode is not supported");

    CreateStream_t createAudio;

    mSpeaker = true;
    createAudio.HwDevice = true;
    createAudio.interface = TAF_AUDIO_IF_I2S_SPEAKER;
    createAudio.channelMode = mode;

    return CreateStream(&createAudio);
}

/**
 * Setup for I2S Tx
 */
taf_audio_StreamRef_t taf_Audio::OpenI2sTx
(
    taf_audio_I2SChannel_t mode
)
{
    TAF_ERROR_IF_RET_VAL((mode == TAF_AUDIO_I2S_MONO || mode == TAF_AUDIO_I2S_REVERSE),
            NULL, "Channel mode is not supported");

    CreateStream_t createAudio;

    createAudio.HwDevice = true;
    createAudio.interface = TAF_AUDIO_IF_I2S_MIC;
    createAudio.channelMode = mode;

    return CreateStream(&createAudio);
}

/**
 * Setup for PCM Rx
 */
taf_audio_StreamRef_t taf_Audio::OpenPcmRx
(
    uint32_t timeslot
)
{
    CreateStream_t createAudio;

    mSpeaker = true;
    createAudio.HwDevice = true;
    createAudio.interface = TAF_AUDIO_IF_PCM_SPEAKER;
    createAudio.timeSlot = timeslot;

    return CreateStream(&createAudio);
}

/**
 * Setup for PCM Tx
 */
taf_audio_StreamRef_t taf_Audio::OpenPcmTx
(
    uint32_t timeslot
)
{
    CreateStream_t createAudio;

    createAudio.HwDevice = true;
    createAudio.interface = TAF_AUDIO_IF_PCM_MIC;
    createAudio.timeSlot = timeslot;

    return CreateStream(&createAudio);
}

/**
 * Set sampling rate for the stream
 */
le_result_t taf_Audio::SetSamplePcmSamplingRate
(
    taf_audio_StreamRef_t streamRef,
    uint32_t              samplingRate
)
{
    auto &audio = taf_Audio::GetInstance();
    taf_audio_Stream_t*  streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(audio.AudioRefMap, streamRef);

    TAF_ERROR_IF_RET_VAL( streamPtr == NULL, LE_FAULT, "streamPtr is nullptr!");

    streamPtr->samplePcmConfig.sampleRate = samplingRate;

    return LE_OK;
}

/**
 * Mute or UnMute the stream based StreamMute
 */
le_result_t taf_Audio::Mute
(
 taf_audio_StreamRef_t streamRef,
 StreamMute mute
)
{
    resetCallbackPromise();
    le_result_t res = LE_FAULT;
    auto status = Status::FAILED;

    StreamMute muteObj = mute;
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(AudioRefMap, streamRef);

    TAF_ERROR_IF_RET_VAL( streamPtr == NULL, LE_BAD_PARAMETER,"streamPtr is nullptr!");
    LE_INFO("Request Mute.");

    if (mAudioVoiceStream && (mSlotId == SLOT_ID_1)) {
        muteObj.dir = StreamDirection::TX;
        status = mAudioVoiceStream->setMute(muteObj, StreamMuteUnmuteCallback);
    } else if (mAudioVoiceStream2 && (mSlotId == SLOT_ID_2)) {
        muteObj.dir = StreamDirection::TX;
        status = mAudioVoiceStream2->setMute(muteObj, StreamMuteUnmuteCallback);
    } else if (mAudioPlayStream) {
        muteObj.dir = StreamDirection::RX;
        status = mAudioPlayStream->setMute(muteObj, StreamMuteUnmuteCallback);
    } else if (mAudioCaptureStream) {
        muteObj.dir = StreamDirection::TX;
        status = mAudioCaptureStream->setMute(muteObj, StreamMuteUnmuteCallback);
    }
    if (status == Status::SUCCESS) {
        ErrorCode error = gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS != error) {
            if (muteObj.enable) {
                LE_ERROR("Request to Mute failed error: %d", int (error));
            } else {
                LE_ERROR("Request to UnMute failed");
            }
            return LE_FAULT;
        }
        res = LE_OK;
    } else {
        if (muteObj.enable) {
            LE_ERROR("Request to Mute failed. Error: %d", int(status));
        } else {
            LE_ERROR("Request to UnMute failed");
        }
        return LE_FAULT;
    }

    return res;
}

/**
 * Play DTMF tone for Inband voice call
 */
le_result_t taf_Audio::PlayDtmf
(
 taf_audio_StreamRef_t rStreamRef,
 const char*          pdtmfPtr,
 uint32_t             uduration,
 uint32_t             upause
)
{
    resetCallbackPromise();
    auto status = Status::FAILED;
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(AudioRefMap, rStreamRef);

    TAF_ERROR_IF_RET_VAL( streamPtr == NULL, LE_BAD_PARAMETER,"streamPtr is nullptr!");

    DtmfTone dtmfTone = {};
    dtmfTone.direction = StreamDirection::RX;
    dtmfTone.lowFreq = DtmfLowFreq::FREQ_697;
    dtmfTone.highFreq = DtmfHighFreq::FREQ_1209;
    uint16_t gain = 10000;
    if (mAudioVoiceStream) {
        status = mAudioVoiceStream->playDtmfTone(
                dtmfTone, uduration, gain, PlayDtmfCallback);
    } else if (mAudioVoiceStream2) {
        status = mAudioVoiceStream2->playDtmfTone(
                dtmfTone, uduration, gain, PlayDtmfCallback);
    }
    if(status == Status::SUCCESS) {
        ErrorCode error = gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS != error) {
            LE_DEBUG("Request to play Dtmf Tone failed");
        }
    }else {
        LE_DEBUG("Request to play Dtmf Tone failed");
    }
    return LE_OK;
}

/**
 * Get the player interface
 */
taf_audio_StreamRef_t taf_Audio::OpenPlayer
(
)
{
    CreateStream_t createAudio;
    mPlayer = true;
    createAudio.HwDevice = true;
    createAudio.interface = TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY;

    return CreateStream(&createAudio);
}

/**
 * Read header info and detect file format
 */
static ssize_t ReadHeader
(
    int fd, void* bufPtr, size_t bufSize
)
{
    TAF_ERROR_IF_RET_VAL((bufPtr == NULL) || (fd < 0) , -1,"Supplied NULL string pointer / invalid file descriptor");

    int bytesRead = 0, tempBufSize = 0, readReq = bufSize;
    char *Str;

    if (bufSize == 0)
    {
        return tempBufSize;
    }

    do
    {
        Str = (char *)(bufPtr);
        Str = Str + tempBufSize;

        bytesRead = read(fd, Str, readReq);

        if ((bytesRead < 0) && (errno != EINTR) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
        {
            LE_ERROR("Error while reading file, errno: %d (%m)", errno);
            return bytesRead;
        }
        else
        {
            if (bytesRead == 0)
            {
                return tempBufSize;
            }

            tempBufSize += bytesRead;

            if (tempBufSize < (int)bufSize)
            {
                readReq = bufSize - tempBufSize;
            }
        }
    }
    while (tempBufSize < (int)bufSize);

    return tempBufSize;
}

/**
 * Play the file
 */
static void* Play( void* ctxPtr) {

    auto &audio = taf_Audio::GetInstance();
    uint32_t numBytes =0;
    uint32_t size = 0;

    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)ctxPtr;

    if(audio.mAudioPlayStream) {
        while(!audio.mFreeBuffers.empty()) {
            audio.mFreeBuffers.pop();
        }

        if(audio.mFile) {
            fseek(audio.mFile, 0, SEEK_SET);
        } else {
            LE_ERROR("Unable to read file");
            return NULL;
        }

        for(int i = 0; i < TOTAL_BUFFERS; i++) {
            audio.mStreamBuffer = audio.mAudioPlayStream->getStreamBuffer();

            if(audio.mStreamBuffer != nullptr) {
                size = audio.mStreamBuffer->getMinSize();
                if(size == 0) {
                    size =  audio.mStreamBuffer->getMaxSize();
                }
                audio.mStreamBuffer->setDataSize(size);
                audio.mFreeBuffers.push(audio.mStreamBuffer);
            } else {
                LE_DEBUG( "Failed to get Stream Buffer ");
                return NULL;
            }
        }

        audio.mIsPlaying = true;
        audio.mEmptyPipeline = true;

        LE_INFO( "Audio playback started" );

        taf_audio_StreamEvent_t streamEvent;
        streamEvent.streamPtr = streamPtr;
        streamEvent.streamEvent = TAF_AUDIO_BITMASK_MEDIA_EVENT;

        while (!feof(audio.mFile) && audio.mIsPlaying)
        {
            if(!audio.mFreeBuffers.empty() && (audio.mEmptyPipeline)) {
                audio.mStreamBuffer = audio.mFreeBuffers.front();
                audio.mFreeBuffers.pop();
                numBytes = fread(audio.mStreamBuffer->getRawBuffer(),1,size,audio.mFile);

                if(numBytes != size && !feof(audio.mFile)) {
                    LE_DEBUG( "Unable to read specified bytes, bytes read: %d", numBytes);
                    audio.mStreamBuffer->reset();
                    audio.mFreeBuffers.push(audio.mStreamBuffer);
                    audio.mIsPlaying = false;
                    break;
                }

                audio.mStreamBuffer->setDataSize(numBytes);
                Status status = audio.mAudioPlayStream->write(audio.mStreamBuffer, audio.WriteCallback);

                if(status != telux::common::Status::SUCCESS) {
                    LE_DEBUG( "Request to write to stream failed.");
                } else {
                    LE_DEBUG( "Request to write to stream sent.");
                }
            } else {
                le_sem_Wait(audio.mSemRef);
            }
        }

        if (audio.mFileFormat == AudioFormat::PCM_16BIT_SIGNED) {
            while(audio.mFreeBuffers.size() != TOTAL_BUFFERS) {
                le_sem_Wait(audio.mSemRef);
            }
        } else if ((audio.mFileFormat == AudioFormat::AMRWB_PLUS) ||
                (audio.mFileFormat == AudioFormat::AMRWB) ||
                (audio.mFileFormat == AudioFormat::AMRNB)){
            std::promise<bool> p;

            auto status = audio.mAudioPlayStream->stopAudio(
                    StopType::STOP_AFTER_PLAY, [&p](telux::common::ErrorCode error) {
                    if (error == telux::common::ErrorCode::SUCCESS) {
                    p.set_value(true);
                    } else {
                    p.set_value(false);
                    LE_DEBUG("Failed to stop after playing buffers" );
                    }
                    });
            if(status == telux::common::Status::SUCCESS){
                LE_DEBUG("Request to stop playback after pending buffers Sent" );
            } else {
                LE_DEBUG("Request to stop playback after pending buffers failed" );
            }
            if (p.get_future().get()) {
                LE_DEBUG("Pending buffers played successfully" );
            }
        }

        if (audio.mIsPlaying){
            LE_INFO( "Play completed successfully");
        } else {
            LE_INFO("Play Stopped");
        }

        audio.mIsPlaying = false;
        fflush(audio.mFile);
        fclose(audio.mFile);
        streamEvent.event.mediaEvent = TAF_AUDIO_MEDIA_ENDED;
        le_event_Report(streamPtr->eventId, &streamEvent,
                sizeof(taf_audio_StreamEvent_t));
    }
    return NULL;
}

/**
 * Get wave file format info and start play
 */
static le_result_t PlayWave
(
    taf_audio_Stream_t* streamPtr
)
{
    auto &audio = taf_Audio::GetInstance();
    le_result_t res = LE_FAULT;
    WavHeader_t wHdr;

    if (!audio.mIsPlayStreamCreated) {
        if (ReadHeader(streamPtr->fd, &wHdr, sizeof(wHdr)) != sizeof(wHdr))
        {
            LE_WARN("WAV detection: cannot read header");
            return LE_FAULT;
        }

        if ((wHdr.riffId != ID_RIFF)  ||
                (wHdr.riffFmt != ID_WAVE) ||
                (wHdr.chunkId != ID_FMT)    ||
                (wHdr.formatTag != FORMAT_PCM) ||
                (wHdr.chunkSize != 16))
        {
            LE_WARN("WAV detection: unrecognized wav format");
            lseek(streamPtr->fd, -sizeof(wHdr), SEEK_CUR);
            return LE_FAULT;
        }

        StreamConfig config = {};
        config.type = StreamType::PLAY;
        config.sampleRate = wHdr.sampleRate;
        config.channelTypeMask = wHdr.channelsCount;
        config.format = AudioFormat::PCM_16BIT_SIGNED;

        // Set the config device type based on output device
        le_hashmap_It_Ref_t connItr =
                (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
        taf_audio_Connector_t const * currentconnPtr;
        taf_audio_Stream_t const * outStreamPtr;
        le_hashmap_It_Ref_t strmItr;
        while (le_hashmap_NextNode(connItr)==LE_OK)
        {
            currentconnPtr = (taf_audio_Connector_t const *)le_hashmap_GetValue(connItr);
            strmItr = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(currentconnPtr->audioOutList);
            while (le_hashmap_NextNode(strmItr)==LE_OK) {
                outStreamPtr = (taf_audio_Stream_t const *)le_hashmap_GetValue(strmItr);
                if (outStreamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER){
                    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
                    LE_DEBUG("set config with device type speaker");
                }
                else if (outStreamPtr->interface == TAF_AUDIO_IF_PCM_SPEAKER) {
                    config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_HEADSET_SPEAKER);
                    LE_DEBUG("set config with device type headset speaker");
                }
                else if (outStreamPtr->interface == TAF_AUDIO_IF_I2S_SPEAKER) {
                    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
                    LE_DEBUG("set config with device type speaker");
                }
            }
        }
        audio.mFileFormat = config.format;
        res = audio.StartAudio(config);
        TAF_ERROR_IF_RET_VAL( (res != LE_OK), LE_FAULT," Config failed");
        audio.mIsPlayStreamCreated = true;
        return LE_OK;
    } else {
        return LE_OK;
    }

    return LE_FAULT;
}

/**
 * Get AMR file format info and Play
 */
static le_result_t PlayAmr
(
    taf_audio_Stream_t* streamPtr
)
{
    auto &audio = taf_Audio::GetInstance();
    le_result_t res = LE_FAULT;
    taf_audio_FileFormat_t format = TAF_AUDIO_FILE_MAX;


    if (!audio.mIsPlayStreamCreated) {
        StreamConfig config = {};
        config.type = StreamType::PLAY;
        char header[10] = {0};

        if ( read(streamPtr->fd, header, 9) != 9 )
        {
            LE_WARN("AMR detection: cannot read header");
            return LE_FAULT;
        }
        if ( strncmp(header, "#!AMR", 5) == 0 )
        {
            format = TAF_AUDIO_FILE_MAX;

            if ( strncmp(header+5, "-WB\n", 4) == 0 )
            {
                LE_DEBUG("AMR-WB Detected");
                format = TAF_AUDIO_FILE_AMR_WB;
            }
            else if ( strncmp(header+5, "-NB\n", 4) == 0 )
            {
                LE_DEBUG("AMR-NB Detected");
                format = TAF_AUDIO_FILE_AMR_NB;
            }
            else if ( strncmp(header+5, "\n", 1) == 0 )
            {
                LE_DEBUG("AMR-NB Detected");
                format = TAF_AUDIO_FILE_AMR_NB;
                lseek(streamPtr->fd, -3, SEEK_CUR);
            }
            else
            {
                LE_ERROR("Not an AMR file");
                return LE_FAULT;
            }
            if (format == TAF_AUDIO_FILE_AMR_WB)
            {
                config.format = AudioFormat::AMRWB;
                config.sampleRate = 16000;
            }
            else
            {
                config.format = AudioFormat::AMRNB;
                config.sampleRate = 8000;
            }

            config.channelTypeMask = 1;

            // Set the config device type based on output device
            le_hashmap_It_Ref_t connItr =
                    (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
            taf_audio_Connector_t const * currentconnPtr;
            taf_audio_Stream_t const * outStreamPtr;
            le_hashmap_It_Ref_t strmItr;
            while (le_hashmap_NextNode(connItr)==LE_OK)
            {
                currentconnPtr = (taf_audio_Connector_t const *)le_hashmap_GetValue(connItr);
                strmItr = (le_hashmap_It_Ref_t)
                        le_hashmap_GetIterator(currentconnPtr->audioOutList);
                while (le_hashmap_NextNode(strmItr)==LE_OK) {
                    outStreamPtr = (taf_audio_Stream_t const *)le_hashmap_GetValue(strmItr);
                    if (outStreamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER){
                        config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
                        LE_DEBUG("set config with device type speaker");
                    }
                    else if (outStreamPtr->interface == TAF_AUDIO_IF_PCM_SPEAKER) {
                        config.deviceTypes.emplace_back((DeviceType)DEVICE_TYPE_HEADSET_SPEAKER);
                        LE_DEBUG("set config with device type headset speaker");
                    }
                    else if (outStreamPtr->interface == TAF_AUDIO_IF_I2S_SPEAKER) {
                        config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
                        LE_DEBUG("set config with device type speaker");
                    }
                }
            }
            audio.mFileFormat = config.format;
            res = audio.StartAudio(config);
            TAF_ERROR_IF_RET_VAL( (res != LE_OK), LE_FAULT," Config failed");
            audio.mIsPlayStreamCreated = true;
            return LE_OK;
        }
    } else {
            return LE_OK;
    }
    return LE_FAULT;
}

/**
 * Create stream for the given playback and start Audio
 */
le_result_t taf_Audio::PlayFile
(
 taf_audio_StreamRef_t  streamRef,
 int fd
)
{
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(AudioRefMap, streamRef);
    TAF_KILL_CLIENT_IF_RET_VAL((streamPtr == NULL) || (fd < TAF_AUDIO_NO_FD) || (fd == 0), LE_FAULT, "Invalid reference");
    le_result_t res = LE_FAULT;

    if (( fd != TAF_AUDIO_NO_FD ) && ( streamPtr->fd != fd ))
    {
        LE_DEBUG("close previous streamPtr->fd.%d of interface.%d",
                 streamPtr->fd, streamPtr->interface);

        close(streamPtr->fd);
        streamPtr->fd = fd;
    }

    res = PlayWave(streamPtr);

    if (res != LE_OK)
    {
        res = PlayAmr(streamPtr);

        if (res != LE_OK) {
            LE_INFO( "Use Default Config ");
            if (!mIsPlayStreamCreated) {
                auto &audio = taf_Audio::GetInstance();
                StreamConfig config = {};
                config.type = StreamType::PLAY;
#ifdef TARGET_SA515M
                config.slotId = DEFAULT_SLOT_ID;
#endif
                config.format = AudioFormat::PCM_16BIT_SIGNED;
                config.sampleRate = DEFAULT_SAMPLERATE;
                config.channelTypeMask = ChannelType::LEFT;
                audio.mFileFormat = config.format;

                // Set the config device type based on output device
                le_hashmap_It_Ref_t connItr =
                        (le_hashmap_It_Ref_t)le_hashmap_GetIterator(streamPtr->connList);
                taf_audio_Connector_t const * currentconnPtr;
                taf_audio_Stream_t const * outStreamPtr;
                le_hashmap_It_Ref_t strmItr;
                while (le_hashmap_NextNode(connItr)==LE_OK)
                {
                    currentconnPtr = (taf_audio_Connector_t const *)le_hashmap_GetValue(connItr);
                    strmItr = (le_hashmap_It_Ref_t)
                            le_hashmap_GetIterator(currentconnPtr->audioOutList);
                    while (le_hashmap_NextNode(strmItr)==LE_OK) {
                        outStreamPtr = (taf_audio_Stream_t const *)le_hashmap_GetValue(strmItr);
                        if (outStreamPtr->interface == TAF_AUDIO_IF_CODEC_SPEAKER){
                            config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
                            LE_DEBUG("set config with device type speaker");
                        }
                        else if (outStreamPtr->interface == TAF_AUDIO_IF_PCM_SPEAKER) {
                            config.deviceTypes
                                    .emplace_back((DeviceType)DEVICE_TYPE_HEADSET_SPEAKER);
                            LE_DEBUG("set config with device type headset speaker");
                        }
                        else if (outStreamPtr->interface == TAF_AUDIO_IF_I2S_SPEAKER) {
                            config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
                            LE_DEBUG("set config with device type speaker");
                        }
                    }
                }
                res = StartAudio(config);
                TAF_ERROR_IF_RET_VAL( (res != LE_OK), LE_FAULT," Config failed");
                mIsPlayStreamCreated = true;
            }
        }
    }

    mFile = fdopen( streamPtr->fd, "r");
    le_thread_Start(le_thread_Create("PlayThread", Play, streamPtr));

    return LE_OK;
}

/**
 * Stop active audio stream
 */
le_result_t taf_Audio::Stop
(
 taf_audio_StreamRef_t  streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(audio.AudioRefMap, streamRef);

    TAF_KILL_CLIENT_IF_RET_VAL((streamPtr == NULL), LE_FAULT, "Invalid reference");

    le_result_t res = audio.StopAudio();
    TAF_ERROR_IF_RET_VAL( (res != LE_OK), LE_FAULT,"Stop failed");
    return LE_OK;
}

/**
 * Set volume for active stream
 */
le_result_t taf_Audio::SetVolume
(
    taf_audio_StreamRef_t streamRef,
    int32_t gain
)
{
    auto &audio = taf_Audio::GetInstance();
    resetCallbackPromise();
    Status status = Status::FAILED;
    StreamVolume streamVol;
    ChannelVolume channelVol;
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(audio.AudioRefMap, streamRef);

    TAF_KILL_CLIENT_IF_RET_VAL((streamPtr == NULL), LE_BAD_PARAMETER, "Invalid reference");


    channelVol.channelType = ChannelType::LEFT;
    channelVol.vol = gain / 100;

    //streamVol.dir = StreamDirection::RX;
    channelVol.channelType = ChannelType::LEFT;
    streamVol.volume.emplace_back(channelVol);
    LE_DEBUG("channeltype:%d Volume: %d ",uint32_t(channelVol.channelType), uint32_t(channelVol.vol));

    if (audio.mAudioVoiceStream) {
        status = audio.mAudioVoiceStream->setVolume(streamVol, audio.setStreamVolumeCallback);
    } else if (audio.mAudioVoiceStream2) {
        status = audio.mAudioVoiceStream2->setVolume(streamVol, audio.setStreamVolumeCallback);
    } else if (audio.mAudioPlayStream) {
        status = audio.mAudioPlayStream->setVolume(streamVol, audio.setStreamVolumeCallback);
    } else if (audio.mAudioCaptureStream) {
        streamVol.dir = StreamDirection::TX;
        status = audio.mAudioCaptureStream->setVolume(streamVol, audio.setStreamVolumeCallback);
    }
    if(status == Status::SUCCESS) {
        ErrorCode error = audio.gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS != error) {
            LE_ERROR("Set volume failed error: %d", int(error));
        }
    } else {
        LE_ERROR("Set volume failed error: %d", int(status));
    }

    return LE_OK;
}

/**
 * Get volume from active streams
 */
le_result_t taf_Audio::GetVolume
(
    taf_audio_StreamRef_t streamRef,
    int32_t *gainPtr
)
{
    auto &audio = taf_Audio::GetInstance();
    auto status = Status::FAILED;
    resetCallbackPromise();
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(audio.AudioRefMap, streamRef);

    TAF_KILL_CLIENT_IF_RET_VAL((streamPtr == NULL) || (gainPtr == NULL), LE_BAD_PARAMETER, "Invalid reference");

    if (audio.mAudioVoiceStream) {
        status = audio.mAudioVoiceStream->getVolume(StreamDirection::RX, audio.getStreamVolumeCallback);
    } else if (audio.mAudioVoiceStream2) {
        status = audio.mAudioVoiceStream2->getVolume(StreamDirection::RX, audio.getStreamVolumeCallback);
    } else if (audio.mAudioPlayStream) {
        status = audio.mAudioPlayStream->getVolume(StreamDirection::RX, audio.getStreamVolumeCallback);
    } else if (audio.mAudioCaptureStream) {
        status = audio.mAudioCaptureStream->getVolume(StreamDirection::TX, audio.getStreamVolumeCallback);
    }
    if(status == Status::SUCCESS) {
        ErrorCode error = audio.gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS != error) {
            LE_ERROR("Get volume failed error: %d", int(error));
        }
        else {
            int i = 0;
            for (auto channel_volume : audio.mVol.volume) {
                LE_DEBUG("ChannelVolume [%d] channeltype:%d Volume: %d ",i
                        ,uint32_t(channel_volume.channelType), uint32_t(channel_volume.vol));
                *gainPtr = int (channel_volume.vol * 100);
            }
        }
    }
    else {
        LE_ERROR("Get volume failed error: %d", int(status));
    }
    streamPtr->gain = *gainPtr;
    return LE_OK;
}

/**
 * Enable Noise Suppressor
 */
le_result_t taf_Audio::EnableNoiseSuppressor
(
 taf_audio_StreamRef_t  streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(audio.AudioRefMap, streamRef);

    TAF_KILL_CLIENT_IF_RET_VAL((streamPtr == NULL), LE_BAD_PARAMETER, "Invalid reference");
    LE_INFO("Feature Not supported");

    return LE_OK;
}

/**
 * Enable Echo Canceller
 */
le_result_t taf_Audio::EnableEchoCanceller
(
 taf_audio_StreamRef_t  streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(audio.AudioRefMap, streamRef);

    TAF_KILL_CLIENT_IF_RET_VAL((streamPtr == NULL), LE_BAD_PARAMETER, "Invalid reference");

    streamPtr->echoCancellerEnabled = true;
    return LE_OK;
}

/**
 * Disable Noise Suppressor
 */
le_result_t taf_Audio::DisableNoiseSuppressor
(
 taf_audio_StreamRef_t  streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(audio.AudioRefMap, streamRef);

    TAF_KILL_CLIENT_IF_RET_VAL((streamPtr == NULL), LE_BAD_PARAMETER, "Invalid reference");
    LE_INFO("Feature Not supported");

    return LE_OK;
}

/**
 * Disable Echo Canceller
 */
le_result_t taf_Audio::DisableEchoCanceller
(
 taf_audio_StreamRef_t  streamRef
)
{
    auto &audio = taf_Audio::GetInstance();
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(audio.AudioRefMap, streamRef);

    TAF_KILL_CLIENT_IF_RET_VAL((streamPtr == NULL), LE_BAD_PARAMETER, "Invalid reference");

    streamPtr->echoCancellerEnabled = false;
    return LE_OK;
}

/**
 * Get the status of Noise Suppressor
 */
le_result_t taf_Audio::IsNoiseSuppressorEnabled
(
 taf_audio_StreamRef_t  streamRef,
 bool* status
)
{
    auto &audio = taf_Audio::GetInstance();
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(audio.AudioRefMap, streamRef);

    TAF_KILL_CLIENT_IF_RET_VAL((streamPtr == NULL) || (status == NULL), LE_BAD_PARAMETER, "Invalid reference");
    LE_INFO("Feature Not supported");

    return LE_OK;
}

/**
 * Get the status of EchoCanceller
 */
le_result_t taf_Audio::IsEchoCancellerEnabled
(
 taf_audio_StreamRef_t  streamRef,
 bool* status
)
{
    auto &audio = taf_Audio::GetInstance();
    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(audio.AudioRefMap, streamRef);

    TAF_KILL_CLIENT_IF_RET_VAL((streamPtr == NULL) || (status == NULL), LE_BAD_PARAMETER, "Invalid reference");

    streamPtr->echoCancellerEnabled = *status;
    return LE_OK;
}


/**
 * Initialize audio streams
 */
void taf_Audio::Init(void)
{
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();

    auto &audioFactory = AudioFactory::getInstance();

    mAudioManager = audioFactory.getAudioManager();
    bool isReady = false;
    if (mAudioManager) {
        isReady = mAudioManager->isSubsystemReady();
    } else {
        LE_ERROR("Invalid Audio Manager");
        return;
    }

    if (!isReady) {
        LE_INFO("Audio subsystem is not ready, Please wait ...");
        std::future<bool> f = mAudioManager->onSubsystemReady();
        isReady = f.get();
    }

    if (isReady) {
        endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        LE_INFO("Elapsed Time for Audio Subsystems to ready : %f", elapsedTime.count());
    } else {
        LE_ERROR(" *** ERROR - Unable to initialize audio subsystem");
        return;
    }

    // Instantiate Listener
    mAudioListener = std::make_shared<tafAudioListener>();
    auto status = mAudioManager->registerListener(mAudioListener);
    if(status != telux::common::Status::SUCCESS) {
        LE_INFO("Audio Listener Registeration failed");
    }

    mVoiceListener = std::make_shared<tafVoiceListener>();
    mPlayListener = std::make_shared<tafPlayListener>();

    AudioPool  = le_mem_InitStaticPool(tafAudio,MAX_STREAM,sizeof(taf_audio_Stream_t));
    le_mem_SetDestructor(AudioPool,ReleaseStream);
    AudioConnPool  = le_mem_InitStaticPool(tafAudioConnector,MAX_CONNECTOR,sizeof(taf_audio_Connector_t));
    HashMapPool  = le_mem_InitStaticPool(tafAudioHashmap,HASHMAP_SIZE,sizeof(struct hashMapList));
    SessionRefPool = le_mem_InitStaticPool(tafSessionRef,MAX_STREAM,sizeof(taf_SessionRef_t));
    EventIdPool = le_mem_InitStaticPool(tafEventIdPool, MAX_STREAM, sizeof(struct tafEventIdList));
    EventHandlerRefNodePool = le_mem_InitStaticPool(tafEventHandlerRef,MAX_CONNECTOR,sizeof(EventHandlerRefNode_t));
    AudioRefMap = le_ref_CreateMap("TAFAudioStreamMap", MAX_STREAM);
    AudioConnRefMap = le_ref_CreateMap("TAFAudioConMap", MAX_CONNECTOR);
    EventHandlerRefMap = le_ref_CreateMap("TAFEventHandlerMap", MAX_CONNECTOR);
    HashMapList = LE_DLS_LIST_INIT;
    mSemRef = le_sem_Create("tafSem", 0);
}

/**
 * Returns audio instance
 */
taf_Audio &taf_Audio::GetInstance()
{
    static taf_Audio instance;
    return instance;
}
