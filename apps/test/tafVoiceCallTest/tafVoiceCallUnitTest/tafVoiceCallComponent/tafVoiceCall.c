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

#define MAX_DESTINATION_LEN_BYTE    51
#define MAX_DESTINATION_LEN         50

typedef struct
{
    le_sem_Ref_t                         semaphore;
    le_thread_Ref_t                      threadRef;
    uint8_t                              phoneId;
    char                                 destId[MAX_DESTINATION_LEN_BYTE];
    taf_voicecall_StateHandlerRef_t      stateHandlerRef;
    taf_voicecall_CallRef_t              requestRef;
    taf_voicecall_CallEndCause_t         appCause;
} UnitTestContext_t;

static UnitTestContext_t AppCtx;

static taf_voicecall_Event_t LocalExpectEvent;
static taf_voicecall_CallRef_t LocalExpectCallRef;
static taf_voicecall_CallRef_t LocalExpectCallWaitingRef;

static void MyCallEventHandler(taf_voicecall_CallRef_t callRef, \
                               const char* id, \
                               taf_voicecall_Event_t callEvent, \
                               void* ctxPtr)
{
    LE_INFO("Client get event: callEvent %d, callRef: %p!\n", (uint32_t)callEvent, callRef);
    LE_INFO("Client get event: id: %s!\n", id);

    if ((callEvent != TAF_VOICECALL_EVENT_DIALING)&&(callEvent != TAF_VOICECALL_EVENT_WAITING))
    {
        LocalExpectEvent = callEvent;
        LocalExpectCallRef = callRef;
        //LE_INFO("LocalExpectEvent %d, LocalExpectCallRef: %p!\n", (uint32_t)LocalExpectEvent, LocalExpectCallRef);
        le_sem_Post(AppCtx.semaphore);
    }

    if (callEvent == TAF_VOICECALL_EVENT_WAITING)
    {
      LocalExpectEvent = callEvent;
      LocalExpectCallWaitingRef = callRef;
      le_sem_Post(AppCtx.semaphore);
    }

    if (callEvent == TAF_VOICECALL_EVENT_ALERTING)
    {
    }
    else if (callEvent == TAF_VOICECALL_EVENT_ACTIVE)
    {
    }
    else if (callEvent == TAF_VOICECALL_EVENT_ENDED)
    {
    }
    else if (callEvent == TAF_VOICECALL_EVENT_INCOMING)
    {
    }
    else if (callEvent == TAF_VOICECALL_EVENT_ONHOLD)
    {
    }

#if 0

    if ((LocalExpectEvent == callEvent) && (LocalExpectCallRef == callRef))
    {
        le_sem_Post(AppCtx.semaphore);
    }

    if ((TAF_VOICECALL_EVENT_INCOMING == callEvent) && (LocalExpectEvent == callEvent) && (LocalExpectCallRef == NULL))
    {
        LocalExpectCallRef = callRef;
        le_sem_Post(AppCtx.semaphore);
    }
#endif

    return;
}

le_result_t wait_call_event(taf_voicecall_Event_t event, taf_voicecall_CallRef_t callRef, int seconds)
{
    le_clk_Time_t timeToWait = {seconds, 0};
    LocalExpectEvent = event;
    LocalExpectCallRef = callRef;
    LE_INFO("LocalExpectEvent: %d, LocalExpectCallRef: %p, callRef: %p", LocalExpectEvent, LocalExpectCallRef, callRef);
    return le_sem_WaitWithTimeOut(AppCtx.semaphore, timeToWait);
}

le_result_t wait_call(int seconds)
{
    le_clk_Time_t timeToWait = {seconds, 0};

    return le_sem_WaitWithTimeOut(AppCtx.semaphore, timeToWait);
}


