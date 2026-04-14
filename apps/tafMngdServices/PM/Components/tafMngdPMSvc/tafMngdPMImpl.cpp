/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafMngdPMSvc.hpp"
#include "tafMngdPMCommon.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "rpcPm/tafMngdRpcPm.hpp"
#include "limit.h"
#include <setjmp.h>

using namespace tafsvc;
namespace pt = boost::property_tree;

#define MAX_NUM_OF_RETRY   10
#define RETRY_TIMER_INTERVAL  3000
#define TIMER_SAFECALL 5
DECLARE_SAFE_CALL();

/**
 * To convert TafState to string
 */
const char* tafMngdPMSvc::TafStateToString(uint8_t tafState)
{
    const char *state;
    switch(tafState)
    {
        case TAF_MNGDPM_STATE_RESUME:
            state = "Resume";
            break;
        case TAF_MNGDPM_STATE_SUSPEND:
            state = "Suspend";
            break;
        case TAF_MNGDPM_STATE_SHUTDOWN:
            state = "Shutdown";
            break;
        case TAF_MNGDPM_STATE_RESTART:
            state = "Restart";
            break;
        case TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE:
            state = "Releasing wake source";
            break;
        case TAF_MNGDPM_STATE_SUSPENDING:
            state = "Suspending";
            break;
        case TAF_MNGDPM_STATE_RESTARTING:
            state = "Restarting";
            break;
        case TAF_MNGDPM_STATE_SHUTTING_DOWN:
            state = "Shutting down";
            break;
        case TAF_MNGDPM_STATE_WAKING_UP:
            state = "Waking up";
            break;
        default :
            state = "Unknown";
            break;
    }
    return state;
}

/**
 * Parse JSON config file
 */
le_result_t tafMngdPMSvc::ParseJsonConfiguration(std::string configPath)
{
    LE_DEBUG("ParseJsonConfig %s", configPath.c_str());

    if (configPath.empty())
    {
        LE_ERROR("configPath is empty!");
        return LE_FAULT;
    }

    std::ifstream jsonFile(configPath);
    if (!jsonFile.is_open())
    {
        LE_WARN ("Unable to open %s", configPath.c_str());
        configPath = TAF_MNGDPM_DEFAULT_CONF_PATH;
        std::ifstream jsonFile(configPath);
        if(!jsonFile.is_open())
            return LE_FAULT;
    }

    // Create a root
    pt::ptree root;
    // Load the json file in this ptree
    try
    {
        pt::read_json(configPath, root);
    }
    catch (const std::exception &e)
    {
        LE_WARN ("read_json exception: %s. Check validity of JSON.", e.what());
        return LE_FAULT;
    }
    auto &mpms = tafMngdPMSvc::GetInstance();

    long int bootup_awake_time = root.get<int>("bootup_awake_time");
    LE_INFO("bootup_awake_time is %ld", bootup_awake_time);
    mpms.config.bootup_awake_time = bootup_awake_time;

    bool hal_enabled = root.get<bool>("hal_enabled");
    LE_INFO("hal_enabled is %d", hal_enabled);
    mpms.config.hal_enabled = hal_enabled;

    long int hal_state_prepare_timeout = root.get<int>("hal_state_prepare_timeout");
    LE_INFO("hal_state_prepare_timeout is %ld", hal_state_prepare_timeout);
    mpms.config.hal_state_prepare_timeout = hal_state_prepare_timeout;

    long int hal_wakeup_vehicle_timeout = root.get<int>("hal_wakeup_vehicle_timeout");
    LE_INFO("hal_wakeup_vehicle_timeout is %ld", hal_wakeup_vehicle_timeout);
    mpms.config.hal_wakeup_vehicle_timeout = hal_wakeup_vehicle_timeout;

    long int state_change_ack_timeout = root.get<int>("state_change_ack_timeout");
    LE_INFO("state_change_ack_timeout is %ld", state_change_ack_timeout);
    mpms.config.state_change_ack_timeout = state_change_ack_timeout;

    return LE_OK;
}

/**
 * Set shutdown state to NAD
 */
le_result_t tafMngdPMSvc::ShutdownNAD()
{
    le_result_t res = taf_pm_SetAllVMPowerState(TAF_PM_STATE_SHUTDOWN);
    if(res != LE_OK)
    {
        LE_ERROR("Failed to shutdown the NAD");
    }
    else
    {
        LE_INFO("Shutting down the NAD!");
        le_hashmap_It_Ref_t hashIter =
                (le_hashmap_It_Ref_t)le_hashmap_GetIterator(vmStateHashmap);
        while (LE_OK == le_hashmap_NextNode(hashIter))
        {
            taf_mngdPm_vmState_t *vmStatePtr =
                    (taf_mngdPm_vmState_t*)le_hashmap_GetValue(hashIter);

            if(vmStatePtr) {
                vmStatePtr->state = TAF_MNGDPM_STATE_SHUTDOWN;
            }
        }
    }
    return res;
}

/**
 * Set Restart state to NAD
 */
le_result_t tafMngdPMSvc::RestartNAD()
{
    le_result_t res = taf_pm_SetAllVMPowerState(TAF_PM_STATE_RESTART);
    if(res != LE_OK)
    {
        LE_ERROR("Failed to restart the NAD");
    }
    else
    {
        LE_INFO("Restarting the NAD!");
        le_hashmap_It_Ref_t hashIter =
                (le_hashmap_It_Ref_t)le_hashmap_GetIterator(vmStateHashmap);
        while (LE_OK == le_hashmap_NextNode(hashIter))
        {
            taf_mngdPm_vmState_t *vmStatePtr =
                    (taf_mngdPm_vmState_t*)le_hashmap_GetValue(hashIter);

            if(vmStatePtr) {
                vmStatePtr->state = TAF_MNGDPM_STATE_RESTART;
            }
        }
    }
    return res;
}

/**
 * Set suspend state to NAD
 */
le_result_t tafMngdPMSvc::SuspendNAD()
{
    LE_INFO("SuspendNAD");
    le_result_t res = taf_pm_SetAllVMPowerState(TAF_PM_STATE_SUSPEND);
    if(res != LE_OK)
    {
        LE_ERROR("Failed to suspend the NAD");
    }
    else
    {
        LE_INFO("Suspending the NAD!");
        le_hashmap_It_Ref_t hashIter =
                (le_hashmap_It_Ref_t)le_hashmap_GetIterator(vmStateHashmap);
        while (LE_OK == le_hashmap_NextNode(hashIter))
        {
            taf_mngdPm_vmState_t *vmStatePtr =
                    (taf_mngdPm_vmState_t*)le_hashmap_GetValue(hashIter);

            if(vmStatePtr) {
                vmStatePtr->state = TAF_MNGDPM_STATE_SUSPEND;
            }
        }
    }
    return res;
}

/**
 * NodeStateChange request callback function for VHAL module
 */
void tafMngdPMSvc::NodeStateChangeReqRespCB
(
    uint8_t pmNodeId,
    hal_pm_NodeState_t state,
    hal_pm_PowerMode_t mode
)
{
    LE_INFO("***** %s *****", __FUNCTION__);
    LE_INFO("pmNodeId: %d", pmNodeId);
    LE_INFO("hal_pm_NodeState_t: %d", state);
    LE_INFO("hal_pm_PowerMode_t: %d", mode);

    taf_pm_SendStateChangeAck(powerStateRef, TAF_PM_STATE_ALL_ACKED, TAF_PM_PVM,
            TAF_PM_READY);
}

/**
 * Shutdown response callback function for VHAL module
 */
void tafMngdPMSvc::ShutdownPrepareRespCB
(
    uint8_t pmNodeId,
    hal_pm_NodeState_t state,
    hal_pm_PowerMode_t mode,
    const uint8_t shutdownReason,
    hal_pm_RspReason_t reason
)
{
    LE_INFO("***** %s *****", __FUNCTION__);
    LE_INFO("pmNodeId: %d", pmNodeId);
    LE_INFO("hal_pm_NodeState_t: %d", state);
    LE_INFO("hal_pm_PowerMode_t: %d", mode);
    LE_INFO("hal_pm_RspReason_t: %d", reason);
    auto &mpms = tafMngdPMSvc::GetInstance();
    if(le_timer_IsRunning(mpms.vhalAckTimerRef))
    {
        LE_DEBUG("Stop the timer");
        le_timer_Stop(mpms.vhalAckTimerRef);
    }
    if (mode == HAL_PM_SHUTDOWN_MODE_NORMAL && reason == HAL_PM_RSP_READY)
    {
        if(RequestStateChange(TAF_MNGDPM_STATE_SHUTTING_DOWN) != LE_OK)
        {
            if(shutdownCB.shutdownCallbackFunc)
            {
                shutdownCB.shutdownCallbackFunc(
                    TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
                    TAF_MNGDPM_NOT_READY,
                    LE_OK,
                    shutdownCB.shutdownCBCtxPtr);
            }
            shutdownCB.shutdownCallbackFunc = nullptr;
            return;
        }

        if(shutdownCB.shutdownCallbackFunc)
        {
            shutdownCB.shutdownCallbackFunc(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL, TAF_MNGDPM_READY,
                    LE_OK, shutdownCB.shutdownCBCtxPtr);
        }
        le_result_t res = ShutdownNAD();
        if(res == LE_OK)
        {
            powerMode.isGraceful = false;
        }
    }
    else if (mode == HAL_PM_SHUTDOWN_MODE_NORMAL && reason == HAL_PM_RSP_NOT_READY)
    {
        tafMngdPMSvc::ProcessStateChange(stateMachine.prevState);
        powerMode.isForceful = false;
        if(shutdownCB.shutdownCallbackFunc)
        {
            shutdownCB.shutdownCallbackFunc(
                TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
                TAF_MNGDPM_NOT_READY,
                LE_OK,
                shutdownCB.shutdownCBCtxPtr);
        }
    }
    else if (mode == HAL_PM_SHUTDOWN_MODE_NORMAL && reason == HAL_PM_RSP_INVALID_REQUEST)
    {
        tafMngdPMSvc::ProcessStateChange(stateMachine.prevState);
        powerMode.isForceful = false;
        if(shutdownCB.shutdownCallbackFunc)
        {
            shutdownCB.shutdownCallbackFunc(
                TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
                TAF_MNGDPM_INVALID_REQUEST,
                LE_OK,
                shutdownCB.shutdownCBCtxPtr);
        }
    }
    shutdownCB.shutdownCallbackFunc = nullptr;
}

