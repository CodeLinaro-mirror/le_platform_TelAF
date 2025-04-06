/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "tafMngdConnAdmin.hpp"

using namespace tafsvc;

void Admin_init()
{
    auto &admin = tafMngdConnAdmin::GetInstance();
    admin.Init();

    LE_INFO("Admin_init...\n");
}

//--------------------------------------------------------------------------------------------------
/**
 ** Gets the data reference for the given Data ID.
 **
 ** @return
 **  - NULL -- Error.
 **  - Others -- The data reference.
 */
//--------------------------------------------------------------------------------------------------
taf_mngdConn_DataRef_t taf_mngdConn_GetData( uint8_t dataId )
{
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.GetRefByDataId(dataId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data reference for the given data name(as provided in configuration json).
 *
 * @return
 *  - NULL -- Error.
 *  - Others -- The data reference.
 */
//--------------------------------------------------------------------------------------------------
taf_mngdConn_DataRef_t taf_mngdConn_GetDataByName(const char* LE_NONNULL dataName)
{
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.GetRefByName(dataName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data object id (from configuration file) for the given data reference.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_NOT_FOUND -- Data reference not found.
 *   - Appropriate error is returned on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mngdConn_GetDataIdByRef
(
    taf_mngdConn_DataRef_t dataRef,
        ///< [IN] The data reference.
    uint8_t* dataIdPtr
        ///< [OUT] The data object id from configuration json.
)
{
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.GetDataIdByRef(dataRef, dataIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data object name (as provided in configuration json) for the given data reference.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_NOT_FOUND -- Data reference not found.
 *   - Appropriate error is returned on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mngdConn_GetDataNameByRef( taf_mngdConn_DataRef_t dataRef,
                                            char* dataName, size_t dataNameSize )
{
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.GetDataNameByRef(dataRef, dataName, dataNameSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data profile number (as provided in the configuration JSON) for the given data
 * reference. The profile number can be used by applications with the data call service to get
 * in depth details about the profile.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_NOT_FOUND -- Data reference not found.
 *   - Appropriate error is returned on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mngdConn_GetProfileNumberByRef
(
    taf_mngdConn_DataRef_t dataRef,
        ///< [IN] The data reference.
    uint8_t* dataProfileNumberPtr
        ///< [OUT] The data profile number from the configuration JSON.
)
{
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.GetProfileNumberByRef(dataRef, dataProfileNumberPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the phone id that is used by the data object (as provided in the configuration JSON).
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_NOT_FOUND -- Data reference not found.
 *   - Appropriate error is returned on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mngdConn_GetPhoneIdByRef
(
    taf_mngdConn_DataRef_t dataRef,
        ///< [IN] The data reference.
    uint8_t* phoneIdPtr
        ///< [OUT] The phone id used by the provided data reference.
)
{
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.GetPhoneIdByRef(dataRef, phoneIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Starts a data cellular session for the given dataRef.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_DUPLICATE -- The data connection is already started.
 *   - LE_IN_PROGRESS -- The data connection is retrying to create.
 *   - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mngdConn_StartData
(
    taf_mngdConn_DataRef_t dataRef
)
{
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.Startdata(dataRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Stops a data cellular session for the given dataRef.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mngdConn_StopData
(
    taf_mngdConn_DataRef_t dataRef
)
{
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.Stopdata(dataRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data connection state information for the given data reference.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_BAD_PARAMETER -- Bad parameter.
 *   - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mngdConn_GetDataConnectionState
(
    taf_mngdConn_DataRef_t dataRef,
    taf_mngdConn_DataState_t* statePtr
)
{
    auto &admin = tafMngdConnAdmin::GetInstance();

    return admin.GetConnectionState(dataRef, statePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data connection IP addresses for the given dataRef.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_BAD_PARAMETER -- Bad parameter.
 *   - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mngdConn_GetDataConnectionIPAddresses
(
    taf_mngdConn_DataRef_t dataRef,
    char *ipv4AddrPtr,
    size_t ipv4AddrSize,
    char *ipv6AddrPtr,
    size_t ipv6AddrSize
)
{
    auto &admin = tafMngdConnAdmin::GetInstance();

    return admin.GetConnectionIPAddresses(dataRef, ipv4AddrPtr, ipv4AddrSize,
                                          ipv6AddrPtr, ipv6AddrSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Starts the data retry mechanism. If data session is connected, the data session will be
 * disconnected before data retry mechanism is started. This functions is asynchronous and
 * applications should monitor data events via DataState handler.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_BAD_PARAMETER -- Bad parameter.
 *   - Appropriate error is returned on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mngdConn_StartDataRetry(
    taf_mngdConn_DataRef_t dataRef
    ///< [IN] The data reference.
)
{
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.StartDataRetry(dataRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Cancels a scheduled recovery process. A recovery could be scheduled for one or more data
 * connetions. For a scheduled recovery process to be canceled, this API should be called for all
 * data references for which recovery has been scheduled.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_NOT_POSSIBLE -- A recovery process has not been scheduled.
 *   - LE_NOT_PERMITTED -- A recovery process has already started.
 *   - Appropriate error is returned on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mngdConn_CancelRecovery
(
        taf_mngdConn_DataRef_t dataRef
        ///< [IN] The data reference.
)
{
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.CancelRecovery(dataRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds state change handler to monitor the connectivity state.
 */
//--------------------------------------------------------------------------------------------------
taf_mngdConn_DataStateHandlerRef_t taf_mngdConn_AddDataStateHandler
(
    taf_mngdConn_DataRef_t dataRef,
    taf_mngdConn_DataStateHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    TAF_ERROR_IF_RET_VAL(dataRef == NULL, NULL, "Null ptr(dataRef)");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.AddDataStateHandler(dataRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_mngdConn_DataState'
 */
//--------------------------------------------------------------------------------------------------
void taf_mngdConn_RemoveDataStateHandler(taf_mngdConn_DataStateHandlerRef_t handlerRef)
{

    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_mngdConn_RecoveryEvent'
 *
 * Events to report recovery state.
 */
//--------------------------------------------------------------------------------------------------
taf_mngdConn_RecoveryEventHandlerRef_t taf_mngdConn_AddRecoveryEventHandler(
    taf_mngdConn_RecoveryEventHandlerFunc_t handlerPtr,
    ///< [IN] The event handler reference.
    void *contextPtr
    ///< [IN]
)
{
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.AddRecoveryEventHandler(handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_mngdConn_RecoveryEvent'
 */
//--------------------------------------------------------------------------------------------------
void taf_mngdConn_RemoveRecoveryEventHandler(
    taf_mngdConn_RecoveryEventHandlerRef_t handlerRef
    ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

COMPONENT_INIT
{
    LE_INFO("tafMngdConnSvc COMPONENT init...");

    Admin_init();
    LE_INFO("COMPONENT end init");
    return;
}
