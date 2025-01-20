/*
* Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <chrono>
#include <thread>
#include <dirent.h>


using namespace std;
using namespace telux::tafsvc;

le_timer_Ref_t HmsTimerRef = NULL;
le_sem_Ref_t SemRef = NULL;
le_thread_Ref_t ThreadRef = NULL;

LE_MEM_DEFINE_STATIC_POOL(UbiDevListPool, TAF_HMS_MAX_LIST_POOL_SIZE,
    sizeof(taf_hms_ubiDevInfoList_t));
LE_MEM_DEFINE_STATIC_POOL(UbiDevInfoPool, TAF_HMS_MAX_LIST_POOL_SIZE,
    sizeof(taf_hms_ubiDevInfo_t));
LE_MEM_DEFINE_STATIC_POOL(UbiVolListPool, TAF_HMS_MAX_LIST_POOL_SIZE,
    sizeof(taf_hms_ubiVolInfoList_t));
LE_MEM_DEFINE_STATIC_POOL(UbiVolInfoPool, TAF_HMS_MAX_LIST_POOL_SIZE,
    sizeof(taf_hms_ubiVolInfo_t));
LE_MEM_DEFINE_STATIC_POOL(MtdListPool, TAF_HMS_MAX_LIST_POOL_SIZE, sizeof(taf_hms_mtdInfoList_t));
LE_MEM_DEFINE_STATIC_POOL(MtdInfoPool, TAF_HMS_MAX_LIST_POOL_SIZE, sizeof(taf_hms_mtdInfo_t));


LE_REF_DEFINE_STATIC_MAP(UbiDevListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(UbiDevRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(UbiVolListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(UbiVolRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(MtdListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(MtdRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);


//--------------------------------------------------------------------------------------------------
/**
 * Returns HMS instance.
 */
