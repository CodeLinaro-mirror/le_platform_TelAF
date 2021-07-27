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

#include "legato.h"
#include "interfaces.h"
#include "telux/tel/PhoneFactory.hpp"
#include "tafVoiceCall.hpp"
#include <unistd.h>

using namespace telux::tel;
using namespace telux::common;
using namespace telux::tafsvc;
using namespace std;


LE_MEM_DEFINE_STATIC_POOL(tafCall,MAX_TAFCALL_OBJ,sizeof(taf_VoiceCtrl_t));
LE_MEM_DEFINE_STATIC_POOL(tafCallRef,MAX_TAFCALL_OBJ,sizeof(taf_CallRefNode_t));
LE_MEM_DEFINE_STATIC_POOL(tafSessionCtx,MAX_TAFCALL_SESSION,sizeof(taf_SessionCtx_t));
LE_MEM_DEFINE_STATIC_POOL(tafSessionRef,MAX_TAFCALL_SESSION,sizeof(taf_SessionRef_t));
LE_MEM_DEFINE_STATIC_POOL(tafHandler,MAX_TAFCALL_SESSION,sizeof(taf_HandlerCtx_t));

taf_VoiceCall* taf_Handler::TafCallPtr = NULL;
bool  taf_VoiceCall::isEnableDebug = false;

//-----------------------------------------------------------------------------
//Class tafDialCallback Implementation
//

void tafDialCallback::makeCallResponse(telux::common::ErrorCode error, std::shared_ptr<telux::tel::ICall> iCall)
{
    TAF_ERROR_IF_RET_NIL(iCall == nullptr, "iCall is nullptr!");

    LE_DEBUG("phone[%d] in callIndex[%d], phoneNum: %s, error: %d\n",
        iCall->getPhoneId(), iCall->getCallIndex(), iCall->getRemotePartyNumber().c_str(), (uint32_t)error);

    taf_VoiceCtrl_t* callCtxPtr;
    auto &myCall = taf_VoiceCall::GetInstance();

    callCtxPtr = myCall.GetCallCtx(iCall->getPhoneId(), (const char* )iCall->getRemotePartyNumber().c_str());
    TAF_ERROR_IF_RET_NIL(callCtxPtr == NULL, "cannot found call Ctx for phone[%d] num[%s]",
        iCall->getCallIndex(), iCall->getRemotePartyNumber().c_str());

    callCtxPtr->iCall = iCall;
    callCtxPtr->callRspErrCode = error;
    if (callCtxPtr->callRspErrCode != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error response(0x%x) when making a call, iCall: %p", (uint32_t)callCtxPtr->callRspErrCode, &*callCtxPtr->iCall);
    }

    return;
}

//-----------------------------------------------------------------------------
//Class tafCallListener Implementation
//

void tafCallListener::onIncomingCall(std::shared_ptr<telux::tel::ICall> iCall)
{
    TAF_ERROR_IF_RET_NIL(iCall == nullptr, "iCall is nullptr!");

    callEvent_t msgCallEvent = { 0 };
    auto &myCall = taf_VoiceCall::GetInstance();
    telux::tel::CallState state = iCall->getCallState();
    int8_t phoneId;

    le_utf8_Copy(msgCallEvent.dest, iCall->getRemotePartyNumber().c_str(), MAX_DESTINATION_LEN, NULL);
    phoneId = iCall->getPhoneId();

    LE_DEBUG("In coming call: %s(0x%x), icall: %p, phoneId: %d, phoneNum: %s， dir: %s\n",
        callStateToString(state), (uint32_t)state, &*iCall, phoneId, msgCallEvent.dest, callDirectionToString(iCall->getCallDirection()));

    msgCallEvent.phoneId = phoneId;
    msgCallEvent.iCall = iCall;
    msgCallEvent.event = stateToEvent(state);
    msgCallEvent.inComingCall = true;
    le_event_Report(myCall.CallEvent, &msgCallEvent, sizeof(callEvent_t));
}

// To handle the event during the call
void tafCallListener::onCallInfoChange(std::shared_ptr<telux::tel::ICall> iCall)
{
    TAF_ERROR_IF_RET_NIL(iCall == nullptr, "iCall is nullptr!");

    callEvent_t msgCallEvent = { 0 };
    auto &myCall = taf_VoiceCall::GetInstance();
    telux::tel::CallState state = iCall->getCallState();

    bool isIncomingCall = false;
    if (iCall->getCallDirection() == telux::tel::CallDirection::INCOMING)
    {
        isIncomingCall = true;
    }

    LE_DEBUG("Call[%d] state changed to: %s, dir: %s, dest: %s\n",
        iCall->getCallIndex(), callStateToString(state), callDirectionToString(iCall->getCallDirection()),
        iCall->getRemotePartyNumber().c_str());

    // skip dialing event because it is a temporary status, and always come before makeCallResponse
    if (state == telux::tel::CallState::CALL_DIALING)
    {
        LE_INFO("skip dialing event");
        return;
    }

    // To fix the corner case, iCall is released later when testing with telsdk app,
    // callRef is used for the event report.
    taf_VoiceCtrl_t* callCtxPtr = myCall.GetCallCtx(iCall);
    TAF_ERROR_IF_RET_NIL(callCtxPtr == NULL, "cannot get call context for event[%s]",
        callStateToString(state));
    msgCallEvent.callRef = callCtxPtr->callRef;
    le_utf8_Copy(msgCallEvent.dest, iCall->getRemotePartyNumber().c_str(), MAX_DESTINATION_LEN, NULL);
    msgCallEvent.phoneId = iCall->getPhoneId();
    msgCallEvent.event = stateToEvent(state);
    msgCallEvent.inComingCall = isIncomingCall;

    if (msgCallEvent.event == TAF_VOICECALL_EVENT_ENDED)
    {
        msgCallEvent.termination = endCauseToTermination(iCall->getCallEndCause());
        LE_DEBUG("EndCause: %d, termination: %d", (uint32_t)iCall->getCallEndCause(), (uint32_t)msgCallEvent.termination);
    }

    le_event_Report(myCall.CallEvent, &msgCallEvent,sizeof(callEvent_t));
}

