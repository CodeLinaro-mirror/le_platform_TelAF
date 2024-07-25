/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "tafMngdPMSvc.hpp"
#include "tafMngdPMCommon.hpp"
#include "can/tafMngdPMCan.hpp"
#include "sms/tafMngdPMSms.hpp"
#include "gpio/tafMngdPMGpio.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace telux::tafsvc;

LE_MEM_DEFINE_STATIC_POOL(SessionCtx, MAX_SESSION, sizeof(taf_mngdPm_SessionNode_t));

/**
 * Sets the NAD to Targeted  power mode.
 */
le_result_t taf_mngdPm_SetNodeTargetedPowerMode(uint8_t pm_node_id,
        taf_mngdPm_TargetedPowerMode_t targetPowerMode)
{
    LE_INFO("taf_mngdPm_SetNodeTargetedPowerMode targetPowerMode : %d", targetPowerMode);

    auto &mpms = tafMngdPMSvc::GetInstance();

    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return LE_UNSUPPORTED;
    }

    if(targetPowerMode == TAF_MNGDPM_SUSPEND || targetPowerMode == TAF_MNGDPM_SHUTDOWN)
    {
        le_result_t res = tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE);
        if(res != LE_OK)
        {
            return res;
        }
    }

    mpms.targetedPowerMode = targetPowerMode;
    mpms.powerMode.isGraceful = true;

    // Acquire a wakelock to get notified on last wakeup source release.
    if (mpms.ws != nullptr)
    {
        le_result_t res = tafMngdPMSvc::AcquireWakeLock();
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
    return LE_OK;
}

/**
 * ShutDown the system with requested mode.
 */
le_result_t taf_mngdPm_ShutdownReqAsync(taf_mngdPm_ShutdownMode_t mode,
    taf_mngdPm_AsyncShutdownReqHandlerFunc_t handlerPtr, void* contextPtr)
{
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return LE_UNSUPPORTED;
    }

    TAF_ERROR_IF_RET_VAL(!handlerPtr, LE_BAD_PARAMETER, "invalid handlerRef");

    auto &mpms = tafMngdPMSvc::GetInstance();
    if(mpms.pmInf && mpms.pmInf->nodeStateChangePrepareAsync)
    {
        LE_INFO("Send shutdownReqAsync %d", HAL_PM_SHUTDOWN_MODE_NORMAL);
        (*(mpms.pmInf->nodeStateChangePrepareAsync))(NODE_ID, HAL_PM_NODE_STATE_SHUTDOWN,
                HAL_PM_SHUTDOWN_MODE_NORMAL, tafMngdPMSvc::ShutdownPrepareRespCB);
        taf_mngdPm_RequestedState_t statePtr = SYSTEM_NORMAL_SHUTDOWN;
        le_timer_SetContextPtr(mpms.vhalAckTimerRef, &statePtr);
        le_timer_Start(mpms.vhalAckTimerRef);
        mpms.shutdownCB.shutdownCallbackFunc = handlerPtr;
        mpms.shutdownCB.shutdownCBCtxPtr = contextPtr;
        mpms.shutdownCB.sessionRef = taf_mngdPm_GetClientSessionRef();
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
        handlerPtr(mode, TAF_MNGDPM_READY, contextPtr);
    }

    return LE_OK;
}

/**
 * Restarts the system with requested mode.
 */
le_result_t taf_mngdPm_RestartReqAsync(taf_mngdPm_RestartMode_t mode,
    taf_mngdPm_AsyncRestartReqHandlerFunc_t handlerPtr, void* contextPtr)
{
    auto &mpms = tafMngdPMSvc::GetInstance();

    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return LE_UNSUPPORTED;
    }

    mpms.powerMode.isRestart = true;
    TAF_ERROR_IF_RET_VAL(mpms.handlerRef == nullptr, LE_BAD_PARAMETER, "invalid handlerRef");

    if(mode == TAF_MNGDPM_RESTART_SYSTEM_OFF_ON)
    {
        if(mpms.pmInf && mpms.pmInf->nodeStateChangePrepareAsync)
        {
            LE_INFO("Send restartReqAsync %d", HAL_PM_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF);
            (*(mpms.pmInf->nodeStateChangePrepareAsync))(NODE_ID, HAL_PM_NODE_STATE_RESTART, HAL_PM_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF, tafMngdPMSvc::RestartPrepareRespCB);
            mpms.statePtr = RESTART_WITH_NAD_POWER_OFF_ON;
            le_timer_SetContextPtr(mpms.vhalAckTimerRef, &(mpms.statePtr));
            le_timer_Start(mpms.vhalAckTimerRef);
            mpms.restartCB.restartCallbackFunc = handlerPtr;
            mpms.restartCB.restartCBCtxPtr = contextPtr;
            mpms.restartCB.sessionRef = taf_mngdPm_GetClientSessionRef();
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
            handlerPtr(mode, TAF_MNGDPM_READY, contextPtr);
        }
    }

    return LE_OK;
}

