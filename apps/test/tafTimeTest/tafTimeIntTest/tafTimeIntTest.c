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

static taf_time_TimeSourceChangeHandlerRef_t timeSourceChangeHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Print help menu to stdout and exit.
 */
//--------------------------------------------------------------------------------------------------
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
        "    app runProc tafTimeIntTest tafTimeIntTest -- get gnssTime\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get systemTime\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- handler 10\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- set time 1688998899 1000\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- set timeLoop 1688998899 1000 33\n"
        "\n"
        "DESCRIPTION:\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- help\n"
        "       Display this help and exit.\n"
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get gnssTime\n"
        "       Get time service maintained gnss time \n"
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- get systemTime\n"
        "       Get system time \n"
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- set systemTime\n"
        "       set system time with seconds + nanosec as input\n"
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- set timeLoop\n"
        "       set system time with seconds + nanosec in a loop at sertain"
        "       time interval without time out."
        "\n"
        "    app runProc tafTimeIntTest tafTimeIntTest -- handler time\n"
        "       Handler test with sleep time 'seconds' as input\n"
        "\n"
    );

    exit(EXIT_SUCCESS);
}

void TimeSourceChangeHandler
(
    taf_time_TimeSources_t PreTimeSource,
    taf_time_TimeSources_t NewTimeSource
)
{
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

void IntTestSetSystemTime
(
    taf_time_TimeSpec_t *newTime
)
{
    le_result_t result;

    result = taf_time_SetSystemTime(newTime, true);
    LE_TEST_ASSERT(result == LE_OK,
                   "Test: taf_time_SetSystemTime() APIs.");
    LE_INFO("Set the time to %"PRIu64".%"PRIu64, newTime->sec, newTime->nanosec);
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

    LE_INFO("Set the time to %"PRIu64".%"PRIu64, newTime->sec, newTime->nanosec);
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
    LE_INFO("System time is %"PRIu64".%"PRIu64, systemTime.sec, systemTime.nanosec);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get GNSS time that is maintained in time service
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
        LE_INFO("GNSS time is %"PRIu64".%"PRIu64, gnssTime.sec, gnssTime.nanosec);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This thread is created for adding handlers for network registation state, packet swicthed state,
 * network registation rejection, and Radio Access Technology change.
 */
//--------------------------------------------------------------------------------------------------
void* HandlerTestThread
(
    void* contextPtr ///< [IN] Thread context.
)
{
    // Connect to service.
    taf_time_ConnectService();

    // Add handler.
    timeSourceChangeHandlerRef = taf_time_AddTimeSourceChangeHandler(
        (taf_time_TimeSourceChangeHandlerFunc_t)TimeSourceChangeHandler, NULL);
    LE_TEST_OK(timeSourceChangeHandlerRef != NULL, "taf_time_AddTimeSourceChangeHandler - OK");

    le_sem_Post((le_sem_Ref_t)contextPtr);
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handlers for network registation state, packet swicthed state, network registation
 * rejection, Radio Access Technology change, and IMS registration status.
 */
//--------------------------------------------------------------------------------------------------
void RemoveTestHandler
(
    void
)
{
    // Remove handler
    taf_time_RemoveTimeSourceChangeHandler(timeSourceChangeHandlerRef);
    LE_TEST_OK(true, "taf_time_RemoveTimeSourceChangeHandler - OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Create thread for test handler.
 */
//--------------------------------------------------------------------------------------------------
void CreateHandlerTestThread
(
    void
)
{
    le_sem_Ref_t semaphore = le_sem_Create("timeSemaphore", 0);
    le_thread_Ref_t threadRef = le_thread_Create("TimeSrcChangeTh", HandlerTestThread, (void*)semaphore);
    le_thread_Start(threadRef);

    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function checks the number of input parameters, if it is less than argNum, then it prints
 * the help menu.
 */
//--------------------------------------------------------------------------------------------------
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
void TimeGetCmdTest(void)
{
    TimeCheckArgs(2);
    const char* cmd = le_arg_GetArg(1);
    if (strncmp(cmd, "systemTime", strlen("systemTime")) == 0)
    {
        TestGetSystemTime();
    }
    else if (strncmp(cmd, "gnssTime", strlen("gnssTime")) == 0)
    {
        TestGetGnssTime();
    }
}
void TimeSetCmdTest(void)
{
    static le_mem_PoolRef_t NewTimePool = NULL;
    taf_time_TimeSpec_t *newTime;

    TimeCheckArgs(2);
    const char*  cmd = le_arg_GetArg(1);

    if (strncmp(cmd, "time", strlen(cmd)) == 0)
    {
        TimeCheckArgs(4);
        NewTimePool = le_mem_CreatePool("NewTimePool", sizeof(taf_time_TimeSpec_t));
        newTime = (taf_time_TimeSpec_t*) le_mem_ForceAlloc(NewTimePool);

        newTime->sec = strtol(le_arg_GetArg(2), NULL, 10);;
        newTime->nanosec = strtol(le_arg_GetArg(3), NULL, 10);;
        LE_INFO("======== Test set system time ========\n");

        IntTestSetSystemTime(newTime);
    }
    else if (strncmp(cmd, "timeLoop", strlen(cmd)) == 0)
    {
        le_result_t result;
        TimeCheckArgs(5);
        NewTimePool = le_mem_CreatePool("NewTimePool", sizeof(taf_time_TimeSpec_t));
        newTime = (taf_time_TimeSpec_t*) le_mem_ForceAlloc(NewTimePool);

        newTime->sec = strtol(le_arg_GetArg(2), NULL, 10);;
        newTime->nanosec = strtol(le_arg_GetArg(3), NULL, 10);;
        long time = strtol(le_arg_GetArg(4), NULL, 10);

        result = taf_time_SetSystemTime(newTime, true);
        while (LE_OK == result)
        {
            LE_INFO("======== Loop test continue ========\n");
            result = taf_time_SetSystemTime(newTime, true);
            le_thread_Sleep(time);
        }
        LE_INFO("======== Loop test exit: %d ========\n", result);
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
    CreateHandlerTestThread();

    //Try to trigger response
    TestSetSystemTime();

    // Wait for handler's response.
    le_thread_Sleep(time);

    RemoveTestHandler();

}
//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    TimeCheckArgs(1);
    const char* cmd = le_arg_GetArg(0);

    LE_TEST_INFO("======== Time Service int Test ========");

    if (strncmp(cmd, "get", strlen("get")) == 0)
    {
        TimeGetCmdTest();
    }
    else if (strncmp(cmd, "set", strlen("set")) == 0)
    {
        TimeSetCmdTest();
    }
    else if (strncmp(cmd, "handler", strlen("handler")) == 0)
    {
        TimeHandlerTest();
    }
    else
    {
        TimePrintHelpMenu();
    }

    exit(EXIT_SUCCESS);
}

