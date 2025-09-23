/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafMngdPMSvc.hpp"
#include "tafMngdPMCommon.hpp"
#include "rpcPm/tafMngdRpcPm.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#ifdef __cplusplus
extern "C" {
#endif
#include "watchdogChain.h"
#ifdef __cplusplus
}
#endif

using namespace tafsvc;

LE_MEM_DEFINE_STATIC_POOL(SessionCtx, MAX_SESSION, sizeof(taf_mngdPm_SessionNode_t));

/**
 * Sets the NAD to Targeted  power mode.
 */
le_result_t taf_mngdPm_SetNodeTargetedPowerMode(uint8_t pm_node_id,
        taf_mngdPm_TargetedPowerMode_t targetPowerMode)
{
    LE_INFO("taf_mngdPm_SetNodeTargetedPowerMode targetPowerMode : %d", targetPowerMode);
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client");
        return LE_UNSUPPORTED;
    }

    le_result_t res = LE_FAULT;
    auto &mpms = tafMngdPMSvc::GetInstance();
    if(pm_node_id == 1)
    {
        auto &rpcPm = tafMngdRpcPm::GetInstance();
        rpcPm.rpcTargetedPowerMode = targetPowerMode;
        res = tafMngdRpcPm::AcquireRpcNodeWakeLock();
        if(res == LE_OK)
            LE_INFO("Wake source acquired successfully");
        res = tafMngdRpcPm::ReleaseRpcNodeWakeLock();
        if(res == LE_OK)
            LE_INFO("Wake source released successfully");
    }
    else
    {
        mpms.targetedPowerMode = targetPowerMode;
        if(targetPowerMode == TAF_MNGDPM_SUSPEND || targetPowerMode == TAF_MNGDPM_SHUTDOWN)
        {
            res = tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE);
            if(res != LE_OK)
            {
                return res;
            }
            mpms.powerMode.isGraceful = true;
        }
        // Acquire a wakelock to get notified on last wakeup source release.
        if (mpms.ws != nullptr)
        {
            res = tafMngdPMSvc::AcquireWakeLock();
            if(res == LE_OK)
                LE_INFO("Wake source acquired successfully");
            res = tafMngdPMSvc::ReleaseWakeLock();
            if(res == LE_OK)
                LE_INFO("Wake source released successfully");
        }
        else
        {
            LE_ERROR("Failed to create wakeup source!");
        }
    }
    return res;
}

/**
 * ShutDown the system with requested mode and shutdown reason.
 */
le_result_t taf_mngdPm_ShutdownReqAsync(taf_mngdPm_ShutdownMode_t mode,
    taf_mngdPm_AsyncShutdownReqHandlerFunc_t handlerPtr, void* contextPtr,
    taf_mngdPm_ShutdownReason_t reason)
{
    LE_INFO("taf_mngdPm_ShutdownReqAsync");
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client");
        if(handlerPtr != NULL)
        {
            handlerPtr(mode, TAF_MNGDPM_NOT_READY, LE_UNSUPPORTED, contextPtr);
        }
        return LE_UNSUPPORTED;
    }

    if(reason > 1 && reason < 15)
    {
        LE_ERROR("Shutdown reason:%d is not supported", reason);
        if(handlerPtr != NULL)
        {
            handlerPtr(mode, TAF_MNGDPM_NOT_READY, LE_UNSUPPORTED, contextPtr);
        }
        return LE_UNSUPPORTED;
    }
    TAF_ERROR_IF_RET_VAL(!handlerPtr, LE_BAD_PARAMETER, "invalid handlerRef");

    auto &mpms = tafMngdPMSvc::GetInstance();
    if (mpms.stateMachine.currentState == TAF_MNGDPM_STATE_SHUTTING_DOWN ||
            mpms.stateMachine.currentState == TAF_MNGDPM_STATE_SHUTDOWN)
    {
        handlerPtr(mode, TAF_MNGDPM_NOT_READY, LE_BUSY, contextPtr);
        LE_INFO("ShutdownReqAsync is already in progress");
        return LE_BUSY;
    }

    if (mpms.RequestStateChange(TAF_MNGDPM_STATE_SHUTTING_DOWN) != LE_OK)
    {
        handlerPtr(mode, TAF_MNGDPM_NOT_READY, LE_OK, contextPtr);
        return LE_OK;
    }
    mpms.powerMode.isForceful = true;
    tafMngdPMSvc::ProcessStateChange(TAF_MNGDPM_STATE_SHUTTING_DOWN);
    if(mpms.pmInf && mpms.pmInf->nodeStateChangePrepareAsync)
    {
        LE_INFO("Send shutdownReqAsync %d", HAL_PM_SHUTDOWN_MODE_NORMAL);
        const uint8_t shutdownReason = (uint8_t)reason;
        mpms.statePtr = SYSTEM_NORMAL_SHUTDOWN;
        le_timer_SetContextPtr(mpms.vhalAckTimerRef, &(mpms.statePtr));
        le_timer_Start(mpms.vhalAckTimerRef);
        mpms.shutdownCB.shutdownCallbackFunc = handlerPtr;
        mpms.shutdownCB.shutdownCBCtxPtr = contextPtr;
        mpms.shutdownCB.sessionRef = taf_mngdPm_GetClientSessionRef();
        (*(mpms.pmInf->nodeStateChangePrepareAsync))(NODE_ID, HAL_PM_NODE_STATE_SHUTDOWN,
                HAL_PM_SHUTDOWN_MODE_NORMAL, shutdownReason, tafMngdPMSvc::ShutdownPrepareRespCB);
    }
    else
    {
        LE_INFO("Ignore VHAL response if drive is not available");
        le_result_t res = tafMngdPMSvc::ShutdownNAD();
        if(res == LE_OK)
        {
            mpms.powerMode.isGraceful = false;
        }
        // Send ready incase of driver not available.
        handlerPtr(mode, TAF_MNGDPM_READY, LE_OK, contextPtr);
    }

    return LE_OK;
}

/**
 * Restarts the system with requested mode.
 */
