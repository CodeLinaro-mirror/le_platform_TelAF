/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "legato.h"
#include "tafMngdConnAdmin.hpp"

using namespace telux::tafsvc;

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
 * Gets the data object id (from configuration file) for the given data reference.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_NOT_FOUND -- Data reference not found.
 *   - Appropriate error is returned on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mngdConn_DataGetId
(
    taf_mngdConn_DataRef_t dataRef,
        ///< [IN] The data reference.
    uint8_t* dataIdPtr
        ///< [OUT] The data object id from configuration json.
)
{
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.DataGetId(dataRef, dataIdPtr);
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
le_result_t taf_mngdConn_DataStart
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
le_result_t taf_mngdConn_DataStop
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
le_result_t taf_mngdConn_DataGetConnectionState
(
    taf_mngdConn_DataRef_t dataRef,
    uint8_t* dataIdPtr,
    taf_mngdConn_DataState_t* statePtr
)
{
    auto &admin = tafMngdConnAdmin::GetInstance();

    return admin.GetConnectionState(dataRef, dataIdPtr, statePtr);
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
le_result_t taf_mngdConn_DataGetConnectionIPAddresses
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
 * Cancels a scheduled L1 recovery process. This API should be called for all data references that
 * scheduled a L1 recovery.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_NOT_POSSIBLE -- A L1 recovery process has not been scheduled.
 *   - LE_NOT_PERMITTED -- A L1 recovery process has already started.
 *   - Appropriate error is returned on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_mngdConn_CancelL1Recovery(taf_mngdConn_DataRef_t dataRef)
{
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.CancelL1Recovery(dataRef);
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
 * Add handler function for EVENT 'taf_mngdConn_RecoveryState'
 *
 * Events to report recovery state.
 */
//--------------------------------------------------------------------------------------------------
taf_mngdConn_RecoveryStateHandlerRef_t taf_mngdConn_AddRecoveryStateHandler(
    taf_mngdConn_RecoveryStateHandlerFunc_t handlerPtr,
    ///< [IN] The event handler reference.
    void *contextPtr
    ///< [IN]
)
{
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.AddRecoveryStateHandler(handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_mngdConn_RecoveryState'
 */
//--------------------------------------------------------------------------------------------------
void taf_mngdConn_RemoveRecoveryStateHandler(
    taf_mngdConn_RecoveryStateHandlerRef_t handlerRef
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
