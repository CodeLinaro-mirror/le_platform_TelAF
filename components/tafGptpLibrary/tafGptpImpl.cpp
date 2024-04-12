/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#include <iostream>

#include "tafGptp.hpp"

#define TAF_TIME_CLOCKFD                   3
#define TAF_TIME_FD_TO_CLOCKID(fd)        ((~(clockid_t) (fd) << 3) | TAF_TIME_CLOCKFD)
#define TAF_TIME_LOCAL_PTP_NODE           "/dev/ptp0"

//--------------------------------------------------------------------------------------------------
/**
 * Get local ptp time.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_NOT_FOUND -- if file not exist.
 *     - LE_FAULT -- if any other error occurs.
 */
//--------------------------------------------------------------------------------------------------
extern "C" LE_SHARED le_result_t taf_time_GetLocalPtpTime
(
    struct timespec* timeValPtr
)
{
    struct timespec ts;
    clockid_t clkid;

    int fd = open(TAF_TIME_LOCAL_PTP_NODE, O_RDONLY);
    if (-1 == fd)
    {
        LE_WARN ("Open %s failed: %d, %s", TAF_TIME_LOCAL_PTP_NODE,
                                                errno, strerror(errno));
        return LE_NOT_FOUND;
    }

    clkid = TAF_TIME_FD_TO_CLOCKID(fd);
    if (-1 == clock_gettime(clkid, &ts))
    {
        LE_WARN("Get ptp time failed: %d, %s", errno, strerror(errno));
        close(fd);
        return LE_FAULT;
    }
    timeValPtr->tv_sec = ts.tv_sec;
    timeValPtr->tv_nsec = ts.tv_nsec;

    close(fd);
    return LE_OK;
}

COMPONENT_INIT
{
    LE_INFO("gPTP component initialization done\n");
}