le_result_t taf_mngdPm_RestartReqAsync(taf_mngdPm_RestartMode_t mode,
    taf_mngdPm_AsyncRestartReqHandlerFunc_t handlerPtr, void* contextPtr,
    taf_mngdPm_RestartReason_t reason)
{
    LE_INFO("taf_mngdPm_RestartReqAsync");
    auto &mpms = tafMngdPMSvc::GetInstance();

    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client");
        if(handlerPtr != NULL)
        {
            handlerPtr(mode, TAF_MNGDPM_NOT_READY, LE_UNSUPPORTED, contextPtr);
        }
        return LE_UNSUPPORTED;
    }
    if(reason > 2 && reason < 15)
    {
        LE_ERROR("Restart reason:%d is not supported", reason);
        if(handlerPtr != NULL)
        {
            handlerPtr(mode, TAF_MNGDPM_NOT_READY, LE_UNSUPPORTED, contextPtr);
        }
        return LE_UNSUPPORTED;
    }
    TAF_ERROR_IF_RET_VAL(mpms.handlerRef == nullptr, LE_BAD_PARAMETER, "invalid handlerRef");

    if(mode == TAF_MNGDPM_RESTART_SYSTEM_OFF_ON)
    {
        LE_INFO("mode is TAF_MNGDPM_RESTART_SYSTEM_OFF_ON");
        if((mpms.stateMachine.currentState == TAF_MNGDPM_STATE_SHUTTING_DOWN) ||
                (mpms.stateMachine.currentState == TAF_MNGDPM_STATE_SHUTDOWN))
        {
            handlerPtr(mode, TAF_MNGDPM_NOT_READY, LE_BUSY, contextPtr);
            LE_INFO("RestartReqAsync is already in progress");
            return LE_BUSY;
        }

        else {
            if(mpms.RequestStateChange(TAF_MNGDPM_STATE_SHUTTING_DOWN) == LE_OK)
            {
                tafMngdPMSvc::ProcessStateChange(TAF_MNGDPM_STATE_SHUTTING_DOWN);
            }
            else
            {
                handlerPtr(mode, TAF_MNGDPM_NOT_READY, LE_OK, contextPtr);
                return LE_OK;
            }
        }
    }
    else if(mode == TAF_MNGDPM_RESTART_MODE_NAD_REBOOT)
    {
        LE_INFO("mode is TAF_MNGDPM_RESTART_MODE_NAD_REBOOT");
        if(mpms.stateMachine.currentState == TAF_MNGDPM_STATE_RESTARTING ||
                mpms.stateMachine.currentState == TAF_MNGDPM_STATE_RESTART)
        {
            handlerPtr(mode, TAF_MNGDPM_NOT_READY, LE_BUSY, contextPtr);
            LE_INFO("RestartReqAsync is already in progress");
            return LE_BUSY;
        }
        else {
            if (mpms.RequestStateChange(TAF_MNGDPM_STATE_RESTARTING) == LE_OK)
            {
                tafMngdPMSvc::ProcessStateChange(TAF_MNGDPM_STATE_RESTARTING);
            }
            else
            {
                handlerPtr(mode, TAF_MNGDPM_NOT_READY, LE_OK, contextPtr);
                return LE_OK;
            }
        }
    }

    if(mpms.pmInf && mpms.pmInf->nodeStateChangePrepareAsync)
    {
        const uint8_t restartReason = (uint8_t)reason;
        if(mode == TAF_MNGDPM_RESTART_SYSTEM_OFF_ON)
        {
            LE_INFO("Send restartReqAsync %d", HAL_PM_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF);
            mpms.powerMode.isShutDown = true;
            mpms.statePtr = RESTART_WITH_NAD_POWER_OFF_ON;
            le_timer_SetContextPtr(mpms.vhalAckTimerRef, &(mpms.statePtr));
            le_timer_Start(mpms.vhalAckTimerRef);
            mpms.restartCB.restartCallbackFunc = handlerPtr;
            mpms.restartCB.restartCBCtxPtr = contextPtr;
            mpms.restartCB.sessionRef = taf_mngdPm_GetClientSessionRef();
            (*(mpms.pmInf->nodeStateChangePrepareAsync))(NODE_ID, HAL_PM_NODE_STATE_RESTART, HAL_PM_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF,
                    restartReason, tafMngdPMSvc::RestartPrepareRespCB);
        }
        else if(mode == TAF_MNGDPM_RESTART_MODE_NAD_REBOOT)
        {
            LE_INFO("Send restartReqAsync %d", HAL_PM_RESTART_MODE_NAD_REBOOT);
            mpms.powerMode.isRestart = true;
            mpms.statePtr = RESTART_WITH_NAD_REBOOT;
            le_timer_SetContextPtr(mpms.vhalAckTimerRef, &(mpms.statePtr));
            le_timer_Start(mpms.vhalAckTimerRef);
            mpms.restartCB.restartCallbackFunc = handlerPtr;
            mpms.restartCB.restartCBCtxPtr = contextPtr;
            mpms.restartCB.sessionRef = taf_mngdPm_GetClientSessionRef();
            (*(mpms.pmInf->nodeStateChangePrepareAsync))(NODE_ID, HAL_PM_NODE_STATE_RESTART, HAL_PM_RESTART_MODE_NAD_REBOOT,
                    restartReason, tafMngdPMSvc::RestartPrepareRespCB);
        }
    }
    else
    {
        LE_INFO("Ignore VHAL response if drive is not available");
        if(mode == TAF_MNGDPM_RESTART_SYSTEM_OFF_ON)
        {
            LE_INFO("mode is TAF_MNGDPM_RESTART_SYSTEM_OFF_ON");
            le_result_t res = tafMngdPMSvc::ShutdownNAD();
            if(res == LE_OK)
            {
                mpms.powerMode.isShutDown = true;
                mpms.powerMode.isGraceful = false;
            }
        }
        else if(mode == TAF_MNGDPM_RESTART_MODE_NAD_REBOOT)
        {
            LE_INFO("mode is TAF_MNGDPM_RESTART_MODE_NAD_REBOOT");
            le_result_t res = tafMngdPMSvc::RestartNAD();
            if(res == LE_OK)
            {
                mpms.powerMode.isRestart = true;
                mpms.powerMode.isGraceful = false;
            }
        }
        // Send ready incase of driver not available.
        handlerPtr(mode, TAF_MNGDPM_READY, LE_OK, contextPtr);
    }
    return LE_OK;
}

/**
 * WakeupVehicleReq with requested reason.
 */
le_result_t taf_mngdPm_WakeupVehicleReqAsync(int32_t reason,
    taf_mngdPm_AsyncWakeupVehicleReqHandlerFunc_t handlerPtr, void* contextPtr)
{
    LE_INFO("taf_mngdPm_WakeupVehicleReqAsync");
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client");
        if (handlerPtr != nullptr)
        {
            handlerPtr(reason, VEHICHLE_WAKEUP_STATUS_INVALID_REQ, LE_UNSUPPORTED, contextPtr);
        }
        return LE_UNSUPPORTED;
    }

    auto &mpms = tafMngdPMSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(mpms.handlerRef == nullptr, LE_BAD_PARAMETER, "invalid handlerRef");
    if(mpms.wakeupVehicleCB.wakeupVehicleCallbackFunc != nullptr)
    {
            mpms.wakeupVehicleCB.wakeupVehicleCallbackFunc(VEHICHLE_WAKEUP_REASON_DEFAULT, VEHICHLE_WAKEUP_STATUS_UNKNOWN,
                    LE_BUSY, contextPtr);
            LE_INFO("WakeupVehicleReqAsync is already in progress");
            return LE_BUSY;
    }
    if(reason == VEHICHLE_WAKEUP_REASON_DEFAULT)
    {
        if(mpms.pmInf && mpms.pmInf->wakeupVehicleReqAsync)
        {
            LE_INFO("Send wakeupVehicleReqAsync %d", HAL_PM_VEHICHLE_WAKEUP_STATUS_AWAKE);
            if(mpms.stateMachine.currentState != TAF_MNGDPM_STATE_RESUME &&
                mpms.stateMachine.currentState != TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE)
            {
                LE_INFO("WakeupVehicleReqAsync not allowed in current state: %d",
                    mpms.stateMachine.currentState);
                handlerPtr = nullptr;
                return LE_UNSUPPORTED;
            }
            mpms.wakeupModePtr = WAKEUP_VEHICHLE_REQ_DEFAULT;
            le_timer_SetContextPtr(mpms.wakeupVehicleTimerRef, &(mpms.wakeupModePtr));
            le_timer_Start(mpms.wakeupVehicleTimerRef);
            LE_INFO("Timer has started");
            mpms.wakeupVehicleCB.wakeupVehicleCallbackFunc = handlerPtr;
            mpms.wakeupVehicleCB.wakeupVehicleCBCtxPtr = contextPtr;
            mpms.wakeupVehicleCB.sessionRef = taf_mngdPm_GetClientSessionRef();
            LE_INFO("Client with sessionRef %p", mpms.wakeupVehicleCB.sessionRef);
            (*(mpms.pmInf->wakeupVehicleReqAsync))(VEHICHLE_WAKEUP_REASON_DEFAULT, tafMngdPMSvc::WakeupVehicleCB);
        }
        else
        {
            LE_ERROR("Returning unsupported if drive is not available");

            if (handlerPtr != nullptr)
            {
                handlerPtr(reason, VEHICHLE_WAKEUP_STATUS_INVALID_REQ, LE_OK, contextPtr);
            }
            // Send ready incase of driver not available.
            return LE_UNSUPPORTED;
        }
    }

    return LE_OK;
}

/**
 * Sets the Modem wakeupSource type.
 */
le_result_t taf_mngdPm_SetModemWakeupSource (
        uint32_t wakeupSource)
{
    LE_INFO("taf_mngdPm_SetModemWakeupSource");
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client");
        return LE_UNSUPPORTED;
    }

    taf_mngdPm_WakeupType_t wakeupType;
    if((wakeupSource & (1)) != 0)
    {
        LE_INFO("wakeupType is SMS");
        wakeupType = TAF_MNGDPM_SMS;
        tafMngdPMSvc::SetModemWakeupSource(wakeupType);
    }
    if((wakeupSource & (1 << 1)) != 0)
    {
        LE_INFO("wakeupType is VOICE_CALL");
        wakeupType = TAF_MNGDPM_VOICE_CALL;
        tafMngdPMSvc::SetModemWakeupSource(wakeupType);
    }
    if((wakeupSource & (1 << 2)) != 0)
    {
        LE_INFO("wakeupType is MCU_VHAL");
        wakeupType = TAF_MNGDPM_MCU_VHAL;
        tafMngdPMSvc::SetModemWakeupSource(wakeupType);
    }
    else if(wakeupSource > 7)
    {
        LE_ERROR("Invalid wakeupType!");
        return LE_BAD_PARAMETER;
    }

    return LE_OK;
}

/**
 * Creates the node wakeupSource reference.
 */
