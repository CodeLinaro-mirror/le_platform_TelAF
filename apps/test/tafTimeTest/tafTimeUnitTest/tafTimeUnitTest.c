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
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for source status change which is using source reference and registered to time service.
 */
 //--------------------------------------------------------------------------------------------------
void TimeSourceStatusHandler
(
    bool isAvailable,
    void* contextPtr
)
{
    if (isAvailable)
    {
        LE_INFO("Time source is Available!");
    }
    else
    {
        LE_INFO("Time source is NOT Available!");
    }
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
    taf_time_TimeSpec_t *newTimePtr;
    NewTimePool = le_mem_CreatePool("NewTimePool", sizeof(taf_time_TimeSpec_t));
    newTimePtr = (taf_time_TimeSpec_t*) le_mem_ForceAlloc(NewTimePool);

    newTimePtr->sec = 1667788990;
    newTimePtr->nanosec = 10000;

    result = taf_time_SetSystemTime((const taf_time_TimeSpec_t*)newTimePtr, true);
    LE_TEST_ASSERT(result == LE_OK, "Test: taf_time_SetSystemTime() APIs - true");

    newTimePtr->sec += 20000000;
    result = taf_time_SetSystemTime((const taf_time_TimeSpec_t*)newTimePtr, false);
    LE_TEST_ASSERT(result == LE_OK, "Test: taf_time_SetSystemTime() APIs - false");

    LE_INFO("Set the time to %"PRIu64".%"PRIu64, newTimePtr->sec, newTimePtr->nanosec);
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
    uint8_t sourceId = 1;
    taf_time_TimeRef_t timeSrcRef;

    timeSrcRef = taf_time_GetTimeRef(sourceId);
    LE_TEST_ASSERT(timeSrcRef != NULL, "taf_time_GetTimeRef() API.");

    // Get time from a specify time source and related Reference.
    result = taf_time_GetTime(timeSrcRef, &time);
    LE_TEST_ASSERT(result == LE_OK || result == LE_UNAVAILABLE,
                               "taf_time_GetTime() API.");
    if (result == LE_OK)
    {
        LE_INFO("Reference %d time is %"PRIu64".%"PRIu64, sourceId, time.sec, time.nanosec);
        ConvertSecToDateTime(time);
    }
    LE_INFO("timeSrcRef %p, sourceId (0x%x), status %d.", timeSrcRef, sourceId, result);

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

//--------------------------------------------------------------------------------------------------
/**
 * This thread is created for adding handlers for testing reference time source notification.
 */
//--------------------------------------------------------------------------------------------------
void TimeValueChangeHandlerTest
(
    void
)
{
    le_result_t result;
    uint8_t sourceId = 3;

    taf_time_TimeSpec_t timeVal;
    taf_time_TimeRef_t timeSrcRef;

    timeSrcRef = taf_time_GetTimeRef(sourceId);
    LE_TEST_ASSERT(timeSrcRef != NULL, "taf_time_GetTimeRef() API.");

    // Get time from a specify time source and related Reference.
    result = taf_time_GetTime(timeSrcRef, &timeVal);
    LE_TEST_ASSERT(result == LE_OK || result == LE_UNAVAILABLE,
                                         "taf_time_GetTime() API.");
    if (result == LE_OK)
    {
        LE_INFO("Reference %d time is %"PRIu64".%"PRIu64,
                              sourceId, timeVal.sec, timeVal.nanosec);
    }
    LE_INFO("timeSrcRef %p, sourceId (0x%x), status %d.", timeSrcRef, sourceId, result);

    // Register Reference time source got change Handler.
    TimeValueChangeHandlerRef = taf_time_AddTimeValueChangeHandler(sourceId,
              (taf_time_TimeValueChangeHandlerFunc_t)TimeValueChangeHandler, NULL);
    LE_TEST_OK(TimeValueChangeHandlerRef != NULL, "taf_time_AddTimeValueChangeHandler() - OK");

}

//--------------------------------------------------------------------------------------------------
/**
 * This thread is created for adding handlers for testing source status change notification.
 */
 //--------------------------------------------------------------------------------------------------
void TimeSourceStatusHandlerTest(
)
{
    uint8_t sourceid = TAF_TIME_SRC_NAME_RTC;
    taf_time_SourceRef_t sourceRef = NULL;
    uint8_t eventType = 2;

    //Get the reference for specific source
    sourceRef = taf_time_GetSourceRef(sourceid);

    LE_TEST_ASSERT(sourceRef != NULL, "taf_time_GetSourceRef sourceRef - OK");

    //Register handler for source status change
     TimeSourceStatusHandlerRef = taf_time_AddTimeSourceStatusHandler(sourceRef, eventType,
        (taf_time_TimeSourceStatusHandlerFunc_t)TimeSourceStatusHandler, NULL);
    LE_TEST_ASSERT(TimeSourceStatusHandlerRef != NULL, "taf_time_AddTimeSourceStatusHandler() - OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Register handler for time related information change.
 */
//--------------------------------------------------------------------------------------------------
void TestTimeRegistrationHandler
(
    void
)
{
    // Test time source change handler
    TimeSourceChangeHandlerRef = taf_time_AddTimeSourceChangeHandler(
        (taf_time_TimeSourceChangeHandlerFunc_t)TimeSourceChangeHandler, NULL);
    LE_TEST_OK(TimeSourceChangeHandlerRef != NULL, "taf_time_AddTimeSourceChangeHandler - OK");

    TimeValueChangeHandlerTest();
    TimeSourceStatusHandlerTest();

    taf_time_RemoveTimeSourceChangeHandler(TimeSourceChangeHandlerRef);
    LE_TEST_OK(true, "taf_time_RemoveTimeSourceChangeHandler - void");

    taf_time_RemoveTimeValueChangeHandler(TimeValueChangeHandlerRef);
    LE_TEST_OK(true, "taf_time_RemoveTimeValueChangeHandler - OK");

    taf_time_RemoveTimeSourceStatusHandler(TimeSourceStatusHandlerRef);
    LE_TEST_OK(true, "taf_time_RemoveTimeSourceStatusHandler - OK");
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

        /* Add new time source here */

        case TAF_TIME_SRC_NAME_UNKNOWN:
            return "UNKNOWN";

        case TAF_TIME_SRC_NAME_SYSTEM:
            return "SYSTEM";

    }
    return "unknown";
}

//--------------------------------------------------------------------------------------------------
/**
 * Test RTC Async set time API.
 */
//--------------------------------------------------------------------------------------------------
void setRTCTimeAsync(le_result_t responseState, void* contextPtr)
{
    LE_TEST_ASSERT(responseState == LE_OK, "Test: setRTCTimeAsync response mode is LE_OK");
    le_sem_Post((le_sem_Ref_t)contextPtr);
}

void* TestSetRTCAsync(void* cxtPtr)
{
        taf_time_ConnectService();
        static le_mem_PoolRef_t NewTimePool = NULL;
        taf_time_TimeSpec_t* newTimePtr;
        NewTimePool = le_mem_CreatePool("TimePool", sizeof(taf_time_TimeSpec_t));
        newTimePtr = (taf_time_TimeSpec_t*)le_mem_ForceAlloc(NewTimePool);
        newTimePtr->sec = 1712345678;
        newTimePtr->nanosec = 12345678;
        le_result_t res = taf_time_SetRtcTimeReqAsync(newTimePtr, setRTCTimeAsync, (void*)cxtPtr);
        LE_TEST_ASSERT(res == LE_OK || res == LE_UNSUPPORTED,
            "Test: taf_time_SetRtcTimeReqAsync() APIs - ok");

        if (res == LE_UNSUPPORTED)
        {
            // If RTC VHAL was not installed, for RTC async API it will report LE_UNSUPPORTED
            // and without callback, so need to release the semphone here.
            LE_INFO("RTC Async set time API (work with VHAL) received: Unsupported");
            le_sem_Post((le_sem_Ref_t)cxtPtr);
        }

        le_event_RunLoop();
        return NULL;
}

void TestRtcVhalAsyncSetTime
(
    void
)
{
    le_sem_Ref_t semASetRtcVhal = le_sem_Create("AsynSetRtcVhal", 0);
    le_thread_Ref_t threadRef = le_thread_Create("TestSetRTCAsyncThread",
        TestSetRTCAsync, (void*)semASetRtcVhal);
    le_thread_Start(threadRef);

    le_sem_Wait(semASetRtcVhal);
    le_sem_Delete(semASetRtcVhal);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get source reference through source ID and get source related attributes from the reference.
 */
 //--------------------------------------------------------------------------------------------------
void TestGetSourceRef
(
    void
)
{
    uint8_t sourceId = 0xFF;
    taf_time_SourceRef_t srcRef;

    srcRef = taf_time_GetSourceRef(sourceId);
    LE_TEST_ASSERT(srcRef != NULL, "taf_time_GetSourceRef() API - OK.");
}

void TestGetSystemTimeSourceID()
{
    taf_time_TimeSources_t systemTimeSrc;
    le_result_t res = taf_time_GetSystemTimeSourceID(&systemTimeSrc);
    LE_ASSERT(res == LE_OK);
    LE_INFO("The lastest system time is set by %s", SourceNameIndexToStr(systemTimeSrc));
}

void TestGetTimeZone()
{
    uint8_t sourceId = 3;
    taf_time_SourceRef_t srcRef;

    srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);
    int8_t timeZone = 0;
    le_result_t res = taf_time_GetTimeZone(srcRef, &timeZone);
    LE_TEST_ASSERT(res == LE_OK, "taf_time_GetTimeZone - OK. TimeZone is %d", timeZone);
}

void TestGetDayAdj()
{
    uint8_t sourceId = 3;
    taf_time_SourceRef_t srcRef;

    srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);
    uint8_t dayltSavAdj;
    le_result_t res = taf_time_GetTimeDayAdj(srcRef, &dayltSavAdj);
    LE_TEST_ASSERT(res == LE_OK, "taf_time_GetTimeDayAdj - OK. Day Light Saving is %d", dayltSavAdj);
}

void TestFailedLoops()
{
    uint8_t sourceId = 0;
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
    uint8_t sourceId = 3;
    taf_time_SourceRef_t srcRef;
    bool isAvailable;
    srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);
    isAvailable = taf_time_IsAvailable(srcRef);
    if (isAvailable)
    {
        LE_INFO("Time source is Available!");
    }
    else
    {
        LE_INFO("Time source is NOT Available!");
    }
}

