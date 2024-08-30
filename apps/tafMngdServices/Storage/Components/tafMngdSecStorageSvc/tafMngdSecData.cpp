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
#include "limit.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/xattr.h>
#include <openssl/md5.h>
#include <openssl/err.h>
#include <openssl/evp.h>

using namespace telux::tafsvc;

/**
 * Create data reference and item
 */
taf_mngdStorSec_DataRef_t tafMngdStorageSvc::CreateData
(
    const char* dataLabel
)
{
    LE_INFO("CreateData, dataLabel = %s", dataLabel);

    taf_mngdStorSec_DataRef_t dataRef;

    TAF_ERROR_IF_RET_VAL(
        CheckValidPosixFileName(dataLabel) != LE_OK,
        nullptr,
        "Invalid data label string");

    TAF_ERROR_IF_RET_VAL(
        CreateStorageDir() != LE_OK,
        nullptr,
        "Cannot create storage dir");

    tafMngdStorage_SecData_t *dataPtr = nullptr;

    char dataItemPath[LIMIT_MAX_PATH_BYTES] = {0};

    TAF_ERROR_IF_RET_VAL(GetDataPath(dataLabel,
                                                dataItemPath,
                                                LIMIT_MAX_PATH_BYTES) != LE_OK,
                            nullptr,
                            "cannot get data path");

    // Check if the data file already exists
    if(IsFileExisting(dataItemPath))
    {
        LE_ERROR("The data item already exists");
        return nullptr;
    }

    if(FindDataRef(dataLabel, &dataRef) == LE_NOT_FOUND)
    {
        LE_INFO("Create new data for '%s'", dataLabel);

        dataPtr = (tafMngdStorage_SecData_t*)le_mem_ForceAlloc(SecDataPool);

        memset((void*)dataPtr, 0, sizeof(tafMngdStorage_SecData_t));

        dataPtr->dataRef =
        (taf_mngdStorSec_DataRef_t)le_ref_CreateRef(SecDataRefMap, dataPtr);

        dataPtr->clientSessionRef = taf_mngdStorSec_GetClientSessionRef();

        snprintf(dataPtr->dataLabel, sizeof(dataPtr->dataLabel), "%s", dataLabel);

        dataPtr->isInWritingProcess = false;
        dataPtr->isInReadingProcess = false;
        dataRef = dataPtr->dataRef;

        LE_DEBUG("Create data file");

        if(CreateDataItem(dataRef) == LE_OK)
        {
            snprintf(dataPtr->path, sizeof(dataPtr->path), "%s", dataItemPath);
        }
        else
        {
            le_ref_DeleteRef(SecDataRefMap, dataRef);
            le_mem_Release(dataPtr);

            return nullptr;
        }
    }
    else
    {
        LE_ERROR("The data reference already exists");
        return nullptr;
    }

    return dataRef;
}

/**
 * Get data reference
 */
taf_mngdStorSec_DataRef_t tafMngdStorageSvc::GetDataRef
(
    const char* dataLabel
)
{
    LE_INFO("GetDataRef, dataLabel = %s", dataLabel);

    taf_mngdStorSec_DataRef_t dataRef;

   char dataItemPath[LIMIT_MAX_PATH_BYTES] = {0};

   TAF_ERROR_IF_RET_VAL(
        CheckValidPosixFileName(dataLabel) != LE_OK,
        nullptr,
        "Invalid data label string");

    TAF_ERROR_IF_RET_VAL(GetDataPath(dataLabel,
                                                dataItemPath,
                                                LIMIT_MAX_PATH_BYTES) != LE_OK,
                            nullptr,
                            "cannot get data path");

    // Check if the data file exists
    if(IsFileExisting(dataItemPath) == false)
    {
        LE_ERROR("The data item doesn't exist");
        return nullptr;
    }

    tafMngdStorage_SecData_t *dataPtr = nullptr;

    if(FindDataRef(dataLabel, &dataRef) == LE_NOT_FOUND)
    {
        LE_DEBUG("Create new data for '%s'", dataLabel);

        dataPtr = (tafMngdStorage_SecData_t*)le_mem_ForceAlloc(SecDataPool);

        memset((void*)dataPtr, 0, sizeof(tafMngdStorage_SecData_t));

        dataPtr->dataRef =
        (taf_mngdStorSec_DataRef_t)le_ref_CreateRef(SecDataRefMap, dataPtr);

        dataPtr->clientSessionRef = taf_mngdStorSec_GetClientSessionRef();
        snprintf(dataPtr->dataLabel, sizeof(dataPtr->dataLabel), "%s", dataLabel);
        snprintf(dataPtr->path, sizeof(dataPtr->path), "%s", dataItemPath);

        dataPtr->isInWritingProcess = false;
        dataPtr->isInReadingProcess = false;

        dataRef = dataPtr->dataRef;
    }

    LE_DEBUG("Get data reference for '%s'", dataLabel);

    return dataRef;
}

