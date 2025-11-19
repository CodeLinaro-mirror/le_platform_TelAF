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

//-------------------------------------------------------------------------------------------------
/**
 * Print help menu to stdout and exit.
 */
 //-------------------------------------------------------------------------------------------------
void TimePrintHelpMenu
(
    void
)
{
    puts(
        "NAME:\n"
        "app runProc tafTimeIntTest tafTimeIntTest - Time Service Integration Test.\n"
        "\n"
        "SYNOPSIS:\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- help\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- asyncGet rtcTime\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get GetTime 1\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- handler 10\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- timeChaHandler 65 3\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- set time 1688998899 1000 [1]\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- set timeLoop 1688998899 1000 33\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- set trusttime 1777668866 1000 1\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- timeSourceStatusHandler 1 0 40\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get GetSourceDetails 1\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get GetSystemTimeSourceID\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get DayAdj 3\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get TimeZone 3\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- GptpComponent\n"
        "\n"
        "DESCRIPTION:\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- help\n"
        "       Display this help and exit.\n"
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get systemTime\n"
        "       Get system time.\n"
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get GetTime sourceId\n"
        "       Get source time of 'sourceId' and will return reference which can be used\n"
        "       to get other time information that this 'sourceId' time was created.\n"
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- handler 'WaitSeconds'\n"
        "       Monitor system time status, will receive notification when system time\n"
        "       got changed in 'WaitSeconds' seconds."
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- set time 'seconds' 'nanoseconds'\n"
        "       Set system time with 'seconds' + 'nanose' as input.\n"
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- set trusttime 'ExAPP ID' 'seconds' 'true/false'\n"
        "       Set trust system time with 'sourceId' + 'seconds' + 'validity' as input.\n"
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- set timeLoop 'sec' 'nanos' 'WiatSec'\n"
        "       Set system time with 'seconds' + 'nanosec' + 'interval' as in put, this"
        "       command will set the system time in a loop according to its paramtere."
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- timeSourceStatusHandler sourceId eventType WaitSeconds\n"
        "       Monitor the availability status of 'sourceId' time source for 'WaitSeconds',"
        "       will receive a notification when the status of the source changes."
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get GetSourceDetails sourceId\n"
        "       Get source reference of 'sourceId' and will return reference which can be used\n"
        "       to get other time source information that this 'sourceId' time was created.\n"
        "\n"
         "    app runProc tafTimeIntTest tafTimeIntTest -- get GetSystemTimeSourceID\n"
        "       Get time source that has set the system time most recently\n"
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get DayAdj sourceId\n"
        "       Get daylight adjustment for the given source Id.\n"
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get TimeZone sourceId\n"
        "       Get TimeZone for the given source Id.\n"
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- GptpComponent\n"
        "       Get the ptp time.\n"
        "\n"
    );

    exit(EXIT_SUCCESS);
}

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

//-------------------------------------------------------------------------------------------------
/**
 * Handler for time source change that used by system.
 */
 //-------------------------------------------------------------------------------------------------
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

//-------------------------------------------------------------------------------------------------
/**
 * Handler for time source which is using reference and registered to time service.
 */
 //------------------------------------------------------------------------------------------------
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

    le_sem_Post((le_sem_Ref_t)contextPtr);
    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Set system time through parameter 'CLOCK_REALTIME'.
 */
 //-------------------------------------------------------------------------------------------------
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
    const char* arg2 = le_arg_GetArg(2);
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
 * This thread is created for adding handlers for time source status change notification.
 */
 //------------------------------------------------------------------------------------------------
void* TimeHandlerTestThread
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
    const char* arg2 = le_arg_GetArg(2);
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
void CreateTimeHandlerTestThread
(
    void
)
{
    le_sem_Ref_t semaphore = le_sem_Create("timeSemaphore", 0);
    le_thread_Ref_t threadRef = le_thread_Create("TimeSrcChangeTh",
        TimeHandlerTestThread, (void*)semaphore);
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

//-------------------------------------------------------------------------------------------------
/**
 * This function checks the number of input parameters, if it is less than argNum, then it prints
 * the help menu.
 */
 //------------------------------------------------------------------------------------------------
void TimeCheckArgs
(
    uint8_t argNum ///< [IN] The number of arguments.
)
{
    if (le_arg_NumArgs() < argNum)
    {
        TimePrintHelpMenu();
    }
}

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

void CreateGetRtcVhalTestThread
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

void AsyncGetCmdTest(void)
{
    TimeCheckArgs(2);
    const char* cmd = le_arg_GetArg(1);
    if (cmd != NULL && strncmp(cmd, "rtcTime", strlen(cmd)) == 0)
    {
        CreateGetRtcVhalTestThread();
    }
    else
    {
        TimePrintHelpMenu();
    }
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
    const char* arg2 = le_arg_GetArg(2);
    if (arg2 == NULL)
    {
        LE_ERROR("Invalid arguments.");
        return;
    }
    uint8_t sourceId = strtol(arg2, NULL, 10);
    taf_time_SourceRef_t srcRef;

    srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);
    int32_t failedLoops =0;
    int64_t loopIntervalSec = 0;
    bool isAvailable, validity;

    le_result_t res = taf_time_GetFailedLoops(srcRef, &failedLoops, &loopIntervalSec);
    LE_ASSERT(res == LE_OK);

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
    const char* arg2 = le_arg_GetArg(2);
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
    LE_ASSERT(res == LE_OK);
    LE_INFO("Time zone is:  %d", timeZone);
}

