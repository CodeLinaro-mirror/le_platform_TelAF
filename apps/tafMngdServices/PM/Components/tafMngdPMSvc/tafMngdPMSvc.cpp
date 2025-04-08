/*
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafMngdPMSvc.hpp"
#include "tafMngdPMCommon.hpp"
#include "rpcPm/tafMngdRpcPm.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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
        if(handlerPtr != NULL)
        {
            handlerPtr(mode, TAF_MNGDPM_NOT_READY, LE_UNSUPPORTED, contextPtr);
        }
        return LE_UNSUPPORTED;
    }

    if(reason > 1 && reason < 15)
    {
        LE_INFO("Shutdown reason is not supported");
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
        if(handlerPtr != NULL)
        {
            handlerPtr(mode, TAF_MNGDPM_NOT_READY, LE_UNSUPPORTED, contextPtr);
        }
        return LE_UNSUPPORTED;
    }
    if(reason > 2 && reason < 15)
    {
        LE_INFO("Restart reason is not supported");
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
            if(mpms.stateMachine.currentState != TAF_MNGDPM_STATE_RESUME)
            {
                LE_INFO("Current state is not resume to trigger WakeupVehicleReqAsync");
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
            LE_INFO("Returning unsupported if drive is not available");

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
                LE_INFO("wakeupType found in wsWhiteList");
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
        LE_INFO("wakeupType not found in wsWhiteList");
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
            LE_INFO("WakeupType matched with nodeWsRefList for StayAwakeNode");
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
            LE_INFO("WakeupType matched with nodeWsRefList for RelaxNode");
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
 * Local api which Keeps the system awake by acquiring wake lock for the given reference.
 */
