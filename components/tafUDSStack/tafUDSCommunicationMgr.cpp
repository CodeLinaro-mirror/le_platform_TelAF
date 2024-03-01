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

    //P2 star timer
    p2StarTimerRef = le_timer_Create("UDSP2StarTimer");
    le_timer_SetMsInterval(p2StarTimerRef, UDS_P2_STAR_SERVER);
    le_timer_SetRepeat(p2StarTimerRef, 1);
    le_timer_SetHandler(p2StarTimerRef, P2StarTimeoutHandler);

    //S3 timer
    s3TimerRef = le_timer_Create("UDSS3Timer");
    le_timer_SetMsInterval(s3TimerRef, UDS_S3_SERVER);
    le_timer_SetRepeat(s3TimerRef, 1);
    le_timer_SetHandler(s3TimerRef, S3TimeoutHandler);

    LE_INFO("UDS communication manager ok.");
    return;
}

void UdsCommunicationMgr::P2StarTimeoutHandler
(
    le_timer_Ref_t timerRef
)
{
    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();
    LE_INFO("P2 star time out");

    udsCmMgr.readyToRecvData = true;
    memset(udsCmMgr.recvBuf, 0, UDS_DATA_SIZE);
    udsCmMgr.recvDataLen = 0;
    udsCmMgr.sendDataLen = 0;
}

/*
 * When 'timeout' or 'disconnection' happen, change to DEFAULT session
 * and raise a indication to applications.
*/
void UdsCommunicationMgr::IndicateWhenChangingToDefault()
{
    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();

    if (DEFAULT_SESSION == udsCmMgr.SessionType)
    {
        // No session change, just ignore
        return;
    }

    taf_SessionType_t oldSessionType = udsCmMgr.SessionType;
    udsCmMgr.SessionType = DEFAULT_SESSION;

    taf_doip_DiagMsg_t sesChangeMsg;
    if(udsCmMgr.udsIndicationHandler.safeRef == NULL)
    {
        LE_ERROR("Not find handler to notify session change");
        return;
    }

    taf_UDSIndicationHandler_t* udsHandler =
            (taf_UDSIndicationHandler_t*)le_ref_Lookup(udsCmMgr.udsHandlerRefMap,
                    udsCmMgr.udsIndicationHandler.safeRef);

    if(udsHandler == NULL || udsHandler->funcPtr == NULL)
    {
        LE_ERROR("Not find handler to notify session change");
        return;
    }

    udsCmMgr.sesChangeBuf[0] = udsCmMgr.sesChangeId;
    udsCmMgr.sesChangeBuf[1] = oldSessionType;
    udsCmMgr.sesChangeBuf[2] = udsCmMgr.SessionType;
    sesChangeMsg.dataPtr = udsCmMgr.sesChangeBuf;
    sesChangeMsg.dataLen = UDS_SESSION_CHANGE_DATA_SIZE;

    // Indicate session change to the application.
    udsHandler->funcPtr(&(udsCmMgr.addrInfo), &sesChangeMsg,
                        TAF_DOIP_RESULT_OK, udsHandler->ctxPtr);
}

void UdsCommunicationMgr::S3TimeoutHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("Session time out");
    IndicateWhenChangingToDefault();
}

