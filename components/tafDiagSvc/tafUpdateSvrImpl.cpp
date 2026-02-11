/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDiagBackend.hpp"
#include "tafUpdateSvr.hpp"
#include <arpa/inet.h>

using namespace tafsvc;
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
    RxNrcStatusRefMap = le_ref_CreateMap("RxNrcStatusRefMap", DEFAULT_RXMSG_REF_CNT);

    // Create memory pools.
    SvcPool = le_mem_CreatePool("Server SvcPool", sizeof(taf_UpdateSvc_t));
    RxFileXferMsgPool = le_mem_CreatePool("Server RxFileXferMsgPool", sizeof(taf_FileXferRxMsg_t));
    RxXferDataMsgPool = le_mem_CreatePool("Server RxXferDataMsgPool", sizeof(taf_XferDataRxMsg_t));
    RxXferExitMsgPool = le_mem_CreatePool("Server RxXferExitMsgPool", sizeof(taf_XferExitRxMsg_t));
    vlanPool = le_mem_CreatePool("DiagUpdateVlanPool", sizeof(taf_UpdateVlanIdNode_t));
    NrcStatusMsgPool = le_mem_CreatePool("NrcStatusMsgPool", sizeof(taf_UpdateNrcStatusMsg_t));

    RxFileXferHandlerPool = le_mem_CreatePool("Server RxFileXferHandlerPool",
            sizeof(taf_FileXferHandler_t));
    RxXferDataHandlerPool = le_mem_CreatePool("Server RxXferDataHandlerPool",
            sizeof(taf_XferDataHandler_t));
    RxXferExitHandlerPool = le_mem_CreatePool("Server RxXferExitHandlerPool",
            sizeof(taf_XferExitHandler_t));

    // Create the event and add event handler for RequestFileTransfer (0x38).
    FileXferEvent = le_event_CreateIdWithRefCounting("UpdateSvc RequestFileTransfer Event");
    FileXferEventHandlerRef = le_event_AddHandler("ReqFileXferEvtHandler",
            FileXferEvent, taf_UpdateSvr::RxFileXferEventHandler);

    // Create the event and add event handler for TransferData (0x36).
    XferDataEvent = le_event_CreateIdWithRefCounting("UpdateSvc TransferData Event");
    XferDataEventHandlerRef = le_event_AddHandler("XferDataEvtHandler",
            XferDataEvent, taf_UpdateSvr::RxXferDataEventHandler);

    // Create the event and add event handler for RequestTransferExit (0x37).
    XferExitEvent = le_event_CreateIdWithRefCounting("UpdateSvc TransferData Event");
    XferExitEventHandlerRef = le_event_AddHandler("XferExitEvtHandler",
            XferExitEvent, taf_UpdateSvr::RxXferExitEventHandler);

    // Create the event ID for NRC notification.
    NrcEventId = le_event_CreateIdWithRefCounting("NRC Event");

    // Create client session close hander.
    le_msg_AddServiceCloseHandler(taf_diagUpdate_GetServiceRef(),
                                  taf_UpdateSvr::OnClientDisconnection, NULL);

    auto& backend = taf_DiagBackend::GetInstance();

    backend.RegisterUdsService(reqFileXferSvcId, this);
    backend.RegisterUdsService(fileDataXferSvcId, this);
    backend.RegisterUdsService(fileXferExitSvcId, this);
    backend.RegisterUdsService(nrcStatusMsgId, this);

    LE_DEBUG("taf_DiagUpdateSvr Init completed!");

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
    taf_UpdateSvc_t* svcPtr = FindSvcInList(taf_diagUpdate_GetClientSessionRef());

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

        svcPtr->supportedVlanList = LE_DLS_LIST_INIT;
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

    LE_DEBUG("Get serviceRef %p for Diag update service.", svcPtr->svcRef);

    return svcPtr->svcRef;
}

taf_UpdateSvc_t* taf_UpdateSvr::FindSvcInList
(
    le_msg_SessionRef_t sessionRef
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_UpdateSvc_t* svcPtr = (taf_UpdateSvc_t *)le_ref_GetValue(iterRef);
        if ((svcPtr != NULL) && (svcPtr->sessionRef == sessionRef))
        {
            return svcPtr;
        }
    }

    return NULL;
}

