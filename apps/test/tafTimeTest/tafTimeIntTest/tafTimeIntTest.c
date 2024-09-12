/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
        "    app runProc tafTimeIntTest tafTimeIntTest -- set rtcTime 1688998899 1000\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- asyncSet rtcTime 1688998899 1000\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- asyncGet rtcTime\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get GetTime 1\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- handler 10\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- timeChaHandler 65 3\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- set time 1688998899 1000\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- set timeLoop 1688998899 1000 33\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- timeSourceStatusHandler 1 0 40\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get GetSourceDetails 1\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get GetSystemTimeSourceID\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get DayAdj 3\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get TimeZone 3\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- set Validity 3 0\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- GptpComponent\n"
        "\n"
        "DESCRIPTION:\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- help\n"
        "       Display this help and exit.\n"
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- set rtcTime\n"
        "       Update rtc time with 'seconds' + 'nanose' as input.\n"
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
        "    app runProc tafTimeIntTest tafTimeIntTest -- set Validity sourceId validity\n"
        "       Sets the validity of given source Id.\n"
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- GptpComponent\n"
        "       Get the ptp time.\n"
        "\n"
    );

    exit(EXIT_SUCCESS);
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

    strftime(tmpBuffer, 80, "%c", timeinfo);
    LE_INFO("UTC time: %s\n", tmpBuffer);
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

    result = taf_time_SetSystemTime((const taf_time_TimeSpec_t*)newTimePtr, true);
    LE_TEST_ASSERT(result == LE_OK, "Test: taf_time_SetSystemTime() APIs - true");

    newTimePtr->sec += 20000000;
    result = taf_time_SetSystemTime((const taf_time_TimeSpec_t*)newTimePtr, false);
    LE_TEST_ASSERT(result == LE_OK, "Test: taf_time_SetSystemTime() APIs - false");

    LE_INFO("Set the time to %"PRIu64".%"PRIu64, newTimePtr->sec, newTimePtr->nanosec);
}

void TestSetTimeToRtc(taf_time_TimeSpec_t* newTime)
{
    le_result_t result;
    result = taf_time_SetTimeToRtc(newTime);
    LE_TEST_ASSERT((result == LE_OK || result == LE_UNSUPPORTED),
        "Test: taf_time_SetTimeToRtc() APIs.");
    if (result == LE_OK)
    {
        LE_INFO("Set time %"PRIu64".%"PRIu64" to RTC\n", newTime->sec, newTime->nanosec);
    }
    else
    {
        LE_INFO("Setting time to read-only RTC is not supported\n");
    }
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
    uint8_t sourceId = strtol(le_arg_GetArg(2), NULL, 10);
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
    uint8_t sourceId = strtol(le_arg_GetArg(2), NULL, 10);

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
    if (strncmp(cmd, "rtcTime", strlen(cmd)) == 0)
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
    uint8_t sourceId = strtol(le_arg_GetArg(2), NULL, 10);
    taf_time_SourceRef_t srcRef;

    srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);
    int32_t failedLoops =0;
    int64_t loopIntervalSec = 0;
    bool isAvailable, validity;

    le_result_t res = taf_time_GetFailedLoops(srcRef, &failedLoops, &loopIntervalSec);
    LE_ASSERT(res == LE_OK);
    LE_INFO("The number of failed loops are %d. Loop interval is  %" PRIu64 "",
    failedLoops, loopIntervalSec);
    isAvailable = taf_time_IsAvailable(srcRef);
    if (isAvailable)
    {
        LE_INFO("Time source is Available!");
    }
    else
    {
        LE_INFO("Time source is NOT Available!");
    }

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

        /* Add new time source here */

        case TAF_TIME_SRC_NAME_UNKNOWN:
            return "UNKNOWN";

        case TAF_TIME_SRC_NAME_SYSTEM:
            return "SYSTEM";

    }
    return "unknown";
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
    uint8_t sourceId = strtol(le_arg_GetArg(2), NULL, 10);
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
    uint8_t sourceId = strtol(le_arg_GetArg(2), NULL, 10);
    taf_time_SourceRef_t srcRef;

    srcRef = taf_time_GetSourceRef(sourceId);
    LE_ASSERT(srcRef != NULL);
    uint8_t dayltSavAdj;
    le_result_t res = taf_time_GetTimeDayAdj(srcRef, &dayltSavAdj);
    LE_ASSERT(res == LE_OK);
    LE_INFO("Day Light Saving is: %d", dayltSavAdj);
}

