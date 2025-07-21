/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define TEST_SERVICE_ID_1     0x238
#define TEST_SERVICE_ID_2     0x239
#define TEST_SERVICE_ID_3     0x23A
#define TEST_INSTANCE_ID      0x1

#define TEST_EVENTGROUP_ID    0x4465
#define TEST_EVENT_ID         0x8778

#define TEST_MAJ_VERSION_1      22
#define TEST_MIN_VERSION_1      1234

#define TEST_MAJ_VERSION_2      44
#define TEST_MIN_VERSION_2      5678

#define TEST_MAJ_VERSION_3      66
#define TEST_MIN_VERSION_3      9999

#define TEST_PORT 13501

static taf_someipSvr_ServiceRef_t ServiceRef1 = NULL;
static taf_someipSvr_ServiceRef_t ServiceRef2 = NULL;
static taf_someipSvr_ServiceRef_t ServiceRef3 = NULL;

static le_timer_Ref_t NotifyTimerRef = NULL;

static uint32_t RxCnt = 0;
static uint32_t EvtCnt = 0;

static char MsgData[TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE] = { 0 };
static char EventData[TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE] = { 0 };

//--------------------------------------------------------------------------------------------------
/**
 * SOMEIP Rx message Handler
 */
//--------------------------------------------------------------------------------------------------
void RxMessageHandler
(
    taf_someipSvr_RxMsgRef_t msgRef,
    void* contextPtr
)
{
    uint16_t serviceId, instanceId;
    size_t msgSize;

    if (LE_OK != taf_someipSvr_GetServiceId(msgRef, &serviceId, &instanceId))
    {
        LE_TEST_FATAL("Failed to get ServiceId from the msgRef.");
    }

    // Build the response message.
    msgSize = snprintf(MsgData, sizeof(MsgData), "[Service0x%x/0x%x: Reply Cnt=%u.]",
                       serviceId, instanceId, RxCnt++);

    // Send the response message.
    if(LE_OK != taf_someipSvr_SendResponse(msgRef, false, 0, (uint8_t*)MsgData, msgSize))
    {
        LE_TEST_FATAL("Failed to call taf_someipSvr_SendResponse().");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Notify timer handler
 */
//--------------------------------------------------------------------------------------------------
static void NotifyTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    size_t evtSize;

    // Build the event message for service 0x238/0x1, then send the event.
    evtSize = snprintf(EventData, sizeof(EventData), "[Service0x%x/0x%x: Event Cnt=%u.]",
                       TEST_SERVICE_ID_1, TEST_INSTANCE_ID, EvtCnt++);
    if (LE_OK != taf_someipSvr_Notify(ServiceRef1, TEST_EVENT_ID, (uint8_t*)EventData, evtSize))
    {
        LE_TEST_FATAL("Failed to call taf_someipSvr_Notify().");
    }

    // Build the event message for service 0x239/0x1, then send the event.
    evtSize = snprintf(EventData, sizeof(EventData), "[Service0x%x/0x%x: Event Cnt=%u.]",
                       TEST_SERVICE_ID_2, TEST_INSTANCE_ID, EvtCnt++);
    if (LE_OK != taf_someipSvr_Notify(ServiceRef2, TEST_EVENT_ID, (uint8_t*)EventData, evtSize))
    {
        LE_TEST_FATAL("Failed to call taf_someipSvr_Notify().");
    }

    // Build the event message for service 0x23A/0x1, then send the event.
    evtSize = snprintf(EventData, sizeof(EventData), "[Service0x%x/0x%x: Event Cnt=%u.]",
                       TEST_SERVICE_ID_3, TEST_INSTANCE_ID, EvtCnt++);
    if (LE_OK != taf_someipSvr_Notify(ServiceRef3, TEST_EVENT_ID, (uint8_t*)EventData, evtSize))
    {
        LE_TEST_FATAL("Failed to call taf_someipSvr_Notify().");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * SOMEIP subscription Handler
 */
//--------------------------------------------------------------------------------------------------
static void SubscriptionHandler
(
    taf_someipSvr_ServiceRef_t serviceRef,
    uint16_t eventGroupId,
    bool isSubscribed,
    void* contextPtr
)
{
    if (serviceRef == ServiceRef1)
    {
        LE_TEST_INFO("SUBSCRIPTION HANDLED [Service0x%x/0x%x: EventGroup=0x%x is %s]",
                     TEST_SERVICE_ID_1, TEST_INSTANCE_ID, eventGroupId,
                     isSubscribed ? "Subscribed" : "Unsubscribed");
        return;
    }
    else if (serviceRef == ServiceRef2)
    {
        LE_TEST_INFO("SUBSCRIPTION HANDLED [Service0x%x/0x%x: EventGroup=0x%x is %s]",
                     TEST_SERVICE_ID_2, TEST_INSTANCE_ID, eventGroupId,
                     isSubscribed ? "Subscribed" : "Unsubscribed");
        return;
    }
    else if (serviceRef == ServiceRef3)
    {
        LE_TEST_INFO("SUBSCRIPTION HANDLED [Service0x%x/0x%x: EventGroup=0x%x is %s]",
                     TEST_SERVICE_ID_3, TEST_INSTANCE_ID, eventGroupId,
                     isSubscribed ? "Subscribed" : "Unsubscribed");
        return;
    }

    LE_TEST_FATAL("Unknown serviceRef.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Offer Service Test
 */
//--------------------------------------------------------------------------------------------------
static void OfferServiceTest
(
    void
)
{
    // Get the service Reference.
    ServiceRef1 = taf_someipSvr_GetService(TEST_SERVICE_ID_1, TEST_INSTANCE_ID);
    LE_TEST_ASSERT(ServiceRef1 != NULL, "OfferService1Test taf_someipSvr_GetService() API.");
    ServiceRef2 = taf_someipSvr_GetService(TEST_SERVICE_ID_2, TEST_INSTANCE_ID);
    LE_TEST_ASSERT(ServiceRef2 != NULL, "OfferService2Test taf_someipSvr_GetService() API.");
    ServiceRef3 = taf_someipSvr_GetService(TEST_SERVICE_ID_3, TEST_INSTANCE_ID);
    LE_TEST_ASSERT(ServiceRef3 != NULL, "OfferService3Test taf_someipSvr_GetService() API.");

    // Set the version=22.1234 for service 1
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServiceVersion(ServiceRef1,
                   TEST_MAJ_VERSION_1, TEST_MIN_VERSION_1),
                   "OfferService1Test taf_someipSvr_SetServiceVersion() API.");

    // Set the version=44.5678 for service 2
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServiceVersion(ServiceRef2,
                   TEST_MAJ_VERSION_2, TEST_MIN_VERSION_2),
                   "OfferService2Test taf_someipSvr_SetServiceVersion() API.");

    // Set the version=66.9999 for service 3
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServiceVersion(ServiceRef3,
                   TEST_MAJ_VERSION_3, TEST_MIN_VERSION_3),
                   "OfferService3Test taf_someipSvr_SetServiceVersion() API.");

    // Set the UDP and TCP port.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServicePort(ServiceRef1, TEST_PORT, TEST_PORT, false),
                   "OfferService1Test taf_someipSvr_SetServicePort() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServicePort(ServiceRef2, TEST_PORT, TEST_PORT, false),
                   "OfferService2Test taf_someipSvr_SetServicePort() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServicePort(ServiceRef3, TEST_PORT, TEST_PORT, false),
                   "OfferService3Test taf_someipSvr_SetServicePort() API.");

    // Offer the service with given version and ports.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferService(ServiceRef1),
                   "OfferService1Test taf_someipSvr_OfferService() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferService(ServiceRef2),
                   "OfferService2Test taf_someipSvr_OfferService() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferService(ServiceRef3),
                   "OfferService3Test taf_someipSvr_OfferService() API.");

    // Register Message Handler.
    LE_TEST_ASSERT(NULL != taf_someipSvr_AddRxMsgHandler(ServiceRef1, RxMessageHandler, NULL),
                   "OfferService1Test taf_someipSvr_AddRxMsgHandler() API.");
    LE_TEST_ASSERT(NULL != taf_someipSvr_AddRxMsgHandler(ServiceRef2, RxMessageHandler, NULL),
                   "OfferService2Test taf_someipSvr_AddRxMsgHandler() API.");
    LE_TEST_ASSERT(NULL != taf_someipSvr_AddRxMsgHandler(ServiceRef3, RxMessageHandler, NULL),
                   "OfferService3Test taf_someipSvr_AddRxMsgHandler() API.");

    // Register Subscribe handler.
    LE_TEST_ASSERT(NULL != taf_someipSvr_AddSubscriptionHandler(ServiceRef1, TEST_EVENTGROUP_ID,
                                                                SubscriptionHandler, NULL),
                   "OfferService1Test taf_someipSvr_AddSubscriptionHandler() API.");
    LE_TEST_ASSERT(NULL != taf_someipSvr_AddSubscriptionHandler(ServiceRef2, TEST_EVENTGROUP_ID,
                                                                SubscriptionHandler, NULL),
                   "OfferService2Test taf_someipSvr_AddSubscriptionHandler() API.");
    LE_TEST_ASSERT(NULL != taf_someipSvr_AddSubscriptionHandler(ServiceRef3, TEST_EVENTGROUP_ID,
                                                                SubscriptionHandler, NULL),
                   "OfferService3Test taf_someipSvr_AddSubscriptionHandler() API.");

    // Offer event.
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_EnableEvent(ServiceRef1, TEST_EVENT_ID,
                   TEST_EVENTGROUP_ID), "OfferService1Test taf_someipSvr_EnableEvent() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_EnableEvent(ServiceRef2, TEST_EVENT_ID,
                   TEST_EVENTGROUP_ID), "OfferService2Test taf_someipSvr_EnableEvent() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_EnableEvent(ServiceRef3, TEST_EVENT_ID,
                   TEST_EVENTGROUP_ID), "OfferService3Test taf_someipSvr_EnableEvent() API.");

    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferEvent(ServiceRef1, TEST_EVENT_ID),
        "OfferService1Test taf_someipSvr_OfferEvent() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferEvent(ServiceRef2, TEST_EVENT_ID),
        "OfferService2Test taf_someipSvr_OfferEvent() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferEvent(ServiceRef3, TEST_EVENT_ID),
        "OfferService3Test taf_someipSvr_OfferEvent() API.");

    // Create notify timer.
    NotifyTimerRef = le_timer_Create("Notify timer");
    le_timer_SetMsInterval(NotifyTimerRef, 5000);
    le_timer_SetHandler(NotifyTimerRef, NotifyTimerHandler);
    le_timer_SetRepeat(NotifyTimerRef, 0);
    le_timer_SetWakeup(NotifyTimerRef, false);
    le_timer_Start(NotifyTimerRef);
}

COMPONENT_INIT
{
    OfferServiceTest();
}
