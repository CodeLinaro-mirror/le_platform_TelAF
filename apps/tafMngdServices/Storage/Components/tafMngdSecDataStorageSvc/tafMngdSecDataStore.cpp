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
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

using namespace telux::tafsvc;

// Defines secure storage size for each client
#ifndef MSS_SEC_STORE_SIZE
#define MSS_SEC_STORE_SIZE (1024 * 8)
#endif

void tafMngdStorageSvc::InitStorage
(
)
{
    struct stat sb;

    // assume SECURE_STORAGE dir exists in the system
    if(stat(SECURE_STORAGE, &sb) == -1)
    {
        LE_INFO("secure storage dir does not exist, create it.");

        if(mkdir(SECURE_STORAGE, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", SECURE_STORAGE);
            exit(-1);
        }
    }
    else if((sb.st_mode & S_IFMT) == S_IFDIR)
    {
        // assume the sub dir was created as well. do nothing
        LE_INFO("RFS storage was found!");
    }
    else
    {
        // some other file objects. delete first
        LE_ERROR("Delete the file object, then create the storage");

        // try to delete it
        unlink(SECURE_STORAGE);

        // Create the directory
        if(mkdir(SECURE_STORAGE, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", SECURE_STORAGE);
            exit(-1);
        }
    }

    // Create memory pools
    SecDataPool = le_mem_CreatePool("SecDataPool", sizeof(tafMngdStorage_SecData_t));

    // Create reference maps
    SecDataRefMap = le_ref_CreateMap("SecDataRefMap", SECURE_MAX_NUM_OF_DATA);

    // Create memory pools
    ClientDataPool = le_mem_CreatePool("ClientDataPool", sizeof(tafMngdStorage_ClientData_t));

    // Create reference maps
    ClientDataRefMap = le_ref_CreateMap("ClientDataRefMap", SECURE_MAX_NUM_OF_CLIENT_DATA);

    // Create memory pools
    DataChangeHandlerPool =
        le_mem_CreatePool("DataChangeHandlerPool", sizeof(tafMngdStorage_DataChangeHandler_t));

    // Create reference maps
    DataChangeHandlerRefMap =
        le_ref_CreateMap("DataChangeHandlerRefMap", SECURE_MAX_NUM_OF_DATA_HANDLER);

    // Release storage reference for disconnected session
    le_msg_AddServiceCloseHandler(taf_mngdStorSecData_GetServiceRef(),
                                    ReleaseDataRef,
                                    nullptr);

    // Create key event
    DataChangeEventId =
        le_event_CreateId("DataChangeEventId", sizeof(tafMngdStorage_DataChangeEvent_t));

    le_event_AddHandler("DataChangeEventhandler", DataChangeEventId, DataChangeEventHandler);

    LoadAllSharedAppData();
}

le_result_t tafMngdStorageSvc::GetStoragePath
(
    char* bufferPtr,
    size_t bufferSize
)
{
    char nsStr[LIMIT_MAX_PATH_BYTES] = {0};

    TAF_ERROR_IF_RET_VAL(
        GetClientNamespace(nsStr,
                            sizeof(nsStr)) != LE_OK,
                            LE_FAULT, "Cannot get namespace");

    snprintf(bufferPtr, bufferSize, "%s%s", SECURE_STORAGE, nsStr);

    LE_INFO("storage path is: %s", bufferPtr);

    return LE_OK;
}

le_result_t tafMngdStorageSvc::CreateStorageDir
(
)
{
    char storagePath[LIMIT_MAX_PATH_BYTES] = {0};
    struct stat sb;

    TAF_ERROR_IF_RET_VAL(GetStoragePath(storagePath,
                            sizeof(storagePath)) != LE_OK,
                            LE_FAULT, "Cannot get storage path");

    if(stat(storagePath, &sb) == -1)
    {
        LE_INFO("%s does not exist, create it.", storagePath);

        if(mkdir(storagePath, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", storagePath);
            return LE_FAULT;
        }
    }
    else if((sb.st_mode & S_IFMT) == S_IFDIR)
    {
        // assume the sub dir was created as well. do nothing
        LE_INFO("storage was found!");
    }
    else
    {
        // some other file objects. delete first
        LE_ERROR("Delete the file object, then create the storage");

        // try to delete it
        unlink(storagePath);

        // Create the directory
        if(mkdir(storagePath, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", storagePath);
            return LE_FAULT;
        }
    }

    return LE_OK;
}

le_result_t tafMngdStorageSvc::GetClientNamespace
(
    char* namespaceStr,
    size_t strSize
)
{
    pid_t pid;
    char appPath[LIMIT_MAX_PATH_BYTES] = {0};

    le_msg_SessionRef_t sessionRef = taf_mngdStorSecData_GetClientSessionRef();

    if (sessionRef != nullptr)
    {
        // Get the client pid.
        if (le_msg_GetClientProcessId(sessionRef, &pid) != LE_OK)
        {
            LE_ERROR("Failed to get the pid from client session reference.");
            return LE_FAULT;
        }
    }
    else
    {
        // Get self pid.
        pid = getpid();
    }

    // Retrieve the app name from the pid.
    if (le_appInfo_GetName(pid, appPath, sizeof(appPath)) == LE_OK)
    {
        snprintf(namespaceStr, strSize, "%s", appPath);
        LE_INFO("Get namespaceStr: %s.", namespaceStr);
    }
    else
    {
        return LE_FAULT;
    }

    return LE_OK;
}

uint32_t tafMngdStorageSvc::GetStorageFreeSpace
(
)
{
    if(GetStorageUsedSize() >= GetStorageMaxSize())
    {
        return 0;
    }
    return (GetStorageMaxSize() - GetStorageUsedSize());
}

le_result_t tafMngdStorageSvc::CheckStorageSizeLimit
(
    size_t inputSize
)
{
    if(GetStorageFreeSpace() >= inputSize)
    {
        return LE_OK;
    }

    return LE_NO_MEMORY;
}

uint32_t tafMngdStorageSvc::GetStorageMaxSize
(
)
{
    uint32_t value = MSS_SEC_STORE_SIZE;

    return value;
}

uint32_t tafMngdStorageSvc::GetStorageUsedSize
(
)
{
    char storagePath[LIMIT_MAX_PATH_BYTES] = {0};

    TAF_ERROR_IF_RET_VAL(
        GetStoragePath(storagePath,
                                sizeof(storagePath)) != LE_OK,
                                0, "Cannot get storage path");

    return GetFilesSizeInDirectory(storagePath);
}

void tafMngdStorageSvc::LoadAllSharedAppData
(
    void
)
{
    struct dirent *namespace_entry;
    DIR *namespace_dir = opendir(SECURE_STORAGE);

    if (namespace_dir == nullptr)
    {
        LE_ERROR("Unable to open base directory");
        return;
    }

    while ((namespace_entry = readdir(namespace_dir)) != nullptr)
    {
        if (namespace_entry->d_type == DT_DIR) {
            // Skip "." and ".." directories
            if (strcmp(namespace_entry->d_name, ".") == 0 ||
                strcmp(namespace_entry->d_name, "..") == 0)
            {
                continue;
            }

            char namespace_path[1024];
            snprintf(namespace_path, sizeof(namespace_path), "%s/%s",
                        SECURE_STORAGE, namespace_entry->d_name);

            struct dirent *data_entry;
            DIR *data_dir = opendir(namespace_path);

            if (data_dir == nullptr)
            {
                LE_ERROR("Unable to open namespace directory");
                continue;
            }

            while ((data_entry = readdir(data_dir)) != nullptr)
            {
                if (data_entry->d_type == DT_REG)
                {
                    LoadSecDataRefForDataSharedKey(namespace_entry->d_name, data_entry->d_name);
                }
            }

            closedir(data_dir);
        }
    }

    closedir(namespace_dir);
}

void tafMngdStorageSvc::LoadSecDataRefForDataSharedKey
(
    const char* namespacePtr,
    const char* dataNamePtr
)
{
    LE_INFO("CreateSecDataRefForDataSharedKey");

    char keyId[TAF_KS_MAX_KEY_ID_SIZE] = {0};
    snprintf(keyId, TAF_KS_MAX_KEY_ID_SIZE, "%s%s", namespacePtr, dataNamePtr);

    LE_INFO("Check keyId: %s", keyId);

    taf_ks_KeyRef_t keyRef;
    taf_ks_KeyUsage_t keyCap;
    taf_ks_AppCapMask_t appCap;

    TAF_ERROR_IF_RET_NIL(LE_OK != taf_ks_GetKey(keyId, &keyRef),
                            "Failed to get key with key id: %s", keyId);

    char appName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };

    le_result_t res = taf_ks_GetFirstSharedApp(keyRef, appName, sizeof(appName), &keyCap, &appCap);

    if(res == LE_OK)
    {
        tafMngdStorage_SecData_t *dataPtr = nullptr;
        tafMngdStorage_SecDataRef_t secDataRef = nullptr;

        dataPtr = (tafMngdStorage_SecData_t*)le_mem_ForceAlloc(SecDataPool);

        memset((void*)dataPtr, 0, sizeof(tafMngdStorage_SecData_t));

        secDataRef =
            (tafMngdStorage_SecDataRef_t)le_ref_CreateRef(SecDataRefMap, dataPtr);

        snprintf(dataPtr->dataName, sizeof(dataPtr->dataName), "%s", dataNamePtr);
        snprintf(dataPtr->ownerAppName, sizeof(dataPtr->ownerAppName), "%s", namespacePtr);

        dataPtr->isInWritingProcess = false;
        dataPtr->isInReadingProcess = false;

        RefreshSharedAppInfo(secDataRef);
    }
}