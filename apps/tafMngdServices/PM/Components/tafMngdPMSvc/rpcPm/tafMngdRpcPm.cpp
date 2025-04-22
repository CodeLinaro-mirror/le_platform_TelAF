/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafMngdRpcPm.hpp"
#include "tafMngdPMCommon.hpp"
#include "tafMngdPMSvc.hpp"


using namespace telux::tafsvc;


/**
 * Set shutdown state to NAD
 */
le_result_t tafMngdRpcPm::ShutdownRpcNAD()
{
    LE_INFO(" shutdown the RPC NAD");
    le_result_t res = LE_FAULT;
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    if(rpcPm.IsRpcConnected) {
        LE_INFO("RPC Connected");
        res = taf_rpcPm_SetAllVMPowerState(TAF_RPCPM_STATE_SHUTDOWN);
        if(res != LE_OK)
        {
            LE_ERROR("Failed to shutdown the NAD");
        }
        else
        {
            LE_INFO(" shutdown the RPC NAD");
        }
    }
    return res;
}

/**
 * Initiates the Restart for the given node.
 */
le_result_t tafMngdRpcPm::RestartRpcNAD()
{
    LE_INFO(" Restart the RPC NAD");
    le_result_t res = LE_FAULT;
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    if(rpcPm.IsRpcConnected) {
        res = taf_rpcPm_SetAllVMPowerState(TAF_PM_STATE_RESTART);
        if(res != LE_OK)
        {
            LE_ERROR("Failed to restart the NAD");
        }
        else
        {
            LE_INFO(" RestartRpcNAD the RPC NAD");
        }
    }
    return res;
}


/**
 * Set shutdown state to NAD
 */
le_result_t tafMngdRpcPm::SuspendRpcNAD()
{
    LE_INFO(" Suspend the RPC NAD");
    le_result_t res = LE_FAULT;
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    if(rpcPm.IsRpcConnected) {
        LE_INFO("RPC Connected");
        res = taf_rpcPm_SetAllVMPowerState(TAF_RPCPM_STATE_SUSPEND);
        if(res != LE_OK)
        {
            LE_ERROR("Failed to Suspend the NAD");
        }
        else
        {
            LE_INFO(" Suspend the RPC NAD");
        }
	}
    return res;
}

/**
 * Acquire wakesource and let system stay awake
 */
le_result_t tafMngdRpcPm::AcquireRpcNodeWakeLock()
{
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    LE_INFO("AcquireRpcNodeWakeLock rpcWsCount:%d", rpcPm.rpcWsCount);
    le_result_t res = LE_FAULT;
    if(rpcPm.IsRpcConnected) {
        if(rpcPm.rpcWsCount == 0) {
            if(rpcPm.rpcWs == nullptr)
                rpcPm.rpcWs = taf_rpcPm_NewWakeupSource(WAKELOCK_WITHOUT_REF, "rpcPm");
            if (rpcPm.rpcWs != nullptr)
            {
                res = taf_rpcPm_StayAwake(rpcPm.rpcWs);
                if(res == LE_OK) {
                    LE_INFO("Wake source from RPC PM acquired successfully");
                    rpcPm.rpcWsCount++;
                }
                else
                {
                    LE_INFO("failed to acquire rpcWs");
                }
            }
            else
            {
                LE_ERROR("Failed to create wakeup source!");
            }
        }
        else
        {
            LE_INFO("Wake source from RPC PM acquired successfully");
            rpcPm.rpcWsCount++;
            res = LE_OK;
        }
    }
    return res;
}

/**
 * Release wakesource
 */
le_result_t tafMngdRpcPm::ReleaseRpcNodeWakeLock()
{
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    LE_INFO("ReleaseRpcNodeWakeLock rpcWsCount:%d", rpcPm.rpcWsCount);

    le_result_t res = LE_FAULT;
    if(rpcPm.IsRpcConnected) {
        if(rpcPm.rpcWsCount > 0 )
        {
            LE_INFO("Wake source released successfully");
            rpcPm.rpcWsCount--;
            res = LE_OK;
            if(rpcPm.rpcWsCount == 0 )
            {
                if (rpcPm.rpcWs != nullptr)
                {
                    res = taf_rpcPm_Relax(rpcPm.rpcWs);
                    if(res == LE_OK) {
                        LE_INFO("Wake source released successfully");
                    }
                }
                else
                {
                    LE_ERROR("Failed to release wakeup lock!");
                    res = LE_FAULT;
                }
            }
        }
        else
        {
            LE_ERROR("No wakeup lock acquired to release !");
        }
    }
    return res;
}