/**
 * Find data reference
 */
le_result_t tafMngdStorageSvc::FindDataRef
(
    const char* dataLabel,
    taf_mngdStorSec_DataRef_t* dataRef
)
{
    LE_DEBUG("FindDataRef");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(SecDataRefMap);

    // Scan all the data nodes
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        tafMngdStorage_SecData_t* dataPtr = (tafMngdStorage_SecData_t*)le_ref_GetValue(iterRef);
        if(dataPtr == nullptr)
        {
            LE_ERROR("dataPtr == nullptr");
            return LE_FAULT;
        }

        // Find the node that context matches to the current client and storage but session has been closed
        if ((strcmp(dataPtr->dataLabel, dataLabel) == 0) &&
            dataPtr->clientSessionRef == taf_mngdStorSec_GetClientSessionRef())
        {
            (*dataRef) = (taf_mngdStorSec_DataRef_t)le_ref_GetSafeRef(iterRef);

            LE_DEBUG("Find data '%s' for client session (%p)",
                    dataPtr->dataLabel, dataPtr->clientSessionRef);

            return LE_OK;
        }
    }

    return LE_NOT_FOUND;
}

/**
 * Get data path
 */
le_result_t tafMngdStorageSvc::GetDataPath
(
    const char* dataLabel,
    char* bufferPtr,
    size_t bufferSize
)
{
    char storagePath[LIMIT_MAX_PATH_BYTES] = {0};

    le_result_t result = GetStoragePath(storagePath, sizeof(storagePath));

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Cannot get storage path");

    snprintf(bufferPtr, bufferSize, "%s/%s", storagePath, dataLabel);

    LE_INFO("storage path is: %s", bufferPtr);

    return LE_OK;
}

/**
 * Create data item
 */
le_result_t tafMngdStorageSvc::CreateDataItem
(
    taf_mngdStorSec_DataRef_t dataRef
)
{
    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, dataRef);

    char dataItemPath[LIMIT_MAX_PATH_BYTES] = {0};

    TAF_ERROR_IF_RET_VAL(GetDataPath(dataPtr->dataLabel,
                                                dataItemPath,
                                                LIMIT_MAX_PATH_BYTES) != LE_OK,
                            LE_BAD_PARAMETER,
                            "cannot get data item path");

    dataPtr->writeOp.outputFd = taf_rfs_Open(dataItemPath, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
    if (dataPtr->writeOp.outputFd >= 0)
    {
        LE_INFO("File %s created successfully.", dataItemPath);
        taf_rfs_Close(dataPtr->writeOp.outputFd);
    }
    else
    {
        LE_ERROR("Error creating file");
        return LE_FAULT;
    }

    return LE_OK;
}

/**
 * Release data reference
 */
