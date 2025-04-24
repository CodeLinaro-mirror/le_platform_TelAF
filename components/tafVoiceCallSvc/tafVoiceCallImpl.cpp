/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include "tafVoiceCall.hpp"
#include <unistd.h>
#include "taf_pa_voicecall.h"

using namespace std;
using namespace tafsvc;

#define VoiceCallInfoConfFile "/tmp/.VoiceCallInfo"

LE_MEM_DEFINE_STATIC_POOL(tafCall,MAX_TAFCALL_OBJ,sizeof(taf_VoiceCtrl_t));
LE_MEM_DEFINE_STATIC_POOL(tafCallRef,MAX_TAFCALL_OBJ,sizeof(taf_CallRefNode_t));
LE_MEM_DEFINE_STATIC_POOL(tafSessionCtx,MAX_TAFCALL_SESSION,sizeof(taf_SessionCtx_t));
LE_MEM_DEFINE_STATIC_POOL(tafSessionRef,MAX_TAFCALL_SESSION,sizeof(taf_SessionRef_t));
LE_MEM_DEFINE_STATIC_POOL(tafHandler,MAX_TAFCALL_SESSION,sizeof(taf_HandlerCtx_t));

//taf_VoiceCall* taf_Handler::TafCallPtr = nullptr;
bool  VoiceCallSvc::isEnableDebug = false;

// session close handler
void Handler::CloseSessHandler(le_msg_SessionRef_t sessionRef, void* ctxPtr)
{
    TAF_ERROR_IF_RET_NIL(!sessionRef, "sessionRef is NULL");

    auto &myCall = VoiceCallSvc::GetInstance();
    myCall.ReleaseSession(sessionRef, ctxPtr);
}

// ctrlCtrl memory free handler
void Handler::ReleaseCallCtrlHandler(void* objPtr)
{
    auto &myCall = VoiceCallSvc::GetInstance();
    myCall.DestructorCallCtx(objPtr);
}

// First layer state/event handler
void Handler::ProcessStateChanged(void *reportPtr)
{
    CallEvent_t *eventVoicePtr = (CallEvent_t *)reportPtr;

    auto &myCall = VoiceCallSvc::GetInstance();
    myCall.CallHandler(eventVoicePtr);
}

// State changed listener to platform adapter
void Handler::PaEventListener(taf_pa_voicecall_Ref_t reference, taf_pa_voicecall_event_t event, void *contextPtr)
{
    auto &myCall = VoiceCallSvc::GetInstance();
    CallEvent_t msgCallEvent;

    int8_t phoneId = taf_pa_voicecall_GetCallPhoneId(reference);
    taf_pa_voicecall_dir_t direction = taf_pa_voicecall_GetCallDirection(reference);
    char destinationPtr[PA_MAX_DESTINATION_LEN_BYTE];
    TAF_ERROR_IF_RET_NIL(taf_pa_voicecall_GetCallDestination(reference, destinationPtr, PA_MAX_DESTINATION_LEN_BYTE) != LE_OK, "Cannot get dest ID");
    if (event == TAF_PA_VOICECALL_EVENT_ENDED)
    {
        taf_pa_voicecall_termination_t termination;
        if (taf_pa_voicecall_GetCallTermination(reference, &termination) == LE_OK)
        {
            msgCallEvent.termination = myCall.EndCauseConvert(termination);
        }
        else
        {
            LE_ERROR("Cannot get termination code from: %p!", reference);
        }
    }

    taf_pa_voicecall_DeleteReference(reference);

    LE_INFO("PA event phone %d, dest %s, event %s", phoneId, destinationPtr, myCall.PaEventToString(event));

    // To fix the corner case, iCall is released later when testing with telsdk app,
    // callRef is used for the event report.
    taf_VoiceCtrl_t* callCtxPtr = myCall.GetCallCtx(phoneId, destinationPtr, myCall.DirConvert(direction));
    if ((callCtxPtr == NULL) && (event != TAF_PA_VOICECALL_EVENT_INCOMING) && (event != TAF_PA_VOICECALL_EVENT_WAITING))
    {
        LE_ERROR("Cannot get ctx from phone %d and dest: %s, event: %s", phoneId, destinationPtr, myCall.PaEventToString(event));
        return;
    }
    else
    {
        LE_DEBUG("Incoming or waiting call Id: %d, CtxPtr: %p, event: %s", phoneId, callCtxPtr, myCall.PaEventToString(event));
    }

    if (callCtxPtr != NULL)
    {
        msgCallEvent.callRef = callCtxPtr->callRef;
    }
    else
    {
        msgCallEvent.callRef = nullptr;
    }
    le_utf8_Copy(msgCallEvent.dest, destinationPtr, MAX_DESTINATION_LEN, NULL);
    msgCallEvent.phoneId = phoneId;
    msgCallEvent.event = myCall.EventConvert(event);
    msgCallEvent.direction = myCall.DirConvert(direction);
    le_event_Report(myCall.CallEvent, &msgCallEvent, sizeof(CallEvent_t));
    return;
}