const char* tafCallListener::callStateToString(telux::tel::CallState state)
{
    const char *callStateString[] = {"idle", "active", "on_hold", "dialing", "incoming", "waiting", "alerting", "ended"};
    // "state" started with -1
    uint32_t state_fixed = (uint32_t)state + 1;

    if ((uint32_t)state >= sizeof(callStateString)/sizeof(callStateString[0]))
    {
        return "invalid";
    }
    return callStateString[state_fixed];
}

const char* tafCallListener::callDirectionToString(telux::tel::CallDirection direction)
{
    switch (direction) {
    case telux::tel::CallDirection::INCOMING:
        return "INCOMING";
    case telux::tel::CallDirection::OUTGOING:
        return "OUTGOING";
    default:
        return "NONE";
    }
}

taf_voicecall_CallEndCause_t tafCallListener::endCauseToTermination(telux::tel::CallEndCause endCause)
{
    taf_voicecall_CallEndCause_t termination = TAF_VOICECALL_END_UNDEFINED;

    switch(endCause) {
        case telux::tel::CallEndCause::UNOBTAINABLE_NUMBER:
        case telux::tel::CallEndCause::NUMBER_CHANGED:
        case telux::tel::CallEndCause::DESTINATION_OUT_OF_ORDER:
        case telux::tel::CallEndCause::INVALID_NUMBER_FORMAT:
        case telux::tel::CallEndCause::INCOMPATIBLE_DESTINATION:
            termination = TAF_VOICECALL_END_UNOBTAINABLE_NUMBER;
        break;

        case telux::tel::CallEndCause::NO_ROUTE_TO_DESTINATION:
        case telux::tel::CallEndCause::CHANNEL_UNACCEPTABLE:
        case telux::tel::CallEndCause::RESP_TO_STATUS_ENQUIRY:
        case telux::tel::CallEndCause::REQUESTED_FACILITY_NOT_SUBSCRIBED:
        case telux::tel::CallEndCause::BEARER_CAPABILITY_NOT_AUTHORIZED:
        case telux::tel::CallEndCause::BEARER_CAPABILITY_UNAVAILABLE:
        case telux::tel::CallEndCause::SERVICE_OPTION_NOT_AVAILABLE:
        case telux::tel::CallEndCause::BEARER_SERVICE_NOT_IMPLEMENTED:
        case telux::tel::CallEndCause::REQUESTED_FACILITY_NOT_IMPLEMENTED:
        case telux::tel::CallEndCause::SERVICE_OR_OPTION_NOT_IMPLEMENTED:
        case telux::tel::CallEndCause::INVALID_TRANSACTION_IDENTIFIER:
        case telux::tel::CallEndCause::USER_NOT_MEMBER_OF_CUG:
        case telux::tel::CallEndCause::CALL_BARRED:
        case telux::tel::CallEndCause::FDN_BLOCKED:
        case telux::tel::CallEndCause::IMSI_UNKNOWN_IN_VLR:
        case telux::tel::CallEndCause::IMEI_NOT_ACCEPTED:
        case telux::tel::CallEndCause::DIAL_MODIFIED_TO_USSD:
        case telux::tel::CallEndCause::DIAL_MODIFIED_TO_SS:
        case telux::tel::CallEndCause::DIAL_MODIFIED_TO_DIAL:
        case telux::tel::CallEndCause::OPERATOR_DETERMINED_BARRING:
        case telux::tel::CallEndCause::NETWORK_OUT_OF_ORDER:
            termination = TAF_VOICECALL_END_NETWORK_FAIL;
        break;

        case telux::tel::CallEndCause::NORMAL:
        case telux::tel::CallEndCause::NORMAL_UNSPECIFIED:
            termination = TAF_VOICECALL_END_NORMAL;
        break;

        case telux::tel::CallEndCause::BUSY:
        case telux::tel::CallEndCause::NO_ANSWER_FROM_USER:
        case telux::tel::CallEndCause::PREEMPTION:
        case telux::tel::CallEndCause::FACILITY_REJECTED:
        case telux::tel::CallEndCause::CONGESTION:
        case telux::tel::CallEndCause::SWITCHING_EQUIPMENT_CONGESTION:
        case telux::tel::CallEndCause::REQUESTED_CIRCUIT_OR_CHANNEL_NOT_AVAILABLE:
        case telux::tel::CallEndCause::RESOURCES_UNAVAILABLE_OR_UNSPECIFIED:
            termination = TAF_VOICECALL_END_BUSY;
        break;

        case telux::tel::CallEndCause::CALL_REJECTED:
            termination = TAF_VOICECALL_END_REJECTED;
        break;

        case telux::tel::CallEndCause::NO_USER_RESPONDING:
            termination = TAF_VOICECALL_END_NORESPONSE;
        break;

        case telux::tel::CallEndCause::TEMPORARY_FAILURE:
        case telux::tel::CallEndCause::ACCESS_INFORMATION_DISCARDED:
        case telux::tel::CallEndCause::QOS_UNAVAILABLE:
        case telux::tel::CallEndCause::INCOMING_CALLS_BARRED_WITHIN_CUG:
        case telux::tel::CallEndCause::ACM_LIMIT_EXCEEDED:
        case telux::tel::CallEndCause::ONLY_DIGITAL_INFORMATION_BEARER_AVAILABLE:
        case telux::tel::CallEndCause::INVALID_TRANSIT_NW_SELECTION:
        case telux::tel::CallEndCause::SEMANTICALLY_INCORRECT_MESSAGE:
        case telux::tel::CallEndCause::INVALID_MANDATORY_INFORMATION:
        case telux::tel::CallEndCause::MESSAGE_TYPE_NON_IMPLEMENTED:
        case telux::tel::CallEndCause::MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE:
        case telux::tel::CallEndCause::INFORMATION_ELEMENT_NON_EXISTENT:
        case telux::tel::CallEndCause::CONDITIONAL_IE_ERROR:
        case telux::tel::CallEndCause::MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE:
        case telux::tel::CallEndCause::RECOVERY_ON_TIMER_EXPIRED:
        case telux::tel::CallEndCause::PROTOCOL_ERROR_UNSPECIFIED:
        case telux::tel::CallEndCause::INTERWORKING_UNSPECIFIED:
        case telux::tel::CallEndCause::CDMA_LOCKED_UNTIL_POWER_CYCLE:
        case telux::tel::CallEndCause::CDMA_DROP:
        case telux::tel::CallEndCause::CDMA_INTERCEPT:
        case telux::tel::CallEndCause::CDMA_REORDER:
        case telux::tel::CallEndCause::CDMA_SO_REJECT:
        case telux::tel::CallEndCause::CDMA_RETRY_ORDER:
        case telux::tel::CallEndCause::CDMA_ACCESS_FAILURE:
        case telux::tel::CallEndCause::CDMA_PREEMPTED:
        case telux::tel::CallEndCause::CDMA_NOT_EMERGENCY:
        case telux::tel::CallEndCause::CDMA_ACCESS_BLOCKED:
        case telux::tel::CallEndCause::ERROR_UNSPECIFIED:
            termination = TAF_VOICECALL_END_UNDEFINED;
        break;

        default:
            termination = TAF_VOICECALL_END_UNDEFINED;
        break;;
    }

    return termination;
}