/**
 * Releases the acquired wake lock for the given reference.
 */
le_result_t tafMngdRpcPm::RelaxRpcNode(taf_mngdPm_wsRef_t wsRef)
{
    LE_INFO("RelaxRpcNode");

    auto &rpcPm = tafMngdRpcPm::GetInstance();
    le_result_t res = LE_FAULT;
    if(rpcPm.IsRpcConnected)
    {
        le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(rpcPm.rpcWsRefList));

        while (linkHandlerPtr)
        {
            taf_nodeWsRefCtx_t * wsRefCtxPtr =
                    CONTAINER_OF(linkHandlerPtr, taf_nodeWsRefCtx_t, link);
            linkHandlerPtr = le_dls_PeekPrev(&(rpcPm.rpcWsRefList), linkHandlerPtr);
            if (wsRefCtxPtr && wsRef && wsRefCtxPtr->wsRef == wsRef &&
                    wsRefCtxPtr->sessionRef == taf_mngdPm_GetClientSessionRef() && wsRefCtxPtr->isAcquiredLock)
            {
                LE_INFO("WakeupType matched with whitelisting wakeup_source");
                res = ReleaseRpcNodeWakeLock();
                if(res == LE_OK)
                {
                    LE_INFO("Rpc client Released WakeLock");
                    wsRefCtxPtr->isAcquiredLock = false;
                }
                break;
            }
        }
    }
    return res;
}

/**
 * Keeps the system awake by acquiring wake lock for the given reference.
 */
le_result_t tafMngdRpcPm::StayAwakeRpcNode(taf_mngdPm_wsRef_t wsRef)
{
    LE_INFO("RPC StayAwakeRpcNode");

    auto &rpcPm = tafMngdRpcPm::GetInstance();
    le_result_t res = LE_FAULT;
    if(rpcPm.IsRpcConnected) {
        le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(rpcPm.rpcWsRefList));
        while (linkHandlerPtr)
        {
            taf_nodeWsRefCtx_t * wsRefCtxPtr =
                    CONTAINER_OF(linkHandlerPtr, taf_nodeWsRefCtx_t, link);
            linkHandlerPtr = le_dls_PeekPrev(&(rpcPm.rpcWsRefList), linkHandlerPtr);
            if (wsRefCtxPtr && wsRef && wsRefCtxPtr->wsRef == wsRef &&
                    wsRefCtxPtr->sessionRef == taf_mngdPm_GetClientSessionRef())
            {
                LE_INFO("Wake source reference found.");
                //check if already a wakelock acquired
                if(wsRefCtxPtr->isAcquiredLock)
                {
                     LE_INFO("RPC client WakeLock already acquired");
                     return LE_FAULT;
                }
                res = AcquireRpcNodeWakeLock();
                if(res == LE_OK)
                {
                    LE_INFO("RPC client Acquired wakelock");
                    wsRefCtxPtr->isAcquiredLock = true;
                }
                break;
            }
        }
    }
    else
    {
        LE_INFO("RPC not Connected");
        return LE_FAULT;
    }
    return res;
}

/**
 * Creates the node wakeupSource reference.
 */
taf_mngdPm_wsRef_t tafMngdRpcPm::NewRpcNodeWakeupSource(uint8_t pmNodeId,
        taf_mngdPm_WakeupType_t wakeupType)

