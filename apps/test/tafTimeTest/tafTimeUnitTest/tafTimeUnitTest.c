/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

//--------------------------------------------------------------------------------------------------
/**
 * Handler for time source change.
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
        LE_INFO("Notification: No available time source. Pre %d, New %d\n",
                                            PreTimeSource, NewTimeSource);
    }
    else
    {
        LE_INFO("Notification: Time source changed. Pre %d, New %d\n",
                                            PreTimeSource, NewTimeSource);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Register handler for time source change.
 */
//--------------------------------------------------------------------------------------------------
void TestTimeSourceChangeRegistration
(
    void
)
{
    taf_time_TimeSourceChangeHandlerRef_t timeSourceChangeHandlerRef =
        taf_time_AddTimeSourceChangeHandler(
        (taf_time_TimeSourceChangeHandlerFunc_t)TimeSourceChangeHandler, NULL);
    LE_TEST_OK(timeSourceChangeHandlerRef != NULL,
        "taf_time_AddTimeSourceChangeHandler - !NULL");

    taf_time_RemoveTimeSourceChangeHandler(timeSourceChangeHandlerRef);
    LE_TEST_OK(true, "taf_time_RemoveTimeSourceChangeHandler - void");
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
    taf_time_TimeSpec_t *newTime;
    NewTimePool = le_mem_CreatePool("NewTimePool", sizeof(taf_time_TimeSpec_t));
    newTime = (taf_time_TimeSpec_t*) le_mem_ForceAlloc(NewTimePool);

    newTime->sec = 1667788990;
    newTime->nanosec = 10000;

    result = taf_time_SetSystemTime((const taf_time_TimeSpec_t*)newTime, true);
    LE_TEST_ASSERT(result == LE_OK,
                   "Test: taf_time_SetSystemTime() APIs - true");

    result = taf_time_SetSystemTime((const taf_time_TimeSpec_t*)newTime, false);
    LE_TEST_ASSERT(result == LE_OK,
                   "Test: taf_time_SetSystemTime() APIs - false");

    LE_INFO("Set the time to %lld.%ld", (long long)newTime->sec, newTime->nanosec);
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
    LE_TEST_ASSERT(result == LE_OK,
                   "Test: taf_time_GetSystemTime() APIs.");
    LE_INFO("System time is %lld.%ld\n", (long long)systemTime.sec, systemTime.nanosec);
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
    LE_TEST_ASSERT((result == LE_OK||result == LE_NOT_FOUND),
                        "Test: taf_time_GetGnssTime() APIs.");
    if (result == LE_OK)
    {
        LE_INFO("GNSS time is %lld.%ld", (long long)gnssTime.sec, gnssTime.nanosec);
    }
}

/**
 * Application initialization.
 */
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("==========================================");
    LE_TEST_INFO("===== TimeSvc client API test BEGIN =====");
    LE_TEST_INFO("==========================================");

    TestSetSystemTime();
    TestGetSystemTime();
    TestGetGnssTime();
    TestTimeSourceChangeRegistration();

    LE_TEST_INFO("===== TimeSvc client API test DONE =====");
}
