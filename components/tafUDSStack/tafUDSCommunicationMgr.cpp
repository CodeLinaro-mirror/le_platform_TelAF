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

    timerRef = le_timer_Create("UDSP2Timer");
    le_timer_SetMsInterval(timerRef, UDS_P2_STAR_SERVER);
    le_timer_SetRepeat(timerRef, 1);
    le_timer_SetHandler(timerRef, P2TimeoutHandler);

    LE_INFO("UDS communication manager ok.");
    return;
}

void UdsCommunicationMgr::P2TimeoutHandler
(
    le_timer_Ref_t timerRef
)
{
    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();
    LE_INFO("--- time out");

    udsCmMgr.readyToRecvData = true;
    memset(udsCmMgr.recvBuf, 0, UDS_DATA_SIZE);
    udsCmMgr.recvDataLen = 0;
    udsCmMgr.sendDataLen = 0;
}

static taf_doip_PowerMode_t PowerModeQueryHandler
(
    void* userPtr
)
{
    LE_DEBUG("PowerModeQueryHandler");

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
 * Send NRC.
 */
le_result_t UdsCommunicationMgr::SendNRC
(
    uint8_t sid,
    uint8_t errorCode,
    taf_doip_AddrInfo_t*  addrInfoPtr
)
{
    LE_DEBUG("SendNRC, sid= 0x%x, error code=0x%x",sid, errorCode);

    // pack the NRC data
    sendBuf[0] = UDS_NEGATIVE_RESP_SID;
    sendBuf[1] = sid;
    sendBuf[2] = errorCode;
    sendDataLen = UDS_NEG_RESP_LEN;

    SendData(addrInfoPtr);

    return LE_OK;
}

void UdsCommunicationMgr::SendData
(
    taf_doip_AddrInfo_t*  addrInfoPtr
)
{
    le_result_t ret;
    taf_doip_AddrInfo_t respAddrInfo;
    taf_doip_DiagMsg_t respDiagMsg;

    respAddrInfo.sa = addrInfoPtr->ta;
    respAddrInfo.ta = addrInfoPtr->sa;
    respAddrInfo.taType = addrInfoPtr->taType;
    respDiagMsg.dataPtr = sendBuf;
    respDiagMsg.dataLen = sendDataLen;

    ret = taf_doip_DiagRequest(&respAddrInfo, &respDiagMsg);
    if(ret == LE_OK)
    {
        LE_DEBUG("Send Diagnostic response successfully");
    }
    else
    {
        LE_ERROR("Failed to send Diagnostic response");
    }
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

    char securedDidNode[DID_NODE_LEN] = { 0 };
    snprintf(securedDidNode, sizeof(securedDidNode), DID_READ_SEC_PROPERTY_SUPPORTED_FUNCTION,
            dataId);
    bool IsSecured = le_cfg_QuickGetBool(securedDidNode, false);

    LE_DEBUG("securedDidNode =%s,supported: %d", securedDidNode, IsSecured);
    if(IsSecured == true && securityLevel == 0)
    {
        LE_DEBUG("Did is secured and the server is not unlocked.");
        return SECURITY_ACCESS_DENY;
    }

    char readDidNode[DID_NODE_LEN] = { 0 };
    snprintf(readDidNode, sizeof(readDidNode), DID_READ_PROPERTY_SUPPORTED_FUNCTION, dataId);
    bool IsSupported = le_cfg_QuickGetBool(readDidNode, false);

    LE_DEBUG("readDidNode =%s,supported: %d", readDidNode, IsSupported);
    if(IsSupported == false)
    {
        LE_DEBUG("ReadDid is not supported.");
        return REQ_OUT_OF_RANGE;
    }

    char node[DID_NODE_LEN] = { 0 };
    snprintf(node, sizeof(node), DID_CONFIG_TREE_VALUE_FORMAT, dataId);

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
            LE_DEBUG("Data length is too long");
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

    char securedDidNode[DID_NODE_LEN] = { 0 };
    snprintf(securedDidNode, sizeof(securedDidNode), DID_WRITE_SEC_PROPERTY_SUPPORTED_FUNCTION,
            dataId);
    bool IsSecured = le_cfg_QuickGetBool(securedDidNode, false);

    LE_DEBUG("securedDidNode =%s,supported: %d", securedDidNode, IsSecured);
    if(IsSecured == true && securityLevel == 0)
    {
        LE_DEBUG("Did is secured and the server is not unlocked.");
        return SECURITY_ACCESS_DENY;
    }

    char writeDidNode[DID_NODE_LEN] = { 0 };
    snprintf(writeDidNode, sizeof(writeDidNode), DID_WRITE_PROPERTY_SUPPORTED_FUNCTION, dataId);
    bool IsSupported = le_cfg_QuickGetBool(writeDidNode, false);

    LE_DEBUG("writeDidNode =%s,supported: %d", writeDidNode, IsSupported);
    if(IsSupported == false)
    {
        LE_DEBUG("WriteDid is not supported.");
        return REQ_OUT_OF_RANGE;
    }

    char node[DID_NODE_LEN] = { 0 };
    snprintf(node, sizeof(node), DID_CONFIG_TREE_VALUE_FORMAT, dataId);
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
    taf_doip_AddrInfo_t*  addrInfoPtr
)
{
    LE_DEBUG("ReadDTCInfoResp");

    uint8_t ret;
    // received service ID and sub function.
    uint8_t sid = recvBuf[0];
    uint8_t subFunc;

    // Check the pointer.
    if(addrInfoPtr == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_READ_DTC_INFO_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the ReadDTC request msg minimum length.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    subFunc = recvBuf[1] & 0x7f;

    // Supported sub function check, only support sub function 0x2 currently.
    if(subFunc != DTC_SUB_FUNCTION_REPORT_DTC_BY_STATUS)
    {
        return SendNRC(sid, SUBFUNCTION_NOT_SUPPORTED, addrInfoPtr);
    }

    //Send RCRRP, since maybe it will spend much time to read data from config tree
    SendNRC(sid, REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING, addrInfoPtr);

    uint8_t statusMask = recvBuf[2];
    ret = readDTCByStatusMask(statusMask);

    if (ret == REQ_OUT_OF_RANGE)
    {
        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);
    }

    //Send positive response
    SendData(addrInfoPtr);
    return LE_OK;
}

/**
 * Send ReadDataByIdentifier response message.
 */
le_result_t UdsCommunicationMgr::ReadDIDResp
(
    taf_doip_AddrInfo_t*  addrInfoPtr
)
{
    LE_DEBUG("ReadDIDResp");

    uint16_t didNum = 0;
    uint16_t did = 0;
    uint8_t ret;

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_READ_DID_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the ReadDID request msg minimum length.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Maximum length check, NRC 13
    if((recvDataLen -1) % 2 !=0)
    {
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Atleast one did present, NRC 31
    didNum = (recvDataLen -1)/2;
    if(didNum == 0)
    {
        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);
    }

    //Send RCRRP, since maybe it will spend much time to read data from config tree
    SendNRC(sid, REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING, addrInfoPtr);

    sendBuf[0] = READ_DID_RESPONSE_ID;
    sendDataLen = 1;
    for(uint16_t i = 0; i < didNum; i++)
    {
        //Total response length exceeded, NRC 14
        if(sendDataLen + UDS_DID_LEN > UDS_DATA_SIZE )
        {
            return SendNRC(sid, RESP_TOO_LONG, addrInfoPtr);
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
            return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
        }

        ret = readDIDFromConfigTree(did);

        //If response is negative, send NRC
        if(ret != POSITIVE_RESPONSE)
        {
            return SendNRC(sid, ret, addrInfoPtr);
        }

    }

    //Send positive response
    SendData(addrInfoPtr);

    return LE_OK;
}

/**
 * Send WriteDataByIdentifier response message
 */
le_result_t UdsCommunicationMgr::WriteDIDResp
(
    taf_doip_AddrInfo_t*  addrInfoPtr
)
{
    LE_DEBUG("WriteDIDResp");

    uint16_t did = 0;
    uint8_t ret;
    uint8_t* dataPtr = NULL;

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Check active session type for WriteDataByIdentifier.
    if (SessionType == DEFAULT_SESSION)
    {
        LE_DEBUG("Default session type is active for WriteDIDResp.");
        return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_WRITE_DID_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the WriteDID request msg minimum length.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    //Send RCRRP, since maybe it will spend much time to write data to config tree
    SendNRC(sid, REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING, addrInfoPtr);

    did = (recvBuf[1] << 8) + recvBuf[2];
    dataPtr = &recvBuf[3];

    ret = writeDIDToConfigTree(did, dataPtr, recvDataLen-UDS_WRITE_DID_REQ_BASE_LEN);
    if (ret != POSITIVE_RESPONSE)
    {
        return SendNRC(sid, ret, addrInfoPtr);
    }

    // Fill the response data
    sendBuf[0] = WRITE_DID_RESPONSE_ID;
    sendBuf[1] = recvBuf[1];
    sendBuf[2] = recvBuf[2];
    sendDataLen = UDS_WRITE_DID_RESP_LEN;

    //Send positive response
    SendData(addrInfoPtr);

    return LE_OK;
}

/**
 * Send Session control response message.
 */
le_result_t UdsCommunicationMgr::SessionCtrlResp
(
    taf_doip_AddrInfo_t*  addrInfoPtr
)
{
    LE_DEBUG("SessionCtrlResp");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_SESSION_CTRL_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the session control request msg minimum length.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    switch (recvBuf[1] & 0x7F)
    {
        case DEFAULT_SESSION:
            break;
        case PROGRAMMING_SESSION:
            break;
        case EXTENDED_DIAGNOSTIC_SESSION:
            break;
        default:
            LE_DEBUG("Requested session type is not supported");
            return SendNRC(sid, SUBFUNCTION_NOT_SUPPORTED, addrInfoPtr);
    }

    if(SessionType != DEFAULT_SESSION)
    {
        reqSeedLevel = 0;
        securityLevel = 0;
        LE_INFO("Session switched, reset the security level");
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

    //Send positive response
    SendData(addrInfoPtr);

    return LE_OK;
}

/**
 * Indicate received ECUReset message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateECUResetReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateECUResetReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Check active session type for ECUReset.
    if (SessionType != EXTENDED_DIAGNOSTIC_SESSION)
    {
        LE_DEBUG("Extended session type is not active.");
        *isInternalHandle = true;
        return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_ECU_RESET_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the ECUReset request msg minimum length.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    //Will send indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

/**
 * Indicate received Security access message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateSecAccessReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateSecAccessReq");

    // received service ID
    uint8_t sid = recvBuf[0];
    uint8_t subFunc;

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Check active session type for SecurityAccess.
    if (SessionType == DEFAULT_SESSION)
    {
        LE_DEBUG("Default session type is active for IndicateSecAccessReq.");
        *isInternalHandle = true;
        return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_SECURITY_ACCESS_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the SecurityAccess msg minimum length.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    subFunc = recvBuf[1] & 0x7f;

    if(subFunc == 0 || (0x43 <= subFunc && subFunc >= 0x5E) || subFunc == 0x7f)
    {
        LE_DEBUG("subFunc is reserved.");
        *isInternalHandle = true;
        return SendNRC(sid, SUBFUNCTION_NOT_SUPPORTED, addrInfoPtr);
    }

    // requestSeed
    if (subFunc % 2 !=0)
    {
        reqSeedLevel = subFunc;
        //If already unlock with the same security level, send 0 as the seed.
        if(subFunc == securityLevel)
        {
            //send data 00
            // Fill the response data
            sendBuf[0] = SECURITY_ACCESS_RESPONSE_ID;
            sendBuf[1] = subFunc;
            sendBuf[2] = 0;
            sendBuf[3] = 0;
            sendDataLen = UDS_SECURITY_ACCESS_RESP_SEED_ZERO_LEN;

            //Send positive response
            SendData(addrInfoPtr);
            *isInternalHandle = true;
            return LE_OK;
        }
        else
        {
            //Will send the indication to the diag service
            *isInternalHandle = false;
            return LE_OK;
        }

    }
    // sendKey
    else
    {
        // Check negative err code for minimum sendkey request msg length
        if(recvDataLen < UDS_SECURITY_ACCESS_SEND_KEY_REQ_MIN_LEN)
        {
            LE_DEBUG("recvDataLen is less than the SecurityAccess sendkey msg minimum length.");
            *isInternalHandle = true;
            return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
        }

        //Without first receiving a 'requestSeed' request message.
        if(reqSeedLevel == 0)
        {
            LE_DEBUG("Without first receiving a 'requestSeed' request message.");
            *isInternalHandle = true;
            return SendNRC(sid, REQ_SEQUENCE_ERROR, addrInfoPtr);
        }

        //'SendKey' level shall equal the 'requestSeed' SubFunction parameter value plus one.
        if(subFunc != reqSeedLevel + 1)
        {
            LE_DEBUG("SendKey level shall equal the 'requestSeed' SubFunc param value plus one.");
            *isInternalHandle = true;
            return SendNRC(sid, REQ_SEQUENCE_ERROR, addrInfoPtr);
        }

        reqSeedLevel = 0;

        //Will send the indication to the diag service
        *isInternalHandle = false;
        return LE_OK;
    }

    return LE_OK;
}

/**
 * Indicate received Routine control message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateRoutinrCtrlReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateRoutinrCtrlReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Check active session type for RoutinrCtrlReq.
    if (SessionType == DEFAULT_SESSION)
    {
        LE_DEBUG("Default session type is active for RequestFileTransfer.");
        *isInternalHandle = true;
        return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_ROUTINE_CTRL_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the RoutinrCtrlReq msg minimum length.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    //Will send the indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

/**
 * Check NRC and Indicate received TransferData (0x36) message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateRxXferDataReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateRxXferDataReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Check active session type for TransferData.
    if (SessionType != PROGRAMMING_SESSION)
    {
        LE_DEBUG("Programming session type is not active for TransferData.");
        *isInternalHandle = true;
        return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
    }

    if (!isXferActive)
    {
        *isInternalHandle = true;
        return SendNRC(sid, UPLOAD_DOWNLOAD_NOT_ACCEPTED, addrInfoPtr);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        isXferActive = false;
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for minimum request msg length
    if (recvDataLen < UDS_REQ_XFER_DATA_BASE_LEN)
    {
        isXferActive = false;
        LE_DEBUG("recvDataLen is less than the RxXferDataReq msg minimum length.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    //Will send the indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

/**
 * Check NRC and Indicate received RequestTransferExit (0x37) message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateRxXferExitReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateRxXferExitReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Check active session type for RequestTransferExit.
    if (SessionType != PROGRAMMING_SESSION)
    {
        LE_DEBUG("Programming session type is not active for RequestTransferExit.");
        *isInternalHandle = true;
        return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
    }

    if (!isXferActive)
    {
        *isInternalHandle = true;
        return SendNRC(sid, UPLOAD_DOWNLOAD_NOT_ACCEPTED, addrInfoPtr);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for minimum request msg length
    if (recvDataLen < UDS_REQ_XFER_EXIT_BASE_LEN)
    {
        LE_DEBUG("recvDataLen is less than the RxXferExitReq msg minimum length.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    //Will send indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

/**
 * Check NRC and Indicate received RequestFileTransfer (0x38) message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateRxFileXferReq
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateRxFileXferReq");

    // received service ID
    uint8_t sid = recvBuf[0];
    uint8_t fileSizeParameterLen = 0;

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Check active session type for RequestFileTransfer.
    if (SessionType != PROGRAMMING_SESSION)
    {
        LE_DEBUG("Programming session type is not active for RequestFileTransfer.");
        *isInternalHandle = true;
        return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
    }

    if (isXferActive)
    {
        *isInternalHandle = true;
        return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for minimum request msg length
    if (recvDataLen < UDS_REQ_FILE_XFER_BASE_LEN)
    {
        LE_DEBUG("recvDataLen is less than the RxFileXferReq msg minimum length.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    uint8_t modeOfOperation = recvBuf[1];
    uint16_t filePathAndNameLen = recvBuf[2] << 8 | recvBuf[3]; //check MSB

    if (filePathAndNameLen < 1 || (filePathAndNameLen >= (UDS_DATA_SIZE -
            UDS_REQ_FILE_XFER_BASE_LEN - UDS_REQ_FILE_XFER_DATA_FORMAT_ID_LEN)))
    {
        *isInternalHandle = true;
        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);
    }

    LE_DEBUG("modeofoperation =%d",modeOfOperation);
    switch (modeOfOperation)
    {
        case ADD_FILE:
        {
            if (recvDataLen < (UDS_REQ_FILE_XFER_BASE_LEN + filePathAndNameLen +
                    UDS_REQ_FILE_XFER_DATA_FORMAT_ID_LEN))
            {
                *isInternalHandle = true;
                return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
            }

            fileSizeParameterLen = recvBuf[UDS_REQ_FILE_XFER_BASE_LEN +
                    filePathAndNameLen + UDS_REQ_FILE_XFER_DATA_FORMAT_ID_LEN];

            if (fileSizeParameterLen > 4) //the file size will be more than 1G
            {
                *isInternalHandle = true;
                return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);
            }

            if (recvDataLen < (UDS_REQ_FILE_XFER_BASE_LEN + filePathAndNameLen +
                    UDS_REQ_FILE_XFER_DATA_FORMAT_ID_LEN +
                            UDS_REQ_FILE_XFER_FILE_SIZE_PARAMETER_LEN + fileSizeParameterLen*2))
            {
                *isInternalHandle = true;
                return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
            }
        }
        break;

        case DELETE_FILE:
        {
            if (recvDataLen < UDS_REQ_FILE_XFER_BASE_LEN + filePathAndNameLen)
            {
                *isInternalHandle = true;
                return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
            }
        }
        break;

        default:
            LE_ERROR("Requested mode of operation is not supported");
            *isInternalHandle = true;
            return SendNRC(sid, CONDITIONS_NOT_CORRECT, addrInfoPtr);
    }

    //Will send the indication to the diag service
    *isInternalHandle = false;
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
        return LE_FAULT;
    }

    if(udsIndicationHandler.safeRef == NULL)
    {
        LE_ERROR("Not find handler");
        return SendNRC(sid, GENERAL_PROGRAMMING_FAILURE, addrInfoPtr);
    }

    taf_UDSIndicationHandler_t* udsHandler =
            (taf_UDSIndicationHandler_t*)le_ref_Lookup(udsHandlerRefMap,
                    udsIndicationHandler.safeRef);

    if(udsHandler == NULL)
    {
        LE_ERROR("Not find handler");
        return SendNRC(sid, GENERAL_PROGRAMMING_FAILURE, addrInfoPtr);
    }
    if(udsHandler->funcPtr == NULL)
    {
        LE_ERROR("Not find handler");
        return SendNRC(sid, GENERAL_PROGRAMMING_FAILURE, addrInfoPtr);
    }

    //Send RCRRP, since the application might spend much time to handle the request.

    SendNRC(sid, REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING, addrInfoPtr);

    LE_DEBUG("------callback -------");
    readyToRecvData = false;
    le_timer_Start(timerRef);

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
    le_result_t ret = LE_OK;

    if(addrInfoPtr == NULL)
    {
        LE_ERROR("addrInfoPtr invalid.");
        return;
    }

    if (result == TAF_DOIP_RESULT_SA_REGISTERED)
    {
        LE_DEBUG("result =%d",result);
        return;
    }

    if (result == TAF_DOIP_RESULT_SA_DEREGISTERED)
    {
        le_timer_Stop(udsCmMgr.timerRef);
        udsCmMgr.readyToRecvData = true;
        udsCmMgr.isXferActive = false;
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

    if(!udsCmMgr.readyToRecvData)
    {
        LE_ERROR("Handle in progress, can't receive another request");
        return;
    }

    memcpy((char*)(udsCmMgr.recvBuf), (char*)(diagMsgPtr->dataPtr), UDS_DATA_SIZE);
    udsCmMgr.recvDataLen = diagMsgPtr->dataLen;
    udsCmMgr.sendDataLen = 0;

    // Check negative err code for minimum request msg length
    if(udsCmMgr.recvDataLen < UDS_REQ_MIN_LEN)
    {
        LE_ERROR("recvDataLen is less than the UDS msg minimum length.");
        return;
    }

    // received service ID
    uint8_t sid = udsCmMgr.recvBuf[0];

    LE_DEBUG("-------Request service id = 0x%x",sid);
    //Handle the request message according to the service id
    switch (sid)
    {
        case SESSION_CONTROL_REQUEST_ID:  // 0x10
        {
            // Check NRC and Handle it internally and then response to client.
            ret = udsCmMgr.SessionCtrlResp(addrInfoPtr);
            isInternalHandle = true;
        }
        break;
        case ECU_RESET_REQUEST_ID:  // 0x11
        {
            // Check NRC and then send indication to TelAf diag service if necessary for ECUReset
            // request msg.
            ret = udsCmMgr.IndicateECUResetReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case READ_DTC_INFO_REQUEST_ID:  // 0x19
        {
            // Check NRC and Handle it internally and then response to client.
            ret = udsCmMgr.ReadDTCInfoResp(addrInfoPtr);
            isInternalHandle = true;
        }
        break;
        case READ_DID_REQUEST_ID:  // 0x22
        {
            // Check NRC and Handle it internally and then response to client.
            ret = udsCmMgr.ReadDIDResp(addrInfoPtr);
            isInternalHandle = true;
        }
        break;
        case SECURITY_ACCESS_REQUEST_ID:  // 0x27
        {
            // Check NRC and then send indication to TelAf diag service if necessary for
            // Security access request msg.
            ret = udsCmMgr.IndicateSecAccessReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case WRITE_DID_REQUEST_ID:  // 0x2E
        {
            // Check NRC and Handle it internally and then response to client.
            ret = udsCmMgr.WriteDIDResp(addrInfoPtr);
            isInternalHandle = true;
        }
        break;
        case ROUTINE_CONTROL_REQUEST_ID: // 0x31
        {
            // Check NRC and then send indication to TelAf diag service if necessary for
            // RoutineControl request msg.
            ret = udsCmMgr.IndicateRoutinrCtrlReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case TRANSFER_DATA_REQUEST_ID:  // 0x36
        {
            // Check NRC and then send indication to TelAf diag service if necessary for
            // TransferData request msg.
            ret = udsCmMgr.IndicateRxXferDataReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case REQUEST_TRANSFER_EXIT_REQUEST_ID:  // 0x37
        {
            // Check NRC and then send indication to TelAf diag service if necessary for
            // RequestTransferExit request msg.
            ret = udsCmMgr.IndicateRxXferExitReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        case REQUEST_FILE_TRANSFER_REQUEST_ID:  // 0x38
        {
            // Check NRC and then send indication to TelAf diag service if necessary
            // for RequestFileTransfer request msg.
            ret = udsCmMgr.IndicateRxFileXferReq(addrInfoPtr, &isInternalHandle);
        }
        break;
        default:
        {
            LE_DEBUG("Service type is not supported");
            ret = udsCmMgr.SendNRC(sid, SERVICE_NOT_SUPPORTED, addrInfoPtr);
            isInternalHandle = true;
        }
        break;
    }

    if(ret != LE_OK)
    {
        LE_ERROR("Failed to handle request");
        return;
    }

    if(!isInternalHandle)
    {
        udsCmMgr.CheckAndSendInd(sid, addrInfoPtr, diagMsgPtr);
    }

    return;
}

/**
 * Receive confirmation message from DoIP.
 */
void UdsCommunicationMgr::DiagConfirmHandler
(
    const taf_doip_AddrInfo_t*  addrInfoPtr, ///< [IN] Logical address information pointer.
    taf_doip_Result_t           result,      ///< [IN] Result of the confirm execution
    void*                       userPtr      ///< [IN] User-defined pointer
)
{
    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();

    LE_DEBUG("Receive doip confirmation, result is %d", result);

    if (result == TAF_DOIP_RESULT_OK)
    {
        le_timer_Stop(udsCmMgr.timerRef);
        udsCmMgr.readyToRecvData = true;
    }
    else
    {
        LE_ERROR("Failed to send the uds response.%d", result);
    }
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

    ConfirmRef = taf_doip_AddDiagConfirmHandler(DoipEntityRef,
        (taf_doip_DiagConfirmHandlerFunc_t)DiagConfirmHandler, NULL);
    if (ConfirmRef == NULL)
    {
        LE_FATAL("Failed to register diag confirmation handler");
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
        case SECURITY_ACCESS_REQUEST_ID:
            ret = SecurityAccessResp(serviceId, dataPtr, dataSize, err);
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
 * Check error code and Pack SecurityAccess message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::SecurityAccessResp
(
    uint8_t serviceId,
    const uint8_t* dataPtr,
    uint16_t dataSize,
    uint8_t err
)
{
    LE_DEBUG("SecurityAccessResp");

    // Check the send dataLength.
    if (dataSize > UDS_DATA_SIZE - UDS_SECURITY_ACCESS_RESP_MIN_LEN)
    {
        LE_ERROR("Send dataLength is more than max size.");
        return LE_FAULT;
    }

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    uint8_t securityAccessType = recvBuf[1] & 0x7F;

    sendBuf[0] = SECURITY_ACCESS_RESPONSE_ID;
    sendBuf[1] = securityAccessType;

    if (dataPtr != NULL && dataSize != 0)
    {
        memcpy(sendBuf + UDS_SECURITY_ACCESS_RESP_MIN_LEN, dataPtr, dataSize);
        sendDataLen = UDS_SECURITY_ACCESS_RESP_MIN_LEN + dataSize;
    }
    else
    {
        sendDataLen = UDS_SECURITY_ACCESS_RESP_MIN_LEN;
    }

    if (securityAccessType % 2 ==0)
        securityLevel = securityAccessType - 1;

    LE_DEBUG("securityLevel = %d", securityLevel);
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

    // Check the send dataLength.
    if (dataSize > UDS_DATA_SIZE - UDS_ROUTINE_CTRL_RESP_MIN_LEN)
    {
        LE_ERROR("Send dataLength is more than max size.");
        return LE_FAULT;
    }

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

    // Check the send dataLength.
    if (dataSize > UDS_DATA_SIZE - UDS_RESP_XFER_DATA_BASE_LEN)
    {
        LE_ERROR("Send dataLength is more than max size.");
        return LE_FAULT;
    }

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
