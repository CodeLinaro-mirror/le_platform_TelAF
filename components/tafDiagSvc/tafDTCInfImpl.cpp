/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include "tafDTCInf.hpp"
#include "tafDataAccessComp.h"
#include "tafEventSvr.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of DTC server.
 */
//--------------------------------------------------------------------------------------------------
taf_DTCInf &taf_DTCInf::GetInstance
(
)
{
    static taf_DTCInf instance;

    return instance;
}

//-------------------------------------------------------------------------------------------------
/**
 * UDS stack handler function
 */
//-------------------------------------------------------------------------------------------------
void taf_DTCInf::UDSMsgHandler
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

    //uint8_t msgPos = 1;  // Skip sid
    uint8_t errCode = 0;
    le_result_t result;

    if (sid == reqReadDTCSvcId)
    {
        LE_DEBUG("ReadDTCInformation request message received!");

        //taf_diagReadDTC_SubFunc_t subFunc = taf_diagReadDTC_SubFunc_t(msgPtr[1] & 0x7F)
        switch (taf_diagReadDTC_SubFunc_t(msgPtr[1] & 0x7F))
        {
            case REPORT_NO_OF_DTC_BY_STATUS_MASK:  // 0x01
            {
                LE_DEBUG("Requested DTC subfunction type is reportNumberOfDTCByStatusMask!");

                if (msgLen != NO_OF_DTC_BY_STATUS_MASK_REQ_LEN)
                {
                    LE_DEBUG("Requested DTC message length is incorrect!");
                    errCode = INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
                    SendNRCResp(sid, addrPtr, errCode);

                    return;
                }

                uint8_t dtcStatusMask = msgPtr[2];
                result = GetNumOfDtcByStatusMask(dtcStatusMask);
                if(result != LE_OK)
                {
                    LE_ERROR("Error while getting data for no of dtc by status mask!");
                    return;
                }
            }
            break;

            case REPORT_DTC_BY_STATUS_MASK:  // 0x02
            {
                LE_DEBUG("Requested DTC subfunction type is reportDTCByStatusMask!");

                if (msgLen != DTC_BY_STATUS_MASK_REQ_LEN)
                {
                    LE_DEBUG("Requested DTC message length is incorrect!");
                    errCode = INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
                    SendNRCResp(sid, addrPtr, errCode);

                    return;
                }

                uint8_t statusMask = msgPtr[2];

                result = GetDtcByStatusMask(statusMask);
                if(result != LE_OK)
                {
                    LE_ERROR("Error while getting data for dtc for status mask!");
                    return;
                }
            }
            break;

            case REPORT_DTC_EXT_DATA_REC_BY_DTC_NO:  // 0x06
            {
                LE_DEBUG("Requested DTC subfunction type is reportDTCExtDataRecordByDTCNumber!");

                if (msgLen != DTC_EXT_DATA_REC_BY_DTC_NO_REQ_LEN)
                {
                    LE_DEBUG("Requested DTC message length is incorrect!");
                    errCode = INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
                    SendNRCResp(sid, addrPtr, errCode);

                    return;
                }

                uint32_t dtcMaskRec = ((msgPtr[2] << 16) + (msgPtr[3] << 8) + (msgPtr[4]));
                uint8_t dtcExtDataRec = msgPtr[5];

                result = GetExtDataRecordByDTCNum(dtcMaskRec, dtcExtDataRec);
                if(result != LE_OK)
                {
                    LE_ERROR("Error while getting data for dtc for ext data record!");
                    return;
                }
            }
            break;

            case REPORT_SUPPORTED_DTC:  //0x0A
            {
                LE_DEBUG("Requested DTC subfunction type is reportSupportedDTC!");

                if (msgLen != SUPPORTED_DTC_REQ_LEN)
                {
                    LE_DEBUG("Requested DTC message length is incorrect!");
                    errCode = INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
                    SendNRCResp(sid, addrPtr, errCode);

                    return;
                }

                result = GetSupportedDtc();
                if(result != LE_OK)
                {
                    LE_ERROR("Error while getting data for dtc report supported DTC!");
                    return;
                }
            }
            break;

            case REPORT_DTC_FAULT_DETECTION_COUNTER:  // 0x14
            {
                LE_DEBUG("Requested DTC subfunction type is reportDTCFaultDetectionCounter!");

                if (msgLen != DTC_FAULT_DETECTION_COUNTER_REQ_LEN)
                {
                    LE_DEBUG("Requested DTC message length is incorrect!");
                    errCode = INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
                    SendNRCResp(sid, addrPtr, errCode);

                    return;
                }

                result = GetFaultDetCounter();
                if(result != LE_OK)
                {
                    LE_ERROR("Error while getting data for fault detection counter!");
                    return;
                }
            }
            break;

            default:
            {
                LE_DEBUG("Requested DTC subfunction type is not supported");
                errCode = TAF_DIAG_SUBFUNCTION_NOT_SUPPORTED;
                SendNRCResp(sid, addrPtr, errCode);

                return;
            }
        }
    }
    else if (sid == reqClearDTCSvcId)
    {
        LE_DEBUG("ClearDiagnosticInformation request message received!");

        if (msgLen < CLEAR_DTC_INFO_REQ_MIN_LEN)
        {
            LE_DEBUG("Requested DTC message length is incorrect!");
            errCode = INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            SendNRCResp(sid, addrPtr, errCode);

            return;
        }

        uint32_t grpOfDTC = ((msgPtr[1] << 16) + (msgPtr[2] << 8) + (msgPtr[3]));
        LE_DEBUG("DTc clear request: %x", grpOfDTC);
        result = GetClearDTCResp(grpOfDTC);

        if(result == LE_UNSUPPORTED)
        {
            LE_DEBUG("requestOutOfRange!");
            errCode = REQ_OUT_OF_RANGE;
            SendNRCResp(sid, addrPtr, errCode);

            return;
        }
        else if(result == LE_IO_ERROR)
        {
            LE_DEBUG("requestOutOfRange!");
            errCode = GENERAL_PROGRAMMING_FAILURE;
            SendNRCResp(sid, addrPtr, errCode);

            return;
        }
    }
    else
    {
        LE_DEBUG("Service(0x%x) is invalid", sid);
        errCode = TAF_DIAG_SERVICE_NOT_SUPPORTED;  // ServiceNotSupported
        SendNRCResp(sid, addrPtr, errCode);

        return;
    }

    // Send response to the client for the requested service.
    result = SendDTCResp(sid, addrPtr, respBuf, respBufLen);
    if (result != LE_OK)
    {
        LE_ERROR("Send response error!");
        return;
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the msg for reportNumberOfDTCByStatusMask (0x01)
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCInf::GetNumOfDtcByStatusMask
(
    uint8_t statusMask
)
{
    LE_DEBUG("GetNumOfDtcByStatusMask");

    taf_DataAccess_NumOfDTC_t numOfDTC;
    le_result_t result;
    result = taf_DataAccess_GetNumOfDtcByStatusMask(statusMask, &numOfDTC);
    if (result != LE_OK)
    {
        LE_ERROR("Error while getting data from dataAccess module!");
        return result;
    }

    respBuf[0] = numOfDTC.availableMask;
    respBuf[1] = numOfDTC.formatIdentifier;
    respBuf[2] = (numOfDTC.dtcCnt & 0xff00) >> 8;
    respBuf[3] = numOfDTC.dtcCnt & 0xff;

    respBufLen = NO_OF_DTC_BY_STATUS_MASK_RESP_BASE_LEN;
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the msg for reportDTCByStatusMask (0x02)
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCInf::GetDtcByStatusMask
(
    uint8_t statusMask
)
{
    LE_DEBUG("GetDtcByStatusMask");

    taf_DataAccess_DTCStatusRec_t dtcStatusRec;

    le_result_t result;
    result = taf_DataAccess_GetDtcByStatusMask(statusMask, &dtcStatusRec);
    if (result != LE_OK)
    {
        LE_ERROR("Error while getting data from dataAccess module!");
        return result;
    }

    respBuf[0] = dtcStatusRec.availableMask;
    respBufLen = DTC_BY_STATUS_MASK_RESP_BASE_LEN;

    int i = 0;

    if(le_dls_NumLinks(&dtcStatusRec.dtcStatusRecList) > 0)
    {
        le_dls_Link_t* linkPtr = NULL;
        linkPtr = le_dls_Pop(&dtcStatusRec.dtcStatusRecList);

        while(linkPtr != NULL)
        {
            taf_DataAccess_DTCStatus_t * dtcStatusPtr = CONTAINER_OF(linkPtr,
                    taf_DataAccess_DTCStatus_t, link);

            if (dtcStatusPtr != NULL)
            {
                respBuf[DTC_BY_STATUS_MASK_RESP_BASE_LEN + i*4]
                        = (dtcStatusPtr->dtc & 0xff0000) >> 16;
                respBuf[DTC_BY_STATUS_MASK_RESP_BASE_LEN + i*4 + 1]
                        = (dtcStatusPtr->dtc & 0xff00) >> 8;
                respBuf[DTC_BY_STATUS_MASK_RESP_BASE_LEN + i*4 + 2]
                        = (dtcStatusPtr->dtc & 0xff);
                respBuf[DTC_BY_STATUS_MASK_RESP_BASE_LEN + i*4 + 3]
                        = dtcStatusPtr->status;

                i++;

                // Release memory for dtcStatusPtr
                le_mem_Release((void*)dtcStatusPtr);
            }

            // Returns the link next to a specified link.
            linkPtr = le_dls_Pop(&dtcStatusRec.dtcStatusRecList);
        }
    }

    // Response msg length.
    respBufLen = respBufLen + (i*4);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the msg for reportDTCExtDataRecordByDTCNumber (0x06)
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCInf::GetExtDataRecordByDTCNum
(
    uint32_t dtcMaskRcd,
    uint8_t dtcExtDataRec
)
{
    LE_DEBUG("GetSupportedDtc");

    taf_DataAccess_ExtDataRec_t extDataRec;

    le_result_t result;
    result = taf_DataAccess_GetExtDataRecByDtc(dtcMaskRcd, dtcExtDataRec, &extDataRec);
    if (result != LE_OK)
    {
        LE_ERROR("Error while getting data from dataAccess module!");
        return result;
    }

    respBuf[0] = (extDataRec.dtc & 0xff0000) >> 16;
    respBuf[1] = (extDataRec.dtc & 0xff00) >> 8;
    respBuf[2] = (extDataRec.dtc & 0xff);
    respBuf[3] = extDataRec.status;
    respBufLen = DTC_EXT_DATA_REC_BY_DTC_NO_RESP_BASE_LEN;


    if(le_dls_NumLinks(&extDataRec.extDataList) > 0)
    {
        le_dls_Link_t* linkPtr = NULL;
        linkPtr = le_dls_Pop(&extDataRec.extDataList);

        while(linkPtr != NULL)
        {
            taf_DataAccess_ExtData_t * extDataPtr = CONTAINER_OF(linkPtr,
                    taf_DataAccess_ExtData_t, link);

            if (extDataPtr != NULL)
            {
                respBuf[respBufLen] = extDataPtr->recNumber;
                LE_DEBUG("DTCExtDataRecordNumber is %x",extDataPtr->recNumber);
                respBufLen = respBufLen + 1;

                memcpy(respBuf + respBufLen, extDataPtr->extData, extDataPtr->extDataSize);
                respBufLen = respBufLen + extDataPtr->extDataSize;

                LE_DEBUG("DTCExtDataRecord length is %d", extDataPtr->extDataSize);

                // Release memory for extDataPtr
                le_mem_Release((void*)extDataPtr);
            }

            // Returns the link next to a specified link
            linkPtr = le_dls_Pop(&extDataRec.extDataList);
        }
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the msg for reportSupportedDTC (0x0A)
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCInf::GetSupportedDtc
(
)
{
    LE_DEBUG("GetSupportedDtc");

    taf_DataAccess_DTCStatusRec_t dtcStatusRec;

    le_result_t result;
    result = taf_DataAccess_GetSupportedDtc(&dtcStatusRec);
    if (result != LE_OK)
    {
        LE_ERROR("Error while getting data from dataAccess module!");
        return result;
    }

    respBuf[0] = dtcStatusRec.availableMask;
    respBufLen = SUPPORTED_DTC_RESP_BASE_LEN;

    int i = 0;

    if(le_dls_NumLinks(&dtcStatusRec.dtcStatusRecList) > 0)
    {
        le_dls_Link_t* linkPtr = NULL;
        linkPtr = le_dls_Pop(&dtcStatusRec.dtcStatusRecList);

        while(linkPtr != NULL)
        {
            taf_DataAccess_DTCStatus_t * dtcStatusPtr = CONTAINER_OF(linkPtr,
                    taf_DataAccess_DTCStatus_t, link);

            if (dtcStatusPtr != NULL)
            {
                respBuf[SUPPORTED_DTC_RESP_BASE_LEN + i*4]
                        = (dtcStatusPtr->dtc & 0xff0000) >> 16;
                respBuf[SUPPORTED_DTC_RESP_BASE_LEN + i*4 + 1]
                        = (dtcStatusPtr->dtc & 0xff00) >> 8;
                respBuf[SUPPORTED_DTC_RESP_BASE_LEN + i*4 + 2]
                        = (dtcStatusPtr->dtc & 0xff);
                respBuf[SUPPORTED_DTC_RESP_BASE_LEN + i*4 + 3]
                        = dtcStatusPtr->status;

                i++;

                // Release memory for dtcPtr
                le_mem_Release((void*)dtcStatusPtr);
            }

            // Returns the link next to a specified link.
            linkPtr = le_dls_Pop(&dtcStatusRec.dtcStatusRecList);
        }
    }

    // Response msg length.
    respBufLen = respBufLen + (i*4);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the msg for reportDTCFaultDetectionCounter (0x14)
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCInf::GetFaultDetCounter
(
)
{
    LE_DEBUG("GetFaultDetCounter");

    le_dls_List_t FDCList = LE_DLS_LIST_INIT;

    // le_result_t result;
    auto &diagEvent = taf_EventSvr::GetInstance();

    diagEvent.ReportDTCFaultDetectionCounter(&FDCList);

    respBufLen = DTC_FAULT_DETECTION_COUNTER_RESP_BASE_LEN;

    int i = 0;

    if(le_dls_NumLinks(&FDCList) > 0)
    {
        le_dls_Link_t* linkPtr = NULL;

        linkPtr = le_dls_Pop(&FDCList);
        while (linkPtr)
        {
            taf_DiagEvent_FDCInfo_t* fdcInfoPtr = CONTAINER_OF(linkPtr, taf_DiagEvent_FDCInfo_t,
                    link);

            if (fdcInfoPtr != NULL)
            {
                respBuf[DTC_FAULT_DETECTION_COUNTER_RESP_BASE_LEN + i*4]
                        = (fdcInfoPtr->dtc & 0xff0000) >> 16;
                respBuf[DTC_FAULT_DETECTION_COUNTER_RESP_BASE_LEN + i*4 + 1]
                        = (fdcInfoPtr->dtc & 0xff00) >> 8;
                respBuf[DTC_FAULT_DETECTION_COUNTER_RESP_BASE_LEN + i*4 + 2]
                        = (fdcInfoPtr->dtc & 0xff);
                respBuf[DTC_FAULT_DETECTION_COUNTER_RESP_BASE_LEN + i*4 + 3]
                        = fdcInfoPtr->fdc;

                i++;

                le_mem_Release(fdcInfoPtr);
            }

            // Removes and returns the link at the head of the list.
            linkPtr = le_dls_Pop(&FDCList);
        }
    }

    // Response msg length.
    respBufLen = DTC_FAULT_DETECTION_COUNTER_RESP_BASE_LEN + (i*4);

    return LE_OK;
}

le_result_t taf_DTCInf::GetClearDTCResp
(
    uint32_t grpOfDTC
)
{
    LE_DEBUG("GetClearDTCResp");

    le_result_t result;
    auto &diagEvent = taf_EventSvr::GetInstance();

    result = diagEvent.ClearDtc(grpOfDTC);


    if (result == LE_UNSUPPORTED)    // Check for NRC 0x31
    {
        LE_DEBUG("Send NRC 0x31");
        return result;
    }
    else if(result == LE_IO_ERROR)     // Check for NRC 0x72
    {
        LE_DEBUG("Send NRC 0x72");
        return result;
    }
    else if (result == LE_OK)
    {
        LE_DEBUG("Send Positive response!");
        respBufLen = CLEAR_DTC_INFO_RESP_LEN;
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send DTC service response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCInf::SendDTCResp
(
    uint8_t sid,
    const taf_uds_AddrInfo_t* addrInfoPtr,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    LE_DEBUG("SendDTCResp");

    le_result_t ret;

    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();

    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = addrInfoPtr->ta;
    addrInfo.ta = addrInfoPtr->sa;
    addrInfo.taType = addrInfoPtr->taType;

    // Positive response.
    if (dataPtr == NULL || dataSize == 0)
    {
        ret = backend.RespDiagPositive(sid, &addrInfo);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send positive response.(%d)", ret);
            return ret;
        }
    }
    else
    {
        ret = backend.RespDiagPositive(sid, &addrInfo, dataPtr, dataSize);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send positive response.(%d)", ret);
            return ret;
        }
    }

    // clear response buffer and length
    memset(respBuf, 0, RESPONSE_DATA_SIZE);
    respBufLen = 0;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send NRC response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCInf::SendNRCResp
(
    uint8_t sid,
    const taf_uds_AddrInfo_t*  addrInfoPtr,
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
 * UDS stack handler function
 */
//-------------------------------------------------------------------------------------------------
void taf_DTCInf::Init
(
)
{
    LE_INFO("DTC service Init!");

    auto& backend = taf_DiagBackend::GetInstance();

    // Register for ReadDTCInformation service (0x19)
    backend.RegisterUdsService(reqReadDTCSvcId, this);

    // Register for ClearDiagnosticInformation service (0x14)
    backend.RegisterUdsService(reqClearDTCSvcId, this);

    LE_INFO("Diag DTC Service started!");
}