/**
 * Restart response callback function for VHAL module
 */
void tafMngdPMSvc::RestartPrepareRespCB
(
    uint8_t pmNodeId,
    hal_pm_NodeState_t state,
    hal_pm_PowerMode_t mode,
    const uint8_t restartReason,
    hal_pm_RspReason_t reason
)
{
    LE_INFO("***** %s *****", __FUNCTION__);
    LE_INFO("pmNodeId: %d", pmNodeId);
    LE_INFO("hal_pm_NodeState_t: %d", state);
    LE_INFO("hal_pm_PowerMode_t: %d", mode);
    LE_INFO("hal_pm_RspReason_t: %d", reason);
    auto &mpms = tafMngdPMSvc::GetInstance();
    if(le_timer_IsRunning(mpms.vhalAckTimerRef))
    {
        LE_INFO("vhalAckTimerRef");
        LE_DEBUG("Stop the timer");
        le_timer_Stop(mpms.vhalAckTimerRef);
    }
    if (mode == HAL_PM_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF && reason == HAL_PM_RSP_READY)
    {
        if(RequestStateChange(TAF_MNGDPM_STATE_SHUTTING_DOWN) != LE_OK)
        {
            if(restartCB.restartCallbackFunc)
            {
                restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON, TAF_MNGDPM_NOT_READY,
                        LE_OK, restartCB.restartCBCtxPtr);
            }
            restartCB.restartCallbackFunc = nullptr;
            return;
        }

        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON, TAF_MNGDPM_READY,
                    LE_OK, restartCB.restartCBCtxPtr);
        }
        le_result_t res = ShutdownNAD();
        if(res == LE_OK)
        {
            powerMode.isGraceful = false;
        }
    }
    else if (mode == HAL_PM_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF && reason == HAL_PM_RSP_NOT_READY)
    {
        tafMngdPMSvc::ProcessStateChange(stateMachine.prevState);
        powerMode.isShutDown = false;
        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON, TAF_MNGDPM_NOT_READY,
                    LE_OK, restartCB.restartCBCtxPtr);
        }
    }
    else if (mode == HAL_PM_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF && reason == HAL_PM_RSP_INVALID_REQUEST)
    {
        tafMngdPMSvc::ProcessStateChange(stateMachine.prevState);
        powerMode.isShutDown = false;
        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON, TAF_MNGDPM_INVALID_REQUEST,
                    LE_OK, restartCB.restartCBCtxPtr);
        }
    }
    if (mode == HAL_PM_RESTART_MODE_NAD_REBOOT && reason == HAL_PM_RSP_READY)
    {
        if(RequestStateChange(TAF_MNGDPM_STATE_RESTARTING) != LE_OK)
        {
            if(restartCB.restartCallbackFunc)
            {
                restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT, TAF_MNGDPM_NOT_READY,
                        LE_OK, restartCB.restartCBCtxPtr);
            }
            restartCB.restartCallbackFunc = nullptr;
            return;
        }

        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT, TAF_MNGDPM_READY,
                    LE_OK, restartCB.restartCBCtxPtr);
        }
        le_result_t res = RestartNAD();
        if(res == LE_OK)
        {
            LE_INFO("RestartNAD is success");
        }
    }
    else if (mode == HAL_PM_RESTART_MODE_NAD_REBOOT && reason == HAL_PM_RSP_NOT_READY)
    {
        tafMngdPMSvc::ProcessStateChange(stateMachine.prevState);
        powerMode.isRestart = false;
        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT, TAF_MNGDPM_NOT_READY,
                    LE_OK, restartCB.restartCBCtxPtr);
        }
    }
    else if (mode == HAL_PM_RESTART_MODE_NAD_REBOOT && reason == HAL_PM_RSP_INVALID_REQUEST)
    {
        tafMngdPMSvc::ProcessStateChange(stateMachine.prevState);
        powerMode.isRestart = false;
        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT, TAF_MNGDPM_INVALID_REQUEST,
                    LE_OK, restartCB.restartCBCtxPtr);
        }
    }
    restartCB.restartCallbackFunc = nullptr;
}

/**
 * WakeupVehicle response callback function for VHAL module
 */
void tafMngdPMSvc::WakeupVehicleCB
(
    int32_t reason,
    int32_t response
)
{
    LE_INFO("***** %s *****", __FUNCTION__);
    LE_INFO("hal_pm_WakeupVehicleReason: %d", reason);
    LE_INFO("hal_pm_RspReason_t: %d", response);
    auto &mpms = tafMngdPMSvc::GetInstance();
    if(le_timer_IsRunning(mpms.wakeupVehicleTimerRef))
    {
        LE_INFO("wakeupVehicleTimerRef");
        LE_DEBUG("Stop the timer");
        le_timer_Stop(mpms.wakeupVehicleTimerRef);
    }
    if (reason == VEHICHLE_WAKEUP_REASON_DEFAULT && response == HAL_PM_VEHICHLE_WAKEUP_STATUS_AWAKE)
    {
        LE_INFO("Response for the wakeupVehicleCall is HAL_PM_VEHICHLE_WAKEUP_STATUS_AWAKE");
        if(wakeupVehicleCB.wakeupVehicleCallbackFunc)
        {
            wakeupVehicleCB.wakeupVehicleCallbackFunc(VEHICHLE_WAKEUP_REASON_DEFAULT, VEHICHLE_WAKEUP_STATUS_AWAKE,
                    LE_OK, wakeupVehicleCB.wakeupVehicleCBCtxPtr);
        }
    }
    else if (reason == VEHICHLE_WAKEUP_REASON_DEFAULT && response == HAL_PM_VEHICHLE_WAKEUP_STATUS_INVALID_REQ)
    {
        LE_INFO("Response for the wakeupVehicleCall is HAL_PM_VEHICHLE_WAKEUP_STATUS_INVALID_REQ");
        if(wakeupVehicleCB.wakeupVehicleCallbackFunc)
        {
            wakeupVehicleCB.wakeupVehicleCallbackFunc(VEHICHLE_WAKEUP_REASON_DEFAULT, VEHICHLE_WAKEUP_STATUS_INVALID_REQ,
                    LE_OK, wakeupVehicleCB.wakeupVehicleCBCtxPtr);
        }
    }
    else if (reason == VEHICHLE_WAKEUP_REASON_DEFAULT && response == HAL_PM_VEHICHLE_WAKEUP_STATUS_UNKNOWN)
    {
        LE_INFO("Response for the wakeupVehicleCall is HAL_PM_VEHICHLE_WAKEUP_STATUS_UNKNOWN");
        if(wakeupVehicleCB.wakeupVehicleCallbackFunc)
        {
            wakeupVehicleCB.wakeupVehicleCallbackFunc(VEHICHLE_WAKEUP_REASON_DEFAULT, VEHICHLE_WAKEUP_STATUS_UNKNOWN,
                    LE_OK, wakeupVehicleCB.wakeupVehicleCBCtxPtr);
        }
    }
    wakeupVehicleCB.wakeupVehicleCallbackFunc = nullptr;
}

/**
 * Node event callback function for VHAL module
 */
void tafMngdPMSvc::NodeEventCB
(
    uint8_t pm_node_id,
    const char* pm_node_event_info
)
{
    LE_INFO("NodeEventCB for node %d with node info %s", pm_node_id, pm_node_event_info);
    le_result_t res = LE_FAULT;
    if(strncmp(pm_node_event_info, RELAX, strlen(RELAX)) == 0)
    {
        if(tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE) != LE_OK)
        {
            return;
        }

        res = tafMngdPMSvc::ReleaseWakeLock();
        if(res == LE_OK)
        {
            LE_INFO(" ReleaseWakeLock successfull");
            tafMngdPMSvc::ProcessStateChange(TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE);
        }
    }
    else if(strncmp(pm_node_event_info, STAYAWAKE, strlen(STAYAWAKE)) == 0)
    {
        if(tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_WAKING_UP) != LE_OK)
        {
            return;
        }

        res = tafMngdPMSvc::AcquireWakeLock();
        if(res == LE_OK)
        {
            LE_INFO(" AcquireWakeLock successfull");
            tafMngdPMSvc::ProcessStateChange(TAF_MNGDPM_STATE_WAKING_UP);
        }
    }
    else if(strncmp(pm_node_event_info, SHUTDOWN, strlen(SHUTDOWN)) == 0)
    {
        LE_INFO("SHUTDOWN from VHAL");

        if(tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_SHUTTING_DOWN) != LE_OK)
        {
            return;
        }
    }
    else
    {
        LE_ERROR("Received unsupported mode from VHAL");
    }
}

/**
 * Client connection callback function
 */
void tafMngdPMSvc::OnClientConnection(le_msg_SessionRef_t sessionRef, void *ctxPtr)
{
    LE_DEBUG("OnClientConnection");

    taf_mngdPm_SessionNode_t* sessionNodePtr = nullptr;

    sessionNodePtr =
        (taf_mngdPm_SessionNode_t*)le_mem_TryAlloc(mngdPmClientInfo.SessionNodePool);

    TAF_ERROR_IF_RET_NIL(sessionNodePtr == nullptr, "Cannot allocate sessionNode");

    sessionNodePtr->sessionRef = sessionRef;

    if (sessionRef && (LE_OK == le_msg_GetClientProcessId(sessionRef, &sessionNodePtr->procId)) &&
    (LE_OK == le_appInfo_GetName(sessionNodePtr->procId, sessionNodePtr->name, sizeof(sessionNodePtr->name)-1)))
    {
        LE_INFO("Client %s/%d connected", sessionNodePtr->name, sessionNodePtr->procId);
    }
    // update client record in table
    if (le_hashmap_Put(mngdPmClientInfo.clients, sessionRef, sessionNodePtr))
    {
        LE_ERROR("Failed to add client record for session %p.", sessionRef);
    }
}