static void* ut_tafVoiceCall_StateHandler(void* ctxPtr)
{
    taf_voicecall_StateHandlerRef_t callHandlerRef;
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;

    taf_voicecall_ConnectService();

    LE_INFO("Add null state handler and remove test");
    callHandlerRef = taf_voicecall_AddStateHandler(NULL, NULL);
    LE_ASSERT(callHandlerRef != NULL);
    taf_voicecall_RemoveStateHandler(callHandlerRef);

    LE_INFO("Add valid state handler and remove test");
    callHandlerRef = taf_voicecall_AddStateHandler((taf_voicecall_StateHandlerFunc_t)MyCallEventHandler, NULL);
    LE_ASSERT(callHandlerRef != NULL);
    // supervisor will kill this app if callHandlerRef is null, so cannot test NULL for input
    taf_voicecall_RemoveStateHandler(callHandlerRef);

    callHandlerRef = taf_voicecall_AddStateHandler((taf_voicecall_StateHandlerFunc_t)MyCallEventHandler, NULL);
    LE_ASSERT(callHandlerRef != NULL);

    appCtxPtr->stateHandlerRef = callHandlerRef;
    le_sem_Post(appCtxPtr->semaphore);

    le_event_RunLoop();

    return NULL;
}

static void ut_tafVoiceCall_makecall(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;

    appCtxPtr->requestRef = taf_voicecall_Start(appCtxPtr->destId, appCtxPtr->phoneId);
    LE_ASSERT(appCtxPtr->requestRef != NULL);
    le_sem_Post(appCtxPtr->semaphore);

    return;
}

static void ut_tafVoiceCall_get_endcause(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_GetEndCause(appCtxPtr->requestRef, &appCtxPtr->appCause);
    LE_ASSERT(leRet == LE_OK);
    le_sem_Post(appCtxPtr->semaphore);

    return;
}

static void ut_tafVoiceCall_get_endcause_invalid(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_GetEndCause(appCtxPtr->requestRef, &appCtxPtr->appCause);
    LE_ASSERT(leRet != LE_OK);
    le_sem_Post(appCtxPtr->semaphore);

    return;
}

static void ut_tafVoiceCall_delete(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_Delete(appCtxPtr->requestRef);
    LE_ASSERT(leRet == LE_OK);

    // the second delete should return error
    leRet = taf_voicecall_Delete(appCtxPtr->requestRef);
    LE_ASSERT(leRet != LE_OK);

    le_sem_Post(appCtxPtr->semaphore);

    return;
}

static void ut_tafVoiceCall_answer(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_Answer(appCtxPtr->requestRef);
    LE_ASSERT(leRet == LE_OK);

    return;
}


static void ut_tafVoiceCall_hold(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_Hold(appCtxPtr->requestRef);
    LE_ASSERT(leRet == LE_OK);

    return;
}

static void ut_tafVoiceCall_resume(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_Resume(appCtxPtr->requestRef);
    LE_ASSERT(leRet == LE_OK);

    return;
}

static void ut_tafVoiceCall_end(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_End(appCtxPtr->requestRef);
    LE_ASSERT(leRet == LE_OK);

    return;
}

static void ut_tafVoiceCall_swap(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_Swap(appCtxPtr->requestRef);
    LE_ASSERT(leRet == LE_OK);

    return;
}

le_result_t ut_tafVoiceCall_ValidCall_Hold()
{
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_hold, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(5));
    LE_ASSERT((TAF_VOICECALL_EVENT_ONHOLD == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_hold, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(5));
    LE_ASSERT((TAF_VOICECALL_EVENT_CALL_HOLD_FAILED == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));
    return LE_OK;
}

le_result_t ut_tafVoiceCall_ValidCall_Resume()
{
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_resume, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(5));
    LE_INFO("LocalExpectEvent： %d, LocalExpectCallRef: %p, AppCtx.requestRef: %p", (uint32_t)LocalExpectEvent, LocalExpectCallRef, AppCtx.requestRef);
    LE_ASSERT((TAF_VOICECALL_EVENT_ACTIVE == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_resume, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(5));
    LE_ASSERT((TAF_VOICECALL_EVENT_CALL_RESUME_FAILED == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    return LE_OK;
}

le_result_t ut_tafVoiceCall_ValidCall_End()
{
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_end, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(5));
    LE_INFO("LocalExpectEvent： %d, LocalExpectCallRef: %p, AppCtx.requestRef: %p", (uint32_t)LocalExpectEvent, LocalExpectCallRef, AppCtx.requestRef);
    LE_ASSERT((TAF_VOICECALL_EVENT_ENDED == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

#if 0 //disable this case, currently the first "end" will unlink this session from the call context
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_end, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(10));
    LE_ASSERT((TAF_VOICECALL_EVENT_CALL_END_FAILED == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));
#endif
    return LE_OK;
}

le_result_t ut_tafVoiceCall_Call_Delete()
{
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_delete, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(5));

    return LE_OK;
}

le_result_t ut_tafVoiceCall_IncomingCall()
{
    LE_INFO("===== waiting for the incoming call =====");
    LE_ASSERT_OK(wait_call(120));
    LE_ASSERT(TAF_VOICECALL_EVENT_INCOMING == LocalExpectEvent);

    AppCtx.requestRef = LocalExpectCallRef;
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_answer, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(5));
    LE_ASSERT((TAF_VOICECALL_EVENT_ACTIVE == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));
    sleep(5);
    return LE_OK;
}

