/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"
#include "tafHMS.hpp"

using namespace telux::tafsvc;
using namespace std;


//--------------------------------------------------------------------------------------------------
/**
 ** Gets the total cpu usage from " /proc/stat ".
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetCpuLoad
(
    double* cpuCurrentLoadPtr
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetCpuLoad(cpuCurrentLoadPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 ** Gets number of CPU core from " /proc/cpuinfo ".
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_hms_GetCpuCoreNum
(
    void
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetCpuCoreNum();
}

//--------------------------------------------------------------------------------------------------
/**
 ** Gets the CPU usage of each core from " /proc/stat ".
 ** The API provides total CPU usage of each core ranging from 0 to 100 %.
 ** The coreID range can be 0 - (cpuCoreNum-1), the result value can be LE_OK.
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetIndvCoreUsage
(
    uint32_t coreID,
        ///< [IN] Core ID
    double* cpuUsagePtr
        ///< [OUT] cpuUsage
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetIndvCoreUsage(coreID, cpuUsagePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 ** Gets the meminfo value from " /proc/meminfo ".
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetRamMemInfo
(
    uint32_t* ramTotalMemPtr,
    uint32_t* ramUsedMemPtr,
    uint32_t* ramFreeMemPtr
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetRamMemInfo(ramTotalMemPtr, ramUsedMemPtr, ramFreeMemPtr);
}


/**
 * The initialization of TelAF Health Monitor component.
*/
COMPONENT_INIT
{
    LE_INFO("TelAF Health Monitor Service init Started...");
    auto &hms = taf_Hms::GetInstance();
    hms.Init();
    LE_INFO("TelAF Health Monitor Service init completed...");
}