/**
 * Client disconnection callback function
 */
void tafMngdPMSvc::OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *ctxPtr)
{
    LE_DEBUG("OnClientDisconnection");
    taf_mngdPm_SessionNode_t* sessionNodePtr =
            (taf_mngdPm_SessionNode_t*)le_hashmap_Remove(mngdPmClientInfo.clients, sessionRef);

    TAF_ERROR_IF_RET_NIL(sessionNodePtr == nullptr, "Failed to remove sessionRef %p from table",
                         sessionRef);
    if(restartCB.sessionRef == sessionRef)
         restartCB.restartCallbackFunc = nullptr;
    if(shutdownCB.sessionRef == sessionRef)
        shutdownCB.shutdownCallbackFunc = nullptr;
    LE_INFO("Client with sessionRef %p (process %d) disconnected", sessionRef, sessionNodePtr->procId);
    le_mem_Release(sessionNodePtr);

    auto &mpms = tafMngdPMSvc::GetInstance();
    //clear state change registered clients
    for(auto it = mpms.regClientrecrd.begin(); it != mpms.regClientrecrd.end(); )
    {
        if(it->sessionRef == sessionRef)
        {
            LE_INFO("Client with sessionRef %p", it->sessionRef);
            it = mpms.regClientrecrd.erase(it);
        }
        else
        {
            ++it;
        }
    }

    //Clear system WsReflist
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.wsRefList));

    while (linkHandlerPtr)
    {
        taf_wsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_wsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.wsRefList), linkHandlerPtr);
        if (wsRefCtxPtr && wsRefCtxPtr->sessionRef == sessionRef)
        {
            LE_INFO("Client with sessionRef %p", wsRefCtxPtr->sessionRef);
            if(wsRefCtxPtr->wakeSourceState == WAKE_SOURCE_ACQUIRED) {
                le_result_t res = ReleaseWakeSource(wsRefCtxPtr);
                if(res == LE_OK)
                {
                    LE_INFO("Released lock for the client with sessionRef %p", wsRefCtxPtr->sessionRef);
                }
            }
            else if(wsRefCtxPtr->wakeSourceState == WAKE_SOURCE_IGNORED) {
                wsRefCtxPtr->wakeSourceState = WAKE_SOURCE_NOT_ACQUIRED;
            }
            le_ref_DeleteRef(mpms.wsRefMap, wsRefCtxPtr->wsRef);
            le_dls_Remove(&(mpms.wsRefList), &wsRefCtxPtr->link);
            free((void*)wsRefCtxPtr->wsTag);
            le_mem_Release((void*)wsRefCtxPtr);
        }
    }
    //Clear node nodeWsRefList
    le_dls_Link_t* linkNodeHandlerPtr = le_dls_PeekTail(&(mpms.nodeWsRefList));

    while (linkNodeHandlerPtr)
    {
        taf_nodeWsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkNodeHandlerPtr, taf_nodeWsRefCtx_t, link);
        linkNodeHandlerPtr = le_dls_PeekPrev(&(mpms.nodeWsRefList), linkNodeHandlerPtr);
        if (wsRefCtxPtr && wsRefCtxPtr->sessionRef == sessionRef)
        {
            LE_INFO("Client with sessionRef %p", wsRefCtxPtr->sessionRef);
            if(wsRefCtxPtr->isAcquiredLock) {
                le_result_t res = tafMngdPMSvc::ReleaseWakeLock();
                if(res == LE_OK)
                {
                    LE_INFO("Released lock");
                    wsRefCtxPtr->isAcquiredLock = false;
                }
            }
            le_ref_DeleteRef(mpms.nodeWsRefMap, wsRefCtxPtr->wsRef);
            le_dls_Remove(&(mpms.nodeWsRefList), &wsRefCtxPtr->link);
            free((void*)wsRefCtxPtr->vhalTag);
            le_mem_Release((void*)wsRefCtxPtr);
        }
    }
    //Clear infoReportHandlerList
    le_dls_Link_t* infoReportLinkHandlerPtr = le_dls_PeekTail(&infoReportHandlerList);
    while (infoReportLinkHandlerPtr)
    {
        taf_mngdPm_InfoReportCb_t * handlerCtxPtr =
                CONTAINER_OF(infoReportLinkHandlerPtr, taf_mngdPm_InfoReportCb_t, link);
        infoReportLinkHandlerPtr = le_dls_PeekPrev(&infoReportHandlerList, infoReportLinkHandlerPtr);
        if (handlerCtxPtr && handlerCtxPtr->sessionRef == sessionRef)
        {
            LE_INFO("Clearing Bub state change handler reg client");
            le_ref_DeleteRef(mpms.infoReportHandlerRefMap, handlerCtxPtr->handlerRef);
            le_dls_Remove(&(mpms.infoReportHandlerList), &handlerCtxPtr->link);
            le_mem_Release((void*)handlerCtxPtr);
        }
    }
    // Clear nodePowerStateList
    le_dls_Link_t* nodePowerStateListHandlerPtr = le_dls_PeekTail(&nodePowerStateHandlerList);
    while (nodePowerStateListHandlerPtr)
    {
        taf_mngdPm_NodePowerStateCtxt_t * handlerCtxPtr =
                CONTAINER_OF(nodePowerStateListHandlerPtr, taf_mngdPm_NodePowerStateCtxt_t, link);
        nodePowerStateListHandlerPtr = le_dls_PeekPrev(&nodePowerStateHandlerList, nodePowerStateListHandlerPtr);
        // Release the per-handler immediate-notify node state ref, if any
        LE_INFO("Clearing node power state handler for client sessionRef %p",
            handlerCtxPtr->sessionRef);
        le_ref_DeleteRef(mpms.nodePowerStateHandlerMap, handlerCtxPtr->handlerRef);
        le_dls_Remove(&(mpms.nodePowerStateHandlerList), &handlerCtxPtr->link);
        le_mem_Release((void*)handlerCtxPtr);
    }
    //Clear wakeupVehicle client's data
    if(mpms.wakeupVehicleCB.sessionRef == sessionRef)
    {
        mpms.wakeupVehicleCB.wakeupVehicleCallbackFunc = nullptr;
        mpms.wakeupVehicleCB.wakeupVehicleCBCtxPtr = nullptr;
        mpms.wakeupVehicleCB.sessionRef = nullptr;
    }
}

/**
 * Check whether the client session is valid or not
 */
bool tafMngdPMSvc::IsClientValid()
{
    le_msg_SessionRef_t sessionRef = taf_mngdPm_GetClientSessionRef();

    taf_mngdPm_SessionNode_t* sessionNodePtr =
            (taf_mngdPm_SessionNode_t*)le_hashmap_Get(mngdPmClientInfo.clients, sessionRef);

    TAF_ERROR_IF_RET_VAL(sessionNodePtr == nullptr,
                            false,
                            "cannot find session %p from table", sessionRef);

    pid_t pid;

    TAF_ERROR_IF_RET_VAL(LE_OK != le_msg_GetClientProcessId(sessionRef, &pid),
                            false,
                            "cannot get process id for session %p", sessionRef);

    TAF_ERROR_IF_RET_VAL(pid != sessionNodePtr->procId,
                            false,
                            "pid mismatched");

    return true;
}

/**
 * State change callback function for PM service
 */
void tafMngdPMSvc::StateChangeHandler(taf_pm_State_t state, void* contextPtr)
{
    LE_INFO("State change triggered for %s\n", TafStateToString((uint8_t)state));

    // Update the VMs data on receiving state change caused by any other sources like SMS, CAN
    le_hashmap_It_Ref_t hashIter = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(vmStateHashmap);
    while (LE_OK == le_hashmap_NextNode(hashIter))
    {
        taf_mngdPm_vmState_t *vmStatePtr = (taf_mngdPm_vmState_t*)le_hashmap_GetValue(hashIter);
        if(vmStatePtr) {
            vmStatePtr->state = (uint8_t)state;
        }
    }
}

// Process the cached awake requests
void tafMngdPMSvc::ProcessCachedAwakeReqs()
{
    LE_INFO("Check & process cached requests!");
    auto &mpms = tafMngdPMSvc::GetInstance();

    if(wsCachedReqsRefSet.empty()){
        LE_INFO("No cached awake requests found!");
        return;
    }

    for (auto it = wsCachedReqsRefSet.begin(); it != wsCachedReqsRefSet.end();)
    {
        bool found = false;
        le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.wsRefList));
        while (linkHandlerPtr) {
            taf_wsRefCtx_t* wsRefCtxPtr = CONTAINER_OF(linkHandlerPtr, taf_wsRefCtx_t, link);
            linkHandlerPtr = le_dls_PeekPrev(&(mpms.wsRefList), linkHandlerPtr);

            if (wsRefCtxPtr && wsRefCtxPtr->wsRef == *it) {
                found = true;
                if(mpms.IsAuthorizedStayAwakeReason(wsRefCtxPtr->reason)) {
                    LE_INFO("Cached ws for %s with wsReason:%d is authorized", wsRefCtxPtr->wsTag, wsRefCtxPtr->reason);
                    le_result_t res = mpms.AcquireWakeSource(wsRefCtxPtr);
                    if(res != LE_OK) {
                        LE_ERROR("Failed to acquire ws of %s with wsReason:%d", wsRefCtxPtr->wsTag, wsRefCtxPtr->reason);
                    }
                } else {
                    LE_INFO("StayAwakeReason:%d for cachedAwakeReqs is unauthorized!, wsState: %d", wsRefCtxPtr->reason, wsRefCtxPtr->wakeSourceState);
                    wsRefCtxPtr->wakeSourceState = WAKE_SOURCE_IGNORED;
                }
                it = wsCachedReqsRefSet.erase(it);
                break;
            }
        }

        if (!found) {
            LE_WARN("Cached wsRef %p not found in the wsList", *it);
            it = wsCachedReqsRefSet.erase(it);
        }
    }
}

