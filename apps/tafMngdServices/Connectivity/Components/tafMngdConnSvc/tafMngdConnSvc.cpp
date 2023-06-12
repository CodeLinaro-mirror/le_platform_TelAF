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
taf_mngd_Conn_DataRef_t taf_mngd_Conn_GetData( uint8_t dataId )
{
    auto &admin = tafMngdConnAdmin::GetInstance();
    return admin.GetRefByDataId(dataId);
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
le_result_t taf_mngd_Conn_DataStart
(
    taf_mngd_Conn_DataRef_t dataRef
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
le_result_t taf_mngd_Conn_DataStop
(
    taf_mngd_Conn_DataRef_t dataRef
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
le_result_t taf_mngd_Conn_DataGetConnectionState
(
    taf_mngd_Conn_DataRef_t dataRef,
    uint8_t* dataIdPtr,
    taf_mngd_Conn_DataState_t* statePtr
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
le_result_t taf_mngd_Conn_DataGetConnectionIPAddresses
(
    taf_mngd_Conn_DataRef_t dataRef,
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
 * Adds state change handler to monitor the connectivity state.
 */
//--------------------------------------------------------------------------------------------------
taf_mngd_Conn_DataStateHandlerRef_t taf_mngd_Conn_AddDataStateHandler
(
    taf_mngd_Conn_DataRef_t dataRef,
    taf_mngd_Conn_DataStateHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &admin = tafMngdConnAdmin::GetInstance();

    TAF_ERROR_IF_RET_VAL(dataRef == NULL, NULL, "Null ptr(dataRef)");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");

    le_event_Id_t dataStateEvent = admin.GetDataStateEvent(dataRef);
    if(dataStateEvent == NULL)
    {
        LE_ERROR("Data event is not initialized");
        return NULL;
    }

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("ConnStateHandler",
        dataStateEvent, admin.FirstLayerConnStateHandler,
        (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_mngd_Conn_DataStateHandlerRef_t)(handlerRef);

}

void taf_mngd_Conn_RemoveDataStateHandler(taf_mngd_Conn_DataStateHandlerRef_t handlerRef)
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
