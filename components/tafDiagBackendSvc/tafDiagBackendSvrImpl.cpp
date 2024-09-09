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
#include "tafDiagBackendSvr.hpp"
#include "tafDiagStackInf.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance.
 */
//--------------------------------------------------------------------------------------------------
taf_DiagBackend &taf_DiagBackend::GetInstance()
{
    static taf_DiagBackend instance;

    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_DiagBackend::Init(void)
{
    LE_INFO("Diag Backend Init!");

    auto &diagBackend = taf_DiagBackend::GetInstance();

    // Create memory pools.
    RespMsgPool = le_mem_CreatePool("RespMsgPool", sizeof(taf_DiagRespMsg_t));

    // Create Internal event and register the handler function.
    diagBackend.DiagEventReqEvtId = le_event_CreateIdWithRefCounting("DiagEventRequestEvent");
    le_event_AddHandler("Request Diag Event Handler", diagBackend.DiagEventReqEvtId,
            DiagEventReqEvtHandler);

    // Create the event ID for telaf diag service
    DiagEvent = le_event_CreateIdWithRefCounting("DiagEvent");
}

//--------------------------------------------------------------------------------------------------
/**
 * DiagEvent request handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_DiagBackend::DiagEventReqEvtHandler
(
    void* reqPtr
)
{
    LE_INFO("DiagEventReqEvtHandler!");
    auto &diagBackend = taf_DiagBackend::GetInstance();

    taf_DiagReqMsg_t* diagEventMsgPtr = (taf_DiagReqMsg_t*)reqPtr;
    TAF_ERROR_IF_RET_NIL(diagEventMsgPtr == NULL, "diagEventMsgPtr is Null");

    // Report Diag handler to telaf service
    le_event_ReportWithRefCounting(diagBackend.DiagEvent, diagEventMsgPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a handler for a given service.
 */
//-------------------------------------------------------------------------------------------------
taf_diagBackend_DiagEventHandlerRef_t taf_DiagBackend::AddDiagEventHandler
(
    taf_diagBackend_DiagHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_INFO("AddDiagEventHandler!");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");

    // Register handler for given service request.
    DiagEventHandlerRef = le_event_AddLayeredHandler("DiagEventHandlerRef", DiagEvent,
            DiagEventHandler, (void *)handlerPtr);

    le_event_SetContextPtr(DiagEventHandlerRef, contextPtr);

    return (taf_diagBackend_DiagEventHandlerRef_t)(DiagEventHandlerRef);
}

//-------------------------------------------------------------------------------------------------
/**
 * Diag event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_DiagBackend::DiagEventHandler
(
    void* reportPtr,
    void* subHandlerFunc
)
{
    LE_INFO("Diag Event Handler!");

    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_DiagReqMsg_t* diagReqMsgPtr = (taf_DiagReqMsg_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(diagReqMsgPtr == NULL, "reportPtr is Null");
    TAF_ERROR_IF_RET_NIL(subHandlerFunc == NULL, "Null ptr(subHandlerFunc)");

    // Passing dummy value
    taf_diagBackend_AddrInfo_t addrInfo;
    addrInfo.sa = 0x0;
    addrInfo.ta = 0x0;
    addrInfo.taType = TAF_DIAGBACKEND_TA_TYPE_PHYSICAL;
    addrInfo.vlanId = 0;
    addrInfo.ifName[0] = '\0';

    taf_diagBackend_DiagHandlerFunc_t handlerFunc
            = (taf_diagBackend_DiagHandlerFunc_t)subHandlerFunc;
    handlerFunc(diagReqMsgPtr->ref, &addrInfo, diagReqMsgPtr->svcId,
            diagReqMsgPtr->data, diagReqMsgPtr->dataLen, le_event_GetContextPtr());

    LE_DEBUG("DiagEventHandler completed");

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_DiagBackend::RemoveDiagEventHandler
(
    taf_diagBackend_DiagEventHandlerRef_t handlerRef
)
{
    LE_INFO("RemoveDiagEventHandler!");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//-------------------------------------------------------------------------------------------------
/**
 * Send requested service response.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DiagBackend::SendResp
(
    taf_diagBackend_DiagInfRef_t ref,
    const taf_diagBackend_AddrInfo_t* addrInfoPtr,
    uint8_t serviceID,
    uint8_t errCode,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    LE_INFO("SendDiagResp!");

    TAF_ERROR_IF_RET_VAL(ref == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(addrInfoPtr == NULL, LE_BAD_PARAMETER, "Invalid addrPtr");

    auto &diagStack = taf_DiagStack::GetInstance();

    taf_DiagRespMsg_t *diagRespMsgPtr = NULL;
    diagRespMsgPtr = (taf_DiagRespMsg_t*)le_mem_ForceAlloc(RespMsgPool);

    LE_DEBUG("Recieved ref: %p",ref);
    LE_DEBUG("Error code: %d",errCode);
    LE_DEBUG("Received data size: %zu",dataSize);
    LE_DEBUG("Received svcID: %x",serviceID);

    for(int j=0; j < int(dataSize); j++){
        LE_DEBUG("data[%d]: %x",j,dataPtr[j]);
    }

    diagRespMsgPtr->ref = ref;
    diagRespMsgPtr->errCode = errCode;

    taf_diagBackend_ServiceId_t svcID = taf_diagBackend_ServiceId_t(serviceID);

    if(errCode == 0)
    {
        if(svcID == SID_READ_DATA_BY_IDENTIFIER)
        {
            if (dataSize <= DID_LEN)
            {
                LE_ERROR("Incorrect data size : %" PRIuS, dataSize);
                return LE_FAULT;
            }
            else
            {
                memcpy(diagRespMsgPtr->dataRec, &dataPtr[DID_LEN], dataSize-DID_LEN);
                diagRespMsgPtr->dataLen = dataSize-DID_LEN;
            }
        }
        else
        {
            memcpy(diagRespMsgPtr->dataRec, dataPtr, dataSize);
            diagRespMsgPtr->dataLen = dataSize;
        }
    }
    else
    {
        memcpy(diagRespMsgPtr->dataRec, dataPtr, dataSize);
        diagRespMsgPtr->dataLen = dataSize;
    }

    for(int k=0; k < int(diagRespMsgPtr->dataLen); k++){
        LE_DEBUG("diagRespMsgPtr->dataRec[%d]: %x",k,diagRespMsgPtr->dataRec[k]);
    }

    LE_DEBUG("Reporting for given service response");
    le_event_ReportWithRefCounting(diagStack.DiagEventRespEvtId, diagRespMsgPtr);

    LE_DEBUG("SendResp reported");

    return LE_OK;
}
