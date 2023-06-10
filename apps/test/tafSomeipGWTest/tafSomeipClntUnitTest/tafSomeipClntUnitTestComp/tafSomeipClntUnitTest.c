/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define TEST_SERVICE_ID1      0x1234
#define TEST_SERVICE_ID2      0x2345
#define TEST_INSTANCE_ID      0x5678

#define TEST_METHOD_ID        0x2000
#define TEST_METHOD_ID1       0x3000

#define TEST_EVENT_ID         0x8777
#define TEST_EVENT_ID1        0x8888
#define TEST_EVENT_ID2        0x9999

#define TEST_EVENTGROUP_ID    0x4465
#define TEST_EVENTGROUP_ID1   0x7795


static taf_someipClnt_ServiceRef_t ServiceRef1 = NULL;
static taf_someipClnt_ServiceRef_t ServiceRef2 = NULL;
static taf_someipClnt_StateChangeHandlerRef_t StateHandlerRef1 = NULL;
static taf_someipClnt_StateChangeHandlerRef_t StateHandlerRef2 = NULL;
taf_someipClnt_EventMsgHandlerRef_t EventHandleRef;
taf_someipClnt_EventMsgHandlerRef_t EventHandlerRef1;
static le_timer_Ref_t TimerRef = NULL;

typedef struct
{
   le_thread_Ref_t threadRef;
   le_sem_Ref_t semRef;
   le_thread_Destructor_t destructorFunc;
   le_timer_Ref_t timerRef;
   taf_someipClnt_ServiceRef_t serviceRef1;
   taf_someipClnt_ServiceRef_t serviceRef2;
   taf_someipClnt_StateChangeHandlerRef_t stateHandlerRef1;
   taf_someipClnt_StateChangeHandlerRef_t stateHandlerRef2;
   taf_someipClnt_EventMsgHandlerRef_t eventHandleRef;
   taf_someipClnt_EventMsgHandlerRef_t eventHandlerRef1;
}
RequestThreadCxt_t;

RequestThreadCxt_t RequestCtx = { 0 };

//--------------------------------------------------------------------------------------------------
/**
 * Cleanup mainThread resources.
 */
//--------------------------------------------------------------------------------------------------
static void CleanupResources
(
    void
)
{
    // Remove the handlers.
    taf_someipClnt_RemoveStateChangeHandler(StateHandlerRef1);
    taf_someipClnt_RemoveStateChangeHandler(StateHandlerRef2);
    taf_someipClnt_RemoveEventMsgHandler(EventHandleRef);
    taf_someipClnt_RemoveEventMsgHandler(EventHandlerRef1);

    // Release the service.
    taf_someipClnt_ReleaseService(ServiceRef1);
    taf_someipClnt_ReleaseService(ServiceRef2);
}