{
    LE_INFO("NewRpcNodeWakeupSource");
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    if(rpcPm.IsRpcConnected) {
        LE_INFO("RPC Connected");
        taf_nodeWsRefCtx_t * wsCtxPtr =
                (taf_nodeWsRefCtx_t *)le_mem_ForceAlloc(rpcPm.rpcWsRefPool);
        if(wsCtxPtr) {
            wsCtxPtr->wsRef = (taf_mngdPm_wsRef_t)le_ref_CreateRef(
                    rpcPm.rpcWsRefMap, wsCtxPtr);
            wsCtxPtr->pmNodeId = pmNodeId;
            wsCtxPtr->wakeupType = wakeupType;
            wsCtxPtr->sessionRef = taf_mngdPm_GetClientSessionRef();
            wsCtxPtr->link = LE_DLS_LINK_INIT;
            wsCtxPtr->isAcquiredLock= false;
            le_dls_Queue(&(rpcPm.rpcWsRefList), &wsCtxPtr->link);
            return wsCtxPtr->wsRef;
        }
    }
    return NULL;
}

/**
 * Romves the rpc wakeupSource reference.
 */
void RemoveRpcNodeWakeupSource()
{
    LE_INFO("RemoveRpcNodeWakeupSource");
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(rpcPm.rpcWsRefList));

    while (linkHandlerPtr)
    {
        taf_nodeWsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_nodeWsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(rpcPm.rpcWsRefList), linkHandlerPtr);
        if (wsRefCtxPtr && wsRefCtxPtr->wsRef)
        {
            le_ref_DeleteRef(rpcPm.rpcWsRefMap, wsRefCtxPtr->wsRef);
            le_dls_Remove(&(rpcPm.rpcWsRefList), &wsRefCtxPtr->link);
            le_mem_Release((void*)wsRefCtxPtr);
        }
    }
}

/**
 * SendRpcNodePowerStateChangeAck for the given node.
 */
