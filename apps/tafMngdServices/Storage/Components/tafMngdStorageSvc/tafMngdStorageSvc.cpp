/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "tafMngdStorageSvc.hpp"

using namespace telux::tafsvc;

/**
 * Gets the used size of the storage
 */
le_result_t taf_mngdStorSec_GetUsedSize
(
    uint32_t *sizePtr
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    *sizePtr = mss.GetStorageUsedSize();

    return LE_OK;
}

/**
 * Gets the free size of the storage
 */
le_result_t taf_mngdStorSec_GetFreeSize
(
    uint32_t *sizePtr
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    *sizePtr = mss.GetStorageFreeSpace();

    return LE_OK;
}

/**
 * Creates data reference for the new data label
 */
taf_mngdStorSec_DataRef_t taf_mngdStorSec_CreateData
(
    const char *dataLabel
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.CreateData(dataLabel);
}

/**
 * Gets data reference for the new data label
 */
taf_mngdStorSec_DataRef_t taf_mngdStorSec_GetDataRef
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
le_result_t taf_mngdStorSec_WriteDataStart
(
    taf_mngdStorSec_DataRef_t dataRef
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.WriteDataStart(dataRef);
}

/**
 * Processes writing data operation for the data label
 */
le_result_t taf_mngdStorSec_WriteDataChunk
(
    taf_mngdStorSec_DataRef_t dataRef,
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
le_result_t taf_mngdStorSec_WriteDataEnd
(
    taf_mngdStorSec_DataRef_t dataRef
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.WriteDataEnd(dataRef);
}

/**
 * Reads data of the data label
 */
le_result_t taf_mngdStorSec_ReadDataFirstChunk
(
    taf_mngdStorSec_DataRef_t dataRef,
    uint8_t *bufferPtr,
    size_t *readSize
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.ReadDataFirstChunk(dataRef, bufferPtr, readSize);
}

/**
 * Reads the next data chunk from the given data item
 */
le_result_t taf_mngdStorSec_ReadDataNextChunk
(
    taf_mngdStorSec_DataRef_t dataRef,
    uint8_t *bufferPtr,
    size_t *readSize
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.ReadDataNextChunk(dataRef, bufferPtr, readSize);
}

/**
 * Gets the data size for the given data item in bytes
 */
le_result_t taf_mngdStorSec_GetDataSize
(
    taf_mngdStorSec_DataRef_t dataRef,
    uint32_t *dataSize
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.GetDataSize(dataRef, dataSize);
}

/**
 * Deletes data and the data label
 */
le_result_t taf_mngdStorSec_DeleteData
(
    taf_mngdStorSec_DataRef_t dataRef
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.DeleteData(dataRef);
}

/**
 * Copy the file from the paths configured in the MSS configuration JSON file to configuration
 * storage and also checks the validity of file
 */
le_result_t taf_mngdStorCfg_UpdateFile
(
    void
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.UpdateFile();
}

/**
 * Replace the orignal configuration file with the updated file in config storage.
 */
le_result_t taf_mngdStorCfg_Sync
(
    void
){
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.Sync();
}

/**
 * Revert configuration file with orignal version in config storage.
 */
le_result_t taf_mngdStorCfg_Rollback
(
    void
)
{
    return LE_NOT_IMPLEMENTED;
}

/**
 * Get the configuration storage reference.
 */
taf_mngdStorCfg_ConfigRef_t  taf_mngdStorCfg_GetRef
(
)
{
    return nullptr;
}

/**
 * Get the major and minor version of the given configuration file reference.
 */
le_result_t taf_mngdStorCfg_GetVersion
(
    taf_mngdStorCfg_ConfigRef_t ConfigRef,
    uint32_t* MajorVersionPtr,
    uint32_t* MinorVersionPtr,
    uint32_t* PatchVersionPtr
)
{
    return LE_NOT_IMPLEMENTED;
}

/**
 * Get the value and data type of the node.
 */
le_result_t taf_mngdStorCfg_GetValue
(
    taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char* LE_NONNULL groupName,
    const char* LE_NONNULL nodeName,
    taf_mngdStorCfg_NodeType_t* typePtr,
    char* nodeValue,
    size_t nodeValueSize
)
{
    return LE_NOT_IMPLEMENTED;
}

/**
 * Get the data type of the node.
 */
le_result_t taf_mngdStorCfg_GetType
(
   taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char* LE_NONNULL groupName,
    const char* LE_NONNULL nodeName,
    taf_mngdStorCfg_NodeType_t* typePtr
)
{
    return LE_NOT_IMPLEMENTED;
}

/**
 * Get the value of the node with data type string.
 */
le_result_t taf_mngdStorCfg_GetString
(
    taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char* LE_NONNULL groupName,
    const char* LE_NONNULL nodeName,
    char* nodeValue,
    size_t nodeValueSize
)
{
    return LE_NOT_IMPLEMENTED;
}

/**
 * Get the value of the node with data type integer.
 */
le_result_t taf_mngdStorCfg_GetInt
(
    taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char* LE_NONNULL groupName,
    const char* LE_NONNULL nodeName,
    int32_t* nodeValuePtr
){
    return LE_NOT_IMPLEMENTED;
}

/**
 * Get the value of the node with data type float.
 */
le_result_t taf_mngdStorCfg_GetFloat
(
    taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char* LE_NONNULL groupName,
    const char* LE_NONNULL nodeName,
    double* nodeValuePtr
){
    return LE_NOT_IMPLEMENTED;
}

/**
 * Get the value of the node with data type boolean.
 */
le_result_t taf_mngdStorCfg_GetBool
(
    taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char* LE_NONNULL groupName,
    const char* LE_NONNULL nodeName,
    int32_t* nodeValuePtr
){
    return LE_NOT_IMPLEMENTED;
}

COMPONENT_INIT
{
    LE_INFO("tafMngdStorageSvc COMPONENT init...");

    auto &mss = tafMngdStorageSvc::GetInstance();

    mss.Init();
}
