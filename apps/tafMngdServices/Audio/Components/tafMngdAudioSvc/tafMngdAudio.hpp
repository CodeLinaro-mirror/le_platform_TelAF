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
#include <queue>
#include <telux/audio/AudioFactory.hpp>
#include <telux/audio/AudioManager.hpp>
#include <telux/audio/AudioPlayer.hpp>

using namespace telux::common;
using namespace telux::audio;

#define MAX_CONNECTOR              8
#define HASHMAP_SIZE               10
#define MAX_STREAM                 6
#define MAX_ROUTE                  10
#define TOTAL_BUFFERS              2
#define DEFAULT_SAMPLERATE         48000
#define DEFAULT_BITSPERSAMPLE      16
#define MAX_NUM_OF_PLAYLIST        4
#define MAX_NUM_OF_PLAYBACK_FILES  8

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

#define CHECK_OUTPUT_IF(interface)     ((interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_0) || \
        (interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_1) || \
        (interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_2) || \
        (interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_3) || \
        (interface == TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_4) || \
        (interface == TAF_MNGD_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX) || \
        (interface == TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE) \
        )

/**
 * Symbols used to populate wave header file.
 */
#define FORMAT_PCM 1
#define ID_RIFF    0x46464952
#define ID_WAVE    0x45564157
#define ID_FMT     0x20746d66
#define ID_DATA    0x61746164

typedef struct StreamEventHandlerRef* StreamEventHandlerRef_t;

typedef struct taf_mngd_audio_Connector {
    le_hashmap_Ref_t              audioInList;
    le_hashmap_Ref_t              audioOutList;
    le_msg_SessionRef_t           sessionRef;
    taf_mngd_audio_ConnectorRef_t connRef;
    le_dls_Link_t                 connLink;
}
taf_mngd_audio_Connector_t;

typedef struct{
    le_hashmap_Ref_t    hashMapRef;
    bool                isUsed;
    le_dls_Link_t       hashMapLink;
}
taf_mngd_audio_hashMapList_t;

typedef enum
{
    TAF_MNGD_AUDIO_IF_CODEC_MIC_0,
    TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_0,
    TAF_MNGD_AUDIO_IF_CODEC_MIC_1,
    TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_1,
    TAF_MNGD_AUDIO_IF_CODEC_MIC_2,
    TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_2,
    TAF_MNGD_AUDIO_IF_CODEC_MIC_3,
    TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_3,
    TAF_MNGD_AUDIO_IF_CODEC_MIC_4,
    TAF_MNGD_AUDIO_IF_CODEC_SPEAKER_4,
    TAF_MNGD_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,
    TAF_MNGD_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX,
    TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_PLAY,
    TAF_MNGD_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE,
    TAF_MNGD_AUDIO_NUM_INTERFACES
}
taf_mngd_audio_If_t;

typedef struct taf_mngd_audio_Stream {
    bool             device;
    bool             playFile;
    bool echoCancellerEnabled;
    uint32_t         gain;
    int32_t          fd;
    uint32_t         timeSlot;
    taf_mngd_audio_If_t interface;
    le_hashmap_Ref_t connList;
    taf_mngd_audio_Direction_t direction;
    taf_mngd_audio_StreamRef_t streamRef;
    le_event_Id_t    eventId;
    le_dls_List_t    streamRefWithEventHdlrList;
    le_dls_List_t    sessionRefList;
    le_dls_Link_t  streamLink;
}taf_mngd_audio_Stream_t;

typedef struct
{
    taf_mngd_audio_If_t interface;
    bool            HwDevice;
    taf_mngd_audio_Direction_t direction;
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
    taf_mngd_audio_RouteId_t routeId;
    taf_mngd_audio_Mode_t mode;
    taf_mngd_audio_StreamRef_t sinkRef;
    taf_mngd_audio_StreamRef_t sourceRef;
    le_msg_SessionRef_t sessionRef;
    taf_mngd_audio_RouteRef_t routeRef;
}
taf_mngd_audio_Route_t;

typedef enum
{
    TAF_MNGD_AUDIO_BITMASK_MEDIA_EVENT = 0x1
}
taf_mngd_audio_StreamEventBitMask_t;

/**
 * Stream Event Handler Reference Node structure
 */
typedef struct
{
    le_event_HandlerRef_t             handlerRef;
    StreamEventHandlerRef_t           streamHandlerRef;
    taf_mngd_audio_StreamEventBitMask_t    streamEventMask;
    struct taf_mngd_audio_Stream*          streamPtr;
    void*                             userCtx;
    le_dls_Link_t                     next;
}
EventHandlerRefNode_t;