void VoiceCallSvc::ShowAll()
{
    uint32_t i = 0, j = 0, k = 0;

    if (isEnableDebug == false)
    {
        return;
    }

    LE_DEBUG("========================== voice call show all start ==========================");
    LE_DEBUG("SessionCtx list num: %" PRIuS, le_dls_NumLinks(&SessionCtxList));
    le_dls_Link_t* linkPtr = le_dls_Peek(&SessionCtxList);
    while (linkPtr)
    {
        taf_SessionCtx_t* sessionCtxTmpPtr = CONTAINER_OF(linkPtr, taf_SessionCtx_t, link);
        linkPtr = le_dls_PeekNext(&SessionCtxList, linkPtr);

        LE_DEBUG("   [%d]sessionCtx:%p Ref: %p Handler Num: %" PRIuS ", CallRef Num: %" PRIuS,
            i++, sessionCtxTmpPtr, sessionCtxTmpPtr->sessionRef,
            le_dls_NumLinks(&sessionCtxTmpPtr->handlerList), le_dls_NumLinks(&sessionCtxTmpPtr->callRefList));

        le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&sessionCtxTmpPtr->handlerList);
        while (linkHandlerPtr)
        {
            taf_HandlerCtx_t * handlerCtxPtr = CONTAINER_OF(linkHandlerPtr, taf_HandlerCtx_t, link);
            linkHandlerPtr = le_dls_PeekPrev(&sessionCtxTmpPtr->handlerList, linkHandlerPtr);

            LE_DEBUG("       [%d]handler ptr: %p, ref: %p", j++, handlerCtxPtr->handlerPtr, handlerCtxPtr->handlerRef);
        }

        le_dls_Link_t* linkCallRef = le_dls_PeekTail(&sessionCtxTmpPtr->callRefList);
        while (linkCallRef)
        {
            taf_CallRefNode_t * callRefPtr = CONTAINER_OF(linkCallRef, taf_CallRefNode_t, link);
            linkCallRef = le_dls_PeekPrev(&sessionCtxTmpPtr->callRefList, linkCallRef);

            LE_DEBUG("       [%d]callRef: %p", k++, callRefPtr->callRef);
        }
    }

    i = 0, j = 0, k = 0;
    LE_DEBUG("CallCtx Num: %" PRIuS, le_dls_NumLinks(&CallCtrlList));
    linkPtr = le_dls_Peek(&CallCtrlList);
    while ( linkPtr )
    {
        taf_VoiceCtrl_t* callCtxPtr = CONTAINER_OF( linkPtr, taf_VoiceCtrl_t, link);
        linkPtr = le_dls_PeekNext(&CallCtrlList, linkPtr);
        LE_DEBUG("   [%d]ID: %d, destId: %s, callRef: %p, event: %s, termination: %s",
            i++, callCtxPtr->phoneId, callCtxPtr->destId, callCtxPtr->callRef,
            EventToString(callCtxPtr->event), TerminationToString(callCtxPtr->termination));

        le_dls_Link_t* linkSessionRefPtr = le_dls_Peek(&(callCtxPtr->sessionRefList));
        while (linkSessionRefPtr)
        {
            taf_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkSessionRefPtr, taf_SessionRef_t, link);
            linkSessionRefPtr = le_dls_PeekNext(&(callCtxPtr->sessionRefList), linkSessionRefPtr);
            LE_DEBUG("       [%d]sessionRef %p", j++, sessionRefPtr->sessionRef);
        }
    }

    LE_DEBUG("========================== voice call show  all  end ==========================");
}

const char * VoiceCallSvc::EventToString(taf_voicecall_Event_t event)
{
    const char *retPtr = "null";

    switch (event)
    {
        case TAF_VOICECALL_EVENT_ACTIVE:
            retPtr = "active";
        break;

        case TAF_VOICECALL_EVENT_ONHOLD:
            retPtr = "onhold";
        break;

        case TAF_VOICECALL_EVENT_DIALING:
            retPtr = "dialing";
        break;

        case TAF_VOICECALL_EVENT_INCOMING:
            retPtr = "incoming";
        break;

        case TAF_VOICECALL_EVENT_WAITING:
            retPtr = "waiting";
        break;

        case TAF_VOICECALL_EVENT_ALERTING:
            retPtr = "alerting";
        break;

        case TAF_VOICECALL_EVENT_ENDED:
            retPtr = "ended";
        break;

        default:
        break;
    }

    return retPtr;
}

const char * VoiceCallSvc::TerminationToString(taf_voicecall_CallEndCause_t termination)
{
    const char *termPtr = "undefined";

    switch (termination)
    {
    case TAF_VOICECALL_END_NORMAL:
        termPtr = "normal";
    break;

    case TAF_VOICECALL_END_NETWORK_FAIL:
        termPtr = "network_fail";
    break;

    case TAF_VOICECALL_END_UNOBTAINABLE_NUMBER:
        termPtr = "unobtainable_number";
    break;

    case TAF_VOICECALL_END_BUSY:
        termPtr = "busy";
    break;

    case TAF_VOICECALL_END_LOCAL:
        termPtr = "local";
    break;

    case TAF_VOICECALL_END_REMOTE:
        termPtr = "remote";
    break;

    case TAF_VOICECALL_END_UNDEFINED:
        termPtr = "undefined";
    break;

    case TAF_VOICECALL_END_REJECTED:
        termPtr = "rejected";
    break;

    case TAF_VOICECALL_END_NORESPONSE:
        termPtr = "noresponse";
    break;

    default:
        termPtr = "undefined";
    break;
    }

    return termPtr;
}

le_result_t VoiceCallSvc::SendCallEventToClient(taf_VoiceCtrl_t *callCtxPtr)
{
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_BAD_PARAMETER, "callCtxPtr is null");
    bool isIncomingCall = false;
    if ((callCtxPtr->event == TAF_VOICECALL_EVENT_INCOMING) || (callCtxPtr->event == TAF_VOICECALL_EVENT_WAITING))
    {
        isIncomingCall = true;
    }

    // for incoming call, boardcast its events to all sessions if not session is link to this callCtx
    if ((isIncomingCall == true) && (le_dls_NumLinks(&callCtxPtr->sessionRefList) == 0))
    {
        LE_DEBUG("link sessionRef to callCtx for incoming call");
        le_dls_Link_t* linkPtr = le_dls_Peek(&SessionCtxList);
        while (linkPtr)
        {
            taf_SessionCtx_t* sessionCtxTmpPtr = CONTAINER_OF(linkPtr, taf_SessionCtx_t, link);
            linkPtr = le_dls_PeekNext(&SessionCtxList, linkPtr);

            taf_SessionRef_t* sessionRefNodePtr = GetSessionRefNodeFromCallCtx(callCtxPtr, sessionCtxTmpPtr->sessionRef);
            size_t numLinks = le_dls_NumLinks(&sessionCtxTmpPtr->handlerList);
            if ((sessionRefNodePtr == NULL) && (numLinks > 0))
            {
                LE_DEBUG("Set sessionRef(%p) to callCtx for incoming call", sessionCtxTmpPtr->sessionRef);
                le_result_t leRet = SetSessionRefToCallCtx(callCtxPtr, sessionCtxTmpPtr->sessionRef);
                TAF_ERROR_IF_RET_VAL(leRet != LE_OK, LE_NOT_IMPLEMENTED, "Cannot set sessionRef to callCtx");
                SetCallRef(callCtxPtr);
            }
            else
            {
                LE_WARN("sessionRefNode already exist or session handler(%" PRIuS ") is nout bound", numLinks);
            }
        }
    }

    le_dls_Link_t* linkPtr = le_dls_PeekTail(&callCtxPtr->sessionRefList);
    while (linkPtr)
    {
        taf_SessionRef_t *sessionRefNodePtr = CONTAINER_OF(linkPtr, taf_SessionRef_t, link);
        linkPtr = le_dls_PeekPrev(&callCtxPtr->sessionRefList, linkPtr);

        taf_SessionCtx_t *sessionCtxPtr = GetSessionCtx(sessionRefNodePtr->sessionRef);
        TAF_ERROR_IF_RET_VAL(sessionCtxPtr == NULL, LE_NOT_FOUND, "Cannot get sessionCtxPtr from sessionRef");

        le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&sessionCtxPtr->handlerList);
        while (linkHandlerPtr)
        {
            taf_HandlerCtx_t * handlerCtxPtr = CONTAINER_OF(linkHandlerPtr, taf_HandlerCtx_t, link);
            linkHandlerPtr = le_dls_PeekPrev(&sessionCtxPtr->handlerList, linkHandlerPtr);
            if (handlerCtxPtr->handlerPtr)
            {
                size_t length = strnlen(callCtxPtr->destId, TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES);
                if (length > (TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES-1))
                {
                     le_utf8_Copy(callCtxPtr->destId, "Unknown", sizeof(callCtxPtr->destId), NULL);
                     LE_ERROR("The destId length exceeds the max length");
                }
                handlerCtxPtr->handlerPtr(callCtxPtr->callRef, callCtxPtr->destId, callCtxPtr->event, handlerCtxPtr->usrContext);
            }
        }

        // if the call have been ended, unlink this session from the call context
        if (callCtxPtr->event == TAF_VOICECALL_EVENT_ENDED)
        {
            LE_INFO("Unbind sessionRef %p from callCtx %p in ENDED", sessionCtxPtr->sessionRef, callCtxPtr);
            UnsetSessionRefToCallCtx(callCtxPtr, sessionCtxPtr->sessionRef);
        }
    }

    return LE_OK;
}