taf_voicecall_Event_t tafCallListener::stateToEvent(telux::tel::CallState state)
{
    taf_voicecall_Event_t event = TAF_VOICECALL_EVENT_ENDED;

    switch (state)
    {
        case telux::tel::CallState::CALL_ACTIVE:
            event = TAF_VOICECALL_EVENT_ACTIVE;
        break;

        case telux::tel::CallState::CALL_ON_HOLD:
            event = TAF_VOICECALL_EVENT_ONHOLD;
        break;

        case telux::tel::CallState::CALL_DIALING:
            event = TAF_VOICECALL_EVENT_DIALING;
        break;

        case telux::tel::CallState::CALL_INCOMING:
            event = TAF_VOICECALL_EVENT_INCOMING;
        break;

        case telux::tel::CallState::CALL_WAITING:
            event = TAF_VOICECALL_EVENT_WAITING;
        break;

        case telux::tel::CallState::CALL_ALERTING:
            event = TAF_VOICECALL_EVENT_ALERTING;
        break;

        case telux::tel::CallState::CALL_ENDED:
            event = TAF_VOICECALL_EVENT_ENDED;
        break;

        default:
        break;
    }

    return event;
}

//-----------------------------------------------------------------------------
// Class Handler Implementations
//
taf_Handler::taf_Handler()
{
    return;
}

taf_Handler::~taf_Handler()
{
    return;
}

void taf_Handler::Init()
{
    return;
}

// session close handler
void taf_Handler::CloseSessHandler(le_msg_SessionRef_t sessionRef, void* ctxPtr)
{
    TAF_ERROR_IF_RET_NIL(!sessionRef, "sessionRef is NULL");

    auto &myCall = taf_VoiceCall::GetInstance();
    myCall.ReleaseSession(sessionRef, ctxPtr);
}

// ctrlCtrl memory free handler
void taf_Handler::ReleaseCallCtrlHandler(void* objPtr)
{
    auto &myCall = taf_VoiceCall::GetInstance();
    myCall.DestructorCallCtx(objPtr);
}

