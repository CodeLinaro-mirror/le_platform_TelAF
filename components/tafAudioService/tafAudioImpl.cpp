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

// Mutex used to prevent races between the threads.
static le_mutex_Ref_t tafMutex;

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
                LE_DEBUG("audio started successfully.\n");
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

void RegisterDtmfListenerCallback(ErrorCode error)
{
        if (error != ErrorCode::SUCCESS) {
                LE_DEBUG("registerListener() failed with error %d", int(error));
        }
        LE_DEBUG("registerListener() succeeded.");
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
                return;
        }
        audio.gCallbackPromise.set_value(error);

        LE_DEBUG("Mute/Unmute succeeded.");
        return;
}

void taf_Audio::WriteCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer, uint32_t bytes,
                telux::common::ErrorCode error)
{
        auto &audio = taf_Audio::GetInstance();
        if (ErrorCode::SUCCESS == error) {
                LE_DEBUG( "write() succeeded");
        } else {
                LE_DEBUG( "write failed with error code %d", int(error) );
        }
        buffer->reset();
        le_mutex_Unlock(tafMutex);
        audio.gCallbackPromise.set_value(error);
        return;
}

void taf_Audio::DeletePlayCallback(ErrorCode error) {
        auto &audio = taf_Audio::GetInstance();
        if (ErrorCode::SUCCESS == error) {
                LE_DEBUG("deletePlayStream() succeeded.");
                audio.mAudioPlayStream.reset();
                audio.mAudioPlayStream = nullptr;
        }
        audio.gCallbackPromise.set_value(error);
        return;
}

void taf_Audio::ReadCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
                telux::common::ErrorCode error)
{
        auto &audio = taf_Audio::GetInstance();
   if (ErrorCode::SUCCESS == error) {
        LE_DEBUG( "Read succeeded");
    } else {
        LE_DEBUG( "Read failed with error code %d", int(error) );
    }
    buffer->reset();
    le_mutex_Unlock(tafMutex);
    audio.gCallbackPromise.set_value(error);
    return;
}

