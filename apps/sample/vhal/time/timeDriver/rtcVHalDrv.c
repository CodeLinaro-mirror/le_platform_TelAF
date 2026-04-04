/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafHalTime.h"
#include <linux/rtc.h>

//#define TAF_TIME_DEV_NAME "/dev/rtc0"
#define TAF_HAL_RTC_DEV_NAME      "/dev/rtc0"
#define TAF_HAL_FILE_NAME         "/tmp/rtc0"
#define TAF_HAL_RTC_IS_READ_ONLY  1

static TAF_HAL_GETRTCASYNCCALLBACK getRTCAsyncCallbackFunc = NULL;
static TAF_HAL_SETRTCASYNCCALLBACK setRTCAsyncCallbackFunc = NULL;

static le_mem_PoolRef_t GetRTCRequestPoolRef;
static le_event_Id_t GetRTCRequestEventId;
static le_mem_PoolRef_t setRTCRequestPoolRef;
static le_event_Id_t SetRTCRequestEventId;

typedef struct
{
    le_result_t responseState;
} SetRTCRequest_t;

typedef struct
{
    struct TimeSpec timeVal;
    le_result_t responseState;
} GetRTCRequest_t;

struct timespec bootTimeWhenSetRTCTime = (struct timespec){0, 0};

//--------------------------------------------------------------------------------------------------
/**
 * Get system boot time.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- If any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t GetBootTime
(
    struct timespec *timeValPtr
)
{
    struct timespec bootTime;

    if ( clock_gettime(CLOCK_BOOTTIME, &bootTime) < 0 )
    {
        return LE_FAULT;
    }
    timeValPtr->tv_sec = bootTime.tv_sec;
    timeValPtr->tv_nsec = bootTime.tv_nsec;
    return LE_OK;
}

static int WriteTimeToFile(struct TimeSpec timeVal)
{
    int fd = open(TAF_HAL_FILE_NAME, O_RDWR | O_CREAT | O_SYNC, 0666);
    if (fd < 0)
    {
        LE_ERROR("Open file %s failed", TAF_HAL_FILE_NAME);
        return 1;
    }
    ssize_t wrote = write(fd, &timeVal, sizeof(struct TimeSpec));
    if (wrote < 0 || wrote != (ssize_t)sizeof(struct TimeSpec))
    {
        LE_ERROR("Writing to file %s failed", TAF_HAL_FILE_NAME);
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}

static int ReadTimeFromFile(struct TimeSpec* timeVal)
{
    int fd = open(TAF_HAL_FILE_NAME, O_RDONLY);
    if (fd < 0)
    {
        LE_ERROR("Open file %s failed", TAF_HAL_FILE_NAME);
        return 1;
    }
    ssize_t rd = read(fd, timeVal, sizeof(struct TimeSpec));
    if (rd < 0 || rd != (ssize_t)sizeof(struct TimeSpec))
    {
        LE_ERROR("Read from %s failed", TAF_HAL_FILE_NAME);
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}

static void taf_hal_PowerOn()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return;
}

static void taf_hal_PowerOff()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return;
}

static int taf_hal_HwInit()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return 0;
}

static void taf_hal_Sleep()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return;
}

static void tal_hal_Wakeup()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return;
}

static int taf_hal_SelfTest()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return 0;
}

static le_result_t tal_hal_GetRtcTime(struct TimeSpec* timeVal)
{
    int ret;
    struct tm rtc_tm;
    memset(&rtc_tm, 0, sizeof(struct tm));

#ifdef TAF_HAL_RTC_IS_READ_ONLY

    struct timespec currBootTime = {0,0};
    le_result_t result = GetBootTime(&currBootTime);
    if (result != LE_OK)
    {
        timeVal->sec = 0;
        timeVal->nanosec = 0;
        LE_ERROR("Unable to read system boot time");
        return LE_FAULT;
    }

    struct TimeSpec timeInFile = {0, 0};
    ret = ReadTimeFromFile(&timeInFile);
    if (ret != 0 || timeInFile.sec <= 0)
    {
        LE_ERROR("%s, simulate read RTC time failed when reading from file", __func__);
        timeVal->sec = 0;
        timeVal->nanosec = 0;
        return LE_FAULT;
    }

    time_t deltaSec = currBootTime.tv_sec - bootTimeWhenSetRTCTime.tv_sec;
    if (deltaSec < 0) deltaSec = 0;
    timeVal->sec = timeInFile.sec + (uint64_t)deltaSec;
    timeVal->nanosec = 0; //dropping the accuracy for simulation test

    LE_INFO("The RTC time is: %"PRIu64", deltaSec: %"PRIu64"", timeVal->sec, deltaSec);

#else
    LE_INFO("TestDrv: %s", __FUNCTION__);
    time_t secs = 0;
    int fd;
    do
    {
        fd = TEMP_FAILURE_RETRY(open(TAF_HAL_RTC_DEV_NAME, O_RDONLY | O_CLOEXEC));
        if (fd < 0)
        {
            fd = -errno;
        }
    } while (fd == -EBUSY);

    if (fd < 0)
    {
        LE_ERROR("%s, open %s failed\n", __func__, TAF_HAL_RTC_DEV_NAME);
        return LE_FAULT;
    }

    do
    {
        ret = TEMP_FAILURE_RETRY(ioctl(fd, RTC_RD_TIME, &rtc_tm));
        if (ret < 0) {
            ret = -errno;
        }
    } while (ret == -EBUSY);

    LE_INFO("%s: Time read from RTC -- year = %d, month = %d,"
        "day = %d\n", __func__, rtc_tm.tm_year, rtc_tm.tm_mon,
        rtc_tm.tm_mday);
    close(fd);
    if (ret < 0)
    {
        LE_ERROR("%s, read RTC failed\n", __func__);
        return LE_FAULT;
    }

    /* Convert to UTC time */
    secs = mktime(&rtc_tm);
    secs += rtc_tm.tm_gmtoff;
    if (secs < 0) {
        LE_ERROR("Invalid RTC seconds = %ld\n", secs);
        return LE_FAULT;
    }

    timeVal->sec = secs;
    timeVal->nanosec = 0;