// call request handler
void taf_Handler::ProcessReq(void *callReq)
{
    callReq_t *pReq;

    auto &myCall = taf_VoiceCall::GetInstance();

    LE_DEBUG("Process the call request!\n");

    pReq = (callReq_t *)callReq;
    tafCallCmd_t cmd = pReq->cmdID;

    switch(cmd)
    {
        case CMD_START_CALL:
        {
            myCall.MakeCall(pReq->callCtrlPtr, pReq->callCtrlPtr->destId, pReq->callCtrlPtr->phoneId);
        }
        break;

        case CMD_END_CALL:
        {
            taf_voicecall_CallRef_t callRef    = pReq->callRef;
            le_msg_SessionRef_t     sessionRef = pReq->sessionRef;
            myCall.StopCall(callRef, sessionRef);
        }
        break;

        case CMD_ANSWER_CALL:
        {
            LE_INFO("in incoming call!");
            taf_voicecall_CallRef_t callRef    = pReq->callRef;
            le_msg_SessionRef_t     sessionRef = pReq->sessionRef;
            myCall.AnswerCall(callRef, sessionRef);
        }
        break;

        case CMD_DELETE_CALL:
        {
            taf_voicecall_CallRef_t callRef    = pReq->callRef;
            le_msg_SessionRef_t     sessionRef = pReq->sessionRef;
            myCall.DeleteCall(callRef, sessionRef);
        }
        break;

        case CMD_HOLD_CALL:
        {
            taf_voicecall_CallRef_t callRef    = pReq->callRef;
            le_msg_SessionRef_t     sessionRef = pReq->sessionRef;
            myCall.HoldCall(callRef, sessionRef);
        }
        break;

        case CMD_RESUME_CALL:
        {
            taf_voicecall_CallRef_t callRef    = pReq->callRef;
            le_msg_SessionRef_t     sessionRef = pReq->sessionRef;
            myCall.ResumeCall(callRef, sessionRef);
        }
        break;

    }

    return;
}


// First layer state/event handler
void taf_Handler::ProcessStateChanged(void *reportPtr)
{
    callEvent_t *eventVoicePtr = (callEvent_t *)reportPtr;

    auto &myCall = taf_VoiceCall::GetInstance();
    myCall.CallHandler(eventVoicePtr);
}


//-----------------------------------------------------------------------------
// Class tafVoiceCall implementation
//
void taf_VoiceCall::ShowAll()
{
    uint32_t i = 0, j = 0, k = 0;

    if (isEnableDebug == false)
    {
        return;
    }

    LE_DEBUG("========================== voice call show all start ==========================");
    LE_DEBUG("SessionCtx list num: %d", le_dls_NumLinks(&SessionCtxList));
    le_dls_Link_t* linkPtr = le_dls_Peek(&SessionCtxList);
    while (linkPtr)
    {
        taf_SessionCtx_t* sessionCtxTmpPtr = CONTAINER_OF(linkPtr, taf_SessionCtx_t, link);
        linkPtr = le_dls_PeekNext(&SessionCtxList, linkPtr);

        LE_DEBUG("   [%d]sessionCtx:%p Ref: %p Handler Num: %d, CallRef Num: %d",
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

            LE_DEBUG("       [%d]callRef: 0x%x", k++, (uint32_t)callRefPtr->callRef);
        }
    }

    i = 0, j = 0, k = 0;
    LE_DEBUG("CallCtx Num: %d", le_dls_NumLinks(&CallCtrlList));
    linkPtr = le_dls_Peek(&CallCtrlList);
    while ( linkPtr )
    {
        taf_VoiceCtrl_t* callCtxPtr = CONTAINER_OF( linkPtr, taf_VoiceCtrl_t, link);
        linkPtr = le_dls_PeekNext(&CallCtrlList, linkPtr);
        LE_DEBUG("   [%d]ID: %d, destId: %s, callRef: 0x%x, event: %s, termination: %s",
            i++, callCtxPtr->phoneId, callCtxPtr->destId, (uint32_t)callCtxPtr->callRef,
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

const char * taf_VoiceCall::EventToString(taf_voicecall_Event_t event)
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

const char * taf_VoiceCall::TerminationToString(taf_voicecall_CallEndCause_t termination)
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

le_result_t taf_VoiceCall::SendCallEventToClient(taf_VoiceCtrl_t *callCtxPtr, bool isIncomingCall)
{
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_BAD_PARAMETER, "callCtxPtr is null");

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
                LE_WARN("sessionRefNode already exist or session handler(%d) is nout bound", numLinks);
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
                handlerCtxPtr->handlerPtr(callCtxPtr->callRef, callCtxPtr->destId, callCtxPtr->event, handlerCtxPtr->usrContext);
            }
        }

        // if the call have been ended, unlink this session from the call context
        if (callCtxPtr->event == TAF_VOICECALL_EVENT_ENDED)
        {
            UnsetSessionRefToCallCtx(callCtxPtr, sessionCtxPtr->sessionRef);
        }
    }

    return LE_OK;
}

