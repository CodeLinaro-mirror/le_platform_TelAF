/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafHalPM.h"

//--------------------------------------------------------------------------------------------------
/**
 * Power request resources.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    POWER_REQUEST_SHUTDOWN,
    POWER_REQUEST_RESTART,
    POWER_REQUEST_SUSPEND
} PowerRequest;

typedef struct
{
    uint8_t nodeId;
    hal_pm_NodeState_t state;
    PowerRequest type;
    hal_pm_PowerMode_t powerMode;
} PowerChangeReq_t;

static le_mem_PoolRef_t PowerRequestPoolRef;


#define CFG_PM_VHAL_MODULE_PATH "pmVHalDrv:/"
#define CFG_NODE_ACK "/data/le_fs/ack"
#define CFG_NODE_EVENT "/data/le_fs/event"
#define CFG_BUB_EVENT "/data/le_fs/bubevent"
#define CFG_WAKEUP_VEHICHLE_RESP "/data/le_fs/wakeupvehichleresp"


//--------------------------------------------------------------------------------------------------
/**
 * Note state change notification resources.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t pm_node_id;
    hal_pm_NodeState_t state;

} NodeStateChangeNotif_t;

static le_mem_PoolRef_t NodeStateChangeNotifPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Shutdown response structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t pmNodeId;
    hal_pm_NodeState_t state;
    hal_pm_PowerMode_t mode;
} ShutdownResp_t;

//--------------------------------------------------------------------------------------------------
/**
 * Restart response structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t pmNodeId;
    hal_pm_NodeState_t state;
    hal_pm_PowerMode_t mode;
} RestartResp_t;

//--------------------------------------------------------------------------------------------------
/**
 * Suspend response structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t pmNodeId;
    hal_pm_NodeState_t state;
    hal_pm_PowerMode_t mode;
} SuspendResp_t;

//--------------------------------------------------------------------------------------------------
/**
 * Note state change response structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t pm_node_id;
    hal_pm_NodeState_t state;
    hal_pm_ConfirmStatus_t status;
} NodeStateChangeResp_t;

//--------------------------------------------------------------------------------------------------
/**
 * Callback functions.
 */