void TestSetValidity
(
    void
)
{
    uint8_t sourceId = strtol(le_arg_GetArg(2), NULL, 10);
    uint8_t validity = strtol(le_arg_GetArg(3), NULL, 10);
    bool validityFlag = (validity == 0) ? false : true;
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


void TimeGetCmdTest(void)
{
    TimeCheckArgs(2);
    const char* cmd = le_arg_GetArg(1);

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
    le_result_t result;

    TimeCheckArgs(2);
    const char* cmd = le_arg_GetArg(1);

    if (strncmp(cmd, "time", strlen(cmd)) == 0)
    {
        TimeCheckArgs(4);
        NewTimePool = le_mem_CreatePool("NewTimePool", sizeof(taf_time_TimeSpec_t));
        newTimePtr = (taf_time_TimeSpec_t*)le_mem_ForceAlloc(NewTimePool);

        newTimePtr->sec = strtol(le_arg_GetArg(2), NULL, 10);;
        newTimePtr->nanosec = strtol(le_arg_GetArg(3), NULL, 10);;
        LE_INFO("======== Test set system time ========\n");

        result = taf_time_SetSystemTime(newTimePtr, true);
        LE_TEST_ASSERT(result == LE_OK,
            "Test: taf_time_SetSystemTime() APIs.");
        LE_INFO("Set the time to %"PRIu64".%"PRIu64, newTimePtr->sec, newTimePtr->nanosec);
    }
    else if (strncmp(cmd, "rtcTime", strlen(cmd)) == 0)
    {
        TimeCheckArgs(4);
        NewTimePool = le_mem_CreatePool("NewTimePool", sizeof(taf_time_TimeSpec_t));
        newTimePtr = (taf_time_TimeSpec_t*)le_mem_ForceAlloc(NewTimePool);
        newTimePtr->sec = strtol(le_arg_GetArg(2), NULL, 10);;
        newTimePtr->nanosec = strtol(le_arg_GetArg(3), NULL, 10);;
        LE_INFO("======== Test set time to RTC ========\n");

        TestSetTimeToRtc(newTimePtr);
    }
    else if (strncmp(cmd, "timeLoop", strlen(cmd)) == 0)
    {
        le_result_t result;
        TimeCheckArgs(5);
        NewTimePool = le_mem_CreatePool("NewTimePool", sizeof(taf_time_TimeSpec_t));
        newTimePtr = (taf_time_TimeSpec_t*)le_mem_ForceAlloc(NewTimePool);

        newTimePtr->sec = strtol(le_arg_GetArg(2), NULL, 10);;
        newTimePtr->nanosec = strtol(le_arg_GetArg(3), NULL, 10);;
        long time = strtol(le_arg_GetArg(4), NULL, 10);

        // Register handler to receive notification if any
        CreateTimeHandlerTestThread();

        result = taf_time_SetSystemTime(newTimePtr, true);
        while (LE_OK == result)
        {
            LE_INFO("======== Loop test continue ========\n");
            result = taf_time_SetSystemTime(newTimePtr, true);
            le_thread_Sleep(time);
        }
        LE_INFO("======== Loop test exit: %d ========\n", result);
    }
    else if (strncmp(cmd, "Validity", strlen(cmd)) == 0)
    {
        TimeCheckArgs(4);
        TestSetValidity();
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

    long time = strtol(le_arg_GetArg(1), NULL, 10);
    CreateTimeHandlerTestThread();

    //Try to trigger response
    TestSetSystemTime();

    // Wait for handler's response.
    le_thread_Sleep(time);

    RemoveTestHandler();
}

void TimeValueChangeHandlerTest(void)
{
    TimeCheckArgs(3);
    LE_TEST_INFO("======== Handler Test ========\n");

    long time = strtol(le_arg_GetArg(1), NULL, 10);
    CreateTimeValueChangeHandlerTestThread(time);

    RemoveRefTimeTestHandler();
}

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
        newTimePtr->sec = strtol(le_arg_GetArg(2), NULL, 10);;
        newTimePtr->nanosec = strtol(le_arg_GetArg(3), NULL, 10);;
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

void CreateSetRtcVhalTestThread
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

void AsyncSetCmdTest(void)
{
    TimeCheckArgs(2);
    const char* cmd = le_arg_GetArg(1);
    if (strncmp(cmd, "rtcTime", strlen(cmd)) == 0)
    {
        CreateSetRtcVhalTestThread();

    }
    else
    {
        TimePrintHelpMenu();
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
        if (status)
        {
            LE_INFO("Time source is Available!");
        }
        else
        {
            LE_INFO("Time source is NOT Available!");
        }
    }
    if(eventType == TAF_TIME_STATUS_EVENT_VALIDITY)
    {
        if (status)
        {
            LE_INFO("Time source is valid!");
        }
        else
        {
            LE_INFO("Time source is NOT valid!");
        }
    }
}

void* TimeSourceStatusHandlerTestThread(
    void* contextPtr ///< [IN] Thread context.
)
{
    // Connect to service.
    taf_time_ConnectService();
    uint8_t sourceid = strtol(le_arg_GetArg(1), NULL, 10);
    uint8_t eventType = strtol(le_arg_GetArg(2), NULL, 10);
    taf_time_SourceRef_t sourceRef = NULL;
    sourceRef = taf_time_GetSourceRef(sourceid);

    LE_ASSERT(sourceRef != NULL);

    TimeSourceStatusHandlerRef = taf_time_AddTimeSourceStatusHandler(sourceRef, eventType,
        (taf_time_TimeSourceStatusHandlerFunc_t)TimeSourceStatusHandler, NULL);

    LE_ASSERT(TimeSourceStatusHandlerRef != NULL);
    LE_INFO("Handler registered successfully!.");
    le_sem_Post((le_sem_Ref_t)contextPtr);
    le_event_RunLoop();
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

    long time = strtol(le_arg_GetArg(3), NULL, 10);
    le_sem_Ref_t semaphore = le_sem_Create("timeSourceStatusSemaphore", 0);
    le_thread_Ref_t threadRef = le_thread_Create("TimeSourceStatusThread",
        TimeSourceStatusHandlerTestThread, (void*)semaphore);
    le_thread_Start(threadRef);

    le_thread_Sleep(time);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    RemoveRefTimeSourceStatusTestHandler();
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

    LE_INFO("Reference gptp time is %ld.%ld", gptpTimeValPtr.tv_sec,
        gptpTimeValPtr.tv_nsec);

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
    else if (strncmp(cmd, "asyncSet", strlen(cmd)) == 0)
    {
        AsyncSetCmdTest();
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
