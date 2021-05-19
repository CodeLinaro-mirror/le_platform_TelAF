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

#define TIMEOUT 5
#define DEFAULT_STREAMPOOL_SIZE    1
#define MAX_STREAM                 6
#define HASHMAP_SIZE               10
#define MAX_CONNECTOR              8

#define CHECK_OUTPUT_IF(interface)     (   (interface == TAF_AUDIO_IF_CODEC_SPEAKER) || \
                                           (interface == TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX) \
                                       )

typedef struct
{
    taf_audio_StreamRef_t  streamRef;
    le_dls_Link_t          RefNodeLink;
} taf_StreamRefNode_t;


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
    TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,
    TAF_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX,
    TAF_AUDIO_NUM_INTERFACES
}
taf_audio_If_t;

namespace telux {
namespace tafsvc {

        class tafVoiceListener : public IVoiceListener {
                public:
                        void onDtmfToneDetection(DtmfTone dtmfTone) override;
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

        }
        CreateStream_t;

        typedef struct taf_audio_Stream {
                bool             device;
                uint32_t         gain;
                int32_t          fd;
                taf_audio_If_t interface;
                le_hashmap_Ref_t connList;
                taf_audio_StreamRef_t streamRef;
                le_dls_List_t    sessionRefList;
                le_dls_Link_t  streamLink;
        }taf_audio_Stream_t;

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
                        std::shared_ptr<tafVoiceListener> mVoiceListener;

                        bool mAudioStarted = false;
                        bool mVoiceEnabled1 = false;
                        bool mVoiceEnabled2 = false;
                        bool mModemRx = false;
                        bool mModemTx = false;
                        bool mSpeaker = false;
                        bool mMic = false;
                        uint32_t mSlotId;
                        uint32_t mSize;

                        le_result_t StartAudio( StreamConfig config );
                        le_result_t StopAudio();
                        le_result_t DeleteAudio();
                        le_result_t PlayDtmfTone(DtmfTone tone, uint32_t duration, uint16_t gain);

                        le_mem_PoolRef_t SessionRefPool = NULL;
                        le_mem_PoolRef_t AudioPool = NULL;
                        le_mem_PoolRef_t AudioRefPool = NULL;
                        le_mem_PoolRef_t AudioConnPool = NULL;
                        le_mem_PoolRef_t HashMapPool = NULL;

                        le_ref_MapRef_t AudioConnRefMap = NULL;
                        le_ref_MapRef_t AudioRefMap = NULL;

                        le_dls_List_t  HashMapList = LE_DLS_LIST_INIT;
                        le_dls_List_t  ConnList=LE_DLS_LIST_INIT;

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
                        le_result_t PlayDtmf(taf_audio_StreamRef_t streamRef,
                                        const char* dtmfPtr, uint32_t duration, uint32_t pause );
                        le_result_t Mute(taf_audio_StreamRef_t streamRef, StreamMute mute );


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
                        static void ReadCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer, ErrorCode error);
        };
}
}
