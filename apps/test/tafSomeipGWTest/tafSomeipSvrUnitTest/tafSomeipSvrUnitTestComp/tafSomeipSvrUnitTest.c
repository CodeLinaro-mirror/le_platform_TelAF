/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define TEST_SERVICE_ID1      0x2345
#define TEST_SERVICE_ID2      0x1234
#define TEST_INSTANCE_ID      0x5678
#define TEST_EVENT_ID         0x8778
#define TEST_EVENT_ID1        0x6666
#define TEST_GET_METHOD_ID    0x0001
#define TEST_SET_METHOD_ID    0x0002
#define TEST_EVENTGROUP_ID    0x4465
#define TEST_EVENTGROUP_ID1   0x7777


#define EXIT_RX_MSG_CNT       100

const static uint8_t MajVersion = 0x10;
const static uint32_t MinVersion = 0x30304040;
const static uint16_t UdpPort1 = 12345;
const static uint16_t TcpPort1 = 12344;
const static uint16_t UdpPort2 = 54321;
const static uint16_t TcpPort2 = 44321;

static uint32_t RxMsgCnt = 0;
static taf_someipSvr_RxMsgHandlerRef_t RxMsgHandlerRef = NULL;
static taf_someipSvr_ServiceRef_t ServiceRef = NULL;

static size_t PayloadSize;
static uint8_t PayloadData[TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE];
static char PayloadString[2*TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE + 1];

typedef struct
{
   le_thread_Ref_t threadRef;
   le_sem_Ref_t semRef;
   taf_someipSvr_RxMsgHandlerRef_t msgHandleRef;
   le_thread_Destructor_t destructorFunc;
   le_timer_Ref_t timerRef;
   taf_someipSvr_ServiceRef_t serverRef;
}
NotifyThreadCxt_t;

NotifyThreadCxt_t NotifyCtx = { 0 };

//--------------------------------------------------------------------------------------------------
/**
 * Tests for Event APIs.
 */
