/*
 *  Copyright (c) 2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"

static le_thread_Ref_t ThreadRef = NULL;
static le_timer_Ref_t TimerRef = NULL;
static le_timer_Ref_t Timer1Ref = NULL;
static uint32_t MsgCnt = 0;
static uint16_t MySystemId = 0;

static void TimerHandler
(
    le_timer_Ref_t timerRef
)
{
    char reqMsg[128] = { 0 };
    char rspMsg[128] = { 0 };
    uint16_t rspSystemId = 0;

    snprintf(reqMsg, sizeof(reqMsg), "HelloWorld_0x%x", MsgCnt++);
    tafRpcPrinter_Print(MySystemId, reqMsg, &rspSystemId, rspMsg, sizeof(rspMsg));

    LE_INFO("Sent request: '%s'", reqMsg);
    LE_INFO("Received response from system(0x%x): '%s'", rspSystemId, rspMsg);
}

static void ChangeHandler
(
    uint32_t chgId,
    const char* evtMsg,
    void* contextPtr
)
{
    LE_UNUSED(contextPtr);

    LE_INFO("Received event: 'chgId=0x%x content=%s'", chgId, evtMsg);
}

static void* ClientThread
(
    void* context
)
{
    LE_UNUSED(context);

    tafRpcPrinter_ConnectService();

    Timer1Ref = le_timer_Create("Helloworld timer1");
    le_timer_SetMsInterval(Timer1Ref, 5000);
    le_timer_SetHandler(Timer1Ref, TimerHandler);
    le_timer_SetRepeat(Timer1Ref, 0);
    le_timer_SetWakeup(Timer1Ref, false);
    le_timer_Start(Timer1Ref);

    tafRpcPrinter_AddChangeHandler(3, ChangeHandler, NULL);
    tafRpcPrinter_AddChangeHandler(4, ChangeHandler, NULL);

    LE_INFO("HelloRPC client thread is running.");
    le_event_RunLoop();
}


COMPONENT_INIT
{
    MySystemId = taf_someipClnt_GetClientId();

    TimerRef = le_timer_Create("Helloworld timer");
    le_timer_SetMsInterval(TimerRef, 5000);
    le_timer_SetHandler(TimerRef, TimerHandler);
    le_timer_SetRepeat(TimerRef, 0);
    le_timer_SetWakeup(TimerRef, false);
    le_timer_Start(TimerRef);

    tafRpcPrinter_AddChangeHandler(1, ChangeHandler, NULL);
    tafRpcPrinter_AddChangeHandler(2, ChangeHandler, NULL);

    ThreadRef = le_thread_Create("SubThread", ClientThread, NULL);
    le_thread_Start(ThreadRef);
}
