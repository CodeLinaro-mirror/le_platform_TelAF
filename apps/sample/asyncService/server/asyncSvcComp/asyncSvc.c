/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

static le_event_Id_t EventId = NULL;                    // For events.
static le_mem_PoolRef_t CmdPoolRef = NULL;              // Save ongoing client requests.
static le_thread_Ref_t ServiceThreadRef = NULL;         // Service main thread.
static le_thread_Ref_t WorkerThreadRef = NULL;          // Handle client requests.
static le_thread_Ref_t EventProducerThreadRef = NULL;   // Produce event data.
static le_timer_Ref_t TimerRef = NULL;                  // Event timer.


//--------------------------------------------------------------------------------------------------
/**
 * Command data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    asyncSvcDemo_ServerCmdRef_t cmdRef;  // Command reference from service layer.
    uint8_t reqParam;                    // Request parameter.
    le_result_t retCode;                 // Result code.
}CmdData_t;

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
        asyncSvcDemo_MyHdlrFunc_t clientHandlerFunc =
            (asyncSvcDemo_MyHdlrFunc_t)secondLayerHandlerFunc;
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
    snprintf(evtData.msg, sizeof(evtData.msg), "Test asyncSvr event 0x%x", MsgCnt++);
    le_event_Report(EventId, &evtData, sizeof(EvtData_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Main function for worker thread.
 */
//--------------------------------------------------------------------------------------------------
static void* WorkerThread
(
    void* context
)
{
    LE_UNUSED(context);

    LE_INFO("Worker thread is now running.");
    le_event_RunLoop();
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

    TimerRef = le_timer_Create("ProducerEventTimer");
    le_timer_SetMsInterval(TimerRef, 1000);
    le_timer_SetHandler(TimerRef, TimerHandler);
    le_timer_SetRepeat(TimerRef, 0);
    le_timer_SetWakeup(TimerRef, false);
    le_timer_Start(TimerRef);

    LE_INFO("Event producer thread is now running.");
    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Response handler which is called within service main thread to send back the response to clients.
 */
//--------------------------------------------------------------------------------------------------
static void RespHandler
(
    void* cmdPtr,
    void* context
)
{
    // Sanity check.
    LE_ASSERT(cmdPtr != NULL);
    LE_UNUSED(context);

    // Get the secs and cmdRef from command data.
    CmdData_t* cmdDataPtr = (CmdData_t*)cmdPtr;
    asyncSvcDemo_ServerCmdRef_t cmdRef = cmdDataPtr->cmdRef;
    le_result_t result = cmdDataPtr->retCode;

    // Send back the response to client.
    asyncSvcDemo_MyCmdRespond(cmdRef, result);

    // Free the command data object.
    le_mem_Release(cmdDataPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Command handler which is called within work thread context to process the client requests.
 */
//--------------------------------------------------------------------------------------------------
static void CmdHandler
(
    void* cmdPtr,
    void* context
)
{
    // Sanity check.
    LE_ASSERT(cmdPtr != NULL);
    LE_UNUSED(context);

    // Get the secs from the request parameter of command data.
    CmdData_t* cmdDataPtr = (CmdData_t*)cmdPtr;
    uint8_t secs = cmdDataPtr->reqParam;

    // Sleep the required seconds.
    le_thread_Sleep(secs);

    // Set the result code.
    cmdDataPtr->retCode = LE_OK;

    // Handler is done, forward the command response back to the service main thread.
    // This step is must-have because the command request is received from service main thread,
    // thus the response must be also sent within the same thread context.
    le_event_QueueFunctionToThread(ServiceThreadRef, RespHandler, cmdDataPtr, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Command received from client.
 */
//--------------------------------------------------------------------------------------------------
void asyncSvcDemo_MyCmd
(
    asyncSvcDemo_ServerCmdRef_t cmdRef,
    uint8_t secs ///< [IN]
)
{
    // Create a command object.
    CmdData_t* cmdDataPtr = le_mem_ForceAlloc(CmdPoolRef);
    memset(cmdDataPtr, 0, sizeof(CmdData_t));

    // Set command parameters.
    cmdDataPtr->cmdRef = cmdRef;
    cmdDataPtr->reqParam = secs;

    // Forward the command request to work thread to keep service main thread unblocked.
    le_event_QueueFunctionToThread(WorkerThreadRef, CmdHandler, cmdDataPtr, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'mySvcDemo_MyEvt'
 */
//--------------------------------------------------------------------------------------------------
asyncSvcDemo_MyEvtHandlerRef_t asyncSvcDemo_AddMyEvtHandler
(
    asyncSvcDemo_MyHdlrFunc_t hdlrPtr,
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

    return (asyncSvcDemo_MyEvtHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'mySvcDemo_MyEvt'
 */
//--------------------------------------------------------------------------------------------------
void asyncSvcDemo_RemoveMyEvtHandler
(
    asyncSvcDemo_MyEvtHandlerRef_t handlerRef
        ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Termination handler.
 */
//--------------------------------------------------------------------------------------------------
static void TerminateHandler
(
    int sigNum
)
{
    LE_INFO("Terminating AsyncSvc ...");

    // Stop the timer first
    if (TimerRef != NULL)
    {
        le_timer_Stop(TimerRef);
        le_timer_Delete(TimerRef);
    }

    // First cancel event thread.
    le_thread_Cancel(EventProducerThreadRef);
    le_thread_Join(EventProducerThreadRef, NULL);

    // Second cancel the worker thread.
    le_thread_Cancel(WorkerThreadRef);
    le_thread_Join(WorkerThreadRef, NULL);

    // Now we are safe to exit.
    exit(0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Component Init function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    EventId = le_event_CreateId("ProducerEvent", sizeof(EvtData_t));
    CmdPoolRef = le_mem_CreatePool("CommandPool", sizeof(CmdData_t));

    ServiceThreadRef = le_thread_GetCurrent();
    WorkerThreadRef = le_thread_Create("WorkerThread", WorkerThread, NULL);
    EventProducerThreadRef = le_thread_Create("EventProducerThread", EventProducerThread, NULL);

    le_thread_SetJoinable(WorkerThreadRef);
    le_thread_SetJoinable(EventProducerThreadRef);
    le_thread_Start(WorkerThreadRef);
    le_thread_Start(EventProducerThreadRef);

    le_sig_SetEventHandler(SIGTERM, TerminateHandler);
    LE_INFO("AsyncSvc started.");
}
