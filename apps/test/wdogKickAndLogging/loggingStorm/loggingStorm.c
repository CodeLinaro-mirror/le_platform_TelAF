/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

/* | sleep (ms) | logging (n) times | sleep (ms) | logging (n) times | ... */

static int BeSilentMs = -1;
static int LoggingTimes = -1;
static bool LoggingFirst = false;

static void PrintHelp(void)
{
    puts("/<path>/<to>/tafTestLogginStorm -ms <sleep-ms> -n <logging-times> [--logging-first]\n");
    exit(EXIT_SUCCESS);
}

static void CheckBeSilentMs(int value)
{
    if (value < 0 || value > 60000)
    {
        fprintf(stderr, "Invalid parameter (-ms): %d, should be 0 < ms < 60000\n", value);
        exit(EXIT_FAILURE);
    }
    else
    {
        BeSilentMs = value;
    }
}

static void CheckLoggingTimes(int value)
{
    if (value < 0)
    {
        fprintf(stderr, "Invalid parameter (-n): %d, should be n > 0\n", value);
        exit(EXIT_FAILURE);
    }
    else
    {
        LoggingTimes = value;
    }
}

static void LoggingTime
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("Time to start logging <-- (%d)", LoggingTimes);

    for (int i = 0; i < LoggingTimes; i++)
    {
        LE_INFO("Logging Storm ~ [%d]", i);
    }

    LE_INFO("Time to start sleeping <-- (%d)", BeSilentMs);
}

COMPONENT_INIT
{
    le_arg_SetFlagCallback(PrintHelp, "h", "help");
    le_arg_SetIntCallback(CheckBeSilentMs, "ms", NULL);
    le_arg_SetIntCallback(CheckLoggingTimes, "n", NULL);
    le_arg_SetFlagVar(&LoggingFirst, NULL, "logging-first");
    le_arg_Scan();

    if (LE_OK != le_arg_GetFlagOption("ms", NULL))
    {
        fprintf(stderr, "Err: -ms/ is required, Try --help\n");
        exit(EXIT_FAILURE);
    }
    if (LE_OK != le_arg_GetFlagOption("n", NULL))
    {
        fprintf(stderr, "Err: -n/ is required, Try --help\n");
        exit(EXIT_FAILURE);
    }

    LE_INFO("[test] output from [%s], print %d(times), silent %d(ms)",
            LoggingFirst == true ? "logging" : "sleep",
            LoggingTimes, BeSilentMs);
    printf("[test] output from [%s], print %d(times), silent %d(ms)\n",
           LoggingFirst == true ? "logging" : "sleep",
           LoggingTimes, BeSilentMs);

    if (LoggingFirst == true)
    {
        LoggingTime(NULL);
    }

    le_timer_Ref_t TimerRef = le_timer_Create("LoggingStorm-Timer");
    le_timer_SetMsInterval(TimerRef, BeSilentMs);
    le_timer_SetHandler(TimerRef, LoggingTime);
    le_timer_SetRepeat(TimerRef, 0);
    le_timer_SetWakeup(TimerRef, false);
    le_timer_Start(TimerRef);

    /* Main thread event-loop forever */
}