//--------------------------------------------------------------------------------------------------
taf_Hms &taf_Hms::GetInstance()
{
    static taf_Hms instance;
    return instance;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to read CPU usage from /proc/stat.
 */
//--------------------------------------------------------------------------------------------------
static taf_hms_CPUCore_t GetCpuUsage()
{
    FILE *file = fopen("/proc/stat", "r");
    if (!file)
    {
        LE_FATAL("Failed to fopen /proc/stat");
    }

    taf_hms_CPUCore_t usage;
    fscanf(file, "cpu %d %d %d %d %d %d %d %d %d %d",
        &usage.user, &usage.nice, &usage.system, &usage.idle, &usage.iowait,
        &usage.irq, &usage.softirq, &usage.steal, &usage.guest, &usage.guest_nice);
    fclose(file);
    return usage;
}


//--------------------------------------------------------------------------------------------------
/**
 * Computes the total CPU time.
 */
//--------------------------------------------------------------------------------------------------
static double GetTotalTime(const taf_hms_CPUCore_t *usage)
{
    return usage->user + usage->nice + usage->system + usage->idle +
           usage->iowait + usage->irq + usage->softirq + usage->steal +
           usage->guest + usage->guest_nice;
}


//--------------------------------------------------------------------------------------------------
/**
 * Computes the idle CPU time.
 */
//--------------------------------------------------------------------------------------------------
static double GetIdleTime(const taf_hms_CPUCore_t *usage)
{
    return usage->idle + usage->iowait;
}


//--------------------------------------------------------------------------------------------------
/**
 * Calculates the CPU usage percentage based on the differences in CPU times.
 */
//--------------------------------------------------------------------------------------------------
static double CalculateCpuUsage(const taf_hms_CPUCore_t *start, const taf_hms_CPUCore_t *end)
{
    float start_total = GetTotalTime(start);
    float end_total = GetTotalTime(end);
    float total_diff = end_total - start_total;

    float start_idle = GetIdleTime(start);
    float end_idle = GetIdleTime(end);
    float idle_diff = end_idle - start_idle;

    return 100.0 * (total_diff - idle_diff) / total_diff;
}


//--------------------------------------------------------------------------------------------------
/**
 * Timer handler.
 */
//--------------------------------------------------------------------------------------------------
static void TimerExpiryHandler(le_timer_Ref_t timerRef)
{
    LE_DEBUG("Timer expired, posting semaphore");
    le_sem_Post(SemRef);
    le_timer_Delete(timerRef);
    le_thread_Exit(0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sub thread.
 */
//--------------------------------------------------------------------------------------------------
static void* SubThreadMain(void* context)
{
    LE_INFO("Sub-thread started, starting timer");

    // Create and configure the timer
    HmsTimerRef = le_timer_Create("hmsTimer");     //create timer
    le_timer_SetMsInterval(HmsTimerRef, 1000);        //update every 1 seconds
    le_timer_SetHandler(HmsTimerRef, TimerExpiryHandler);
    le_timer_SetRepeat(HmsTimerRef, 1);                   //set no repeat
    le_timer_Start(HmsTimerRef);
    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 ** Gets the total cpu usage from " /proc/stat ".
 ** Calculates the total CPU usage by measuring the difference in CPU times over a 1-second
 **  interval.
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
    TAF_ERROR_IF_RET_VAL(cpuCurrentLoadPtr == NULL, LE_BAD_PARAMETER, "cpuCurrentLoadPtr pointer is NULL");
    // Get CPU usage at "START" point
    taf_hms_CPUCore_t start_usage = GetCpuUsage();

    // Create a semaphore
    SemRef = le_sem_Create("Semaphore", 0);

    // Create and start the sub-thread
    ThreadRef = le_thread_Create("SubThread", SubThreadMain, NULL);
    le_thread_Start(ThreadRef);

    // Main thread waits for the semaphore to be posted
    le_sem_Wait(SemRef);

    // Delete semaphore.
    le_sem_Delete(SemRef);

    // Get CPU usage at "END" point
    taf_hms_CPUCore_t end_usage = GetCpuUsage();

    // Calculate and log the CPU usage difference
    double cpu_usage = CalculateCpuUsage(&start_usage, &end_usage);
    *cpuCurrentLoadPtr = cpu_usage;
    LE_DEBUG("Total current CPU usage: %.2f%%\n", cpu_usage);

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
    if (fp == NULL)
    {
        LE_ERROR("Error opening /proc/cpuinfo");
        return LE_FAULT;
    }

    // Read line by line and count the number of cores
    while (fgets(line, sizeof(line), fp))
    {
        if (strncmp(line, "processor", 9) == 0)
        {
            core_count++;
        }
    }

    fclose(fp);

    // Print the total number of CPU cores
    LE_INFO("Total CPU cores: %d\n", core_count);
    return core_count;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to calculate total CPU usage for a core.
 */
//--------------------------------------------------------------------------------------------------
static double CalculateCoreCpuUsage(taf_hms_CPUCore_t core)
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
    TAF_ERROR_IF_RET_VAL(cpuUsagePtr == NULL, LE_BAD_PARAMETER, "cpuUsagePtr pointer is NULL");
    FILE* fp;
    char buffer[1024];
    uint32_t cpu_count = 0;
    taf_hms_CPUCore_t cpu_usage[MAX_CORES] = {0};

    fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
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
                *cpuUsagePtr = CalculateCoreCpuUsage(cpu_usage[coreID]);
                return LE_OK;
            }
        }
    }

    // Core ID not found
    LE_ERROR("Error: Core ID %d not found\n", coreID);
    fclose(fp);
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
    TAF_ERROR_IF_RET_VAL(ramTotalMemPtr == NULL || ramUsedMemPtr == NULL || ramFreeMemPtr == NULL,
                     LE_BAD_PARAMETER,
                     "One or more input pointers are NULL");
    FILE *file;
    char buffer[128];
    uint32_t total_mem = 0, free_mem = 0, used_mem = 0;

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


//--------------------------------------------------------------------------------------------------
/**
 * Function to read a file from the given path.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t ReadSysfsFile(const char *path)
{
    uint32_t value = 0;
    uint32_t  rc  = 0;
    FILE *fp = NULL;

    if(NULL == path)
    {
        LE_ERROR("Failed to open file\n");
        return LE_FAULT;
    }
    fp = fopen(path, "r");
    if (NULL == fp)
    {
        return LE_FAULT;
    }

    rc = fscanf(fp, "%d\n", &value);
    UNUSED(rc);
    fclose(fp);
    return value;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to read a string from the given path.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t ReadSysfsStringFile(const char *path, char *buffer, size_t length)
{
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        LE_ERROR("Failed to open file");
        return LE_FAULT;
    }

    // Clear the buffer before reading new data
    memset(buffer, 0, length);

    if (fgets(buffer, length, file) == NULL)
    {
        perror("Failed to read file");
        fclose(file);
        return LE_FAULT;
    }
    fclose(file);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get device count from the sys class path.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t GetUbiDeviceCount
(
)
{
    DIR *dir;
    struct dirent *entry;
    uint32_t count = 0;

    dir = opendir(UBI_CLASS_PATH);
    if (dir == NULL)
    {
        LE_ERROR("Error opening UBI class path");
        return LE_FAULT;
    }

    // Iterate through each entry in the directory
    while ((entry = readdir(dir)) != NULL)
    {
        // Check if the entry is a directory and its name starts with "ubi"
        if (entry->d_type == DT_DIR && strncmp(entry->d_name, "ubi", 3) == 0)
        {
            count++;
        }
    }

    closedir(dir);
    LE_INFO("UBI device count: %d", count);
    return count;
}


//--------------------------------------------------------------------------------------------------
/**
 ** Gets the list of available UBI device information.
 **
 ** @return
 *  - NULL                            No information found.
 *  - taf_hms_ubiDevInfoListRef    The UBI device Info list object reference.
 */
//-------------------------------------------------------------------------------------------------
taf_hms_UbiDevInfoListRef_t taf_Hms::GetUbiDevInfoList
(
    void
)
{
    taf_hms_ubiDevInfoList_t* ubiDevList = (taf_hms_ubiDevInfoList_t*)
        le_mem_ForceAlloc(UbiDevListPool);
    memset(ubiDevList, 0, sizeof(taf_hms_ubiDevInfoList_t));
    ubiDevList->ubiDevInfoList = LE_SLS_LIST_INIT;
    ubiDevList->currPtr = NULL;

    taf_hms_ubiDevInfo_t* ubiDevInfoPtr;

    uint32_t count = GetUbiDeviceCount();
    char path[MAX_PATH_LENGTH];
    if (count >= 0)
    {
        for(uint32_t i = 0; i <= count; i++)
        {
            ubiDevInfoPtr = (taf_hms_ubiDevInfo_t*)le_mem_ForceAlloc(UbiDevInfoPool);
            memset(ubiDevInfoPtr, 0, sizeof(taf_hms_ubiDevInfo_t));

            // Get bad block count
            snprintf(path, sizeof(path), UBI_DEV_BB_COUNT_PATH, i);
            ubiDevInfoPtr->badBlockCnt = ReadSysfsFile(path);

            // Get max erase count
            snprintf(path, sizeof(path), UBI_DEV_E_COUNT_PATH, i);
            ubiDevInfoPtr->eraseCnt = ReadSysfsFile(path);

            ubiDevInfoPtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(ubiDevList->ubiDevInfoList), &(ubiDevInfoPtr->link));
            ubiDevInfoPtr->ref =
                 (taf_hms_UbiDevInfoRef_t)le_ref_CreateRef(UbiDevRefMap, (void*)ubiDevInfoPtr);
        }
        ubiDevList->ref =
            (taf_hms_UbiDevInfoListRef_t)le_ref_CreateRef(UbiDevListRefMap, ubiDevList);
        return ubiDevList->ref;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 ** Deletes the UbiDevInfoList list retrieved with taf_hms_GetUbiDevInfoList().
 **
 ** @return
 **  - LE_BAD_PARAMETER -- Bad parameters.
 **  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::DeleteUbiDevInfoList
(
    taf_hms_UbiDevInfoListRef_t ubiDevInfoListRef
)
{
    taf_hms_ubiDevInfoList_t* listPtr =
        (taf_hms_ubiDevInfoList_t*)le_ref_Lookup(UbiDevListRefMap, ubiDevInfoListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");
    LE_DEBUG("DeleteUBIDevInfoList : %p", ubiDevInfoListRef);
    taf_hms_ubiDevInfo_t* ubiDevInfoPtr;
    le_sls_Link_t* linkPtr;
    while ((linkPtr = le_sls_Pop(&(listPtr->ubiDevInfoList))) != NULL)
    {
        ubiDevInfoPtr = CONTAINER_OF(linkPtr, taf_hms_ubiDevInfo_t, link);
        le_mem_Release(ubiDevInfoPtr);
    }
    le_ref_DeleteRef(UbiDevListRefMap, ubiDevInfoListRef);
    le_mem_Release(listPtr);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the first UBI device Info object reference in the list of the
 * ubiDevInfoList retrieved with taf_hms_GetUBIDevInfoList().
 *
 * @return
 *  - NULL                          No information found.
 *  - taf_hms_ubiDevInfoListRef      The UBI device Info object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_UbiDevInfoRef_t taf_Hms::GetFirstUbiDevInfo
(
    taf_hms_UbiDevInfoListRef_t ubiDevInfoListRef
)
{
    taf_hms_ubiDevInfoList_t* ubiDevListPtr =
        (taf_hms_ubiDevInfoList_t*)le_ref_Lookup(UbiDevListRefMap, ubiDevInfoListRef);

    TAF_ERROR_IF_RET_VAL(ubiDevListPtr == NULL, NULL, "Failed to retrieve ubi device list.");

    le_sls_Link_t* ubiDevLinkPtr = le_sls_Peek(&(ubiDevListPtr->ubiDevInfoList));
    if (ubiDevLinkPtr != NULL)
    {
        taf_hms_ubiDevInfo_t* ubiDevPtr = CONTAINER_OF(ubiDevLinkPtr, taf_hms_ubiDevInfo_t, link);
        ubiDevListPtr->currPtr = ubiDevLinkPtr;
        return ubiDevPtr->ref;
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next UBI device Info object reference in the list of the
 * UBIInfoList retrieved with taf_hms_GetUBIInfoList().
 *
 * @return
 *  - NULL                          No information found.
 *  - taf_hms_ubiDevInfoListRef      The UBI device Info object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_UbiDevInfoRef_t taf_Hms::GetNextUbiDevInfo
(
    taf_hms_UbiDevInfoListRef_t ubiDevInfoListRef
)
{
    taf_hms_ubiDevInfoList_t* ubiDevListPtr =
        (taf_hms_ubiDevInfoList_t*)le_ref_Lookup(UbiDevListRefMap, ubiDevInfoListRef);

    TAF_ERROR_IF_RET_VAL(ubiDevListPtr == NULL, NULL, "Failed to retrieve next ubi device list.");

    le_sls_Link_t* ubiDevLinkPtr =
        le_sls_PeekNext(&(ubiDevListPtr->ubiDevInfoList), ubiDevListPtr->currPtr);
    if (ubiDevLinkPtr != NULL)
    {
        taf_hms_ubiDevInfo_t* ubiDevPtr = CONTAINER_OF(ubiDevLinkPtr, taf_hms_ubiDevInfo_t, link);
        ubiDevListPtr->currPtr = ubiDevLinkPtr;
        return ubiDevPtr->ref;
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 ** Gets UBI device ID
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::GetUbiDevId
(
    taf_hms_UbiDevInfoRef_t ubiDevInfoRef,
    uint32_t* ubiDevIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(ubiDevIdPtr == NULL, LE_BAD_PARAMETER, "ubiDevIdPtr pointer is NULL");
    taf_hms_ubiDevInfo_t* ubiDevPtr = (taf_hms_ubiDevInfo_t*)le_ref_Lookup(UbiDevRefMap,
        ubiDevInfoRef);

    TAF_ERROR_IF_RET_VAL(ubiDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
        ubiDevPtr);

    *ubiDevIdPtr = ubiDevPtr->devId;
    TAF_ERROR_IF_RET_VAL(*ubiDevIdPtr < 0, LE_FAULT, "Failed to return erase count.");
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 ** Get UBI information for erase count from " /sys/class/ubi/ubi%d/ ".
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::GetUbiDevMaxEraseCnt
(
    taf_hms_UbiDevInfoRef_t ubiDevInfoRef,
    uint32_t* ubiEraseCntPtr
)
{
    TAF_ERROR_IF_RET_VAL(ubiEraseCntPtr == NULL, LE_BAD_PARAMETER, "ubiEraseCntPtr pointer is NULL");
    taf_hms_ubiDevInfo_t* ubiDevPtr = (taf_hms_ubiDevInfo_t*)le_ref_Lookup(UbiDevRefMap,
        ubiDevInfoRef);

    TAF_ERROR_IF_RET_VAL(ubiDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
        ubiDevPtr);

    *ubiEraseCntPtr = ubiDevPtr->eraseCnt;
    TAF_ERROR_IF_RET_VAL(*ubiEraseCntPtr < 0, LE_FAULT, "Failed to return erase count.");
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 ** Get UBI information for bad block count from " /sys/class/ubi/ubi%d/ ".
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::GetUbiDevBadBlkCnt
(
    taf_hms_UbiDevInfoRef_t ubiDevInfoRef,
    uint32_t* ubiBbCntPtr
)
{
    TAF_ERROR_IF_RET_VAL(ubiBbCntPtr == NULL, LE_BAD_PARAMETER, "ubiBbCntPtr pointer is NULL");
    taf_hms_ubiDevInfo_t* ubiDevPtr = (taf_hms_ubiDevInfo_t*)le_ref_Lookup(UbiDevRefMap,
        ubiDevInfoRef);

    TAF_ERROR_IF_RET_VAL(ubiDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
        ubiDevPtr);

    *ubiBbCntPtr = ubiDevPtr->badBlockCnt;
    TAF_ERROR_IF_RET_VAL(*ubiBbCntPtr < 0, LE_FAULT, "Failed to return bad block count.");
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the first UBI volume Info object reference in the list of the
 * ubiVolInfoList retrieved with taf_hms_GetUbiVolInfoList().
 *
 * @return
 *  - NULL                          No information found.
 *  - taf_hms_ubiVolInfoListRef      The UBI device Info object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_UbiVolInfoRef_t taf_Hms::GetFirstUbiVolInfo
(
     taf_hms_UbiDevInfoRef_t ubiDevInfoRef
)
{
    taf_hms_ubiVolInfoList_t* ubiVolListPtr =
        (taf_hms_ubiVolInfoList_t*)le_ref_Lookup(UbiVolListRefMap, ubiDevInfoRef);

    TAF_ERROR_IF_RET_VAL(ubiVolListPtr == NULL, NULL, "Failed to retrieve ubi device list.");

    le_sls_Link_t* ubiVolLinkPtr = le_sls_Peek(&(ubiVolListPtr->ubiVolInfoList));
    if (ubiVolLinkPtr != NULL)
    {
        taf_hms_ubiVolInfo_t* ubiVolPtr = CONTAINER_OF(ubiVolLinkPtr, taf_hms_ubiVolInfo_t, link);
        ubiVolListPtr->currPtr = ubiVolLinkPtr;
        return ubiVolPtr->ref;
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next UBI volume Info object reference in the list of the
 * ubiVolInfoList retrieved with taf_hms_GetUbiVolInfoList().
 *
 * @return
 *  - NULL                          No information found.
 *  - taf_hms_ubiVolInfoListRef      The UBI volume Info object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_UbiVolInfoRef_t taf_Hms::GetNextUbiVolInfo
(
    taf_hms_UbiDevInfoRef_t ubiDevInfoRef
)
{
    taf_hms_ubiVolInfoList_t* ubiVolListPtr =
        (taf_hms_ubiVolInfoList_t*)le_ref_Lookup(UbiVolListRefMap, ubiDevInfoRef);

    TAF_ERROR_IF_RET_VAL(ubiVolListPtr == NULL, NULL, "Failed to retrieve next ubi device list.");

    le_sls_Link_t* ubiVolLinkPtr =
        le_sls_PeekNext(&(ubiVolListPtr->ubiVolInfoList), ubiVolListPtr->currPtr);
    if (ubiVolLinkPtr != NULL)
    {
        taf_hms_ubiVolInfo_t* ubiVolPtr = CONTAINER_OF(ubiVolLinkPtr, taf_hms_ubiVolInfo_t, link);
        ubiVolListPtr->currPtr = ubiVolLinkPtr;
        return ubiVolPtr->ref;
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 ** Gets UBI volume ID.
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::GetUbiVolId
(
    taf_hms_UbiVolInfoRef_t ubiVolInfoRef,
    uint32_t* ubiVolIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(ubiVolIdPtr == NULL, LE_BAD_PARAMETER, "ubiVolIdPtr pointer is NULL");
    taf_hms_ubiVolInfo_t* ubiVolPtr =
        (taf_hms_ubiVolInfo_t*)le_ref_Lookup(UbiVolRefMap, ubiVolInfoRef);

    TAF_ERROR_IF_RET_VAL(ubiVolPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            ubiVolPtr);

    *ubiVolIdPtr = ubiVolPtr->ubiVolId;
    TAF_ERROR_IF_RET_VAL(*ubiVolIdPtr < 0, LE_FAULT, "Failed to return Ubi volume size.");
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 ** Get name of UBI volume.
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::GetUbiVolName
(
    taf_hms_UbiVolInfoRef_t ubiVolInfoRef,
    char* ubiVolName,
    size_t ubiVolNameSize
)
{
    TAF_ERROR_IF_RET_VAL(ubiVolName == NULL, LE_BAD_PARAMETER, "ubiVolName pointer is NULL");
    taf_hms_ubiVolInfo_t* ubiVolPtr =
        (taf_hms_ubiVolInfo_t*)le_ref_Lookup(UbiVolRefMap, ubiVolInfoRef);
    TAF_ERROR_IF_RET_VAL(ubiVolPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
        ubiVolPtr);

    snprintf(ubiVolName, sizeof(ubiVolPtr->ubiVolumeName), "%s", ubiVolPtr->ubiVolumeName);

    TAF_ERROR_IF_RET_VAL(ubiVolName == NULL, LE_FAULT, "Failed to return Ubi volume name.");
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 ** Get size of UBI volume.
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::GetUbiVolSize
(
    taf_hms_UbiVolInfoRef_t ubiVolInfoRef,
    uint32_t* ubiVolSizePtr
)
{
    TAF_ERROR_IF_RET_VAL(ubiVolSizePtr == NULL, LE_BAD_PARAMETER, "ubiVolSizePtr pointer is NULL");
    taf_hms_ubiVolInfo_t* ubiVolPtr =
        (taf_hms_ubiVolInfo_t*)le_ref_Lookup(UbiVolRefMap, ubiVolInfoRef);

    TAF_ERROR_IF_RET_VAL(ubiVolPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
        ubiVolPtr);

    *ubiVolSizePtr = ubiVolPtr->ubiVolSize;
    TAF_ERROR_IF_RET_VAL(*ubiVolSizePtr < 0, LE_FAULT, "Failed to return Ubi volume size.");
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get MTD count from the sys class path.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t GetMtdCount()
{
    DIR* dir;
    struct dirent* entry;
    int mtd_count = 0;

    dir = opendir(MTD_CLASS_PATH);
    if (dir == NULL) {
        LE_ERROR("opendir");
        return LE_FAULT;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }
        // Check if entry->d_name starts with "mtd" (case-insensitive)
        if (strncasecmp(entry->d_name, "mtd", 3) == 0)
        {
            mtd_count++;
        }
    }

    closedir(dir);
    mtd_count = mtd_count / 2;
    LE_INFO("MTD device count: %d", mtd_count);
    return mtd_count;
}

//--------------------------------------------------------------------------------------------------
/**
 ** Gets the list of available MTD Node.
 **
 ** @return
 *  - NULL                            No information found.
 *  - taf_hms_mtdDevInfoListRef    The mtdInfo list object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_MtdDevInfoListRef_t taf_Hms::GetMtdDevInfoList
(
    void
)
{
    taf_hms_mtdInfoList_t* mtdDevList = (taf_hms_mtdInfoList_t*)le_mem_ForceAlloc(MtdListPool);
    memset(mtdDevList, 0, sizeof(taf_hms_mtdInfoList_t));
    mtdDevList->mtdInfoList = LE_SLS_LIST_INIT;
    mtdDevList->currPtr = NULL;

    taf_hms_mtdInfo_t* mtdInfoPtr;

    uint32_t count = GetMtdCount();
    char path[MAX_PATH_LENGTH];
    char buffer[BUFFER_SIZE];
    if (count >= 0)
    {
        for(uint32_t i = 0; i < count; i++)
        {
            mtdInfoPtr = (taf_hms_mtdInfo_t*)le_mem_ForceAlloc(MtdInfoPool);
            memset(mtdInfoPtr, 0, sizeof(taf_hms_mtdInfo_t));
            mtdInfoPtr->mtdBlockCnt = count;

            // Get mtd block size
            snprintf(path, sizeof(path), MTD_DEV_SIZE_PATH, i);
            mtdInfoPtr->mtdBlockSize = ReadSysfsFile(path);

            // Get device name
            snprintf(path, sizeof(path), MTD_DEV_NAME_PATH, i);
            uint8_t result = ReadSysfsStringFile(path, buffer, sizeof(buffer));
            if (result == LE_OK)
            {
                le_utf8_Copy(mtdInfoPtr->mtdDevName, buffer, BUFFER_SIZE, NULL);
            }
            else
            {
                LE_INFO("Failed ! to read data from mtd path");
            }

           mtdInfoPtr->link = LE_SLS_LINK_INIT;
           le_sls_Queue(&(mtdDevList->mtdInfoList), &(mtdInfoPtr->link));
           mtdInfoPtr->ref =
                (taf_hms_MtdDevInfoRef_t)le_ref_CreateRef(MtdRefMap, (void*)mtdInfoPtr);
        }
        mtdDevList->ref =
            (taf_hms_MtdDevInfoListRef_t)le_ref_CreateRef(MtdListRefMap, mtdDevList);
        return mtdDevList->ref;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 ** Deletes the MtdInfoList list retrieved with taf_hms_GetMtdInfoList().
 **
 ** @return
 **  - LE_BAD_PARAMETER -- Bad parameters.
 **  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::DeleteMtdDevInfoList
(
    taf_hms_MtdDevInfoListRef_t mtdDevInfoListRef
)
{
    taf_hms_mtdInfoList_t* listPtr =
        (taf_hms_mtdInfoList_t*)le_ref_Lookup(MtdListRefMap, mtdDevInfoListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");
    LE_DEBUG("DeleteMTDInfoList : %p", mtdDevInfoListRef);
    taf_hms_mtdInfo_t* mtdInfoPtr;
    le_sls_Link_t* linkPtr;
    while ((linkPtr = le_sls_Pop(&(listPtr->mtdInfoList))) != NULL)
    {
        mtdInfoPtr = CONTAINER_OF(linkPtr, taf_hms_mtdInfo_t, link);
        le_mem_Release(mtdInfoPtr);
    }
    le_ref_DeleteRef(MtdListRefMap, mtdDevInfoListRef);
    le_mem_Release(listPtr);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the first mtdInfo object reference in the list of the
 * mtdInfoList retrieved with taf_hms_GetMtdInfoList().
 *
 * @return
 *  - NULL                          No information found.
 *  - taf_hms_mtdDevInfoListRef      The mtdInfo object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_MtdDevInfoRef_t taf_Hms::GetFirstMtdDevInfo
(
    taf_hms_MtdDevInfoListRef_t mtdDevInfoListRef
)
{
    taf_hms_mtdInfoList_t* mtdListPtr =
        (taf_hms_mtdInfoList_t*)le_ref_Lookup(MtdListRefMap, mtdDevInfoListRef);

    TAF_ERROR_IF_RET_VAL(mtdListPtr == NULL, NULL, "Failed to retrieve ubi device list.");

    le_sls_Link_t* mtdLinkPtr = le_sls_Peek(&(mtdListPtr->mtdInfoList));
    if (mtdLinkPtr != NULL)
    {
        taf_hms_mtdInfo_t* mtdDevPtr = CONTAINER_OF(mtdLinkPtr, taf_hms_mtdInfo_t, link);
        mtdListPtr->currPtr = mtdLinkPtr;
        return mtdDevPtr->ref;
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next mtdInfo object reference in the list of the
 * mtdInfoList retrieved with taf_hms_GetMtdInfoList().
 *
 * @return
 *  - NULL                          No information found.
 *  - taf_hms_mtdDevInfoListRef      The mtdInfo object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_MtdDevInfoRef_t taf_Hms::GetNextMtdDevInfo
(
    taf_hms_MtdDevInfoListRef_t mtdDevInfoListRef
)
{
    taf_hms_mtdInfoList_t* mtdListPtr =
        (taf_hms_mtdInfoList_t*)le_ref_Lookup(MtdListRefMap, mtdDevInfoListRef);

    TAF_ERROR_IF_RET_VAL(mtdListPtr == NULL, NULL, "Failed to retrieve next ubi device list.");

    le_sls_Link_t* mtdLinkPtr =
        le_sls_PeekNext(&(mtdListPtr->mtdInfoList), mtdListPtr->currPtr);
    if (mtdLinkPtr != NULL)
    {
        taf_hms_mtdInfo_t* mtdDevPtr = CONTAINER_OF(mtdLinkPtr, taf_hms_mtdInfo_t, link);
        mtdListPtr->currPtr = mtdLinkPtr;
        return mtdDevPtr->ref;
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 ** Get MTD information for name from " /sys/class/mtd/mtd%d/ ".
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::GetMtdDevName
(
    taf_hms_MtdDevInfoRef_t mtdDevInfoRef,
    char* mtdName,
    size_t mtdNameSize
)
{
    TAF_ERROR_IF_RET_VAL(mtdName == NULL, LE_BAD_PARAMETER, "mtdName pointer is NULL");
    taf_hms_mtdInfo_t* mtdDevPtr =
        (taf_hms_mtdInfo_t*)le_ref_Lookup(MtdRefMap, mtdDevInfoRef);
    TAF_ERROR_IF_RET_VAL(mtdDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            mtdDevPtr);

    snprintf(mtdName, sizeof(mtdDevPtr->mtdDevName), "%s", mtdDevPtr->mtdDevName);

    TAF_ERROR_IF_RET_VAL(mtdName == NULL, LE_FAULT, "Failed to return MTD device name.");
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 ** Get MTD information for block size.
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::GetMtdDevBlkSize
(
    taf_hms_MtdDevInfoRef_t mtdDevInfoRef,
    uint32_t* mtdBlkSizePtr
)
{
    TAF_ERROR_IF_RET_VAL(mtdBlkSizePtr == NULL, LE_BAD_PARAMETER, "mtdBlkSizePtr pointer is NULL");
    taf_hms_mtdInfo_t* mtdDevPtr =
        (taf_hms_mtdInfo_t*)le_ref_Lookup(MtdRefMap, mtdDevInfoRef);

    TAF_ERROR_IF_RET_VAL(mtdDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            mtdDevPtr);

    *mtdBlkSizePtr = mtdDevPtr->mtdBlockSize;
    TAF_ERROR_IF_RET_VAL(*mtdBlkSizePtr < 0, LE_FAULT, "Failed to return MTD device size.");
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 ** Gets MTD information for device ID.
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::GetMtdDevId
(
    taf_hms_MtdDevInfoRef_t mtdDevInfoRef,
    uint32_t* mtdDevIdPtr
)
{
   TAF_ERROR_IF_RET_VAL(mtdDevIdPtr == NULL, LE_BAD_PARAMETER, "mtdDevIdPtr pointer is NULL");
   taf_hms_mtdInfo_t* mtdDevPtr =
        (taf_hms_mtdInfo_t*)le_ref_Lookup(MtdRefMap, mtdDevInfoRef);

    TAF_ERROR_IF_RET_VAL(mtdDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            mtdDevPtr);

    *mtdDevIdPtr = mtdDevPtr->mtdDevId;
    TAF_ERROR_IF_RET_VAL(*mtdDevIdPtr < 0, LE_FAULT, "Failed to return MTD device ID.");
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 ** Gets MTD information for block count.
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::GetMtdDevBlkCnt
(
    taf_hms_MtdDevInfoRef_t mtdDevInfoRef,
    uint32_t* mtdBlkCntPtr
)
{
    TAF_ERROR_IF_RET_VAL(mtdBlkCntPtr == NULL, LE_BAD_PARAMETER, "mtdBlkCntPtr pointer is NULL");
    taf_hms_mtdInfo_t* mtdDevPtr =
        (taf_hms_mtdInfo_t*)le_ref_Lookup(MtdRefMap, mtdDevInfoRef);

    TAF_ERROR_IF_RET_VAL(mtdDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            mtdDevPtr);

    *mtdBlkCntPtr = mtdDevPtr->mtdBlockCnt;
    TAF_ERROR_IF_RET_VAL(*mtdBlkCntPtr < 0, LE_FAULT, "Failed to return MTD Block size.");
    return LE_OK;
}

void taf_Hms::Init()
{
    LE_INFO("tafHMSvc started");

    UbiDevListPool = le_mem_InitStaticPool(UbiDevListPool, TAF_HMS_MAX_LIST_POOL_SIZE,
        sizeof(taf_hms_ubiDevInfoList_t));
    UbiDevInfoPool = le_mem_InitStaticPool(UbiDevInfoPool, TAF_HMS_MAX_LIST_POOL_SIZE,
        sizeof(taf_hms_ubiDevInfo_t));
    UbiVolListPool = le_mem_InitStaticPool(UbiVolListPool, TAF_HMS_MAX_LIST_POOL_SIZE,
        sizeof(taf_hms_ubiVolInfoList_t));
    UbiVolInfoPool = le_mem_InitStaticPool(UbiVolInfoPool, TAF_HMS_MAX_LIST_POOL_SIZE,
        sizeof(taf_hms_ubiVolInfo_t));
    MtdListPool = le_mem_InitStaticPool(MtdListPool, TAF_HMS_MAX_LIST_POOL_SIZE,
        sizeof(taf_hms_mtdInfoList_t));
    MtdInfoPool = le_mem_InitStaticPool(MtdInfoPool, TAF_HMS_MAX_LIST_POOL_SIZE,
        sizeof(taf_hms_mtdInfo_t));

    UbiDevListRefMap = le_ref_InitStaticMap(UbiDevListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    UbiDevRefMap = le_ref_InitStaticMap(UbiDevRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    UbiVolListRefMap = le_ref_InitStaticMap(UbiVolListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    UbiVolRefMap = le_ref_InitStaticMap(UbiVolRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    MtdListRefMap = le_ref_InitStaticMap(MtdListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    MtdRefMap = le_ref_InitStaticMap(MtdRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
}
