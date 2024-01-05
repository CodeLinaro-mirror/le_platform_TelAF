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

#include "legato.h"
#include "interfaces.h"
#include "tafSecuritySvr.hpp"

using namespace telux::tafsvc;

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
    taf_SecuritySvc_t* servicePtr = GetServiceObj();

    // Create a service object if it doesn't exist in the list.
    if (servicePtr == NULL)
    {
        servicePtr = (taf_SecuritySvc_t *)le_mem_ForceAlloc(SvcPool);
        memset(servicePtr, 0, sizeof(taf_SecuritySvc_t));

        // Init the service Rx Handler.
        servicePtr->handlerRef = NULL;

        // Attach the service to the client.
        servicePtr->sessionRef = taf_diagSecurity_GetClientSessionRef();

        // Create a Safe Reference for this service object
        servicePtr->svcRef = (taf_diagSecurity_ServiceRef_t)le_ref_CreateRef(SvcRefMap,
                servicePtr);

        LE_DEBUG("svcRef %p of client %p is created for security access.",
                servicePtr->svcRef, servicePtr->sessionRef);
    }
    else
    {
        // Only the service owner app can get the service reference for subsequent operations.
        if (servicePtr->sessionRef != taf_diagSecurity_GetClientSessionRef())
        {
            LE_ERROR("The service is created by other client.");
            return NULL;
        }
    }

    LE_INFO("Get serviceRef %p for Diag security service.", servicePtr->svcRef);

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
)
{
    LE_DEBUG("find the service object!");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t *)le_ref_GetValue(iterRef);
        if (servicePtr != NULL)
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
void taf_SecuritySvr::UDSMsgHandler
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

    uint8_t msgPos = 1;  // Skip sid
    uint8_t errCode = 0;

    // Send address information
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = addrPtr->ta;
    addrInfo.ta = addrPtr->sa;
    addrInfo.taType = addrPtr->taType;

    if (sid == SID_SECURITY_ACCESS)
    {
        taf_SecAccessRxMsg_t* rxSecAccessMsgPtr = NULL;

        rxSecAccessMsgPtr = (taf_SecAccessRxMsg_t*)le_mem_ForceAlloc(RxSecAccessMsgPool);
        memset(rxSecAccessMsgPtr, 0, sizeof(taf_SecAccessRxMsg_t));

        memcpy(&rxSecAccessMsgPtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
        rxSecAccessMsgPtr->subFunc = msgPtr[msgPos] & 0x7F;
        msgPos += 1;

        // This parameter record contains securityAccessDataRecord or securityKey payload based
        // on subFunction type.
        // Format and length of this parameter(s) are vehicle manufacturer specific.
        if ((msgLen - msgPos) > TAF_DIAGSECURITY_MAX_SEC_ACCESS_PAYLOAD_SIZE)
        {
            LE_DEBUG("Message length(%" PRIuS ") is out of range", msgLen - msgPos);
            errCode = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            SendNRCResp(sid, &addrInfo, errCode);
        }

        memcpy(rxSecAccessMsgPtr->Payload, msgPtr + msgPos, msgLen - msgPos);
        rxSecAccessMsgPtr->PayloadLen = msgLen - msgPos;

        rxSecAccessMsgPtr->link = LE_DLS_LINK_INIT;
        rxSecAccessMsgPtr->rxMsgRef = (taf_diagSecurity_RxSecAccessMsgRef_t)le_ref_CreateRef(
                RxSecAccessMsgRefMap, rxSecAccessMsgPtr);

        LE_DEBUG("Receive message(%p) for serviceId: 0x%x and subFunction: 0x%x)",
                rxSecAccessMsgPtr->rxMsgRef, sid, rxSecAccessMsgPtr->subFunc);

        // Report the security access request message to message handler in service layer.
        le_event_ReportWithRefCounting(SecAccessEvent, rxSecAccessMsgPtr);
    }
    else
    {
        LE_DEBUG("Service(0x%x) is invalid", sid);
        errCode = TAF_DIAG_SERVICE_NOT_SUPPORTED; // ServiceNotSupported
        SendNRCResp(sid, &addrInfo, errCode);
    }

    return;
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

    LE_INFO("Security access: Registered Rx Handler");

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


    servicePtr = (taf_SecuritySvc_t*)security.GetServiceObj();
    if (servicePtr == NULL)
    {
        le_ref_DeleteRef(security.RxSecAccessMsgRefMap, rxSecAccessMsgPtr->rxMsgRef);
        le_mem_Release(rxSecAccessMsgPtr);
        return;
    }

    if (servicePtr->handlerRef == NULL)
    {
        LE_WARN("Did not register handler for security access service.");
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

        return;
    }

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
    LE_DEBUG("GetSecAccessPayloadLen!");

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
    LE_DEBUG("GetSecAccessPayload!");

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
 * Send NRC response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_SecuritySvr::SendNRCResp
(
    uint8_t sid,
    taf_uds_AddrInfo_t*  addrInfoPtr,
    uint8_t errCode
)
{
    LE_DEBUG("SendNRCResp");

    TAF_ERROR_IF_RET_VAL(addrInfoPtr == NULL, LE_BAD_PARAMETER, "Invalid addrInfoPtr");

    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = addrInfoPtr->ta;
    addrInfo.ta = addrInfoPtr->sa;
    addrInfo.taType = addrInfoPtr->taType;
    backend.RespDiagNegative(sid, &addrInfo, errCode);

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
    taf_diagSecurity_SecAccessErrorCode_t errCode,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    LE_DEBUG("SendSecAccessResp");

    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    le_result_t ret;

    taf_SecAccessRxMsg_t* rxSecAccessMsgPtr =
            (taf_SecAccessRxMsg_t*)le_ref_Lookup(RxSecAccessMsgRefMap, rxMsgRef);
    if (rxSecAccessMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    taf_SecuritySvc_t* servicePtr = (taf_SecuritySvc_t*)GetServiceObj();
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

    // Clear the UDS Rx message list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&(servicePtr->rxMsgList));
    while (linkPtr != NULL)
    {
        taf_SecAccessRxMsg_t* rxSecAccessMsgPtr = CONTAINER_OF(linkPtr, taf_SecAccessRxMsg_t,
                link);
        if (rxSecAccessMsgPtr != NULL)
        {
            LE_INFO("Release (rxMsgRef: %p)", rxSecAccessMsgPtr->rxMsgRef);
            le_ref_DeleteRef(RxSecAccessMsgRefMap, rxSecAccessMsgPtr->rxMsgRef);
            le_mem_Release(rxSecAccessMsgPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&(servicePtr->rxMsgList));
    }

    // Clear the registered handler
    if (servicePtr->handlerRef != NULL)
    {
        taf_SecAccessReqHandler_t* handlerObjPtr = NULL;

        handlerObjPtr = (taf_SecAccessReqHandler_t*)le_ref_Lookup(ReqSecAccessHandlerRefMap,
                servicePtr->handlerRef);
        if (handlerObjPtr != NULL)
        {
            le_ref_DeleteRef(ReqSecAccessHandlerRefMap, handlerObjPtr->handlerRef);
            le_mem_Release(handlerObjPtr);
        }

        servicePtr->handlerRef = NULL;
    }

    // Clear service object
    le_ref_DeleteRef(SvcRefMap, servicePtr->svcRef);
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
    LE_DEBUG("OnClientDisconnection");

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

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_SecuritySvr::Init()
{
    LE_INFO("tafSecuritySvr Init!");

    // Create memory pools.
    SvcPool = le_mem_CreatePool("SecuritySvcPool", sizeof(taf_SecuritySvc_t));
    RxSecAccessMsgPool = le_mem_CreatePool("SecAccessRxMsgPool", sizeof(taf_SecAccessRxMsg_t));
    ReqSecAccessHandlerPool = le_mem_CreatePool("SecAccessReqHandlerPool",
            sizeof(taf_SecAccessReqHandler_t));

    // Create reference maps
    SvcRefMap = le_ref_CreateMap("SecuritySvcRefMap", DEFAULT_SVC_REF_CNT);
    RxSecAccessMsgRefMap = le_ref_CreateMap("SecAccessRxMsgRefMap", DEFAULT_RX_MSG_REF_CNT);
    ReqSecAccessHandlerRefMap = le_ref_CreateMap("SecAccessReqHandlerRefMap",
            DEFAULT_RX_HANDLER_REF_CNT);

    // Create event and add the event handler.
    SecAccessEvent = le_event_CreateIdWithRefCounting("SecAccessEvent");
    SecAccessEventHandlerRef = le_event_AddHandler("SecAccessEventHandlerRef", SecAccessEvent,
            taf_SecuritySvr::RxSecAccessEventHandler);

    // Set session close handler
    le_msg_AddServiceCloseHandler(taf_diagSecurity_GetServiceRef(), OnClientDisconnection, NULL);

    auto& backend = taf_DiagBackend::GetInstance();
    backend.RegisterUdsService(reqSecAccessSvcId, this);

    LE_INFO("Diag Security Service started!");
}