le_result_t tafMngdRpcPm::SendRpcNodePowerStateChangeAck
(
    uint8_t pmNodeId,
    taf_mngdPm_nodePowerStateRef_t Ref,
    taf_mngdPm_NodeClientAck_t ack
)
{
    LE_INFO("SendRpcNodePowerStateChangeAck");
    taf_mngdPm_NodePowerState_t state = TAF_MNGDPM_NODE_STATE_RESUME;
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    bool IsClientPresent = false;
    if(rpcPm.IsRpcConnected) {
        // validate client record existed in state change registered clients
        for(const auto &client:rpcPm.rpcRegClientrecrd)
        {
            if (((client.nodeStateRef == (taf_mngdPm_nodePowerStateRef_t)Ref) &&
                    (client.sessionRef == taf_mngdPm_GetClientSessionRef())))
            {
                LE_INFO("Client found in record");
                IsClientPresent = true;
                state = client.state;
                break;
            }
        }
        if(IsClientPresent)
        {
            if(ack == TAF_MNGDPM_CLIENT_NOT_READY)
            {
                LE_INFO("Received NACK from client");
                rpcPm.SendAckToRpcPms(state, TAF_RPCPM_NOT_READY);
                return LE_OK;
            }
            else
            {
                LE_INFO("Received ACK from client");
                rpcPm.ackRpcClientrecrdSize++;
                LE_INFO("rpcRegClientrecrd size is %zu ,ackRpcClientrecrdSize size is:%d",
                        rpcPm.rpcRegClientrecrd.size(), rpcPm.ackRpcClientrecrdSize);
                //If Last acknowledged client , proceed for ack state change
                if((int8_t)rpcPm.rpcRegClientrecrd.size() == rpcPm.ackRpcClientrecrdSize)
                {
                    rpcPm.clientSize = 0;
                    rpcPm.ackRpcClientrecrdSize = 0;
                    rpcPm.SendAckToRpcPms(state, TAF_RPCPM_READY);
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
    }
    return LE_FAULT;
}

/**
 * Ex State change callback function for RPC PM service
 */
void tafMngdRpcPm::RpcPmStateChangeExHandler(taf_rpcPm_PowerStateRef_t psRef,
                taf_rpcPm_NadVm_t vm_id, taf_rpcPm_State_t state, void* contextPtr)
{
    LE_INFO("State change triggered in RpcPmStateChangeExHandler");
    auto &rpcPm = tafMngdRpcPm::GetInstance();

    rpcPm.rpcPowerStateRef = psRef;
    taf_mngdPm_NodePowerStateChange_t rpcPowerStateChange;
    if(state == TAF_RPCPM_STATE_ALL_ACKED)
    {
        LE_DEBUG("Send ACK");
        taf_rpcPm_SendStateChangeAck(rpcPm.rpcPowerStateRef, state, vm_id, TAF_RPCPM_READY);
        return;
    }
    if(state == TAF_RPCPM_STATE_ALL_WAKELOCKS_RELEASED)
    {
        LE_DEBUG("Received all wakelocks released notification");
        if(rpcPm.rpcTargetedPowerMode == TAF_MNGDPM_SHUTDOWN)
        {
            LE_INFO("RPC ShutdownNAD");
            le_result_t res = rpcPm.ShutdownRpcNAD();
            if(res == LE_OK) {
                LE_INFO("RPC ShutdownNAD triggered from MPMS");
                return;
            }
            else
            {
                LE_ERROR("Failed RPC ShutdownNAD!");
            }
        }
        else if(rpcPm.rpcWsCount == 0)
        {
            LE_INFO("SuspendRpcNAD");
	        le_result_t res = rpcPm.SuspendRpcNAD();
            if(res == LE_OK) {
                LE_INFO("SuspendRpcNAD triggered from MPMS");
	            return;
            }
            else
            {
                LE_ERROR("Failed RPC SuspendNAD!");
            }
        }
    }
    else if(state == TAF_RPCPM_STATE_SUSPEND)
    {
        rpcPowerStateChange.state = TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE;
        le_event_Report(rpcPm.rpcNodePowerStateChange, &rpcPowerStateChange, sizeof(taf_mngdPm_NodePowerStateChange_t));
    }
    else if(state == TAF_RPCPM_STATE_SHUTDOWN)
    {

        rpcPowerStateChange.state = TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE;
        le_event_Report(rpcPm.rpcNodePowerStateChange, &rpcPowerStateChange, sizeof(taf_mngdPm_NodePowerStateChange_t));
    }
    else if(state == TAF_RPCPM_STATE_RESTART)
    {

        rpcPowerStateChange.state = TAF_MNGDPM_NODE_STATE_RESTART_PREPARE;
        le_event_Report(rpcPm.rpcNodePowerStateChange, &rpcPowerStateChange, sizeof(taf_mngdPm_NodePowerStateChange_t));
    }
    else if(state == TAF_RPCPM_STATE_RESUME)
    {
        rpcPowerStateChange.state = TAF_MNGDPM_NODE_STATE_RESUME;
        le_event_Report(rpcPm.rpcNodePowerStateChange, &rpcPowerStateChange, sizeof(taf_mngdPm_NodePowerStateChange_t));

    }
}

void tafMngdRpcPm::SendAckToRpcPms(taf_mngdPm_NodePowerState_t state, taf_rpcPm_ClientAck_t ackType)
{
    LE_INFO("SendAckToRpcPms");
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    if(rpcPm.IsRpcConnected)
    {
        if (state == TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE)
        {
            LE_INFO("TAF_PM_STATE_SHUTDOWN");
            taf_rpcPm_SendStateChangeAck(rpcPm.rpcPowerStateRef, TAF_RPCPM_STATE_SHUTDOWN, TAF_RPCPM_PVM, ackType);
        }
        else if(state== TAF_MNGDPM_NODE_STATE_RESTART_PREPARE)
        {
            LE_INFO("TAF_PM_STATE_SHUTDOWN");
            taf_rpcPm_SendStateChangeAck(rpcPm.rpcPowerStateRef, TAF_RPCPM_STATE_RESTART, TAF_RPCPM_PVM, ackType);
        }
        else if (state == TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE)
        {
            LE_INFO("TAF_RPCPM_STATE_SUSPEND");
            taf_rpcPm_SendStateChangeAck(rpcPm.rpcPowerStateRef, TAF_RPCPM_STATE_SUSPEND, TAF_RPCPM_PVM, ackType);
        }
        else if (state == TAF_MNGDPM_NODE_STATE_RESUME)
        {
            LE_INFO("TAF_RPCPM_STATE_RESUME");
            taf_rpcPm_SendStateChangeAck(rpcPm.rpcPowerStateRef, TAF_RPCPM_STATE_RESUME, TAF_RPCPM_PVM,
                    ackType);
        }
    }
}

void tafMngdRpcPm::DeleteRpcNodePowerStateRefs()
{
    LE_INFO("DeleteRpcNodePowerStateRefs");
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    if(rpcPm.rpcRegClientrecrd.size() != 0) {
        for (const auto &it:rpcPm.rpcRegClientrecrd )
        {
            le_ref_DeleteRef(rpcPm.rpcNodePowerStateRefMap, it.nodeStateRef);
        }
    }
}

/**
 * To Call Clients for Extend power state change notification
 */
void CallRpcNodePowerStateHandlerFunc(taf_mngdPm_NodePowerState_t state)
{
    LE_INFO("CallRpcNodePowerStateHandlerFunc");
    auto &mpms = tafMngdPMSvc::GetInstance();
    auto &rpcPm = tafMngdRpcPm::GetInstance();

    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(rpcPm.rpcNodePowerStateHandlerList));
    //clearing the previous references for new state notification

    rpcPm.DeleteRpcNodePowerStateRefs();
    rpcPm.rpcRegClientrecrd.clear();
    rpcPm.ackRpcClientrecrdSize = 0;
    while (linkHandlerPtr)
    {
        taf_mngdPm_NodePowerStateCtxt_t * rpcHandlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_mngdPm_NodePowerStateCtxt_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(rpcPm.rpcNodePowerStateHandlerList), linkHandlerPtr);
        if (rpcHandlerCtxPtr->handlerPtr)
        {
            LE_INFO("Client found");
            if(mpms.IsConfiguredBitMask(state, rpcHandlerCtxPtr->powerStateMask))
            {
                taf_NodePowerStateRef_t* nodeStateListPtr =
                        (taf_NodePowerStateRef_t*)le_mem_ForceAlloc(rpcPm.rpcNodePowerStateRefPool);
                nodeStateListPtr->nodeStateRef =
                        (taf_mngdPm_nodePowerStateRef_t)le_ref_CreateRef(rpcPm.rpcNodePowerStateRefMap, nodeStateListPtr);
                    rpcPm.rpcRegClientrecrd.push_back({nodeStateListPtr->nodeStateRef,
                            rpcHandlerCtxPtr->sessionRef, state});
                rpcHandlerCtxPtr->handlerPtr(rpcHandlerCtxPtr->pmNodeId, nodeStateListPtr->nodeStateRef, state,
                        rpcHandlerCtxPtr->nodePowerStateHandlerCtxPtr);
                LE_INFO("Notified to Client");
            }
            else {
                continue;
            }
            rpcPm.clientSize++;
        }
    }
    if(rpcPm.clientSize == 0)
    {
        LE_INFO("No client registered in MPMS, ack to PMS immediately for state:%d", state);
        rpcPm.SendAckToRpcPms(state, TAF_RPCPM_READY);
    }
}

/**
 * State change layered handler function
 */
void tafMngdRpcPm::RpcNodePowerStateChanged(void* reportPtr)
{
    LE_INFO("RpcNodePowerStateChanged");
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");
    taf_mngdPm_NodePowerStateChange_t* rpcPowerStateChange =(taf_mngdPm_NodePowerStateChange_t*)reportPtr;
    if(rpcPowerStateChange->state == TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE)
    {
        CallRpcNodePowerStateHandlerFunc(TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE);
    }
    else if(rpcPowerStateChange->state == TAF_MNGDPM_NODE_STATE_RESTART_PREPARE)
    {
        CallRpcNodePowerStateHandlerFunc(TAF_MNGDPM_NODE_STATE_RESTART_PREPARE);
    }
    else if(rpcPowerStateChange->state == TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE)
    {
        CallRpcNodePowerStateHandlerFunc(rpcPowerStateChange->state);
    }
    else if(rpcPowerStateChange->state == TAF_MNGDPM_NODE_STATE_RESUME)
    {
        CallRpcNodePowerStateHandlerFunc(rpcPowerStateChange->state);
    }
    else
    {
        LE_INFO("Invalid state");
    }
}

taf_mngdPm_NodePowerStateChangeHandlerRef_t tafMngdRpcPm::AddRpcNodePowerStateChangeHandler
(
    taf_mngdPm_NodePowerStateChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr,
    uint8_t pmNodeId,
    taf_mngdPm_NodePowerStateChangeBitMask_t stateMask
)
{

    LE_INFO("AddRpcNodePowerStateChangeHandler");
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    if(rpcPm.IsRpcConnected)
    {
        //save handlerref according to bitmask
        TAF_ERROR_IF_RET_VAL(handlerFuncPtr == NULL, NULL, "INVALID handler reference.");
        taf_mngdPm_NodePowerStateCtxt_t * rpcHandlerCtxPtr =
                (taf_mngdPm_NodePowerStateCtxt_t *)le_mem_ForceAlloc(rpcPm.rpcNodePowerStateHandlerPool);
        rpcHandlerCtxPtr->handlerPtr = handlerFuncPtr;
        rpcHandlerCtxPtr->sessionRef = taf_mngdPm_GetClientSessionRef();
        rpcHandlerCtxPtr->pmNodeId = pmNodeId;
        rpcHandlerCtxPtr->powerStateMask = stateMask;
        rpcHandlerCtxPtr->handlerRef = (taf_mngdPm_NodePowerStateChangeHandlerRef_t)le_ref_CreateRef(
                rpcPm.rpcNodePowerStateHandlerMap, rpcHandlerCtxPtr);
        rpcHandlerCtxPtr->link = LE_DLS_LINK_INIT;
        rpcHandlerCtxPtr->nodePowerStateHandlerCtxPtr = contextPtr;
        le_dls_Queue((&(rpcPm.rpcNodePowerStateHandlerList)), &rpcHandlerCtxPtr->link);

        return (taf_mngdPm_NodePowerStateChangeHandlerRef_t)rpcHandlerCtxPtr->handlerRef;
    }
    return NULL;
}

/**
 * Removes the client from NodePowerStateChangeHandler.
 */
void tafMngdRpcPm::RemoveRpcNodePowerStateChangeHandler(taf_mngdPm_NodePowerStateChangeHandlerRef_t handlerRef)
{
    LE_INFO("RemoveRpcNodePowerStateChangeHandler");
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    if(rpcPm.IsRpcConnected)
    {
        le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(rpcPm.rpcNodePowerStateHandlerList));
        while (linkHandlerPtr)
        {
            taf_mngdPm_NodePowerStateCtxt_t * rpcHandlerCtxPtr =
                    CONTAINER_OF(linkHandlerPtr, taf_mngdPm_NodePowerStateCtxt_t, link);
            linkHandlerPtr = le_dls_PeekPrev(&(rpcPm.rpcNodePowerStateHandlerList), linkHandlerPtr);
            if (rpcHandlerCtxPtr && rpcHandlerCtxPtr->handlerRef == handlerRef)
            {
                le_ref_DeleteRef(rpcPm.rpcNodePowerStateHandlerMap, handlerRef);
                le_dls_Remove(&(rpcPm.rpcNodePowerStateHandlerList), &rpcHandlerCtxPtr->link);
                le_mem_Release((void*)rpcHandlerCtxPtr);
            }
        }
    }
}