/**
 * WakeupVehicleReq with requested reason.
 */
le_result_t taf_mngdPm_WakeupVehicleReqAsync(int32_t reason,
    taf_mngdPm_AsyncWakeupVehicleReqHandlerFunc_t handlerPtr, void* contextPtr)
{
    auto &mpms = tafMngdPMSvc::GetInstance();

    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return LE_UNSUPPORTED;
    }
    TAF_ERROR_IF_RET_VAL(mpms.handlerRef == nullptr, LE_BAD_PARAMETER, "invalid handlerRef");

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
            taf_mngdPm_RequestedWakeupVehicle_t wakeupMode = WAKEUP_VEHICHLE_REQ_DEFAULT;
            le_timer_SetContextPtr(mpms.wakeupVehicleTimerRef, &(wakeupMode));
            le_timer_Start(mpms.wakeupVehicleTimerRef);
            LE_INFO("Timer has started");
            mpms.wakeupVehicleCB.wakeupVehicleCallbackFunc = handlerPtr;
            mpms.wakeupVehicleCB.wakeupVehicleCBCtxPtr = contextPtr;
            mpms.wakeupVehicleCB.sessionRef = taf_mngdPm_GetClientSessionRef();
            (*(mpms.pmInf->wakeupVehicleReqAsync))(VEHICHLE_WAKEUP_REASON_DEFAULT, tafMngdPMSvc::WakeupVehicleCB);
        }
        else
        {
            LE_INFO("Returning unsupported if drive is not available");
            handlerPtr = nullptr;
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
    taf_mngdPm_WakeupType_t wakeupType;
    LE_INFO("taf_mngdPm_SetModemWakeupSource");
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
    bool inThewsWhiteList = false;
    auto &mpms = tafMngdPMSvc::GetInstance();
    if(wakeupType == TAF_MNGDPM_APP_STAYAWAKE)
    {
        inThewsWhiteList = true;
    }
    else
    {
        for (auto it = mpms.wsWhiteList.begin(); it != mpms.wsWhiteList.end(); ++it ) {
            if (*it == wakeupType) {
                LE_INFO("wakeupType found in wsWhiteList");
                inThewsWhiteList = true;
                break;
            }
        }
    }
    if(inThewsWhiteList)
    {
        taf_wsRefCtx_t * wsCtxPtr =
                (taf_wsRefCtx_t *)le_mem_ForceAlloc(mpms.wsRefPool);
        wsCtxPtr->wsRef = (taf_mngdPm_wsRef_t)le_ref_CreateRef(
                mpms.wsRefMap, wsCtxPtr);
        wsCtxPtr->vhalTag = vhalTag;
        wsCtxPtr->pmNodeId = pmNodeId;
        wsCtxPtr->wakeupType = wakeupType;
        wsCtxPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(mpms.wsRefList), &wsCtxPtr->link);
        inThewsWhiteList = false;

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

    auto &mpms = tafMngdPMSvc::GetInstance();

    le_result_t res = LE_FAULT;
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.wsRefList));
    bool ispresent = false;

    res = tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_WAKING_UP);
    if(res != LE_OK)
    {
        return res;
    }

    while (linkHandlerPtr)
    {
        taf_wsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_wsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.wsRefList), linkHandlerPtr);
        if(wsRefCtxPtr->wakeupType == TAF_MNGDPM_APP_STAYAWAKE)
        {
            ispresent = true;
            LE_INFO("WakeupType is TAF_MNGDPM_APP_STAYAWAKE");
            res = tafMngdPMSvc::AcquireWakeLock();
            //sending notification to VHAL
            if((res == LE_OK) && (wsRefCtxPtr->vhalTag != NULL) && (mpms.pmInf) && (mpms.pmInf->nodeInfoNotification))
            {
                LE_INFO("send nodeInfoNotification for vhalTag:%s", wsRefCtxPtr->vhalTag);
                (*(mpms.pmInf->nodeInfoNotification))(wsRefCtxPtr->pmNodeId,
                    HAL_PM_NODE_INFO_LOCK_ACQUIRED, wsRefCtxPtr->vhalTag);
            }
            break;
        }
        else if (wsRefCtxPtr && wsRef && wsRefCtxPtr->wsRef == wsRef)
        {
            for (auto it = mpms.wsWhiteList.begin(); it != mpms.wsWhiteList.end(); ++it ) {
                if (*it == wsRefCtxPtr->wakeupType)
                {
                    ispresent = true;
                    LE_INFO("WakeupType matched with whitelisting wakeup_source");
                    res = tafMngdPMSvc::AcquireWakeLock();
                    //sending notification to VHAL
                    if((res == LE_OK) && (wsRefCtxPtr->vhalTag != NULL) && (mpms.pmInf) && (mpms.pmInf->nodeInfoNotification))
                    {
                        LE_INFO("send nodeInfoNotification for vhalTag:%s", wsRefCtxPtr->vhalTag);
                        (*(mpms.pmInf->nodeInfoNotification))(wsRefCtxPtr->pmNodeId,
                            HAL_PM_NODE_INFO_LOCK_ACQUIRED, wsRefCtxPtr->vhalTag);
                    }
                    break;
                }
            }
        }
    }
    if(ispresent)
    {
        tafMngdPMSvc::ProcessStateChange(TAF_MNGDPM_STATE_WAKING_UP);
        return res;
    }
    else
        return LE_FAULT;
}