void taf_VoiceCall::CallHandler(callEvent_t *eventVoicePtr)
{
    TAF_ERROR_IF_RET_NIL(eventVoicePtr == NULL, "eventVoicePtr is NULL");
    taf_VoiceCtrl_t* callCtxPtr;
    if (eventVoicePtr->iCall != nullptr)
    {
        callCtxPtr = GetCallCtx(eventVoicePtr->iCall);
    }
    else
    {
        callCtxPtr = GetCallCtx(eventVoicePtr->callRef);
    }
    TAF_ERROR_IF_RET_NIL((callCtxPtr == NULL) && (eventVoicePtr->event != TAF_VOICECALL_EVENT_INCOMING),
        "cannot found callCtx for event: %s", EventToString(eventVoicePtr->event));

    // for incoming call, may need to create callCtx if cannot found
    if (eventVoicePtr->event == TAF_VOICECALL_EVENT_INCOMING)
    {
        if ((callCtxPtr == NULL) || (callCtxPtr->isInProgress == true))
        {
            LE_INFO("No callCtx found, need to new one");
            callCtxPtr = CreateCallCtx(eventVoicePtr->phoneId, eventVoicePtr->dest);
            TAF_ERROR_IF_RET_NIL(callCtxPtr == NULL, "cannot create callCtx");
        }
        callCtxPtr->iCall = eventVoicePtr->iCall;
    }

    // update the latest event
    callCtxPtr->lastEvent = callCtxPtr->event;
    callCtxPtr->event = eventVoicePtr->event;

    if (callCtxPtr->event == TAF_VOICECALL_EVENT_ENDED)
    {
        callCtxPtr->termination = eventVoicePtr->termination;
        callCtxPtr->isInProgress = false;
        callCtxPtr->iCall = nullptr;
    }

    SendCallEventToClient(callCtxPtr, eventVoicePtr->inComingCall);

    if (callCtxPtr->event == TAF_VOICECALL_EVENT_ENDED)
    {
        LE_DEBUG("This call[%p] is ended", callCtxPtr);
        ShowAll();
    }
}

taf_voicecall_StateHandlerRef_t taf_VoiceCall::CreateStateHandlerCtx(taf_SessionCtx_t* sessionCtxPtr, taf_voicecall_StateHandlerFunc_t handlerPtr, void* contextPtr)
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

le_result_t taf_VoiceCall::RemoveStateHandlerCtx(le_msg_SessionRef_t sessionRef, taf_voicecall_StateHandlerRef_t handlerRef)
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

le_result_t taf_VoiceCall::RemoveStateHandlerCtx(le_msg_SessionRef_t sessionRef)
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

taf_SessionCtx_t* taf_VoiceCall::GetSessionCtx(le_msg_SessionRef_t sessionRef)
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

taf_SessionCtx_t* taf_VoiceCall::CreateSessionCtx(void)
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

le_result_t taf_VoiceCall::ReleaseSession(le_msg_SessionRef_t sessionRef, void* ctxPtr)
{
    le_dls_Link_t* linkPtr = NULL;

    taf_SessionCtx_t* sessionCtx = GetSessionCtx(sessionRef);
    TAF_ERROR_IF_RET_VAL(sessionCtx == NULL, LE_NOT_FOUND, "Cannot get sessionCtx");

    LE_DEBUG("To close the sessionRef: %p", sessionRef);

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

taf_VoiceCtrl_t* taf_VoiceCall::GetCallCtx(std::shared_ptr<telux::tel::ICall> iCall)
{
    le_dls_Link_t* linkPtr = NULL;

    TAF_ERROR_IF_RET_VAL(iCall == NULL, NULL, "iCall is null");

    linkPtr = le_dls_Peek(&CallCtrlList);
    while (linkPtr)
    {
        taf_VoiceCtrl_t* callCtxPtr = CONTAINER_OF(linkPtr, taf_VoiceCtrl_t, link);
        linkPtr = le_dls_PeekNext(&CallCtrlList, linkPtr);

        if (callCtxPtr->iCall == iCall)
        {
            LE_DEBUG("Get call ctrl %p", callCtxPtr);
            return callCtxPtr;
        }
    }

    return NULL;
}

taf_VoiceCtrl_t* taf_VoiceCall::GetCallCtx(int8_t phoneId, const char* destinationPtr)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&CallCtrlList);
    while ( linkPtr )
    {
        taf_VoiceCtrl_t* callCtx = CONTAINER_OF( linkPtr, taf_VoiceCtrl_t, link);
        linkPtr = le_dls_PeekNext(&CallCtrlList, linkPtr);

        // Check phone number and only return the client call object.
        if ((strncmp(destinationPtr, callCtx->destId, sizeof(callCtx->destId)) == 0) &&
            (callCtx->phoneId == phoneId))
        {
            LE_DEBUG("Getcall ctrl %p", callCtx);
            return callCtx;
        }
    }

    return NULL;
}


taf_VoiceCtrl_t* taf_VoiceCall::GetCallCtx(taf_voicecall_CallRef_t reference)
{
    taf_VoiceCtrl_t* callCtxPtr = (taf_VoiceCtrl_t* )le_ref_Lookup(CallCtrlRefMap, (void*)reference);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, NULL, "Cannot get callCtx from ref(%p)", reference);

    return callCtxPtr;
}

taf_VoiceCtrl_t* taf_VoiceCall::CreateCallCtx(int8_t phoneId, const char* destinationPtr)
{
    taf_VoiceCtrl_t* callCtx = NULL;

    callCtx = (taf_VoiceCtrl_t*)le_mem_ForceAlloc(CallCtrlPool);
    TAF_ERROR_IF_RET_VAL(!callCtx, NULL, "cannot alloc callCtr");

    le_utf8_Copy(callCtx->destId, destinationPtr, sizeof(callCtx->destId), NULL);
    callCtx->phoneId = phoneId;
    callCtx->event = TAF_VOICECALL_EVENT_ENDED;
    callCtx->lastEvent = TAF_VOICECALL_EVENT_ENDED;
    callCtx->termination = TAF_VOICECALL_END_UNDEFINED;
    callCtx->terminationCode = -1;
    callCtx->isInProgress = false;
    callCtx->sessionRefList = LE_DLS_LIST_INIT;
    callCtx->link = LE_DLS_LINK_INIT;
    callCtx->iCall = nullptr;

    le_dls_Queue(&CallCtrlList, &callCtx->link);

    return callCtx;
}

