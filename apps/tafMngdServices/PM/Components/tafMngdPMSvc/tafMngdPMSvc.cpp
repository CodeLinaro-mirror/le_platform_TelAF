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
 * Set the NAD to requested power state.
 */
le_result_t taf_mngd_pm_SetNadPowerState(taf_mngd_pm_Nad_t nad, taf_mngd_pm_State_t state)
{
    LE_DEBUG("taf_mngd_pm_setNadPowerState state : %s", tafMngdPMSvc::TafStateToString(state));

    auto &mpms = tafMngdPMSvc::GetInstance();

    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return LE_UNSUPPORTED;
    }

    le_result_t res = taf_pm_SetAllVMPowerState((taf_pm_State_t)state);
    if(res != LE_OK)
    {
        LE_ERROR("Failed to set the NAD power state");
    }
    else
    {
        le_hashmap_It_Ref_t hashIter = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(mpms.vmStateHashmap);
        while (LE_OK == le_hashmap_NextNode(hashIter))
        {
            taf_mngdPm_State_t *vmStatePtr = (taf_mngdPm_State_t*)le_hashmap_GetValue(hashIter);

            if(vmStatePtr) {
                vmStatePtr->state = state;
            }
        }
    }
    return res;
}
/**
 * Set the VM to requested power state
 */

le_result_t taf_mngd_pm_SetVMPowerState(taf_mngd_pm_Nad_t nad, const char *machineName,
        taf_mngd_pm_State_t state)
{
    LE_DEBUG("taf_mngd_pm_SetVMPowerState VM : %s state : %s", machineName,
            tafMngdPMSvc::TafStateToString(state));

    auto &mpms = tafMngdPMSvc::GetInstance();

    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return LE_UNSUPPORTED;
    }

    TAF_ERROR_IF_RET_VAL(state == TAF_MNGD_PM_STATE_UNKNOWN || machineName == NULL,
            LE_BAD_PARAMETER, "state or machineName is not valid");

    char name[32] = {0};
    taf_pm_VMListRef_t vmListRef = taf_pm_GetMachineList( );

    TAF_ERROR_IF_RET_VAL(!vmListRef, LE_UNSUPPORTED, "Machine list not available");

    le_result_t res = taf_pm_GetFirstMachineName(vmListRef, name, 32);
    while(res == LE_OK)
    {
        if(machineName && strncmp(machineName, name, TAF_MNGD_PM_MACHINE_NAME_LEN) == 0)
        {
            taf_mngdPm_State_t *vmStatePtr =
                    (taf_mngdPm_State_t*)le_hashmap_Get(mpms.vmStateHashmap, name);
            TAF_ERROR_IF_RET_VAL(!vmStatePtr, LE_BAD_PARAMETER, "Machine name not available");

            // return LE_BAD_PARAMETER when received request for the same current power state.
            TAF_ERROR_IF_RET_VAL(vmStatePtr->state == state, LE_BAD_PARAMETER,
                    "Requested the already existing state");

            res = taf_pm_SetVMPowerState((taf_pm_State_t)state, machineName);
            if(res != LE_OK)
            {
                LE_ERROR("Failed to trigger state change of %s VM", machineName);
                return res;
            }
            else
            {
                LE_INFO("Successfully requested state change for %s VM", machineName);
                taf_pm_DeleteMachineList(vmListRef);
                le_hashmap_It_Ref_t hashIter =
                        (le_hashmap_It_Ref_t)le_hashmap_GetIterator(mpms.vmStateHashmap);
                while (LE_OK == le_hashmap_NextNode(hashIter))
                {
                    taf_mngdPm_State_t *vmStatePtr =
                            (taf_mngdPm_State_t*)le_hashmap_GetValue(hashIter);
                    if(vmStatePtr)
                    {
                        if(strncmp(vmStatePtr->vmName, machineName,
                                TAF_MNGD_PM_MACHINE_NAME_LEN) == 0)
                        {
                            LE_DEBUG("Update %s VM state to %s", vmStatePtr->vmName,
                                    tafMngdPMSvc::TafStateToString(state));
                            vmStatePtr->state = state;
                        }
                    }
                }
                return res;
            }
        }
        res = taf_pm_GetNextMachineName(vmListRef, name, 32);
    }
    taf_pm_DeleteMachineList(vmListRef);
    LE_ERROR("machineName %s provided is not available", machineName);
    return LE_BAD_PARAMETER;
}

