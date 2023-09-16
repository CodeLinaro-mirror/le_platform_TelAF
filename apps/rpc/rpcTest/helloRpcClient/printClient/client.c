/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

static le_thread_Ref_t ThreadRef = NULL;
static le_timer_Ref_t TimerRef = NULL;
static le_timer_Ref_t Timer1Ref = NULL;
static uint32_t MsgCnt = 0;

static void TimerHandler
(
    le_timer_Ref_t timerRef
)
{
    char reqMsg[128] = { 0 };
    char rspMsg[128] = { 0 };

    snprintf(reqMsg, sizeof(reqMsg), "HelloWorld_0x%x", MsgCnt++);
    printer_Print(reqMsg, rspMsg, sizeof(rspMsg));

    LE_INFO("Sent request: '%s'", reqMsg);
    LE_INFO("Received response: '%s'", rspMsg);
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

    printer_ConnectService();

    Timer1Ref = le_timer_Create("Helloworld timer1");
    le_timer_SetMsInterval(Timer1Ref, 5000);
    le_timer_SetHandler(Timer1Ref, TimerHandler);
    le_timer_SetRepeat(Timer1Ref, 0);
    le_timer_SetWakeup(Timer1Ref, false);
    le_timer_Start(Timer1Ref);

    printer_AddChangeHandler(3, ChangeHandler, NULL);
    printer_AddChangeHandler(4, ChangeHandler, NULL);

    LE_INFO("HelloRPC client thread is running.");
    le_event_RunLoop();
}


COMPONENT_INIT
{
    TimerRef = le_timer_Create("Helloworld timer");
    le_timer_SetMsInterval(TimerRef, 5000);
    le_timer_SetHandler(TimerRef, TimerHandler);
    le_timer_SetRepeat(TimerRef, 0);
    le_timer_SetWakeup(TimerRef, false);
    le_timer_Start(TimerRef);

    printer_AddChangeHandler(1, ChangeHandler, NULL);
    printer_AddChangeHandler(2, ChangeHandler, NULL);

    ThreadRef = le_thread_Create("SubThread", ClientThread, NULL);
    le_thread_Start(ThreadRef);
}
