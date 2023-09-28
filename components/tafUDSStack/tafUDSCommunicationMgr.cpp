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

#include "tafUDSCommunicationMgr.hpp"
#include "legato.h"
#include "interfaces.h"

using namespace taf::uds;

/*=================================================================================================
 FUNCTION        UdsCommunicationMgr::GetInstance
 DESCRIPTION     Get an instance of UdsCommunicationMgr
 PARAMETERS      void
 RETURN VALUE    UdsCommunicationMgr: Instance reference
=================================================================================================*/
UdsCommunicationMgr &UdsCommunicationMgr::GetInstance()
{
    static UdsCommunicationMgr instance;

    return instance;
}

UdsCommunicationMgr::UdsCommunicationMgr()
{}

UdsCommunicationMgr::~UdsCommunicationMgr()
{}

/**
 * UDS stack Communication Manager Init.
 */
void UdsCommunicationMgr::Init
(
)
{
    LE_INFO("UDS communication manager init.");

    udsHandlerRefMap = le_ref_CreateMap("udsHandlerRefMap", TAF_UDS_HANDLER_REF_CNT);

    LE_INFO("UDS communication manager ok.");
    return;
}

static taf_doip_PowerMode_t PowerModeQueryHandler
(
    void* userPtr
)
{
    LE_INFO("PowerModeQueryHandler");

    return TAF_DOIP_POWER_MODE_READY;
}

/**
 * Start UDS stack.
 */
le_result_t UdsCommunicationMgr::UdsStart
(
    const char* configPathPtr
)
{
    LE_INFO("UDS stack start.");

    le_result_t ret;

    if(configPathPtr == NULL)
    {
        LE_FATAL("configPathPtr is Null");
        return LE_FAULT;
    }

    LE_INFO("Start UDS server with config file %s", configPathPtr);

    //Init and start DoIP to enable the ability of sending/receiving data packets
    if (DoipEntityRef == NULL)
    {
        DoipEntityRef = taf_doip_Create(configPathPtr);
        if(DoipEntityRef == NULL)
        {
            LE_FATAL("Failed to create DoIP Entity");
            return LE_FAULT;
        }
    }

    ret = taf_doip_Start(DoipEntityRef);
    if (ret != LE_OK)
    {
        LE_FATAL("Failed to start DoIP Entity(%d)", ret);
        return LE_FAULT;
    }

    PmQueryRef = taf_doip_AddPowerModeQueryHandler(DoipEntityRef,
        PowerModeQueryHandler, NULL);
    if (PmQueryRef == NULL)
    {
        LE_FATAL("Failed to register power mode query handler");
        return LE_FAULT;
    }

    return LE_OK;
}

/**
 * Pack NRC.
 */
le_result_t UdsCommunicationMgr::SetNRC
(
    uint8_t sid,
    uint8_t errorCode
)
{
    LE_DEBUG("SetNRC, sid= 0x%x, error code=0x%x",sid, errorCode);

    // pack the NRC data
    sendBuf[0] = UDS_NEGATIVE_RESP_SID;
    sendBuf[1] = sid;
    sendBuf[2] = errorCode;
    sendDataLen = UDS_NEG_RESP_LEN;

    return LE_OK;
}

/**
 * Read DID from ConfigTree on UDS client request.
 */
uint8_t UdsCommunicationMgr::readDIDFromConfigTree
(
    const uint16_t dataId
)
{
    le_cfg_ConnectService();

    char node[DID_NODE_LEN] = { 0 };
    snprintf(node, sizeof(node), DID_CONFIG_TREE_DATA_FORMAT, dataId);

    le_cfg_IteratorRef_t iteratorRef_r = le_cfg_CreateReadTxn(node);

    if (iteratorRef_r == NULL)
    {
        LE_ERROR("No DID node.");
        le_cfg_CancelTxn(iteratorRef_r);
        return REQ_OUT_OF_RANGE;
    }

    //get DID data list
    if(le_cfg_GoToFirstChild (iteratorRef_r) != LE_OK)
    {
        LE_ERROR("Can't find the data node");
        le_cfg_CancelTxn(iteratorRef_r);
        return REQ_OUT_OF_RANGE;
    }

    int i = 0;
    uint8_t data;
    do{

        if(sendDataLen+i >= UDS_DATA_SIZE)
        {
            LE_ERROR("Data length is too long");
            le_cfg_CancelTxn(iteratorRef_r);
            return RESP_TOO_LONG;
        }

        data = le_cfg_GetInt(iteratorRef_r, "", 0);
        sendBuf[sendDataLen+i] = data;

        i++;
    }while (le_cfg_GoToNextSibling(iteratorRef_r) == LE_OK);

    sendDataLen = sendDataLen+i;
    le_cfg_CancelTxn(iteratorRef_r);

    return POSITIVE_RESPONSE;
}