//--------------------------------------------------------------------------------------------------
static void ThreadDestructor
(
    void* paramPtr
 )
{
    NotifyThreadCxt_t* notifyCtxPtr = paramPtr;

    // Stop and delete the timer.
    le_timer_Delete(notifyCtxPtr->timerRef);

    // Stop the event.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_StopOfferEvent(notifyCtxPtr->serverRef, TEST_EVENT_ID),
                   "ThreadDestructor taf_someipSvr_StopOfferEvent() API.");

    // Stop the event.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_StopOfferEvent(notifyCtxPtr->serverRef, TEST_EVENT_ID1),
                   "ThreadDestructor taf_someipSvr_StopOfferEvent() API.");

    // Stop the service.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_StopOfferService(notifyCtxPtr->serverRef),
                   "ThreadDestructor taf_someipSvr_StopOfferService() API.");

    // Remove the MsgHandler.
    taf_someipSvr_RemoveRxMsgHandler(notifyCtxPtr->msgHandleRef);

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
    static uint8_t itsData[10] = {0};
    static uint32_t itsSize = 0;
    taf_someipSvr_ServiceRef_t serviceRef = le_timer_GetContextPtr(timerRef);

    itsSize++;

    for (uint8_t i = 0; i < itsSize; ++i)
    {
        itsData[i] = i;
    }

    LE_TEST_INFO("Setting event (Length=0x%x).", itsSize);
    LE_ASSERT(LE_OK == taf_someipSvr_Notify(serviceRef, TEST_EVENT_ID, itsData, itsSize));

    if (itsSize == sizeof(itsData))
    {
        itsSize = 0;
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * SOMEIP request message Handler
 */
//--------------------------------------------------------------------------------------------------
void RxMessageHandler
(
    taf_someipSvr_RxMsgRef_t msgRef,
    void* contextPtr
)
{
    RxMsgCnt++;
    LE_TEST_INFO("RxMsgCnt=%u", RxMsgCnt);

    uint16_t serviceId;
    uint16_t instanceId;
    // Get the serviceId and instanceId.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_GetSerivceId(msgRef, &serviceId, &instanceId),
                   "RxMessageHandler taf_someipSvr_GetSerivceId() API.");

    taf_someipSvr_ServiceRef_t serviceRef = taf_someipSvr_GetService(serviceId, instanceId);
    LE_TEST_ASSERT(serviceRef != NULL, "RxMessageHandler taf_someipSvr_GetService() API.");

    uint16_t methodId;
    uint16_t clientId;
    uint8_t msgType;
    // Get the methodId, clientId and msgType.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_GetMethodId(msgRef, &methodId),
              "RxMessageHandler taf_someipSvr_GetMethodId() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_GetClientId(msgRef, &clientId),
              "RxMessageHandler taf_someipSvr_GetClientId() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_GetMsgType(msgRef, &msgType),
              "RxMessageHandler taf_someipSvr_GetMsgType() API.");

    // Get the payload size and data.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_GetPayloadSize(msgRef, &PayloadSize),

             "RxMessageHandler taf_someipSvr_GetPayloadSize() API.");

    LE_TEST_INFO("message (servId/instId/methId/cliId/msgType/len=0x%x/0x%x/0x%x/0x%x/0x%x/0x%x)",
            serviceId, instanceId, methodId, clientId, msgType, PayloadSize);
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_GetPayloadData(msgRef, PayloadData, &PayloadSize),
              "RxMessageHandler taf_someipSvr_GetPayloadData() API.");

    if (PayloadSize != 0)
    {
        le_hex_BinaryToString(PayloadData, PayloadSize, PayloadString, sizeof(PayloadString));
        LE_TEST_INFO("MESSAGE PAYLOAD [%s]", PayloadString);
    }

    // Send back the response with the same payload data.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SendResponse(msgRef, false, 0, PayloadData, PayloadSize),
                   "RxMessageHandler taf_someipSvr_SendResponse() API.");

    // Release the message.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_ReleaseRxMsg(msgRef),
              "RxMessageHandler taf_someipSvr_ReleaseRxMsg().");

    if ((RxMsgCnt >= EXIT_RX_MSG_CNT) && (serviceRef != NotifyCtx.serverRef))
    {
        taf_someipSvr_RemoveRxMsgHandler(RxMsgHandlerRef);

        LE_TEST_ASSERT(LE_OK == taf_someipSvr_StopOfferService(ServiceRef),
                       "RxMessageHandler taf_someipSvr_StopOfferService() API.");

        // Stop and remove the event test thread.
        if (NotifyCtx.threadRef)
        {
            LE_ASSERT(LE_OK == le_thread_Cancel(NotifyCtx.threadRef));
            LE_ASSERT(LE_OK == le_thread_Join(NotifyCtx.threadRef, NULL));
            LE_TEST_INFO("EventApiTest thread is stopped.");
        }

        LE_TEST_INFO("=== telaf someip server API test END ===");
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
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServiceVersion(serviceRef,
                   TAF_SOMEIPDEF_DEFAULT_MAJOR, TAF_SOMEIPDEF_DEFAULT_MINOR),
                   "EventApiThread taf_someipSvr_SetServiceVersion() API.");

    // Set the UDP/TCP port.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServicePort(serviceRef, UdpPort2, TcpPort2, true),
                   "EventApiThread taf_someipSvr_SetServicePort() API.");

    // Offer the service with given version and ports.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferService(serviceRef),
                   "EventApiThread taf_someipSvr_OfferService() API.");

    // Register Message Handler.
    notifyCtxPtr->msgHandleRef = taf_someipSvr_AddRxMsgHandler(serviceRef, RxMessageHandler, NULL);
    LE_TEST_ASSERT(NULL != notifyCtxPtr->msgHandleRef,
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

    // Enable an event type event and offer event.
    LE_TEST_ASSERT(LE_OK ==
        taf_someipSvr_EnableEvent(serviceRef, TEST_EVENT_ID1, TEST_EVENTGROUP_ID),
        "EventApiThread taf_someipSvr_EnableEvent() API.");

    // Set the event cycle time, which means the event will keep being sent with
    // the setting time interval after taf_someipSvr_Notify() API is called.
    LE_TEST_ASSERT(LE_OK ==
        taf_someipSvr_SetEventCycleTime(serviceRef, TEST_EVENT_ID1, 20000),
        "EventApiThread taf_someipSvr_SetEventCycleTime() API.");

    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferEvent(serviceRef, TEST_EVENT_ID1),
        "EventApiThread taf_someipSvr_OfferEvent() API.");

    // Triger the cyclical event with 10s interval.
    uint8_t cycleData[6] = { 06, 0x5, 0x4, 0x3, 0x2, 0x1 };
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_Notify(serviceRef, TEST_EVENT_ID1,
                                                  cycleData, sizeof(cycleData)),
        "EventApiThread taf_someipSvr_Notify() API.");

    // Save the server reference and eventId.
    notifyCtxPtr->serverRef = serviceRef;

    // Start a timer to send field notifications.
    notifyCtxPtr->timerRef = le_timer_Create("Event test timer");
    le_timer_SetMsInterval(notifyCtxPtr->timerRef, 5000);
    le_timer_SetHandler(notifyCtxPtr->timerRef, TimerHandler);
    le_timer_SetRepeat(notifyCtxPtr->timerRef, 0);
    le_timer_SetWakeup(notifyCtxPtr->timerRef, false);
    le_timer_SetContextPtr(notifyCtxPtr->timerRef, (void*)serviceRef);
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
static void MessageApiTest
(
    void
)
{
    // Get the service Reference.
    ServiceRef = taf_someipSvr_GetService(TEST_SERVICE_ID1, TEST_INSTANCE_ID);
    LE_TEST_ASSERT(ServiceRef != NULL, "MessageApiTest taf_someipSvr_GetService() API.");

    // Set the service version, we use the default versions for message test.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServiceVersion(ServiceRef,
                   TAF_SOMEIPDEF_DEFAULT_MAJOR, TAF_SOMEIPDEF_DEFAULT_MINOR),
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
}

COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("=== telaf someip server API test BEGIN ===");

    ServiceApiTest(TEST_SERVICE_ID1, TEST_INSTANCE_ID, MajVersion, MinVersion, UdpPort1, TcpPort1);
    ServiceApiTest(TEST_SERVICE_ID1, TEST_INSTANCE_ID, MajVersion, MinVersion, UdpPort2, TcpPort2);
    MessageApiTest();
    EventApiTest();
}