#endif

    return LE_OK;
}

static le_result_t tal_hal_SetRtcTime(struct TimeSpec timeVal)
{
    int ret = 0;
    timeVal.nanosec = 0; //dropping the accuracy for simulation test

#ifdef TAF_HAL_RTC_IS_READ_ONLY
    /*
     * Since the RTC in QC was set to read only, so the RTC time cannot
     * be set. Here define "TAF_HAL_RTC_IS_READ_ONLY" to use write file
     * instead of write RTC for verifying the write logic.
     */
    le_result_t result = GetBootTime(&bootTimeWhenSetRTCTime);
    if (result)
    {
        LE_ERROR("Unable to read system boot time\n");
    }
    ret = WriteTimeToFile(timeVal);
    if (ret != 0)
    {
        LE_ERROR("%s, simulate set RTC failed in writing file\n", __func__);
        return LE_FAULT;
    }

    LE_INFO("Update rtc time to: %"PRIu64", nanosec: %"PRIu64" to RTC\n",
        timeVal.sec, timeVal.nanosec);
#else
    int fd;
    do
    {
        fd = TEMP_FAILURE_RETRY(open(TAF_HAL_RTC_DEV_NAME, O_WRONLY));
        if (fd < 0)
        {
            fd = -errno;
        }
    } while (fd == -EBUSY);

    if (fd < 0)
    {
        LE_ERROR("%s, open %s failed\n", __func__, TAF_HAL_RTC_DEV_NAME);
        return LE_FAULT;
    }

    do
    {
        ret = TEMP_FAILURE_RETRY(ioctl(fd, RTC_SET_TIME, timeVal));
        if (ret < 0) {
            ret = -errno;
        }
    } while (ret == -EBUSY);
    close(fd);

    if (ret < 0)
    {
        LE_ERROR("%s, write RTC failed\n", __func__);
        return LE_FAULT;
    }

#endif

    return LE_OK;
}

static void* taf_hal_GetModInf(void)
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return &(TAF_HAL_INFO_TAB.timeInf);
}

static void GetRTCRespHandler (void* context)
{
    struct TimeSpec timeVal = ((GetRTCRequest_t*)context)->timeVal;
    le_result_t responseState = ((GetRTCRequest_t*)context)->responseState;
    if (getRTCAsyncCallbackFunc)
    {
        getRTCAsyncCallbackFunc(timeVal, responseState);
    }
    else
    {

        LE_ERROR("get RTC callback function is NULL");
    }
    return;
}

static void ProcessGetRTCRequest(void* param1,void* param2)
{
    LE_UNUSED(param2);
    GetRTCRequest_t* req = (GetRTCRequest_t*)(param1);

    req->responseState = LE_FAULT;

#ifdef TAF_HAL_RTC_IS_READ_ONLY
    struct timespec currBootTime = {0, 0};
    le_result_t result = GetBootTime(&currBootTime);
    if (result != LE_OK)
    {
        req->timeVal.sec = 0;
        req->timeVal.nanosec = 0;
        LE_ERROR("Unable to read system boot time");
        return;
    }

    struct TimeSpec timeInFile = {0, 0};
    int ret = ReadTimeFromFile(&timeInFile);
    if (ret != 0 || timeInFile.sec <= 0)
    {
        LE_ERROR("%s, simulate read RTC time failed when reading from file", __func__);
        req->timeVal.sec = 0;
        req->timeVal.nanosec = 0;
        return;
    }

    time_t deltaSec = currBootTime.tv_sec - bootTimeWhenSetRTCTime.tv_sec;
    if (deltaSec < 0) deltaSec = 0;
    req->timeVal.sec = timeInFile.sec + (uint64_t)deltaSec;
    req->timeVal.nanosec = 0; //dropping the accuracy for simulation test
    req->responseState = LE_OK;

#else
    //Get the RTC value
    req->timeVal.sec = 1712345678;
    req->timeVal.nanosec = 10086;
    req->responseState = LE_OK;
#endif

    le_event_Report(GetRTCRequestEventId, (void*)req, sizeof(GetRTCRequest_t));
    le_mem_Release(req);
}

