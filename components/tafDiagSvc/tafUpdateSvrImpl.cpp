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
#include "tafDiagBackend.hpp"
#include "tafUpdateSvr.hpp"
#include <arpa/inet.h>

using namespace telux::tafsvc;
using namespace std;

//--------------------------------------------------------------------------------------------------
/**
 * Get the single instance of TelAF DiagUpdate server.
 */
//--------------------------------------------------------------------------------------------------
taf_UpdateSvr& taf_UpdateSvr::GetInstance()
{
    static taf_UpdateSvr instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_UpdateSvr::Init
(
    void
)
{
    // Create reference maps.
    SvcRefMap = le_ref_CreateMap("Server SvcRefMap", DEFAULT_SVC_REF_CNT);
    RxFileXferMsgRefMap = le_ref_CreateMap("Server RxFileXferMsgRefMap", DEFAULT_RXMSG_REF_CNT);
    RxXferDataMsgRefMap = le_ref_CreateMap("Server RxXferDataMsgRefMap", DEFAULT_RXMSG_REF_CNT);
    RxXferExitMsgRefMap = le_ref_CreateMap("Server RxXferExitMsgRefMap", DEFAULT_RXMSG_REF_CNT);
    RxFileXferHandlerRefMap = le_ref_CreateMap("Server RxFileXferHandlerRefMap",
            DEFAULT_RXHANDLER_REF_CNT);
    RxXferDataHandlerRefMap = le_ref_CreateMap("Server RxXferDataHandlerRefMap",
            DEFAULT_RXHANDLER_REF_CNT);
    RxXferExitHandlerRefMap = le_ref_CreateMap("Server RxXferExitHandlerRefMap",
            DEFAULT_RXHANDLER_REF_CNT);

    // Create memory pools.
    SvcPool = le_mem_CreatePool("Server SvcPool", sizeof(taf_UpdateSvc_t));
    RxFileXferMsgPool = le_mem_CreatePool("Server RxFileXferMsgPool", sizeof(taf_FileXferRxMsg_t));
    RxXferDataMsgPool = le_mem_CreatePool("Server RxXferDataMsgPool", sizeof(taf_XferDataRxMsg_t));
    RxXferExitMsgPool = le_mem_CreatePool("Server RxXferExitMsgPool", sizeof(taf_XferExitRxMsg_t));

    RxFileXferHandlerPool = le_mem_CreatePool("Server RxFileXferHandlerPool",
            sizeof(taf_FileXferHandler_t));
    RxXferDataHandlerPool = le_mem_CreatePool("Server RxXferDataHandlerPool",
            sizeof(taf_XferDataHandler_t));
    RxXferExitHandlerPool = le_mem_CreatePool("Server RxXferExitHandlerPool",
            sizeof(taf_XferExitHandler_t));

    // Create the event and add event handler for RequestFileTransfer (0x38).
    FileXferEvent = le_event_CreateIdWithRefCounting("UpdateSvc RequestFileTransfer Event");
    FileXferEventHandlerRef = le_event_AddHandler("UpdateSvc RequestFileTransfer Event Handler",
            FileXferEvent, taf_UpdateSvr::RxFileXferEventHandler);

    // Create the event and add event handler for TransferData (0x36).
    XferDataEvent = le_event_CreateIdWithRefCounting("UpdateSvc TransferData Event");
    XferDataEventHandlerRef = le_event_AddHandler("UpdateSvc TransferData Event Handler",
            XferDataEvent, taf_UpdateSvr::RxXferDataEventHandler);

    // Create the event and add event handler for RequestTransferExit (0x37).
    XferExitEvent = le_event_CreateIdWithRefCounting("UpdateSvc TransferData Event");
    XferExitEventHandlerRef = le_event_AddHandler("UpdateSvc TransferData Event Handler",
            XferExitEvent, taf_UpdateSvr::RxXferExitEventHandler);

    // Create client session close hander.
    le_msg_AddServiceCloseHandler(taf_diagUpdate_GetServiceRef(),
                                  taf_UpdateSvr::OnClientDisconnection, NULL);

    auto& backend = taf_DiagBackend::GetInstance();

    backend.RegisterUdsService(reqFileXferSvcId, this);
    backend.RegisterUdsService(fileDataXferSvcId, this);
    backend.RegisterUdsService(fileXferExitSvcId, this);

    LE_INFO("taf_DiagUpdateSvr Service started");
}

void taf_UpdateSvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    auto& update = taf_UpdateSvr::GetInstance();

    le_ref_IterRef_t iterRef = le_ref_GetIterator(update.SvcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_UpdateSvc_t* svcPtr = (taf_UpdateSvc_t*)le_ref_GetValue(iterRef);
        if (svcPtr != NULL && svcPtr->sessionRef == sessionRef)
        {
            update.RemoveUpdateSvc(svcPtr->svcRef);
        }
    }
}

taf_diagUpdate_ServiceRef_t taf_UpdateSvr::CreateUpdateSvc
(
)
{
    // Search the service.
    taf_UpdateSvc_t* svcPtr = FindSvcInList();

    // Create a service object if it doesn't exist in the list.
    if (svcPtr == NULL)
    {
        svcPtr = (taf_UpdateSvc_t *)le_mem_ForceAlloc(SvcPool);
        memset(svcPtr, 0, sizeof(taf_UpdateSvc_t));

        // Init the service Rx Handler.
        svcPtr->fileXferRef = NULL;
        svcPtr->xferDataRef = NULL;
        svcPtr->xferExitRef = NULL;

        // Init update message list.
        svcPtr->reqFileXferMsgList  = LE_DLS_LIST_INIT;
        svcPtr->xferDataMsgList     = LE_DLS_LIST_INIT;
        svcPtr->reqXferExitMsgList  = LE_DLS_LIST_INIT;

        // Attach the service to the client.
        svcPtr->sessionRef = taf_diagUpdate_GetClientSessionRef();

        // Create a Safe Reference for this service object
        svcPtr->svcRef = (taf_diagUpdate_ServiceRef_t)le_ref_CreateRef(SvcRefMap, svcPtr);

        // Init update service state.
        svcPtr->state = TAF_DIAG_UPDATE_INIT;

        LE_INFO("svcRef %p of client %p is created for Diag update service.",
                svcPtr->svcRef, svcPtr->sessionRef);
    }
    else
    {
        // Only the service owner app can get the service reference for subsequent operations.
        if (svcPtr->sessionRef != taf_diagUpdate_GetClientSessionRef())
        {
            LE_ERROR("The Service is not created by the client.");
            return NULL;
        }
    }

    LE_INFO("Get serviceRef %p for Diag update service.", svcPtr->svcRef);

    return svcPtr->svcRef;
}

