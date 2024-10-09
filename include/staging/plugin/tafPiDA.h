/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFPIDA_H
#define TAFPIDA_H

#include "tafHalIF.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Download agent module name.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_DA_MODULE_NAME "TafPiDA"

//--------------------------------------------------------------------------------------------------
/**
 * Download session reference.
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_pi_da_Session* taf_pi_da_SessionRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Download status enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PI_DA_STATUS_INIT,        ///< Download initialized.
    TAF_PI_DA_STATUS_DOWNLOADING, ///< Downloading.
    TAF_PI_DA_STATUS_PAUSED,      ///< Download paused.
    TAF_PI_DA_STATUS_FINISH,      ///< Download finished.
    TAF_PI_DA_STATUS_ERROR        ///< Download failed with error.
} taf_pi_da_Status_t;

//--------------------------------------------------------------------------------------------------
/**
 * Initialize download agent.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_PI_DA_INIT)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Get download session.
 *
 * @param[in]  confFile Download session configuration file.
 * @param[out] sessRef  Download session reference.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_DA_GET_SESSION)(const char* confFile, taf_pi_da_SessionRef_t* sessRef);

//--------------------------------------------------------------------------------------------------
/**
 * Start download.
 *
 * @param[in] sessRef  Download session reference.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_DA_START)(taf_pi_da_SessionRef_t sessRef);

//--------------------------------------------------------------------------------------------------
/**
 * Pause download.
 *
 * @param[in] sessRef Download session reference.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_DA_PAUSE)(taf_pi_da_SessionRef_t sessRef);

//--------------------------------------------------------------------------------------------------
/**
 * Resume download.
 *
 * @param[in] sessRef Download session reference.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_DA_RESUME)(taf_pi_da_SessionRef_t sessRef);

//--------------------------------------------------------------------------------------------------
/**
 * Cancel download.
 *
 * @param[in] sessRef Download session reference.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_DA_CANCEL)(taf_pi_da_SessionRef_t sessRef);

//--------------------------------------------------------------------------------------------------
/**
 * Get download progress.
 *
 * @param[in]  sessRef Download session reference.
 * @param[out] status  Download status.
 * @param[out] percent Download percentage.
 * @param[out] error   Error code in download process
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_DA_GET_PROGRESS)(taf_pi_da_SessionRef_t sessRef,
    taf_pi_da_Status_t* status, int* percent, int* error);

//--------------------------------------------------------------------------------------------------
/**
 * Download agent information structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    // Initialization.
    TAF_PI_DA_INIT init;

    // Download session.
    TAF_PI_DA_GET_SESSION getSess;

    // Download action.
    TAF_PI_DA_START startDownload;
    TAF_PI_DA_PAUSE pauseDownload;
    TAF_PI_DA_RESUME resumeDownload;
    TAF_PI_DA_CANCEL cancelDownload;

    // Download status.
    TAF_PI_DA_GET_PROGRESS getProgress;
} da_Inf_t;

//--------------------------------------------------------------------------------------------------
/**
 * Download agent information table structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf; ///< Device manager information.
    da_Inf_t daInf;           ///< Download agent information.
} da_InfoTab_t;

#endif