void tafMngdStorageSvc::ReleaseDataRef
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    le_ref_IterRef_t iterRef = le_ref_GetIterator(mss.SecDataRefMap);

    tafMngdStorage_SecData_t* dataPtr = nullptr;

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        dataPtr = (tafMngdStorage_SecData_t*)le_ref_GetValue(iterRef);
        if(dataPtr == nullptr)
        {
            LE_ERROR("dataPtr == nullptr");
            continue; // Skip to next node
        }

        if ((dataPtr->dataRef != nullptr) &&
            (dataPtr->clientSessionRef == sessionRef))
        {
            if(dataPtr->isInWritingProcess == true)
            {
                taf_rfs_Close(dataPtr->writeOp.outputFd);
            }

            if(dataPtr->isInReadingProcess == true)
            {
                taf_rfs_Close(dataPtr->readOp.outputFd);
            }

            LE_INFO("Remove data reference for session (%p)", dataPtr->clientSessionRef);

            le_ref_DeleteRef(mss.SecDataRefMap, dataPtr->dataRef);
            dataPtr->dataRef = nullptr;

            le_mem_Release(dataPtr);
            dataPtr = nullptr;
        }
    }
}

/**
 * Initialize write data operation
 */
le_result_t tafMngdStorageSvc::WriteDataStart
(
    taf_mngdStorSec_DataRef_t dataRef
)
{
    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr->isInWritingProcess == true,
                            LE_BUSY,
                            "data is in writing process");

    memset((void*)(&(dataPtr->writeOp)), 0, sizeof(WriteOp_t));

    char dataItemPath[LIMIT_MAX_PATH_BYTES] = {0};

    TAF_ERROR_IF_RET_VAL(GetDataPath(dataPtr->dataLabel,
                                            dataItemPath,
                                            LIMIT_MAX_PATH_BYTES) != LE_OK,
                            LE_BAD_PARAMETER,
                            "cannot get data item path");

    TAF_ERROR_IF_RET_VAL(IsFileExisting(dataItemPath) == false,
                            LE_UNAVAILABLE,
                            "data item does not exist");

    uint8_t nonce[EVP_MAX_MD_SIZE] = {0};

    uint8_t nonceData[TAF_MNGDSTORSEC_MAX_DATA_LABLE_BYTES] = {0};

    char ns[TAF_MNGDSTORSEC_MAX_DATA_LABLE_BYTES] = {0};

    TAF_ERROR_IF_RET_VAL(GetClientNamespace(ns,
                                                    sizeof(ns)) != LE_OK,
                            LE_BAD_PARAMETER,
                            "cannot get data namespace");

    memscpy(nonceData, sizeof(nonceData), ns, sizeof(ns));

    // Caculate the md5 of the file ID, later use the md5 as the nonce.
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if(ctx == nullptr)
    {
        LE_ERROR("ctx is nullptr");
        return LE_FAULT;
    }

    const EVP_MD* method = EVP_md5();

    EVP_DigestInit_ex(ctx, method, nullptr);
    EVP_DigestUpdate(ctx, nonceData, sizeof(nonceData));
    EVP_DigestFinal_ex(ctx, nonce, nullptr);
    EVP_MD_CTX_free(ctx);

    uint8_t aead[12] = {0};
    memscpy(aead, sizeof(aead), dataPtr->path, sizeof(dataPtr->path));

    const char* keyId = dataPtr->dataLabel;
    taf_ks_KeyRef_t* keyRefPtr = &(dataPtr->writeOp.keyRef);
    taf_ks_CryptoSessionRef_t* sessionRefPtr = &(dataPtr->writeOp.sessionRef);

    if (LE_NOT_FOUND == taf_ks_GetKey(keyId, keyRefPtr))
    {
        TAF_ERROR_IF_RET_VAL(LE_OK != taf_ks_CreateKey(keyId, TAF_KS_AES_ENCRYPT_DECRYPT, keyRefPtr),
                             LE_FAULT,
                             "Failed to create key");

        TAF_ERROR_IF_RET_VAL(LE_OK != taf_ks_ProvisionAesKeyValue(*keyRefPtr,
                                                                TAF_KS_AES_SIZE_256,
                                                                TAF_KS_AES_MODE_GCM,
                                                                nullptr, 0),
                            LE_FAULT,
                            "Failed to provision key");
    }

    TAF_ERROR_IF_RET_VAL(LE_OK != taf_ks_CryptoSessionCreate(*keyRefPtr, sessionRefPtr),
                            LE_FAULT,
                            "Failed to create session");

    TAF_ERROR_IF_RET_VAL(LE_OK != taf_ks_CryptoSessionSetAesNonce(*sessionRefPtr, nonce, 12),
                            LE_FAULT,
                            "Failed to set nonce");

    TAF_ERROR_IF_RET_VAL(LE_OK != taf_ks_CryptoSessionStart(*sessionRefPtr, TAF_KS_CRYPTO_ENCRYPT),
                            LE_FAULT,
                            "Failed to start session");

    TAF_ERROR_IF_RET_VAL(LE_OK != taf_ks_CryptoSessionProcessAead(*sessionRefPtr,
                                                                    aead,
                                                                    sizeof(aead)),
                            LE_FAULT,
                            "Failed to process aead");

    dataPtr->isInWritingProcess = true;

    dataPtr->writeOp.outputFd = -1;

    dataPtr->writeOp.outputFd = taf_rfs_Open(dataPtr->path, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);

    if (dataPtr->writeOp.outputFd < 0)
    {
        LE_ERROR("Failed to open data file '%s'.", dataPtr->path);
        taf_rfs_Close(dataPtr->writeOp.outputFd);
        return LE_FAULT;
    }

    LE_INFO("WriteDataStart operation done");

    return LE_OK;
}

