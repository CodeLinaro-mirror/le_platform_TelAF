/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
    PowerRequest type;
    union
    {
        hal_pm_ShutdownMode_t shutdownMode;
        hal_pm_RestartMode_t restartMode;
        hal_pm_SuspendMode_t suspendMode;
    };
} PowerChangeReq_t;

static le_mem_PoolRef_t PowerRequestPoolRef;

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
    hal_pm_ShutdownMode_t mode;
    hal_pm_RspReason_t reason;
} ShutdownResp_t;

//--------------------------------------------------------------------------------------------------
/**
 * Restart response structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    hal_pm_RestartMode_t mode;
    hal_pm_RspReason_t reason;
} RestartResp_t;

//--------------------------------------------------------------------------------------------------
/**
 * Restart response structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    hal_pm_SuspendMode_t mode;
    hal_pm_RspReason_t reason;
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
static hal_pm_ShutDownRspCallbackFunc_t shutdownCallbackFunc = NULL;
static hal_pm_RestartRspCallbackFunc_t restartCallbackFunc = NULL;
static hal_pm_SuspendRspCallbackFunc_t suspendCallbackFunc = NULL;
static hal_pm_NodeStateChangeNotificationConfirmCallbackFunc_t nodeStateChangeCallbackFunc = NULL;
static hal_pm_NodeEventCallbackFunc_t nodeEventCallback = NULL;

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
 * Handler to process a shutdown response.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessShutdownRespHandler
(
    void* context
)
{
    LE_INFO("PM_Drv: %s", __FUNCTION__);

    hal_pm_ShutdownMode_t mode = ((ShutdownResp_t*)context)->mode;
    hal_pm_RspReason_t reason = ((ShutdownResp_t*)context)->reason;

    if(shutdownCallbackFunc)
    {
        shutdownCallbackFunc(mode, reason);
    }
    else
    {
        LE_ERROR("shutdown callback function is NULL");
    }
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

    hal_pm_RestartMode_t mode = ((RestartResp_t*)context)->mode;
    hal_pm_RspReason_t reason = ((RestartResp_t*)context)->reason;

    if(restartCallbackFunc)
    {
        restartCallbackFunc(mode, reason);
    }
    else
    {
        LE_ERROR("restart callback function is NULL");
    }
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

    hal_pm_SuspendMode_t mode = ((RestartResp_t*)context)->mode;
    hal_pm_RspReason_t reason = ((RestartResp_t*)context)->reason;

    if(suspendCallbackFunc)
    {
        suspendCallbackFunc(mode, reason);
    }
    else
    {
        LE_ERROR("suspend callback function is NULL");
    }
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
            resp.mode = req->shutdownMode;
            resp.reason = HAL_PM_RSP_READY;

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
            resp.mode = req->restartMode;
            resp.reason = HAL_PM_RSP_READY;

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
            resp.mode = req->suspendMode;
            resp.reason = HAL_PM_RSP_READY;

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
    hal_pm_ShutdownMode_t mode,
    hal_pm_ShutDownRspCallbackFunc_t callback
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);

    shutdownCallbackFunc = callback;

    PowerChangeReq_t* req = (PowerChangeReq_t *)le_mem_ForceAlloc(PowerRequestPoolRef);

    if(req == NULL)
    {
        return LE_NO_MEMORY;
    }

    req->type = POWER_REQUEST_SHUTDOWN;
    req->shutdownMode= mode;

    le_event_QueueFunction(ProcessPowerRequest, (void*)(req), NULL);

    return LE_OK;
}

static le_result_t taf_hal_RestartReqAsync
(
    hal_pm_RestartMode_t mode,
    hal_pm_RestartRspCallbackFunc_t callback
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);

    restartCallbackFunc = callback;

    PowerChangeReq_t* req = (PowerChangeReq_t *)le_mem_ForceAlloc(PowerRequestPoolRef);

    if(req == NULL)
    {
        return LE_NO_MEMORY;
    }

    req->type = POWER_REQUEST_RESTART;
    req->restartMode = mode;

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
        response = HAL_PM_VEHICHLE_WAKEUP_STATUS_AWAKE;
        callback(reason,response);
        return LE_OK;
    }
    else
    {
        return LE_BAD_PARAMETER;
    }
}

static le_result_t taf_hal_NodeStateChangePrepareAsync
(
   uint8_t pmNodeId,
   hal_pm_NodeState_t state,
   hal_pm_ShutdownMode_t mode,
   hal_pm_NodeStateChangePrepareCallbackFunc_t callback
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);

    callback(pmNodeId, state, mode, HAL_PM_RSP_READY);

    return LE_OK;
}

static le_result_t taf_hal_NodeStateChangeReqAsync
(
   uint8_t pmNodeId,
   hal_pm_NodeState_t state,
   hal_pm_ShutdownMode_t mode,
   hal_pm_NodeStateChangeReqCallbackFunc_t callback
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);

    callback(pmNodeId, state, mode);

    return LE_OK;
}

static le_result_t taf_hal_SuspendReqAsync
(
    hal_pm_SuspendMode_t mode,
    hal_pm_SuspendRspCallbackFunc_t callback
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);

    suspendCallbackFunc = callback;

    PowerChangeReq_t* req = (PowerChangeReq_t *)le_mem_ForceAlloc(PowerRequestPoolRef);

    if(req == NULL)
    {
        return LE_NO_MEMORY;
    }

    req->type = POWER_REQUEST_SUSPEND;
    req->suspendMode = mode;

    le_event_QueueFunction(ProcessPowerRequest, (void*)(req), NULL);
    return LE_OK;
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
    const char* vhalTag
)
{
    LE_INFO("PM_VHAL: %s", __FUNCTION__);

    LE_INFO("hal_pm_NodeInfo_t: %d", info);

    LE_INFO("vhalTag: %s", vhalTag);
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
        .shutdownReqAsync = taf_hal_ShutdownReqAsync,
        .restartReqAsync = taf_hal_RestartReqAsync,
        .suspendReqAsync = taf_hal_SuspendReqAsync,
        .nodeStateChangeNotification = taf_hal_NodeStateChangeNotification,
        .nodeInfoNotification = taf_hal_nodeInfoNotification,
        .addNodeEventHandler = taf_hal_addNodeEventHanlder,
        .wakeupVehicleReqAsync = taf_hal_WakeupVehicleReqAsync,
        .nodeStateChangePrepareAsync = taf_hal_NodeStateChangePrepareAsync,
        .nodeStateChangeReqAsync = taf_hal_NodeStateChangeReqAsync
    }
};

// DOT NOT USE COMPONET_INIT define module init for your service/apps
COMPONENT_INIT
{
    LE_INFO("Test Drv is loading");
}
