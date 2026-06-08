/*
 *  Copyright (c) 2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"

typedef struct
{
    tafRpcPrinter_ChangeHandlerRef_t ref;
    uint32_t chgId;
    tafRpcPrinter_ChangeHandlerFunc_t handlefunc;
    void* context;
}ChangeHandler_t;

static le_mem_PoolRef_t ChangeHandlerPool = NULL;
static le_ref_MapRef_t ChangeHandlerRefMap = NULL;
static le_timer_Ref_t TimerRef = NULL;
static uint16_t MySystemId = 0;

void tafRpcPrinter_Print
(
    uint16_t reqSystemId,
    const char* reqMsg,
    uint16_t* rspSystemIdPtr,
    char* rspMsg,
    size_t rspMsgSize
)
{
    snprintf(rspMsg, rspMsgSize, "%s", reqMsg);
    *rspSystemIdPtr = MySystemId;

    LE_INFO("Received request from system(0x%x): '%s'", reqSystemId, reqMsg);
    LE_INFO("sent response: '%s'", rspMsg);
}

tafRpcPrinter_ChangeHandlerRef_t tafRpcPrinter_AddChangeHandler
(
    uint32_t chgId,
    tafRpcPrinter_ChangeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    ChangeHandler_t* chgHandlerPtr = (ChangeHandler_t*)le_mem_ForceAlloc(ChangeHandlerPool);
    memset(chgHandlerPtr, 0, sizeof(ChangeHandler_t));

    chgHandlerPtr->chgId = chgId;
    chgHandlerPtr->handlefunc = handlerPtr;
    chgHandlerPtr->context = contextPtr;
    chgHandlerPtr->ref =
        (tafRpcPrinter_ChangeHandlerRef_t)le_ref_CreateRef(ChangeHandlerRefMap, chgHandlerPtr);

    return chgHandlerPtr->ref;
}

void tafRpcPrinter_RemoveChangeHandler
(
    tafRpcPrinter_ChangeHandlerRef_t handlerRef
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

    snprintf(msgData, sizeof(msgData), "Change message 0x%x from system(0x%x).",
             msgCnt++, MySystemId);

    le_ref_IterRef_t iterRef = le_ref_GetIterator(ChangeHandlerRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        ChangeHandler_t* chgHandlerPtr = (ChangeHandler_t*)le_ref_GetValue(iterRef);
        chgHandlerPtr->handlefunc(chgHandlerPtr->chgId, msgData, chgHandlerPtr->context);
    }

}

COMPONENT_INIT
{
    MySystemId = taf_someipClnt_GetClientId();

    ChangeHandlerPool = le_mem_CreatePool("ChangeHandlerPool", sizeof(ChangeHandler_t));
    ChangeHandlerRefMap = le_ref_CreateMap("ChangeHandlerRefMap", 16);
    TimerRef = le_timer_Create("NotifyChanges timer");

    le_timer_SetMsInterval(TimerRef, 10000);
    le_timer_SetHandler(TimerRef, TimerHandler);
    le_timer_SetRepeat(TimerRef, 0);
    le_timer_SetWakeup(TimerRef, false);
    le_timer_Start(TimerRef);
}