/**
 * Process write data operation
 */
le_result_t tafMngdStorageSvc::WriteDataChunk
(
    taf_mngdStorSec_DataRef_t dataRef,
    const uint8_t *bufferPtr,
    size_t bufferSize
)
{
    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr->isInWritingProcess == false,
                            LE_UNAVAILABLE,
                            "Data is not in writing process");

    TAF_ERROR_IF_RET_VAL(bufferSize > TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE,
                            LE_OVERFLOW,
                            "Data size %" PRIuS " is larger than the limitation %d",
                            bufferSize, TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE);

    TAF_ERROR_IF_RET_VAL(CheckSize(bufferSize) != LE_OK,
                            LE_OVERFLOW,
                            "Storage free size is not enough for the data size %" PRIuS,
                            bufferSize);

    taf_ks_CryptoSessionRef_t* sessionRefPtr = &(dataPtr->writeOp.sessionRef);
    uint8_t* encryptedData = dataPtr->writeOp.encryptedData;
    size_t* encryptedDataSize = &(dataPtr->writeOp.encryptedSize);

    *encryptedDataSize = sizeof(dataPtr->writeOp.encryptedData);

    le_result_t result = taf_ks_CryptoSessionProcess(*sessionRefPtr,
                                                        bufferPtr,
                                                        bufferSize,
                                                        encryptedData,
                                                        encryptedDataSize);

    TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "Process crypto session error");

    if (*encryptedDataSize > 0)
    {
        if(taf_rfs_Write(dataPtr->writeOp.outputFd,
                                    encryptedData,
                                    *encryptedDataSize) != (ssize_t)*encryptedDataSize)
        {
            LE_ERROR("Failed to write encrypted data to file '%s'.", dataPtr->path);
            taf_rfs_Close(dataPtr->writeOp.outputFd);
            return LE_FAULT;
        }
        LE_INFO("Write encrypted data size = %" PRIuS , *encryptedDataSize);
    }

    LE_INFO("WriteDataChunk operation done");

    return LE_OK;
}

le_result_t tafMngdStorageSvc::WriteDataEnd
(
    taf_mngdStorSec_DataRef_t dataRef
)
{
    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr->isInWritingProcess == false,
                            LE_UNAVAILABLE,
                            "data is not in writing process");

    taf_ks_CryptoSessionRef_t* sessionRefPtr = &(dataPtr->writeOp.sessionRef);
    uint8_t* encryptedData = dataPtr->writeOp.encryptedData;
    size_t* encryptedDataSize = &(dataPtr->writeOp.encryptedSize);

    *encryptedDataSize = sizeof(dataPtr->writeOp.encryptedData);

    le_result_t result = taf_ks_CryptoSessionEnd(*sessionRefPtr,
                                                    nullptr, 0,
                                                    encryptedData,
                                                    encryptedDataSize);

    TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "Failed to end crypto session");

    if (*encryptedDataSize > 0)
    {
        if(taf_rfs_Write(dataPtr->writeOp.outputFd,
                                    encryptedData,
                                    *encryptedDataSize) != (ssize_t)*encryptedDataSize)
        {
            LE_ERROR("Failed to write encrypted data to file '%s'", dataPtr->path);
            taf_rfs_Close(dataPtr->writeOp.outputFd);
            return LE_FAULT;
        }
        LE_INFO("Write encrypted data size = %" PRIuS, *encryptedDataSize);
    }

    taf_rfs_Close(dataPtr->writeOp.outputFd);

    dataPtr->isInWritingProcess = false;

    LE_INFO("WriteDataEnd operation done");

    return LE_OK;
}

