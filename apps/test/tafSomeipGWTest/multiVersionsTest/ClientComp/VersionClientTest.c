/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"


#define TEST_SERVICE_ID_1 0x238
#define TEST_SERVICE_ID_2 0x239
#define TEST_SERVICE_ID_3 0x23A
#define TEST_INSTANCE_ID  0x1

#define TEST_EVENTGROUP_ID    0x4465
#define TEST_EVENT_ID         0x8778

#define TEST_MAJ_VERSION_1      22
#define TEST_MIN_VERSION_1      1234

#define TEST_MAJ_VERSION_2      44
#define TEST_MIN_VERSION_2      5678

#define TEST_METHOD_ID    0x2000
#define ROUTING_INTERFACE_NAME "bridge0"

static taf_someipClnt_ServiceRef_t ServiceRef1 = NULL;
static taf_someipClnt_ServiceRef_t ServiceRef2 = NULL;
static taf_someipClnt_ServiceRef_t ServiceRef3 = NULL;

static char MsgData[TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE] = { 0 };
static char EventData[TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE] = { 0 };

static le_timer_Ref_t TimerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * SOMEIP state change Handler
 */
//--------------------------------------------------------------------------------------------------
static void StateChangeHandler
(
    taf_someipClnt_ServiceRef_t serviceRef,
    taf_someipClnt_State_t state,
    void* contextPtr
)
{
    uint16_t serviceId;
    uint8_t majVer;
    uint32_t minVer;

    if (serviceRef == ServiceRef1)
    {
        serviceId = TEST_SERVICE_ID_1;
    }
    else if (serviceRef == ServiceRef2)
    {
        serviceId = TEST_SERVICE_ID_2;
    }
    else if (serviceRef == ServiceRef3)
    {
        serviceId = TEST_SERVICE_ID_3;
    }
    else
    {
        LE_TEST_FATAL("Unknown serivceRef.");
    }

    // Log the service state.
    LE_TEST_INFO("Service(0x%x/0x%x) State is %s.", serviceId, TEST_INSTANCE_ID,
                 state == TAF_SOMEIPCLNT_AVAILABLE ? "AVAILABLE" : "UNAVAILABLE");

    // Get the service version once it's available.
    if ((state == TAF_SOMEIPCLNT_AVAILABLE) &&
        (LE_OK == taf_someipClnt_GetVersion(serviceRef, &majVer, &minVer)))
    {
        LE_TEST_INFO("Service(0x%x/0x%x) Version is '%u.%u'.",
                     serviceId, TEST_INSTANCE_ID, majVer, minVer);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * SOMEIP response message handler.
 */
//--------------------------------------------------------------------------------------------------
static void ResponseHandler
(
    le_result_t result,
    bool isErrRsp,
    uint8_t returnCode,
    const uint8_t* dataPtr,
    size_t dataSize,
    void* contextPtr
)
{
    uint16_t serviceId;

    if (contextPtr == (void*)ServiceRef1)
    {
        serviceId = TEST_SERVICE_ID_1;
    }
    else if (contextPtr == (void*)ServiceRef2)
    {
        serviceId = TEST_SERVICE_ID_2;
    }
    else if (contextPtr == (void*)ServiceRef3)
    {
        serviceId = TEST_SERVICE_ID_3;
    }
    else
    {
        LE_TEST_FATAL("Unknown serivceRef.");
    }

    LE_INFO("Service(0x%x/0x%x) Response handler: [%s] [%s] [%d]",
            serviceId, TEST_INSTANCE_ID, LE_RESULT_TXT(result),
            isErrRsp ? "MT_ERROR" : "MT_RESPONSE", returnCode);

    // Get the normal response payload message.
    if ((result == LE_OK) && (dataPtr != NULL) && (dataSize != 0))
    {
        memset(MsgData, 0, sizeof(MsgData));
        memcpy(MsgData, dataPtr, dataSize);
        LE_TEST_INFO("RESPONSE PAYLOAD: %s", MsgData);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * SOMEIP event message Handler
 */
//--------------------------------------------------------------------------------------------------
static void EventMsgHandler
(
    taf_someipClnt_ServiceRef_t serviceRef,
    uint16_t eventId,
    const uint8_t* dataPtr,
    size_t dataSize,
    void* contextPtr
)
{
    if (((serviceRef != ServiceRef1) && (serviceRef != ServiceRef2) && (serviceRef != ServiceRef3))
        || (eventId != TEST_EVENT_ID))
    {
        LE_TEST_FATAL("Unknown serivceRef/eventId.");
    }

    // Dump the payload data if it's not empty.
    if (dataSize != 0)
    {
        memset(EventData, 0, sizeof(EventData));
        memcpy(EventData, dataPtr, dataSize);

        LE_TEST_INFO("EVENT PAYLOAD: %s", EventData);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The handler for cyclical request.
 */
//--------------------------------------------------------------------------------------------------
static void TimerHandler
(
    le_timer_Ref_t timerRef
)
{
    taf_someipClnt_TxMsgRef_t txMsgRef;

    // Create and send a request message for service1 via UDP/TCP connection.
    txMsgRef = taf_someipClnt_CreateMsg(ServiceRef1, TEST_METHOD_ID);
    taf_someipClnt_RequestResponse(txMsgRef, ResponseHandler, ServiceRef1);

    // Create and send a request message for service2 via UDP/TCP connection.
    txMsgRef = taf_someipClnt_CreateMsg(ServiceRef2, TEST_METHOD_ID);
    taf_someipClnt_RequestResponse(txMsgRef, ResponseHandler, ServiceRef2);

    // Create and send a request message for service3 via UDP/TCP connection.
    txMsgRef = taf_someipClnt_CreateMsg(ServiceRef3, TEST_METHOD_ID);
    taf_someipClnt_RequestResponse(txMsgRef, ResponseHandler, ServiceRef3);
}

//--------------------------------------------------------------------------------------------------
/**
 * Request Service Test
 */
//--------------------------------------------------------------------------------------------------
static void RequestServiceTest
(
    void
)
{
    // Request the services with dedicated versions.
    ServiceRef1 = taf_someipClnt_RequestServiceWithVersion(TEST_SERVICE_ID_1, TEST_INSTANCE_ID,
        TEST_MAJ_VERSION_1, TEST_MIN_VERSION_1, "bridge0");
    LE_TEST_ASSERT(ServiceRef1 != NULL, "Test RequestServiceWithVersion() with TEST_SERVICE_ID_1.");
    LE_TEST_ASSERT(NULL == taf_someipClnt_RequestServiceWithVersion(TEST_SERVICE_ID_1,
        TEST_INSTANCE_ID, TEST_MAJ_VERSION_1+1, TEST_MIN_VERSION_1, "bridge0"),
        "Test wrong service version.");

    ServiceRef2 = taf_someipClnt_RequestServiceWithVersion(TEST_SERVICE_ID_2, TEST_INSTANCE_ID,
        TEST_MAJ_VERSION_2, TEST_MIN_VERSION_2, "bridge0");
    LE_TEST_ASSERT(ServiceRef2 != NULL, "Test RequestServiceWithVersion() with TEST_SERVICE_ID_2.");
    LE_TEST_ASSERT(NULL == taf_someipClnt_RequestServiceWithVersion(TEST_SERVICE_ID_2,
        TEST_INSTANCE_ID, TEST_MAJ_VERSION_2, TEST_MIN_VERSION_2+1, "bridge0"),
        "Test wrong service version.");

    ServiceRef3 = taf_someipClnt_RequestServiceEx(TEST_SERVICE_ID_3, TEST_INSTANCE_ID, "bridge0");
    LE_TEST_ASSERT(ServiceRef3 != NULL, "Test RequestServiceWithVersion() with TEST_SERVICE_ID_3.");
    LE_TEST_ASSERT(NULL == taf_someipClnt_RequestServiceWithVersion(TEST_SERVICE_ID_3,
        TEST_INSTANCE_ID, TAF_SOMEIPDEF_DEFAULT_MAJOR, TAF_SOMEIPDEF_DEFAULT_MINOR, "bridge0"),
        "Test wrong service version.");

    // Register state handlers.
    taf_someipClnt_AddStateChangeHandler(ServiceRef1, StateChangeHandler, NULL);
    taf_someipClnt_AddStateChangeHandler(ServiceRef2, StateChangeHandler, NULL);
    taf_someipClnt_AddStateChangeHandler(ServiceRef3, StateChangeHandler, NULL);

    // Get Init service state.
    taf_someipClnt_State_t state;
    uint8_t majVer;
    uint32_t minVer;
    if (LE_OK == taf_someipClnt_GetState(ServiceRef1, &state))
    {
        LE_TEST_INFO("Service(0x%x/0x%x) Init State is %s.", TEST_SERVICE_ID_1, TEST_INSTANCE_ID,
                     state == TAF_SOMEIPCLNT_AVAILABLE ? "AVAILABLE" : "UNAVAILABLE");

        // Get the service version once it's available.
        if ((state == TAF_SOMEIPCLNT_AVAILABLE) &&
             (LE_OK == taf_someipClnt_GetVersion(ServiceRef1, &majVer, &minVer)))

        {
            LE_TEST_INFO("Service(0x%x/0x%x) Version is '%u.%u.",
                         TEST_SERVICE_ID_1, TEST_INSTANCE_ID, majVer, minVer);
        }
    }
    if (LE_OK == taf_someipClnt_GetState(ServiceRef2, &state))
    {
        LE_TEST_INFO("Service(0x%x/0x%x) Init State is %s.", TEST_SERVICE_ID_2, TEST_INSTANCE_ID,
                     state == TAF_SOMEIPCLNT_AVAILABLE ? "AVAILABLE" : "UNAVAILABLE");

        // Get the service version once it's available.
        if ((state == TAF_SOMEIPCLNT_AVAILABLE) &&
            (LE_OK == taf_someipClnt_GetVersion(ServiceRef2, &majVer, &minVer)))
        {
            LE_TEST_INFO("Service(0x%x/0x%x) Version is '%u.%u.",
                         TEST_SERVICE_ID_2, TEST_INSTANCE_ID, majVer, minVer);
        }
    }
    if (LE_OK == taf_someipClnt_GetState(ServiceRef3, &state))
    {
        LE_TEST_INFO("Service(0x%x/0x%x) Init State is %s.", TEST_SERVICE_ID_3, TEST_INSTANCE_ID,
                     state == TAF_SOMEIPCLNT_AVAILABLE ? "AVAILABLE" : "UNAVAILABLE");

        // Get the service version once it's available.
        if ((state == TAF_SOMEIPCLNT_AVAILABLE) &&
            (LE_OK == taf_someipClnt_GetVersion(ServiceRef3, &majVer, &minVer)))
        {
            LE_TEST_INFO("Service(0x%x/0x%x) Version is '%u.%u.",
                         TEST_SERVICE_ID_3, TEST_INSTANCE_ID, majVer, minVer);
        }
    }

    // Set up the event and group.
    LE_TEST_ASSERT(LE_OK == taf_someipClnt_EnableEventGroup(ServiceRef1,
                                                            TEST_EVENTGROUP_ID,
                                                            TEST_EVENT_ID,
                                                            TAF_SOMEIPDEF_ET_EVENT),
                   "Test taf_someipClnt_EnableEventGroup() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipClnt_EnableEventGroup(ServiceRef2,
                                                            TEST_EVENTGROUP_ID,
                                                            TEST_EVENT_ID,
                                                            TAF_SOMEIPDEF_ET_EVENT),
                   "Test taf_someipClnt_EnableEventGroup() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipClnt_EnableEventGroup(ServiceRef3,
                                                            TEST_EVENTGROUP_ID,
                                                            TEST_EVENT_ID,
                                                            TAF_SOMEIPDEF_ET_EVENT),
                   "Test taf_someipClnt_EnableEventGroup() API.");

    // Subscribe the event group.
    LE_TEST_ASSERT(LE_OK == taf_someipClnt_SubscribeEventGroup(ServiceRef1,
                                                               TEST_EVENTGROUP_ID),
                   "Test taf_someipClnt_SubscribeEventGroup() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipClnt_SubscribeEventGroup(ServiceRef2,
                                                               TEST_EVENTGROUP_ID),
                   "Test taf_someipClnt_SubscribeEventGroup() API.");
    LE_TEST_ASSERT(LE_OK == taf_someipClnt_SubscribeEventGroup(ServiceRef3,
                                                               TEST_EVENTGROUP_ID),
                   "Test taf_someipClnt_SubscribeEventGroup() API.");

    // Add event message handler.
    taf_someipClnt_AddEventMsgHandler(ServiceRef1, TEST_EVENTGROUP_ID, EventMsgHandler, NULL);
    taf_someipClnt_AddEventMsgHandler(ServiceRef2, TEST_EVENTGROUP_ID, EventMsgHandler, NULL);
    taf_someipClnt_AddEventMsgHandler(ServiceRef3, TEST_EVENTGROUP_ID, EventMsgHandler, NULL);

    // Start the send message timer.
    // Start a timer to periodically send the request message and get the response.
    TimerRef = le_timer_Create("mainThread: RequestResponse test Timer");
    le_timer_SetMsInterval(TimerRef, 5000);
    le_timer_SetHandler(TimerRef, TimerHandler);
    le_timer_SetRepeat(TimerRef, 0);
    le_timer_SetWakeup(TimerRef, false);
    le_timer_SetContextPtr(TimerRef, NULL);
    le_timer_Start(TimerRef);
}

COMPONENT_INIT
{
    RequestServiceTest();
}