void TestGetDayAdj
(
    void
)
{
    const char* arg2 = le_arg_GetArg(2);
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
    LE_ASSERT(res == LE_OK);
    LE_INFO("Day Light Saving is: %d", dayltSavAdj);
}

//---------------------------------------------------------------
void TestSetTrustTime(const char* arg2, const char* arg3, const char* arg4)
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

    LE_INFO("======== Test set trust time ========");


    taf_time_TimeSources_t sourceId = TAF_TIME_SRC_NAME_EX_APP;
    taf_time_SourceRef_t srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);

    le_result_t result = taf_time_SetTrustTime(srcRef, &newTime, validityFlag);
    LE_TEST_ASSERT(result == LE_OK, "Test: taf_time_SetTrustTime() APIs.");

    LE_INFO("Set trust time %"PRIu64".%"PRIu64 ", validity: %d",
        newTime.sec, newTime.nanosec, validityFlag);

    validityFlag = taf_time_IsSourceValid(srcRef);
    LE_INFO("Validity of %s is set to %s",
    SourceNameIndexToStr(sourceId), validityFlag ? "true" : "false");
}

void TimeGetCmdTest(void)
{
    TimeCheckArgs(2);
    const char* cmd = le_arg_GetArg(1);
    if (cmd == NULL)
    {
        LE_ERROR("cmd is NULL.");
        return;
    }

    if (strncmp(cmd, "GetTime", strlen(cmd)) == 0)
    {
        TimeCheckArgs(3);
        TestGetTimeRef();
    }
    else if (strncmp(cmd, "GetSourceDetails", strlen(cmd)) == 0)
    {
        TimeCheckArgs(3);
        TestGetSourceDetails();
    }
    else if (strncmp(cmd, "GetSystemTimeSourceID", strlen(cmd)) == 0)
    {
        TimeCheckArgs(2);
        TestGetSystemTimeSourceID();
    }
    else if (strncmp(cmd, "TimeZone", strlen(cmd)) == 0)
    {
        TimeCheckArgs(3);
        TestGetTimeZone();
    }
    else if (strncmp(cmd, "DayAdj", strlen(cmd)) == 0)
    {
        TimeCheckArgs(3);
        TestGetDayAdj();
    }
}
void TimeSetCmdTest(void)
{
    static le_mem_PoolRef_t NewTimePool = NULL;
    taf_time_TimeSpec_t* newTimePtr;

    TimeCheckArgs(2);
    const char* cmd = le_arg_GetArg(1);
    if (cmd == NULL)
    {
        LE_ERROR("cmd is NULL.");
        return;
    }

    if (strncmp(cmd, "time", strlen(cmd)) == 0)
    {
        TimeCheckArgs(4);
        const char* arg2 = le_arg_GetArg(2);
        const char* arg3 = le_arg_GetArg(3);
        const char* arg4 = le_arg_GetArg(4);
        TestSetTrustTime(arg2, arg3, arg4);
    }
    else if (strncmp(cmd, "timeLoop", strlen(cmd)) == 0)
    {
        le_result_t result;
        TimeCheckArgs(5);
        NewTimePool = le_mem_CreatePool("NewTimePool", sizeof(taf_time_TimeSpec_t));
        newTimePtr = (taf_time_TimeSpec_t*)le_mem_ForceAlloc(NewTimePool);
        const char* arg2 = le_arg_GetArg(2);
        const char* arg3 = le_arg_GetArg(3);
        const char* arg4 = le_arg_GetArg(4);
        long time = 0;
        if (arg2 != NULL && arg3 != NULL && arg4 != NULL)
        {
            newTimePtr->sec = strtol(arg2, NULL, 10);
            newTimePtr->nanosec = strtol(arg3, NULL, 10);
            time = strtol(arg4, NULL, 10);
        }
        // Register handler to receive notification if any
        CreateTimeHandlerTestThread();

        taf_time_TimeSources_t sourceId = TAF_TIME_SRC_NAME_EX_APP;
        taf_time_SourceRef_t srcRef = taf_time_GetSourceRef(sourceId);
        LE_ASSERT(srcRef != NULL);

        result = taf_time_SetTrustTime(srcRef, newTimePtr, false);
        while (LE_OK == result)
        {
            LE_INFO("======== Loop test continue ========\n");
            result = taf_time_SetTrustTime(srcRef, newTimePtr, true);
            le_thread_Sleep(time);
        }
        LE_INFO("======== Loop test exit: %d ========\n", result);
    }
    else if (strncmp(cmd, "trusttime", strlen(cmd)) == 0)
    {
        TimeCheckArgs(5);
        const char* arg2 = le_arg_GetArg(2);
        const char* arg3 = le_arg_GetArg(3);
        const char* arg4 = le_arg_GetArg(4);
        TestSetTrustTime(arg2, arg3, arg4);
    }
    else
    {
        TimePrintHelpMenu();
    }
}
void TimeHandlerTest(void)
{
    TimeCheckArgs(2);
    LE_TEST_INFO("======== Handler Test ========\n");

    const char* arg1 = le_arg_GetArg(1);
    if (arg1 != NULL)
    {
        long time = strtol(arg1, NULL, 10);
        CreateTimeHandlerTestThread();

        //Try to trigger response
        TestSetSystemTime();

        // Wait for handler's response.
        le_thread_Sleep(time);

        RemoveTestHandler();
    }
}