static taf_doip_PowerMode_t PowerModeQueryHandler
(
    void* userPtr
)
{
    LE_DEBUG("PowerModeQueryHandler");

    return TAF_DOIP_POWER_MODE_NOT_SUPPORTED;
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
 * Read DTC from ConfigTree on UDS client defined statusMask request for subFunction
 * reportDTCByStatusMask (0x02).
 */
uint8_t UdsCommunicationMgr::readDTCByStatusMask
(
    uint8_t statusMask
)
{
    le_cfg_ConnectService();
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
 * Indicate received ReadDataByIdentifier message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateReadDIDResp
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateReadDIDResp");

    uint16_t didNum = 0;
    uint16_t dataId = 0;

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    *isInternalHandle = true;

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

    le_cfg_ConnectService();

    for(uint16_t i = 0; i < didNum; i++)
    {
        dataId = ((recvBuf[i*UDS_DID_LEN + 1]) << 8) + recvBuf[i*UDS_DID_LEN + 2];
        //Check security attribute
        char securedDidNode[DID_NODE_LEN] = { 0 };
        snprintf(securedDidNode, sizeof(securedDidNode), DID_READ_SEC_PROPERTY_SUPPORTED_FUNCTION,
                dataId);
        bool IsSecured = le_cfg_QuickGetBool(securedDidNode, false);

        LE_DEBUG("securedDidNode =%s,supported: %d", securedDidNode, IsSecured);
        if(IsSecured == true && securityLevel == 0)
        {
            LE_DEBUG("Did is secured and the server is not unlocked.");
            return SendNRC(sid, SECURITY_ACCESS_DENY, addrInfoPtr);
        }

        char readDidNode[DID_NODE_LEN] = { 0 };
        snprintf(readDidNode, sizeof(readDidNode), DID_READ_PROPERTY_SUPPORTED_FUNCTION, dataId);
        bool IsSupported = le_cfg_QuickGetBool(readDidNode, false);

        LE_DEBUG("readDidNode =%s,supported: %d", readDidNode, IsSupported);
        if(IsSupported == false)
        {
            LE_DEBUG("ReadDid is not supported.");
            return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);
        }
    }

    //Will send indication to the diag service
    *isInternalHandle = false;

    return LE_OK;
}

/**
 * Indicate received WriteDataByIdentifier message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateWriteDIDResp
(
    taf_doip_AddrInfo_t*  addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateWriteDIDResp");
    uint16_t dataId = 0;

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    *isInternalHandle = true;

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

    le_cfg_ConnectService();

    dataId = ((recvBuf[1]) << 8) + recvBuf[2];
    //Check security attribute
    char securedDidNode[DID_NODE_LEN] = { 0 };
    snprintf(securedDidNode, sizeof(securedDidNode), DID_WRITE_SEC_PROPERTY_SUPPORTED_FUNCTION,
            dataId);
    bool IsSecured = le_cfg_QuickGetBool(securedDidNode, false);

    LE_DEBUG("securedDidNode =%s,supported: %d", securedDidNode, IsSecured);
    if(IsSecured == true && securityLevel == 0)
    {
        LE_DEBUG("Did is secured and the server is not unlocked.");
        return SendNRC(sid, SECURITY_ACCESS_DENY, addrInfoPtr);
    }

    //Check write attribute
    char writeDidNode[DID_NODE_LEN] = { 0 };
    snprintf(writeDidNode, sizeof(writeDidNode), DID_WRITE_PROPERTY_SUPPORTED_FUNCTION, dataId);
    bool IsSupported = le_cfg_QuickGetBool(writeDidNode, false);

    LE_DEBUG("writeDidNode =%s,supported: %d", writeDidNode, IsSupported);
    if(IsSupported == false)
    {
        LE_DEBUG("WriteDid is not supported.");
        return SendNRC(sid, REQ_OUT_OF_RANGE, addrInfoPtr);
    }

    //Will send indication to the diag service
    *isInternalHandle = false;

    return LE_OK;
}

/**
 * Indicate received SessionCtrl message to Diag service.
 */
