/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#include <iostream>

#include "taf_gptpTime.h"

#define TAF_TIME_CLOCKFD                   3
#define TAF_TIME_FD_TO_CLOCKID(fd)        ((~(clockid_t) (fd) << 3) | TAF_TIME_CLOCKFD)
#define TAF_TIME_LOCAL_PTP_NODE           "/dev/ptp0"

#define GPTP_DEVICE_STR_BUF_MAX    60
#define GPTP_DEVICE_NUM_MAX        2

le_mem_PoolRef_t GptpTimeMemPoolRef;
le_ref_MapRef_t GptpTimeRefMap;

typedef struct
{
    clockid_t clkid;
    int fd;
    const char* deviceName;
    taf_gptpTime_Ref_t safeRef;
} taf_GptpTime_t;

taf_GptpTime_t* isGptpRefExist(const char* deviceName)
{

    le_ref_IterRef_t iterRef = le_ref_GetIterator(GptpTimeRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_GptpTime_t* gptpTime  = (taf_GptpTime_t*)le_ref_GetValue(iterRef);
        if ((gptpTime != NULL)
            && (gptpTime->deviceName == deviceName)
            )
        {
            return gptpTime;
        }
    }

    return NULL;
}

LE_SHARED taf_gptpTime_Ref_t taf_gptpTime_CreateRef
(
    const char* deviceName
)
{
    std::string devName = deviceName;
    if(devName.size() > GPTP_DEVICE_STR_BUF_MAX)
    {
        LE_ERROR("Device name too long!");
        return NULL;
    }
    if(GptpTimeMemPoolRef == NULL)
    {
        GptpTimeMemPoolRef = le_mem_CreatePool("GptpTimePool", sizeof(taf_GptpTime_t));
    }
    if(GptpTimeRefMap == NULL)
    {
        GptpTimeRefMap = le_ref_CreateMap("GptpTime", GPTP_DEVICE_NUM_MAX);
    }
    if(GptpTimeMemPoolRef == NULL || GptpTimeRefMap == NULL)
    {
        LE_ERROR("Unable to allocate memory for gptp time. Please try again.");
        return NULL;
    }
    taf_GptpTime_t* gptpTime = isGptpRefExist(deviceName);
    if(gptpTime != NULL)
    {
        return gptpTime->safeRef;
    }
    gptpTime = (taf_GptpTime_t*)le_mem_ForceAlloc(GptpTimeMemPoolRef);
    int fd = open(deviceName, O_RDONLY);
    if (-1 == fd)
    {
        switch (errno)
        {
            case EPERM:
                LE_ERROR("No permission to open the device\n");
                return NULL;

            case ENOENT:
                LE_ERROR("Not found the device\n");
                return NULL;

            case ETXTBSY:
                LE_ERROR("The device is busy\n");
                return NULL;

            default:
                LE_ERROR("Open %s failed: %d, %s", deviceName,
                                                    errno, strerror(errno));
                return NULL;
        }
    }
    clockid_t clkid = TAF_TIME_FD_TO_CLOCKID(fd);
    gptpTime->deviceName = deviceName;
    gptpTime->clkid = clkid;
    gptpTime->fd = fd;
    gptpTime->safeRef = (taf_gptpTime_Ref_t)le_ref_CreateRef(GptpTimeRefMap, gptpTime);
    return gptpTime->safeRef;
}

LE_SHARED le_result_t taf_gptpTime_GetTimeValue
(
    taf_gptpTime_Ref_t gptpTimeRef,
    struct timespec* gptpTimeValuePtr
)
{
    taf_GptpTime_t* gptpPtr = (taf_GptpTime_t*)le_ref_Lookup(GptpTimeRefMap, gptpTimeRef);
    if(gptpPtr == NULL)
    {
        LE_ERROR("GPTP Reference is not found!");
        return LE_BAD_PARAMETER;
    }

    struct timespec ts;
    if (-1 == clock_gettime(gptpPtr->clkid, &ts))
    {
        switch (errno)
        {
            case EPERM:
                LE_ERROR("No permission to read time\n");
                return LE_NOT_PERMITTED;

            case EINVAL:
                LE_ERROR("Invalid parameter\n");
                return LE_BAD_PARAMETER;

            default:
                LE_ERROR("Get time failed (errno = %d)\n", errno);
                return LE_FAULT;
        }
    }
    gptpTimeValuePtr->tv_sec = ts.tv_sec;
    gptpTimeValuePtr->tv_nsec = ts.tv_nsec;
    return LE_OK;
}
 
LE_SHARED le_result_t taf_gptpTime_DeleteRef
(
    taf_gptpTime_Ref_t gptpTimeRef
)
{
    int result;
    taf_GptpTime_t* gptpPtr = (taf_GptpTime_t*)le_ref_Lookup(GptpTimeRefMap, gptpTimeRef);
    if(gptpPtr == NULL)
    {
        LE_ERROR("GPTP Reference is not found!");
        return LE_BAD_PARAMETER;
    }
    result = close(gptpPtr->fd);
    if (result != 0)
    {
        LE_ERROR("Failed to close file descriptor %d. Errno = %d.", gptpPtr->fd, errno);
        return LE_FAULT;
    }
    le_ref_DeleteRef(GptpTimeRefMap, gptpTimeRef);
    le_mem_Release(gptpPtr);
    return LE_OK;
}

COMPONENT_INIT
{
    LE_INFO("GPTP component initialization done\n");
}

