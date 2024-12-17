/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafHalAudio.h"
#include <queue>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <telux/audio/AudioFactory.hpp>
#include <telux/audio/AudioManager.hpp>
#include <telux/audio/AudioPlayer.hpp>
#include <telux/tel/CallManager.hpp>
#include <telux/tel/CallListener.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>

using namespace telux::common;
using namespace telux::audio;

#define SUBSYSTEM_TIMEOUT          5
#define STOP_TIMEOUT               5
#define MAX_CONNECTOR              8
#define HASHMAP_SIZE               10
#define MAX_STREAM                 6
#define MAX_ROUTE                  10
#define TOTAL_BUFFERS              2
#define DEFAULT_SAMPLERATE         48000
#define DEFAULT_BITSPERSAMPLE      16
#define DEFAULT_NUM_CHANNELS       2
#define BITS_PER_BYTE              8
#define MAX_NUM_OF_PLAYLIST        4
#define MAX_NUM_OF_PLAYBACK_FILES  8
#define MAX_DTMF_GAIN              8192
#define SECS_IN_MILLISES           1000
#define MILLISECS_IN_MICROSECS     1000
#define MAX_RESPONSE_DELAY         100
#define DEFAULT_RIFF_SIZE          sizeof(WavHeader_t) - 8 // Total file size - 8 bytes

#define DEVICE_TYPE_SINK_0   1
#define DEVICE_TYPE_SINK_1   2
#define DEVICE_TYPE_SINK_2   3
#define DEVICE_TYPE_SINK_3   4
#define DEVICE_TYPE_SINK_4   5
#define DEVICE_TYPE_SOURCE_0 257
#define DEVICE_TYPE_SOURCE_1 258
#define DEVICE_TYPE_SOURCE_2 259
#define DEVICE_TYPE_SOURCE_3 260
#define DEVICE_TYPE_SOURCE_4 261

#define CHECK_OUTPUT_IF(interface)     ((interface == TAF_AUDIO_IF_CODEC_SPEAKER_1) || \
        (interface == TAF_AUDIO_IF_CODEC_SPEAKER_2) || \
        (interface == TAF_AUDIO_IF_CODEC_SPEAKER_3) || \
        (interface == TAF_AUDIO_IF_CODEC_SPEAKER_4) || \
        (interface == TAF_AUDIO_IF_CODEC_SPEAKER_5) || \
        (interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX) || \
        (interface == TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE) \
        )

#define AUDIO_SVC_PROC_CONFIG_PATH "system:/apps/tafAudioSvc/procs/tafAudioSvc"
#define DEFAULT_MAX_FILE_BYTES 90112
#define MAX_FILE_BYTES_NODE_NAME "maxFileBytes"
#define INFINITE_TONE_DURATION 65535

/**
 * Symbols used to populate wave header file.
 */
#define FORMAT_PCM 1
#define ID_RIFF    0x46464952
#define ID_WAVE    0x45564157
#define ID_FMT     0x20746d66
#define ID_DATA    0x61746164

typedef struct StreamEventHandlerRef* StreamEventHandlerRef_t;
typedef struct taf_audio_DtmfStreamEventHandlerRef* taf_audio_DtmfStreamEventHandlerRef_t;

typedef struct taf_audio_Connector {
    le_hashmap_Ref_t              audioInList;
    le_hashmap_Ref_t              audioOutList;
    le_msg_SessionRef_t           sessionRef;
    taf_audio_ConnectorRef_t connRef;
    le_dls_Link_t                 connLink;
}
taf_audio_Connector_t;

typedef struct{
    le_hashmap_Ref_t    hashMapRef;
    bool                isUsed;
    le_dls_Link_t       hashMapLink;
}
taf_audio_hashMapList_t;

typedef enum
{
    TAF_AUDIO_IF_CODEC_MIC_1,
    TAF_AUDIO_IF_CODEC_SPEAKER_1,
    TAF_AUDIO_IF_CODEC_MIC_2,
    TAF_AUDIO_IF_CODEC_SPEAKER_2,
    TAF_AUDIO_IF_CODEC_MIC_3,
    TAF_AUDIO_IF_CODEC_SPEAKER_3,
    TAF_AUDIO_IF_CODEC_MIC_4,
    TAF_AUDIO_IF_CODEC_SPEAKER_4,
    TAF_AUDIO_IF_CODEC_MIC_5,
    TAF_AUDIO_IF_CODEC_SPEAKER_5,
    TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,
    TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX,
    TAF_AUDIO_IF_DSP_FRONTEND_FILE_PLAY,
    TAF_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE,
    TAF_AUDIO_NUM_INTERFACES
}
taf_audio_If_t;