//--------------------------------------------------------------------------------------------------
/**
 * SOMEIP state change Handler
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void StateChangeHandler
(
    taf_someipClnt_ServiceRef_t serviceRef,
    taf_someipClnt_State_t state,
    void* contextPtr
)
{
    LE_UNUSED(contextPtr);
    uint16_t serviceId;
    uint16_t instanceId;

    if ((serviceRef == ServiceRef1) || (serviceRef == RequestCtx.serviceRef1))
    {
        serviceId = TEST_SERVICE_ID1;
        instanceId = TEST_INSTANCE_ID;
    }
    else if ((serviceRef == ServiceRef2) || (serviceRef == RequestCtx.serviceRef2))
    {
        serviceId = TEST_SERVICE_ID2;
        instanceId = TEST_INSTANCE_ID;
    }
    else
    {
        LE_ERROR("Unknown serviceRef(%p).", serviceRef);
        return;
    }

    // Log the service state.
    LE_TEST_INFO("Service(0x%x/0x%x) State is %s.", serviceId, instanceId,
                 state == TAF_SOMEIPCLNT_AVAILABLE ? "AVAILABLE" : "UNAVAILABLE");

    // Get the service version once it's available.
    if (state == TAF_SOMEIPCLNT_AVAILABLE)
    {
        uint8_t majVer;
        uint32_t minVer;

        le_result_t result = taf_someipClnt_GetVersion(serviceRef, &majVer, &minVer);
        LE_TEST_ASSERT(result == LE_OK, "ServiceApiTest taf_someipClnt_GetVersion() API.");

        LE_TEST_INFO("Service(0x%x/0x%x) Version is '0x%x/0x%x'.",
                     serviceId, instanceId, majVer, minVer);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * SOMEIP event message Handler
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void EventMsgHandler
(
    taf_someipClnt_ServiceRef_t serviceRef,
    uint16_t eventId,
    const uint8_t* dataPtr,
    size_t dataSize,
    void* contextPtr
)
{
    LE_UNUSED(contextPtr);
    uint16_t serviceId;
    uint16_t instanceId;

    if ((serviceRef == ServiceRef2) || (serviceRef == RequestCtx.serviceRef2))
    {
        serviceId = TEST_SERVICE_ID2;
        instanceId = TEST_INSTANCE_ID;
    }
    else if ((serviceRef == ServiceRef1) || (serviceRef == RequestCtx.serviceRef1))
    {
        serviceId = TEST_SERVICE_ID1;
        instanceId = TEST_INSTANCE_ID;
    }
    else
    {
        LE_ERROR("Unknown serviceRef(%p).", serviceRef);
        return;
    }

    // Log the event information.
    LE_TEST_INFO("EVENT(0x%x) of service(0x%x/0x%x) received.", eventId, serviceId, instanceId);

    // Dump the payload data if it's not empty.
    if (dataSize != 0)
    {
        char PayloadString[2*dataSize + 1];
        le_hex_BinaryToString(dataPtr, dataSize, PayloadString, 2*dataSize + 1);
        LE_TEST_INFO("EVENT PAYLOAD [%s]", PayloadString);
    }

    // If the terminate event is recieved by main thread, cancel the sub thread
    // and terminates the client test app.
    if ((serviceRef == ServiceRef1) && (eventId == TEST_EVENT_ID2) &&
        (dataSize == 2) && (dataPtr[0] == 0xff) && (dataPtr[1] == 0xff))
    {
        LE_TEST_INFO("Termination event received.");

        CleanupResources();

        if (RequestCtx.threadRef)
        {
            LE_ASSERT(LE_OK == le_thread_Cancel(RequestCtx.threadRef));
            LE_ASSERT(LE_OK == le_thread_Join(RequestCtx.threadRef, NULL));
            LE_TEST_INFO("EventApiTest thread is stopped.");
        }

        LE_TEST_INFO("========================================");
        LE_TEST_INFO("=== telaf someip client API test END ===");
        LE_TEST_INFO("========================================");

        LE_TEST_EXIT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Sub Thread Destructor.
 */
//--------------------------------------------------------------------------------------------------
static void ThreadDestructor
(
    void* paramPtr
)
{
    RequestThreadCxt_t* requestCtxPtr = paramPtr;
    LE_ASSERT(requestCtxPtr != NULL);

    // Stop and delete the timer.
    if (requestCtxPtr->timerRef != NULL)
    {
        le_timer_Delete(requestCtxPtr->timerRef);
        requestCtxPtr->timerRef = NULL;
    }

    // Delete the sem.
    le_sem_Delete(requestCtxPtr->semRef);

    // Remove the handlers.
    taf_someipClnt_RemoveStateChangeHandler(requestCtxPtr->stateHandlerRef1);
    taf_someipClnt_RemoveStateChangeHandler(requestCtxPtr->stateHandlerRef2);
    taf_someipClnt_RemoveEventMsgHandler(requestCtxPtr->eventHandleRef);
    taf_someipClnt_RemoveEventMsgHandler(requestCtxPtr->eventHandlerRef1);

    // Unsubscribe event groups.
    taf_someipClnt_UnsubscribeEventGroup(requestCtxPtr->serviceRef2, TEST_EVENTGROUP_ID);
    taf_someipClnt_UnsubscribeEventGroup(requestCtxPtr->serviceRef1, TEST_EVENTGROUP_ID1);

    // Disable event groups.
    taf_someipClnt_DisableEventGroup(requestCtxPtr->serviceRef2, TEST_EVENTGROUP_ID);
    taf_someipClnt_DisableEventGroup(requestCtxPtr->serviceRef1, TEST_EVENTGROUP_ID1);

    // Release the service.
    taf_someipClnt_ReleaseService(requestCtxPtr->serviceRef1);
    taf_someipClnt_ReleaseService(requestCtxPtr->serviceRef2);

    // Disconnect the telaf SOME/IP GW service.
    taf_someipClnt_DisconnectService();
}

