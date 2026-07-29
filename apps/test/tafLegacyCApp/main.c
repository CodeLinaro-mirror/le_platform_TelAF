/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * tafLegacyCApp - a "legacy" C application that calls TelAF APIs WITHOUT being
 * a Legato app (no .adef, not built by mkapp).
 *
 * It follows the pattern documented in "How to Use TelAF API in Legacy C App"
 * and the apps/sample/legacyDcsApp sample: the service's client-side IPC stubs
 * are generated from the .api with ifgen / CMake generate_client(), the program
 * is linked against liblegato, and it calls the typed service APIs directly:
 *
 *   1. taf_time_ConnectService()  - once per thread that uses TelAF APIs.
 *   2. taf_time_*()               - call the service APIs directly.
 *
 * This example connects to tafTimeSvc (taf_time interface) and reads the
 * current system time.
 *
 * Build: see CMakeLists.txt (generate_client(taf_time.api) + -llegato).
 * Run on target:
 *   sdir bind "<root>.taf_time" "<telaf>.taf_time"
 *   ./tafLegacyCApp
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "legato.h"
#include "taf_time_interface.h"

//--------------------------------------------------------------------------------------------------
/**
 * Read and print the current system time from tafTimeSvc.
 */
//--------------------------------------------------------------------------------------------------
static void ReadSystemTime
(
    void
)
{
    // The SYSTEM time source always exists; get its reference object first.
    taf_time_TimeRef_t timeRef = taf_time_GetTimeRef(TAF_TIME_SRC_NAME_SYSTEM);
    if (timeRef == NULL)
    {
        LE_ERROR("taf_time_GetTimeRef(SYSTEM) returned NULL");
        return;
    }

    taf_time_TimeSpec_t timeVal = {0};
    le_result_t result = taf_time_GetRefSystemTime(timeRef, &timeVal);
    if (result == LE_OK)
    {
        LE_INFO("TelAF system time: %" PRIu64 " s, %" PRIu64 " ns",
                timeVal.sec, timeVal.nanosec);
        printf("LEGACY_APP: TelAF system time = %llu.%09llu\n",
               (unsigned long long)timeVal.sec, (unsigned long long)timeVal.nanosec);
    }
    else
    {
        LE_ERROR("taf_time_GetRefSystemTime() failed: %s", LE_RESULT_TXT(result));
        printf("LEGACY_APP: taf_time_GetRefSystemTime() failed: %s\n",
               LE_RESULT_TXT(result));
    }

    // Also report which source the system time is currently synced to.
    taf_time_TimeSources_t srcId = TAF_TIME_SRC_NAME_UNKNOWN;
    if (taf_time_GetSystemTimeSourceID(&srcId) == LE_OK)
    {
        LE_INFO("System time source ID: %d", srcId);
        printf("LEGACY_APP: system time source ID = %d\n", srcId);
    }

    (void)taf_time_ReleaseTimeRef(timeRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * TelAF worker thread: sets up the Legato thread context, connects to the
 * service, calls the APIs, then enters the Legato event loop (so async handlers
 * and IPC keep working).
 */
//--------------------------------------------------------------------------------------------------
static void* TelafTask
(
    void* arg
)
{
    LE_UNUSED(arg);

    // Required for any non-Legato thread that wants to use Legato/TelAF IPC.
    le_thread_InitLegatoThreadData("tafLegacyCApp_thread");

    // Connect to tafTimeSvc.  Call once per thread that uses taf_time APIs.
    taf_time_ConnectService();
    LE_INFO("Connected to taf_time service");
    printf("LEGACY_APP: connected to taf_time service\n");

    ReadSystemTime();

    // Enter the event loop so any registered handlers / IPC continue to work.
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Main thread.  Runs the TelAF work on a dedicated thread and then idles, so
 * the process stays alive and inspectable on the target.
 */
//--------------------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    LE_UNUSED(argc);
    LE_UNUSED(argv);

    LE_INFO("tafLegacyCApp start");
    printf("LEGACY_APP: tafLegacyCApp start\n");

    pthread_attr_t attr;
    pthread_t tid;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    int ret = pthread_create(&tid, &attr, TelafTask, NULL);
    if (ret != 0)
    {
        LE_ERROR("pthread_create failed, ret=%d", ret);
        return 1;
    }

    // Keep the process alive (the TelAF thread runs the event loop).
    while (1)
    {
        sleep(1);
    }

    return 0;
}