typedef struct taf_audio_Stream {
    bool             device;
    bool             playFile;
    bool echoCancellerEnabled;
    int32_t          fd;
    uint32_t         timeSlot;
    double           volLevel;
    bool             isMute;
    taf_audio_If_t interface;
    le_hashmap_Ref_t connList;
    taf_audio_Direction_t direction;
    taf_audio_StreamRef_t streamRef;
    taf_audio_DtmfStreamEventHandlerRef_t dtmfEventHandler;
    le_event_Id_t    eventId;
    le_dls_List_t    streamRefWithEventHdlrList;
    le_dls_List_t    sessionRefList;
    le_dls_Link_t  streamLink;
}taf_audio_Stream_t;

typedef struct
{
    taf_audio_If_t interface;
    bool            HwDevice;
    taf_audio_Direction_t direction;
}
StreamConfig_t;

typedef struct
{
    le_msg_SessionRef_t sessionRef;
    le_dls_Link_t       refNodeLink;
}
taf_SessionRefNode_t;

typedef struct
{
    taf_audio_RouteId_t routeId;
    taf_audio_Mode_t mode;
    taf_audio_StreamRef_t sinkRef;
    taf_audio_StreamRef_t sourceRef;
    taf_audio_Stream* modemRxPtr;
    taf_audio_Stream* modemTxPtr;
    le_msg_SessionRef_t sessionRef;
    taf_audio_RouteRef_t routeRef;
}
taf_audio_Route_t;

typedef enum
{
    TAF_AUDIO_BITMASK_MEDIA_EVENT = 0x1,
    TAF_AUDIO_BITMASK_DTMF_DETECTION = 0x02
}
taf_audio_StreamEventBitMask_t;

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

typedef struct {
    le_event_Id_t  eventId;
    bool           inUse;
    le_dls_Link_t  next;
}tafEventIdList;

typedef struct
{
    std::string absoluteFilePath; //Absolute path of the file
    int32_t  repeat; // Defines how a file should be played. -1 = infinite loop, 0 = play once,
                     // x = repeat X times.
    StreamConfig config;
    taf_audio_Stream* streamPtr;
}taf_PlaybackFile_t;

typedef struct
{
    taf_PlaybackFile_t filesToPlay[MAX_NUM_OF_PLAYBACK_FILES]; // Array of Playback files
    uint32_t numOfFilesToPlay;                                 // Number of Playback files
    bool isPlaybackInProgress;
    le_msg_SessionRef_t sessionRef;
    le_sem_Ref_t semRef;
    le_result_t pbRes;
}taf_PbList_t;

typedef struct {
    uint16_t durationRx;
    uint32_t durationTx;
    uint32_t pause;
    int32_t slotId;
    double dtmfGain;
    std::vector<std::pair<int, int>> frequencyList{};
    const char* dtmfChars;
    le_result_t result;
}taf_Dtmf_t;

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

/*
 * Buffer type
 */
typedef enum
{
    TAF_AUDIO_PB_BUFFER,
    TAF_AUDIO_REC_BUFFER
} taf_audio_BufferType_t;

typedef struct
{
    taf_audio_BufferType_t bufferType;
    taf_audio_Stream_t*  streamPtr;
} taf_audio_BufferEvent_t;

namespace telux {
namespace tafsvc {

    class tafPromptsStatusListener : public telux::audio::IPlayListListener {
        public:
            void onPlaybackStarted() override;
            void onPlaybackStopped() override;
            void onError(telux::common::ErrorCode error, std::string file) override;
            void onFilePlayed(std::string file) override;
            void onPlaybackFinished() override;
            taf_audio_Stream_t* streamPtr;
    };

    class tafPlayListener : public telux::audio::IPlayListener {
        public:
            void onReadyForWrite() override;
            void onPlayStopped() override;
    };