/**
 * Ex State change callback function for PM service
 */
void tafMngdPMSvc::StateChangeExHandler(taf_pm_PowerStateRef_t psRef,
        taf_pm_NadVm_t vm_id, taf_pm_State_t state, void* contextPtr)
{
    LE_INFO("State change triggered in StateChangeExHandler");
    powerStateRef = psRef;
    taf_mngdPm_NodePowerStateChange_t powerStateChange;
    if(state == TAF_PM_STATE_ALL_ACKED && !pmInf)
    {
        LE_DEBUG("Send ACK if there is no driver loaded");
        taf_pm_SendStateChangeAck(powerStateRef, state, vm_id, TAF_PM_READY);
        return;
    }
    if(state == TAF_PM_STATE_ALL_WAKELOCKS_RELEASED)
    {
        LE_DEBUG("Received all wakelocks released notification");
        le_result_t res;
        if(targetedPowerMode == TAF_MNGDPM_SHUTDOWN)
        {
            res = ShutdownNAD();
            if(res == LE_OK) {
                powerMode.isGraceful = true;
                ProcessStateChange(TAF_MNGDPM_STATE_SHUTTING_DOWN);
            }
        }
        else if(wsCount == 0)
        {
            res = SuspendNAD();
            if(res == LE_OK) {
                powerMode.isSuspend = true;
                ProcessStateChange(TAF_MNGDPM_STATE_SUSPENDING);
                LE_INFO("Suspend triggered for NAD on wakelocks released");
            }
        }
    }
    else if ((state == TAF_PM_STATE_ALL_ACKED) && (pmInf) && (pmInf->nodeStateChangeReqAsync))
    {
        if(powerMode.isGraceful)
        {
            powerMode.isGraceful = false;
            // When in graceful power mode, handle state transitions differently based on current state
            if(stateMachine.currentState == TAF_MNGDPM_STATE_SUSPEND){
                // For suspend state, send a suspend request to VHAL to maintain state consistency
                LE_DEBUG("Send nodeStateChangeReqAsync %d", HAL_PM_SUSPEND_MODE_FULL);
                le_result_t result = (*(pmInf->nodeStateChangeReqAsync))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SUSPEND, HAL_PM_SUSPEND_MODE_FULL, tafMngdPMSvc::NodeStateChangeReqRespCB);
                if (result != LE_OK){
                    LE_ERROR("Failed to send nodeStateChangeReqAsync for suspend to VHAL: %s", LE_RESULT_TXT(result));
                }
            }else if(stateMachine.currentState == TAF_MNGDPM_STATE_SHUTDOWN){
                // For shhutdown state, proceed with normal shutdown request
                LE_DEBUG("Send nodeStateChangeReqAsync %d", HAL_PM_SHUTDOWN_MODE_GRACEFUL);
                le_result_t result = (*(pmInf->nodeStateChangeReqAsync))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SHUTDOWN, HAL_PM_SHUTDOWN_MODE_GRACEFUL, tafMngdPMSvc::NodeStateChangeReqRespCB);
                if (result != LE_OK) {
                    LE_ERROR("Failed to send nodeStateChangeReqAsync for shutdown to VHAL: %s", LE_RESULT_TXT(result));
                }
            }else {
                //No further action to be taken for the states apart from graceful suspend & shutdown
                LE_ERROR("Invalid state: %s for graceful power mode", TafStateToString(stateMachine.currentState));
            }
        }
        else if(powerMode.isShutDown)
        {
            powerMode.isShutDown = false;
            if(stateMachine.currentState == TAF_MNGDPM_STATE_SHUTDOWN){
                LE_DEBUG("Send shutdownReqAsync %d", HAL_PM_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF);
                (*(pmInf->nodeStateChangeReqAsync))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_RESTART, HAL_PM_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF, tafMngdPMSvc::NodeStateChangeReqRespCB);
            } else{
                LE_DEBUG("Invalid state: %s for system restart, skipping vhal notification",
                    TafStateToString(stateMachine.currentState));
            }
        }
        else if(powerMode.isRestart)
        {
            powerMode.isRestart = false;
            if(stateMachine.currentState == TAF_MNGDPM_STATE_RESTART){
                LE_DEBUG("Send RestartReqAsync %d", HAL_PM_RESTART_MODE_NAD_REBOOT);
                (*(pmInf->nodeStateChangeReqAsync))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_RESTART, HAL_PM_RESTART_MODE_NAD_REBOOT, tafMngdPMSvc::NodeStateChangeReqRespCB);
            } else{
                LE_DEBUG("Invalid state: %s for system reboot, skipping vhal notification",
                    TafStateToString(stateMachine.currentState));
            }
        }
        else if(powerMode.isSuspend)
        {
            powerMode.isSuspend = false;
            if(stateMachine.currentState == TAF_MNGDPM_STATE_SUSPEND){
                LE_DEBUG("Send SuspendReqAsync %d", HAL_PM_SUSPEND_MODE_FULL);
                (*(pmInf->nodeStateChangeReqAsync))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SUSPEND, HAL_PM_SUSPEND_MODE_FULL, tafMngdPMSvc::NodeStateChangeReqRespCB);
            } else{
                LE_DEBUG("Invalid state: %s for system suspend, skipping vhal notification",
                    TafStateToString(stateMachine.currentState));
            }
        }
        else if(powerMode.isForceful)
        {
            powerMode.isForceful = false;
            if(stateMachine.currentState == TAF_MNGDPM_STATE_SHUTDOWN){
                LE_DEBUG("nodeStateChangeReqAsync triggered to VHAL on forceful shutdown");
                (*(pmInf->nodeStateChangeReqAsync))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SHUTDOWN, HAL_PM_SHUTDOWN_MODE_NORMAL, tafMngdPMSvc::NodeStateChangeReqRespCB);
            } else{
            LE_DEBUG("Invalid state: %s for forceful shutdown, skipping vhal notification",
                TafStateToString(stateMachine.currentState));
            }
        }
        // Process the cached awake requests
        ProcessCachedAwakeReqs();
    }
    else if(state == TAF_PM_STATE_SUSPEND)
    {
        ProcessStateChange(TAF_MNGDPM_STATE_SUSPEND);
        powerStateChange.state = TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE;
        le_event_Report(nodePowerStateChange, &powerStateChange, sizeof(taf_mngdPm_NodePowerStateChange_t));

        if(pmInf && pmInf->nodeStateChangeNotification)
        {
            LE_DEBUG("Send state change notification %d", HAL_PM_NODE_STATE_SUSPEND);
            (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SUSPEND,
                    NULL);
        }
    }
    else if(state == TAF_PM_STATE_SHUTDOWN)
    {
        ProcessStateChange(TAF_MNGDPM_STATE_SHUTDOWN);
        if(powerMode.isShutDown ||  powerMode.isForceful || powerMode.isGraceful)
        {
            powerStateChange.state = TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE;
            le_event_Report(nodePowerStateChange, &powerStateChange, sizeof(taf_mngdPm_NodePowerStateChange_t));
            if(pmInf && pmInf->nodeStateChangeNotification)
            {
                LE_DEBUG("Send state change notification %d", HAL_PM_NODE_STATE_SHUTDOWN);
                (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SHUTDOWN,
                        NULL);
            }
         }
    }
    else if(state == TAF_PM_STATE_RESTART)
    {
        ProcessStateChange(TAF_MNGDPM_STATE_RESTART);
        if(powerMode.isRestart)
        {
            powerStateChange.state = TAF_MNGDPM_NODE_STATE_RESTART_PREPARE;
            le_event_Report(nodePowerStateChange, &powerStateChange, sizeof(taf_mngdPm_NodePowerStateChange_t));
            if(pmInf && pmInf->nodeStateChangeNotification)
            {
                LE_DEBUG("Send state change notification %d", HAL_PM_NODE_STATE_RESTART);
                (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_RESTART,
                        NULL);
            }
        }
    }
    else if(state == TAF_PM_STATE_RESUME)
    {
        ProcessStateChange(TAF_MNGDPM_STATE_RESUME);
        powerStateChange.state = TAF_MNGDPM_NODE_STATE_RESUME;
        le_event_Report(nodePowerStateChange, &powerStateChange, sizeof(taf_mngdPm_NodePowerStateChange_t));

        if(pmInf && pmInf->nodeStateChangeNotification)
        {
            LE_DEBUG("Send state change notification %d", HAL_PM_NODE_STATE_RESUME);
            (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_RESUME,
                    NULL);
        }
    }
}

/**
 * VHAL ack timer handler
 */
void tafMngdPMSvc::VhalAckTimerHandler(le_timer_Ref_t timerRef)
{
    LE_INFO("VhalAckTimerHandler");
    auto &mpms = tafMngdPMSvc::GetInstance();
    taf_mngdPm_RequestedState_t* state =
      (taf_mngdPm_RequestedState_t*)le_timer_GetContextPtr(timerRef);
    LE_INFO("VhalAckTimer Expired after %ld msec for state %d", mpms.config.hal_state_prepare_timeout,
            *(state));
    tafMngdPMSvc::ProcessStateChange(stateMachine.prevState);
    if(*(state) == SYSTEM_NORMAL_SHUTDOWN)
    {
        LE_INFO("VhalAckTimer expire for SYSTEM_FORCEFUL_SHUTDOWN");
        mpms.powerMode.isForceful = false;
        if(shutdownCB.shutdownCallbackFunc)
        {
            shutdownCB.shutdownCallbackFunc(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL, TAF_MNGDPM_TIMEOUT,
                    LE_OK, shutdownCB.shutdownCBCtxPtr);
        }
        shutdownCB.shutdownCallbackFunc = nullptr;
    }
    else if (*(state) == RESTART_WITH_NAD_POWER_OFF_ON)
    {
        LE_INFO("VhalAckTimer expire for TAF_MNGDPM_RESTART_SYSTEM_OFF_ON");
        mpms.powerMode.isShutDown = false;
        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON, TAF_MNGDPM_TIMEOUT,
                    LE_OK, restartCB.restartCBCtxPtr);
        }
        restartCB.restartCallbackFunc = nullptr;
    }
    else if (*(state) == RESTART_WITH_NAD_REBOOT)
    {
        LE_INFO("VhalAckTimer expire for TAF_MNGDPM_RESTART_MODE_NAD_REBOOT");
        mpms.powerMode.isRestart = false;
        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT, TAF_MNGDPM_TIMEOUT,
                    LE_OK, restartCB.restartCBCtxPtr);
        }
        restartCB.restartCallbackFunc = nullptr;
    }
}
/**
 * VHAL ack timer handler
 */
