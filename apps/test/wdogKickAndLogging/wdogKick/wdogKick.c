/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "watchdogChain.h"

#define NUM_OF_WDOG 1
#define IDX_OF_WDOG 0

static int KickIntervalSec = -1;

static void PrintHelp(void)
{
    puts("/<path>/<to>/tafTestWdogKick -s <second> | --second-kick=<second>\n");
    exit(EXIT_SUCCESS);
}

static void CheckKickInternalSec(int value)
{
    // The 3600s was configured in ADEF file.
    if (value < 0 || value > 3600)
    {
        fprintf(stderr, "Invalid parameter: %d, should be 0 < second < 3600\n", value);
        exit(EXIT_FAILURE);
    }
    else
    {
        KickIntervalSec = value;
    }
}

COMPONENT_INIT
{
    le_arg_SetFlagCallback(PrintHelp, "h", "help");
    le_arg_SetIntCallback(CheckKickInternalSec, "s", "second-kick");
    le_arg_Scan();

    if (LE_OK != le_arg_GetFlagOption("s", "second-kick"))
    {
        fprintf(stderr, "Err: -s/--second-kick is required, Try --help\n");
        exit(EXIT_FAILURE);
    }

    le_clk_Time_t kickInterval = { .sec = KickIntervalSec};
    le_wdogChain_Init(NUM_OF_WDOG);
    le_wdogChain_MonitorEventLoop(IDX_OF_WDOG, kickInterval);

    LE_INFO("[test] kick wdog per (%d) second", KickIntervalSec);
    printf("[test] kick wdog per (%d) second\n", KickIntervalSec);

    /* Main thread event-loop forever */
}
