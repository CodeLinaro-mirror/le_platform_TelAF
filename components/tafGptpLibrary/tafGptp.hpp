/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_PTP_LIB_H
#define TAF_PTP_LIB_H

#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

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
LE_SHARED le_result_t taf_time_GetLocalPtpTime
(
    timespec* timeVal
);


#ifdef __cplusplus
}
#endif

#endif // TAF_PTP_LIB_H

