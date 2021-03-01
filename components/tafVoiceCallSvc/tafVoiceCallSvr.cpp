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
 * @file       tafVoiceCallSvr.cpp
 * @brief      This file provides the taf voice call service as interfaces described
 *             in taf_voicecall.api. The voice call service will be started automatically.
 */
#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include <telux/tel/PhoneFactory.hpp>
#include "tafVoiceCall.hpp"

using namespace telux::tel;
using namespace telux::common;
using namespace telux::tafsvc;


COMPONENT_INIT
{
    LE_INFO("tafVoiceCall Service Init...\n");
    auto &myCall = taf_VoiceCall::GetInstance();
    myCall.Init();

    // install the handler
    taf_Handler myHandler;

    LE_INFO(" Voice Call service Ready...\n");

}

/*======================================================================
 FUNCTION        taf_voicecall_AddStateHandler
 DESCRIPTION     Client use this API to add an user state handler
 DEPENDENCIES    Initialization of Voice Call Service
 PARAMETERS      [IN]  handlerPtr:   handler pointer.
                 [IN]  contextPtr:  handler context.
 RETURN VALUE    taf_voicecall_StateHandlerRef_t reference
 SIDE EFFECTS    N/A
======================================================================*/
taf_voicecall_StateHandlerRef_t taf_voicecall_AddStateHandler
(
    taf_voicecall_StateHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Input handlerPtr is NULL!");

    auto &myCall = taf_VoiceCall::GetInstance();
    taf_voicecall_StateHandlerRef_t  handlerRef;

    taf_SessionCtx_t* sessionCtxPtr = myCall.GetSessionCtx(taf_voicecall_GetClientSessionRef());
    if (!sessionCtxPtr)
    {
        sessionCtxPtr = myCall.CreateSessionCtx();
        if (!sessionCtxPtr)
        {
            LE_ERROR("Impossible to create the session context");
            return NULL;
        }
    }

    handlerRef = myCall.CreateStateHandlerCtx(sessionCtxPtr, handlerPtr, contextPtr);

    return (taf_voicecall_StateHandlerRef_t)(handlerRef);
}

/*======================================================================
 FUNCTION        taf_voicecall_RemoveStateHandler
 DESCRIPTION     Client use this API to remove an user state handler
 DEPENDENCIES    Initialization of Voice Call Service
 PARAMETERS      [IN]  handlerRef:   handler reference.
 RETURN VALUE    N/A
 SIDE EFFECTS    N/A
======================================================================*/
void taf_voicecall_RemoveStateHandler
(
    taf_voicecall_StateHandlerRef_t handlerRef
)
{
    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "HandlerPtr is NULL!");
    auto &myCall = taf_VoiceCall::GetInstance();
    myCall.RemoveStateHandlerCtx(taf_voicecall_GetClientSessionRef(), handlerRef);
}

