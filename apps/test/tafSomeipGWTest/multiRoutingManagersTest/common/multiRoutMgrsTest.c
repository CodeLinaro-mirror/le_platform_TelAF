/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include "multiRoutMgrsTest.h"

//--------------------------------------------------------------------------------------------------
/**
 * Server Test Info pool
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ServerTestInfoPool, TEST_MAX_APP_ID, sizeof(ServerAppInfo_t));

static le_mem_PoolRef_t ServerTestInfoPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Client Test Info pool
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ClientTestInfoPool, TEST_MAX_APP_ID, sizeof(ClientAppInfo_t));

static le_mem_PoolRef_t ClientTestInfoPoolRef = NULL;


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
    static uint32_t evtCnt = 0;
    char eventData[TEST_MAX_MSG_SIZE] = { 0 };

    ServerAppInfo_t* svrInfoPtr = (ServerAppInfo_t*)le_timer_GetContextPtr(timerRef);
    if (svrInfoPtr == NULL)
    {
        LE_TEST_FATAL("NotifyTimerHandler: context is NULL.");
    }

    snprintf(eventData, sizeof(eventData), "event message %u", evtCnt++);

    if (LE_OK != taf_someipSvr_Notify(svrInfoPtr->svrRef, svrInfoPtr->eid,
                                      (uint8_t*)eventData, sizeof(eventData)))
    {
        LE_TEST_FATAL("NotifyTimerHandler: Notify event failed.");
    }

    LE_TEST_INFO("VSOMEIP EVENT SENT [intf='%s',sid=0x%x,eid=0x%x:'%s']",
                 svrInfoPtr->dev, svrInfoPtr->sid, svrInfoPtr->eid, eventData);
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
    uint16_t myServiceId = (uint16_t)contextPtr;

    LE_TEST_INFO("VSOMEIP SUBSCRIPTION HANDLED [sid=0x%x,eid=0x%x:'%s']",
                 myServiceId, eventGroupId, isSubscribed ? "Subscribed" : "Unsubscribed");
}


//--------------------------------------------------------------------------------------------------
/**
 * SOMEIP Rx message Handler
 */