taf_UpdateSvc_t* taf_UpdateSvr::FindSvcInList
(
    uint16_t vlanId
)
{
    LE_DEBUG("find the diag update service object!");
    bool isFound = false;
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_UpdateSvc_t* servicePtr = (taf_UpdateSvc_t *)le_ref_GetValue(iterRef);
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
                taf_UpdateVlanIdNode_t *vlan = CONTAINER_OF(linkPtr,
                    taf_UpdateVlanIdNode_t, link);
                if ((vlan != NULL) && (vlan->vlanId == vlanId))
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
    ClearVlanList(svcPtr);

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
            // Free the message
            le_ref_DeleteRef(RxXferExitMsgRefMap, msgPtr->rxMsgRef);
            le_mem_Release(msgPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&svcPtr->reqXferExitMsgList);
    }
}

void taf_UpdateSvr::ClearVlanList
(
    taf_UpdateSvc_t* svcPtr
)
{
    // Clear the vlan id list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&svcPtr->supportedVlanList);
    while (linkPtr != NULL)
    {
        taf_UpdateVlanIdNode_t *vlanPtr = CONTAINER_OF(linkPtr, taf_UpdateVlanIdNode_t, link);
        if (vlanPtr != NULL)
        {
            le_mem_Release(vlanPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&svcPtr->supportedVlanList);
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

    /* moopDirection Usage:
     * 0 - Download direction (moop: 01/03/06)
     * 1 - Upload direction (moop: 04/05)
     * 2 - No direction (moop: 02)
    */
    static int moopDirection = 2;

    TAF_ERROR_IF_RET_NIL(msgPtr == NULL, "Invalid dataPtr");
    TAF_ERROR_IF_RET_NIL(addrPtr == NULL, "Invalid adrInfoPtr");

    if (sid == reqFileXferSvcId)  // RequestFileTransfer service of UDS
    {
        uint8_t moo;    // Mode Of Operation
        uint16_t fileNameLen;

        moo = msgPtr[dataPtrPos];
        if (moo == 0 || moo > TAF_DIAG_UPDATE_RESUME_FILE)
        {
            LE_DEBUG("Mode of operation(0x%x) is out of range", moo);
            // UDS_0x38_NRC_31: Invalid Moop
            nrc = TAF_DIAG_REQUEST_OUT_OF_RANGE;
            goto errOut;
        }

        dataPtrPos += 1;

        fileNameLen = ntohs(*((uint16_t*)(msgPtr + dataPtrPos)));
        if (fileNameLen >= TAF_DIAGUPDATE_MAX_PATH_AND_NAME_SIZE)
        {
            LE_DEBUG("File name length(%d) is invalid", fileNameLen);
            // UDS_0x38_NRC_13: Invalid file name len
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

        if (moo == TAF_DIAG_UPDATE_ADD_FILE
        ||  moo == TAF_DIAG_UPDATE_REPLACE_FILE
        ||  moo == TAF_DIAG_UPDATE_RESUME_FILE)
        {
            moopDirection = 0; // Download
        }
        else if (moo == TAF_DIAG_UPDATE_READ_FILE
        ||       moo == TAF_DIAG_UPDATE_READ_DIR)
        {
            moopDirection = 1; // Upload
        }
        else // moo == TAF_DIAG_UPDATE_DELETE_FILE
        {
            moopDirection = 2; // No direction
        }
    }
    else if (sid == fileDataXferSvcId)  // TransferData service of UDS
    {
        uint8_t blkSeqCnt;

        blkSeqCnt = msgPtr[dataPtrPos];
        dataPtrPos += 1;

        // 'transferRequestParameterRecord' parameter record contains parameter(s)
        //  which are required by the server to support the transfer of data.
        // Format and length of this parameter(s) are vehicle manufacturer specific.
        if ((msgLen - dataPtrPos) > TAF_DIAGUPDATE_MAX_XFER_PARAM_REC_SIZE)
        {
            LE_WARN("Message length(%" PRIuS ") is out of range", msgLen - dataPtrPos);
            // UDS_0x36_NRC_13: Block parameter record is overflow
            nrc = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            goto errOut;
        }

        if (moopDirection == 0) // Download direction
        {
            // 3 = SID + BSC + minimum TRPR_
            if (msgLen + 1 < 3) // Plus 'SID' size first
            {
                LE_WARN("Record size less 3 in download direction");
                // UDS_0x36_NRC_13: Record size less 3 in download direction
                nrc = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
                goto errOut;
            }
        }
        else if (moopDirection == 1) // Upload direction
        {
            // 2 = SID + BSC
            if (msgLen + 1 < 2) // Plus 'SID' size first
            {
                LE_WARN("Record size less 2 in upload direction");
                // UDS_0x36_NRC_13: Record size less 2 in upload direction
                nrc = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
                goto errOut;
            }
        }
        else
        {
            // Never be here for TAF_DIAG_UPDATE_DELETE_FILE
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
    else if (sid == fileXferExitSvcId)  // RequestTransferExit service of UDS.
    {
        // This parameter record contains parameter(s), which are required
        // by the server to support the transfer of data.
        // Format and length of this parameter(s) are vehicle manufacturer specific.
        if ((msgLen - dataPtrPos) > TAF_DIAGUPDATE_MAX_XFER_PARAM_REC_SIZE)
        {
            LE_WARN("Message length(%" PRIuS ") is out of range", msgLen - dataPtrPos);
            // UDS_0x37_NRC_13: Block parameter record is overflow
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
    else if (sid == nrcStatusMsgId)  // NRC status notification.
    {
        if(msgLen < NRC_NOTIFICATION_MIN_LEN)
        {
            LE_ERROR("Wrong NRC notification message length");
            return;
        }

    #ifdef LE_CONFIG_DIAG_FEATURE_A
        ReportNrcStatus(addrPtr, msgPtr[1], msgPtr[2]);
    #endif

        return;
    }
    else
    {
        LE_WARN("Service(0x%x) is invalid for update", sid);
        nrc = TAF_DIAG_SERVICE_NOT_SUPPORTED; // ServiceNotSupported
        goto errOut;
    }

    return;
errOut:
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = addrPtr->ta;
    addrInfo.ta = addrPtr->sa;
    addrInfo.taType = addrPtr->taType;
    addrInfo.vlanId = addrPtr->vlanId;
    le_utf8_Copy(addrInfo.ifName, addrPtr->ifName, MAX_INTERFACE_NAME_LEN, NULL);

    RspNegativeMsg(&addrInfo, sid, nrc, true);

    return;
}

le_result_t taf_UpdateSvr::RspNegativeMsg
(
    taf_uds_AddrInfo_t* addrPtr,
    uint8_t sid,
    uint8_t nrc,
    bool isInternal
)
{
    // Negative message: 0x7F SID NRC
    auto& backend = taf_DiagBackend::GetInstance();

#ifdef LE_CONFIG_DIAG_FEATURE_A
    //Send NRC notification except NRC 0x21.
    if(isInternal && (nrc != TAF_DIAG_BUSY_REPEAT_REQUEST))
        ReportNrcStatus(addrPtr, sid, nrc);
#endif

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

    LE_DEBUG("Created handlerRef(%p) for Service reference %p.",
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

    LE_DEBUG("Created handlerRef(%p) for Service reference %p.",
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

    LE_DEBUG("Created handlerRef(%p) for Service reference %p.",
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


/*
 * Get the programming session interruption indication
**/
void taf_UpdateSvr::programmingInterrupt(uint16_t vlanId)
{
    taf_UpdateSvr& update = taf_UpdateSvr::GetInstance();

#ifndef LE_CONFIG_DIAG_VSTACK
    taf_UpdateSvc_t* svcPtr = update.FindSvcInList(vlanId);
#else
    taf_UpdateSvc_t* svcPtr = update.FindSvcInList((uint16_t)0);
#endif
    if (svcPtr != NULL)
    {
        svcPtr->state = TAF_DIAG_UPDATE_INIT; // reset the state-machine
    }
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

#ifndef LE_CONFIG_DIAG_VSTACK
    svcPtr = update.FindSvcInList(msgPtr->addrInfo.vlanId);
#else
    svcPtr = update.FindSvcInList((uint16_t)0);
#endif
    if (svcPtr == NULL)
    {
        LE_WARN("Not found registered update service for this request");
        // UDS_0x38_NRC_21: Not found registered diag update svc
        nrc = TAF_DIAG_BUSY_REPEAT_REQUEST;  // busyrepeatreq
        goto errOut;
    }

    if (svcPtr->fileXferRef == NULL)
    {
        LE_WARN("Did not register RequestFileTransfer handler for update service");
        // UDS_0x38_NRC_21: Bad svc ref
        nrc = TAF_DIAG_BUSY_REPEAT_REQUEST;  // busyrepeatreq
        goto errOut;
    }

    // Loopup message handler of this service
    handlerCtxPtr = (taf_FileXferHandler_t*)
        le_ref_Lookup(update.RxFileXferHandlerRefMap, svcPtr->fileXferRef);
    if (handlerCtxPtr == NULL || handlerCtxPtr->func == NULL)
    {
        LE_WARN("Can not find RequestFileTransfer handler object!");
        // UDS_0x38_NRC_21: Bad callback fn
        nrc = TAF_DIAG_BUSY_REPEAT_REQUEST;  // busyrepeatreq
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
        || msgPtr->operationType == TAF_DIAG_UPDATE_READ_DIR
        || msgPtr->operationType == TAF_DIAG_UPDATE_RESUME_FILE)
    {
        if (svcPtr->state != TAF_DIAG_UPDATE_TRANS)
        {
            svcPtr->state = TAF_DIAG_UPDATE_REQ;
        }
        else
        {
            LE_WARN("Trsnsfer is in progress");
            // UDS_0x38_NRC_22: Transfer is in progress
            nrc = TAF_DIAG_CONDITION_NOT_CORRECT;
            goto errOut;
        }
    }

    return;
errOut:
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = msgPtr->addrInfo.ta;
    addrInfo.ta = msgPtr->addrInfo.sa;
    addrInfo.taType = msgPtr->addrInfo.taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = msgPtr->addrInfo.vlanId;
    le_utf8_Copy(addrInfo.ifName, msgPtr->addrInfo.ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif
    update.RspNegativeMsg(&addrInfo, msgPtr->serviceId, nrc, true);

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

#ifndef LE_CONFIG_DIAG_VSTACK
    svcPtr = update.FindSvcInList(msgPtr->addrInfo.vlanId);
#else
    svcPtr = update.FindSvcInList((uint16_t)0);
#endif
    if (svcPtr == NULL)
    {
        LE_WARN("Not found registered update service for this request");
        // UDS_0x36_NRC_21: Not found svc
        nrc = TAF_DIAG_BUSY_REPEAT_REQUEST;  // busyrepeatreq
        goto errOut;
    }

    if (svcPtr->state == TAF_DIAG_UPDATE_INIT
        || svcPtr->state == TAF_DIAG_UPDATE_EXIT)
    {
        LE_WARN("The update state(0x%x) is incorrect", svcPtr->state);
        // UDS_0x36_NRC_24: Transfer is NOT in progress (again)
        nrc = TAF_DIAG_REQUEST_SEQUENCE_ERROR;  // requestSequenceError
        goto errOut;
    }

    if (svcPtr->xferDataRef == NULL)
    {
        LE_WARN("Did not register TransferData handler for update service");
        // UDS_0x36_NRC_21: Invalid handler ref
        nrc = TAF_DIAG_BUSY_REPEAT_REQUEST;  // busyrepeatreq
        goto errOut;
    }

    // Loopup message handler of this service
    handlerCtxPtr = (taf_XferDataHandler_t*)
        le_ref_Lookup(update.RxXferDataHandlerRefMap, svcPtr->xferDataRef);
    if (handlerCtxPtr == NULL || handlerCtxPtr->func == NULL)
    {
        LE_WARN("Can not find TransferData handler object!");
        // UDS_0x36_NRC_21: Invalid handler callback
        nrc = TAF_DIAG_BUSY_REPEAT_REQUEST;  // busyrepeatreq
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
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = msgPtr->addrInfo.vlanId;
    le_utf8_Copy(addrInfo.ifName, msgPtr->addrInfo.ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif
    update.RspNegativeMsg(&addrInfo, msgPtr->serviceId, nrc, true);

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

#ifndef LE_CONFIG_DIAG_VSTACK
    svcPtr = update.FindSvcInList(msgPtr->addrInfo.vlanId);
#else
    svcPtr = update.FindSvcInList((uint16_t)0);
#endif
    if (svcPtr == NULL)
    {
        LE_WARN("Not found registered update service for this request");
        // UDS_0x37_NRC_21: Not found registered svc
        nrc = TAF_DIAG_BUSY_REPEAT_REQUEST;  // busyrepeatreq
        goto errOut;
    }

    if (svcPtr->state == TAF_DIAG_UPDATE_INIT
        || svcPtr->state == TAF_DIAG_UPDATE_EXIT)
    {
        LE_WARN("The update state(0x%x) is incorrect", svcPtr->state);
        // UDS_0x37_NRC_24: Transfer is not active (again)
        nrc = TAF_DIAG_REQUEST_SEQUENCE_ERROR;  // requestSequenceError
        goto errOut;
    }

    if (svcPtr->xferExitRef == NULL)
    {
        LE_WARN("Did not register RequestTransferExit handler for update service");
        // UDS_0x37_NRC_21: Invalid svc ref
        nrc = TAF_DIAG_BUSY_REPEAT_REQUEST;  // busyrepeatreq
        goto errOut;
    }

    // Loopup message handler of this service
    handlerCtxPtr = (taf_XferExitHandler_t*)
        le_ref_Lookup(update.RxXferExitHandlerRefMap, svcPtr->xferExitRef);
    if (handlerCtxPtr == NULL || handlerCtxPtr->func == NULL)
    {
        LE_WARN("Can not find RequestTransferExit handler object!");
        // UDS_0x37_NRC_21: Invalid callback handler
        nrc = TAF_DIAG_BUSY_REPEAT_REQUEST;  // busyrepeatreq
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
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = msgPtr->addrInfo.vlanId;
    le_utf8_Copy(addrInfo.ifName, msgPtr->addrInfo.ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif
    update.RspNegativeMsg(&addrInfo, msgPtr->serviceId, nrc, true);

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

/*
 * Converts a given number to big-endian format and stores it in a destination buffer.
 */
static void BigEndianNumberFormat(size_t number, uint8_t *buffer, size_t bufsiz)
{
    for (size_t i = 0; i < bufsiz; i++)
    {
        buffer[i] = (number >> (8 * (bufsiz - i - 1))) & 0xFF;
    }
}

/*
 * Calculating a number requires how many bytes to storage.
 */
static uint16_t HowManyChars(uint64_t bigNumber)
{
    uint16_t nbytes = 0;
    while (bigNumber) {
        bigNumber >>= 1;
        nbytes++;
    }

    nbytes = (nbytes + 7) / 8;
    return nbytes;
}

le_result_t taf_UpdateSvr::SetFilePosition(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
    uint64_t filePosition
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    taf_FileXferRxMsg_t* msgPtr = (taf_FileXferRxMsg_t*)
                                  le_ref_Lookup(RxFileXferMsgRefMap, rxMsgRef);
    TAF_ERROR_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Cannot find the msg reference");
    TAF_ERROR_IF_RET_VAL(msgPtr->operationType != TAF_DIAGUPDATE_RESUME_FILE,
                         LE_BAD_PARAMETER, "Set filePosition in not RESULE_FILE context");

    LE_DEBUG("File Position: %" PRIu64, filePosition);
    BigEndianNumberFormat(filePosition, mFilePosition, TAF_DIAGUPDATE_FILE_POSITION_SIZE);

    return LE_OK;
}

le_result_t taf_UpdateSvr::SetFileSizeOrDirInfoLength(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
    uint64_t fileSizeUncompressedOrDirInfoLength,
    uint64_t fileSizeCompressed
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    taf_FileXferRxMsg_t* msgPtr = (taf_FileXferRxMsg_t*)
                                  le_ref_Lookup(RxFileXferMsgRefMap, rxMsgRef);
    TAF_ERROR_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Cannot find the msg reference");

    if (fileSizeUncompressedOrDirInfoLength > UINT32_MAX
    ||  fileSizeCompressed > UINT32_MAX)
    {
        LE_ERROR("Too big file size more than 4G");
        return LE_OVERFLOW;
    }

    if (msgPtr->operationType != TAF_DIAGUPDATE_READ_FILE
    &&  msgPtr->operationType != TAF_DIAGUPDATE_READ_DIR)
    {
        LE_ERROR("Set fileSizeOrDirInfoLength in not READ_FILE or READ_DIR context");
        return LE_BAD_PARAMETER;
    }

    nCharsToSave = HowManyChars(fileSizeUncompressedOrDirInfoLength);
    nCharsToNotUsed = TAF_DIAGUPDATE_FILE_SIZE_OR_DIR_INFO_LEN - nCharsToSave;

    BigEndianNumberFormat(fileSizeUncompressedOrDirInfoLength,
                          mFileSizeUncompressedOrDirInfoLength,
                          TAF_DIAGUPDATE_FILE_SIZE_OR_DIR_INFO_LEN);

    if (msgPtr->operationType == TAF_DIAGUPDATE_READ_FILE)
    {
        BigEndianNumberFormat(fileSizeCompressed,
                              mFileSizeCompressed,
                              TAF_DIAGUPDATE_FILE_SIZE_OR_DIR_INFO_LEN);
    }

    BigEndianNumberFormat(nCharsToSave, mFileSizeOrDirInfoParameterLength, SIZE_OF_FSDIL);
    LE_DEBUG("fileSizeOrDirInfoParameterLength = %d", nCharsToSave);

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
#ifndef LE_CONFIG_DIAG_VSTACK
    taf_UpdateSvc_t* svcPtr = FindSvcInList(msgPtr->addrInfo.vlanId);
#else
    taf_UpdateSvc_t* svcPtr = FindSvcInList((uint16_t)0);
#endif
    if (svcPtr == NULL)
    {
        LE_ERROR("Not found registered update service for this request");
        return LE_NOT_FOUND;
    }

    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = msgPtr->addrInfo.ta;
    addrInfo.ta = msgPtr->addrInfo.sa;
    addrInfo.taType = msgPtr->addrInfo.taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = msgPtr->addrInfo.vlanId;
    le_utf8_Copy(addrInfo.ifName, msgPtr->addrInfo.ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif
    if (errCode == TAF_DIAGUPDATE_FILE_XFER_NO_ERROR)
    {
        uint8_t *buffer = NULL ;
        uint16_t bufsize = 0;

        if (msgPtr->operationType == TAF_DIAGUPDATE_RESUME_FILE)
        {
            buffer = mFilePosition;
            bufsize = TAF_DIAGUPDATE_FILE_POSITION_SIZE;
        }
        else if (msgPtr->operationType == TAF_DIAGUPDATE_READ_DIR)
        {
            uint8_t d_buffer[SIZE_OF_FSDIL + TAF_DIAGUPDATE_FILE_SIZE_OR_DIR_INFO_LEN] = {0};
            memcpy(d_buffer, mFileSizeOrDirInfoParameterLength, SIZE_OF_FSDIL);
            memcpy(d_buffer + SIZE_OF_FSDIL,
                   mFileSizeUncompressedOrDirInfoLength + nCharsToNotUsed,
                   nCharsToSave);
            buffer = d_buffer;
            bufsize = SIZE_OF_FSDIL + nCharsToSave;
        }
        else if (msgPtr->operationType == TAF_DIAGUPDATE_READ_FILE)
        {
            uint8_t f_buffer[SIZE_OF_FSDIL + TAF_DIAGUPDATE_FILE_SIZE_OR_DIR_INFO_LEN * 2] = {0};

            memcpy(f_buffer, mFileSizeOrDirInfoParameterLength, SIZE_OF_FSDIL);
            if (nCharsToSave > TAF_DIAGUPDATE_FILE_SIZE_OR_DIR_INFO_LEN)
            {
                LE_ERROR("Buffer size exceeding max limit");
                return LE_FAULT;
            }
            memcpy(f_buffer + SIZE_OF_FSDIL,
                   mFileSizeUncompressedOrDirInfoLength + nCharsToNotUsed,
                   nCharsToSave);
            memcpy(f_buffer + SIZE_OF_FSDIL + nCharsToSave,
                   mFileSizeCompressed + nCharsToNotUsed,
                   nCharsToSave);
            buffer = f_buffer;
            bufsize = SIZE_OF_FSDIL + nCharsToSave * 2;
        }

        if (buffer != NULL)
        {
            ret = RspPositiveMsg(&addrInfo, msgPtr->serviceId, buffer, bufsize);
        }
        else
        {
            ret = RspPositiveMsg(&addrInfo, msgPtr->serviceId, NULL, 0);
        }

        if (ret != LE_OK)
        {
            LE_ERROR("Failed to respond positive message for RequestFileTransfer(%d)", ret);
            return ret;
        }

    }
    else
    {
        ret = RspNegativeMsg(&addrInfo, msgPtr->serviceId, (uint8_t)errCode, false);
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

#ifndef LE_CONFIG_DIAG_VSTACK
    svcPtr = FindSvcInList(msgPtr->addrInfo.vlanId);
#else
    svcPtr = FindSvcInList((uint16_t)0);
#endif
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
#ifndef LE_CONFIG_DIAG_VSTACK
    taf_UpdateSvc_t* svcPtr = FindSvcInList(msgPtr->addrInfo.vlanId);
#else
    taf_UpdateSvc_t* svcPtr = FindSvcInList((uint16_t)0);
#endif
    if (svcPtr == NULL)
    {
        LE_ERROR("Not found registered update service for this request");
        return LE_NOT_FOUND;
    }

    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = msgPtr->addrInfo.ta;
    addrInfo.ta = msgPtr->addrInfo.sa;
    addrInfo.taType = msgPtr->addrInfo.taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = msgPtr->addrInfo.vlanId;
    le_utf8_Copy(addrInfo.ifName, msgPtr->addrInfo.ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif
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
        ret = RspNegativeMsg(&addrInfo, msgPtr->serviceId, (uint8_t)errCode, false);
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

#ifndef LE_CONFIG_DIAG_VSTACK
    svcPtr = FindSvcInList(msgPtr->addrInfo.vlanId);
#else
    svcPtr = FindSvcInList((uint16_t)0);
#endif
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
#ifndef LE_CONFIG_DIAG_VSTACK
    taf_UpdateSvc_t* svcPtr = FindSvcInList(msgPtr->addrInfo.vlanId);
#else
    taf_UpdateSvc_t* svcPtr = FindSvcInList((uint16_t)0);
#endif
    if (svcPtr == NULL)
    {
        LE_ERROR("Not found registered update service for this request");
        return LE_NOT_FOUND;
    }

    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = msgPtr->addrInfo.ta;
    addrInfo.ta = msgPtr->addrInfo.sa;
    addrInfo.taType = msgPtr->addrInfo.taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = msgPtr->addrInfo.vlanId;
    le_utf8_Copy(addrInfo.ifName, msgPtr->addrInfo.ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif
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
        ret = RspNegativeMsg(&addrInfo, msgPtr->serviceId, (uint8_t)errCode, false);
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

#ifndef LE_CONFIG_DIAG_VSTACK
    svcPtr = FindSvcInList(msgPtr->addrInfo.vlanId);
#else
    svcPtr = FindSvcInList((uint16_t)0);
#endif
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

    return LE_OK;
}

#ifdef LE_CONFIG_DIAG_FEATURE_A
//-------------------------------------------------------------------------------------------------
/**
 * Report NRC status.
 */
//-------------------------------------------------------------------------------------------------
void taf_UpdateSvr::ReportNrcStatus
(
    const taf_uds_AddrInfo_t* addrPtr,
    uint8_t sid,
    uint8_t nrc
)
{
    TAF_ERROR_IF_RET_NIL(addrPtr == NULL, "Invalid addrPtr");

    taf_UpdateNrcStatusMsg_t* nrcStatusMsgPtr = NULL;

    nrcStatusMsgPtr = (taf_UpdateNrcStatusMsg_t*)le_mem_ForceAlloc(NrcStatusMsgPool);
    memset(nrcStatusMsgPtr, 0, sizeof(taf_UpdateNrcStatusMsg_t));

    memcpy(&nrcStatusMsgPtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
    nrcStatusMsgPtr->sid = sid;
    nrcStatusMsgPtr->nrc = nrc;
    nrcStatusMsgPtr->nrcStatusRef = (taf_diagUpdate_NrcStatusRef_t)le_ref_CreateRef(
            RxNrcStatusRefMap, nrcStatusMsgPtr);

    // Report the request message to message handler in service layer.
    le_event_ReportWithRefCounting(NrcEventId, nrcStatusMsgPtr);

    return;
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * FirstLayerNrcStatusHandler.
 */
//--------------------------------------------------------------------------------------------------
void taf_UpdateSvr::FirstLayerNrcStatusHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    taf_UpdateSvr& update = taf_UpdateSvr::GetInstance();

    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_UpdateNrcStatusMsg_t* nrcStatusEvent = (taf_UpdateNrcStatusMsg_t *)reportPtr;
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == NULL, "Null ptr(secondLayerHandlerFunc)");

    taf_diagUpdate_NrcStatusHandlerFunc_t handlerFunc =
                                    (taf_diagUpdate_NrcStatusHandlerFunc_t)secondLayerHandlerFunc;

    handlerFunc(nrcStatusEvent->nrcStatusRef, nrcStatusEvent->addrInfo.vlanId, nrcStatusEvent->sid,
            nrcStatusEvent->nrc, le_event_GetContextPtr());

    le_ref_DeleteRef(update.RxNrcStatusRefMap, nrcStatusEvent->nrcStatusRef);
    le_mem_Release(reportPtr);
}

le_result_t taf_UpdateSvr::ReleaseNrcStatusMsg
(
    taf_diagUpdate_NrcStatusRef_t statusRef
)
{
    LE_INFO("ReleaseNrcStatusMsg");
    return LE_OK;
}

le_result_t taf_UpdateSvr::SetVlanId
(
    taf_diagUpdate_ServiceRef_t svcRef,
    uint16_t vlanId
)
{
    taf_UpdateSvc_t* servicePtr = (taf_UpdateSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
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
        taf_UpdateVlanIdNode_t *vlan = CONTAINER_OF(linkPtr,
            taf_UpdateVlanIdNode_t, link);
        if (vlan != NULL && vlan->vlanId == vlanId)
        {
            return LE_OK;
        }
        linkPtr = le_dls_PeekNext(&servicePtr->supportedVlanList, linkPtr);
    }

    taf_UpdateVlanIdNode_t *vlanPtr = (taf_UpdateVlanIdNode_t *)
        le_mem_ForceAlloc(vlanPool);
    if (vlanPtr == NULL)
    {
        LE_ERROR("Failed to allocate memory.");
        return LE_NO_MEMORY;
    }

    vlanPtr->vlanId = vlanId;
    vlanPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->supportedVlanList, &vlanPtr->link);

    return LE_OK;
#else
    return LE_NOT_IMPLEMENTED;
#endif
}

le_result_t taf_UpdateSvr::GetVlanIdFromMsg
(
    taf_diagUpdate_RxMsgRef_t rxMsgRef,
    uint16_t* vlanIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(vlanIdPtr == NULL, LE_BAD_PARAMETER, "Invalid vlanIdPtr");

#ifndef LE_CONFIG_DIAG_VSTACK
    taf_FileXferRxMsg_t* fileXferMsgPtr = (taf_FileXferRxMsg_t*)
            le_ref_Lookup(RxFileXferMsgRefMap, rxMsgRef);
    if (fileXferMsgPtr == NULL)
    {
        taf_XferDataRxMsg_t* xferDataMsgPtr = (taf_XferDataRxMsg_t*)
                le_ref_Lookup(RxXferDataMsgRefMap, rxMsgRef);
        if (xferDataMsgPtr == NULL)
        {
            taf_XferExitRxMsg_t* xferExitMsgPtr = (taf_XferExitRxMsg_t*)
                    le_ref_Lookup(RxXferExitMsgRefMap, rxMsgRef);
            if(xferExitMsgPtr == NULL)
            {
                LE_ERROR("Can not find the rxMsgRef");
                return LE_NOT_FOUND;
            }
            else
            {
                *vlanIdPtr = xferExitMsgPtr->addrInfo.vlanId;
                return LE_OK;
            }
        }
        else
        {
            *vlanIdPtr = xferDataMsgPtr->addrInfo.vlanId;
            return LE_OK;
        }
    }
    else
    {
        *vlanIdPtr = fileXferMsgPtr->addrInfo.vlanId;
        return LE_OK;
    }

#else
    return LE_NOT_IMPLEMENTED;
#endif
}