void tafMngdPMSvc::VehichleWakeupTimerHandler(le_timer_Ref_t timerRef)
{
    LE_INFO("VehichleWakeupTimerHandler");
    auto &mpms = tafMngdPMSvc::GetInstance();
    taf_mngdPm_RequestedWakeupVehicle_t* wakeupMode =
      (taf_mngdPm_RequestedWakeupVehicle_t*)le_timer_GetContextPtr(timerRef);

    LE_INFO("VehichleWakeupTimer Expired after %ld msec for wakeupMode %d",
            mpms.config.hal_wakeup_vehicle_timeout, *(wakeupMode));

    taf_mngdPm_RequestedWakeupVehicle_t* mode =
            (taf_mngdPm_RequestedWakeupVehicle_t*)le_timer_GetContextPtr(timerRef);

    if(*(mode) == WAKEUP_VEHICHLE_REQ_DEFAULT)
    {
        LE_INFO("VehichleWakeupTimer expire for WAKEUP_VEHICHLE_REQ_DEFAULT");
        if(wakeupVehicleCB.wakeupVehicleCallbackFunc)
        {
            wakeupVehicleCB.wakeupVehicleCallbackFunc(WAKEUP_VEHICHLE_REQ_DEFAULT, TAF_MNGDPM_TIMEOUT,
                    LE_OK, wakeupVehicleCB.wakeupVehicleCBCtxPtr);
        }
        wakeupVehicleCB.wakeupVehicleCallbackFunc = nullptr;
    }
}
/**
 * Timer to wait wakesource request from apps
 */
void tafMngdPMSvc::WaitWakeSourceTimer()
{
    LE_INFO("WaitWakeSourceTimer");
    auto &mpms = tafMngdPMSvc::GetInstance();
    le_result_t res;
    //timer to wait for wake source from apps
    wakeSourceTimerRef = le_timer_Create("WAKE SOURCE timer");
    le_timer_SetWakeup(mpms.wakeSourceTimerRef, false);
    le_timer_SetMsInterval(wakeSourceTimerRef, mpms.config.bootup_awake_time);
    le_timer_SetHandler(wakeSourceTimerRef, WakeSourceTimerHandler);
    //acquire wakesource
    res = AcquireWakeLock();
    if(res == LE_OK) {
        LE_INFO("acquired wake lock after init");
    }
    le_timer_Start(wakeSourceTimerRef);
    LE_INFO("Started WaitWakeSourceTimer for wakesource request from apps");
}

/**
 * Timer handler for wakesource request
 */
void tafMngdPMSvc::WakeSourceTimerHandler(le_timer_Ref_t timerRef)
{
    LE_INFO("WaitWakeSourceTimer Expired for WakeSourceTimerHandler");
    auto &mpms = tafMngdPMSvc::GetInstance();
    le_result_t res = ReleaseWakeLock();
    if(res == LE_OK)
        LE_INFO("ReleaseWakeLock after %ld msec timeout", mpms.config.bootup_awake_time);
}

//--------------------------------------------------------------------------------------------------
/**
 * Load PM VHAL module. If succeeded, send EVT_LOAD_PMVHAL_READY event.
 */
 //-------------------------------------------------------------------------------------------------
le_result_t tafMngdPMSvc::InitVHalModule()
{
    auto &mpms = tafMngdPMSvc::GetInstance();
    // Load the driver and does not care the version
    pmInf = (hal_pm_Inf_t *)taf_devMgr_LoadDrv(TAF_PM_MODULE_NAME, nullptr);

    if(pmInf == nullptr)
    {
        LE_ERROR("Can not load the driver %s, retry later", TAF_PM_MODULE_NAME);
        return LE_FAULT;
    }

    if (pmInf->InitHAL != NULL)
    {
        LE_DEBUG("Call pmInf(%p) init function", pmInf);
        int ret = 0;
        LE_DEBUG("Before safe call init");
        ENTER_SAFE_CALL(TIMER_SAFECALL, ret, (*(pmInf->InitHAL)));
        EXIT_SAFE_CALL();

        if (ret == -1)
        {
            LE_CRIT("Failed to init PMVHAL: %s", TAF_PM_MODULE_NAME);
            return LE_FAULT;
        }
    }

    if ((mpms.pmInf) && (mpms.pmInf->addNodeEventHandler))
    {
        LE_INFO("addNodeEventHanlder for node %d", NODE_ID);
        int ret = 0;
        LE_DEBUG("Before safe call addNodeEventHandler");
        ENTER_SAFE_CALL(TIMER_SAFECALL, ret, (*(pmInf->addNodeEventHandler)),NODE_ID, NodeEventCB);
        EXIT_SAFE_CALL();

        if (ret == -1)
        {
            LE_CRIT("Failed to add addNodeEventHandler for %d", NODE_ID);
            return LE_FAULT;
        }
    }

    mpms.vhalAckTimerRef = le_timer_Create("VHAL ACK timer");
    le_timer_SetWakeup(mpms.vhalAckTimerRef, false);
    le_timer_SetMsInterval(mpms.vhalAckTimerRef, mpms.config.hal_state_prepare_timeout);
    le_timer_SetHandler(mpms.vhalAckTimerRef, VhalAckTimerHandler);
    //creating the timer for vehichle wakeup
    mpms.wakeupVehicleTimerRef = le_timer_Create("VEHICHLE WAKEUP timer");
    le_timer_SetWakeup(mpms.wakeupVehicleTimerRef, false);
    le_timer_SetMsInterval(mpms.wakeupVehicleTimerRef, mpms.config.hal_wakeup_vehicle_timeout);
    le_timer_SetHandler(mpms.wakeupVehicleTimerRef, VehichleWakeupTimerHandler);

    // PMVHAL successfully loaded
    LE_INFO("Loaded module %s successfully", TAF_PM_MODULE_NAME);

    taf_mngdPm_readyEvtType_t readyType;
    readyType.type = EVT_LOAD_PMVHAL_READY;
    mpms.isPmVhalReady = true;
    le_event_Report(mpms.pmEvtReady, &readyType, sizeof(readyType));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the event when loading pmvhal is successful.
 */
 //-------------------------------------------------------------------------------------------------
void tafMngdPMSvc::PMVhalReadyEvtHandler(void * reportPtr)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");
    taf_mngdPm_readyEvtType_t* evtType =(taf_mngdPm_readyEvtType_t*)reportPtr;

    switch(evtType->type)
    {
        case EVT_LOAD_PMVHAL_READY:
            // After PMVHAL is loaded, advertise the service to client sides
            LE_INFO("Advertise MngdPM service");
            taf_mngdPm_AdvertiseService();

            WaitWakeSourceTimer();

            // Set session open handler
            le_msg_AddServiceOpenHandler(taf_mngdPm_GetServiceRef(), tafMngdPMSvc::OnClientConnection,
                NULL);
            // Set session close handler
            le_msg_AddServiceCloseHandler(taf_mngdPm_GetServiceRef(), tafMngdPMSvc::OnClientDisconnection,
                NULL);
            // Set session close handler  for RPCPM
            le_msg_AddServiceCloseHandler(taf_mngdPm_GetServiceRef(), tafMngdRpcPm::OnClientDisconnection,
                NULL);

        break;

        default:
            LE_ERROR("Wrong event type");
        break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Load PMVHAL until retry count has reaches the maximum value.
 */
 //-------------------------------------------------------------------------------------------------
void tafMngdPMSvc::RetryHandler(le_timer_Ref_t timerRef)
{
    auto &mpms = GetInstance();

    uint32_t expiryCount = le_timer_GetExpiryCount(timerRef);
    LE_DEBUG("expiryCount = %d", expiryCount);
    if(expiryCount <= MAX_NUM_OF_RETRY)
    {
        // Load PMVHAL. Retry in case of failure.
        if(!mpms.isPmVhalReady)
        {
            le_result_t res = mpms.InitVHalModule();
            if(res == LE_OK){
                LE_INFO("PMVHAL loaded successfully");
                mpms.isPmVhalReady = true;

                // Load PMVHAL successfully. Delete the timer.
                le_timer_Delete(timerRef);
            }
        }
    }

    if(!mpms.isPmVhalReady && expiryCount == MAX_NUM_OF_RETRY)
    {
        //Load PMVHAL timeout.
        LE_CRIT("Load PMVHAL timeout, failed to load PMVHAL module after all retry attempts!");
        le_timer_Delete(timerRef);
        exit(EXIT_SUCCESS);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a timer to retry loading PMVHAL module again.
 */
 //-------------------------------------------------------------------------------------------------
void tafMngdPMSvc::GetPmVhalReady(void *p1, void *p2)
{
    LE_UNUSED(p1);
    LE_UNUSED(p2);

    // Start a timer for the retry-action
    le_timer_Ref_t retryTimer = le_timer_Create("retry-timer-pmvhal");

    if (retryTimer == NULL)
    {
        LE_ERROR("Failed to le_timer_Create for the retry-timer");
        return;
    }

    le_timer_SetRepeat(retryTimer, MAX_NUM_OF_RETRY);
    le_timer_SetHandler(retryTimer, RetryHandler);
    le_timer_SetWakeup(retryTimer, false);
    le_timer_SetMsInterval(retryTimer, RETRY_TIMER_INTERVAL);
    le_timer_Start(retryTimer);
    LE_INFO("Retry timer for loading PMVHAL start ...");
}

/**
 * Local api which Keeps the system awake by acquiring wake lock for the given reference.
 */
le_result_t tafMngdPMSvc::AcquireWakeSource(taf_wsRefCtx_t * wsRefCtxPtr)
{
    LE_INFO("AcquireWakeSource");
    auto &mpms = tafMngdPMSvc::GetInstance();
    le_result_t res = LE_FAULT;
    res = tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_WAKING_UP);
    if(res != LE_OK)
    {
        return res;
    }
    res = tafMngdPMSvc::AcquireWakeLock();
    if(res == LE_OK)
    {
        LE_INFO("Acquired wake source for %s with the stayawakeReason:%d, wakeSourceState set to: WAKE_SOURCE_ACQUIRED ", wsRefCtxPtr->wsTag, wsRefCtxPtr->reason);
        wsRefCtxPtr->wakeSourceState = WAKE_SOURCE_ACQUIRED;
        tafMngdPMSvc::ProcessStateChange(TAF_MNGDPM_STATE_WAKING_UP);
        //sending notification to VHAL
        if((mpms.pmInf) && (mpms.pmInf->nodeInfoNotification))
        {
            LE_INFO("notify node info for reason:%d", wsRefCtxPtr->reason);
            (*(mpms.pmInf->nodeInfoNotification))(NODE_ID,
                HAL_PM_NODE_INFO_LOCK_ACQUIRED, (const uint8_t)wsRefCtxPtr->reason);
        }
    }
    else
    {
        LE_ERROR("Failed to acquire wake source for the stayawakeReason:%d", wsRefCtxPtr->reason);
    }
    return res;
}

/**
 * Acquire wakesource and let system stay awake
 */
le_result_t tafMngdPMSvc::AcquireWakeLock()
{
    le_result_t res = LE_FAULT;
    if(wsCount == 0) {
        if(ws == nullptr)
            ws = taf_pm_NewWakeupSource(WAKELOCK_WITHOUT_REF, "mpms");
        if (ws != nullptr && !powerMode.isWsAcquired)
        {
            res = taf_pm_StayAwake(ws);
            if(res == LE_OK) {
                powerMode.isWsAcquired = true;
                wsCount++;
                LE_INFO("Acquired WakeLock wsCount:%d", wsCount);
            }
            else
            {
                LE_ERROR("failed to acquire ws");
            }
        }
        else
        {
            LE_ERROR("Failed to create wakeup source!");
        }
    }
    else
    {
        wsCount++;
        LE_INFO("Acquired WakeLock wsCount:%d", wsCount);
        res = LE_OK;
    }
    return res;
}

/**
* Local api to clear non authorized system wake sources acquired by client after filter set.
*/
void ClearUnauthorizedWs()
{
    auto &mpms = tafMngdPMSvc::GetInstance();
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.wsRefList));
    while (linkHandlerPtr)
    {
        taf_wsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_wsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.wsRefList), linkHandlerPtr);
        if (wsRefCtxPtr && wsRefCtxPtr->sessionRef && wsRefCtxPtr->wakeSourceState)
        {
            LE_INFO("Clear non authorized wake source with sessionRef %p", wsRefCtxPtr->sessionRef);
            wsRefCtxPtr->wakeSourceState = WAKE_SOURCE_NOT_ACQUIRED;
        }
    }
}

