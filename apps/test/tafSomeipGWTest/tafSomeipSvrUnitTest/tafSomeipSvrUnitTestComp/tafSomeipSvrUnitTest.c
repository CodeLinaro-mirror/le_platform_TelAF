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

#define EXIT_RX_MSG_CNT       50

const static uint8_t MajVersion = 0x12;
const static uint32_t MinVersion = 0x34567890;
const static uint16_t UdpPort1 = 12345;
const static uint16_t TcpPort1 = 12344;
const static uint16_t UdpPort2 = 54321;
const static uint16_t TcpPort2 = 44321;

static uint32_t RxMsgCnt = 0;
static taf_someipSvr_RxMsgHandlerRef_t RxMsgHandlerRef = NULL;
taf_someipSvr_SubscriptionHandlerRef_t SubsHandlerRef = NULL;
static taf_someipSvr_ServiceRef_t ServiceRef = NULL;

static uint8_t PayloadData[TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE];

typedef struct
{
   le_thread_Ref_t threadRef;
   le_sem_Ref_t semRef;
   taf_someipSvr_RxMsgHandlerRef_t rxMsgHandleRef;
   taf_someipSvr_SubscriptionHandlerRef_t subsHandleRef;
   le_thread_Destructor_t destructorFunc;
   le_timer_Ref_t timerRef;
   taf_someipSvr_ServiceRef_t serviceRef;
   bool EventsOffered;
}
NotifyThreadCxt_t;

NotifyThreadCxt_t NotifyCtx = { 0 };

//--------------------------------------------------------------------------------------------------
/**
 * Thread Destructor.
 */
//--------------------------------------------------------------------------------------------------
static void ThreadDestructor
(
    void* paramPtr
)
{
    NotifyThreadCxt_t* notifyCtxPtr = paramPtr;
    LE_ASSERT(notifyCtxPtr != NULL);

    // Remove the MsgHandler.
    taf_someipSvr_RemoveRxMsgHandler(notifyCtxPtr->rxMsgHandleRef);

    // Remove the SubsHandlers
    taf_someipSvr_RemoveSubscriptionHandler(notifyCtxPtr->subsHandleRef);

    // Stop the service, this will also stop all events first.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_StopOfferService(notifyCtxPtr->serviceRef),
                   "ThreadDestructor taf_someipSvr_StopOfferService() API.");

    // Delete the sem.
    le_sem_Delete(notifyCtxPtr->semRef);

    // Stop and delete the timer.
    le_timer_Delete(notifyCtxPtr->timerRef);

    // Disconnect the telaf service.
    taf_someipSvr_DisconnectService();
}

//--------------------------------------------------------------------------------------------------
/**
 * The handler for cyclical events.
 */