void tafMngdRpcPm::TryConnectService()
{
    IsRpcConnected = false;
    le_result_t res = taf_rpcPm_TryConnectService();
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    if(res == LE_OK) {
        IsRpcConnected = true; //use this bool everyt time before rpc call.
        LE_INFO("client connected successfully to the service.");
        le_timer_Stop(rpcPm.RpcConnectTimerRef);
        //Restore wake source reference if clients acquired wakesource
        if(rpcPm.rpcWsCount > 0)
        {
            if(rpcPm.rpcWs == nullptr)
                rpcPm.rpcWs = taf_rpcPm_NewWakeupSource(WAKELOCK_WITHOUT_REF, "rpcPm");
            if (rpcPm.rpcWs != nullptr)
            {
                res = taf_rpcPm_StayAwake(rpcPm.rpcWs);
                if(res == LE_OK)
                {
                    LE_INFO("Wake source restored successfully");
                }
            }
        }
        RpcRetryCount = 0;
    }
    else if(res == LE_UNAVAILABLE) {
        LE_INFO("server is not currently offering the service to which the client is bound.");
        RpcRetryCount++;
        le_timer_SetContextPtr(rpcPm.RpcConnectTimerRef, NULL);
        le_timer_Start(rpcPm.RpcConnectTimerRef);
        LE_INFO("Started RpcConnectTimerRef");
    }
    else if(res == LE_NOT_PERMITTED) {
        LE_INFO("client interface is not bound to any service (doesn't have a binding)");
    }
    else if(res == LE_COMM_ERROR) {
        LE_INFO("Service Directory cannot be reached");
    }
}