/*======================================================================
 FUNCTION        taf_voicecall_Start
 DESCRIPTION     Client use this API to start a call
 DEPENDENCIES    Initialization of Voice Call Service
 PARAMETERS      [IN]  destinationID     : remote phone number.
                 [IN]  phoneId           : slot number
 RETURN VALUE    taf_voicecall_CallRef_t : the call reference
 SIDE EFFECTS    N/A
======================================================================*/
taf_voicecall_CallRef_t taf_voicecall_Start
(
    const char* destinationID,
    uint8_t phoneId
)
{
    TAF_ERROR_IF_RET_VAL(destinationID == NULL, NULL, "destinationID is NULL!");
    TAF_ERROR_IF_RET_VAL(phoneId > MAX_PHONE_ID, NULL, "phoneId[%d] is invalid!", MAX_PHONE_ID);

    auto &myCall = taf_VoiceCall::GetInstance();
    callReq_t req;
    taf_voicecall_CallRef_t callRef = NULL;

    taf_SessionCtx_t* sessionCtxPtr = myCall.GetSessionCtx(taf_voicecall_GetClientSessionRef());
    if (!sessionCtxPtr)
    {
        sessionCtxPtr = myCall.CreateSessionCtx();
        TAF_ERROR_IF_RET_VAL((!sessionCtxPtr), NULL, "Impossible to create the session context");
    }

    req.callCtrlPtr = myCall.GetCallCtx(phoneId, destinationID);
    if (req.callCtrlPtr != NULL)
    {
        // if this callCtrl is already created, and the call is in progress, return error
        if ((req.callCtrlPtr->isInProgress == true) && (req.callCtrlPtr->event != TAF_VOICECALL_EVENT_ENDED))
        {
            LE_ERROR("The callCtrl bind with phone(%d) and number(%s) is in progress!", phoneId, destinationID);
            return NULL;
        }
        // the case callCtx is not owned by this session, add sessionRef to it and add 1 to mem_ref
        else if (myCall.GetSessionRefNodeFromCallCtx(req.callCtrlPtr, taf_voicecall_GetClientSessionRef()) == NULL)
        {
            LE_INFO("Cannot get sessionRef from callCtrlPtr!");
            myCall.SetSessionRefToCallCtx(req.callCtrlPtr, taf_voicecall_GetClientSessionRef());
        }

        callRef = req.callCtrlPtr->callRef;
    }
    else
    {
        LE_INFO("Create callCtrl for phone(%d) and dest(%s)", phoneId, destinationID);
        req.callCtrlPtr = myCall.CreateCallCtx(phoneId, destinationID);
        TAF_ERROR_IF_RET_VAL((req.callCtrlPtr == NULL), NULL, "Cannot create callCtrl for phone(%d) dest(%s)", phoneId, destinationID);

        callRef = myCall.SetCallRef(req.callCtrlPtr); //myCall.setCallRefToSessionCtx(req.callCtrlPtr, sessionCtxPtr);
        TAF_ERROR_IF_RET_VAL((callRef == NULL), NULL, "cannot link callRef to SessionCtx");

        le_result_t leRet = myCall.SetSessionRefToCallCtx(req.callCtrlPtr, taf_voicecall_GetClientSessionRef());
        TAF_ERROR_IF_RET_VAL((leRet != LE_OK), NULL, "set sessionRef to callCtx failed");
    }

    req.cmdID = CMD_START_CALL;
    le_event_Report(myCall.ReqEvent, &req, sizeof(callReq_t));

    return callRef;
}

/*======================================================================
 FUNCTION        taf_voicecall_End
 DESCRIPTION     Client use this API to stop a call
 DEPENDENCIES    Should start a call first
 PARAMETERS      [IN]  reference     : the call reference return by
                                       taf_voicecall_Start()
 RETURN VALUE    le_result_t         : LE_OK     - success 
                                       otherwise - failure
 SIDE EFFECTS    N/A
======================================================================*/
le_result_t taf_voicecall_End
(
    taf_voicecall_CallRef_t reference
)
{
    callReq_t req;
    auto &myCall = taf_VoiceCall::GetInstance();

    req.cmdID = CMD_END_CALL;
    req.callRef = reference;
    req.sessionRef = taf_voicecall_GetClientSessionRef();
    le_event_Report(myCall.ReqEvent, &req, sizeof(callReq_t));

    return LE_OK;
}

/*======================================================================
 FUNCTION        taf_voicecall_Delete
 DESCRIPTION     Client use this API to delete a call
 DEPENDENCIES    Should stop a call first
 PARAMETERS      [IN]  reference     : the call reference return by
                                       taf_voicecall_Start()
 RETURN VALUE    le_result_t         : LE_OK     - success 
                                       otherwise - failure
 SIDE EFFECTS    N/A
======================================================================*/
le_result_t taf_voicecall_Delete
(
    taf_voicecall_CallRef_t reference
)
{
    auto &myCall = taf_VoiceCall::GetInstance();

    return myCall.DeleteCall(reference, taf_voicecall_GetClientSessionRef());
}

