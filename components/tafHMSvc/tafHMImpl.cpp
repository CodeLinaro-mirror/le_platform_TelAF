/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/*
 * @file       tafHMImpl.cpp
 * @brief      This file describes the implementation method that health
 *             monitor service is in use.
 */

#include "legato.h"
#include "interfaces.h"
#include "tafHMS.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>


using namespace telux::tafsvc;


/**
 * Returns HMS instance
 */
taf_Hms &taf_Hms::GetInstance()
{
    static taf_Hms instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 ** Gets the cpu Idle value from " /proc/stat ".
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::GetCpuLoad
(
    double* cpuCurrentLoadPtr
)
{
    FILE *file;
    char buffer[128];
    uint32_t user, nice, system, idle;

    file = fopen("/proc/stat", "r");
    if (file == NULL) {
        LE_ERROR("Error opening /proc/stat");
        return LE_FAULT;
    }

    // Read the first line of /proc/stat
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
       LE_ERROR("Error reading /proc/stat");
        fclose(file);
        return LE_FAULT;
    }

    // Parse the CPU usage information
    sscanf(buffer, "cpu %d %d %d %d", &user, &nice, &system, &idle);

    fclose(file);

    // Calculate CPU idle time as a percentage
    uint32_t total = user + nice + system + idle;
    double idle_percentage = ((double)idle / total) * 100;

    *cpuCurrentLoadPtr = idle_percentage;
    LE_INFO("CPU Idle Percentage: %.2f%%\n", idle_percentage);
    return LE_OK;
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
le_result_t taf_Hms::GetRamMemInfo
(
    uint32_t* ramTotalMemPtr,
    uint32_t* ramUsedMemPtr,
    uint32_t* ramFreeMemPtr
)
{
    FILE *file;
    char buffer[128];
    uint32_t total_mem, free_mem, used_mem;

    file = fopen("/proc/meminfo", "r");
    if (file == NULL)
    {
        LE_ERROR( "Error - Failed opening /proc/meminfo" );
        return LE_FAULT;
    }

    // Read lines from /proc/meminfo to find total and free memory
    while (fgets(buffer, sizeof(buffer), file) != NULL)
    {
        if (sscanf(buffer, "MemTotal: %d kB", &total_mem) == 1)
        {
            continue;
        }
        else if (sscanf(buffer, "MemFree: %d kB", &free_mem) == 1)
        {
            break;
        }
    }

    fclose(file);

    // Calculate used memory
    used_mem = total_mem - free_mem;

    *ramTotalMemPtr = total_mem;
    LE_INFO("Total Memory: %d kB\n", total_mem);
    *ramUsedMemPtr = used_mem;
    LE_INFO("Used Memory: %d kB\n", used_mem);
    *ramFreeMemPtr = free_mem;
    LE_INFO("Free Memory: %d kB\n", free_mem);
    return LE_OK;
}


void taf_Hms::Init()
{
    LE_INFO("tafHMSvc started");
}