void VoiceCallSvc::CallHandler(CallEvent_t *eventVoicePtr)
{
    TAF_ERROR_IF_RET_NIL(eventVoicePtr == NULL, "eventVoicePtr is NULL");

    taf_VoiceCtrl_t* callCtxPtr = GetCallCtx(eventVoicePtr->callRef);
    TAF_ERROR_IF_RET_NIL((callCtxPtr == NULL) && (eventVoicePtr->event != TAF_VOICECALL_EVENT_INCOMING) && (eventVoicePtr->event != TAF_VOICECALL_EVENT_WAITING),
        "cannot found callCtx for event: %s", EventToString(eventVoicePtr->event));

    LE_INFO("Event: %s, callRef: %p, callCtx: %p, dest: %s",
        EventToString(eventVoicePtr->event), eventVoicePtr->callRef, callCtxPtr, eventVoicePtr->dest);

    // for incoming call, may need to create callCtx if cannot found
    if ((callCtxPtr == NULL) && 
        ((eventVoicePtr->event == TAF_VOICECALL_EVENT_INCOMING) ||
         (eventVoicePtr->event == TAF_VOICECALL_EVENT_WAITING)))
    {
        LE_INFO("No callCtx for event %s, create one", EventToString(eventVoicePtr->event));
        callCtxPtr = CreateCallCtx(eventVoicePtr->phoneId, eventVoicePtr->dest, taf_voicecall_Direction_t::INCOMING);
        TAF_ERROR_IF_RET_NIL(callCtxPtr == NULL, "Cannot create call context");
    }

    // update the latest event
    callCtxPtr->lastEvent = callCtxPtr->event;
    callCtxPtr->event = eventVoicePtr->event;

    if (callCtxPtr->event == TAF_VOICECALL_EVENT_ENDED)
    {
        callCtxPtr->termination = eventVoicePtr->termination;
        callCtxPtr->isInProgress = false;
    }

    SendCallEventToClient(callCtxPtr);

    if (callCtxPtr->event == TAF_VOICECALL_EVENT_ENDED)
    {
        LE_DEBUG("This call[%p] is ended", callCtxPtr);
        if ((eventVoicePtr->direction == taf_voicecall_Direction_t::INCOMING) && (le_dls_NumLinks(&SessionCtxList) == 0))
        {
            LE_DEBUG("Call DestructorCallCtx");
            le_mem_Release(callCtxPtr);
        }
        ShowAll();
    }
}

taf_voicecall_StateHandlerRef_t VoiceCallSvc::CreateStateHandlerCtx(taf_SessionCtx_t* sessionCtxPtr, taf_voicecall_StateHandlerFunc_t handlerPtr, void* contextPtr)
{
    taf_HandlerCtx_t * handlerCtxPtr = (taf_HandlerCtx_t *)le_mem_ForceAlloc(HandlerPool);
    handlerCtxPtr->handlerPtr = handlerPtr;
    handlerCtxPtr->usrContext = contextPtr;
    handlerCtxPtr->handlerRef = (taf_voicecall_StateHandlerRef_t)le_ref_CreateRef(HandlerRefMap, handlerCtxPtr);
    handlerCtxPtr->sessionCtxPtr = sessionCtxPtr;
    handlerCtxPtr->link = LE_DLS_LINK_INIT;

    le_dls_Queue(&sessionCtxPtr->handlerList, &handlerCtxPtr->link);

    return handlerCtxPtr->handlerRef;
}

le_result_t VoiceCallSvc::RemoveStateHandlerCtx(le_msg_SessionRef_t sessionRef, taf_voicecall_StateHandlerRef_t handlerRef)
{
    taf_SessionCtx_t* sessionPtr = GetSessionCtx(sessionRef);
    TAF_ERROR_IF_RET_VAL(sessionPtr == NULL, LE_NOT_FOUND, "sessionRef(%p) is invalid", sessionRef);

    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&(sessionPtr->handlerList));
    while (linkPtr)
    {
        taf_HandlerCtx_t* handlerCtxPtr = CONTAINER_OF(linkPtr, taf_HandlerCtx_t, link);
        linkPtr = le_dls_PeekNext(&(sessionPtr->handlerList), linkPtr);

        if ((handlerCtxPtr) && (handlerCtxPtr->handlerRef == handlerRef))
        {
            le_ref_DeleteRef(HandlerRefMap, handlerRef);
            le_dls_Remove(&(sessionPtr->handlerList), &(handlerCtxPtr->link));
            le_mem_Release((void*)handlerCtxPtr);
            return LE_OK;
        }
    }

    LE_ERROR("cannot found invalid handler reference");
    return LE_NOT_FOUND;
}

le_result_t VoiceCallSvc::RemoveStateHandlerCtx(le_msg_SessionRef_t sessionRef)
{
    taf_SessionCtx_t* sessionPtr = GetSessionCtx(sessionRef);
    TAF_ERROR_IF_RET_VAL(sessionPtr == NULL, LE_NOT_FOUND, "sessionRef(%p) is invalid", sessionRef);

    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&(sessionPtr->handlerList));
    while (linkPtr)
    {
        taf_HandlerCtx_t* handlerCtxPtr = CONTAINER_OF(linkPtr, taf_HandlerCtx_t, link);
        linkPtr = le_dls_PeekNext(&(sessionPtr->handlerList), linkPtr);

        if (handlerCtxPtr)
        {
            le_ref_DeleteRef(HandlerRefMap, handlerCtxPtr->handlerRef);
            le_dls_Remove(&(sessionPtr->handlerList), &(handlerCtxPtr->link));
            le_mem_Release((void*)handlerCtxPtr);
            return LE_OK;
        }
    }

    LE_ERROR("cannot found invalid handler reference");
    return LE_NOT_FOUND;
}