/**
 * Gets the VM current power state.
 */
le_result_t taf_mngd_pm_GetVMPowerState(taf_mngd_pm_Nad_t nad, const char *machineName,
        taf_mngd_pm_State_t *state)
{
    LE_DEBUG("taf_mngd_pm_GetVMPowerState %s state", machineName);

    auto &mpms = tafMngdPMSvc::GetInstance();

    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return LE_UNSUPPORTED;
    }

    TAF_ERROR_IF_RET_VAL(state == NULL || machineName == NULL, LE_BAD_PARAMETER,
            "state or machineName is not valid");

    le_hashmap_It_Ref_t hashIter = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(mpms.vmStateHashmap);
    while (LE_OK == le_hashmap_NextNode(hashIter))
    {
        taf_mngdPm_State_t *vmStatePtr = (taf_mngdPm_State_t*)le_hashmap_GetValue(hashIter);

        if(vmStatePtr)
        {
            if(strncmp(vmStatePtr->vmName, machineName, TAF_MNGD_PM_MACHINE_NAME_LEN) == 0)
            {
                LE_DEBUG("Update %s state", vmStatePtr->vmName);
                *state = vmStatePtr->state;
                return LE_OK;
            }
        }
    }
    LE_ERROR("machineName %s provided is not available", machineName);
    return LE_BAD_PARAMETER;
}

/**
 * Sets the NAD to Targeted  power mode.
 */
le_result_t taf_mngd_pm_SetNodeTargetedPowerMode(uint8_t pm_node_id,
        taf_mngd_pm_TargetedPowerMode_t targetPowerMode)
{
    LE_INFO("taf_mngd_pm_SetNodeTargetedPowerMode targetPowerMode : %d", targetPowerMode);

    auto &mpms = tafMngdPMSvc::GetInstance();

    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return LE_UNSUPPORTED;
    }

    if(targetPowerMode == TAF_MNGD_PM_SUSPEND || targetPowerMode == TAF_MNGD_PM_SHUTDOWN)
    {
        le_result_t res = tafMngdPMSvc::RequestStateChange(TAF_MNGD_PM_STATE_RELEASING_WAKE_SOURCE);
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
        le_result_t res = taf_pm_StayAwake(mpms.ws);
        if(res == LE_OK)
            LE_DEBUG("Wake source acquired successfully");
        res = taf_pm_Relax(mpms.ws);
        if(res == LE_OK)
            LE_DEBUG("Wake source released successfully");
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
le_result_t taf_mngd_pm_ShutdownReqAsync(taf_mngd_pm_ShutdownMode_t mode,
    taf_mngd_pm_AsyncShutdownReqHandlerFunc_t handlerPtr, void* contextPtr)
{
    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return LE_UNSUPPORTED;
    }

    TAF_ERROR_IF_RET_VAL(!handlerPtr, LE_BAD_PARAMETER, "invalid handlerRef");

    auto &mpms = tafMngdPMSvc::GetInstance();
    if(mpms.pmInf)
    {
        LE_INFO("Send shutdownReqAsync %d", PM_HAL_SHUTDOWN_MODE_FORCEFUL);
        (*(mpms.pmInf->shutdownReqAsync))(PM_HAL_SHUTDOWN_MODE_FORCEFUL, tafMngdPMSvc::ShutdownRespCB);
        taf_mngdPm_RequestedState_t statePtr = SYSTEM_FORCEFUL_SHUTDOWN;
        le_timer_SetContextPtr(mpms.vhalAckTimerRef, &statePtr);
        le_timer_Start(mpms.vhalAckTimerRef);
        mpms.shutdownCB.shutdownCallbackFunc = handlerPtr;
        mpms.shutdownCB.shutdownCBCtxPtr = contextPtr;
        mpms.shutdownCB.sessionRef = taf_mngd_pm_GetClientSessionRef();
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
        handlerPtr(mode, TAF_MNGD_PM_READY, contextPtr);
    }

    return LE_OK;
}

