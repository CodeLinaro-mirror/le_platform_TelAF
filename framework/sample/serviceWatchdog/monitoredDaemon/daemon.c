/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "watchdogChain.h"

//--------------------------------------------------------------------------------------------------
/**
 * Enum for monitored threads.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    MONITOR_MAIN_THREAD_LOOP,
    MONITOR_SUB_THREAD_LOOP
}
MonitorThread_t;

//--------------------------------------------------------------------------------------------------
/**
 * Kick interval (in seconds) for each thread.
 */
//--------------------------------------------------------------------------------------------------
#define MAIN_THREAD_KICK_INTERVAL 10
#define SUB_THREAD_KICK_INTERVAL 15

//--------------------------------------------------------------------------------------------------
/**
 * Entry function for sub thread.
 */
//--------------------------------------------------------------------------------------------------
static void* SubThreadFunction
(
    void* contextPtr
)
{
    // Enable bit0 and bit1 in watchdog chain, which means the watchdogDaemon is kicked only
    // when both bit0 and bit1 are all kicked.
    le_wdogChain_Init(2);

    // Start watchdog 1 and kick bit1 of watchdog chain in sub thread.
    le_clk_Time_t watchdogInterval = { .sec = SUB_THREAD_KICK_INTERVAL };
    le_wdogChain_MonitorEventLoop(MONITOR_SUB_THREAD_LOOP, watchdogInterval);

    LE_INFO("Started watchdog to monitor sub thread.");

    // Call SubThreadInit() here, so that the watchdog can monitor it
    // in case it gets stuck for long time.
    // SubThreadInit();

    // Run the event loop
    le_event_RunLoop();

    return NULL;
}


COMPONENT_INIT
{
    // Enable bit0 in watchdog chain.
    le_wdogChain_Init(1);

    // Start watchdog 0 and kick bit0 of watchdog chain in main thread.
    le_clk_Time_t watchdogInterval = { .sec = MAIN_THREAD_KICK_INTERVAL };
    le_wdogChain_MonitorEventLoop(MONITOR_MAIN_THREAD_LOOP, watchdogInterval);

    LE_INFO("Started watchdog to monitor main thread.");

    // Call MainThreadInit() here, so that the watchdog can monitor it
    // in case it gets stuck for long time.
    // MainThreadInit();

    // Start the sub thread.
    le_thread_Start(le_thread_Create("SubThread", SubThreadFunction, NULL));

#ifdef ENABLE_TIMEOUT_TEST
    // Forcely trigger watchdog timeout for test.
    LE_INFO("Started timeout test.");
    le_thread_Sleep(65);
#endif
}