    class tafVoiceListener : public telux::audio::IVoiceListener {
        public:
            virtual void onDtmfToneDetection(DtmfTone dtmfTone) override;
    };

    class tafSignallingDtmfListener : public telux::common::ICommandResponseCallback {
        public:
        tafSignallingDtmfListener(std::string commandName);
        void commandResponse(telux::common::ErrorCode error) override;

        private:
        std::string commandName_;
    };


class taf_Audio : public ITafSvc
{

    public:

        static taf_Audio &GetInstance();

        taf_Audio() {};
        ~taf_Audio() {};

        std::promise<telux::common::ErrorCode> gCallbackPromise, gDelCbPromise;
        bool mIsPlaying = false, mIsTxPlaying = false, mIsPbError = false;
        bool mEmptyPipeline = false;
        bool isRecMuteSet = false, isRxRecMuteSet = false;
        AudioFormat mPbFileFormat = AudioFormat::UNKNOWN;
        le_sem_Ref_t mPlayCompletedSemRef;
        le_dls_List_t  EventIdList = LE_DLS_LIST_INIT;
        taf_audio_StreamRef_t mDtmfAudioRef = NULL;
        le_event_Id_t bufferEventId;

        void Init(void);

        taf_audio_Connector_t* CreateConnector();
        void DeleteConnector( taf_audio_ConnectorRef_t connectorRef );
        le_result_t Connect ( taf_audio_ConnectorRef_t connectorRef,
                taf_audio_StreamRef_t streamRef );
        void Disconnect ( taf_audio_ConnectorRef_t connectorRef,
                taf_audio_StreamRef_t streamRef );
        void Close( taf_audio_StreamRef_t streamRef );
        taf_audio_StreamRef_t OpenModemVoiceRx(uint32_t slotId);
        taf_audio_StreamRef_t OpenModemVoiceTx(uint32_t slotId, bool enableEcnr);
        taf_audio_RouteRef_t OpenRoute( taf_audio_RouteId_t route,
                taf_audio_Mode_t mode, taf_audio_StreamRef_t *sinkRef,
                taf_audio_StreamRef_t *sourceRef);
        le_result_t CloseRoute( taf_audio_RouteRef_t routeRef );
        taf_audio_StreamRef_t OpenPlayer(taf_audio_Direction_t direction);
        taf_audio_StreamRef_t OpenRecorder(taf_audio_Direction_t direction);
        taf_audio_MediaHandlerRef_t AddMediaHandler(taf_audio_StreamRef_t streamRef,
                taf_audio_MediaHandlerFunc_t handlerPtr, void* contextPtr);
        taf_audio_DtmfDetectorHandlerRef_t AddDtmfDetectorHandler(taf_audio_StreamRef_t streamRef,
                taf_audio_DtmfDetectorHandlerFunc_t handlerPtr, void* contextPtr);
        void RemoveMediaHandler(taf_audio_MediaHandlerRef_t handlerRef);
        void RemoveDtmfDetectorHandler(taf_audio_DtmfDetectorHandlerRef_t handlerRef);
        le_result_t RecordFile( taf_audio_StreamRef_t streamRef, const char *srcPath);
        le_result_t Stop(taf_audio_StreamRef_t streamRef);
        le_result_t PlayFile( taf_audio_StreamRef_t streamRef, const char *srcPath);
        le_result_t setVhalRouteStatus(taf_audio_Mode_t mode, bool status);
        le_result_t PlayList( taf_audio_StreamRef_t streamRef,
                const taf_audio_PlayFileConfig_t*  playFileConfigPtr, size_t playFileConfigSize);
        le_result_t SetMute( taf_audio_StreamRef_t streamRef, bool isMute);
        le_result_t GetMute( taf_audio_StreamRef_t streamRef, bool *isMute);
        le_result_t SetVolume( taf_audio_StreamRef_t streamRef, double volLevel, bool isClient);
        le_result_t GetVolume( taf_audio_StreamRef_t streamRef, double *volLevel);
        le_result_t PlaySignallingDtmf(uint32_t slotId, const char* dtmfPtr, uint32_t duration,
                uint32_t pause);
        le_result_t PlayDtmf(taf_audio_StreamRef_t streamRef,
                const char* dtmfPtr, uint16_t duration, uint32_t pause, double gain);
        le_result_t StopDtmf(taf_audio_StreamRef_t streamRef);
        le_result_t StopSignallingDtmf(uint32_t slotId);
        char getDTMFChar(telux::audio::DtmfLowFreq lowFreq, telux::audio::DtmfHighFreq highFreq);

