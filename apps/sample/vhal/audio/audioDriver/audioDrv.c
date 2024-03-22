/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafHalAudio.h"

#define MAX_NODES 4
#define CFG_NODE_TYPE "/data/le_fs/node%d/type"
#define CFG_NODE_POWERSTATE "/data/le_fs/node%d/power"
#define CFG_NODE_MUTESTATE "/data/le_fs/node%d/mute"
#define CFG_NODE_GAIN "/data/le_fs/node%d/gain"
#define CFG_NODE_EVENT "/data/le_fs/node%d/event"

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

static int GetNode_Type(uint8_t nodeId)
{
    FILE* file;
    int type = -1;
    char nodeTypePath[50];
    snprintf(nodeTypePath, sizeof(nodeTypePath), CFG_NODE_TYPE, nodeId);
    file = fopen(nodeTypePath, "r");

    if(file == NULL)
    {
        LE_WARN("type node %s doesn't exist", nodeTypePath );
        return -1;
    }
    while(fscanf(file, "%d", &type) != EOF)
    {
        LE_INFO("node %d type is %d", nodeId, type);
    }

    fclose(file);

    return type;
}

static int GetNode_Power(uint8_t nodeId)
{
    FILE* file;
    int power = -1;
    char nodePowerPath[50];
    snprintf(nodePowerPath, sizeof(nodePowerPath), CFG_NODE_POWERSTATE, nodeId);
    file = fopen(nodePowerPath, "r");

    if(file == NULL)
    {
        LE_WARN("type node %s doesn't exist", nodePowerPath );
        return -1;
    }
    while(fscanf(file, "%d", &power) != EOF)
    {
        LE_INFO("node %d power state is %d", nodeId, power);
    }

    fclose(file);

    return power;
}

static int SetNode_Power(uint8_t nodeId, int state)
{
    FILE* file;
    char nodePowerPath[50];
    snprintf(nodePowerPath, sizeof(nodePowerPath), CFG_NODE_POWERSTATE, nodeId);
    file = fopen(nodePowerPath, "w");

    if(file == NULL)
    {
        LE_WARN("type node %s doesn't exist", nodePowerPath );
        return -1;
    }
    LE_INFO("state is %d", state);
    fprintf(file, "%d", state);
    fflush(file);
    fclose(file);

    return 0;
}

static int GetNode_Mute(uint8_t nodeId)
{
    FILE* file;
    int isMute = -1;
    char nodeMutePath[50];
    snprintf(nodeMutePath, sizeof(nodeMutePath), CFG_NODE_MUTESTATE, nodeId);
    file = fopen(nodeMutePath, "r");

    if(file == NULL)
    {
        LE_WARN("type node %s doesn't exist", nodeMutePath );
        return -1;
    }
    while(fscanf(file, "%d", &isMute) != EOF)
    {
        LE_INFO("node %d mute state is %d", nodeId, isMute);
    }

    fclose(file);

    return isMute;
}

static int SetNode_Mute(uint8_t nodeId, int isMute)
{
    FILE* file;
    char nodeMutePath[50];
    snprintf(nodeMutePath, sizeof(nodeMutePath), CFG_NODE_MUTESTATE, nodeId);
    file = fopen(nodeMutePath, "w");

    if(file == NULL)
    {
        LE_WARN("type node %s doesn't exist", nodeMutePath );
        return -1;
    }
    LE_INFO("isMute is %d", isMute);
    fprintf(file, "%d", isMute);
    fflush(file);
    fclose(file);

    return 0;
}

static int GetNode_Gain(uint8_t nodeId, hal_audio_direction_t direction, double *gain)
{
    FILE* file;
    int integer;
    double gainPerc;
    char nodeGainPath[50];
    snprintf(nodeGainPath, sizeof(nodeGainPath), CFG_NODE_GAIN, nodeId);
    file = fopen(nodeGainPath, "r");

    if(file == NULL)
    {
        LE_WARN("type node %s doesn't exist", nodeGainPath );
        return -1;
    }

    while (fscanf(file, "%d:%lf", &integer, &gainPerc) != EOF) {
        if(direction == (hal_audio_direction_t)integer) {
            *gain = gainPerc;
        }
    }

    fclose(file);

    return 0;
}

static int SetNode_Gain(uint8_t nodeId, hal_audio_direction_t direction, double gain)
{
    FILE* file;
    char nodeGainPath[50];
    snprintf(nodeGainPath, sizeof(nodeGainPath), CFG_NODE_GAIN, nodeId);
    file = fopen(nodeGainPath, "w");

    if(file == NULL)
    {
        LE_WARN("type node %s doesn't exist", nodeGainPath );
        return -1;
    }
    LE_INFO("gain is %f direction: %d", gain, direction);

    fprintf(file, "%d:%f", direction, gain);
    fflush(file);
    fclose(file);

    return 0;
}

