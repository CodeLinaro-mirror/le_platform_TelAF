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
#include <queue>
#include <string>
#include <memory>
#include <vector>
#include <telux/audio/AudioFactory.hpp>
#include <telux/audio/AudioManager.hpp>
#include <telux/audio/AudioListener.hpp>
#include <telux/audio/AudioDefines.hpp>
#include "telux/common/CommonDefines.hpp"
#include "tafSvcIF.hpp"

using namespace telux::common;
using namespace telux::audio;

#define TIMEOUT                    5
#define DEFAULT_STREAMPOOL_SIZE    1
#define MAX_STREAM                 6
#define HASHMAP_SIZE               10
#define MAX_CONNECTOR              8
#define TOTAL_BUFFERS              2
#define BITS_PER_SAMPLE            16
#define DEFAULT_SAMPLERATE         48000

#define CHECK_OUTPUT_IF(interface)     (   (interface == TAF_AUDIO_IF_CODEC_SPEAKER) || \
        (interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX) || \
        (interface == TAF_AUDIO_IF_PCM_SPEAKER) || \
        (interface == TAF_AUDIO_IF_I2S_SPEAKER) || \
        (interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE) \
        )

typedef struct StreamEventHandlerRef* StreamEventHandlerRef_t;
typedef struct taf_audio_DtmfStreamEventHandlerRef* taf_audio_DtmfStreamEventHandlerRef_t;

typedef struct
{
    taf_audio_StreamRef_t  streamRef;
    le_dls_Link_t          RefNodeLink;
} taf_StreamRefNode_t;

typedef enum
{
    TAF_AUDIO_BITMASK_MEDIA_EVENT = 0x1,
    TAF_AUDIO_BITMASK_DTMF_DETECTION = 0x02
}
taf_audio_StreamEventBitMask_t;

typedef struct
{
    le_msg_SessionRef_t sessionRef;
    le_dls_Link_t       refLink;
} taf_SessionRef_t;

typedef struct
{
    le_msg_SessionRef_t sessionRef;
    le_dls_Link_t       refNodeLink;
}
taf_SessionRefNode_t;

typedef struct taf_audio_Connector {
    le_hashmap_Ref_t        audioInList;
    le_hashmap_Ref_t        audioOutList;
    le_msg_SessionRef_t     sessionRef;
    taf_audio_ConnectorRef_t connRef;
    le_dls_Link_t           connLink;
}
taf_audio_Connector_t;

typedef enum
{
    TAF_AUDIO_IF_CODEC_MIC,
    TAF_AUDIO_IF_CODEC_SPEAKER,
    TAF_AUDIO_IF_PCM_MIC,
    TAF_AUDIO_IF_PCM_SPEAKER,
    TAF_AUDIO_IF_I2S_MIC,
    TAF_AUDIO_IF_I2S_SPEAKER,
    TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,
    TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX,
    TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY,
    TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE,
    TAF_AUDIO_NUM_INTERFACES
}
taf_audio_If_t;

struct tafEventIdList {
    le_event_Id_t  eventId;
    bool           inUse;
    le_dls_Link_t  next;
};

/**
 * Audio format.
 */
typedef enum
{
    TAF_AUDIO_FILE_WAVE,
    TAF_AUDIO_FILE_AMR_NB,
    TAF_AUDIO_FILE_AMR_WB,
    TAF_AUDIO_FILE_MAX
}
taf_audio_FileFormat_t;

typedef struct
{
    uint32_t sampleRate;
    uint16_t channelsCount;
    uint16_t bitsPerSample;
    uint32_t byteRate;
}
taf_audio_SamplePcmConfig_t;

/**
 * Wave header file structure.
 */
typedef struct {
    uint32_t riffId;
    uint32_t riffSize;
    uint32_t riffFmt;
    uint32_t chunkId;
    uint32_t chunkSize;
    uint16_t formatTag;
    uint16_t channelsCount;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint32_t chunkDataId;
    uint32_t chunkDataSize;
} WavHeader_t;

/**
 * Symbols used to populate wave header file.
 */