void taf_Audio::DeleteCaptureCallback(ErrorCode error) {
         auto &audio = taf_Audio::GetInstance();
        if (ErrorCode::SUCCESS == error) {
                LE_DEBUG("deleteCaptureStream() succeeded.");
                audio.mAudioCaptureStream.reset();
                audio.mAudioCaptureStream = nullptr;
        }
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
        LE_DEBUG("Create and Start voice audio\n");
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
                        LE_ERROR("failed to Create a stream");
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
                } else if(tafAudioStream->getType() == StreamType::CAPTURE) {
                        mAudioCaptureStream = std::dynamic_pointer_cast<
                                telux::audio::IAudioCaptureStream>(tafAudioStream);
                        LE_DEBUG("Audio Capture Stream is Created" );
                } else {
                        LE_DEBUG("Unkonw Stream Created" );
                }
        }

        if(mAudioVoiceStream && (config.slotId == SLOT_ID_1) && !mVoiceEnabled1) {
                status = mAudioVoiceStream->startAudio(StartAudioCallback);
        }

        if(mAudioVoiceStream2 && (config.slotId == SLOT_ID_2) && !mVoiceEnabled2) {
                status = mAudioVoiceStream2->startAudio(StartAudioCallback);
        } else if(mAudioPlayStream) {
                mStreamBuffer = mAudioPlayStream->getStreamBuffer();
                if(mStreamBuffer != nullptr) {
                        mSize = mStreamBuffer->getMinSize();
                        if(mSize == 0) {
                                mSize =  mStreamBuffer->getMaxSize();
                        }
                        mStreamBuffer->setDataSize(mSize);
                } else {
                        LE_ERROR( "Failed to get Stream Buffer ");
                        return LE_FAULT;
                }
                memset(mStreamBuffer->getRawBuffer(),0,mSize);
                status = mAudioPlayStream->write(mStreamBuffer,WriteCallback);
                if(status != telux::common::Status::SUCCESS) {
                        LE_DEBUG( "Request to write to stream failed.");
                } else {
                        LE_DEBUG( "Request to write to stream sent.");
                        le_mutex_Lock(tafMutex);
                }
        } else if(mAudioCaptureStream) {
                mStreamBuffer = mAudioCaptureStream->getStreamBuffer();
                if(mStreamBuffer != nullptr) {
                        mSize = mStreamBuffer->getMinSize();
                        if(mSize == 0) {
                                mSize =  mStreamBuffer->getMaxSize();
                        }
                } else {
                        LE_ERROR( "Failed to get Stream Buffer ");
                        return LE_FAULT;
                }
                status = mAudioCaptureStream->read(mStreamBuffer, mSize, ReadCallback);
                if(status != telux::common::Status::SUCCESS) {
                        LE_DEBUG( "Request to read to stream failed.");
                } else {
                        LE_DEBUG( "Request to read to stream sent.");
                        le_mutex_Lock(tafMutex);
                }
        }
        if (status == Status::SUCCESS) {
                LE_DEBUG("Request to start voice stream sent.\n");
                ErrorCode error = gCallbackPromise.get_future().get();
                if (ErrorCode::SUCCESS != error) {
                        LE_ERROR("Request to start failed");
                        return LE_FAULT;
                }
        } else {
                LE_ERROR("Request to start voice stream failed.\n");
                return LE_FAULT;
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
        LE_DEBUG("Stop voice stream\n");
        resetCallbackPromise();
        auto status = Status::FAILED;
        if (mAudioVoiceStream && (mSlotId == SLOT_ID_1) && mVoiceEnabled1) {
                status = mAudioVoiceStream->stopAudio(StopAudioCallback);
        }

        if (mAudioVoiceStream2 && (mSlotId == SLOT_ID_2)&& mVoiceEnabled2) {
                status = mAudioVoiceStream2->stopAudio(StopAudioCallback);
        } else  if (mAudioPlayStream) {
                status = mAudioPlayStream->stopAudio(StopType::FORCE_STOP,StopAudioCallback);
        } else  if (mAudioCaptureStream) {
                LE_DEBUG("mAudioCaptureStream no API for Stop ");
        }
        if (status == Status::SUCCESS) {
                LE_DEBUG("Audio is disabled for call ");
                ErrorCode error = gCallbackPromise.get_future().get();
                if (ErrorCode::SUCCESS != error) {
                        LE_ERROR("Request to Stop voice stream failed");
                }
        } else {
                LE_ERROR("Error in disabling audio ");
                return LE_FAULT;
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
        } else if (mAudioVoiceStream2 && (mSlotId == SLOT_ID_2)) {
                status = mAudioManager->deleteStream(mAudioVoiceStream2, DeleteVoiceCallback2);
        } else if (mAudioPlayStream) {
                status = mAudioManager->deleteStream(mAudioPlayStream, DeletePlayCallback);
        } else if (mAudioCaptureStream) {
                status = mAudioManager->deleteStream(mAudioCaptureStream, DeleteCaptureCallback);
        }
        if (status == Status::SUCCESS) {
                LE_DEBUG("Audio is disabled for call ");
                ErrorCode error = gCallbackPromise.get_future().get();
                if (ErrorCode::SUCCESS != error) {
                        LE_ERROR("Request to Stop voice stream failed");
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

    TAF_ERROR_IF_RET_NIL( connPtr == NULL, "connPtr is nullptr!");
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
        if (mModemRx && mSpeaker)
        {
                config.type = StreamType::VOICE_CALL;
#ifdef TARGET_SA515M
                config.slotId = (SlotId)mSlotId;
#endif
                config.sampleRate = 16000;
                config.format = AudioFormat::PCM_16BIT_SIGNED;
                config.channelTypeMask = ChannelType::LEFT;
                config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
        }
        res = StartAudio(config);
    }

    return res;
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
            case TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
            case TAF_AUDIO_IF_CODEC_MIC:
            case TAF_AUDIO_IF_CODEC_SPEAKER:
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

/**
 * Release the connecter
 */
void taf_Audio::ReleaseStream( void *objPtr )
{
    LE_ASSERT(objPtr);
    auto &audio = taf_Audio::GetInstance();

    taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)objPtr;

    audio.DisconnectConnectors (streamPtr);

    le_hashmap_RemoveAll(streamPtr->connList);
    audio.ClearHashMap(streamPtr->connList);
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
    DeleteStream(streamPtr, taf_audio_GetClientSessionRef(), false);
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
        taf_audio_Stream_t* streamPtr = (taf_audio_Stream_t*)le_ref_Lookup(AudioRefMap, streamRef);

        TAF_ERROR_IF_RET_VAL( streamPtr == NULL, LE_BAD_PARAMETER,"streamPtr is nullptr!");

        if (mAudioVoiceStream && (mSlotId == SLOT_ID_1)) {
                mute.dir = StreamDirection::TX;
                status = mAudioVoiceStream->setMute(mute, StreamMuteUnmuteCallback);
        } else if (mAudioVoiceStream2 && (mSlotId == SLOT_ID_2)) {
                mute.dir = StreamDirection::TX;
                status = mAudioVoiceStream2->setMute(mute, StreamMuteUnmuteCallback);
        } else if (mAudioPlayStream) {
                mute.dir = StreamDirection::RX;
                status = mAudioPlayStream->setMute(mute, StreamMuteUnmuteCallback);
        } else if (mAudioCaptureStream) {
                mute.dir = StreamDirection::TX;
                status = mAudioCaptureStream->setMute(mute, StreamMuteUnmuteCallback);
        }
        if (status == Status::SUCCESS) {
                LE_DEBUG("Request Mute.\n");
                ErrorCode error = gCallbackPromise.get_future().get();
                if (ErrorCode::SUCCESS != error) {
                        if (mute.enable) {
                                LE_ERROR("Request to Mute failed");
                        } else {
                                LE_ERROR("Request to UnMute failed");
                        }
                        return LE_FAULT;
                }
        } else {
                if (mute.enable) {
                        LE_ERROR("Request to Mute failed");
                } else {
                        LE_ERROR("Request to UnMute failed");
                }
                return LE_FAULT;
        }
        TAF_ERROR_IF_RET_VAL( res != LE_OK, res, "Mute/Unmute Failed");

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
    //uint32_t sampleDuration = 1000;
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
 * Initialize audio streams
 */
void taf_Audio::Init(void)
{
        auto &audioFactory = AudioFactory::getInstance();

        mAudioManager = audioFactory.getAudioManager();
        AudioPool  = le_mem_InitStaticPool(tafAudio,MAX_STREAM,sizeof(taf_audio_Stream_t));
        le_mem_SetDestructor(AudioPool,ReleaseStream);
        AudioConnPool  = le_mem_InitStaticPool(tafAudioConnector,MAX_CONNECTOR,sizeof(taf_audio_Connector_t));
        HashMapPool  = le_mem_InitStaticPool(tafAudioHashmap,HASHMAP_SIZE,sizeof(struct hashMapList));
        SessionRefPool = le_mem_InitStaticPool(tafSessionRef,MAX_STREAM,sizeof(taf_SessionRef_t));
        AudioRefMap = le_ref_CreateMap("TAFAudioStreamMap", MAX_STREAM);
        AudioConnRefMap = le_ref_CreateMap("TAFAudioConMap", MAX_CONNECTOR);
        HashMapList = LE_DLS_LIST_INIT;
        // Instantiate voiceListener
        mVoiceListener = std::shared_ptr<tafVoiceListener>();
        tafMutex = le_mutex_CreateNonRecursive("tafMutex");
}

/**
 * Returns audio instance
 */
taf_Audio &taf_Audio::GetInstance()
{
        LE_DEBUG("Audio Subsystem GetInstance.\n");
        static taf_Audio instance;
        return instance;
}
