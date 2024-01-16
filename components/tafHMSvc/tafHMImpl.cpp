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


using namespace std;
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
 ** Gets the total cpu usage from " /proc/stat ".
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
    double currentCPULoad = 100.0 - idle_percentage;

    *cpuCurrentLoadPtr = currentCPULoad;
    LE_INFO("Current CPU Load: %.2f%%\n", currentCPULoad);
    return LE_OK;
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
uint32_t taf_Hms::GetCpuCoreNum
(
    void
)
{
    FILE *fp;
    char line[256];
    int core_count = 0;

    // Open /proc/cpuinfo file
    fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) {
        LE_ERROR("Error opening /proc/cpuinfo");
        return LE_FAULT;
    }

    // Read line by line and count the number of cores
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "processor", 9) == 0) {
            core_count++;
        }
    }

    fclose(fp);

    // Print the total number of CPU cores
    LE_INFO("Total CPU cores: %d\n", core_count);
    return core_count;
}


// Function to calculate total CPU usage for a core
double calculate_core_cpu_usage(struct CPUCore core)
{
    uint32_t total_non_idle = core.user + core.nice + core.system +
                         core.irq + core.softirq + core.steal + core.guest;
    uint32_t total_time = total_non_idle + core.idle;
    return ((double)total_non_idle / total_time) * 100.0;
}

//--------------------------------------------------------------------------------------------------
/**
 ** Gets the CPU usage of each core from " /proc/stat ".
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::GetIndvCoreUsage
(
    uint32_t coreID,
        ///< [IN] Core ID
    double* cpuUsagePtr
        ///< [OUT] cpuUsage
)
{
    FILE* fp;
    char buffer[1024];
    uint32_t cpu_count = 0;
    struct CPUCore cpu_usage[MAX_CORES];

    fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        LE_ERROR("Error opening /proc/stat");
        return LE_FAULT;
    }

    // Read /proc/stat line by line
    while (fgets(buffer, sizeof(buffer), fp))
    {
        if (strncmp(buffer, "cpu", 3) == 0)
        {
            uint32_t current_core_id;
            sscanf(buffer, "cpu%d", &current_core_id);
            if (current_core_id == coreID)
            {
                // Parse the buffer manually to extract CPU usage fields
                char *ptr = buffer;
                while (*ptr != '\0')
                {
                    if (strncmp(ptr, " ", 1) == 0)
                    {
                        uint32_t value;
                        if (sscanf(ptr, " %d", &value) == 1)
                        {
                            switch(cpu_count)
                            {
                                case 0:
                                    cpu_usage[coreID].user = value;
                                    break;
                                case 1:
                                    cpu_usage[coreID].nice = value;
                                    break;
                                case 2:
                                    cpu_usage[coreID].system = value;
                                    break;
                                case 3:
                                    cpu_usage[coreID].idle = value;
                                    break;
                                case 4:
                                    cpu_usage[coreID].iowait = value;
                                    break;
                                case 5:
                                    cpu_usage[coreID].irq = value;
                                    break;
                                case 6:
                                    cpu_usage[coreID].softirq = value;
                                    break;
                                case 7:
                                    cpu_usage[coreID].steal = value;
                                    break;
                                case 8:
                                    cpu_usage[coreID].guest = value;
                                    break;
                                default:
                                    break;
                            }
                            cpu_count++;
                            if (cpu_count == MAX_FIELDS) break;
                        }
                    }
                    ptr++;
                }
                fclose(fp);
                *cpuUsagePtr = calculate_core_cpu_usage(cpu_usage[coreID]);
                return LE_OK;
            }
        }
    }

    // Core ID not found
    LE_ERROR("Error: Core ID %d not found\n", coreID);
    return LE_FAULT;
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