/*======================================================================
 FUNCTION        taf_voicecall_Answer
 DESCRIPTION     Client use this API to answer a call
 DEPENDENCIES    This API is used on incoming call case
 PARAMETERS      [IN]  reference     : the call reference return by
                                       state handler
 RETURN VALUE    le_result_t         : LE_OK     - success 
                                       otherwise - failure
 SIDE EFFECTS    N/A
======================================================================*/
le_result_t taf_voicecall_Answer
(
    taf_voicecall_CallRef_t reference
)
{
    callReq_t req;
    auto &myCall = taf_VoiceCall::GetInstance();

    req.cmdID = CMD_ANSWER_CALL;
    req.callRef = reference;
    req.sessionRef = taf_voicecall_GetClientSessionRef();
    le_event_Report(myCall.ReqEvent, &req, sizeof(callReq_t));

    return LE_OK;
}

/*======================================================================
 FUNCTION        taf_voicecall_GetEndCause
 DESCRIPTION     Client use this API to get the end cause
 DEPENDENCIES    This call should be on stop state when calling this API
 PARAMETERS      [IN]   reference     : the call reference
                 [OUT]  causePtr      : end cause
 RETURN VALUE    le_result_t         : LE_OK     - success 
                                       otherwise - failure
 SIDE EFFECTS    N/A
======================================================================*/
le_result_t taf_voicecall_GetEndCause
(
    taf_voicecall_CallRef_t reference,
    taf_voicecall_CallEndCause_t *causePtr
)

{
    TAF_ERROR_IF_RET_VAL(causePtr == NULL, LE_FAULT, "causePtr is NULL");

    auto &myCall = taf_VoiceCall::GetInstance();

    taf_VoiceCtrl_t* callCtxPtr = (taf_VoiceCtrl_t* )le_ref_Lookup(myCall.CallCtrlRefMap, (void*)reference);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "Cannot get callCtx from ref(%p)", reference);
    TAF_ERROR_IF_RET_VAL((callCtxPtr->event != TAF_VOICECALL_EVENT_ENDED) || (callCtxPtr->isInProgress == true),
        LE_FAULT, "callCtx is on running, event: %d, isInProgress: %d", callCtxPtr->event, callCtxPtr->isInProgress);

    *causePtr = callCtxPtr->termination;
    return LE_OK;
}

/*======================================================================
 FUNCTION        taf_voicecall_Hold
 DESCRIPTION     Client use this API to hold a call
 DEPENDENCIES    This call should be on active state
 PARAMETERS      [IN]   reference     : the call reference
 RETURN VALUE    le_result_t         : LE_OK     - success 
                                       otherwise - failure
 SIDE EFFECTS    N/A
======================================================================*/
le_result_t taf_voicecall_Hold
(
    taf_voicecall_CallRef_t reference
)
{
    callReq_t req;
    auto &myCall = taf_VoiceCall::GetInstance();

    req.cmdID = CMD_HOLD_CALL;
    req.callRef = reference;
    req.sessionRef = taf_voicecall_GetClientSessionRef();
    le_event_Report(myCall.ReqEvent, &req, sizeof(callReq_t));

    return LE_OK;
}

/*======================================================================
 FUNCTION        taf_voicecall_Resume
 DESCRIPTION     Client use this API to hold a call
 DEPENDENCIES    This call should be on holding state
 PARAMETERS      [IN]   reference     : the call reference
 RETURN VALUE    le_result_t         : LE_OK     - success 
                                       otherwise - failure
 SIDE EFFECTS    N/A
======================================================================*/
le_result_t taf_voicecall_Resume
(
    taf_voicecall_CallRef_t reference
)
{
    callReq_t req;
    auto &myCall = taf_VoiceCall::GetInstance();

    req.cmdID = CMD_RESUME_CALL;
    req.callRef = reference;
    req.sessionRef = taf_voicecall_GetClientSessionRef();
    le_event_Report(myCall.ReqEvent, &req, sizeof(callReq_t));

    return LE_OK;
}

