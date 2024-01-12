/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"



void TestGetCPULoad(void)
{
    LE_TEST_INFO("===== Get Current CPU Load Idle Value =====");
    le_result_t result;
    double cpuLoadValue;
    result = taf_hms_GetCpuLoad(&cpuLoadValue);
    if(result == LE_OK)
    {
        LE_INFO("CPU Load Idle Percentage: %.2f%%\n",cpuLoadValue);
    }
    else
    {
        LE_INFO("Failed ! to get CPU Load information");
    }
    LE_TEST_OK(result == LE_OK, "taf_hms_GetCpuLoad - LE_OK");
    LE_INFO("===== UnitTest Completed for Cpu Load =====");
}

void TestGetMemInfo()
{
    LE_TEST_INFO("===== Get Current Mem info Value =====");
    le_result_t result;
    uint32_t ramTotalMem, ramUsedMem, ramFreeMem;
    result = taf_hms_GetRamMemInfo( &ramTotalMem, &ramUsedMem, &ramFreeMem);
    if(result == LE_OK)
    {
        LE_INFO(" Total Memory: %d kB\n", ramTotalMem);
        LE_INFO(" Used Memory: %d kB\n", ramUsedMem);
        LE_INFO(" Free Memory: %d kB\n", ramFreeMem);
    }
    else
    {
        LE_INFO("Failed ! to get RAM information");
    }
    LE_TEST_OK(result == LE_OK, "taf_hms_GetRamMemInfo - LE_OK");
    LE_INFO("===== UnitTest Completed for RAM Mem Info =====");
    LE_TEST_EXIT;
}


COMPONENT_INIT
{
    TestGetCPULoad();
    TestGetMemInfo();
    LE_INFO("---------- All Tests Complete --------------------------");
    exit(EXIT_SUCCESS);
}
