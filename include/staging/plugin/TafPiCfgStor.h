/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __TAF_PI_CFGSTOR_H__
#define __TAF_PI_CFGSTOR_H__

#include "tafHalIF.hpp"


//--------------------------------------------------------------------------------------------------
/**
 * Config storage plugin module name.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_CFGSTOR_MODULE_NAME "TafPiCfgStor"

//--------------------------------------------------------------------------------------------------
/**
 * Initialize config storage plugin module.
 * @param
 *
 * @return
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pi_cfgStor_init_t)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Authenticate file.
 * @param
 *      filePath    File to be authenticated
 *
 * @return
 *      result of the authentication
 *
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*pi_cfgStor_auth_t)
(
    const char* filePath
);

//--------------------------------------------------------------------------------------------------
/**
 * Merge file.
 * @param
 *      outputFilePath    - the output file
 *      configFilePath    - the file to be merged
 *
 * @return
 *      result of the merge operation
 *
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*pi_cfgStor_merge_t)
(
    const char* outputFilePath,
    const char* configFilePath
);

//--------------------------------------------------------------------------------------------------
/**
 * Config storage plugin information structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    // Initialization.
    pi_cfgStor_init_t init;

    // Authenticate file.
    pi_cfgStor_auth_t auth;

    // Merge file.
    pi_cfgStor_merge_t merge;

} cfgStor_Inf_t;

//--------------------------------------------------------------------------------------------------
/**
 * Config storage plugin information table structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf; ///< Device manager information.
    cfgStor_Inf_t cfgStorInf; ///< Config storage plugin information.
} pi_cfgStor_InfoTab_t;

extern pi_cfgStor_InfoTab_t TAF_HAL_INFO_TAB;


#endif