le_result_t isCallNumberValid
(
    const char* callNumber
)
{
    if (NULL == callNumber)
    {
        LE_ERROR("Phone number NULL");
        return LE_FAULT;
    }
    int i = 0;
    int numLength = strlen(callNumber);
    if (numLength+1 > MAX_DESTINATION_LEN)
    {
        LE_INFO("The number is too long!");
        return LE_FAULT;
    }
    for (i = 0; i <= numLength-1; i++)
    {
        char dig = *callNumber;
        if(!isdigit(dig))
        {
            LE_INFO("The input contains non-digit symbol %c", dig);
            return LE_FAULT;
        }
        callNumber++;
    }
    return LE_OK;
}

le_result_t ut_tafVoiceCall_ValidCall_Start()
{
    int fd = -1;
    ssize_t readCnt = 0;

    if (access("/tmp/CallNumber", 0) == 0)
    {
        fd = le_fd_Open("/tmp/CallNumber",  O_RDONLY);
        if (fd < 0)
        {
            LE_INFO("open call number file failed!");
            le_utf8_Copy(AppCtx.destId, "10010", MAX_DESTINATION_LEN, NULL);
        }
        else
        {
            readCnt = le_fd_Read(fd, AppCtx.destId, sizeof(AppCtx.destId));
            AppCtx.destId[MAX_DESTINATION_LEN-1] = '\0';
            if (readCnt != strlen(AppCtx.destId))
            {
                LE_INFO("read call number file failed! %" PRIuS" %" PRIuS, readCnt, strlen(AppCtx.destId));
                le_utf8_Copy(AppCtx.destId, "10010", MAX_DESTINATION_LEN, NULL);
            }
            else
            {
                if(isCallNumberValid(AppCtx.destId) != LE_OK)
                {
                    LE_INFO("Phone number is not valid and use the default call number!");
                    le_utf8_Copy(AppCtx.destId, "10010", MAX_DESTINATION_LEN, NULL);
                }
            }
        }
        le_fd_Close(fd);
    }
    else
    {
        le_utf8_Copy(AppCtx.destId, "10010", MAX_DESTINATION_LEN, NULL);
    }

    AppCtx.phoneId = 1;
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_makecall, &AppCtx, NULL);

    // the first event came from ut_tafVoiceCall_makecall
    LE_ASSERT_OK(wait_call(5));
    LE_ASSERT(AppCtx.requestRef != NULL);

    // the 2nd event is alerting
    LE_ASSERT_OK(wait_call(60));
    LE_ASSERT((TAF_VOICECALL_EVENT_ALERTING == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    LE_INFO("===== waiting for remote party to answer this call =====");
    // the 3rd event is active
    LE_ASSERT_OK(wait_call(120));
    LE_ASSERT((TAF_VOICECALL_EVENT_ACTIVE == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    return LE_OK;
}

le_result_t ut_tafVoiceCall_ValidCall_get_endcause()
{
    AppCtx.appCause = TAF_VOICECALL_END_REMOTE;
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_get_endcause, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(5));
    LE_ASSERT(AppCtx.appCause != TAF_VOICECALL_END_REMOTE);
    return LE_OK;
}

le_result_t ut_tafVoiceCall_Invalid_get_endcause()
{
    AppCtx.appCause = TAF_VOICECALL_END_REMOTE;
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_get_endcause_invalid, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(5));
    // Actually framework will reset AppCtx.appCause
    //LE_INFO("AppCtx.appCause： %d", (uint32_t)AppCtx.appCause);
    //LE_ASSERT(AppCtx.appCause == TAF_VOICECALL_END_REMOTE);
    return LE_OK;
}

le_result_t ut_tafVoiceCall_InvalidCall()
{
    le_utf8_Copy(AppCtx.destId, "11111", MAX_DESTINATION_LEN, NULL);
    AppCtx.phoneId = 1;
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_makecall, &AppCtx, NULL);

    // the first event came from ut_tafVoiceCall_makecall
    LE_ASSERT_OK(wait_call(1));
    LE_ASSERT(AppCtx.requestRef != NULL);

    // the 2nd event is alerting
    LE_ASSERT_OK(wait_call(60));
    LE_ASSERT((TAF_VOICECALL_EVENT_ALERTING == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    // the 3rd event is ended
    LE_ASSERT_OK(wait_call(120));
    LE_ASSERT((TAF_VOICECALL_EVENT_ENDED == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    AppCtx.appCause = -1;
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_get_endcause, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(1));
    LE_ASSERT(AppCtx.appCause != -1);

    return LE_OK;
}

le_result_t ut_tafVoiceCall_CallWaiting()
{
    taf_voicecall_CallRef_t LocalCallRef = AppCtx.requestRef;
    LE_INFO("===== waiting for the second incoming call =====");
    LE_ASSERT_OK(wait_call(60));
    LE_ASSERT(TAF_VOICECALL_EVENT_WAITING == LocalExpectEvent);

    AppCtx.requestRef = LocalExpectCallWaitingRef;
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_answer, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(10));
    LE_ASSERT(((TAF_VOICECALL_EVENT_ONHOLD == LocalExpectEvent) && (LocalCallRef == LocalExpectCallRef))
              ||((TAF_VOICECALL_EVENT_ACTIVE == LocalExpectEvent) && (LocalExpectCallWaitingRef == AppCtx.requestRef))
              );
    LE_ASSERT_OK(wait_call(10));
    LE_ASSERT(((TAF_VOICECALL_EVENT_ONHOLD == LocalExpectEvent) && (LocalCallRef == LocalExpectCallRef))
              ||((TAF_VOICECALL_EVENT_ACTIVE == LocalExpectEvent) && (LocalExpectCallWaitingRef == AppCtx.requestRef))
              );
    LE_INFO("===== Answer the second call done =====");

    le_thread_Sleep(5);

    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_swap, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(10));
    LE_ASSERT(((TAF_VOICECALL_EVENT_ACTIVE == LocalExpectEvent) && (LocalCallRef == LocalExpectCallRef))
              ||((TAF_VOICECALL_EVENT_ONHOLD == LocalExpectEvent) && (LocalExpectCallWaitingRef == AppCtx.requestRef))
              );
    LE_ASSERT_OK(wait_call(10));
    LE_ASSERT(((TAF_VOICECALL_EVENT_ACTIVE == LocalExpectEvent) && (LocalCallRef == LocalExpectCallRef))
              ||((TAF_VOICECALL_EVENT_ONHOLD == LocalExpectEvent) && (LocalExpectCallWaitingRef == AppCtx.requestRef))
              );
    LE_INFO("===== swap call done =====");

    le_thread_Sleep(5);

    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_swap, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(10));
    LE_ASSERT(((TAF_VOICECALL_EVENT_ONHOLD == LocalExpectEvent) && (LocalCallRef == LocalExpectCallRef))
           ||((TAF_VOICECALL_EVENT_ACTIVE == LocalExpectEvent) && (LocalExpectCallWaitingRef == AppCtx.requestRef))
           );
    LE_ASSERT_OK(wait_call(10));
    LE_ASSERT(((TAF_VOICECALL_EVENT_ONHOLD == LocalExpectEvent) && (LocalCallRef == LocalExpectCallRef))
           ||((TAF_VOICECALL_EVENT_ACTIVE == LocalExpectEvent) && (LocalExpectCallWaitingRef == AppCtx.requestRef))
           );
    LE_INFO("===== swap call done =====");

    le_thread_Sleep(5);

    LE_INFO("===== end one call =====");
    ut_tafVoiceCall_ValidCall_End();

    LE_INFO("===== end the another call =====");
    AppCtx.requestRef = LocalCallRef;
    ut_tafVoiceCall_ValidCall_End();
    return LE_OK;
}

