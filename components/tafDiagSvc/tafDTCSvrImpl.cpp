/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDTCSvr.hpp"
#include "tafEventSvr.hpp"
#include "tafDataAccessComp.h"
#include <algorithm>
#include <chrono>

using namespace tafsvc;
using namespace std;

LE_MEM_DEFINE_STATIC_POOL(tafDtcSessionRef, TAF_DTC_MAX_SESSION_REF,
        sizeof(taf_diagDTC_SessionRef_t));

//DTC data definition
LE_MEM_DEFINE_STATIC_POOL(DtcDataListPool, TAF_DIAGDTC_DTC_DATA_LISTS_MAX_NUM,
        sizeof(taf_DtcDataList_t));

LE_MEM_DEFINE_STATIC_POOL(DtcDataPool, TAF_DIAGDTC_DTC_DATA_MAX_NUM, sizeof(taf_DtcData_t));

LE_MEM_DEFINE_STATIC_POOL(DtcDataSafeRefPool, TAF_DIAGDTC_DTC_DATA_MAX_NUM,
        sizeof(taf_DtcDataSafeRef_t));

LE_REF_DEFINE_STATIC_MAP(DtcDataListRefMap, TAF_DIAGDTC_DTC_DATA_LISTS_MAX_NUM);

LE_REF_DEFINE_STATIC_MAP(DtcDataSafeRefMap, TAF_DIAGDTC_DTC_DATA_MAX_NUM);

LE_MEM_DEFINE_STATIC_POOL(DtcDataValueString, TAF_DTC_DATA_VALUE_MAX_NUM,
        TAF_DIAGDTC_DATA_VALUE_MAX_BYTES);

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of TelAF DTC server.
 */
//--------------------------------------------------------------------------------------------------
taf_DTCSvr &taf_DTCSvr::GetInstance
(
)
{
    static taf_DTCSvr instance;

    return instance;
}

//-------------------------------------------------------------------------------------------------
/**
 * Create a reference for the service, or get the reference of a service if the reference already
 * exist.
 */
//-------------------------------------------------------------------------------------------------
taf_diagDTC_ServiceRef_t taf_DTCSvr::GetService
(
    uint32_t dtcCode
)
{
    // Search the service.
    taf_diagDTC_DtcCtx_t* dtcCtxPtr = GetDtcCtxByCode(dtcCode);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, NULL, "DTC code is not supported");

    le_msg_SessionRef_t sessionRef = taf_diagDTC_GetClientSessionRef();
    //Service reference is not created
    if(dtcCtxPtr->svcRef== NULL)
    {
        // Create a Safe Reference for this service object
        taf_diagDTC_ServiceRef_t dtcRef = (taf_diagDTC_ServiceRef_t)le_ref_CreateRef(SvcRefMap,
                dtcCtxPtr);
        TAF_ERROR_IF_RET_VAL(dtcRef == NULL, NULL, "cannot alloc dtcRef");

        dtcCtxPtr->svcRef = dtcRef;

        dtcCtxPtr->sessionRefList = LE_DLS_LIST_INIT;

        LE_DEBUG("svcRef %p of client %p is created for DTC code 0x%x.",
                dtcCtxPtr->svcRef, sessionRef, dtcCode);
    }
    else
    {
        LE_DEBUG("server ref %p already created",dtcCtxPtr->svcRef );
    }

    // Add client reference to the session list
    AddSessionToDtcCtx(dtcCtxPtr, sessionRef);

    return dtcCtxPtr->svcRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get DTC code by the given service reference.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::GetCode
(
    taf_diagDTC_ServiceRef_t svcRef,
    uint32_t* dtcCodePtr
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");
    TAF_ERROR_IF_RET_VAL(dtcCodePtr == NULL, LE_BAD_PARAMETER, "dtcCodePtr is null");

    taf_diagDTC_DtcCtx_t* dtcCtxPtr = GetDtcCtx(svcRef);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_UNSUPPORTED, "Fail to get DTC context");

    *dtcCodePtr = dtcCtxPtr->dtcCode;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get DTC code by the given service reference.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::GetFaultDetectionCounter
(
    taf_diagDTC_ServiceRef_t svcRef,
    uint8_t* faultDetectionCounterPtr
)
{
    uint32_t dtcCode;
    le_result_t result;
    auto &diagEvent = taf_EventSvr::GetInstance();
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");

    result=GetCode(svcRef, &dtcCode);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get DTC code");
        return result;
    }
    return diagEvent.GetFaultDetectionCounter(dtcCode, faultDetectionCounterPtr);
}