taf_SessionCtx_t* VoiceCallSvc::GetSessionCtx(le_msg_SessionRef_t sessionRef)
{
    taf_SessionCtx_t* sessionCtxPtr = NULL;

    le_dls_Link_t* linkPtr = le_dls_Peek(&SessionCtxList);
    while (linkPtr)
    {
        taf_SessionCtx_t* sessionCtxTmpPtr = CONTAINER_OF(linkPtr, taf_SessionCtx_t, link);
        linkPtr = le_dls_PeekNext(&SessionCtxList, linkPtr);

        if ( sessionCtxTmpPtr->sessionRef == sessionRef )
        {
            sessionCtxPtr = sessionCtxTmpPtr;
        }
    }

    LE_DEBUG("sessionCtx %p found for the sessionRef %p", sessionCtxPtr, sessionRef);

    return sessionCtxPtr;
}

taf_SessionCtx_t* VoiceCallSvc::CreateSessionCtx(void)
{
    // Create the session context
    taf_SessionCtx_t* sessionCtxPtr = (taf_SessionCtx_t*)le_mem_ForceAlloc(SessionCtxPool);
    sessionCtxPtr->sessionRef = taf_voicecall_GetClientSessionRef();
    sessionCtxPtr->link = LE_DLS_LINK_INIT;
    sessionCtxPtr->callRefList = LE_DLS_LIST_INIT;

    le_dls_Queue(&SessionCtxList, &(sessionCtxPtr->link));

    LE_DEBUG("Context for sessionRef %p created at %p", sessionCtxPtr->sessionRef, sessionCtxPtr);

    return sessionCtxPtr;
}

le_result_t VoiceCallSvc::ReleaseSession(le_msg_SessionRef_t sessionRef, void* ctxPtr)
{
    le_dls_Link_t* linkPtr = NULL;

    taf_SessionCtx_t* sessionCtx = GetSessionCtx(sessionRef);
    TAF_ERROR_IF_RET_VAL(sessionCtx == NULL, LE_NOT_FOUND, "Cannot get sessionCtx");

    LE_INFO("To close the sessionRef: %p", sessionRef);

    // remove sessionCtx from callCtrl
    linkPtr = le_dls_Peek(&CallCtrlList);
    while (linkPtr)
    {
        taf_VoiceCtrl_t* callCtxPtr = CONTAINER_OF(linkPtr, taf_VoiceCtrl_t, link);
        linkPtr = le_dls_PeekNext(&CallCtrlList, linkPtr);

        taf_SessionRef_t* sessionRefNodePtr = GetSessionRefNodeFromCallCtx(callCtxPtr, sessionRef);
        if (sessionRefNodePtr == NULL)
        {
            LE_ERROR("this session is not bound to callCtxPtr(%p), skip", callCtxPtr);
            continue;
        }

        if ((callCtxPtr->event == TAF_VOICECALL_EVENT_ACTIVE) ||
            (callCtxPtr->event == TAF_VOICECALL_EVENT_ONHOLD) ||
            (callCtxPtr->event == TAF_VOICECALL_EVENT_DIALING) ||
            (callCtxPtr->event == TAF_VOICECALL_EVENT_ALERTING) ||
            (callCtxPtr->event == TAF_VOICECALL_EVENT_WAITING))
        {
            LE_INFO("The call[%s] will be hung up as session %p is released",
            EventToString(callCtxPtr->event), sessionRef);
            StopCall(callCtxPtr->callRef, sessionRef);
        }

        LE_DEBUG("sessionRef(%p) is bound to callCtx, unlink it", sessionRef);
        le_dls_Remove(&(callCtxPtr->sessionRefList), &(sessionRefNodePtr->link));
        le_mem_Release(sessionRefNodePtr);
        le_mem_Release(callCtxPtr);
    }

    // remove state handler
    RemoveStateHandlerCtx(sessionRef);

    // finally remove sessionCtx
    le_dls_Remove(&SessionCtxList, &sessionCtx->link);
    le_mem_Release(sessionCtx);

    LE_DEBUG("Finish to close this session %p", sessionRef);
    ShowAll();

    return LE_OK;
}

taf_VoiceCtrl_t* VoiceCallSvc::GetCallCtx(int8_t phoneId, const char* destinationPtr, taf_voicecall_Direction_t dir)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&CallCtrlList);
    while ( linkPtr )
    {
        taf_VoiceCtrl_t* callCtx = CONTAINER_OF( linkPtr, taf_VoiceCtrl_t, link);
        linkPtr = le_dls_PeekNext(&CallCtrlList, linkPtr);
        LE_INFO("Link phoneId: %d, dest： %s, dir: %d", callCtx->phoneId, callCtx->destId, static_cast<int>(callCtx->dir));
        // Check phone number and only return the client call object.
        if ((strncmp(destinationPtr, callCtx->destId, sizeof(callCtx->destId)) == 0) &&
            (callCtx->phoneId == phoneId) && (callCtx->dir == dir))
        {
            LE_DEBUG("Getcall ctrl %p", callCtx);
            return callCtx;
        }
    }

    return NULL;
}


taf_VoiceCtrl_t* VoiceCallSvc::GetCallCtx(taf_voicecall_CallRef_t reference)
{
    taf_VoiceCtrl_t* callCtxPtr = (taf_VoiceCtrl_t* )le_ref_Lookup(CallCtrlRefMap, (void*)reference);
    TAF_ERROR_IF_RET_VAL(reference == NULL, NULL, "This reference is NULL, maybe incoming call");
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, NULL, "Cannot get callCtx from ref(%p)", reference);

    return callCtxPtr;
}

