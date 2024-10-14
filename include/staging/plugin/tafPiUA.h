/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFPIUA_H
#define TAFPIUA_H

#include "tafHalIF.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Update agent module name.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_UA_MODULE_NAME "TafPiUA"

 //--------------------------------------------------------------------------------------------------
/**
 * Update agent type enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PI_UA_TYPE_DELTA_XDELTA, ///< Delta update with XDelta.
    TAF_PI_UA_TYPE_DELTA_BSDIFF  ///< Delta update with bsdiff.
} taf_pi_ua_UAType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Update status enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PI_UA_STATUS_INIT,     ///< Update initialized.
    TAF_PI_UA_STATUS_UPDATING, ///< Updating.
    TAF_PI_UA_STATUS_PAUSED,   ///< Update paused.
    TAF_PI_UA_STATUS_FINISH,   ///< Update finished.
    TAF_PI_UA_STATUS_ERROR     ///< Update failed with error.
} taf_pi_ua_Status_t;

//--------------------------------------------------------------------------------------------------
/**
 * Update session reference.
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_pi_ua_Session* taf_pi_ua_SessionRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Initialize update agent.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_PI_UA_INIT)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Get update session.
 *
 * @param[in]  confFile Update session configuration file.
 * @param[out] sessRef  Update session reference.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_UA_GET_SESSION)(const char* confFile, taf_pi_ua_SessionRef_t* sessRef);

//--------------------------------------------------------------------------------------------------
/**
 * Get update agent type.
 *
 * @param[in] sessRef Update session reference.
 * @param[out] uaType Updater agent type.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_UA_GET_UA_TYPE)(taf_pi_ua_SessionRef_t sessRef, taf_pi_ua_UAType_t* uaType);

//--------------------------------------------------------------------------------------------------
/**
 * Start install package
 *
 * @param[in]  sessRef Update session reference.
 * @param[in]  pkgFile Package file for installation.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_UA_START_INSTALL)(taf_pi_ua_SessionRef_t sessRef, const char* pkgFile);

//--------------------------------------------------------------------------------------------------
/**
 * Pause installation
 *
 * @param[in]  sessRef Update session reference.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_UA_PAUSE_INSTALL)(taf_pi_ua_SessionRef_t sessRef);

//--------------------------------------------------------------------------------------------------
/**
 * Resume installation
 *
 * @param[in]  sessRef Update session reference.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_UA_RESUME_INSTALL)(taf_pi_ua_SessionRef_t sessRef);

//--------------------------------------------------------------------------------------------------
/**
 * Get update progress.
 *
 * @param[in]  sessRef Update session reference.
 * @param[out] status  Update status.
 * @param[out] percent Update percentage.
 * @param[out] error   Error code in update process.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_UA_GET_PROGRESS)(taf_pi_ua_SessionRef_t sessRef, taf_pi_ua_Status_t* status,
    int* percent, int* error);

//--------------------------------------------------------------------------------------------------
/**
 * Update agent information structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    // Initialization.
    TAF_PI_UA_INIT init;

    // Update session.
    TAF_PI_UA_GET_SESSION getSess;
    TAF_PI_UA_GET_UA_TYPE getUAType;

    // Update action.
    TAF_PI_UA_START_INSTALL startInstall;
    TAF_PI_UA_PAUSE_INSTALL pauseInstall;
    TAF_PI_UA_RESUME_INSTALL resumeInstall;

    // Update status.
    TAF_PI_UA_GET_PROGRESS getProgress;
} ua_Inf_t;

//--------------------------------------------------------------------------------------------------
/**
 * Update agent information table structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf; ///< Device manager information.
    ua_Inf_t uaInf;           ///< Update agent information.
} ua_InfoTab_t;

#endif