/**
 * Releases the acquired wake lock for the given reference.
 */
le_result_t taf_mngdPm_RelaxNode(taf_mngdPm_wsRef_t wsRef)
{
    LE_INFO("taf_mngdPm_RelaxNode");

    auto &mpms = tafMngdPMSvc::GetInstance();

    le_result_t res = LE_FAULT;
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.wsRefList));
    bool ispresent = false;

    res = tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE);
    if(res != LE_OK)
    {
        return res;
    }

    while (linkHandlerPtr)
    {
        taf_wsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_wsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.wsRefList), linkHandlerPtr);
        if ((wsRefCtxPtr->wakeupType == TAF_MNGDPM_APP_STAYAWAKE) ||
                ((wsRefCtxPtr) && wsRef && (wsRefCtxPtr->wsRef == wsRef)))
        {
            ispresent = true;
            LE_INFO("WakeupType matched with whitelisting wakeup_source");
            //sending notification to VHAL
            if((wsRefCtxPtr->vhalTag != NULL) && (mpms.pmInf) && (mpms.pmInf->nodeInfoNotification))
            {
                LE_INFO("nodeInfoNotification for vhalTag: %s", wsRefCtxPtr->vhalTag);
                (*(mpms.pmInf->nodeInfoNotification))(wsRefCtxPtr->pmNodeId, HAL_PM_NODE_INFO_LOCK_RELEASED,
                    wsRefCtxPtr->vhalTag);
            }
            res = tafMngdPMSvc::ReleaseWakeLock();
        }
        break;
    }
    if(ispresent)
    {
        tafMngdPMSvc::ProcessStateChange(TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE);
        return res;
    }
    else
        return LE_FAULT;
}

/**
 * Initiates the forceful restart for the given node.
 */
le_result_t taf_mngdPm_ShutdownNode (uint8_t pmNodeId)
{
    LE_DEBUG("taf_mngdPm_ShutdownNode pmNodeId : %d", pmNodeId);

    auto &mpms = tafMngdPMSvc::GetInstance();
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return LE_UNSUPPORTED;
    }
    le_result_t res = mpms.ShutdownNAD();
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
    if (reboot(RB_AUTOBOOT)) {
        LE_INFO("System is rebooted");
        return LE_OK;
    }
    else {
        LE_INFO("System reboot failed");
        return LE_FAULT;
    }
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
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

/**
 * Adds the client to Info Report Handler.
 */