/**
 * RpcConnect timer handler
 */
void tafMngdRpcPm::RpcConnectTimerHandler(le_timer_Ref_t timerRef)
{
    LE_INFO("RpcConnectTimerHandler");
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    if(RpcRetryCount < 2) {
        LE_INFO("Retry RpcConnectTimerHandler");
        TryConnectService();
    }
    else
    {
        LE_INFO("Timeout occurred for RpcConnectTimerHandler");
        RpcRetryCount = 0;
        if(rpcPm.rpcWs)
        {
            rpcPm.rpcWs = NULL;
        }
        if(rpcPm.rpcWsCount > 0)
        {
            rpcPm.rpcWsCount = 0;
        }
        RemoveRpcNodeWakeupSource();
        return;
    }
}

void tafMngdRpcPm::RpcDisconnectHandler(void* contextPtr)
{
    LE_INFO("RpcDisconnectHandler");
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    rpcPm.TryConnectService();
}

tafMngdRpcPm &tafMngdRpcPm::GetInstance()
{
    static tafMngdRpcPm instance;
    return instance;
}

void tafMngdRpcPm::OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *ctxPtr)
{
    LE_INFO("Rpc OnClientDisconnection");
    auto &rpcPm = tafMngdRpcPm::GetInstance();
    //clear state change registered clients
    for(auto it = rpcPm.rpcRegClientrecrd.begin(); it != rpcPm.rpcRegClientrecrd.end(); )
    {
        if(it->sessionRef == sessionRef)
        {
            LE_INFO("Client with sessionRef %p", it->sessionRef);
            it = rpcPm.rpcRegClientrecrd.erase(it);
        }
        else
        {
            ++it;
        }
    }
    //Clear rpcWsRefList
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(rpcPm.rpcWsRefList));

    while (linkHandlerPtr)
    {
        taf_nodeWsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_nodeWsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(rpcPm.rpcWsRefList), linkHandlerPtr);
        if (wsRefCtxPtr && wsRefCtxPtr->sessionRef == sessionRef)
        {
            if(wsRefCtxPtr->isAcquiredLock)
            {
                LE_INFO("Client with sessionRef %p", wsRefCtxPtr->sessionRef);
                le_result_t res = tafMngdRpcPm::ReleaseRpcNodeWakeLock();
                if(res == LE_OK)
                {
                    LE_INFO("Released lock");
                    wsRefCtxPtr->isAcquiredLock = false;
                }
            }
            le_ref_DeleteRef(rpcPm.rpcWsRefMap, wsRefCtxPtr->wsRef);
            le_dls_Remove(&(rpcPm.rpcWsRefList), &wsRefCtxPtr->link);
            free((void*)wsRefCtxPtr->vhalTag);
            le_mem_Release((void*)wsRefCtxPtr);
        }
    }
}