taf_UpdateSvc_t* taf_UpdateSvr::FindSvcInList
(
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_UpdateSvc_t* svcPtr = (taf_UpdateSvc_t *)le_ref_GetValue(iterRef);
        if (svcPtr != NULL)
        {
            return svcPtr;
        }
    }

    return NULL;
}

le_result_t taf_UpdateSvr::RemoveUpdateSvc
(
    taf_diagUpdate_ServiceRef_t svcRef
)
{
    taf_UpdateSvc_t* svcPtr = (taf_UpdateSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    if (svcPtr == NULL)
    {
        LE_ERROR("Bad parameters");
        return LE_BAD_PARAMETER;
    }

    // Release update message resources.
    ClearFileXferMsgList(svcPtr);
    ClearXferDataMsgList(svcPtr);
    ClearXferExitMsgList(svcPtr);

    // Clear the registered handler
    if (svcPtr->fileXferRef != NULL)
    {
        RemoveRxFileXferReqHandler(svcPtr->fileXferRef);
        svcPtr->fileXferRef = NULL;
    }

    if (svcPtr->xferDataRef != NULL)
    {
        RemoveRxXferDataReqHandler(svcPtr->xferDataRef);
        svcPtr->xferDataRef = NULL;
    }

    if (svcPtr->xferExitRef != NULL)
    {
        RemoveRxXferExitReqHandler(svcPtr->xferExitRef);
        svcPtr->xferExitRef = NULL;
    }

    // Clear service object
    le_ref_DeleteRef(SvcRefMap, (void*)svcPtr->svcRef);
    le_mem_Release(svcPtr);

    return LE_OK;
}

void taf_UpdateSvr::ClearFileXferMsgList
(
    taf_UpdateSvc_t* svcPtr
)
{
    // Clear the UDS Rx message of RequestFileTransfer list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&svcPtr->reqFileXferMsgList);
    while (linkPtr != NULL)
    {
        taf_FileXferRxMsg_t* msgPtr = CONTAINER_OF(linkPtr, taf_FileXferRxMsg_t, link);
        if (msgPtr != NULL)
        {
            LE_INFO("Release ReqMsg(ref=%p, sid=0x%x)",
                msgPtr->rxMsgRef, msgPtr->serviceId);

            // Free the message
            le_ref_DeleteRef(RxFileXferMsgRefMap, msgPtr->rxMsgRef);
            le_mem_Release(msgPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&svcPtr->reqFileXferMsgList);
    }
}

void taf_UpdateSvr::ClearXferDataMsgList
(
    taf_UpdateSvc_t* svcPtr
)
{
    // Clear the UDS Rx message of RequestFileTransfer list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&svcPtr->xferDataMsgList);
    while (linkPtr != NULL)
    {
        taf_XferDataRxMsg_t* msgPtr = CONTAINER_OF(linkPtr, taf_XferDataRxMsg_t, link);
        if (msgPtr != NULL)
        {
            LE_INFO("Release ReqMsg(ref=%p, sid=0x%x)",
                msgPtr->rxMsgRef, msgPtr->serviceId);

            // Free the message
            le_ref_DeleteRef(RxXferDataMsgRefMap, msgPtr->rxMsgRef);
            le_mem_Release(msgPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&svcPtr->xferDataMsgList);
    }
}

void taf_UpdateSvr::ClearXferExitMsgList
(
    taf_UpdateSvc_t* svcPtr
)
{
    // Clear the UDS Rx message of RequestFileTransfer list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&svcPtr->reqXferExitMsgList);
    while (linkPtr != NULL)
    {
        taf_XferExitRxMsg_t* msgPtr = CONTAINER_OF(linkPtr, taf_XferExitRxMsg_t, link);
        if (msgPtr != NULL)
        {
            LE_INFO("Release ReqMsg(ref=%p, sid=0x%x)",
                msgPtr->rxMsgRef, msgPtr->serviceId);

            // Free the message
            le_ref_DeleteRef(RxXferExitMsgRefMap, msgPtr->rxMsgRef);
            le_mem_Release(msgPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&svcPtr->reqXferExitMsgList);
    }
}

void taf_UpdateSvr::UDSMsgHandler
(
    const taf_uds_AddrInfo_t* addrPtr,
    uint8_t sid,
    uint8_t* msgPtr,
    size_t msgLen
)
{
    uint8_t nrc = 0;
    uint8_t dataPtrPos = 1;  // Skip sid

    TAF_ERROR_IF_RET_NIL(msgPtr == NULL, "Invalid dataPtr");

    if (sid == SID_REQUEST_FILE_TRANSFER)  // RequestFileTransfer service of UDS
    {
        uint8_t moo;    // Mode Of Operation
        uint16_t fileNameLen;

        moo = msgPtr[dataPtrPos];
        if (moo == 0 || moo > TAF_DIAG_UPDATE_RESUME_FILE)
        {
            LE_DEBUG("Mode of operation(0x%x) is out of range", moo);
            nrc = TAF_DIAG_REQUEST_OUT_OF_RANGE;
            goto errOut;
        }

        dataPtrPos += 1;

        fileNameLen = ntohs(*((uint16_t*)(msgPtr + dataPtrPos)));
        if (fileNameLen >= TAF_DIAGUPDATE_MAX_PATH_AND_NAME_SIZE)
        {
            LE_DEBUG("File name length(%d) is invalid", fileNameLen);
            nrc = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            goto errOut;
        }

        dataPtrPos += sizeof(uint16_t);

        uint8_t fileName[TAF_DIAGUPDATE_MAX_PATH_AND_NAME_SIZE];
        memcpy(fileName, msgPtr + dataPtrPos, fileNameLen);
        fileName[fileNameLen] = (uint8_t)'\0';
        dataPtrPos += fileNameLen;

        uint8_t dataFormatId = 0;
        if ((moo != TAF_DIAG_UPDATE_DELETE_FILE) && (moo != TAF_DIAG_UPDATE_READ_DIR))
        {
            // If the modeOfOperation parameter equals to 0216 (DeleteFile) and 0516 (ReadDir),
            // this parameter shall not be included in the request message.
            dataFormatId = msgPtr[dataPtrPos];
            dataPtrPos += 1;
        }

        uint8_t fileSizeParamLen = 0;
        uint32_t fileSizeUnComp = 0;
        uint32_t fileSizeComp = 0;
        if ((moo != TAF_DIAG_UPDATE_DELETE_FILE) && (moo != TAF_DIAG_UPDATE_READ_FILE)
            && (moo != TAF_DIAG_UPDATE_READ_DIR))
        {
            // If the modeOfOperation parameter equals to 0216 (DeleteFile), 0416 (ReadFile)
            // or 0516 (ReadDir), this parameter shall not be included in the request message.
            fileSizeParamLen = msgPtr[dataPtrPos];
            dataPtrPos += 1;

            memcpy(&fileSizeUnComp, msgPtr + dataPtrPos, fileSizeParamLen);
            dataPtrPos += fileSizeParamLen;
            fileSizeUnComp = fileSizeUnComp << ((sizeof(uint32_t) - fileSizeParamLen) * 8);
            fileSizeUnComp = ntohl(fileSizeUnComp);

            memcpy(&fileSizeComp, msgPtr + dataPtrPos, fileSizeParamLen);
            dataPtrPos += fileSizeParamLen;
            fileSizeComp = fileSizeComp << ((sizeof(uint32_t) - fileSizeParamLen) * 8);
            fileSizeComp = ntohl(fileSizeComp);
            LE_DEBUG("fileSizeParamLen:0x%x, fileSizeUnComp:0x%x, fileSizeComp:0x%x",
                fileSizeParamLen, fileSizeUnComp, fileSizeComp);
        }

        LE_DEBUG("File:%s, moo: %d", fileName, moo);

        // Create the incoming Rx message from UDS stack.
        taf_FileXferRxMsg_t* rxFileXferMsgPtr =
                (taf_FileXferRxMsg_t*)le_mem_ForceAlloc(RxFileXferMsgPool);
        memset(rxFileXferMsgPtr, 0, sizeof(taf_FileXferRxMsg_t));

        // Fill message fields.
        rxFileXferMsgPtr->serviceId = sid;
        rxFileXferMsgPtr->operationType = (taf_diagUpdate_ModeOfOpsType_t)moo;
        rxFileXferMsgPtr->filePathAndNameLen = fileNameLen;
        le_utf8_Copy((char*)rxFileXferMsgPtr->filePathAndName, (const char*)fileName,
            strlen((const char*)fileName) + 1, NULL);
        rxFileXferMsgPtr->dataFormatID = dataFormatId;
        rxFileXferMsgPtr->fileSizeParamLen = fileSizeParamLen;
        rxFileXferMsgPtr->unCompFileSize = fileSizeUnComp;
        rxFileXferMsgPtr->compFileSize = fileSizeComp;
        memcpy(&rxFileXferMsgPtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
        rxFileXferMsgPtr->link = LE_DLS_LINK_INIT;
        rxFileXferMsgPtr->rxMsgRef = (taf_diagUpdate_RxFileXferMsgRef_t)
            le_ref_CreateRef(RxFileXferMsgRefMap, rxFileXferMsgPtr);

        // Report the request message to message handler in service layer.
        le_event_ReportWithRefCounting(FileXferEvent, rxFileXferMsgPtr);
    }
    else if (sid == SID_TRANSFER_DATA)  // TransferData service of UDS
    {
        uint8_t blkSeqCnt;

        blkSeqCnt = msgPtr[dataPtrPos];
        dataPtrPos += 1;

        // 'transferRequestParameterRecord' parameter record contains parameter(s)
        //  which are required by the server to support the transfer of data.
        // Format and length of this parameter(s) are vehicle manufacturer specific.
        if ((msgLen - dataPtrPos) > TAF_DIAGUPDATE_MAX_XFER_PARAM_REC_SIZE)
        {
            LE_DEBUG("Message length(%" PRIuS ") is out of range", msgLen - dataPtrPos);
            nrc = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            goto errOut;
        }

        // Create the incoming Rx message from UDS stack.
        taf_XferDataRxMsg_t* rxXferDataMsgPtr =
                (taf_XferDataRxMsg_t*)le_mem_ForceAlloc(RxXferDataMsgPool);
        memset(rxXferDataMsgPtr, 0, sizeof(taf_XferDataRxMsg_t));

        // Fill message fields.
        rxXferDataMsgPtr->serviceId = sid;
        rxXferDataMsgPtr->blockSeqCount = blkSeqCnt;
        memcpy(rxXferDataMsgPtr->xferDataRec, msgPtr + dataPtrPos, msgLen - dataPtrPos);
        rxXferDataMsgPtr->xferDataSize = msgLen - dataPtrPos;

        memcpy(&rxXferDataMsgPtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
        rxXferDataMsgPtr->link = LE_DLS_LINK_INIT;
        rxXferDataMsgPtr->rxMsgRef = (taf_diagUpdate_RxXferDataMsgRef_t)
            le_ref_CreateRef(RxXferDataMsgRefMap, rxXferDataMsgPtr);

        // Report the request message to message handler in service layer.
        le_event_ReportWithRefCounting(XferDataEvent, rxXferDataMsgPtr);
    }
    else if (sid == SID_REQUEST_TRANSFER_EXIT)  // RequestTransferExit service of UDS.
    {
        // This parameter record contains parameter(s), which are required
        // by the server to support the transfer of data.
        // Format and length of this parameter(s) are vehicle manufacturer specific.
        if ((msgLen - dataPtrPos) > TAF_DIAGUPDATE_MAX_XFER_PARAM_REC_SIZE)
        {
            LE_DEBUG("Message length(%" PRIuS ") is out of range", msgLen - dataPtrPos);
            nrc = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            goto errOut;
        }


        // Create the incoming Rx message from UDS stack.
        taf_XferExitRxMsg_t* rxXferExitMsgPtr =
                (taf_XferExitRxMsg_t*)le_mem_ForceAlloc(RxXferExitMsgPool);
        memset(rxXferExitMsgPtr, 0, sizeof(taf_XferExitRxMsg_t));

        // Fill message fields.
        rxXferExitMsgPtr->serviceId = sid;
        memcpy(rxXferExitMsgPtr->exitDataRec, msgPtr + dataPtrPos, msgLen - dataPtrPos);
        rxXferExitMsgPtr->exitDataSize = msgLen - dataPtrPos;

        memcpy(&rxXferExitMsgPtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
        rxXferExitMsgPtr->link = LE_DLS_LINK_INIT;
        rxXferExitMsgPtr->rxMsgRef = (taf_diagUpdate_RxXferExitMsgRef_t)
        le_ref_CreateRef(RxXferExitMsgRefMap, rxXferExitMsgPtr);

        // Report the request message to message handler in service layer.
        le_event_ReportWithRefCounting(XferExitEvent, rxXferExitMsgPtr);
    }
    else
    {
        LE_DEBUG("Service(0x%x) is invalid for update", sid);
        nrc = TAF_DIAG_SERVICE_NOT_SUPPORTED; // ServiceNotSupported
        goto errOut;
    }

    return;
errOut:
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = addrPtr->ta;
    addrInfo.ta = addrPtr->sa;
    addrInfo.taType = addrPtr->taType;
    RspNegativeMsg(&addrInfo, sid, nrc);

    return;
}

le_result_t taf_UpdateSvr::RspNegativeMsg
(
    taf_uds_AddrInfo_t* addrPtr,
    uint8_t sid,
    uint8_t nrc
)
{
    // Negative message: 0x7F SID NRC
    auto& backend = taf_DiagBackend::GetInstance();

    return backend.RespDiagNegative(sid, addrPtr, nrc);
}

le_result_t taf_UpdateSvr::RspPositiveMsg
(
    taf_uds_AddrInfo_t* addrPtr,
    uint8_t sid,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    auto& backend = taf_DiagBackend::GetInstance();

    if (dataPtr == NULL || dataSize == 0)
    {
        return backend.RespDiagPositive(sid, addrPtr);
    }
    else
    {
        return backend.RespDiagPositive(sid, addrPtr, dataPtr, dataSize);
    }
}

taf_diagUpdate_RxFileXferMsgHandlerRef_t taf_UpdateSvr::AddRxFileXferReqHandler
(
    taf_diagUpdate_ServiceRef_t svcRef,
    taf_diagUpdate_RxFileXferMsgHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    // Search the service in the list.
    taf_UpdateSvc_t* svcPtr = (taf_UpdateSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    if ((svcPtr == NULL) || (handlerPtr == NULL))
    {
        LE_ERROR("Bad parameters");
        return NULL;
    }

    // Check if a Rx Handler for the service is already registered.
    if (svcPtr->fileXferRef != NULL)
    {
        LE_ERROR("Rx Handler for svcRef %p is already registered.", svcPtr->svcRef);
        return NULL;
    }

    // Create and set the Rx Handler.
    taf_FileXferHandler_t* handlerCtxPtr =
            (taf_FileXferHandler_t*)le_mem_ForceAlloc(RxFileXferHandlerPool);
    memset(handlerCtxPtr, 0, sizeof(taf_FileXferHandler_t));

    // Init the fields.
    handlerCtxPtr->svcRef = svcRef;
    handlerCtxPtr->func = handlerPtr;
    handlerCtxPtr->context = contextPtr;
    handlerCtxPtr->handlerRef = (taf_diagUpdate_RxFileXferMsgHandlerRef_t)
            le_ref_CreateRef(RxFileXferHandlerRefMap, handlerCtxPtr);

    // Attach the Rx Handler to the service.
    svcPtr->fileXferRef = handlerCtxPtr->handlerRef;

    LE_INFO("Created handlerRef(%p) for Service reference %p.",
            handlerCtxPtr->handlerRef, svcPtr->svcRef);

    return handlerCtxPtr->handlerRef;
}

void taf_UpdateSvr::RemoveRxFileXferReqHandler
(
    taf_diagUpdate_RxFileXferMsgHandlerRef_t handlerRef
)
{
    taf_UpdateSvc_t* svcPtr;
    taf_FileXferHandler_t* handlerCtxPtr;

    handlerCtxPtr = (taf_FileXferHandler_t*)le_ref_Lookup(RxFileXferHandlerRefMap, handlerRef);
    if (handlerCtxPtr == NULL)
    {
        LE_ERROR("Invalid reference");
        return;
    }

    svcPtr = (taf_UpdateSvc_t*)le_ref_Lookup(SvcRefMap, handlerCtxPtr->svcRef);
    if (svcPtr == NULL)
    {
        LE_WARN("The handler is not belong to any update service.");
        le_ref_DeleteRef(RxFileXferHandlerRefMap, (void*)handlerRef);
        le_mem_Release(handlerCtxPtr);
        return;
    }

    // Detach the handler from service.
    svcPtr->fileXferRef = NULL;

    // Clear Rx Handler resources
    handlerCtxPtr->context    = NULL;
    handlerCtxPtr->func       = NULL;
    handlerCtxPtr->handlerRef = NULL;
    handlerCtxPtr->svcRef     = NULL;

    le_ref_DeleteRef(RxFileXferHandlerRefMap, (void*)handlerRef);
    le_mem_Release(handlerCtxPtr);
}

taf_diagUpdate_RxXferDataMsgHandlerRef_t taf_UpdateSvr::AddRxXferDataReqHandler
(
    taf_diagUpdate_ServiceRef_t svcRef,
    taf_diagUpdate_RxXferDataMsgHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    // Search the service in the list.
    taf_UpdateSvc_t* svcPtr = (taf_UpdateSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    if ((svcPtr == NULL) || (handlerPtr == NULL))
    {
        LE_ERROR("Bad parameters");
        return NULL;
    }

    // Check if a Rx Handler for the service is already registered.
    if (svcPtr->xferDataRef != NULL)
    {
        LE_ERROR("Rx Handler for svcRef %p is already registered.", svcPtr->svcRef);
        return NULL;
    }

    // Create and set the Rx Handler.
    taf_XferDataHandler_t* handlerCtxPtr =
            (taf_XferDataHandler_t*)le_mem_ForceAlloc(RxXferDataHandlerPool);
    memset(handlerCtxPtr, 0, sizeof(taf_XferDataHandler_t));

    // Init the fields.
    handlerCtxPtr->svcRef = svcRef;
    handlerCtxPtr->func = handlerPtr;
    handlerCtxPtr->context = contextPtr;
    handlerCtxPtr->handlerRef = (taf_diagUpdate_RxXferDataMsgHandlerRef_t)
            le_ref_CreateRef(RxXferDataHandlerRefMap, handlerCtxPtr);

    // Attach the Rx Handler to the service.
    svcPtr->xferDataRef = handlerCtxPtr->handlerRef;

    LE_INFO("Created handlerRef(%p) for Service reference %p.",
            handlerCtxPtr->handlerRef, svcPtr->svcRef);

    return handlerCtxPtr->handlerRef;
}

void taf_UpdateSvr::RemoveRxXferDataReqHandler
(
    taf_diagUpdate_RxXferDataMsgHandlerRef_t handlerRef
)
{
    taf_UpdateSvc_t* svcPtr;
    taf_XferDataHandler_t* handlerCtxPtr;

    handlerCtxPtr = (taf_XferDataHandler_t*)le_ref_Lookup(RxXferDataHandlerRefMap, handlerRef);
    if (handlerCtxPtr == NULL)
    {
        LE_ERROR("Invalid reference");
        return;
    }

    svcPtr = (taf_UpdateSvc_t*)le_ref_Lookup(SvcRefMap, handlerCtxPtr->svcRef);
    if (svcPtr == NULL)
    {
        LE_WARN("The handler is not belong to any update service.");
        le_ref_DeleteRef(RxXferDataHandlerRefMap, (void*)handlerRef);
        le_mem_Release(handlerCtxPtr);
        return;
    }

    // Detach the handler from service.
    svcPtr->xferDataRef = NULL;

    // Clear Rx Handler resources
    handlerCtxPtr->context    = NULL;
    handlerCtxPtr->func       = NULL;
    handlerCtxPtr->handlerRef = NULL;
    handlerCtxPtr->svcRef     = NULL;

    le_ref_DeleteRef(RxXferDataHandlerRefMap, (void*)handlerRef);
    le_mem_Release(handlerCtxPtr);
}

taf_diagUpdate_RxXferExitMsgHandlerRef_t taf_UpdateSvr::AddRxXferExitReqHandler
(
    taf_diagUpdate_ServiceRef_t svcRef,
    taf_diagUpdate_RxXferExitMsgHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    // Search the service in the list.
    taf_UpdateSvc_t* svcPtr = (taf_UpdateSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    if ((svcPtr == NULL) || (handlerPtr == NULL))
    {
        LE_ERROR("Bad parameters");
        return NULL;
    }

    // Check if a Rx Handler for the service is already registered.
    if (svcPtr->xferExitRef != NULL)
    {
        LE_ERROR("Rx Handler for svcRef %p is already registered.", svcPtr->svcRef);
        return NULL;
    }

    // Create and set the Rx Handler.
    taf_XferExitHandler_t* handlerCtxPtr =
            (taf_XferExitHandler_t*)le_mem_ForceAlloc(RxXferExitHandlerPool);
    memset(handlerCtxPtr, 0, sizeof(taf_XferExitHandler_t));

    // Init the fields.
    handlerCtxPtr->svcRef = svcRef;
    handlerCtxPtr->func = handlerPtr;
    handlerCtxPtr->context = contextPtr;
    handlerCtxPtr->handlerRef = (taf_diagUpdate_RxXferExitMsgHandlerRef_t)
            le_ref_CreateRef(RxXferExitHandlerRefMap, handlerCtxPtr);

    // Attach the Rx Handler to the service.
    svcPtr->xferExitRef = handlerCtxPtr->handlerRef;

    LE_INFO("Created handlerRef(%p) for Service reference %p.",
            handlerCtxPtr->handlerRef, svcPtr->svcRef);

    return handlerCtxPtr->handlerRef;
}

void taf_UpdateSvr::RemoveRxXferExitReqHandler
(
    taf_diagUpdate_RxXferExitMsgHandlerRef_t handlerRef
)
{
    taf_UpdateSvc_t* svcPtr;
    taf_XferExitHandler_t* handlerCtxPtr;

    handlerCtxPtr = (taf_XferExitHandler_t*)le_ref_Lookup(RxXferExitHandlerRefMap, handlerRef);
    if (handlerCtxPtr == NULL)
    {
        LE_ERROR("Invalid reference");
        return;
    }

    svcPtr = (taf_UpdateSvc_t*)le_ref_Lookup(SvcRefMap, handlerCtxPtr->svcRef);
    if (svcPtr == NULL)
    {
        LE_WARN("The handler is not belong to any update service.");
        le_ref_DeleteRef(RxXferExitHandlerRefMap, (void*)handlerRef);
        le_mem_Release(handlerCtxPtr);
        return;
    }

    // Detach the handler from service.
    svcPtr->xferExitRef = NULL;

    // Clear Rx Handler resources
    handlerCtxPtr->context    = NULL;
    handlerCtxPtr->func       = NULL;
    handlerCtxPtr->handlerRef = NULL;
    handlerCtxPtr->svcRef     = NULL;

    le_ref_DeleteRef(RxXferExitHandlerRefMap, (void*)handlerRef);
    le_mem_Release(handlerCtxPtr);
}

void taf_UpdateSvr::RxFileXferEventHandler
(
    void* reportPtr
)
{
    uint8_t nrc = 0;
    taf_UpdateSvc_t* svcPtr;
    taf_FileXferRxMsg_t* msgPtr;
    taf_FileXferHandler_t* handlerCtxPtr;

    if (reportPtr == NULL)
    {
        return;
    }

    msgPtr = (taf_FileXferRxMsg_t*)reportPtr;
    taf_UpdateSvr& update = taf_UpdateSvr::GetInstance();

    svcPtr = update.FindSvcInList();
    if (svcPtr == NULL)
    {
        LE_DEBUG("Not found registered update service for this request");
        nrc = TAF_DIAG_CONDITION_NOT_CORRECT;  // conditionsNotCorrect
        goto errOut;
    }

    if ((svcPtr->state == TAF_DIAG_UPDATE_EXIT)
        && (msgPtr->operationType == TAF_DIAG_UPDATE_RESUME_FILE))
    {
        LE_DEBUG("The update state(0x%x) is incorrect", svcPtr->state);
        nrc = TAF_DIAG_REQUEST_SEQUENCE_ERROR;  // requestSequenceError
        goto errOut;
    }

    if (svcPtr->fileXferRef == NULL)
    {
        LE_DEBUG("Did not register RequestFileTransfer handler for update service");
        nrc = TAF_DIAG_CONDITION_NOT_CORRECT;  // conditionsNotCorrect
        goto errOut;
    }

    // Loopup message handler of this service
    handlerCtxPtr = (taf_FileXferHandler_t*)
        le_ref_Lookup(update.RxFileXferHandlerRefMap, svcPtr->fileXferRef);
    if (handlerCtxPtr == NULL || handlerCtxPtr->func == NULL)
    {
        LE_DEBUG("Can not find RequestFileTransfer handler object!");
        nrc = TAF_DIAG_CONDITION_NOT_CORRECT;  // conditionsNotCorrect
        goto errOut;
    }

    // Add the message in service message list and notify to application.
    msgPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&svcPtr->reqFileXferMsgList, &msgPtr->link);

    handlerCtxPtr->func(msgPtr->rxMsgRef, msgPtr->operationType, handlerCtxPtr->context);

    // Update update service state if download or upload request is received.
    if (msgPtr->operationType == TAF_DIAG_UPDATE_ADD_FILE
        || msgPtr->operationType == TAF_DIAG_UPDATE_REPLACE_FILE
        || msgPtr->operationType == TAF_DIAG_UPDATE_READ_FILE
        || msgPtr->operationType == TAF_DIAG_UPDATE_READ_DIR)
    {
        if (svcPtr->state != TAF_DIAG_UPDATE_TRANS)
        {
            svcPtr->state = TAF_DIAG_UPDATE_REQ;
        }
    }

    return;
errOut:
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = msgPtr->addrInfo.ta;
    addrInfo.ta = msgPtr->addrInfo.sa;
    addrInfo.taType = msgPtr->addrInfo.taType;
    update.RspNegativeMsg(&addrInfo, msgPtr->serviceId, nrc);

    le_ref_DeleteRef(update.RxFileXferMsgRefMap, msgPtr->rxMsgRef);
    le_mem_Release(msgPtr);
}

void taf_UpdateSvr::RxXferDataEventHandler
(
    void* reportPtr
)
{
    uint8_t nrc = 0;
    taf_UpdateSvc_t* svcPtr;
    taf_XferDataRxMsg_t* msgPtr;
    taf_XferDataHandler_t* handlerCtxPtr;

    if (reportPtr == NULL)
    {
        return;
    }

    msgPtr = (taf_XferDataRxMsg_t*)reportPtr;
    taf_UpdateSvr& update = taf_UpdateSvr::GetInstance();

    svcPtr = update.FindSvcInList();
    if (svcPtr == NULL)
    {
        LE_DEBUG("Not found registered update service for this request");
        nrc = TAF_DIAG_CONDITION_NOT_CORRECT;  // conditionsNotCorrect
        goto errOut;
    }

    if (svcPtr->state == TAF_DIAG_UPDATE_INIT
        || svcPtr->state == TAF_DIAG_UPDATE_EXIT)
    {
        LE_DEBUG("The update state(0x%x) is incorrect", svcPtr->state);
        nrc = TAF_DIAG_REQUEST_SEQUENCE_ERROR;  // requestSequenceError
        goto errOut;
    }

    if (svcPtr->xferDataRef == NULL)
    {
        LE_DEBUG("Did not register TransferData handler for update service");
        nrc = TAF_DIAG_CONDITION_NOT_CORRECT;  // conditionsNotCorrect
        goto errOut;
    }

    // Loopup message handler of this service
    handlerCtxPtr = (taf_XferDataHandler_t*)
        le_ref_Lookup(update.RxXferDataHandlerRefMap, svcPtr->xferDataRef);
    if (handlerCtxPtr == NULL || handlerCtxPtr->func == NULL)
    {
        LE_DEBUG("Can not find TransferData handler object!");
        nrc = TAF_DIAG_CONDITION_NOT_CORRECT;  // conditionsNotCorrect
        goto errOut;
    }

    // Add the message in service message list and notify to application.
    msgPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&svcPtr->xferDataMsgList, &msgPtr->link);

    handlerCtxPtr->func(msgPtr->rxMsgRef, handlerCtxPtr->context);

    if (svcPtr->state == TAF_DIAG_UPDATE_REQ)
    {
        svcPtr->state = TAF_DIAG_UPDATE_TRANS;
    }

    return;
errOut:
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = msgPtr->addrInfo.ta;
    addrInfo.ta = msgPtr->addrInfo.sa;
    addrInfo.taType = msgPtr->addrInfo.taType;
    update.RspNegativeMsg(&addrInfo, msgPtr->serviceId, nrc);

    le_ref_DeleteRef(update.RxXferDataMsgRefMap, msgPtr->rxMsgRef);
    le_mem_Release(msgPtr);
}

void taf_UpdateSvr::RxXferExitEventHandler
(
    void* reportPtr
)
{
    uint8_t nrc = 0;
    taf_UpdateSvc_t* svcPtr;
    taf_XferExitRxMsg_t* msgPtr;
    taf_XferExitHandler_t* handlerCtxPtr;

    if (reportPtr == NULL)
    {
        return;
    }

    msgPtr = (taf_XferExitRxMsg_t*)reportPtr;
    taf_UpdateSvr& update = taf_UpdateSvr::GetInstance();

    svcPtr = update.FindSvcInList();
    if (svcPtr == NULL)
    {
        LE_DEBUG("Not found registered update service for this request");
        nrc = TAF_DIAG_CONDITION_NOT_CORRECT;  // conditionsNotCorrect
        goto errOut;
    }

    if (svcPtr->state == TAF_DIAG_UPDATE_INIT
        || svcPtr->state == TAF_DIAG_UPDATE_EXIT)
    {
        LE_DEBUG("The update state(0x%x) is incorrect", svcPtr->state);
        nrc = TAF_DIAG_REQUEST_SEQUENCE_ERROR;  // requestSequenceError
        goto errOut;
    }

    if (svcPtr->xferExitRef == NULL)
    {
        LE_DEBUG("Did not register RequestTransferExit handler for update service");
        nrc = TAF_DIAG_CONDITION_NOT_CORRECT;  // conditionsNotCorrect
        goto errOut;
    }

    // Loopup message handler of this service
    handlerCtxPtr = (taf_XferExitHandler_t*)
        le_ref_Lookup(update.RxXferExitHandlerRefMap, svcPtr->xferExitRef);
    if (handlerCtxPtr == NULL || handlerCtxPtr->func == NULL)
    {
        LE_DEBUG("Can not find RequestTransferExit handler object!");
        nrc = TAF_DIAG_CONDITION_NOT_CORRECT;  // conditionsNotCorrect
        goto errOut;
    }

    // Add the message in service message list and notify to application.
    msgPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&svcPtr->reqXferExitMsgList, &msgPtr->link);

    handlerCtxPtr->func(msgPtr->rxMsgRef, handlerCtxPtr->context);

    svcPtr->state = TAF_DIAG_UPDATE_EXIT;

    return;
errOut:
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = msgPtr->addrInfo.ta;
    addrInfo.ta = msgPtr->addrInfo.sa;
    addrInfo.taType = msgPtr->addrInfo.taType;
    update.RspNegativeMsg(&addrInfo, msgPtr->serviceId, nrc);

    le_ref_DeleteRef(update.RxXferExitMsgRefMap, msgPtr->rxMsgRef);
    le_mem_Release(msgPtr);
}

le_result_t taf_UpdateSvr::GetFilePathAndNameLen
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
    uint16_t* fileLenPtr)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(fileLenPtr == NULL, LE_BAD_PARAMETER, "Invalid fileLenPtr");

    taf_FileXferRxMsg_t* msgPtr = (taf_FileXferRxMsg_t*)
        le_ref_Lookup(RxFileXferMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    *fileLenPtr = strlen((const char*)msgPtr->filePathAndName) + 1;

    return LE_OK;
}

le_result_t taf_UpdateSvr::GetFilePathAndName
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
    uint8_t* fileNamePtr,
    size_t* fileNameSizePtr)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(fileNamePtr == NULL, LE_BAD_PARAMETER, "Invalid fileNamePtr");
    TAF_ERROR_IF_RET_VAL(fileNameSizePtr == NULL, LE_BAD_PARAMETER, "Invalid fileNameSizePtr");

    taf_FileXferRxMsg_t* msgPtr = (taf_FileXferRxMsg_t*)
        le_ref_Lookup(RxFileXferMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (*fileNameSizePtr <= strlen((const char*)msgPtr->filePathAndName))
    {
        LE_ERROR("The buffer size of file name is too small.%" PRIuS "-%" PRIuS,
            *fileNameSizePtr, strlen((const char*)msgPtr->filePathAndName));
        return LE_OVERFLOW;
    }

    le_utf8_Copy((char*)fileNamePtr, (const char*)msgPtr->filePathAndName,
        *fileNameSizePtr, fileNameSizePtr);
    *fileNameSizePtr += 1;  // Contains '\0'

    return LE_OK;
}

le_result_t taf_UpdateSvr::GetDataFormatID
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
    uint8_t* dataFormatIDPtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(dataFormatIDPtr == NULL, LE_BAD_PARAMETER, "Invalid dataFormatIDPtr");

    taf_FileXferRxMsg_t* msgPtr = (taf_FileXferRxMsg_t*)
        le_ref_Lookup(RxFileXferMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    *dataFormatIDPtr = msgPtr->dataFormatID;

    return LE_OK;
}

le_result_t taf_UpdateSvr::GetFileSizeParamLen
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
    uint8_t* fileParamLenPtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(fileParamLenPtr == NULL, LE_BAD_PARAMETER, "Invalid fileParamLenPtr");

    taf_FileXferRxMsg_t* msgPtr = (taf_FileXferRxMsg_t*)
        le_ref_Lookup(RxFileXferMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    *fileParamLenPtr = msgPtr->fileSizeParamLen;

    return LE_OK;
}

le_result_t taf_UpdateSvr::GetUnCompFileSize
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
    uint32_t* unCompFileSizePtr)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(unCompFileSizePtr == NULL, LE_BAD_PARAMETER, "Invalid unCompFileSizePtr");

    taf_FileXferRxMsg_t* msgPtr = (taf_FileXferRxMsg_t*)
        le_ref_Lookup(RxFileXferMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    *unCompFileSizePtr = msgPtr->unCompFileSize;

    return LE_OK;
}

le_result_t taf_UpdateSvr::GetCompFileSize
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
    uint32_t* compFileSizePtr)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(compFileSizePtr == NULL, LE_BAD_PARAMETER, "Invalid compFileSizePtr");

    taf_FileXferRxMsg_t* msgPtr = (taf_FileXferRxMsg_t*)
        le_ref_Lookup(RxFileXferMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    *compFileSizePtr = msgPtr->compFileSize;

    return LE_OK;
}

le_result_t taf_UpdateSvr::SendFileXferResp
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
    taf_diagUpdate_FileXferErrorCode_t errCode
)
{
    le_result_t ret;

    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    taf_FileXferRxMsg_t* msgPtr = (taf_FileXferRxMsg_t*)
        le_ref_Lookup(RxFileXferMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    // Get the service.
    taf_UpdateSvc_t* svcPtr = FindSvcInList();
    if (svcPtr == NULL)
    {
        LE_ERROR("Not found registered update service for this request");
        return LE_NOT_FOUND;
    }

    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = msgPtr->addrInfo.ta;
    addrInfo.ta = msgPtr->addrInfo.sa;
    addrInfo.taType = msgPtr->addrInfo.taType;
    if (errCode == TAF_DIAGUPDATE_FILE_XFER_NO_ERROR)
    {
        ret = RspPositiveMsg(&addrInfo, msgPtr->serviceId, NULL, 0);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to respond positive message for RequestFileTransfer(%d)", ret);
            return ret;
        }
    }
    else
    {
        ret = RspNegativeMsg(&addrInfo, msgPtr->serviceId, (uint8_t)errCode);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to respond negative message for RequestFileTransfer(%d)", ret);
            return ret;
        }
    }

    // Remove the message from service message list.
    le_dls_Remove(&svcPtr->reqFileXferMsgList, &msgPtr->link);

    // Free the message
    le_ref_DeleteRef(RxFileXferMsgRefMap, msgPtr->rxMsgRef);
    le_mem_Release(msgPtr);

    LE_DEBUG("Release reqMsg(%p) resource for RequestFileTransfer", rxMsgRef);

    return LE_OK;
}

le_result_t taf_UpdateSvr::ReleaseRxFileXferMsg
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef
)
{
    taf_UpdateSvc_t* svcPtr;

    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    taf_FileXferRxMsg_t* msgPtr = (taf_FileXferRxMsg_t*)
        le_ref_Lookup(RxFileXferMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    svcPtr = FindSvcInList();
    if (svcPtr == NULL)
    {
        LE_ERROR("Cannot find the update service");
        return LE_NOT_FOUND;
    }

    // Remove the message from service message list.
    le_dls_Remove(&svcPtr->reqFileXferMsgList, &msgPtr->link);

    // Free the message
    le_ref_DeleteRef(RxFileXferMsgRefMap, msgPtr->rxMsgRef);
    le_mem_Release(msgPtr);

    LE_DEBUG("Release reqMsg(%p) resource for RequestFileTransfer", rxMsgRef);

    return LE_OK;
}

le_result_t taf_UpdateSvr::GetblockSeqCount
(
    taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef,
    uint8_t* countPtr)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(countPtr == NULL, LE_BAD_PARAMETER, "Invalid countPtr");

    taf_XferDataRxMsg_t* msgPtr = (taf_XferDataRxMsg_t*)
        le_ref_Lookup(RxXferDataMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    *countPtr = msgPtr->blockSeqCount;

    return LE_OK;
}

le_result_t taf_UpdateSvr::GetXferDataParamRecLen
(
    taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef,
    uint16_t* xferDataRecLenPtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(xferDataRecLenPtr == NULL, LE_BAD_PARAMETER, "Invalid xferDataRecLenPtr");

    taf_XferDataRxMsg_t* msgPtr = (taf_XferDataRxMsg_t*)
        le_ref_Lookup(RxXferDataMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    *xferDataRecLenPtr = (uint16_t)msgPtr->xferDataSize;

    return LE_OK;
}

le_result_t taf_UpdateSvr::GetXferDataParamRec
(
    taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef,
    uint8_t* xferDataRecPtr,
    size_t* xferDataRecSizePtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(xferDataRecPtr == NULL, LE_BAD_PARAMETER, "Invalid xferDataRecPtr");
    TAF_ERROR_IF_RET_VAL(xferDataRecSizePtr == NULL, LE_BAD_PARAMETER,
        "Invalid xferDataRecSizePtr");

    taf_XferDataRxMsg_t* msgPtr = (taf_XferDataRxMsg_t*)
        le_ref_Lookup(RxXferDataMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (*xferDataRecSizePtr < msgPtr->xferDataSize)
    {
        LE_ERROR("The buffer(%" PRIuS ") is insufficient. Need size:%" PRIuS,
            *xferDataRecSizePtr, msgPtr->xferDataSize);
        return LE_OVERFLOW;
    }

    memcpy(xferDataRecPtr, msgPtr->xferDataRec, msgPtr->xferDataSize);
    *xferDataRecSizePtr = msgPtr->xferDataSize;

    return LE_OK;
}

le_result_t taf_UpdateSvr::SendXferDataResp
(
    taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef,
    taf_diagUpdate_XferDataErrorCode_t errCode,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    le_result_t ret;

    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    taf_XferDataRxMsg_t* msgPtr = (taf_XferDataRxMsg_t*)
        le_ref_Lookup(RxXferDataMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    // Get the service.
    taf_UpdateSvc_t* svcPtr = FindSvcInList();
    if (svcPtr == NULL)
    {
        LE_ERROR("Not found registered update service for this request");
        return LE_NOT_FOUND;
    }

    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = msgPtr->addrInfo.ta;
    addrInfo.ta = msgPtr->addrInfo.sa;
    addrInfo.taType = msgPtr->addrInfo.taType;
    if (errCode == TAF_DIAGUPDATE_XFER_DATA_NO_ERROR)
    {
        ret = RspPositiveMsg(&addrInfo, msgPtr->serviceId, dataPtr, dataSize);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to respond positive message for RequestFileTransfer(%d)", ret);
            return ret;
        }
    }
    else
    {
        ret = RspNegativeMsg(&addrInfo, msgPtr->serviceId, (uint8_t)errCode);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to respond negative message for RequestFileTransfer(%d)", ret);
            return ret;
        }
    }

    // Remove the message from service message list.
    le_dls_Remove(&svcPtr->xferDataMsgList, &msgPtr->link);

    // Free the message
    le_ref_DeleteRef(RxXferDataMsgRefMap, msgPtr->rxMsgRef);
    le_mem_Release(msgPtr);

    LE_DEBUG("Release reqMsg(%p) resource for TransferData", rxMsgRef);

    return LE_OK;
}

le_result_t taf_UpdateSvr::ReleaseRxXferDataMsg
(
    taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef
)
{
    taf_UpdateSvc_t* svcPtr;

    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    taf_XferDataRxMsg_t* msgPtr = (taf_XferDataRxMsg_t*)
        le_ref_Lookup(RxXferDataMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    svcPtr = FindSvcInList();
    if (svcPtr == NULL)
    {
        LE_ERROR("Cannot find the update service");
        return LE_NOT_FOUND;
    }

    // Remove the message from service message list.
    le_dls_Remove(&svcPtr->xferDataMsgList, &msgPtr->link);

    // Free the message
    le_ref_DeleteRef(RxXferDataMsgRefMap, msgPtr->rxMsgRef);
    le_mem_Release(msgPtr);

    LE_DEBUG("Release reqMsg(%p) resource for TransferData", rxMsgRef);

    return LE_OK;
}

le_result_t taf_UpdateSvr::GetXferExitParamRecLen
(
    taf_diagUpdate_RxXferExitMsgRef_t rxMsgRef,
    uint16_t* exitDataRecLenPtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(exitDataRecLenPtr == NULL, LE_BAD_PARAMETER, "Invalid exitDataRecLenPtr");

    taf_XferExitRxMsg_t* msgPtr = (taf_XferExitRxMsg_t*)
        le_ref_Lookup(RxXferExitMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    *exitDataRecLenPtr = (uint16_t)msgPtr->exitDataSize;

    return LE_OK;
}

le_result_t taf_UpdateSvr::GetXferExitParamRec
(
    taf_diagUpdate_RxXferExitMsgRef_t rxMsgRef,
    uint8_t* exitDataRecPtr,
    size_t* exitDataRecSizePtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(exitDataRecPtr == NULL, LE_BAD_PARAMETER, "Invalid exitDataRecPtr");
    TAF_ERROR_IF_RET_VAL(exitDataRecSizePtr == NULL, LE_BAD_PARAMETER,
        "Invalid exitDataRecSizePtr");

    taf_XferExitRxMsg_t* msgPtr = (taf_XferExitRxMsg_t*)
        le_ref_Lookup(RxXferExitMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (*exitDataRecSizePtr < msgPtr->exitDataSize)
    {
        LE_ERROR("The buffer(%" PRIuS ") is insufficient. Need size:%" PRIuS,
            *exitDataRecSizePtr, msgPtr->exitDataSize);
        return LE_OVERFLOW;
    }

    memcpy(exitDataRecPtr, msgPtr->exitDataRec, msgPtr->exitDataSize);
    *exitDataRecSizePtr = msgPtr->exitDataSize;

    return LE_OK;

}

le_result_t taf_UpdateSvr::SendXferExitResp
(
    taf_diagUpdate_RxXferExitMsgRef_t rxMsgRef,
    taf_diagUpdate_XferExitErrorCode_t errCode,
    const uint8_t* dataPtr,
    size_t dataSize)
{
    le_result_t ret;

    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    taf_XferExitRxMsg_t* msgPtr = (taf_XferExitRxMsg_t*)
        le_ref_Lookup(RxXferExitMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    // Get the service.
    taf_UpdateSvc_t* svcPtr = FindSvcInList();
    if (svcPtr == NULL)
    {
        LE_ERROR("Not found registered update service for this request");
        return LE_NOT_FOUND;
    }

    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = msgPtr->addrInfo.ta;
    addrInfo.ta = msgPtr->addrInfo.sa;
    addrInfo.taType = msgPtr->addrInfo.taType;
    if (errCode == TAF_DIAGUPDATE_XFER_EXIT_NO_ERROR)
    {
        ret = RspPositiveMsg(&addrInfo, msgPtr->serviceId, dataPtr, dataSize);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to respond positive message for RequestFileTransfer(%d)", ret);
            return ret;
        }
    }
    else
    {
        ret = RspNegativeMsg(&addrInfo, msgPtr->serviceId, (uint8_t)errCode);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to respond negative message for RequestFileTransfer(%d)", ret);
            return ret;
        }
    }

    // Remove the message from service message list.
    le_dls_Remove(&svcPtr->reqXferExitMsgList, &msgPtr->link);

    // Free the message
    le_ref_DeleteRef(RxXferExitMsgRefMap, msgPtr->rxMsgRef);
    le_mem_Release(msgPtr);

    LE_DEBUG("Release reqMsg(%p) resource for TransferData", rxMsgRef);

    return LE_OK;
}

le_result_t taf_UpdateSvr::ReleaseRxXferExitMsg
(
    taf_diagUpdate_RxXferExitMsgRef_t rxMsgRef
)
{
    taf_UpdateSvc_t* svcPtr;

    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    taf_XferExitRxMsg_t* msgPtr = (taf_XferExitRxMsg_t*)
        le_ref_Lookup(RxXferExitMsgRefMap, rxMsgRef);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    svcPtr = FindSvcInList();
    if (svcPtr == NULL)
    {
        LE_ERROR("Cannot find the update service");
        return LE_NOT_FOUND;
    }

    // Remove the message from service message list.
    le_dls_Remove(&svcPtr->reqXferExitMsgList, &msgPtr->link);

    // Free the message
    le_ref_DeleteRef(RxXferExitMsgRefMap, msgPtr->rxMsgRef);
    le_mem_Release(msgPtr);

    LE_DEBUG("Release reqMsg(%p) resource for TransferData", rxMsgRef);

    return LE_OK;
}