/**
 * Write DID to ConfigTree on UDS client request.
 */
uint8_t UdsCommunicationMgr::writeDIDToConfigTree
(
    const uint16_t dataId,
    const uint8_t* dataPtr,
    uint16_t dataSize
)
{
    le_cfg_ConnectService();

    if(dataPtr == NULL)
    {
        LE_ERROR("Null pointer.");
        return REQ_OUT_OF_RANGE;
    }

    char node[DID_NODE_LEN] = { 0 };
    snprintf(node, sizeof(node), DID_CONFIG_TREE_DATA_FORMAT, dataId);
    le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(node);

    if (wrIter == NULL)
    {
        return REQ_OUT_OF_RANGE;
    }

    //Need to clear the config tree since the length of the value written may be less than the old.
    if(le_cfg_IsEmpty(wrIter, "") == false)
    {
        le_cfg_SetEmpty(wrIter, "");
        LE_DEBUG("after clear, dataId=0x%x",dataId);
        le_cfg_CommitTxn(wrIter);
        wrIter = le_cfg_CreateWriteTxn(node);
    }

    for(int i=0; i < dataSize; i++)
    {
        char dataStr[DID_NODE_LEN] = {0};
        snprintf(dataStr, sizeof(dataStr), DID_DATA_FORMAT, i+1);
        le_cfg_SetInt(wrIter, dataStr, dataPtr[i]); //Store data
    }
    le_cfg_CommitTxn(wrIter);

    return POSITIVE_RESPONSE;
}

/**
 * Read DTC from ConfigTree on UDS client defined statusMask request for subFunction
 * reportDTCByStatusMask (0x02).
 */
uint8_t UdsCommunicationMgr::readDTCByStatusMask
(
    uint8_t statusMask
)
{
    le_cfg_IteratorRef_t iteratorRef_r = le_cfg_CreateReadTxn(DTC_CONFIG_TREE_NODE);

    if (le_cfg_NodeExists(iteratorRef_r, DTC_STATUS_AVAILABILITY_MASK) == false)
    {
        LE_ERROR("No availability mask.");

        le_cfg_CancelTxn(iteratorRef_r);
        return REQ_OUT_OF_RANGE;
    }

    //get DTCStatusAvailabilityMask
    int DTCStatusAvailabilityMask = le_cfg_GetInt(iteratorRef_r, DTC_STATUS_AVAILABILITY_MASK, 0);
    sendBuf[0] = READ_DTC_INFO_RESPONSE_ID;
    sendBuf[1] = DTC_SUB_FUNCTION_REPORT_DTC_BY_STATUS;
    sendBuf[2] = DTCStatusAvailabilityMask;
    sendDataLen = UDS_READ_DTC_INFO_RESP_BASE_LEN;

    //No info node, return DTCStatusAvailabilityMask
    if (le_cfg_NodeExists(iteratorRef_r, DTC_INFORMATION) == false)
    {
        le_cfg_CancelTxn(iteratorRef_r);
        return POSITIVE_RESPONSE;
    }

    //get info list
    le_cfg_GoToNode (iteratorRef_r, DTC_INFORMATION);

    if(le_cfg_GoToFirstChild (iteratorRef_r) != LE_OK)
    {
        le_cfg_CancelTxn(iteratorRef_r);
        return POSITIVE_RESPONSE;
    }

    int i=0;
    do {
        char DTCStr[DTC_STR_INFO_LEN];
        uint32_t statusOfDTC;
        uint32_t DTCNumber;
        le_cfg_GetNodeName(iteratorRef_r, "", DTCStr, DTC_STR_INFO_LEN);
        statusOfDTC = le_cfg_GetInt(iteratorRef_r, DTC_STATUS, 0);

        if( (statusOfDTC & statusMask) == 0)
            continue;

        if (sscanf(DTCStr, "%x", &DTCNumber) == 1) {
            sendBuf[UDS_READ_DTC_INFO_RESP_BASE_LEN + i*4] = (DTCNumber & 0xff0000) >> 16;
            sendBuf[UDS_READ_DTC_INFO_RESP_BASE_LEN + i*4 + 1] = (DTCNumber & 0xff00) >> 8;
            sendBuf[UDS_READ_DTC_INFO_RESP_BASE_LEN + i*4 + 2] = DTCNumber & 0xff;
            sendBuf[UDS_READ_DTC_INFO_RESP_BASE_LEN + i*4 + 3] = statusOfDTC;
            i++;
        }
        else
        {
            LE_ERROR("Error DTC format.");
            le_cfg_CancelTxn(iteratorRef_r);
            return REQ_OUT_OF_RANGE;
        }

    }while (le_cfg_GoToNextSibling(iteratorRef_r) == LE_OK);

    sendDataLen = UDS_READ_DTC_INFO_RESP_BASE_LEN + (i*4);

    le_cfg_CancelTxn(iteratorRef_r);

    return POSITIVE_RESPONSE;
}