//--------------------------------------------------------------------------------------------------
static void TimerHandler
(
    le_timer_Ref_t timerRef
)
{
    static uint8_t testCnt = 0;
    static uint8_t itsData[10] = {0};
    static uint32_t itsSize = 0;

    NotifyThreadCxt_t* notifyCtxPtr = le_timer_GetContextPtr(timerRef);
    taf_someipSvr_ServiceRef_t serviceRef = notifyCtxPtr->serviceRef;

    testCnt++;

    if (notifyCtxPtr->EventsOffered)
    {
        // Events online lasts 2 mins.
        if (testCnt >= 24)
        {
            testCnt = 0;
            LE_TEST_ASSERT(LE_OK == taf_someipSvr_StopOfferEvent(serviceRef, TEST_EVENT_ID),
                           "EventApiThread taf_someipSvr_StopOfferEvent() API.");

            LE_TEST_ASSERT(LE_OK == taf_someipSvr_StopOfferEvent(serviceRef, TEST_EVENT_ID1),
                           "EventApiThread taf_someipSvr_StopOfferEvent() API.");

            notifyCtxPtr->EventsOffered = false;
        }
    }
    else
    {
        // Events offline lasts 1 min.
        if (testCnt >= 12)
        {
            testCnt = 0;
            LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferEvent(serviceRef, TEST_EVENT_ID),
                           "EventApiThread taf_someipSvr_OfferEvent() API.");

            LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferEvent(serviceRef, TEST_EVENT_ID1),
                           "EventApiThread taf_someipSvr_OfferEvent() API.");

            notifyCtxPtr->EventsOffered = true;
        }
    }

    itsSize++;

    for (uint8_t i = 0; i < itsSize; ++i)
    {
        itsData[i] = i;
    }

    if (notifyCtxPtr->EventsOffered)
    {
        LE_TEST_INFO("Setting event (Length=0x%x).", itsSize);
        LE_ASSERT(LE_OK == taf_someipSvr_Notify(serviceRef, TEST_EVENT_ID, itsData, itsSize));
    }

    if (itsSize == sizeof(itsData))
    {
        itsSize = 0;
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * SOMEIP subscription Handler
 */
//--------------------------------------------------------------------------------------------------
void SubscriptionHandler
(
    taf_someipSvr_ServiceRef_t serviceRef,
    uint16_t eventGroupId,
    bool isSubscribed,
    void* contextPtr
)
{
    uint16_t serviceId;
    uint16_t instanceId;

    if (serviceRef == NotifyCtx.serviceRef)
    {
        serviceId = TEST_SERVICE_ID2;
        instanceId = TEST_INSTANCE_ID;
    }
    else if (serviceRef == ServiceRef)
    {
        serviceId = TEST_SERVICE_ID1;
        instanceId = TEST_INSTANCE_ID;
    }
    else
    {
        LE_ERROR("Unknown serviceRef(%p).", serviceRef);
        return;
    }

    LE_INFO("A client for group(Id=0x%x) of Service(0x%x/0x%x) is %s.",
            eventGroupId, serviceId, instanceId, isSubscribed ? "Subscribed" : "Unsubscribed");
}

//--------------------------------------------------------------------------------------------------
/**
 * SOMEIP request message Handler
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void RxMessageHandler
(
    taf_someipSvr_RxMsgRef_t msgRef,
    void* contextPtr
)
{
    uint16_t serviceId;
    uint16_t instanceId;
    uint16_t methodId;
    uint16_t clientId;
    uint8_t msgType;
    size_t payloadSize;

    // Get the serviceId and instanceId.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_GetServiceId(msgRef, &serviceId, &instanceId),
                   "RxMessageHandler taf_someipSvr_GetServiceId() API.");

    taf_someipSvr_ServiceRef_t serviceRef = taf_someipSvr_GetService(serviceId, instanceId);
    LE_TEST_ASSERT(serviceRef != NULL, "RxMessageHandler taf_someipSvr_GetService() API.");

    // Get the methodId, clientId and msgType.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_GetMethodId(msgRef, &methodId),
                   "RxMessageHandler taf_someipSvr_GetMethodId() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_GetClientId(msgRef, &clientId),
                   "RxMessageHandler taf_someipSvr_GetClientId() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_GetMsgType(msgRef, &msgType),
                   "RxMessageHandler taf_someipSvr_GetMsgType() API.");

    // Get the payload size and data.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_GetPayloadSize(msgRef, &payloadSize),
                   "RxMessageHandler taf_someipSvr_GetPayloadSize() API.");

    LE_TEST_INFO(
        "REQUEST (servId/instId/methId/cliId/msgType/len=0x%x/0x%x/0x%x/0x%x/0x%x/0x%x) recieved.",
        serviceId, instanceId, methodId, clientId, msgType, payloadSize);

    if (payloadSize != 0)
    {
        LE_TEST_ASSERT(LE_OK == taf_someipSvr_GetPayloadData(msgRef, PayloadData, &payloadSize),
                       "RxMessageHandler taf_someipSvr_GetPayloadData() API.");

        char payloadString[2*payloadSize + 1];
        le_hex_BinaryToString(PayloadData, payloadSize, payloadString, 2*payloadSize + 1);
        LE_TEST_INFO("REQUEST PAYLOAD [%s]", payloadString);
    }

    if (msgType == TAF_SOMEIPDEF_MT_REQUEST)
    {
        // Send back the response with the same payload data,
        // the message will be automatically freed.
        LE_TEST_ASSERT(LE_OK == taf_someipSvr_SendResponse(msgRef, false, 0,
                                                           PayloadData, payloadSize),
                       "RxMessageHandler taf_someipSvr_SendResponse() API.");
    }
    else
    {
        // Release the message for a non-return request.
        LE_TEST_ASSERT(LE_OK == taf_someipSvr_ReleaseRxMsg(msgRef),
                       "RxMessageHandler taf_someipSvr_ReleaseRxMsg().");
    }

    if (serviceRef == ServiceRef)
    {
        RxMsgCnt++;
        LE_TEST_INFO("RxMsgCnt=%u", RxMsgCnt);
    }

    if (RxMsgCnt >= EXIT_RX_MSG_CNT)
    {
        // Remove handlers.
        taf_someipSvr_RemoveRxMsgHandler(RxMsgHandlerRef);
        taf_someipSvr_RemoveSubscriptionHandler(SubsHandlerRef);

        // Send a termination event to client before exiting.
        uint8_t termCmd[2] = { 0xff, 0xff };
        LE_TEST_ASSERT(LE_OK == taf_someipSvr_Notify(ServiceRef, TEST_EVENT_ID2, termCmd, 2),
                       "RxMessageHandler sending termination event.");

        LE_TEST_ASSERT(LE_OK == taf_someipSvr_StopOfferService(ServiceRef),
                       "RxMessageHandler taf_someipSvr_StopOfferService() API.");

        // Stop and remove the event test thread.
        if (NotifyCtx.threadRef)
        {
            LE_ASSERT(LE_OK == le_thread_Cancel(NotifyCtx.threadRef));
            LE_ASSERT(LE_OK == le_thread_Join(NotifyCtx.threadRef, NULL));
            LE_TEST_INFO("EventApiTest thread is stopped.");
        }

        LE_TEST_INFO("========================================");
        LE_TEST_INFO("=== telaf someip server API test END ===");
        LE_TEST_INFO("========================================");

        LE_TEST_EXIT;
    }
    return;
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
    NotifyThreadCxt_t* notifyCtxPtr = context;

    // Connect service in sub thread.
    taf_someipSvr_ConnectService();

    // Set thread destructor so that when the thread gets exited, we can do some clearup work.
    le_thread_AddDestructor(notifyCtxPtr->destructorFunc, (void *)notifyCtxPtr);

    // Get the service Reference.
    taf_someipSvr_ServiceRef_t serviceRef =
        taf_someipSvr_GetService(TEST_SERVICE_ID2, TEST_INSTANCE_ID);
    LE_TEST_ASSERT(serviceRef != NULL, "EventApiThread taf_someipSvr_GetService() API.");

    // Set the service version, we use the default versions for message test.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServiceVersion(serviceRef, 0x33, 0x66667777),
                   "EventApiThread taf_someipSvr_SetServiceVersion() API.");

    // Set the UDP/TCP port.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServicePort(serviceRef, UdpPort2, TcpPort2, true),
                   "EventApiThread taf_someipSvr_SetServicePort() API.");

    // Offer the service with given version and ports.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferService(serviceRef),
                   "EventApiThread taf_someipSvr_OfferService() API.");

    // Register Message Handler.
    notifyCtxPtr->rxMsgHandleRef = taf_someipSvr_AddRxMsgHandler(serviceRef, RxMessageHandler, NULL);
    LE_TEST_ASSERT(NULL != notifyCtxPtr->rxMsgHandleRef,
                   "EventApiThread taf_someipSvr_AddRxMsgHandler() API.");

    // Register again will fail since a service can register only one RxHandler.
    LE_TEST_ASSERT(NULL == taf_someipSvr_AddRxMsgHandler(serviceRef, RxMessageHandler, NULL),
                   "EventApiThread taf_someipSvr_AddRxMsgHandler() API.");

    // Enable the field type event.
    LE_TEST_ASSERT(LE_OK ==
        taf_someipSvr_EnableEvent(serviceRef, TEST_EVENT_ID, TEST_EVENTGROUP_ID),
        "EventApiThread taf_someipSvr_EnableEvent() API.");

    // Set the event type.
    LE_TEST_ASSERT(LE_OK ==
        taf_someipSvr_SetEventType(serviceRef, TEST_EVENT_ID, TAF_SOMEIPDEF_ET_FIELD),
        "EventApiThread taf_someipSvr_SetEventType() API.");

    // Offer the event.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferEvent(serviceRef, TEST_EVENT_ID),
        "EventApiThread taf_someipSvr_OfferEvent() API.");

    // Disable the event, this shall get failed since the event is already offered.
    LE_TEST_ASSERT(LE_NOT_PERMITTED == taf_someipSvr_DisableEvent(serviceRef, TEST_EVENT_ID),
        "EventApiThread taf_someipSvr_DisableEvent() API.");

    // Stop the event.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_StopOfferEvent(serviceRef, TEST_EVENT_ID),
        "EventApiThread taf_someipSvr_StopOfferEvent() API.");

    // Disable the event, this shall get succeeded.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_DisableEvent(serviceRef, TEST_EVENT_ID),
        "EventApiThread taf_someipSvr_DisableEvent() API.");

    // Offer the event again and this shall get failed, since the event is not enabled.
    LE_TEST_ASSERT(LE_NOT_PERMITTED == taf_someipSvr_OfferEvent(serviceRef, TEST_EVENT_ID),
        "EventApiThread taf_someipSvr_OfferEvent() API.");

    // Enable the event again.
    LE_TEST_ASSERT(LE_OK ==
        taf_someipSvr_EnableEvent(serviceRef, TEST_EVENT_ID, TEST_EVENTGROUP_ID),
        "EventApiThread taf_someipSvr_EnableEvent() API.");

    // Offer the event.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferEvent(serviceRef, TEST_EVENT_ID),
        "EventApiThread taf_someipSvr_OfferEvent() API.");

    // Enable an new cyclical event in the same group and offer the event.
    LE_TEST_ASSERT(LE_OK ==
        taf_someipSvr_EnableEvent(serviceRef, TEST_EVENT_ID1, TEST_EVENTGROUP_ID),
        "EventApiThread taf_someipSvr_EnableEvent() API.");

    // Set the event cycle time with 30s interval, which means the event will keep sending
    // with the setting time interval after taf_someipSvr_Notify() API is called once.
    LE_TEST_ASSERT(LE_OK ==
        taf_someipSvr_SetEventCycleTime(serviceRef, TEST_EVENT_ID1, 30000),
        "EventApiThread taf_someipSvr_SetEventCycleTime() API.");

    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferEvent(serviceRef, TEST_EVENT_ID1),
        "EventApiThread taf_someipSvr_OfferEvent() API.");

    // Fill the cyclical event payload, and keep sending the event.
    uint8_t cycleData[7] = { 0x77, 0x66, 0x55, 0x44, 0x33, 0x22 , 0x11};
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_Notify(serviceRef, TEST_EVENT_ID1,
                                                  cycleData, sizeof(cycleData)),
        "EventApiThread taf_someipSvr_Notify() API.");

    // Register subscription handler.
    notifyCtxPtr->subsHandleRef =
        taf_someipSvr_AddSubscriptionHandler(serviceRef, TEST_EVENTGROUP_ID,
                                             SubscriptionHandler, NULL);
    LE_TEST_ASSERT(NULL != notifyCtxPtr->subsHandleRef,
                   "EventApiThread taf_someipSvr_AddSubscriptionHandler() API.");

    // Register again will fail.
    LE_TEST_ASSERT(NULL == taf_someipSvr_AddSubscriptionHandler(serviceRef, TEST_EVENTGROUP_ID,
                                                                SubscriptionHandler, NULL),
                   "EventApiThread taf_someipSvr_AddSubscriptionHandler() API.");

    // Save the server reference and eventId.
    notifyCtxPtr->serviceRef = serviceRef;
    notifyCtxPtr->EventsOffered = true;

    // Start a timer to send field notifications.
    notifyCtxPtr->timerRef = le_timer_Create("Event test timer");
    le_timer_SetMsInterval(notifyCtxPtr->timerRef, 5000);
    le_timer_SetHandler(notifyCtxPtr->timerRef, TimerHandler);
    le_timer_SetRepeat(notifyCtxPtr->timerRef, 0);
    le_timer_SetWakeup(notifyCtxPtr->timerRef, false);
    le_timer_SetContextPtr(notifyCtxPtr->timerRef, (void*)notifyCtxPtr);
    le_timer_Start(notifyCtxPtr->timerRef);

    // Unblock the main thread.
    le_sem_Post(notifyCtxPtr->semRef);

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
    NotifyCtx.semRef = le_sem_Create("EventApiTestSem", 0);
    NotifyCtx.destructorFunc = ThreadDestructor;
    NotifyCtx.threadRef = le_thread_Create("EventApiTestThread", EventApiThread, (void*)&NotifyCtx);

    le_thread_SetJoinable(NotifyCtx.threadRef);
    le_thread_Start(NotifyCtx.threadRef);

    le_sem_Wait(NotifyCtx.semRef);
    LE_TEST_INFO("EventApiTest thread is running.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests for service APIs.
 */
//--------------------------------------------------------------------------------------------------
static void ServiceApiTest
(
    uint16_t serviceId,
    uint16_t instanceId,
    uint8_t majVersion,
    uint32_t minVersion,
    uint16_t udpPort,
    uint16_t tcpPort
)
{
    // Get the service Reference.
    taf_someipSvr_ServiceRef_t serviceRef = taf_someipSvr_GetService(serviceId, instanceId);
    LE_TEST_ASSERT(serviceRef != NULL, "ServiceApiTest taf_someipSvr_GetService() API.");

    // Set the service version.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServiceVersion(serviceRef, majVersion, minVersion),
                   "ServiceApiTest taf_someipSvr_SetServiceVersion() API.");

    // Set the UDP/TCP port.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServicePort(serviceRef, udpPort, tcpPort, true),
                   "ServiceApiTest taf_someipSvr_SetServicePort() API.");

    // Offer the service with given version and ports.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferService(serviceRef),
                   "ServiceApiTest taf_someipSvr_OfferService() API.");

    // Stop the service
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_StopOfferService(serviceRef),
                   "ServiceApiTest taf_someipSvr_StopOfferService() API.");
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
    // Get the service Reference.
    ServiceRef = taf_someipSvr_GetService(TEST_SERVICE_ID1, TEST_INSTANCE_ID);
    LE_TEST_ASSERT(ServiceRef != NULL, "MessageApiTest taf_someipSvr_GetService() API.");

    // Set the service version, we use the default versions for message test.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServiceVersion(ServiceRef, MajVersion, MinVersion),
                   "MessageApiTest taf_someipSvr_SetServiceVersion() API.");

    // Set the UDP/TCP port.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServicePort(ServiceRef, UdpPort1, TcpPort1, true),
                   "MessageApiTest taf_someipSvr_SetServicePort() API.");

    // Offer the service with given version and ports.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferService(ServiceRef),
                   "MessageApiTest taf_someipSvr_OfferService() API.");

    // Register Message Handler.
    RxMsgHandlerRef = taf_someipSvr_AddRxMsgHandler(ServiceRef, RxMessageHandler, NULL);
    LE_TEST_ASSERT(NULL != RxMsgHandlerRef, "MessageApiTest taf_someipSvr_AddRxMsgHandler() API.");

    SubsHandlerRef = taf_someipSvr_AddSubscriptionHandler(ServiceRef, TEST_EVENTGROUP_ID1,
                                                          SubscriptionHandler, NULL);
    LE_TEST_ASSERT(NULL != SubsHandlerRef,
        "MessageApiTest taf_someipSvr_AddSubscriptionHandler() API.");

    // Enable termination event.
    LE_TEST_ASSERT(LE_OK ==
        taf_someipSvr_EnableEvent(ServiceRef, TEST_EVENT_ID2, TEST_EVENTGROUP_ID1),
        "MessageApiTest taf_someipSvr_EnableEvent() API.");

    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferEvent(ServiceRef, TEST_EVENT_ID2),
        "MessageApiTest taf_someipSvr_OfferEvent() API.");
}

COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("==========================================");
    LE_TEST_INFO("=== telaf someip server API test BEGIN ===");
    LE_TEST_INFO("==========================================");

    ServiceApiTest(TEST_SERVICE_ID1, TEST_INSTANCE_ID, MajVersion, MinVersion, UdpPort1, TcpPort1);
    ServiceApiTest(TEST_SERVICE_ID1, TEST_INSTANCE_ID, MajVersion, MinVersion, UdpPort2, TcpPort2);
    MessageApiTest();
    EventApiTest();
}
