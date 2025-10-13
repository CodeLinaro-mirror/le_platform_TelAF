/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafAudioVhal.hpp"
#include "tafSvcIF.hpp"
#include <setjmp.h>

#define TIMER_SAFECALL 5
DECLARE_SAFE_CALL();

using namespace taf::audioVhal;

LE_MEM_DEFINE_STATIC_POOL(NodeEventHandlerRef, MAX_VENDOR_NODES,
        sizeof(NodeEventHandlerRefNode_t));

/**
 * Returns audio vhal instance
 */
taf_AudioVhal &taf_AudioVhal::GetInstance()
{
    static taf_AudioVhal instance;
    return instance;
}


/**
 * Load Audio VHAL driver.
 */
le_result_t taf_AudioVhal::LoadDriver()
{
    // load driver
    audioInf = (hal_audio_Inf_t *)taf_devMgr_LoadDrv(TAF_AUDIO_MODULE_NAME, nullptr);

    if(audioInf == nullptr)
    {
        LE_ERROR("Can not load the driver %s", TAF_AUDIO_MODULE_NAME);
        isVhalLoaded = false;
        return LE_FAULT;
    }
    else // successfully loaded
    {
        LE_INFO("Driver loaded successfully....");
        isVhalLoaded = true;

        int ret = 0;
        LE_DEBUG("Before safe call init");
        ENTER_SAFE_CALL(TIMER_SAFECALL, ret, (*(audioInf->InitHAL)));
        EXIT_SAFE_CALL();

        if (ret == -1)
        {
            LE_ERROR("Failed to init Audio VHAL : %s", TAF_AUDIO_MODULE_NAME);
            return LE_FAULT;
        }

        NodeEventHandlerRefPool = le_mem_InitStaticPool(NodeEventHandlerRef, MAX_VENDOR_NODES,
                sizeof(NodeEventHandlerRefNode_t));
        NodeEventHandlerList = LE_DLS_LIST_INIT;
        LE_DEBUG("Advertise Audio vendor service");
        AdvertiseVendorService();
        return LE_OK;
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Advertise audioVendor service.
 */
 //------------------------------------------------------------------------------------------------
void taf_AudioVhal::AdvertiseVendorService()
{
    taf_audioVendor_AdvertiseService();
}

bool taf_AudioVhal::isAudioDrvAvailable()
{
    LE_DEBUG("isVhalLoaded : %s", isVhalLoaded ? "true" : "false");
    return isVhalLoaded;
}

le_result_t taf_AudioVhal::OpenRoute(bool status, taf_audio_RouteId_t routeId,
        taf_audio_Mode_t mode)
{
    LE_DEBUG("OpenRoute status %s route %d mode %d", (status ? "true" : "false"), routeId, mode);
    le_result_t result;
    int ret = 0;
    ENTER_SAFE_CALL_EX(TIMER_SAFECALL, ret, result, audioInf->CtlSetAudioStatus(status,
            (uint32_t)routeId, (hal_audio_Mode_t)mode));
    EXIT_SAFE_CALL();
    if(ret == -1)
    {
        LE_ERROR("Failed to call OpenRoute");
        return LE_FAULT;
    }
    return result;
}

le_result_t taf_AudioVhal::GetNodeType( uint8_t audioNodeId,
        taf_audioVendor_NodeType_t *nodeType )
{
    LE_DEBUG("GetNodeType %d", audioNodeId);
    TAF_ERROR_IF_RET_VAL(nodeType == NULL, LE_BAD_PARAMETER, "nodeType pointer is NULL");
    hal_audio_NodeType_t halNodeType;
    le_result_t res = LE_FAULT;
    int ret = 0;
    ENTER_SAFE_CALL_EX(TIMER_SAFECALL, ret, res, audioInf->GetNodeType(audioNodeId,
            &halNodeType));
    EXIT_SAFE_CALL();
    if(ret == -1)
    {
        LE_ERROR("Failed to call GetNodeType");
        return res;
    }
    if(res == LE_OK)
    {
        if(halNodeType == HAL_AUDIO_NODE_TYPE_CODEC)
            *nodeType = TAF_AUDIOVENDOR_AUDIO_CODEC;
        else if(halNodeType == HAL_AUDIO_NODE_TYPE_PA)
            *nodeType = TAF_AUDIOVENDOR_AUDIO_PA;
        else if(halNodeType == HAL_AUDIO_NODE_TYPE_A2B)
            *nodeType = TAF_AUDIOVENDOR_AUDIO_A2B;
        else
            *nodeType = TAF_AUDIOVENDOR_INVALID;
    }
    return res;
}

le_result_t taf_AudioVhal::SendNodeVendorConfig(uint8_t audioNodeId, const char* configPath)
{
    LE_DEBUG("SendNodeVendorConfig Node id : %d configPath : %s", audioNodeId, configPath);
    le_result_t res = LE_FAULT;
    int ret = 0;
    ENTER_SAFE_CALL_EX(TIMER_SAFECALL, ret, res,
            audioInf->SendNodeVendorConfig(audioNodeId, configPath));
    EXIT_SAFE_CALL();
    if(ret == -1)
    {
        LE_ERROR("Failed to call SendNodeVendorConfig");
    }
    return res;
}

le_result_t taf_AudioVhal::SetNodePowerState(uint8_t audioNodeId,
        taf_audioVendor_NodePowerState_t state)
{
    LE_DEBUG("SetNodePowerState node id : %d state : %d", audioNodeId, state);
    le_result_t res = LE_FAULT;
    int ret = 0;
    ENTER_SAFE_CALL_EX(TIMER_SAFECALL, ret, res,
            audioInf->SetNodePowerState(audioNodeId, (hal_audio_PowerState_t)state));
    EXIT_SAFE_CALL();
    if(ret == -1)
    {
        LE_ERROR("Failed to call SetNodePowerState");
    }
    return res;
}

le_result_t taf_AudioVhal::GetNodePowerState(uint8_t audioNodeId,
        taf_audioVendor_NodePowerState_t* state)
{
    LE_DEBUG("GetNodePowerState node id : %d audioInf %p", audioNodeId, audioInf);
    TAF_ERROR_IF_RET_VAL(state == NULL, LE_BAD_PARAMETER, "state pointer is NULL");
    hal_audio_PowerState_t vhalState;
    le_result_t res = LE_FAULT;
    int ret = 0;
    ENTER_SAFE_CALL_EX(TIMER_SAFECALL, ret, res,
            audioInf->GetNodePowerState(audioNodeId, &vhalState));
    EXIT_SAFE_CALL();
    if(ret == -1)
    {
        LE_ERROR("Failed to call GetNodePowerState");
        return res;
    }

    *state = (taf_audioVendor_NodePowerState_t)vhalState;
    LE_DEBUG("state is %d", *state);
    return res;
}

le_result_t taf_AudioVhal::SetNodeMuteState(uint8_t audioNodeId, bool mute)
{
    LE_DEBUG("SetNodeMuteState node id : %d mute : %s", audioNodeId, mute ? "true" : "false");
    le_result_t res = LE_FAULT;
    int ret = 0;
    ENTER_SAFE_CALL_EX(TIMER_SAFECALL, ret, res,
            audioInf->SetNodeMuteState(audioNodeId, mute));
    EXIT_SAFE_CALL();
    if(ret == -1)
    {
        LE_ERROR("Failed to call SetNodeMuteState");
        return res;
    }
    return res;
}

le_result_t taf_AudioVhal::GetNodeMuteState(uint8_t audioNodeId, bool *isMuted)
{
    LE_DEBUG("GetNodeMuteState node id : %d ", audioNodeId);
    le_result_t res = LE_FAULT;
    int ret = 0;
    ENTER_SAFE_CALL_EX(TIMER_SAFECALL, ret, res,
            audioInf->GetNodeMuteState(audioNodeId, isMuted));
    EXIT_SAFE_CALL();
    if(ret == -1)
    {
        LE_ERROR("Failed to call GetNodeMuteState");
        return res;
    }
    return res;
}

le_result_t taf_AudioVhal::SetNodeGain (uint8_t nodeId, taf_audioVendor_Direction_t direction,
        double gain)
{
    LE_DEBUG("SetNodeGain node id : %d direction: %d gain: %f", nodeId, direction, gain);

    if (audioInf->SetNodeGain) {
        le_result_t res = LE_FAULT;
        int ret = 0;
        ENTER_SAFE_CALL_EX(TIMER_SAFECALL, ret, res,
                audioInf->SetNodeGain(nodeId, (hal_audio_direction_t)direction, gain));
        EXIT_SAFE_CALL();
        if(ret == -1)
        {
            LE_ERROR("Failed to call SetNodeGain");
            return res;
        }
        return res;
    }

    return LE_UNSUPPORTED;
}

le_result_t taf_AudioVhal::GetNodeGain (uint8_t nodeId, taf_audioVendor_Direction_t direction,
        double *gain)
{
    LE_DEBUG("GetNodeGain node id : %d direction: %d", nodeId, direction);

    if (audioInf->GetNodeGain) {
        le_result_t res = LE_FAULT;
        int ret = 0;
        ENTER_SAFE_CALL_EX(TIMER_SAFECALL, ret, res,
                audioInf->GetNodeGain(nodeId, (hal_audio_direction_t)direction, gain));
        EXIT_SAFE_CALL();
        if(ret == -1)
        {
            LE_ERROR("Failed to call GetNodeGain");
            return res;
        }
        return res;
    }

    return LE_UNSUPPORTED;

}

void taf_AudioVhal::NodeEventHandler(void* reportPtr, void* secondLayerHandlerFunc)
{
    taf_AudioVhal audioVhal = taf_AudioVhal::GetInstance();
    NodeEvent_t* eventPtr = (NodeEvent_t*)reportPtr;
    LE_DEBUG("NodeEventHandler nodeId %d event %d", eventPtr->nodeId, eventPtr->event);
    le_dls_Link_t* linkHandlerPtr = NULL;
    linkHandlerPtr = le_dls_PeekTail(&audioVhal.NodeEventHandlerList);
    while(linkHandlerPtr)
    {
        NodeEventHandlerRefNode_t * handlerRefPtr = CONTAINER_OF(linkHandlerPtr,
                NodeEventHandlerRefNode_t, next);
        linkHandlerPtr = le_dls_PeekPrev(&audioVhal.NodeEventHandlerList, linkHandlerPtr);
        if(handlerRefPtr->nodeId == eventPtr->nodeId)
        {
            LE_INFO("NodeId registered received the event");
            taf_audioVendor_NodeStateHandlerFunc_t clientFunc =
                    (taf_audioVendor_NodeStateHandlerFunc_t)secondLayerHandlerFunc;
            clientFunc(eventPtr->nodeId, eventPtr->event, handlerRefPtr->userCtx);
        }
    }
}

void taf_AudioVhal::NodeEventCB(uint8_t nodeId, hal_audio_DevEvent_t event)
{
    LE_DEBUG("NodeEventCB nodeId : %d event : %d", nodeId, event);
    taf_AudioVhal audioVhal = taf_AudioVhal::GetInstance();
    NodeEvent_t nodeEvent;
    nodeEvent.nodeId = nodeId;
    nodeEvent.event = (taf_audioVendor_Event_t)event;
    // Notify the repective callbackFunc
    le_dls_Link_t* linkHandlerPtr = NULL;
    linkHandlerPtr = le_dls_PeekTail(&audioVhal.NodeEventHandlerList);
    while(linkHandlerPtr)
    {
        NodeEventHandlerRefNode_t * handlerRefPtr = CONTAINER_OF(linkHandlerPtr,
                NodeEventHandlerRefNode_t, next);
        linkHandlerPtr = le_dls_PeekPrev(&audioVhal.NodeEventHandlerList, linkHandlerPtr);
        if(handlerRefPtr->nodeId == nodeId)
        {
            LE_INFO("NodeId registered received the event");
            le_event_Report(handlerRefPtr->eventId, (void*)&nodeEvent, sizeof(NodeEvent_t));
        }
    }
}

taf_audioVendor_NodeStateChangeHandlerRef_t taf_AudioVhal::AddNodeStateChangeHandler(
        uint8_t audioNodeId, taf_audioVendor_NodeStateHandlerFunc_t handlerPtr, void* contextPtr)
{
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handler reference");

    LE_DEBUG("Add handler for node %d.", audioNodeId);

    le_result_t res = LE_FAULT;
    int ret = 0;
    ENTER_SAFE_CALL_EX(TIMER_SAFECALL, ret, res,
            audioInf->AddNodeStateChangeHandler(audioNodeId, NodeEventCB));
    EXIT_SAFE_CALL();
    if(ret == -1)
    {
        LE_ERROR("Failed to call AddNodeStateChangeHandler");
        return NULL;
    }
    TAF_ERROR_IF_RET_VAL(res != LE_OK, NULL, "Failed to register for node event change");

    NodeEventHandlerRefNode_t* nodeHandlerRefPtr =
            (NodeEventHandlerRefNode_t*)le_mem_ForceAlloc(NodeEventHandlerRefPool);

    nodeHandlerRefPtr->next = LE_DLS_LINK_INIT;

    nodeHandlerRefPtr->nodeId = audioNodeId;

    char nodeEventId[50];
    snprintf(nodeEventId, sizeof(nodeEventId), "nodeEvent-%d", audioNodeId);
    nodeHandlerRefPtr->eventId = le_event_CreateId(nodeEventId, sizeof(NodeEvent_t));

    nodeHandlerRefPtr->handlerRef =(taf_audioVendor_NodeStateChangeHandlerRef_t)
            le_event_AddLayeredHandler("NodeEventHandler", nodeHandlerRefPtr->eventId,
            NodeEventHandler, (void*)handlerPtr);

    nodeHandlerRefPtr->userCtx = contextPtr;

    le_dls_Queue(&NodeEventHandlerList, &nodeHandlerRefPtr->next);

    return nodeHandlerRefPtr->handlerRef;
}

void taf_AudioVhal::RemoveNodeStateChangeHandler(
        taf_audioVendor_NodeStateChangeHandlerRef_t handlerRef )
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);

    // Notify the repective callbackFunc
    le_dls_Link_t* linkHandlerPtr = NULL;
    linkHandlerPtr = le_dls_PeekTail(&NodeEventHandlerList);
    while(linkHandlerPtr)
    {
        NodeEventHandlerRefNode_t * handlerRefPtr = CONTAINER_OF(linkHandlerPtr,
                NodeEventHandlerRefNode_t, next);
        linkHandlerPtr = le_dls_PeekPrev(&NodeEventHandlerList, linkHandlerPtr);
        if(handlerRefPtr->handlerRef == handlerRef)
        {
            LE_INFO("Remove handler from the list");
            le_dls_Remove(&NodeEventHandlerList, &(handlerRefPtr->next));
            le_mem_Release(handlerRefPtr);
        }
    }
}

le_result_t taf_AudioVhal::SendVendorConfig(const char* configPath)
{
    LE_DEBUG("SendVendorConfig %s", configPath);
    le_result_t res = LE_FAULT;
    int ret = 0;
    ENTER_SAFE_CALL_EX(TIMER_SAFECALL, ret, res,
            audioInf->SendVendorConfig(configPath));
    EXIT_SAFE_CALL();
    if(ret == -1)
    {
        LE_ERROR("Failed to call SendVendorConfig");
        return res;
    }
    return res;
}

le_result_t taf_AudioVhal::CtlReportBubStatus(hal_audio_bubStatus_t bubStatus)
{
    LE_DEBUG("CtlReportBubStatus %d", bubStatus);
    le_result_t res = LE_FAULT;
    int ret = 0;
    ENTER_SAFE_CALL_EX(TIMER_SAFECALL, ret, res,
            audioInf->CtlReportBubStatus(bubStatus));
    EXIT_SAFE_CALL();
    if(ret == -1)
    {
        LE_ERROR("Failed to call CtlReportBubStatus");
        return res;
    }
    return res;
}