#define FORMAT_PCM 1
#define ID_RIFF    0x46464952
#define ID_WAVE    0x45564157
#define ID_FMT     0x20746d66
#define ID_DATA    0x61746164

namespace telux {
namespace tafsvc {

    class tafAudioListener : public telux::audio::IAudioListener {
        public:
            void Printfunc();
    };

    class tafVoiceListener : public telux::audio::IVoiceListener {
        public:
            virtual void onDtmfToneDetection(DtmfTone dtmfTone) override;
    };

    class tafPlayListener : public telux::audio::IPlayListener {
        public:
            void onReadyForWrite() override;
            void onPlayStopped() override;
    };

    struct hashMapList {
        le_hashmap_Ref_t    hashMapRef;
        bool                isUsed;
        le_dls_Link_t       hashMapLink;
    };

    typedef struct
    {
        taf_audio_If_t interface;
        bool            HwDevice;
        uint32_t       timeSlot;
        taf_audio_I2SChannel_t channelMode;
    }
    CreateStream_t;

    typedef struct taf_audio_Stream {
        bool             device;
        bool             playFile;
        bool echoCancellerEnabled;
        uint32_t         gain;
        int32_t          fd;
        uint32_t         timeSlot;
        taf_audio_I2SChannel_t channelMode;
        taf_audio_If_t interface;
        taf_audio_FileFormat_t format;
        taf_audio_SamplePcmConfig_t  samplePcmConfig;
        taf_audio_Format_t   encodingFormat;
        taf_audio_AmrMode_t amrMode;
        le_hashmap_Ref_t connList;
        taf_audio_StreamRef_t streamRef;
        taf_audio_DtmfStreamEventHandlerRef_t dtmfEventHandler;
        le_event_Id_t    eventId;
        le_dls_List_t    streamRefWithEventHdlrList;
        le_dls_List_t    sessionRefList;
        le_dls_Link_t  streamLink;
    }taf_audio_Stream_t;

    /**
     * Stream Event Handler Reference Node structure
     */
    typedef struct
    {
        le_event_HandlerRef_t             handlerRef;
        StreamEventHandlerRef_t           streamHandlerRef;
        taf_audio_StreamEventBitMask_t    streamEventMask;
        struct taf_audio_Stream*          streamPtr;
        void*                             userCtx;
        le_dls_Link_t                     next;
    }
    EventHandlerRefNode_t;

    typedef struct
    {
        taf_audio_Stream_t*            streamPtr;
        taf_audio_StreamEventBitMask_t streamEvent;
        union
        {
            taf_audio_MediaEvent_t     mediaEvent;
            char                       dtmf;
        } event;
    }
    taf_audio_StreamEvent_t;

    class taf_Audio : public ITafSvc {
        public:
            void Init(void);

            static taf_Audio &GetInstance();

            taf_Audio() {};
            ~taf_Audio() {};

            std::promise<telux::common::ErrorCode> gCallbackPromise;
            std::shared_ptr<telux::audio::IAudioManager> mAudioManager;
            std::shared_ptr<telux::audio::IAudioStream> mAudioStream;
            std::shared_ptr<telux::audio::IAudioVoiceStream> mAudioVoiceStream;
            std::shared_ptr<telux::audio::IAudioVoiceStream> mAudioVoiceStream2;
            std::shared_ptr<telux::audio::IAudioPlayStream> mAudioPlayStream;
            std::shared_ptr<telux::audio::IAudioCaptureStream> mAudioCaptureStream;
            std::shared_ptr<telux::audio::IStreamBuffer> mStreamBuffer;
            std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> mFreeBuffers;
            std::shared_ptr<telux::audio::IAudioListener> mAudioListener;
            std::shared_ptr<telux::audio::IPlayListener> mPlayListener;
            std::shared_ptr<telux::audio::IVoiceListener> mVoiceListener;
            std::shared_ptr<telux::audio::IAudioStream> mStream;