/**
 * tafMngdPMSvc initialization
 */
void tafMngdRpcPm::Init(void)
{
    LE_INFO("Init");
    auto &rpcPm = tafMngdRpcPm::GetInstance();

    RpcConnectTimerRef = le_timer_Create("RPC Connection Retry timer");
    le_timer_SetWakeup(RpcConnectTimerRef, false);

    le_timer_SetMsInterval(RpcConnectTimerRef, RPC_CONNECT_TIMEOUT);
    le_timer_SetHandler(RpcConnectTimerRef, RpcConnectTimerHandler);

    rpcPm.RpcRetryCount = 0;
    rpcPm.TryConnectService();

    if(rpcPm.IsRpcConnected) {
        taf_rpcPm_SetNonExitServerDisconnectHandler(tafMngdRpcPm::RpcDisconnectHandler, NULL);
        rpcPm.rpcHandlerExRef = taf_rpcPm_AddStateChangeExHandler(tafMngdRpcPm::RpcPmStateChangeExHandler, NULL);
        if (rpcPm.rpcHandlerExRef)
            LE_INFO("Register RPC Extended state change handler is successfull");
    }
    le_msg_AddServiceCloseHandler(taf_mngdPm_GetServiceRef(), tafMngdRpcPm::OnClientDisconnection, NULL);
    rpcPm.rpcWsRefPool = le_mem_CreatePool("tafrpcWsRefList", sizeof(taf_nodeWsRefCtx_t));
    rpcPm.rpcWsRefList = LE_DLS_LIST_INIT;
    rpcPm.rpcWsRefMap = le_ref_CreateMap("tafwsRef", TAF_REF_POOL_SIZE);

    rpcPm.rpcNodePowerStateChange = le_event_CreateId("rpcNodePowerStateChange", sizeof(taf_mngdPm_NodePowerStateChange_t));
    le_event_AddHandler("tafNodePowerStateChange event", rpcPm.rpcNodePowerStateChange, rpcPm.RpcNodePowerStateChanged);
    rpcPm.rpcNodePowerStateRefPool = le_mem_CreatePool("rpcNodePowerStateRef", sizeof(taf_NodePowerStateRef_t));
    rpcPm.rpcNodePowerStateHandlerMap = le_ref_CreateMap("rpcNodePowerStateHandlerMap", TAF_REF_POOL_SIZE);
    rpcPm.rpcNodePowerStateRefMap = le_ref_CreateMap("rpcNodePowerStateRefMap", TAF_REF_POOL_SIZE);
    rpcPm.rpcNodePowerStateHandlerPool = le_mem_CreatePool("rpcNodePowerStateHandlerList",
        sizeof(taf_mngdPm_NodePowerStateCtxt_t));
    rpcPm.rpcNodePowerStateHandlerList = LE_DLS_LIST_INIT;

    LE_INFO("End Init");
}