/**
* Local api to clear non authorized syatem wake sources after AuthorizeStayAwakeReason api called.
*/
void tafMngdPMSvc::RefreshWakeSources()
{
    LE_INFO("RefreshWakeSources");
    auto &mpms = tafMngdPMSvc::GetInstance();
    le_result_t res = LE_FAULT;
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.wsRefList));
    while (linkHandlerPtr)
    {
        taf_wsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_wsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.wsRefList), linkHandlerPtr);

        if (wsRefCtxPtr)
        {
            if(wsRefCtxPtr->wakeSourceState)
            {
                if(mpms.IsAuthorizedStayAwakeReason(wsRefCtxPtr->reason))
                {
                    if(wsRefCtxPtr->wakeSourceState == WAKE_SOURCE_IGNORED)
                    {
                        LE_INFO("wakeSource of %s with the authorized stayawakeReason:%d changed from WAKE_SOURCE_IGNORED to WAKE_SOURCE_ACQUIRED", wsRefCtxPtr->wsTag, wsRefCtxPtr->reason);
                        AcquireWakeSource(wsRefCtxPtr);
                    }
                    else
                    {
                        continue;
                    }
                }
                else if(wsRefCtxPtr->wakeSourceState == WAKE_SOURCE_ACQUIRED)
                {
                    LE_INFO("wakeSource of %s with the unauthorized stayawakeReason:%d changed from WAKE_SOURCE_ACQUIRED to WAKE_SOURCE_IGNORED", wsRefCtxPtr->wsTag, wsRefCtxPtr->reason);
                    ReleaseWakeSource(wsRefCtxPtr);
                    wsRefCtxPtr->wakeSourceState = WAKE_SOURCE_IGNORED;
                    LE_INFO("RefreshWakeSource: unauthorized Wake Source State: %d, current system state: %d",wsRefCtxPtr->wakeSourceState, mpms.stateMachine.currentState);
                }
            }
            else
            {
                LE_INFO("wakeSource state: %d for the %s with the stayawakeReason:%d not acquired", wsRefCtxPtr->wakeSourceState, wsRefCtxPtr->wsTag, wsRefCtxPtr->reason);
                continue;
            }
        }
    }
    if(wsCount == 0)
    {
        res = mpms.ReleaseWakeLock();
        if(res == LE_OK)
        {
            LE_INFO("ReleaseWakeLock is success when no active wakeSource");
        }
    }
}

/**
 * Local api which Releases the acquired wake lock for the given reference.
 */
le_result_t tafMngdPMSvc::ReleaseWakeSource(taf_wsRefCtx_t * wsRefCtxPtr)
{
    LE_INFO("ReleaseWakeSource");
    auto &mpms = tafMngdPMSvc::GetInstance();
    le_result_t res = LE_FAULT;
    res = tafMngdPMSvc::RequestStateChange(
            TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE);
    if(res != LE_OK)
    {
        return res;
    }
    res = tafMngdPMSvc::ReleaseWakeLock();
    if(res == LE_OK)
    {
        LE_INFO("client released wakesource for %s the with reason: %d, wakeSourceState set to: WAKE_SOURCE_NOT_ACQUIRED ", wsRefCtxPtr->wsTag, wsRefCtxPtr->reason);
        tafMngdPMSvc::ProcessStateChange(
                TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE);
        wsRefCtxPtr->wakeSourceState = WAKE_SOURCE_NOT_ACQUIRED;
        LE_INFO("ReleaseWakeSource state: %d", wsRefCtxPtr->wakeSourceState);
        //sending notification to VHAL
        if((mpms.pmInf) && (mpms.pmInf->nodeInfoNotification))
        {
            LE_INFO("notify node info for reason: %d", wsRefCtxPtr->reason);
            (*(mpms.pmInf->nodeInfoNotification))(NODE_ID,
                    HAL_PM_NODE_INFO_LOCK_RELEASED, (const uint8_t)wsRefCtxPtr->reason);
        }
    }
    return res;
}

/**
 * Release wakesource
 */
le_result_t tafMngdPMSvc::ReleaseWakeLock()
{
    LE_INFO("ReleaseWakeLock");
    le_result_t res = LE_FAULT;
    if(wsCount > 0 )
    {
        LE_INFO("Wake source released successfully");
        wsCount--;
        LE_INFO("ReleaseWakeLock wsCount:%d", wsCount);
        res = LE_OK;
    }
    if(wsCount == 0 )
    {
        if (ws != nullptr && powerMode.isWsAcquired)
        {
            if(tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE) != LE_OK)
            {
                return res;
            }
            ClearUnauthorizedWs();
            res = taf_pm_Relax(ws);
            if(res == LE_OK) {
                LE_INFO("Wake source from pms released successfully");
                powerMode.isWsAcquired = false;
            }
        }
        else
        {
            LE_ERROR("Failed to release wakeup lock!");
            res = LE_FAULT;
        }
    }
    return res;
}

/**
 * Request MPMS state change condition check
 */
le_result_t tafMngdPMSvc::RequestStateChange(uint8_t requestedState)
{
    LE_INFO("current state %s", TafStateToString(stateMachine.currentState));
    LE_INFO("requested state %s", TafStateToString(requestedState));

    le_result_t res = LE_OK;
    if (stateMachine.currentState == requestedState)
    {
        return res;
    }
    switch(requestedState)
    {
        case TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE:
            if(stateMachine.currentState != TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE &&
               stateMachine.currentState != TAF_MNGDPM_STATE_RESUME)
            {
                res = LE_NOT_PERMITTED;
            }
            break;

        case TAF_MNGDPM_STATE_SUSPENDING:
            if(stateMachine.currentState != TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE &&
               stateMachine.currentState != TAF_MNGDPM_STATE_RESUME)
            {
                res = LE_NOT_PERMITTED;
            }
            break;

        case TAF_MNGDPM_STATE_RESTARTING:
        case TAF_MNGDPM_STATE_SHUTTING_DOWN:
            if(stateMachine.currentState != TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE &&
               stateMachine.currentState != TAF_MNGDPM_STATE_RESUME)
            {
                res = LE_NOT_PERMITTED;
            }
            break;

        case TAF_MNGDPM_STATE_WAKING_UP:
            if(stateMachine.currentState == TAF_MNGDPM_STATE_SHUTTING_DOWN
                    || stateMachine.currentState == TAF_MNGDPM_STATE_RESTARTING
                            ||  stateMachine.currentState == TAF_MNGDPM_STATE_SHUTDOWN
                                    ||  stateMachine.currentState == TAF_MNGDPM_STATE_RESTART
                                        ||  stateMachine.currentState == TAF_MNGDPM_STATE_SUSPENDING)
            {
                res = LE_NOT_PERMITTED;
            }
            break;

        default:
            res = LE_NOT_PERMITTED;
            break;
    }

    if(res == LE_NOT_PERMITTED)
    {
        LE_ERROR("RequestStateChange for %d is NOT PERMITTED", requestedState);
        return res;
    }

    LE_INFO("RequestStateChange for %d is OK", requestedState);
    return LE_OK;
}