//--------------------------------------------------------------------------------------------------
static void RxMessageHandler
(
    taf_someipSvr_RxMsgRef_t msgRef,
    void* contextPtr
)
{
    uint16_t serviceId = 0;
    uint16_t instanceId = 0;
    uint16_t methodId = 0;
    uint16_t clientId = 0;
    uint8_t msgType = 0;
    size_t payloadSize = TEST_MAX_MSG_SIZE;
    char payloadData[TEST_MAX_MSG_SIZE + 1] = { 0 };

    uint16_t myServiceId = (uint16_t)contextPtr;

    if ((LE_OK != taf_someipSvr_GetServiceId(msgRef, &serviceId, &instanceId)) ||
        (LE_OK != taf_someipSvr_GetMethodId(msgRef, &methodId)) ||
        (LE_OK != taf_someipSvr_GetClientId(msgRef, &clientId)) ||
        (LE_OK != taf_someipSvr_GetMsgType(msgRef, &msgType)) ||
        (LE_OK != taf_someipSvr_GetPayloadData(msgRef, (uint8_t*)payloadData, &payloadSize)))
    {
        LE_TEST_FATAL("RxMessageHandler: Get message information failed.");
    }

    if ((serviceId != myServiceId) || (instanceId != myServiceId) || (methodId != myServiceId) ||
        (msgType != TAF_SOMEIPDEF_MT_REQUEST) || (payloadSize != TEST_MAX_MSG_SIZE))
    {
        LE_TEST_FATAL("RxMessageHander: Msg header check failed.");
    }


    LE_TEST_INFO("VSOMEIP REQUEST RECEIVED [sid=0x%x,cid=0x%x:'%s']",
                 myServiceId, clientId, payloadData);

    if (LE_OK != taf_someipSvr_SendResponse(msgRef, false, TAF_SOMEIPDEF_E_OK,
                                             (uint8_t*)payloadData, payloadSize))
    {
        LE_TEST_FATAL("RxMessageHandler: Send message response failed.");
    }
}


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
    uint16_t myServiceId = (uint16_t)contextPtr;

    LE_TEST_INFO("VSOMEIP STATE CHANGED [sid=0x%x:'%s']", myServiceId,
                 state == TAF_SOMEIPCLNT_AVAILABLE ? "AVAILABLE" : "UNAVAILABLE");
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
    char eventData[TEST_MAX_MSG_SIZE + 1] = { 0 };
    uint16_t myEventId = (uint16_t)contextPtr;

    if (dataSize != TEST_MAX_MSG_SIZE)
    {
        LE_TEST_FATAL("EventMsgHandler: Invalid dataSize.");
    }

    memcpy(eventData, dataPtr, dataSize);

    LE_INFO("VSOMEIP EVENT RECEIVED [eid=0x%x:'%s']", myEventId, eventData);
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
    char payloadData[TEST_MAX_MSG_SIZE + 1] = { 0 };
    uint16_t myServiceId = (uint16_t)contextPtr;

    LE_INFO("VSOMEIP RESPONSE HANDLED: [%s] [%s] [%d]", LE_RESULT_TXT(result),
            isErrRsp ? "MT_ERROR" : "MT_RESPONSE", returnCode);

    if ((result == LE_OK) && (dataPtr != NULL) && (dataSize == TEST_MAX_MSG_SIZE))
    {
        memcpy((uint8_t*)payloadData, dataPtr, dataSize);
        LE_TEST_INFO("VSOMEIP RESPONSE RECEIVED [sid=0x%x:'%s']", myServiceId, payloadData);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * The request timer handler
 */
//--------------------------------------------------------------------------------------------------
static void RequestTimerHandler
(
    le_timer_Ref_t timerRef
)
{
	static uint32_t msgCnt = 0;
    char payloadData[TEST_MAX_MSG_SIZE] = { 0 };

    ClientAppInfo_t* cliInfoPtr = (ClientAppInfo_t*)le_timer_GetContextPtr(timerRef);
    if (cliInfoPtr == NULL)
    {
        LE_TEST_FATAL("RequestTimerHandler: context is NULL.");
    }

    taf_someipClnt_TxMsgRef_t txMsgRef =
        taf_someipClnt_CreateMsg(cliInfoPtr->cliRef, cliInfoPtr->sid);
    if (txMsgRef == NULL)
    {
        LE_TEST_FATAL("ReqeustTimerHandler: txMsgRef is NULL.");
    }

    snprintf(payloadData, sizeof(payloadData), "command message %u", msgCnt++);

    if ((LE_OK != taf_someipClnt_SetTimeout(txMsgRef, 1000)) ||
        (LE_OK != taf_someipClnt_SetPayload(txMsgRef, (uint8_t*)payloadData, sizeof(payloadData))))
    {
        LE_TEST_FATAL("RequestTimerHandler: Set message payload failed.");
    }

    taf_someipClnt_RequestResponse(txMsgRef, ResponseHandler, (void*)cliInfoPtr->sid);

    LE_TEST_INFO("VSOMEIP REQUEST SENT [intf='%s',sid=0x%x:'%s']",
                 cliInfoPtr->dev, cliInfoPtr->sid, payloadData);
}


//--------------------------------------------------------------------------------------------------
/**
 * Data struct initialization.
 */
//--------------------------------------------------------------------------------------------------
void multiRoutMgrTest_DataInit
(
    void
)
{
    ServerTestInfoPoolRef =
        le_mem_InitStaticPool(ServerTestInfoPool, TEST_MAX_APP_ID, sizeof(ServerAppInfo_t));

    ClientTestInfoPoolRef =
        le_mem_InitStaticPool(ClientTestInfoPool, TEST_MAX_APP_ID, sizeof(ClientAppInfo_t));
}


//--------------------------------------------------------------------------------------------------
/**
 * Offer a service with a given test app ID.
 */
//--------------------------------------------------------------------------------------------------
void multiRoutMgrTest_OfferService
(
    AppTestId_t appId, ///< [IN]
    ServerAppInfo_t* serverInfoPtr   ///< [OUT]
)
{
    LE_UNUSED(serverInfoPtr);

    uint16_t myServiceId = 0;
    uint16_t myPort = 0;
    uint16_t myEventId = 0;
    char myDevName[TAF_SOMEIPDEF_MAX_IFNAME_LENGTH] = "";

    taf_someipSvr_ServiceRef_t myServiceRef = NULL;
    taf_someipSvr_RxMsgHandlerRef_t myRxMsgHandlerRef = NULL;
    taf_someipSvr_SubscriptionHandlerRef_t mySubsHandlerRef = NULL;
    le_timer_Ref_t myNotifyTimerRef = NULL;

    switch (appId)
    {
        case RT0_APP_A:
            myServiceId = RT0_SID_A;
            myPort = RT0_PORT_A;
            myEventId = RT0_EID_A;
            le_utf8_Copy(myDevName, RT0_DEV_NAME, sizeof(myDevName), NULL);
            break;

        case RT0_APP_B:
            myServiceId = RT0_SID_B;
            myPort = RT0_PORT_B;
            myEventId = RT0_EID_B;
            le_utf8_Copy(myDevName, RT0_DEV_NAME, sizeof(myDevName), NULL);
            break;

        case RT1_APP_A:
            myServiceId = RT1_SID_A;
            myPort = RT1_PORT_A;
            myEventId = RT1_EID_A;
            le_utf8_Copy(myDevName, RT1_DEV_NAME, sizeof(myDevName), NULL);
            break;

        case RT1_APP_B:
            myServiceId = RT1_SID_B;
            myPort = RT1_PORT_B;
            myEventId = RT1_EID_B;
            le_utf8_Copy(myDevName, RT1_DEV_NAME, sizeof(myDevName), NULL);
            break;

        case RT2_APP_A:
            myServiceId = RT2_SID_A;
            myPort = RT2_PORT_A;
            myEventId = RT2_EID_A;
            le_utf8_Copy(myDevName, RT2_DEV_NAME, sizeof(myDevName), NULL);
            break;

        case RT2_APP_B:
            myServiceId = RT2_SID_B;
            myPort = RT2_PORT_B;
            myEventId = RT2_EID_B;
            le_utf8_Copy(myDevName, RT2_DEV_NAME, sizeof(myDevName), NULL);
            break;

        default:
            LE_TEST_FATAL("OfferService: Check appId failed.");
            break;
    }

    // Error injection test. Input an invalid interfaceName will always return NULL.
    LE_TEST_ASSERT(NULL == taf_someipSvr_GetServiceEx(myServiceId, myServiceId, "abcdefg"),
                   "taf_someipSvr_GetServiceEx() API.");

    myServiceRef =
        taf_someipSvr_GetServiceEx(myServiceId, myServiceId, myDevName);
    LE_TEST_ASSERT(myServiceRef != NULL, "taf_someipSvr_GetServiceEx() API.");

    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServiceVersion(myServiceRef,
                   TEST_MAJ_VERSION, TEST_MIN_VERSION), "taf_someipSvr_SetServiceVersion() API.");

    LE_TEST_ASSERT(LE_OK == taf_someipSvr_SetServicePort(myServiceRef,
                   myPort, myPort, true), "taf_someipSvr_SetServicePort() API.");

    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferService(myServiceRef),
                   "taf_someipSvr_OfferService() API.");

    myRxMsgHandlerRef =
        taf_someipSvr_AddRxMsgHandler(myServiceRef, RxMessageHandler, (void*)myServiceId);
    LE_TEST_ASSERT(NULL != myRxMsgHandlerRef, "taf_someipSvr_AddRxMsgHandler() API.");

    mySubsHandlerRef =
        taf_someipSvr_AddSubscriptionHandler(myServiceRef, myEventId,
                                             SubscriptionHandler, (void*)myServiceId);
    LE_TEST_ASSERT(NULL != mySubsHandlerRef,
                   "taf_someipSvr_AddSubscriptionHandler() API.");

    LE_TEST_ASSERT(LE_OK ==
        taf_someipSvr_EnableEvent(myServiceRef, myEventId, myEventId),
        "taf_someipSvr_EnableEvent() API.");

    LE_TEST_ASSERT(LE_OK == taf_someipSvr_OfferEvent(myServiceRef, myEventId),
        "taf_someipSvr_OfferEvent() API.");


    char timerName[64] = "";
    snprintf(timerName, sizeof(timerName), "NotifyTimer(0x%x)", myServiceId);
    myNotifyTimerRef = le_timer_Create(timerName);

    ServerAppInfo_t* svrInfoPtr = le_mem_TryAlloc(ServerTestInfoPoolRef);
    if (svrInfoPtr == NULL)
    {
        LE_TEST_FATAL("svrInfoPtr is NULL.");
    }
    memset(svrInfoPtr, 0, sizeof(ServerAppInfo_t));
    svrInfoPtr->sid = myServiceId;
    svrInfoPtr->eid = myEventId;
    svrInfoPtr->port = myPort;
    le_utf8_Copy(svrInfoPtr->dev, myDevName, sizeof(svrInfoPtr->dev), NULL);
    svrInfoPtr->svrRef = myServiceRef;
    svrInfoPtr->rxRef = myRxMsgHandlerRef;
    svrInfoPtr->subsRef = mySubsHandlerRef;
    svrInfoPtr->ntimerRef = myNotifyTimerRef;

    le_timer_SetMsInterval(myNotifyTimerRef, 5000);
    le_timer_SetHandler(myNotifyTimerRef, NotifyTimerHandler);
    le_timer_SetRepeat(myNotifyTimerRef, 0);
    le_timer_SetWakeup(myNotifyTimerRef, false);
    le_timer_SetContextPtr(myNotifyTimerRef, svrInfoPtr);
    le_timer_Start(myNotifyTimerRef);

    LE_TEST_INFO("OFFER TEST SERVICE [intf:'%s',sid=0x%x,eid=0x%x,port=%u]",
                 myDevName, myServiceId, myEventId, myPort);
}