taf_mngdPm_wsRef_t taf_mngdPm_NewNodeWakeupSource( uint8_t pmNodeId,
    taf_mngdPm_WakeupType_t wakeupType, const char* vhalTag)
{
    LE_INFO("taf_mngdPm_NewNodeWakeupSource");
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client");
        return NULL;
    }

    auto &mpms = tafMngdPMSvc::GetInstance();
    bool isWsWhitelisted = false;
    if(wakeupType == TAF_MNGDPM_APP_STAYAWAKE)
    {
        isWsWhitelisted = true;
    }
    else
    {
        for(auto &client : mpms.wsWhiteList)
        {
            if((client.wakeupType == wakeupType) &&
                    (client.sessionRef == taf_mngdPm_GetClientSessionRef()))
            {
                LE_INFO("wakeupType:%d found in wsWhiteList", wakeupType);
                isWsWhitelisted = true;
                break;
            }
        }
    }
    if(isWsWhitelisted)
    {
        isWsWhitelisted = false;

        if(pmNodeId == 1)
        {
            LE_INFO("RPC NewNodeWakeupSource");
            auto &rpcPm = tafMngdRpcPm::GetInstance();
            taf_mngdPm_wsRef_t wsRef = rpcPm.NewRpcNodeWakeupSource(pmNodeId, wakeupType);
            if(wsRef) {
                LE_INFO("NewRpcNodeWakeupSource triggered from MPMS");
                return wsRef;
            }
            else
            {
                LE_ERROR("Failed RPC StayAwakeNode!");
                return NULL;
            }
        }
        taf_nodeWsRefCtx_t * wsCtxPtr =
                (taf_nodeWsRefCtx_t *)le_mem_ForceAlloc(mpms.nodeWsRefPool);
        wsCtxPtr->wsRef = (taf_mngdPm_wsRef_t)le_ref_CreateRef(
                mpms.nodeWsRefMap, wsCtxPtr);
        wsCtxPtr->vhalTag = strdup(vhalTag);
        wsCtxPtr->pmNodeId = pmNodeId;
        wsCtxPtr->wakeupType = wakeupType;
        wsCtxPtr->sessionRef = taf_mngdPm_GetClientSessionRef();
        wsCtxPtr->link = LE_DLS_LINK_INIT;
        wsCtxPtr->isAcquiredLock= false;

        le_dls_Queue(&(mpms.nodeWsRefList), &wsCtxPtr->link);
        return wsCtxPtr->wsRef;
    }
    else {
        LE_ERROR("wakeupType:%d not found in wsWhiteList", wakeupType);
        return NULL;
    }
}

/**
 * Keeps the system awake by acquiring wake lock for the given reference.
 */
le_result_t taf_mngdPm_StayAwakeNode(taf_mngdPm_wsRef_t wsRef)
{
    LE_INFO("taf_mngdPm_StayAwakeNode");
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client");
        return LE_UNSUPPORTED;
    }

    auto &mpms = tafMngdPMSvc::GetInstance();
    le_result_t res = LE_FAULT;
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.nodeWsRefList));
    bool ispresent = false;

    while (linkHandlerPtr)
    {
        taf_nodeWsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_nodeWsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.nodeWsRefList), linkHandlerPtr);

        if (wsRefCtxPtr && wsRef && wsRefCtxPtr->wsRef == wsRef &&
                wsRefCtxPtr->sessionRef == taf_mngdPm_GetClientSessionRef())
        {
            LE_INFO("WakeupType:%d matched with nodeWsRefList for StayAwakeNode", wsRefCtxPtr->wakeupType);
            //check if already a wakelock acquired
            if(wsRefCtxPtr->isAcquiredLock)
            {
                 LE_INFO("WakeLock already acquired");
                 return LE_DUPLICATE;
            }
            res = tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_WAKING_UP);
            if(res != LE_OK)
            {
                return res;
            }
            res = tafMngdPMSvc::AcquireWakeLock();
            if(res == LE_OK)
            {
                LE_INFO("Acquired wakelock");
                wsRefCtxPtr->isAcquiredLock = true;
                ispresent = true;
            }
            break;
        }
    }
    if(ispresent)
    {
        tafMngdPMSvc::ProcessStateChange(TAF_MNGDPM_STATE_WAKING_UP);
        return res;
    }
    else
    {
        LE_INFO("RPC StayAwakeNode");
        auto &rpcPm = tafMngdRpcPm::GetInstance();
        le_result_t res = rpcPm.StayAwakeRpcNode(wsRef);
        if(res == LE_OK) {
            LE_INFO("RPC StayAwakeNode triggered from MPMS");
            return res;
        }
    }
    return res;
}

/**
 * Releases the acquired wake lock for the given reference.
 */
le_result_t taf_mngdPm_RelaxNode(taf_mngdPm_wsRef_t wsRef)
{
    LE_INFO("taf_mngdPm_RelaxNode");
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client");
        return LE_UNSUPPORTED;
    }

    auto &mpms = tafMngdPMSvc::GetInstance();
    le_result_t res = LE_FAULT;
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.nodeWsRefList));
    bool ispresent = false;

    while (linkHandlerPtr)
    {
        taf_nodeWsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_nodeWsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.nodeWsRefList), linkHandlerPtr);

        if (wsRefCtxPtr && wsRef && wsRefCtxPtr->wsRef == wsRef &&
                wsRefCtxPtr->sessionRef == taf_mngdPm_GetClientSessionRef())
        {
            LE_INFO("WakeupType:%d matched with nodeWsRefList for RelaxNode", wsRefCtxPtr->wakeupType);
            if(wsRefCtxPtr->isAcquiredLock)
            {
                res = tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE);
                if(res != LE_OK)
                {
                    return res;
                }
                res = tafMngdPMSvc::ReleaseWakeLock();
                if(res == LE_OK)
                {
                    LE_INFO("client Released WakeLock");
                    wsRefCtxPtr->isAcquiredLock = false;
                    ispresent = true;
                }
            }
            else
            {
                LE_INFO("The wake source is not acquired.");
                return LE_UNAVAILABLE;
            }
            break;
        }
    }
    if(ispresent)
    {
        tafMngdPMSvc::ProcessStateChange(TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE);
        return res;
    }
    else
    {
        LE_INFO("RPC RelaxNode");
        auto &rpcPm = tafMngdRpcPm::GetInstance();
        le_result_t res = rpcPm.RelaxRpcNode(wsRef);
        if(res == LE_OK) {
            LE_INFO("RPC RelaxNode triggered from MPMS");
            return res;
        }
    }
    return LE_FAULT;
}

/**
 * Creates the system wakeupSource reference for a given StayAwake Reason.
 */
taf_mngdPm_wsRef_t taf_mngdPm_CreateWakeupSource (
taf_mngdPm_StayAwakeReason_t reason,
taf_mngdPm_WsOpt_t option,
const char *wsTag
)
{
    LE_INFO("taf_mngdPm_CreateWakeupSource");
    auto &mpms = tafMngdPMSvc::GetInstance();
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        pid_t procId;
        le_msg_SessionRef_t sessionRef = taf_mngdPm_GetClientSessionRef();
        le_msg_GetClientProcessId(sessionRef, &procId);
        LE_ERROR("Invalid client for session:%p, PID:%d ", sessionRef, procId);
        return NULL;
    }

    if (wsTag == NULL)
    {
        LE_ERROR("Invalid request due to null wsTag");
        return NULL;
    }

    if(reason > 4 && reason <= 15)
    {
        LE_ERROR("StayAwakeReason:%d is not supported", reason);
        return NULL;
    }
    LE_INFO("Create wakeup source for wsTag:%s with StayAwakeReason:%d", wsTag, reason);
    taf_wsRefCtx_t * wsCtxPtr =
            (taf_wsRefCtx_t *)le_mem_ForceAlloc(mpms.wsRefPool);
    wsCtxPtr->wsRef = (taf_mngdPm_wsRef_t)le_ref_CreateRef(
            mpms.wsRefMap, wsCtxPtr);
    wsCtxPtr->wsTag = strdup(wsTag);
    wsCtxPtr->reason = reason;
    wsCtxPtr->sessionRef = taf_mngdPm_GetClientSessionRef();
    wsCtxPtr->link = LE_DLS_LINK_INIT;
    wsCtxPtr->wakeSourceState = WAKE_SOURCE_NOT_ACQUIRED;
    wsCtxPtr->option = option;
    le_dls_Queue(&(mpms.wsRefList), &wsCtxPtr->link);
    return wsCtxPtr->wsRef;
}

/**
 * Authorize the StayAwake reason with bit mask.
 *
 */
