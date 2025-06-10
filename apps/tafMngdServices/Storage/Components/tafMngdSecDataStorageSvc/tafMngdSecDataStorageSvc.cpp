/*
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafMngdStorageSvc.hpp"

using namespace tafsvc;

/**
 * Gets the used size of the storage
 */
le_result_t taf_mngdStorSecData_GetUsedSize
(
    uint32_t *sizePtr
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(sizePtr == nullptr, LE_BAD_PARAMETER, "output pointer is NULL");
    *sizePtr = mss.GetStorageUsedSize(nullptr);

    return LE_OK;
}

/**
 * Gets the free size of the storage
 */
le_result_t taf_mngdStorSecData_GetFreeSize
(
    uint32_t *sizePtr
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(sizePtr == nullptr, LE_BAD_PARAMETER, "output pointer is NULL");
    *sizePtr = mss.GetStorageFreeSpace(nullptr);

    return LE_OK;
}

/**
 * Creates data reference for the new data name
 */
le_result_t taf_mngdStorSecData_CreateData
(
    const char *dataName
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.CreateData(dataName);
}

/**
 * Gets data reference for the new data label
 */
taf_mngdStorSecData_DataRef_t taf_mngdStorSecData_GetDataRef
(
    const char *dataLabel
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.GetDataRef(dataLabel);
}

/**
 * Initializes writing data operation for the data label
 */
le_result_t taf_mngdStorSecData_WriteDataStart
(
    taf_mngdStorSecData_DataRef_t dataRef
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.WriteDataStart(dataRef);
}

/**
 * Processes writing data operation for the data label
 */
le_result_t taf_mngdStorSecData_WriteDataChunk
(
    taf_mngdStorSecData_DataRef_t dataRef,
    const uint8_t *bufferPtr,
    size_t bufferSize
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.WriteDataChunk(dataRef, bufferPtr, bufferSize);
}

/**
 * Ends writing data operation for the data label
 */
le_result_t taf_mngdStorSecData_WriteDataEnd
(
    taf_mngdStorSecData_DataRef_t dataRef
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.WriteDataEnd(dataRef);
}

/**
 * Reads data of the data label
 */
le_result_t taf_mngdStorSecData_ReadDataFirstChunk
(
    taf_mngdStorSecData_DataRef_t dataRef,
    uint8_t *bufferPtr,
    size_t *readSize
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(bufferPtr == nullptr, LE_BAD_PARAMETER, "output pointer is NULL");
    return mss.ReadDataFirstChunk(dataRef, bufferPtr, readSize);
}

/**
 * Reads the next data chunk from the given data item
 */
le_result_t taf_mngdStorSecData_ReadDataNextChunk
(
    taf_mngdStorSecData_DataRef_t dataRef,
    uint8_t *bufferPtr,
    size_t *readSize
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(bufferPtr == nullptr, LE_BAD_PARAMETER, "output pointer is NULL");
    return mss.ReadDataNextChunk(dataRef, bufferPtr, readSize);
}

/**
 * Gets the data size for the given data item in bytes
 */
le_result_t taf_mngdStorSecData_GetDataSize
(
    taf_mngdStorSecData_DataRef_t dataRef,
    uint32_t *dataSize
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(dataSize == nullptr, LE_BAD_PARAMETER, "output pointer is NULL");
    return mss.GetDataSize(dataRef, dataSize);
}

/**
 * Deletes data and the data label
 */
le_result_t taf_mngdStorSecData_DeleteData
(
    taf_mngdStorSecData_DataRef_t dataRef
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.DeleteData(dataRef);
}

/**
 * Shares a data to another application to use
 */
le_result_t taf_mngdStorSecData_ShareData
(
    taf_mngdStorSecData_DataRef_t dataRef,
    const char* LE_NONNULL appName,
    taf_mngdStorSecData_DataUsage_t usage
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.ShareData(dataRef, appName, usage);
}

/**
 * Cancels the data sharing
 */
le_result_t taf_mngdStorSecData_CancelDataSharing
(
    taf_mngdStorSecData_DataRef_t dataRef,
    const char* LE_NONNULL appName
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.CancelDataSharing(dataRef, appName);;
}

/**
 * Gets the first application that the given data is shared to
 */
le_result_t taf_mngdStorSecData_GetFirstSharedApp
(
    taf_mngdStorSecData_DataRef_t dataRef,
    char* appName,
    size_t appNameSize,
    taf_mngdStorSecData_DataUsage_t* usage
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(appName == nullptr || usage == nullptr, LE_BAD_PARAMETER,
        "output pointer is NULL");
    return mss.GetFirstSharedApp(dataRef, appName, appNameSize, usage);
}

/**
 * Gets the next application that the given data is shared to
 */
le_result_t taf_mngdStorSecData_GetNextSharedApp
(
    taf_mngdStorSecData_DataRef_t dataRef,
    char* appName,
    size_t appNameSize,
    taf_mngdStorSecData_DataUsage_t* usage
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(appName == nullptr || usage == nullptr, LE_BAD_PARAMETER,
        "output pointer is NULL");
    return mss.GetNextSharedApp(dataRef, appName, appNameSize, usage);
}

/**
 * Add handler for data state change.
 */
taf_mngdStorSecData_DataStateChangeHandlerRef_t taf_mngdStorSecData_AddDataStateChangeHandler
(
    const char* LE_NONNULL dataName,
    const char* LE_NONNULL appName,
    taf_mngdStorSecData_DataStateChangeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.AddDataStateChangeHandler(dataName, appName, handlerPtr, contextPtr);
}

/**
 * Remove handler function for data state change.
 */
void taf_mngdStorSecData_RemoveDataStateChangeHandler
(
    taf_mngdStorSecData_DataStateChangeHandlerRef_t handlerRef
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    mss.RemoveDataStateChangeHandler(handlerRef);
}


COMPONENT_INIT
{
    LE_INFO("tafMngdStorageSvc COMPONENT init...");

    auto &mss = tafMngdStorageSvc::GetInstance();

    mss.Init();

    LE_INFO("tafMngdStorageSvc COMPONENT initialized");
}
