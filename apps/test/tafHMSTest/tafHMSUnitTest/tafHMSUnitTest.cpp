/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"


//Threshold value for CPU Load
#define CPU_THRESHOLD_VALUE 30.0

//Threshold value for Free MEM
#define FREE_MEM_THRESHOLD_VALUE 20

/*======================================================================
 FUNCTION        TestGetCPULoad
 DESCRIPTION     Get current CPU Load API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestGetCPULoad(void)
{
    LE_TEST_INFO("===== Get Current Total CPU Load Value =====");
    le_result_t result;
    double cpuLoadValue;
    result = taf_hms_GetCpuLoad(&cpuLoadValue);
    if(result == LE_OK)
    {
        LE_INFO("CPU Load Percentage: %.2f%%\n",cpuLoadValue);
    }
    else
    {
        LE_INFO("Failed ! to get CPU Load information");
    }
    LE_TEST_OK(result == LE_OK, "taf_hms_GetCpuLoad - LE_OK");
    LE_INFO("===== UnitTest Completed for Cpu Load =====");
}

/*======================================================================
 FUNCTION        TestGetCpuCoreNum
 DESCRIPTION     Get CPU core number
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestGetCpuCoreNum(void)
{
    LE_INFO("===== TestGetCpuCoreNum =====");
    uint32_t cpuCoreNum = taf_hms_GetCpuCoreNum();
    LE_INFO("Total Number of CPU cores available - LE_OK: %d\n", cpuCoreNum);
}

/*======================================================================
 FUNCTION        TestGetIndvCoreUsage
 DESCRIPTION     Get each core CPU Usage API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void TestGetIndvCoreUsage(void)
{
    LE_TEST_INFO("===== Get each core CPU usage  =====");
    le_result_t result;
    uint32_t core_id = 0; // Change the core ID as needed
    double cpuUsage;
    uint32_t cpuCoreNum = taf_hms_GetCpuCoreNum();

    for (uint32_t i=0; i<=cpuCoreNum-1; i++)
    {
        result = taf_hms_GetIndvCoreUsage(core_id, &cpuUsage);
        if (result == LE_OK)
        {
            LE_INFO("CPU Usage for core %d %.2f%%\n", core_id, cpuUsage);
        }
        core_id++;
    }
    LE_INFO("===== UnitTest Completed for Each core Cpu Usage =====");
}

/*======================================================================
 FUNCTION        TestGetMemInfo
 DESCRIPTION     Get current RAM Free Memory API test
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
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
}


COMPONENT_INIT
{
    TestGetCPULoad();

    TestGetIndvCoreUsage();

    TestGetMemInfo();

    LE_INFO("---------- All Tests Complete --------------------------");
    exit(EXIT_SUCCESS);
}