le_result_t taf_mngdPm_AuthorizeStayAwakeReason (
taf_mngdPm_StayAwakeReasonBitMask_t stayAwakeReasonBitMask
)
{
    LE_INFO("taf_mngdPm_AuthorizeStayAwakeReason %u", stayAwakeReasonBitMask);
    auto &mpms = tafMngdPMSvc::GetInstance();
    mpms.stayAwakeReasonMask.reset();
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is STAY_AWAKE_REASON_BIT_MASK_NORMAL");
        mpms.stayAwakeReasonMask.set(0);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_ECALL_ACTIVE");
        mpms.stayAwakeReasonMask.set(1);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_CALLBACK)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_ECALL_CALLBACK");
        mpms.stayAwakeReasonMask.set(2);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_SW_UPDATE)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_SW_UPDATE");
        mpms.stayAwakeReasonMask.set(3);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VEH_NETWORK)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VEH_NETWORK");
        mpms.stayAwakeReasonMask.set(4);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_1)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_1");
        mpms.stayAwakeReasonMask.set(16);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_2)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_2");
        mpms.stayAwakeReasonMask.set(17);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_3)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_3");
        mpms.stayAwakeReasonMask.set(18);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_4)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_4");
        mpms.stayAwakeReasonMask.set(19);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_5)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_5");
        mpms.stayAwakeReasonMask.set(20);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_6)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_6");
        mpms.stayAwakeReasonMask.set(21);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_7)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_7");
        mpms.stayAwakeReasonMask.set(22);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_8)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_1");
        mpms.stayAwakeReasonMask.set(23);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_9)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_9");
        mpms.stayAwakeReasonMask.set(24);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_10)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_10");
        mpms.stayAwakeReasonMask.set(25);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_11)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_11");
        mpms.stayAwakeReasonMask.set(26);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_12)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_12");
        mpms.stayAwakeReasonMask.set(27);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_13)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_1");
        mpms.stayAwakeReasonMask.set(28);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_14)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_1");
        mpms.stayAwakeReasonMask.set(29);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_15)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_15");
        mpms.stayAwakeReasonMask.set(30);
    }
    if((stayAwakeReasonBitMask & (1 << TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_16)) != 0)
    {
        LE_INFO("stayAwakeReasonBitMask is TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VENDOR_16");
        mpms.stayAwakeReasonMask.set(31);
    }
    mpms.RefreshWakeSources();
    return LE_OK;
}

/**
 * Keeps the system awake by acquiring wake lock for the given reference.
 */
le_result_t taf_mngdPm_StayAwake(taf_mngdPm_wsRef_t wsRef)
{
    LE_INFO("taf_mngdPm_StayAwake");
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client");
        return LE_UNSUPPORTED;
    }

    auto &mpms = tafMngdPMSvc::GetInstance();
    le_result_t res = LE_FAULT;
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.wsRefList));
    while (linkHandlerPtr)
    {
        taf_wsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_wsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.wsRefList), linkHandlerPtr);

        if (wsRefCtxPtr && wsRef && wsRefCtxPtr->wsRef == wsRef &&
                wsRefCtxPtr->sessionRef == taf_mngdPm_GetClientSessionRef())
        {
            LE_INFO("wsRef is valid in wsRefList for %s with StayAwakeReason:%d, WsState:%d ",wsRefCtxPtr->wsTag, wsRefCtxPtr->reason, wsRefCtxPtr->wakeSourceState);
            //check if already a wakelock acquired
            if(wsRefCtxPtr->wakeSourceState)
            {
                 LE_INFO("WakeLock is already acquired for %s with StayAwakeReason:%d, WsState:%d", wsRefCtxPtr->wsTag, wsRefCtxPtr->reason,wsRefCtxPtr->wakeSourceState);
                 return LE_DUPLICATE;
            }

            if(mpms.stateMachine.currentState == TAF_MNGDPM_STATE_SUSPENDING && (mpms.IsAuthorizedStayAwakeReason(wsRefCtxPtr->reason)))
            {
                LE_INFO("Cache system awake request for %s and wsReason:%d in suspending state", wsRefCtxPtr->wsTag, wsRefCtxPtr->reason);
                mpms.wsCachedReqsRefSet.insert(wsRefCtxPtr->wsRef);

                return LE_OK;
            }

            if(mpms.IsAuthorizedStayAwakeReason(wsRefCtxPtr->reason))
            {
                LE_INFO("stayAwakeReason:%d is in authorized stayAwakeReasonList", wsRefCtxPtr->reason);
                res = mpms.AcquireWakeSource(wsRefCtxPtr);
            }
            else
            {
                LE_INFO("Non authorized StayAwakeReason for stayawake");
                if (mpms.stateMachine.currentState == TAF_MNGDPM_STATE_SUSPEND || mpms.stateMachine.currentState == TAF_MNGDPM_STATE_SUSPENDING)
                {
                    wsRefCtxPtr->wakeSourceState = WAKE_SOURCE_NOT_ACQUIRED;
                    LE_ERROR("StayAwake LE_NOT_PERMITTED: unauthorized Wake Source State: %d, current system state: %d",wsRefCtxPtr->wakeSourceState, mpms.stateMachine.currentState);
                    return LE_NOT_PERMITTED;
                }
                else
                {
                    wsRefCtxPtr->wakeSourceState = WAKE_SOURCE_IGNORED;
                    LE_INFO("StayAwake: unauthorized Wake Source State: %d, current system state: %d",wsRefCtxPtr->wakeSourceState, mpms.stateMachine.currentState);
                    return LE_OK;
                }
            }
            break;
        }
    }
    return res;
}

/**
 * Releases the system acquired wake lock for the given reference.
 */
le_result_t taf_mngdPm_Relax(taf_mngdPm_wsRef_t wsRef)
{
    LE_INFO("taf_mngdPm_Relax");
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client");
        return LE_UNSUPPORTED;
    }

    auto &mpms = tafMngdPMSvc::GetInstance();
    le_result_t res = LE_FAULT;
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.wsRefList));
    while (linkHandlerPtr)
    {
        taf_wsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_wsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.wsRefList), linkHandlerPtr);

        if (wsRefCtxPtr && wsRef && wsRefCtxPtr->wsRef == wsRef &&
                wsRefCtxPtr->sessionRef == taf_mngdPm_GetClientSessionRef())
        {
            LE_INFO("wsRef is valid in wsRefList for %s with StayAwakeReason:%d, WsState:%d ",wsRefCtxPtr->wsTag, wsRefCtxPtr->reason, wsRefCtxPtr->wakeSourceState);
            if(wsRefCtxPtr->wakeSourceState)
            {
                if(mpms.IsAuthorizedStayAwakeReason(wsRefCtxPtr->reason))
                {
                    LE_INFO("Authorized StayAwakeReason:%d for relax, releasing the wakeup source!", wsRefCtxPtr->reason);
                    res = mpms.ReleaseWakeSource(wsRefCtxPtr);
                }
                else
                {
                    if(wsRefCtxPtr->wakeSourceState == WAKE_SOURCE_IGNORED)
                    {
                        LE_INFO("Unauthorized StayAwakeReason:%d for relax, wake source state set from WAKE_SOURCE_IGNORED to WAKE_SOURCE_NOT_ACQUIRED ", wsRefCtxPtr->reason);
                        wsRefCtxPtr->wakeSourceState = WAKE_SOURCE_NOT_ACQUIRED;
                        LE_INFO("Relax: unauthorized Wake Source State: %d, current system state: %d",wsRefCtxPtr->wakeSourceState, mpms.stateMachine.currentState);
                    }
                    return LE_OK;
                }
            }
            else
            {
                LE_ERROR("The wake source for %s is not acquired for satyawakereason:%d.", wsRefCtxPtr->wsTag, wsRefCtxPtr->reason);
                return LE_UNAVAILABLE;
            }
            break;
        }
    }
    return res;
}

/**
 * Initiates the forceful restart for the given node.
 */
le_result_t taf_mngdPm_ShutdownNode (uint8_t pmNodeId)
{
    LE_DEBUG("taf_mngdPm_ShutdownNode for pmNodeId : %d", pmNodeId);
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client for the sessionRef:%p", taf_mngdPm_GetClientSessionRef());
        return LE_UNSUPPORTED;
    }

    auto &mpms = tafMngdPMSvc::GetInstance();
    if(pmNodeId == 1)
    {
        auto &rpcPm = tafMngdRpcPm::GetInstance();
        le_result_t res = rpcPm.ShutdownRpcNAD();
        if(res == LE_OK)
        {
            LE_INFO("RPC ShutdownNAD is successful");
        }
        return res;
    }

    le_result_t res = mpms.ShutdownNAD();
    if(res == LE_OK)
    {
        LE_INFO("ShutdownNAD is successful");
        mpms.powerMode.isShutDown = true;
    }
    return res;
}