//--------------------------------------------------------------------------------------------------
static hal_pm_WakeupVehicleRspCallbackFunc_t wakeupVehichleRspCallBack = NULL;
static hal_pm_NodeStateChangeReqCallbackFunc_t NodeStateChangeCbFunc = NULL;
static hal_pm_NodeStateChangeNotificationConfirmCallbackFunc_t nodeStateChangeCallbackFunc = NULL;
static hal_pm_NodeEventCallbackFunc_t nodeEventCallback = NULL;
static hal_pm_AddBubStatusCallbackFunc_t bubStatusEventCallback = NULL;
static uint8_t CurrentStayAwakeReason;
//--------------------------------------------------------------------------------------------------
/**
 * Event ID for shutdown response.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t ShutdownRespEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for restart response.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t RestartRespEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for suspend response.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t SuspendRespEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for node state change notification response.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NoteStateChangeRespEventId;

//--------------------------------------------------------------------------------------------------
/**

 * Event ID for node event callback.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NodeEventEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a shutdown response.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessShutdownRespHandler
(
    void* context
)
{
    LE_INFO("PM_Drv: %s", __FUNCTION__);
    uint8_t pmNodeId =  ((RestartResp_t*)context)->pmNodeId;
    hal_pm_NodeState_t state = ((ShutdownResp_t*)context)->state;
    hal_pm_PowerMode_t mode = ((ShutdownResp_t*)context)->mode;

    if(NodeStateChangeCbFunc)
    {
        NodeStateChangeCbFunc(pmNodeId, state, mode);
    }
    else
    {
        LE_ERROR("shutdown callback function is NULL");
    }
    NodeStateChangeCbFunc = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a restart response.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessRestartRespHandler
(
    void* context
)
{
    LE_INFO("PM_Drv: %s", __FUNCTION__);
    uint8_t pmNodeId =  ((RestartResp_t*)context)->pmNodeId;
    hal_pm_NodeState_t state = ((RestartResp_t*)context)->state;
    hal_pm_PowerMode_t mode = ((RestartResp_t*)context)->mode;

    if(NodeStateChangeCbFunc)
    {
        NodeStateChangeCbFunc(pmNodeId, state, mode);
    }
    else
    {
        LE_ERROR("restart callback function is NULL");
    }
    NodeStateChangeCbFunc = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a suspend response.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessSuspendRespHandler
(
    void* context
)
{
    LE_INFO("PM_Drv: %s", __FUNCTION__);
    uint8_t pmNodeId =  ((RestartResp_t*)context)->pmNodeId;
    hal_pm_NodeState_t state = ((RestartResp_t*)context)->state;
    hal_pm_PowerMode_t mode = ((RestartResp_t*)context)->mode;

    if(NodeStateChangeCbFunc)
    {
        NodeStateChangeCbFunc(pmNodeId, state, mode);
    }
    else
    {
        LE_ERROR("suspend callback function is NULL");
    }
    NodeStateChangeCbFunc = NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a note state change response.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessNodeStateChangeRespHandler
(
    void* context
)
{
    LE_INFO("PM_Drv: %s", __FUNCTION__);

    uint8_t pm_node_id = ((NodeStateChangeResp_t*)context)->pm_node_id;
    hal_pm_NodeState_t state = ((NodeStateChangeResp_t*)context)->state;
    hal_pm_ConfirmStatus_t status = ((NodeStateChangeResp_t*)context)->status;

    if(nodeStateChangeCallbackFunc)
    {
        nodeStateChangeCallbackFunc(pm_node_id, state, status);
    }
    else
    {
        LE_ERROR("note state change callback function is NULL");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a node event.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessNodeEventHandler
(
    void* context
)
{
    LE_INFO("PM_Drv: %s", __FUNCTION__);

    char* event = (char*)context;
    if(nodeEventCallback)
    {
        nodeEventCallback(0, event);
    }
    else
    {
        LE_ERROR("note event callback function is NULL");
    }
}

static void taf_hal_PowerOn()
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);
    return;
}

static void taf_hal_PowerOff()
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);
    return;
}

static int taf_hal_HwInit()
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);
    return 0;
}

static int taf_hal_SelfTest()
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);
    return 0;
}

static void* taf_hal_GetModInf(void)
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return &(TAF_HAL_INFO_TAB.pmInf);
}

static void* NodeEventThread
(
    void* contextPtr
)
{
    LE_INFO("PM_Drv: %s", __FUNCTION__);
    while(1)
    {
        FILE* file;
        file = fopen(CFG_NODE_EVENT, "r");
        if(file != NULL)
        {
            char event[100] = {};
            while(fgets(event, sizeof(event), file) != NULL)
            {
                LE_INFO("config node %s is %s", CFG_NODE_EVENT, event);
            }

            fclose(file);

            // fire event to to trigger node event
            le_event_Report(NodeEventEventId, (void*)&event, sizeof(event));
        }
        le_thread_Sleep(15);
    }

    return 0;
}

static void* BubStatusThread
(
    void* contextPtr
)
{
    LE_INFO("PM_Drv: %s", __FUNCTION__);
    int32_t prevAck = -1;
    while(1)
    {
        FILE* file;
        file = fopen(CFG_BUB_EVENT, "r");
        if(file != NULL)
        {
           int32_t ack = -1;
           while(fscanf(file, "%d", &ack) != EOF)
           {
               LE_INFO("config node %s is %d", CFG_BUB_EVENT, ack);
           }
           fclose(file);
           if(prevAck != ack)
           {
           // fire event to call bub status handler
               if(bubStatusEventCallback)
               {
                   int32_t* bubstatus = &ack;
                   LE_INFO("bubstatus: %d", *bubstatus);
                   bubStatusEventCallback(bubstatus);
               }
               else
               {
                   LE_ERROR("Bub Status event callback function is NULL");
               }               prevAck = ack;
           }
           else
           {
               printf("No change in BUB status");
           }
        }
        le_thread_Sleep(10);
    }
    return 0;
}

static int GetConfig_Ack()
{
    FILE* file;
    int ack = -1;

    file = fopen(CFG_NODE_ACK, "r");

    if(file == NULL)
    {
        LE_WARN("config node %s doesn't exist", CFG_NODE_ACK);
        return -1;
    }
    while(fscanf(file, "%d", &ack) != EOF)
    {
        LE_INFO("config node %s is %d", CFG_NODE_ACK, ack);
    }

    fclose(file);

    return ack;
}

static int GetWakeupVehichle_Resp()
{
    FILE* file;
    int ack = -1;

    file = fopen(CFG_WAKEUP_VEHICHLE_RESP, "r");

    if(file == NULL)
    {
        LE_WARN("wakeup vehichle resp %s doesn't exist", CFG_WAKEUP_VEHICHLE_RESP);
        return -1;
    }
    while(fscanf(file, "%d", &ack) != EOF)
    {
        LE_INFO("wakeup vehichle resp %s is %d", CFG_WAKEUP_VEHICHLE_RESP, ack);
    }

    fclose(file);

    return ack;
}
// Used in QueueFunction to process power requests
static void ProcessPowerRequest
(
    void* param1,
    void* param2
)
{
    LE_INFO("PM_Drv: %s", __FUNCTION__);

    PowerChangeReq_t* req = (PowerChangeReq_t*)(param1);

    switch (req->type)
    {
        case POWER_REQUEST_SHUTDOWN:
        {
            // use a timer here to simulate the communication with hardware component
            LE_INFO("send the shutdown request with mode: %u to hardware component ......",
                    req->type);

            // hardware communication
            LE_INFO("get the shutdown response from hardware component ......");

            // set up the response parameters
            ShutdownResp_t resp = {0};
            resp.pmNodeId = req->nodeId;
            resp.state = req->state;
            resp.mode = req->powerMode;

            // fire event to to trigger response process
            le_event_Report(ShutdownRespEventId, (void*)&resp, sizeof(ShutdownResp_t));
        }
        break;

        case POWER_REQUEST_RESTART:
        {
            // use a timer here to simulate the communication with hardware component
            LE_INFO("send the restart request with mode: %u to hardware component ......",
                    req->type);

            // hardware communication

            LE_INFO("get the restart response from hardware component ......");

            // set up the response parameters
            RestartResp_t resp = {0};
            resp.pmNodeId = req->nodeId;
            resp.state = req->state;
            resp.mode = req->powerMode;

            // fire event to to trigger response process
            le_event_Report(RestartRespEventId, (void*)&resp, sizeof(RestartResp_t));
        }
        break;

        case POWER_REQUEST_SUSPEND:
        {
            // use a timer here to simulate the communication with hardware component
            LE_INFO("send the suspend request with mode: %u to hardware component ......",
                    req->type);

            // hardware communication

            LE_INFO("get the suspend response from hardware component ......");

            // set up the response parameters
            SuspendResp_t resp = {0};
            resp.pmNodeId = req->nodeId;
            resp.state = req->state;
            resp.mode = req->powerMode;

            // fire event to to trigger response process
            le_event_Report(SuspendRespEventId, (void*)&resp, sizeof(SuspendResp_t));
        }
        break;

        default:
            LE_ERROR("Invalid power request type: %u", req->type);
            le_mem_Release(req);
            return;
    }
    le_mem_Release(req);
}

// Used in QueueFunction to process node state change notification
static void ProcessNodeStateChangeNotification
(
    void* param1,
    void* param2
)
{
    LE_INFO("PM_Drv: %s", __FUNCTION__);

    NodeStateChangeNotif_t* notif = (NodeStateChangeNotif_t*)param1;

    // use a timer here to simulate the communication with hardware component
    LE_INFO("send the node(%u) state change notification(%u) to hardware component ......",
            notif->pm_node_id, notif->state);

    // hardware communication

    LE_INFO("get the node state change response from hardware component ......");

    // set up the response parameters
    NodeStateChangeResp_t resp = {0};
    resp.pm_node_id = notif->pm_node_id;
    resp.state = notif->state;
    resp.status = HAL_PM_NODE_STATUS_READY;

    // fire event to to trigger response process
    le_event_Report(NoteStateChangeRespEventId, (void*)&resp, sizeof(RestartResp_t));

    le_mem_Release(notif);
}

static le_result_t taf_hal_ShutdownReqAsync
(
    uint8_t pmNodeId,
    hal_pm_NodeState_t state,
    hal_pm_PowerMode_t mode,
    hal_pm_NodeStateChangeReqCallbackFunc_t callback
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);

    NodeStateChangeCbFunc = callback;

    PowerChangeReq_t* req = (PowerChangeReq_t *)le_mem_ForceAlloc(PowerRequestPoolRef);

    if(req == NULL)
    {
        return LE_NO_MEMORY;
    }
    req->nodeId = pmNodeId;
    req->state = state;
    req->type = POWER_REQUEST_SHUTDOWN;
    req->powerMode= mode;

    le_event_QueueFunction(ProcessPowerRequest, (void*)(req), NULL);

    return LE_OK;
}

static le_result_t taf_hal_RestartReqAsync
(
    uint8_t pmNodeId,
    hal_pm_NodeState_t state,
    hal_pm_PowerMode_t mode,
    hal_pm_NodeStateChangeReqCallbackFunc_t callback
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);

    NodeStateChangeCbFunc = callback;

    PowerChangeReq_t* req = (PowerChangeReq_t *)le_mem_ForceAlloc(PowerRequestPoolRef);

    if(req == NULL)
    {
        return LE_NO_MEMORY;
    }

    req->nodeId = pmNodeId;
    req->state = state;
    req->type = POWER_REQUEST_RESTART;
    req->powerMode = mode;

    le_event_QueueFunction(ProcessPowerRequest, (void*)(req), NULL);
    return LE_OK;
}

static le_result_t taf_hal_SuspendReqAsync
(
    uint8_t pmNodeId,
    hal_pm_NodeState_t state,
    hal_pm_PowerMode_t mode,
    hal_pm_NodeStateChangeReqCallbackFunc_t callback
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);

    NodeStateChangeCbFunc = callback;

    PowerChangeReq_t* req = (PowerChangeReq_t *)le_mem_ForceAlloc(PowerRequestPoolRef);

    if(req == NULL)
    {
        return LE_NO_MEMORY;
    }

    req->nodeId = pmNodeId;
    req->state = state;
    req->type = POWER_REQUEST_SUSPEND;
    req->powerMode = mode;

    le_event_QueueFunction(ProcessPowerRequest, (void*)(req), NULL);
    return LE_OK;
}
static le_result_t taf_hal_WakeupVehicleReqAsync
(
    int32_t reason,
    hal_pm_WakeupVehicleRspCallbackFunc_t  callback
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);
    wakeupVehichleRspCallBack = callback;
    if(reason == HAL_PM_VEHICHLE_WAKEUP_REASON_DEFAULT)
    {
        int32_t response;
        response = (int32_t)GetWakeupVehichle_Resp();
        if(response == -1)
           return LE_FAULT;
        callback(reason,response);
        return LE_OK;
    }
    else
    {
        return LE_BAD_PARAMETER;
    }
}

static le_result_t taf_hal_NodeStateChangePrepareAsync(
   uint8_t pmNodeId,
   hal_pm_NodeState_t state,
   hal_pm_PowerMode_t mode,
   const uint8_t reason,
   hal_pm_NodeStateChangePrepareCallbackFunc_t callback
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);
    LE_INFO("reason %d", reason);

    hal_pm_RspReason_t responseReason = (hal_pm_RspReason_t)GetConfig_Ack();
    if(responseReason == 2)
    {
        return LE_OK;
    }
    callback(pmNodeId, state, mode, reason, responseReason);

    return LE_OK;
}

static le_result_t taf_hal_NodeStateChangeReqAsync
(
   uint8_t pmNodeId,
   hal_pm_NodeState_t state,
   hal_pm_PowerMode_t mode,
   hal_pm_NodeStateChangeReqCallbackFunc_t callback
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);
    le_result_t res = LE_FAULT;
    if(state == HAL_PM_NODE_STATE_SHUTDOWN)
    {
        res = taf_hal_ShutdownReqAsync(pmNodeId, state, mode, callback);
    }
    else if(state == HAL_PM_NODE_STATE_RESTART)
    {
        res = taf_hal_RestartReqAsync(pmNodeId, state, mode, callback);
    }
    else if(state == HAL_PM_NODE_STATE_SUSPEND)
    {
        res = taf_hal_SuspendReqAsync(pmNodeId, state, mode, callback);
    }
    else
    {
        LE_INFO("given state is not supported");
    }
    return res;
}

static void taf_hal_NodeStateChangeNotification
(
    uint8_t pm_node_id,
    hal_pm_NodeState_t state,
    hal_pm_NodeStateChangeNotificationConfirmCallbackFunc_t callback
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);

    nodeStateChangeCallbackFunc = callback;

    NodeStateChangeNotif_t* notif =
        (NodeStateChangeNotif_t *)le_mem_ForceAlloc(NodeStateChangeNotifPoolRef);
    notif->pm_node_id = pm_node_id;
    notif->state = state;

    le_event_QueueFunction(ProcessNodeStateChangeNotification, (void*)(notif), NULL);
}

static void taf_hal_nodeInfoNotification
(
    uint8_t pm_node_id,
    hal_pm_NodeInfo_t info,
    const uint8_t reason
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);

    LE_INFO("hal_pm_NodeInfo_t: %d", info);
    CurrentStayAwakeReason = reason;
    LE_INFO("reason: %d", reason);
}

static le_result_t taf_hal_addNodeEventHanlder
(
    uint8_t pm_node_id,
    hal_pm_NodeEventCallbackFunc_t callback
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);

    LE_INFO("pm_node_id: %d", pm_node_id);

    nodeEventCallback = callback;
    return LE_OK;
}

static le_result_t taf_hal_AddBubStatusHandler
(
    hal_pm_AddBubStatusCallbackFunc_t callback
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);
    bubStatusEventCallback = callback;
    return LE_OK;
}

static le_result_t taf_hal_GetBubStatus
(
    int32_t *status
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);
    int32_t ack = -1;
    FILE* file;
    file = fopen(CFG_BUB_EVENT, "r");
    if(file != NULL)
    {
       while(fscanf(file, "%d", &ack) != EOF)
       {
           LE_INFO("config node %s is %d", CFG_BUB_EVENT, ack);
       }
       fclose(file);
    }
    *status = ack;
    LE_INFO("status %d", *status);
    return LE_OK;
}
// Initialization function
static void Init(void)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);

    // Create the memory pools.
    PowerRequestPoolRef = le_mem_CreatePool("PowerChangeRequest", sizeof(PowerChangeReq_t));

    // Allocation pool size.
    le_mem_ExpandPool(PowerRequestPoolRef, 20); // 20 requests supported max

    // Create the memory pools.
    NodeStateChangeNotifPoolRef = le_mem_CreatePool("NodeStateChangeNotif",
                                                    sizeof(NodeStateChangeNotif_t));

    // Allocation pool size.
    le_mem_ExpandPool(NodeStateChangeNotifPoolRef, 30); // 30 notifications supported max

    // Create an event Id for shutdown response event.
    ShutdownRespEventId = le_event_CreateId("ShutdownRespEventId", sizeof(ShutdownResp_t));

    // Register handler for shutdown response events.
    le_event_AddHandler("ProcessShutdownRespHandler",
                            ShutdownRespEventId,
                            ProcessShutdownRespHandler);

    // Create an event Id for restart response event.
    RestartRespEventId = le_event_CreateId("RestartRespEventId", sizeof(RestartResp_t));

    // Register handler for restart response events.
    le_event_AddHandler("ProcessRestartRespHandler",
                            RestartRespEventId,
                            ProcessRestartRespHandler);

    // Create an event Id for suspend response event.
    SuspendRespEventId = le_event_CreateId("SuspendRespEventId", sizeof(SuspendResp_t));

    // Register handler for restart response events.
    le_event_AddHandler("ProcessSuspendRespHandler",
                            SuspendRespEventId,
                            ProcessSuspendRespHandler);

    // Create an event Id for node state change response event.
    NoteStateChangeRespEventId = le_event_CreateId("NoteStateChangeRespEventId",
                                                    sizeof(NodeStateChangeResp_t));

    // Register handler for node state change response events.
    le_event_AddHandler("ProcessShutdownRespHandler",
                            NoteStateChangeRespEventId,
                            ProcessNodeStateChangeRespHandler);

    // Create an event Id for node event event.
    NodeEventEventId = le_event_CreateId("NodeEventEventId",
                                         (sizeof(char) * 100));

    // Register handler for event events.
    le_event_AddHandler("ProcessNodeEventHandler",
                            NodeEventEventId,
                            ProcessNodeEventHandler);

    le_thread_Ref_t NodeEventThreadRef = le_thread_Create(
                                                    "NodeEventThread",
                                                    NodeEventThread,
                                                    NULL);
    le_thread_Start(NodeEventThreadRef);

    le_thread_Ref_t BubStatusThreadRef = le_thread_Create(
                                                    "BubStatusThread",
                                                    BubStatusThread,
                                                    NULL);
    le_thread_Start(BubStatusThreadRef);
}

LE_SHARED hal_pm_InfoTab_t TAF_HAL_INFO_TAB = {
    // management interface always comes first
    .mgrInf = {
        .name = TAF_PM_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .moduleType = TAF_MODULETYPE_HAL,
        .serviceMax = 1,
        .hwInitInf = taf_hal_HwInit,
        .powerOffInf = taf_hal_PowerOff,
        .powerOnInf = taf_hal_PowerOn,
        .selfTest = taf_hal_SelfTest,
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    },

    .pmInf = {
        .InitHAL = Init,
        .nodeStateChangeNotification = taf_hal_NodeStateChangeNotification,
        .nodeInfoNotification = taf_hal_nodeInfoNotification,
        .addNodeEventHandler = taf_hal_addNodeEventHanlder,
        .wakeupVehicleReqAsync = taf_hal_WakeupVehicleReqAsync,
        .nodeStateChangePrepareAsync = taf_hal_NodeStateChangePrepareAsync,
        .nodeStateChangeReqAsync = taf_hal_NodeStateChangeReqAsync,
        .addBubStatusHandler = taf_hal_AddBubStatusHandler,
        .getBubStatus = taf_hal_GetBubStatus
    }
};

// DOT NOT USE COMPONET_INIT define module init for your service/apps
COMPONENT_INIT
{
    LE_INFO("Test Drv is loading");
}