/**
 * Process MPMS state change
 */
void tafMngdPMSvc::ProcessStateChange(uint8_t toState)
{
    LE_INFO("current state %s", TafStateToString(stateMachine.currentState));

    switch(toState)
    {
        case TAF_MNGDPM_STATE_WAKING_UP:
            if(stateMachine.currentState == TAF_MNGDPM_STATE_RESUME
               || stateMachine.currentState == TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE)
            {
                // No need to change state from RESUME and RELEASING_WAKE_SOURCE to WAKING_UP
                toState = stateMachine.currentState;
            }
            break;

        default:
            break;
    }

    if(toState != stateMachine.currentState)
    {
        stateMachine.prevState = stateMachine.currentState;
        stateMachine.currentState = toState;
    }

    LE_INFO("change to state %s", TafStateToString(toState));
}

void tafMngdPMSvc::DeleteNodePowerStateRefs()
{
    LE_INFO("DeleteNodePowerStateRefs");
    auto &mpms = tafMngdPMSvc::GetInstance();
    for (const auto &client : mpms.regClientrecrd ) {
            le_ref_DeleteRef(nodePowerStateRefMap, client.nodeStateRef);
    }
}

bool tafMngdPMSvc::IsSameAsCurrentState(taf_mngdPm_NodePowerState_t nodeState, uint8_t tafState)
{
    LE_INFO("IsSameAsCurrentState");
    if((nodeState == TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE) && (tafState == TAF_MNGDPM_STATE_SHUTDOWN))
    {
        return true;
    }
    else if((nodeState == TAF_MNGDPM_NODE_STATE_RESTART_PREPARE) && (tafState == TAF_MNGDPM_STATE_SHUTDOWN))
    {
        return true;
    }
    else if((nodeState == TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE) && (tafState == TAF_MNGDPM_STATE_SUSPEND))
    {
        return true;
    }
    else if((nodeState == TAF_MNGDPM_NODE_STATE_RESUME) && (tafState == TAF_MNGDPM_STATE_RESUME))
    {
        return true;
    }
    else
    {
        LE_ERROR("Not same as the current State!");
        return false;
    }
}

bool tafMngdPMSvc::IsConfiguredBitMask(taf_mngdPm_NodePowerState_t state, taf_mngdPm_NodePowerStateChangeBitMask_t stateMask)
{
    LE_INFO("IsConfiguredBitMask");
    bool isSameBitMask = false;
    if(state == TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE && (stateMask & (1)) !=0)
    {
        isSameBitMask = true;
    }
    else if(state == TAF_MNGDPM_NODE_STATE_RESTART_PREPARE && (stateMask & (1 << 1)) !=0)
    {
        isSameBitMask = true;
    }
    else if(state == TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE && (stateMask & (1 << 2)) !=0)
    {
        isSameBitMask = true;
    }
    else if(state == TAF_MNGDPM_NODE_STATE_RESUME && (stateMask & (1 << 3)) !=0)
    {
        isSameBitMask = true;
    }
    else
    {
        LE_ERROR("IsConfiguredBitMask: false");
        isSameBitMask = false;
    }
    return isSameBitMask;
}

void tafMngdPMSvc::SendAckToPms(taf_mngdPm_NodePowerState_t state, taf_pm_ClientAck_t ackType)
{
    LE_INFO("SendAckToPms");
    auto &mpms = tafMngdPMSvc::GetInstance();
    if (state == TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE)
    {
        LE_INFO("TAF_PM_STATE_SHUTDOWN");
        taf_pm_SendStateChangeAck(mpms.powerStateRef, TAF_PM_STATE_SHUTDOWN, TAF_PM_PVM, ackType);
    }
    else if(state== TAF_MNGDPM_NODE_STATE_RESTART_PREPARE)
    {
        LE_INFO("TAF_PM_STATE_SHUTDOWN");
        taf_pm_SendStateChangeAck(mpms.powerStateRef, TAF_PM_STATE_RESTART, TAF_PM_PVM, ackType);
    }
    else if (state == TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE)
    {
        LE_INFO("TAF_PM_STATE_SUSPEND");
        taf_pm_SendStateChangeAck(mpms.powerStateRef, TAF_PM_STATE_SUSPEND, TAF_PM_PVM, ackType);
    }
    else if (state == TAF_MNGDPM_NODE_STATE_RESUME)
    {
        LE_INFO("TAF_PM_STATE_RESUME");
        taf_pm_SendStateChangeAck(mpms.powerStateRef, TAF_PM_STATE_RESUME, TAF_PM_PVM,
                ackType);
    }
}

/**
 * State change ack timer handler
 */
void tafMngdPMSvc::StateChangeAckTimerHandler(le_timer_Ref_t timerRef)
{
    auto &mpms = tafMngdPMSvc::GetInstance();
    taf_mngdPm_SessionNode_t* sessionNodePtr;
    int8_t UnresponsiveClients = (int8_t)mpms.regClientrecrd.size() -  mpms.ackClientrecrdSize;
    LE_WARN("No of Unresponsive clients %d", UnresponsiveClients);
    for (const auto &client : mpms.regClientrecrd )
    {
        if(!client.isAcked)
        {
            //Getting the current client data from mngdPmClientInfo
            sessionNodePtr = mpms.To_taf_mngdPm_SessionNode_t(le_hashmap_Get(mpms.mngdPmClientInfo.clients,
                    client.sessionRef));
            LE_WARN("client %s not acknowledged for state change", sessionNodePtr->name);
        }
    }
    mpms.clientSize = 0;
    mpms.ackClientrecrdSize = 0;
    taf_mngdPm_NodePowerState_t* state = (taf_mngdPm_NodePowerState_t*)le_timer_GetContextPtr(timerRef);
    if (state == nullptr) {
        LE_ERROR("State pointer is null");
        return;
    }
    LE_INFO("StateChangeAckTimer Expired after %ld msec for state %d", mpms.config.state_change_ack_timeout,
            *(state));
    mpms.SendAckToPms(*(state), TAF_PM_READY);
    return;
}

/**
 * To Call Clients for Extend power state change notification
 */
void tafMngdPMSvc::CallNodePowerStateHandlerFunc(taf_mngdPm_NodePowerState_t state)
{
    LE_INFO("CallNodePowerStateHandlerFunc");
    auto &mpms = tafMngdPMSvc::GetInstance();
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.nodePowerStateHandlerList));
    //clearing the previous references for new state notification
    DeleteNodePowerStateRefs();
    mpms.regClientrecrd.clear();
    mpms.ackClientrecrdSize = 0;
    mpms.clientSize = 0;
    while (linkHandlerPtr)
    {
        taf_mngdPm_NodePowerStateCtxt_t * handlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_mngdPm_NodePowerStateCtxt_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.nodePowerStateHandlerList), linkHandlerPtr);
        if (handlerCtxPtr->handlerPtr)
        {
            LE_INFO("Client found");
            if(mpms.IsConfiguredBitMask(state, handlerCtxPtr->powerStateMask))
            {
                taf_NodePowerStateRef_t* nodeStateListPtr =
                        (taf_NodePowerStateRef_t*)le_mem_ForceAlloc(mpms.nodePowerStateRefPool);
                nodeStateListPtr->nodeStateRef =
                        (taf_mngdPm_nodePowerStateRef_t)le_ref_CreateRef(mpms.nodePowerStateRefMap,
                                nodeStateListPtr);
                mpms.regClientrecrd.push_back({nodeStateListPtr->nodeStateRef,
                        handlerCtxPtr->sessionRef, state, false});
                handlerCtxPtr->handlerPtr(handlerCtxPtr->pmNodeId, nodeStateListPtr->nodeStateRef,
                        state, handlerCtxPtr->nodePowerStateHandlerCtxPtr);
                LE_INFO("Notified to Client");
            }
            else {
                continue;
            }
            mpms.clientSize++;
        }
    }
    if(mpms.clientSize == 0)
    {
        LE_INFO("No client registered in MPMS, ack to PMS immediately for state:%d", state);
        mpms.SendAckToPms(state, TAF_PM_READY);
    }
    else
    {
        mpms.currentStateChangePtr = state;
        le_timer_SetContextPtr(mpms.stateChangeAckTimerRef, &(mpms.currentStateChangePtr));
        le_timer_Start(mpms.stateChangeAckTimerRef);
        LE_INFO("stateChangeAck Timer has started");
    }
}

/**
 * State change layered handler function
 */
void tafMngdPMSvc::NodePowerStateChanged(void* reportPtr)
{
    LE_INFO("NodePowerStateChanged");
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");
    taf_mngdPm_NodePowerStateChange_t* powerStateChange =(taf_mngdPm_NodePowerStateChange_t*)reportPtr;
    if(powerStateChange->state == TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE)
    {
        CallNodePowerStateHandlerFunc(powerStateChange->state);
    }
    else if(powerStateChange->state == TAF_MNGDPM_NODE_STATE_RESTART_PREPARE)
    {
        CallNodePowerStateHandlerFunc(powerStateChange->state);
    }
    else if(powerStateChange->state == TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE)
    {
        CallNodePowerStateHandlerFunc(powerStateChange->state);
    }
    else if(powerStateChange->state == TAF_MNGDPM_NODE_STATE_RESUME)
    {
        CallNodePowerStateHandlerFunc(powerStateChange->state);
    }
    else
    {
        LE_ERROR("Invalid state");
    }
}

/**
 * Call Clients for Bub Status Event notification
 */