        private:

        std::shared_ptr<telux::audio::IAudioManager> mAudioManager;
        std::shared_ptr<telux::audio::IAudioVoiceStream> mAudioVoiceStream;
        std::shared_ptr<telux::audio::IAudioCaptureStream> mAudioCaptureStream; // local capture stream

        // incall downlink capture stream
        std::shared_ptr<telux::audio::IAudioCaptureStream> mAudioRxCaptureStream;
        std::shared_ptr<telux::audio::IAudioPlayStream> mAudioPlayStream;
        std::shared_ptr<telux::audio::IAudioLoopbackStream> mAudioLoopbackStream;
        std::shared_ptr<telux::audio::IStreamBuffer> mPbStreamBuffer, mRecStreamBuffer,
                mRxRecStreamBuffer;

        // mAudioPlayer - local/incall downlink playback, mTxAudioPlayer - incall uplink playback
        std::shared_ptr<telux::audio::IAudioPlayer> mAudioPlayer, mTxAudioPlayer;
        std::shared_ptr<telux::audio::IPlayListener> mPlayListener;
        std::shared_ptr<telux::audio::IVoiceListener> mVoiceListener;

        // local/incall downlink playback listener
        std::shared_ptr<tafPromptsStatusListener> repeatedPlayerStatusListener;
        // incall uplink playback listener
        std::shared_ptr<tafPromptsStatusListener> repeatedTxPlayerStatusListener;
        std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> mPbFreeBuffers,
                mRecFreeBuffers, mRxRecFreeBuffers;

        bool isVhalAvailable = false;
        bool isEcnrEnabled = false;
        bool mCallStarted = false;
        bool mVoiceEnabled1 = false;
        bool mDtmfStarted = false;
        bool mDtmfStartedTx = false;
        bool mModemRx = false;
        bool mSpeaker = false;
        bool mModemTx = false;
        bool mMic = false;
        bool mIsCaptureStreamCreated = false, mIsRxCaptureStreamCreated = false;
        bool mIsPlayStreamCreated = false;
        bool mIsRecording = false, mIsRxRecording = false;
        bool mIsMpmsReady = false;
        uint32_t mBufferRecordedTillNow, mRxBufferRecordedTillNow;
        uint32_t maxFileBytes;
        int32_t  currentRepeat;
        FILE *mFile, // File ptr for local recording
                *mRxFile; //File ptr for incall downlink recording
        FILE *mPlayFile;
        le_sem_Ref_t mRecordSemRef, mRxRecordSemRef, mPbStartedSemRef, mRecStartedSemRef,
                mDtmfStartedSemRef, mDtmfStartedSemRefTx;
        SlotId mRxSlotId = INVALID_SLOT_ID , mTxSlotId = INVALID_SLOT_ID;
        StreamConfig voiceStreamConfig = {};
        taf_PbList_t pbList;
        taf_Dtmf_t dtmfDataRx{}, dtmfDataTx{};
        taf_mngdPm_InfoReportHandlerRef_t bubHandlerRef;
        taf_PlaybackFile_t currentPbFile;
        le_event_HandlerRef_t bufferHandlerRef;

        le_mem_PoolRef_t ConnectorPool = NULL;
        le_mem_PoolRef_t StreamPool = NULL;
        le_mem_PoolRef_t HashMapPool = NULL;
        le_mem_PoolRef_t SessionRefPool = NULL;
        le_mem_PoolRef_t RoutePool = NULL;
        le_mem_PoolRef_t EventHandlerRefNodePool = NULL;
        le_mem_PoolRef_t EventIdPool = NULL;
        le_mem_PoolRef_t PlaybackListPool = NULL;

        le_ref_MapRef_t ConnectorRefMap = NULL;
        le_ref_MapRef_t StreamRefMap = NULL;
        le_ref_MapRef_t RouteRefMap = NULL;
        le_ref_MapRef_t EventHandlerRefMap = NULL;

