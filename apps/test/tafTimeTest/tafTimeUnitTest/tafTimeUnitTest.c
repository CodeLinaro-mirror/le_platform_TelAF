/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include "taf_gptpTime.h"

static taf_time_TimeSourceChangeHandlerRef_t TimeSourceChangeHandlerRef = NULL;
static taf_time_TimeValueChangeHandlerRef_t TimeValueChangeHandlerRef = NULL;
static taf_time_TimeSourceStatusHandlerRef_t TimeSourceStatusHandlerRef = NULL;

#define TAF_GPTP_DEVICE_0          "/dev/ptp0"

//--------------------------------------------------------------------------------------------------
/**
 * For automated test cases, no manual operation or waiting is required. However, for integration
 * test cases, manual input is necessary. Here use this flag to separate the different scenarios.
 */
//--------------------------------------------------------------------------------------------------
bool NeedWaitHandlerEvent = false;

//--------------------------------------------------------------------------------------------------
/**
 * Covert time source index name to string.
 */
//--------------------------------------------------------------------------------------------------
const char* SourceNameIndexToStr
(
    taf_time_TimeSources_t sourceName
)
{
    switch (sourceName)
    {
        case TAF_TIME_SRC_NAME_RTC:
            return "RTC";

        case TAF_TIME_SRC_NAME_GNSS:
            return "GNSS";

        case TAF_TIME_SRC_NAME_EX_APP:
            return "ExAPP";

        case TAF_TIME_SRC_NAME_NETWORK:
            return "NETWORK";

        case TAF_TIME_SRC_NAME_NETWORK2:
            return "NETWORK2";

        // Add new time source here

        case TAF_TIME_SRC_NAME_UNKNOWN:
            return "UNKNOWN";

        case TAF_TIME_SRC_NAME_SYSTEM:
            return "SYSTEM";

    }
    return "unknown";
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert the seconds from epoch to date time format.
 */
//--------------------------------------------------------------------------------------------------
void ConvertSecToDateTime
(
    taf_time_TimeSpec_t timeVal
)
{
    time_t epoch_seconds = timeVal.sec;
    struct tm *timeinfo = gmtime(&epoch_seconds);
    char tmpBuffer[80];

    if (timeinfo != NULL)
    {
        strftime(tmpBuffer, 80, "%c", timeinfo);
        LE_INFO("UTC time: %s\n", tmpBuffer);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for time source change that used by system.
 */
//--------------------------------------------------------------------------------------------------
void TimeSourceChangeHandler
(
    taf_time_TimeSources_t PreTimeSource,
    taf_time_TimeSources_t NewTimeSource
)
{
    // Just verify the API, there is NO call back in this test
    if (PreTimeSource == NewTimeSource)
    {
        LE_INFO("Notification - No available time source. Old %d, New %d\n",
                                            PreTimeSource, NewTimeSource);
    }
    else
    {
        LE_INFO("Notification - TimeSourceChange: Old %d, New %d\n",
                                            PreTimeSource, NewTimeSource);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for time source which is using reference and registered to time service.
 */
//--------------------------------------------------------------------------------------------------
void TimeValueChangeHandler
(
    taf_time_TimeRef_t timeSrcRef,
    taf_time_TimeSpec_t* sourceTime,
    void* contextPtr
)
{
    le_result_t result;
    taf_time_TimeSpec_t time;
    LE_INFO("Reference(%p) time from notification is %"PRIu64".%"PRIu64,
                                 timeSrcRef, sourceTime->sec, sourceTime->nanosec);

    // Get reference system time through reference object.
    result = taf_time_GetRefSystemTime(timeSrcRef, &time);
    LE_TEST_ASSERT(result == LE_OK || result == LE_UNAVAILABLE,
                             "taf_time_GetRefSystemTime() API.");
    if (result == LE_OK)
    {
        LE_INFO("Reference system time is %"PRIu64".%"PRIu64, time.sec, time.nanosec);
    }

    // Get reference gptp time through reference object.
    result = taf_time_GetRefGptpTime(timeSrcRef, &time);
    LE_TEST_ASSERT(result == LE_OK || result == LE_UNAVAILABLE,
                                "taf_time_GetRefGptpTime() API.");
    if (result == LE_OK)
    {
        LE_INFO("Reference gptp time is %"PRIu64".%"PRIu64, time.sec, time.nanosec);
    }

    if (NeedWaitHandlerEvent)
        le_sem_Post((le_sem_Ref_t)contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set system time through parameter 'CLOCK_REALTIME'.
 */
//--------------------------------------------------------------------------------------------------
void TestSetSystemTime
(
    void
)
{
    static le_mem_PoolRef_t NewTimePool = NULL;
    le_result_t result;
    taf_time_TimeSpec_t* newTimePtr;
    NewTimePool = le_mem_CreatePool("NewTimePool", sizeof(taf_time_TimeSpec_t));
    newTimePtr = (taf_time_TimeSpec_t*)le_mem_ForceAlloc(NewTimePool);

    newTimePtr->sec = 1667788990;
    newTimePtr->nanosec = 10000;

    taf_time_TimeSources_t sourceId = TAF_TIME_SRC_NAME_EX_APP;
    taf_time_SourceRef_t srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);

    result = taf_time_SetTrustTime(srcRef, (const taf_time_TimeSpec_t*)newTimePtr, false);
    LE_TEST_ASSERT(result == LE_OK, "Test: taf_time_SetTrustTime() APIs - true");
    LE_INFO("Set the time to %"PRIu64".%"PRIu64 ", validity: true",
                                                   newTimePtr->sec, newTimePtr->nanosec);

    le_thread_Sleep(1);
    newTimePtr->sec += 20000000;

    result = taf_time_SetTrustTime(srcRef, (const taf_time_TimeSpec_t*)newTimePtr, true);
    LE_TEST_ASSERT(result == LE_OK, "Test: taf_time_SetTrustTime() APIs - false");
    LE_INFO("Set the time to %"PRIu64".%"PRIu64 ", validity: false",
                                                    newTimePtr->sec, newTimePtr->nanosec);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get time through time source ID and return related reference.
 */
//--------------------------------------------------------------------------------------------------
void TestGetTimeRef
(
    void
)
{
    le_result_t result;
    taf_time_TimeSpec_t time;
    const char* arg2 = "1";
    if (arg2 == NULL)
    {
        LE_ERROR("Invalid arguments.");
        return;
    }
    uint8_t sourceId = strtol(arg2, NULL, 10);
    taf_time_TimeRef_t timeSrcRef;

    timeSrcRef = taf_time_GetTimeRef(sourceId);
    LE_TEST_ASSERT(timeSrcRef != NULL, "taf_time_GetTimeRef() API.");

    // Get time from a specify time source and related Reference.
    result = taf_time_GetTime(timeSrcRef, &time);
    LE_TEST_ASSERT(result == LE_OK || result == LE_UNAVAILABLE,
        "taf_time_GetTime() API.");
    if (result == LE_OK)
    {
        LE_INFO("Reference %s time is %"PRIu64".%"PRIu64,
            SourceNameIndexToStr(sourceId), time.sec, time.nanosec);
        ConvertSecToDateTime(time);
    }
    LE_INFO("timeSrcRef %p, sourceId (0x%x), name %s, result %d.",
                        timeSrcRef, sourceId, SourceNameIndexToStr(sourceId), result);

    // Get reference system time through reference object.
    result = taf_time_GetRefSystemTime(timeSrcRef, &time);
    LE_TEST_ASSERT(result == LE_OK || result == LE_UNAVAILABLE,
        "taf_time_GetRefSystemTime() API.");
    if (result == LE_OK)
    {
        LE_INFO("Reference system time is %"PRIu64".%"PRIu64, time.sec, time.nanosec);
        ConvertSecToDateTime(time);
    }

    // Get reference gptp time through reference object.
    result = taf_time_GetRefGptpTime(timeSrcRef, &time);
    LE_TEST_ASSERT(result == LE_OK || result == LE_UNAVAILABLE,
        "taf_time_GetRefGptpTime() API.");
    if (result == LE_OK)
    {
        LE_INFO("Reference gptp time is %"PRIu64".%"PRIu64, time.sec, time.nanosec);
        ConvertSecToDateTime(time);
    }

    // Release the memory for this reference.
    result = taf_time_ReleaseTimeRef(timeSrcRef);
    if (result == LE_OK)
    {
        LE_INFO("The reference was successfully removed\n");
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Adds a handler to monitor the system time source type.
 * The handler will receive a notification whenever the source type changes.
 */
 //------------------------------------------------------------------------------------------------
void* TimeSrcChaHandlerTestThread
(
    void* contextPtr ///< [IN] Thread context.
)
{
    // Connect to service.
    taf_time_ConnectService();

    // Test time source change notification
    TimeSourceChangeHandlerRef = taf_time_AddTimeSourceChangeHandler(
        (taf_time_TimeSourceChangeHandlerFunc_t)TimeSourceChangeHandler, NULL);
    LE_TEST_OK(TimeSourceChangeHandlerRef != NULL, "taf_time_AddTimeSourceChangeHandler - OK");

    le_sem_Post((le_sem_Ref_t)contextPtr);
    le_event_RunLoop();

    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * This thread is created for adding handlers for testing reference time source notification.
 */
 //------------------------------------------------------------------------------------------------
void* TimeValueChangeHandlerTestThread
(
    void* contextPtr ///< [IN] Thread context.
)
{
    const char* arg2 = "3"; // Use 'NETWORK' for time value change test.
    if (arg2 == NULL)
    {
        LE_ERROR("Invalid arguments.");
        return NULL;
    }
    uint8_t sourceId = strtol(arg2, NULL, 10);

    // Connect to service.
    taf_time_ConnectService();

    // Register Reference time source got change Handler.
    TimeValueChangeHandlerRef = taf_time_AddTimeValueChangeHandler(sourceId,
        (taf_time_TimeValueChangeHandlerFunc_t)TimeValueChangeHandler, (le_sem_Ref_t)contextPtr);
    LE_TEST_OK(TimeValueChangeHandlerRef != NULL, "taf_time_AddTimeValueChangeHandler() - OK");

    if (!NeedWaitHandlerEvent)
        le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove handlers for time source status change notification.
 */
 //------------------------------------------------------------------------------------------------
void RemoveTestHandler
(
    void
)
{
    // Remove handler
    taf_time_RemoveTimeSourceChangeHandler(TimeSourceChangeHandlerRef);
    LE_TEST_OK(true, "taf_time_RemoveTimeSourceChangeHandler - OK");
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove handlers for reference time source.
 */
 //------------------------------------------------------------------------------------------------
void RemoveRefTimeTestHandler
(
    void
)
{
    // Remove handler
    taf_time_RemoveTimeValueChangeHandler(TimeValueChangeHandlerRef);
    LE_TEST_OK(true, "taf_time_RemoveTimeValueChangeHandler - OK");
}

//-------------------------------------------------------------------------------------------------
/**
 * Create thread for test handler.
 */
 //------------------------------------------------------------------------------------------------
void CreateTimeSourceChangeHandlerTestThread
(
    void
)
{
    le_sem_Ref_t semaphore = le_sem_Create("timeSemaphore", 0);
    le_thread_Ref_t threadRef = le_thread_Create("TimeSrcChangeTh",
        TimeSrcChaHandlerTestThread, (void*)semaphore);
    le_thread_Start(threadRef);

    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);
}

//-------------------------------------------------------------------------------------------------
/**
 * Create thread for test handler.
 */
 //------------------------------------------------------------------------------------------------
void CreateTimeValueChangeHandlerTestThread
(
    long time
)
{
    le_sem_Ref_t semaphore = le_sem_Create("timeSemaphore", 0);
    le_thread_Ref_t threadRef = le_thread_Create("TimeSrcChangeTh",
        TimeValueChangeHandlerTestThread, (void*)semaphore);
    le_thread_Start(threadRef);

    le_thread_Sleep(time);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test RTC Async get time API.
 */
//--------------------------------------------------------------------------------------------------
static void getRTCTimeAsync(const taf_time_TimeSpec_t* timeVal,
    le_result_t responseState, void* contextPtr)
{
    LE_INFO("responseState: %d", responseState);
    LE_TEST_ASSERT(responseState == LE_OK, "Test: getRTCTimeAsync response mode is LE_OK");
    LE_INFO("Received async vhal RTC time is %"PRIu64".%"PRIu64, timeVal->sec, timeVal->nanosec);

    le_sem_Post((le_sem_Ref_t)contextPtr);
}

void* TestGetRTCAsync(void* ctxPtr)
{
    taf_time_ConnectService();
    le_result_t res = taf_time_GetRtcTimeReqAsync(getRTCTimeAsync, (void*)ctxPtr);
    LE_TEST_ASSERT(res == LE_OK || res == LE_UNSUPPORTED, "taf_time_GetRtcTimeReqAsync - OK");

    if (res == LE_UNSUPPORTED)
    {
        // If RTC VHAL was not installed, for RTC async API it will report LE_UNSUPPORTED
        // and without callback, so need to release the semphone here.
        LE_INFO("RTC Async get time API (work with VHAL) received: Unsupported");
        le_sem_Post((le_sem_Ref_t)ctxPtr);
    }

    le_event_RunLoop();
    return NULL;
}

void TestRtcVhalAsyncGetTime
(
    void
)
{
    le_sem_Ref_t semAGetRtcVhal = le_sem_Create("AsynGetRtcVhal", 0);
    le_thread_Ref_t threadRef = le_thread_Create("TestGetRTCAsyncThread",
        TestGetRTCAsync, (void*)semAGetRtcVhal);

    le_thread_Start(threadRef);

    le_sem_Wait(semAGetRtcVhal);
    le_sem_Delete(semAGetRtcVhal);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the information related to a time source.
 */
 //--------------------------------------------------------------------------------------------------
void TestGetSourceDetails
(
    void
)
{
    const char* arg2 = "255"; // Use system time ID as example
    if (arg2 == NULL)
    {
        LE_ERROR("Invalid arguments.");
        return;
    }
    uint8_t sourceId = strtol(arg2, NULL, 10);
    taf_time_SourceRef_t srcRef;

    srcRef = taf_time_GetSourceRef(sourceId);
    LE_TEST_ASSERT(srcRef != NULL, "taf_time_GetSourceRef() API - OK.");
    int32_t failedLoops =0;
    int64_t loopIntervalSec = 0;
    bool isAvailable, validity;

    le_result_t res = taf_time_GetFailedLoops(srcRef, &failedLoops, &loopIntervalSec);
    LE_TEST_ASSERT(res == LE_OK, "taf_time_GetFailedLoops() API - OK.");

    LE_INFO("The number of failed loops are %d. Loop interval is  %" PRId64 "",
    failedLoops, loopIntervalSec);

    isAvailable = taf_time_IsAvailable(srcRef);
    LE_INFO("Time source: %s is %s",
          SourceNameIndexToStr(sourceId), isAvailable ? "Available" : "NOT Available");

    validity = taf_time_IsSourceValid(srcRef);
    LE_INFO("Time source: %s is %s",
          SourceNameIndexToStr(sourceId), validity ? "valid" : "NOT valid");
}

void TestGetSystemTimeSourceID()
{
    taf_time_TimeSources_t systemTimeSrc;
    le_result_t res = taf_time_GetSystemTimeSourceID(&systemTimeSrc);
    LE_ASSERT(res == LE_OK);
    LE_INFO("The lastest system time is set by %s", SourceNameIndexToStr(systemTimeSrc));
}

void TestGetTimeZone
(
    void
)
{
    const char* arg2 = "3"; // NETWORK
    if (arg2 == NULL)
    {
        LE_ERROR("Invalid arguments.");
        return;
    }
    uint8_t sourceId = strtol(arg2, NULL, 10);
    taf_time_SourceRef_t srcRef;

    srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);
    int8_t timeZone = 0;
    le_result_t res = taf_time_GetTimeZone(srcRef, &timeZone);
    LE_TEST_ASSERT(res == LE_OK, "taf_time_GetTimeZone - OK. TimeZone is %d", timeZone);
}

void TestGetDayAdj
(
    void
)
{
    const char* arg2 = "3"; // NETWORK
    if (arg2 == NULL)
    {
        LE_ERROR("Invalid arguments.");
        return;
    }
    uint8_t sourceId = strtol(arg2, NULL, 10);
    taf_time_SourceRef_t srcRef;

    srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);
    uint8_t dayltSavAdj;
    le_result_t res = taf_time_GetTimeDayAdj(srcRef, &dayltSavAdj);
    LE_TEST_ASSERT(res == LE_OK, "taf_time_GetTimeDayAdj - OK. Day Light Saving is %d", dayltSavAdj);
}

void TestFailedLoops()
{
    uint8_t sourceId = TAF_TIME_SRC_NAME_RTC;
    taf_time_SourceRef_t srcRef;
    int32_t failedLoops =0;
    int64_t loopIntervalSec = 0;
    srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);
    le_result_t res = taf_time_GetFailedLoops(srcRef, &failedLoops, &loopIntervalSec);
    LE_ASSERT(res == LE_OK);
    LE_INFO("The number of failed loops are %d. Loop interval is  %" PRIu64 "",
    failedLoops, loopIntervalSec);
}

void TestGetSourceAvailability()
{
    uint8_t sourceId = TAF_TIME_SRC_NAME_NETWORK;
    taf_time_SourceRef_t srcRef;
    bool isAvailable;
    srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);
    isAvailable = taf_time_IsAvailable(srcRef);

    LE_INFO("Time source: %s is %s",
          SourceNameIndexToStr(sourceId), isAvailable ? "Available" : "NOT Available");
}

void TestGetSourceValidity()
{
    uint8_t sourceId = TAF_TIME_SRC_NAME_NETWORK;
    taf_time_SourceRef_t srcRef;
    bool validity;
    srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);
    validity = taf_time_IsSourceValid(srcRef);

    LE_INFO("Time source: %s is %s",
          SourceNameIndexToStr(sourceId), validity ? "valid" : "NOT valid");
}

//--------------------------------------------------------------------------------------------------
/**
 * Set time and validity for system
 */
//--------------------------------------------------------------------------------------------------
void TestSetTrustTime
(
    const char* arg2,
    const char* arg3,
    const char* arg4,
    bool * flag
)
{
    taf_time_TimeSpec_t newTime = {0};

    bool validityFlag = false;
    uint8_t validity = 0;

    if (arg2 == NULL || arg3 == NULL)
    {
        LE_ERROR("Parameter is not correct");
        return;
    }
    newTime.sec = strtol(arg2, NULL, 10);
    newTime.nanosec = strtol(arg3, NULL, 10);

    if (arg4 != NULL )
    {
        validity = strtol(arg4, NULL, 10);
    }
    validityFlag = (validity == 0) ? false : true;

    taf_time_TimeSources_t sourceId = TAF_TIME_SRC_NAME_EX_APP;
    taf_time_SourceRef_t srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);

    LE_INFO("Set trust time %"PRIu64".%"PRIu64 ", validity: %d",
        newTime.sec, newTime.nanosec, validityFlag);

    le_result_t result = taf_time_SetTrustTime(srcRef, &newTime, validityFlag);
    LE_TEST_ASSERT(result == LE_OK, "Test: taf_time_SetTrustTime() APIs.");

    validityFlag = taf_time_IsSourceValid(srcRef);
    LE_INFO("Validity of %s is set to %s",
    SourceNameIndexToStr(sourceId), validityFlag ? "true" : "false");
    *flag = validityFlag;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test set time and validity for the system
 */
//--------------------------------------------------------------------------------------------------
void TestSetTrustTimeAPI
(
    void
)
{
    bool validityGet = false;

    //-----------------------------------------------------------------------------
    //Test set 'true' case:
    bool validitySet = true;

    TestSetTrustTime("1777668866", "300", "1", &validityGet);
    LE_TEST_ASSERT(validitySet == validityGet, "Test: taf_time_SetTrustTime() APIs.");

    sleep(1);
    //-----------------------------------------------------------------------------
    //Test set 'false' case:
    validitySet = false;
    TestSetTrustTime("1777668899", "500", "0", &validityGet);
    LE_TEST_ASSERT(validitySet == validityGet, "Test: taf_time_SetTrustTime() APIs.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler to monitor the time source change for the system and use set time API trigger
 * a notification.
 */
//--------------------------------------------------------------------------------------------------
void SystemTimeSourceChangeHandlerTest(void)
{
    const char* arg1 = "3"; // Wait 3 seconds for time source change test.
    if (arg1 != NULL)
    {
        long time = strtol(arg1, NULL, 10);
        CreateTimeSourceChangeHandlerTestThread();

        //Try to trigger response (Assume change Other time source to ExAPP)
        TestSetSystemTime();

        // Wait for handler's response.
        le_thread_Sleep(time);

        RemoveTestHandler();
    }
}

void TimeValueChangeHandlerTest(void)
{
    const char* arg1 = "3"; // Wait 3 seconds for the value change test.
    if (arg1 != NULL)
    {
        long time = strtol(arg1, NULL, 10);
        CreateTimeValueChangeHandlerTestThread(time);

        RemoveRefTimeTestHandler();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for time source change that used by system.
 */
//--------------------------------------------------------------------------------------------------
void TimeSourceStatusHandler
(
    taf_time_SourceRef_t sourceRef,
    taf_time_StatusEventType_t eventType,
    bool status,
    void* contextPtr
)
{
    LE_ASSERT(sourceRef != NULL);
    if(eventType == TAF_TIME_STATUS_EVENT_AVAILABILITY)
    {

        LE_INFO("Time source is %s", status ? "Available" : "NOT Available");
    }
    if(eventType == TAF_TIME_STATUS_EVENT_VALIDITY)
    {
        LE_INFO("Time source is %s", status ? "valid" : "NOT valid");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test adding handler for time source status change notification.
 */
//--------------------------------------------------------------------------------------------------
void* TimeSourceStatusHandlerTestThread(
    void* contextPtr ///< [IN] Thread context.
)
{
    // Connect to service.
    taf_time_ConnectService();
    const char* arg1 = "2"; // ExAPP Can be triggered through API and help procces this test.
    const char* arg2 = "3"; // '3' contains type '1' & '2'.
    if (arg1 != NULL && arg2 != NULL)
    {
        uint8_t sourceid = strtol(arg1, NULL, 10);
        uint8_t eventType = strtol(arg2, NULL, 10);
        taf_time_SourceRef_t sourceRef = NULL;
        sourceRef = taf_time_GetSourceRef(sourceid);

        LE_ASSERT(sourceRef != NULL);

        TimeSourceStatusHandlerRef = taf_time_AddTimeSourceStatusHandler(sourceRef, eventType,
            (taf_time_TimeSourceStatusHandlerFunc_t)TimeSourceStatusHandler, NULL);

        LE_ASSERT(TimeSourceStatusHandlerRef != NULL);
        LE_INFO("Handler registered successfully!.");
        le_sem_Post((le_sem_Ref_t)contextPtr);
        le_event_RunLoop();
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove reference for time source status change handler.
 */
//--------------------------------------------------------------------------------------------------
void RemoveRefTimeSourceStatusTestHandler
(
    void
)
{
    // Remove handler
    taf_time_RemoveTimeSourceStatusHandler(TimeSourceStatusHandlerRef);
    LE_TEST_OK(true, "taf_time_RemoveTimeSourceStatusHandler -OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test time source status change notification.
 */
//--------------------------------------------------------------------------------------------------
void TimeSourceStatusHandlerTest(void)
{
    taf_time_TimeSpec_t timeVal = {1667788990, 1000};
    taf_time_SourceRef_t srcRef = taf_time_GetSourceRef(TAF_TIME_SRC_NAME_EX_APP);
    le_result_t result = LE_OK;

    const char* arg3 = "6"; // Default wait seconds is 6
    if (arg3 != NULL)
    {
        long time = strtol(arg3, NULL, 10);
        le_sem_Ref_t semaphore = le_sem_Create("timeSourceStatusSemaphore", 0);
        le_thread_Ref_t threadRef = le_thread_Create("TimeSourceStatusThread",
            TimeSourceStatusHandlerTestThread, (void*)semaphore);
        le_thread_Start(threadRef);
        le_sem_Wait(semaphore);

        //Handler thread is ready, try to trigger the notification.
        result = taf_time_SetTrustTime(srcRef, &timeVal, false);
        LE_TEST_ASSERT(result == LE_OK, "Set trust time false");
        le_thread_Sleep(time/2);

        result = taf_time_SetTrustTime(srcRef, &timeVal, true);
        LE_TEST_ASSERT(result == LE_OK, "Set trust time true");
        le_thread_Sleep(time/2);

        le_sem_Delete(semaphore);
        RemoveRefTimeSourceStatusTestHandler();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Verify the interface for ptp lib.
 */
//--------------------------------------------------------------------------------------------------
void TestGptpComponent()
{
    long loopNum = 1, waitSec = 0;
    le_result_t res;
    struct timespec gptpTimeValPtr;
    taf_gptpTime_Ref_t gptpTimeRef;

    const char* arg1 = "1"; // Default loopNum is 1
    if (arg1 != NULL)
    {
        loopNum = strtol(arg1, NULL, 10);
        if (loopNum <= 0 ) loopNum = 1;
    }

    const char* arg2 = "1"; // Default wait second is 1
    if (arg2 != NULL)
    {
        waitSec = strtol(arg2, NULL, 10);
        if (waitSec <= 0 ) waitSec = 0;
    }

    LE_INFO("Test loop: %ld, wait seconds: %ld", loopNum, waitSec);

    gptpTimeRef = taf_gptpTime_CreateRef(TAF_GPTP_DEVICE_0);
    LE_ASSERT(gptpTimeRef != NULL);

    while(loopNum--)
    {
        sleep(waitSec);
        res = taf_gptpTime_GetTimeValue(gptpTimeRef, &gptpTimeValPtr);
        LE_ASSERT(res == LE_OK);

        LE_INFO("Reference gptp time is %lld.%ld", (long long)gptpTimeValPtr.tv_sec,
            gptpTimeValPtr.tv_nsec);
    }

    taf_gptpTime_DeleteRef(gptpTimeRef);
    LE_ASSERT(res == LE_OK);
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("==========================================");
    LE_TEST_INFO("===== TimeSvc client API test BEGIN =====");
    LE_TEST_INFO("==========================================");

    LE_TEST_INFO("======== Test time source change handler start ... ========");
    // Handler 1: Validate system time source switching behavior.
    // This test ensures the time source switches to ExAPP only if the current source is NOT ExAPP.
    // Important: Run this as the first test case to guarantee that no previous test has already
    // switched the time source to ExAPP.
    SystemTimeSourceChangeHandlerTest();

    LE_TEST_INFO("======== Test set system time start ... ========");
    TestSetSystemTime();

    LE_TEST_INFO("======== Test TestGetTimeRef start ... ========");
    TestGetTimeRef();

    LE_TEST_INFO("======== Test TestGetTimeRef start ... ========");
    TestGetSourceDetails();

    LE_TEST_INFO("======== Test TestGetSystemTimeSourceID start ... ========");
    TestGetSystemTimeSourceID();

    LE_TEST_INFO("======== Test TestGetTimeZone start ... ========");
    TestGetTimeZone();

    LE_TEST_INFO("======== Test TestGetDayAdj start ... ========");
    TestGetDayAdj();

    LE_TEST_INFO("======== Test TestFailedLoops start ... ========");
    TestFailedLoops();

    LE_TEST_INFO("======== Test TestGetSourceAvailability start ... ========");
    TestGetSourceAvailability();

    LE_TEST_INFO("======== Test TestGetSourceValidity start ... ========");
    TestGetSourceValidity();

    LE_TEST_INFO("======== Test TestSetTrustTimeAPI start ... ========");
    TestSetTrustTimeAPI();

    LE_TEST_INFO("======== Test TestRtcVhalAsyncGetTime start ... ========");
    TestRtcVhalAsyncGetTime();

    LE_TEST_INFO("======== Test TimeValueChangeHandlerTest start ... ========");
    TimeValueChangeHandlerTest();

    LE_TEST_INFO("======== Test TimeSourceStatusHandlerTest start ... ========");
    TimeSourceStatusHandlerTest();

    LE_TEST_INFO("======== Test TestGptpComponent start ... ========");
    TestGptpComponent();

    LE_TEST_INFO("===== TimeSvc client API test DONE =====");

    LE_TEST_EXIT;
}