            bool mVoiceEnabled1 = false;
            bool mVoiceEnabled2 = false;
            bool mModemRx = false;
            bool mSpeaker = false;
            bool mModemTx = false;
            bool mMic = false;
            bool mIsPlaying = false;
            bool mIsPlayStreamCreated = false;
            bool mEmptyPipeline = false;
            bool mCallStarted = false;
            bool mIsRecording = false;
            bool mIsCaptureStreamCreated = false;
            uint32_t mSlotId;
            uint32_t mBufferRecordedTillNow;
            FILE *mFile;
            int mFd = -1;
            AudioFormat mFileFormat = AudioFormat::UNKNOWN;
            StreamVolume mVol;
            le_sem_Ref_t mSemRef;
            StreamConfig voiceStreamConfig = {};

            le_result_t StartAudio( StreamConfig config );
            le_result_t StopAudio(taf_audio_Stream_t* streamPtr);
            le_result_t DeleteAudio(taf_audio_Stream_t* streamPtr);
            le_result_t PlayDtmfTone(DtmfTone tone, uint32_t duration, uint16_t gain);

            le_mem_PoolRef_t SessionRefPool = NULL;
            le_mem_PoolRef_t AudioPool = NULL;
            le_mem_PoolRef_t AudioRefPool = NULL;
            le_mem_PoolRef_t AudioConnPool = NULL;
            le_mem_PoolRef_t HashMapPool = NULL;
            le_mem_PoolRef_t EventHandlerRefNodePool = NULL;
            le_mem_PoolRef_t EventIdPool = NULL;

            le_ref_MapRef_t AudioConnRefMap = NULL;
            le_ref_MapRef_t AudioRefMap = NULL;
            le_ref_MapRef_t EventHandlerRefMap = NULL;

            le_dls_List_t  HashMapList = LE_DLS_LIST_INIT;
            le_dls_List_t  ConnList = LE_DLS_LIST_INIT;
            le_dls_List_t  EventIdList = LE_DLS_LIST_INIT;

            static void ReleaseStream( void* objPtr );
            void InitAudio( taf_audio_Stream_t* streamPtr );
            void DeleteHashMap( taf_audio_Connector_t* connectorPtr );
            void ClearHashMap( le_hashmap_Ref_t hashMapRef );
            le_hashmap_Ref_t GetHashMap( );
            void CloseConnector( taf_audio_Connector_t*   connectorPtr);
            void DisconnectConnectors( taf_audio_Stream_t*     streamPtr);
            void DeleteStream( taf_audio_Stream_t*  streamPtr,
                    le_msg_SessionRef_t sessionRef, bool releaseRef);
            taf_audio_StreamRef_t CreateStream( CreateStream_t* streamPtr );