//-------------------------------------------------------------------------------------------------
/**
 * Read DTC status.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::ReadStatus
(
    taf_diagDTC_ServiceRef_t svcRef,
    uint8_t* statusPtr
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");
    TAF_ERROR_IF_RET_VAL(statusPtr == NULL, LE_BAD_PARAMETER, "statusPtr is null");

    taf_diagDTC_DtcCtx_t* dtcCtxPtr = GetDtcCtx(svcRef);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_UNSUPPORTED, "dtcCtxPtr is null");

    //Check if DTC is suppressed
    if(dtcCtxPtr->suppressionStatus == true)
    {
        LE_WARN("DTC code:0x%x is suppressed", dtcCtxPtr->dtcCode);
        return LE_UNAVAILABLE;
    }

    *statusPtr = taf_DataAccess_GetDTCStatus(dtcCtxPtr->dtcCode);
    LE_DEBUG("Database: get DTC code:0x%x, DTC status in DB:0x%x", dtcCtxPtr->dtcCode, *statusPtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Set activation status.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::SetActivationStatus
(
    taf_diagDTC_ServiceRef_t svcRef,
    taf_diagDTC_ActivationStatus_t status
)
{
    le_result_t result;
    taf_diagDTC_ActivationStatus_t curActStatus;
    auto &diagEvent = taf_EventSvr::GetInstance();

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");

    taf_diagDTC_DtcCtx_t* dtcCtxPtr = GetDtcCtx(svcRef);
    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_UNSUPPORTED, "dtcCtxPtr is null");

    result = GetActivationStatus(svcRef, &curActStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get activation status");
        return LE_FAULT;
    }

    if(curActStatus == status)
    {
        LE_DEBUG("Status is same as current");
        return LE_OK;
    }

    //Set activation status into DB.
    if(status == TAF_DIAGDTC_INACTIVE)
        result = diagEvent.DisableDTCSetting(dtcCtxPtr->dtcCode);
    else if(status == TAF_DIAGDTC_ACTIVE)
        result = diagEvent.EnableDTCSetting(dtcCtxPtr->dtcCode);
    else
        result = LE_BAD_PARAMETER;

    //Report activation status
    if(result == LE_OK)
    {

        taf_diagDTC_ActStatus_t* actStatusIndPtr =
                (taf_diagDTC_ActStatus_t*)le_mem_ForceAlloc(ActStatusPool);

        actStatusIndPtr->svcRef = dtcCtxPtr->svcRef;
        actStatusIndPtr->actStatus = status;

        le_event_ReportWithRefCounting(dtcCtxPtr->actStatusEvId, (void*)actStatusIndPtr);
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get activation status.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::GetActivationStatus
(
    taf_diagDTC_ServiceRef_t svcRef,
    taf_diagDTC_ActivationStatus_t* statusPtr
)
{
    uint8_t activationStatus = 0;

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER,   "svcRef is null");
    TAF_ERROR_IF_RET_VAL(statusPtr == NULL, LE_BAD_PARAMETER, "statusPtr is null");

    taf_diagDTC_DtcCtx_t* dtcCtxPtr = GetDtcCtx(svcRef);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_UNSUPPORTED, "dtcCtxPtr is null");

    //Activation status may be set by DTOOL, read it from database.
    activationStatus = taf_DataAccess_GetDTCActivation(dtcCtxPtr->dtcCode);

    if(activationStatus == 1)
    {
        *statusPtr = TAF_DIAGDTC_ACTIVE;
    }
    else
    {
        *statusPtr = TAF_DIAGDTC_INACTIVE;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Set suppression.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::SetSuppression
(
    taf_diagDTC_ServiceRef_t svcRef,
    bool suppressionStatus
)
{
    le_result_t result;
    auto &diagEvent = taf_EventSvr::GetInstance();

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");

    taf_diagDTC_DtcCtx_t* dtcCtxPtr = GetDtcCtx(svcRef);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_UNSUPPORTED, "dtcCtxPtr is null");

    //Call tafEvent module to set suppression status, event module need to check suppression status.
    result = diagEvent.SetDTCSuppression(dtcCtxPtr->dtcCode, suppressionStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set suppression for DTC 0x%x", dtcCtxPtr->dtcCode);
        return result;
    }

    dtcCtxPtr->suppressionStatus = suppressionStatus;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get suppression.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::GetSuppression
(
    taf_diagDTC_ServiceRef_t svcRef,
    bool* suppressionStatusPtr
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");
    TAF_ERROR_IF_RET_VAL(suppressionStatusPtr == NULL, LE_BAD_PARAMETER,
            "suppressionStatusPtr is null");

    taf_diagDTC_DtcCtx_t* dtcCtxPtr = GetDtcCtx(svcRef);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_UNSUPPORTED, "dtcCtxPtr is null");

    *suppressionStatusPtr = dtcCtxPtr->suppressionStatus;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear DTC information.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::ClearInfo
(
    taf_diagDTC_ServiceRef_t svcRef,
    taf_diagDTC_ReqClientType_t clientType
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");

    taf_diagDTC_DtcCtx_t* dtcCtxPtr = GetDtcCtx(svcRef);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_UNSUPPORTED, "dtcCtxPtr is null");

    //Check if DTC is suppressed
    if(dtcCtxPtr->suppressionStatus == true)
    {
        LE_WARN("DTC code:0x%x is suppressed", dtcCtxPtr->dtcCode);
        return LE_UNAVAILABLE;
    }

    return diagEvent.ClearDtc(dtcCtxPtr->dtcCode, clientType);

}

//-------------------------------------------------------------------------------------------------
/**
 * Get DTC context by service reference.
 */
//-------------------------------------------------------------------------------------------------
taf_diagDTC_DtcCtx_t* taf_DTCSvr::GetDtcCtx
(
    taf_diagDTC_ServiceRef_t svcRef
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "svcRef is NULL");

    taf_diagDTC_DtcCtx_t* dtcCtxPtr = (taf_diagDTC_DtcCtx_t*)le_ref_Lookup(SvcRefMap, svcRef);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, NULL, "Invalid dtcCtxPtr");

    return dtcCtxPtr;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get DTC context by DTC code.
 */
