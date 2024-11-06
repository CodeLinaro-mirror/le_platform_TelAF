/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDiagSvr.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of Diag server.
 */
//--------------------------------------------------------------------------------------------------
taf_DiagSvr &taf_DiagSvr::GetInstance
(
)
{
    static taf_DiagSvr instance;

    return instance;
}

//-------------------------------------------------------------------------------------------------
/**
 * Sets an enable condition.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DiagSvr::SetEnableCondition
(
    uint8_t enableConditionID,
    bool conditionFulfilled
)
{
    LE_DEBUG("SetEnableCondition!");

    auto &diag = taf_DiagSvr::GetInstance();
    bool isEnableIdAvailable = false;

    // Check enable id already present.
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&enableStatusList);
    while (linkPtr)
    {
        taf_DiagEnableStatus_t* enableCtxPtr = CONTAINER_OF(linkPtr, taf_DiagEnableStatus_t,
                link);
        linkPtr = le_dls_PeekNext(&enableStatusList, linkPtr);

        if (enableCtxPtr->enableConditionID == enableConditionID)
        {
            LE_DEBUG("Get enableConditionID %p by id %d", enableCtxPtr, enableConditionID);
            enableCtxPtr->conditionFulfilled = conditionFulfilled;
            isEnableIdAvailable = true;
            break;
        }
    }

    if (!isEnableIdAvailable)
    {
        taf_DiagEnableStatus_t* enableStatusPtr = NULL;
        enableStatusPtr = (taf_DiagEnableStatus_t *)le_mem_ForceAlloc(diag.EnableMemPool);

        enableStatusPtr->enableConditionID = enableConditionID;
        enableStatusPtr->conditionFulfilled = conditionFulfilled;
        enableStatusPtr->link = LE_DLS_LINK_INIT;

        // add this event context to list
        le_dls_Queue(&diag.enableStatusList, &enableStatusPtr->link);
    }

    // Set event enable condition status
    auto& diagEvent = taf_EventSvr::GetInstance();
    diagEvent.SetEventEnableStatus(enableConditionID);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get an enable condition status.
 */
//-------------------------------------------------------------------------------------------------
bool taf_DiagSvr::GetEnableConditionStatus
(
    uint8_t enableConditionID
)
{
    LE_DEBUG("GetEnableConditionStatus!");

    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&enableStatusList);
    while (linkPtr)
    {
        taf_DiagEnableStatus_t* enableCtxPtr = CONTAINER_OF(linkPtr, taf_DiagEnableStatus_t,
                link);
        linkPtr = le_dls_PeekNext(&enableStatusList, linkPtr);

        if (enableCtxPtr->enableConditionID == enableConditionID)
        {
            LE_DEBUG("Get enableConditionID %p by id %d", enableCtxPtr, enableConditionID);
            return enableCtxPtr->conditionFulfilled;
        }
    }

    LE_DEBUG("Requested enableCondition ID not found : %d", enableConditionID);
    return false;
}


//-------------------------------------------------------------------------------------------------
/**
 * Create a reference for the service, or get the reference of a service if the reference already
 * exist.
 */