/**
 * Initiates the forceful shutdown for the given node.
 */
le_result_t taf_mngdPm_RestartNode (uint8_t pmNodeId)
{
    LE_INFO("taf_mngdPm_RestartNode for pmNodeId : %d", pmNodeId);
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client for the sessionRef:%p", taf_mngdPm_GetClientSessionRef());
        return LE_UNSUPPORTED;
    }

    le_result_t res = LE_FAULT;
    if(pmNodeId == 1)
    {
        auto &rpcPm = tafMngdRpcPm::GetInstance();
        res = rpcPm.RestartRpcNAD();
        if(res == LE_OK)
        {
            LE_INFO("RPC RestartRpcNAD is successful");
        }
        return res;
    }

    auto &mpms = tafMngdPMSvc::GetInstance();
    res = mpms.RestartNAD();
    if(res == LE_OK)
    {
        LE_INFO("RestartNAD is successful");
        mpms.powerMode.isRestart = true;
    }

    return res;
}

/**
 * Adds the client to StateChangeHandler.
 */
taf_mngdPm_StateChangeHandlerRef_t taf_mngdPm_AddStateChangeHandler
(
    taf_mngdPm_StateChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    LE_INFO("taf_mngdPm_AddStateChangeHandler");
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client for the sessionRef:%p", taf_mngdPm_GetClientSessionRef());
        return NULL;
    }

    auto &mpms = tafMngdPMSvc::GetInstance();
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("MPMSStateChangeHandler",
        mpms.stateChange, tafMngdPMSvc::StateLayeredHandler, (void*)handlerFuncPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);
    return (taf_mngdPm_StateChangeHandlerRef_t)handlerRef;
}

/**
 * Removes the client from StateChangeHandler.
 */
void taf_mngdPm_RemoveStateChangeHandler(taf_mngdPm_StateChangeHandlerRef_t handlerRef)
{
    LE_INFO("taf_mngdPm_RemoveStateChangeHandler");

    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

/**
 * Adds the client to Info Report Handler.
 */
taf_mngdPm_InfoReportHandlerRef_t taf_mngdPm_AddInfoReportHandler(taf_mngdPm_InfoReportBitMask_t infoReportMask,
        taf_mngdPm_InfoReportHandlerFunc_t handlerPtr, void* contextPtr)
{
    LE_DEBUG("AddInfoReportHandler");
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client for the sessionRef:%p", taf_mngdPm_GetClientSessionRef());
        return NULL;
    }

    auto &mpms = tafMngdPMSvc::GetInstance();
    if((!mpms.pmInf) || (mpms.pmInf->addBubStatusHandler == NULL) )
    {
        LE_INFO("Ignore the AddInfoReportHandler when no VHAL present");
        return NULL;
    }
    else if((infoReportMask & (INFO_REPORT_MASK_BUB)) == 0)     //Need to store the bitmask of each client in future
    {
        LE_INFO("Ignore the AddInfoReportHandler for invalid bitmask");
        return NULL;
    }
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "INVALID handler reference.");
    taf_mngdPm_InfoReportCb_t * handlerCtxPtr =
            (taf_mngdPm_InfoReportCb_t *)le_mem_ForceAlloc(mpms.infoReportHandlerPool);
    handlerCtxPtr->handlerPtr = handlerPtr;
    handlerCtxPtr->handlerRef = (taf_mngdPm_InfoReportHandlerRef_t)le_ref_CreateRef(
            mpms.infoReportHandlerRefMap, handlerCtxPtr);
    handlerCtxPtr->link = LE_DLS_LINK_INIT;
    handlerCtxPtr->infoReportHandlerCtxPtr = contextPtr;
    handlerCtxPtr->sessionRef = taf_mngdPm_GetClientSessionRef();
    le_dls_Queue((&(mpms.infoReportHandlerList)), &handlerCtxPtr->link);

    LE_INFO("Send addBubStatusHandler request to VHAL");
    (*(mpms.pmInf->addBubStatusHandler))(mpms.InfoReportVhalCB);

    return handlerCtxPtr->handlerRef;
}

/**
 * Removes Info Report handler
 */
void taf_mngdPm_RemoveInfoReportHandler(taf_mngdPm_InfoReportHandlerRef_t handlerRef)
{
    LE_INFO("RemoveInfoReportHandler");
    auto &mpms = tafMngdPMSvc::GetInstance();
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.infoReportHandlerList));
    while (linkHandlerPtr)
    {
        taf_mngdPm_InfoReportCb_t * handlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_mngdPm_InfoReportCb_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.infoReportHandlerList), linkHandlerPtr);
        if (handlerCtxPtr && handlerCtxPtr->handlerRef == handlerRef)
        {
            le_ref_DeleteRef(mpms.infoReportHandlerRefMap, handlerRef);
            le_dls_Remove(&(mpms.infoReportHandlerList), &handlerCtxPtr->link);
            le_mem_Release((void*)handlerCtxPtr);
        }
    }
}

taf_mngdPm_NodePowerStateChangeHandlerRef_t taf_mngdPm_AddNodePowerStateChangeHandler
(
    taf_mngdPm_NodePowerStateChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr,
    uint8_t pmNodeId,
    taf_mngdPm_NodePowerStateChangeBitMask_t stateMask
)
{
    LE_INFO("taf_mngdPm_AddNodePowerStateChangeHandler");
    auto &mpms = tafMngdPMSvc::GetInstance();
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client for the sessionRef:%p", taf_mngdPm_GetClientSessionRef());
        return NULL;
    }

    pid_t procId;
    le_msg_SessionRef_t sessionRef = taf_mngdPm_GetClientSessionRef();
    le_msg_GetClientProcessId(sessionRef, &procId);
    LE_INFO("Registered client for the sessionRef:%p with PID:%d, NodeID:%d, stateMask:%d", sessionRef,
        procId,pmNodeId, stateMask);

    if(pmNodeId == 1)
    {
        auto &rpcPm = tafMngdRpcPm::GetInstance();
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref =
             rpcPm.AddRpcNodePowerStateChangeHandler(handlerFuncPtr, contextPtr, pmNodeId, stateMask);
        return ref;
    }
    //save handlerref according to bitmask
    TAF_ERROR_IF_RET_VAL(handlerFuncPtr == NULL, NULL, "INVALID handler reference.");
    taf_mngdPm_NodePowerStateCtxt_t * handlerCtxPtr =
            (taf_mngdPm_NodePowerStateCtxt_t *)le_mem_ForceAlloc(mpms.nodePowerStateHandlerPool);
    handlerCtxPtr->handlerPtr = handlerFuncPtr;
    handlerCtxPtr->sessionRef = taf_mngdPm_GetClientSessionRef();
    handlerCtxPtr->pmNodeId = pmNodeId;
    handlerCtxPtr->powerStateMask = stateMask;
    handlerCtxPtr->handlerRef = (taf_mngdPm_NodePowerStateChangeHandlerRef_t)le_ref_CreateRef(
            mpms.nodePowerStateHandlerMap, handlerCtxPtr);
    handlerCtxPtr->link = LE_DLS_LINK_INIT;
    handlerCtxPtr->nodePowerStateHandlerCtxPtr = contextPtr;

    le_dls_Queue((&(mpms.nodePowerStateHandlerList)), &handlerCtxPtr->link);

    // Immediately notify this client with the current node state (if interested).
    if (mpms.IsConfiguredBitMask(tafMngdPMSvc::currentNodePowerState, stateMask))
    {
        LE_INFO("Immediate notify client with current node power state: %d",
                tafMngdPMSvc::currentNodePowerState);

        // Create a short-lived nodeStateRef; no PMS ack timer involved for immediate notify
        taf_NodePowerStateRef_t* immediateNodePwStatePtr =
            (taf_NodePowerStateRef_t*)le_mem_ForceAlloc(mpms.nodePowerStateRefPool);
        immediateNodePwStatePtr->nodeStateRef =
            (taf_mngdPm_nodePowerStateRef_t)le_ref_CreateRef(mpms.nodePowerStateRefMap, immediateNodePwStatePtr);

        handlerCtxPtr->initialNodePowerState.nodeStateRef = immediateNodePwStatePtr->nodeStateRef;
        handlerCtxPtr->initialNodePowerState.sessionRef   = handlerCtxPtr->sessionRef;
        handlerCtxPtr->initialNodePowerState.state        = tafMngdPMSvc::currentNodePowerState;
        handlerCtxPtr->initialNodePowerState.isAcked      = false;


        // Call the client handler directly
        handlerCtxPtr->handlerPtr(
            handlerCtxPtr->pmNodeId,
            immediateNodePwStatePtr->nodeStateRef,
            handlerCtxPtr->initialNodePowerState.state,
            handlerCtxPtr->nodePowerStateHandlerCtxPtr);
    }

    return (taf_mngdPm_NodePowerStateChangeHandlerRef_t)handlerCtxPtr->handlerRef;
}

