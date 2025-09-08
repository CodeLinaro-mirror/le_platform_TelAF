/*
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
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

using namespace tafsvc;

/**
 * Create secure data reference and item
 */
tafMngdStorage_SecDataRef_t tafMngdStorageSvc::CreateSecDataRef
(
    const char* dataNamePtr,
    const char* appNamePtr
)
{
    LE_INFO("CreateSecDataRef for '%s'", dataNamePtr);

    tafMngdStorage_SecData_t *dataPtr = nullptr;
    tafMngdStorage_SecDataRef_t secDataRef = nullptr;
    char keyId[TAF_KS_MAX_KEY_ID_SIZE] = {0};
    char myAppName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };

    dataPtr = (tafMngdStorage_SecData_t*)le_mem_ForceAlloc(SecDataPool);
    if (dataPtr == nullptr)
    {
        LE_ERROR("Memory allocation failed");
        return nullptr;
    }

    memset((void*)dataPtr, 0, sizeof(tafMngdStorage_SecData_t));

    secDataRef = (tafMngdStorage_SecDataRef_t)le_ref_CreateRef(SecDataRefMap, dataPtr);
    if (secDataRef == nullptr)
    {
        LE_ERROR("Reference creation failed");
        goto cleanup;
    }

    snprintf(dataPtr->dataName, sizeof(dataPtr->dataName), "%s", dataNamePtr);

    if(appNamePtr == nullptr)
    {
        // Get appName from clientSession.
        if (LE_OK != GetAppNameBySessionRef(taf_mngdStorSecData_GetClientSessionRef(),
                                            myAppName, sizeof(myAppName)))
        {
            LE_ERROR("Failed to get client appName.");
            goto cleanup;
        }

        snprintf(dataPtr->ownerAppName, sizeof(dataPtr->ownerAppName), "%s", myAppName);
    }
    else
    {
        snprintf(dataPtr->ownerAppName, sizeof(dataPtr->ownerAppName), "%s", appNamePtr);
    }

    dataPtr->isInWritingProcess = false;
    dataPtr->isInReadingProcess = false;

    if (GetDataPath(appNamePtr, dataNamePtr, dataPtr->path, sizeof(dataPtr->path)) != LE_OK)
    {
        LE_ERROR("cannot get data path");
        goto cleanup;
    }

    if (GetDataKeyId(secDataRef, keyId, TAF_KS_MAX_KEY_ID_SIZE) != LE_OK)
    {
        LE_ERROR("cannot get data keyId");
        goto cleanup;
    }

    if (LE_NOT_FOUND == taf_ks_GetKey(keyId, &(dataPtr->keyRef)))
    {
        if (LE_OK != taf_ks_CreateKey(keyId, TAF_KS_AES_ENCRYPT_DECRYPT, &(dataPtr->keyRef)))
        {
            LE_ERROR("Failed to create key");
            goto cleanup;
        }

        if (LE_OK != taf_ks_ProvisionAesKeyValue(dataPtr->keyRef, TAF_KS_AES_SIZE_256, TAF_KS_AES_MODE_GCM, nullptr, 0))
        {
            LE_ERROR("Failed to provision key");
            goto cleanup;
        }
    }

    return secDataRef;

cleanup:
    le_mem_Release(dataPtr);
    return nullptr;
}


/**
 * Create secure data item
 */
le_result_t tafMngdStorageSvc::CreateData
(
    const char* dataName
)
{
    TAF_ERROR_IF_RET_VAL(
        CheckValidPosixFileName(dataName) != LE_OK,
        LE_BAD_PARAMETER,
        "Invalid data label string");

    LE_INFO("CreateData, dataName = %s", dataName);

    TAF_ERROR_IF_RET_VAL(
        CreateStorageDir() != LE_OK,
        LE_FAULT,
        "Cannot create storage dir");

    char dataItemPath[LIMIT_MAX_PATH_BYTES] = {0};

    TAF_ERROR_IF_RET_VAL(GetDataPath(nullptr,
                                        dataName,
                                        dataItemPath,
                                        LIMIT_MAX_PATH_BYTES) != LE_OK,
                            LE_FAULT,
                            "cannot get data path");

    // Check if the data file already exists
    if(IsFileExisting(dataItemPath))
    {
        LE_ERROR("The data item already exists");
        return LE_DUPLICATE;
    }

    char myAppName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };

    // Get appName from clientSession.
    if (LE_OK != GetAppNameBySessionRef(taf_mngdStorSecData_GetClientSessionRef(),
                                        myAppName, sizeof(myAppName)))
    {
        LE_ERROR("Failed to get client appName.");
        return LE_FAULT;
    }

    tafMngdStorage_SecDataRef_t secDataRef = nullptr;

    if(FindSecDataRef(dataName, myAppName, &secDataRef) == LE_NOT_FOUND)
    {
        secDataRef = CreateSecDataRef(dataName, myAppName);

        TAF_ERROR_IF_RET_VAL(secDataRef == nullptr, LE_FAULT, "Cannot create sec data ref");

        if(CreateDataItem(secDataRef) != LE_OK)
        {
            LE_ERROR("Failed to create data file for '%s'", dataName);

            tafMngdStorage_SecData_t* dataPtr =
                    (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, secDataRef);

            le_ref_DeleteRef(SecDataRefMap, secDataRef);
            le_mem_Release(dataPtr);

            return LE_FAULT;
        }
    }
    else
    {
        LE_ERROR("The data reference is already exists");
        return LE_DUPLICATE;
    }

    return LE_OK;
}

/**
 * Get data reference
 */
