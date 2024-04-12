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

static taf_time_TimeSourceChangeHandlerRef_t TimeSourceChangeHandlerRef = NULL;
static taf_time_TimeValueChangeHandlerRef_t TimeValueChangeHandlerRef = NULL;

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
    taf_time_TimeSourceRef_t timeSrcRef,
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
 * Get system time through parameter 'CLOCK_REALTIME'.
 */
//--------------------------------------------------------------------------------------------------
void TestGetSystemTime
(
    void
)
{
    le_result_t result;
    taf_time_TimeSpec_t systemTime;

    result = taf_time_GetSystemTime(&systemTime);
    LE_TEST_ASSERT(result == LE_OK, "Test: taf_time_GetSystemTime() APIs.");

    LE_INFO("System time is %"PRIu64".%"PRIu64, systemTime.sec, systemTime.nanosec);
    ConvertSecToDateTime(systemTime);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get GNSS time that is maintained in time service.
 */
//--------------------------------------------------------------------------------------------------
void TestGetGnssTime
(
    void
)
{
    le_result_t result;
    taf_time_TimeSpec_t gnssTime;

    //GNSS not ready will return not found. Here just verify the API
    result = taf_time_GetGnssTime(&gnssTime);
    LE_TEST_ASSERT((result == LE_OK||result == LE_UNAVAILABLE),
                        "Test: taf_time_GetGnssTime() APIs.");
    if (result == LE_OK)
    {
        LE_INFO("GNSS time is %"PRIu64".%"PRIu64, gnssTime.sec, gnssTime.nanosec);
        ConvertSecToDateTime(gnssTime);
    }
    else
    {
        LE_INFO("GNSS time is not available now\n");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get RTC time from device or VHAL interface.
 */
//--------------------------------------------------------------------------------------------------
void TestGetRtcTime
(
    void
)
{
    le_result_t result;
    taf_time_TimeSpec_t rtcTime;

    result = taf_time_GetRtcTime(&rtcTime);
    LE_TEST_ASSERT(result == LE_OK,
        "Test: taf_time_GetRtcTime() APIs.");
    LE_INFO("RTC time is %"PRIu64".%"PRIu64, rtcTime.sec, rtcTime.nanosec);
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
    taf_time_TimeSourceRef_t timeSrcRef;

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
    taf_time_TimeSourceRef_t timeSrcRef;

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

    taf_time_RemoveTimeSourceChangeHandler(TimeSourceChangeHandlerRef);
    LE_TEST_OK(true, "taf_time_RemoveTimeSourceChangeHandler - void");

    taf_time_RemoveTimeValueChangeHandler(TimeValueChangeHandlerRef);
    LE_TEST_OK(true, "taf_time_RemoveTimeValueChangeHandler - OK");
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

    TestGetSystemTime();

    TestGetGnssTime();
    TestGetRtcTime();

    TestGetTimeRef();

    TestRtcVhalAsyncSetTime();
    TestRtcVhalAsyncGetTime();

    TestTimeRegistrationHandler();

    LE_TEST_INFO("===== TimeSvc client API test DONE =====");

    LE_TEST_EXIT;
}
