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
#include <chrono>
#include <thread>
#include <dirent.h>


using namespace std;
using namespace telux::tafsvc;

//Default Threshold values
/*double CPU_MID_THRESHOLD = 50.0; // Mid threshold in percentage
double CPU_CRITICAL_THRESHOLD = 90.0; // Critical threshold in percentage
const uint32_t CPU_DEBOUNCE_VALUE = 3; // Debounce value in percentage
const uint32_t MEM_MID_THRESHOLD = 30; // Mid threshold in KB
const uint32_t MEM_CRITICAL_THRESHOLD = 10; // Critical threshold in KB
const uint32_t MEM_DEBOUNCE_VALUE = 5; // Debounce value in KB
*/

LE_MEM_DEFINE_STATIC_POOL(ubiDevListPool, TAF_HMS_MAX_LIST_POOL_SIZE, sizeof(taf_hms_ubiDevInfoList_t));
LE_MEM_DEFINE_STATIC_POOL(ubiDevInfoPool, TAF_HMS_MAX_LIST_POOL_SIZE, sizeof(taf_hms_ubiDevInfo_t));
LE_MEM_DEFINE_STATIC_POOL(ubiVolListPool, TAF_HMS_MAX_LIST_POOL_SIZE, sizeof(taf_hms_ubiVolInfoList_t));
LE_MEM_DEFINE_STATIC_POOL(ubiVolInfoPool, TAF_HMS_MAX_LIST_POOL_SIZE, sizeof(taf_hms_ubiVolInfo_t));
LE_MEM_DEFINE_STATIC_POOL(mtdListPool, TAF_HMS_MAX_LIST_POOL_SIZE, sizeof(taf_hms_mtdInfoList_t));
LE_MEM_DEFINE_STATIC_POOL(mtdInfoPool, TAF_HMS_MAX_LIST_POOL_SIZE, sizeof(taf_hms_mtdInfo_t));