le_result_t tafMngdStorageSvc::ReadDataFirstChunk
(
    taf_mngdStorSec_DataRef_t dataRef,
    uint8_t *bufferPtr,
    size_t *readSize
)
{
    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, dataRef);

    size_t buffSize = *readSize;
    *readSize = 0;

    TAF_ERROR_IF_RET_VAL(dataPtr->isInWritingProcess == true,
                            LE_BUSY,
                            "data is in writing process");

    TAF_ERROR_IF_RET_VAL(dataPtr->isInReadingProcess == true,
                            LE_BUSY,
                            "data is in reading process");

    TAF_ERROR_IF_RET_VAL(*readSize > TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE,
                            LE_OVERFLOW,
                            "Read size %" PRIuS " is larger than the limitation %d",
                            *readSize, TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE);

    memset((void*)(&(dataPtr->readOp)), 0, sizeof(ReadOp_t));

    uint8_t nonce[EVP_MAX_MD_SIZE] = {0};

    uint8_t nonceData[TAF_MNGDSTORSEC_MAX_DATA_LABLE_BYTES] = {0};

    char ns[TAF_MNGDSTORSEC_MAX_DATA_LABLE_BYTES] = {0};

    TAF_ERROR_IF_RET_VAL(GetClientNamespace(ns, sizeof(ns)) != LE_OK,
                            LE_BAD_PARAMETER,
                            "cannot get data namespace");

    memscpy(nonceData, sizeof(nonceData), ns, sizeof(ns));

    // Caculate the md5 of the file ID, later use the md5 as the nonce.
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if(ctx == nullptr)
    {
        LE_ERROR("ctx is nullptr");
        return LE_FAULT;
    }

    const EVP_MD* method = EVP_md5();

    EVP_DigestInit_ex(ctx, method, nullptr);
    EVP_DigestUpdate(ctx, nonceData, sizeof(nonceData));
    EVP_DigestFinal_ex(ctx, nonce, nullptr);
    EVP_MD_CTX_free(ctx);

    uint8_t aead[12] = {0};
    memscpy(aead, sizeof(aead), dataPtr->path, sizeof(dataPtr->path));

    const char* keyId = dataPtr->dataLabel;

    taf_ks_KeyRef_t* keyRefPtr = &(dataPtr->readOp.keyRef);
    taf_ks_CryptoSessionRef_t* sessionRefPtr = &(dataPtr->readOp.sessionRef);

    uint8_t* decryptedData = dataPtr->readOp.decryptedData;
    size_t* decryptedDataSize = &(dataPtr->readOp.decryptedSize);

    le_result_t result = LE_OK;

    TAF_ERROR_IF_RET_VAL(LE_OK != taf_ks_GetKey(keyId, keyRefPtr),
                            LE_FAULT,
                            "Failed to get key with key id: %s", keyId);

    TAF_ERROR_IF_RET_VAL(LE_OK != taf_ks_CryptoSessionCreate(*keyRefPtr, sessionRefPtr),
                        LE_FAULT,
                        "Failed to create session");

    TAF_ERROR_IF_RET_VAL(LE_OK != taf_ks_CryptoSessionSetAesNonce(*sessionRefPtr, nonce, 12),
                            LE_FAULT,
                            "Failed to set nonce");

    TAF_ERROR_IF_RET_VAL(LE_OK != taf_ks_CryptoSessionStart(*sessionRefPtr, TAF_KS_CRYPTO_DECRYPT),
                            LE_FAULT,
                            "Failed to start session");

    TAF_ERROR_IF_RET_VAL(LE_OK != taf_ks_CryptoSessionProcessAead(*sessionRefPtr,
                                                                    aead,
                                                                    sizeof(aead)),
                            LE_FAULT,
                            "Failed to process aead");

    struct stat fileStat = {0};
    uint8_t tmpBuf[TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE] = {0};
    size_t readFileSize = TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE;

    dataPtr->isInReadingProcess = true;

    dataPtr->readOp.outputFd = -1;

    dataPtr->readOp.outputFd = taf_rfs_Open(dataPtr->path, O_RDONLY, 0);

    size_t totalDecryptedSize = 0;

    if (dataPtr->readOp.outputFd < 0)
    {
        LE_ERROR("Failed to open data file '%s'.", dataPtr->path);
        taf_rfs_Close(dataPtr->readOp.outputFd);
        return LE_FAULT;
    }

    // Check the input data file size.
    fstat(dataPtr->readOp.outputFd, &fileStat);
    dataPtr->readOp.fileSize = fileStat.st_size;
    if (dataPtr->readOp.fileSize == 0)
    {
        LE_DEBUG("data is empty");

        taf_rfs_Close(dataPtr->readOp.outputFd);
        *readSize = 0;

        return LE_OK;
    }

    if(dataPtr->readOp.fileSize <= TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE)
    {
        ssize_t bytesRead = taf_rfs_Read(dataPtr->readOp.outputFd, tmpBuf, &readFileSize);

        if (bytesRead != (ssize_t)dataPtr->readOp.fileSize)
        {
            LE_ERROR("Error (%s) reading data file '%s'.", LE_ERRNO_TXT(errno), dataPtr->path);
            taf_rfs_Close(dataPtr->readOp.outputFd);
            return LE_FAULT;
        }

        // Decrypt the data
        *decryptedDataSize = sizeof(dataPtr->readOp.decryptedData);
        result = taf_ks_CryptoSessionProcess(*sessionRefPtr,
                                             tmpBuf,
                                             bytesRead,
                                             decryptedData,
                                             decryptedDataSize);

        totalDecryptedSize = *decryptedDataSize;

        TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "Failed to process crypto session");

        if (*decryptedDataSize > 0)
        {
            (*readSize) = memscpy(bufferPtr,
                                    buffSize,
                                    decryptedData,
                                    *decryptedDataSize);

            LE_INFO("Copy decrypted data size = %" PRIuS, *readSize);
        }

        *decryptedDataSize = sizeof(dataPtr->readOp.decryptedData);
        result = taf_ks_CryptoSessionEnd(*sessionRefPtr,
                                         nullptr, 0,
                                         decryptedData,
                                         decryptedDataSize);

        totalDecryptedSize += *decryptedDataSize;

        TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "Failed to end crypto session");

        if (*decryptedDataSize > 0)
        {
            *readSize +=
                memscpy(bufferPtr + *readSize,
                        buffSize - *readSize,
                        decryptedData,
                        *decryptedDataSize);

            LE_INFO("Copy decrypted data size = %" PRIuS, *readSize);
        }

        taf_rfs_Close(dataPtr->readOp.outputFd);

        dataPtr->isInReadingProcess = false;

        LE_INFO("Total output data size = %" PRIuS, *readSize);

        if(*readSize < totalDecryptedSize)
        {
            LE_ERROR("Total decrypted data size = %" PRIuS, totalDecryptedSize);

            return LE_OVERFLOW;
        }
    }
    else
    {
        dataPtr->readOp.readIterator = 0;

        size_t currentSize = TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE;

        ssize_t bytesRead = taf_rfs_Read(dataPtr->readOp.outputFd, tmpBuf, &currentSize);
        if (bytesRead != (ssize_t)currentSize)
        {
            LE_ERROR("Error (%s) reading data file '%s'.",
                        LE_ERRNO_TXT(errno), dataPtr->path);
            taf_rfs_Close(dataPtr->readOp.outputFd);
            return LE_FAULT;
        }

        // Encrypt the data chunk.
        *decryptedDataSize = sizeof(dataPtr->readOp.decryptedData);
        result = taf_ks_CryptoSessionProcess(*sessionRefPtr,
                                                tmpBuf,
                                                bytesRead,
                                                decryptedData,
                                                decryptedDataSize);

        totalDecryptedSize = *decryptedDataSize;

        TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "Failed to process crypto session");

        if (*decryptedDataSize > 0)
        {
            *readSize =
                memscpy(bufferPtr, buffSize, decryptedData, *decryptedDataSize);

            LE_INFO("Copy decrypted data size = %" PRIuS, *readSize);

            if(*readSize < totalDecryptedSize)
            {
                LE_ERROR("Total decrypted data size = %" PRIuS, totalDecryptedSize);

                dataPtr->isInReadingProcess = false;

                taf_rfs_Close(dataPtr->readOp.outputFd);

                return LE_OVERFLOW;
            }
            else
            {
                dataPtr->readOp.ReadDecryptedDataSize = *readSize;
            }
        }

        dataPtr->readOp.readIterator++;
    }

    return LE_OK;
}


