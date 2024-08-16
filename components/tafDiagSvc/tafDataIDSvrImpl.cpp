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
#include "tafDataIDSvr.hpp"
#ifndef LE_CONFIG_DIAG_VSTACK
#include "tafSnapshotSvc.hpp"
#endif
// #include <arpa/inet.h>

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of TelAF RWDID server.
 */
//--------------------------------------------------------------------------------------------------
taf_DataIDSvr &taf_DataIDSvr::GetInstance
(
)
{
    static taf_DataIDSvr instance;

    return instance;
}

//-------------------------------------------------------------------------------------------------
/**
 * Create a reference for the service, or get the reference of a service if the reference already
 * exist.
 */
//-------------------------------------------------------------------------------------------------
taf_diagDataID_ServiceRef_t taf_DataIDSvr::GetService
(
)
{
    LE_DEBUG("Gets the DataID service!");

    // Search the service.
    taf_DataIDSvc_t* servicePtr = GetServiceObj();

    // Create a service object if it doesn't exist in the list.
    if (servicePtr == NULL)
    {
        servicePtr = (taf_DataIDSvc_t *)le_mem_ForceAlloc(SvcPool);
        memset(servicePtr, 0, sizeof(taf_DataIDSvc_t));

        // Init the service Rx Handler.
        servicePtr->readDIDHandlerRef = NULL;
        servicePtr->writeDIDHandlerRef = NULL;

        // Init dataID message list.
        servicePtr->readDIDMsgList  = LE_DLS_LIST_INIT;
        servicePtr->writeDIDMsgList = LE_DLS_LIST_INIT;

        // Attach the service to the client.
        servicePtr->sessionRef = taf_diagDataID_GetClientSessionRef();

        // Create a Safe Reference for this service object
        servicePtr->svcRef = (taf_diagDataID_ServiceRef_t)le_ref_CreateRef(SvcRefMap,
                servicePtr);

        LE_DEBUG("svcRef %p of client %p is created for DataId.",
                servicePtr->svcRef, servicePtr->sessionRef);
    }
    else
    {
        // Only the service owner app can get the service reference for subsequent operations.
        if (servicePtr->sessionRef != taf_diagDataID_GetClientSessionRef())
        {
            LE_ERROR("The service is created by other client.");
            return NULL;
        }
    }

    LE_INFO("Get serviceRef %p for Diag DataId service.", servicePtr->svcRef);

    return servicePtr->svcRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the service instacnce object, if the service reference already created and return the
 * object pointer.
 */
//-------------------------------------------------------------------------------------------------
taf_DataIDSvc_t* taf_DataIDSvr::GetServiceObj
(
)
{
    LE_DEBUG("find the service object!");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_DataIDSvc_t* servicePtr = (taf_DataIDSvc_t *)le_ref_GetValue(iterRef);
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
void taf_DataIDSvr::UDSMsgHandler
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

    if (sid == reqReadDIDSvcId)
    {
        taf_ReadDIDRxMsg_t* rxReadDIDMsgPtr = NULL;

        rxReadDIDMsgPtr = (taf_ReadDIDRxMsg_t*)le_mem_ForceAlloc(RxReadDIDMsgPool);
        memset(rxReadDIDMsgPtr, 0, sizeof(taf_ReadDIDRxMsg_t));

        memcpy(&rxReadDIDMsgPtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
        rxReadDIDMsgPtr->serviceId = sid;

        // This parameter record contains ReadDID.
        if (((msgLen - msgPos) > MAX_READ_DID_REQ_LEN) || (msgLen - msgPos)%2 != 0)
        {
            LE_DEBUG("Message length(%" PRIuS ") is not in correct format", msgLen - msgPos);
            errCode = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            SendNRCResp(sid, addrPtr, errCode);
            le_mem_Release(rxReadDIDMsgPtr);
            return;
        }

        for (size_t i = 0; i < (msgLen-msgPos)/sizeof(uint16_t); i++)
        {
            rxReadDIDMsgPtr->readDID[i] = (msgPtr[i*2 + msgPos] << 8) + msgPtr[i*2 + msgPos + 1];
        }
        rxReadDIDMsgPtr->readDIDLen = (msgLen - msgPos)/sizeof(uint16_t);

        rxReadDIDMsgPtr->link = LE_DLS_LINK_INIT;
        rxReadDIDMsgPtr->readDIDRxMsgRef = (taf_diagDataID_RxReadDIDMsgRef_t)le_ref_CreateRef(
                RxReadDIDMsgRefMap, rxReadDIDMsgPtr);

        LE_DEBUG("Receive message(%p) for serviceId: 0x%x)", rxReadDIDMsgPtr->readDIDRxMsgRef,
                sid);

        // Report the Read DID request message to message handler in service layer.
        le_event_ReportWithRefCounting(ReadDIDEvent, rxReadDIDMsgPtr);
    }
    else if (sid == reqWriteDIDSvcId)
    {
        taf_WriteDIDRxMsg_t* rxWriteDIDMsgPtr = NULL;

        rxWriteDIDMsgPtr = (taf_WriteDIDRxMsg_t*)le_mem_ForceAlloc(RxWriteDIDMsgPool);
        memset(rxWriteDIDMsgPtr, 0, sizeof(taf_WriteDIDRxMsg_t));

        memcpy(&rxWriteDIDMsgPtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
        rxWriteDIDMsgPtr->serviceId = sid;

        // This parameter record contains WriteDID.
        if ((msgLen - msgPos) < MIN_WRITE_DID_REQ_LEN)
        {
            LE_DEBUG("Message length(%" PRIuS ") is not correct", msgLen - msgPos);
            errCode = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            SendNRCResp(sid, addrPtr, errCode);
            le_mem_Release(rxWriteDIDMsgPtr);
            return;
        }

        //rxWriteDIDMsgPtr->writeDID = ntohs(*((uint16_t*)(msgPtr + msgPos)));
        rxWriteDIDMsgPtr->writeDID = (msgPtr[msgPos] << 8) + msgPtr[msgPos + 1];
        msgPos += 2;

        // This parameter record contains WriteDID dataRec length.
        if (((msgLen - msgPos) > TAF_DIAGDATAID_MAX_DID_DATA_RECORD_SIZE))
        {
            LE_DEBUG("Message length(%" PRIuS ") is not correct", msgLen - msgPos);
            errCode = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            SendNRCResp(sid, addrPtr, errCode);
            le_mem_Release(rxWriteDIDMsgPtr);
            return;
        }
        memcpy(rxWriteDIDMsgPtr->dataRec, msgPtr + msgPos, msgLen - msgPos);
        rxWriteDIDMsgPtr->dataRecLen = msgLen - msgPos;

        rxWriteDIDMsgPtr->link = LE_DLS_LINK_INIT;
        rxWriteDIDMsgPtr->writeDIDRxMsgRef = (taf_diagDataID_RxWriteDIDMsgRef_t)le_ref_CreateRef(
                RxWriteDIDMsgRefMap, rxWriteDIDMsgPtr);

        LE_DEBUG("Receive message(%p) for serviceId: 0x%x)", rxWriteDIDMsgPtr->writeDIDRxMsgRef,
                sid);

        // Report the Write DID request message to message handler in service layer.
        le_event_ReportWithRefCounting(WriteDIDEvent, rxWriteDIDMsgPtr);
    }
    else
    {
        LE_DEBUG("Service(0x%x) is invalid", sid);
        errCode = TAF_DIAG_SERVICE_NOT_SUPPORTED; // ServiceNotSupported
        SendNRCResp(sid, addrPtr, errCode);
    }

    return;
}

#ifndef LE_CONFIG_DIAG_VSTACK
le_result_t taf_DataIDSvr::SnapshotTriggerTheCollectionOfDIDs
(
    uint16_t* dids,
    size_t numOfDids,
    taf_ReadDIDRxMsg_t **msgPPtr
)
{
    TAF_ERROR_IF_RET_VAL(dids == NULL, LE_BAD_PARAMETER, "Invalid dids");

    *msgPPtr = NULL;

    auto & did = taf_DataIDSvr::GetInstance();

    taf_DataIDSvc_t* servicePtr = (taf_DataIDSvc_t*)did.GetServiceObj();

    if (servicePtr == NULL)
    {
        return LE_UNAVAILABLE;
    }

    if (servicePtr->readDIDHandlerRef == NULL)
    {
        return LE_UNAVAILABLE;
    }

    taf_ReadDIDHandler_t* handlerObjPtr =
                  (taf_ReadDIDHandler_t*) le_ref_Lookup(did.ReqReadDIDHandlerRefMap,
                                                        servicePtr->readDIDHandlerRef);
    if (handlerObjPtr == NULL || handlerObjPtr->func == NULL)
    {
        return LE_UNAVAILABLE;
    }

    taf_ReadDIDRxMsg_t* rxReadDIDMsgPtr = NULL;

    rxReadDIDMsgPtr = (taf_ReadDIDRxMsg_t*)le_mem_ForceAlloc(RxReadDIDMsgPool);
    memset(rxReadDIDMsgPtr, 0, sizeof(taf_ReadDIDRxMsg_t));

    // Note: we use the addrInfo to differentiate the internal and external indications
    // Internal: sa & ta : 0x0000
    // External: sa & ta : value from ISO standard

    rxReadDIDMsgPtr->serviceId = SID_READ_DATA_BY_IDENTIFIER;

    if (numOfDids > TAF_DIAGDATAID_MAX_READ_DID_SIZE)
    {
        LE_ERROR("Bad size of DID list: %" PRIuS, numOfDids);
        return LE_BAD_PARAMETER;
    }

    memcpy(rxReadDIDMsgPtr->readDID, dids, numOfDids * sizeof(uint16_t));
    rxReadDIDMsgPtr->readDIDLen = numOfDids;

    rxReadDIDMsgPtr->link = LE_DLS_LINK_INIT;

    rxReadDIDMsgPtr->readDIDRxMsgRef =
    (taf_diagDataID_RxReadDIDMsgRef_t) le_ref_CreateRef(RxReadDIDMsgRefMap,
                                                        rxReadDIDMsgPtr);

    LE_INFO("[Snapshot] Report the Read DID request message.");
    le_event_ReportWithRefCounting(ReadDIDEvent, rxReadDIDMsgPtr);

    *msgPPtr = rxReadDIDMsgPtr;

    return LE_OK;
}
#endif

//-------------------------------------------------------------------------------------------------
/**
 * Add a Rx handler for a given service (ReadDID).
 */
//-------------------------------------------------------------------------------------------------
taf_diagDataID_RxReadDIDMsgHandlerRef_t taf_DataIDSvr::AddRxReadDIDMsgHandler
(
    taf_diagDataID_ServiceRef_t svcRef,
    taf_diagDataID_RxReadDIDMsgHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_DEBUG("AddRxReadDIDMsgHandler!");

    taf_DataIDSvc_t* servicePtr = (taf_DataIDSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, NULL, "Invalid service reference provided");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    if (servicePtr->readDIDHandlerRef != NULL)
    {
        LE_ERROR("Rx read DID handler is already registered");

        return NULL;
    }

    taf_ReadDIDHandler_t* handlerObjPtr = NULL;
    handlerObjPtr = (taf_ReadDIDHandler_t*)le_mem_ForceAlloc(ReqReadDIDHandlerPool);
    memset(handlerObjPtr, 0, sizeof(taf_ReadDIDHandler_t));

    // Initialize the RxHandler object.
    handlerObjPtr->svcRef     = svcRef;
    handlerObjPtr->func       = handlerPtr;
    handlerObjPtr->ctxPtr     = contextPtr;
    handlerObjPtr->handlerRef = (taf_diagDataID_RxReadDIDMsgHandlerRef_t)le_ref_CreateRef(
            ReqReadDIDHandlerRefMap, handlerObjPtr);

    // Attach handler to service.
    servicePtr->readDIDHandlerRef = handlerObjPtr->handlerRef;

    LE_INFO("Read DID: Registered Rx Handler");

    return handlerObjPtr->handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * ReadDID event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_DataIDSvr::RxReadDIDEventHandler
(
    void* reportPtr
)
{
    LE_DEBUG("RxReadDIDEventHandler!");

    auto &did = taf_DataIDSvr::GetInstance();

    taf_ReadDIDRxMsg_t* rxReadDIDMsgPtr = (taf_ReadDIDRxMsg_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(rxReadDIDMsgPtr == NULL, "rxReadDIDMsgPtr is Null");

    taf_DataIDSvc_t* servicePtr = NULL;
    taf_ReadDIDHandler_t* handlerObjPtr = NULL;

    servicePtr = (taf_DataIDSvc_t*)did.GetServiceObj();
    if (servicePtr == NULL)
    {
        LE_WARN("Not found registered DID service for this request!");
        // UDS_0x22_NRC_21: Bad svc ref
        did.SendNRCResp(rxReadDIDMsgPtr->serviceId, &(rxReadDIDMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(did.RxReadDIDMsgRefMap, rxReadDIDMsgPtr->readDIDRxMsgRef);
        le_mem_Release(rxReadDIDMsgPtr);
        return;
    }

    if (servicePtr->readDIDHandlerRef == NULL)
    {
        LE_WARN("Did not register handler for Read DID service.");
        // UDS_0x22_NRC_21: handler not registered
        did.SendNRCResp(rxReadDIDMsgPtr->serviceId, &(rxReadDIDMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(did.RxReadDIDMsgRefMap, rxReadDIDMsgPtr->readDIDRxMsgRef);
        le_mem_Release(rxReadDIDMsgPtr);
        return;
    }

    // Lookup message handler of this service
    handlerObjPtr =
            (taf_ReadDIDHandler_t*)le_ref_Lookup(did.ReqReadDIDHandlerRefMap,
                    servicePtr->readDIDHandlerRef);
    if (handlerObjPtr == NULL || handlerObjPtr->func == NULL)
    {
        // UDS_0x22_NRC_21: handler is null
        did.SendNRCResp(rxReadDIDMsgPtr->serviceId, &(rxReadDIDMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(did.RxReadDIDMsgRefMap, rxReadDIDMsgPtr->readDIDRxMsgRef);
        le_mem_Release(rxReadDIDMsgPtr);
        return;
    }

    // Add the message in service message list and notify to application.
    rxReadDIDMsgPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->readDIDMsgList, &rxReadDIDMsgPtr->link);
    handlerObjPtr->func(rxReadDIDMsgPtr->readDIDRxMsgRef, rxReadDIDMsgPtr->readDID,
            rxReadDIDMsgPtr->readDIDLen, handlerObjPtr->ctxPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the Rx handler for a given service (ReadDID).
 */
//-------------------------------------------------------------------------------------------------
void taf_DataIDSvr::RemoveRxReadDIDMsgHandler
(
    taf_diagDataID_RxReadDIDMsgHandlerRef_t handlerRef
)
{
    LE_DEBUG("RemoveRxReadDIDMsgHandler!");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");

    taf_DataIDSvc_t* servicePtr = NULL;
    taf_ReadDIDHandler_t* handlerObjPtr = NULL;

    handlerObjPtr = (taf_ReadDIDHandler_t*)le_ref_Lookup(ReqReadDIDHandlerRefMap,
            handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerObjPtr == NULL, "Invalid handlerObjPtr");

    servicePtr = (taf_DataIDSvc_t*)le_ref_Lookup(SvcRefMap, handlerObjPtr->svcRef);
    if (servicePtr == NULL)
    {
        LE_WARN("The handler is not belong to this service.");
        le_ref_DeleteRef(ReqReadDIDHandlerRefMap, handlerRef);
        le_mem_Release(handlerObjPtr);

        return;
    }

    // Detach the handler from service.
    servicePtr->readDIDHandlerRef = NULL;

    // Clear Rx Handler resources
    handlerObjPtr->handlerRef = NULL;
    handlerObjPtr->svcRef     = NULL;
    handlerObjPtr->func       = NULL;
    handlerObjPtr->ctxPtr     = NULL;

    // Free the handler.
    le_ref_DeleteRef(ReqReadDIDHandlerRefMap, handlerRef);
    le_mem_Release(handlerObjPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send readDID response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataIDSvr::SendReadDIDResp
(
    taf_diagDataID_RxReadDIDMsgRef_t rxMsgRef,
    uint8_t errCode,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    LE_DEBUG("SendReadDIDResp");

    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(dataPtr == NULL, LE_BAD_PARAMETER, "Invalid dataPtr");

    le_result_t ret = LE_OK;

    taf_ReadDIDRxMsg_t* rxReadDIDMsgPtr =
            (taf_ReadDIDRxMsg_t*)le_ref_Lookup(RxReadDIDMsgRefMap, rxMsgRef);
    if (rxReadDIDMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    taf_DataIDSvc_t* servicePtr = NULL;
    servicePtr = (taf_DataIDSvc_t*)GetServiceObj();
    if (servicePtr == NULL)
    {
        LE_ERROR("Not found registered DID service for this request!");
        return LE_NOT_FOUND;
    }
    
    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = rxReadDIDMsgPtr->addrInfo.ta;
    addrInfo.ta = rxReadDIDMsgPtr->addrInfo.sa;
    addrInfo.taType = rxReadDIDMsgPtr->addrInfo.taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    if (addrInfo.sa == 0x0000 && addrInfo.ta == 0x0000)
    {
        taf_SnapshotSvr::GetInstance().
                           storeDidsAsSnapshot(dataPtr,
                                               dataSize,
                                               rxReadDIDMsgPtr);
        goto r_release_msg;
    }
#endif
    if (errCode == 0)
    {
        // Positive response.
        ret = backend.RespDiagPositive(reqReadDIDSvcId, &addrInfo, dataPtr, dataSize);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send read DID positive response.(%d)", ret);
            goto r_release_msg;
        }
    }
    else
    {
        // Negative response.
        ret = backend.RespDiagNegative(reqReadDIDSvcId, &addrInfo, errCode);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send read DID negative response.(%d)", ret);
            goto r_release_msg;
        }
    }

r_release_msg:

    // Remove the message from service message list.
    le_dls_Remove(&(servicePtr->readDIDMsgList), &(rxReadDIDMsgPtr->link));

    // Free the message
    le_ref_DeleteRef(RxReadDIDMsgRefMap, rxReadDIDMsgPtr->readDIDRxMsgRef);
    le_mem_Release(rxReadDIDMsgPtr);

    return ret;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a Rx handler for a given service (WriteDID).
 */
//-------------------------------------------------------------------------------------------------
taf_diagDataID_RxWriteDIDMsgHandlerRef_t taf_DataIDSvr::AddRxWriteDIDMsgHandler
(
    taf_diagDataID_ServiceRef_t svcRef,
    taf_diagDataID_RxWriteDIDMsgHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_DEBUG("AddRxWriteDIDMsgHandler!");

    taf_DataIDSvc_t* servicePtr = (taf_DataIDSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, NULL, "Invalid service reference provided");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    if (servicePtr->writeDIDHandlerRef != NULL)
    {
        LE_ERROR("Rx write DID handler is already registered");

        return NULL;
    }

    taf_WriteDIDHandler_t* handlerObjPtr = NULL;
    handlerObjPtr = (taf_WriteDIDHandler_t*)le_mem_ForceAlloc(ReqWriteDIDHandlerPool);
    memset(handlerObjPtr, 0, sizeof(taf_WriteDIDHandler_t));

    // Initialize the RxHandler object.
    handlerObjPtr->svcRef     = svcRef;
    handlerObjPtr->func       = handlerPtr;
    handlerObjPtr->ctxPtr     = contextPtr;
    handlerObjPtr->handlerRef = (taf_diagDataID_RxWriteDIDMsgHandlerRef_t)le_ref_CreateRef(
            ReqWriteDIDHandlerRefMap, handlerObjPtr);

    // Attach handler to service.
    servicePtr->writeDIDHandlerRef = handlerObjPtr->handlerRef;

    LE_INFO("Write DID: Registered Rx Handler");

    return handlerObjPtr->handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * WriteDID event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_DataIDSvr::RxWriteDIDEventHandler
(
    void* reportPtr
)
{
    LE_DEBUG("RxWriteDIDEventHandler!");

    auto &did = taf_DataIDSvr::GetInstance();

    taf_WriteDIDRxMsg_t* rxWriteDIDMsgPtr = (taf_WriteDIDRxMsg_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(rxWriteDIDMsgPtr == NULL, "rxWriteDIDMsgPtr is Null");

    taf_DataIDSvc_t* servicePtr = NULL;
    taf_WriteDIDHandler_t* handlerObjPtr = NULL;

    servicePtr = (taf_DataIDSvc_t*)did.GetServiceObj();
    if (servicePtr == NULL)
    {
        LE_WARN("Not found registered DID service for this request!");
        // UDS_0x2E_NRC_21: service pointer is null
        did.SendNRCResp(rxWriteDIDMsgPtr->serviceId, &(rxWriteDIDMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(did.RxWriteDIDMsgRefMap, rxWriteDIDMsgPtr->writeDIDRxMsgRef);
        le_mem_Release(rxWriteDIDMsgPtr);
        return;
    }

    if (servicePtr->writeDIDHandlerRef == NULL)
    {
        LE_WARN("Did not register handler for write DID service.");
        // UDS_0x2E_NRC_21: handler is not registered
        did.SendNRCResp(rxWriteDIDMsgPtr->serviceId, &(rxWriteDIDMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(did.RxWriteDIDMsgRefMap, rxWriteDIDMsgPtr->writeDIDRxMsgRef);
        le_mem_Release(rxWriteDIDMsgPtr);
        return;
    }

    // Lookup message handler of this service
    handlerObjPtr =
            (taf_WriteDIDHandler_t*)le_ref_Lookup(did.ReqWriteDIDHandlerRefMap,
                    servicePtr->writeDIDHandlerRef);
    if (handlerObjPtr == NULL || handlerObjPtr->func == NULL)
    {
        // UDS_0x2E_NRC_21: handler is null
        did.SendNRCResp(rxWriteDIDMsgPtr->serviceId, &(rxWriteDIDMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(did.RxWriteDIDMsgRefMap, rxWriteDIDMsgPtr->writeDIDRxMsgRef);
        le_mem_Release(rxWriteDIDMsgPtr);
        return;
    }

    // Add the message in service message list and notify to application.
    rxWriteDIDMsgPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->writeDIDMsgList, &rxWriteDIDMsgPtr->link);
    handlerObjPtr->func(rxWriteDIDMsgPtr->writeDIDRxMsgRef, rxWriteDIDMsgPtr->writeDID,
            handlerObjPtr->ctxPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the Rx handler for a given service (WriteDID).
 */
//-------------------------------------------------------------------------------------------------
void taf_DataIDSvr::RemoveRxWriteDIDMsgHandler
(
    taf_diagDataID_RxWriteDIDMsgHandlerRef_t handlerRef
)
{
    LE_DEBUG("RemoveRxWriteDIDMsgHandler!");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");

    taf_DataIDSvc_t* servicePtr = NULL;
    taf_WriteDIDHandler_t* handlerObjPtr = NULL;

    handlerObjPtr = (taf_WriteDIDHandler_t*)le_ref_Lookup(ReqWriteDIDHandlerRefMap,
            handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerObjPtr == NULL, "Invalid handlerObjPtr");

    servicePtr = (taf_DataIDSvc_t*)le_ref_Lookup(SvcRefMap, handlerObjPtr->svcRef);
    if (servicePtr == NULL)
    {
        LE_WARN("The handler is not belong to this service.");
        le_ref_DeleteRef(ReqWriteDIDHandlerRefMap, handlerRef);
        le_mem_Release(handlerObjPtr);

        return;
    }

    // Detach the handler from service.
    servicePtr->writeDIDHandlerRef = NULL;

    // Clear Rx Handler resources
    handlerObjPtr->handlerRef = NULL;
    handlerObjPtr->svcRef     = NULL;
    handlerObjPtr->func       = NULL;
    handlerObjPtr->ctxPtr     = NULL;

    // Free the handler.
    le_ref_DeleteRef(ReqWriteDIDHandlerRefMap, handlerRef);
    le_mem_Release(handlerObjPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the dataRecord of the Rx writeDID message.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataIDSvr::GetWriteDataRecord
(
    taf_diagDataID_RxWriteDIDMsgRef_t rxMsgRef,
    uint8_t* dataRecordPtr,
    size_t* dataRecordSizePtr
)
{
    LE_DEBUG("GetWriteDataRecord!");

    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(dataRecordPtr == NULL, LE_BAD_PARAMETER, "Invalid payloadPtr");
    TAF_ERROR_IF_RET_VAL(dataRecordSizePtr == NULL, LE_BAD_PARAMETER, "Invalid payloadSizePtr");

    taf_WriteDIDRxMsg_t* rxWriteDIDMsgPtr =
            (taf_WriteDIDRxMsg_t*)le_ref_Lookup(RxWriteDIDMsgRefMap, rxMsgRef);
    if (rxWriteDIDMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    memcpy(dataRecordPtr, rxWriteDIDMsgPtr->dataRec, rxWriteDIDMsgPtr->dataRecLen);
    *dataRecordSizePtr = rxWriteDIDMsgPtr->dataRecLen;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send Write DID response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataIDSvr::SendWriteDIDResp
(
    taf_diagDataID_RxWriteDIDMsgRef_t rxMsgRef,
    uint8_t errCode,
    uint16_t dataId
)
{
    LE_DEBUG("SendWriteDIDResp");

    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    le_result_t ret = LE_OK;

    taf_WriteDIDRxMsg_t* rxWriteDIDMsgPtr =
            (taf_WriteDIDRxMsg_t*)le_ref_Lookup(RxWriteDIDMsgRefMap, rxMsgRef);
    if (rxWriteDIDMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    auto &backend = taf_DiagBackend::GetInstance();

    taf_DataIDSvc_t* servicePtr = (taf_DataIDSvc_t*)GetServiceObj();
    if (servicePtr == NULL)
    {
        LE_ERROR("Not found registered write DID service for this request!");
        ret = LE_NOT_FOUND;
        goto w_release_msg;
    }

    // Call UDS function to send the response message.
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = rxWriteDIDMsgPtr->addrInfo.ta;
    addrInfo.ta = rxWriteDIDMsgPtr->addrInfo.sa;
    addrInfo.taType = rxWriteDIDMsgPtr->addrInfo.taType;

    writeDataId[0] = ((dataId & 0xFF00) >> 8);
    writeDataId[1] = (dataId & 0x00FF);

    if (errCode == 0)
    {
        // Positive response.
        ret = backend.RespDiagPositive(reqWriteDIDSvcId, &addrInfo, writeDataId,
                sizeof(uint16_t));
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send write DID positive response.(%d)", ret);
            goto w_release_msg;
        }
    }
    else
    {
        // Negative response.
        ret = backend.RespDiagNegative(reqWriteDIDSvcId, &addrInfo, errCode);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send write DID negative response.(%d)", ret);
            goto w_release_msg;
        }
    }


w_release_msg:
    // Remove the message from service message list.
    le_dls_Remove(&(servicePtr->writeDIDMsgList), &(rxWriteDIDMsgPtr->link));

    // Free the message
    le_ref_DeleteRef(RxWriteDIDMsgRefMap, rxWriteDIDMsgPtr->writeDIDRxMsgRef);
    le_mem_Release(rxWriteDIDMsgPtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send NRC response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataIDSvr::SendNRCResp
(
    uint8_t sid,
    const taf_uds_AddrInfo_t*  addrInfoPtr,
    uint8_t errCode
)
{
    LE_DEBUG("SendNRCResp");

    TAF_ERROR_IF_RET_VAL(addrInfoPtr == NULL, LE_BAD_PARAMETER, "Invalid addrInfoPtr");

    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = addrInfoPtr->ta;
    addrInfo.ta = addrInfoPtr->sa;
    addrInfo.taType = addrInfoPtr->taType;

    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();
    backend.RespDiagNegative(sid, &addrInfo, errCode);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the created service and release the alloted memory.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataIDSvr::RemoveSvc
(
    taf_diagDataID_ServiceRef_t svcRef
)
{
    LE_DEBUG("RemoveSvc");

    taf_DataIDSvc_t* servicePtr = (taf_DataIDSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid servicePtr");

    // Release DID message resources.
    ClearReadDIDMsgList(servicePtr);
    ClearWriteDIDMsgList(servicePtr);

    // Clear the registered readDID handler
    if (servicePtr->readDIDHandlerRef != NULL)
    {
        RemoveRxReadDIDMsgHandler(servicePtr->readDIDHandlerRef);
        servicePtr->readDIDHandlerRef = NULL;
    }

    // Clear the registered WriteDID handler
    if (servicePtr->writeDIDHandlerRef != NULL)
    {
        RemoveRxWriteDIDMsgHandler(servicePtr->writeDIDHandlerRef);
        servicePtr->writeDIDHandlerRef = NULL;
    }

    // Clear service object
    le_ref_DeleteRef(SvcRefMap, (void*)servicePtr->svcRef);
    le_mem_Release(servicePtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear readDID message list.
 */
//-------------------------------------------------------------------------------------------------
void taf_DataIDSvr::ClearReadDIDMsgList
(
    taf_DataIDSvc_t* servicePtr
)
{
    LE_DEBUG("ClearReadDIDMsgList");
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear the UDS Rx message of readDID list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->readDIDMsgList);
    while (linkPtr != NULL)
    {
        taf_ReadDIDRxMsg_t* rxReadDIDMsgPtr = CONTAINER_OF(linkPtr, taf_ReadDIDRxMsg_t, link);
        if (rxReadDIDMsgPtr != NULL)
        {
            LE_INFO("Release ReqMsg(ref=%p)", rxReadDIDMsgPtr->readDIDRxMsgRef);
            // Free the message
            le_ref_DeleteRef(RxReadDIDMsgRefMap, rxReadDIDMsgPtr->readDIDRxMsgRef);
            le_mem_Release(rxReadDIDMsgPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->readDIDMsgList);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear writeDID message list.
 */
//-------------------------------------------------------------------------------------------------
void taf_DataIDSvr::ClearWriteDIDMsgList
(
    taf_DataIDSvc_t* servicePtr
)
{
    LE_DEBUG("ClearWriteDIDMsgList");
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear the UDS Rx message of writeDID list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->writeDIDMsgList);
    while (linkPtr != NULL)
    {
        taf_WriteDIDRxMsg_t* rxWriteDIDMsgPtr = CONTAINER_OF(linkPtr, taf_WriteDIDRxMsg_t, link);
        if (rxWriteDIDMsgPtr != NULL)
        {
            LE_INFO("Release ReqMsg(ref=%p)", rxWriteDIDMsgPtr->writeDIDRxMsgRef);
            // Free the message
            le_ref_DeleteRef(RxWriteDIDMsgRefMap, rxWriteDIDMsgPtr->writeDIDRxMsgRef);
            le_mem_Release(rxWriteDIDMsgPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->writeDIDMsgList);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Session close handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_DataIDSvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    LE_DEBUG("OnClientDisconnection");

    auto &did = taf_DataIDSvr::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(did.SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_DataIDSvc_t* servicePtr = (taf_DataIDSvc_t *)le_ref_GetValue(iterRef);
        LE_ASSERT(servicePtr != NULL);
        LE_ASSERT(servicePtr->svcRef == le_ref_GetSafeRef(iterRef));

        if (servicePtr->sessionRef == sessionRef)
        {
            did.RemoveSvc(servicePtr->svcRef);
        }
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_DataIDSvr::Init
(
    void
)
{
    LE_INFO("taf_DataIDSvr Init!");

    // Create memory pools.
    SvcPool = le_mem_CreatePool("DIDSvcPool", sizeof(taf_DataIDSvc_t));
    RxReadDIDMsgPool = le_mem_CreatePool("ReadDIDRxMsgPool", sizeof(taf_ReadDIDRxMsg_t));
    ReqReadDIDHandlerPool = le_mem_CreatePool("ReadDIDReqHandlerPool",
            sizeof(taf_ReadDIDHandler_t));
    RxWriteDIDMsgPool = le_mem_CreatePool("WriteDIDRxMsgPool", sizeof(taf_WriteDIDRxMsg_t));
    ReqWriteDIDHandlerPool = le_mem_CreatePool("WriteDIDReqHandlerPool",
            sizeof(taf_WriteDIDHandler_t));

    // Create reference maps
    SvcRefMap = le_ref_CreateMap("DIDSvcRefMap", DEFAULT_SVC_REF_CNT);
    RxReadDIDMsgRefMap = le_ref_CreateMap("ReadDIDRxMsgRefMap", DEFAULT_RX_MSG_REF_CNT);
    ReqReadDIDHandlerRefMap = le_ref_CreateMap("ReadDIDReqHandlerRefMap",
            DEFAULT_RX_HANDLER_REF_CNT);
    RxWriteDIDMsgRefMap = le_ref_CreateMap("WriteDIDRxMsgRefMap", DEFAULT_RX_MSG_REF_CNT);
    ReqWriteDIDHandlerRefMap = le_ref_CreateMap("WriteDIDReqHandlerRefMap",
            DEFAULT_RX_HANDLER_REF_CNT);

    // Create the event and add event handler for WriteDID (0x22).
    ReadDIDEvent = le_event_CreateIdWithRefCounting("ReadDIDEvent");
    ReadDIDEventHandlerRef = le_event_AddHandler("ReadDIDEventHandlerRef", ReadDIDEvent,
            taf_DataIDSvr::RxReadDIDEventHandler);

    // Create the event and add event handler for WriteDID (0x2E).
    WriteDIDEvent = le_event_CreateIdWithRefCounting("WriteDIDEvent");
    WriteDIDEventHandlerRef = le_event_AddHandler("WriteDIDEventHandlerRef", WriteDIDEvent,
            taf_DataIDSvr::RxWriteDIDEventHandler);

    // Set session close handler
    le_msg_AddServiceCloseHandler(taf_diagDataID_GetServiceRef(), OnClientDisconnection, NULL);

    auto& backend = taf_DiagBackend::GetInstance();
    backend.RegisterUdsService(reqReadDIDSvcId, this);
    backend.RegisterUdsService(reqWriteDIDSvcId, this);

    LE_INFO("Diag DataID Service started!");
}
