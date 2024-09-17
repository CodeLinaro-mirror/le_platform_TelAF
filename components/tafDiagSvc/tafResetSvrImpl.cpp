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
#include <string>
#include "tafResetSvr.hpp"
#include "configuration.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of TelAF ECURestet server.
 */
//--------------------------------------------------------------------------------------------------
taf_ResetSvr &taf_ResetSvr::GetInstance()
{
    static taf_ResetSvr instance;

    return instance;
}

//-------------------------------------------------------------------------------------------------
/**
 * Create a reference for the service, or get the reference of a service if the reference already
 * exist.
 */
//-------------------------------------------------------------------------------------------------
taf_diagReset_ServiceRef_t taf_ResetSvr::GetService
(
    uint8_t resetType
)
{
    LE_DEBUG("Gets the Reset Service");

    // Reset type check. Exception if can't get node from config file.
    if (resetType != TAF_DIAGRESET_ALL_RESET)
    {
        try
        {
            cfg::Node node = cfg::top_reset_all<int>("sub_function_identifier", resetType);
            LE_INFO("Reset type 0x%x is supported", resetType);
        }
        catch (const std::exception& e)
        {
            LE_ERROR("ECU reset type 0x%x is not configured in YAML file.", resetType);
            return NULL;
        }
    }

    // Search the service.
    taf_ResetSvc_t* servicePtr = GetServiceObj(resetType);

    // Create a service object if it doesn't exist in the list.
    if (servicePtr == NULL)
    {
        servicePtr = (taf_ResetSvc_t *)le_mem_ForceAlloc(SvcPool);
        memset(servicePtr, 0, sizeof(taf_ResetSvc_t));

        // Init the resetType.
        servicePtr->resetType = resetType;

        // Init the service Rx Handler.
        servicePtr->handlerRef = NULL;

        // Attach the service to the client.
        servicePtr->sessionRef = taf_diagReset_GetClientSessionRef();

        // Create a Safe Reference for this service object
        servicePtr->svcRef = (taf_diagReset_ServiceRef_t)le_ref_CreateRef(SvcRefMap, servicePtr);

        LE_INFO("svcRef %p of client %p is created for ECU reset type %x.",
                servicePtr->svcRef, servicePtr->sessionRef, resetType);
    }
    else
    {
        if (servicePtr->resetType == resetType)
        {
            // Only the service owner app can get the service reference for subsequent operations.
            if (servicePtr->sessionRef != taf_diagReset_GetClientSessionRef())
            {
                LE_ERROR("The service for reset type (0x%x) created by other client.",
                        servicePtr->resetType);
                return NULL;
            }
        }
        else if (servicePtr->resetType == TAF_DIAGRESET_ALL_RESET &&
                resetType != TAF_DIAGRESET_ALL_RESET)
        {
            LE_ERROR("Service is already created to handle all type of Reset");
            return NULL;
        }
        else if(servicePtr->resetType != TAF_DIAGRESET_ALL_RESET &&
                resetType == TAF_DIAGRESET_ALL_RESET)
        {
            LE_ERROR("Service is already created for Reset type 0x%x", servicePtr->resetType);
            return NULL;
        }
    }

    return servicePtr->svcRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the service instacnce object by resetType if the service reference already created and
 * return the object pointer.
 */
//-------------------------------------------------------------------------------------------------
taf_ResetSvc_t* taf_ResetSvr::GetServiceObj
(
    uint8_t resetType
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_ResetSvc_t* servicePtr = (taf_ResetSvc_t *)le_ref_GetValue(iterRef);
        if (servicePtr != NULL)
        {
            if ((servicePtr->resetType == resetType) ||
                    (servicePtr->resetType == TAF_DIAGRESET_ALL_RESET &&
                            resetType != TAF_DIAGRESET_ALL_RESET) ||
                                    (servicePtr->resetType != TAF_DIAGRESET_ALL_RESET &&
                                            resetType == TAF_DIAGRESET_ALL_RESET))
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
void taf_ResetSvr::UDSMsgHandler
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

    if (sid != reqSvcId)
    {
        LE_ERROR("The service(0x%x) is invalid", sid);
        return;
    }

    taf_ResetRxMsg_t* rxMsgPtr = NULL;

    rxMsgPtr = (taf_ResetRxMsg_t*)le_mem_ForceAlloc(RxMsgPool);
    memset(rxMsgPtr, 0, sizeof(taf_ResetRxMsg_t));

    memcpy(&rxMsgPtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
    rxMsgPtr->subFunc = msgPtr[1] & 0x7F;
    rxMsgPtr->link = LE_DLS_LINK_INIT;
    rxMsgPtr->rxMsgRef = (taf_diagReset_RxMsgRef_t)le_ref_CreateRef(RxMsgRefMap, rxMsgPtr);

    LE_DEBUG("Receive message(%p) for serviceId: 0x%x and subFunction: 0x%x)",
            rxMsgPtr->rxMsgRef, sid, rxMsgPtr->subFunc);

    // Report the request message to message handler in service layer.
    le_event_ReportWithRefCounting(ResetEvent, rxMsgPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a Rx handler for a given service.
 */
//-------------------------------------------------------------------------------------------------
taf_diagReset_RxMsgHandlerRef_t taf_ResetSvr::AddRxMsgHandler
(
    taf_diagReset_ServiceRef_t svcRef,
    taf_diagReset_RxMsgHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_DEBUG("AddRxMsgHandler");

    taf_ResetSvc_t* servicePtr = (taf_ResetSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, NULL, "Invalid service reference provided");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    if (servicePtr->handlerRef != NULL)
    {
        LE_ERROR("Rx handler is already registered for the ECUReset service type: 0x%x.",
                servicePtr->resetType);

        return NULL;
    }

    taf_ResetReqHandler_t* handlerObjPtr = NULL;
    handlerObjPtr = (taf_ResetReqHandler_t*)le_mem_ForceAlloc(ReqHandlerPool);

    // Initialize the RxHandler object.
    handlerObjPtr->svcRef     = svcRef;
    handlerObjPtr->func       = handlerPtr;
    handlerObjPtr->ctxPtr     = contextPtr;
    handlerObjPtr->handlerRef =
            (taf_diagReset_RxMsgHandlerRef_t)le_ref_CreateRef(ReqHandlerRefMap, handlerObjPtr);

    // Attach handler to service.
    servicePtr->handlerRef = handlerObjPtr->handlerRef;

    LE_INFO("ECUReset: Registered Rx Handler for reset type %x", servicePtr->resetType);

    return handlerObjPtr->handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Reset event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_ResetSvr::RxReqEventHandler
(
    void* reportPtr
)
{
    LE_DEBUG("RxReqEventHandler");

    auto &reset = taf_ResetSvr::GetInstance();

    taf_ResetRxMsg_t* rxMsgPtr = (taf_ResetRxMsg_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(rxMsgPtr == NULL, "rxMsgPtr is Null");

    taf_ResetSvc_t* servicePtr = NULL;
    taf_ResetReqHandler_t* handlerObjPtr = NULL;

    servicePtr = (taf_ResetSvc_t*)reset.GetServiceObj(rxMsgPtr->subFunc);
    if (servicePtr == NULL)
    {
        servicePtr = (taf_ResetSvc_t*)reset.GetServiceObj(TAF_DIAGRESET_ALL_RESET);
        if(servicePtr == NULL)
        {
            LE_WARN("Not found registered ECU reset service type: 0x%x for this request",
                    rxMsgPtr->subFunc);
            // UDS_0x11_NRC_21: service pointer is null
            reset.SendNRCResp(&(rxMsgPtr->addrInfo), TAF_DIAG_BUSY_REPEAT_REQUEST);
            le_ref_DeleteRef(reset.RxMsgRefMap, rxMsgPtr->rxMsgRef);
            le_mem_Release(rxMsgPtr);
            return;
        }
    }

    if (servicePtr->handlerRef == NULL)
    {
        LE_WARN("Did not register handler for ECU reset service type: 0x%x",
                rxMsgPtr->subFunc);
        // UDS_0x11_NRC_21: handler is not registered
        reset.SendNRCResp(&(rxMsgPtr->addrInfo), TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(reset.RxMsgRefMap, rxMsgPtr->rxMsgRef);
        le_mem_Release(rxMsgPtr);
        return;
    }

    // Lookup message handler of this service
    handlerObjPtr =
            (taf_ResetReqHandler_t*)le_ref_Lookup(reset.ReqHandlerRefMap, servicePtr->handlerRef);
    if (handlerObjPtr == NULL || handlerObjPtr->func == NULL)
    {
        // UDS_0x11_NRC_21: handler is null
        reset.SendNRCResp(&(rxMsgPtr->addrInfo), TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(reset.RxMsgRefMap, rxMsgPtr->rxMsgRef);
        le_mem_Release(rxMsgPtr);
        return;
    }

    uint8_t resetType = rxMsgPtr->subFunc;

    // Add the message in service message list and notify to application.
    rxMsgPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->rxMsgList, &rxMsgPtr->link);

    if(servicePtr->resetType == resetType || servicePtr->resetType == TAF_DIAGRESET_ALL_RESET)
    {
        handlerObjPtr->func(rxMsgPtr->rxMsgRef, resetType, handlerObjPtr->ctxPtr);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the Rx handler for a given service.
 */
//-------------------------------------------------------------------------------------------------
void taf_ResetSvr::RemoveRxMsgHandler
(
    taf_diagReset_RxMsgHandlerRef_t handlerRef
)
{
    LE_DEBUG("RemoveRxMsgHandler");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");

    taf_ResetSvc_t* servicePtr = NULL;
    taf_ResetReqHandler_t* handlerPtr = NULL;

    handlerPtr = (taf_ResetReqHandler_t*)le_ref_Lookup(ReqHandlerRefMap, handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerPtr == NULL, "Invalid handlerPtr");

    servicePtr = (taf_ResetSvc_t*)le_ref_Lookup(SvcRefMap, handlerPtr->svcRef);
    if (servicePtr == NULL)
    {
        LE_WARN("The handler is not belong to this service.");
        le_ref_DeleteRef(ReqHandlerRefMap, handlerRef);
        le_mem_Release(handlerPtr);

        return;
    }

    // Detach the handler from service.
    servicePtr->handlerRef = NULL;

    // Clear Rx Handler resources
    handlerPtr->handlerRef = NULL;
    handlerPtr->svcRef     = NULL;
    handlerPtr->func       = NULL;
    handlerPtr->ctxPtr     = NULL;

    // Free the handler.
    le_ref_DeleteRef(ReqHandlerRefMap, handlerRef);
    le_mem_Release(handlerPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send NRC response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_ResetSvr::SendNRCResp
(
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
    backend.RespDiagNegative(reqSvcId, &addrInfo, errCode);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send ECUReset response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_ResetSvr::SendResp
(
    taf_diagReset_RxMsgRef_t rxMsgRef,
    uint8_t errCode
)
{
    LE_DEBUG("SendResp");

    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    le_result_t ret;

    taf_ResetRxMsg_t* rxMsgPtr = (taf_ResetRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    TAF_ERROR_IF_RET_VAL(rxMsgPtr == NULL, LE_BAD_PARAMETER, "Invalid rxMsgPtr");

    taf_ResetSvc_t* servicePtr = (taf_ResetSvc_t*)GetServiceObj(rxMsgPtr->subFunc);
    if (servicePtr == NULL)
    {
        servicePtr = (taf_ResetSvc_t*)GetServiceObj(TAF_DIAGRESET_ALL_RESET);
        if(servicePtr == NULL)
        {
            LE_ERROR("Cannot find the service(type:0x%x)", rxMsgPtr->subFunc);

            return LE_NOT_FOUND;
        }
    }

    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = rxMsgPtr->addrInfo.ta;
    addrInfo.ta = rxMsgPtr->addrInfo.sa;
    addrInfo.taType = rxMsgPtr->addrInfo.taType;

    if (errCode == 0)
    {
        // Positive Response
        ret = backend.RespDiagPositive(reqSvcId, &addrInfo);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send routine control positive response.(%d)", ret);
            return ret;
        }
    }
    else
    {
        // Negative response.
        ret = backend.RespDiagNegative(reqSvcId, &addrInfo, errCode);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send ECUReset negative response.(%d)", ret);
            return ret;
        }
    }

    // Remove the message from service message list.
    le_dls_Remove(&(servicePtr->rxMsgList), &(rxMsgPtr->link));

    // Free the message
    le_ref_DeleteRef(RxMsgRefMap, rxMsgPtr->rxMsgRef);
    le_mem_Release(rxMsgPtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear reset message list.
 */
//-------------------------------------------------------------------------------------------------
void taf_ResetSvr::ClearResetMsgList
(
    taf_ResetSvc_t* servicePtr
)
{
    LE_DEBUG("ClearResetMsgList");
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear the UDS Rx message of reset list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->rxMsgList);
    while (linkPtr != NULL)
    {
        taf_ResetRxMsg_t* msgPtr = CONTAINER_OF(linkPtr, taf_ResetRxMsg_t, link);
        if (msgPtr != NULL)
        {
            LE_INFO("Release (rxMsgRef: %p, subFunc: 0x%x)", msgPtr->rxMsgRef, msgPtr->subFunc);
            // Free the message
            le_ref_DeleteRef(RxMsgRefMap, msgPtr->rxMsgRef);
            le_mem_Release(msgPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->rxMsgList);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the created service and release the alloted memory.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_ResetSvr::RemoveSvc
(
    taf_diagReset_ServiceRef_t svcRef
)
{
    LE_DEBUG("RemoveSvc");

    taf_ResetSvc_t* servicePtr = (taf_ResetSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid servicePtr");

    // Release reset message resources.
    ClearResetMsgList(servicePtr);

    // Clear the registered reset handler
    if (servicePtr->handlerRef != NULL)
    {
        RemoveRxMsgHandler(servicePtr->handlerRef);
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
void taf_ResetSvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    LE_DEBUG("OnClientDisconnection");

    auto &reset = taf_ResetSvr::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(reset.SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_ResetSvc_t* servicePtr = (taf_ResetSvc_t *)le_ref_GetValue(iterRef);
        LE_ASSERT(servicePtr != NULL);
        LE_ASSERT(servicePtr->svcRef == le_ref_GetSafeRef(iterRef));

        if (servicePtr->sessionRef == sessionRef)
        {
            reset.RemoveSvc(servicePtr->svcRef);
        }
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_ResetSvr::Init()
{
    LE_INFO("tafResetSvr Init!");

    // Create memory pools.
    SvcPool = le_mem_CreatePool("ResetSvcPool", sizeof(taf_ResetSvc_t));
    RxMsgPool = le_mem_CreatePool("ResetRxMsgPool", sizeof(taf_ResetRxMsg_t));
    ReqHandlerPool = le_mem_CreatePool("ResetReqHandlerPool", sizeof(taf_ResetReqHandler_t));

    // Create reference maps
    SvcRefMap = le_ref_CreateMap("ResetSvcRefMap", DEFAULT_SVC_REF_CNT);
    RxMsgRefMap = le_ref_CreateMap("ResetRxMsgRefMap", DEFAULT_RX_MSG_REF_CNT);
    ReqHandlerRefMap = le_ref_CreateMap("ResetReqHandlerRefMap", DEFAULT_RX_HANDLER_REF_CNT);

    // Create event and add the event handler.
    ResetEvent = le_event_CreateIdWithRefCounting("ResetEvent");
    ResetEventHandlerRef = le_event_AddHandler("ResetEventHandlerRef", ResetEvent,
            taf_ResetSvr::RxReqEventHandler);

    // Set session close handler
    le_msg_AddServiceCloseHandler(taf_diagReset_GetServiceRef(), OnClientDisconnection, NULL);

    auto& backend = taf_DiagBackend::GetInstance();
    backend.RegisterUdsService(reqSvcId, this);

    LE_INFO("taf_ResetSvr Service started");
}