void TestGetSourceValidity()
{
    uint8_t sourceId = TAF_TIME_SRC_NAME_NETWORK;
    taf_time_SourceRef_t srcRef;
    bool validity;
    srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);
    validity = taf_time_IsSourceValid(srcRef);
    if (validity)
    {
        LE_INFO("Time source is valid!");
    }
    else
    {
        LE_INFO("Time source is NOT valid!");
    }
}

void TestSetSourceValidity()
{
    uint8_t sourceId = TAF_TIME_SRC_NAME_RTC;
    bool validityFlag = false;
    taf_time_SourceRef_t srcRef;

    srcRef = taf_time_GetSourceRef(sourceId);

    LE_ASSERT(srcRef != NULL);
    le_result_t res = taf_time_SetValidity(srcRef, validityFlag);
    LE_ASSERT(res == LE_OK);
    LE_INFO("taf_time_SetValidity - LE_OK");
    validityFlag = taf_time_IsSourceValid(srcRef);
    if (validityFlag)
    {
        LE_INFO("Validity of time source is set to true");
    }
    else
    {
        LE_INFO("Validity of time source is set to false");
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Verify the interface for ptp lib.
 */
 //------------------------------------------------------------------------------------------------
void TestGptpComponent()
{
    le_result_t res;
    struct timespec gptpTimeValPtr;
    taf_gptpTime_Ref_t gptpTimeRef;

    gptpTimeRef = taf_gptpTime_CreateRef(TAF_GPTP_DEVICE_0);
    LE_ASSERT(gptpTimeRef != NULL);

    res = taf_gptpTime_GetTimeValue(gptpTimeRef, &gptpTimeValPtr);
    LE_ASSERT(res == LE_OK);

    LE_INFO("Reference gptp time is %lld.%ld", (long long)gptpTimeValPtr.tv_sec,
        gptpTimeValPtr.tv_nsec);

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

    TestSetSystemTime();
    TestGetTimeRef();
    TestGetSourceRef();
    TestGetSystemTimeSourceID();
    TestGetTimeZone();
    TestGetDayAdj();
    TestFailedLoops();
    TestGetSourceAvailability();
    TestGetSourceValidity();
    TestSetSourceValidity();

    TestRtcVhalAsyncSetTime();
    TestRtcVhalAsyncGetTime();

    TestTimeRegistrationHandler();

    TestGptpComponent();

    LE_TEST_INFO("===== TimeSvc client API test DONE =====");

    LE_TEST_EXIT;
}