void taf_VoiceCall::DestructorCallCtx(void* objPtr)
{
    taf_VoiceCtrl_t *callCtxPtr = (taf_VoiceCtrl_t*)objPtr;

    if (callCtxPtr)
    {
        LE_DEBUG("releasing callPtr: %p", callCtxPtr);
        callCtxPtr->iCall = nullptr;
        le_ref_DeleteRef(CallCtrlRefMap, callCtxPtr->callRef);
        le_dls_Remove(&CallCtrlList, &callCtxPtr->link);
    }
}

taf_voicecall_CallRef_t taf_VoiceCall::SetCallRef(taf_VoiceCtrl_t* callCtxPtr)
{
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, NULL, "callCtxPtr is null");
    taf_voicecall_CallRef_t callRef = (taf_voicecall_CallRef_t)le_ref_CreateRef(CallCtrlRefMap, (void *)callCtxPtr);
    callCtxPtr->callRef = callRef;
    return callRef;
}

le_result_t taf_VoiceCall::SetSessionRefToCallCtx(taf_VoiceCtrl_t* callCtxPtr, le_msg_SessionRef_t sessionRef)
{
    taf_SessionRef_t* newSessionRefPtr = (taf_SessionRef_t *)le_mem_ForceAlloc(SessionRefPool);
    TAF_ERROR_IF_RET_VAL(newSessionRefPtr == NULL, LE_NO_MEMORY, "Cannot alloc mem for sessionRefNode");
    newSessionRefPtr->sessionRef = sessionRef;
    newSessionRefPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&callCtxPtr->sessionRefList, &(newSessionRefPtr->link));
    le_mem_AddRef(callCtxPtr);

    return LE_OK;
}

taf_SessionRef_t* taf_VoiceCall::GetSessionRefNodeFromCallCtx(taf_VoiceCtrl_t* callCtxPtr, le_msg_SessionRef_t sessionRef)
{
    le_dls_Link_t* linkPtr = NULL;

    if (callCtxPtr)
    {
        linkPtr = le_dls_Peek(&(callCtxPtr->sessionRefList));
    }

    while ( linkPtr )
    {
        taf_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkPtr, taf_SessionRef_t, link);
        linkPtr = le_dls_PeekNext(&(callCtxPtr->sessionRefList), linkPtr);

        if (sessionRefPtr->sessionRef == sessionRef)
        {
            LE_DEBUG("callPtr %p created by sessionRef %p", callCtxPtr, sessionRef);
            return sessionRefPtr;
        }
    }

    return NULL;
}

le_result_t taf_VoiceCall::UnsetSessionRefToCallCtx(taf_VoiceCtrl_t* callCtxPtr, le_msg_SessionRef_t sessionRef)
{

    taf_SessionRef_t* sessionRefPtr = GetSessionRefNodeFromCallCtx(callCtxPtr, sessionRef);
    TAF_ERROR_IF_RET_VAL(sessionRefPtr == NULL, LE_NOT_FOUND, "Cannot found sessionRefPtr for callCtxPtr: %p", callCtxPtr);
    le_dls_Remove(&(callCtxPtr->sessionRefList), &(sessionRefPtr->link));
    le_mem_Release(sessionRefPtr);
    le_mem_Release(callCtxPtr);
    return LE_OK;
}

le_result_t taf_VoiceCall::MakeCall(taf_VoiceCtrl_t *callCtxPtr, const char *dialNumber,int phoneId)
{
    callCtxPtr->tafCallStatus = CallMgr->makeCall(phoneId, std::string(dialNumber), CallCb);
    if (callCtxPtr->tafCallStatus != telux::common::Status::SUCCESS)
    {
        LE_ERROR("Make call failed, return value: 0x%x", (uint32_t)callCtxPtr->tafCallStatus);
        callEvent_t msgCallEvent = {false, 0, "", nullptr, callCtxPtr->callRef, TAF_VOICECALL_EVENT_RESOURCE_BUSY, TAF_VOICECALL_END_UNDEFINED};
        le_event_Report(CallEvent, &msgCallEvent, sizeof(callEvent_t));
        return LE_FAULT;
    }

    // set to dailing status for default MO call
    callCtxPtr->event = TAF_VOICECALL_EVENT_DIALING;
    callCtxPtr->isInProgress = true;

    LE_DEBUG("MakeCall[%p] done", callCtxPtr);
    ShowAll();

    return LE_OK;
}

