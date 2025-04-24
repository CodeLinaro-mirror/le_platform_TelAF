/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafDTCSvc.cpp
 * @brief      This file provides the telaf DTC service as interfaces described
 *             in taf_diagDTC.api. The Diag DTC service will be started automatically.
 */

#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include "tafDTCSvr.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the DTC reference, if there's no DTC service, a new one will be created.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to create the service.
 *
 */
//--------------------------------------------------------------------------------------------------
taf_diagDTC_ServiceRef_t taf_diagDTC_GetService
(
    uint32_t dtcCode
        ///< [IN]
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.GetService(dtcCode);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the DTC code for the given DTC reference.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- DTC reference was not found.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_GetCode
(
    taf_diagDTC_ServiceRef_t svcRef,
        ///< [IN] The DTC reference.
    uint32_t* dtcCodePtr
        ///< [OUT] The DTC code.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.GetCode(svcRef, dtcCodePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the fault detection counter for the given DTC reference.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- DTC reference was not found.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_GetFaultDetectionCounter
(
    taf_diagDTC_ServiceRef_t svcRef,
        ///< [IN] The DTC reference.
    uint8_t* faultDetectionCounterPtr
        ///< [OUT] The DTC code.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.GetFaultDetectionCounter(svcRef, faultDetectionCounterPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Reads DTC status.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_ReadStatus
(
    taf_diagDTC_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint8_t* statusPtr
        ///< [OUT] DTC status.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.ReadStatus(svcRef, statusPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets activation status.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_SetActivationStatus
(
    taf_diagDTC_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagDTC_ActivationStatus_t status
        ///< [IN] Activation status.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.SetActivationStatus(svcRef, status);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets activation status.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_GetActivationStatus
(
    taf_diagDTC_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagDTC_ActivationStatus_t* statusPtr
        ///< [OUT] Activation status.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.GetActivationStatus(svcRef, statusPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets DTC suppression.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_SetSuppression
(
    taf_diagDTC_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    bool suppressionStatus
        ///< [IN] Set or clear DTC suppression status.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.SetSuppression(svcRef, suppressionStatus);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets DTC suppression status.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_GetSuppression
(
    taf_diagDTC_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    bool* suppressionStatusPtr
        ///< [OUT] DTC suppression status.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.GetSuppression(svcRef, suppressionStatusPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Clears DTC information.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_ClearInfo
(
    taf_diagDTC_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.ClearInfo(svcRef, TAF_DIAGDTC_APP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagDTC_Status'
 *
 * This event provides information on DTC status.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDTC_StatusHandlerRef_t taf_diagDTC_AddStatusHandler
(
    taf_diagDTC_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagDTC_StatusHandlerFunc_t handlerPtr,
        ///< [IN] DTC handler.
    void* contextPtr
        ///< [IN]
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "Null ptr(svcRef)");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");

    le_event_Id_t dtcStatusEventId = diagDTC.GetDtcStatusEvent(svcRef);
    if(dtcStatusEventId == NULL)
    {
        LE_ERROR("DTC status event is not initialized");
        return NULL;
    }

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("DTCStatusHandler",
        dtcStatusEventId, diagDTC.FirstLayerStatusHandler, (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_diagDTC_StatusHandlerRef_t)(handlerRef);

}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagDTC_Status'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagDTC_RemoveStatusHandler
(
    taf_diagDTC_StatusHandlerRef_t handlerRef
        ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagDTC_ClearDTCStatus'
 *
 * This event provides information on Clear DTC status.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDTC_ClearStatusHandlerRef_t taf_diagDTC_AddClearStatusHandler
(
    taf_diagDTC_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagDTC_ClearStatusHandlerFunc_t handlerPtr,
        ///< [IN] DTC status handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diagDTC_AddClearStatusHandler!!");
    auto &diagDTC = taf_DTCSvr::GetInstance();

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "Null ptr(svcRef)");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");

    le_event_Id_t clearDtcStatusEventId = diagDTC.GetClearDtcStatusEvent(svcRef);
    if(clearDtcStatusEventId == NULL)
    {
        LE_ERROR("Clear DTC status event is not initialized");
        return NULL;
    }

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("ClearDTCStatusHandler",
        clearDtcStatusEventId, diagDTC.FirstLayerClearDtcStatusHandler, (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_diagDTC_ClearStatusHandlerRef_t)(handlerRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagDTC_ClearDTCStatus'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagDTC_RemoveClearStatusHandler
(
    taf_diagDTC_ClearStatusHandlerRef_t handlerRef
        ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes the DTC service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_RemoveSvc
(
    taf_diagDTC_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.RemoveSvc(svcRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets DTC data list, including snapshot data, extended data, etc.
 *
 * @return
 *   - NULL -- If the DTC was not found or the DTC data list is empty.
 *   - Others -- The DTC data list reference.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDTC_DataListRef_t taf_diagDTC_GetDataList
(
    taf_diagDTC_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.GetDataList(svcRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the first DTC data reference with a list.
 *
 * @return
 *   - NULL -- The DTC data list doesn't exist.
 *   - Others -- The first DTC data reference.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDTC_DataRef_t taf_diagDTC_GetFirstData
(
    taf_diagDTC_DataListRef_t dataListRef
        ///< [IN] Reference of a DTC data list.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.GetFirstData(dataListRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the next DTC data reference with a list.
 *
 * @return
 *   - NULL -- The DTC data list or the next interface doesn't exist.
 *   - Others -- The next DTC data reference.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDTC_DataRef_t taf_diagDTC_GetNextData
(
    taf_diagDTC_DataListRef_t dataListRef
        ///< [IN] Reference of a DTC data list.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.GetNextData(dataListRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes the DTC data list.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_NOT_FOUND -- DTC data list was not found.
 *   - LE_BAD_PARAMETER -- Bad parameter.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_DeleteDataList
(
    taf_diagDTC_DataListRef_t dataListRef
        ///< [IN] Reference of a DTC data list.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.DeleteDataList(dataListRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets DTC code by the DTC data reference.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_NOT_FOUND -- DTC data was not found.
 *   - LE_BAD_PARAMETER -- Bad parameter.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_GetDataDtcCode
(
    taf_diagDTC_DataRef_t dataRef,
        ///< [IN] DTC data reference.
    uint32_t* dtcCodePtr
        ///< [OUT] DTC code.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.GetDataDtcCode(dataRef, dtcCodePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets data type by the DTC data reference.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_NOT_FOUND -- DTC data was not found.
 *   - LE_BAD_PARAMETER -- Bad parameter.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_GetDataType
(
    taf_diagDTC_DataRef_t dataRef,
        ///< [IN] DTC data reference.
    taf_diagDTC_DataType_t* dataTypePtr
        ///< [OUT] Data type.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.GetDataType(dataRef, dataTypePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets record number by the DTC data reference.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_NOT_FOUND -- DTC data was not found.
 *   - LE_BAD_PARAMETER -- Bad parameter.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_GetRecordNumber
(
    taf_diagDTC_DataRef_t dataRef,
        ///< [IN] DTC data reference.
    uint8_t* recordNumberPtr
        ///< [OUT] Record number.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.GetRecordNumber(dataRef, recordNumberPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets did by the DTC data reference if the data type is snapshot data.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_NOT_FOUND -- DTC data was not found.
 *   - LE_BAD_PARAMETER -- Bad parameter.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_GetDataId
(
    taf_diagDTC_DataRef_t dataRef,
        ///< [IN] DTC data reference.
    uint16_t* dataIdPtr
        ///< [OUT] Data ID.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.GetDataId(dataRef, dataIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets data value by the DTC data reference.
 *
 * @return
 *   - LE_OK -- Succeeded.
 *   - LE_NOT_FOUND -- DTC data was not found.
 *   - LE_BAD_PARAMETER -- Bad parameter.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_GetDataValue
(
    taf_diagDTC_DataRef_t dataRef,
        ///< [IN] DTC data reference.
    uint8_t* valuePtr,
        ///< [OUT] Data ID.
    size_t* valueSizePtr
        ///< [INOUT]
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.GetDataValue(dataRef, valuePtr, valueSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference for all DTC, if there is no service, a new one will be created.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to create the service.
 *
 */
//--------------------------------------------------------------------------------------------------
taf_diagDTC_AllServiceRef_t taf_diagDTC_GetAllService
(
    void
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.GetAllService();
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagDTC_AllStatus'
 *
 * This event provides information on DTC status.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDTC_AllStatusHandlerRef_t taf_diagDTC_AddAllStatusHandler
(
    taf_diagDTC_AllServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagDTC_AllStatusHandlerFunc_t handlerPtr,
        ///< [IN] ALL DTC handler.
    void* contextPtr
        ///< [IN]
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "Null ptr(svcRef)");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");

    le_event_Id_t allDtcStatusEventId = diagDTC.GetAllDtcStatusEvent(svcRef);
    if(allDtcStatusEventId == NULL)
    {
        LE_ERROR("all DTC status event is not initialized");
        return NULL;
    }

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("AllDTCStatusHandler",
        allDtcStatusEventId, diagDTC.FirstLayerAllDtcStatusHandler, (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_diagDTC_AllStatusHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagDTC_AllStatus'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagDTC_RemoveAllStatusHandler
(
    taf_diagDTC_AllStatusHandlerRef_t handlerRef
        ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagDTC_ClearAllDTCStatus'
 *
 * This event provides information on Clear All DTC status.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDTC_ClearAllStatusHandlerRef_t taf_diagDTC_AddClearAllStatusHandler
(
    taf_diagDTC_AllServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagDTC_ClearAllStatusHandlerFunc_t handlerPtr,
        ///< [IN] DTC handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diagDTC_AddClearAllStatusHandler!!");
    auto &diagDTC = taf_DTCSvr::GetInstance();

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "Null ptr(svcRef)");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");

    le_event_Id_t clearAllDtcStatusEventId = diagDTC.GetClearAllDtcStatusEvent(svcRef);
    if(clearAllDtcStatusEventId == NULL)
    {
        LE_ERROR("clear all DTC status event is not initialized");
        return NULL;
    }

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("ClearAllDTCStatusHandler",
        clearAllDtcStatusEventId, diagDTC.FirstLayerClearAllDtcStatusHandler, (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_diagDTC_ClearAllStatusHandlerRef_t)(handlerRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagDTC_ClearAllDTCStatus'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagDTC_RemoveClearAllStatusHandler
(
    taf_diagDTC_ClearAllStatusHandlerRef_t handlerRef
        ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Clears all DTC information.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_ClearAllInfo
(
    taf_diagDTC_AllServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.ClearAllInfo(svcRef, TAF_DIAGDTC_APP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets all DTC suppression.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_SetAllSuppression
(
    taf_diagDTC_AllServiceRef_t svcRef,
        ///< [IN] Service reference.
    bool suppressionStatus
        ///< [IN] Set or clear DTC suppression status.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.SetAllSuppression(svcRef, suppressionStatus);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes ALL DTC service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDTC_RemoveAllSvc
(
    taf_diagDTC_AllServiceRef_t svcRef
        ///< [IN] Service reference for ALL DTC.
)
{
    auto &diagDTC = taf_DTCSvr::GetInstance();

    return diagDTC.RemoveAllSvc(svcRef);
}