typedef struct
{
    taf_mngd_audio_Stream_t*            streamPtr;
    taf_mngd_audio_StreamEventBitMask_t streamEvent;
    union
    {
        taf_mngd_audio_MediaEvent_t     mediaEvent;
        char                       dtmf;
    } event;
}
taf_mngd_audio_StreamEvent_t;

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
}taf_PlaybackFile_t;

typedef struct
{
    taf_PlaybackFile_t filesToPlay[MAX_NUM_OF_PLAYBACK_FILES]; // Array of Playback files
    uint32_t numOfFilesToPlay;                                 // Number of Playback files
    bool isPlaybackInProgress;
    le_msg_SessionRef_t sessionRef;
    taf_mngd_audio_PlayListRef_t playListRef;

}taf_PlaybackList_t;

/**
 * Audio format.
 */
typedef enum
{
    TAF_MNGD_AUDIO_FILE_WAVE,
    TAF_MNGD_AUDIO_FILE_AMR_NB,
    TAF_MNGD_AUDIO_FILE_AMR_WB,
    TAF_MNGD_AUDIO_FILE_MAX
}
taf_mngd_audio_FileFormat_t;

namespace telux {
namespace tafsvc {

    class tafPromptsStatusListener : public telux::audio::IPlayListListener {
        public:
            void onPlaybackStarted() override;
            void onPlaybackStopped() override;
            void onError(telux::common::ErrorCode error, std::string file) override;
            void onFilePlayed(std::string file) override;
            void onPlaybackFinished() override;
    };

class taf_MngdAudio : public ITafSvc
{

    public:

        static taf_MngdAudio &GetInstance();

        taf_MngdAudio() {};
        ~taf_MngdAudio() {};

        std::promise<telux::common::ErrorCode> gCallbackPromise;
        bool mIsPlaying = false;
        taf_mngd_audio_Stream_t* playerStreamPtr;
        AudioFormat mFileFormat = AudioFormat::UNKNOWN;
        taf_mngd_audio_PlayListRef_t currPlayListRef;
        le_ref_MapRef_t PlaybackListRefMap = NULL;
        le_dls_List_t  EventIdList = LE_DLS_LIST_INIT;

        void Init(void);

        taf_mngd_audio_Connector_t* CreateConnector();
        void DeleteConnector( taf_mngd_audio_ConnectorRef_t connectorRef );
        le_result_t Connect ( taf_mngd_audio_ConnectorRef_t connectorRef,
                taf_mngd_audio_StreamRef_t streamRef );
        void Disconnect ( taf_mngd_audio_ConnectorRef_t connectorRef,
                taf_mngd_audio_StreamRef_t streamRef );
        void Close( taf_mngd_audio_StreamRef_t streamRef );
        taf_mngd_audio_StreamRef_t OpenModemVoiceRx(uint32_t slotId);
        taf_mngd_audio_StreamRef_t OpenModemVoiceTx(uint32_t slotId, bool enableEcnr);
        taf_mngd_audio_RouteRef_t OpenRoute( taf_mngd_audio_RouteId_t route,
                taf_mngd_audio_Mode_t mode, taf_mngd_audio_StreamRef_t *sinkRef,
                taf_mngd_audio_StreamRef_t *sourceRef);
        le_result_t CloseRoute( taf_mngd_audio_RouteRef_t routeRef );
        taf_mngd_audio_StreamRef_t OpenPlayer(taf_mngd_audio_Direction_t direction);
        taf_mngd_audio_StreamRef_t OpenRecorder(taf_mngd_audio_Direction_t direction);
        taf_mngd_audio_MediaHandlerRef_t AddMediaHandler(taf_mngd_audio_StreamRef_t streamRef,
                taf_mngd_audio_MediaHandlerFunc_t handlerPtr, void* contextPtr);
        void RemoveMediaHandler(taf_mngd_audio_MediaHandlerRef_t handlerRef);
        le_result_t RecordFile( taf_mngd_audio_StreamRef_t streamRef, const char *srcPath);
        le_result_t Stop(taf_mngd_audio_StreamRef_t streamRef);
        le_result_t PlayFile( taf_mngd_audio_StreamRef_t streamRef, const char *srcPath);
        le_result_t setVhalRouteStatus(taf_mngd_audio_Mode_t mode, bool status);
        taf_mngd_audio_PlayListRef_t CreatePlayList();
        le_result_t AddPlayListEntry(taf_mngd_audio_PlayListRef_t playListRef, const char *scrPath,
                int32_t repeat);
        le_result_t DeletePlayList(taf_mngd_audio_PlayListRef_t playListRef);
        le_result_t PlayFileList ( taf_mngd_audio_StreamRef_t streamRef,
                taf_mngd_audio_PlayListRef_t playListRef);

