/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafSecuritySvr.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of TelAF Security server.
 */
//--------------------------------------------------------------------------------------------------
taf_SecuritySvr &taf_SecuritySvr::GetInstance()
{
    static taf_SecuritySvr instance;

    return instance;
}

//-------------------------------------------------------------------------------------------------
/**
 * Create a reference for the service, or get the reference of a service if the reference already
 * exist.
 */
//-------------------------------------------------------------------------------------------------
taf_diagSecurity_ServiceRef_t taf_SecuritySvr::GetService
(
)
{
    LE_DEBUG("Gets the security service!");

    // Search the service.
    taf_SecuritySvc_t* servicePtr = GetServiceObj(taf_diagSecurity_GetClientSessionRef());

    // Create a service object if it doesn't exist in the list.
    if (servicePtr == NULL)
    {
        servicePtr = (taf_SecuritySvc_t *)le_mem_ForceAlloc(SvcPool);
        memset(servicePtr, 0, sizeof(taf_SecuritySvc_t));

        // Init the sessionCtrl service Rx Handler.
        servicePtr->SesTypeHandlerRef = NULL;

        // Init the session change Handler.
        servicePtr->sesChangeHandlerRef = NULL;

        // Init the securityAccess service Rx Handler.
        servicePtr->handlerRef = NULL;

        // Attach the service to the client.
        servicePtr->sessionRef = taf_diagSecurity_GetClientSessionRef();

        servicePtr->selectedVlanId = 0;

        // Create a Safe Reference for this service object
        servicePtr->svcRef = (taf_diagSecurity_ServiceRef_t)le_ref_CreateRef(SvcRefMap,
                servicePtr);

        servicePtr->supportedVlanList = LE_DLS_LIST_INIT;
    }

    LE_DEBUG("Get serviceRef %p for Diag security service.", servicePtr->svcRef);

    return servicePtr->svcRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the service instacnce object, if the service reference already created and return the
 * object pointer.
 */
//-------------------------------------------------------------------------------------------------
taf_SecuritySvc_t* taf_SecuritySvr::GetServiceObj
(
    le_msg_SessionRef_t sessionRef
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t *)le_ref_GetValue(iterRef);
        if (servicePtr != NULL && servicePtr->sessionRef == sessionRef)
        {
            return servicePtr;
        }
    }

    return NULL;
}

taf_SecuritySvc_t* taf_SecuritySvr::GetServiceObj
(
    uint16_t vlanId
)
{
    bool isFound = false;
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t *)le_ref_GetValue(iterRef);
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
                taf_SecurityVlanIdNode_t *vlan = CONTAINER_OF(linkPtr,
                    taf_SecurityVlanIdNode_t, link);
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

//-------------------------------------------------------------------------------------------------
/**
 * UDS stack handler function
 */
