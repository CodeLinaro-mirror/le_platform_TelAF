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

static le_mem_PoolRef_t AsyncRtcEventPoolRef;
static le_event_Id_t    AsyncRTCRequestEventId;

typedef enum
{
    RTC_EVT_SET = 0,
    RTC_EVT_GET = 1,
} RtcEvtType;

typedef struct
{
    RtcEvtType   type;           // distinguish GET vs SET
    struct TimeSpec timeVal;     // valid for GET, optional for SET
    le_result_t  responseState;  // LE_OK / LE_FAULT / LE_UNAVAILABLE, etc.
} RtcEvent_t;

struct timespec bootTimeOnSetRTCTime = (struct timespec){0, 0};

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

    struct timespec presentBootTime = {0,0}, deltaBootTime = {0,0};
    le_result_t result = GetBootTime(&presentBootTime);
    if (result)
    {
        LE_ERROR("Unable to read system boot time\n");
    }
    deltaBootTime.tv_sec = presentBootTime.tv_sec - bootTimeOnSetRTCTime.tv_sec;
    deltaBootTime.tv_nsec = presentBootTime.tv_nsec - bootTimeOnSetRTCTime.tv_nsec;

    struct TimeSpec timeInFile;
    ret = ReadTimeFromFile(&timeInFile);
    if (ret != 0)
    {
        LE_ERROR("%s, simulate read RTC time failed when reading from file\n", __func__);
        return LE_FAULT;
    }

    if (timeInFile.sec < 0)
    {
        LE_ERROR("%s, time in file not correct\n", __func__);
        return LE_FAULT;
    }

    LE_INFO("The RTC time is: %"PRIu64", nanosec: %"PRIu64" from RTC file\n",
        timeInFile.sec + deltaBootTime.tv_sec, timeInFile.nanosec + deltaBootTime.tv_nsec);

    timeVal->sec = timeInFile.sec + deltaBootTime.tv_sec;
    timeVal->nanosec = 0; //dropping the accuracy for simulation test

#else
    LE_INFO("TestDrv: %s", __FUNCTION__);

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
    le_result_t result = GetBootTime(&bootTimeOnSetRTCTime);
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

static void RtcRespHandler(void* context)
{
    RtcEvent_t* evt = (RtcEvent_t*)context;

    switch (evt->type)
    {
        case RTC_EVT_SET:
            if (setRTCAsyncCallbackFunc)
            {
                setRTCAsyncCallbackFunc(evt->responseState);
            }
            else
            {
                LE_WARN("SET RTC callback is NULL.");
            }
            break;

        case RTC_EVT_GET:
            if (getRTCAsyncCallbackFunc)
            {
                getRTCAsyncCallbackFunc(evt->timeVal, evt->responseState);
            }
            else
            {
                LE_WARN("GET RTC callback is NULL.");
            }
            break;

        default:
            LE_ERROR("Unknown RTC event type: %d", (int)evt->type);
            break;
    }
}

static void ProcessGetRTCRequest(void* param1,void* param2)
{
    LE_UNUSED(param2);
    RtcEvent_t* evt = (RtcEvent_t*)param1;
    if (evt == NULL)
    {
        LE_ERROR("Parameter 'evt' is NULL");
        return;
    }

    evt->type          = RTC_EVT_GET;
    evt->responseState = LE_UNAVAILABLE;

#ifdef TAF_HAL_RTC_IS_READ_ONLY
    struct timespec presentBootTime = {0, 0};
    le_result_t result = GetBootTime(&presentBootTime);
    if (result != LE_OK)
    {
        LE_ERROR("Unable to read system boot time");
    }

    struct TimeSpec timeInFile = {0, 0};
    int ret = ReadTimeFromFile(&timeInFile);
    if (ret != 0 || timeInFile.sec <= 0)
    {
        LE_ERROR("%s, time in file not correct\n", __func__);
        evt->timeVal.sec     = 0;
        evt->timeVal.nanosec = 0;
        evt->responseState   = LE_FAULT;
    }
    else
    {
        time_t deltaSec = presentBootTime.tv_sec - bootTimeOnSetRTCTime.tv_sec;
        if (deltaSec < 0) deltaSec = 0;
        evt->timeVal.sec     = timeInFile.sec + (uint64_t)deltaSec;
        evt->timeVal.nanosec = 0;
        evt->responseState   = LE_OK;
    }

#else
    evt->timeVal.sec = 1712345678;
    evt->timeVal.nanosec = 10086;
    evt->responseState = LE_OK;
#endif

    le_event_Report(AsyncRTCRequestEventId, (void*)evt, sizeof(RtcEvent_t));
    le_mem_Release(evt);
}