static le_result_t taf_hal_getRtcTimeReqAsync(TAF_HAL_GETRTCASYNCCALLBACK callback)
{
    getRTCAsyncCallbackFunc = callback;
    GetRTCRequest_t* req = (GetRTCRequest_t*)le_mem_ForceAlloc(GetRTCRequestPoolRef);
    le_event_QueueFunction(ProcessGetRTCRequest, (void*)(req), NULL);
    return LE_OK;
}

static void SetRTCRespHandler(void* context)
{
    le_result_t responseState = ((SetRTCRequest_t*)context)->responseState;
    if (setRTCAsyncCallbackFunc)
    {
        setRTCAsyncCallbackFunc(responseState);
    }
    else
    {
        LE_ERROR("set RTC callback function is NULL");
    }
    return;
}

static void ProcessSetRTCRequest(void* param1, void* param2)
{
    SetRTCRequest_t* req = (SetRTCRequest_t*)(param1);
    //Set the return value of response
    req->responseState = LE_OK;
    le_event_Report(SetRTCRequestEventId, (void*)req, sizeof(SetRTCRequest_t));
    le_mem_Release(req);
}

static le_result_t taf_hal_setRtcTimeReqAsync(const struct TimeSpec* timeVal,
    TAF_HAL_SETRTCASYNCCALLBACK callback)
{
    setRTCAsyncCallbackFunc = callback;

    LE_DEBUG("VHAL received new time:  %"PRIu64".%"PRIu64" trying to update to RTC",
        timeVal->sec, timeVal->nanosec);
#ifdef TAF_HAL_RTC_IS_READ_ONLY
    le_result_t result = GetBootTime(&bootTimeWhenSetRTCTime);
    if (result != LE_OK)
    {
        LE_ERROR("Unable to read system boot time");
    }
    /*
     * Since the RTC in QC was set to read only, so the RTC time cannot
     * be set. Here define "TAF_HAL_RTC_IS_READ_ONLY" to use write file
     * instead of write RTC Hardware device for verifying the write logic.
     */
    int ret = WriteTimeToFile(*timeVal);
    if (ret != 0)
    {
        LE_ERROR("%s, simulate async set RTC failed in writing file\n", __func__);
    }
#endif

    SetRTCRequest_t* req = (SetRTCRequest_t*)le_mem_ForceAlloc(setRTCRequestPoolRef);
    le_event_QueueFunction(ProcessSetRTCRequest, (void*)(req), NULL);
    return LE_OK;
}

static void taf_hal_Init(void)
{
    GetRTCRequestPoolRef = le_mem_CreatePool("GetRTCRequest", sizeof(GetRTCRequest_t));
    GetRTCRequestEventId = le_event_CreateId("GetRTCRequestEventId", sizeof(GetRTCRequest_t));
    // Register handler for get RTC asyn response events.
      le_event_AddHandler("GetRTCRespHandler", GetRTCRequestEventId, GetRTCRespHandler);

      setRTCRequestPoolRef = le_mem_CreatePool("SetRTCRequest", sizeof(SetRTCRequest_t));
      SetRTCRequestEventId = le_event_CreateId("SetRTCRequestEventId", sizeof(SetRTCRequest_t));
      // Register handler for set RTC asyn response events.
      le_event_AddHandler("SetRTCRespHandler", SetRTCRequestEventId, SetRTCRespHandler);
      return;
}

LE_SHARED time_InfoTab_t TAF_HAL_INFO_TAB = {
    // always come first
    .mgrInf = {
        .name = TAF_TIME_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .hwInitInf = taf_hal_HwInit,
        .powerOffInf = taf_hal_PowerOff,
        .powerOnInf = taf_hal_PowerOn,
        .selfTest = taf_hal_SelfTest, // tafModule will send test command
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    },

    .timeInf = {
        .InitHAL = taf_hal_Init,
        .sleepHAL = taf_hal_Sleep,
        .wakeUpHAL = tal_hal_Wakeup,

        // IMPORTANT: Do not mix asynchronous and synchronous APIs.
        // Below synchronous API pair must be used together.
        .getRtcTimeHAL = tal_hal_GetRtcTime,
        .setRtcTimeHAL = tal_hal_SetRtcTime,

        // Below asynchronous API pair must be used together.
        .getRtcTimeReqAsync = taf_hal_getRtcTimeReqAsync,
        .setRtcTimeReqAsync = taf_hal_setRtcTimeReqAsync,
    },
};

// DOT NOT USE COMPONET_INIT define module init for your service/apps
COMPONENT_INIT
{
    // Do not put your specfici init in here, define your init
    LE_INFO("Time test Drv is loading\n");
}