        le_dls_List_t  ConnectorList = LE_DLS_LIST_INIT;
        le_dls_List_t  HashMapList = LE_DLS_LIST_INIT;

        taf_audio_StreamRef_t CreateStream( StreamConfig_t* streamConfPtr );
        void InitStream( taf_audio_Stream_t* streamPtr );
        void CloseConnectorPaths( taf_audio_Connector_t*   connectorPtr );
        void DisconnectConnectors(taf_audio_Stream_t* streamPtr);
        void DeleteHashMap( taf_audio_Connector_t* connectorPtr );
        void ClearHashMap( le_hashmap_Ref_t hashMapRef );
        le_hashmap_Ref_t GetHashMap( );
        le_result_t ConnectStreamPaths( taf_audio_Stream_t* streamPtr,
                le_hashmap_Ref_t streamListPtr );
        le_result_t StartAudio( StreamConfig config );
        le_result_t PlayWave( taf_audio_Stream_t* streamPtr, const char *srcPath);
        le_result_t PlayAmr( taf_audio_Stream_t* streamPtr, const char *srcPath);
        le_result_t ReadPcmHeader( taf_audio_Stream_t* streamPtr, const char *srcPath,
                StreamConfig &config);
        le_result_t ReadAmrHeader( taf_audio_Stream_t* streamPtr, const char *srcPath,
                StreamConfig &config);
        ssize_t ReadHeader( int fd, void* bufPtr, size_t bufSize);
        le_result_t setWavHeader( FILE *mFile, taf_audio_Stream_t *config);
        le_result_t StopAudio(taf_audio_Stream_t* streamPtr);
        le_result_t DeleteAudioStream(taf_audio_Stream_t* streamPtr);
        void ReleaseStream( taf_audio_Stream_t*  streamPtr, le_msg_SessionRef_t sessionRef,
                bool allReferences);
        StreamEventHandlerRef_t AddStreamEventHandler( taf_audio_Stream_t* sPtr,
                le_event_HandlerFunc_t handlerPtr,
                taf_audio_StreamEventBitMask_t streamEventBitMask, void* contextPtr );
        void RemoveStreamEventHandler( StreamEventHandlerRef_t handlerRef );
        le_result_t StopandDelete(taf_audio_Stream_t* strmPtr, le_hashmap_Ref_t strmListPtr);
        void PlayAudioFile( taf_PlaybackFile_t fileToPlay);
        void PbBufferHandler();

        static void ClientSessionCloseEventHandler( le_msg_SessionRef_t sessionRef,
                            void* contextPtr);
        static void DestructStream( void *objPtr );
        static void StartAudioCallback(ErrorCode error);
        static void StopAudioCallback(ErrorCode error);
        static void DeleteVoiceCallback(ErrorCode error);
        static void DeletePlayCallback(ErrorCode error);
        static void FirstLayerEventHandler( void* reportPtr, void* secondLayerHandlerFunc );
        static le_event_Id_t CreateEventId();
        static void* Record( void* ctxPtr);
        static void* PlayList( void* ctxPtr);
        static void* RegisterBufferEvent( void* ctxPtr);
        static void ReadCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
                    telux::common::ErrorCode error);
        static void RxReadCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
                    telux::common::ErrorCode error);
        static void WriteCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
                uint32_t bytes, telux::common::ErrorCode error);
        static void StreamMuteUnmuteCallback(ErrorCode error);
        static void BuBStatusCB(int32_t status, void *contextPtr);
        static void RegisterDtmfListenerCallback(ErrorCode error);
        static void PlayDtmfCallback(ErrorCode error);
        static void StopDtmfCallback(ErrorCode error);
        std::shared_ptr<tafSignallingDtmfListener> onStartDtmfTone = nullptr;
        std::shared_ptr<tafSignallingDtmfListener> onStopDtmfTone = nullptr;
        std::shared_ptr<telux::tel::ICallManager> callManager = nullptr;
        std::pair<int, int> getDTMFFrequencies(char key);
        static void* playAllDtmfTones(void* dtmfTones);
        static void* playDTMFonTX(void* dtmfTones);
        static void BufferEventHandler(void* contextPtr);
};
}
}