le_result_t AcquireWakeSource(taf_wsRefCtx_t * wsRefCtxPtr)
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
        LE_INFO("Acquired wakelock");
        wsRefCtxPtr->isAcquiredLock = true;
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
        LE_INFO("Failed to acquire wake source.");
    }
    return res;
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
    if(tafMngdPMSvc::IsClientValid() == false || wsTag == NULL)
    {
        return NULL;
    }
    if(reason > 4 && reason <= 15)
    {
        LE_INFO("StayAwakeReason is not supported");
        return NULL;
    }
    taf_wsRefCtx_t * wsCtxPtr =
            (taf_wsRefCtx_t *)le_mem_ForceAlloc(mpms.wsRefPool);
    wsCtxPtr->wsRef = (taf_mngdPm_wsRef_t)le_ref_CreateRef(
            mpms.wsRefMap, wsCtxPtr);
    wsCtxPtr->wsTag = strdup(wsTag);
    wsCtxPtr->reason = reason;
    wsCtxPtr->sessionRef = taf_mngdPm_GetClientSessionRef();
    wsCtxPtr->link = LE_DLS_LINK_INIT;
    wsCtxPtr->isAcquiredLock= false;
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
    mpms.ClearUnAuthorizedWakeSources();
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
            LE_INFO("wsRef is valid in wsRefList for StayAwake");
            //check if already a wakelock acquired
            if(wsRefCtxPtr->isAcquiredLock)
            {
                 LE_INFO("WakeLock already acquired");
                 return LE_DUPLICATE;
            }
            if(mpms.IsAuthorizedStayAwakeReason(wsRefCtxPtr->reason))
            {
                LE_INFO("stayAwakeReason is in authorized stayAwakeReasonList");
                res = AcquireWakeSource(wsRefCtxPtr);
            }
            else
            {
                LE_INFO("Non authorized StayAwakeReason for stayawake");
                wsRefCtxPtr->isAcquiredLock = true;
                return LE_OK;
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
            if(wsRefCtxPtr->isAcquiredLock)
            {
                if(mpms.IsAuthorizedStayAwakeReason(wsRefCtxPtr->reason))
                {
                    res = mpms.ReleaseWakeSource(wsRefCtxPtr);
                }
                else
                {
                    LE_INFO("Non authorized StayAwakeReason for relax");
                    wsRefCtxPtr->isAcquiredLock = false;
                    return LE_OK;
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
    return res;
}

/**
 * Initiates the forceful restart for the given node.
 */
le_result_t taf_mngdPm_ShutdownNode (uint8_t pmNodeId)
{
    LE_DEBUG("taf_mngdPm_ShutdownNode pmNodeId : %d", pmNodeId);
    if(tafMngdPMSvc::IsClientValid() == false)
    {
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
    LE_INFO("taf_mngdPm_RestartNode");
    if(tafMngdPMSvc::IsClientValid() == false)
    {
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
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return NULL;
    }

    if(pmNodeId == 1)
    {
        auto &rpcPm = tafMngdRpcPm::GetInstance();
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref =
             rpcPm.AddRpcNodePowerStateChangeHandler(handlerFuncPtr, contextPtr, pmNodeId, stateMask);
        return ref;
    }
    auto &mpms = tafMngdPMSvc::GetInstance();
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

    return (taf_mngdPm_NodePowerStateChangeHandlerRef_t)handlerCtxPtr->handlerRef;
}

/**
 * Removes the client from NodePowerStateChangeHandler.
 */
void taf_mngdPm_RemoveNodePowerStateChangeHandler(taf_mngdPm_NodePowerStateChangeHandlerRef_t handlerRef)
{
    LE_INFO("RemoveNodePowerStateHandler");

    bool isPmsNodeChangeRef = false;
    auto &mpms = tafMngdPMSvc::GetInstance();
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.nodePowerStateHandlerList));
    while (linkHandlerPtr)
    {
        taf_mngdPm_NodePowerStateCtxt_t * handlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_mngdPm_NodePowerStateCtxt_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.nodePowerStateHandlerList), linkHandlerPtr);
        if (handlerCtxPtr && handlerCtxPtr->handlerRef == handlerRef)
        {
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
        return LE_UNSUPPORTED;
    }
    //Getting the current client data from mngdPmClientInfo
    taf_mngdPm_SessionNode_t* sessionNodePtr;
    sessionNodePtr = mpms.To_taf_mngdPm_SessionNode_t(le_hashmap_Get(mpms.mngdPmClientInfo.clients,
            taf_mngdPm_GetClientSessionRef()));

    taf_mngdPm_NodePowerState_t state = TAF_MNGDPM_NODE_STATE_RESUME;
    if(pmNodeId == 1)
    {
        auto &rpcPm = tafMngdRpcPm::GetInstance();
        le_result_t res = rpcPm.SendRpcNodePowerStateChangeAck(pmNodeId, Ref, ack);
        return res;
    }
    // validate client record existed in state change registered clients
    bool isClientPresent = false;
    for (const auto &client : mpms.regClientrecrd) {
        if (((client.nodeStateRef == (taf_mngdPm_nodePowerStateRef_t)Ref)) &&
                    (client.sessionRef == taf_mngdPm_GetClientSessionRef())) {
            LE_INFO("Client found in record");
            state = client.state;
            isClientPresent = true;
            break;
        }
    }
    if (!isClientPresent) {
        LE_INFO("Client not found in the regClientrecrd");
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

COMPONENT_INIT
{
    LE_INFO("tafMngdPMSvc COMPONENT init...");

    auto &mpms = tafMngdPMSvc::GetInstance();

    mpms.Init();

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

    le_msg_AddServiceOpenHandler(taf_mngdPm_GetServiceRef(), tafMngdPMSvc::OnClientConnection,
            NULL);

    le_msg_AddServiceCloseHandler(taf_mngdPm_GetServiceRef(), tafMngdPMSvc::OnClientDisconnection,
            NULL);

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
    if(mpms.config.hal_enabled) {
        res = tafMngdPMSvc::InitVHalModule();
        if((res == LE_OK) && (mpms.pmInf) && (mpms.pmInf->addNodeEventHandler))
        {
            LE_INFO("addNodeEventHanlder for node %d", NODE_ID);
            (*(mpms.pmInf->addNodeEventHandler))(NODE_ID, tafMngdPMSvc::NodeEventCB);
        }
    }
    mpms.handlerExRef = taf_pm_AddStateChangeExHandler(tafMngdPMSvc::StateChangeExHandler, NULL);
    if (mpms.handlerExRef)
        LE_INFO("Register Extended state change handler is successfull");

    mpms.WaitWakeSourceTimer();
    mpms.infoReportHandlerPool = le_mem_CreatePool("infoReportHandlerList", sizeof(taf_mngdPm_InfoReportCb_t));
    mpms.infoReportHandlerList = LE_DLS_LIST_INIT;
    mpms.infoReportHandlerRefMap = le_ref_CreateMap("infoReportHandlerRef", TAF_REF_POOL_SIZE);
    mpms.infoReport = le_event_CreateId("tafInfoReportCbEvent", sizeof(bubStatusEvent_t));
    le_event_AddHandler("tafPMInfoReporCbtevent", mpms.infoReport, mpms.InfoReportCB);
    //All supported stay awake reasons are authorized by default.
    taf_mngdPm_AuthorizeStayAwakeReason(AUTHORIZE_ALL_STAY_AWAKE_REASON);

    //creating the timer for vehichle wakeup
    mpms.stateChangeAckTimerRef = le_timer_Create("STATE CHANGE ACK timer");
    le_timer_SetMsInterval(mpms.stateChangeAckTimerRef, mpms.config.state_change_ack_timeout);
    le_timer_SetHandler(mpms.stateChangeAckTimerRef, mpms.StateChangeAckTimerHandler);
    LE_INFO("COMPONENT end init");
}
