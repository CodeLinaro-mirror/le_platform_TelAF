/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <string>
#include "tafROESvr.hpp"
#include "configuration.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of TelAF ResponseOnEvent server.
 */
//--------------------------------------------------------------------------------------------------
taf_ROESvr &taf_ROESvr::GetInstance
(
)
{
    static taf_ROESvr instance;

    return instance;
}

//-------------------------------------------------------------------------------------------------
/**
 * UDS stack handler function
 */
//-------------------------------------------------------------------------------------------------
void taf_ROESvr::UDSMsgHandler
(
    const taf_uds_AddrInfo_t* addrPtr,
    uint8_t sid,
    uint8_t* msgPtr,
    size_t msgLen
)
{
    uint8_t errCode = 0;

    LE_DEBUG("ResponseOnEvent service UDSMsgHandler!");

    TAF_ERROR_IF_RET_NIL(addrPtr == NULL, "Invalid addrPtr");
    TAF_ERROR_IF_RET_NIL(msgPtr == NULL, "Invalid msgPtr");

    uint8_t msgPos = 1;  // Skip sid
    uint8_t subFunc = msgPtr[msgPos];
    uint8_t type = subFunc & ROE_EVENT_MASK;
    if (type != STOP_RESPONSE_ON_EVENT &&
        type != ON_DTC_STATUS_CHANGE &&
        type != START_RESPONSE_ON_EVENT)
    {
        LE_WARN("Sunfunction(0x%x) is invalid, send NRC %x",
                subFunc, TAF_DIAG_SUBFUNCTION_NOT_SUPPORTED);
        errCode = TAF_DIAG_SUBFUNCTION_NOT_SUPPORTED; // SubfunctionNotSupported
        SendNRCResp(sid, addrPtr, errCode);
        return;
    }

    taf_ROERxMsg_t *rxMsgPtr = NULL;
    rxMsgPtr = (taf_ROERxMsg_t*)le_mem_ForceAlloc(RxMsgPool);
    memset(rxMsgPtr, 0, sizeof(taf_ROERxMsg_t));

    msgPos++;
    memcpy(&rxMsgPtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
    rxMsgPtr->subFunc = subFunc;
    rxMsgPtr->evWinTime = msgPtr[msgPos];
    msgPos++;
    rxMsgPtr->link = LE_DLS_LINK_INIT;
    rxMsgPtr->rxMsgRef = le_ref_CreateRef(RxMsgRefMap, rxMsgPtr);

    switch (type)
    {
        case ON_DTC_STATUS_CHANGE:
        {
            if (msgLen < MIN_ROE_ON_DTC_STATUS_CHANGE)
            {
                LE_WARN("Invalid length %" PRIuS " for subFunc(0x%x), send NRC %x",
                        msgLen, subFunc, TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
                le_ref_DeleteRef(RxMsgRefMap, rxMsgPtr->rxMsgRef);
                le_mem_Release(rxMsgPtr);
                errCode = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
                SendNRCResp(sid, addrPtr, errCode);
            }
            rxMsgPtr->evTypeRec[0] = msgPtr[msgPos];
            rxMsgPtr->evTypeRecLen = 1;
            msgPos++;

            memcpy(rxMsgPtr->respToRec, msgPtr + msgPos, msgLen - msgPos);
            rxMsgPtr->respToRecLen = msgLen - msgPos;
            break;
        }
        default:
            break;
    }

    // Report the ROE request message to message handler in service layer.
    le_event_ReportWithRefCounting(ReqEvent, rxMsgPtr);
}

//-------------------------------------------------------------------------------------------------
/**
 * Send response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_ROESvr::SendResp
(
    taf_ROERxMsg_t* rxMsgPtr,
    uint8_t errCode
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgPtr == NULL, LE_BAD_PARAMETER, "Invalid addrInfoPtr");

    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = rxMsgPtr->addrInfo.ta;
    addrInfo.ta = rxMsgPtr->addrInfo.sa;
    addrInfo.taType = rxMsgPtr->addrInfo.taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = rxMsgPtr->addrInfo.vlanId;
    le_utf8_Copy(addrInfo.ifName, rxMsgPtr->addrInfo.ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif

    le_result_t ret = LE_OK;
    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();

    if (errCode == 0)
    {
        // Positive response.
        ret = backend.RespDiagPositive(reqSvcId, &addrInfo, rxMsgPtr->resp, rxMsgPtr->respLen);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send ROE positive response.(%d)", ret);
            return ret;
        }
    }
    else
    {
        // Negative response.
        ret = backend.RespDiagNegative(reqSvcId, &addrInfo, errCode);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send ROE negative response.(%d)", ret);
        }
    }

    // Remove the message from service message list.
    le_dls_Remove(&rxMsgList, &(rxMsgPtr->link));

    le_ref_DeleteRef(RxMsgRefMap, rxMsgPtr->rxMsgRef);
    le_mem_Release(rxMsgPtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send NRC response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
void taf_ROESvr::SendNRCResp
(
    uint8_t sid,
    const taf_uds_AddrInfo_t* addrInfoPtr,
    uint8_t errCode
)
{
    TAF_ERROR_IF_RET_NIL(addrInfoPtr == NULL, "Invalid addrInfoPtr");

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
}

//-------------------------------------------------------------------------------------------------
/**
 * ResponseOnEvent event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_ROESvr::RxROEEventHandler
(
    void* reportPtr
)
{
    LE_DEBUG("RxROEEventHandler!");
    auto &ROEIns = taf_ROESvr::GetInstance();

    taf_ROERxMsg_t *rxMsgPtr = (taf_ROERxMsg_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(rxMsgPtr == NULL, "rxMsgPtr is Null");

    rxMsgPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&ROEIns.rxMsgList, &rxMsgPtr->link);

    // Nothing to be done currently.
    if ((rxMsgPtr->subFunc & ROE_EVENT_MASK) == REPORT_ACTIVATED_EVENTS)
    {
        rxMsgPtr->resp[0] = 0;    // numberOfActivatedEvents.
        rxMsgPtr->respLen = 1;
    }
    else
    {
        rxMsgPtr->resp[0] = 0;    // numberOfIdentifiedEvents.
        rxMsgPtr->resp[1] = rxMsgPtr->evWinTime;  // eventWindowTime.
        rxMsgPtr->respLen = 2;

        // Echo the eventTypeRecord and serviceToRespondToRecord.
        memcpy(rxMsgPtr->resp + rxMsgPtr->respLen, rxMsgPtr->evTypeRec, rxMsgPtr->evTypeRecLen);
        rxMsgPtr->respLen += rxMsgPtr->evTypeRecLen;
        memcpy(rxMsgPtr->resp + rxMsgPtr->respLen, rxMsgPtr->respToRec, rxMsgPtr->respToRecLen);
        rxMsgPtr->respLen += rxMsgPtr->respToRecLen;
    }

    le_result_t ret = ROEIns.SendResp(rxMsgPtr, 0);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to send ROE response.(%d)", ret);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_ROESvr::Init
(
    void
)
{
    // Create memory pools.
    RxMsgPool = le_mem_CreatePool("ROERxMsgPool", sizeof(taf_ROERxMsg_t));

    // Create reference maps
    RxMsgRefMap = le_ref_CreateMap("ROERxMsgRefMap", DEFAULT_RX_MSG_REF_CNT);

    // Create the event and add event handler for ResponseOnEvent (0x86).
    ReqEvent = le_event_CreateIdWithRefCounting("ROEEvent");
    ReqEventHandlerRef = le_event_AddHandler("ROEEventHandlerRef", ReqEvent,
            taf_ROESvr::RxROEEventHandler);

    auto& backend = taf_DiagBackend::GetInstance();
    backend.RegisterUdsService(reqSvcId, this);

    rxMsgList  = LE_DLS_LIST_INIT;
    LE_DEBUG("taf_ROESvr Init completed!");
}