/**
 * Removes the client from NodePowerStateChangeHandler.
 */
void taf_mngdPm_RemoveNodePowerStateChangeHandler(taf_mngdPm_NodePowerStateChangeHandlerRef_t handlerRef)
{
    auto &mpms = tafMngdPMSvc::GetInstance();
    pid_t procId;
    le_msg_SessionRef_t sessionRef = taf_mngdPm_GetClientSessionRef();
    le_msg_GetClientProcessId(sessionRef, &procId);
    LE_INFO("RemoveNodePowerStateHandler with the sessionRef:%p, PID:%d", sessionRef, procId);

    bool isPmsNodeChangeRef = false;
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.nodePowerStateHandlerList));
    while (linkHandlerPtr)
    {
        taf_mngdPm_NodePowerStateCtxt_t * handlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_mngdPm_NodePowerStateCtxt_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.nodePowerStateHandlerList), linkHandlerPtr);
        if (handlerCtxPtr && handlerCtxPtr->handlerRef == handlerRef)
        {
            // Release the per-handler immediate-notify node state ref, if any
            if (handlerCtxPtr->initialNodePowerState.nodeStateRef)
            {
                taf_NodePowerStateRef_t* nodeRefPtr =
                    (taf_NodePowerStateRef_t*) le_ref_Lookup(
                        mpms.nodePowerStateRefMap,
                        handlerCtxPtr->initialNodePowerState.nodeStateRef);

                if (nodeRefPtr)
                {
                    LE_INFO("Releasing initialNodePowerState ref %p for sessionRef %p",
                        handlerCtxPtr->initialNodePowerState.nodeStateRef,
                        handlerCtxPtr->initialNodePowerState.sessionRef);
                    le_ref_DeleteRef(
                        mpms.nodePowerStateRefMap,
                        handlerCtxPtr->initialNodePowerState.nodeStateRef);
                    le_mem_Release(nodeRefPtr);
                }

                handlerCtxPtr->initialNodePowerState.nodeStateRef = NULL;
                handlerCtxPtr->initialNodePowerState.isAcked = false;
            }

            le_ref_DeleteRef(mpms.nodePowerStateHandlerMap, handlerRef);
            le_dls_Remove(&(mpms.nodePowerStateHandlerList), &handlerCtxPtr->link);
            le_mem_Release((void*)handlerCtxPtr);
            isPmsNodeChangeRef = true;
        }
    }
    if(!isPmsNodeChangeRef)
    {
        auto &rpcPm = tafMngdRpcPm::GetInstance();
        rpcPm.RemoveRpcNodePowerStateChangeHandler(handlerRef);
        return;
    }
}

/**
 * Gets the Info Report.
 */
le_result_t taf_mngdPm_GetInfoReport(taf_mngdPm_InfoDataId_t infoReportId, int32_t* report)
{
    LE_INFO("taf_mngdPm_GetInfoReport");
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client for the sessionRef:%p", taf_mngdPm_GetClientSessionRef());
        return LE_UNSUPPORTED;
    }

    le_result_t res = LE_FAULT ;
    auto &mpms = tafMngdPMSvc::GetInstance();
    if((!mpms.pmInf) || (mpms.pmInf->getBubStatus == NULL))
    {
        LE_INFO("Ignore the GetInfoReport when no VHAL present");
        return LE_FAULT;
    }
    if(infoReportId == TAF_MNGDPM_INFO_REPORT_BUB)
    {
        LE_INFO("Send getBubStatus request to VHAL");
        res = (*(mpms.pmInf->getBubStatus))(report);
    }
    else
    {
        return LE_BAD_PARAMETER;
    }
    return res;
}

/**
 * SendNodePowerStateChangeAck for the given node.
 */
le_result_t taf_mngdPm_SendNodePowerStateChangeAck (uint8_t pmNodeId,
        taf_mngdPm_nodePowerStateRef_t Ref, taf_mngdPm_NodeClientAck_t ack)
{
    LE_INFO("taf_mngdPm_SendNodePowerStateChangeAck");
    auto &mpms = tafMngdPMSvc::GetInstance();

    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client for the sessionRef:%p", taf_mngdPm_GetClientSessionRef());
        return LE_UNSUPPORTED;
    }
    //Getting the current client data from mngdPmClientInfo
    taf_mngdPm_SessionNode_t* sessionNodePtr;
    sessionNodePtr = mpms.To_taf_mngdPm_SessionNode_t(le_hashmap_Get(mpms.mngdPmClientInfo.clients,
            taf_mngdPm_GetClientSessionRef()));

    if(pmNodeId == 1)
    {
        auto &rpcPm = tafMngdRpcPm::GetInstance();
        le_result_t res = rpcPm.SendRpcNodePowerStateChangeAck(pmNodeId, Ref, ack);
        return res;
    }

    taf_mngdPm_NodePowerState_t state = TAF_MNGDPM_NODE_STATE_RESUME;

    // Iterate handler list and match input Ref
    // against handlerCtxPtr->initialNodePowerState.nodeStateRef.
    // If it matches, set the per-handler flag to true; else continue.
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.nodePowerStateHandlerList));
    while (linkHandlerPtr)
    {
        taf_mngdPm_NodePowerStateCtxt_t * handlerCtxPtr =
            CONTAINER_OF(linkHandlerPtr, taf_mngdPm_NodePowerStateCtxt_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.nodePowerStateHandlerList), linkHandlerPtr);

        if (!handlerCtxPtr)
            continue;

        if (handlerCtxPtr->initialNodePowerState.nodeStateRef == Ref)
        {
            handlerCtxPtr->initialNodePowerState.isAcked = true;
            return LE_OK;
        }
    }

    // validate client record existed in state change registered clients
    bool isClientPresent = false;
    for (const auto &client : mpms.regClientrecrd) {
        if (((client.nodeStateRef == (taf_mngdPm_nodePowerStateRef_t)Ref)) &&
                    (client.sessionRef == taf_mngdPm_GetClientSessionRef())) {
            LE_INFO("Client with sessionRef:%p found in record", taf_mngdPm_GetClientSessionRef());
            state = client.state;
            isClientPresent = true;
            break;
        }
    }
    if (!isClientPresent) {
        LE_ERROR("Client with sessionRef:%p not found in the regClientrecrd", taf_mngdPm_GetClientSessionRef());
        return LE_FAULT;
    }

    if(mpms.IsSameAsCurrentState(state, mpms.stateMachine.currentState) && sessionNodePtr)
    {
        if(!(le_timer_IsRunning(mpms.stateChangeAckTimerRef)))
        {
            LE_ERROR("State change Ack timer expired for current state");
            return LE_OK;
        }
        if(ack == TAF_MNGDPM_CLIENT_NOT_READY)
        {
            LE_INFO("Received NACK from client %s", sessionNodePtr->name);
            mpms.SendAckToPms(state, TAF_PM_NOT_READY);
            return LE_OK;
        }
        else
        {
            LE_INFO("Received ACK from client %s", sessionNodePtr->name);
            //mark the client as acknowledged for state change
            for(auto it = mpms.regClientrecrd.begin(); it != mpms.regClientrecrd.end(); it++)
            {
                if(it->sessionRef == taf_mngdPm_GetClientSessionRef())
                {
                    LE_INFO("Client with sessionRef %p", it->sessionRef);
                    it->isAcked = true;
                    break;
                }
            }
            mpms.ackClientrecrdSize++;
            LE_INFO("regClientrecrd size is %zu ,ackClientrecrd size is:%d", mpms.regClientrecrd.size(),
                    mpms.ackClientrecrdSize);
            //If Last acknowledged client , proceed for ack state change
            if((int8_t)mpms.regClientrecrd.size() == mpms.ackClientrecrdSize)
            {
                if(le_timer_IsRunning(mpms.stateChangeAckTimerRef))
                {
                    LE_INFO("Stop the stateChangeAckTimer");
                    le_timer_Stop(mpms.stateChangeAckTimerRef);
                }
                mpms.clientSize = 0;
                mpms.ackClientrecrdSize = 0;
                mpms.SendAckToPms(state, TAF_PM_READY);
                return LE_OK;
            }
            else
            {
                LE_INFO("All clients not acknowledged for state change yet");
                return LE_OK;
            }
        }
    }
    else
    {
        LE_ERROR("Client Ack response not sent for current transition");
    }
    return LE_FAULT;
}

