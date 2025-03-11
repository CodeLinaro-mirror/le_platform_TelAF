/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __TIME_H__

#define __TIME_H__

#include "tafHalIF.hpp"
#include <stdint.h>

 // Define the name or ID for HAL module
#define TAF_TIME_MODULE_NAME "TafHalTime"

//--------------------------------------------------------------------------------------------------
/**
 * Identifies the type of timespec.
 */
 //--------------------------------------------------------------------------------------------------
struct TimeSpec
{
    uint64_t sec;      ///< Seconds since epoch
    uint64_t nanosec;  ///< Nanoseconds
};

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Time VHAL driver.
 * @param void
 *
 * @return void
 */
 //--------------------------------------------------------------------------------------------------
typedef void (*TAF_HAL_INIT)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Request to get the RTC time.
 *
 * @instaging
 * @param
 *      timeVal        TimeSpec structure to store the RTC time.
 *
 * @return
 *      Returns for the request.
 */
 //--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_GETRTCTIME)(struct TimeSpec* timeVal);

//--------------------------------------------------------------------------------------------------
/**
 * Request to set the RTC time.
 *
 * @instaging
 * @param
 *      timeVal        TimeSpec structure to store the RTC time.
 *
 * @return
 *      Returns for the request.
 */
 //--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_SETRTCTIME)(struct TimeSpec timeVal);

//--------------------------------------------------------------------------------------------------
/**
 * Callback function to get the RTC time asynchronously.
 *
 * @instaging
 * @param    timVal         TimeSpec structure to store the RTC time.
 * @param    responseState  State of the response
 *
 * @return
 *      void
 */
 //--------------------------------------------------------------------------------------------------
typedef void(*TAF_HAL_GETRTCASYNCCALLBACK)
(
    struct TimeSpec timeVal, le_result_t responseState
);

//--------------------------------------------------------------------------------------------------
/**
 * Request to get the RTC time asynchronously.
 *
 * @instaging
 * @param
 *      callback        Callback function to response the request.
 *
 * @return
 *      void
 */
 //--------------------------------------------------------------------------------------------------
typedef le_result_t(*TAF_HAL_GETRTCTIMEREQASYNC)(TAF_HAL_GETRTCASYNCCALLBACK CALLBACK);

//--------------------------------------------------------------------------------------------------
/**
 * Callback function to set the RTC time asynchronously.
 *
 * @instaging
 * @param
 *      responseState State of the response
 *
 * @return
 *      void
 */
 //--------------------------------------------------------------------------------------------------
typedef void(*TAF_HAL_SETRTCASYNCCALLBACK)
(
    le_result_t responseState
);

//--------------------------------------------------------------------------------------------------
/**
 * Request to set the RTC time asynchronously.
 *
 * @instaging
 * @param    timeVal         TimeSpec structure to store the RTC time.
 * @param    callback        Callback function to response the request.
 *
 * @return
 *      void
 */
 //--------------------------------------------------------------------------------------------------
typedef le_result_t(*TAF_HAL_SETRTCTIMEREQASYNC)(const struct TimeSpec* timeVal,
    TAF_HAL_SETRTCASYNCCALLBACK CALLBACK);

typedef struct
{
    // hal module do the translation

    TAF_HAL_INIT InitHAL;

    // let the app/service to handle pm, not device manager
    TAF_HAL_SLEEP sleepHAL;
    TAF_HAL_WAKEUP wakeUpHAL;

    TAF_HAL_GETRTCTIME getRtcTimeHAL;
    TAF_HAL_GETRTCTIMEREQASYNC getRtcTimeReqAsync;
    TAF_HAL_SETRTCTIMEREQASYNC setRtcTimeReqAsync;
    TAF_HAL_SETRTCTIME setRtcTimeHAL;

} time_Inf_t;

typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf; // for device manager
    time_Inf_t timeInf; // for app/service

} time_InfoTab_t;

extern time_InfoTab_t TAF_HAL_INFO_TAB;

#endif