//-------------------------------------------------------------------------------------------------
taf_diagDTC_DtcCtx_t* taf_DTCSvr::GetDtcCtxByCode
(
    uint32_t dtcCode
)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&DtcCtxList);
    while (linkPtr)
    {
        taf_diagDTC_DtcCtx_t* dtcCtxPtr = CONTAINER_OF(linkPtr, taf_diagDTC_DtcCtx_t,
                link);
        linkPtr = le_dls_PeekNext(&DtcCtxList, linkPtr);

        if (dtcCtxPtr->dtcCode == dtcCode)
        {
            LE_DEBUG("Get dtcCtxPtr %p by code %d", dtcCtxPtr, dtcCode);
            return dtcCtxPtr;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get DTC data list.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDTC_DataListRef_t taf_DTCSvr::GetDataList
(
    taf_diagDTC_ServiceRef_t svcRef
)
{
    le_dls_List_t list = LE_DLS_LIST_INIT;
    le_result_t result;
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "svcRef is null");

    taf_diagDTC_DtcCtx_t* dtcCtxPtr = GetDtcCtx(svcRef);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, NULL, "dtcCtxPtr is null");

    //Check if DTC is suppressed
    if(dtcCtxPtr->suppressionStatus == true)
    {
        LE_WARN("DTC code:0x%x is suppressed", dtcCtxPtr->dtcCode);
        return NULL;
    }

    result = taf_DataAccess_GetDTCData(dtcCtxPtr->dtcCode, &list);
    if (result != LE_OK)
    {
        LE_ERROR("Error while getting data from dataAccess module!");
        return NULL;
    }

    if(le_dls_NumLinks(&list) > 0)
    {
        taf_DtcDataList_t* dtcDataListPtr = (taf_DtcDataList_t*)le_mem_ForceAlloc(DtcDataListPool);
        dtcDataListPtr->dtcDataList = LE_SLS_LIST_INIT;
        dtcDataListPtr->safeRefList = LE_SLS_LIST_INIT;
        dtcDataListPtr->currPtr = NULL;

        le_dls_Link_t* linkPtr = NULL;
        linkPtr = le_dls_Pop(&list);

        while(linkPtr != NULL)
        {
            taf_DataAccess_DtcDataInfo_t * dtcStatusPtr = CONTAINER_OF(linkPtr,
                    taf_DataAccess_DtcDataInfo_t, link);

            if (dtcStatusPtr != NULL)
            {
                taf_DtcData_t* dtcDataPtr;
                dtcDataPtr = (taf_DtcData_t*)le_mem_ForceAlloc(DtcDataPool);
                dtcDataPtr->info.dtcCode = dtcStatusPtr->dtcCode;
                dtcDataPtr->info.recordNum = dtcStatusPtr->recordNum;

                switch(dtcStatusPtr->dataType)
                {
                    case SNAPSHOT_DATA:
                        dtcDataPtr->info.dataType = TAF_DIAGDTC_SNAPSHOT_DATA;
                        break;
                    case EXTENDED_DATA:
                        dtcDataPtr->info.dataType = TAF_DIAGDTC_EXTENDED_DATA;
                        break;
                    default:
                        dtcDataPtr->info.dataType = TAF_DIAGDTC_UNKNOWN_DATA;
                }

                if(dtcDataPtr->info.dataType == TAF_DIAGDTC_SNAPSHOT_DATA)
                    dtcDataPtr->info.dataId = dtcStatusPtr->dataId;

                dtcDataPtr->info.dataLen = dtcStatusPtr->dataLen;
                LE_INFO("Data length=%d", dtcDataPtr->info.dataLen);
                if(dtcDataPtr->info.dataLen > TAF_DIAGDTC_DATA_VALUE_MAX_BYTES)
                {
                    LE_ERROR("DTC data value length is too big");
                    le_mem_Release(dtcDataPtr);
                    linkPtr = le_dls_Pop(&list);
                    continue;
                }

                dtcDataPtr->info.dataValue = (uint8_t*)le_mem_ForceVarAlloc(DtcDataValuePool,
                        dtcDataPtr->info.dataLen);

                if(dtcStatusPtr->dataValue == NULL)
                {
                    LE_ERROR("DTC data value is NULL");
                    le_mem_Release(dtcDataPtr);
                    linkPtr = le_dls_Pop(&list);
                    continue;
                }

                memcpy(dtcDataPtr->info.dataValue, dtcStatusPtr->dataValue,
                        dtcDataPtr->info.dataLen);

                dtcDataPtr->link = LE_SLS_LINK_INIT;
                le_sls_Queue(&(dtcDataListPtr->dtcDataList), &(dtcDataPtr->link));
            }

            // Returns the link next to a specified link.
            linkPtr = le_dls_Pop(&list);
        }

        //Call data handle module API to release the memory;
        taf_DataAccess_ReleaseDTCData(&list);

        return (taf_diagDTC_DataListRef_t)le_ref_CreateRef(DtcDataListRefMap,
                (void*)dtcDataListPtr);
    }
    else
    {
        LE_INFO("No DTC data");
        return NULL;
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the first DTC data reference with a list.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDTC_DataRef_t taf_DTCSvr::GetFirstData
(
    taf_diagDTC_DataListRef_t dataListRef
)
{
    taf_DtcDataList_t* dtcDataListPtr = (taf_DtcDataList_t*)le_ref_Lookup(DtcDataListRefMap,
            dataListRef);

    TAF_ERROR_IF_RET_VAL(dtcDataListPtr == NULL, NULL, "failed to look up the reference:%p",
            dataListRef);

    le_sls_Link_t* linkPtr = le_sls_Peek(&(dtcDataListPtr->dtcDataList));
    TAF_ERROR_IF_RET_VAL(linkPtr == NULL, NULL, "Empty list");

    taf_DtcData_t* dtcDataPtr = CONTAINER_OF(linkPtr, taf_DtcData_t , link);
    dtcDataListPtr->currPtr = linkPtr;

    taf_DtcDataSafeRef_t* safeRefPtr = (taf_DtcDataSafeRef_t*)le_mem_ForceAlloc(DtcDataSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(DtcDataSafeRefMap, (void*)dtcDataPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(dtcDataListPtr->safeRefList), &(safeRefPtr->link));

    return (taf_diagDTC_DataRef_t)safeRefPtr->safeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the next DTC data reference with a list.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDTC_DataRef_t taf_DTCSvr::GetNextData
(
    taf_diagDTC_DataListRef_t dataListRef
)
{
    taf_DtcDataList_t* dtcDataListPtr = (taf_DtcDataList_t*)le_ref_Lookup(DtcDataListRefMap,
            dataListRef);

    TAF_ERROR_IF_RET_VAL(dtcDataListPtr == NULL, NULL, "failed to look up the reference:%p",
            dataListRef);

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(dtcDataListPtr->dtcDataList),
            dtcDataListPtr->currPtr);

    if(linkPtr == nullptr)
    {
        LE_DEBUG("Reach to the end of list");
        return NULL;
    }

    taf_DtcData_t* dtcDataPtr = CONTAINER_OF(linkPtr, taf_DtcData_t , link);
    dtcDataListPtr->currPtr = linkPtr;

    taf_DtcDataSafeRef_t* safeRefPtr = (taf_DtcDataSafeRef_t*)le_mem_ForceAlloc(DtcDataSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(DtcDataSafeRefMap, (void*)dtcDataPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(dtcDataListPtr->safeRefList), &(safeRefPtr->link));

    return (taf_diagDTC_DataRef_t)safeRefPtr->safeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes the DTC data list.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::DeleteDataList
(
    taf_diagDTC_DataListRef_t dataListRef
)
{
    taf_DtcData_t* dtcDataPtr;
    taf_DtcDataSafeRef_t* safeRefPtr;
    le_sls_Link_t *linkPtr;

    TAF_ERROR_IF_RET_VAL(dataListRef == NULL, LE_BAD_PARAMETER, "Null reference(dataListRef)");

    taf_DtcDataList_t* dtcDataListPtr = (taf_DtcDataList_t*)le_ref_Lookup(DtcDataListRefMap,
            dataListRef);

    TAF_ERROR_IF_RET_VAL(dtcDataListPtr == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    while ((linkPtr = le_sls_Pop(&(dtcDataListPtr->dtcDataList))) != NULL)
    {
        dtcDataPtr = CONTAINER_OF(linkPtr, taf_DtcData_t, link);
        le_mem_Release(dtcDataPtr->info.dataValue);
        le_mem_Release(dtcDataPtr);
    }

    while ((linkPtr = le_sls_Pop(&(dtcDataListPtr->safeRefList))) != NULL)
    {
        safeRefPtr = CONTAINER_OF(linkPtr, taf_DtcDataSafeRef_t, link);
        le_ref_DeleteRef(DtcDataSafeRefMap, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(DtcDataListRefMap, dataListRef);

    le_mem_Release(dtcDataListPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets DTC data code by the DTC data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::GetDataDtcCode
(
    taf_diagDTC_DataRef_t dataRef,
    uint32_t* dtcCodePtr
)
{
    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_NOT_FOUND, "dataRef is null");
    TAF_ERROR_IF_RET_VAL(dtcCodePtr == NULL, LE_BAD_PARAMETER, "dtcCodePtr is null");

    taf_DtcData_t* dtcDataPtr = (taf_DtcData_t*)le_ref_Lookup(DtcDataSafeRefMap, dataRef);
    TAF_ERROR_IF_RET_VAL(dtcDataPtr == NULL, LE_FAULT, "Invalid para(null reference ptr)");

    *dtcCodePtr = dtcDataPtr->info.dtcCode;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets DTC data type by the DTC data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::GetDataType
(
    taf_diagDTC_DataRef_t dataRef,
    taf_diagDTC_DataType_t* dataTypePtr
)
{
    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_NOT_FOUND, "dataRef is null");
    TAF_ERROR_IF_RET_VAL(dataTypePtr == NULL, LE_BAD_PARAMETER, "dataTypePtr is null");

    taf_DtcData_t* dtcDataPtr = (taf_DtcData_t*)le_ref_Lookup(DtcDataSafeRefMap, dataRef);
    TAF_ERROR_IF_RET_VAL(dtcDataPtr == NULL, LE_FAULT, "Invalid para(null reference ptr)");

    *dataTypePtr = dtcDataPtr->info.dataType;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets DTC record number by the DTC data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::GetRecordNumber
(
    taf_diagDTC_DataRef_t dataRef,
    uint8_t* recordNumberPtr
)
{
    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_NOT_FOUND, "dataRef is null");
    TAF_ERROR_IF_RET_VAL(recordNumberPtr == NULL, LE_BAD_PARAMETER, "recordNumberPtr is null");

    taf_DtcData_t* dtcDataPtr = (taf_DtcData_t*)le_ref_Lookup(DtcDataSafeRefMap, dataRef);
    TAF_ERROR_IF_RET_VAL(dtcDataPtr == NULL, LE_FAULT, "Invalid para(null reference ptr)");

    *recordNumberPtr = dtcDataPtr->info.recordNum;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data ID by the DTC data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::GetDataId
(
    taf_diagDTC_DataRef_t dataRef,
    uint16_t* dataIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_NOT_FOUND, "dataRef is null");
    TAF_ERROR_IF_RET_VAL(dataIdPtr == NULL, LE_BAD_PARAMETER, "dataIdPtr is null");

    taf_DtcData_t* dtcDataPtr = (taf_DtcData_t*)le_ref_Lookup(DtcDataSafeRefMap, dataRef);
    TAF_ERROR_IF_RET_VAL(dtcDataPtr == NULL, LE_FAULT, "Invalid para(null reference ptr)");

    if(dtcDataPtr->info.dataType != TAF_DIAGDTC_SNAPSHOT_DATA)
    {
        LE_ERROR("Not snapshot data");
        return LE_FORMAT_ERROR;
    }

    *dataIdPtr = dtcDataPtr->info.dataId;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data value by the DTC data reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::GetDataValue
(
    taf_diagDTC_DataRef_t dataRef,
    uint8_t* valuePtr,
    size_t* valueSizePtr
)
{
    TAF_ERROR_IF_RET_VAL(dataRef == NULL, LE_NOT_FOUND, "dataRef is null");
    TAF_ERROR_IF_RET_VAL(valuePtr == NULL, LE_BAD_PARAMETER, "valuePtr is null");
    TAF_ERROR_IF_RET_VAL(valueSizePtr == NULL, LE_BAD_PARAMETER, "valueSizePtr is null");

    taf_DtcData_t* dtcDataPtr = (taf_DtcData_t*)le_ref_Lookup(DtcDataSafeRefMap, dataRef);
    TAF_ERROR_IF_RET_VAL(dtcDataPtr == NULL, LE_FAULT, "Invalid para(null reference ptr)");

    *valueSizePtr = (size_t)dtcDataPtr->info.dataLen;
    if(dtcDataPtr->info.dataLen > TAF_DIAGDTC_DATA_VALUE_MAX_BYTES)
    {
        LE_ERROR("Data length is too long");
        return LE_FAULT;
    }

    if(dtcDataPtr->info.dataLen > 0)
    {
        memcpy(valuePtr, dtcDataPtr->info.dataValue, dtcDataPtr->info.dataLen);
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the created service and release the allocated memory.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::RemoveSvc
(
    taf_diagDTC_ServiceRef_t svcRef
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");
    le_msg_SessionRef_t sessionRef = taf_diagDTC_GetClientSessionRef();

    //Find DTC context one by one
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_diagDTC_DtcCtx_t* dtcCtxPtr = (taf_diagDTC_DtcCtx_t*)le_ref_GetValue(iterRef);

        if (dtcCtxPtr != NULL)
        {
            if (dtcCtxPtr->svcRef == svcRef)
            {
                //Remove client session reference from DTC session reference list
                if( RemoveSessionFromDtcCtx(dtcCtxPtr, sessionRef) == LE_OK)
                    LE_DEBUG(" remove session %p, from dtcCtxPtr %p with dtc code 0x%x",
                            sessionRef, dtcCtxPtr, dtcCtxPtr->dtcCode);
            }

            //If session number of links is 0, release DTC context
            if( le_dls_NumLinks(&dtcCtxPtr->sessionRefList)  == 0)
            {
                // Clear service object
                le_ref_DeleteRef(SvcRefMap, (void*)dtcCtxPtr->svcRef);
                dtcCtxPtr->svcRef = NULL;
            }
        }
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add client session into dtc context.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::AddSessionToDtcCtx
(
    taf_diagDTC_DtcCtx_t* dtcCtxPtr,
    le_msg_SessionRef_t sessionRef
)
{
    le_dls_Link_t* linkPtr = NULL;

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_BAD_PARAMETER, "this DTC context is NULL");

    linkPtr = le_dls_Peek(&(dtcCtxPtr->sessionRefList));
    while (linkPtr)
    {
        taf_diagDTC_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkPtr,
                taf_diagDTC_SessionRef_t, link);
        linkPtr = le_dls_PeekNext(&(dtcCtxPtr->sessionRefList), linkPtr);

        if (sessionRefPtr->sessionRef == sessionRef)
        {
            return LE_DUPLICATE;
        }
    }

    taf_diagDTC_SessionRef_t* newSessionRefPtr =
            (taf_diagDTC_SessionRef_t *)le_mem_ForceAlloc(SessionRefPool);

    newSessionRefPtr->sessionRef = sessionRef;
    newSessionRefPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&dtcCtxPtr->sessionRefList, &(newSessionRefPtr->link));

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove client session from dtc context.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::RemoveSessionFromDtcCtx
(
    taf_diagDTC_DtcCtx_t* dtcCtxPtr,
    le_msg_SessionRef_t sessionRef
)
{
    le_dls_Link_t* linkPtr = NULL;

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, LE_BAD_PARAMETER, "this DTC context is NULL");

    linkPtr = le_dls_Peek(&(dtcCtxPtr->sessionRefList));
    while (linkPtr)
    {
        taf_diagDTC_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkPtr, taf_diagDTC_SessionRef_t,
                link);
        linkPtr = le_dls_PeekNext(&(dtcCtxPtr->sessionRefList), linkPtr);

        if (sessionRefPtr->sessionRef == sessionRef)
        {
            le_dls_Remove(&(dtcCtxPtr->sessionRefList), &(sessionRefPtr->link));
            le_mem_Release(sessionRefPtr);
            return LE_OK;
        }
    }

    LE_DEBUG("Cannot found session context with ref(%p) from dtcCode(%d)", sessionRef,
            dtcCtxPtr->dtcCode);

    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 * FirstLayerActivationStatusHandler.
 */
//--------------------------------------------------------------------------------------------------
void taf_DTCSvr::FirstLayerActStatusHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_diagDTC_ActStatus_t* actStatusEvent = (taf_diagDTC_ActStatus_t *)reportPtr;
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == NULL, "Null ptr(secondLayerHandlerFunc)");

    taf_diagDTC_ActivationStatusHandlerFunc_t handlerFunc =
            (taf_diagDTC_ActivationStatusHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc(actStatusEvent->svcRef, actStatusEvent->actStatus, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get DTC status event ID, used to notify the status change.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t taf_DTCSvr::GetActStatusEvent
(
    taf_diagDTC_ServiceRef_t svcRef
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "svcRef is null");

    taf_diagDTC_DtcCtx_t* dtcCtxPtr = GetDtcCtx(svcRef);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, NULL, "dtcCtxPtr is null");

    return dtcCtxPtr->actStatusEvId;
}

//--------------------------------------------------------------------------------------------------
/**
 * FirstLayerUdsStatusHandler.
 */
//--------------------------------------------------------------------------------------------------
void taf_DTCSvr::FirstLayerStatusHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_diagDTC_Status_t* dtcStatusEvent = (taf_diagDTC_Status_t *)reportPtr;
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == NULL, "Null ptr(secondLayerHandlerFunc)");

    taf_diagDTC_StatusHandlerFunc_t handlerFunc =
                                    (taf_diagDTC_StatusHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc(dtcStatusEvent->svcRef, dtcStatusEvent->dtcStatus, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * FirstLayerClearDTCStatusHandler.
 */
//--------------------------------------------------------------------------------------------------
void taf_DTCSvr::FirstLayerClearDtcStatusHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_diagDTC_ClearStatus_t* clearDtcStatusEvent = (taf_diagDTC_ClearStatus_t *)reportPtr;
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == NULL, "Null ptr(secondLayerHandlerFunc)");

    taf_diagDTC_ClearStatusHandlerFunc_t handlerFunc =
            (taf_diagDTC_ClearStatusHandlerFunc_t)secondLayerHandlerFunc;

    handlerFunc(clearDtcStatusEvent->svcRef, clearDtcStatusEvent->clientType,
            le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Report DTC status.
 */
//--------------------------------------------------------------------------------------------------
void taf_DTCSvr::ReportDTCStatus
(
    uint32_t dtcCode,
    uint8_t dtcStatus
)
{
    // Search the service.
    taf_diagDTC_DtcCtx_t* dtcCtxPtr = GetDtcCtxByCode(dtcCode);

    TAF_ERROR_IF_RET_NIL(dtcCtxPtr == NULL, "DTC code is not supported");

    //Check if DTC is suppressed
    if(dtcCtxPtr->suppressionStatus == true)
    {
        LE_DEBUG("DTC code:0x%x is suppressed, don't notify the status", dtcCtxPtr->dtcCode);
        return;
    }

    if(dtcCtxPtr->svcRef != NULL)
    {
        taf_diagDTC_Status_t* dtcStatusIndPtr =
                (taf_diagDTC_Status_t*)le_mem_ForceAlloc(DtcStatusPool);

        dtcStatusIndPtr->svcRef = dtcCtxPtr->svcRef;
        dtcStatusIndPtr->dtcStatus = dtcStatus;

        le_event_ReportWithRefCounting(dtcCtxPtr->dtcStatusEventId, (void*)dtcStatusIndPtr);
    }

    //Report event for all DTC.
    taf_diagDTC_AllStatus_t* allDtcStatusIndPtr =
                (taf_diagDTC_AllStatus_t*)le_mem_ForceAlloc(AllDtcStatusPool);

    allDtcStatusIndPtr->svcRef = AllDtcCtx.svcRef;
    allDtcStatusIndPtr->dtcCode = dtcCode;
    allDtcStatusIndPtr->dtcStatus = dtcStatus;

    le_event_ReportWithRefCounting(AllDtcCtx.allDtcStatusEventId, (void*)allDtcStatusIndPtr);

}

//--------------------------------------------------------------------------------------------------
/**
 * Report Clear DTC status.
 */
//--------------------------------------------------------------------------------------------------
void taf_DTCSvr::ReportClearDTCStatus
(
    uint32_t dtcCode,
    taf_diagDTC_ReqClientType_t clientType
)
{
    // Search the service.
    taf_diagDTC_DtcCtx_t* dtcCtxPtr = GetDtcCtxByCode(dtcCode);

    TAF_ERROR_IF_RET_NIL(dtcCtxPtr == NULL, "DTC code is not supported");

    if(dtcCtxPtr->svcRef != NULL)
    {
        LE_DEBUG("Report Single DTC!!");
        taf_diagDTC_ClearStatus_t* clearDtcStatusIndPtr =
                (taf_diagDTC_ClearStatus_t*)le_mem_ForceAlloc(ClearDtcStatusPool);

        clearDtcStatusIndPtr->svcRef = dtcCtxPtr->svcRef;
        clearDtcStatusIndPtr->clientType = clientType;

        le_event_ReportWithRefCounting(dtcCtxPtr->clearDtcStatusEventId,
                (void*)clearDtcStatusIndPtr);
    }
    else
    {
        LE_ERROR("Service reference not found to notify");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Report Clear All DTC status.
 */
//--------------------------------------------------------------------------------------------------
void taf_DTCSvr::ReportClearAllDTCStatus
(
    taf_diagDTC_ReqClientType_t clientType
)
{

    if(AllDtcCtx.svcRef != NULL)
    {
        LE_DEBUG("Report Clear All DTC!!");
        taf_diagDTC_ClearAllStatus_t* clearAllDtcStatusIndPtr =
                (taf_diagDTC_ClearAllStatus_t*)le_mem_ForceAlloc(ClearAllDtcStatusPool);

        clearAllDtcStatusIndPtr->svcRef = AllDtcCtx.svcRef;
        clearAllDtcStatusIndPtr->clientType = clientType;

        le_event_ReportWithRefCounting(AllDtcCtx.clearAllDtcStatusEventId,
                (void*)clearAllDtcStatusIndPtr);
    }
    else
    {
        LE_ERROR("Service reference not found to notify");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get DTC status event ID, used to notify the status change.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t taf_DTCSvr::GetDtcStatusEvent
(
    taf_diagDTC_ServiceRef_t svcRef
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "svcRef is null");

    taf_diagDTC_DtcCtx_t* dtcCtxPtr = GetDtcCtx(svcRef);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, NULL, "dtcCtxPtr is null");

    return dtcCtxPtr->dtcStatusEventId;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get Clear DTC status event ID, used to notify the status change.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t taf_DTCSvr::GetClearDtcStatusEvent
(
    taf_diagDTC_ServiceRef_t svcRef
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "svcRef is null");

    taf_diagDTC_DtcCtx_t* dtcCtxPtr = GetDtcCtx(svcRef);

    TAF_ERROR_IF_RET_VAL(dtcCtxPtr == NULL, NULL, "dtcCtxPtr is null");

    return dtcCtxPtr->clearDtcStatusEventId;
}

//-------------------------------------------------------------------------------------------------
/**
 * Update suppression status for all DTC.
 */
//-------------------------------------------------------------------------------------------------
void taf_DTCSvr::UpdateAllDtcSuppressionStatus
(
    bool suppressionStatus
)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&DtcCtxList);
    while (linkPtr)
    {
        taf_diagDTC_DtcCtx_t* dtcCtxPtr = CONTAINER_OF(linkPtr, taf_diagDTC_DtcCtx_t,
                link);
        linkPtr = le_dls_PeekNext(&DtcCtxList, linkPtr);

        dtcCtxPtr->suppressionStatus = suppressionStatus;
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Create a service reference for all DTC, or get the service reference of all DTC if the reference
 * already exist.
 */
//-------------------------------------------------------------------------------------------------
taf_diagDTC_AllServiceRef_t taf_DTCSvr::GetAllService
(
)
{
    le_msg_SessionRef_t sessionRef = taf_diagDTC_GetClientSessionRef();

    //Service reference is not created
    TAF_ERROR_IF_RET_VAL(AllDtcCtx.svcRef == NULL, NULL, "Failed to get service reference");

    // Add client reference to the session list
    AddSessionToAllDtcCtx(sessionRef);

    return AllDtcCtx.svcRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get all DTC status event ID, used to notify the status change.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t taf_DTCSvr::GetAllDtcStatusEvent
(
    taf_diagDTC_AllServiceRef_t svcRef
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "Null ptr(svcRef)");

    return AllDtcCtx.allDtcStatusEventId;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get all Clear DTC status event ID, used to notify the status change.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t taf_DTCSvr::GetClearAllDtcStatusEvent
(
    taf_diagDTC_AllServiceRef_t svcRef
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "Null ptr(svcRef)");

    return AllDtcCtx.clearAllDtcStatusEventId;
}

//--------------------------------------------------------------------------------------------------
/**
 * FirstLayerUdsStatusHandler.
 */
//--------------------------------------------------------------------------------------------------
void taf_DTCSvr::FirstLayerAllDtcStatusHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_diagDTC_AllStatus_t* dtcStatusEvent = (taf_diagDTC_AllStatus_t *)reportPtr;
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == NULL, "Null ptr(secondLayerHandlerFunc)");

    taf_diagDTC_AllStatusHandlerFunc_t handlerFunc =
                                    (taf_diagDTC_AllStatusHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc(dtcStatusEvent->svcRef, dtcStatusEvent->dtcCode, dtcStatusEvent->dtcStatus,
            le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * FirstLayerClearAllDtcStatusHandler.
 */
//--------------------------------------------------------------------------------------------------
void taf_DTCSvr::FirstLayerClearAllDtcStatusHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_diagDTC_ClearAllStatus_t* clearAllDtcStatusEvent = (taf_diagDTC_ClearAllStatus_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == NULL, "Null ptr(secondLayerHandlerFunc)");

    taf_diagDTC_ClearAllStatusHandlerFunc_t handlerFunc =
              (taf_diagDTC_ClearAllStatusHandlerFunc_t)secondLayerHandlerFunc;

    handlerFunc(clearAllDtcStatusEvent->svcRef, clearAllDtcStatusEvent->clientType,
              le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear All DTC information.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::ClearAllInfo
(
    taf_diagDTC_AllServiceRef_t svcRef,
    taf_diagDTC_ReqClientType_t clientType
)
{
    auto &diagEvent = taf_EventSvr::GetInstance();

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");

    return diagEvent.ClearDtc(ALL_DTC_GROUP, clientType);
}

//-------------------------------------------------------------------------------------------------
/**
 * Set suppression for ALL DTC.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::SetAllSuppression
(
    taf_diagDTC_AllServiceRef_t svcRef,
    bool suppressionStatus
)
{
    le_result_t result;
    auto &diagEvent = taf_EventSvr::GetInstance();

    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");

    //Call tafEvent module to set suppression status, event module need to check suppression status.
    result = diagEvent.SetAllDTCSuppression(suppressionStatus);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to set suppression for all DTC");
        return result;
    }

    UpdateAllDtcSuppressionStatus(suppressionStatus);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the created service of all DTC and release the allocated memory.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::RemoveAllSvc
(
    taf_diagDTC_AllServiceRef_t svcRef
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_BAD_PARAMETER, "svcRef is null");
    le_msg_SessionRef_t sessionRef = taf_diagDTC_GetClientSessionRef();

    RemoveSessionFromAllDtcCtx(sessionRef);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add client session into allDtc context.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::AddSessionToAllDtcCtx
(
    le_msg_SessionRef_t sessionRef
)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&(AllDtcCtx.sessionRefList));
    while (linkPtr)
    {
        taf_diagDTC_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkPtr,
                taf_diagDTC_SessionRef_t, link);
        linkPtr = le_dls_PeekNext(&(AllDtcCtx.sessionRefList), linkPtr);

        if (sessionRefPtr->sessionRef == sessionRef)
        {
            LE_DEBUG("Session(%p) has been added to AllDtcCtx", sessionRef);
            return LE_DUPLICATE;
        }
    }

    taf_diagDTC_SessionRef_t* newSessionRefPtr =
            (taf_diagDTC_SessionRef_t *)le_mem_ForceAlloc(SessionRefPool);

    newSessionRefPtr->sessionRef = sessionRef;
    newSessionRefPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&AllDtcCtx.sessionRefList, &(newSessionRefPtr->link));

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove client session from allDtc context.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DTCSvr::RemoveSessionFromAllDtcCtx
(
    le_msg_SessionRef_t sessionRef
)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&(AllDtcCtx.sessionRefList));
    while (linkPtr)
    {
        taf_diagDTC_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkPtr, taf_diagDTC_SessionRef_t,
                link);
        linkPtr = le_dls_PeekNext(&(AllDtcCtx.sessionRefList), linkPtr);

        if (sessionRefPtr->sessionRef == sessionRef)
        {
            LE_DEBUG("remove ref(%p) from AllDTC", sessionRef);
            le_dls_Remove(&AllDtcCtx.sessionRefList, &(sessionRefPtr->link));
            le_mem_Release(sessionRefPtr);
            return LE_OK;
        }
    }

    LE_DEBUG("Cannot found session context with ref(%p) from AllDTC", sessionRef);

    return LE_NOT_FOUND;
}

//-------------------------------------------------------------------------------------------------
/**
 * Intialize DTC context.
 */
//-------------------------------------------------------------------------------------------------
void taf_DTCSvr::InitDtcCtx
(
    uint32_t dtcCode
)
{
    taf_diagDTC_DtcCtx_t* dtcCtxPtr = NULL;
    auto& diagDTC = taf_DTCSvr::GetInstance();
    char dtcStatusName[32] = {0};
    char clearDtcStatusName[32] = {0};
    char actStatusName [32] = {0};

    dtcCtxPtr = (taf_diagDTC_DtcCtx_t *)le_mem_ForceAlloc(diagDTC.DtcPool);
    TAF_ERROR_IF_RET_NIL(dtcCtxPtr == NULL, "cannot alloc dtcCtxPtr");

    memset(dtcCtxPtr, 0, sizeof(taf_diagDTC_DtcCtx_t));

    dtcCtxPtr->dtcCode = dtcCode;

    //Create event id for DTC
    snprintf(dtcStatusName, sizeof(dtcStatusName)-1, "dtcEvId-%x", dtcCode);
    dtcCtxPtr->dtcStatusEventId = le_event_CreateIdWithRefCounting(dtcStatusName);

    //Create event id for clear DTC
    snprintf(clearDtcStatusName, sizeof(clearDtcStatusName)-1, "clearDtcCtx-%x", dtcCode);
    dtcCtxPtr->clearDtcStatusEventId = le_event_CreateIdWithRefCounting(clearDtcStatusName);

    //Create event id for DTC activation
    snprintf(actStatusName, sizeof(actStatusName)-1, "actEvId-%x", dtcCode);
    dtcCtxPtr->actStatusEvId = le_event_CreateIdWithRefCounting(actStatusName);

    //Get suppression status from database
    dtcCtxPtr->suppressionStatus = taf_DataAccess_GetDTCSuppression(dtcCode);

    LE_DEBUG("DTC code:0x%x, suppression status in DB:0x%x", dtcCode, dtcCtxPtr->suppressionStatus);

    dtcCtxPtr->link = LE_DLS_LINK_INIT;

    // add this DTC context to list
    le_dls_Queue(&diagDTC.DtcCtxList, &dtcCtxPtr->link);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Intialize all DTC context.
 */
//-------------------------------------------------------------------------------------------------
void taf_DTCSvr::InitAllDtcCtx
(
)
{
    //Create event id for allDTC
    AllDtcCtx.allDtcStatusEventId = le_event_CreateIdWithRefCounting("AllDtcCtx");

    //Create event id for clear all DTC
    AllDtcCtx.clearAllDtcStatusEventId = le_event_CreateIdWithRefCounting("ClearAllDtcCtx");

    AllDtcCtx.sessionRefList = LE_DLS_LIST_INIT;

    // Create a Safe Reference for service object
    taf_diagDTC_AllServiceRef_t allDtcRef = (taf_diagDTC_AllServiceRef_t)le_ref_CreateRef(
            AllSvcRefMap, &AllDtcCtx);

    TAF_ERROR_IF_RET_NIL(allDtcRef == NULL, "cannot alloc allDtcRef");

    AllDtcCtx.svcRef = allDtcRef;

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Handle the client disconnection.
 */
//-------------------------------------------------------------------------------------------------
void taf_DTCSvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    auto& diagDTC = taf_DTCSvr::GetInstance();
    LE_DEBUG(" Client %p disconnected", sessionRef);
    TAF_ERROR_IF_RET_NIL(sessionRef == NULL, "sessionRef is nullptr!");

    //Find DTC context one by one
    le_ref_IterRef_t iterRef = le_ref_GetIterator(diagDTC.SvcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_diagDTC_DtcCtx_t* dtcCtxPtr = (taf_diagDTC_DtcCtx_t*)le_ref_GetValue(iterRef);

        if (dtcCtxPtr != NULL)
        {
            //Remove client session reference from DTC session reference list
            diagDTC.RemoveSessionFromDtcCtx(dtcCtxPtr, sessionRef);

            //If session number of links is 0, release DTC context
            if( le_dls_NumLinks(&dtcCtxPtr->sessionRefList) == 0)
            {
                // Clear service object
                le_ref_DeleteRef(diagDTC.SvcRefMap, (void*)dtcCtxPtr->svcRef);
                dtcCtxPtr->svcRef = NULL;
            }
        }
    }

    //Remove session from AllDTC
    diagDTC.RemoveSessionFromAllDtcCtx(sessionRef);

}

//--------------------------------------------------------------------------------------------------
/**
 * Get DTC list and initialize the context.
 */
//--------------------------------------------------------------------------------------------------
void taf_DTCSvr::DtcConfiguration(cfg::Node & node)
{
    uint32_t dtcCode;

    std::map<uint32_t, std::shared_ptr<cfg::Node>> dtc_map = cfg::get_dtc_nodes();

    for (const auto & dtc: dtc_map)
    {
        dtcCode= dtc.first;

        if((dtcCode & 0xff000000) != 0)
        {
            LE_FATAL("Incorrect DTC code:%d",dtcCode);
        }

        InitDtcCtx(dtcCode);

    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_DTCSvr::Init
(
    void
)
{
    LE_INFO("taf_DTCSvr Init!");

    // Create memory pools for DTC service.
    DtcPool = le_mem_CreatePool("DtcServicePool", sizeof(taf_diagDTC_DtcCtx_t));

    // Create memory pools for DTC status.
    DtcStatusPool = le_mem_CreatePool("dtcStatusPool", sizeof(taf_diagDTC_Status_t));

    // Create memory pools for clear DTC status.
    ClearDtcStatusPool = le_mem_CreatePool("clearDtcStatusPool", sizeof(taf_diagDTC_ClearStatus_t));

    // Create memory pools for all DTC status.
    AllDtcStatusPool = le_mem_CreatePool("AllDtcStatusPool", sizeof(taf_diagDTC_AllStatus_t));

    // Create memory pools for clear DTC status.
    ClearAllDtcStatusPool = le_mem_CreatePool("clearAllDtcStatusPool",
            sizeof(taf_diagDTC_ClearAllStatus_t));
    // Create memory pools for activation status.
    ActStatusPool = le_mem_CreatePool("actStatusPool", sizeof(taf_diagDTC_ActStatus_t));

    // DTC data value pool allocation
    DtcDataValuePool = le_mem_InitStaticPool(DtcDataValueString,
                                                 TAF_DTC_DATA_VALUE_MAX_NUM,
                                                 TAF_DIAGDTC_DATA_VALUE_MAX_BYTES);
    // Most DTC data is short, so create them from a sub-pool
    DtcDataValuePool = le_mem_CreateReducedPool(DtcDataValuePool, "DtcDataValueSmallPool",
            TAF_DTC_DATA_VALUE_TIPICAL_NUM, TAF_DTC_DATA_VALUE_TIPICAL_BYTES);

    // Initiate memory pool for DtcDataList
    DtcDataListPool = le_mem_InitStaticPool(DtcDataListPool, TAF_DIAGDTC_DTC_DATA_LISTS_MAX_NUM,
            sizeof(taf_DtcDataList_t));

    // Initiate memory pool for DtcData
    DtcDataPool = le_mem_InitStaticPool(DtcDataPool, TAF_DIAGDTC_DTC_DATA_MAX_NUM,
            sizeof(taf_DtcData_t));

    // Initiate memory pool for DtcData safe reference
    DtcDataSafeRefPool = le_mem_InitStaticPool(DtcDataSafeRefPool, TAF_DIAGDTC_DTC_DATA_MAX_NUM,
            sizeof(taf_DtcDataSafeRef_t));

    // Initiate reference map for DTC data.
    DtcDataListRefMap = le_ref_InitStaticMap(DtcDataListRefMap, TAF_DIAGDTC_DTC_DATA_LISTS_MAX_NUM);
    DtcDataSafeRefMap = le_ref_InitStaticMap(DtcDataSafeRefMap, TAF_DIAGDTC_DTC_DATA_MAX_NUM);

    // Create session reference pools.
    SessionRefPool = le_mem_InitStaticPool(tafDtcSessionRef, TAF_DTC_MAX_SESSION_REF,
            sizeof(taf_diagDTC_SessionRef_t));

    // Create reference maps for DTC service.
    SvcRefMap = le_ref_CreateMap("DtcSvcRefMap", DEFAULT_DTC_SVC_REF_CNT);

    // Create reference maps for all DTC service.
    AllSvcRefMap = le_ref_CreateMap("AllDtcSvcRefMap", DEFAULT_ALL_DTC_SVC_REF_CNT);

    // Set client session close handler.
    le_msg_AddServiceCloseHandler(taf_diagDTC_GetServiceRef(), OnClientDisconnection, NULL);

    //Initialize the data from configuration module.
    try
    {
        cfg::Node & root = cfg::get_root_node();
        DtcConfiguration(root);
    }
    catch (const std::exception& e)
    {
        LE_FATAL("Exception: %s", e.what() );
    }

    InitAllDtcCtx();

}
