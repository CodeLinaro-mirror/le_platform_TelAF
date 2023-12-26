/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"
#include "tafHMS.hpp"

using namespace telux::tafsvc;
using namespace std;

le_result_t taf_hms_GetCpuLoad
(
    double* cpuCurrentLoadPtr
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetCpuLoad(cpuCurrentLoadPtr);
}

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