le_result_t tafMngdStorageSvc::ReadDataNextChunk
(
    taf_mngdStorSec_DataRef_t dataRef,
    uint8_t *bufferPtr,
    size_t *readSize
)
{
    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, dataRef);

    size_t buffSize = *readSize;
    *readSize = 0;

    TAF_ERROR_IF_RET_VAL(dataPtr->isInWritingProcess == true,
                            LE_BUSY,
                            "data is in writing process");

    TAF_ERROR_IF_RET_VAL(dataPtr->isInReadingProcess == false,
                            LE_BUSY,
                            "data is not in reading process");

    TAF_ERROR_IF_RET_VAL(*readSize > TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE,
                            LE_OVERFLOW,
                            "Read size %" PRIuS " is larger than the limitation %d",
                            *readSize, TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE);

    uint8_t tmpBuf[TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE] = {0};

    taf_ks_CryptoSessionRef_t* sessionRefPtr = &(dataPtr->readOp.sessionRef);

    uint8_t* decryptedData = dataPtr->readOp.decryptedData;
    size_t* decryptedDataSize = &(dataPtr->readOp.decryptedSize);

    le_result_t result = LE_OK;

    if((dataPtr->readOp.readIterator * TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE) <
        dataPtr->readOp.fileSize)
    {
        size_t currentSize =
            dataPtr->readOp.fileSize -
            (dataPtr->readOp.readIterator * TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE);

        if (currentSize > TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE)
        {
            currentSize = TAF_MNGDSTORSEC_MAX_DATA_CHUNK_SIZE;
        }

        ssize_t bytesRead = taf_rfs_Read(dataPtr->readOp.outputFd, tmpBuf, &currentSize);
        if (bytesRead != (ssize_t)currentSize)
        {
            LE_ERROR("Error (%s) reading data file '%s'.",
                        LE_ERRNO_TXT(errno), dataPtr->path);
            taf_rfs_Close(dataPtr->readOp.outputFd);
            return LE_FAULT;
        }

        // Encrypt the data chunk.
        *decryptedDataSize = sizeof(dataPtr->readOp.decryptedData);
        result = taf_ks_CryptoSessionProcess(*sessionRefPtr,
                                                tmpBuf,
                                                bytesRead,
                                                decryptedData,
                                                decryptedDataSize);

        TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "Failed to process crypto session");

        if (*decryptedDataSize > 0)
        {
            *readSize =
                memscpy(bufferPtr, buffSize, decryptedData, *decryptedDataSize);

            LE_INFO("Copy decrypted data size = %" PRIuS, *readSize);
        }

        if(*readSize < *decryptedDataSize)
        {
            LE_ERROR("Total decrypted data size = %" PRIuS, *decryptedDataSize);

            dataPtr->isInReadingProcess = false;

            taf_rfs_Close(dataPtr->readOp.outputFd);

            return LE_OVERFLOW;
        }
        else
        {
            dataPtr->readOp.ReadDecryptedDataSize += *readSize;
        }

        dataPtr->readOp.readIterator++;
    }
    else
    {
        *decryptedDataSize = sizeof(dataPtr->readOp.decryptedData);
        result = taf_ks_CryptoSessionEnd(*sessionRefPtr,
                                         nullptr, 0,
                                         decryptedData,
                                         decryptedDataSize);

        TAF_ERROR_IF_RET_VAL(LE_OK != result, LE_FAULT, "Failed to end crypto session");

        if (*decryptedDataSize > 0)
        {
            *readSize =
                memscpy(bufferPtr, buffSize, decryptedData, *decryptedDataSize);

            LE_INFO("Copy decrypted data size =%" PRIuS, *readSize);
        }

        if(*readSize < *decryptedDataSize)
        {
            LE_ERROR("Total decrypted data size = %" PRIuS, *decryptedDataSize);

            dataPtr->isInReadingProcess = false;

            taf_rfs_Close(dataPtr->readOp.outputFd);

            return LE_OVERFLOW;
        }
        else
        {
            dataPtr->readOp.ReadDecryptedDataSize += *readSize;
        }

        LE_INFO("Total read decrypted data size = %" PRIuS,
                    dataPtr->readOp.ReadDecryptedDataSize);

        dataPtr->isInReadingProcess = false;

        taf_rfs_Close(dataPtr->readOp.outputFd);
    }

    return LE_OK;
}

