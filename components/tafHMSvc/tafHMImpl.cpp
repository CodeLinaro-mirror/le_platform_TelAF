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
#include <dirent.h>
#include <future>

#include <unordered_map>
#include <algorithm>

using namespace std;
using namespace tafsvc;

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
LE_MEM_DEFINE_STATIC_POOL(ModemEventInfoPool, TAF_HMS_MAX_EVENT_POOL_SIZE,
    sizeof(taf_hms_modemEventInfo_t));
LE_MEM_DEFINE_STATIC_POOL(ModemInfoPool, TAF_HMS_MAX_REF_POOL_SIZE,
    sizeof(taf_hms_modemInfo_t));


LE_REF_DEFINE_STATIC_MAP(UbiDevListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(UbiDevRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(UbiVolListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(UbiVolRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(MtdListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(MtdRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(ModemEventInfoRefMap, TAF_HMS_MAX_EVENT_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(ModemInfoRefMap, TAF_HMS_MAX_REF_POOL_SIZE);

std::atomic<bool> tafHmsListener::ModemAvailability{true};
static std::atomic<int> pendingCount{0};

// One-shot gate to print logs when connection between modem and APPs from LOST to ONLINE again.
static bool oneTimeLog = true;

extern std::shared_ptr<ModemStatus> modemStatus;

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

void taf_Hms::AdvertiseService()
{
    taf_hms_AdvertiseService();
    le_msg_AddServiceCloseHandler(taf_hms_GetServiceRef(), OnClientDisconnection, NULL);
    LE_INFO("taf_hms service advertised");
}

void taf_Hms::OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *contextPtr)
{
    auto &hms = taf_Hms::GetInstance();
    LE_INFO("HMS client session closed: %p", sessionRef);
    LE_UNUSED(contextPtr);

    std::vector<taf_hms_UbiDevInfoListRef_t> UbiRefToRemove;
    le_ref_IterRef_t ubiIterRef = le_ref_GetIterator(hms.UbiDevListRefMap);
    while (le_ref_NextNode(ubiIterRef) == LE_OK)
    {
        taf_hms_ubiDevInfoList_t* ubiListPtr =
            (taf_hms_ubiDevInfoList_t*)le_ref_GetValue(ubiIterRef);

        if ((ubiListPtr != nullptr) && (ubiListPtr->sessionRef == sessionRef))
        {
            LE_DEBUG("Deleting UBI device list ref %p for closed session %p",
                ubiListPtr->ref, sessionRef);
            UbiRefToRemove.push_back(ubiListPtr->ref);
        }
    }
    for (taf_hms_UbiDevInfoListRef_t ubiRef : UbiRefToRemove)
    {
        hms.DeleteUbiDevInfoList(ubiRef);
    }

    std::vector<taf_hms_MtdDevInfoListRef_t> mtdRefToRemove;
    le_ref_IterRef_t mtdIterRef = le_ref_GetIterator(hms.MtdListRefMap);
    while (le_ref_NextNode(mtdIterRef) == LE_OK)
    {
        taf_hms_mtdInfoList_t* mtdListPtr =
            (taf_hms_mtdInfoList_t*)le_ref_GetValue(mtdIterRef);

        if ((mtdListPtr != nullptr) && (mtdListPtr->sessionRef == sessionRef))
        {
            LE_DEBUG("Deleting MTD device list ref %p for closed session %p",
                mtdListPtr->ref, sessionRef);
            mtdRefToRemove.push_back(mtdListPtr->ref);
        }
    }
    for (taf_hms_MtdDevInfoListRef_t mtdRef : mtdRefToRemove)
    {
        hms.DeleteMtdDevInfoList(mtdRef);
    }

    std::vector<taf_hms_ModemEvtHandlerRef_t> ModemInfoRefToRemove;
    le_ref_IterRef_t infoIterRef = le_ref_GetIterator(hms.ModemInfoRefMap);
    while (le_ref_NextNode(infoIterRef) == LE_OK)
    {
        taf_hms_modemInfo_t* infoPtr =
            (taf_hms_modemInfo_t*)le_ref_GetValue(infoIterRef);

        if ((infoPtr != nullptr) && (infoPtr->sessionRef == sessionRef))
        {
            LE_DEBUG("Removing modem handler ref %p for closed session %p",
                infoPtr->handlerRef, sessionRef);
            ModemInfoRefToRemove.push_back(infoPtr->handlerRef);
        }
    }
    for (taf_hms_ModemEvtHandlerRef_t handlerRef : ModemInfoRefToRemove)
    {
        hms.RemoveModemEvtHandler(handlerRef);
    }
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

    if (fscanf(file, "cpu %d %d %d %d %d %d %d %d %d %d",
        &usage.user, &usage.nice, &usage.system, &usage.idle, &usage.iowait,
        &usage.irq, &usage.softirq, &usage.steal, &usage.guest, &usage.guest_nice) <= 0)
    {
        LE_WARN("Failed to scan data from file");
    }

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

    if (total_diff <= 0.0f)
    {
        return 0.0;
    }

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
    le_timer_SetWakeup(HmsTimerRef, false);
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

    fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL)
    {
        LE_ERROR("Error opening /proc/cpuinfo");
        return 0;
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
    if (total_time == 0)
    {
        return 0.0;
    }
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
    TAF_ERROR_IF_RET_VAL(coreID >= MAX_CORES, LE_BAD_PARAMETER,
                         "coreID %u out of range (max %d)", coreID, MAX_CORES - 1);
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
        LE_ERROR("ReadSysfsFile: path is NULL");
        return 0;
    }
    fp = fopen(path, "r");
    if (NULL == fp)
    {
        LE_ERROR("ReadSysfsFile: failed to open %s", path);
        return 0;
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
static le_result_t ReadSysfsStringFile(const char *path, char *buffer, size_t length)
{
    if (path == NULL || buffer == NULL || length == 0)
    {
        LE_ERROR("ReadSysfsStringFile: invalid parameter");
        return LE_BAD_PARAMETER;
    }

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
        return 0;
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
    ubiDevList->sessionRef = taf_hms_GetClientSessionRef();

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
        if (ubiDevInfoPtr->ref != NULL)
        {
            le_ref_DeleteRef(UbiDevRefMap, ubiDevInfoPtr->ref);
            ubiDevInfoPtr->ref = NULL;
        }
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
    TAF_ERROR_IF_RET_VAL(ubiVolName == NULL || ubiVolNameSize == 0,
                         LE_BAD_PARAMETER, "Invalid UBI volume name buffer");
    taf_hms_ubiVolInfo_t* ubiVolPtr =
        (taf_hms_ubiVolInfo_t*)le_ref_Lookup(UbiVolRefMap, ubiVolInfoRef);
    TAF_ERROR_IF_RET_VAL(ubiVolPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
        ubiVolPtr);

    snprintf(ubiVolName, ubiVolNameSize, "%s", ubiVolPtr->ubiVolumeName);
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
        return 0;
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
    mtdDevList->sessionRef = taf_hms_GetClientSessionRef();

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
            le_result_t result = ReadSysfsStringFile(path, buffer, sizeof(buffer));
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
        if (mtdInfoPtr->ref != NULL)
        {
            le_ref_DeleteRef(MtdRefMap, mtdInfoPtr->ref);
            mtdInfoPtr->ref = NULL;
        }
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
    TAF_ERROR_IF_RET_VAL(mtdName == NULL || mtdNameSize == 0,
                         LE_BAD_PARAMETER, "Invalid MTD name buffer");
    taf_hms_mtdInfo_t* mtdDevPtr =
        (taf_hms_mtdInfo_t*)le_ref_Lookup(MtdRefMap, mtdDevInfoRef);
    TAF_ERROR_IF_RET_VAL(mtdDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            mtdDevPtr);

    snprintf(mtdName, mtdNameSize, "%s", mtdDevPtr->mtdDevName);

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

//--------------------------------------------------------------------------------------------------
/**
 ** Convert modem event type to a string.
 */
//--------------------------------------------------------------------------------------------------
const char* ModemEventTypeToStr
(
    taf_hms_ModemEvtType_t eventType
)
{
    switch (eventType)
    {
        case TAF_HMS_MODEM_EVENT_TYPE_CONNECTION_LOST:
            return "CONNECTION_LOST";

        case TAF_HMS_MODEM_EVENT_TYPE_CONTINUE_REBOOT:
            return "CONTINUE_REBOOT";

        case TAF_HMS_MODEM_EVENT_TYPE_CONNECTION_AVAIL:
            return "CONNECTION_AVAIL";
    }
    return "UNKNOWN";
}

taf_hms_ModemEvtBitmask_t ModemEvtTypeMatchToBitmask
(
    taf_hms_ModemEvtType_t type
)
{
    switch (type)
    {
        case TAF_HMS_MODEM_EVENT_TYPE_CONTINUE_REBOOT:
            return TAF_HMS_MODEM_EVENT_BIT_CONTINUE_REBOOT;

        case TAF_HMS_MODEM_EVENT_TYPE_CONNECTION_LOST:
            return TAF_HMS_MODEM_EVENT_BIT_CONNECTION_LOST;

        case TAF_HMS_MODEM_EVENT_TYPE_CONNECTION_AVAIL:
            // Clients register CONNECTION_LOST bit to receive both LOST and AVAIL events
            return TAF_HMS_MODEM_EVENT_BIT_CONNECTION_LOST;

        default:
            LE_FATAL("Fix me, no matched bitmask for type %d", (int)type);
    }
}

void ResetModemStatusCounterHandler(void)
{
    auto &hms_List = tafHmsListener::GetInstance();

    hms_List.ModemCrashCounter = 0;
    LE_DEBUG("Reseting the timer for modem crash monitor");
}

static void DoRequestOperatingModeTrampoline(void*, void*)
{
    if (modemStatus)
    {
        modemStatus->DoRequestOperatingMode();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Periodic main-thread timer handler driving the modem-health state machine.
 *
 * Runs every MODEM_CHECK_STATUS_INTERVAL ms on the main thread (the timer is
 * created in ModemStatus::StartWorkerThread and fires on the thread that
 * created it). Two independent jobs are performed:
 *
 *   1. CONTINUE_REBOOT clients: every TAF_HMS_MODEM_RESET_TIMER ms the modem
 *      crash counter is reset so old crashes don't keep escalating the
 *      severity reported to clients.
 *   2. CONNECTION_LOST clients: requestOperatingMode() is queued onto the
 *      worker thread each tick. If no SUCCESS response comes back for
 *      COUNTER_RESPONSE_TIME_OUT consecutive ticks the modem is declared
 *      unresponsive, a CONNECTION_LOST event is dispatched, and the
 *      one-shot ONLINE-log gate is re-armed for the next recovery.
 */
//--------------------------------------------------------------------------------------------------
void ModemStatusTimerHandler(le_timer_Ref_t timerRef)
{
    auto &hms_List = tafHmsListener::GetInstance();

    if (hms_List.counter >= INT_MAX)
    {
        hms_List.counter = 0;
    }
    hms_List.counter++;

    // For registered event: CONTINUE_REBOOT.
    // Reset the modem crash counter every TAF_HMS_MODEM_RESET_TIMER ms so
    // the severity reported to clients reflects only recent crashes.
    if (hms_List.AllModemEventMap & TAF_HMS_MODEM_EVENT_BIT_CONTINUE_REBOOT)
    {
        int resetLoop = (TAF_HMS_MODEM_RESET_TIMER / MODEM_CHECK_STATUS_INTERVAL) > 2 ?
            (TAF_HMS_MODEM_RESET_TIMER / MODEM_CHECK_STATUS_INTERVAL) : 2;

        //Reset monitoring duration for modem reboot in every 120 seconds.
        if (hms_List.counter % resetLoop == 0)
        {
            ResetModemStatusCounterHandler();
        }
    }

    // For registered event: CONNECTION_LOST.
    // Probe the modem each tick and detect unresponsiveness via pendingCount.
    if (hms_List.AllModemEventMap & TAF_HMS_MODEM_EVENT_BIT_CONNECTION_LOST)
    {
        TAF_ERROR_IF_RET_NIL(modemStatus == nullptr, "modemStatus is null");
        if (pendingCount.load() < COUNTER_EVENT_REPORT_DONE)
        {
            pendingCount++;
        }

        // No SUCCESS response for COUNTER_RESPONSE_TIME_OUT consecutive
        // ticks: declare the modem unresponsive and notify clients once.
        if (pendingCount.load() == COUNTER_RESPONSE_TIME_OUT)
        {
            LE_WARN("No response %d ms elapsed", MODEM_CHECK_STATUS_INTERVAL * pendingCount.load());

            pendingCount.store(COUNTER_EVENT_REPORT_DONE);
            modemStatus->ReportModemStatus(TAF_HMS_MODEM_EVENT_TYPE_CONNECTION_LOST,
                                           TAF_HMS_MODEM_EVENT_SEVERITY_HIGH);

            // Connection lost enable one-shot log
            oneTimeLog = true;
        }

        // Queue the next operating-mode request on the worker thread.
        // The worker may block in PhoneInit, so the DoRequest must not run on the
        // main thread.
        if (modemStatus->GetWorkerThread() != nullptr)
        {
            le_event_QueueFunctionToThread(modemStatus->GetWorkerThread(),
                                           DoRequestOperatingModeTrampoline, nullptr, nullptr);
        }
    }

}

void tafHmsListener::StartResetTimer(void)
{
    TAF_ERROR_IF_RET_NIL(modemStatus == nullptr, "modemStatus is null");
    modemStatus->StartWorkerThread();
}

taf_hms_operationStatus_t ModemOnStatusChangeConvert
(
    telux::common::OperationalStatus operationalStatus
)
{
    switch(operationalStatus)
    {
        case telux::common::OperationalStatus::UNAVAILABLE:
            return MODEM_ON_CHANGE_STATUS_UNAVAILABLE;

        case telux::common::OperationalStatus::OPERATIONAL:
            return MODEM_ON_CHANGE_STATUS_OPERATIONAL;

        default:
            return MODEM_ON_CHANGE_STATUS_UNKNOWN;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * telux ISubsystemListener callback.
 *
 * Invoked by the telux SDK on its own thread whenever the operational status
 * of a registered subsystem (MPSS in our case) changes. We can not safely
 * touch tafHMSvc state from the SDK thread, so the new status is marshalled
 * to the main thread via le_event_Report; the actual handling happens in
 * MdStatusOnChangeCBNotify().
 *
 * @param[in] subsystemInfo          Identifies the subsystem (unused; we only
 *                                   register for MPSS).
 * @param[in] newOperationalStatus   New operational status reported by SDK.
 */
//--------------------------------------------------------------------------------------------------
void tafHmsListener::onStateChange(telux::common::SubsystemInfo subsystemInfo,
                telux::common::OperationalStatus newOperationalStatus)
{
    auto &hms = taf_Hms::GetInstance();
    taf_hms_modemOperaInfo_t onChEventType;

    onChEventType.operationalStatus = static_cast<int>(newOperationalStatus);
    le_event_Report(hms.MdStatusOnChangeCBId, &onChEventType, sizeof(taf_hms_modemOperaInfo_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Main-thread handler for modem operational-status changes.
 *
 */
//--------------------------------------------------------------------------------------------------
void MdStatusOnChangeCBNotify(void* reportPtr)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("MdStatusOnChangeCBNotify: null reportPtr");
        return;
    }

    auto &hms = taf_Hms::GetInstance();
    auto &hms_List = tafHmsListener::GetInstance();

    taf_hms_modemOperaInfo_t *onChEventType = (taf_hms_modemOperaInfo_t*)reportPtr;
    telux::common::OperationalStatus newOperationalStatus =
        static_cast<telux::common::OperationalStatus>(onChEventType->operationalStatus);
    taf_hms_operationStatus_t status = ModemOnStatusChangeConvert(newOperationalStatus);

    LE_DEBUG("Got status change, operationalStatus %d, crash counter %d",
            (int)newOperationalStatus, hms_List.ModemCrashCounter);

    //Check if the modem has crashed
    if(newOperationalStatus == telux::common::OperationalStatus::UNAVAILABLE)
    {
        tafHmsListener::ModemAvailability.store(false);
        LE_ERROR("Modem status became UNAVAILABLE");
        return;
    }

    //Check if it is a reboot i.e. UNAVAILABLE --> OPERATIONAL
    if(newOperationalStatus == telux::common::OperationalStatus::OPERATIONAL &&
        tafHmsListener::ModemAvailability.load())
    {
        LE_DEBUG("Modem status is OPERATIONAL");
        return;
    }

    // UNAVAILABLE -> OPERATIONAL transition (modem reboot).
    tafHmsListener::ModemAvailability.store(true);
    if (hms_List.AllModemEventMap & TAF_HMS_MODEM_EVENT_BIT_CONTINUE_REBOOT)
    {
        hms_List.ModemCrashCounter++;
    }

    // The mapping above only treats OPERATIONAL as a reboot trigger; any
    // other intermediate state is ignored by this handler.
    if(status != MODEM_ON_CHANGE_STATUS_OPERATIONAL)
    {
        LE_DEBUG("Modem status is NOT OPERATIONAL: %d", status);
        return;
    }

    // traverse throught all the clients which registered for the modem
    // event and set value as per counter
    le_ref_IterRef_t iterRef = le_ref_GetIterator(hms.ModemInfoRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_hms_modemInfo_t* clientInfo  = (taf_hms_modemInfo_t*)le_ref_GetValue(iterRef);
        TAF_ERROR_IF_RET_NIL(clientInfo == nullptr, "No registered client for modem info");

        taf_hms_modemEventInfo_t newEvent;
        newEvent.modemInfo = clientInfo;

        //Check if client has registered for monitoring modem
        if (clientInfo->handlerFunc == NULL)
        {
            continue;
        }

        //Check if any client has registered for a reboot type
        if(!(TAF_HMS_MODEM_EVENT_BIT_CONTINUE_REBOOT & clientInfo->reqEventBits))
        {
            continue;
        }

        if (hms_List.ModemCrashCounter <= TAF_HMS_MODEM_EVENT_SEVERITY_COUNT_LOW) {
            newEvent.eventLevel = TAF_HMS_MODEM_EVENT_SEVERITY_LOW;
        }
        else if (hms_List.ModemCrashCounter <= TAF_HMS_MODEM_EVENT_SEVERITY_COUNT_MEDIUM) {
            newEvent.eventLevel = TAF_HMS_MODEM_EVENT_SEVERITY_MEDIUM;
        }
        else if (hms_List.ModemCrashCounter >= TAF_HMS_MODEM_EVENT_SEVERITY_COUNT_HIGH) {
            newEvent.eventLevel = TAF_HMS_MODEM_EVENT_SEVERITY_HIGH;
        }

        newEvent.eventType = TAF_HMS_MODEM_EVENT_TYPE_CONTINUE_REBOOT;

        le_event_Report(hms.MdStatusChNotifyCliId, &newEvent, sizeof(taf_hms_modemEventInfo_t));
    }
}

static void DispatchEventToClient(taf_hms_modemEventInfo_t* evt)
{
    TAF_ERROR_IF_RET_NIL(evt == nullptr || evt->modemInfo == nullptr, "invalid event");
    TAF_ERROR_IF_RET_NIL(evt->modemInfo->handlerFunc == NULL, "client handler is NULL");

    auto &hms = taf_Hms::GetInstance();

    // Allocate a persistent event ref that the client will release via ReleaseModemEvt.
    taf_hms_modemEventInfo_t* evtAlloc =
        (taf_hms_modemEventInfo_t*)le_mem_ForceAlloc(hms.ModemEventInfoPool);
    evtAlloc->eventType  = evt->eventType;
    evtAlloc->eventLevel = evt->eventLevel;
    evtAlloc->modemInfo  = evt->modemInfo;
    evtAlloc->ref = (taf_hms_ModemEventRef_t)le_ref_CreateRef(
                        hms.ModemEventInfoRefMap, (void*)evtAlloc);

    if (evtAlloc->ref == NULL)
    {
        LE_ERROR("le_ref_CreateRef failed, dropping event");
        le_mem_Release(evtAlloc);
        return;
    }

    LE_DEBUG("Type %s, Level %d, clientId=%u",
            ModemEventTypeToStr(evtAlloc->eventType),
            (int)evtAlloc->eventLevel,
            evtAlloc->modemInfo->clientId);

    evtAlloc->modemInfo->handlerFunc(evtAlloc->eventType, evtAlloc->eventLevel,
                                     evtAlloc->ref, evtAlloc->modemInfo->contextPtr);
}

void taf_Hms::ModemStatusChNotifyClient(void* reportPtr)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "reportPtr is nullptr");

    taf_hms_modemEventInfo_t* incoming = (taf_hms_modemEventInfo_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(incoming->modemInfo == nullptr, "modemInfo is nullptr");
    TAF_ERROR_IF_RET_NIL(incoming->modemInfo->handlerFunc == NULL, "clientHandlerFunc is NULL");

    if (!(ModemEvtTypeMatchToBitmask(incoming->eventType) & incoming->modemInfo->reqEventBits))
    {
        return;
    }

    DispatchEventToClient(incoming);
}


// Monotonic client ID counter - incremented each time a new handler is registered.
static std::atomic<uint32_t> nextClientId{1};

//--------------------------------------------------------------------------------------------------
/**
 * Registers a client handler for one or more modem events.
 *
 * Allocates a per-client record from ModemInfoPool, stores the requested
 * event bitmask and handler function, assigns a monotonic clientId for
 * logging, and adds the new bits to the global event map. On the first
 * registration the modem-status worker thread and its periodic timer are
 * started via StartResetTimer().
 *
 * @param[in] reqEventBits     Bitmask of taf_hms_ModemEvtBitmask_t the
 *                             client wants to receive. Must be non-zero.
 * @param[in] handlerFuncPtr   Client callback. Must not be NULL.
 * @param[in] contextPtr       Opaque context returned to the client on each
 *                             callback invocation.
 *
 * @return Handler reference passed to RemoveModemEvtHandler(), or NULL on
 *         invalid arguments / allocation failure.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_ModemEvtHandlerRef_t taf_Hms::AddModemEvtHandler
(
    taf_hms_ModemEvtBitmask_t     reqEventBits,
    taf_hms_ModemEvtHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    auto &mppsListener = tafHmsListener::GetInstance();
    TAF_ERROR_IF_RET_VAL(handlerFuncPtr == NULL, NULL, "INVALID handler function pointer.");
    TAF_ERROR_IF_RET_VAL(reqEventBits == 0, NULL, "Event bit is not correct");

    taf_hms_modemInfo_t* newEvt = (taf_hms_modemInfo_t*)le_mem_ForceAlloc(ModemInfoPool);
    newEvt->handlerFunc  = handlerFuncPtr;
    newEvt->contextPtr   = contextPtr;
    newEvt->reqEventBits = reqEventBits;
    newEvt->sessionRef   = taf_hms_GetClientSessionRef();
    newEvt->clientId     = nextClientId++;
    newEvt->handlerRef   = (taf_hms_ModemEvtHandlerRef_t)le_ref_CreateRef(ModemInfoRefMap, newEvt);

    if (newEvt->handlerRef == NULL)
    {
        LE_ERROR("Failed to create handler reference, releasing memory.");
        le_mem_Release(newEvt);
        return NULL;
    }

    //Add new events to event map.
    mppsListener.AllModemEventMap |= reqEventBits;
    mppsListener.StartResetTimer();
    LE_INFO("New register event '0x%x' for reference '%p' clientId=%u sessionRef=%p, "
            "all events: 0x%08" PRIx64 "",
            reqEventBits, newEvt->handlerRef, newEvt->clientId,
            (void*)newEvt->sessionRef, mppsListener.AllModemEventMap);
    return (taf_hms_ModemEvtHandlerRef_t)newEvt->handlerRef;
}

void tafHmsListener::DeleteResetTime(void)
{
    TAF_ERROR_IF_RET_NIL(modemStatus == nullptr, "modemStatus is null");
    modemStatus->StopWorkerThread();
}

bool taf_Hms::IsModemEventMapEmpty(void)
{
    auto &mppsListener = tafHmsListener::GetInstance();
    mppsListener.AllModemEventMap = 0x0; //Clean it first.

    // Traverse throught all the clients which registered for the modem
    // event and reflash the support events.
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ModemInfoRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_hms_modemInfo_t* clientInfo  = (taf_hms_modemInfo_t*)le_ref_GetValue(iterRef);
        if (!clientInfo)
        {
            LE_WARN("No registered client for modem info");
            break;
        }

        if (!clientInfo->handlerFunc)
        {
            continue;
        }

        mppsListener.AllModemEventMap |= clientInfo->reqEventBits;
    }
    LE_DEBUG("Remaining supported all events: 0x%08" PRIx64 "",mppsListener.AllModemEventMap);
    return mppsListener.AllModemEventMap == 0;
}

void taf_Hms::RemoveModemEvtHandler(taf_hms_ModemEvtHandlerRef_t handlerRef)
{
    auto &mppsListener = tafHmsListener::GetInstance();
    TAF_ERROR_IF_RET_NIL(handlerRef == nullptr, "Invalid para(null reference)");

    taf_hms_modemInfo_t* handlerPtr =
        (taf_hms_modemInfo_t*)le_ref_Lookup(ModemInfoRefMap, handlerRef);
    // RemoveModemEvtHandler will be triggered by both OnClientDisconnection and stub code
    // generated by taf_hms_AdvertiseService, which ever is triggered first will delete the
    // handlerPtr.
    if (handlerPtr == nullptr)
    {
        LE_WARN("Invalid para(null reference ptr)");
        return;
    }

    // Release any modem event objects still pointing to this handler info.
    le_ref_IterRef_t eventIterRef = le_ref_GetIterator(ModemEventInfoRefMap);
    while (le_ref_NextNode(eventIterRef) == LE_OK)
    {
        taf_hms_modemEventInfo_t* evtPtr =
            (taf_hms_modemEventInfo_t*)le_ref_GetValue(eventIterRef);

        if ((evtPtr != nullptr) && (evtPtr->modemInfo == handlerPtr))
        {
            LE_DEBUG("Releasing modem event ref %p linked to handler ref %p",
                evtPtr->ref, handlerRef);
            le_ref_DeleteRef(ModemEventInfoRefMap, evtPtr->ref);
            le_mem_Release(evtPtr);
        }
    }

    handlerPtr->handlerFunc = NULL;
    handlerPtr->contextPtr = NULL;
    handlerPtr->reqEventBits = 0;
    handlerPtr->sessionRef = NULL;

    //Remove the 'de-register' handler from ModemInfoRefMap
    le_ref_DeleteRef(ModemInfoRefMap, handlerRef);
    le_mem_Release(handlerPtr);

    // Update all remained modem event maps and delete the timer if no events remain.
    if (IsModemEventMapEmpty())
    {
        mppsListener.DeleteResetTime();
    }
}

le_result_t taf_Hms::ReleaseModemEvt(taf_hms_ModemEventRef_t eventRef)
{
    TAF_ERROR_IF_RET_VAL(eventRef == nullptr, LE_NOT_FOUND, "Invalid para(null reference)");
    taf_hms_modemEventInfo_t* evtPtr =
        (taf_hms_modemEventInfo_t*)le_ref_Lookup(ModemEventInfoRefMap, eventRef);
    TAF_ERROR_IF_RET_VAL(evtPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");
    LE_DEBUG("ReleaseModemEvt : %p", eventRef);
    le_ref_DeleteRef(ModemEventInfoRefMap, eventRef);
    le_mem_Release(evtPtr);
    return LE_OK;
}

static std::string OperatingModeToString(telux::tel::OperatingMode operatingMode)
{
    switch (operatingMode)
    {
        case telux::tel::OperatingMode::ONLINE: return "ONLINE";
        case telux::tel::OperatingMode::AIRPLANE: return "AIRPLANE";
        case telux::tel::OperatingMode::FACTORY_TEST: return "FACTORY_TEST";
        case telux::tel::OperatingMode::OFFLINE: return "OFFLINE";
        case telux::tel::OperatingMode::RESETTING: return "RESETTING";
        case telux::tel::OperatingMode::SHUTTING_DOWN: return "SHUTTING_DOWN";
        case telux::tel::OperatingMode::PERSISTENT_LOW_POWER: return "PERSISTENT_LOW_POWER";
        default: return "Unknown";
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Main-thread handler for telux requestOperatingMode() responses.
 *
 * The actual SDK callback (ModemStatus::operatingModeResponse) runs on a
 * telux thread and only marshals its arguments via le_event_Report; this
 * function consumes the marshalled data on the main thread.
 *
 * On a SUCCESS response:
 *   - If a CONNECTION_LOST event had previously been raised
 *     (pendingCount == COUNTER_EVENT_REPORT_DONE), notify clients with a
 *     CONNECTION_AVAIL event so they know the modem is reachable again.
 *   - Reset pendingCount; the timer handler will start counting again on
 *     the next tick.
 */
//--------------------------------------------------------------------------------------------------
static void OperModeRespCBNotify(void* reportPtr)
{
    if (reportPtr == nullptr || modemStatus == nullptr)
    {
        LE_ERROR("modemStatus or reportPtr is nullptr");
        return;
    }

    taf_hms_operModeRespData_t *data = (taf_hms_operModeRespData_t*)reportPtr;
    telux::tel::OperatingMode operatingMode =
        static_cast<telux::tel::OperatingMode>(data->operatingMode);
    telux::common::ErrorCode error =
        static_cast<telux::common::ErrorCode>(data->error);

    if (error == telux::common::ErrorCode::SUCCESS)
    {
        // First successful response after a previously-reported
        // CONNECTION_LOST: tell clients the modem is back.
        if (pendingCount.load() == COUNTER_EVENT_REPORT_DONE)
        {
            modemStatus->ReportModemStatus(TAF_HMS_MODEM_EVENT_TYPE_CONNECTION_AVAIL,
                                           TAF_HMS_MODEM_EVENT_SEVERITY_LOW);
        }
        pendingCount.store(0);

        if (operatingMode == telux::tel::OperatingMode::ONLINE && oneTimeLog)
        {
            LE_INFO("Operating Mode is: %s, pendingCount: %d",
                    OperatingModeToString(operatingMode).c_str(), pendingCount.load());

            oneTimeLog = false;// One-shot log is printed, disable it.
        }
    }
    else
    {
        LE_ERROR("Operating Mode unknown, errorCode: %d, pendingCount: %d",
                 static_cast<int>(error), pendingCount.load());
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * SDK callback - called from telux SDK thread.
 * Marshals the response to the main thread via le_event_Report.
 */
//--------------------------------------------------------------------------------------------------
void ModemStatus::operatingModeResponse(telux::tel::OperatingMode operatingMode,
                                        telux::common::ErrorCode error)
{
    auto &hms = taf_Hms::GetInstance();
    taf_hms_operModeRespData_t data;
    data.operatingMode = static_cast<int>(operatingMode);
    data.error         = static_cast<int>(error);
    le_event_Report(hms.OperModeRespId, &data, sizeof(data));
}

// ---------------------------------------------------------------------------
// modemStatus: the shared_ptr-managed instance used for shared_from_this().
// ---------------------------------------------------------------------------
std::shared_ptr<ModemStatus> modemStatus = std::make_shared<ModemStatus>();

//--------------------------------------------------------------------------------------------------
/**
 * Worker thread entry point.
 */
//--------------------------------------------------------------------------------------------------
void* ModemStatus::WorkerThreadFunc(void*)
{
    LE_INFO("WorkerThread: started");
    le_event_RunLoop();

    LE_INFO("WorkerThread: exiting");
    return nullptr;
}

void ModemStatus::DoRequestOperatingMode(void)
{
    if (!tafHmsListener::ModemAvailability.load())
    {
        LE_WARN("Modem UNAVAILABLE, skipping requestOperatingMode");
        return;
    }

    if (!PhoneInit())
    {
        LE_WARN("PhoneInit failed, will retry next tick");
        return;
    }

    phoneManager_->requestOperatingMode(shared_from_this());
}

//--------------------------------------------------------------------------------------------------
/**
 * StartWorkerThread - called from main thread.
 * Starts the worker thread if not already running.
 */
//--------------------------------------------------------------------------------------------------
void ModemStatus::StartWorkerThread(void)
{
    if (workerThread_ != nullptr)
    {
        LE_INFO("WorkerThread: already running");
        return;
    }

    workerThread_ = le_thread_Create("ModemStatusWorker", WorkerThreadFunc, this);
    le_thread_SetJoinable(workerThread_);
    le_thread_Start(workerThread_);

    mainTimer_ = le_timer_Create("ModemStatusMainTimer");
    le_timer_SetMsInterval(mainTimer_, MODEM_CHECK_STATUS_INTERVAL);
    le_timer_SetRepeat(mainTimer_, 0);  // repeat forever
    le_timer_SetHandler(mainTimer_, ModemStatusTimerHandler);
    le_timer_SetWakeup(mainTimer_, false);
    le_timer_Start(mainTimer_);
    LE_INFO("WorkerThread: started");
}

//--------------------------------------------------------------------------------------------------
/**
 * StopWorkerThread - called from main thread for graceful shutdown.
 * Signals the worker thread to exit and joins it.
 */
//--------------------------------------------------------------------------------------------------
void ModemStatus::StopWorkerThread(void)
{
    if (workerThread_ == nullptr)
    {
        return;
    }

    if (mainTimer_ != nullptr)
    {
        le_timer_Stop(mainTimer_);
        le_timer_Delete(mainTimer_);
        mainTimer_ = nullptr;
    }

    le_thread_Cancel(workerThread_);
    le_thread_Join(workerThread_, nullptr);

   // Worker thread is guaranteed dead here. Clear related flags.
    workerThread_ = nullptr;
    phoneManager_.reset();
    pendingCount.store(0);

    LE_INFO("WorkerThread: stopped");
}

//--------------------------------------------------------------------------------------------------
/**
 * ReportModemStatus - called from main thread.
 * Iterates registered clients and dispatches the event via le_event_Report.
 */
//--------------------------------------------------------------------------------------------------
void ModemStatus::ReportModemStatus(taf_hms_ModemEvtType_t eventType,
                             taf_hms_ModemEvtSeverity_t eventLevel)
{
    auto &hms = taf_Hms::GetInstance();
    bool isAvail = (eventType == TAF_HMS_MODEM_EVENT_TYPE_CONNECTION_AVAIL);

    le_ref_IterRef_t iterRef = le_ref_GetIterator(hms.ModemInfoRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_hms_modemInfo_t* clientInfo = (taf_hms_modemInfo_t*)le_ref_GetValue(iterRef);
        TAF_ERROR_IF_RET_NIL(clientInfo == nullptr, "No registered client for modem info");

        //Check if client has registered for monitoring modem
        if (clientInfo->handlerFunc == NULL)
        {
            LE_WARN("%s skipped for clientId=%u: handlerFunc is NULL",
                    ModemEventTypeToStr(eventType), clientInfo->clientId);
            continue;
        }

        //Check if the client has registered for connection status
        if (!(ModemEvtTypeMatchToBitmask(eventType) & clientInfo->reqEventBits))
        {
            LE_WARN("%s skipped for clientId=%u: "
                    "reqEventBits=0x%x does not match bitmask=0x%x",
                    ModemEventTypeToStr(eventType), clientInfo->clientId,
                    (unsigned)clientInfo->reqEventBits,
                    (unsigned)ModemEvtTypeMatchToBitmask(eventType));
            continue;
        }

        taf_hms_modemEventInfo_t newEvent;
        newEvent.modemInfo  = clientInfo;
        newEvent.eventType  = eventType;
        newEvent.eventLevel = eventLevel;

        if (isAvail)
        {
            DispatchEventToClient(&newEvent);
        }
        else
        {
            le_event_Report(hms.MdStatusChNotifyCliId, &newEvent,
                            sizeof(taf_hms_modemEventInfo_t));
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Called from worker thread only.
 * Acquires IPhoneManager with a "TAF_HMS_PHONE_MANAGER_TIMEOUT" second timeout.
 */
//--------------------------------------------------------------------------------------------------
bool ModemStatus::PhoneInit(void)
{
    if (phoneManager_)
    {
        return true;
    }

    auto prom = std::make_shared<std::promise<telux::common::ServiceStatus>>();
    auto tempManager = telux::tel::PhoneFactory::getInstance().getPhoneManager(
        [prom](telux::common::ServiceStatus status)
        {
            try   { prom->set_value(status); }
            catch (const std::future_error &e)
            { LE_ERROR("Promise already satisfied: %s", e.what()); }
        });

    if (!tempManager)
    {
        LE_ERROR("ERROR - Failed to get Phone Manager");
        return false;
    }

    auto statusNow = tempManager->getServiceStatus();
    if (statusNow != telux::common::ServiceStatus::SERVICE_AVAILABLE)
    {
        LE_WARN("Phone Manager subsystem is not ready (timeout %ds)",
                TAF_HMS_PHONE_MANAGER_TIMEOUT);
    }

    auto fut  = prom->get_future();
    if (fut.wait_for(std::chrono::seconds(TAF_HMS_PHONE_MANAGER_TIMEOUT))
            != std::future_status::ready)
    {
        LE_ERROR("Timed out after %ds waiting for SDK callback",
                 TAF_HMS_PHONE_MANAGER_TIMEOUT);
        return false;
    }

    if (fut.get() != telux::common::ServiceStatus::SERVICE_AVAILABLE)
    {
        LE_ERROR("Service not available, will retry next tick");
        return false;
    }

    phoneManager_ = tempManager;
    LE_INFO("PhoneInit: success, phoneManager_=valid");
    return true;
}

//--------------------------------------------------------------------------------------------------
/*
* | Reset type                | Reset reason               | Description                          |
* |:-------------------------:|:--------------------------:|:------------------------------------:|
* | TAF_HMS_RESET_UNKNOWN     | unknown                    | Unknow case                          |
* | TAF_HMS_RESET_CRASH       | panic                      | Kernel crash                         |
* | TAF_HMS_RESET_CRASH       | telaf crash                | TelAF crash                          |
* | TAF_HMS_RESET_CRASH       | ['xxx' crash]              | [Reserved: 'xxx' crash]              |
* | TAF_HMS_RESET_UPDATE      | recovery                   | System recovery                      |
* | TAF_HMS_RESET_UPDATE      | ['telaf' update]           | [Reserved: TelAF update]             |
* | TAF_HMS_RESET_UPDATE      | ['configuration' update]   | [Reserved: Configuration update]     |
* | TAF_HMS_RESET_UPDATE      | ['xxx' update]             | [Reserved: 'xxx' update]             |
* | TAF_HMS_RESET_CORRUPTED   | dm-verity device corrupted | dm-verity device was corrupted       |
* | TAF_HMS_RESET_CORRUPTED   | ['telaf image' corrupted]  | [Reserved: TelAF image was corrupted]|
* | TAF_HMS_RESET_CORRUPTED   | ['xxx image' corrupted]    | [Reserved: 'xxx' was corrupted]      |
* | TAF_HMS_RESET_AUTH_FAILED | ['telaf' auth failed]      | [Reserved: TelAF auth failed]        |
* | TAF_HMS_RESET_AUTH_FAILED | ['xxx' auth failed]        | [Reserved: 'xxx' auth failed]        |
* | TAF_HMS_RESET_USER        | user                       | Not defined user reboot              |
* | TAF_HMS_RESET_USER        | bootloader                 | reboot bootloader/fastboot continue  |
* | TAF_HMS_RESET_USER        | dm-verity enforcing        | dm-verity operation                  |
* | TAF_HMS_RESET_USER        | keys clear                 | dm-verity operation                  |
* | TAF_HMS_RESET_WDOG        | watchdog bark              | Watchdog bark                        |
* | TAF_HMS_RESET_WDOG        | ['xxx' watchdog bark]      | [Reserved: 'xxx' watchdog bark]      |
* | TAF_HMS_RESET_HARD        | rtc                        | RTC alarm boot                       |
* | TAF_HMS_RESET_HARD        | [Hardware switch]          | [Reserved: Hardware switch]          |
* | TAF_HMS_RESET_HARD        | [Power down]               | [Reserved: Power source unplugged]   |
* | TAF_HMS_RESET_TEMP_CRIT   | [Critical Temp]            | [Reserved: Critical voltage level]   |
* | TAF_HMS_RESET_VOLT_CRIT   | [Critical Voltage]         | [Reserved: Critical temperature LV]  |
* | TAF_HMS_xxx               | system-normal      | Triggered by system in a supported scenario" |
* | TAF_HMS_xxx               | system-abnormal    | Triggered by system in an errorscenario"     |
*
* Note, we can use below example command to verify the NOT 'Reserved' feature:
* 1. "panic" --  reboot panic
* 2. "recovery" -- reboot recovery
* 3. "telaf crash" -- killall deviceManager
* 4. "watchdog bark" -- echo 'k' > /dev/watchdog0
*/
//--------------------------------------------------------------------------------------------------
taf_hms_Reset_t taf_Hms::ParseBootReason(const std::string& reasonStrRaw)
{
    std::string reasonStr = reasonStrRaw;

    //Important: Convert the ''string' to lowercase for case-insensitive comparison.
    //Please use lowercase when trying to add a new reason.
    std::transform(reasonStr.begin(), reasonStr.end(), reasonStr.begin(), ::tolower);

    LE_DEBUG("Reason string: %s\n", reasonStr.c_str());

    if ((reasonStr.size() >= 5 &&
             reasonStr.compare(reasonStr.size() - 5, 5, "crash") == 0) ||
             reasonStr == "panic")
    {
        return TAF_HMS_RESET_CRASH;
    }
    else if ((reasonStr.size() >= 6 &&
             reasonStr.compare(reasonStr.size() - 6, 6, "update") == 0) ||
             reasonStr == "recovery")
    {
        return TAF_HMS_RESET_UPDATE;
    }
    else if (reasonStr.size() >= 9 &&
             reasonStr.compare(reasonStr.size() - 9, 9, "corrupted") == 0)
    {
        return TAF_HMS_RESET_CORRUPTED;
    }
    else if (reasonStr.size() >= 11 &&
             reasonStr.compare(reasonStr.size() - 11, 11, "auth failed") == 0)
    {
        return TAF_HMS_RESET_AUTH_FAILED;
    }
    else if (reasonStr == "user" ||
             reasonStr == "bootloader" ||
             reasonStr == "dm-verity enforcing" ||
             reasonStr == "keys clear" ||
             reasonStr == "system-normal")
    {
        return TAF_HMS_RESET_USER;
    }
    else if (reasonStr == "hardware switch" ||
             reasonStr == "rtc" ||
             reasonStr == "power down")
    {
        return TAF_HMS_RESET_HARD;
    }
    else if (reasonStr.size() >= 13 &&
             reasonStr.compare(reasonStr.size() - 13, 13, "watchdog bark") == 0)
    {
        return TAF_HMS_RESET_WDOG;
    }
    else if (reasonStr == "critical temp")
    {
        return TAF_HMS_RESET_TEMP_CRIT;
    }
    else if (reasonStr == "critical voltage")
    {
        return TAF_HMS_RESET_VOLT_CRIT;
    }
    else if (reasonStr == "admin-trigger")
    {
        return TAF_HMS_RESET_ADMIN_TRIGGER;
    }
    else
    {
        return TAF_HMS_RESET_UNKNOWN;
    }
}

std::string BootReasonToString(taf_hms_Reset_t reason)
{
    switch (reason)
    {
        case TAF_HMS_RESET_CRASH:
            return "TAF_HMS_RESET_CRASH";

        case TAF_HMS_RESET_UPDATE:
            return "TAF_HMS_RESET_UPDATE";

        case TAF_HMS_RESET_CORRUPTED:
            return "TAF_HMS_RESET_CORRUPTED";

        case TAF_HMS_RESET_AUTH_FAILED:
            return "TAF_HMS_RESET_AUTH_FAILED";

        case TAF_HMS_RESET_USER:
            return "TAF_HMS_RESET_USER";

        case TAF_HMS_RESET_HARD:
            return "TAF_HMS_RESET_HARD";

        case TAF_HMS_RESET_POWER_DOWN:
            return "TAF_HMS_RESET_POWER_DOWN";

        case TAF_HMS_RESET_WDOG:
            return "TAF_HMS_RESET_WDOG";

        case TAF_HMS_RESET_TEMP_CRIT:
            return "TAF_HMS_RESET_TEMP_CRIT";

        case TAF_HMS_RESET_VOLT_CRIT:
            return "TAF_HMS_RESET_VOLT_CRIT";

        case TAF_HMS_RESET_ADMIN_TRIGGER:
            return "TAF_HMS_RESET_ADMIN_TRIGGER";

        default:
            return "TAF_HMS_RESET_UNKNOWN";
    }
}

//--------------------------------------------------------------------------------------------------
/**
 ** Reads the boot sub-reason from a file.
 **
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::ReadSubReason
(
    const std::string& filePath,
    char* subReasonStr,
    size_t subReasonSize
)
{
    TAF_ERROR_IF_RET_VAL(subReasonStr == NULL || subReasonSize == 0,
                         LE_BAD_PARAMETER, "Invalid sub-reason output buffer");

    std::string tmpStr;
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        LE_ERROR("Failed to open file: %s", filePath.c_str());
        return LE_NOT_FOUND;
    }

    std::getline(file, tmpStr);
    file.close();

    LE_DEBUG("Reboot sub-reason string: %s",tmpStr.c_str());

    return le_utf8_Copy(subReasonStr, tmpStr.c_str(), subReasonSize, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the last reset information reason
 *
 * @return
 *      - LE_OK          on success
 *      - LE_UNSUPPORTED if it is not supported by the platform
 *        LE_OVERFLOW    specific reset information length exceeds the maximum length.
 *      - LE_FAULT       for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_Hms::GetResetInformation
(
    taf_hms_Reset_t* resetPtr,          ///< [OUT] Reset information
    char* resetSpecificInfoStrPtr,      ///< [OUT] Reset specific information
    size_t resetSpecificInfoStrSize     ///< [IN]  The length of specific information string.
)
{
    TAF_ERROR_IF_RET_VAL(resetPtr == NULL || resetSpecificInfoStrPtr == NULL ||
                         resetSpecificInfoStrSize == 0,
                         LE_BAD_PARAMETER, "Invalid reset information output parameter");

    le_result_t res = LE_OK;
    std::ifstream file(TAF_HMS_BOOT_REASON_PATH);
    if (!file.is_open())
    {
        LE_ERROR("Failed to open %s", TAF_HMS_BOOT_REASON_PATH);
        return LE_NOT_FOUND;
    }

    std::string reasonStr;
    std::getline(file, reasonStr);
    file.close();

    taf_hms_Reset_t reason = ParseBootReason(reasonStr);
    if (reason == TAF_HMS_RESET_ADMIN_TRIGGER)
    {
        char subReasonBuf[128] = {0};
        res = ReadSubReason(TAF_HMS_BOOT_SUB_REASON_PATH, subReasonBuf, sizeof(subReasonBuf));
        if (res != LE_OK)
        {
            //Can not read sub-reason, return upper reason later.
            LE_ERROR("Failed to read sub-reason from %s", TAF_HMS_BOOT_SUB_REASON_PATH);
        }
        else
        {
            reasonStr = subReasonBuf;
            reason = ParseBootReason(reasonStr);
        }
    }

    res = le_utf8_Copy(resetSpecificInfoStrPtr, reasonStr.c_str(), resetSpecificInfoStrSize, NULL);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to copy reason string to buffer");
        return res;
    }

    LE_INFO("Reset info - type: %d - %s: %s",
            reason, BootReasonToString(reason).c_str(), resetSpecificInfoStrPtr);

    *resetPtr = reason;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
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
    ModemEventInfoPool = le_mem_InitStaticPool(ModemEventInfoPool, TAF_HMS_MAX_EVENT_POOL_SIZE,
        sizeof(taf_hms_modemEventInfo_t));
    ModemInfoPool = le_mem_InitStaticPool(ModemInfoPool, TAF_HMS_MAX_REF_POOL_SIZE,
            sizeof(taf_hms_modemInfo_t));

    UbiDevListRefMap = le_ref_InitStaticMap(UbiDevListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    UbiDevRefMap = le_ref_InitStaticMap(UbiDevRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    UbiVolListRefMap = le_ref_InitStaticMap(UbiVolListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    UbiVolRefMap = le_ref_InitStaticMap(UbiVolRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    MtdListRefMap = le_ref_InitStaticMap(MtdListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    MtdRefMap = le_ref_InitStaticMap(MtdRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    ModemInfoRefMap = le_ref_InitStaticMap(ModemInfoRefMap, TAF_HMS_MAX_REF_POOL_SIZE);
    ModemEventInfoRefMap = le_ref_InitStaticMap(ModemEventInfoRefMap, TAF_HMS_MAX_EVENT_POOL_SIZE);


    //Modem monitor
    telux::common::ErrorCode ec;
    telux::common::ServiceStatus serviceStatus;

    auto promisePtr = std::make_shared<std::promise<telux::common::ServiceStatus>>();

    auto &subsystemFact = telux::platform::SubsystemFactory::getInstance();

    subsystemMgr = subsystemFact.getSubsystemManager(
        [promisePtr](telux::common::ServiceStatus srvStatus){
            try {
                promisePtr->set_value(srvStatus);
            } catch (const std::future_error &e) {

                LE_FATAL("Promise already satisfied: %s", e.what());
            }
        });

    if (!subsystemMgr) {
        LE_FATAL("Couldn't get the subsystemMgr");
    }

    auto future = promisePtr->get_future();
    if (future.wait_for(std::chrono::seconds(TAF_HMS_SUBSYSTEM_MANAGER_TIMEOUT)) == std::future_status::ready) {
        serviceStatus = future.get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LE_FATAL("ISubsystemManager unavailable");
        }
    } else {
        LE_FATAL("Timeout waiting for serviceStatus callback");
    }

    telux::common::SubsystemInfo subsysInfo{};
    std::vector<telux::common::SubsystemInfo> listOfSubsystems;
    stateListener = std::make_shared<tafHmsListener>();

    subsysInfo.location = telux::common::ProcType::LOCAL_PROC;
    subsysInfo.subsystems = telux::common::Subsystem::MPSS;
    listOfSubsystems.push_back(subsysInfo);
    ec = subsystemMgr->registerListener(stateListener, listOfSubsystems);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LE_FATAL("Can't register listener, err ");
    }

    // Set modem to ready
    tafHmsListener::ModemAvailability.store(true);

    MdStatusChNotifyCliId =
        le_event_CreateId("MdStatusChNotifyCliId", sizeof(taf_hms_modemEventInfo_t));

    le_event_AddHandler("ModemStatusChangeIdHandlerRef",
        MdStatusChNotifyCliId, ModemStatusChNotifyClient);

    MdStatusOnChangeCBId =
        le_event_CreateId("MdStatusOnChangeCBId", sizeof(taf_hms_modemOperaInfo_t));
    le_event_AddHandler("MdStatusOnChangeCBIdHandlerRef",
        MdStatusOnChangeCBId, MdStatusOnChangeCBNotify);

    OperModeRespId =
        le_event_CreateId("OperModeRespId", sizeof(taf_hms_operModeRespData_t));
    le_event_AddHandler("OperModeRespIdHandlerRef",
        OperModeRespId, OperModeRespCBNotify);

    AdvertiseService();
}
