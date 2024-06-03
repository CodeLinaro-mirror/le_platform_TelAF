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
#include "tafSecBackendSvr.hpp"
#include "tafDiagStackInf.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance.
 */
//--------------------------------------------------------------------------------------------------
taf_SecBackend &taf_SecBackend::GetInstance()
{
    static taf_SecBackend instance;

    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_SecBackend::Init(void)
{
    LE_INFO("SecurityBackend Init!");

    auto &SecBackend = taf_SecBackend::GetInstance();

    // Create memory pools for sessionCtrl response.
    RespSesMsgPool = le_mem_CreatePool("RespSesMsgPool", sizeof(taf_SesCtrlRespMsg_t));

    // Create Internal event and register the handler function for sessionCtrl request.
    SecBackend.SesCtrlReqEvtId = le_event_CreateIdWithRefCounting("SesCtrlRequestEvent");
    le_event_AddHandler("Request SesCtrl Event Handler", SecBackend.SesCtrlReqEvtId,
            SesCtrlReqEvtHandler);

    // Create Internal event and register the handler function for session change notification.
    SecBackend.SesChangeEvtId = le_event_CreateIdWithRefCounting("SesChangeEvent");
    le_event_AddHandler("Session change Event Handler", SecBackend.SesChangeEvtId,
            SesChangeEvtHandler);

    // Create the event ID for telaf diag service for session request msg
    DSCEvent = le_event_CreateIdWithRefCounting("DSCEvent");

    // Create the event ID for telaf diag service for session change notification
    DSCChangeEvent = le_event_CreateIdWithRefCounting("DSCChangeEvent");
}

//--------------------------------------------------------------------------------------------------
/**
 * SessionCtrl request handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_SecBackend::SesCtrlReqEvtHandler
(
    void* reqPtr
)
{
    LE_INFO("SesCtrlReqEvtHandler!");
    auto &SecBackend = taf_SecBackend::GetInstance();

    taf_SesCtrlReqMsg_t* sesCtrlMsgPtr = (taf_SesCtrlReqMsg_t*)reqPtr;
    TAF_ERROR_IF_RET_NIL(sesCtrlMsgPtr == NULL, "sesCtrlMsgPtr is Null");
    LE_DEBUG("SesCtrlReqEvtHandler sesCtrlMsgPtr address: %p", reqPtr);

    // Report sessionCtrl handler to telaf Diag service
    le_event_ReportWithRefCounting(SecBackend.DSCEvent, sesCtrlMsgPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a handler for a given sessionCtrl service.
 */
//-------------------------------------------------------------------------------------------------
taf_diagSecBackend_SesTypeCheckHandlerRef_t taf_SecBackend::AddSesTypeCheckHandler
(
    taf_diagSecBackend_SesTypeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_INFO("AddSesTypeCheckHandler!");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");

    // Register handler for sessionCtrl request.
    DSCEventHandlerRef = le_event_AddLayeredHandler("DSCEventHandlerRef", DSCEvent,
            SesCtrlEventHandler, (void *)handlerPtr);

    le_event_SetContextPtr(DSCEventHandlerRef, contextPtr);

    return (taf_diagSecBackend_SesTypeCheckHandlerRef_t)(DSCEventHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_SecBackend::RemoveSesTypeCheckHandler
(
    taf_diagSecBackend_SesTypeCheckHandlerRef_t handlerRef
)
{
    LE_INFO("RemoveSesTypeCheckHandler!");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");
	le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//-------------------------------------------------------------------------------------------------
/**
 * SessionCtrl event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_SecBackend::SesCtrlEventHandler
(
    void* reportPtr,
    void* subHandlerFunc
)
{
    LE_INFO("SesCtrlEventHandler!");

    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");
    LE_INFO("SesCtrlEventHandler sesCtrlMsgPtr address: %p", reportPtr);

    taf_SesCtrlReqMsg_t* sesCtrlMsgPtr = (taf_SesCtrlReqMsg_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(sesCtrlMsgPtr == NULL, "sesCtrlMsgPtr is Null");
    TAF_ERROR_IF_RET_NIL(subHandlerFunc == NULL, "Null ptr(subHandlerFunc)");

    taf_diagSecBackend_SesTypeHandlerFunc_t handlerFunc
            = (taf_diagSecBackend_SesTypeHandlerFunc_t)subHandlerFunc;
    handlerFunc(sesCtrlMsgPtr->sesTypeRef, sesCtrlMsgPtr->sesType, le_event_GetContextPtr());

    LE_DEBUG("SesCtrlEventHandler completed");

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send sessionCtrl response.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_SecBackend::SendSesTypeCheckResp
(
    taf_diagSecBackend_SesTypeCheckRef_t sesTypeRef,
    taf_diagSecBackend_SesControlErrorCode_t errCode
)
{
    LE_INFO("SendSesTypeCheckResp!");

    TAF_ERROR_IF_RET_VAL(sesTypeRef == NULL, LE_BAD_PARAMETER, "Invalid msgRef");

    auto &diagStack = taf_DiagStack::GetInstance();

    taf_SesCtrlRespMsg_t *sesCtrlRespMsgPtr = NULL;
    sesCtrlRespMsgPtr = (taf_SesCtrlRespMsg_t*)le_mem_ForceAlloc(RespSesMsgPool);

    sesCtrlRespMsgPtr->sesTypeRef = sesTypeRef;
    sesCtrlRespMsgPtr->errCode = errCode;

    // Report sessionCtrl response event.
    le_event_ReportWithRefCounting(diagStack.SesCtrlRespEvtId, sesCtrlRespMsgPtr);

    LE_DEBUG("sessionCtrl response reported");

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Session change notification handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_SecBackend::SesChangeEvtHandler
(
    void* reqPtr
)
{
    LE_INFO("SesChangeEvtHandler!");
    auto &SecBackend = taf_SecBackend::GetInstance();

    taf_SesCtrlChangeMsg_t* sesChangeMsgPtr = (taf_SesCtrlChangeMsg_t*)reqPtr;
    TAF_ERROR_IF_RET_NIL(sesChangeMsgPtr == NULL, "sesChangeMsgPtr is Null");

    // Report session change handler to telaf Diag service
    le_event_ReportWithRefCounting(SecBackend.DSCChangeEvent, sesChangeMsgPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a handler for a session change notification.
 */
//-------------------------------------------------------------------------------------------------
taf_diagSecBackend_SesChangeHandlerRef_t taf_SecBackend::AddSesChangeHandler
(
    taf_diagSecBackend_SesChangeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_INFO("AddSesChangeHandler!");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");

    // Register handler for session change notification.
    DSCChangeEventHandlerRef = le_event_AddLayeredHandler("DSCChangeEventHandlerRef",
            DSCChangeEvent, SesTypeChangeHandler, (void *)handlerPtr);

    le_event_SetContextPtr(DSCChangeEventHandlerRef, contextPtr);

    return (taf_diagSecBackend_SesChangeHandlerRef_t)(DSCChangeEventHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_SecBackend::RemoveSesChangeHandler
(
    taf_diagSecBackend_SesChangeHandlerRef_t handlerRef
)
{
    LE_INFO("RemoveSesChangeHandler!");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");
	le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//-------------------------------------------------------------------------------------------------
/**
 * Session change event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_SecBackend::SesTypeChangeHandler
(
    void* reportPtr,
    void* subHandlerFunc
)
{
    LE_INFO("SesTypeChangeHandler!");

    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");
    LE_DEBUG("SesTypeChangeHandler reportPtr address: %p", reportPtr);

    taf_SesCtrlChangeMsg_t* sesChangeMsgPtr = (taf_SesCtrlChangeMsg_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(sesChangeMsgPtr == NULL, "sesChangeMsgPtr is Null");
    TAF_ERROR_IF_RET_NIL(subHandlerFunc == NULL, "Null ptr(subHandlerFunc)");

    taf_diagSecBackend_SesChangeHandlerFunc_t handlerFunc
            = (taf_diagSecBackend_SesChangeHandlerFunc_t)subHandlerFunc;
    handlerFunc(sesChangeMsgPtr->prev_session, sesChangeMsgPtr->current_session,
            le_event_GetContextPtr());

    // Release the memory.
    le_mem_Release(sesChangeMsgPtr);

    LE_DEBUG("SesTypeChangeHandler completed");

    return;
}