void tafMngdPMSvc::InfoReportCB(void* reportPtr)
{
    LE_INFO("InfoReportCB");
    bubStatusEvent_t* stateEvent = (bubStatusEvent_t*)reportPtr;
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&infoReportHandlerList);
    while (linkHandlerPtr)
    {
        taf_mngdPm_InfoReportCb_t * handlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_mngdPm_InfoReportCb_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&infoReportHandlerList, linkHandlerPtr);
        if (handlerCtxPtr->handlerPtr)
        {
            LE_INFO("Notifying to clients");
            handlerCtxPtr->handlerPtr(stateEvent->status, handlerCtxPtr->infoReportHandlerCtxPtr);
        }
    }
    if(stateEvent->status == TAF_MNGDPM_BUB_STATUS_IN_USE)
    {
        LE_INFO("Bub is in use");
        le_result_t res = taf_pm_SetPowerMode(TAF_PM_POWER_MODE_LOW_POWER);
        if(res == LE_OK)
        {
            LE_INFO("Power Mode is set to Low Power");
        }
    }
    else
    {
        LE_INFO("Bub is not in low power mode");
        le_result_t res = taf_pm_SetPowerMode(TAF_PM_POWER_MODE_NORMAL);
        if(res == LE_OK)
        {
            LE_INFO("Power Mode is set to normal");
        }
    }
}

/**
 * VHAL callback for Bub Status Event notification
 */
void tafMngdPMSvc::InfoReportVhalCB(int32_t* reportPtr)
{
    LE_INFO("InfoReportVhalCB");
    bubStatusEvent_t bubStatusEvent;
    int32_t bubStatus = *reportPtr;
    bubStatusEvent.status = (taf_mngdPm_BubStatus_t)bubStatus;
    le_event_Report(infoReport, &bubStatusEvent, sizeof(bubStatusEvent_t));
}

/**
 * Authorize StayAwake Reason for a given reason.
 */
bool tafMngdPMSvc::IsAuthorizedStayAwakeReason(taf_mngdPm_StayAwakeReason_t stayAwakeReason)
{
    LE_INFO("AuthorizeStayAwakeReason");
    auto &mpms = tafMngdPMSvc::GetInstance();
    unsigned int clientMask = 0;

    if(stayAwakeReason < 32)
    {
        //getting decimal value for given stay awake reason.
        clientMask = (unsigned int)pow(2, (unsigned int)stayAwakeReason);
    }
    //Converting to binary format to check the given stay awake reason is authorized.
    std::bitset<32> clientStayAwakeReasonMask(clientMask);
    if((clientStayAwakeReasonMask & mpms.stayAwakeReasonMask) == clientStayAwakeReasonMask)
    {
        LE_INFO("stayAwakeReason: %d is authorized", stayAwakeReason);
        return true;
    }
    else
    {
        LE_ERROR("stayAwakeReason: %d is unauthorized", stayAwakeReason);
        return false;
    }

}

/**
 * Type-cast from void *(mpm client table record pointer) to taf_mngdPm_SessionNode_t
 */
taf_mngdPm_SessionNode_t* tafMngdPMSvc::To_taf_mngdPm_SessionNode_t(void *c)
{
    taf_mngdPm_SessionNode_t *cl = (taf_mngdPm_SessionNode_t *)c;
    TAF_ERROR_IF_RET_VAL(cl == NULL, NULL, "INVALID conversion.");

    return cl;
}

/**
 * Get MPMS instance
 */
tafMngdPMSvc &tafMngdPMSvc::GetInstance()
{
   static tafMngdPMSvc instance;
   return instance;
}

/**
 * tafMngdPMSvc initialization
 */
void tafMngdPMSvc::Init(void)
{
    // Local -> node_id => 0, Remote -> node_id => 1
    cbLocalMap = le_ref_CreateMap("mpms-l-cb-map", TAF_MNGDPM_VM_HASH_SIZE);
    cbRemoteMap = le_ref_CreateMap("mpms-r-cb-map", TAF_MNGDPM_VM_HASH_SIZE);

    cbHandlerPool = le_mem_CreatePool("mpms-cb-pool", sizeof(CallbackHandlerCombo_t));

    // Flags to indicate if the enable-action was done or not.
    enableLocal = false;
    enableRemote = false;
}

void tafMngdPMSvc::EnableLocalOnce(void)
{
    pmsLocalWakeupHandler =
        taf_pm_AddModemAwakeHandler(
            tafMngdPMSvc::ClientCallbackDispatcher,
            cbLocalMap,
            0);

    LE_FATAL_IF(
        pmsLocalWakeupHandler == NULL,
        "Failed to taf_pm_AddModemAwakeHandler"
    );

    enableLocal = true;
}

void tafMngdPMSvc::EnableRemoteOnce(void)
{
    pmsRemoteWakeupHandler =
        taf_rpcPm_AddModemAwakeHandler(
            tafMngdPMSvc::ClientCallbackDispatcher,
            cbRemoteMap,
            0);

    LE_FATAL_IF(
        pmsRemoteWakeupHandler == NULL,
        "Failed to pmsRemoteWakeupHandler"
    );

    enableRemote = true;
}

void tafMngdPMSvc::ClientCallbackDispatcher
(
    taf_pm_ModemAwakeEventRef_t ref,
    taf_pm_NodeModemWsBitMask_t wsBitmask,
    void * contextPtr
)
{
    LE_UNUSED(ref);

    LE_INFO("Hit: ClientCallbackDispatcher");

    le_ref_MapRef_t cbMap = (le_ref_MapRef_t )contextPtr;

    le_ref_IterRef_t iterRef = le_ref_GetIterator(cbMap);

    CallbackHandlerCombo_t *combo = NULL;

    while ( le_ref_NextNode(iterRef) == LE_OK )
    {
        combo = (CallbackHandlerCombo_t *)le_ref_GetValue(iterRef);

        // [t, main-thread]: one by one
        combo->callback((taf_mngdPm_NodeModemAwakeEventRef_t) NULL,
                        combo->nodeId,
                        wsBitmask,
                        combo->context);
    }

    LE_INFO("All callback dispatched [done]");
}

/**
 * initialize static variables
 */
le_mem_PoolRef_t tafMngdPMSvc::vmStatePool;
le_hashmap_Ref_t tafMngdPMSvc::vmStateHashmap;
le_mem_PoolRef_t tafMngdPMSvc::wsRefPool;
le_dls_List_t tafMngdPMSvc::wsRefList;
le_ref_MapRef_t tafMngdPMSvc::wsRefMap;

//cached awake requests ws reference set
std::unordered_set<taf_mngdPm_wsRef_t>  tafMngdPMSvc::wsCachedReqsRefSet;

le_mem_PoolRef_t tafMngdPMSvc::nodeWsRefPool;
le_dls_List_t tafMngdPMSvc::nodeWsRefList;
le_ref_MapRef_t tafMngdPMSvc::nodeWsRefMap;

taf_pm_StateChangeHandlerRef_t tafMngdPMSvc::handlerRef = nullptr;
taf_pm_StateChangeExHandlerRef_t tafMngdPMSvc::handlerExRef = nullptr;
taf_pm_PowerStateRef_t tafMngdPMSvc::powerStateRef = nullptr;
taf_pm_WakeupSourceRef_t tafMngdPMSvc::ws = nullptr;

taf_mngdPm_TargetedPowerMode_t tafMngdPMSvc::targetedPowerMode = TAF_MNGDPM_RESUME;
taf_mngdPm_RestartCb_t tafMngdPMSvc::restartCB;
taf_mngdPm_ShutdownCb_t tafMngdPMSvc::shutdownCB;

uint8_t tafMngdPMSvc::wsCount = 0;
taf_powerMode_t tafMngdPMSvc::powerMode{};
taf_stateMachine_t tafMngdPMSvc::stateMachine{};

hal_pm_Inf_t* tafMngdPMSvc::pmInf = nullptr;
le_timer_Ref_t tafMngdPMSvc::vhalAckTimerRef = nullptr;
le_timer_Ref_t tafMngdPMSvc::wakeSourceTimerRef = nullptr;
le_timer_Ref_t tafMngdPMSvc::wakeupVehicleTimerRef = nullptr;
taf_mngdPm_RequestedWakeupVehicle_t tafMngdPMSvc::wakeupModePtr;
taf_mngdPm_RequestedState_t tafMngdPMSvc::statePtr;
taf_mngdPm_Client_t tafMngdPMSvc::mngdPmClientInfo;

taf_mngdPm_WakeupVehicleCb_t tafMngdPMSvc::wakeupVehicleCB;

le_mem_PoolRef_t tafMngdPMSvc::infoReportHandlerPool;
le_dls_List_t tafMngdPMSvc::infoReportHandlerList;
le_ref_MapRef_t tafMngdPMSvc::infoReportHandlerRefMap;
le_event_Id_t tafMngdPMSvc::infoReport;
//Node Power State change handler
le_event_Id_t tafMngdPMSvc::nodePowerStateChange;
le_mem_PoolRef_t tafMngdPMSvc::nodePowerStateHandlerPool;
le_dls_List_t tafMngdPMSvc::nodePowerStateHandlerList;
le_ref_MapRef_t tafMngdPMSvc::nodePowerStateHandlerMap;
le_mem_PoolRef_t tafMngdPMSvc::nodePowerStateRefPool;
le_ref_MapRef_t tafMngdPMSvc::nodePowerStateRefMap;
int8_t tafMngdPMSvc::clientSize;
int8_t tafMngdPMSvc::ackClientrecrdSize;
taf_mngdPm_config_t tafMngdPMSvc::config;

//authorize stayawake reason
std::bitset<32>  tafMngdPMSvc::stayAwakeReasonMask;

//resources for clients state change acknowledgement
le_timer_Ref_t tafMngdPMSvc::stateChangeAckTimerRef;
taf_mngdPm_NodePowerState_t tafMngdPMSvc::currentStateChangePtr;

le_mem_PoolRef_t tafMngdPMSvc::cbHandlerPool;
le_ref_MapRef_t tafMngdPMSvc::cbLocalMap;
le_ref_MapRef_t tafMngdPMSvc::cbRemoteMap;
bool tafMngdPMSvc::enableLocal;
bool tafMngdPMSvc::enableRemote;
taf_pm_ModemAwakeHandlerRef_t tafMngdPMSvc::pmsLocalWakeupHandler;
taf_pm_ModemAwakeHandlerRef_t tafMngdPMSvc::pmsRemoteWakeupHandler;

le_event_Id_t tafMngdPMSvc::pmEvtReady;
void tafMngdPMSvc::PMVhalReadyEvtHandler(void * reportPtr);