taf_VoiceCtrl_t* VoiceCallSvc::CreateCallCtx(int8_t phoneId, const char* destinationPtr, taf_voicecall_Direction_t dir)
{
    taf_VoiceCtrl_t* callCtx = NULL;

    LE_DEBUG("Create ctx for phoneId: %d, dest： %s, dir: %d", phoneId, destinationPtr, (int)dir);

    callCtx = (taf_VoiceCtrl_t*)le_mem_ForceAlloc(CallCtrlPool);
    TAF_ERROR_IF_RET_VAL(!callCtx, NULL, "cannot alloc callCtr");

    le_utf8_Copy(callCtx->destId, destinationPtr, sizeof(callCtx->destId), NULL);
    callCtx->phoneId = phoneId;
    callCtx->dir = dir;
    callCtx->event = TAF_VOICECALL_EVENT_ENDED;
    callCtx->lastEvent = TAF_VOICECALL_EVENT_ENDED;
    callCtx->termination = TAF_VOICECALL_END_UNDEFINED;
    callCtx->terminationCode = -1;
    callCtx->isInProgress = false;
    callCtx->sessionRefList = LE_DLS_LIST_INIT;
    callCtx->link = LE_DLS_LINK_INIT;

    le_dls_Queue(&CallCtrlList, &callCtx->link);

    return callCtx;
}

void VoiceCallSvc::DestructorCallCtx(void* objPtr)
{
    taf_VoiceCtrl_t *callCtxPtr = (taf_VoiceCtrl_t*)objPtr;

    if (callCtxPtr)
    {
        LE_DEBUG("releasing callPtr: %p", callCtxPtr);
        if (callCtxPtr->callRef != nullptr)
        {
            le_ref_DeleteRef(CallCtrlRefMap, callCtxPtr->callRef);
        }
        le_dls_Remove(&CallCtrlList, &callCtxPtr->link);
    }
}

taf_voicecall_CallRef_t VoiceCallSvc::SetCallRef(taf_VoiceCtrl_t* callCtxPtr)
{
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, NULL, "callCtxPtr is null");
    taf_voicecall_CallRef_t callRef = (taf_voicecall_CallRef_t)le_ref_CreateRef(CallCtrlRefMap, (void *)callCtxPtr);
    callCtxPtr->callRef = callRef;
    return callRef;
}

le_result_t VoiceCallSvc::SetSessionRefToCallCtx(taf_VoiceCtrl_t* callCtxPtr, le_msg_SessionRef_t sessionRef)
{
    taf_SessionRef_t* newSessionRefPtr = (taf_SessionRef_t *)le_mem_ForceAlloc(SessionRefPool);
    TAF_ERROR_IF_RET_VAL(newSessionRefPtr == NULL, LE_NO_MEMORY, "Cannot alloc mem for sessionRefNode");
    LE_DEBUG("Binding session %p to callCtx %p...", sessionRef, callCtxPtr);
    newSessionRefPtr->sessionRef = sessionRef;
    newSessionRefPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&callCtxPtr->sessionRefList, &(newSessionRefPtr->link));
    le_mem_AddRef(callCtxPtr);

    return LE_OK;
}

taf_SessionRef_t* VoiceCallSvc::GetSessionRefNodeFromCallCtx(taf_VoiceCtrl_t* callCtxPtr, le_msg_SessionRef_t sessionRef)
{
    le_dls_Link_t* linkPtr = NULL;

    if (callCtxPtr)
    {
        linkPtr = le_dls_Peek(&(callCtxPtr->sessionRefList));
    }

    TAF_ERROR_IF_RET_VAL(linkPtr == NULL, NULL, "This callCtx %p doesn't bind with any session", callCtxPtr);

    while ( linkPtr )
    {
        taf_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkPtr, taf_SessionRef_t, link);
        linkPtr = le_dls_PeekNext(&(callCtxPtr->sessionRefList), linkPtr);

        if (sessionRefPtr->sessionRef == sessionRef)
        {
            LE_DEBUG("Got session node with sessionRef %p and callPtr %p", sessionRef, callCtxPtr);
            return sessionRefPtr;
        }
    }

    LE_ERROR("Cannot get session node with sessionRef: %p and callCtx: %p", sessionRef, callCtxPtr);
    return NULL;
}

le_result_t VoiceCallSvc::UnsetSessionRefToCallCtx(taf_VoiceCtrl_t* callCtxPtr, le_msg_SessionRef_t sessionRef)
{

    taf_SessionRef_t* sessionRefPtr = GetSessionRefNodeFromCallCtx(callCtxPtr, sessionRef);
    TAF_ERROR_IF_RET_VAL(sessionRefPtr == NULL, LE_NOT_FOUND, "Cannot found sessionRefPtr for callCtxPtr: %p", callCtxPtr);
    le_dls_Remove(&(callCtxPtr->sessionRefList), &(sessionRefPtr->link));
    le_mem_Release(sessionRefPtr);
    le_mem_Release(callCtxPtr);
    return LE_OK;
}

le_result_t VoiceCallSvc::MakeCall(taf_VoiceCtrl_t *callCtxPtr, const char *dialNumber, int phoneId)
{
    auto callback = [](taf_pa_voicecall_Ref_t reference, le_result_t result, void* contextPtr) 
    {
        if (result != LE_OK)
        {
            LE_ERROR("Make call callback failed: %d, callRef: %p", result, contextPtr);
        }
        else
        {
            LE_INFO("Make call callback done");
        }
    };

    taf_pa_voicecall_Ref_t callInfoRef = taf_pa_voicecall_CreateReference(phoneId, dialNumber, TAF_PA_VOICECALL_DIR_OUTGOING);
    TAF_ERROR_IF_RET_VAL(callInfoRef == nullptr, LE_FAULT, "Cannot create call reference");
    le_result_t result = taf_pa_voicecall_Make(callInfoRef, callback, callCtxPtr->callRef);
    TAF_ERROR_IF_RET_VAL(taf_pa_voicecall_DeleteReference(callInfoRef) != LE_OK, LE_FAULT,  "Cannot free call reference");
    if (result != LE_OK)
    {
        LE_ERROR("Make call failed, return value: %d", result);
        CallEvent_t msgCallEvent = {0, taf_voicecall_Direction_t::NONE,
            "", callCtxPtr->callRef,
            TAF_VOICECALL_EVENT_RESOURCE_BUSY, TAF_VOICECALL_END_UNDEFINED};
        le_event_Report(CallEvent, &msgCallEvent, sizeof(CallEvent_t));
        return LE_OK; /* return OK as the error event will be reported by listener */
    }
    // set to dailing status for default MO call
    callCtxPtr->event = TAF_VOICECALL_EVENT_DIALING;
    callCtxPtr->isInProgress = true;

    LE_DEBUG("Make call[%p] done", callCtxPtr);
    ShowAll();
    return LE_OK;
}