static void* NodeEventThread
(
    void* contextPtr
)
{
    int event = -1;
    int prevEvent = -1;
        NodeEventHandlerRef_t* nodeEventHandlerPtr = (NodeEventHandlerRef_t*)contextPtr;
        int nodeId = nodeEventHandlerPtr->nodeId;
        char nodeEventPath[50];
        snprintf(nodeEventPath, sizeof(nodeEventPath), CFG_NODE_EVENT, nodeId);
    LE_INFO("Start thread for %d node event", nodeId);

    while(1)
    {
        FILE* file;

        NodeEventHandlerRef_t* nodeEventHandlerPtr = (NodeEventHandlerRef_t*)contextPtr;
        int nodeId = nodeEventHandlerPtr->nodeId;
        char nodeEventPath[50];
        snprintf(nodeEventPath, sizeof(nodeEventPath), CFG_NODE_EVENT, nodeId);

        file = fopen(nodeEventPath, "r");

        if(file != NULL)
        {
            while(fscanf(file, "%d", &event) != EOF)
            {
                if((event != -1) && (prevEvent != event)) {
                    LE_INFO("config node %s is %d", nodeEventPath, event);
                    prevEvent = event;
                    le_dls_Link_t* linkHandlerPtr = NULL;
                    linkHandlerPtr = le_dls_PeekTail(&NodeEventHandlerList);
                    while(linkHandlerPtr)
                    {
                        NodeEventHandlerRef_t * handlerRefPtr = CONTAINER_OF(linkHandlerPtr,
                                NodeEventHandlerRef_t, next);
                        linkHandlerPtr = le_dls_PeekPrev(&NodeEventHandlerList, linkHandlerPtr);
                        if(handlerRefPtr->nodeId == nodeId)
                        {
                            LE_INFO("NodeId registered received the event report it");
                            NodeEvent_t nodeEvent;
                            nodeEvent.nodeId = nodeId;
                            nodeEvent.event = event;
                            le_event_Report(handlerRefPtr->eventId, (void*)&nodeEvent,
                                    sizeof(NodeEvent_t));
                        }
                    }
                }
                le_thread_Sleep(3);
            }
            fclose(file);
        }
    }
    return 0;
}

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
    *nodeType = (hal_audio_NodeType_t)GetNode_Type(nodeId);
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
    if( SetNode_Power(nodeId, (int)state) != -1) {
        return LE_OK;
    }
    return LE_FAULT;
}

static le_result_t taf_hal_GetNodePowerState
(
    uint8_t nodeId,
    hal_audio_PowerState_t *state
)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);
    LE_DEBUG("nodeId : %d", nodeId);
    int pState = GetNode_Power(nodeId);
    LE_DEBUG("pState is %d", pState);
    if(pState != -1) {
        *state = (hal_audio_PowerState_t)pState;
        return LE_OK;
    }
    return LE_FAULT;
}

static le_result_t taf_hal_SetNodeMuteState
(
    uint8_t nodeId,
    bool mute
)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);
    LE_DEBUG("nodeId : %d mute : %s", nodeId, mute ? "true" : "false");
    if( SetNode_Mute(nodeId, mute ? 1 : 0) != -1) {
        return LE_OK;
    }
    return LE_FAULT;
}

static le_result_t taf_hal_GetNodeMuteState
(
    uint8_t nodeId,
    bool *isMuted
)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);
    LE_DEBUG("nodeId : %d", nodeId);
    int muteState = GetNode_Mute(nodeId);
    if(muteState != -1)
    {
        *isMuted = muteState ? true : false;
        return LE_OK;
    }
    return LE_FAULT;
}

static le_result_t taf_hal_SetNodeGain
(
    uint8_t nodeId,
    hal_audio_direction_t direction,
    double gain
)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);
    LE_DEBUG("nodeId : %d direction: %d gain: %f", nodeId, direction, gain);

    int ret = SetNode_Gain(nodeId, direction, gain);
    if(ret < 0){
        return LE_FAULT;
    }

    return LE_OK;
}

static le_result_t taf_hal_GetNodeGain
(
    uint8_t nodeId,
    hal_audio_direction_t direction,
    double *gain
)
{
    LE_DEBUG("AudioTestDrv: %s", __FUNCTION__);
    LE_DEBUG("nodeId : %d", nodeId);

    int ret = GetNode_Gain(nodeId, direction, gain);
    if(ret < 0){
        return LE_FAULT;
    }

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
    //uint8_t* nodeIdPtr = &nodeId;
    le_thread_Ref_t NodeEventThreadRef = le_thread_Create(
                                                    "NodeEventThread",
                                                    NodeEventThread,
                                                    (void *)nodeHandlerRefPtr);
    le_thread_Start(NodeEventThreadRef);
    LE_INFO("Started thread for %d node event", nodeId);
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
        .SetNodeGain = taf_hal_SetNodeGain,
        .GetNodeGain = taf_hal_GetNodeGain,
        .AddNodeStateChangeHandler = taf_hal_AddNodeStateChangeHandler,
    },
};

// DOT NOT USE COMPONET_INIT define module init for your service/apps
COMPONENT_INIT
{
    // Do not put your specfici init in here, define your init
    LE_INFO("Audio Drv is loading\n");
}