/**
 * Restarts the system with requested mode.
 */
le_result_t taf_mngd_pm_RestartReqAsync(taf_mngd_pm_RestartMode_t mode,
    taf_mngd_pm_AsyncRestartReqHandlerFunc_t handlerPtr, void* contextPtr)
{
    auto &mpms = tafMngdPMSvc::GetInstance();

    if(tafMngdPMSvc::IsClientValid() == false)
    {
        return LE_UNSUPPORTED;
    }

    mpms.powerMode.isRestart = true;
    TAF_ERROR_IF_RET_VAL(mpms.handlerRef == nullptr, LE_BAD_PARAMETER, "invalid handlerRef");

    if(mode == TAF_MNGD_PM_RESTART_SYSTEM_OFF_ON)
    {
        if(mpms.pmInf)
        {
            LE_INFO("Send restartReqAsync %d", PM_HAL_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF);
            (*(mpms.pmInf->restartReqAsync))(PM_HAL_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF, tafMngdPMSvc::RestartRespCB);
            mpms.statePtr = RESTART_WITH_NAD_POWER_OFF_ON;
            le_timer_SetContextPtr(mpms.vhalAckTimerRef, &(mpms.statePtr));
            le_timer_Start(mpms.vhalAckTimerRef);
            mpms.restartCB.restartCallbackFunc = handlerPtr;
            mpms.restartCB.restartCBCtxPtr = contextPtr;
            mpms.restartCB.sessionRef = taf_mngd_pm_GetClientSessionRef();
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
            handlerPtr(mode, TAF_MNGD_PM_READY, contextPtr);
        }
    }

    return LE_OK;
}

/**
 * Sets the Modem wakeupSource type.
 */
