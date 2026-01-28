/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafDataIDStorSvc.cpp
 * @brief      This file provides the telaf Read/Write DID service as interfaces described
 *             in taf_diagDataIDStor .api. The Diag Storage service will be started automatically.
 */


#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include "tafDidStore.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference of a DataIDStor service. If there is no service, a new one will be created.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to create the service.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDidStore_ServiceRef_t taf_diagDidStore_GetService
(
    void
)
{
    auto &didStore = taf_diagDidStore::GetInstance();
    return didStore.GetService();
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the data record of the data ID.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid MsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDidStore_Read
(
    taf_diagDidStore_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint16_t dataId,
        ///< [IN] Data identifier.
    uint8_t* dataRecordPtr,
        ///< [OUT] Data record.
    size_t* dataRecordSizePtr
        ///< [INOUT]
)
{
    auto &didStore = taf_diagDidStore::GetInstance();

    taf_DidStore_t* servicePtr = (taf_DidStore_t*)le_ref_Lookup(didStore.SvcRefMap, svcRef);
    if(servicePtr == NULL)
    {
        LE_ERROR("Invalid service reference provided");
        return LE_BAD_PARAMETER;
    }

    // Get and Check client app name is configured for requested read DID.
    char readAppName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };

    if (LE_OK != didStore.GetAppNameBySessionRef(taf_diagDidStore_GetClientSessionRef(),
            readAppName, sizeof(readAppName)))
    {
        LE_ERROR("Failed to get client appName.");
        return LE_FAULT;
    }

    if(didStore.IsReadAppAccessible(dataId, readAppName) == false)
    {
        LE_ERROR("Calling app is not in the read access list");
        return LE_FAULT;
    }

    return didStore.Read(dataId, dataRecordPtr, dataRecordSizePtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Writes the data record to data ID.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid MsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDidStore_Write
(
    taf_diagDidStore_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint16_t dataId,
        ///< [IN] data identifier.
    const uint8_t* dataPtr,
        ///< [IN] Data payload.
    size_t dataSize
        ///< [IN]
)
{
    auto &didStore = taf_diagDidStore::GetInstance();

    taf_DidStore_t* servicePtr = (taf_DidStore_t*)le_ref_Lookup(didStore.SvcRefMap, svcRef);
    if(servicePtr == NULL)
    {
        LE_ERROR("Invalid service reference provided");
        return LE_BAD_PARAMETER;
    }

    // Get and Check client app name is configured for requested read DID.
    char writeAppName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };

    if (LE_OK != didStore.GetAppNameBySessionRef(taf_diagDidStore_GetClientSessionRef(),
            writeAppName, sizeof(writeAppName)))
    {
        LE_ERROR("Failed to get client appName.");
        return LE_FAULT;
    }

    if(didStore.IsWriteAppAccessible(dataId, writeAppName) == false)
    {
        LE_ERROR("Calling app is not in the write access list");
        return LE_FAULT;
    }

    return didStore.Write(dataId, dataPtr, dataSize);
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagDataIDStor_MsgNotify'
 *
 * This event provides information about the the Data ID change notification.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDidStore_DataIdChangeHandlerRef_t taf_diagDidStore_AddDataIdChangeHandler
(
    taf_diagDidStore_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint16_t dataId,
        ///< [IN] Data identifier.
    taf_diagDidStore_DataIdChangeHandlerFunc_t handlerPtr,
        ///< [IN] Received message handler.
    void* contextPtr
        ///< [IN]
)
{
    auto &didStore = taf_diagDidStore::GetInstance();
    return didStore.AddDataIdChangeHandler(svcRef, dataId, handlerPtr, contextPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagDataIDStor_MsgNotify'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagDidStore_RemoveDataIdChangeHandler
(
    taf_diagDidStore_DataIdChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
    auto &didStore = taf_diagDidStore::GetInstance();
    return didStore.RemoveDataIdChangeHandler(handlerRef);
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the reference of a DID change handler.
 *
 * @instaging
 * @return
 *     - Reference to the DID change handler.
 *     - NULL if errors.
 *
 */
//-------------------------------------------------------------------------------------------------
taf_diagDidStore_DIDChangeHandlerRef_t taf_diagDidStore_GetDIDHandlerRef
(
    taf_diagDidStore_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    auto &didStore = taf_diagDidStore::GetInstance();
    return didStore.GetDIDHandlerRef(svcRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add data id to handler for change notification.
 *
 * @instaging
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid handlerRef.
 *     - LE_DUPLICATE -- Data identifier already added.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDidStore_AddDIDToHandler
(
    taf_diagDidStore_DIDChangeHandlerRef_t handlerRef,
        ///< [IN] Handler reference.
    uint16_t dataId
        ///< [IN] Data identifier.
)
{
    auto &didStore = taf_diagDidStore::GetInstance();
    return didStore.AddDIDToHandler(handlerRef, dataId);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove data id from handler.
 *
 * @instaging
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid handlerRef.
 *     - LE_NOT_FOUND -- Data identifier not found.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDidStore_RemoveDIDFromHandler
(
    taf_diagDidStore_DIDChangeHandlerRef_t handlerRef,
        ///< [IN] Handler reference.
    uint16_t dataId
        ///< [IN] Data identifier.
)
{
    auto &didStore = taf_diagDidStore::GetInstance();
    return didStore.RemoveDIDFromHandler(handlerRef, dataId);
}

/**
 * The initialization of TelAF DID storage component.
*/
COMPONENT_INIT
{
    LE_INFO("TelAF DID Storage Service init Started...");
    auto &didStore = taf_diagDidStore::GetInstance();
    didStore.Init();
    LE_DEBUG("TelAF DID Storage Service init completed...");
}
