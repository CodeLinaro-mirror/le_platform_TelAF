/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafHalAudio.h"

#define MAX_NODES 4

typedef struct
{
    hal_audio_DevStateChangeCallback_t   callback;
    le_event_HandlerRef_t                handlerRef;
    uint8_t                              nodeId;
    le_dls_Link_t                        next;
    le_event_Id_t                        eventId;
}
NodeEventHandlerRef_t;

typedef struct {
    uint8_t nodeId;
    hal_audio_DevEvent_t event;
}NodeEvent_t;

le_mem_PoolRef_t NodeEventHandlerRefPool = NULL;
le_dls_List_t NodeEventHandlerList;

LE_MEM_DEFINE_STATIC_POOL(NodeEventHandlerRef, MAX_NODES, sizeof(NodeEventHandlerRef_t));

static le_result_t taf_hal_CtlSetAudioStatus(bool status,
     uint32_t route, hal_audio_Mode_t mode)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);
    LE_DEBUG("status : %s route : %d mode : %d", status ? "true" : "false", route, mode);
    return LE_OK;
}

static le_result_t taf_hal_SendVendorConfig
(
    const char* configPath
)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);
    LE_DEBUG("configPath : %s", configPath);
    return LE_OK;
}

static le_result_t taf_hal_GetNodeType
(
    uint8_t nodeId,
    hal_audio_NodeType_t *nodeType
)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);
    LE_DEBUG("nodeId %d ",nodeId);
    *nodeType = HAL_AUDIO_NODE_TYPE_CODEC;
    return LE_OK;
}

static le_result_t taf_hal_SendNodeVendorConfig
(
    uint8_t nodeId,
    const char* configPath
)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);
    LE_DEBUG("nodeId : %d configPath : %s", nodeId, configPath);
    return LE_OK;
}

static le_result_t taf_hal_SetNodePowerState
(
    uint8_t nodeId,
    hal_audio_PowerState_t state
)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);
    LE_DEBUG("nodeId : %d powerState : %d", nodeId, state);
    return LE_OK;
}

static le_result_t taf_hal_GetNodePowerState
(
    uint8_t nodeId,
    hal_audio_PowerState_t *state
)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);
    LE_DEBUG("nodeId : %d", nodeId);
    return LE_OK;
}

static le_result_t taf_hal_SetNodeMuteState
(
    uint8_t nodeId,
    bool mute
)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);
    LE_DEBUG("nodeId : %d mute : %s", nodeId, mute ? "true" : "false");
    return LE_OK;
}

static le_result_t taf_hal_GetNodeMuteState
(
    uint8_t nodeId,
    bool *isMuted
)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);
    LE_DEBUG("nodeId : %d", nodeId);
    return LE_OK;
}

static void NodeEventHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    NodeEvent_t* nodeEventPtr = (NodeEvent_t*)reportPtr;
    LE_INFO("NodeEventHandler nodeId : %d event : %d", nodeEventPtr->nodeId, nodeEventPtr->event);

    le_dls_Link_t* linkHandlerPtr = NULL;
    linkHandlerPtr = le_dls_PeekTail(&NodeEventHandlerList);
    while(linkHandlerPtr)
    {
        NodeEventHandlerRef_t * handlerRefPtr = CONTAINER_OF(linkHandlerPtr,
                NodeEventHandlerRef_t, next);
        linkHandlerPtr = le_dls_PeekPrev(&NodeEventHandlerList, linkHandlerPtr);
        if(handlerRefPtr->nodeId == nodeEventPtr->nodeId)
        {
            LE_INFO("NodeId registered received the event");
            hal_audio_DevStateChangeCallback_t cbFunc =
                    (hal_audio_DevStateChangeCallback_t)secondLayerHandlerFunc;
            cbFunc(nodeEventPtr->nodeId, nodeEventPtr->event);
        }
    }
}