le_result_t UdsCommunicationMgr::IndicateSessionCtrlReq
(
    taf_doip_AddrInfo_t* addrInfoPtr,
    bool* isInternalHandle
)
{
    LE_DEBUG("IndicateSessionCtrlReq");

    // received service ID
    uint8_t sid = recvBuf[0];

    // Check the pointer.
    if(addrInfoPtr == NULL || isInternalHandle == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_FAULT;
    }

    // Received data length shall not be more than the UDS_DATA_SIZE (MAX limit)
    if(recvDataLen > UDS_DATA_SIZE)
    {
        LE_DEBUG("recvDataLen is more than the UDS_DATA_SIZE.");
        *isInternalHandle = true;
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    // Check negative err code for minimum request msg length
    if(recvDataLen < UDS_SESSION_CTRL_REQ_MIN_LEN)
    {
        LE_DEBUG("recvDataLen is less than the SessionCtrl request msg minimum length.");
        *isInternalHandle = true;
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
        case VEHICLE_MANUFACTURER_SPECIFIC_SESSION:
            break;
        case FOTA_SESSION:
            break;
        case DOWNLOADED_ENUMLATION_SESSION:
            break;
        case SYSTEM_SUPPLIER_SPECIFIC_SESSION:
            break;
        default:
            LE_DEBUG("Requested session type is not supported");
            *isInternalHandle = true;
            return SendNRC(sid, SUBFUNCTION_NOT_SUPPORTED, addrInfoPtr);
    }

    // copy addressInfo localy to use while sending session change indication.
    addrInfo.sa = addrInfoPtr->sa;
    addrInfo.ta = addrInfoPtr->ta;
    addrInfo.taType = addrInfoPtr->taType;

    //Will send indication to the diag service
    *isInternalHandle = false;
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
 * Check whether the file name is ASCII format.
 */
static bool isAcsiiFormat(const char * arr, size_t arrLen)
{
    for (size_t i = 0; i <= arrLen; i++)
    {
        if (arr[i] < 0 || arr[i] > 127)
        {
            return false;
        }
    }
    return true;
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
    LE_DEBUG("In %s", __FUNCTION__);

    #define RTF_SID recvBuf[0]
    #define RFT_MOOP recvBuf[1]
    #define LENGTH_OF_FILE_NAME (((recvBuf[2]) << 8 ) | (recvBuf[3]))
    #define LENGTH_OF_FILE_SIZE(buffer, loc) (buffer[loc])
    #define isTransferInProgress() isXferActive

    LE_INFO("[RFT] Request for moop:[0x%02X]", RFT_MOOP);

    // Check active session type for RequestFileTransfer.
    if (SessionType != PROGRAMMING_SESSION)
    {
        LE_ERROR("Programming session type is not active for RequestFileTransfer.");
        return SendNRC(RTF_SID, CONDITIONS_NOT_CORRECT, addrInfoPtr);
    }
    // Minimum length checking, the filePathAndNameLength >= 1
    else if (recvDataLen < RFT_MIN_LEN || recvDataLen > UDS_DATA_SIZE)
    {
        LE_ERROR("Bad data size for 0x38");
        return SendNRC(RTF_SID, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }
    // The validity check of the message parameters depends on the modeOfOperation parameter
    else if (RFT_MOOP < MOOP_ADD_FILE || RFT_MOOP > MOOP_RESUME_FILE)
    {
        LE_ERROR("Bad moop for 0x38");
        return SendNRC(RTF_SID, REQ_OUT_OF_RANGE, addrInfoPtr);
    }
    else
    {
        uint16_t filePathAndNameLength = LENGTH_OF_FILE_NAME;

        // Maximum length can be computed using fileSizeParamterLength and filePathAndNameLength
        switch(RFT_MOOP)
        {
            case MOOP_READ_DIR:
            case MOOP_DELETE_FILE:
            {
                if (recvDataLen != (RFT_BASE_LEN + filePathAndNameLength))
                {
                    LE_ERROR("Data length is mismatched for [0x%02X]", RFT_MOOP);
                    return SendNRC(RTF_SID, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
                }
            }
            break;

            case MOOP_ADD_FILE:
            case MOOP_RESUME_FILE:
            case MOOP_REPLACE_FILE:
            {
                uint8_t fileSizeParameterLength =
                    LENGTH_OF_FILE_SIZE(recvBuf, RFT_BASE_LEN + filePathAndNameLength + SIZE_OF_DFI_);

                if (fileSizeParameterLength > 4 /* 4 byptes == 32 bits --> 4GB */
                 || recvDataLen != (RFT_BASE_LEN + filePathAndNameLength
                                    + SIZE_OF_DFI_
                                    + SIZE_OF_FSL + (fileSizeParameterLength * 2)))
                {
                    LE_ERROR("Data length is mismatched for [0x%02X]", RFT_MOOP);
                    return SendNRC(RTF_SID, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
                }
            }
            break;

            case MOOP_READ_FILE:
            {
                if (recvDataLen != (RFT_BASE_LEN + filePathAndNameLength + SIZE_OF_DFI_))
                {
                    LE_ERROR("Data length is mismatched for [0x%02X]", RFT_MOOP);
                    return SendNRC(RTF_SID, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
                }
            }
            break;
        }

        // Check the specified filePathAndName is valid.
        if (!isAcsiiFormat((char *)(recvBuf + INDEX_FP_B1), filePathAndNameLength))
        {
            LE_ERROR("Data is not ASCII format");
            return SendNRC(RTF_SID, REQ_OUT_OF_RANGE, addrInfoPtr);
        }

        // Check if in the process of downloading or uploading data
        if (isTransferInProgress())
        {
            LE_ERROR("Bad order, transfer is in progress...");
            return SendNRC(RTF_SID, CONDITIONS_NOT_CORRECT, addrInfoPtr);
        }
    }

    //Will send the indication to the diag service
    *isInternalHandle = false;
    return LE_OK;
}

/**
 * Send Tester present response message.
 */
le_result_t UdsCommunicationMgr::TesterPresentResp
(
    taf_doip_AddrInfo_t*  addrInfoPtr
)
{
    LE_DEBUG("TesterPresentResp");

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

    // Check negative err code for request msg length
    if(recvDataLen != UDS_TESTER_PRESENT_REQ_LEN)
    {
        LE_DEBUG("recvDataLen is incorrect.");
        return SendNRC(sid, INCORRECT_MSG_LEN_OR_INVALID_FORMAT, addrInfoPtr);
    }

    if( (recvBuf[1] & 0x7F) != 0)
    {
        LE_DEBUG("sub function is incorrect.");
        return SendNRC(sid, SUBFUNCTION_NOT_SUPPORTED, addrInfoPtr);
    }

    // Fill the response data
    sendBuf[0] = TESTER_PRESENT_RESPONSE_ID;
    sendBuf[1] = 0;
    sendDataLen = UDS_TESTER_PRESENT_RESP_LEN;

    //Send positive response
    SendData(addrInfoPtr);

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
        LE_ERROR("Bad parameters for address & msg handler");
        return LE_FAULT;
    }

    if(udsIndicationHandler.safeRef == NULL)
    {
        LE_ERROR("Not found any valid reference.");
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
        LE_ERROR("Not found handler callback.");
        return SendNRC(sid, GENERAL_PROGRAMMING_FAILURE, addrInfoPtr);
    }

    //Send RCRRP, since the application might spend much time to handle the request.

    SendNRC(sid, REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING, addrInfoPtr);

    LE_DEBUG("------callback -------");
    readyToRecvData = false;
    le_timer_Start(p2StarTimerRef);

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
        LE_INFO("Disconnected, stopped the running timer");

        // Stop s3 timer
        if(le_timer_IsRunning(udsCmMgr.s3TimerRef))
        {
            LE_DEBUG("stop s3 running timer");
            le_timer_Stop(udsCmMgr.s3TimerRef);
        }

        // Stop p2 timer
        LE_DEBUG("stop P2 timer");
        le_timer_Stop(udsCmMgr.p2StarTimerRef);

        udsCmMgr.readyToRecvData = true;
        udsCmMgr.isXferActive = false;

        IndicateWhenChangingToDefault();

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
            // Check NRC and then send indication to TelAf diag service if necessary for Session
            // control request msg.
            ret = udsCmMgr.IndicateSessionCtrlReq(addrInfoPtr, &isInternalHandle);
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
            // Check NRC and then send indication to TelAf diag service if necessary for readDid.
            ret = udsCmMgr.IndicateReadDIDResp(addrInfoPtr, &isInternalHandle);
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
            // Check NRC and then send indication to TelAf diag service if necessary for writeDid.
            ret = udsCmMgr.IndicateWriteDIDResp(addrInfoPtr, &isInternalHandle);
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
        case TESTER_PRESENT_REQUEST_ID:  // 0x10
        {
            // Check NRC and Handle it internally and then response to client.
            ret = udsCmMgr.TesterPresentResp(addrInfoPtr);
            isInternalHandle = true;
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

    // Keep a diagnostic session other than the defaultSession active while not receiving any
    // diagnostic request message
    if((sid != SESSION_CONTROL_REQUEST_ID) && (udsCmMgr.SessionType != DEFAULT_SESSION))
    {
        LE_DEBUG("In non-default session, received the request, then restart the timer");
        le_timer_Restart(udsCmMgr.s3TimerRef);
    }

    if(!isInternalHandle)
    {
        udsCmMgr.CheckAndSendInd(sid, addrInfoPtr, diagMsgPtr);
    }

    return;
}

/**
 * Session change timer setting
*/
void UdsCommunicationMgr::SesChangeTimer
(
)
{
    auto& udsCmMgr = UdsCommunicationMgr::GetInstance();
    LE_DEBUG("SesChangeTimer");

    if (udsCmMgr.recvBuf[0] == SESSION_CONTROL_REQUEST_ID)
    {
        taf_SessionType_t newSessionType;

        newSessionType = (taf_SessionType_t)(udsCmMgr.recvBuf[1] & 0x7F);

        if(udsCmMgr.SessionType != newSessionType)
        {
            // Session switched to default session.
            if(newSessionType == DEFAULT_SESSION)
            {
                if(le_timer_IsRunning(udsCmMgr.s3TimerRef))
                    le_timer_Stop(udsCmMgr.s3TimerRef);
            }
            //Session switched to non-default session.
            else
            {
                if(le_timer_IsRunning(udsCmMgr.s3TimerRef))
                    le_timer_Restart(udsCmMgr.s3TimerRef);
                else
                    le_timer_Start(udsCmMgr.s3TimerRef);
            }

            //Non default session to other session.
            if(udsCmMgr.SessionType != DEFAULT_SESSION)
            {
                udsCmMgr.reqSeedLevel = 0;
                udsCmMgr.securityLevel = 0;
                LE_DEBUG("Session switched, reset the security level");
            }
        }
    }
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

    // If service response is session control type then start timer for Non-default session.
    if (udsCmMgr.recvBuf[0] == SESSION_CONTROL_REQUEST_ID)
    {
        udsCmMgr.SesChangeTimer();
    }

    if (result == TAF_DOIP_RESULT_OK)
    {
        le_timer_Stop(udsCmMgr.p2StarTimerRef);
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
        case SESSION_CONTROL_REQUEST_ID:
            ret = SessionCtrlResp(serviceId, err);
        break;
        case ECU_RESET_REQUEST_ID:
            ret = ECUResetResp(serviceId, err);
        break;
        case READ_DID_REQUEST_ID:
            ret = ReadDIDResp(serviceId, dataPtr, dataSize, err);
        break;
        case WRITE_DID_REQUEST_ID:
            ret = WriteDIDResp(serviceId, err);
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
            ret = ReqFileXferResp(serviceId, dataPtr, dataSize, err);
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
 * Check error code and Pack SessionCtrlResp message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::SessionCtrlResp
(
    uint8_t serviceId,
    uint8_t err
)
{
    LE_INFO("SessionCtrlResp");

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    taf_SessionType_t oldSessionType;
    taf_SessionType_t newSessionType;

    newSessionType = (taf_SessionType_t)(recvBuf[1] & 0x7F);

    // copy the previous session type as old session locally.
    oldSessionType = SessionType;

    // change the session type as requested and maintain it in stack
    SessionType = newSessionType;

    // Fill the response data to send the session response msg to DTool
    sendBuf[0] = SESSION_CONTROL_RESPONSE_ID;
    sendBuf[1] = recvBuf[1] & 0x7F;
    sendBuf[2] = (UDS_P2_SERVER & 0xff00) >> 8;
    sendBuf[3] = UDS_P2_SERVER & 0xff;
    sendBuf[4] = (UDS_P2_STAR_SERVER & 0xff00) >> 8;
    sendBuf[5] = UDS_P2_STAR_SERVER & 0xff;
    sendDataLen = UDS_SESSION_CTRL_RESP_LEN;

    // Notify session change to the application.
    if (oldSessionType != newSessionType)
    {
        taf_doip_DiagMsg_t sesChangeMsg;
        if(udsIndicationHandler.safeRef == NULL)
        {
            LE_ERROR("Not find handler to notify session change");
            return LE_OK;
        }

        taf_UDSIndicationHandler_t* udsHandler =
                (taf_UDSIndicationHandler_t*)le_ref_Lookup(udsHandlerRefMap,
                        udsIndicationHandler.safeRef);

        if(udsHandler == NULL || udsHandler->funcPtr == NULL)
        {
            LE_ERROR("Not find handler to notify session change!");
            return LE_OK;
        }

        LE_DEBUG("Notify session change to application!");
        sesChangeBuf[0] = sesChangeId;
        sesChangeBuf[1] = oldSessionType;
        sesChangeBuf[2] = SessionType;
        sesChangeMsg.dataPtr = sesChangeBuf;
        sesChangeMsg.dataLen = UDS_SESSION_CHANGE_DATA_SIZE;
        udsHandler->funcPtr(&addrInfo, &sesChangeMsg, TAF_DOIP_RESULT_OK, udsHandler->ctxPtr);
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
 * Check error code and Pack ReadDID message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::ReadDIDResp
(
    uint8_t serviceId,
    const uint8_t* dataPtr,
    uint16_t dataSize,
    uint8_t err
)
{
    LE_DEBUG("ReadDIDResp");

    // Check the send dataLength.
    if (dataSize > UDS_DATA_SIZE - UDS_READ_DID_RESP_BASE_LEN ||
        dataSize < UDS_READ_DID_RESP_MIN_LEN)
    {
        LE_ERROR("dataLength is not correct.");
        return LE_FAULT;
    }

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    sendBuf[0] = READ_DID_RESPONSE_ID;

    if (dataPtr != NULL && dataSize != 0)
    {
        memcpy(sendBuf + UDS_READ_DID_RESP_BASE_LEN, dataPtr, dataSize);
        sendDataLen = UDS_READ_DID_RESP_BASE_LEN + dataSize;
    }
    else
    {
        return LE_FAULT;
    }

    return LE_OK;
}

/**
 * Check error code and Pack WriteDID message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::WriteDIDResp
(
    uint8_t serviceId,
    uint8_t err
)
{
    LE_DEBUG("WriteDIDResp");

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    sendBuf[0] = WRITE_DID_RESPONSE_ID;
    sendBuf[1] = recvBuf[1];
    sendBuf[2] = recvBuf[2];
    sendDataLen = UDS_WRITE_DID_RESP_LEN;

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

// Given a number, determine how many bytes it takes
static uint8_t HowManyChars(uint16_t maxNumberOfBlock)
{
    uint8_t nbytes = 0;
    while (maxNumberOfBlock) {
        maxNumberOfBlock >>= 1;
        nbytes++;
    }

    nbytes = (nbytes + 7) / 8;
    return nbytes;
}

/**
 * Check error code and Pack ReqFileXferResp message to send to Diag client/tool.
 */
le_result_t UdsCommunicationMgr::ReqFileXferResp
(
    uint8_t serviceId,
    const uint8_t* dataPtr,
    uint16_t dataSize,
    uint8_t err
)
{
    LE_DEBUG("In %s", __FUNCTION__);

    if (POSITIVE_RESPONSE != err)
    {
        LE_DEBUG("Error code reported from Diag service");
        SetNRC(serviceId, err);
        return LE_OK;
    }

    uint16_t filePathAndNameLength = LENGTH_OF_FILE_NAME;
    uint16_t maxNumberOfBlockLen = UDS_DATA_SIZE - 4; // Reduce the source & target addresses.
    uint8_t lengthFormatIdentifier = HowManyChars(maxNumberOfBlockLen);

    // Echo DFI_ in response
    #define RFT_DFI_ (recvBuf[RFT_BASE_LEN + filePathAndNameLength])

    switch (RFT_MOOP)
    {
        case MOOP_DELETE_FILE:
        {
            sendBuf[0] = serviceId + 0x40;
            sendBuf[1] = RFT_MOOP;
            sendDataLen = (SIZE_OF_SID + SIZE_OF_MOOP);
        }
        break;

        case MOOP_ADD_FILE:
        case MOOP_REPLACE_FILE:
        case MOOP_RESUME_FILE:
        {
            isXferActive = true;

            sendBuf[0] = serviceId + 0x40;
            sendBuf[1] = RFT_MOOP;
            sendBuf[2] = lengthFormatIdentifier;

            // Convert maxNumberOfBlockLen to big-endian storage.
            for (uint8_t index = 0; index < sizeof(maxNumberOfBlockLen); index++)
            {
                uint8_t shiftBytes = sizeof(maxNumberOfBlockLen) - 1 - index;
                uint8_t indexValue = (maxNumberOfBlockLen >> (shiftBytes * 8)) & 0xFF;
                sendBuf[RRFT_BASE_LEN + SIZE_OF_LFID + index] = indexValue;
            }

            sendBuf[RRFT_BASE_LEN + SIZE_OF_LFID + sizeof(maxNumberOfBlockLen)] = RFT_DFI_;

            sendDataLen = RRFT_BASE_LEN + SIZE_OF_LFID + sizeof(maxNumberOfBlockLen) + SIZE_OF_DFI_;

            // For MOOP_RESUME_FILE, filePosition is required
            if (dataPtr)
            {
                memcpy(sendBuf + sendDataLen, dataPtr, dataSize);
                sendDataLen += dataSize;
            }
            else
            {
                LE_FATAL_IF(RFT_MOOP == MOOP_RESUME_FILE, "No file position parameter");
            }
        }
        break;

        case MOOP_READ_FILE:
        case MOOP_READ_DIR:
        {
            isXferActive = true;

            sendBuf[0] = serviceId + 0x40;
            sendBuf[1] = RFT_MOOP;
            sendBuf[2] = lengthFormatIdentifier;

            // Convert maxNumberOfBlockLen to big-endian storage.
            for (uint8_t index = 0; index < sizeof(maxNumberOfBlockLen); index++)
            {
                uint8_t shiftBytes = sizeof(maxNumberOfBlockLen) - 1 - index;
                uint8_t indexValue = (maxNumberOfBlockLen >> (shiftBytes * 8)) & 0xFF;
                sendBuf[RRFT_BASE_LEN + SIZE_OF_LFID + index] = indexValue;
            }

            if (RFT_MOOP == MOOP_READ_DIR)
            {
                // For MOOP_READ_DIR, the fixed 0x00 is reponsed
                sendBuf[RRFT_BASE_LEN + SIZE_OF_LFID + sizeof(maxNumberOfBlockLen)] = 0x00;
            }
            else
            {
                sendBuf[RRFT_BASE_LEN + SIZE_OF_LFID + sizeof(maxNumberOfBlockLen)] = RFT_DFI_;
            }

            sendDataLen = RRFT_BASE_LEN + SIZE_OF_LFID + sizeof(maxNumberOfBlockLen) + SIZE_OF_DFI_;

            LE_FATAL_IF(dataPtr == NULL, "No file size or dir info length arguments");

            memcpy(sendBuf + sendDataLen, dataPtr, dataSize);
            sendDataLen += dataSize;
        }
        break;

        default:
            LE_ERROR("Response for requested mode of operation is not supported");
            break;
    }

    return LE_OK;
}