le_result_t VoiceCallSvc::AnswerCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef)
{
    taf_VoiceCtrl_t* callCtxPtr = (taf_VoiceCtrl_t* )le_ref_Lookup(CallCtrlRefMap, (void*)callRef);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "Cannot found callCtxPtr!");

    taf_SessionRef_t *sessionRefNodePtr = GetSessionRefNodeFromCallCtx(callCtxPtr, sessionRef);
    TAF_ERROR_IF_RET_VAL(sessionRefNodePtr == NULL, LE_NOT_FOUND, "This sessionRef(%p) is not bound to this callCtx(%p)", sessionRef, callCtxPtr);

    auto callCallback = [](taf_pa_voicecall_Ref_t reference, le_result_t result, void* contextPtr) 
    {
        if (result != LE_OK)
        {
            LE_ERROR("Answer call callback failed: %d, callRef: %p", result, contextPtr);
        }
        else
        {
            LE_INFO("Answer call callback done");
        }
    };

    taf_pa_voicecall_Ref_t callInfoRef = taf_pa_voicecall_CreateReference(callCtxPtr->phoneId, callCtxPtr->destId, TAF_PA_VOICECALL_DIR_INCOMING);
    TAF_ERROR_IF_RET_VAL(callInfoRef == nullptr, LE_FAULT, "Cannot create call reference");
    le_result_t result = taf_pa_voicecall_Answer(callInfoRef, callCallback, callCtxPtr->callRef);
    TAF_ERROR_IF_RET_VAL(taf_pa_voicecall_DeleteReference(callInfoRef) != LE_OK, LE_FAULT,  "Cannot free call reference");
    if (result != LE_OK)
    {
        LE_ERROR("Answer call failed, return value: 0x%x", (uint32_t)result);
        CallEvent_t msgCallEvent = {0, taf_voicecall_Direction_t::NONE,
            "", callCtxPtr->callRef,
            TAF_VOICECALL_EVENT_CALL_ANSWER_FAILED, TAF_VOICECALL_END_UNDEFINED};
        le_event_Report(CallEvent, &msgCallEvent, sizeof(CallEvent_t));
        return LE_OK; /* return OK as the error event will be reported by listener */
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(&SessionCtxList);
    while (linkPtr)
    {
        taf_SessionCtx_t* sessionCtxTmpPtr = CONTAINER_OF(linkPtr, taf_SessionCtx_t, link);
        linkPtr = le_dls_PeekNext(&SessionCtxList, linkPtr);

        if (sessionCtxTmpPtr->sessionRef == sessionRef)
        {
            continue;
        }

        sessionRefNodePtr = GetSessionRefNodeFromCallCtx(callCtxPtr, sessionCtxTmpPtr->sessionRef);
        if (sessionRefNodePtr != NULL)
        {
            UnsetSessionRefToCallCtx(callCtxPtr, sessionCtxTmpPtr->sessionRef);
        }
    }
    return LE_OK;
}

le_result_t VoiceCallSvc::StopCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef)
{
    taf_VoiceCtrl_t* callCtxPtr = (taf_VoiceCtrl_t* )le_ref_Lookup(CallCtrlRefMap, (void*)callRef);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "Cannot found callCtx from ref(%p)", (void *)callRef);

    auto callCallback = [](taf_pa_voicecall_Ref_t reference, le_result_t result, void* contextPtr) 
    {
        if (result != LE_OK)
        {
            LE_ERROR("Stop callback failed: %d, callRef: %p", result, contextPtr);
        }
        else
        {
            LE_INFO("Stop callback done");
        }
    };

    taf_pa_voicecall_Ref_t callInfoRef = taf_pa_voicecall_CreateReference(callCtxPtr->phoneId, callCtxPtr->destId, DirToPADir(callCtxPtr->dir));
    TAF_ERROR_IF_RET_VAL(callInfoRef == nullptr, LE_FAULT, "Cannot create call reference");
    le_result_t result = taf_pa_voicecall_Stop(callInfoRef, callCallback, callCtxPtr->callRef);
    TAF_ERROR_IF_RET_VAL(taf_pa_voicecall_DeleteReference(callInfoRef) != LE_OK, LE_FAULT,  "Cannot free call reference");
    if (result != LE_OK)
    {
        LE_ERROR("Stop call failed, return value: 0x%x", (uint32_t)result);
        CallEvent_t msgCallEvent = {0, taf_voicecall_Direction_t::NONE,
            "", callCtxPtr->callRef,
            TAF_VOICECALL_EVENT_CALL_END_FAILED, TAF_VOICECALL_END_UNDEFINED};
        le_event_Report(CallEvent, &msgCallEvent, sizeof(CallEvent_t));
        return LE_OK; /* return OK as the error event will be reported by listener */
    }
    return LE_OK;
}

le_result_t VoiceCallSvc::HoldCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef)
{
    taf_VoiceCtrl_t* callCtxPtr = (taf_VoiceCtrl_t* )le_ref_Lookup(CallCtrlRefMap, (void*)callRef);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "Cannot found callCtx from ref(%p)", (void *)callRef);

    auto callCallback = [](taf_pa_voicecall_Ref_t reference, le_result_t result, void* contextPtr) 
    {
        if (result != LE_OK)
        {
            LE_ERROR("Hold callback failed: %d, callRef: %p", result, contextPtr);
        }
        else
        {
            LE_INFO("Hold callback done");
        }
    };

    taf_pa_voicecall_Ref_t callInfoRef = taf_pa_voicecall_CreateReference(callCtxPtr->phoneId, callCtxPtr->destId, DirToPADir(callCtxPtr->dir));
    TAF_ERROR_IF_RET_VAL(callInfoRef == nullptr, LE_FAULT, "Cannot create call reference");
    le_result_t result = taf_pa_voicecall_Hold(callInfoRef, callCallback, callCtxPtr->callRef);
    TAF_ERROR_IF_RET_VAL(taf_pa_voicecall_DeleteReference(callInfoRef) != LE_OK, LE_FAULT,  "Cannot free call reference");
    if (result != LE_OK)
    {
        LE_ERROR("Hold call failed, return value: 0x%x", (uint32_t)result);
        CallEvent_t msgCallEvent = {0, taf_voicecall_Direction_t::NONE,
            "", callCtxPtr->callRef,
            TAF_VOICECALL_EVENT_CALL_HOLD_FAILED, TAF_VOICECALL_END_UNDEFINED};
        le_event_Report(CallEvent, &msgCallEvent, sizeof(CallEvent_t));
        return LE_OK; /* return OK as the error event will be reported by listener */
    }

    return LE_OK;
}