//--------------------------------------------------------------------------------------------------
/**
 * Test thread.
 */
//--------------------------------------------------------------------------------------------------
static void* EventApiThread
(
    void* context
)
{
    RequestThreadCxt_t* requestCtxPtr = context;
    LE_ASSERT(requestCtxPtr != NULL);

    // Connect service in sub thread.
    taf_someipClnt_ConnectService();

    // Set thread destructor so that when the thread gets exited, we can do some clearup work.
    le_thread_AddDestructor(requestCtxPtr->destructorFunc, (void *)requestCtxPtr);

    // Request the services and register state handlers.
    requestCtxPtr->serviceRef1 = taf_someipClnt_RequestService(TEST_SERVICE_ID1, TEST_INSTANCE_ID);
    requestCtxPtr->serviceRef2 = taf_someipClnt_RequestService(TEST_SERVICE_ID2, TEST_INSTANCE_ID);
    requestCtxPtr->stateHandlerRef1 =
        taf_someipClnt_AddStateChangeHandler(requestCtxPtr->serviceRef1, StateChangeHandler, NULL);
    requestCtxPtr->stateHandlerRef2 =
        taf_someipClnt_AddStateChangeHandler(requestCtxPtr->serviceRef2, StateChangeHandler, NULL);

    // Set up the event and group.
    LE_TEST_ASSERT(LE_OK == taf_someipClnt_EnableEventGroup(requestCtxPtr->serviceRef2,
                                                            TEST_EVENTGROUP_ID,
                                                            TEST_EVENT_ID,
                                                            TAF_SOMEIPDEF_ET_FIELD),
                   "Test taf_someipClnt_EnableEventGroup() API.");

    LE_TEST_ASSERT(LE_OK == taf_someipClnt_EnableEventGroup(requestCtxPtr->serviceRef2,
                                                            TEST_EVENTGROUP_ID,
                                                            TEST_EVENT_ID1,
                                                            TAF_SOMEIPDEF_ET_EVENT),
                   "Test taf_someipClnt_EnableEventGroup() API.");

    LE_TEST_ASSERT(LE_OK == taf_someipClnt_EnableEventGroup(requestCtxPtr->serviceRef1,
                                                            TEST_EVENTGROUP_ID1,
                                                            TEST_EVENT_ID2,
                                                            TAF_SOMEIPDEF_ET_EVENT),
                   "Test taf_someipClnt_EnableEventGroup() API.");


    // Subscribe the event group.
    LE_TEST_ASSERT(LE_OK == taf_someipClnt_SubscribeEventGroup(requestCtxPtr->serviceRef2,
                                                               TEST_EVENTGROUP_ID),
                   "Test taf_someipClnt_SubscribeEventGroup() API.");

    LE_TEST_ASSERT(LE_OK == taf_someipClnt_SubscribeEventGroup(requestCtxPtr->serviceRef1,
                                                               TEST_EVENTGROUP_ID1),
                   "Test taf_someipClnt_SubscribeEventGroup() API.");

    // Add event message handler.
    requestCtxPtr->eventHandleRef = taf_someipClnt_AddEventMsgHandler(requestCtxPtr->serviceRef2,
                                                                      TEST_EVENTGROUP_ID,
                                                                      EventMsgHandler, NULL);
    LE_TEST_ASSERT(requestCtxPtr->eventHandleRef != NULL,
                   "Test taf_someipClnt_AddEventMsgHandler() API.");

    requestCtxPtr->eventHandlerRef1 = taf_someipClnt_AddEventMsgHandler(requestCtxPtr->serviceRef1,
                                                                        TEST_EVENTGROUP_ID1,
                                                                        EventMsgHandler, NULL);
    LE_TEST_ASSERT(requestCtxPtr->eventHandlerRef1 != NULL,
                   "Test taf_someipClnt_AddEventMsgHandler() API.");

    requestCtxPtr->timerRef = NULL;

    // Unblock the main thread.
    le_sem_Post(requestCtxPtr->semRef);

    // Run event loop of the sub thread.
    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests for Event APIs.
 */
//--------------------------------------------------------------------------------------------------
static void EventApiTest
(
    void
)
{
    RequestCtx.semRef = le_sem_Create("EventApiTestSem", 0);
    RequestCtx.destructorFunc = ThreadDestructor;
    RequestCtx.threadRef = le_thread_Create("EventApiTestThread",
                                            EventApiThread, (void*)&RequestCtx);
    le_thread_SetJoinable(RequestCtx.threadRef);
    le_thread_Start(RequestCtx.threadRef);

    le_sem_Wait(RequestCtx.semRef);
    LE_TEST_INFO("EventApiTest thread is running.");
}

//--------------------------------------------------------------------------------------------------
/**
 * SOMEIP response message handler.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void ResponseHandler
(
    le_result_t result,
    bool isErrRsp,
    uint8_t returnCode,
    const uint8_t* dataPtr,
    size_t dataSize,
    void* contextPtr
)
{
    LE_INFO("Response handler: [%s] [%s] [%d]", LE_RESULT_TXT(result),
            isErrRsp ? "MT_ERROR" : "MT_RESPONSE", returnCode);

    if ((result == LE_OK) && (dataPtr != NULL) && (dataSize != 0))
    {
        char PayloadString[2*dataSize + 1];
        le_hex_BinaryToString(dataPtr, dataSize, PayloadString, 2*dataSize + 1);
        LE_TEST_INFO("RESPONSE PAYLOAD [%s]", PayloadString);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The handler for cyclical request.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void TimerHandler
(
    le_timer_Ref_t timerRef
)
{
    static uint32_t tms = 0;
    static uint8_t itsData[20] = {0};
    static uint32_t itsSize = 0;
    taf_someipClnt_ServiceRef_t serviceRef = le_timer_GetContextPtr(timerRef);
    uint16_t serviceId = TEST_SERVICE_ID1;
    uint16_t instanceId = TEST_INSTANCE_ID;

    LE_ASSERT(serviceRef == ServiceRef1);

    // Create a request message.
    taf_someipClnt_TxMsgRef_t txMsgRef = taf_someipClnt_CreateMsg(serviceRef, TEST_METHOD_ID);
    LE_ASSERT(txMsgRef != NULL);

    // Set the timeout for response.
    tms++;
    LE_TEST_ASSERT(LE_OK == taf_someipClnt_SetTimeout(txMsgRef, 10*tms),
                   "Test taf_someipClnt_SetTimeout(%dms) API.", 10*tms);

    // Set the payload for the txMsg.
    itsSize++;
    for (uint8_t i = 0; i < itsSize; ++i)
    {
        itsData[i] = i;
    }
    LE_TEST_ASSERT(LE_OK == taf_someipClnt_SetPayload(txMsgRef, itsData, itsSize),
                   "Test taf_someipClnt_SetPayload() API.");

    // Send the request.
    LE_TEST_INFO("Sending Request(service=0x%x/0x%x/0x%x): size=(%d).",
            serviceId, instanceId, TEST_METHOD_ID, itsSize);
    taf_someipClnt_RequestResponse(txMsgRef, ResponseHandler, NULL);

    // Create and send a non-retrun request.
    LE_TEST_INFO("Sending a non-return Request(service=0x%x/0x%x/0x%x).",
            serviceId, instanceId, TEST_METHOD_ID1);
    taf_someipClnt_TxMsgRef_t nonRetMsgRef = taf_someipClnt_CreateMsg(serviceRef, TEST_METHOD_ID1);
    LE_TEST_ASSERT(LE_OK == taf_someipClnt_SetNonRet(nonRetMsgRef),
                   "Test taf_someipClnt_SetNonRet API.");
    taf_someipClnt_RequestResponse(nonRetMsgRef, ResponseHandler, NULL);

    // Reset the txMsg size.
    if (itsSize == sizeof(itsData))
    {
        itsSize = 0;
    }

    // Reset the tms.
    if (tms == 20)
    {
        tms = 0;
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests for service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void ServiceApiTest
(
    void
)
{
    taf_someipClnt_StateChangeHandlerRef_t handlerRef;
    taf_someipClnt_ServiceRef_t serviceRef1, serviceRef2, serviceRef3;
    taf_someipClnt_State_t state;
    le_result_t result = LE_OK;

    // Request the service and get the service reference.
    serviceRef1 = taf_someipClnt_RequestService(TEST_SERVICE_ID1, TEST_INSTANCE_ID);
    // Request the service again and will get the same reference.
    serviceRef2 = taf_someipClnt_RequestService(TEST_SERVICE_ID1, TEST_INSTANCE_ID);
    LE_TEST_ASSERT((serviceRef1 == serviceRef2) && (serviceRef1 != NULL),
                   "ServiceApiTest taf_someipClnt_RequestService() API.");

    // Request the second service.
    serviceRef3 = taf_someipClnt_RequestService(TEST_SERVICE_ID2, TEST_INSTANCE_ID);
    LE_TEST_ASSERT((serviceRef3 != serviceRef1) && (serviceRef3 != NULL),
                   "ServiceApiTest taf_someipClnt_RequestService() API.");

    // Get the first service state.
    result = taf_someipClnt_GetState(serviceRef1, &state);
    LE_TEST_ASSERT(result == LE_OK, "ServiceApiTest taf_someipClnt_GetState() API.");
    LE_TEST_INFO("Service(0x%x/0x%x): State(%s).", TEST_SERVICE_ID1, TEST_INSTANCE_ID,
                 state == TAF_SOMEIPCLNT_AVAILABLE ? "Available" : "Unavailable");

    // Get the second service state.
    result = taf_someipClnt_GetState(serviceRef3, &state);
    LE_TEST_ASSERT(result == LE_OK, "ServiceApiTest taf_someipClnt_GetState() API.");
    LE_TEST_INFO("Service(0x%x/0x%x): State(%s).", TEST_SERVICE_ID2, TEST_INSTANCE_ID,
                 state == TAF_SOMEIPCLNT_AVAILABLE ? "Available" : "Unavailable");

    // Register a state handler for the first service.
    handlerRef = taf_someipClnt_AddStateChangeHandler(serviceRef1, StateChangeHandler, NULL);
    LE_TEST_ASSERT(handlerRef != NULL,
                   "ServiceApiTest taf_someipClnt_AddStateChangeHandler() API.");

    // Release the first service after a state handler is registered. This shall get LE_BUSY
    // since there are active handlers registered.
    result = taf_someipClnt_ReleaseService(serviceRef1);
    LE_TEST_ASSERT(result == LE_BUSY, "ServiceApiTest taf_someipClnt_ReleaseService() APIs.");

    // Remove the handler.
    taf_someipClnt_RemoveStateChangeHandler(handlerRef);

    // The first service can be released after of its handlers are removed.
    result = taf_someipClnt_ReleaseService(serviceRef1);
    LE_TEST_ASSERT(result == LE_OK,
                   "ServiceApiTest taf_someipClnt_ReleaseService() APIs.");

    // Release the second service.
    result = taf_someipClnt_ReleaseService(serviceRef3);
    LE_TEST_ASSERT(result == LE_OK,
                   "ServiceApiTest taf_someipClnt_ReleaseService() APIs.");

    // Release the service again, this shall get LE_BAD_PARAMETER since the serviceRef is
    // invalid now.
    result = taf_someipClnt_ReleaseService(serviceRef1);
    LE_TEST_ASSERT(result == LE_BAD_PARAMETER,
                   "ServiceApiTest taf_someipClnt_ReleaseService() APIs.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests for Rx message APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void MessageApiTest
(
    void
)
{
    // Request the services and register state handlers.
    ServiceRef1 = taf_someipClnt_RequestService(TEST_SERVICE_ID1, TEST_INSTANCE_ID);
    ServiceRef2 = taf_someipClnt_RequestService(TEST_SERVICE_ID2, TEST_INSTANCE_ID);
    StateHandlerRef1 = taf_someipClnt_AddStateChangeHandler(ServiceRef1, StateChangeHandler, NULL);
    StateHandlerRef2 = taf_someipClnt_AddStateChangeHandler(ServiceRef2, StateChangeHandler, NULL);

    // Register event message handlers.
    EventHandleRef = taf_someipClnt_AddEventMsgHandler(ServiceRef2, TEST_EVENTGROUP_ID,
                                                       EventMsgHandler, NULL);
    EventHandlerRef1 = taf_someipClnt_AddEventMsgHandler(ServiceRef1, TEST_EVENTGROUP_ID1,
                                                         EventMsgHandler, NULL);

    // Start the send message timer.
    // Start a timer to periodically send the request message and get the response.
    TimerRef = le_timer_Create("mainThread: RequestResponse test Timer");
    le_timer_SetMsInterval(TimerRef, 10000);
    le_timer_SetHandler(TimerRef, TimerHandler);
    le_timer_SetRepeat(TimerRef, 0);
    le_timer_SetWakeup(TimerRef, false);
    le_timer_SetContextPtr(TimerRef, (void*)ServiceRef1);
    le_timer_Start(TimerRef);

    // Create a request message.
    taf_someipClnt_TxMsgRef_t txMsgRef =
        taf_someipClnt_CreateMsg(ServiceRef1, TEST_METHOD_ID);
    LE_TEST_ASSERT(txMsgRef != NULL, "Test taf_someipClnt_CreateMsg() API.");

    // Test DeleteMsg() API before sending the message.
    LE_TEST_ASSERT(LE_OK == taf_someipClnt_DeleteMsg(txMsgRef),
                   "Test taf_someipClnt_DeleteMsg() API before sending.");

    // Test DeleteMsg() API again before sending the message.
    LE_TEST_ASSERT(LE_OK != taf_someipClnt_DeleteMsg(txMsgRef),
                   "Test taf_someipClnt_DeleteMsg() API again before sending.");

    // Test CreateMsg() API again.
    txMsgRef = taf_someipClnt_CreateMsg(ServiceRef1, TEST_METHOD_ID);
    LE_TEST_ASSERT(txMsgRef != NULL, "Test taf_someipClnt_CreateMsg() API again.");

    // Test sending the empty request.
    taf_someipClnt_RequestResponse(txMsgRef, ResponseHandler, NULL);

    // Test DeleteMsg() API after sending the message, because the txMsg is released
    // automatically after sending.
    LE_TEST_ASSERT(LE_OK != taf_someipClnt_DeleteMsg(txMsgRef),
                   "Test taf_someipClnt_DeleteMsg() API after sending.");
}

COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("==========================================");
    LE_TEST_INFO("=== telaf someip client API test BEGIN ===");
    LE_TEST_INFO("==========================================");

    ServiceApiTest();
    EventApiTest();
    MessageApiTest();
}
