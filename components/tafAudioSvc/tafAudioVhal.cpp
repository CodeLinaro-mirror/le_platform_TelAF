/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafAudioVhal.hpp"
#include "tafSvcIF.hpp"

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

void taf_AudioVhal::Init()
{
    // load driver
    audioInf = (hal_audio_Inf_t *)taf_devMgr_LoadDrv(TAF_AUDIO_MODULE_NAME, nullptr);

    if(audioInf == nullptr)
    {
        LE_ERROR("Can not load the driver %s", TAF_AUDIO_MODULE_NAME);
        isVhalLoaded = false;
    }
    else // successfully loaded
    {
        LE_INFO("Driver loaded successfully....");
        isVhalLoaded = true;

        // init first
        (*(audioInf->InitHAL))();
        NodeEventHandlerRefPool = le_mem_InitStaticPool(NodeEventHandlerRef, MAX_VENDOR_NODES,
                sizeof(NodeEventHandlerRefNode_t));
        NodeEventHandlerList = LE_DLS_LIST_INIT;
    }
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
    return audioInf->CtlSetAudioStatus(status, (uint32_t)routeId, (hal_audio_Mode_t)mode);
}

le_result_t taf_AudioVhal::GetNodeType( uint8_t audioNodeId,
        taf_audioVendor_NodeType_t *nodeType )
{
    LE_DEBUG("GetNodeType %d", audioNodeId);
    TAF_ERROR_IF_RET_VAL(nodeType == NULL, LE_BAD_PARAMETER, "nodeType pointer is NULL");
    hal_audio_NodeType_t halNodeType;
    le_result_t res = audioInf->GetNodeType(audioNodeId, &halNodeType);
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
    return audioInf->SendNodeVendorConfig(audioNodeId, configPath);
}

le_result_t taf_AudioVhal::SetNodePowerState(uint8_t audioNodeId,
        taf_audioVendor_NodePowerState_t state)
{
    LE_DEBUG("SetNodePowerState node id : %d state : %d", audioNodeId, state);
    return audioInf->SetNodePowerState(audioNodeId, (hal_audio_PowerState_t)state);
}

le_result_t taf_AudioVhal::GetNodePowerState(uint8_t audioNodeId,
        taf_audioVendor_NodePowerState_t* state)
{
    LE_DEBUG("GetNodePowerState node id : %d audioInf %p", audioNodeId, audioInf);
    TAF_ERROR_IF_RET_VAL(state == NULL, LE_BAD_PARAMETER, "state pointer is NULL");
    hal_audio_PowerState_t vhalState;
    le_result_t res = audioInf->GetNodePowerState(audioNodeId, &vhalState);

    *state = (taf_audioVendor_NodePowerState_t)vhalState;
    LE_DEBUG("state is %d", *state);
    return res;
}

le_result_t taf_AudioVhal::SetNodeMuteState(uint8_t audioNodeId, bool mute)
{
    LE_DEBUG("SetNodeMuteState node id : %d mute : %s", audioNodeId, mute ? "true" : "false");
    return audioInf->SetNodeMuteState(audioNodeId, mute);
}

le_result_t taf_AudioVhal::GetNodeMuteState(uint8_t audioNodeId, bool *isMuted)
{
    LE_DEBUG("GetNodeMuteState node id : %d ", audioNodeId);
    return audioInf->GetNodeMuteState(audioNodeId, isMuted);
}

le_result_t taf_AudioVhal::SetNodeGain (uint8_t nodeId, taf_audioVendor_Direction_t direction,
        double gain)
{
    LE_DEBUG("SetNodeGain node id : %d direction: %d gain: %f", nodeId, direction, gain);

    if (audioInf->SetNodeGain) {
        return audioInf->SetNodeGain(nodeId, (hal_audio_direction_t)direction, gain);
    }

    return LE_UNSUPPORTED;
}

le_result_t taf_AudioVhal::GetNodeGain (uint8_t nodeId, taf_audioVendor_Direction_t direction,
        double *gain)
{
    LE_DEBUG("GetNodeGain node id : %d direction: %d", nodeId, direction);

    if (audioInf->GetNodeGain) {
        return audioInf->GetNodeGain(nodeId, (hal_audio_direction_t)direction, gain);
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

    le_result_t res = (*(audioInf->AddNodeStateChangeHandler))(audioNodeId, NodeEventCB);
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
    return audioInf->SendVendorConfig(configPath);
}

le_result_t taf_AudioVhal::CtlReportBubStatus(hal_audio_bubStatus_t bubStatus)
{
    LE_DEBUG("CtlReportBubStatus %d", bubStatus);
    return audioInf->CtlReportBubStatus(bubStatus);
}