//Rpc Pms WakeSource
bool tafMngdRpcPm::IsRpcConnected = false;
le_mem_PoolRef_t tafMngdRpcPm::rpcWsRefPool;
le_dls_List_t tafMngdRpcPm::rpcWsRefList;
le_ref_MapRef_t tafMngdRpcPm::rpcWsRefMap;
uint8_t tafMngdRpcPm::rpcWsCount;

//Rpc Statchangeexhandler
taf_rpcPm_StateChangeExHandlerRef_t tafMngdRpcPm::rpcHandlerExRef = nullptr;
taf_rpcPm_PowerStateRef_t tafMngdRpcPm::rpcPowerStateRef = nullptr;
taf_rpcPm_WakeupSourceRef_t tafMngdRpcPm::rpcWs = nullptr;

//Node Power State change handler
le_event_Id_t tafMngdRpcPm::rpcNodePowerStateChange;
le_mem_PoolRef_t tafMngdRpcPm::rpcNodePowerStateHandlerPool;
le_dls_List_t tafMngdRpcPm::rpcNodePowerStateHandlerList;
le_ref_MapRef_t tafMngdRpcPm::rpcNodePowerStateHandlerMap;
le_mem_PoolRef_t tafMngdRpcPm::rpcNodePowerStateRefPool;
le_ref_MapRef_t tafMngdRpcPm::rpcNodePowerStateRefMap;
int8_t tafMngdRpcPm::clientSize;

//Rpc Connection retry
taf_mngdPm_TargetedPowerMode_t tafMngdRpcPm::rpcTargetedPowerMode;
le_timer_Ref_t tafMngdRpcPm::RpcConnectTimerRef = nullptr;
int tafMngdRpcPm::RpcRetryCount;
int8_t tafMngdRpcPm::ackRpcClientrecrdSize;
