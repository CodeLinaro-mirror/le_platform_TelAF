/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __TAFDIAGDID_H__

#define __TAFDIAGDID_H__

#include "tafHalIF.hpp"

// Define the name or ID for HAL module
#define TAF_DIAGDID_MODULE_NAME "TafHalDiagDID"

//--------------------------------------------------------------------------------------------------
/**
 * Initializes all the required resources
 * @param void
 *
 * @return void
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*INIT)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Handler to return the value of getting DID request.
 * @param
 *      dataID     - DID
 *      value       - the value of the DID
 *      len         - the length of the DID value
 *
 * @return void
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_HAL_DIAGDID_GETHANDLER)
(
    uint16_t dataID,
    uint8_t *value,
    size_t len
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets DID value asynchronously.
 * @param
 *      dataID     - DID
 *      handler     - callback handler
 *
 * @return
 *      result of registering call back
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_DIAGDID_GETASYNC)
(
    uint16_t dataID,
    TAF_HAL_DIAGDID_GETHANDLER handler
);

//--------------------------------------------------------------------------------------------------
/**
 * Hander to return the result of setting DID request.
 * @param
 *      dataID    - DID
 *      result     - result of setting DID value
 *
 * @return void
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_HAL_DIAGDID_SETHANDLER)
(
    uint16_t dataID,
    le_result_t result
);

//--------------------------------------------------------------------------------------------------
/**
 * Sets DID value asynchronously.
 * @param
 *      dataID      - DID
 *      context     - handler function context
 *      handler     - callback handler
 *
 * @return
 *      result of registering call back
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_DIAGDID_SETASYNC)
(
    uint16_t dataID,
    uint8_t *value,
    size_t len,
    TAF_HAL_DIAGDID_SETHANDLER handler
);

typedef struct
{
    INIT initHAL;

    TAF_HAL_DIAGDID_GETASYNC diagDIDGetAsync;

    TAF_HAL_DIAGDID_SETASYNC diagDIDSetAsync;

} diagDID_Inf_t;

typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf;   // management interface for device manager
    diagDID_Inf_t diagInf;      // module interface for application/service

} diagDID_InfoTab_t;

extern diagDID_InfoTab_t TAF_HAL_INFO_TAB;

#endif