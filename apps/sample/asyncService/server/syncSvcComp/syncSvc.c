/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

static le_event_Id_t EventId = NULL;                    // For events.

//--------------------------------------------------------------------------------------------------
/**
 * Event data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char msg[64];                    // message content in the event.
}EvtData_t;

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer for event handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerEventHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    EvtData_t* evtDataPtr = (EvtData_t*)reportPtr;

    if (evtDataPtr != NULL)
    {
        syncSvcDemo_MyHdlrFunc_t clientHandlerFunc =
            (syncSvcDemo_MyHdlrFunc_t)secondLayerHandlerFunc;
        clientHandlerFunc(evtDataPtr->msg, le_event_GetContextPtr());
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Timer handler to producer events regularly.
 */
//--------------------------------------------------------------------------------------------------
static void TimerHandler
(
    le_timer_Ref_t timerRef
)
{
    static uint32_t MsgCnt = 0;
    EvtData_t evtData;

    // Generate the event and notify clients.
    snprintf(evtData.msg, sizeof(evtData.msg), "Test syncSvr event 0x%x", MsgCnt++);
    le_event_Report(EventId, &evtData, sizeof(EvtData_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Main function for event producer thread.
 *
 * Produce an event every 1000ms
 */
//--------------------------------------------------------------------------------------------------
static void* EventProducerThread
(
    void* context
)
{
    LE_UNUSED(context);

    le_timer_Ref_t timerRef = le_timer_Create("ProducerEventTimer");
    le_timer_SetMsInterval(timerRef, 1000);
    le_timer_SetHandler(timerRef, TimerHandler);
    le_timer_SetRepeat(timerRef, 0);
    le_timer_SetWakeup(timerRef, false);
    le_timer_Start(timerRef);

    LE_INFO("Event producer thread is now running.");
    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'mySvcDemo_MyEvt'
 */
//--------------------------------------------------------------------------------------------------
syncSvcDemo_MyEvtHandlerRef_t syncSvcDemo_AddMyEvtHandler
(
    syncSvcDemo_MyHdlrFunc_t hdlrPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    le_event_HandlerRef_t handlerRef = NULL;

    if (hdlrPtr != NULL)
    {
        handlerRef = le_event_AddLayeredHandler("EventHandler", EventId,
                                                FirstLayerEventHandler,
                                                (le_event_HandlerFunc_t)hdlrPtr);

        le_event_SetContextPtr(handlerRef, contextPtr);
    }

    return (syncSvcDemo_MyEvtHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'mySvcDemo_MyEvt'
 */
//--------------------------------------------------------------------------------------------------
void syncSvcDemo_RemoveMyEvtHandler
(
    syncSvcDemo_MyEvtHandlerRef_t handlerRef
        ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Command received from client.
 */
//--------------------------------------------------------------------------------------------------
le_result_t syncSvcDemo_MyCmd
(
    uint8_t secs
        ///< [IN]
)
{
    // Sleep the required seconds. This will block service main thread thus notifications queuqed
    // in service main thread will be delayed to deliver.
    le_thread_Sleep(secs);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Component Init function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    EventId = le_event_CreateId("ProducerEvent", sizeof(EvtData_t));
    le_thread_Start(le_thread_Create("EventProducerThread", EventProducerThread, NULL));
}