static void ut_tafVoiceCall_ReturnFailedvalue(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_Hold(appCtxPtr->requestRef);
    if (leRet == LE_NOT_FOUND)
    {
      LE_INFO("taf_voicecall_Hold return Cannot found callCtx");
    }

    leRet = taf_voicecall_Resume(appCtxPtr->requestRef);
    if (leRet == LE_NOT_FOUND)
    {
      LE_INFO("taf_voicecall_Resume return Cannot found callCtx");
    }

    leRet = taf_voicecall_Swap(appCtxPtr->requestRef);
    if (leRet == LE_NOT_FOUND)
    {
      LE_INFO("taf_voicecall_Swap return Cannot found callCtx");
    }

    leRet = taf_voicecall_Answer(appCtxPtr->requestRef);
    if (leRet == LE_NOT_FOUND)
    {
      LE_INFO("taf_voicecall_Answer return Cannot found callCtx");
    }

    leRet = taf_voicecall_End(appCtxPtr->requestRef);
    if (leRet == LE_NOT_FOUND)
    {
      LE_INFO("taf_voicecall_End return Cannot found callCtx");
    }

    leRet = taf_voicecall_Delete(appCtxPtr->requestRef);
    if (leRet == LE_NOT_FOUND)
    {
      LE_INFO("taf_voicecall_Delete return Cannot found callCtx");
    }

    return;
}

