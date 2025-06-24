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
 *  Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define MAX_DESTINATION_LEN_BYTE    51
#define MAX_DESTINATION_LEN         50

typedef struct
{
#ifdef USE_LE_SEM
    le_sem_Ref_t                         semaphore;
#else
    sem_t                                rawSemaphore;
#endif
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

#ifdef USE_LE_SEM
le_result_t post_call_event()
{
    le_sem_Post(AppCtx.semaphore);
    return LE_OK;
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

#else
le_result_t post_call_event()
{
    int before = -1, after = -1;
    sem_getvalue(&AppCtx.rawSemaphore, &before);
    sem_post(&AppCtx.rawSemaphore);
    sem_getvalue(&AppCtx.rawSemaphore, &after);
    LE_INFO("Semaphore value: before post = %d, after post = %d", before, after);
    return LE_OK;
}

le_result_t wait_call_event(taf_voicecall_Event_t event, taf_voicecall_CallRef_t callRef, int seconds)
{
    struct timespec now, ts;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    {
        LE_ERROR("clock_gettime() failed: %s", strerror(errno));
        return LE_FAULT;
    }

    ts.tv_sec = now.tv_sec + seconds;
    ts.tv_nsec = now.tv_nsec;

    if (ts.tv_nsec >= 1000000000)
    {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }

    LocalExpectEvent = event;
    LocalExpectCallRef = callRef;

    LE_INFO("LocalExpectEvent: %d, LocalExpectCallRef: %p, callRef: %p", LocalExpectEvent, LocalExpectCallRef, callRef);

    int result = sem_clockwait(&AppCtx.rawSemaphore, CLOCK_MONOTONIC, &ts);

    if (result == -1)
    {
        if (errno == ETIMEDOUT)
        {
            LE_ERROR("timeout after %d seconds. errno: ETIMEDOUT (%d)", seconds, errno);
            return LE_TIMEOUT;
        }
        else
        {
            LE_ERROR("failed. errno: %d (%s)", errno, strerror(errno));
            return LE_FAULT;
        }
    }

    return LE_OK;
}

le_result_t wait_call(int seconds)
{
    struct timespec now, ts;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    {
        LE_ERROR("clock_gettime() failed: %s", strerror(errno));
        return LE_FAULT;
    }

    ts.tv_sec = now.tv_sec + seconds;
    ts.tv_nsec = now.tv_nsec;

    if (ts.tv_nsec >= 1000000000)
    {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }

    int before = -1, after = -1;
    sem_getvalue(&AppCtx.rawSemaphore, &before);
    int result = sem_clockwait(&AppCtx.rawSemaphore, CLOCK_MONOTONIC, &ts);
    if (result == -1)
    {
        if (errno == ETIMEDOUT)
        {
            LE_ERROR("wait_call() timeout after %d seconds. errno: ETIMEDOUT (%d)", seconds, errno);
            return LE_TIMEOUT;
        }
        else
        {
            LE_ERROR("wait_call() failed. errno: %d (%s)", errno, strerror(errno));
            return LE_FAULT;
        }
    }
    sem_getvalue(&AppCtx.rawSemaphore, &after);
    LE_INFO("Semaphore value: before wait = %d, after wait = %d", before, after);

    return LE_OK;
}
#endif

static void MyCallEventHandler(taf_voicecall_CallRef_t callRef, \
                               const char* id, \
                               taf_voicecall_Event_t callEvent, \
                               void* ctxPtr)
{
    LE_INFO("Get event: callEvent %d, callRef: %p, id: %s", (uint32_t)callEvent, callRef, id);

    if (callEvent != TAF_VOICECALL_EVENT_WAITING)
    {
        LocalExpectEvent = callEvent;
        LocalExpectCallRef = callRef;
        LE_INFO("Sem post for non-waiting");
        post_call_event();
    }
    else
    {
        LocalExpectEvent = callEvent;
        LocalExpectCallWaitingRef = callRef;
        LE_INFO("Sem post for waiting");
        post_call_event();
    }

    return;
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
    post_call_event();

    le_event_RunLoop();

    return NULL;
}

static void ut_tafVoiceCall_makecall(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;

    appCtxPtr->requestRef = taf_voicecall_Start(appCtxPtr->destId, appCtxPtr->phoneId);
    LE_ASSERT(appCtxPtr->requestRef != NULL);
    post_call_event();

    return;
}

static void ut_tafVoiceCall_get_endcause(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_GetEndCause(appCtxPtr->requestRef, &appCtxPtr->appCause);
    LE_INFO("End cause: %d", (int)appCtxPtr->appCause);
    LE_ASSERT(leRet == LE_OK);
    post_call_event();

    return;
}

static void ut_tafVoiceCall_get_endcause_invalid(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_GetEndCause(appCtxPtr->requestRef, &appCtxPtr->appCause);
    LE_ASSERT(leRet != LE_OK);
    post_call_event();

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

    post_call_event();

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

static void ut_tafVoiceCall_end_duplicated(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_End(appCtxPtr->requestRef);
    LE_ASSERT(leRet == LE_OK);
    post_call_event();

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
    LE_ASSERT_OK(wait_call(10));
    LE_ASSERT((TAF_VOICECALL_EVENT_ONHOLD == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_hold, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(10));
    LE_ASSERT((TAF_VOICECALL_EVENT_CALL_HOLD_FAILED == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));
    return LE_OK;
}

le_result_t ut_tafVoiceCall_ValidCall_Resume()
{
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_resume, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(10));
    LE_INFO("LocalExpectEvent %d, LocalExpectCallRef: %p, AppCtx.requestRef: %p", (uint32_t)LocalExpectEvent, LocalExpectCallRef, AppCtx.requestRef);
    LE_ASSERT((TAF_VOICECALL_EVENT_ACTIVE == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_resume, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(10));
    LE_ASSERT((TAF_VOICECALL_EVENT_CALL_RESUME_FAILED == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    return LE_OK;
}

le_result_t ut_tafVoiceCall_ValidCall_End()
{
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_end, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(10));
    LE_INFO("LocalExpectEvent %d, LocalExpectCallRef: %p, AppCtx.requestRef: %p", (uint32_t)LocalExpectEvent, LocalExpectCallRef, AppCtx.requestRef);
    LE_ASSERT((TAF_VOICECALL_EVENT_ENDED == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    /* duplicated call end should be returned LE_OK */
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_end_duplicated, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(10));

    return LE_OK;
}

le_result_t ut_tafVoiceCall_Call_Delete()
{
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_delete, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(10));

    return LE_OK;
}

le_result_t ut_tafVoiceCall_IncomingCall()
{
    LE_INFO("===== waiting for the incoming call =====");
    LE_ASSERT_OK(wait_call(120));
    LE_ASSERT(TAF_VOICECALL_EVENT_INCOMING == LocalExpectEvent);

    AppCtx.requestRef = LocalExpectCallRef;
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_answer, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(10));
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
            if (readCnt != strlen(AppCtx.destId))
            {
                LE_INFO("read call number file failed! %" PRIuS" %" PRIuS, readCnt, strlen(AppCtx.destId));
                le_utf8_Copy(AppCtx.destId, "10010", MAX_DESTINATION_LEN, NULL);
            }
            else
            {
                AppCtx.destId[readCnt-1] = '\0';
                if(isCallNumberValid(AppCtx.destId) != LE_OK)
                {
                    LE_INFO("Phone number %s is not valid and use the default call number!, readCnt:%ld", AppCtx.destId, readCnt);
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
    LE_ASSERT_OK(wait_call(10));
    LE_ASSERT(AppCtx.requestRef != NULL);

    // the 2nd event is dialing
    LE_ASSERT_OK(wait_call(60));
    LE_ASSERT((TAF_VOICECALL_EVENT_DIALING == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    // the 3nd event is alerting
    LE_ASSERT_OK(wait_call(60));
    LE_ASSERT((TAF_VOICECALL_EVENT_ALERTING == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    LE_INFO("===== waiting for remote party to answer this call =====");
    // the 4rd event is active
    LE_ASSERT_OK(wait_call(120));
    LE_ASSERT((TAF_VOICECALL_EVENT_ACTIVE == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    return LE_OK;
}

le_result_t ut_tafVoiceCall_ValidCall_get_endcause()
{
    AppCtx.appCause = TAF_VOICECALL_END_UNDEFINED;
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_get_endcause, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(10));
    LE_ASSERT(AppCtx.appCause != TAF_VOICECALL_END_UNDEFINED);
    return LE_OK;
}

le_result_t ut_tafVoiceCall_Invalid_get_endcause()
{
    AppCtx.appCause = TAF_VOICECALL_END_UNDEFINED;
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_get_endcause_invalid, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(10));
    // Actually framework will reset AppCtx.appCause
    //LE_INFO("AppCtx.appCause %d", (uint32_t)AppCtx.appCause);
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

    // the 2nd event is dialing
    LE_ASSERT_OK(wait_call(60));
    LE_ASSERT((TAF_VOICECALL_EVENT_DIALING == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    // the 3nd event is alerting
    LE_ASSERT_OK(wait_call(60));
    LE_ASSERT((TAF_VOICECALL_EVENT_ALERTING == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    // the 4rd event is ended
    LE_ASSERT_OK(wait_call(120));
    LE_ASSERT((TAF_VOICECALL_EVENT_ENDED == LocalExpectEvent) && (LocalExpectCallRef == AppCtx.requestRef));

    AppCtx.appCause = -1;
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_get_endcause, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(1));
    LE_ASSERT(AppCtx.appCause != -1);

    // the return value should be LE_OK even it is already ended
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_end, &AppCtx, NULL);

    // should still return LE_OK for duplicated calling stop API
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_end, &AppCtx, NULL);

    return LE_OK;
}




#define DECLARE_TIMESTAMP(name) \
 struct timespec name##_start, name##_end;

#define START_TIMESTAMP(name) \
 clock_gettime(CLOCK_MONOTONIC, &name##_start);

#define END_TIMESTAMP(name) \
    clock_gettime(CLOCK_MONOTONIC, &name##_end); \
    long sec_diff_##name = name##_end.tv_sec - name##_start.tv_sec; \
    long nsec_diff_##name = name##_end.tv_nsec - name##_start.tv_nsec; \
    if (nsec_diff_##name < 0) { \
        sec_diff_##name--; \
        nsec_diff_##name += 1000000000L; \
 } \
    LE_INFO("[TIME] %s took %ld.%09ld seconds", #name, sec_diff_##name, nsec_diff_##name);


le_result_t ut_tafVoiceCall_CallWaiting()
{
    taf_voicecall_CallRef_t firstCallRef = AppCtx.requestRef;

    DECLARE_TIMESTAMP(WaitSecondCall)
    START_TIMESTAMP(WaitSecondCall)
    LE_INFO("===== waiting for the second incoming call =====");
    LE_ASSERT_OK(wait_call(60));
    LE_ASSERT(TAF_VOICECALL_EVENT_WAITING == LocalExpectEvent);
    END_TIMESTAMP(WaitSecondCall)

    AppCtx.requestRef = LocalExpectCallWaitingRef;

    DECLARE_TIMESTAMP(AnswerSecondCall)
    START_TIMESTAMP(AnswerSecondCall)
    LE_INFO("===== answering second call =====");
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_answer, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(60));
    /* Two events from different phone number will be sent. So need to wati_call() two times */
    LE_ASSERT_OK(wait_call(60));
    LE_ASSERT(((TAF_VOICECALL_EVENT_ONHOLD == LocalExpectEvent) && (firstCallRef == LocalExpectCallRef))
    || ((TAF_VOICECALL_EVENT_ACTIVE == LocalExpectEvent) && (LocalExpectCallWaitingRef == AppCtx.requestRef)));
    END_TIMESTAMP(AnswerSecondCall)

    le_thread_Sleep(5);

    DECLARE_TIMESTAMP(Swap1)
    START_TIMESTAMP(Swap1)
    LE_INFO("===== first swap =====");
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_swap, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(60));
    /* Two events from different phone number will be sent. So need to wati_call() two times */
    LE_ASSERT_OK(wait_call(60));
    LE_ASSERT(((TAF_VOICECALL_EVENT_ACTIVE == LocalExpectEvent) && (firstCallRef == LocalExpectCallRef))
    || ((TAF_VOICECALL_EVENT_ONHOLD == LocalExpectEvent) && (LocalExpectCallWaitingRef == AppCtx.requestRef)));
    END_TIMESTAMP(Swap1)

    le_thread_Sleep(5);

    DECLARE_TIMESTAMP(Swap2)
    START_TIMESTAMP(Swap2)
    LE_INFO("===== second swap =====");
    le_event_QueueFunctionToThread(AppCtx.threadRef, ut_tafVoiceCall_swap, &AppCtx, NULL);
    LE_ASSERT_OK(wait_call(60));
    /* Two events from different phone number will be sent. So need to wati_call() two times */
    LE_ASSERT_OK(wait_call(60));
    LE_ASSERT(((TAF_VOICECALL_EVENT_ONHOLD == LocalExpectEvent) && (firstCallRef == LocalExpectCallRef))
    || ((TAF_VOICECALL_EVENT_ACTIVE == LocalExpectEvent) && (LocalExpectCallWaitingRef == AppCtx.requestRef)));
    END_TIMESTAMP(Swap2)

    le_thread_Sleep(5);

    DECLARE_TIMESTAMP(EndCalls)
    START_TIMESTAMP(EndCalls)
    LE_INFO("===== end one call =====");
    ut_tafVoiceCall_ValidCall_End();

    LE_INFO("===== end the another call =====");
    AppCtx.requestRef = firstCallRef;
    ut_tafVoiceCall_ValidCall_End();
    END_TIMESTAMP(EndCalls)

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
    wait_call(10);
    return LE_OK;
}

static void* UnitTestThread
(
    void* contextPtr
)
{
    memset(&AppCtx, 0, sizeof(UnitTestContext_t));
//    le_result_t leRet;

#ifdef USE_LE_SEM
    AppCtx.semaphore = le_sem_Create("tafvoiceCallSem", 0);
#else
    sem_init(&AppCtx.rawSemaphore, 0, 0);
#endif

    LE_INFO("===== call number can be configured at /tmp/callnumber =====");
    // handler test
    LE_INFO("===== state handler test =====");
    AppCtx.threadRef = le_thread_Create("stateThread", ut_tafVoiceCall_StateHandler, &AppCtx);
    le_thread_Start(AppCtx.threadRef);

    wait_call(10);

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