/**
 * Send ReadDTCInformation response message.
 */
le_result_t UdsCommunicationMgr::ReadDTCInfoResp
(
)
{
    LE_DEBUG("ReadDTCInfoResp");

    uint8_t ret;
    // received service ID and sub function.
    uint8_t sid = recvBuf[0];
    uint8_t subFunc = recvBuf[1];

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_READ_DTC_INFO_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the ReadDTC request msg minimum length.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    // Supported sub function check, only support sub function 0x2 currently.
    if(subFunc != DTC_SUB_FUNCTION_REPORT_DTC_BY_STATUS)
    {
        return SetNRC(sid, SUBFUNCTION_NOT_SUPPORTED);
    }

    uint8_t statusMask = recvBuf[2];
    ret = readDTCByStatusMask(statusMask);

    if (ret == REQ_OUT_OF_RANGE)
    {
        return SetNRC(sid, REQ_OUT_OF_RANGE);
    }

    return LE_OK;
}

/**
 * Send ReadDataByIdentifier response message.
 */
le_result_t UdsCommunicationMgr::ReadDIDResp
(
)
{
    LE_DEBUG("ReadDIDResp");

    uint16_t didNum = 0;
    uint16_t did = 0;
    uint8_t ret;

    // received service ID
    uint8_t sid = recvBuf[0];

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_READ_DID_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the ReadDID request msg minimum length.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    // Maximum length check, NRC 13
    if((recvDataLen -1) % 2 !=0)
    {
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    // Atleast one did present, NRC 31
    didNum = (recvDataLen -1)/2;
    if(didNum == 0)
    {
        return SetNRC(sid, REQ_OUT_OF_RANGE);
    }

    sendBuf[0] = READ_DID_RESPONSE_ID;
    sendDataLen = 1;
    for(uint16_t i = 0; i < didNum; i++)
    {
        //Total response length exceeded, NRC 14
        if(sendDataLen + UDS_DID_LEN > UDS_DATA_SIZE )
        {
            return SetNRC(sid, RESP_TOO_LONG);
        }

        if ((i*UDS_DID_LEN + 2) < UDS_DATA_SIZE)
        {
            did = ((recvBuf[i*UDS_DID_LEN + 1]) << 8) +
                    recvBuf[i*UDS_DID_LEN + 2];
            sendBuf[sendDataLen] = recvBuf[i*UDS_DID_LEN + 1];
            sendBuf[sendDataLen+1] = recvBuf[i*UDS_DID_LEN + 2];
            sendDataLen = sendDataLen + UDS_DID_LEN;
        }
        else
        {
            return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
        }

        ret = readDIDFromConfigTree(did);

        //check max send buf
        if(ret == RESP_TOO_LONG)
        {
            return SetNRC(sid, RESP_TOO_LONG);
        }
        //Can't get did value, because the did(2 bytes) is already filled in sendBuf, remove them.
        else if (ret == REQ_OUT_OF_RANGE)
        {
            break;
        }
    }

    if(sendDataLen == 1)
    {
        return SetNRC(sid, REQ_OUT_OF_RANGE);
    }

    return LE_OK;
}

/**
 * Send WriteDataByIdentifier response message
 */
le_result_t UdsCommunicationMgr::WriteDIDResp
(
)
{
    LE_DEBUG("WriteDIDResp");

    uint16_t did = 0;
    uint8_t ret;
    uint8_t* dataPtr = NULL;

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check active session type for WriteDataByIdentifier.
    if (SessionType == DEFAULT_SESSION)
    {
        LE_DEBUG("Default session type is active for WriteDIDResp.");
        return SetNRC(sid, CONDITIONS_NOT_CORRECT);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_WRITE_DID_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the WriteDID request msg minimum length.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    did = (recvBuf[1] << 8) + recvBuf[2];
    dataPtr = &recvBuf[3];

    ret = writeDIDToConfigTree(did, dataPtr, recvDataLen-UDS_WRITE_DID_REQ_BASE_LEN);
    if (ret == REQ_OUT_OF_RANGE)
    {
        return SetNRC(sid, REQ_OUT_OF_RANGE);
    }

    // Fill the response data
    sendBuf[0] = WRITE_DID_RESPONSE_ID;
    sendBuf[1] = recvBuf[1];
    sendBuf[2] = recvBuf[2];
    sendDataLen = UDS_WRITE_DID_RESP_LEN;

    return LE_OK;
}

/**
 * Send Session control response message.
 */
le_result_t UdsCommunicationMgr::SessionCtrlResp
(
)
{
    LE_DEBUG("SessionCtrlResp");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_SESSION_CTRL_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the session control request msg minimum length.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    switch (recvBuf[1])
    {
        case DEFAULT_SESSION:
            break;
        case PROGRAMMING_SESSION:
            break;
        case EXTENDED_DIAGNOSTIC_SESSION:
            break;
        default:
            LE_DEBUG("Requested session type is not supported");
            return SetNRC(sid, SUBFUNCTION_NOT_SUPPORTED);
    }

    SessionType = (taf_SessionType_t)(recvBuf[1] & 0x7F);

    // Fill the response data
    sendBuf[0] = SESSION_CONTROL_RESPONSE_ID;
    sendBuf[1] = recvBuf[1] & 0x7F;
    sendBuf[2] = (UDS_P2_SERVER & 0xff00) >> 8;
    sendBuf[3] = UDS_P2_SERVER & 0xff;
    sendBuf[4] = (UDS_P2_STAR_SERVER & 0xff00) >> 8;
    sendBuf[5] = UDS_P2_STAR_SERVER & 0xff;
    sendDataLen = UDS_SESSION_CTRL_RESP_LEN;

    return LE_OK;
}

/**
 * Indicate received ECUReset message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateECUResetReq
(
)
{
    LE_DEBUG("IndicateECUResetReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check active session type for ECUReset.
    if (SessionType != EXTENDED_DIAGNOSTIC_SESSION)
    {
        LE_DEBUG("Extended session type is not active.");
        return SetNRC(sid, CONDITIONS_NOT_CORRECT);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_ECU_RESET_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the ECUReset request msg minimum length.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    return LE_OK;
}

/**
 * Indicate received Routine control message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateRoutinrCtrlReq
(
)
{
    LE_DEBUG("IndicateRoutinrCtrlReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check active session type for RoutinrCtrlReq.
    if (SessionType == DEFAULT_SESSION)
    {
        LE_DEBUG("Default session type is active for RequestFileTransfer.");
        return SetNRC(sid, CONDITIONS_NOT_CORRECT);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_ROUTINE_CTRL_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the RoutinrCtrlReq msg minimum length.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    return LE_OK;
}

/**
 * Check NRC and Indicate received TransferData (0x36) message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateRxXferDataReq
(
)
{
    LE_DEBUG("IndicateRxXferDataReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check active session type for TransferData.
    if (SessionType != PROGRAMMING_SESSION)
    {
        LE_DEBUG("Programming session type is not active for TransferData.");
        return SetNRC(sid, CONDITIONS_NOT_CORRECT);
    }

    if (!isXferActive)
    {
        return SetNRC(sid, UPLOAD_DOWNLOAD_NOT_ACCEPTED);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        isXferActive = false;
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    // Check negative err code for minimum request msg length
    if (recvDataLen < UDS_REQ_XFER_DATA_BASE_LEN)
    {
        isXferActive = false;
        LE_DEBUG("recvDataLen is less than the RxXferDataReq msg minimum length.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    return LE_OK;
}

/**
 * Check NRC and Indicate received RequestTransferExit (0x37) message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateRxXferExitReq
(
)
{
    LE_DEBUG("IndicateRxXferExitReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check active session type for RequestTransferExit.
    if (SessionType != PROGRAMMING_SESSION)
    {
        LE_DEBUG("Programming session type is not active for RequestTransferExit.");
        return SetNRC(sid, CONDITIONS_NOT_CORRECT);
    }

    if (!isXferActive)
    {
        return SetNRC(sid, UPLOAD_DOWNLOAD_NOT_ACCEPTED);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    // Check negative err code for minimum request msg length
    if (recvDataLen < UDS_REQ_XFER_EXIT_BASE_LEN)
    {
        LE_DEBUG("recvDataLen is less than the RxXferExitReq msg minimum length.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    return LE_OK;
}

/**
 * Check NRC and Indicate received RequestFileTransfer (0x38) message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateRxFileXferReq
(
)
{
    LE_DEBUG("IndicateRxFileXferReq");

    // received service ID
    uint8_t sid = recvBuf[0];
    uint8_t fileSizeParameterLen = 0;

    // Check active session type for RequestFileTransfer.
    if (SessionType != PROGRAMMING_SESSION)
    {
        LE_DEBUG("Programming session type is not active for RequestFileTransfer.");
        return SetNRC(sid, CONDITIONS_NOT_CORRECT);
    }

    if (isXferActive)
    {
        return SetNRC(sid, CONDITIONS_NOT_CORRECT);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    // Check negative err code for minimum request msg length
    if (recvDataLen < UDS_REQ_FILE_XFER_BASE_LEN)
    {
        LE_DEBUG("recvDataLen is less than the RxFileXferReq msg minimum length.");
        return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
    }

    uint8_t modeOfOperation = recvBuf[1];
    uint16_t filePathAndNameLen = recvBuf[2] << 8 | recvBuf[3]; //check MSB

    if (filePathAndNameLen < 1)
    {
        return SetNRC(sid, REQ_OUT_OF_RANGE);
    }

    LE_DEBUG("modeofoperation =%d",modeOfOperation);
    switch (modeOfOperation)
    {
        case ADD_FILE:
        {
            if (recvDataLen < (UDS_REQ_FILE_XFER_BASE_LEN + filePathAndNameLen +
                    UDS_REQ_FILE_XFER_DATA_FORMAT_ID_LEN))
            {
                return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
            }

            fileSizeParameterLen = recvBuf[UDS_REQ_FILE_XFER_BASE_LEN +
                    filePathAndNameLen + UDS_REQ_FILE_XFER_DATA_FORMAT_ID_LEN];

            if (fileSizeParameterLen > 4) //the file size will be more than 1G
            {
                return SetNRC(sid, REQ_OUT_OF_RANGE);
            }

            if (recvDataLen < (UDS_REQ_FILE_XFER_BASE_LEN + filePathAndNameLen +
                    UDS_REQ_FILE_XFER_DATA_FORMAT_ID_LEN +
                            UDS_REQ_FILE_XFER_FILE_SIZE_PARAMETER_LEN + fileSizeParameterLen*2))
            {
                return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
            }
        }
        break;

        case DELETE_FILE:
        {
            if (recvDataLen < UDS_REQ_FILE_XFER_BASE_LEN + filePathAndNameLen)
            {
                return SetNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
            }
        }
        break;

        default:
            LE_ERROR("Requested mode of operation is not supported");
            SetNRC(sid, CONDITIONS_NOT_CORRECT);
            break;
    }

    return LE_OK;
}

/**
 * Check NRC and Send indication message to Diag service.
 */
le_result_t UdsCommunicationMgr::CheckAndSendInd
(
    uint8_t sid,
    taf_doip_AddrInfo_t* addrInfoPtr,
    taf_doip_DiagMsg_t* diagMsgPtr
)
{
    taf_doip_AddrInfo_t indAddrInfo;
    taf_doip_DiagMsg_t indDiagMsg;

    if(addrInfoPtr == NULL || diagMsgPtr == NULL)
    {
        LE_ERROR("Not find handler");
        return SetNRC(sid, GENERAL_PROGRAMMING_FAILURE);
    }

    if(udsIndicationHandler.safeRef == NULL)
    {
        LE_ERROR("Not find handler");
        return SetNRC(sid, GENERAL_PROGRAMMING_FAILURE);
    }

    taf_UDSIndicationHandler_t* udsHandler =
            (taf_UDSIndicationHandler_t*)le_ref_Lookup(udsHandlerRefMap,
                    udsIndicationHandler.safeRef);

    if(udsHandler == NULL)
    {
        LE_ERROR("Not find handler");
        return SetNRC(sid, GENERAL_PROGRAMMING_FAILURE);
    }
    if(udsHandler->funcPtr == NULL)
    {
        LE_ERROR("Not find handler");
        return SetNRC(sid, GENERAL_PROGRAMMING_FAILURE);
    }

    LE_DEBUG("------callback -------");

    indAddrInfo.sa = addrInfoPtr->sa;
    indAddrInfo.ta = addrInfoPtr->ta;
    indAddrInfo.taType = addrInfoPtr->taType;
    indDiagMsg.dataPtr = recvBuf;
    indDiagMsg.dataLen = recvDataLen;
    udsHandler->funcPtr(&indAddrInfo, &indDiagMsg, TAF_DOIP_RESULT_OK, udsHandler->ctxPtr);
    sendDataLen = 0;

    return LE_OK;
}

/**
 * Check NRC and Indicate to Diag service on receiving request message from DoIP.
 */
void UdsCommunicationMgr::DiagIndicationHandler
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    taf_doip_DiagMsg_t*   diagMsgPtr,
    taf_doip_Result_t result,
    void* userPtr
)
{
    LE_DEBUG("DiagIndicationHandler");

    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();
    bool isInternalHandle = true;

    taf_doip_AddrInfo_t respAddrInfo;
    taf_doip_DiagMsg_t respDiagMsg;
    le_result_t ret = LE_OK;

    if(addrInfoPtr == NULL)
    {
        LE_ERROR("addrInfoPtr invalid.");
        return;
    }

    if ((result == TAF_DOIP_RESULT_SA_REGISTERED) || (result == TAF_DOIP_RESULT_SA_DEREGISTERED))
    {
        LE_DEBUG("result =%d",result);
        return;
    }

    if(diagMsgPtr == NULL)
    {
        LE_ERROR("diagMsgPtr invalid.");
        return;
    }

    LE_DEBUG("UDS requst is from 0x%x to 0x%x", addrInfoPtr->sa, addrInfoPtr->ta);
    LE_DEBUG("Data pack length: %" PRIuS, diagMsgPtr->dataLen);

    if (result != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Diag message invalid.");
        return;
    }

    memcpy((char*)(udsCmMgr.recvBuf), (char*)(diagMsgPtr->dataPtr), UDS_DATA_SIZE);
    udsCmMgr.recvDataLen = diagMsgPtr->dataLen;
    udsCmMgr.sendDataLen = 0;

    // recived service ID
    uint8_t sid = udsCmMgr.recvBuf[0];

    LE_DEBUG("-------Request service id = 0x%x",sid);
    //Handle the request message according to the service id
    switch (sid)
    {
        case SESSION_CONTROL_REQUEST_ID:  // 0x10
        {
            // Check NRC and Handle it internally and then response to client.
            ret = udsCmMgr.SessionCtrlResp();
            isInternalHandle = true;
        }
        break;
        case ECU_RESET_REQUEST_ID:  // 0x11
        {
            // Check NRC and then send indication to TelAf diag service for ECUReset request msg.
            ret = udsCmMgr.IndicateECUResetReq();
            isInternalHandle = false;
        }
        break;
        case READ_DTC_INFO_REQUEST_ID:  // 0x19
        {
            // Check NRC and Handle it internally and then response to client.
            ret = udsCmMgr.ReadDTCInfoResp();
            isInternalHandle = true;
        }
        break;
        case READ_DID_REQUEST_ID:  // 0x22
        {
            // Check NRC and Handle it internally and then response to client.
            ret = udsCmMgr.ReadDIDResp();
            isInternalHandle = true;
        }
        break;
        case WRITE_DID_REQUEST_ID:  // 0x2E
        {
            // Check NRC and Handle it internally and then response to client.
            ret = udsCmMgr.WriteDIDResp();
            isInternalHandle = true;
        }
        break;
        case ROUTINE_CONTROL_REQUEST_ID: // 0x31
        {
            // Check NRC and then send indication to TelAf diag service for
            // RoutineControl request msg.
            ret = udsCmMgr.IndicateRoutinrCtrlReq();
            isInternalHandle = false;
        }
        break;
        case TRANSFER_DATA_REQUEST_ID:  // 0x36
        {
            // Check NRC and then send indication to TelAf diag service for
            // TransferData request msg.
            udsCmMgr.IndicateRxXferDataReq();
            isInternalHandle = false;
        }
        break;
        case REQUEST_TRANSFER_EXIT_REQUEST_ID:  // 0x37
        {
            // Check NRC and then send indication to TelAf diag service for
            // RequestTransferExit request msg.
            udsCmMgr.IndicateRxXferExitReq();
            isInternalHandle = false;
        }
        break;
        case REQUEST_FILE_TRANSFER_REQUEST_ID:  // 0x38
        {
            // Check NRC and then send indication to TelAf diag service
            // for RequestFileTransfer request msg.
            udsCmMgr.IndicateRxFileXferReq();
            isInternalHandle = false;
        }
        break;
        default:
        {
            LE_DEBUG("Service type is not supported");
            udsCmMgr.SetNRC(sid, SERVICE_NOT_SUPPORTED);
        }
        break;
    }

    if(ret != LE_OK)
    {
        LE_ERROR("Send error");
        return;
    }

    if(!isInternalHandle && udsCmMgr.sendDataLen == 0)
    {
        udsCmMgr.CheckAndSendInd(sid, addrInfoPtr, diagMsgPtr);
    }

    if(udsCmMgr.sendDataLen > 0 && udsCmMgr.sendDataLen < UDS_DATA_SIZE)
    {
        respAddrInfo.sa = addrInfoPtr->ta;
        respAddrInfo.ta = addrInfoPtr->sa;
        respAddrInfo.taType = addrInfoPtr->taType;
        respDiagMsg.dataPtr = udsCmMgr.sendBuf;
        respDiagMsg.dataLen = udsCmMgr.sendDataLen;

        ret = taf_doip_DiagRequest(&respAddrInfo, &respDiagMsg);
        if(ret == LE_OK)
        {
            LE_DEBUG("sent Diagnostic response successfully");
        }
    }

    return;
}

/**
 * Add handler function to indicate when message recived from DoIP stack.
 */
le_result_t UdsCommunicationMgr::UdsAddDiagIndicationHandler
(
)
{
    LE_INFO("UdsAddDiagIndicationHandler");

    if (DoipEntityRef == NULL)
    {
        LE_ERROR("DoIP stack is not initialized");
        return LE_FAULT;
    }

    IndicationRef = taf_doip_AddDiagIndicationHandler(DoipEntityRef,
            (taf_doip_DiagIndicationHandlerFunc_t)DiagIndicationHandler, NULL);

    if (IndicationRef == NULL)
    {
        LE_FATAL("Failed to register diag indication handler");
        return LE_FAULT;
    }

    return LE_OK;
}

/**
 * Send Diagnostic response message to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::SendUDSResp
(
    uint16_t sa,
    uint16_t ta,
    uint8_t addrType,
    uint8_t serviceId,
    uint8_t err,
    const uint8_t* dataPtr,
    uint16_t dataSize
)
{
    LE_DEBUG("SendUDSResp");

    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();

    le_result_t ret;
    taf_doip_DiagMsg_t respDiagMsg;
    taf_doip_AddrInfo_t respAddrInfo;

    if (DoipEntityRef == NULL)
    {
        LE_ERROR("DoIP stack is not initialized");
        return LE_FAULT;
    }

    if (serviceId != recvBuf[0])
    {
        LE_ERROR("Request and Response service id mismatch");
        return LE_BAD_PARAMETER;
    }

    // Check the send dataLength shall not be more than UDS_DATA_SIZE.
    if (dataSize > UDS_DATA_SIZE)
    {
        LE_ERROR("Send dataLength is more than max size.");
        return LE_FAULT;
    }

    LE_DEBUG("SERVICEID = 0x%x", serviceId);
    //Send UDS response according to the service id.
    switch(serviceId)
    {
        case ECU_RESET_REQUEST_ID:
            ret = ECUResetResp(serviceId, err);
        break;
        case ROUTINE_CONTROL_REQUEST_ID:
            ret = RoutineCtrlResp(serviceId, dataPtr, dataSize, err);
        break;
        case TRANSFER_DATA_REQUEST_ID:
            ret = XferDataResp(serviceId, dataPtr, dataSize, err);
        break;
        case REQUEST_TRANSFER_EXIT_REQUEST_ID:
            ret = ReqXferExitResp(serviceId, err);
        break;
        case REQUEST_FILE_TRANSFER_REQUEST_ID:
            ret = ReqFileXferResp(serviceId, err);
        break;
        default:
            SetNRC(serviceId, SERVICE_NOT_SUPPORTED);
            LE_ERROR("Service type is not supported");
            ret = LE_OK;
        break;
    }

    if (ret != LE_OK)
    {
        LE_ERROR("Send error");
        return ret;
    }

    respAddrInfo.sa = sa;
    respAddrInfo.ta = ta;
    respAddrInfo.taType = (taf_doip_TaType_t)addrType;
    respDiagMsg.dataPtr = udsCmMgr.sendBuf;
    respDiagMsg.dataLen = udsCmMgr.sendDataLen;

    ret = taf_doip_DiagRequest(&respAddrInfo, &respDiagMsg);
    if (ret == LE_OK)
    {
        LE_DEBUG("Requested Diagnostic message response sent.");
    }

    return LE_OK;
}

/**
 * Check error code and Pack ECUResetResp message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::ECUResetResp
(
    uint8_t serviceId,
    uint8_t err
)
{
    LE_DEBUG("ECUResetResp");

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    uint8_t resetType = recvBuf[1] & 0x7F;

    sendBuf[0] = ECU_RESET_RESPONSE_ID;
    sendBuf[1] = resetType;
    sendDataLen = UDS_ECU_RESET_RESP_BASE_LEN;

    return LE_OK;
}

/**
 * Check error code and Pack RoutineCtrlResp message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::RoutineCtrlResp
(
    uint8_t serviceId,
    const uint8_t* dataPtr,
    uint16_t dataSize,
    uint8_t err
)
{
    LE_DEBUG("RoutineCtrlResp");

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    uint8_t routineControlType = recvBuf[1] & 0x7F;
    uint16_t routineIdentifier = (recvBuf[2] << 8) + recvBuf[3];

    sendBuf[0] = ROUTINE_CONTROL_RESPONSE_ID;
    sendBuf[1] = routineControlType;
    sendBuf[2] = routineIdentifier >> 8;
    sendBuf[3] = routineIdentifier;

    if (dataPtr != NULL && dataSize != 0)
    {
        memcpy(sendBuf + UDS_ROUTINE_CTRL_RESP_MIN_LEN, dataPtr, dataSize);
        sendDataLen = UDS_ROUTINE_CTRL_RESP_MIN_LEN + dataSize;
    }
    else
    {
        sendDataLen = UDS_ROUTINE_CTRL_RESP_MIN_LEN;
    }

    return LE_OK;
}

/**
 * Check error code and Pack XferDataResp message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::XferDataResp
(
    uint8_t serviceId,
    const uint8_t* dataPtr,
    uint16_t dataSize,
    uint8_t err
)
{
    LE_DEBUG("XferDataResp");

    if (POSITIVE_RESPONSE != err)
    {
        isXferActive = false;
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    uint8_t blockSequenceCounter = recvBuf[1];

    sendBuf[0] = TRANSFER_DATA_RESPONSE_ID;
    sendBuf[1] = blockSequenceCounter;

    if (dataPtr != NULL && dataSize != 0)
    {
        memcpy(sendBuf + UDS_RESP_XFER_DATA_BASE_LEN, dataPtr, dataSize);
        sendDataLen = UDS_RESP_XFER_DATA_BASE_LEN + dataSize;
    }
    else
    {
        sendDataLen = UDS_RESP_XFER_DATA_BASE_LEN;
    }

    return LE_OK;
}

/**
 * Check error code and Pack ReqXferExitResp message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::ReqXferExitResp
(
    uint8_t serviceId,
    uint8_t err
)
{
    LE_DEBUG("ReqXferExitResp");

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    sendBuf[0] = REQUEST_TRANSFER_EXIT_RESPONSE_ID;
    sendDataLen = UDS_RESP_XFER_EXIT_BASE_LEN;

    isXferActive = false;

    return LE_OK;
}

/**
 * Check error code and Pack ReqFileXferResp message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::ReqFileXferResp
(
    uint8_t serviceId,
    uint8_t err
)
{
    LE_DEBUG("ReqFileXferResp");

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    uint8_t modeOfOperation = recvBuf[1];
    uint16_t filePathAndNameLen = recvBuf[2] << 8 | recvBuf[3];
    uint16_t maxNumberOfBlockLen = UDS_DATA_SIZE;
    uint8_t lengthFormatIdentifier = sizeof(maxNumberOfBlockLen) << 4;

    switch (modeOfOperation)
    {
        case ADD_FILE:
        {
            isXferActive = true;

            uint8_t dataFormatIdentifier = recvBuf[UDS_REQ_FILE_XFER_BASE_LEN +
                    filePathAndNameLen];

            sendBuf[0] = REQUEST_FILE_TRANSFER_RESPONSE_ID;
            sendBuf[1] = modeOfOperation;
            sendBuf[2] = lengthFormatIdentifier;
            for (uint8_t index = 0; index < sizeof(maxNumberOfBlockLen); index++)
            {
                uint8_t shiftBytes = sizeof(maxNumberOfBlockLen) - 1 - index;
                uint8_t indexValue = maxNumberOfBlockLen >> (shiftBytes * 8);
                sendBuf[UDS_RESP_FILE_XFER_BASE_LEN + UDS_RESP_FILE_XFER_LEN_FORMAT_ID_LEN + index]
                        = indexValue;
            }
            sendBuf[UDS_RESP_FILE_XFER_BASE_LEN + UDS_RESP_FILE_XFER_LEN_FORMAT_ID_LEN +
                    sizeof(maxNumberOfBlockLen)] = dataFormatIdentifier;

            sendDataLen = UDS_RESP_FILE_XFER_BASE_LEN + UDS_RESP_FILE_XFER_LEN_FORMAT_ID_LEN
                    + sizeof(maxNumberOfBlockLen) + UDS_RESP_FILE_XFER_DATA_FORMAT_ID_LEN;
        }
        break;

        case DELETE_FILE:
        {
            sendBuf[0] = REQUEST_FILE_TRANSFER_RESPONSE_ID;
            sendBuf[1] = modeOfOperation;
            sendDataLen = 2;
        }
        break;

        default:
            LE_ERROR("Response for requested mode of operation is not supported");
            break;
    }

    return LE_OK;
}
