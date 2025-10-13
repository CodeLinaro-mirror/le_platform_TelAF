/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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
    taf_audioVendor_NodeStateChangeHandlerRef_t  handlerRef;
    uint8_t                                       nodeId;
    void*                                         userCtx;
    le_dls_Link_t                                 next;
    le_event_Id_t                                 eventId;
}
NodeEventHandlerRefNode_t;

typedef struct {
    uint8_t nodeId;
    taf_audioVendor_Event_t event;
}NodeEvent_t;

namespace taf {
namespace audioVhal {

class taf_AudioVhal
{
    public:

        hal_audio_Inf_t *audioInf = nullptr;

        bool isVhalLoaded = false;
        le_mem_PoolRef_t NodeEventHandlerRefPool = NULL;
        le_dls_List_t NodeEventHandlerList;

        static taf_AudioVhal &GetInstance();
        static void NodeEventHandler(void* reportPtr, void* secondLayerHandlerFunc);
        static void NodeEventCB(uint8_t nodeId, hal_audio_DevEvent_t event);
        taf_AudioVhal() {};
        ~taf_AudioVhal() {};

        le_result_t LoadDriver();
        void AdvertiseVendorService();
        bool isAudioDrvAvailable();
        le_result_t OpenRoute(bool status, taf_audio_RouteId_t route,
                taf_audio_Mode_t mode);
        le_result_t CtlReportBubStatus(hal_audio_bubStatus_t bubStatus);
        le_result_t SendVendorConfig(const char* configPath);
        le_result_t GetNodeType(uint8_t audioNodeId, taf_audioVendor_NodeType_t *nodeType);
        le_result_t SendNodeVendorConfig(uint8_t audioNodeId, const char* configPath);
        le_result_t SetNodePowerState(uint8_t audioNodeId, taf_audioVendor_NodePowerState_t state);
        le_result_t GetNodePowerState(uint8_t audioNodeId,
                taf_audioVendor_NodePowerState_t *state);
        le_result_t SetNodeMuteState(uint8_t audioNodeId, bool mute);
        le_result_t GetNodeMuteState(uint8_t audioNodeId, bool *isMuted);
        le_result_t SetNodeGain(uint8_t nodeId, taf_audioVendor_Direction_t direction,
                double gain);
        le_result_t GetNodeGain(uint8_t nodeId, taf_audioVendor_Direction_t direction,
                double *gain);
        taf_audioVendor_NodeStateChangeHandlerRef_t AddNodeStateChangeHandler( uint8_t audioNodeId,
                taf_audioVendor_NodeStateHandlerFunc_t handlerPtr, void* contextPtr);
        void RemoveNodeStateChangeHandler(taf_audioVendor_NodeStateChangeHandlerRef_t handlerRef);
};
}
}