void TimeValueChangeHandlerTest(void)
{
    TimeCheckArgs(3);
    LE_TEST_INFO("======== Handler Test ========\n");

    const char* arg1 = le_arg_GetArg(1);
    if (arg1 != NULL)
    {
        long time = strtol(arg1, NULL, 10);
        CreateTimeValueChangeHandlerTestThread(time);

        RemoveRefTimeTestHandler();
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Handler for time source change that used by system.
 */
 //-------------------------------------------------------------------------------------------------
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

void* TimeSourceStatusHandlerTestThread(
    void* contextPtr ///< [IN] Thread context.
)
{
    // Connect to service.
    taf_time_ConnectService();
    const char* arg1 = le_arg_GetArg(1);
    const char* arg2 = le_arg_GetArg(2);
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

//-------------------------------------------------------------------------------------------------
/**
 * Remove handlers for reference time source.
 */
 //------------------------------------------------------------------------------------------------
void RemoveRefTimeSourceStatusTestHandler
(
    void
)
{
    // Remove handler
    taf_time_RemoveTimeSourceStatusHandler(TimeSourceStatusHandlerRef);
    LE_TEST_OK(true, "taf_time_RemoveTimeSourceStatusHandler -OK");
}

//-------------------------------------------------------------------------------------------------
/**
 * Create thread for test handler.
 */
 //------------------------------------------------------------------------------------------------

void TimeSourceStatusHandlerTest(void)
{
    TimeCheckArgs(4);
    LE_TEST_INFO("======== Time Source Status Change Handler Test ========\n");

    const char* arg3 = le_arg_GetArg(3);
    if (arg3 != NULL)
    {
        long time = strtol(arg3, NULL, 10);
        le_sem_Ref_t semaphore = le_sem_Create("timeSourceStatusSemaphore", 0);
        le_thread_Ref_t threadRef = le_thread_Create("TimeSourceStatusThread",
            TimeSourceStatusHandlerTestThread, (void*)semaphore);
        le_thread_Start(threadRef);

        le_thread_Sleep(time);
        le_sem_Wait(semaphore);
        le_sem_Delete(semaphore);

        RemoveRefTimeSourceStatusTestHandler();
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Verify the interface for ptp lib.
 */
 //------------------------------------------------------------------------------------------------
void TestGptpComponent()
{
    long loopNum = 1, waitSec = 0;
    le_result_t res;
    struct timespec gptpTimeValPtr;
    taf_gptpTime_Ref_t gptpTimeRef;

    const char* arg1 = le_arg_GetArg(1);
    if (arg1 != NULL)
    {
        loopNum = strtol(arg1, NULL, 10);
        if (loopNum <= 0 ) loopNum = 1;
    }

    const char* arg2 = le_arg_GetArg(2);
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
 //-------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("*** Checking for Console args ***");
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    TimeCheckArgs(1);
    const char* cmd = le_arg_GetArg(0);
    if (cmd == NULL)
    {
        LE_ERROR("cmd is NULL");
        LE_TEST_EXIT;
    }

    LE_TEST_INFO("======== Time Service int Test ========");

    if (strncmp(cmd, "get", strlen(cmd)) == 0)
    {
        TimeGetCmdTest();
    }
    else if (strncmp(cmd, "set", strlen(cmd)) == 0)
    {
        TimeSetCmdTest();
    }
    else if (strncmp(cmd, "handler", strlen(cmd)) == 0)
    {
        TimeHandlerTest();
    }
    else if (strncmp(cmd, "timeChaHandler", strlen(cmd)) == 0)
    {
        TimeValueChangeHandlerTest();
    }
    else if (strncmp(cmd, "asyncGet", strlen(cmd)) == 0)
    {
        AsyncGetCmdTest();
    }
    else if (strncmp(cmd, "timeSourceStatusHandler", strlen(cmd)) == 0)
    {
        TimeSourceStatusHandlerTest();
    }
    else if (strncmp(cmd, "GptpComponent", strlen(cmd)) == 0)
    {
        TestGptpComponent();
    }
    else
    {
        TimePrintHelpMenu();
    }
   LE_TEST_EXIT;
}
