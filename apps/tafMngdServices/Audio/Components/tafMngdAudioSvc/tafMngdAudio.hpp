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
#include <telux/audio/AudioFactory.hpp>
#include <telux/audio/AudioManager.hpp>

using namespace telux::common;
using namespace telux::audio;

#define MAX_CONNECTOR              8
#define HASHMAP_SIZE               10
#define MAX_STREAM                 6
#define MAX_ROUTE                  10

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
    bool activeStatus;
    taf_mngd_audio_StreamRef_t sinkRef;
    taf_mngd_audio_StreamRef_t sourceRef;
    le_msg_SessionRef_t sessionRef;
    taf_mngd_audio_RouteRef_t routeRef;
}
taf_mngd_audio_Route_t;

namespace telux {
namespace tafsvc {

class taf_MngdAudio : public ITafSvc
{
    public:

        static taf_MngdAudio &GetInstance();

        taf_MngdAudio() {};
        ~taf_MngdAudio() {};

        std::shared_ptr<telux::audio::IAudioManager> mAudioManager;
        std::shared_ptr<telux::audio::IAudioVoiceStream> mAudioVoiceStream;
        std::promise<telux::common::ErrorCode> gCallbackPromise;

        bool isVhalAvailable = false;
        bool isEcnrEnabled = false;
        bool mCallStarted = false;
        bool mVoiceEnabled1 = false;
        bool mModemRx = false;
        bool mSpeaker = false;
        bool mModemTx = false;
        bool mMic = false;
        SlotId mRxSlotId = INVALID_SLOT_ID , mTxSlotId = INVALID_SLOT_ID;
        StreamConfig voiceStreamConfig = {};

        le_mem_PoolRef_t ConnectorPool = NULL;
        le_mem_PoolRef_t StreamPool = NULL;
        le_mem_PoolRef_t HashMapPool = NULL;
        le_mem_PoolRef_t SessionRefPool = NULL;
        le_mem_PoolRef_t RoutePool = NULL;

        le_ref_MapRef_t ConnectorRefMap = NULL;
        le_ref_MapRef_t StreamRefMap = NULL;
        le_ref_MapRef_t RouteRefMap = NULL;

        le_dls_List_t  ConnectorList = LE_DLS_LIST_INIT;
        le_dls_List_t  HashMapList = LE_DLS_LIST_INIT;

        void Init(void);

        taf_mngd_audio_StreamRef_t CreateStream( StreamConfig_t* streamConfPtr );
        void InitStream( taf_mngd_audio_Stream_t* streamPtr );
        taf_mngd_audio_Connector_t* CreateConnector();
        void DeleteHashMap( taf_mngd_audio_Connector_t* connectorPtr );
        void ClearHashMap( le_hashmap_Ref_t hashMapRef );
        le_hashmap_Ref_t GetHashMap( );
        void DeleteConnector( taf_mngd_audio_ConnectorRef_t connectorRef );
        void CloseConnectorPaths( taf_mngd_audio_Connector_t*   connectorPtr );
        void DisconnectConnectors(taf_mngd_audio_Stream_t* streamPtr);
        le_result_t Connect ( taf_mngd_audio_ConnectorRef_t connectorRef,
                taf_mngd_audio_StreamRef_t streamRef );
        void Disconnect ( taf_mngd_audio_ConnectorRef_t connectorRef,
                taf_mngd_audio_StreamRef_t streamRef );
        void Close( taf_mngd_audio_StreamRef_t streamRef );
        void ReleaseStream( taf_mngd_audio_Stream_t*  streamPtr, le_msg_SessionRef_t sessionRef,
                bool allReferences);
        taf_mngd_audio_StreamRef_t OpenModemVoiceRx(uint32_t slotId);
        taf_mngd_audio_StreamRef_t OpenModemVoiceTx(uint32_t slotId, bool enableEcnr);
        taf_mngd_audio_RouteRef_t OpenRoute( taf_mngd_audio_RouteId_t route,
                taf_mngd_audio_Mode_t mode, taf_mngd_audio_StreamRef_t *sinkRef,
                taf_mngd_audio_StreamRef_t *sourceRef);
        le_result_t CloseRoute( taf_mngd_audio_RouteRef_t routeRef );
        static void ClientSessionCloseEventHandler( le_msg_SessionRef_t sessionRef,
                            void* contextPtr);
        static void DestructStream( void *objPtr );
        static void StartAudioCallback(ErrorCode error);
        static void StopAudioCallback(ErrorCode error);
        static void DeleteVoiceCallback(ErrorCode error);

        private:
        le_result_t ConnectStreamPaths( taf_mngd_audio_Stream_t* streamPtr,
                le_hashmap_Ref_t streamListPtr );
        le_result_t StartAudio( StreamConfig config );
};
}
}