//--------------------------------------------------------------------------------------------------
/**
 * Request service with a given test app ID.
 */
//--------------------------------------------------------------------------------------------------
void multiRoutMgrTest_RequestService
(
    AppTestId_t appId, ///< [IN]
    ClientAppInfo_t* ClientInfoPtr   ///< [OUT]
)
{
    LE_UNUSED(ClientInfoPtr);

    uint16_t myServiceId = 0;
    uint16_t myEventId = 0;
    char myDevName[TAF_SOMEIPDEF_MAX_IFNAME_LENGTH] = "";

    taf_someipClnt_ServiceRef_t myServiceRef = NULL;
    taf_someipClnt_StateChangeHandlerRef_t myStateHandlerRef = NULL;
    taf_someipClnt_EventMsgHandlerRef_t myEventHandlerRef = NULL;
    le_timer_Ref_t mySendTimerRef = NULL;

    switch (appId)
    {
        case RT0_APP_A:
            myServiceId = RT0_SID_A;
            myEventId = RT0_EID_A;
            le_utf8_Copy(myDevName, RT0_DEV_NAME, sizeof(myDevName), NULL);
            break;

        case RT0_APP_B:
            myServiceId = RT0_SID_B;
            myEventId = RT0_EID_B;
            le_utf8_Copy(myDevName, RT0_DEV_NAME, sizeof(myDevName), NULL);
            break;

        case RT1_APP_A:
            myServiceId = RT1_SID_A;
            myEventId = RT1_EID_A;
            le_utf8_Copy(myDevName, RT1_DEV_NAME, sizeof(myDevName), NULL);
            break;

        case RT1_APP_B:
            myServiceId = RT1_SID_B;
            myEventId = RT1_EID_B;
            le_utf8_Copy(myDevName, RT1_DEV_NAME, sizeof(myDevName), NULL);
            break;

        case RT2_APP_A:
            myServiceId = RT2_SID_A;
            myEventId = RT2_EID_A;
            le_utf8_Copy(myDevName, RT2_DEV_NAME, sizeof(myDevName), NULL);
            break;

        case RT2_APP_B:
            myServiceId = RT2_SID_B;
            myEventId = RT2_EID_B;
            le_utf8_Copy(myDevName, RT2_DEV_NAME, sizeof(myDevName), NULL);
            break;

        default:
            LE_TEST_FATAL("RequestService: Check appId failed.");
            break;
    }

    // Error injection test with an invalid interface name.
    LE_TEST_ASSERT(NULL == taf_someipClnt_RequestServiceEx(myServiceId, myServiceId, "hijklmn"),
                   "taf_someipClnt_RequestServiceEx() API.");
    // Error injection test with an invalid interface name.
    uint16_t myClientId = 0;
    LE_TEST_ASSERT(LE_OK != taf_someipClnt_GetClientIdEx("hijklmn", &myClientId),
                   "taf_someipClnt_GetClientIdEx() API.");


    myServiceRef = taf_someipClnt_RequestServiceEx(myServiceId, myServiceId, myDevName);
    LE_TEST_ASSERT(myServiceRef != NULL, "taf_someipClnt_RequestServiceEx() API.");

    LE_TEST_ASSERT(LE_OK == taf_someipClnt_GetClientIdEx(myDevName, &myClientId),
                   "taf_someipClnt_GetClientIdEx() API (myClientId=0x%x).", myClientId);

    myStateHandlerRef =
        taf_someipClnt_AddStateChangeHandler(myServiceRef, StateChangeHandler, (void*)myServiceId);
    LE_TEST_ASSERT(myStateHandlerRef != NULL, "taf_someipClnt_AddStateChangeHandler() API.");

    LE_TEST_ASSERT(LE_OK == taf_someipClnt_EnableEventGroup(myServiceRef, myEventId, myEventId,
                                                            TAF_SOMEIPDEF_ET_EVENT),
                   "taf_someipClnt_EnableEventGroup() API.");

    LE_TEST_ASSERT(LE_OK == taf_someipClnt_SubscribeEventGroup(myServiceRef, myEventId),
                   "taf_someipClnt_SubscribeEventGroup() API.");

    myEventHandlerRef =
        taf_someipClnt_AddEventMsgHandler(myServiceRef, myEventId,
                                          EventMsgHandler, (void*)myEventId);
    LE_TEST_ASSERT(myEventHandlerRef != NULL, "taf_someipClnt_AddEventMsgHandler() API.");


    char timerName[64] = "";
    snprintf(timerName, sizeof(timerName), "SendTimer(0x%x)", myServiceId);
    mySendTimerRef = le_timer_Create(timerName);

    ClientAppInfo_t* cliInfoPtr = le_mem_TryAlloc(ClientTestInfoPoolRef);
    if (cliInfoPtr == NULL)
    {
        LE_TEST_FATAL("cliInfoPtr is NULL.");
    }
    memset(cliInfoPtr, 0, sizeof(ClientAppInfo_t));
    cliInfoPtr->sid = myServiceId;
    cliInfoPtr->eid = myEventId;
    le_utf8_Copy(cliInfoPtr->dev, myDevName, sizeof(cliInfoPtr->dev), NULL);
    cliInfoPtr->cliRef = myServiceRef;
    cliInfoPtr->stateRef = myStateHandlerRef;
    cliInfoPtr->evtRef = myEventHandlerRef;
    cliInfoPtr->stimerRef = mySendTimerRef;

    le_timer_SetMsInterval(mySendTimerRef, 5000);
    le_timer_SetHandler(mySendTimerRef, RequestTimerHandler);
    le_timer_SetRepeat(mySendTimerRef, 0);
    le_timer_SetWakeup(mySendTimerRef, false);
    le_timer_SetContextPtr(mySendTimerRef, cliInfoPtr);
    le_timer_Start(mySendTimerRef);

    LE_TEST_INFO("REQUEST TEST SERVICE [intf:'%s',sid=0x%x,eid=0x%x]",
                 myDevName, myServiceId, myEventId);
}