taf_mngdPm_InfoReportHandlerRef_t taf_mngdPm_AddInfoReportHandler(taf_mngdPm_InfoReportBitMask_t infoReportMask,
        taf_mngdPm_InfoReportHandlerFunc_t handlerPtr, void* contextPtr)
{
    LE_DEBUG("AddInfoReportHandler");
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
    //save handlerref according to bitmask
    TAF_ERROR_IF_RET_VAL(handlerFuncPtr == NULL, NULL, "INVALID handler reference.");
    taf_mngdPm_NodePowerStateCtxt_t * handlerCtxPtr =
            (taf_mngdPm_NodePowerStateCtxt_t *)le_mem_ForceAlloc(mpms.nodePowerStateHandlerPool);
    handlerCtxPtr->handlerPtr = handlerFuncPtr;
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
        }
    }
}

/**
 * Gets the Info Report.
 */
le_result_t taf_mngdPm_GetInfoReport(taf_mngdPm_InfoDataId_t infoReportId, int32_t* report)
{
    LE_INFO("taf_mngdPm_GetInfoReport");
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
 * Initiates the forceful shutdown for the given node.
 */
le_result_t taf_mngdPm_SendNodePowerStateChangeAck (uint8_t pmNodeId,
        taf_mngdPm_nodePowerStateRef_t Ref, taf_mngdPm_NodePowerState_t state, taf_mngdPm_NodeClientAck_t ack)
{
    LE_INFO("taf_mngdPm_SendNodePowerStateChangeAck");
    auto &mpms = tafMngdPMSvc::GetInstance();
    // validate client record existed in state change registered clients
    for (auto it = mpms.regClientrecrd.begin(); it != mpms.regClientrecrd.end(); ++it ) {
        if (*it == (taf_mngdPm_nodePowerStateRef_t)Ref) {
            LE_INFO("Client found in record");
            break;
        }
        if (it == mpms.regClientrecrd.end()) {
            LE_INFO("Client not found in the regClientrecrd");
            return LE_FAULT;
        }
    }
    if(mpms.IsSameAsCurrentState(state, mpms.stateMachine.currentState))
    {
        if(ack == TAF_MNGDPM_CLIENT_NOT_READY)
        {
            LE_INFO("Received NACK from client");
            mpms.SendAckToPms(state, TAF_PM_NOT_READY);
            return LE_OK;
        }
        else
        {
            LE_INFO("Received ACK from client");

            mpms.ackClientrecrd.push_back((taf_mngdPm_nodePowerStateRef_t)Ref);
            LE_INFO("regClientrecrd size is %zu ,ackClientrecrd size is:%zu",mpms.regClientrecrd.size(),
                    mpms.ackClientrecrd.size());
            //If Last acknowledged client , proceed for ack state change
            if(mpms.regClientrecrd.size() == mpms.ackClientrecrd.size())
            {
                mpms.clientSize = 0;
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
    mpms.nodePowerStateRefPool = le_mem_CreatePool("nodePowerStateHandlerList", sizeof(taf_NodePowerStateRef_t));
    mpms.nodePowerStateHandlerMap = le_ref_CreateMap("nodePowerStateHandlerMap", TAF_REF_POOL_SIZE);
    mpms.nodePowerStateRefMap = le_ref_CreateMap("nodePowerStateRefMap", TAF_REF_POOL_SIZE);
    mpms.nodePowerStateHandlerPool = le_mem_CreatePool("nodePowerStateHandlerList",
        sizeof(taf_mngdPm_NodePowerStateCtxt_t));
    mpms.nodePowerStateHandlerList = LE_DLS_LIST_INIT;
    mpms.nodePowerStateHandlerMap = le_ref_CreateMap("tafNodePowerStateHandler",
        TAF_REF_POOL_SIZE);

    try
    {
        le_result_t res = tafMngdPMSvc::ParseJsonConfig(TAF_MNGDPM_CONFIG_PATH);
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
        LE_ERROR("Exception while parsing the JSON");
    }
    mpms.wsRefPool = le_mem_CreatePool("tafwsRefList", sizeof(taf_wsRefCtx_t));
    mpms.wsRefList = LE_DLS_LIST_INIT;
    mpms.wsRefMap = le_ref_CreateMap("tafwsRef", TAF_REF_POOL_SIZE);
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

    res = tafMngdPMSvc::InitVHalModule();
    if((res == LE_OK) && (mpms.pmInf) && (mpms.pmInf->addNodeEventHandler))
    {
        LE_INFO("addNodeEventHanlder for node %d", NODE_ID);
        (*(mpms.pmInf->addNodeEventHandler))(NODE_ID, tafMngdPMSvc::NodeEventCB);
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
    LE_INFO("COMPONENT end init");
}
