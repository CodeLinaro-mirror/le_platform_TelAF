/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

static taf_mngdPmTest_MpmsTestRef_t MpmsTestDaemonRef;
static void (*ActionFn)(void);
static const char* bitmaskFromCmd = NULL;

static void StopCommandLine(bool result)
{
    if (result == true)
    {
        exit(EXIT_SUCCESS);
    }
    else
    {
        exit(EXIT_FAILURE);
    }
}

static void PrintHelp(void)
{
    printf("Usage: %s help|get.mpss.filter|set.mpss.filter|"
           "up.wakeup.monitor|down.wakeup.monitor [bitmask]\n",
            le_arg_GetProgramName());
}

static le_result_t HexToUint(const char *hexStr, uint32_t *value)
{
    uint32_t val = 0;

    if (sscanf(hexStr, "%x", &val) != 1)
    {
        printf("Err: bad parameter\n");
        return LE_FAULT;
    }

    *value = val;

    return LE_OK;
}

static void GetMpssFilter(void)
{
    uint32_t bitset = 0;

    le_result_t rst =
        taf_mngdPmTest_GetWakeupFilterFromMpss(MpmsTestDaemonRef,
                                              &bitset);
    if (rst != LE_OK)
    {
        printf("Failed to taf_mngdPmTest_GetWakeupFilterFromMpss\n");
        StopCommandLine(false);
    }

    printf("Get filter: 0x%08x\n", bitset);
    StopCommandLine(true);
}

static void SetMpssFitler(void)
{
    uint32_t bitsetInput = 0;
    uint32_t bitsetOutput = 0;

    le_result_t rst = HexToUint(bitmaskFromCmd, &bitsetInput);
    if (rst != LE_OK)
    {
        printf("Bad hex value for bitset\n");
        StopCommandLine(false);
    }

    rst = taf_mngdPmTest_SetWakeupFitlerToMpss(MpmsTestDaemonRef, bitsetInput);
    if (rst != LE_OK)
    {
        printf("Failed to taf_mngdPmTest_SetWakeupFitlerToMpss\n");
        StopCommandLine(false);
    }

    rst = taf_mngdPmTest_GetWakeupFilterFromMpss(MpmsTestDaemonRef, &bitsetOutput);
    if (rst != LE_OK)
    {
        printf("Failed to taf_mngdPmTest_GetWakeupFilterFromMpss\n");
        StopCommandLine(false);
    }

    printf("Set: 0x%08x, Get: 0x%08x\n", bitsetInput, bitsetOutput);

    StopCommandLine(true);
}

static void UpMonitorForWakeup(void)
{
    le_result_t rst =
        taf_mngdPmTest_UpMonitorForWakeup(MpmsTestDaemonRef);

    if (rst != LE_OK)
    {
        printf("Failed to taf_mngdPmTest_UpMonitorForWakeup\n");
        StopCommandLine(false);
    }
    else
    {
        StopCommandLine(true);
    }
}

static void QueryWakeupReason(void)
{
    uint32_t outputBitset = 0;
    le_result_t rst =
        taf_mngdPmTest_QueryLastWakeupReason(MpmsTestDaemonRef, &outputBitset);

    if (rst != LE_OK)
    {
        printf("Failed to taf_mngdPmTest_QueryLastWakeupReason\n");
        StopCommandLine(false);
    }
    else
    {
        printf("Query result: 0x%04x\n", outputBitset);
        StopCommandLine(true);
    }
}

static void DownMonitorForWakeup(void)
{
    taf_mngdPmTest_DownMonitorForWakeup(MpmsTestDaemonRef);
    StopCommandLine(true);
}

static void BitmaskValueHandler
(
    const char* bitmaskStr
)
{
    bitmaskFromCmd = bitmaskStr;
}

static void RelaxWakeSource
(
)
{
    taf_mngdPmTest_RelaxWakeSource(MpmsTestDaemonRef);
}

static void ActionHandler
(
    const char* command
)
{
    if (strcmp(command, "help") == 0)
    {
        PrintHelp();
        StopCommandLine(true);
    }

    if (strcmp(command, "get.mpss.filter") == 0)
    {
        ActionFn = GetMpssFilter;
    }
    else if (strcmp(command, "set.mpss.filter") == 0)
    {
        ActionFn = SetMpssFitler;

        le_arg_AddPositionalCallback(BitmaskValueHandler);
    }
    else if (strcmp(command, "up.wakeup.monitor") == 0)
    {
        ActionFn = UpMonitorForWakeup;
    }
    else if (strcmp(command, "down.wakeup.monitor") == 0)
    {
        ActionFn = DownMonitorForWakeup;
    }
    else if (strcmp(command, "query.wakeup.reason") == 0)
    {
        ActionFn = QueryWakeupReason;
    }
    else if (strcmp(command, "relax.wake.source") == 0)
    {
        ActionFn = RelaxWakeSource;
    }
    else
    {
        fprintf(stderr, "Unknown command '%s'. Try --help.\n", command);
        StopCommandLine(false);
    }
}

COMPONENT_INIT
{
    le_result_t rst = LE_OK;

    le_arg_SetFlagCallback(PrintHelp, "h", "help");

    le_arg_AddPositionalCallback(ActionHandler);

    le_arg_Scan();

    taf_mngdPmTest_ConnectService();

    rst = taf_mngdPmTest_GetMpmsTestObject(&MpmsTestDaemonRef);
    if (rst != LE_OK)
    {
        printf("Failed to taf_mngdPmTest_GetMpmsTestObject");
        StopCommandLine(false);
    }

    ActionFn(); // NOTE: StopCommandline for each ActionFn !
}
