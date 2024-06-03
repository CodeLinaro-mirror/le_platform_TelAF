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
#include "tafDIDBackendSvr.hpp"
#include "tafDiagStackInf.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance.
 */
//--------------------------------------------------------------------------------------------------
taf_DIDBackend &taf_DIDBackend::GetInstance()
{
    static taf_DIDBackend instance;

    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_DIDBackend::Init(void)
{
    LE_INFO("DIDBackend Init!");

    auto &didBackend = taf_DIDBackend::GetInstance();

    // Create memory pools.
    RespDIDMsgPool = le_mem_CreatePool("RespDIDMsgPool", sizeof(taf_ReadDIDRespMsg_t));

    // Create Internal event and register the handler function.
    didBackend.ReadDIDReqEvtId = le_event_CreateIdWithRefCounting("ReadDIDRequestEvent");
    le_event_AddHandler("Request DID Event Handler", didBackend.ReadDIDReqEvtId,
            ReadDIDReqEvtHandler);

    // Create the event ID for telaf diag service
    ReadDIDEvent = le_event_CreateIdWithRefCounting("ReadDIDEvent");
}

//--------------------------------------------------------------------------------------------------
/**
 * ReadDID request handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_DIDBackend::ReadDIDReqEvtHandler
(
    void* reqPtr
)
{
    LE_INFO("ReadDIDReqEvtHandler!");
    auto &didBackend = taf_DIDBackend::GetInstance();

    taf_ReadDIDReqMsg_t* readDIDMsgPtr = (taf_ReadDIDReqMsg_t*)reqPtr;
    TAF_ERROR_IF_RET_NIL(readDIDMsgPtr == NULL, "readDIDMsgPtr is Null");

    // Report readDID handler to telaf DID service
    le_event_ReportWithRefCounting(didBackend.ReadDIDEvent, readDIDMsgPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a handler for a given ReadDID service.
 */
//-------------------------------------------------------------------------------------------------
taf_diagDIDBackend_ReadDIDHandlerRef_t taf_DIDBackend::AddReadDIDHandler
(
    taf_diagDIDBackend_ReadDIDHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_INFO("AddReadDIDHandler!");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");

    // Register handler for ReadDID request.
    ReadDIDEventHandlerRef = le_event_AddLayeredHandler("ReadDIDEventHandlerRef", ReadDIDEvent,
            ReadDIDHandler, (void *)handlerPtr);

    le_event_SetContextPtr(ReadDIDEventHandlerRef, contextPtr);

    return (taf_diagDIDBackend_ReadDIDHandlerRef_t)(ReadDIDEventHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_DIDBackend::RemoveReadDIDHandler
(
    taf_diagDIDBackend_ReadDIDHandlerRef_t handlerRef
)
{
    LE_INFO("RemoveReadDIDHandler!");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//-------------------------------------------------------------------------------------------------
/**
 * ReadDID event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_DIDBackend::ReadDIDHandler
(
    void* reportPtr,
    void* subHandlerFunc
)
{
    LE_INFO("ReadDIDHandler!");

    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_ReadDIDReqMsg_t* readDIDMsgPtr = (taf_ReadDIDReqMsg_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(readDIDMsgPtr == NULL, "readDIDMsgPtr is Null");
    TAF_ERROR_IF_RET_NIL(subHandlerFunc == NULL, "Null ptr(subHandlerFunc)");

    taf_diagDIDBackend_ReadDIDHandlerFunc_t handlerFunc
            = (taf_diagDIDBackend_ReadDIDHandlerFunc_t)subHandlerFunc;
    handlerFunc(readDIDMsgPtr->readDIDRef, readDIDMsgPtr->readDID, le_event_GetContextPtr());

    LE_DEBUG("ReadDIDHandler completed");

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send readDID response.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DIDBackend::SendReadDIDResp
(
    taf_diagDIDBackend_ReadDIDRef_t readDIDRef,
    taf_diagDIDBackend_ReadDIDErrorCode_t errCode,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    LE_INFO("SendReadDIDResp!");

    TAF_ERROR_IF_RET_VAL(readDIDRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    auto &diagStack = taf_DiagStack::GetInstance();

    taf_ReadDIDRespMsg_t *readDIDRespMsgPtr = NULL;
    readDIDRespMsgPtr = (taf_ReadDIDRespMsg_t*)le_mem_ForceAlloc(RespDIDMsgPool);

    readDIDRespMsgPtr->readDIDRef = readDIDRef;
    readDIDRespMsgPtr->errCode = errCode;
    memcpy(readDIDRespMsgPtr->dataRec, dataPtr, dataSize);
    readDIDRespMsgPtr->dataRecLen = dataSize;

    // Report readDID response event.
    le_event_ReportWithRefCounting(diagStack.ReadDIDRespEvtId, readDIDRespMsgPtr);

    LE_DEBUG("SendReadDIDResp reported");

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a handler for a given WriteDID service.
 */
//-------------------------------------------------------------------------------------------------

taf_diagDIDBackend_WriteDIDHandlerRef_t taf_DIDBackend::AddWriteDIDHandler
(
    taf_diagDIDBackend_WriteDIDHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_INFO("AddWriteDIDHandler not implemented!");
    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove a handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_DIDBackend::RemoveWriteDIDHandler
(
    taf_diagDIDBackend_WriteDIDHandlerRef_t handlerRef
)
{
    LE_INFO("RemoveWriteDIDHandler not implemented!");
    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send WriteDID response.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DIDBackend::SendWriteDIDResp
(
    taf_diagDIDBackend_WriteDIDRef_t writeDIDRef,
    taf_diagDIDBackend_WriteDIDErrorCode_t errCode,
    uint16_t dataId
)
{
    LE_INFO("SendWriteDIDResp not implemented!");
    return LE_NOT_IMPLEMENTED;
}