LE_REF_DEFINE_STATIC_MAP(ubiDevListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(ubiDevRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(ubiVolListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(ubiVolRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(mtdListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
LE_REF_DEFINE_STATIC_MAP(mtdRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);

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


// Function to read a file from the given path
uint32_t read_sysfs_file(const char *path)
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
        return LE_FAULT;

    rc = fscanf(fp, "%d\n", &value);
    UNUSED(rc);
    fclose(fp);
    return value;
}

// Function to read a string from the given path
uint32_t read_sysfs_string_file(const char *path, char *buffer, size_t length)
{
    FILE *file = fopen(path, "r");
    LE_INFO("Reading from path %s", path);
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
    return LE_OK;
}

// Function to get device count from the sys class path
uint32_t get_ubi_device_count
(
)
{
    DIR *dir;
    struct dirent *entry;
    uint32_t count = 0;

    dir = opendir(UBI_CLASS_PATH);
    if (dir == NULL) {
        LE_ERROR("Error opening UBI class path");
        return LE_FAULT;
    }

    // Iterate through each entry in the directory
    while ((entry = readdir(dir)) != NULL) {
        // Check if the entry is a directory and its name starts with "ubi"
        if (entry->d_type == DT_DIR && strncmp(entry->d_name, "ubi", 3) == 0) {
            count++;
        }
    }

    closedir(dir);
    return count;
}

uint32_t get_ubi_volume_count(const char *path)
{
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror("Failed to open sysfs file");
        return -1;
    }

    int value;
    if (fscanf(file, "%d", &value) != 1) {
        perror("Failed to read value from sysfs file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return value;
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
    taf_hms_ubiDevInfoList_t* ubiDevList = (taf_hms_ubiDevInfoList_t*)le_mem_ForceAlloc(ubiDevListPool);
    memset(ubiDevList, 0, sizeof(taf_hms_ubiDevInfoList_t));
    ubiDevList->ubiDevInfoList = LE_SLS_LIST_INIT;
    ubiDevList->currPtr = NULL;

    taf_hms_ubiDevInfo_t* ubiDevInfoPtr;

    uint32_t count = get_ubi_device_count();
    char path[MAX_PATH_LENGTH];
    ubiDevList->ubiDevInfoListSize = count;
    if (count >= 0)
    {
        for(uint32_t i = 0; i <= count; i++)
        {
            ubiDevInfoPtr = (taf_hms_ubiDevInfo_t*)le_mem_ForceAlloc(ubiDevInfoPool);
            memset(ubiDevInfoPtr, 0, sizeof(taf_hms_ubiDevInfo_t));

            // Get bad block count
           snprintf(path, sizeof(path), UBI_DEV_BB_COUNT_PATH, i);
           ubiDevInfoPtr->badBlockCnt = read_sysfs_file(path);

           // Get max erase count
           snprintf(path, sizeof(path), UBI_DEV_E_COUNT_PATH, i);
           ubiDevInfoPtr->eraseCnt = read_sysfs_file(path);

           ubiDevInfoPtr->link = LE_SLS_LINK_INIT;
           le_sls_Queue(&(ubiDevList->ubiDevInfoList), &(ubiDevInfoPtr->link));
           ubiDevInfoPtr->ref =
                (taf_hms_UbiDevInfoRef_t)le_ref_CreateRef(ubiDevRefMap, (void*)ubiDevInfoPtr);
        }
        ubiDevList->ref =
            (taf_hms_UbiDevInfoListRef_t)le_ref_CreateRef(ubiDevListRefMap, ubiDevList);
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
        (taf_hms_ubiDevInfoList_t*)le_ref_Lookup(ubiDevListRefMap, ubiDevInfoListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");
    LE_DEBUG("DeleteUBIDevInfoList : %p", ubiDevInfoListRef);
    taf_hms_ubiDevInfo_t* ubiDevInfoPtr;
    le_sls_Link_t* linkPtr;
    while ((linkPtr = le_sls_Pop(&(listPtr->ubiDevInfoList))) != NULL)
    {
        ubiDevInfoPtr = CONTAINER_OF(linkPtr, taf_hms_ubiDevInfo_t, link);
        le_mem_Release(ubiDevInfoPtr);
    }
    le_ref_DeleteRef(ubiDevListRefMap, ubiDevInfoListRef);
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
            (taf_hms_ubiDevInfoList_t*)le_ref_Lookup(ubiDevListRefMap, ubiDevInfoListRef);

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
            (taf_hms_ubiDevInfoList_t*)le_ref_Lookup(ubiDevListRefMap, ubiDevInfoListRef);

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
    taf_hms_ubiDevInfo_t* ubiDevPtr = (taf_hms_ubiDevInfo_t*)le_ref_Lookup(ubiDevRefMap, ubiDevInfoRef);

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
    taf_hms_ubiDevInfo_t* ubiDevPtr = (taf_hms_ubiDevInfo_t*)le_ref_Lookup(ubiDevRefMap, ubiDevInfoRef);

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
    taf_hms_ubiDevInfo_t* ubiDevPtr = (taf_hms_ubiDevInfo_t*)le_ref_Lookup(ubiDevRefMap, ubiDevInfoRef);

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
            (taf_hms_ubiVolInfoList_t*)le_ref_Lookup(ubiVolListRefMap, ubiDevInfoRef);

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
            (taf_hms_ubiVolInfoList_t*)le_ref_Lookup(ubiVolListRefMap, ubiDevInfoRef);

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
    taf_hms_ubiVolInfo_t* ubiVolPtr =
        (taf_hms_ubiVolInfo_t*)le_ref_Lookup(ubiVolRefMap, ubiVolInfoRef);

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
    taf_hms_ubiVolInfo_t* ubiVolPtr =
            (taf_hms_ubiVolInfo_t*)le_ref_Lookup(ubiVolRefMap, ubiVolInfoRef);
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
    taf_hms_ubiVolInfo_t* ubiVolPtr =
        (taf_hms_ubiVolInfo_t*)le_ref_Lookup(ubiVolRefMap, ubiVolInfoRef);

    TAF_ERROR_IF_RET_VAL(ubiVolPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            ubiVolPtr);

    *ubiVolSizePtr = ubiVolPtr->ubiVolSize;
    TAF_ERROR_IF_RET_VAL(*ubiVolSizePtr < 0, LE_FAULT, "Failed to return Ubi volume size.");
    return LE_OK;
}


uint32_t get_mtd_count()
{
    DIR* dir;
    struct dirent* entry;
    int mtd_count = 0;

    dir = opendir(MTD_CLASS_PATH);
    if (dir == NULL) {
        LE_ERROR("opendir");
        return LE_FAULT;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
    }
    // Check if entry->d_name starts with "mtd" (case-insensitive)
    if (strncasecmp(entry->d_name, "mtd", 3) == 0) {
      mtd_count++;
      }
    }

    closedir(dir);
    LE_INFO("MTD device count: %d", mtd_count);
    mtd_count = mtd_count / 2;
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
    taf_hms_mtdInfoList_t* mtdDevList = (taf_hms_mtdInfoList_t*)le_mem_ForceAlloc(mtdListPool);
    memset(mtdDevList, 0, sizeof(taf_hms_mtdInfoList_t));
    mtdDevList->mtdInfoList = LE_SLS_LIST_INIT;
    mtdDevList->currPtr = NULL;

    taf_hms_mtdInfo_t* mtdInfoPtr;

    uint32_t count = get_mtd_count();
    char path[MAX_PATH_LENGTH];
    char buffer[BUFFER_SIZE];
    mtdDevList->mtdInfoListSize = count;
    if (count >= 0)
    {
        for(uint32_t i = 0; i < count; i++)
        {
            mtdInfoPtr = (taf_hms_mtdInfo_t*)le_mem_ForceAlloc(mtdInfoPool);
            memset(mtdInfoPtr, 0, sizeof(taf_hms_mtdInfo_t));

            // Get mtd block size
            snprintf(path, sizeof(path), MTD_DEV_SIZE_PATH, i);
            mtdInfoPtr->mtdBlockSize = read_sysfs_file(path);

            // Get device name
            snprintf(path, sizeof(path), MTD_DEV_NAME_PATH, i);
            uint8_t result = read_sysfs_string_file(path, buffer, sizeof(buffer));
            if (result == LE_OK)
            {
                char* mtddata = buffer;
                snprintf(mtddata, sizeof(mtdInfoPtr->mtdDevName), "%s",
                        mtdInfoPtr->mtdDevName);
            }
            else
            {
                LE_INFO("Failed ! to read data from mtd path");
            }

           mtdInfoPtr->link = LE_SLS_LINK_INIT;
           le_sls_Queue(&(mtdDevList->mtdInfoList), &(mtdInfoPtr->link));
           mtdInfoPtr->ref =
                (taf_hms_MtdDevInfoRef_t)le_ref_CreateRef(mtdRefMap, (void*)mtdInfoPtr);
        }
        mtdDevList->ref =
            (taf_hms_MtdDevInfoListRef_t)le_ref_CreateRef(mtdListRefMap, mtdDevList);
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
        (taf_hms_mtdInfoList_t*)le_ref_Lookup(mtdListRefMap, mtdDevInfoListRef);
    TAF_ERROR_IF_RET_VAL(listPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");
    LE_DEBUG("DeleteMTDInfoList : %p", mtdDevInfoListRef);
    taf_hms_mtdInfo_t* mtdInfoPtr;
    le_sls_Link_t* linkPtr;
    while ((linkPtr = le_sls_Pop(&(listPtr->mtdInfoList))) != NULL)
    {
        mtdInfoPtr = CONTAINER_OF(linkPtr, taf_hms_mtdInfo_t, link);
        le_mem_Release(mtdInfoPtr);
    }
    le_ref_DeleteRef(mtdListRefMap, mtdDevInfoListRef);
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
            (taf_hms_mtdInfoList_t*)le_ref_Lookup(mtdListRefMap, mtdDevInfoListRef);

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
            (taf_hms_mtdInfoList_t*)le_ref_Lookup(mtdListRefMap, mtdDevInfoListRef);

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
    taf_hms_mtdInfo_t* mtdDevPtr =
            (taf_hms_mtdInfo_t*)le_ref_Lookup(mtdRefMap, mtdDevInfoRef);
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
    taf_hms_mtdInfo_t* mtdDevPtr =
        (taf_hms_mtdInfo_t*)le_ref_Lookup(mtdRefMap, mtdDevInfoRef);

    TAF_ERROR_IF_RET_VAL(mtdDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            mtdDevPtr);

    *mtdBlkSizePtr = mtdDevPtr->mtdBlockSize;
    TAF_ERROR_IF_RET_VAL(*mtdBlkSizePtr < 0, LE_FAULT, "Failed to return MTD device size.");
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 ** Gets MTD information for block size.
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
   taf_hms_mtdInfo_t* mtdDevPtr =
        (taf_hms_mtdInfo_t*)le_ref_Lookup(mtdRefMap, mtdDevInfoRef);

    TAF_ERROR_IF_RET_VAL(mtdDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            mtdDevPtr);

    *mtdDevIdPtr = mtdDevPtr->mtdDevId;
    TAF_ERROR_IF_RET_VAL(*mtdDevIdPtr < 0, LE_FAULT, "Failed to return MTD device ID.");
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 ** Gets MTD information for block size.
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
    taf_hms_mtdInfo_t* mtdDevPtr =
        (taf_hms_mtdInfo_t*)le_ref_Lookup(mtdRefMap, mtdDevInfoRef);

    TAF_ERROR_IF_RET_VAL(mtdDevPtr == NULL, LE_FAULT, "Invalid reference (%p) provided!",
            mtdDevPtr);

    *mtdBlkCntPtr = mtdDevPtr->mtdBlockCnt;
    TAF_ERROR_IF_RET_VAL(*mtdBlkCntPtr < 0, LE_FAULT, "Failed to return MTD Block size.");
    return LE_OK;
}

void taf_Hms::Init()
{
    LE_INFO("tafHMSvc started");

    ubiDevListPool = le_mem_InitStaticPool(ubiDevListPool, TAF_HMS_MAX_LIST_POOL_SIZE,
        sizeof(taf_hms_ubiDevInfoList_t));
    ubiDevInfoPool = le_mem_InitStaticPool(ubiDevInfoPool, TAF_HMS_MAX_LIST_POOL_SIZE,
        sizeof(taf_hms_ubiDevInfo_t));
    ubiVolListPool = le_mem_InitStaticPool(ubiVolListPool, TAF_HMS_MAX_LIST_POOL_SIZE,
        sizeof(taf_hms_ubiVolInfoList_t));
    ubiVolInfoPool = le_mem_InitStaticPool(ubiVolInfoPool, TAF_HMS_MAX_LIST_POOL_SIZE,
        sizeof(taf_hms_ubiVolInfo_t));
    mtdListPool = le_mem_InitStaticPool(mtdListPool, TAF_HMS_MAX_LIST_POOL_SIZE,
        sizeof(taf_hms_mtdInfoList_t));
    mtdInfoPool = le_mem_InitStaticPool(mtdInfoPool, TAF_HMS_MAX_LIST_POOL_SIZE,
        sizeof(taf_hms_mtdInfo_t));

    ubiDevListRefMap = le_ref_InitStaticMap(ubiDevListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    ubiDevRefMap = le_ref_InitStaticMap(ubiDevRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    ubiVolListRefMap = le_ref_InitStaticMap(ubiVolListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    ubiVolRefMap = le_ref_InitStaticMap(ubiVolRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    mtdListRefMap = le_ref_InitStaticMap(mtdListRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);
    mtdRefMap = le_ref_InitStaticMap(mtdRefMap, TAF_HMS_MAX_LIST_POOL_SIZE);

}
