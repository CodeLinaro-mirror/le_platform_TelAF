/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

static le_result_t GetFilter(uint32_t * bitset)
{
    return taf_mngdPm_GetNodeModemWakeupSel(0,
            (taf_mngdPm_NodeModemWsBitMask_t *) bitset);
}

static le_result_t SetFilter(uint32_t bitsetInput)
{

    le_result_t rst =
        taf_mngdPm_SetNodeModemWakeupSel(0,
            (taf_mngdPm_NodeModemWsBitMask_t) bitsetInput);

    if (rst != LE_OK)
    {
        return rst;
    }

    uint32_t bitsetOutput = (uint32_t)(-1);

    rst = GetFilter(&bitsetOutput);

    if (rst != LE_OK)
    {
        return rst;
    }

    printf("Set: 0x%08x, Get: 0x%08x\n",
           bitsetInput, bitsetOutput);

    return LE_OK;
}

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

static void ShowHelp(void)
{
    printf("Usage: %s <get>|<set> [hex-value]\n", le_arg_GetProgramName());
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

COMPONENT_INIT
{
    le_result_t rst = LE_OK;

    int numberOfArgs = le_arg_NumArgs();
    if (numberOfArgs < 1)
    {
        ShowHelp();
        StopCommandLine(false);
    }

    const char * action = le_arg_GetArg(0);
    uint32_t bitset = 0;

    if (strcmp(action, "get") == 0)
    {
        rst = GetFilter(&bitset);
        if (rst != LE_OK)
        {
            printf("Failed to get filter\n");
            StopCommandLine(false);
        }

        printf("Get filter: 0x%08x\n", bitset);
    }
    else if (strcmp(action, "set") == 0)
    {
        if (numberOfArgs < 2)
        {
            ShowHelp();
            StopCommandLine(false);
        }

        const char * hexValue = le_arg_GetArg(1);

        le_result_t rst = HexToUint(hexValue, &bitset);
        if (rst != LE_OK)
        {
            printf("Bad hex value for bitset\n");
            ShowHelp();
            StopCommandLine(false);
        }

        rst = SetFilter(bitset);
        if (rst != LE_OK)
        {
            printf("Failed to set filter\n");
            StopCommandLine(false);
        }
    }
    else
    {
        printf("Bad action for this command\n");
        ShowHelp();
        StopCommandLine(false);
    }

    StopCommandLine(true);
}