le_result_t tafMngdStorageSvc::GetDataSize
(
    taf_mngdStorSec_DataRef_t dataRef,
    uint32_t *size
)
{
    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(GetDataPath(dataPtr->dataLabel,
                                            dataPtr->path,
                                            LIMIT_MAX_PATH_BYTES) != LE_OK,
                            LE_BAD_PARAMETER,
                            "cannot get data item path");

    TAF_ERROR_IF_RET_VAL(IsFileExisting(dataPtr->path) == false,
                            LE_BAD_PARAMETER,
                            "cannot find data");

    struct stat fileStat;

    // Get file statistics
    if (stat(dataPtr->path, &fileStat) == -1)
    {
        LE_ERROR("Failed to get file status");
        return LE_FAULT;
    }

    // Check if the entry is a regular file
    if (S_ISREG(fileStat.st_mode))
    {
        *size = (uint32_t)fileStat.st_size;
    }

    LE_INFO("File %s size is %u", dataPtr->path, *size);

    return LE_OK;
}

le_result_t tafMngdStorageSvc::DeleteData
(
    taf_mngdStorSec_DataRef_t dataRef
)
{
    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr->isInWritingProcess == true,
                            LE_BUSY,
                            "data is in writing process");

    TAF_ERROR_IF_RET_VAL(dataPtr->isInReadingProcess == true,
                            LE_BUSY,
                            "data is in reading process");

    TAF_ERROR_IF_RET_VAL(GetDataPath(dataPtr->dataLabel,
                                            dataPtr->path,
                                            LIMIT_MAX_PATH_BYTES) != LE_OK,
                            LE_BAD_PARAMETER,
                            "cannot get data item path");

    TAF_ERROR_IF_RET_VAL(IsFileExisting(dataPtr->path) == false,
                            LE_BAD_PARAMETER,
                            "cannot find data");

    taf_rfs_Delete(dataPtr->path);

    le_ref_DeleteRef(SecDataRefMap, dataPtr->dataRef);
    le_mem_Release(dataPtr);

    return LE_OK;
}

le_result_t tafMngdStorageSvc::CheckSize
(
    uint32_t writeSize
)
{
    // Consider uint32 overflow case
    if(writeSize > UINT32_MAX - GetStorageUsedSize())
    {
        LE_ERROR("Storage size not enough");
        return LE_NO_MEMORY;
    }

    // Check if free storage size is enough for the requested write operation
    if((writeSize + GetStorageUsedSize()) > GetStorageMaxSize())
    {
        LE_ERROR("Storage size not enough");
        return LE_NO_MEMORY;
    }

    return LE_OK;
}