le_result_t ut_tafVoiceCall_InvalidCall_ReturnFailedvalue()
{
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_ReturnFailedvalue, &AppCtx, NULL);
    LE_INFO("===== InvalidCall_ReturnFailedvalue =====");
    wait_call(5);
    return LE_OK;
}

static void* UnitTestThread
(
    void* contextPtr
)
{
    memset(&AppCtx, 0, sizeof(UnitTestContext_t));
//    le_result_t leRet;

    AppCtx.semaphore = le_sem_Create("tafvoiceCallSem", 0);

    LE_INFO("===== call number can be configured at /tmp/callnumber =====");
    // handler test
    LE_INFO("===== state handler test =====");
    AppCtx.threadRef = le_thread_Create("taf_voiceCall_handler", ut_tafVoiceCall_StateHandler, &AppCtx);
    le_thread_Start(AppCtx.threadRef);

    wait_call(5);

    LE_INFO("===== invalid call test =====");
    ut_tafVoiceCall_InvalidCall();

    // delete this call
    LE_INFO("===== delete call test =====");
    ut_tafVoiceCall_Call_Delete();

    LE_INFO("===== start valid call test =====");
    ut_tafVoiceCall_ValidCall_Start();
    le_thread_Sleep(3);

    LE_INFO("===== hold call test =====");
    ut_tafVoiceCall_ValidCall_Hold();
    le_thread_Sleep(3);

    LE_INFO("===== resume call test =====");
    ut_tafVoiceCall_ValidCall_Resume();

    LE_INFO("===== end call test =====");
    ut_tafVoiceCall_Invalid_get_endcause();
    ut_tafVoiceCall_ValidCall_End();
    ut_tafVoiceCall_ValidCall_get_endcause();

    // delete this call
    LE_INFO("===== delete call test =====");
    ut_tafVoiceCall_Call_Delete();
    ut_tafVoiceCall_Invalid_get_endcause();

    LE_INFO("===== incoming call test =====");
    ut_tafVoiceCall_IncomingCall();
    ut_tafVoiceCall_ValidCall_End();
    ut_tafVoiceCall_ValidCall_get_endcause();
    ut_tafVoiceCall_Call_Delete();

    LE_INFO("===== call waiting test =====");
    ut_tafVoiceCall_ValidCall_Start();
    ut_tafVoiceCall_CallWaiting();
    ut_tafVoiceCall_Call_Delete();

    LE_INFO("===== call return failed value test =====");
    ut_tafVoiceCall_InvalidCall_ReturnFailedvalue();

    LE_INFO("===== taf voice call test done=====");

    exit(EXIT_SUCCESS);
    return NULL;
}

COMPONENT_INIT
{
    le_thread_Start(le_thread_Create("Taf_voicecall_ut", UnitTestThread, NULL));
}