le_result_t taf_VoiceCall::AnswerCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef)
{
    taf_VoiceCtrl_t* callCtxPtr = (taf_VoiceCtrl_t* )le_ref_Lookup(CallCtrlRefMap, (void*)callRef);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "Cannot found callCtxPtr!");

    std::shared_ptr<ICall> iCall = callCtxPtr->iCall;
    TAF_ERROR_IF_RET_VAL(iCall == nullptr, LE_NOT_FOUND, "iCall is null on callCtx(%p), event: %s", callCtxPtr, EventToString(callCtxPtr->event));

    taf_SessionRef_t *sessionRefNodePtr = GetSessionRefNodeFromCallCtx(callCtxPtr, sessionRef);
    TAF_ERROR_IF_RET_VAL(sessionRefNodePtr == NULL, LE_NOT_FOUND, "This sessionRef(%p) is not bound to this callCtx(%p)", sessionRef, callCtxPtr);

    callCtxPtr->tafCallStatus = iCall->answer(nullptr);
    if (callCtxPtr->tafCallStatus != telux::common::Status::SUCCESS)
    {
        LE_ERROR("answer call(%p) failed, error code: %d, icall state: %d, iCall: %p",
            (void *)callRef, (uint32_t)callCtxPtr->tafCallStatus, (uint32_t)iCall->getCallState(), &*callCtxPtr->iCall);
        callEvent_t msgCallEvent = {false, 0, "", nullptr, callCtxPtr->callRef, TAF_VOICECALL_EVENT_CALL_ANSWER_FAILED, TAF_VOICECALL_END_UNDEFINED};
        le_event_Report(CallEvent, &msgCallEvent, sizeof(callEvent_t));
        return LE_FAULT;
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

le_result_t taf_VoiceCall::StopCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef)
{
    taf_VoiceCtrl_t* callCtxPtr = (taf_VoiceCtrl_t* )le_ref_Lookup(CallCtrlRefMap, (void*)callRef);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "Cannot found callCtx from ref(%p)", (void *)callRef);

    std::shared_ptr<ICall> iCall = callCtxPtr->iCall;
    if (iCall != nullptr)
    {
        if(iCall->getCallState() == telux::tel::CallState::CALL_INCOMING)
        {
            callCtxPtr->tafCallStatus = iCall->reject(std::shared_ptr<telux::common::ICommandResponseCallback> (nullptr));
        }
        else
        {
            callCtxPtr->tafCallStatus = iCall->hangup(nullptr);
        }

        if (callCtxPtr->tafCallStatus == telux::common::Status::SUCCESS)
        {
            return LE_OK;
        }
    }

    /* stop error, send msg */
    if (iCall != nullptr)
    {
        LE_ERROR("stop call(%p) failed, error code: %d, icall state: %d, iCall: %p",
            (void *)callRef, (uint32_t)callCtxPtr->tafCallStatus, (uint32_t)iCall->getCallState(), &*callCtxPtr->iCall);
    }
    else
    {
        LE_ERROR("iCall is nullptr");
    }
    callEvent_t msgCallEvent = {false, 0, "", nullptr, callCtxPtr->callRef, TAF_VOICECALL_EVENT_CALL_END_FAILED, TAF_VOICECALL_END_UNDEFINED};
    le_event_Report(CallEvent, &msgCallEvent, sizeof(callEvent_t));
    return LE_FAULT;
}

le_result_t taf_VoiceCall::HoldCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef)
{
    taf_VoiceCtrl_t* callCtxPtr = (taf_VoiceCtrl_t* )le_ref_Lookup(CallCtrlRefMap, (void*)callRef);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "Cannot found callCtx from ref(%p)", (void *)callRef);

    std::shared_ptr<ICall> iCall = callCtxPtr->iCall;
    TAF_ERROR_IF_RET_VAL(iCall == nullptr, LE_NOT_FOUND, "iCall is null on callCtrl(%p), event: %s", callCtxPtr, EventToString(callCtxPtr->event));

    callCtxPtr->tafCallStatus = iCall->hold(nullptr);
    if (callCtxPtr->tafCallStatus != telux::common::Status::SUCCESS)
    {
        LE_ERROR("hold call(%p) failed, error code: %d, icall state: %d, iCall: %p",
            (void *)callRef, (uint32_t)callCtxPtr->tafCallStatus, (uint32_t)iCall->getCallState(), &*callCtxPtr->iCall);
        callEvent_t msgCallEvent = {false, 0, "", nullptr, callCtxPtr->callRef, TAF_VOICECALL_EVENT_CALL_HOLD_FAILED, TAF_VOICECALL_END_UNDEFINED};
        le_event_Report(CallEvent, &msgCallEvent, sizeof(callEvent_t));
        return LE_FAULT;
    }

    return LE_OK;
}

le_result_t taf_VoiceCall::ResumeCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef)
{
    taf_VoiceCtrl_t* callCtxPtr = (taf_VoiceCtrl_t* )le_ref_Lookup(CallCtrlRefMap, (void*)callRef);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "Cannot found callCtx from ref(%p)", (void *)callRef);

    std::shared_ptr<ICall> iCall = callCtxPtr->iCall;
    TAF_ERROR_IF_RET_VAL(iCall == nullptr, LE_NOT_FOUND, "iCall is null on callCtrl(%p), event: %s", callCtxPtr, EventToString(callCtxPtr->event));

    callCtxPtr->tafCallStatus = iCall->resume(nullptr);
    if (callCtxPtr->tafCallStatus != telux::common::Status::SUCCESS)
    {
        LE_ERROR("resume call(%p) failed, error code: %d, icall state: %d, iCall: %p",
            (void *)callRef, (uint32_t)callCtxPtr->tafCallStatus, (uint32_t)iCall->getCallState(), &*callCtxPtr->iCall);
        callEvent_t msgCallEvent = {false, 0, "", nullptr, callCtxPtr->callRef, TAF_VOICECALL_EVENT_CALL_RESUME_FAILED, TAF_VOICECALL_END_UNDEFINED};
        le_event_Report(CallEvent, &msgCallEvent, sizeof(callEvent_t));
        return LE_FAULT;
    }

    return LE_OK;
}