static le_result_t taf_hal_getRtcTimeReqAsync(TAF_HAL_GETRTCASYNCCALLBACK callback)
{
    getRTCAsyncCallbackFunc = callback;

    RtcEvent_t* evt = (RtcEvent_t*)le_mem_ForceAlloc(AsyncRtcEventPoolRef);
    le_event_QueueFunction(ProcessGetRTCRequest, (void*)evt, NULL);

    return LE_OK;
}

static void ProcessSetRTCRequest(void* param1, void* param2)
{
    LE_UNUSED(param2);
    RtcEvent_t* evt = (RtcEvent_t*)param1;
    if (evt == NULL)
    {
        LE_ERROR("Parameter 'evt' is NULL");
        return;
    }

    evt->type          = RTC_EVT_SET;
    evt->responseState = LE_OK;

    le_event_Report(AsyncRTCRequestEventId, (void*)evt, sizeof(RtcEvent_t));
    le_mem_Release(evt);
}

static le_result_t taf_hal_setRtcTimeReqAsync(const struct TimeSpec* timeVal,
    TAF_HAL_SETRTCASYNCCALLBACK callback)
{
    setRTCAsyncCallbackFunc = callback;

    LE_DEBUG("VHAL received new time:  %"PRIu64".%"PRIu64" trying to update to RTC",
        timeVal->sec, timeVal->nanosec);
#ifdef TAF_HAL_RTC_IS_READ_ONLY
    le_result_t result = GetBootTime(&bootTimeOnSetRTCTime);
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

   // Please don't set new time to RTC Hardware device in this function, set it
   // in function "ProcessSetRTCRequest" to avoid long time response.
    RtcEvent_t* evt = (RtcEvent_t*)le_mem_ForceAlloc(AsyncRtcEventPoolRef);
    le_event_QueueFunction(ProcessSetRTCRequest, (void*)evt, NULL);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Why to use this thread "VhalCbHandlerThread":
 * Simulate sending event (cb) back to the service in different thread. But suggest not to send the
 * callback in different thread from vhal to service to avoid below issue:
 * "Legato threading API used in non-Legato thread" issue.
 */
//--------------------------------------------------------------------------------------------------
static void* VhalCbHandlerThread
(
    void* contextPtr
)
{
    LE_UNUSED(contextPtr);
    LE_INFO("rtc_vhal: %s starting", __FUNCTION__);

    le_event_AddHandler("RtcRespHandler", AsyncRTCRequestEventId, RtcRespHandler);
    le_event_RunLoop();

    LE_ERROR("VhalCbHandlerThread: event loop exited unexpectedly.");
    return NULL;
}

static void taf_hal_Init(void)
{
    AsyncRtcEventPoolRef   = le_mem_CreatePool("RtcEventPool", sizeof(RtcEvent_t));
    AsyncRTCRequestEventId = le_event_CreateId("AsyncRTCRequestEventId", sizeof(RtcEvent_t));

    le_thread_Ref_t threadRef = le_thread_Create("VhalCbHandlerTh", VhalCbHandlerThread, NULL);
    if (!threadRef)
    {
        LE_ERROR("Failed to create VHAL callback thread");
        return;
    }
    le_thread_Start(threadRef);
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
        .getRtcTimeHAL = tal_hal_GetRtcTime,
        .setRtcTimeHAL = tal_hal_SetRtcTime,
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
