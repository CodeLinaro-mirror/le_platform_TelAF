/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

#define NAME_SIZE 32

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


/*======================================================================
 FUNCTION        TestUbiDevInfo
 DESCRIPTION     To test UBI device Info
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void Test_taf_Hms_UbiDevInfo()
{
    le_result_t result;
    LE_TEST_INFO("=====Testing UBI Device Information =====");
    taf_hms_UbiDevInfoListRef_t ubiDevListRef = taf_hms_GetUbiDevInfoList();

    LE_TEST_OK((ubiDevListRef != NULL), "taf_hms_GetUbiDevInfoList - LE_OK");

    if(ubiDevListRef != NULL)
    {
        taf_hms_UbiDevInfoRef_t ubiDevInfo = taf_hms_GetFirstUbiDevInfo(ubiDevListRef);
        LE_TEST_OK((ubiDevInfo != NULL), "taf_hms_GetFirstUbiDevInfo - LE_OK");

        while (ubiDevInfo != NULL)
        {
            LE_TEST_INFO("Test get UBI device bad block count");
            uint32_t ubiBadblock;
            result = taf_hms_GetUbiDevBadBlkCnt(ubiDevInfo, &ubiBadblock);
            LE_TEST_OK(result == LE_OK, "taf_hms_GetUbiDevBadBlkCnt - LE_OK. Returned %d",
                        ubiBadblock);
            LE_TEST_INFO("Test get UBI device erase count");
            uint32_t ubiEraseCount;
            result = taf_hms_GetUbiDevMaxEraseCnt(ubiDevInfo, &ubiEraseCount);
            LE_TEST_OK(result == LE_OK, "taf_hms_GetUbiDevMaxEraseCnt - LE_OK. Returned %d", ubiEraseCount);
            ubiDevInfo = taf_hms_GetNextUbiDevInfo(ubiDevListRef);
        }
    }
    LE_INFO("===== UnitTest Completed for UBI device information =====");
}


/*======================================================================
 FUNCTION        TestMtdInfo
 DESCRIPTION     To test MTD device Info
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void Test_taf_Hms_MtdDevInfo()
{
    le_result_t result;
    LE_TEST_INFO("=====Testing MTD device Information =====");
    taf_hms_MtdDevInfoListRef_t mtdListRef = taf_hms_GetMtdDevInfoList();

    LE_TEST_OK((mtdListRef != NULL), "taf_hms_GetMtdDevInfoList - LE_OK");

    if(mtdListRef != NULL)
    {
        taf_hms_MtdDevInfoRef_t mtdInfo = taf_hms_GetFirstMtdDevInfo(mtdListRef);
        LE_TEST_OK((mtdInfo != NULL), "taf_hms_GetFirstMtdDevInfo - LE_OK");

        while (mtdInfo != NULL)
        {
            LE_TEST_INFO("Test get MTD device name");
            char MtdDevName[NAME_SIZE];
            memset(MtdDevName, 0, NAME_SIZE);
            result = taf_hms_GetMtdDevName(mtdInfo, MtdDevName, sizeof(MtdDevName));
            LE_TEST_OK(result == LE_OK, "taf_hms_GetMtdVolName - LE_OK. Returned %s", MtdDevName);

            LE_TEST_INFO("Test get MTD device block size");
            uint32_t mtdblockSize;
            result = taf_hms_GetMtdDevBlkSize(mtdInfo, &mtdblockSize);
            LE_TEST_OK(result == LE_OK, "taf_hms_GetMtdDevBlkSize - LE_OK. Returned %d",
                    mtdblockSize);
            mtdInfo = taf_hms_GetNextMtdDevInfo(mtdListRef);
        }
    }
    LE_INFO("===== UnitTest Completed for MTD device information =====");
}


COMPONENT_INIT
{
    TestGetCPULoad();

    TestGetIndvCoreUsage();

    TestGetMemInfo();

    Test_taf_Hms_UbiDevInfo();

    Test_taf_Hms_MtdDevInfo();

    LE_INFO("---------- All Tests Complete --------------------------");
    exit(EXIT_SUCCESS);
}