/**
 * Sets the whitelisted modem wakeup selection.
 */
le_result_t taf_mngdPm_SetNodeModemWakeupSel
(
    uint8_t pmNodeId,
    taf_mngdPm_NodeModemWsBitMask_t wsBitmask
)
{
    le_result_t res = LE_FAULT;

    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return LE_UNSUPPORTED;
    }

    if(pmNodeId == 1)
    {
        res = taf_rpcPm_SetModemWakeupSel(wsBitmask);
    }
    else
    {
        res = taf_pm_SetModemWakeupSel(wsBitmask);
    }

    LE_DEBUG("%s: lowlever return: %s",
             __FUNCTION__,
             LE_RESULT_TXT(res));

    return res;
}

/**
 * Gets the whitelisted modem wakeup selection.
 */
le_result_t taf_mngdPm_GetNodeModemWakeupSel
(
    uint8_t pmNodeId,
    taf_mngdPm_NodeModemWsBitMask_t* wsBitmaskPtr
        ///< [OUT] Modem wakeup selection to be whitelisted.
)
{
    le_result_t res = LE_FAULT;

    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return LE_UNSUPPORTED;
    }

    if(pmNodeId == 1)
    {
        res = taf_rpcPm_GetModemWakeupSel(wsBitmaskPtr);
    }
    else
    {
        res = taf_pm_GetModemWakeupSel(wsBitmaskPtr);
    }

    LE_DEBUG("%s: lowlever return: %s",
             __FUNCTION__,
             LE_RESULT_TXT(res));

    return res;
}

/**
 * Gets the last modem wakeup reason.
 */
le_result_t taf_mngdPm_GetNodeModemAwakeReason
(
    uint8_t pmNodeId,
    taf_mngdPm_NodeModemWsBitMask_t* wsBitmaskPtr
        ///< [OUT] Modem wakeup reason.
)
{
    le_result_t res = LE_FAULT;

    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return LE_UNSUPPORTED;
    }

    if(pmNodeId == 1)
    {
        res = taf_rpcPm_GetModemAwakeReason(wsBitmaskPtr);
    }
    else
    {
        res = taf_pm_GetModemAwakeReason(wsBitmaskPtr);
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_mngdPm_NodeModemAwake'
 *
 * Node modem awake event.
 *
 * @instaging
 */
//--------------------------------------------------------------------------------------------------
taf_mngdPm_NodeModemAwakeHandlerRef_t taf_mngdPm_AddNodeModemAwakeHandler
(
    taf_mngdPm_NodeModemAwakeHandlerFunc_t handlerPtr,
        ///< [IN] The modem awake event handler.
    void* contextPtr,
        ///< [IN]
    uint8_t pmNodeId,
        ///< [IN]
    taf_mngdPm_NodeModemWsBitMask_t wsBitmask
        ///< [IN]
)
{
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client");
        return NULL;
    }

    LE_INFO("AddNodeModemAwakeHandler [add]");

    auto & power = tafMngdPMSvc::GetInstance();

    // Dynamically register the handlers
    if (pmNodeId == 1)
    {
        if (power.enableRemote == false)
        {
            LE_DEBUG("-> EnableRemoteOnce");
            power.EnableRemoteOnce();
        }
    }
    else
    {
        if (power.enableLocal == false)
        {
            LE_DEBUG("-> EnableLocalOnce");
            power.EnableLocalOnce();
        }
    }

    CallbackHandlerCombo_t *combo =
        (CallbackHandlerCombo_t *) le_mem_ForceAlloc(power.cbHandlerPool);

    combo->callback = handlerPtr;
    combo->context = contextPtr;
    combo->nodeId = pmNodeId;
    combo->bitset = wsBitmask;

    if (pmNodeId == 1)
    {
        // Add 'combo' to remote-cb-map
        combo->ref =
            (taf_mngdPm_NodeModemAwakeHandlerRef_t)
                le_ref_CreateRef(power.cbRemoteMap, combo);
        LE_DEBUG("Add callback to remote-map [Add]");
    }
    else
    {
        // Add 'combo' to local-cb-map
        combo->ref =
            (taf_mngdPm_NodeModemAwakeHandlerRef_t)
                le_ref_CreateRef(power.cbLocalMap, combo);
        LE_DEBUG("Add callback to local-map [Add]");
    }

    return combo->ref;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_mngdPm_NodeModemAwake'
 */
//--------------------------------------------------------------------------------------------------
void taf_mngdPm_RemoveNodeModemAwakeHandler
(
    taf_mngdPm_NodeModemAwakeHandlerRef_t handlerRef
        ///< [IN]
)
{
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client");
        return;
    }

    auto & power = tafMngdPMSvc::GetInstance();

    CallbackHandlerCombo_t * combo = NULL;

    // Go through the local-map then remote-map
    combo = (CallbackHandlerCombo_t *)
                le_ref_Lookup(power.cbLocalMap, handlerRef);

    if (combo)
    {
        LE_INFO("Found ref in local-map [remove]");
        le_ref_DeleteRef(power.cbLocalMap, handlerRef);
        le_mem_Release(combo);
        return; // Fine, stop
    }

    combo = (CallbackHandlerCombo_t *)
                le_ref_Lookup(power.cbRemoteMap, handlerRef);

    if (combo)
    {
        LE_INFO("Found ref in remote-map [remove]");
        le_ref_DeleteRef(power.cbRemoteMap, handlerRef);
        le_mem_Release(combo);
        return; // Hit, return
    }

    LE_WARN("Removing the non-existing handler ref");
}

/**
 * Deletes the given wake source.
 */
le_result_t taf_mngdPm_DeleteWakeupSource(taf_mngdPm_wsRef_t wsRef)
{
    LE_INFO("taf_mngdPm_DeleteWakeupSource");

    if(tafMngdPMSvc::IsClientValid() == false)
    {
        LE_ERROR("Invalid client");
        return LE_UNSUPPORTED;
    }

    auto &mpms = tafMngdPMSvc::GetInstance();
    le_result_t res = LE_NOT_FOUND;
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.wsRefList));

    while (linkHandlerPtr)
    {
        taf_wsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_wsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.wsRefList), linkHandlerPtr);

        if (wsRefCtxPtr && wsRef && wsRefCtxPtr->wsRef == wsRef &&
                wsRefCtxPtr->sessionRef == taf_mngdPm_GetClientSessionRef())
        {
            LE_INFO("wsRef found in wsRefList for %s with sessionRef: %p, stayAwakeReason:%d, wsState:%d", wsRefCtxPtr->wsTag,
                wsRefCtxPtr->sessionRef, wsRefCtxPtr->reason, wsRefCtxPtr->wakeSourceState);
            if(wsRefCtxPtr->wakeSourceState == WAKE_SOURCE_NOT_ACQUIRED) {

                LE_INFO("Delete not acquired wakesource of wsTag:%s for client with sessionRef %p", wsRefCtxPtr->wsTag, wsRefCtxPtr->sessionRef);
                le_ref_DeleteRef(mpms.wsRefMap, wsRefCtxPtr->wsRef);
                le_dls_Remove(&(mpms.wsRefList), &wsRefCtxPtr->link);
                free((void*)wsRefCtxPtr->wsTag);
                le_mem_Release((void*)wsRefCtxPtr);
                LE_INFO("Deletion Complete!!!");
                return LE_OK;
            }
            else if(wsRefCtxPtr->wakeSourceState == WAKE_SOURCE_IGNORED ||
                wsRefCtxPtr->wakeSourceState == WAKE_SOURCE_ACQUIRED)
            {
                LE_WARN("WS of wsTag:%s for client with sessionRef %p is already in use, deletion not permitted", wsRefCtxPtr->wsTag, wsRefCtxPtr->sessionRef);
                return LE_NOT_PERMITTED;
            }
            break;
        }
    }
    LE_INFO("wsRef not found in wsRefList for the sessionRef: %p", taf_mngdPm_GetClientSessionRef());
    return res;
}