le_result_t VoiceCallSvc::ResumeCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef)
{
    taf_VoiceCtrl_t* callCtxPtr = (taf_VoiceCtrl_t* )le_ref_Lookup(CallCtrlRefMap, (void*)callRef);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "Cannot found callCtx from ref(%p)", (void *)callRef);

    auto callCallback = [](taf_pa_voicecall_Ref_t reference, le_result_t result, void* contextPtr) 
    {
        if (result != LE_OK)
        {
            LE_ERROR("Resume callback is failed: %d, callRef: %p", result, contextPtr);
        }
        else
        {
            LE_INFO("Resume callback done");
        }
    };

    taf_pa_voicecall_Ref_t callInfoRef = taf_pa_voicecall_CreateReference(callCtxPtr->phoneId, callCtxPtr->destId, DirToPADir(callCtxPtr->dir));
    TAF_ERROR_IF_RET_VAL(callInfoRef == nullptr, LE_FAULT, "Cannot create call reference");
    le_result_t result = taf_pa_voicecall_Resume(callInfoRef, callCallback, callCtxPtr->callRef);
    TAF_ERROR_IF_RET_VAL(taf_pa_voicecall_DeleteReference(callInfoRef) != LE_OK, LE_FAULT,  "Cannot free call reference");
    if (result != LE_OK)
    {
        LE_ERROR("Resume call failed, return value: 0x%x", (uint32_t)result);
        CallEvent_t msgCallEvent = {0, taf_voicecall_Direction_t::NONE,
            "", callCtxPtr->callRef,
            TAF_VOICECALL_EVENT_CALL_RESUME_FAILED, TAF_VOICECALL_END_UNDEFINED};
        le_event_Report(CallEvent, &msgCallEvent, sizeof(CallEvent_t));
        return LE_OK; /* return OK as the error event will be reported by listener */
    }

    return LE_OK;
}

le_result_t VoiceCallSvc::DeleteCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef)
{
    taf_VoiceCtrl_t* callCtxPtr = (taf_VoiceCtrl_t* )le_ref_Lookup(CallCtrlRefMap, (void*)callRef);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "Cannot found callCtxPtr");

    if ((callCtxPtr->event != TAF_VOICECALL_EVENT_ENDED) || (callCtxPtr->isInProgress == true))
    {
        LE_ERROR("Error state: Event(%s) inprogress(%d), cannot delete", EventToString(callCtxPtr->event), callCtxPtr->isInProgress);
        return LE_FAULT;
    }

    taf_SessionRef_t* sessionRefNodePtr = GetSessionRefNodeFromCallCtx(callCtxPtr, sessionRef);
    if (sessionRefNodePtr != NULL)
    {
        le_dls_Remove(&(callCtxPtr->sessionRefList), &(sessionRefNodePtr->link));
        le_mem_Release(sessionRefNodePtr);
    }
    else
    {
        LE_ERROR("SessionRef %p does not bind with callRef %p, skip", sessionRef, callRef);
    }

    le_mem_Release(callCtxPtr);

    return LE_OK;
}

le_result_t VoiceCallSvc::SwapCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef)
{
    taf_VoiceCtrl_t* callCtxPtr = (taf_VoiceCtrl_t* )le_ref_Lookup(CallCtrlRefMap, (void*)callRef);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "Cannot found callCtx from ref(%p)", (void *)callRef);

    auto callCallback = [](taf_pa_voicecall_Ref_t reference, le_result_t result, void* contextPtr) 
    {
        if (result != LE_OK)
        {
            LE_ERROR("Swap callback failed: %d, callRef: %p", result, contextPtr);
        }
        else
        {
            LE_INFO("Swap callback done");
        }
    };

    taf_pa_voicecall_Ref_t callInfoRef = taf_pa_voicecall_CreateReference(callCtxPtr->phoneId, callCtxPtr->destId, DirToPADir(callCtxPtr->dir));
    TAF_ERROR_IF_RET_VAL(callInfoRef == nullptr, LE_FAULT, "Cannot create call reference");
    le_result_t result = taf_pa_voicecall_Swap(callInfoRef, callCallback, callCtxPtr->callRef);
    TAF_ERROR_IF_RET_VAL(taf_pa_voicecall_DeleteReference(callInfoRef) != LE_OK, LE_FAULT,  "Cannot free call reference");
    if (result != LE_OK)
    {
        LE_ERROR("Swap call failed, return value: 0x%x", (uint32_t)result);
        CallEvent_t msgCallEvent = {0, taf_voicecall_Direction_t::NONE,
            "", callCtxPtr->callRef,
            TAF_VOICECALL_EVENT_CALL_SWAP_FAILED, TAF_VOICECALL_END_UNDEFINED};
        le_event_Report(CallEvent, &msgCallEvent, sizeof(CallEvent_t));
        return LE_OK; /* return OK as the error event will be reported by listener */
    }
    return LE_OK;
}

const char* VoiceCallSvc::PaEventToString(taf_pa_voicecall_event_t event) 
{
    switch (event) {
        case TAF_PA_VOICECALL_EVENT_ALERTING: return "ALERTING";
        case TAF_PA_VOICECALL_EVENT_ACTIVE: return "ACTIVE";
        case TAF_PA_VOICECALL_EVENT_ENDED: return "ENDED";
        case TAF_PA_VOICECALL_EVENT_OFFLINE: return "OFFLINE";
        case TAF_PA_VOICECALL_EVENT_WAITING: return "WAITING";
        case TAF_PA_VOICECALL_EVENT_RESOURCE_BUSY: return "RESOURCE_BUSY";
        case TAF_PA_VOICECALL_EVENT_CALL_END_FAILED: return "CALL_END_FAILED";
        case TAF_PA_VOICECALL_EVENT_CALL_ANSWER_FAILED: return "CALL_ANSWER_FAILED";
        case TAF_PA_VOICECALL_EVENT_INCOMING: return "INCOMING";
        case TAF_PA_VOICECALL_EVENT_ONHOLD: return "ONHOLD";
        case TAF_PA_VOICECALL_EVENT_DIALING: return "DIALING";
        case TAF_PA_VOICECALL_EVENT_CALL_HOLD_FAILED: return "CALL_HOLD_FAILED";
        case TAF_PA_VOICECALL_EVENT_CALL_RESUME_FAILED: return "CALL_RESUME_FAILED";
        case TAF_PA_VOICECALL_EVENT_CALL_SWAP_FAILED: return "CALL_SWAP_FAILED";
        default: return "UNKNOWN";
    }
}