taf_mngdStorSecData_DataRef_t tafMngdStorageSvc::GetDataRef
(
    const char* dataLabel
)
{
    LE_INFO("GetDataRef, dataLabel = %s", dataLabel);

    char myAppName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };
    char ownerAppName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };

    // Get appName from clientSession.
    if (LE_OK != GetAppNameBySessionRef(taf_mngdStorSecData_GetClientSessionRef(),
                                        myAppName, sizeof(myAppName)))
    {
        LE_ERROR("Failed to get client appName.");
        return nullptr;
    }

    const char* dataNamePtr = nullptr;
    const char* ownerAppPtr = nullptr;

    // For a shared data, the dataLabel = "<appName>::<dataName>".
    // Otherwise it's owned by the calling app.
    dataNamePtr = strstr(dataLabel, "::");
    if (dataNamePtr != nullptr)
    {
        // Get the dataName from the dataLabel.
        dataNamePtr = dataNamePtr + 2;

        // Get the owner appName from the dataLabel.
        le_utf8_CopyUpToSubStr(ownerAppName, dataLabel, "::", sizeof(ownerAppName), nullptr);

        // Sanity check.
        if ((dataNamePtr[0] == '\0') || (ownerAppName[0] == '\0'))
        {
            LE_ERROR("Invalid dataLabel format (%s).", dataLabel);
            return nullptr;
        }

        ownerAppPtr = ownerAppName;
        if (strcmp(myAppName, ownerAppName) == 0)
        {
            ownerAppPtr = nullptr;
        }
        else
        {
            TAF_ERROR_IF_RET_VAL(CheckValidPosixFileName(ownerAppPtr) != LE_OK,
                            nullptr,
                            "Invalid owner app string");
        }
    }
    else
    {
        dataNamePtr = dataLabel;
    }

    TAF_ERROR_IF_RET_VAL(CheckValidPosixFileName(dataNamePtr) != LE_OK,
                            nullptr,
                            "Invalid data label string");

    char dataItemPath[LIMIT_MAX_PATH_BYTES] = {0};

    TAF_ERROR_IF_RET_VAL(GetDataPath(ownerAppPtr,
                                        dataNamePtr,
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

    taf_mngdStorSecData_DataRef_t dataRef = nullptr;
    tafMngdStorage_ClientData_t *clientDataPtr = nullptr;

    if(FindClientDataRef(dataNamePtr, &dataRef) == LE_NOT_FOUND)
    {
        LE_INFO("Create new client data ref for '%s'", dataNamePtr);

        clientDataPtr = (tafMngdStorage_ClientData_t*)le_mem_ForceAlloc(ClientDataPool);

        memset((void*)clientDataPtr, 0, sizeof(tafMngdStorage_ClientData_t));

        clientDataPtr->dataRef =
        (taf_mngdStorSecData_DataRef_t)le_ref_CreateRef(ClientDataRefMap, clientDataPtr);

        clientDataPtr->clientSessionRef = taf_mngdStorSecData_GetClientSessionRef();

        snprintf(clientDataPtr->dataLabel, sizeof(clientDataPtr->dataLabel), "%s", dataLabel);

        dataRef = clientDataPtr->dataRef;
    }
    else
    {
        clientDataPtr = (tafMngdStorage_ClientData_t*)le_ref_Lookup(ClientDataRefMap, dataRef);
    }

    // Check if the clientData is valid
    TAF_ERROR_IF_RET_VAL(clientDataPtr == nullptr,nullptr, "Invalid client data reference");

    if(FindSecDataRef(dataNamePtr, ownerAppPtr, &(clientDataPtr->secDataRef)) == LE_NOT_FOUND)
    {
        clientDataPtr->secDataRef = CreateSecDataRef(dataNamePtr, ownerAppPtr);
        TAF_ERROR_IF_RET_VAL(clientDataPtr->secDataRef == nullptr,
                                nullptr,
                                "Cannot create sec data ref");
    }

    RefreshSharedAppInfo(clientDataPtr->secDataRef);

    if(ownerAppPtr != nullptr)
    {
        LE_INFO("Check shared app list for %s", dataLabel);

        if(IsInSharedAppList(dataRef, myAppName) == false)
        {
            LE_ERROR("app: %s is not shared app for data: %s", myAppName, dataLabel);

            tafMngdStorage_SecData_t* dataPtr =
                (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, clientDataPtr->secDataRef);

            le_ref_DeleteRef(SecDataRefMap, clientDataPtr->secDataRef);
            le_mem_Release(dataPtr);

            le_ref_DeleteRef(ClientDataRefMap, dataRef);
            le_mem_Release(clientDataPtr);

            return nullptr;
        }
        else
        {
            clientDataPtr->sharedClient = true;
        }
    }

    LE_DEBUG("Get data reference for '%s'", dataLabel);

    return dataRef;
}

/**
 * Find client data reference
 */
le_result_t tafMngdStorageSvc::FindClientDataRef
(
    const char* dataLabel,
    taf_mngdStorSecData_DataRef_t* dataRef
)
{
    LE_DEBUG("FindClientDataRef");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(ClientDataRefMap);

    // Scan all the data nodes
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        tafMngdStorage_ClientData_t* dataPtr = (tafMngdStorage_ClientData_t*)le_ref_GetValue(iterRef);
        if(dataPtr == nullptr)
        {
            LE_ERROR("dataPtr == nullptr");
            return LE_FAULT;
        }

        // Find the node that context matches to the current client
        if ((strcmp(dataPtr->dataLabel, dataLabel) == 0) &&
             dataPtr->clientSessionRef == taf_mngdStorSecData_GetClientSessionRef())
        {
            (*dataRef) = (taf_mngdStorSecData_DataRef_t)le_ref_GetSafeRef(iterRef);

            LE_DEBUG("Find data '%s' for client session (%p)",
                    dataPtr->dataLabel, dataPtr->clientSessionRef);

            return LE_OK;
        }
    }

    return LE_NOT_FOUND;
}

/**
 * Find secure data reference
 */
le_result_t tafMngdStorageSvc::FindSecDataRef
(
    const char* dataName,
    const char* ownerAppName,
    tafMngdStorage_SecDataRef_t* dataRef
)
{
    LE_DEBUG("FindSecDataRef");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(SecDataRefMap);

    char appName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };

    if(ownerAppName == nullptr)
    {
        // Get appName from clientSession.
        if (LE_OK != GetAppNameBySessionRef(taf_mngdStorSecData_GetClientSessionRef(),
                                            appName, sizeof(appName)))
        {
            LE_ERROR("Failed to get client appName.");
            return LE_FAULT;
        }
    }
    else
    {
        snprintf(appName, sizeof(appName), "%s", ownerAppName);
    }

    // Scan all the data nodes
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        tafMngdStorage_SecData_t* dataPtr = (tafMngdStorage_SecData_t*)le_ref_GetValue(iterRef);
        if(dataPtr == nullptr)
        {
            LE_ERROR("dataPtr == nullptr");
            return LE_FAULT;
        }

        // Find the node that context matches to the current data
        if ((strcmp(dataPtr->dataName, dataName) == 0) &&
            (strcmp(dataPtr->ownerAppName, appName) == 0))
        {
            (*dataRef) = (tafMngdStorage_SecDataRef_t)le_ref_GetSafeRef(iterRef);

            LE_DEBUG("Find data '%s::%s'", dataPtr->ownerAppName, dataPtr->dataName);

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
    const char* storageName,
    const char* dataName,
    char* bufferPtr,
    size_t bufferSize
)
{
    char storagePath[LIMIT_MAX_PATH_BYTES] = {0};

    if(storageName == nullptr)
    {
        le_result_t result = GetStoragePath(storagePath, sizeof(storagePath));

        TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Cannot get storage path");

        snprintf(bufferPtr, bufferSize, "%s/%s", storagePath, dataName);
    }
    else
    {
        snprintf(bufferPtr, bufferSize, "%s%s/%s", secDataStorage, storageName, dataName);
    }

    LE_INFO("data path is: %s", bufferPtr);

    return LE_OK;
}

/**
 * Open temp file
 */