            taf_audio_Connector_t* CreateConnector();
            void DeleteConnector( taf_audio_ConnectorRef_t connectorRef );
            le_result_t Connect ( taf_audio_ConnectorRef_t connectorRef,
                    taf_audio_StreamRef_t    streamRef );
            void Disconnect ( taf_audio_ConnectorRef_t connectorRef,
                    taf_audio_StreamRef_t    streamRef );
            void Close(taf_audio_StreamRef_t    streamRef);
            le_result_t CreateandStart( taf_audio_Stream_t* streamPtr,
                    le_hashmap_Ref_t streamListPtr );
            le_result_t StopandDelete( taf_audio_Stream_t* streamPtr,
                    le_hashmap_Ref_t streamListPtr );
            taf_audio_StreamRef_t OpenSpeaker();
            taf_audio_StreamRef_t OpenMic();
            taf_audio_StreamRef_t OpenModemVoiceRx(uint32_t slotId);
            taf_audio_StreamRef_t OpenModemVoiceTx(uint32_t slotId);
            taf_audio_StreamRef_t OpenI2sRx(taf_audio_I2SChannel_t mode);
            taf_audio_StreamRef_t OpenI2sTx(taf_audio_I2SChannel_t mode);
            taf_audio_StreamRef_t OpenPcmRx(uint32_t timeslot);
            taf_audio_StreamRef_t OpenPcmTx(uint32_t timeslot);
            le_result_t PlayDtmf(taf_audio_StreamRef_t streamRef,
                    const char* dtmfPtr, uint32_t duration, uint32_t pause );
            void StopDtmf(taf_audio_StreamRef_t streamRef);
            le_result_t Mute(taf_audio_StreamRef_t streamRef, StreamMute mute );
            taf_audio_StreamRef_t OpenPlayer();
            taf_audio_StreamRef_t OpenRecorder();
            le_result_t PlayFile(taf_audio_StreamRef_t streamRef, int fd);
            le_result_t Stop(taf_audio_StreamRef_t streamRef);
            le_result_t SetVolume(taf_audio_StreamRef_t streamRef, int32_t gainPtr);
            le_result_t GetVolume(taf_audio_StreamRef_t streamRef, int32_t* gainPtr);
            le_result_t EnableNoiseSuppressor(taf_audio_StreamRef_t streamRef);
            le_result_t EnableEchoCanceller(taf_audio_StreamRef_t streamRef);
            le_result_t DisableNoiseSuppressor(taf_audio_StreamRef_t streamRef);
            le_result_t DisableEchoCanceller(taf_audio_StreamRef_t streamRef);
            le_result_t IsNoiseSuppressorEnabled(taf_audio_StreamRef_t streamRef, bool* status);
            le_result_t IsEchoCancellerEnabled(taf_audio_StreamRef_t streamRef, bool* status);
            le_result_t SetSamplePcmSamplingRate(taf_audio_StreamRef_t streamRef, uint32_t samplingRate);
            le_result_t GetSamplePcmSamplingRate(taf_audio_StreamRef_t streamRef, uint32_t *samplingRate);
            le_result_t SetSamplePcmChannelNumber(taf_audio_StreamRef_t streamRef, uint32_t channelNum);
            le_result_t GetSamplePcmChannelNumber(taf_audio_StreamRef_t streamRef, uint32_t *channelNum);
            le_result_t SetEncodingFormat(taf_audio_StreamRef_t streamRef, taf_audio_Format_t format);
            le_result_t GetEncodingFormat(taf_audio_StreamRef_t streamRef, taf_audio_Format_t *format);
            le_result_t RecordFile(taf_audio_StreamRef_t streamRef , int32_t fd);
            taf_audio_MediaHandlerRef_t AddMediaHandler(taf_audio_StreamRef_t streamRef, taf_audio_MediaHandlerFunc_t handlerPtr,
                        void* contextPtr);
            taf_audio_DtmfDetectorHandlerRef_t AddDtmfDetectorHandler(taf_audio_StreamRef_t streamRef, taf_audio_DtmfDetectorHandlerFunc_t handlerPtr,
                        void* contextPtr);

            // Callback methods for audio stream
            static void DeleteVoiceCallback(ErrorCode error);
            static void DeleteVoiceCallback2(ErrorCode error);
            static void DeletePlayCallback(ErrorCode error);
            static void DeleteCaptureCallback(ErrorCode error);
            static void StartAudioCallback(ErrorCode error);
            static void StopAudioCallback(ErrorCode error);
            static void RegisterDtmfListenerCallback(ErrorCode error);
            static void PlayDtmfCallback(ErrorCode error);
            static void StopDtmfCallback(ErrorCode error);
            static void StreamMuteUnmuteCallback(ErrorCode error);
            static void WriteCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer, uint32_t bytes,
                    telux::common::ErrorCode error);
            static void ReadCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
                    telux::common::ErrorCode error);
            static void setStreamVolumeCallback(ErrorCode error);
            static void getStreamVolumeCallback(StreamVolume volume, ErrorCode error);
            static void FirstLayerEventHandler( void* reportPtr, void* secondLayerHandlerFunc );
            static StreamEventHandlerRef_t AddStreamEventHandler( taf_audio_Stream_t* sPtr, le_event_HandlerFunc_t handlerPtr,
                                 taf_audio_StreamEventBitMask_t streamEventBitMask, void* contextPtr );
            static void ClientSessionCloseEventHandler( le_msg_SessionRef_t sessionRef,
                    void* contextPtr);
            static void* Record( void* ctxPtr);
    };
}
}