le_result_t taf_mngd_pm_SetModemWakeupSource (
        uint32_t wakeupSource)
{
    taf_mngd_pm_WakeupType_t wakeupType;
    LE_INFO("taf_mngd_pm_SetModemWakeupSource");
    if((wakeupSource & (1)) != 0)
    {
        LE_INFO("wakeupType is SMS");
        wakeupType = TAF_MNGD_PM_SMS;
        tafMngdPMSvc::SetModemWakeupSource(wakeupType);
    }
    if((wakeupSource & (1 << 1)) != 0)
    {
        LE_INFO("wakeupType is VOICE_CALL");
        wakeupType = TAF_MNGD_PM_VOICE_CALL;
        tafMngdPMSvc::SetModemWakeupSource(wakeupType);
    }
    if((wakeupSource & (1 << 2)) != 0)
    {
        LE_INFO("wakeupType is MCU_VHAL");
        wakeupType = TAF_MNGD_PM_MCU_VHAL;
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
taf_mngd_pm_wsRef_t taf_mngd_pm_NewNodeWakeupSource( uint8_t pmNodeId,
    taf_mngd_pm_WakeupType_t wakeupType, const char* vhalTag)
{
    LE_INFO("taf_mngd_pm_NewNodeWakeupSource");
    bool inThewsWhiteList = false;
    auto &mpms = tafMngdPMSvc::GetInstance();

    for (auto it = mpms.wsWhiteList.begin(); it != mpms.wsWhiteList.end(); ++it ) {
        if (*it == wakeupType) {
            LE_INFO("wakeupType found in wsWhiteList");
            inThewsWhiteList = true;
            break;
        }
    }
    if(inThewsWhiteList)
    {
        taf_wsRefCtx_t * wsCtxPtr =
                (taf_wsRefCtx_t *)le_mem_ForceAlloc(mpms.wsRefPool);
        wsCtxPtr->wsRef = (taf_mngd_pm_wsRef_t)le_ref_CreateRef(
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
le_result_t taf_mngd_pm_StayAwakeNode(taf_mngd_pm_wsRef_t wsRef)
{
    LE_INFO("taf_mngd_pm_StayAwakeNode");

    auto &mpms = tafMngdPMSvc::GetInstance();

    le_result_t res = LE_FAULT;
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.wsRefList));
    bool ispresent = false;

    res = tafMngdPMSvc::RequestStateChange(TAF_MNGD_PM_STATE_WAKING_UP);
    if(res != LE_OK)
    {
        return res;
    }

    while (linkHandlerPtr)
    {
        taf_wsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_wsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.wsRefList), linkHandlerPtr);
        if (wsRefCtxPtr && wsRef && wsRefCtxPtr->wsRef == wsRef)
        {
            for (auto it = mpms.wsWhiteList.begin(); it != mpms.wsWhiteList.end(); ++it ) {
                if (*it == wsRefCtxPtr->wakeupType) {
                    ispresent = true;
                    LE_INFO("WakeupType matched with whitelisting wakeup_source");
                    mpms.wsCount++;
                    res = tafMngdPMSvc::AcquireWakeLock();
                    //sending notification to VHAL
                    if(res == LE_OK && wsRefCtxPtr->vhalTag != NULL && mpms.pmInf)
                    {
                        LE_INFO("send nodeInfoNotification for vhalTag:%s", wsRefCtxPtr->vhalTag);
                        (*(mpms.pmInf->nodeInfoNotification))(wsRefCtxPtr->pmNodeId,
                            PM_HAL_NODE_INFO_LOCK_ACQUIRED, wsRefCtxPtr->vhalTag);
                    }
                }
            }
        }
    }
    if(ispresent)
    {
        tafMngdPMSvc::ProcessStateChange(TAF_MNGD_PM_STATE_WAKING_UP);
        return res;
    }
    else
        return LE_FAULT;
}

/**
 * Releases the acquired wake lock for the given reference.
 */
le_result_t taf_mngd_pm_RelaxNode(taf_mngd_pm_wsRef_t wsRef)
{
    LE_INFO("taf_mngd_pm_RelaxNode");

    auto &mpms = tafMngdPMSvc::GetInstance();

    le_result_t res = LE_FAULT;
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.wsRefList));
    bool ispresent = false;

    res = tafMngdPMSvc::RequestStateChange(TAF_MNGD_PM_STATE_RELEASING_WAKE_SOURCE);
    if(res != LE_OK)
    {
        return res;
    }

    while (linkHandlerPtr)
    {
        taf_wsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_wsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.wsRefList), linkHandlerPtr);
        if ((wsRefCtxPtr) && wsRef && (wsRefCtxPtr->wsRef == wsRef))
        {
            ispresent = true;
            LE_INFO("WakeupType matched with whitelisting wakeup_source");
            //sending notification to VHAL
            if(wsRefCtxPtr->vhalTag != NULL && mpms.pmInf)
            {
                LE_INFO("nodeInfoNotification for vhalTag: %s", wsRefCtxPtr->vhalTag);
                (*(mpms.pmInf->nodeInfoNotification))(wsRefCtxPtr->pmNodeId, PM_HAL_NODE_INFO_LOCK_RELEASED,
                    wsRefCtxPtr->vhalTag);
                if(mpms.wsCount > 0)
                {
                    mpms.wsCount--;
                }
            res = tafMngdPMSvc::ReleaseWakeLock();
            }
        }
    }
    if(ispresent)
    {
        tafMngdPMSvc::ProcessStateChange(TAF_MNGD_PM_STATE_RELEASING_WAKE_SOURCE);
        return res;
    }
    else
        return LE_FAULT;
}

/**
 * Adds the client to StateChangeHandler.
 */