int tafMngdStorageSvc::OpenTempFile
(
    const char *filename
)
{
    char temp_filename[LIMIT_MAX_PATH_BYTES + sizeof(SECURE_DATA_TEMP_EXTENSION)];
    snprintf(temp_filename, sizeof(temp_filename), "%s%s", filename, SECURE_DATA_TEMP_EXTENSION);
    return taf_rfs_Open(temp_filename, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
}

/**
 * Rename temp file
 */
int tafMngdStorageSvc::RenameTempFile
(
    const char *filename
)
{
    char temp_filename[LIMIT_MAX_PATH_BYTES + sizeof(SECURE_DATA_TEMP_EXTENSION)];
    snprintf(temp_filename, sizeof(temp_filename), "%s%s", filename, SECURE_DATA_TEMP_EXTENSION);

    if (taf_rfs_Rename(temp_filename, filename) != 0)
    {
        return -1;
    }

    LE_INFO("Renamed to data label: %s", filename);

    // Ensure rename is synced
    char dirPath[LIMIT_MAX_PATH_BYTES];
    snprintf(dirPath, sizeof(dirPath), "%s", filename);
    dirPath[LIMIT_MAX_PATH_BYTES - 1] = '\0'; // Ensure null termination
    char* dirName = dirname(dirPath);

    int dirFd = open(dirName, O_DIRECTORY | O_RDONLY);
    if (dirFd >= 0)
    {
        if (fsync(dirFd) != 0)
        {
            LE_WARN("Failed to fsync directory '%s'", dirName);
            close(dirFd);
            return -1;
        }
        close(dirFd);
    }
    else
    {
        LE_WARN("Failed to open directory '%s' for fsync", dirName);
        return -1;
    }
    LE_INFO("fsync for %s finished", filename);

    return 0;
}


/**
 * Delete temp file
 */
void tafMngdStorageSvc::DeleteTempFile
(
    const char *filename
)
{
    char temp_filename[LIMIT_MAX_PATH_BYTES + sizeof(SECURE_DATA_TEMP_EXTENSION)];
    snprintf(temp_filename, sizeof(temp_filename), "%s%s", filename, SECURE_DATA_TEMP_EXTENSION);
    taf_rfs_Delete(temp_filename);
}

le_result_t tafMngdStorageSvc::GetDataKeyId
(
    tafMngdStorage_SecDataRef_t dataRef,
    char* keyId,
    size_t keySize
)
{
    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid data ref");

    snprintf(keyId, keySize, "%s%s", dataPtr->ownerAppName, dataPtr->dataName);

    return LE_OK;
}

/**
 * Create data item
 */
le_result_t tafMngdStorageSvc::CreateDataItem
(
    tafMngdStorage_SecDataRef_t dataRef
)
{
    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid data ref");

    char dataItemPath[LIMIT_MAX_PATH_BYTES] = {0};

    TAF_ERROR_IF_RET_VAL(GetDataPath(nullptr,
                                        dataPtr->dataName,
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

    le_ref_IterRef_t iterRef = le_ref_GetIterator(mss.ClientDataRefMap);

    tafMngdStorage_ClientData_t* clientDataPtr = nullptr;

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        clientDataPtr = (tafMngdStorage_ClientData_t*)le_ref_GetValue(iterRef);
        if(clientDataPtr == nullptr)
        {
            LE_ERROR("clientDataPtr == nullptr");
            continue; // Skip to next node
        }

        if ((clientDataPtr->dataRef != nullptr) &&
            (clientDataPtr->clientSessionRef == sessionRef))
        {
            tafMngdStorage_SecData_t* dataPtr =
            (tafMngdStorage_SecData_t*)le_ref_Lookup(mss.SecDataRefMap, clientDataPtr->secDataRef);

            if(dataPtr != nullptr)
            {
                if(dataPtr->isInWritingProcess == true &&
                    dataPtr->writeOp.clientSessionRef == sessionRef)
                {
                    taf_rfs_Close(dataPtr->writeOp.outputFd);
                    dataPtr->isInWritingProcess = false;
                    dataPtr->writeOp.clientSessionRef = nullptr;
                }

                if(dataPtr->isInReadingProcess == true &&
                    dataPtr->readOp.clientSessionRef == sessionRef)
                {
                    taf_rfs_Close(dataPtr->readOp.outputFd);
                    dataPtr->isInReadingProcess = false;
                    dataPtr->writeOp.clientSessionRef = nullptr;
                }
            }

            LE_INFO("Remove data reference for session (%p)", clientDataPtr->clientSessionRef);

            le_ref_DeleteRef(mss.ClientDataRefMap, clientDataPtr->dataRef);
            clientDataPtr->dataRef = nullptr;

            le_mem_Release(clientDataPtr);
            clientDataPtr = nullptr;
        }
    }
}

/**
 * Initialize write data operation
 */
le_result_t tafMngdStorageSvc::WriteDataStart
(
    taf_mngdStorSecData_DataRef_t dataRef
)
{
    tafMngdStorage_ClientData_t* clientDataPtr =
        (tafMngdStorage_ClientData_t*)le_ref_Lookup(ClientDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(clientDataPtr == nullptr, LE_NOT_FOUND, "invalid client data ref");

    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, clientDataPtr->secDataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid secure data ref");

    TAF_ERROR_IF_RET_VAL(dataPtr->isInWritingProcess == true,
                            LE_BUSY,
                            "data is in writing process");

    TAF_ERROR_IF_RET_VAL(dataPtr->isInReadingProcess == true,
                            LE_BUSY,
                            "data is in reading process");

    memset((void*)(&(dataPtr->writeOp)), 0, sizeof(WriteOp_t));

    if(clientDataPtr->sharedClient == true)
    {
        char myAppName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };

        // Get appName from clientSession.
        if (LE_OK != GetAppNameBySessionRef(taf_mngdStorSecData_GetClientSessionRef(),
                                            myAppName, sizeof(myAppName)))
        {
            LE_ERROR("Failed to get client appName.");
            return LE_FAULT;
        }

        taf_mngdStorSecData_DataUsage_t usage = GetSharedAppUsage(dataRef, myAppName);

        if(usage != TAF_MNGDSTORSECDATA_USAGE_READ_WRITE &&
            usage != TAF_MNGDSTORSECDATA_USAGE_WRITE)
        {
            LE_ERROR("Doesn't have write access");
            return LE_NOT_PERMITTED;
        }
    }

    dataPtr->writeOp.clientSessionRef = taf_mngdStorSecData_GetClientSessionRef();

    TAF_ERROR_IF_RET_VAL(IsFileExisting(dataPtr->path) == false,
                            LE_UNAVAILABLE,
                            "data item does not exist");

    uint8_t nonce[EVP_MAX_MD_SIZE] = {0};

    uint8_t nonceData[TAF_MNGDSTORSECDATA_MAX_DATA_LABEL_BYTES] = {0};

    char ns[TAF_MNGDSTORSECDATA_MAX_DATA_LABEL_BYTES] = {0};

    TAF_ERROR_IF_RET_VAL(GetClientNamespace(ns,
                            sizeof(ns)) != LE_OK,
                            LE_BAD_PARAMETER,
                            "cannot get data namespace");

    memscpy(nonceData, sizeof(nonceData), dataPtr->ownerAppName, sizeof(dataPtr->ownerAppName));

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

    char keyId[TAF_KS_MAX_KEY_ID_SIZE] = {0};
    TAF_ERROR_IF_RET_VAL(GetDataKeyId(clientDataPtr->secDataRef,
                                        keyId,
                                        TAF_KS_MAX_KEY_ID_SIZE) != LE_OK,
                            LE_FAULT,
                            "cannot get data keyId");

    taf_ks_KeyRef_t* keyRefPtr = &(dataPtr->keyRef);
    taf_ks_CryptoSessionRef_t* sessionRefPtr = &(dataPtr->writeOp.sessionRef);

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

    dataPtr->writeOp.outputFd = OpenTempFile(dataPtr->path);

    if (dataPtr->writeOp.outputFd < 0)
    {
        LE_ERROR("Failed to open data file '%s'.", dataPtr->path);

        // Abort the crypto session when error
        taf_ks_CryptoSessionAbort(*sessionRefPtr);
        dataPtr->isInWritingProcess = false;
        dataPtr->writeOp.sessionRef = nullptr;
        dataPtr->writeOp.clientSessionRef = nullptr;

        DeleteTempFile(dataPtr->path);

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
    taf_mngdStorSecData_DataRef_t dataRef,
    const uint8_t *bufferPtr,
    size_t bufferSize
)
{
    tafMngdStorage_ClientData_t* clientDataPtr =
        (tafMngdStorage_ClientData_t*)le_ref_Lookup(ClientDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(clientDataPtr == nullptr, LE_NOT_FOUND, "invalid client data ref");

    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, clientDataPtr->secDataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid secure data ref");

    TAF_ERROR_IF_RET_VAL(dataPtr->isInWritingProcess == false,
                            LE_UNAVAILABLE,
                            "Data is not in writing process");

    TAF_ERROR_IF_RET_VAL(dataPtr->writeOp.clientSessionRef !=
                            taf_mngdStorSecData_GetClientSessionRef(),
                            LE_BUSY,
                            "Data is in writing by another session");

    TAF_ERROR_IF_RET_VAL(bufferSize > TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE,
                            LE_OVERFLOW,
                            "Data size %" PRIuS " is larger than the limitation %d",
                            bufferSize, TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE);

    taf_ks_CryptoSessionRef_t* sessionRefPtr = &(dataPtr->writeOp.sessionRef);
    uint8_t* encryptedData = dataPtr->writeOp.encryptedData;
    size_t* encryptedDataSize = &(dataPtr->writeOp.encryptedSize);
    *encryptedDataSize = sizeof(dataPtr->writeOp.encryptedData);

    le_result_t result = LE_OK;

    if(CheckSize(bufferSize, dataPtr->ownerAppName, dataPtr->path) != LE_OK)
    {
        taf_rfs_Close(dataPtr->writeOp.outputFd);

        dataPtr->isInWritingProcess = false;

        dataPtr->writeOp.clientSessionRef = nullptr;

        dataPtr->writeOp.outputFd = -1;

        DeleteTempFile(dataPtr->path);

        LE_ERROR("Storage free size is not enough for the data size %" PRIuS, bufferSize);

        result = LE_NO_MEMORY;
        goto error_exit;
    }

    result = taf_ks_CryptoSessionProcess(*sessionRefPtr,
                                            bufferPtr,
                                            bufferSize,
                                            encryptedData,
                                            encryptedDataSize);

    if(LE_OK != result)
    {
        LE_ERROR("Process crypto session error");
        result = LE_FAULT;
        goto error_exit;
    }

    if (*encryptedDataSize > 0)
    {
        if(taf_rfs_Write(dataPtr->writeOp.outputFd,
                                    encryptedData,
                                    *encryptedDataSize) != (ssize_t)*encryptedDataSize)
        {
            LE_ERROR("Failed to write encrypted data to file '%s'.", dataPtr->path);

            result = LE_FAULT;
            goto error_exit;
        }
        LE_INFO("Write encrypted data size = %" PRIuS , *encryptedDataSize);
    }

    LE_INFO("WriteDataChunk operation done");

    return LE_OK;

error_exit:
    // Abort the crypto session when abnormal exit
    taf_ks_CryptoSessionAbort(*sessionRefPtr);
    if (dataPtr->writeOp.outputFd >= 0)
    {
        taf_rfs_Close(dataPtr->writeOp.outputFd);
        dataPtr->writeOp.outputFd = -1;
    }

    DeleteTempFile(dataPtr->path);
    dataPtr->isInWritingProcess = false;
    dataPtr->writeOp.sessionRef = nullptr;
    dataPtr->writeOp.clientSessionRef = nullptr;

    return result;
}

le_result_t tafMngdStorageSvc::WriteDataEnd
(
    taf_mngdStorSecData_DataRef_t dataRef
)
{
    tafMngdStorage_ClientData_t* clientDataPtr =
        (tafMngdStorage_ClientData_t*)le_ref_Lookup(ClientDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(clientDataPtr == nullptr, LE_NOT_FOUND, "invalid client data ref");

    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, clientDataPtr->secDataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid secure data ref");

    TAF_ERROR_IF_RET_VAL(dataPtr->isInWritingProcess == false,
                            LE_UNAVAILABLE,
                            "data is not in writing process");

    TAF_ERROR_IF_RET_VAL(dataPtr->writeOp.clientSessionRef !=
                            taf_mngdStorSecData_GetClientSessionRef(),
                            LE_BUSY,
                            "Data is in writing by another session");

    taf_ks_CryptoSessionRef_t* sessionRefPtr = &(dataPtr->writeOp.sessionRef);
    uint8_t* encryptedData = dataPtr->writeOp.encryptedData;
    size_t* encryptedDataSize = &(dataPtr->writeOp.encryptedSize);

    *encryptedDataSize = sizeof(dataPtr->writeOp.encryptedData);

    le_result_t result = taf_ks_CryptoSessionEnd(*sessionRefPtr,
                                                    nullptr, 0,
                                                    encryptedData,
                                                    encryptedDataSize);

    if(LE_OK != result)
    {
        LE_ERROR("Failed to end crypto session");
        result = LE_FAULT;
        goto error_exit;
    }

    if (*encryptedDataSize > 0)
    {
        if(taf_rfs_Write(dataPtr->writeOp.outputFd,
                                    encryptedData,
                                    *encryptedDataSize) != (ssize_t)*encryptedDataSize)
        {
            LE_ERROR("Failed to write encrypted data to file '%s'", dataPtr->path);
            taf_rfs_Close(dataPtr->writeOp.outputFd);
            dataPtr->writeOp.outputFd = -1;
            result = LE_FAULT;
            goto error_exit;
        }
        LE_INFO("Write encrypted data size = %" PRIuS, *encryptedDataSize);
    }

    // Sync to disk before closing the file
    if (fsync(dataPtr->writeOp.outputFd) != 0)
    {
        LE_WARN("Failed to fsync file '%s'", dataPtr->path);
    }
    taf_rfs_Close(dataPtr->writeOp.outputFd);

    dataPtr->isInWritingProcess = false;
    dataPtr->writeOp.outputFd = -1;
    dataPtr->writeOp.sessionRef = nullptr;
    dataPtr->writeOp.clientSessionRef = nullptr;

    if(RenameTempFile(dataPtr->path) != 0)
    {
        DeleteTempFile(dataPtr->path);

        LE_ERROR("Failed to rename temp file");

        return LE_FAULT;
    }

    // Set up data event information
    tafMngdStorage_DataChangeEvent_t dataEvent;
    memset(&dataEvent, 0, sizeof(tafMngdStorage_DataChangeEvent_t));

    dataEvent.secDataRef = clientDataPtr->secDataRef;
    dataEvent.state = TAF_MNGDSTORSECDATA_DATA_UPDATED;

    // Report event for DATA_UPDATED
    le_event_Report(DataChangeEventId, &dataEvent, sizeof(tafMngdStorage_DataChangeEvent_t));

    LE_INFO("WriteDataEnd operation done");

    return LE_OK;

error_exit:

    if (dataPtr->writeOp.outputFd >= 0)
    {
        taf_rfs_Close(dataPtr->writeOp.outputFd);
        dataPtr->writeOp.outputFd = -1;
    }

    DeleteTempFile(dataPtr->path);
    dataPtr->isInWritingProcess = false;
    dataPtr->writeOp.sessionRef = nullptr;

    return result;
}

le_result_t tafMngdStorageSvc::ReadDataFirstChunk
(
    taf_mngdStorSecData_DataRef_t dataRef,
    uint8_t *bufferPtr,
    size_t *readSize
)
{
    tafMngdStorage_ClientData_t* clientDataPtr =
        (tafMngdStorage_ClientData_t*)le_ref_Lookup(ClientDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(clientDataPtr == nullptr, LE_NOT_FOUND, "invalid client data ref");

    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, clientDataPtr->secDataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid secure data ref");

    size_t buffSize = *readSize;
    *readSize = 0;

    TAF_ERROR_IF_RET_VAL(dataPtr->isInWritingProcess == true,
                            LE_BUSY,
                            "data is in writing process");

    TAF_ERROR_IF_RET_VAL(dataPtr->isInReadingProcess == true,
                            LE_BUSY,
                            "data is in reading process");

    TAF_ERROR_IF_RET_VAL(*readSize > TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE,
                            LE_OUT_OF_RANGE,
                            "Read size %" PRIuS " is larger than the limitation %d",
                            *readSize, TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE);

    memset((void*)(&(dataPtr->readOp)), 0, sizeof(ReadOp_t));

    if(clientDataPtr->sharedClient == true)
    {
        char myAppName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };

        // Get appName from clientSession.
        if (LE_OK != GetAppNameBySessionRef(taf_mngdStorSecData_GetClientSessionRef(),
                                            myAppName, sizeof(myAppName)))
        {
            LE_ERROR("Failed to get client appName.");
            return LE_FAULT;
        }

        taf_mngdStorSecData_DataUsage_t usage = GetSharedAppUsage(dataRef, myAppName);

        if(usage != TAF_MNGDSTORSECDATA_USAGE_READ_WRITE &&
            usage != TAF_MNGDSTORSECDATA_USAGE_READ)
        {
            LE_ERROR("Doesn't have read access");
            return LE_NOT_PERMITTED;
        }
    }

    dataPtr->readOp.clientSessionRef = taf_mngdStorSecData_GetClientSessionRef();

    uint8_t nonce[EVP_MAX_MD_SIZE] = {0};

    uint8_t nonceData[TAF_MNGDSTORSECDATA_MAX_DATA_LABEL_BYTES] = {0};

    char ns[TAF_MNGDSTORSECDATA_MAX_DATA_LABEL_BYTES] = {0};

    TAF_ERROR_IF_RET_VAL(GetClientNamespace(ns, sizeof(ns)) != LE_OK,
                            LE_BAD_PARAMETER,
                            "cannot get data namespace");

    memscpy(nonceData, sizeof(nonceData), dataPtr->ownerAppName, sizeof(dataPtr->ownerAppName));

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

    char keyId[TAF_KS_MAX_KEY_ID_SIZE] = {0};
    TAF_ERROR_IF_RET_VAL(GetDataKeyId(clientDataPtr->secDataRef,
                                        keyId,
                                        TAF_KS_MAX_KEY_ID_SIZE) != LE_OK,
                            LE_FAULT,
                            "cannot get data keyId");

    taf_ks_KeyRef_t* keyRefPtr = &(dataPtr->keyRef);
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
    uint8_t tmpBuf[TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE] = {0};
    size_t readFileSize = TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE;

    dataPtr->readOp.outputFd = -1;

    dataPtr->readOp.outputFd = taf_rfs_Open(dataPtr->path, O_RDONLY, 0);

    size_t totalDecryptedSize = 0;

    if (dataPtr->readOp.outputFd < 0)
    {
        LE_ERROR("Failed to open data file '%s'.", dataPtr->path);

        result = LE_FAULT;
        goto error_exit;
    }

    // Check the input data file size.
    fstat(dataPtr->readOp.outputFd, &fileStat);
    dataPtr->readOp.fileSize = fileStat.st_size;
    if (dataPtr->readOp.fileSize == 0)
    {
        LE_INFO("data is empty");

        *readSize = 0;

        result = LE_OK;
        goto error_exit;
    }

    dataPtr->isInReadingProcess = true;

    if(dataPtr->readOp.fileSize <= TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE)
    {
        ssize_t bytesRead = taf_rfs_Read(dataPtr->readOp.outputFd, tmpBuf, &readFileSize);

        if (bytesRead != (ssize_t)dataPtr->readOp.fileSize)
        {
            LE_ERROR("Error (%s) reading data file '%s'.", LE_ERRNO_TXT(errno), dataPtr->path);
            taf_rfs_Close(dataPtr->readOp.outputFd);
            dataPtr->isInReadingProcess = false;
            dataPtr->readOp.outputFd = -1;
            result = LE_FAULT;
            goto error_exit;
        }

        // Decrypt the data
        *decryptedDataSize = sizeof(dataPtr->readOp.decryptedData);
        result = taf_ks_CryptoSessionProcess(*sessionRefPtr,
                                             tmpBuf,
                                             bytesRead,
                                             decryptedData,
                                             decryptedDataSize);

        totalDecryptedSize = *decryptedDataSize;

        if(LE_OK != result)
        {
            LE_ERROR("Failed to process crypto session");
            result = LE_FAULT;
            goto error_exit;
        }

        if (*decryptedDataSize > 0)
        {
            (*readSize) = memscpy(bufferPtr,
                                    buffSize,
                                    decryptedData,
                                    *decryptedDataSize);

            LE_DEBUG("Copy decrypted data size = %" PRIuS, *readSize);
        }

        *decryptedDataSize = sizeof(dataPtr->readOp.decryptedData);
        result = taf_ks_CryptoSessionEnd(*sessionRefPtr,
                                         nullptr, 0,
                                         decryptedData,
                                         decryptedDataSize);

        totalDecryptedSize += *decryptedDataSize;

        if(LE_OK != result)
        {
            LE_ERROR("Failed to end crypto session");

            taf_rfs_Close(dataPtr->readOp.outputFd);
            dataPtr->isInReadingProcess = false;
            dataPtr->readOp.outputFd = -1;
            result = LE_FAULT;
            goto error_exit;
        }

        if (*decryptedDataSize > 0)
        {
            *readSize +=
                memscpy(bufferPtr + *readSize,
                        buffSize - *readSize,
                        decryptedData,
                        *decryptedDataSize);

            LE_DEBUG("Copy decrypted data size = %" PRIuS, *readSize);
        }

        taf_rfs_Close(dataPtr->readOp.outputFd);
        dataPtr->readOp.outputFd = -1;
        dataPtr->isInReadingProcess = false;
        dataPtr->readOp.sessionRef = nullptr;

        LE_INFO("Total output data size = %" PRIuS, *readSize);

        if(*readSize < totalDecryptedSize)
        {
            LE_ERROR("Total decrypted data size = %" PRIuS, totalDecryptedSize);

            return LE_OK;
        }
    }
    else
    {
        dataPtr->readOp.readIterator = 0;

        size_t currentSize = TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE;

        ssize_t bytesRead = taf_rfs_Read(dataPtr->readOp.outputFd, tmpBuf, &currentSize);
        if (bytesRead != (ssize_t)currentSize)
        {
            LE_ERROR("Error (%s) reading data file '%s'.",
                        LE_ERRNO_TXT(errno), dataPtr->path);

            result = LE_FAULT;
            goto error_exit;
        }

        // Encrypt the data chunk.
        *decryptedDataSize = sizeof(dataPtr->readOp.decryptedData);
        result = taf_ks_CryptoSessionProcess(*sessionRefPtr,
                                                tmpBuf,
                                                bytesRead,
                                                decryptedData,
                                                decryptedDataSize);

        totalDecryptedSize = *decryptedDataSize;

        if(LE_OK != result)
        {
            LE_ERROR("Failed to process crypto session");
            result = LE_FAULT;
            goto error_exit;
        }

        if (*decryptedDataSize > 0)
        {
            *readSize =
                memscpy(bufferPtr, buffSize, decryptedData, *decryptedDataSize);

            LE_DEBUG("Copy decrypted data size = %" PRIuS, *readSize);

            if(*readSize < totalDecryptedSize)
            {
                LE_ERROR("Total decrypted data size = %" PRIuS, totalDecryptedSize);

                result = LE_OVERFLOW;
                goto error_exit;
            }
            else
            {
                dataPtr->readOp.ReadDecryptedDataSize = *readSize;
            }
        }

        dataPtr->readOp.readIterator++;
    }

    return LE_OK;

error_exit:
    // Abort the crypto session when abnormal exit
    taf_ks_CryptoSessionAbort(*sessionRefPtr);

    if (dataPtr->readOp.outputFd >= 0)
    {
        taf_rfs_Close(dataPtr->readOp.outputFd);
        dataPtr->readOp.outputFd = -1;
    }
    dataPtr->isInReadingProcess = false;
    dataPtr->readOp.sessionRef = nullptr;

    return result;
}


le_result_t tafMngdStorageSvc::ReadDataNextChunk
(
    taf_mngdStorSecData_DataRef_t dataRef,
    uint8_t *bufferPtr,
    size_t *readSize
)
{
    tafMngdStorage_ClientData_t* clientDataPtr =
        (tafMngdStorage_ClientData_t*)le_ref_Lookup(ClientDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(clientDataPtr == nullptr, LE_NOT_FOUND, "invalid client data ref");

    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, clientDataPtr->secDataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid secure data ref");

    size_t buffSize = *readSize;
    *readSize = 0;

    TAF_ERROR_IF_RET_VAL(dataPtr->isInWritingProcess == true,
                            LE_BUSY,
                            "data is in writing process");

    TAF_ERROR_IF_RET_VAL(dataPtr->isInReadingProcess == false,
                            LE_UNAVAILABLE,
                            "data is not in reading process");

    TAF_ERROR_IF_RET_VAL(dataPtr->readOp.clientSessionRef !=
                            taf_mngdStorSecData_GetClientSessionRef(),
                            LE_BUSY,
                            "Data is in reading by another session");

    TAF_ERROR_IF_RET_VAL(*readSize > TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE,
                            LE_OUT_OF_RANGE,
                            "Read size %" PRIuS " is larger than the limitation %d",
                            *readSize, TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE);

    uint8_t tmpBuf[TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE] = {0};

    taf_ks_CryptoSessionRef_t* sessionRefPtr = &(dataPtr->readOp.sessionRef);

    uint8_t* decryptedData = dataPtr->readOp.decryptedData;
    size_t* decryptedDataSize = &(dataPtr->readOp.decryptedSize);

    le_result_t result = LE_OK;

    if((dataPtr->readOp.readIterator * TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE) <
        dataPtr->readOp.fileSize)
    {
        size_t currentSize =
            dataPtr->readOp.fileSize -
            (dataPtr->readOp.readIterator * TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE);

        if (currentSize > TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE)
        {
            currentSize = TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE;
        }

        ssize_t bytesRead = taf_rfs_Read(dataPtr->readOp.outputFd, tmpBuf, &currentSize);
        if (bytesRead != (ssize_t)currentSize)
        {
            LE_ERROR("Error (%s) reading data file '%s'.",
                        LE_ERRNO_TXT(errno), dataPtr->path);

            result = LE_FAULT;
            goto error_exit;
        }

        // Encrypt the data chunk.
        *decryptedDataSize = sizeof(dataPtr->readOp.decryptedData);
        result = taf_ks_CryptoSessionProcess(*sessionRefPtr,
                                                tmpBuf,
                                                bytesRead,
                                                decryptedData,
                                                decryptedDataSize);

        if(LE_OK != result)
        {
            LE_ERROR("Failed to process crypto session");
            result = LE_FAULT;
            goto error_exit;
        }

        if (*decryptedDataSize > 0)
        {
            *readSize =
                memscpy(bufferPtr, buffSize, decryptedData, *decryptedDataSize);

            LE_DEBUG("Copy decrypted data size = %" PRIuS, *readSize);
        }

        if(*readSize < *decryptedDataSize)
        {
            LE_ERROR("Total decrypted data size = %" PRIuS, *decryptedDataSize);

            dataPtr->isInReadingProcess = false;

            taf_rfs_Close(dataPtr->readOp.outputFd);
            dataPtr->readOp.outputFd = -1;
            result = LE_OVERFLOW;
            goto error_exit;
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

        if(LE_OK != result)
        {
            LE_ERROR("Failed to end crypto session");
            result = LE_FAULT;
            goto error_exit;
        }

        if (*decryptedDataSize > 0)
        {
            *readSize =
                memscpy(bufferPtr, buffSize, decryptedData, *decryptedDataSize);

            LE_DEBUG("Copy decrypted data size =%" PRIuS, *readSize);
        }

        if(*readSize < *decryptedDataSize)
        {
            LE_ERROR("Total decrypted data size = %" PRIuS, *decryptedDataSize);

            dataPtr->isInReadingProcess = false;

            taf_rfs_Close(dataPtr->readOp.outputFd);

            return LE_OK;
        }
        else
        {
            dataPtr->readOp.ReadDecryptedDataSize += *readSize;
        }

        LE_INFO("Total read decrypted data size = %" PRIuS,
                    dataPtr->readOp.ReadDecryptedDataSize);

        dataPtr->isInReadingProcess = false;

        dataPtr->readOp.clientSessionRef = nullptr;

        taf_rfs_Close(dataPtr->readOp.outputFd);

        dataPtr->readOp.outputFd = -1;
        dataPtr->readOp.sessionRef = nullptr;
    }

    return LE_OK;

error_exit:
    // Abort the crypto session when abnormal exit
    taf_ks_CryptoSessionAbort(*sessionRefPtr);

    if (dataPtr->readOp.outputFd >= 0)
    {
        taf_rfs_Close(dataPtr->readOp.outputFd);
        dataPtr->readOp.outputFd = -1;
    }
    dataPtr->isInReadingProcess = false;
    dataPtr->readOp.sessionRef = nullptr;

    return result;
}

le_result_t tafMngdStorageSvc::GetDataSize
(
    taf_mngdStorSecData_DataRef_t dataRef,
    uint32_t *size
)
{
    tafMngdStorage_ClientData_t* clientDataPtr =
        (tafMngdStorage_ClientData_t*)le_ref_Lookup(ClientDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(clientDataPtr == nullptr, LE_NOT_FOUND, "invalid client data ref");

    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, clientDataPtr->secDataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid secure data ref");

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
    taf_mngdStorSecData_DataRef_t dataRef
)
{
    tafMngdStorage_ClientData_t* clientDataPtr =
        (tafMngdStorage_ClientData_t*)le_ref_Lookup(ClientDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(clientDataPtr == nullptr, LE_NOT_FOUND, "invalid client data ref");

    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, clientDataPtr->secDataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid secure data ref");

    TAF_ERROR_IF_RET_VAL(dataPtr->isInWritingProcess == true,
                            LE_BUSY,
                            "data is in writing process");

    TAF_ERROR_IF_RET_VAL(dataPtr->isInReadingProcess == true,
                            LE_BUSY,
                            "data is in reading process");

    TAF_ERROR_IF_RET_VAL(IsFileExisting(dataPtr->path) == false,
                            LE_BAD_PARAMETER,
                            "cannot find data");

    TAF_ERROR_IF_RET_VAL(clientDataPtr->sharedClient == true,
                            LE_NOT_PERMITTED,
                            "shared app is not permitted to delete file");

    // Notify registered shared app before cleaning data
    for(uint i = 0; i < dataPtr->sharedAppList.appCount; i++)
    {
        const char* sharedAppNamePtr = dataPtr->sharedAppList.appInfo[i].appName;
        NotifyClientsForDataChange(dataPtr->dataName,
                                    dataPtr->ownerAppName,
                                    sharedAppNamePtr,
                                    TAF_MNGDSTORSECDATA_DATA_DELETED);
    }

    ClearData(clientDataPtr->secDataRef);

    return LE_OK;
}

void tafMngdStorageSvc::ClearData
(
    tafMngdStorage_SecDataRef_t secDataRef
)
{
    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, secDataRef);

    TAF_ERROR_IF_RET_NIL(dataPtr == nullptr, "invalid secure data ref");

    DeleteTempFile(dataPtr->path);

    taf_ks_DeleteKey(dataPtr->keyRef);
    taf_rfs_Delete(dataPtr->path);

    le_ref_DeleteRef(SecDataRefMap, secDataRef);
    le_mem_Release(dataPtr);
}

le_result_t tafMngdStorageSvc::CheckSize
(
    uint32_t writeSize,
    const char *appNamePtr,
    const char *fileNamePtr
)
{
    // get file size
    size_t fileSize = GetFileSize(fileNamePtr);

    // Consider uint32 overflow case
    if(writeSize > UINT32_MAX - (GetStorageUsedSize(appNamePtr) - fileSize))
    {
        LE_ERROR("Storage size not enough");
        return LE_NO_MEMORY;
    }

    // Check if free storage size is enough for the requested write operation
    if((writeSize + (GetStorageUsedSize(appNamePtr) - fileSize)) > GetStorageMaxSize())
    {
        LE_ERROR("Storage size not enough");
        return LE_NO_MEMORY;
    }

    return LE_OK;
}

le_result_t tafMngdStorageSvc::ShareData
(
    taf_mngdStorSecData_DataRef_t dataRef,
    const char* appName,
    taf_mngdStorSecData_DataUsage_t usage
)
{
    tafMngdStorage_ClientData_t* clientDataPtr =
        (tafMngdStorage_ClientData_t*)le_ref_Lookup(ClientDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(clientDataPtr == nullptr, LE_NOT_FOUND, "invalid client data ref");

    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, clientDataPtr->secDataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid secure data ref");

    TAF_ERROR_IF_RET_VAL(appName == nullptr, LE_BAD_PARAMETER, "invalid appName");

    TAF_ERROR_IF_RET_VAL(clientDataPtr->sharedClient == true,
                            LE_NOT_PERMITTED, "calling client is not the data owner");

    TAF_ERROR_IF_RET_VAL(dataPtr->sharedAppList.appCount >= TAF_MNGDSTORSECDATA_MAX_SHARED_APP_NUM,
        LE_OUT_OF_RANGE, "the number of shared apps has reached the limit.");

    char myAppName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };

    // Get appName from clientSession.
    if (LE_OK != GetAppNameBySessionRef(taf_mngdStorSecData_GetClientSessionRef(),
                                        myAppName, sizeof(myAppName)))
    {
        LE_ERROR("Failed to get client appName.");
        return LE_FAULT;
    }

    // Check if the key owner is the calling application.
    if (strcmp(myAppName, appName) == 0)
    {
        LE_ERROR("The key owner app is the calling app('%s').", appName);
        return LE_FAULT;
    }

    TAF_ERROR_IF_RET_VAL(IsInSharedAppList(dataRef, appName) == true,
                            LE_DUPLICATE,
                            "app '%s' is already shared app", appName);

    taf_ks_KeyUsage_t keyCap;

    switch (usage)
    {
        case TAF_MNGDSTORSECDATA_USAGE_READ_WRITE:
            keyCap = TAF_KS_AES_ENCRYPT_DECRYPT;
            break;

        case TAF_MNGDSTORSECDATA_USAGE_READ:
            keyCap = TAF_KS_AES_DECRYPT_ONLY;
            break;

        case TAF_MNGDSTORSECDATA_USAGE_WRITE:
            keyCap = TAF_KS_AES_ENCRYPT_ONLY;
            break;

        default:
            LE_ERROR("invalid data usage");
            return LE_BAD_PARAMETER;
    }

    le_result_t res = taf_ks_ShareKey(dataPtr->keyRef, appName, keyCap, 0);

    if(res == LE_OK)
    {
        RefreshSharedAppInfo(clientDataPtr->secDataRef);

        // Set up data event information
        tafMngdStorage_DataChangeEvent_t dataEvent;
        memset(&dataEvent, 0, sizeof(tafMngdStorage_DataChangeEvent_t));

        dataEvent.secDataRef = clientDataPtr->secDataRef;
        dataEvent.state = TAF_MNGDSTORSECDATA_DATA_SHARING_ENABLED;
        snprintf(dataEvent.sharedAppName, sizeof(dataEvent.sharedAppName),"%s", appName);

        // Report event for DATA_UPDATED
        le_event_Report(DataChangeEventId, &dataEvent, sizeof(tafMngdStorage_DataChangeEvent_t));
    }

    return res;
}

le_result_t tafMngdStorageSvc::CancelDataSharing
(
    taf_mngdStorSecData_DataRef_t dataRef,
    const char* appName
)
{
    tafMngdStorage_ClientData_t* clientDataPtr =
        (tafMngdStorage_ClientData_t*)le_ref_Lookup(ClientDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(clientDataPtr == nullptr, LE_NOT_FOUND, "invalid client data ref");

    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, clientDataPtr->secDataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid secure data ref");

    TAF_ERROR_IF_RET_VAL(appName == nullptr, LE_BAD_PARAMETER, "invalid appName");

    TAF_ERROR_IF_RET_VAL(clientDataPtr->sharedClient == true,
                            LE_NOT_PERMITTED, "calling client is not the data owner");

    TAF_ERROR_IF_RET_VAL(IsInSharedAppList(dataRef, appName) == false,
                            LE_BAD_PARAMETER,
                            "app '%s' is not shared app", appName);

    le_result_t res = taf_ks_CancelKeySharing(dataPtr->keyRef, appName);

    if(res == LE_OK)
    {
        RefreshSharedAppInfo(clientDataPtr->secDataRef);

        // Set up data event information
        tafMngdStorage_DataChangeEvent_t dataEvent;
        memset(&dataEvent, 0, sizeof(tafMngdStorage_DataChangeEvent_t));

        dataEvent.secDataRef = clientDataPtr->secDataRef;
        dataEvent.state = TAF_MNGDSTORSECDATA_DATA_SHARING_DISABLED;
        snprintf(dataEvent.sharedAppName, sizeof(dataEvent.sharedAppName),"%s", appName);

        // Report event for DATA_UPDATED
        le_event_Report(DataChangeEventId, &dataEvent, sizeof(tafMngdStorage_DataChangeEvent_t));
    }

    return res;
}

le_result_t tafMngdStorageSvc::RefreshSharedAppInfo
(
    tafMngdStorage_SecDataRef_t dataRef
)
{
    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid secure data ref");

    char appName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };
    le_result_t res = LE_OK;

    // Reset the sharedAppList
    dataPtr->sharedAppList.getIterIndex = 0;
    dataPtr->sharedAppList.appCount = 0;
    memset(&(dataPtr->sharedAppList), 0, sizeof(dataPtr->sharedAppList));

    taf_ks_KeyUsage_t keyCap;
    taf_ks_AppCapMask_t appCap;

    for(uint i = 0; i < TAF_MNGDSTORSECDATA_MAX_SHARED_APP_NUM; i++)
    {
        if(i == 0)
        {
            res = taf_ks_GetFirstSharedApp(dataPtr->keyRef,
                                            appName, sizeof(appName),
                                            &keyCap, &appCap);
        }
        else
        {
            res = taf_ks_GetNextSharedApp(dataPtr->keyRef,
                                            appName, sizeof(appName),
                                            &keyCap, &appCap);
        }

        if(res == LE_OK)
        {
            switch (keyCap)
            {
                case TAF_KS_AES_ENCRYPT_DECRYPT:
                    dataPtr->sharedAppList.appInfo[i].usage = TAF_MNGDSTORSECDATA_USAGE_READ_WRITE;
                    break;

                case TAF_KS_AES_DECRYPT_ONLY:
                    dataPtr->sharedAppList.appInfo[i].usage = TAF_MNGDSTORSECDATA_USAGE_READ;
                    break;

                case TAF_KS_AES_ENCRYPT_ONLY:
                    dataPtr->sharedAppList.appInfo[i].usage = TAF_MNGDSTORSECDATA_USAGE_WRITE;
                    break;

                default:
                    LE_ERROR("invalid key usage");
                    return LE_FAULT;
            }

            snprintf(dataPtr->sharedAppList.appInfo[i].appName,
                        sizeof(dataPtr->sharedAppList.appInfo[i].appName),"%s", appName);

            dataPtr->sharedAppList.appCount++;
        }
        else
        {
            // if taf_ks_GetFirstSharedApp is failed
            if(i == 0)
            {
                return res;
            }
            break;
        }
    }

    return LE_OK;
}

bool tafMngdStorageSvc::IsInSharedAppList
(
    taf_mngdStorSecData_DataRef_t dataRef,
    const char* appName
)
{
    tafMngdStorage_ClientData_t* clientDataPtr =
        (tafMngdStorage_ClientData_t*)le_ref_Lookup(ClientDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(clientDataPtr == nullptr, false, "invalid client data ref");

    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, clientDataPtr->secDataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, false, "invalid secure data ref");

    for(uint i = 0; i < dataPtr->sharedAppList.appCount; i++)
    {
        LE_DEBUG("shared app: %s", dataPtr->sharedAppList.appInfo[i].appName);
        LE_DEBUG("check app: %s", appName);

        if (strcmp(dataPtr->sharedAppList.appInfo[i].appName, appName) == 0)
        {
            return true;
        }
    }

    return false;
}

taf_mngdStorSecData_DataUsage_t tafMngdStorageSvc::GetSharedAppUsage
(
    taf_mngdStorSecData_DataRef_t dataRef,
    const char* appName
)
{
    tafMngdStorage_ClientData_t* clientDataPtr =
        (tafMngdStorage_ClientData_t*)le_ref_Lookup(ClientDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(clientDataPtr == nullptr,
                            TAF_MNGDSTORSECDATA_USAGE_UNKNOWN,
                            "invalid client data ref");

    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, clientDataPtr->secDataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr,
                            TAF_MNGDSTORSECDATA_USAGE_UNKNOWN,
                            "invalid secure data ref");

    for(uint i = 0; i < dataPtr->sharedAppList.appCount; i++)
    {
        if (strcmp(dataPtr->sharedAppList.appInfo[i].appName, appName) == 0)
        {
            return dataPtr->sharedAppList.appInfo[i].usage;
        }
    }

    return TAF_MNGDSTORSECDATA_USAGE_UNKNOWN;
}


le_result_t tafMngdStorageSvc::GetFirstSharedApp
(
    taf_mngdStorSecData_DataRef_t dataRef,
    char* appName,
    size_t appNameSize,
    taf_mngdStorSecData_DataUsage_t* usage
)
{
    tafMngdStorage_ClientData_t* clientDataPtr =
        (tafMngdStorage_ClientData_t*)le_ref_Lookup(ClientDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(clientDataPtr == nullptr, LE_NOT_FOUND, "invalid client data ref");

    TAF_ERROR_IF_RET_VAL(clientDataPtr->sharedClient == true,
        LE_NOT_PERMITTED, "calling client is not the data owner");

    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, clientDataPtr->secDataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid secure data ref");

    if(dataPtr->sharedAppList.appCount > 0)
    {
        snprintf(appName, appNameSize, "%s", dataPtr->sharedAppList.appInfo[0].appName);
        *usage = dataPtr->sharedAppList.appInfo[0].usage;

        // move interator to the next index
        dataPtr->sharedAppList.getIterIndex = 1;
    }
    else
    {
        LE_INFO("Shared app count is 0");
        return LE_NOT_FOUND;
    }

    return LE_OK;
}

le_result_t tafMngdStorageSvc::GetNextSharedApp
(
    taf_mngdStorSecData_DataRef_t dataRef,
    char* appName,
    size_t appNameSize,
    taf_mngdStorSecData_DataUsage_t* usage
)
{
    tafMngdStorage_ClientData_t* clientDataPtr =
        (tafMngdStorage_ClientData_t*)le_ref_Lookup(ClientDataRefMap, dataRef);

    TAF_ERROR_IF_RET_VAL(clientDataPtr == nullptr, LE_NOT_FOUND, "invalid client data ref");

    TAF_ERROR_IF_RET_VAL(clientDataPtr->sharedClient == true,
        LE_NOT_PERMITTED, "calling client is not the data owner");

    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, clientDataPtr->secDataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid secure data ref");

    // check if need to call GetFirstSharedApp first
    TAF_ERROR_IF_RET_VAL(dataPtr->sharedAppList.getIterIndex == 0,
                            LE_NOT_PERMITTED,
                            "need to call GetFirstSharedApp first");

    // check if the iterator reaches to the end of the app list
    if(dataPtr->sharedAppList.getIterIndex >= dataPtr->sharedAppList.appCount)
    {
        dataPtr->sharedAppList.getIterIndex = 0; // reset iterator
        LE_WARN("reset iterator for shared app list");
        return LE_NOT_FOUND;
    }

    uint8_t i = dataPtr->sharedAppList.getIterIndex;

    snprintf(appName, appNameSize, "%s", dataPtr->sharedAppList.appInfo[i].appName);
    *usage = dataPtr->sharedAppList.appInfo[i].usage;

    // move interator to the next index
    dataPtr->sharedAppList.getIterIndex++;

    return LE_OK;
}

taf_mngdStorSecData_DataStateChangeHandlerRef_t tafMngdStorageSvc::AddDataStateChangeHandler
(
    const char* dataName,
    const char* appName,
    taf_mngdStorSecData_DataStateChangeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    // Parameter check.
    if ((dataName == nullptr) || (appName == nullptr) || (handlerPtr == nullptr))
    {
        LE_ERROR("Bad parameter.");
        return nullptr;
    }

    char myAppName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };

    // Get appName from clientSession.
    if (LE_OK != GetAppNameBySessionRef(taf_mngdStorSecData_GetClientSessionRef(),
                                        myAppName, sizeof(myAppName)))
    {
        LE_ERROR("Failed to get client appName.");
        return nullptr;
    }

    // Check if keyId string follows the POSIX file name format.
    if (CheckValidPosixFileName(dataName) != LE_OK)
    {
        LE_ERROR("Invalid dataName (%s).", dataName);
        return nullptr;
    }

    // Check if the key owner is the calling application.
    if (strcmp(myAppName, appName) == 0)
    {
        LE_ERROR("The data owner is the calling app('%s').", appName);
        return nullptr;
    }

    // Create a handler object.
    tafMngdStorage_DataChangeHandler_t* myHandlerPtr =
        (tafMngdStorage_DataChangeHandler_t*)le_mem_ForceAlloc(DataChangeHandlerPool);
    memset(myHandlerPtr, 0, sizeof(tafMngdStorage_DataChangeHandler_t));

    // Save handler info.
    myHandlerPtr->clientSessionRef = taf_mngdStorSecData_GetClientSessionRef();
    myHandlerPtr->handleFunc = handlerPtr;
    myHandlerPtr->context = contextPtr;

    // Save the keyId and ownerAppName.
    snprintf(myHandlerPtr->dataName, sizeof(myHandlerPtr->dataName), "%s", dataName);
    snprintf(myHandlerPtr->ownerAppName, sizeof(myHandlerPtr->ownerAppName), "%s", appName);

    // Save sharing state and handler reference.
    myHandlerPtr->state = TAF_MNGDSTORSECDATA_DATA_SHARING_DISABLED;
    myHandlerPtr->handlerRef =
    (taf_mngdStorSecData_DataStateChangeHandlerRef_t)le_ref_CreateRef(DataChangeHandlerRefMap,
                                                                        myHandlerPtr);

    // Get the sharing state of the desired key.
    tafMngdStorage_SecDataRef_t dataRef = nullptr;

    // Find the secure data reference
    if(FindSecDataRef(dataName, appName, &dataRef) == LE_OK)
    {
        tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, dataRef);

        // Check if the data refernce is valid
        TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, nullptr, "Invalid data reference");

        // Check if the calling app is already one of the shared apps
        for(uint i = 0; i < dataPtr->sharedAppList.appCount; i++)
        {
            // Trigger the handler with DATA_SHARING_ENABLE state
            if(strcmp(dataPtr->sharedAppList.appInfo[i].appName,myAppName) == 0)
            {
                myHandlerPtr->state = TAF_MNGDSTORSECDATA_DATA_SHARING_ENABLED;
                myHandlerPtr->handleFunc(myHandlerPtr->dataName, myHandlerPtr->ownerAppName,
                                            myHandlerPtr->state, myHandlerPtr->context);

                break;
            }
        }
    }

    return myHandlerPtr->handlerRef;
}

void tafMngdStorageSvc::RemoveDataStateChangeHandler
(
    taf_mngdStorSecData_DataStateChangeHandlerRef_t handlerRef
)
{
    tafMngdStorage_DataChangeHandler_t* myHandlerPtr =
                    (tafMngdStorage_DataChangeHandler_t*)le_ref_Lookup(DataChangeHandlerRefMap,
                                                                        handlerRef);

    if (myHandlerPtr != nullptr)
    {
        le_ref_DeleteRef(DataChangeHandlerRefMap, handlerRef);
        le_mem_Release(myHandlerPtr);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Event handler for data state change.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdStorageSvc::DataChangeEventHandler
(
    void* reportPtr
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    if(reportPtr != nullptr)
    {
        tafMngdStorage_DataChangeEvent_t* dataEventPtr =
            (tafMngdStorage_DataChangeEvent_t*)reportPtr;

        // Common info
        taf_mngdStorSecData_DataState_t state = dataEventPtr->state;
        tafMngdStorage_SecDataRef_t secDataRef = dataEventPtr->secDataRef;

        tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(mss.SecDataRefMap, secDataRef);

        TAF_ERROR_IF_RET_NIL(dataPtr == nullptr, "invalid secure data ref");

        const char* dataNamePtr = dataPtr->dataName;
        const char* ownerAppNamePtr = dataPtr->ownerAppName;

        // If the state is DATA_UPDATED, notify all the registered shared apps
        // otherwise, only notify the sharing state changed app
        if(state == TAF_MNGDSTORSECDATA_DATA_UPDATED)
        {
            for(uint i = 0; i < dataPtr->sharedAppList.appCount; i++)
            {
                const char* sharedAppNamePtr = dataPtr->sharedAppList.appInfo[i].appName;
                NotifyClientsForDataChange(dataNamePtr, ownerAppNamePtr, sharedAppNamePtr, state);
            }
        }
        else
        {
            const char* sharedAppNamePtr = dataEventPtr->sharedAppName;
            NotifyClientsForDataChange(dataNamePtr, ownerAppNamePtr, sharedAppNamePtr, state);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Notify clients that a data is shared.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdStorageSvc::NotifyClientsForDataChange
(
    const char* dataNamePtr,                       ///< Data ID string
    const char* ownerAppNamePtr,                   ///< Owner app name string
    const char* sharedAppNamePtr,                  ///< Shared app name string
    taf_mngdStorSecData_DataState_t state          ///< Data change state
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    if ((dataNamePtr != nullptr) && (ownerAppNamePtr != nullptr) && (sharedAppNamePtr != nullptr))
    {
        char appName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };

        le_ref_IterRef_t iterRef = le_ref_GetIterator(mss.DataChangeHandlerRefMap);
        while (le_ref_NextNode(iterRef) == LE_OK)
        {
            tafMngdStorage_DataChangeHandler_t* handlerPtr =
                (tafMngdStorage_DataChangeHandler_t*)le_ref_GetValue(iterRef);

            // Get client app name.
            if ((handlerPtr != nullptr) && (LE_OK == GetAppNameBySessionRef(handlerPtr->clientSessionRef,
                                                appName, sizeof(appName))))
            {
                // If the data sharing notification is for this client, call the client handler.
                if ((0 == strcmp(dataNamePtr, handlerPtr->dataName)) &&
                    (0 == strcmp(ownerAppNamePtr, handlerPtr->ownerAppName)) &&
                    (0 == strcmp(sharedAppNamePtr, appName)) &&
                    ((state == TAF_MNGDSTORSECDATA_DATA_UPDATED) || (state != handlerPtr->state)))
                {
                    handlerPtr->state = state;
                    handlerPtr->handleFunc(handlerPtr->dataName, handlerPtr->ownerAppName,
                                            handlerPtr->state, handlerPtr->context);
                }
            }
        }
    }
}