//-------------------------------------------------------------------------------------------------
taf_diag_ServiceRef_t taf_DiagSvr::GetService
(
)
{
    LE_DEBUG("GetService");

    // Search the service.
    taf_DiagSvc_t* servicePtr = GetServiceObj(taf_diag_GetClientSessionRef());

    // Create a service object if it doesn't exist in the list.
    if (servicePtr == NULL)
    {
        servicePtr = (taf_DiagSvc_t *)le_mem_ForceAlloc(SvcPool);
        memset(servicePtr, 0, sizeof(taf_DiagSvc_t));

        // Attach the service to the client.
        servicePtr->sessionRef = taf_diag_GetClientSessionRef();
        LE_INFO("GetService servicePtr->sessionRef: %p", servicePtr->sessionRef);

        // Init message list.
        servicePtr->testerStateHandlerList = LE_DLS_LIST_INIT;

        // Create a Safe Reference for this service object
        servicePtr->svcRef = (taf_diag_ServiceRef_t)le_ref_CreateRef(SvcRefMap, servicePtr);

        LE_INFO("svcRef %p of client %p is created",
                servicePtr->svcRef, servicePtr->sessionRef);
    }

    LE_INFO("Get serviceRef %p for Diag service.", servicePtr->svcRef);
    return servicePtr->svcRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the service instacnce object, if the service reference already created and return the
 * object pointer.
 */
//-------------------------------------------------------------------------------------------------
taf_DiagSvc_t* taf_DiagSvr::GetServiceObj
(
    le_msg_SessionRef_t sessionRef
)
{
    LE_DEBUG("find the service object!");
    LE_INFO("GetServiceObj sessionRef: %p", sessionRef);

    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_DiagSvc_t* servicePtr = (taf_DiagSvc_t *)le_ref_GetValue(iterRef);
        if ((servicePtr != NULL) && (servicePtr->sessionRef == sessionRef))
        {
            return servicePtr;
        }
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * UDS stack handler function
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagSvr::UDSMsgHandler
(
    const taf_uds_AddrInfo_t* addrPtr,
    uint8_t sid,
    uint8_t* msgPtr,
    size_t msgLen
)
{
    LE_DEBUG("UDSMsgHandler!");

    TAF_ERROR_IF_RET_NIL(addrPtr == NULL, "Invalid addrPtr");
    TAF_ERROR_IF_RET_NIL(msgPtr == NULL, "Invalid msgPtr");

    if (sid != stateChangeId)
    {
        LE_ERROR("Invalid notification");
        return;
    }

    if (msgPtr[1] != msgPtr[2])
    {
        LE_INFO("previous state: %d, current state :%d",msgPtr[1], msgPtr[2]);
        LE_INFO("DiagSvc VlanId = %d",addrPtr->vlanId);
        taf_RxTesterStateMsg_t* rxStatePtr = NULL;

        rxStatePtr = (taf_RxTesterStateMsg_t*)le_mem_ForceAlloc(RxMsgPool);
        memset(rxStatePtr, 0, sizeof(taf_RxTesterStateMsg_t));

        memcpy(&rxStatePtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
        rxStatePtr->preTesterState = (taf_diag_State_t)msgPtr[1];
        rxStatePtr->currentTesterState = (taf_diag_State_t)msgPtr[2];
        rxStatePtr->rxStateRef = (taf_diag_TesterStateRef_t)le_ref_CreateRef(RxTesterStateRefMap,
                rxStatePtr);

        LE_DEBUG("Receive message(%p) and current testert state: 0x%x)", rxStatePtr->rxStateRef,
                rxStatePtr->currentTesterState);

        // Report the request message to message handler in service layer.
        le_event_ReportWithRefCounting(TesterStateEvent, rxStatePtr);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a Rx handler.
 */
//-------------------------------------------------------------------------------------------------
taf_diag_TesterStateHandlerRef_t taf_DiagSvr::AddTesterStateHandler
(
    taf_diag_ServiceRef_t svcRef,
    uint16_t vlanId,
    taf_diag_StateChangeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_DEBUG("AddTesterStateHandler");

    taf_DiagSvc_t* servicePtr = (taf_DiagSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, NULL, "Invalid service reference provided");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    // Check given VlanId already registered the handler.
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&servicePtr->testerStateHandlerList);
    while (linkPtr)
    {
        taf_TesterStateHandler_t* handlerObjPtr = CONTAINER_OF(linkPtr, taf_TesterStateHandler_t,
                link);
        linkPtr = le_dls_PeekNext(&servicePtr->testerStateHandlerList, linkPtr);
        if (handlerObjPtr->handlerRef && (handlerObjPtr->vlanId == vlanId))
        {
            LE_DEBUG("Handler is already registerred for Vlan Id: %d", vlanId);
            return handlerObjPtr->handlerRef;
        }
    }

    // Register handler for given Vlan id.
    taf_TesterStateHandler_t* handlerObjPtr = NULL;
    handlerObjPtr = (taf_TesterStateHandler_t*)le_mem_ForceAlloc(ReqHandlerPool);

    // Initialize the RxHandler object.
    handlerObjPtr->svcRef     = svcRef;
    handlerObjPtr->vlanId     = vlanId;
    handlerObjPtr->func       = handlerPtr;
    handlerObjPtr->ctxPtr     = contextPtr;
    handlerObjPtr->handlerRef =
            (taf_diag_TesterStateHandlerRef_t)le_ref_CreateRef(ReqHandlerRefMap, handlerObjPtr);
    handlerObjPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->testerStateHandlerList, &handlerObjPtr->link);

    LE_INFO("Registered Tester present state handler for vlanId : %d", vlanId);

    return handlerObjPtr->handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Tester state event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagSvr::TesterStateEventHandler
(
    void* reportPtr
)
{
    LE_DEBUG("TesterStateEventHandler");

    auto &diag = taf_DiagSvr::GetInstance();

    taf_RxTesterStateMsg_t* rxStatePtr = (taf_RxTesterStateMsg_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(rxStatePtr == NULL, "rxStatePtr is Null");

    LE_DEBUG("VlanID: %d",rxStatePtr->addrInfo.vlanId);

    // Notify the repective callbackFunc
    taf_DiagSvc_t* servicePtr = NULL;
    taf_TesterStateHandler_t* handlerObjPtr = NULL;
    le_ref_IterRef_t iterRef = le_ref_GetIterator(diag.SvcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        servicePtr = (taf_DiagSvc_t *)le_ref_GetValue(iterRef);
        if (servicePtr != NULL)
        {
            le_dls_Link_t* linkHandlerPtr = NULL;
            linkHandlerPtr = le_dls_Peek(&servicePtr->testerStateHandlerList);
            while(linkHandlerPtr)
            {
                handlerObjPtr = CONTAINER_OF(linkHandlerPtr,
                        taf_TesterStateHandler_t, link);
                linkHandlerPtr = le_dls_PeekNext(&servicePtr->testerStateHandlerList,
                        linkHandlerPtr);
                if((handlerObjPtr->func != NULL) && (handlerObjPtr->svcRef == servicePtr->svcRef)
                        && (handlerObjPtr->vlanId == rxStatePtr->addrInfo.vlanId))
                {
                    // Add the message in service message list and notify to application.
                    LE_DEBUG("current state :%d", rxStatePtr->currentTesterState);
                    LE_DEBUG("handlerObjPtr->svcRef :%p, handlerObjPtr->handlerRef :%p",
                            handlerObjPtr->svcRef, handlerObjPtr->handlerRef);

                    // Call the callback function
                    handlerObjPtr->func(rxStatePtr->rxStateRef, handlerObjPtr->vlanId,
                            rxStatePtr->currentTesterState, handlerObjPtr->ctxPtr);
                }
            }
        }
    }

    LE_INFO("Release the received tester state message ptr");
    le_ref_DeleteRef(diag.RxTesterStateRefMap, rxStatePtr->rxStateRef);
    le_mem_Release(rxStatePtr);

    return;
}

le_result_t taf_DiagSvr::ReleaseTesterStateMsg
(
    taf_diag_TesterStateRef_t stateRef
)
{
    LE_INFO("ReleaseTesterStateMsg");
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the registered handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagSvr::RemoveTesterStateHandler
(
    taf_diag_TesterStateHandlerRef_t handlerRef
)
{
    LE_DEBUG("RemoveTesterStateHandler");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");

    taf_DiagSvc_t* servicePtr = NULL;
    taf_TesterStateHandler_t* handlerObjPtr = NULL;

    handlerObjPtr = (taf_TesterStateHandler_t*)le_ref_Lookup(ReqHandlerRefMap,
            handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerObjPtr == NULL, "Invalid handlerObjPtr");

    servicePtr = (taf_DiagSvc_t*)le_ref_Lookup(SvcRefMap, handlerObjPtr->svcRef);
    if (servicePtr == NULL)
    {
        LE_WARN("The handler is not belong to this service.");
        le_ref_DeleteRef(ReqHandlerRefMap, handlerRef);
        le_mem_Release(handlerObjPtr);

        return;
    }

    // Clear Rx Handler resources
    handlerObjPtr->handlerRef = NULL;
    handlerObjPtr->svcRef     = NULL;
    handlerObjPtr->func       = NULL;
    handlerObjPtr->ctxPtr     = NULL;

    // Free the handler.
    le_ref_DeleteRef(ReqHandlerRefMap, handlerRef);
    le_dls_Remove(&servicePtr->testerStateHandlerList, &(handlerObjPtr->link));
    le_mem_Release(handlerObjPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear handler list.
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagSvr::ClearHandlerList
(
    taf_DiagSvc_t* servicePtr
)
{
    LE_DEBUG("ClearHandlerList");
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear the handler list.
    le_dls_Link_t* linkHandlerPtr = le_dls_Pop(&servicePtr->testerStateHandlerList);
    while (linkHandlerPtr != NULL)
    {
        taf_TesterStateHandler_t* handlerObjPtr =
                CONTAINER_OF(linkHandlerPtr, taf_TesterStateHandler_t, link);
        if (handlerObjPtr != NULL)
        {
            LE_DEBUG("Release ReqMsg(ref=%p)", handlerObjPtr->handlerRef);
            // Free the message
            le_ref_DeleteRef(ReqHandlerRefMap, handlerObjPtr->handlerRef);
            le_mem_Release(handlerObjPtr);
        }

        // Process next node.
        linkHandlerPtr = le_dls_Pop(&servicePtr->testerStateHandlerList);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the created service and release the alloted memory.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DiagSvr::RemoveSvc
(
    taf_diag_ServiceRef_t svcRef
)
{
    LE_DEBUG("RemoveSvc");

    taf_DiagSvc_t* servicePtr = (taf_DiagSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid servicePtr");

    // Release registered handler message resources.
    ClearHandlerList(servicePtr);

    // Clear service object
    le_ref_DeleteRef(SvcRefMap, (void*)servicePtr->svcRef);
    le_mem_Release(servicePtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Client session close handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagSvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    LE_DEBUG("OnClientDisconnection");

    auto &diag = taf_DiagSvr::GetInstance();

    // Clear service object and handler
    le_ref_IterRef_t iterRef = le_ref_GetIterator(diag.SvcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_DiagSvc_t* servicePtr = (taf_DiagSvc_t *)le_ref_GetValue(iterRef);
        LE_ASSERT(servicePtr != NULL);
        LE_ASSERT(servicePtr->svcRef == le_ref_GetSafeRef(iterRef));

        if (servicePtr->sessionRef == sessionRef)
        {
            // Release registered handler message resources.
            diag.ClearHandlerList(servicePtr);

            // Clear service object
            le_ref_DeleteRef(diag.SvcRefMap, (void*)servicePtr->svcRef);
            le_mem_Release(servicePtr);
        }
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagSvr::Init
(
    void
)
{
    LE_INFO("taf_DiagSvr Init!");

    // Create memory pools.
    EnableMemPool = le_mem_CreatePool("EnableConditionMemPool", sizeof(taf_DiagEnableStatus_t));

    // Create memory pools.
    SvcPool = le_mem_CreatePool("DiagSvcPool", sizeof(taf_DiagSvc_t));
    RxMsgPool = le_mem_CreatePool("RxMsgPool", sizeof(taf_RxTesterStateMsg_t));
    ReqHandlerPool = le_mem_CreatePool("ReqHandlerPool", sizeof(taf_TesterStateHandler_t));

    // Create reference maps
    SvcRefMap = le_ref_CreateMap("DiagSvcRefMap", DEFAULT_SVC_REF_CNT);
    RxTesterStateRefMap = le_ref_CreateMap("TesterStateRefMap", DEFAULT_RX_TESTER_STATE_REF_CNT);
    ReqHandlerRefMap = le_ref_CreateMap("ReqHandlerRefMap", DEFAULT_RX_HANDLER_REF_CNT);

    // Create event and add the event handler.
    TesterStateEvent = le_event_CreateIdWithRefCounting("TesterStateEvent");
    TesterStateEventHandlerRef = le_event_AddHandler("TesterStateEventHandlerRef",
            TesterStateEvent, taf_DiagSvr::TesterStateEventHandler);

    // Set client session close handler.
    le_msg_AddServiceCloseHandler(taf_diag_GetServiceRef(), OnClientDisconnection, NULL);

    auto& backend = taf_DiagBackend::GetInstance();
    backend.RegisterUdsService(stateChangeId, this);
}