taf_mngd_pm_StateChangeHandlerRef_t taf_mngd_pm_AddStateChangeHandler
(
    taf_mngd_pm_StateChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    auto &mpms = tafMngdPMSvc::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("MPMSStateChangeHandler",
        mpms.stateChange, tafMngdPMSvc::StateLayeredHandler, (void*)handlerFuncPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);
    return (taf_mngd_pm_StateChangeHandlerRef_t)handlerRef;
}

/**
 * Removes the client from StateChangeHandler.
 */
void taf_mngd_pm_RemoveStateChangeHandler(taf_mngd_pm_StateChangeHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
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

    le_msg_AddServiceOpenHandler(taf_mngd_pm_GetServiceRef(), tafMngdPMSvc::OnClientConnection,
            NULL);

    le_msg_AddServiceCloseHandler(taf_mngd_pm_GetServiceRef(), tafMngdPMSvc::OnClientDisconnection,
            NULL);

    mpms.stateChange = le_event_CreateId("stateChange", sizeof(taf_mngd_pm_StateInd_t));

    try
    {
        le_result_t res = tafMngdPMSvc::ParseJsonConfig(TAF_MNGD_PM_CONFIG_PATH);
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
    mpms.wsRefMap = le_ref_CreateMap("tafwsRef", TAF_WAKE_SOURCE_REF_POOL_SIZE);
    mpms.vmStatePool = le_mem_CreatePool("VMStatePool", sizeof(taf_mngdPm_State_t));
    mpms.vmStateHashmap = le_hashmap_Create("VMStateHashMap", TAF_MNGD_PM_VM_HASH_SIZE,
            le_hashmap_HashString, le_hashmap_EqualsString);
    char name[32] = {0};
    taf_pm_VMListRef_t vmListRef = taf_pm_GetMachineList( );
    le_result_t res;
    if(vmListRef)
    {
        res = taf_pm_GetFirstMachineName(vmListRef, name, 32);
        while(res == LE_OK)
        {
            taf_mngdPm_State_t *vmStatePtr = (taf_mngdPm_State_t*)le_mem_ForceAlloc(mpms.vmStatePool);
            memset(vmStatePtr, 0, sizeof(taf_mngdPm_State_t));
            vmStatePtr->nad = TAF_MNGD_PM_NAD1;
            memset(vmStatePtr->vmName, 0, sizeof(vmStatePtr->vmName));
            le_utf8_Copy(vmStatePtr->vmName, name, TAF_MNGD_PM_MACHINE_NAME_LEN, NULL);
            vmStatePtr->state = TAF_MNGD_PM_STATE_RESUME;
            LE_INFO("Add %s machine to hashmap", vmStatePtr->vmName);
            le_hashmap_Put(mpms.vmStateHashmap, vmStatePtr->vmName, vmStatePtr);
            res = taf_pm_GetNextMachineName(vmListRef, name, 32);
        }
        taf_pm_DeleteMachineList(vmListRef);
    }
    mpms.handlerRef = taf_pm_AddStateChangeHandler(tafMngdPMSvc::StateChangeHandler, NULL);
    if (mpms.handlerRef)
        LE_INFO("Register state change handler is successfull");

    mpms.stateMachine.currentState = TAF_MNGD_PM_STATE_RESUME;

    res = tafMngdPMSvc::InitVHalModule();
    if((res == LE_OK) && (mpms.pmInf))
    {
        LE_INFO("addNodeEventHanlder for node %d", NODE_ID);
        (*(mpms.pmInf->addNodeEventHandler))(NODE_ID, tafMngdPMSvc::NodeEventCB);
    }
    mpms.handlerExRef = taf_pm_AddStateChangeExHandler(tafMngdPMSvc::StateChangeExHandler, NULL);
    if (mpms.handlerExRef)
        LE_INFO("Register Extended state change handler is successfull");

    mpms.WaitWakeSourceTimer();
    LE_INFO("COMPONENT end init");
}