static le_result_t taf_hal_AddNodeStateChangeHandler
(
    uint8_t nodeId,
    hal_audio_DevStateChangeCallback_t callback
)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);

    LE_DEBUG("nodeId: %d", nodeId);

    le_dls_Link_t* linkHandlerPtr = NULL;
    linkHandlerPtr = le_dls_PeekTail(&NodeEventHandlerList);
    while(linkHandlerPtr)
    {
        NodeEventHandlerRef_t * handlerRefPtr = CONTAINER_OF(linkHandlerPtr,
                NodeEventHandlerRef_t, next);
        linkHandlerPtr = le_dls_PeekPrev(&NodeEventHandlerList, linkHandlerPtr);
        if(handlerRefPtr->nodeId == nodeId)
        {
            LE_INFO("NodeId already registered");
            return LE_OK;
        }
    }

    NodeEventHandlerRef_t* nodeHandlerRefPtr = (NodeEventHandlerRef_t*)le_mem_ForceAlloc(
            NodeEventHandlerRefPool);

    nodeHandlerRefPtr->next = LE_DLS_LINK_INIT;

    nodeHandlerRefPtr->nodeId = nodeId;

    char nodeEventId[50];
    snprintf(nodeEventId, sizeof(nodeEventId), "nodeEvent-%d", nodeId);
    nodeHandlerRefPtr->eventId = le_event_CreateId(nodeEventId, sizeof(NodeEvent_t));

    nodeHandlerRefPtr->handlerRef = le_event_AddLayeredHandler("NodeEventHandler",
            nodeHandlerRefPtr->eventId, NodeEventHandler, (void*)callback);

    le_dls_Queue(&NodeEventHandlerList, &nodeHandlerRefPtr->next);

    return LE_OK;
}

static le_result_t taf_hal_CtlReportBubStatus(hal_audio_bubStatus_t bubStatus)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);
    LE_DEBUG("status : %d", bubStatus );
    return LE_OK;
}

static void taf_hal_PowerOn()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return;
}

static void taf_hal_PowerOff()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return;
}

static int taf_hal_HwInit()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return 0;
}

static int taf_hal_SelfTest()
{
    LE_INFO("AudioTestDrv: %s", __FUNCTION__);
    return 0;
}

static void* taf_hal_GetModInf(void)
{
    LE_INFO("AudioTestDrv: %s", __FUNCTION__);
    return &(TAF_HAL_INFO_TAB.audioInf);
}

static void Init(void)
{
    LE_INFO("Audio Test Driver init");
    NodeEventHandlerRefPool = le_mem_InitStaticPool(NodeEventHandlerRef, MAX_NODES,
            sizeof(NodeEventHandlerRef_t));
    NodeEventHandlerList = LE_DLS_LIST_INIT;
}

LE_SHARED hal_audio_InfoTab_t TAF_HAL_INFO_TAB = {
    // always come first
    .mgrInf = {
        .name = TAF_AUDIO_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .moduleType = TAF_MODULETYPE_HAL,
        .serviceMax = 1,
        .hwInitInf = taf_hal_HwInit,
        .powerOffInf = taf_hal_PowerOff,
        .powerOnInf = taf_hal_PowerOn,
        .selfTest = taf_hal_SelfTest, // tafModule will send test command
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    },

    .audioInf = {
        .InitHAL = Init,
        .CtlSetAudioStatus = taf_hal_CtlSetAudioStatus,
        .SendVendorConfig = taf_hal_SendVendorConfig,
        .CtlReportBubStatus = taf_hal_CtlReportBubStatus,
        .GetNodeType = taf_hal_GetNodeType,
        .SendNodeVendorConfig = taf_hal_SendNodeVendorConfig,
        .SetNodePowerState = taf_hal_SetNodePowerState,
        .GetNodePowerState = taf_hal_GetNodePowerState,
        .SetNodeMuteState = taf_hal_SetNodeMuteState,
        .GetNodeMuteState = taf_hal_GetNodeMuteState,
        .AddNodeStateChangeHandler = taf_hal_AddNodeStateChangeHandler,
    },
};

// DOT NOT USE COMPONET_INIT define module init for your service/apps
COMPONENT_INIT
{
    // Do not put your specfici init in here, define your init
    LE_INFO("Audio Drv is loading\n");
}