        private:

        std::shared_ptr<telux::audio::IAudioManager> mAudioManager;
        std::shared_ptr<telux::audio::IAudioVoiceStream> mAudioVoiceStream;
        std::shared_ptr<telux::audio::IAudioCaptureStream> mAudioCaptureStream;
        std::shared_ptr<telux::audio::IStreamBuffer> mStreamBuffer;
        std::shared_ptr<telux::audio::IAudioPlayer> mAudioPlayer;
        std::shared_ptr<telux::audio::IPlayListListener> repeatedPlayerStatusListener;
        std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> mFreeBuffers;

        bool isVhalAvailable = false;
        bool isEcnrEnabled = false;
        bool mCallStarted = false;
        bool mVoiceEnabled1 = false;
        bool mModemRx = false;
        bool mSpeaker = false;
        bool mModemTx = false;
        bool mMic = false;
        bool mIsCaptureStreamCreated = false;
        bool mIsRecording = false;
        bool mEmptyPipeline = false;
        uint32_t mBufferRecordedTillNow;
        FILE *mFile;
        le_sem_Ref_t mSemRef;
        SlotId mRxSlotId = INVALID_SLOT_ID , mTxSlotId = INVALID_SLOT_ID;
        StreamConfig voiceStreamConfig = {};

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

        taf_mngd_audio_StreamRef_t CreateStream( StreamConfig_t* streamConfPtr );
        void InitStream( taf_mngd_audio_Stream_t* streamPtr );
        void CloseConnectorPaths( taf_mngd_audio_Connector_t*   connectorPtr );
        void DisconnectConnectors(taf_mngd_audio_Stream_t* streamPtr);
        void DeleteHashMap( taf_mngd_audio_Connector_t* connectorPtr );
        void ClearHashMap( le_hashmap_Ref_t hashMapRef );
        le_hashmap_Ref_t GetHashMap( );
        le_result_t ConnectStreamPaths( taf_mngd_audio_Stream_t* streamPtr,
                le_hashmap_Ref_t streamListPtr );
        le_result_t StartAudio( StreamConfig config );
        le_result_t PlayWave( taf_mngd_audio_Stream_t* streamPtr, const char *srcPath);
        le_result_t PlayAmr( taf_mngd_audio_Stream_t* streamPtr, const char *srcPath);
        le_result_t ReadPcmHeader( taf_mngd_audio_Stream_t* streamPtr, const char *srcPath,
                StreamConfig &config);
        le_result_t ReadAmrHeader( taf_mngd_audio_Stream_t* streamPtr, const char *srcPath,
                StreamConfig &config);
        ssize_t ReadHeader( int fd, void* bufPtr, size_t bufSize);
        le_result_t setWavHeader( FILE *mFile, taf_mngd_audio_Stream_t *config);
        le_result_t StopAudio(taf_mngd_audio_Stream_t* streamPtr);
        le_result_t DeleteAudioStream(taf_mngd_audio_Stream_t* streamPtr);
        void ReleaseStream( taf_mngd_audio_Stream_t*  streamPtr, le_msg_SessionRef_t sessionRef,
                bool allReferences);
        StreamEventHandlerRef_t AddStreamEventHandler( taf_mngd_audio_Stream_t* sPtr,
                le_event_HandlerFunc_t handlerPtr,
                taf_mngd_audio_StreamEventBitMask_t streamEventBitMask, void* contextPtr );
        void RemoveStreamEventHandler( StreamEventHandlerRef_t handlerRef );
        le_result_t StopandDelete(taf_mngd_audio_Stream_t* strmPtr, le_hashmap_Ref_t strmListPtr);

        static void ClientSessionCloseEventHandler( le_msg_SessionRef_t sessionRef,
                            void* contextPtr);
        static void DestructStream( void *objPtr );
        static void StartAudioCallback(ErrorCode error);
        static void StopAudioCallback(ErrorCode error);
        static void DeleteVoiceCallback(ErrorCode error);
        static void DeletePlayCallback(ErrorCode error);
        static void DeleteCaptureCallback(ErrorCode error);
        static void FirstLayerEventHandler( void* reportPtr, void* secondLayerHandlerFunc );
        static le_event_Id_t CreateEventId();
        static void* Record( void* ctxPtr);
        static void ReadCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
                    telux::common::ErrorCode error);

};
}
}
