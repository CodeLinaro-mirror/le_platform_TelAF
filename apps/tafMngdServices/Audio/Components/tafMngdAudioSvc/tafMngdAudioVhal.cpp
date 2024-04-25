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

#include "tafMngdAudioVhal.hpp"
#include "tafSvcIF.hpp"

using namespace taf::audioVhal;

LE_MEM_DEFINE_STATIC_POOL(NodeEventHandlerRef, MAX_VENDOR_NODES,
        sizeof(NodeEventHandlerRefNode_t));

/**
 * Returns managed audio vhal instance
 */
taf_MngdAudioVhal &taf_MngdAudioVhal::GetInstance()
{
    static taf_MngdAudioVhal instance;
    return instance;
}

void taf_MngdAudioVhal::Init()
{
    // load driver
    audioInf = (audio_Inf_t *)taf_devMgr_LoadDrv(TAF_AUDIO_MODULE_NAME, nullptr);

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

bool taf_MngdAudioVhal::isAudioDrvAvailable()
{
    LE_DEBUG("isVhalLoaded : %s", isVhalLoaded ? "true" : "false");
    return isVhalLoaded;
}

le_result_t taf_MngdAudioVhal::OpenRoute(bool status, taf_mngd_audio_RouteId_t routeId,
        taf_mngd_audio_Mode_t mode)
{
    LE_DEBUG("OpenRoute status %s route %d mode %d", (status ? "true" : "false"), routeId, mode);
    return audioInf->CtlSetAudioStatus(status, (uint32_t)routeId, (taf_hal_audio_Mode)mode);
}

le_result_t taf_MngdAudioVhal::GetNodeType( uint8_t audioNodeId,
        taf_mngd_audioHw_NodeType_t *nodeType )
{
    LE_DEBUG("GetNodeType %d", audioNodeId);
    taf_hal_audio_NodeType halNodeType;
    le_result_t res = audioInf->GetNodeType(audioNodeId, &halNodeType);
    if(res == LE_OK)
    {
        if(halNodeType == AUDIO_HAL_NODE_CODEC)
            *nodeType = TAF_MNGD_AUDIOHW_AUDIO_CODEC;
        else if(halNodeType == AUDIO_HAL_NODE_PA)
            *nodeType = TAF_MNGD_AUDIOHW_AUDIO_PA;
        else if(halNodeType == AUDIO_HAL_NODE_A2B)
            *nodeType = TAF_MNGD_AUDIOHW_AUDIO_A2B;
        else
            *nodeType = TAF_MNGD_AUDIOHW_INVALID;
    }
    return res;
}

le_result_t taf_MngdAudioVhal::SendNodeVendorConfig(uint8_t audioNodeId, const char* configPath)
{
    LE_DEBUG("SendNodeVendorConfig Node id : %d configPath : %s", audioNodeId, configPath);
    return audioInf->SendNodeVendorConfig(audioNodeId, configPath);
}

le_result_t taf_MngdAudioVhal::SetNodePowerState(uint8_t audioNodeId,
        taf_mngd_audioHw_NodePowerState_t state)
{
    LE_DEBUG("SetNodePowerState node id : %d state : %d", audioNodeId, state);
    return audioInf->SetNodePowerState(audioNodeId, (taf_hal_audio_Powerstate)state);
}

le_result_t taf_MngdAudioVhal::GetNodePowerState(uint8_t audioNodeId,
        taf_mngd_audioHw_NodePowerState_t* state)
{
    LE_DEBUG("GetNodePowerState node id : %d audioInf %p", audioNodeId, audioInf);
    taf_hal_audio_Powerstate vhalState;
    le_result_t res = audioInf->GetNodePowerState(audioNodeId, &vhalState);

    *state = (taf_mngd_audioHw_NodePowerState_t)vhalState;
    LE_DEBUG("state is %d", *state);
    return res;
}

le_result_t taf_MngdAudioVhal::SetNodeMuteState(uint8_t audioNodeId, bool mute)
{
    LE_DEBUG("SetNodeMuteState node id : %d mute : %s", audioNodeId, mute ? "true" : "false");
    return audioInf->SetNodeMuteState(audioNodeId, mute);
}

le_result_t taf_MngdAudioVhal::GetNodeMuteState(uint8_t audioNodeId, bool *isMuted)
{
    LE_DEBUG("GetNodeMuteState node id : %d ", audioNodeId);
    return audioInf->GetNodeMuteState(audioNodeId, isMuted);
}

void taf_MngdAudioVhal::NodeEventHandler(void* reportPtr, void* secondLayerHandlerFunc)
{
    taf_MngdAudioVhal mngdAudioVhal = taf_MngdAudioVhal::GetInstance();
    NodeEvent_t* eventPtr = (NodeEvent_t*)reportPtr;
    LE_DEBUG("NodeEventHandler nodeId %d event %d", eventPtr->nodeId, eventPtr->event);
    le_dls_Link_t* linkHandlerPtr = NULL;
    linkHandlerPtr = le_dls_PeekTail(&mngdAudioVhal.NodeEventHandlerList);
    while(linkHandlerPtr)
    {
        NodeEventHandlerRefNode_t * handlerRefPtr = CONTAINER_OF(linkHandlerPtr,
                NodeEventHandlerRefNode_t, next);
        linkHandlerPtr = le_dls_PeekPrev(&mngdAudioVhal.NodeEventHandlerList, linkHandlerPtr);
        if(handlerRefPtr->nodeId == eventPtr->nodeId)
        {
            LE_INFO("NodeId registered received the event");
            taf_mngd_audioHw_NodeStateHandlerFunc_t clientFunc =
                    (taf_mngd_audioHw_NodeStateHandlerFunc_t)secondLayerHandlerFunc;
            clientFunc(eventPtr->nodeId, eventPtr->event, handlerRefPtr->userCtx);
        }
    }
}

void taf_MngdAudioVhal::NodeEventCB(uint8_t nodeId, taf_hal_audio_DevEvent event)
{
    LE_DEBUG("NodeEventCB nodeId : %d event : %d", nodeId, event);
    taf_MngdAudioVhal mngdAudioVhal = taf_MngdAudioVhal::GetInstance();
    NodeEvent_t nodeEvent;
    nodeEvent.nodeId = nodeId;
    nodeEvent.event = (taf_mngd_audioHw_Event_t)event;
    // Notify the repective callbackFunc
    le_dls_Link_t* linkHandlerPtr = NULL;
    linkHandlerPtr = le_dls_PeekTail(&mngdAudioVhal.NodeEventHandlerList);
    while(linkHandlerPtr)
    {
        NodeEventHandlerRefNode_t * handlerRefPtr = CONTAINER_OF(linkHandlerPtr,
                NodeEventHandlerRefNode_t, next);
        linkHandlerPtr = le_dls_PeekPrev(&mngdAudioVhal.NodeEventHandlerList, linkHandlerPtr);
        if(handlerRefPtr->nodeId == nodeId)
        {
            LE_INFO("NodeId registered received the event");
            le_event_Report(handlerRefPtr->eventId, (void*)&nodeEvent, sizeof(NodeEvent_t));
        }
    }
}

taf_mngd_audioHw_NodeStateChangeHandlerRef_t taf_MngdAudioVhal::AddNodeStateChangeHandler(
        uint8_t audioNodeId, taf_mngd_audioHw_NodeStateHandlerFunc_t handlerPtr, void* contextPtr)
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

    nodeHandlerRefPtr->handlerRef =(taf_mngd_audioHw_NodeStateChangeHandlerRef_t)
            le_event_AddLayeredHandler("NodeEventHandler", nodeHandlerRefPtr->eventId,
            NodeEventHandler, (void*)handlerPtr);

    nodeHandlerRefPtr->userCtx = contextPtr;

    le_dls_Queue(&NodeEventHandlerList, &nodeHandlerRefPtr->next);

    return nodeHandlerRefPtr->handlerRef;
}

void taf_MngdAudioVhal::RemoveNodeStateChangeHandler(
        taf_mngd_audioHw_NodeStateChangeHandlerRef_t handlerRef )
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

le_result_t taf_MngdAudioVhal::SendVendorConfig(const char* configPath)
{
    LE_DEBUG("SendVendorConfig %s", configPath);
    return audioInf->SendVendorConfig(configPath);
}