//-------------------------------------------------------------------------------------------------
void taf_SecuritySvr::UDSMsgHandler
(
    const taf_uds_AddrInfo_t* addrPtr,
    uint8_t sid,
    uint8_t* msgPtr,
    size_t msgLen
)
{

    TAF_ERROR_IF_RET_NIL(addrPtr == NULL, "Invalid addrPtr");
    TAF_ERROR_IF_RET_NIL(msgPtr == NULL, "Invalid msgPtr");

    uint8_t msgPos = 1;  // Skip sid
    uint8_t errCode = 0;

    if (sid == reqSesCtrlSvcId)
    {
        taf_SesTypeRxMsg_t* rxSesTypePtr = NULL;
        rxSesTypePtr = (taf_SesTypeRxMsg_t*)le_mem_ForceAlloc(RxSesTypePool);
        memset(rxSesTypePtr, 0, sizeof(taf_SesTypeRxMsg_t));

        memcpy(&rxSesTypePtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
        rxSesTypePtr->serviceId = sid;
        rxSesTypePtr->sesType = msgPtr[msgPos];
        msgPos += 1;

        rxSesTypePtr->link = LE_DLS_LINK_INIT;
        rxSesTypePtr->rxSesTypeRef = (taf_diagSecurity_RxSesTypeCheckRef_t)le_ref_CreateRef(
                RxSesTypeRefMap, rxSesTypePtr);

        LE_DEBUG("Receive message(%p) for serviceId: 0x%x and subFunction: 0x%x)",
                rxSesTypePtr->rxSesTypeRef, sid, rxSesTypePtr->sesType);

        // Report the session control request message to message handler in service layer.
        le_event_ReportWithRefCounting(SesTypeEvent, rxSesTypePtr);
    }
    else if(sid == sessionChangeId)
    {
        taf_SesChangeMsg_t* sesChangePtr = NULL;

        sesChangePtr = (taf_SesChangeMsg_t*)le_mem_ForceAlloc(SesChangePool);
        memset(sesChangePtr, 0, sizeof(taf_SesChangeMsg_t));

        memcpy(&sesChangePtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
        sesChangePtr->previousSesType = msgPtr[msgPos];
        msgPos += 1;

        sesChangePtr->currentSesType = msgPtr[msgPos];
        // Update the session type with current session type for received VlanId/non-vlanId.
        UpdateCurrentSesType(addrPtr->vlanId, msgPtr[msgPos]);
        msgPos += 1;

        sesChangePtr->link = LE_DLS_LINK_INIT;
        sesChangePtr->sesChangeRef = (taf_diagSecurity_SesChangeRef_t)le_ref_CreateRef(
                SesChangeRefMap, sesChangePtr);
        LE_DEBUG("Previous session type: %X and Current session type: %x",
                sesChangePtr->previousSesType, sesChangePtr->currentSesType);

        // Report the session control request message to message handler in service layer.
        le_event_ReportWithRefCounting(SesChangeEvent, sesChangePtr);
    }
    else if (sid == reqSecAccessSvcId)
    {
        taf_SecAccessRxMsg_t* rxSecAccessMsgPtr = NULL;

        rxSecAccessMsgPtr = (taf_SecAccessRxMsg_t*)le_mem_ForceAlloc(RxSecAccessMsgPool);
        memset(rxSecAccessMsgPtr, 0, sizeof(taf_SecAccessRxMsg_t));
        rxSecAccessMsgPtr->serviceId = sid;
        memcpy(&rxSecAccessMsgPtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
        rxSecAccessMsgPtr->subFunc = msgPtr[msgPos] & 0x7F;
        msgPos += 1;

        // This parameter record contains securityAccessDataRecord or securityKey payload based
        // on subFunction type.
        // Format and length of this parameter(s) are vehicle manufacturer specific.
        if ((msgLen - msgPos) > TAF_DIAGSECURITY_MAX_SEC_ACCESS_PAYLOAD_SIZE)
        {
            LE_WARN("Message length(%" PRIuS ") is out of range, send NRC %x",
                    msgLen - msgPos, TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
            errCode = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            // UDS_0x27_NRC_13: Payload is overflow (check again)
            SendNRCResp(sid, addrPtr, errCode);
            le_mem_Release(rxSecAccessMsgPtr);
            return;
        }

        memcpy(rxSecAccessMsgPtr->Payload, msgPtr + msgPos, msgLen - msgPos);
        rxSecAccessMsgPtr->PayloadLen = msgLen - msgPos;

        rxSecAccessMsgPtr->link = LE_DLS_LINK_INIT;
        rxSecAccessMsgPtr->rxMsgRef = (taf_diagSecurity_RxSecAccessMsgRef_t)le_ref_CreateRef(
                RxSecAccessMsgRefMap, rxSecAccessMsgPtr);

        // Report the security access request message to message handler in service layer.
        le_event_ReportWithRefCounting(SecAccessEvent, rxSecAccessMsgPtr);
    }
    else
    {
        LE_WARN("Service(0x%x) is invalid, send NRC %x", sid, TAF_DIAG_SERVICE_NOT_SUPPORTED);
        errCode = TAF_DIAG_SERVICE_NOT_SUPPORTED; // ServiceNotSupported
        // UDS_0x27_NRC_11: Got the bad SID for svc
        SendNRCResp(sid, addrPtr, errCode);
    }

    return;
}

void taf_SecuritySvr::UpdateCurrentSesType
(
    uint16_t vlanId,
    uint8_t RxCurrentSesType
)
{
    auto& backend = taf_DiagBackend::GetInstance();

    // Update with the latest session type.
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&backend.VlanAndSesTypeList);
    while (linkPtr)
    {
        taf_RxVlanCurrentSesType_t* vlanSesTypePtr = CONTAINER_OF(linkPtr,
                taf_RxVlanCurrentSesType_t, link);
        linkPtr = le_dls_PeekNext(&backend.VlanAndSesTypeList, linkPtr);

        if (vlanSesTypePtr->vlanId == vlanId)
        {
            vlanSesTypePtr->currentSesType = RxCurrentSesType;
            return;
        }
    }

}

le_result_t taf_SecuritySvr::SelectTargetVlanID
(
    taf_diagSecurity_ServiceRef_t svcRef,
    uint16_t vlanId
)
{
    taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER,
            "Invalid service reference provided");
    TAF_ERROR_IF_RET_VAL(vlanId == 0, LE_BAD_PARAMETER, "Invalid vlan Id");

    // Check Vlan Id is already set or not.
    bool isFound = false;
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&servicePtr->supportedVlanList);
    while (linkPtr)
    {
        taf_SecurityVlanIdNode_t *vlanPtr = CONTAINER_OF(linkPtr,
                taf_SecurityVlanIdNode_t, link);
        if (vlanPtr != NULL && vlanPtr->vlanId == vlanId)
        {
            // Match.
            isFound = true;
            LE_DEBUG("Service object for vlan(x%0x) found", vlanId);
            break;
        }
        linkPtr = le_dls_PeekNext(&servicePtr->supportedVlanList, linkPtr);
    }

    if (!isFound)
    {
        LE_ERROR("VlanId is not set");
        return LE_NOT_FOUND;
    }

    servicePtr->selectedVlanId = vlanId;
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the current active session type.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_SecuritySvr::GetCurrentSesType
(
    taf_diagSecurity_ServiceRef_t svcRef,
    uint8_t* currentTypePtr
)
{
    taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER,
            "Invalid service reference provided");

    auto& backend = taf_DiagBackend::GetInstance();

    le_result_t result = backend.GetCurrentSesType(servicePtr->selectedVlanId,
            currentTypePtr);

    return result;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a Rx handler for a sessionCtrl service.
 */
//-------------------------------------------------------------------------------------------------
taf_diagSecurity_RxSesTypeCheckHandlerRef_t taf_SecuritySvr::AddRxSesTypeCheckHandler
(
    taf_diagSecurity_ServiceRef_t svcRef,
    taf_diagSecurity_RxSesTypeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, NULL, "Invalid service reference provided");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    if (servicePtr->SesTypeHandlerRef != NULL)
    {
        LE_ERROR("Rx handler is already registered");

        return NULL;
    }

    taf_SesTypeReqHandler_t* handlerObjPtr = NULL;
    handlerObjPtr = (taf_SesTypeReqHandler_t*)le_mem_ForceAlloc(ReqSesTypeHandlerPool);

    // Initialize the RxHandler object.
    handlerObjPtr->svcRef     = svcRef;
    handlerObjPtr->func       = handlerPtr;
    handlerObjPtr->ctxPtr     = contextPtr;
    handlerObjPtr->SesTypeHandlerRef =
            (taf_diagSecurity_RxSesTypeCheckHandlerRef_t)le_ref_CreateRef(
                    ReqSesTypeHandlerRefMap, handlerObjPtr);

    // Attach handler to service.
    servicePtr->SesTypeHandlerRef = handlerObjPtr->SesTypeHandlerRef;

    return handlerObjPtr->SesTypeHandlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * SessionCtrl event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_SecuritySvr::RxSesCtrlEventHandler
(
    void* reportPtr
)
{
    auto &security = taf_SecuritySvr::GetInstance();

    taf_SesTypeRxMsg_t* rxSesTypePtr = (taf_SesTypeRxMsg_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(rxSesTypePtr == NULL, "rxSesTypePtr is Null");

    taf_SecuritySvc_t* servicePtr = NULL;
    taf_SesTypeReqHandler_t* handlerObjPtr = NULL;

#ifdef LE_CONFIG_DIAG_VSTACK
    servicePtr = (taf_SecuritySvc_t*)security.GetServiceObj((uint16_t)0);
#else
    servicePtr = (taf_SecuritySvc_t*)security.GetServiceObj(rxSesTypePtr->addrInfo.vlanId);
#endif
    if (servicePtr == NULL)
    {
        LE_DEBUG("Service is not created");
#ifdef LE_CONFIG_DIAG_FEATURE_A
        // UDS_0x10_NRC_21: service pointer is null
        security.SendNRCResp(rxSesTypePtr->serviceId, &(rxSesTypePtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
#else
        security.SendSesPositiveResp(rxSesTypePtr);
#endif
        le_ref_DeleteRef(security.RxSesTypeRefMap, rxSesTypePtr->rxSesTypeRef);
        le_mem_Release(rxSesTypePtr);
        return;
    }

    if (servicePtr->SesTypeHandlerRef == NULL)
    {
        LE_DEBUG("Did not register handler for session control service.");
#ifdef LE_CONFIG_DIAG_FEATURE_A
        // UDS_0x10_NRC_21: handler is not registered
        security.SendNRCResp(rxSesTypePtr->serviceId, &(rxSesTypePtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
#else
        security.SendSesPositiveResp(rxSesTypePtr);
#endif
        le_ref_DeleteRef(security.RxSesTypeRefMap, rxSesTypePtr->rxSesTypeRef);
        le_mem_Release(rxSesTypePtr);
        return;
    }

    // Lookup message handler of this service
    handlerObjPtr =
            (taf_SesTypeReqHandler_t*)le_ref_Lookup(security.ReqSesTypeHandlerRefMap,
                    servicePtr->SesTypeHandlerRef);
    if (handlerObjPtr == NULL || handlerObjPtr->func == NULL)
    {
        LE_DEBUG("Handler is NULL");
#ifdef LE_CONFIG_DIAG_FEATURE_A
        // UDS_0x10_NRC_21: handler is not registered
        security.SendNRCResp(rxSesTypePtr->serviceId, &(rxSesTypePtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
#else
        security.SendSesPositiveResp(rxSesTypePtr);
#endif
        le_ref_DeleteRef(security.RxSesTypeRefMap, rxSesTypePtr->rxSesTypeRef);
        le_mem_Release(rxSesTypePtr);
        return;
    }

    // Add the message in service message list and notify to application.
    rxSesTypePtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->rxSesTypeList, &rxSesTypePtr->link);
    handlerObjPtr->func(rxSesTypePtr->rxSesTypeRef, rxSesTypePtr->sesType,
            handlerObjPtr->ctxPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the Rx handler for a session control service.
 */
//-------------------------------------------------------------------------------------------------
void taf_SecuritySvr::RemoveRxSesTypeCheckHandler
(
    taf_diagSecurity_RxSesTypeCheckHandlerRef_t handlerRef
)
{
    LE_DEBUG("RemoveRxSesTypeCheckHandler!");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");

    taf_SecuritySvc_t* servicePtr = NULL;
    taf_SesTypeReqHandler_t* handlerObjPtr = NULL;

    handlerObjPtr = (taf_SesTypeReqHandler_t*)le_ref_Lookup(ReqSesTypeHandlerRefMap,
            handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerObjPtr == NULL, "Invalid handlerObjPtr");

    servicePtr = (taf_SecuritySvc_t*)le_ref_Lookup(SvcRefMap, handlerObjPtr->svcRef);
    if (servicePtr == NULL)
    {
        LE_WARN("The handler is not belong to this service.");
        le_ref_DeleteRef(ReqSesTypeHandlerRefMap, handlerRef);
        le_mem_Release(handlerObjPtr);

        return;
    }

    // Detach the handler from service.
    servicePtr->SesTypeHandlerRef = NULL;

    // Clear Rx Handler resources
    handlerObjPtr->SesTypeHandlerRef = NULL;
    handlerObjPtr->svcRef            = NULL;
    handlerObjPtr->func              = NULL;
    handlerObjPtr->ctxPtr            = NULL;

    // Free the handler.
    le_ref_DeleteRef(ReqSesTypeHandlerRefMap, handlerRef);
    le_mem_Release(handlerObjPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send Session control response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_SecuritySvr::SendSesTypeCheckResp
(
    taf_diagSecurity_RxSesTypeCheckRef_t rxSesTypeRef,
    uint8_t errCode
)
{
    TAF_ERROR_IF_RET_VAL(rxSesTypeRef == NULL, LE_BAD_PARAMETER, "Invalid rxSesTypeRef");

    le_result_t ret;

    taf_SesTypeRxMsg_t* rxSesTypePtr =
            (taf_SesTypeRxMsg_t*)le_ref_Lookup(RxSesTypeRefMap, rxSesTypeRef);
    if (rxSesTypePtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

#ifdef LE_CONFIG_DIAG_VSTACK
    taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t*)GetServiceObj((uint16_t)0);
#else
    taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t*)
        GetServiceObj(rxSesTypePtr->addrInfo.vlanId);
#endif
    if (servicePtr == NULL)
    {
        LE_ERROR("Not found registered security service for this request!");
        return LE_NOT_FOUND;
    }

    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = rxSesTypePtr->addrInfo.ta;
    addrInfo.ta = rxSesTypePtr->addrInfo.sa;
    addrInfo.taType = rxSesTypePtr->addrInfo.taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = rxSesTypePtr->addrInfo.vlanId;
    le_utf8_Copy(addrInfo.ifName, rxSesTypePtr->addrInfo.ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif
    if (errCode == 0)
    {
        // Positive response.
        ret = backend.RespDiagPositive(reqSesCtrlSvcId, &addrInfo);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send session control positive response.(%d)", ret);
            return ret;
        }
    }
    else
    {
        // Negative response.
        ret = backend.RespDiagNegative(reqSesCtrlSvcId, &addrInfo, errCode);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send session control negative response.(%d)", ret);
            return ret;
        }
    }

    // Remove the message from service message list.
    le_dls_Remove(&(servicePtr->rxSesTypeList), &(rxSesTypePtr->link));

    // Free the message
    le_ref_DeleteRef(RxSesTypeRefMap, rxSesTypePtr->rxSesTypeRef);
    le_mem_Release(rxSesTypePtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send positive Session control response to UDS stack internally.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_SecuritySvr::SendSesPositiveResp
(
    taf_SesTypeRxMsg_t* rxSesTypePtr
)
{
    TAF_ERROR_IF_RET_VAL(rxSesTypePtr == NULL, LE_BAD_PARAMETER, "Invalid rxSesTypePtr");

    le_result_t ret;

    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = rxSesTypePtr->addrInfo.ta;
    addrInfo.ta = rxSesTypePtr->addrInfo.sa;
    addrInfo.taType = rxSesTypePtr->addrInfo.taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = rxSesTypePtr->addrInfo.vlanId;
    le_utf8_Copy(addrInfo.ifName, rxSesTypePtr->addrInfo.ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif
    // Positive response.
    ret = backend.RespDiagPositive(reqSesCtrlSvcId, &addrInfo);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to send session control positive response.(%d)", ret);
        return ret;
    }
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a handler for a session change.
 */
//-------------------------------------------------------------------------------------------------
taf_diagSecurity_SesChangeHandlerRef_t taf_SecuritySvr::AddSesChangeHandler
(
    taf_diagSecurity_ServiceRef_t svcRef,
    taf_diagSecurity_SesChangeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{

    taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, NULL, "Invalid service reference provided");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    if (servicePtr->sesChangeHandlerRef != NULL)
    {
        LE_ERROR("Handler is already registered");
        return NULL;
    }

    taf_SesChangeHandler_t* handlerObjPtr = NULL;
    handlerObjPtr = (taf_SesChangeHandler_t*)le_mem_ForceAlloc(SesChangeHandlerPool);

    // Initialize the RxHandler object.
    handlerObjPtr->svcRef     = svcRef;
    handlerObjPtr->func       = handlerPtr;
    handlerObjPtr->ctxPtr     = contextPtr;
    handlerObjPtr->sesChangeHandlerRef =
            (taf_diagSecurity_SesChangeHandlerRef_t)le_ref_CreateRef(
                    SesChangeHandlerRefMap, handlerObjPtr);

    // Attach handler to service.
    servicePtr->sesChangeHandlerRef = handlerObjPtr->sesChangeHandlerRef;

    LE_DEBUG("Session change: Registered Rx Handler for VLAN ID %d", servicePtr->selectedVlanId);

    return handlerObjPtr->sesChangeHandlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * SessionChange event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_SecuritySvr::SesChangeEventHandler
(
    void* reportPtr
)
{
    LE_DEBUG("SesChangeEventHandler!");

    auto &security = taf_SecuritySvr::GetInstance();

    taf_SesChangeMsg_t* SesChangePtr = (taf_SesChangeMsg_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(SesChangePtr == NULL, "SesChangePtr is Null");

    taf_SecuritySvc_t* servicePtr = NULL;
    taf_SesChangeHandler_t* handlerObjPtr = NULL;

#ifdef LE_CONFIG_DIAG_VSTACK
    servicePtr = (taf_SecuritySvc_t*)security.GetServiceObj((uint16_t)0);
#else
    servicePtr = (taf_SecuritySvc_t*)security.GetServiceObj(SesChangePtr->addrInfo.vlanId);
#endif
    if (servicePtr == NULL)
    {
        le_ref_DeleteRef(security.SesChangeRefMap, SesChangePtr->sesChangeRef);
        le_mem_Release(SesChangePtr);
        return;
    }

    if (servicePtr->sesChangeHandlerRef == NULL)
    {
        LE_WARN("Did not register handler for session change.");
        le_ref_DeleteRef(security.SesChangeRefMap, SesChangePtr->sesChangeRef);
        le_mem_Release(SesChangePtr);
        return;
    }

    // Lookup message handler of this service
    handlerObjPtr =
            (taf_SesChangeHandler_t*)le_ref_Lookup(security.SesChangeHandlerRefMap,
                    servicePtr->sesChangeHandlerRef);
    if (handlerObjPtr == NULL || handlerObjPtr->func == NULL)
    {
        le_ref_DeleteRef(security.SesChangeRefMap, SesChangePtr->sesChangeRef);
        le_mem_Release(SesChangePtr);
        return;
    }

    // Add the message in service message list and notify to application.
    SesChangePtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->rxSesChangeList, &SesChangePtr->link);
    handlerObjPtr->func(SesChangePtr->sesChangeRef, SesChangePtr->previousSesType,
            SesChangePtr->currentSesType, handlerObjPtr->ctxPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the handler for a session change.
 */
//-------------------------------------------------------------------------------------------------
void taf_SecuritySvr::RemoveSesChangeHandler
(
    taf_diagSecurity_SesChangeHandlerRef_t handlerRef
)
{
    LE_DEBUG("RemoveSesChangeHandler!");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");

    taf_SecuritySvc_t* servicePtr = NULL;
    taf_SesChangeHandler_t* handlerObjPtr = NULL;

    handlerObjPtr = (taf_SesChangeHandler_t*)le_ref_Lookup(SesChangeHandlerRefMap,
            handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerObjPtr == NULL, "Invalid handlerObjPtr");

    servicePtr = (taf_SecuritySvc_t*)le_ref_Lookup(SvcRefMap, handlerObjPtr->svcRef);
    if (servicePtr == NULL)
    {
        LE_WARN("The handler is not belong to this service.");
        le_ref_DeleteRef(SesChangeHandlerRefMap, handlerRef);
        le_mem_Release(handlerObjPtr);

        return;
    }

    // Detach the handler from service.
    servicePtr->sesChangeHandlerRef = NULL;

    // Clear Rx Handler resources
    handlerObjPtr->sesChangeHandlerRef = NULL;
    handlerObjPtr->svcRef            = NULL;
    handlerObjPtr->func              = NULL;
    handlerObjPtr->ctxPtr            = NULL;

    // Free the handler.
    le_ref_DeleteRef(SesChangeHandlerRefMap, handlerRef);
    le_mem_Release(handlerObjPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Releases a session change notification message.
 */
//-------------------------------------------------------------------------------------------------

le_result_t taf_SecuritySvr::ReleaseSesChangeMsg
(
    taf_diagSecurity_SesChangeRef_t sesChangeRef
)
{
    taf_SesChangeMsg_t* rxSesChangMsgPtr =
            (taf_SesChangeMsg_t*)le_ref_Lookup(SesChangeRefMap, sesChangeRef);
    TAF_ERROR_IF_RET_VAL(rxSesChangMsgPtr == NULL, LE_BAD_PARAMETER, "Invalid sesChangeRef");

    // Search the service.
    taf_SecuritySvc_t* servicePtr
            = (taf_SecuritySvc_t*)GetServiceObj(rxSesChangMsgPtr->addrInfo.vlanId);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid servicePtr");

    // Remove the message from message list.
    le_dls_Remove(&servicePtr->rxSesChangeList, &rxSesChangMsgPtr->link);

    // Free the message.
    le_ref_DeleteRef(SesChangeRefMap, sesChangeRef);
    le_mem_Release(rxSesChangMsgPtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a Rx handler for a given service.
 */
//-------------------------------------------------------------------------------------------------
taf_diagSecurity_RxSecAccessMsgHandlerRef_t taf_SecuritySvr::AddRxSecAccessMsgHandler
(
    taf_diagSecurity_ServiceRef_t svcRef,
    taf_diagSecurity_RxSecAccessMsgHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_DEBUG("AddRxSecAccessMsgHandler!");

    taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, NULL, "Invalid service reference provided");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    if (servicePtr->handlerRef != NULL)
    {
        LE_ERROR("Rx handler is already registered");
        return NULL;
    }

    taf_SecAccessReqHandler_t* handlerObjPtr = NULL;
    handlerObjPtr = (taf_SecAccessReqHandler_t*)le_mem_ForceAlloc(ReqSecAccessHandlerPool);

    // Initialize the RxHandler object.
    handlerObjPtr->svcRef     = svcRef;
    handlerObjPtr->func       = handlerPtr;
    handlerObjPtr->ctxPtr     = contextPtr;
    handlerObjPtr->handlerRef =
            (taf_diagSecurity_RxSecAccessMsgHandlerRef_t)le_ref_CreateRef(
                    ReqSecAccessHandlerRefMap, handlerObjPtr);

    // Attach handler to service.
    servicePtr->handlerRef = handlerObjPtr->handlerRef;

    LE_DEBUG("Security access: Registered Rx Handler for VLAN ID %d", servicePtr->selectedVlanId);

    return handlerObjPtr->handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Secuity event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_SecuritySvr::RxSecAccessEventHandler
(
    void* reportPtr
)
{
    LE_DEBUG("RxSecAccessEventHandler!");

    auto &security = taf_SecuritySvr::GetInstance();

    taf_SecAccessRxMsg_t* rxSecAccessMsgPtr = (taf_SecAccessRxMsg_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(rxSecAccessMsgPtr == NULL, "rxSecAccessMsgPtr is Null");

    taf_SecuritySvc_t* servicePtr = NULL;
    taf_SecAccessReqHandler_t* handlerObjPtr = NULL;

#ifdef LE_CONFIG_DIAG_VSTACK
    servicePtr = (taf_SecuritySvc_t*)security.GetServiceObj((uint16_t)0);
#else
    servicePtr = (taf_SecuritySvc_t*)security.GetServiceObj(rxSecAccessMsgPtr->addrInfo.vlanId);
#endif
    if (servicePtr == NULL)
    {
        // UDS_0x27_NRC_21: handler is not registered
        LE_WARN("Service is not created, send NRC %x", TAF_DIAG_BUSY_REPEAT_REQUEST);
        security.SendNRCResp(rxSecAccessMsgPtr->serviceId, &(rxSecAccessMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(security.RxSecAccessMsgRefMap, rxSecAccessMsgPtr->rxMsgRef);
        le_mem_Release(rxSecAccessMsgPtr);
        return;
    }

    if (servicePtr->handlerRef == NULL)
    {
        LE_WARN("Did not register handler for security access service, send NRC %x",
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        // UDS_0x27_NRC_21: handler is not registered
        security.SendNRCResp(rxSecAccessMsgPtr->serviceId, &(rxSecAccessMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(security.RxSecAccessMsgRefMap, rxSecAccessMsgPtr->rxMsgRef);
        le_mem_Release(rxSecAccessMsgPtr);
        return;
    }

    // Lookup message handler of this service
    handlerObjPtr =
            (taf_SecAccessReqHandler_t*)le_ref_Lookup(security.ReqSecAccessHandlerRefMap,
                    servicePtr->handlerRef);
    if (handlerObjPtr == NULL || handlerObjPtr->func == NULL)
    {
        // UDS_0x27_NRC_21: handler is null
        security.SendNRCResp(rxSecAccessMsgPtr->serviceId, &(rxSecAccessMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(security.RxSecAccessMsgRefMap, rxSecAccessMsgPtr->rxMsgRef);
        le_mem_Release(rxSecAccessMsgPtr);
        return;
    }

    // Add the message in service message list and notify to application.
    rxSecAccessMsgPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->rxMsgList, &rxSecAccessMsgPtr->link);
    handlerObjPtr->func(rxSecAccessMsgPtr->rxMsgRef, rxSecAccessMsgPtr->subFunc,
            handlerObjPtr->ctxPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the Rx handler for a given service.
 */
//-------------------------------------------------------------------------------------------------
void taf_SecuritySvr::RemoveRxSecAccessMsgHandler
(
    taf_diagSecurity_RxSecAccessMsgHandlerRef_t handlerRef
)
{
    LE_DEBUG("RemoveRxSecAccessMsgHandler!");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");

    taf_SecuritySvc_t* servicePtr = NULL;
    taf_SecAccessReqHandler_t* handlerObjPtr = NULL;

    handlerObjPtr = (taf_SecAccessReqHandler_t*)le_ref_Lookup(ReqSecAccessHandlerRefMap,
            handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerObjPtr == NULL, "Invalid handlerObjPtr");

    servicePtr = (taf_SecuritySvc_t*)le_ref_Lookup(SvcRefMap, handlerObjPtr->svcRef);
    if (servicePtr == NULL)
    {
        LE_WARN("The handler is not belong to this service.");
        le_ref_DeleteRef(ReqSecAccessHandlerRefMap, handlerRef);
        le_mem_Release(handlerObjPtr);
        return;
    }

    // Detach the handler from service.
    servicePtr->handlerRef = NULL;

    // Clear Rx Handler resources
    handlerObjPtr->handlerRef = NULL;
    handlerObjPtr->svcRef     = NULL;
    handlerObjPtr->func       = NULL;
    handlerObjPtr->ctxPtr     = NULL;

    // Free the handler.
    le_ref_DeleteRef(ReqSecAccessHandlerRefMap, handlerRef);
    le_mem_Release(handlerObjPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the securityAccessDataRecord/securityKey payload length of the Rx security access message.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_SecuritySvr::GetSecAccessPayloadLen
(
    taf_diagSecurity_RxSecAccessMsgRef_t rxMsgRef,
    uint16_t* payloadLenPtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(payloadLenPtr == NULL, LE_BAD_PARAMETER, "Invalid payloadLenPtr");

    taf_SecAccessRxMsg_t* rxSecAccessMsgPtr =
            (taf_SecAccessRxMsg_t*)le_ref_Lookup(RxSecAccessMsgRefMap, rxMsgRef);

    if (rxSecAccessMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    *payloadLenPtr = rxSecAccessMsgPtr->PayloadLen;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the securityAccessDataRecord/securityKey payload of the Rx security access message.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_SecuritySvr::GetSecAccessPayload
(
    taf_diagSecurity_RxSecAccessMsgRef_t rxMsgRef,
    uint8_t* payloadPtr,
    size_t* payloadSizePtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(payloadPtr == NULL, LE_BAD_PARAMETER, "Invalid payloadPtr");
    TAF_ERROR_IF_RET_VAL(payloadSizePtr == NULL, LE_BAD_PARAMETER, "Invalid payloadSizePtr");

    taf_SecAccessRxMsg_t* rxSecAccessMsgPtr =
            (taf_SecAccessRxMsg_t*)le_ref_Lookup(RxSecAccessMsgRefMap, rxMsgRef);
    if (rxSecAccessMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (*payloadSizePtr < rxSecAccessMsgPtr->PayloadLen)
    {
        LE_ERROR("The buffer(%" PRIuS ") is insufficient. Need size:%d" , *payloadSizePtr,
                rxSecAccessMsgPtr->PayloadLen);
        return LE_OVERFLOW;
    }

    memcpy(payloadPtr, rxSecAccessMsgPtr->Payload, rxSecAccessMsgPtr->PayloadLen);
    *payloadSizePtr = rxSecAccessMsgPtr->PayloadLen;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send Security access response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_SecuritySvr::SendSecAccessResp
(
    taf_diagSecurity_RxSecAccessMsgRef_t rxMsgRef,
    uint8_t errCode,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    le_result_t ret;

    taf_SecAccessRxMsg_t* rxSecAccessMsgPtr =
            (taf_SecAccessRxMsg_t*)le_ref_Lookup(RxSecAccessMsgRefMap, rxMsgRef);
    if (rxSecAccessMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

#ifdef LE_CONFIG_DIAG_VSTACK
    taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t*)GetServiceObj((uint16_t)0);
#else
    taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t*)
        GetServiceObj(rxSecAccessMsgPtr->addrInfo.vlanId);
#endif
    if (servicePtr == NULL)
    {
        LE_ERROR("Not found registered security service for this request!");
        return LE_NOT_FOUND;
    }

    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = rxSecAccessMsgPtr->addrInfo.ta;
    addrInfo.ta = rxSecAccessMsgPtr->addrInfo.sa;
    addrInfo.taType = rxSecAccessMsgPtr->addrInfo.taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = rxSecAccessMsgPtr->addrInfo.vlanId;
    le_utf8_Copy(addrInfo.ifName, rxSecAccessMsgPtr->addrInfo.ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif
    if (errCode == 0)
    {
        // Positive response.
        if (dataPtr == NULL || dataSize == 0)
        {
            ret = backend.RespDiagPositive(reqSecAccessSvcId, &addrInfo);
            if (ret != LE_OK)
            {
                LE_ERROR("Failed to send security access positive response.(%d)", ret);
                return ret;
            }
        }
        else
        {
            ret = backend.RespDiagPositive(reqSecAccessSvcId, &addrInfo, dataPtr, dataSize);
            if (ret != LE_OK)
            {
                LE_ERROR("Failed to send security access positive response.(%d)", ret);
                return ret;
            }
        }
    }
    else
    {
        // Negative response.
        ret = backend.RespDiagNegative(reqSecAccessSvcId, &addrInfo, errCode);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send security access negative response.(%d)", ret);
            return ret;
        }
    }

    // Remove the message from service message list.
    le_dls_Remove(&(servicePtr->rxMsgList), &(rxSecAccessMsgPtr->link));

    // Free the message
    le_ref_DeleteRef(RxSecAccessMsgRefMap, rxSecAccessMsgPtr->rxMsgRef);
    le_mem_Release(rxSecAccessMsgPtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send NRC response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_SecuritySvr::SendNRCResp
(
    uint8_t sid,
    const taf_uds_AddrInfo_t*  addrInfoPtr,
    uint8_t errCode
)
{
    TAF_ERROR_IF_RET_VAL(addrInfoPtr == NULL, LE_BAD_PARAMETER, "Invalid addrInfoPtr");

    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = addrInfoPtr->ta;
    addrInfo.ta = addrInfoPtr->sa;
    addrInfo.taType = addrInfoPtr->taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = addrInfoPtr->vlanId;
    le_utf8_Copy(addrInfo.ifName, addrInfoPtr->ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif
    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();
    backend.RespDiagNegative(sid, &addrInfo, errCode);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear sessionCtrl message list.
*/
//-------------------------------------------------------------------------------------------------
void taf_SecuritySvr::ClearSesTypeMsgList
(
    taf_SecuritySvc_t* servicePtr
)
{
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear the UDS Rx message of sessionCtrl list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->rxSesTypeList);
    while (linkPtr != NULL)
    {
        taf_SesTypeRxMsg_t* rxSesTypePtr = CONTAINER_OF(linkPtr, taf_SesTypeRxMsg_t, link);
        if (rxSesTypePtr != NULL)
        {
            // Free the message
            le_ref_DeleteRef(RxSesTypeRefMap, rxSesTypePtr->rxSesTypeRef);
            le_mem_Release(rxSesTypePtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->rxSesTypeList);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear session change message list.
*/
//-------------------------------------------------------------------------------------------------
void taf_SecuritySvr::ClearSesChangeMsgList
(
    taf_SecuritySvc_t* servicePtr
)
{
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear the UDS Rx message of sessionCtrl list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->rxSesChangeList);
    while (linkPtr != NULL)
    {
        taf_SesChangeMsg_t* sesChangePtr = CONTAINER_OF(linkPtr, taf_SesChangeMsg_t, link);
        if (sesChangePtr != NULL)
        {
            // Free the message
            le_ref_DeleteRef(SesChangeRefMap, sesChangePtr->sesChangeRef);
            le_mem_Release(sesChangePtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->rxSesChangeList);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear security access message list.
*/
//-------------------------------------------------------------------------------------------------
void taf_SecuritySvr::ClearSecAccessMsgList
(
    taf_SecuritySvc_t* servicePtr
)
{
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear the UDS Rx message of securityAccess list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->rxMsgList);
    while (linkPtr != NULL)
    {
        taf_SecAccessRxMsg_t* rxSecAccessMsgPtr = CONTAINER_OF(linkPtr, taf_SecAccessRxMsg_t,
                link);
        if (rxSecAccessMsgPtr != NULL)
        {
            // Free the message
            le_ref_DeleteRef(RxSecAccessMsgRefMap, rxSecAccessMsgPtr->rxMsgRef);
            le_mem_Release(rxSecAccessMsgPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->rxMsgList);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear vlan list.
*/
//-------------------------------------------------------------------------------------------------
void taf_SecuritySvr::ClearVlanList
(
    taf_SecuritySvc_t* servicePtr
)
{
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear the vlan id list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->supportedVlanList);
    while (linkPtr != NULL)
    {
        taf_SecurityVlanIdNode_t *vlanPtr = CONTAINER_OF(linkPtr, taf_SecurityVlanIdNode_t, link);
        if (vlanPtr != NULL)
        {
            le_mem_Release(vlanPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->supportedVlanList);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the created service and release the alloted memory.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_SecuritySvr::RemoveSvc
(
    taf_diagSecurity_ServiceRef_t svcRef
)
{
    LE_DEBUG("RemoveSvc");

    taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid servicePtr");

    // Release session control and security access message resources.
    ClearSesTypeMsgList(servicePtr);
    ClearSesChangeMsgList(servicePtr);
    ClearSecAccessMsgList(servicePtr);
    ClearVlanList(servicePtr);

    // Clear the registered session control handler
    if (servicePtr->SesTypeHandlerRef != NULL)
    {
        RemoveRxSesTypeCheckHandler(servicePtr->SesTypeHandlerRef);
        servicePtr->SesTypeHandlerRef = NULL;
    }

    // Clear the registered session change handler
    if (servicePtr->sesChangeHandlerRef != NULL)
    {
        RemoveSesChangeHandler(servicePtr->sesChangeHandlerRef);
        servicePtr->sesChangeHandlerRef = NULL;
    }

    // Clear the registered security access handler
    if (servicePtr->handlerRef != NULL)
    {
        RemoveRxSecAccessMsgHandler(servicePtr->handlerRef);
        servicePtr->handlerRef = NULL;
    }

    // Clear service object
    le_ref_DeleteRef(SvcRefMap, (void*)servicePtr->svcRef);
    le_mem_Release(servicePtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Session close handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_SecuritySvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    auto &security = taf_SecuritySvr::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(security.SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t *)le_ref_GetValue(iterRef);
        LE_ASSERT(servicePtr != NULL);
        LE_ASSERT(servicePtr->svcRef == le_ref_GetSafeRef(iterRef));

        if (servicePtr->sessionRef == sessionRef)
        {
            security.RemoveSvc(servicePtr->svcRef);
        }
    }

    return;
}

le_result_t taf_SecuritySvr::SetVlanId
(
    taf_diagSecurity_ServiceRef_t svcRef,
    uint16_t vlanId
)
{
    taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
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
        taf_SecurityVlanIdNode_t *vlan = CONTAINER_OF(linkPtr, taf_SecurityVlanIdNode_t, link);
        if (vlan != NULL && vlan->vlanId == vlanId)
        {
            return LE_OK;
        }
        linkPtr = le_dls_PeekNext(&servicePtr->supportedVlanList, linkPtr);
    }

    taf_SecurityVlanIdNode_t *vlanPtr = (taf_SecurityVlanIdNode_t *)le_mem_ForceAlloc(vlanPool);
    if (vlanPtr == NULL)
    {
        LE_ERROR("Failed to allocate memory.");
        return LE_NO_MEMORY;
    }

    vlanPtr->vlanId = vlanId;
    vlanPtr->link = LE_DLS_LINK_INIT;
    // update the target vlan ID with the last set vlan ID.
    servicePtr->selectedVlanId = vlanId;
    le_dls_Queue(&servicePtr->supportedVlanList, &vlanPtr->link);

    return LE_OK;
#else
    return LE_NOT_IMPLEMENTED;
#endif
}

le_result_t taf_SecuritySvr::GetVlanIdFromMsg
(
    taf_diagSecurity_RxMsgRef_t rxMsgRef,
    uint16_t* vlanIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(vlanIdPtr == NULL, LE_BAD_PARAMETER, "Invalid vlanIdPtr");

#ifndef LE_CONFIG_DIAG_VSTACK
    taf_SesTypeRxMsg_t* rxSesTypePtr =
            (taf_SesTypeRxMsg_t*)le_ref_Lookup(RxSesTypeRefMap, rxMsgRef);
    if (rxSesTypePtr != NULL)
    {
        *vlanIdPtr = rxSesTypePtr->addrInfo.vlanId;
        return LE_OK;
    }

    taf_SecAccessRxMsg_t* rxSecAccessMsgPtr =
            (taf_SecAccessRxMsg_t*)le_ref_Lookup(RxSecAccessMsgRefMap, rxMsgRef);
    if (rxSecAccessMsgPtr != NULL)
    {
        *vlanIdPtr = rxSecAccessMsgPtr->addrInfo.vlanId;
        return LE_OK;
    }

    taf_SesChangeMsg_t* rxSesChangMsgPtr =
            (taf_SesChangeMsg_t*)le_ref_Lookup(SesChangeRefMap, rxMsgRef);
    if (rxSesChangMsgPtr != NULL)
    {
        *vlanIdPtr = rxSesChangMsgPtr->addrInfo.vlanId;
        return LE_OK;
    }

    LE_ERROR("Can not find the rxMsgRef");
    return LE_NOT_FOUND;
#else
    return LE_NOT_IMPLEMENTED;
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_SecuritySvr::Init()
{
    // Create memory pools.
    SvcPool = le_mem_CreatePool("SecuritySvcPool", sizeof(taf_SecuritySvc_t));
    RxSesTypePool = le_mem_CreatePool("RxSesTypePool", sizeof(taf_SesTypeRxMsg_t));
    ReqSesTypeHandlerPool = le_mem_CreatePool("ReqSesTypeHandlerPool",
            sizeof(taf_SesTypeReqHandler_t));
    SesChangePool = le_mem_CreatePool("SesChangePool", sizeof(taf_SesChangeMsg_t));
    SesChangeHandlerPool = le_mem_CreatePool("SesChangeHandlerPool",
            sizeof(taf_SesChangeHandler_t));
    RxSecAccessMsgPool = le_mem_CreatePool("SecAccessRxMsgPool", sizeof(taf_SecAccessRxMsg_t));
    ReqSecAccessHandlerPool = le_mem_CreatePool("SecAccessReqHandlerPool",
            sizeof(taf_SecAccessReqHandler_t));
    vlanPool = le_mem_CreatePool("SecAccessVlanPool", sizeof(taf_SecurityVlanIdNode_t));

    // Create reference maps
    SvcRefMap = le_ref_CreateMap("SecuritySvcRefMap", DEFAULT_SVC_REF_CNT);
    RxSesTypeRefMap = le_ref_CreateMap("SesTypeRxMsgRefMap", DEFAULT_RX_MSG_REF_CNT);
    ReqSesTypeHandlerRefMap = le_ref_CreateMap("SesTypeReqHandlerRefMap",
            DEFAULT_RX_HANDLER_REF_CNT);
    SesChangeRefMap = le_ref_CreateMap("SesChangeRefMap", DEFAULT_RX_MSG_REF_CNT);
    SesChangeHandlerRefMap = le_ref_CreateMap("SesChangeHandlerRefMap",
            DEFAULT_RX_HANDLER_REF_CNT);
    RxSecAccessMsgRefMap = le_ref_CreateMap("SecAccessRxMsgRefMap", DEFAULT_RX_MSG_REF_CNT);
    ReqSecAccessHandlerRefMap = le_ref_CreateMap("SecAccessReqHandlerRefMap",
            DEFAULT_RX_HANDLER_REF_CNT);

    // Create an event and add the event handler for session control.
    SesTypeEvent = le_event_CreateIdWithRefCounting("SesTypeEvent");
    SesTypeEventHandlerRef = le_event_AddHandler("SesTypeEventHandlerRef", SesTypeEvent,
            taf_SecuritySvr::RxSesCtrlEventHandler);

    // Create an event and add the event handler for session change.
    SesChangeEvent = le_event_CreateIdWithRefCounting("SesChangeEvent");
    SesChangeEventHandlerRef = le_event_AddHandler("SesChangeEventHandlerRef", SesChangeEvent,
            taf_SecuritySvr::SesChangeEventHandler);

    // Create an event and add the event handler for security access.
    SecAccessEvent = le_event_CreateIdWithRefCounting("SecAccessEvent");
    SecAccessEventHandlerRef = le_event_AddHandler("SecAccessEventHandlerRef", SecAccessEvent,
            taf_SecuritySvr::RxSecAccessEventHandler);

    // Set session close handler
    le_msg_AddServiceCloseHandler(taf_diagSecurity_GetServiceRef(), OnClientDisconnection, NULL);

    auto& backend = taf_DiagBackend::GetInstance();
    backend.RegisterUdsService(reqSesCtrlSvcId, this);
    backend.RegisterUdsService(sessionChangeId, this);
    backend.RegisterUdsService(reqSecAccessSvcId, this);

    LE_DEBUG("tafSecuritySvr Init completed!");

}
