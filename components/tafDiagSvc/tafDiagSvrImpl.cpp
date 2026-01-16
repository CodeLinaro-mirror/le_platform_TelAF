/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDiagSvr.hpp"

using namespace tafsvc;

le_dls_List_t taf_DiagSvr::cancelFileXferCbList = LE_DLS_LIST_INIT;
le_mutex_Ref_t taf_DiagSvr::cancelFileXferListCbMtx = NULL;
le_mem_PoolRef_t taf_DiagSvr::CancelFileXferCbPool = NULL;

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

        // Init the service Rx Handler.
        servicePtr->testerHandlerRef = NULL;

        // Init message list.
        servicePtr->supportedVlanList = LE_DLS_LIST_INIT;

        // Create a Safe Reference for this service object
        servicePtr->svcRef = (taf_diag_ServiceRef_t)le_ref_CreateRef(SvcRefMap, servicePtr);

        servicePtr->targetVlanId = 0;

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

taf_DiagSvc_t* taf_DiagSvr::GetServiceObj
(
    uint16_t vlanId
)
{
    LE_DEBUG("Find the service object for vlan(0x%x)!", vlanId);
    bool isFound = false;
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_DiagSvc_t* servicePtr = (taf_DiagSvc_t *)le_ref_GetValue(iterRef);
        if (servicePtr != NULL)
        {
            // In some cases. the interface may not set the vlan.
            if (vlanId == 0 && le_dls_NumLinks(&servicePtr->supportedVlanList) == 0)
            {
                return servicePtr;
            }

            // Verify if the vlan is match.
            le_dls_Link_t* linkPtr = NULL;
            linkPtr = le_dls_Peek(&servicePtr->supportedVlanList);
            while (linkPtr)
            {
                taf_DiagVlanIdNode_t *vlan = CONTAINER_OF(linkPtr,
                        taf_DiagVlanIdNode_t, link);
                if (vlan != NULL && vlan->vlanId == vlanId)
                {
                    // Match.
                    isFound = true;
                    break;
                }
                linkPtr = le_dls_PeekNext(&servicePtr->supportedVlanList, linkPtr);
            }

            if (isFound)
            {
                return servicePtr;
            }
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * VLAN ID setting.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DiagSvr::SetVlanId
(
    taf_diag_ServiceRef_t svcRef,
    uint16_t vlanId
)
{
    taf_DiagSvc_t* servicePtr = (taf_DiagSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid service reference");
    TAF_ERROR_IF_RET_VAL(vlanId == 0, LE_BAD_PARAMETER, "Invalid vlan Id");

#ifndef LE_CONFIG_DIAG_VSTACK
    // Check Vlan Id is valid or not.
    auto& backend = taf_DiagBackend::GetInstance();
    if (!backend.isVlanIdValid(vlanId))
    {
        LE_ERROR("VlanId is unknown");
        return LE_UNSUPPORTED;
    }

    // Check if the vlan is set.
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&servicePtr->supportedVlanList);
    while (linkPtr)
    {
        taf_DiagVlanIdNode_t *vlan = CONTAINER_OF(linkPtr, taf_DiagVlanIdNode_t, link);
        if (vlan != NULL && vlan->vlanId == vlanId)
        {
            LE_INFO("The Vlan id(0x%x) was already set for ref%p", vlanId, svcRef);
            return LE_OK;
        }
        linkPtr = le_dls_PeekNext(&servicePtr->supportedVlanList, linkPtr);
    }

    taf_DiagVlanIdNode_t *vlanPtr = (taf_DiagVlanIdNode_t *)le_mem_ForceAlloc(VlanPool);
    if (vlanPtr == NULL)
    {
        LE_INFO("Failed to allocate memory.");
        return LE_NO_MEMORY;
    }

    vlanPtr->vlanId = vlanId;
    //Set target VLAN ID with last set VLAN ID
    servicePtr->targetVlanId = vlanId;
    vlanPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->supportedVlanList, &vlanPtr->link);

    return LE_OK;
#else
    return LE_NOT_IMPLEMENTED;
#endif
}

le_result_t taf_DiagSvr::SelectTargetVlanID
(
    taf_diag_ServiceRef_t svcRef,
    uint16_t vlanId
)
{
    LE_DEBUG("SelectTargetVlanID");

    taf_DiagSvc_t* servicePtr = (taf_DiagSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid service reference");
    TAF_ERROR_IF_RET_VAL(vlanId == 0, LE_BAD_PARAMETER, "Invalid vlan Id");

    // Check if Vlan Id is in supported VLAN list.
    bool isFound = false;
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&servicePtr->supportedVlanList);
    while (linkPtr)
    {
        taf_DiagVlanIdNode_t *vlanPtr = CONTAINER_OF(linkPtr, taf_DiagVlanIdNode_t, link);
        if (vlanPtr != NULL && vlanPtr->vlanId == vlanId)
        {
            // Match.
            isFound = true;
            break;
        }
        linkPtr = le_dls_PeekNext(&servicePtr->supportedVlanList, linkPtr);
    }

    if (!isFound)
    {
        LE_ERROR("VlanId is not set");
        return LE_NOT_FOUND;
    }

    servicePtr->targetVlanId = vlanId;
    return LE_OK;
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

    if (sid == stateChangeId)
    {
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
    }
    else if (sid == cancelFileXferRetId)
    {
        le_result_t result = LE_OK;
        if(msgPtr[1] == 0)
            result = LE_OK;
        else
            result = LE_FAULT;

        LE_INFO("CancelFileXfer result:%d for VlanId:%d", msgPtr[1], addrPtr->vlanId);
        CancelFileXferMsgHandler(addrPtr->vlanId, result, NULL);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * CancelFileXfer handler function
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagSvr::CancelFileXferMsgHandler
(
    uint16_t                   vlanId,         ///< [IN] Vlan Id.
    le_result_t                result,         ///< [IN] The result.
    void*                      userPtr         ///< [IN] User-defined pointer.
)
{
    LE_DEBUG("CancelFileXferMsgHandler vlanId =%d", vlanId);
    //auto &diag = taf_DiagSvr::GetInstance();

    taf_CancelFileXferHandler_t *cancelFileXferCbPtr = FindCancelFileXferCb(vlanId);
    if(cancelFileXferCbPtr != NULL && cancelFileXferCbPtr->func != NULL)
    {
        LE_INFO("vlanId = %d in the list", cancelFileXferCbPtr->vlanId);
        cancelFileXferCbPtr->func(cancelFileXferCbPtr->svcRef, cancelFileXferCbPtr->vlanId, result,
                cancelFileXferCbPtr->ctxPtr );
        DeleteCancelFileXferCb(vlanId, cancelFileXferCbPtr->func);
        return;
    }

    LE_ERROR("Not found callback function");

}
//-------------------------------------------------------------------------------------------------
/**
 * Add a Rx handler.
 */
//-------------------------------------------------------------------------------------------------
taf_diag_TesterStateHandlerRef_t taf_DiagSvr::AddTesterStateHandler
(
    taf_diag_ServiceRef_t svcRef,
    taf_diag_StateChangeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_DEBUG("AddTesterStateHandler");

    taf_DiagSvc_t* servicePtr = (taf_DiagSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, NULL, "Invalid service reference provided");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    if (servicePtr->testerHandlerRef != NULL)
    {
        LE_ERROR("Tester state handler is already registered");
        return NULL;
    }

    // Register handler for given Vlan id.
    taf_TesterStateHandler_t* handlerObjPtr = NULL;
    handlerObjPtr = (taf_TesterStateHandler_t*)le_mem_ForceAlloc(ReqHandlerPool);

    // Initialize the RxHandler object.
    handlerObjPtr->svcRef     = svcRef;
    handlerObjPtr->func       = handlerPtr;
    handlerObjPtr->ctxPtr     = contextPtr;
    handlerObjPtr->handlerRef =
            (taf_diag_TesterStateHandlerRef_t)le_ref_CreateRef(ReqHandlerRefMap, handlerObjPtr);

    // Attach handler to service.
    servicePtr->testerHandlerRef = handlerObjPtr->handlerRef;

    LE_INFO("TesterState: Registered Tester state change Handler");

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

    // Notify the repective callbackFuncPtr
    taf_DiagSvc_t* servicePtr = NULL;
    taf_TesterStateHandler_t* handlerObjPtr = NULL;

#ifdef LE_CONFIG_DIAG_VSTACK
    servicePtr = (taf_DiagSvc_t*)diag.GetServiceObj((uint16_t)0);
#else
    servicePtr = (taf_DiagSvc_t*)diag.GetServiceObj(rxStatePtr->addrInfo.vlanId);
#endif
    if (servicePtr == NULL)
    {
        return;
    }

    if (servicePtr->testerHandlerRef == NULL)
    {
        LE_WARN("Did not register handler for tester state notfication.");
        return;
    }

    // Lookup message handler of this service
    handlerObjPtr = (taf_TesterStateHandler_t*)le_ref_Lookup(diag.ReqHandlerRefMap,
            servicePtr->testerHandlerRef);
    if (handlerObjPtr == NULL || handlerObjPtr->func == NULL)
    {
        LE_INFO("Did not register handler for tester state notfication.");
        return;
    }

    // Add the message in service message list and notify to application.
    handlerObjPtr->func(rxStatePtr->rxStateRef, rxStatePtr->addrInfo.vlanId,
            rxStatePtr->currentTesterState, handlerObjPtr->ctxPtr);

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
    TAF_ERROR_IF_RET_NIL(handlerObjPtr->handlerRef != handlerRef, "Invalid handler reference");

    servicePtr = (taf_DiagSvc_t*)le_ref_Lookup(SvcRefMap, handlerObjPtr->svcRef);
    if (servicePtr == NULL)
    {
        LE_WARN("The handler is not belong to this service.");
        le_ref_DeleteRef(ReqHandlerRefMap, handlerRef);
        le_mem_Release(handlerObjPtr);

        return;
    }

    // Detach the handler from service.
    servicePtr->testerHandlerRef = NULL;

    // Clear Rx Handler resources
    handlerObjPtr->handlerRef = NULL;
    handlerObjPtr->svcRef     = NULL;
    handlerObjPtr->func       = NULL;
    handlerObjPtr->ctxPtr     = NULL;

    // Free the handler.
    le_ref_DeleteRef(ReqHandlerRefMap, handlerRef);
    le_mem_Release(handlerObjPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear vlan list.
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagSvr::ClearVlanList
(
    taf_DiagSvc_t* servicePtr
)
{
    LE_DEBUG("ClearVlanList");
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear the vlan id list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->supportedVlanList);
    while (linkPtr != NULL)
    {
        taf_DiagVlanIdNode_t *vlanPtr = CONTAINER_OF(linkPtr, taf_DiagVlanIdNode_t, link);
        if (vlanPtr != NULL)
        {
            LE_DEBUG("Release vlan node(id=0x%x)", vlanPtr->vlanId);
            le_mem_Release(vlanPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->supportedVlanList);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear CancelFileXfer callback list.
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagSvr::ClearCancelFileXferCbList
(
    taf_DiagSvc_t* servicePtr
)
{
    uint16_t vlanId = 0;
    LE_INFO("ClearCancelFileXferCbList");
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear callback function with vlanId 0.
    if(le_dls_NumLinks(&servicePtr->supportedVlanList) == 0)
    {
        vlanId = 0;
        taf_CancelFileXferHandler_t *cancelFileXferCbPtr =
                FindCancelFileXferCb(servicePtr->svcRef, vlanId);
        LE_INFO("SvcRef:%p, vlanId = %d", servicePtr->svcRef, vlanId);
        if(cancelFileXferCbPtr != NULL && cancelFileXferCbPtr->func != NULL)
        {
            LE_INFO("Found callback for SvcRef:%p, vlanId = %d in the list, Delete it",
                    servicePtr->svcRef, vlanId);
            DeleteCancelFileXferCb(vlanId, cancelFileXferCbPtr->func);
            return;
        }
        return;
    }

    // Clear callback function with vlanId in supportedVlanList.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->supportedVlanList);
    while (linkPtr != NULL)
    {
        taf_DiagVlanIdNode_t *vlanPtr = CONTAINER_OF(linkPtr, taf_DiagVlanIdNode_t, link);
        if (vlanPtr != NULL)
        {
            taf_CancelFileXferHandler_t *cancelFileXferCbPtr =
                    FindCancelFileXferCb(servicePtr->svcRef, vlanPtr->vlanId);
            LE_INFO("SvcRef:%p, vlanId = %d", servicePtr->svcRef, vlanPtr->vlanId);
            if(cancelFileXferCbPtr != NULL && cancelFileXferCbPtr->func != NULL)
            {
                LE_INFO("Found callback for SvcRef:%p, vlanId = %d in the list, Delete it",
                        servicePtr->svcRef, vlanPtr->vlanId);
                DeleteCancelFileXferCb(vlanPtr->vlanId, cancelFileXferCbPtr->func);
                return;
            }

        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->supportedVlanList);
    }

    return;
}

le_result_t taf_DiagSvr::GetIfNameByVlanIdAndStateList
(
    uint16_t vlanId,
    le_dls_List_t* fileXferStateListPtr,
    char* ifNamePtr
)
{
    le_dls_Link_t* linkPtr = NULL;
    LE_DEBUG("GetIfNameByVlanIdAndStateList");

    TAF_ERROR_IF_RET_VAL(fileXferStateListPtr == NULL, LE_NOT_POSSIBLE,
            "fileXferStateListPtr is null");
    TAF_ERROR_IF_RET_VAL(ifNamePtr == NULL, LE_NOT_POSSIBLE, "ifNamePtr is null");

    LE_INFO("link number = %d", (int)le_dls_NumLinks(fileXferStateListPtr));
    if(le_dls_NumLinks(fileXferStateListPtr) <= 0)
    {
        LE_ERROR("Empty list");
        return LE_NOT_FOUND;
    }

    LE_INFO("VLAN ID =%d", vlanId);
    linkPtr = le_dls_Peek(fileXferStateListPtr);
    while (linkPtr)
    {
        taf_uds_FileXferState_t* fileXferStatePtr = CONTAINER_OF(linkPtr, taf_uds_FileXferState_t,
                link);

        linkPtr = le_dls_PeekNext(fileXferStateListPtr, linkPtr);

        if (fileXferStatePtr != NULL)
        {
            LE_DEBUG("VLAN ID =%d", fileXferStatePtr->vlanId);
            //Find the first one
            if (vlanId == fileXferStatePtr->vlanId)
            {
                LE_INFO("Found Vlan ID =%d, ifname =%s", fileXferStatePtr->vlanId,
                        fileXferStatePtr->ifName);
                le_utf8_Copy(ifNamePtr, fileXferStatePtr->ifName, MAX_INTERFACE_NAME_LEN, NULL);
                return LE_OK;
            }
        }

    }

    return LE_NOT_POSSIBLE;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear FileXfer state list.
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagSvr::ClearFileXferStateList
(
    le_dls_List_t* fileXferStateListPtr
)
{
    le_dls_Link_t* linkPtr = NULL;

    TAF_ERROR_IF_RET_NIL(fileXferStateListPtr == NULL, "fileXferStateListPtr is null");

    linkPtr = le_dls_Pop(fileXferStateListPtr);
    while (linkPtr)
    {
        taf_uds_FileXferState_t* fileXferStatePtr = CONTAINER_OF(linkPtr, taf_uds_FileXferState_t,
                link);

        if (fileXferStatePtr != NULL)
        {
             //Release memory in the list
            le_mem_Release(fileXferStatePtr);
        }

        // Removes and returns the link at the head of the list.
        linkPtr = le_dls_Pop(fileXferStateListPtr);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Asynchrous function for CancelFileXfer
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagSvr::CancelFileXferAsync
(
    taf_diag_ServiceRef_t svcRef,
    taf_diag_CancelFileXferCallbackFunc_t callbackFuncPtr,
    void* contextPtr
)
{
    char ifName[MAX_INTERFACE_NAME_LEN];
    taf_uds_AddrInfo_t addrInfo;
    le_result_t result = LE_NOT_POSSIBLE;

    TAF_ERROR_IF_RET_NIL(svcRef == NULL, "svcRef is NULL");
    TAF_ERROR_IF_RET_NIL(callbackFuncPtr == NULL, "Handler function is NULL");

    taf_DiagSvc_t* servicePtr = (taf_DiagSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid service reference provided");

    le_dls_List_t FileXferStateList = LE_DLS_LIST_INIT;
    taf_uds_GetFileXferActiveStateList(&FileXferStateList);

    addrInfo.vlanId = servicePtr->targetVlanId;
    LE_INFO("Find ifName with active fileXfer state for vlan Id:%d", addrInfo.vlanId);

    result = GetIfNameByVlanIdAndStateList(addrInfo.vlanId, &FileXferStateList, ifName);
    if(result == LE_OK)
    {
        LE_DEBUG("Get active fileXfer state successfully with ifName:%s", ifName);
        le_utf8_Copy(addrInfo.ifName, ifName, MAX_INTERFACE_NAME_LEN, NULL);
    }
    else if(result == LE_NOT_FOUND)
    {
        result = LE_NOT_POSSIBLE;
    }

    //Release FileXferStateList
    ClearFileXferStateList(&FileXferStateList);

    if( result != LE_OK)
    {
        LE_INFO("No FileXferState is active");
        callbackFuncPtr(svcRef, addrInfo.vlanId, result, contextPtr);
        return;
    }

    LE_INFO("Call uds interface function to cancel file xfer for ifName=%s", ifName);
    taf_uds_DiagMsg_t diagMsg;
    uint8_t state = 0x1;

    diagMsg.dataPtr = &state;
    diagMsg.dataLen = sizeof(state);

    le_result_t ret = taf_uds_SetData(&addrInfo, &diagMsg, TAF_UDS_DATA_TYPE_FILEXFER_STATE);
    if (ret != LE_OK)
    {
        LE_ERROR("taf_uds_SetData failed");
        callbackFuncPtr(svcRef, addrInfo.vlanId, ret, contextPtr);
        return;
    }

    taf_CancelFileXferHandler_t *cancelFileXferCbPtr = FindCancelFileXferCb(addrInfo.vlanId);
    if(cancelFileXferCbPtr != NULL)
    {
        LE_ERROR("CancelFileXfer is in progress");
        callbackFuncPtr(svcRef, addrInfo.vlanId, LE_IN_PROGRESS, contextPtr);
        return;
    }

    if(AddCancelFileXferCb(svcRef, addrInfo.vlanId, callbackFuncPtr, contextPtr) != LE_OK)
    {
        LE_ERROR("Internal error");
        callbackFuncPtr(svcRef, addrInfo.vlanId, LE_FAULT, contextPtr);
        return;
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Delete CancelFileXfer callback from list
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagSvr::DeleteCancelFileXferCb
(
    uint16_t vlanId,
    taf_diag_CancelFileXferCallbackFunc_t callbackFuncPtr
)
{
    taf_CancelFileXferHandler_t *cancelFileXferCb;

    le_mutex_Lock(cancelFileXferListCbMtx);
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&cancelFileXferCbList);
    while (handlerLinkPtr)
    {
        cancelFileXferCb = CONTAINER_OF(handlerLinkPtr, taf_CancelFileXferHandler_t, link);
        handlerLinkPtr = le_dls_PeekNext(&cancelFileXferCbList, handlerLinkPtr);
        if (cancelFileXferCb->func == callbackFuncPtr && cancelFileXferCb->vlanId == vlanId)
        {
            LE_INFO("Delete handler for vlanId(%d) successfully", vlanId);
            le_dls_Remove(&cancelFileXferCbList, &cancelFileXferCb->link);
            le_mem_Release(cancelFileXferCb);
            break;
        }
    }

    le_mutex_Unlock(cancelFileXferListCbMtx);
}

//-------------------------------------------------------------------------------------------------
/**
 * Find CancelFileXfer callback from list
 */
//-------------------------------------------------------------------------------------------------
taf_CancelFileXferHandler_t* taf_DiagSvr::FindCancelFileXferCb
(
    uint16_t vlanId
)
{
    taf_CancelFileXferHandler_t *cancelFileXferCb;

    le_mutex_Lock(cancelFileXferListCbMtx);
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&cancelFileXferCbList);
    while (handlerLinkPtr)
    {
        cancelFileXferCb = CONTAINER_OF(handlerLinkPtr, taf_CancelFileXferHandler_t, link);
        if (cancelFileXferCb->vlanId == vlanId)
        {
            LE_INFO("Found CancelFileXferCb for vlanId(%d)", vlanId);
            le_mutex_Unlock(cancelFileXferListCbMtx);
            return cancelFileXferCb;
        }
        handlerLinkPtr = le_dls_PeekNext(&cancelFileXferCbList, handlerLinkPtr);
    }

    le_mutex_Unlock(cancelFileXferListCbMtx);
    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * Find CancelFileXfer callback from list
 */
//-------------------------------------------------------------------------------------------------
taf_CancelFileXferHandler_t* taf_DiagSvr::FindCancelFileXferCb
(
    taf_diag_ServiceRef_t svcRef,
    uint16_t vlanId
)
{
    taf_CancelFileXferHandler_t *cancelFileXferCb;

    le_mutex_Lock(cancelFileXferListCbMtx);
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&cancelFileXferCbList);
    while (handlerLinkPtr)
    {
        cancelFileXferCb = CONTAINER_OF(handlerLinkPtr, taf_CancelFileXferHandler_t, link);
        if (cancelFileXferCb->vlanId == vlanId && cancelFileXferCb->svcRef == svcRef)
        {
            LE_INFO("Found CancelFileXferCb for serviceRef reference %p, vlanId(%d)",
                     cancelFileXferCb->svcRef, vlanId);
            le_mutex_Unlock(cancelFileXferListCbMtx);
            return cancelFileXferCb;
        }
        handlerLinkPtr = le_dls_PeekNext(&cancelFileXferCbList, handlerLinkPtr);
    }

    le_mutex_Unlock(cancelFileXferListCbMtx);
    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add CancelFileXfer callback into list
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DiagSvr::AddCancelFileXferCb
(
    taf_diag_ServiceRef_t svcRef,
    uint16_t vlanId,
    taf_diag_CancelFileXferCallbackFunc_t callbackFuncPtr,
    void* contextPtr
)
{
    taf_CancelFileXferHandler_t *cancelFileXferCb;

    TAF_ERROR_IF_RET_VAL(svcRef == NULL || callbackFuncPtr == NULL, LE_BAD_PARAMETER,
            "Invalid service reference or callback function provided");
    cancelFileXferCb = (taf_CancelFileXferHandler_t *)le_mem_ForceAlloc(CancelFileXferCbPool);
    TAF_ERROR_IF_RET_VAL(cancelFileXferCb == NULL, LE_FAULT, "Failed to allocate callback");

    memset(cancelFileXferCb, 0, sizeof(taf_CancelFileXferHandler_t));
    cancelFileXferCb->vlanId = vlanId;
    cancelFileXferCb->svcRef = svcRef;
    cancelFileXferCb->ctxPtr = contextPtr;
    cancelFileXferCb->func = callbackFuncPtr;

    cancelFileXferCb->link = LE_DLS_LINK_INIT;
    le_mutex_Lock(cancelFileXferListCbMtx);
    le_dls_Queue(&cancelFileXferCbList, &cancelFileXferCb->link);
    le_mutex_Unlock(cancelFileXferListCbMtx);
    LE_INFO("Add handler for svcRef:%p, vlanId(%d) successfully", svcRef, vlanId);
    return LE_OK;
}

le_result_t taf_DiagSvr::Pause
(
    taf_diag_ServiceRef_t svcRef
)
{
    taf_uds_DiagMsg_t diagMsg;
    taf_uds_AddrInfo_t addrInfo;

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "Invalid service reference");
    taf_DiagSvc_t* servicePtr = (taf_DiagSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid service reference");

    addrInfo.vlanId = servicePtr->targetVlanId;
    memset(addrInfo.ifName, 0, MAX_INTERFACE_NAME_LEN);

    // Pause diag service to not receive any UDS requests.
    return taf_uds_SetData(&addrInfo, &diagMsg, TAF_UDS_DATA_TYPE_DIAG_PAUSE);
}

le_result_t taf_DiagSvr::Resume
(
    taf_diag_ServiceRef_t svcRef
)
{
    taf_uds_DiagMsg_t diagMsg;
    taf_uds_AddrInfo_t addrInfo;

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "Invalid service reference");
    taf_DiagSvc_t* servicePtr = (taf_DiagSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid service reference");

    addrInfo.vlanId = servicePtr->targetVlanId;
    memset(addrInfo.ifName, 0, MAX_INTERFACE_NAME_LEN);

    // Resume diag service to receive UDS requests.
    return taf_uds_SetData(&addrInfo, &diagMsg, TAF_UDS_DATA_TYPE_DIAG_RESUME);
}

le_result_t taf_DiagSvr::Shutdown
(
    taf_diag_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "Null ptr(svcRef)");

    taf_DiagSvc_t* servicePtr = (taf_DiagSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);

    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "svcRef invalid");

    return taf_uds_Stop();

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

    // Clear CancelFileXfer callback list
    ClearCancelFileXferCbList(servicePtr);

    // Clear VLAN list
    ClearVlanList(servicePtr);

    // Clear the registered Authentication StateExp handler
    if (servicePtr->testerHandlerRef != NULL)
    {
        RemoveTesterStateHandler(servicePtr->testerHandlerRef);
        servicePtr->testerHandlerRef = NULL;
    }

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
            diag.RemoveSvc(servicePtr->svcRef);
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

    cancelFileXferListCbMtx = le_mutex_CreateNonRecursive("cancelFileXferListCbMtx");
    // Create memory pools.
    EnableMemPool = le_mem_CreatePool("EnableConditionMemPool", sizeof(taf_DiagEnableStatus_t));

    // Create memory pools.
    SvcPool = le_mem_CreatePool("DiagSvcPool", sizeof(taf_DiagSvc_t));
    RxMsgPool = le_mem_CreatePool("RxMsgPool", sizeof(taf_RxTesterStateMsg_t));
    ReqHandlerPool = le_mem_CreatePool("ReqHandlerPool", sizeof(taf_TesterStateHandler_t));
    CancelFileXferCbPool = le_mem_CreatePool("CancelFileXferCbPool",
            sizeof(taf_CancelFileXferHandler_t));
    VlanPool = le_mem_CreatePool("DiagVlanPool", sizeof(taf_DiagVlanIdNode_t));

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
    backend.RegisterUdsService(cancelFileXferRetId, this);
}