COMPONENT_INIT
{
    LE_INFO("tafMngdPMSvc COMPONENT init...");

    // Enable bit0 in watchdog chain.
    le_wdogChain_Init(1);

    // Start watchdog 0 and kick bit0 of watchdog chain in main thread.
    le_clk_Time_t watchdogInterval = { .sec = MAIN_THREAD_KICK_INTERVAL };
    le_wdogChain_MonitorEventLoop(MONITOR_MAIN_THREAD_LOOP, watchdogInterval);
    LE_INFO("Watchdog for main thread is started.");

    auto &mpms = tafMngdPMSvc::GetInstance();

    mpms.Init();

    // Initialize new internal event ID and handler for thread-safe processing of NodeEventCB
    mpms.nodeInternalEvent = le_event_CreateId("NodeInternalEvent", sizeof(taf_mngdPm_NodeEventData_t));
    le_event_AddHandler("NodeInternalEventHandler", mpms.nodeInternalEvent, mpms.NodeInternalEventHandler);

    //Init RPC
    auto &rpcPm = tafMngdRpcPm::GetInstance();

    rpcPm.Init();

    mpms.mngdPmClientInfo.SessionNodePool = le_mem_InitStaticPool(SessionCtx,
                                        MAX_SESSION,
                                        sizeof(taf_mngdPm_SessionNode_t));

    // Create table of clients
    mpms.mngdPmClientInfo.clients = le_hashmap_Create("tafMngdPMClient", MAX_SESSION,
                                             le_hashmap_HashVoidPointer,
                                             le_hashmap_EqualsVoidPointer);

    if (NULL == mpms.mngdPmClientInfo.clients)
    {
        LE_FATAL("Failed to create client hashmap");
    }

    mpms.stateChange = le_event_CreateId("stateChange", sizeof(taf_mngdPm_StateInd_t));

    mpms.nodePowerStateChange = le_event_CreateId("nodePowerStateChange", sizeof(taf_mngdPm_NodePowerStateChange_t));
    le_event_AddHandler("tafNodePowerStateChange event", mpms.nodePowerStateChange, mpms.NodePowerStateChanged);
    mpms.nodePowerStateRefPool = le_mem_CreatePool("nodePowerStateRef", sizeof(taf_NodePowerStateRef_t));
    mpms.nodePowerStateHandlerMap = le_ref_CreateMap("nodePowerStateHandlerMap", TAF_REF_POOL_SIZE);
    mpms.nodePowerStateRefMap = le_ref_CreateMap("nodePowerStateRefMap", TAF_REF_POOL_SIZE);
    mpms.nodePowerStateHandlerPool = le_mem_CreatePool("nodePowerStateHandlerList",
        sizeof(taf_mngdPm_NodePowerStateCtxt_t));
    mpms.nodePowerStateHandlerList = LE_DLS_LIST_INIT;
    try
    {
        le_result_t res = tafMngdPMSvc::ParseJsonConfiguration(TAF_MNGDPM_CONFIGURATION_PATH);
        if (res == LE_OK)
        {
            LE_INFO("Successfully parsed the JSON");
        }
        else
        {
            LE_ERROR("Failed to parse the JSON");
        }
    }
    catch (const std::exception &e)
    {
        LE_ERROR("Exception while parsing the ParseJsonConfiguration");
    }

    mpms.pmEvtReady = le_event_CreateId("readyEvt", sizeof(taf_mngdPm_readyEvtType_t));
    le_event_AddHandler("readyEvtHdlr", mpms.pmEvtReady, mpms.PMVhalReadyEvtHandler);

    //PMVHAL
    if(mpms.config.hal_enabled)
    {
        // Load PMVHAL module.
        le_result_t res = tafMngdPMSvc::InitVHalModule();
        if(res != LE_OK) {
            // Retry loading PMVHAL module when failed.
            le_event_QueueFunction(mpms.GetPmVhalReady, NULL, NULL);
        }
    }
    else {
        // Advertise MngdPMSvc in case hal_enabled is set to false
        LE_INFO("Start MngdPM service without PMVHAL module");
        taf_mngdPm_AdvertiseService();

        mpms.WaitWakeSourceTimer();

        // Set session open handler
        le_msg_AddServiceOpenHandler(taf_mngdPm_GetServiceRef(), tafMngdPMSvc::OnClientConnection,
            NULL);
        // Set session close handler
        le_msg_AddServiceCloseHandler(taf_mngdPm_GetServiceRef(), tafMngdPMSvc::OnClientDisconnection,
            NULL);

        // Set session close handler  for RPCPM
        le_msg_AddServiceCloseHandler(taf_mngdPm_GetServiceRef(), tafMngdRpcPm::OnClientDisconnection,
            NULL);
    }

    mpms.wsRefPool = le_mem_CreatePool("tafwsRefList", sizeof(taf_wsRefCtx_t));
    mpms.wsRefList = LE_DLS_LIST_INIT;
    mpms.wsRefMap = le_ref_CreateMap("tafwsRef", TAF_REF_POOL_SIZE);

    mpms.nodeWsRefPool = le_mem_CreatePool("tafnodeWsRefList", sizeof(taf_nodeWsRefCtx_t));
    mpms.nodeWsRefList = LE_DLS_LIST_INIT;
    mpms.nodeWsRefMap = le_ref_CreateMap("tafnodeWsRef", TAF_REF_POOL_SIZE);

    mpms.vmStatePool = le_mem_CreatePool("VMStatePool", sizeof(taf_mngdPm_vmState_t));
    mpms.vmStateHashmap = le_hashmap_Create("VMStateHashMap", TAF_MNGDPM_VM_HASH_SIZE,
            le_hashmap_HashString, le_hashmap_EqualsString);
    char name[32] = {0};
    taf_pm_VMListRef_t vmListRef = taf_pm_GetMachineList( );
    le_result_t res;
    if(vmListRef)
    {
        res = taf_pm_GetFirstMachineName(vmListRef, name, 32);
        while(res == LE_OK)
        {
            taf_mngdPm_vmState_t *vmStatePtr = (taf_mngdPm_vmState_t*)le_mem_ForceAlloc(mpms.vmStatePool);
            memset(vmStatePtr, 0, sizeof(taf_mngdPm_vmState_t));
            vmStatePtr->nad = TAF_MNGDPM_NAD1;
            memset(vmStatePtr->vmName, 0, sizeof(vmStatePtr->vmName));
            le_utf8_Copy(vmStatePtr->vmName, name, TAF_MNGDPM_MACHINE_NAME_LEN, NULL);
            vmStatePtr->state = TAF_MNGDPM_STATE_RESUME;
            LE_INFO("Add %s machine to hashmap", vmStatePtr->vmName);
            le_hashmap_Put(mpms.vmStateHashmap, vmStatePtr->vmName, vmStatePtr);
            res = taf_pm_GetNextMachineName(vmListRef, name, 32);
        }
        taf_pm_DeleteMachineList(vmListRef);
    }
    mpms.handlerRef = taf_pm_AddStateChangeHandler(tafMngdPMSvc::StateChangeHandler, NULL);
    if (mpms.handlerRef)
        LE_INFO("Register state change handler is successfull");

    mpms.stateMachine.currentState = TAF_MNGDPM_STATE_RESUME;

    mpms.handlerExRef = taf_pm_AddStateChangeExHandler(tafMngdPMSvc::StateChangeExHandler, NULL);
    if (mpms.handlerExRef)
        LE_INFO("Register Extended state change handler is successfull");

    mpms.infoReportHandlerPool = le_mem_CreatePool("infoReportHandlerList", sizeof(taf_mngdPm_InfoReportCb_t));
    mpms.infoReportHandlerList = LE_DLS_LIST_INIT;
    mpms.infoReportHandlerRefMap = le_ref_CreateMap("infoReportHandlerRef", TAF_REF_POOL_SIZE);
    mpms.infoReport = le_event_CreateId("tafInfoReportCbEvent", sizeof(bubStatusEvent_t));
    le_event_AddHandler("tafPMInfoReporCbtevent", mpms.infoReport, mpms.InfoReportCB);
    //All supported stay awake reasons are authorized by default.
    taf_mngdPm_AuthorizeStayAwakeReason(AUTHORIZE_ALL_STAY_AWAKE_REASON);

    //creating the timer for vehichle wakeup
    mpms.stateChangeAckTimerRef = le_timer_Create("STATE CHANGE ACK timer");
    le_timer_SetWakeup(mpms.stateChangeAckTimerRef, false);
    le_timer_SetMsInterval(mpms.stateChangeAckTimerRef, mpms.config.state_change_ack_timeout);
    le_timer_SetHandler(mpms.stateChangeAckTimerRef, mpms.StateChangeAckTimerHandler);

    // Initialize service-wide current node power state
    tafMngdPMSvc::InitializeCurrentNodePowerState();

    LE_INFO("COMPONENT end init");
}
