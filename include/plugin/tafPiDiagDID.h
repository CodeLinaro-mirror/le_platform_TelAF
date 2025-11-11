/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __TAFDIAGDID_H__

#define __TAFDIAGDID_H__

#include "tafHalIF.hpp"

// Diag DID module name
#define TAF_DIAGDID_MODULE_NAME "TafPiDiagDID"

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
 *
 * @instaging
 * @param    dataID      DID
 * @param    value       Value of the DID
 * @param    len         Length of the DID value
 * @param    result      Returned error code of getting DID value
 * @param    ctxPtr      context pointer
 *
 * @return void
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_PI_DIAGDID_GETHANDLER)
(
    uint16_t dataID,
    uint8_t *value,
    size_t len,
    uint8_t result,
    void* ctxPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets DID value asynchronously.
 *
 * @instaging
 * @param    dataID      DID
 * @param    handler     Callback handler
 * @param    ctxPtr      Context pointer
 *
 * @return
 *      result of registering call back
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_PI_DIAGDID_GETASYNC)
(
    uint16_t dataID,
    TAF_PI_DIAGDID_GETHANDLER handler,
    void* ctxPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Hander to return the result of setting DID request.
 *
 * @instaging
 * @param    dataID     DID
 * @param    result     Returned error code of setting DID value
 * @param    ctxPtr     Context pointer
 *
 * @return void
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_PI_DIAGDID_SETHANDLER)
(
    uint16_t dataID,
    uint8_t result,
    void* ctxPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Sets DID value asynchronously.
 *
 * @instaging
 * @param    dataID      DID
 * @param    context     Handler function context
 * @param    handler     Callback handler
 * @param    ctxPtr      Context pointer
 *
 * @return
 *      result of registering call back
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_PI_DIAGDID_SETASYNC)
(
    uint16_t dataID,
    uint8_t *value,
    size_t len,
    TAF_PI_DIAGDID_SETHANDLER handler,
    void* ctxPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Handler to notify the changed DID and its value.
 *
 * @instaging
 * @param    dataID      Changed DID
 * @param    value       Value of the DID
 * @param    len         Length of the DID value
 *
 * @return void
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_PI_DIAGDID_DATACHANGECALLBACK)
(
    uint16_t dataID,
    uint8_t *value,
    size_t len
);

//--------------------------------------------------------------------------------------------------
/**
 * Add data change handler to hal module.
 * (The DID change will not be notify, if service has not whitelisted the DID using
 * TAF_PI_DIAGDID_ADDDATACHANGENOTIFICATION API.)
 *
 * @instaging
 * @param    callback    Callback function pointer for notification
 * @return
 *      result for adding the handler
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_PI_DIAGDID_ADDDATACHANGEHANDLER)
(
    TAF_PI_DIAGDID_DATACHANGECALLBACK callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Add DID for data change notification
 *
 * @instaging
 * @param    dataID     Specified DID that is added for notification
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_DUPLICATE -- Data identifier already added.
 *     - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_PI_DIAGDID_ADDDATACHANGENOTIFICATION)
(
    uint16_t dataID
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove DID for data change notification
 *
 * @instaging
 * @param    dataID      Specified DID that is removed for notification
 * @return
 *      result for removing the DID for notification
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_PI_DIAGDID_REMOVEDATACHANGENOTIFICATION)
(
    uint16_t dataID
);

typedef struct
{
    INIT init;

    TAF_PI_DIAGDID_GETASYNC diagDIDGetAsync;

    TAF_PI_DIAGDID_SETASYNC diagDIDSetAsync;

    TAF_PI_DIAGDID_ADDDATACHANGEHANDLER addDataChangeHandler;

    TAF_PI_DIAGDID_ADDDATACHANGENOTIFICATION diagDIDAddDataChangeNotification;

    TAF_PI_DIAGDID_REMOVEDATACHANGENOTIFICATION diagDIDRemoveDataChangeNotification;

} diagDID_Inf_t;

typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf;   // management interface for device manager
    diagDID_Inf_t diagInf;      // module interface for application/service

} diagDID_InfoTab_t;

extern diagDID_InfoTab_t TAF_HAL_INFO_TAB;

#endif