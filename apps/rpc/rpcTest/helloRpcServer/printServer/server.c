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

typedef struct
{
    printer_ChangeHandlerRef_t ref;
    uint32_t chgId;
    printer_ChangeHandlerFunc_t handlefunc;
    void* context;
}ChangeHandler_t;

static le_mem_PoolRef_t ChangeHandlerPool = NULL;
static le_ref_MapRef_t ChangeHandlerRefMap = NULL;
static le_timer_Ref_t TimerRef = NULL;

void printer_Print
(
    const char* reqMsg,
    char* rspMsg,
    size_t rspMsgSize
)
{
    snprintf(rspMsg, rspMsgSize, "%s", reqMsg);

    LE_INFO("Received request: '%s'", reqMsg);
    LE_INFO("sent response: '%s'", rspMsg);
}

printer_ChangeHandlerRef_t printer_AddChangeHandler
(
    uint32_t chgId,
    printer_ChangeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    ChangeHandler_t* chgHandlerPtr = (ChangeHandler_t*)le_mem_ForceAlloc(ChangeHandlerPool);
    memset(chgHandlerPtr, 0, sizeof(ChangeHandler_t));

    chgHandlerPtr->chgId = chgId;
    chgHandlerPtr->handlefunc = handlerPtr;
    chgHandlerPtr->context = contextPtr;
    chgHandlerPtr->ref =
        (printer_ChangeHandlerRef_t)le_ref_CreateRef(ChangeHandlerRefMap, chgHandlerPtr);

    return chgHandlerPtr->ref;
}

void printer_RemoveChangeHandler
(
    printer_ChangeHandlerRef_t handlerRef
)
{
    ChangeHandler_t* chgHandlerPtr =
        (ChangeHandler_t*)le_ref_Lookup(ChangeHandlerRefMap, handlerRef);

    if (chgHandlerPtr != NULL)
    {
        le_ref_DeleteRef(ChangeHandlerRefMap, handlerRef);
        le_mem_Release(chgHandlerPtr);
    }
}

static void TimerHandler
(
    le_timer_Ref_t timerRef
)
{
    static uint32_t msgCnt = 0;
    char msgData[128] = { 0 };

    snprintf(msgData, sizeof(msgData), "Change message 0x%x", msgCnt++);

    le_ref_IterRef_t iterRef = le_ref_GetIterator(ChangeHandlerRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        ChangeHandler_t* chgHandlerPtr = (ChangeHandler_t*)le_ref_GetValue(iterRef);
        chgHandlerPtr->handlefunc(chgHandlerPtr->chgId, msgData, chgHandlerPtr->context);
    }

}

COMPONENT_INIT
{
    ChangeHandlerPool = le_mem_CreatePool("ChangeHandlerPool", sizeof(ChangeHandler_t));
    ChangeHandlerRefMap = le_ref_CreateMap("ChangeHandlerRefMap", 16);
    TimerRef = le_timer_Create("NotifyChanges timer");

    le_timer_SetMsInterval(TimerRef, 10000);
    le_timer_SetHandler(TimerRef, TimerHandler);
    le_timer_SetRepeat(TimerRef, 0);
    le_timer_SetWakeup(TimerRef, false);
    le_timer_Start(TimerRef);
}