le_result_t taf_VoiceCall::DeleteCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef)
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
        LE_ERROR("Cannot get SessionRefNode from callCtrl, skip");
    }

    le_mem_Release(callCtxPtr);

    return LE_OK;
}

void taf_VoiceCall::Init(void)
{
    auto &phoneFactory = PhoneFactory::getInstance();
    PhoneMgr = phoneFactory.getPhoneManager();
    CallMgr = phoneFactory.getCallManager();

    bool isSubSysReady = PhoneMgr->isSubsystemReady();

    if(!isSubSysReady){
        LE_INFO("Waiting for sytem ready!\n");
        std::future<bool> f = PhoneMgr->onSubsystemReady();
        isSubSysReady = f.get();
    }

    // TelAF side initializations
    CallCtrlPool = le_mem_InitStaticPool(tafCall,MAX_TAFCALL_OBJ,sizeof(taf_VoiceCtrl_t));
    le_mem_SetDestructor(CallCtrlPool, taf_Handler::ReleaseCallCtrlHandler);

    CallRefPool = le_mem_InitStaticPool(tafCallRef,MAX_TAFCALL_OBJ,sizeof(taf_CallRefNode_t));

    SessionCtxPool = le_mem_InitStaticPool(tafSessionCtx,MAX_TAFCALL_SESSION,sizeof(taf_SessionCtx_t));
    SessionRefPool = le_mem_InitStaticPool(tafSessionRef,MAX_TAFCALL_SESSION,sizeof(taf_SessionRef_t));

    HandlerPool = le_mem_InitStaticPool(tafHandler,MAX_TAFCALL_SESSION,sizeof(taf_HandlerCtx_t));
    CallCtrlRefMap = le_ref_CreateMap("tafVoiceCallCtrl",MAX_VOICECALL_SUPPORT*2);
    HandlerRefMap = le_ref_CreateMap("tafVoiceCallHandler",MAX_TAFCALL_SESSION*2);

    // Init the handler class static member
    taf_Handler::TafCallPtr = this;

    // add handler for close session
    le_msg_AddServiceCloseHandler(taf_voicecall_GetServiceRef(),    \
                                    taf_Handler::CloseSessHandler,    \
                                    NULL);

    // Create events to handle the call state
    ReqEvent = le_event_CreateId("tafCall Request",sizeof(callReq_t));

    // Handle telsdk call events
    CallEvent = le_event_CreateId("tafCall Event",sizeof(callEvent_t));

    // Add the request handler
    le_event_AddHandler("tafCall Request",ReqEvent,taf_Handler::ProcessReq);

    // Add the state changed handler
    le_event_AddHandler("tafCall state changed",CallEvent,taf_Handler::ProcessStateChanged);

    // register a listener for tafVoiceCall servie
    CallLsn =  std::make_shared<tafCallListener>();
    Status ret = CallMgr->registerListener(CallLsn);
    if(ret!= Status::SUCCESS)
    {
        LE_CRIT("Cannot register Listern for call event!\n");
    }

    CallCb = std::make_shared<tafDialCallback>();

    LE_INFO("System ready, start voice call service!\n");

}

taf_VoiceCall &taf_VoiceCall::GetInstance()
{
    static taf_VoiceCall instance;
    return instance;
}

taf_voicecall_CallRef_t taf_VoiceCall::SetCallRefToSessionCtx(taf_VoiceCtrl_t* callCtxPtr, taf_SessionCtx_t* sessionCtxPtr)
{
    if ((sessionCtxPtr == NULL) || (callCtxPtr == NULL))
    {
        return NULL;
    }

    taf_CallRefNode_t* callNodePtr = (taf_CallRefNode_t *)le_mem_ForceAlloc(CallRefPool);
    if (callNodePtr == NULL)
    {
        return NULL;
    }

    callNodePtr->callRef = (taf_voicecall_CallRef_t)le_ref_CreateRef(CallCtrlRefMap, (void *)callCtxPtr);
    callNodePtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&sessionCtxPtr->callRefList, &callNodePtr->link);

    return callNodePtr->callRef;
}

taf_CallRefNode_t* taf_VoiceCall::GetCallRefNodeFromSessionCtx(taf_VoiceCtrl_t* callCtxPtr, taf_SessionCtx_t* sessionCtxPtr)
{
    le_dls_Link_t* linkPtr = NULL;

    if (sessionCtxPtr == NULL)
    {
        return NULL;
    }

    linkPtr = le_dls_PeekTail(&sessionCtxPtr->callRefList);
    while (linkPtr)
    {
        taf_CallRefNode_t* callRefPtr = CONTAINER_OF(linkPtr, taf_CallRefNode_t, link);
        linkPtr = le_dls_PeekPrev(&sessionCtxPtr->callRefList, linkPtr);

        taf_VoiceCtrl_t* callTmpPtr = (taf_VoiceCtrl_t *)le_ref_Lookup(CallCtrlRefMap, (void *)callRefPtr->callRef);

        if ( callCtxPtr == callTmpPtr )
        {
            LE_INFO("got callRefPtr:%p!", callRefPtr);
            return callRefPtr;
        }
    }

    LE_ERROR("cannot found callCtxPtr: %p, sessionCtxPtr: %p!", callCtxPtr, sessionCtxPtr);
    return NULL;
}