taf_voicecall_Event_t VoiceCallSvc::EventConvert(taf_pa_voicecall_event_t event) 
{
    switch (event) {
        case TAF_PA_VOICECALL_EVENT_ALERTING: return TAF_VOICECALL_EVENT_ALERTING;
        case TAF_PA_VOICECALL_EVENT_ACTIVE: return TAF_VOICECALL_EVENT_ACTIVE;
        case TAF_PA_VOICECALL_EVENT_ENDED: return TAF_VOICECALL_EVENT_ENDED;
        case TAF_PA_VOICECALL_EVENT_OFFLINE: return TAF_VOICECALL_EVENT_OFFLINE;
        case TAF_PA_VOICECALL_EVENT_WAITING: return TAF_VOICECALL_EVENT_WAITING;
        case TAF_PA_VOICECALL_EVENT_RESOURCE_BUSY: return TAF_VOICECALL_EVENT_RESOURCE_BUSY;
        case TAF_PA_VOICECALL_EVENT_CALL_END_FAILED: return TAF_VOICECALL_EVENT_CALL_END_FAILED;
        case TAF_PA_VOICECALL_EVENT_CALL_ANSWER_FAILED: return TAF_VOICECALL_EVENT_CALL_ANSWER_FAILED;
        case TAF_PA_VOICECALL_EVENT_INCOMING: return TAF_VOICECALL_EVENT_INCOMING;
        case TAF_PA_VOICECALL_EVENT_ONHOLD: return TAF_VOICECALL_EVENT_ONHOLD;
        case TAF_PA_VOICECALL_EVENT_DIALING: return TAF_VOICECALL_EVENT_DIALING;
        case TAF_PA_VOICECALL_EVENT_CALL_HOLD_FAILED: return TAF_VOICECALL_EVENT_CALL_HOLD_FAILED;
        case TAF_PA_VOICECALL_EVENT_CALL_RESUME_FAILED: return TAF_VOICECALL_EVENT_CALL_RESUME_FAILED;
        case TAF_PA_VOICECALL_EVENT_CALL_SWAP_FAILED: return TAF_VOICECALL_EVENT_CALL_SWAP_FAILED;
        default: return TAF_VOICECALL_EVENT_ENDED;
    }
}

taf_voicecall_Direction_t VoiceCallSvc::DirConvert(taf_pa_voicecall_dir_t paDir) 
{
    switch (paDir) {
        case TAF_PA_VOICECALL_DIR_INCOMING: return taf_voicecall_Direction_t::INCOMING;
        case TAF_PA_VOICECALL_DIR_OUTGOING: return taf_voicecall_Direction_t::OUTGOING;
        case TAF_PA_VOICECALL_DIR_NONE: return taf_voicecall_Direction_t::NONE;
        default: return taf_voicecall_Direction_t::NONE;
    }
}

taf_pa_voicecall_dir_t VoiceCallSvc::DirToPADir(taf_voicecall_Direction_t dir) 
{
    switch (dir) {
        case taf_voicecall_Direction_t::INCOMING: return TAF_PA_VOICECALL_DIR_INCOMING;
        case taf_voicecall_Direction_t::OUTGOING: return TAF_PA_VOICECALL_DIR_OUTGOING;
        case taf_voicecall_Direction_t::NONE: return TAF_PA_VOICECALL_DIR_NONE;
        default: return TAF_PA_VOICECALL_DIR_NONE;
    }
}

taf_voicecall_CallEndCause_t VoiceCallSvc::EndCauseConvert(taf_pa_voicecall_termination_t paTerm)
{
    switch (paTerm) {
        case TAF_PA_VOICECALL_TERM_NORMAL: return TAF_VOICECALL_END_NORMAL;
        case TAF_PA_VOICECALL_TERM_NETWORK_FAIL: return TAF_VOICECALL_END_NETWORK_FAIL;
        case TAF_PA_VOICECALL_TERM_UNOBTAINABLE_NUMBER: return TAF_VOICECALL_END_UNOBTAINABLE_NUMBER;
        case TAF_PA_VOICECALL_TERM_BUSY: return TAF_VOICECALL_END_BUSY;
        case TAF_PA_VOICECALL_TERM_LOCAL: return TAF_VOICECALL_END_LOCAL;
        case TAF_PA_VOICECALL_TERM_REMOTE: return TAF_VOICECALL_END_REMOTE;
        case TAF_PA_VOICECALL_TERM_UNDEFINED: return TAF_VOICECALL_END_UNDEFINED;
        case TAF_PA_VOICECALL_TERM_REJECTED: return TAF_VOICECALL_END_REJECTED;
        case TAF_PA_VOICECALL_TERM_NORESPONSE: return TAF_VOICECALL_END_NORESPONSE;
        default: return TAF_VOICECALL_END_UNDEFINED;
    }
}

void VoiceCallSvc::Init(void)
{
    // TelAF side initializations
    CallCtrlPool = le_mem_InitStaticPool(tafCall,MAX_TAFCALL_OBJ,sizeof(taf_VoiceCtrl_t));
    le_mem_SetDestructor(CallCtrlPool, Handler::ReleaseCallCtrlHandler);

    CallRefPool = le_mem_InitStaticPool(tafCallRef,MAX_TAFCALL_OBJ,sizeof(taf_CallRefNode_t));

    SessionCtxPool = le_mem_InitStaticPool(tafSessionCtx,MAX_TAFCALL_SESSION,sizeof(taf_SessionCtx_t));
    SessionRefPool = le_mem_InitStaticPool(tafSessionRef,MAX_TAFCALL_SESSION,sizeof(taf_SessionRef_t));

    HandlerPool = le_mem_InitStaticPool(tafHandler,MAX_TAFCALL_SESSION,sizeof(taf_HandlerCtx_t));
    CallCtrlRefMap = le_ref_CreateMap("tafVoiceCallCtrl",MAX_VOICECALL_SUPPORT*2);
    HandlerRefMap = le_ref_CreateMap("tafVoiceCallHandler",MAX_TAFCALL_SESSION*2);

    // add handler for close session
    le_msg_AddServiceCloseHandler(taf_voicecall_GetServiceRef(),    \
                                    Handler::CloseSessHandler,    \
                                    NULL);

    // Handle platform adapter call events
    CallEvent = le_event_CreateId("tafCall Event", sizeof(CallEvent_t));

    // Add the state changed handler
    le_event_AddHandler("tafCall state changed", CallEvent, Handler::ProcessStateChanged);

    if (taf_pa_voicecall_Init() != LE_OK)
    {
        LE_FATAL("Cannot initialize voice call platform adaptor");
    }

    // Register platform adapter listener
    le_result_t result = taf_pa_voicecall_RegisterEventListener(Handler::PaEventListener, nullptr);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to register platform adapter listener, ret: %d", result);
    }

    LE_INFO("System ready, start voice call service!\n");

}

VoiceCallSvc &VoiceCallSvc::GetInstance()
{
    static VoiceCallSvc instance;
    return instance;
}

