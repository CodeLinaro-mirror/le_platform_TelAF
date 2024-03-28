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
#include "tafHalAudio.h"
#include "tafHalLib.hpp"

#define MAX_VENDOR_NODES  8

/**
 * Node Event Handler Reference structure
 */
typedef struct
{
    taf_mngd_audioHw_NodeStateChangeHandlerRef_t  handlerRef;
    uint8_t                                       nodeId;
    void*                                         userCtx;
    le_dls_Link_t                                 next;
    le_event_Id_t                                 eventId;
}
NodeEventHandlerRefNode_t;

typedef struct {
    uint8_t nodeId;
    taf_mngd_audioHw_Event_t event;
}NodeEvent_t;

namespace taf {
namespace audioVhal {

class taf_MngdAudioVhal
{
    public:

        audio_Inf_t *audioInf = nullptr;

        bool isVhalLoaded = false;
        le_mem_PoolRef_t NodeEventHandlerRefPool = NULL;
        le_dls_List_t NodeEventHandlerList;

        static taf_MngdAudioVhal &GetInstance();
        static void NodeEventHandler(void* reportPtr, void* secondLayerHandlerFunc);
        static void NodeEventCB(uint8_t nodeId, taf_hal_audio_DevEvent event);
        taf_MngdAudioVhal() {};
        ~taf_MngdAudioVhal() {};

        void Init(void);
        bool isAudioDrvAvailable();
        le_result_t OpenRoute(bool status, taf_mngd_audio_RouteId_t route,
                taf_mngd_audio_Mode_t mode);
        taf_mngd_audioHw_NodeType_t GetNodeType(uint8_t audioNodeId);
        le_result_t SendNodeVendorConfig(uint8_t audioNodeId, const char* configPath);
        le_result_t SendVendorConfig(const char* configPath);
        le_result_t SetNodePowerState(uint8_t audioNodeId, taf_mngd_audioHw_NodePowerState_t state);
        le_result_t GetNodePowerState(uint8_t audioNodeId,
                taf_mngd_audioHw_NodePowerState_t *state);
        le_result_t SetNodeMuteState(uint8_t audioNodeId, bool mute);
        le_result_t GetNodeMuteState(uint8_t audioNodeId, bool *isMuted);
        taf_mngd_audioHw_NodeStateChangeHandlerRef_t AddNodeStateChangeHandler( uint8_t audioNodeId,
                taf_mngd_audioHw_NodeStateHandlerFunc_t handlerPtr, void* contextPtr);
        void RemoveNodeStateChangeHandler(taf_mngd_audioHw_NodeStateChangeHandlerRef_t handlerRef);
};
}
}
