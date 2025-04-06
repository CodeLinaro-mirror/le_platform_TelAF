/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafMngdSecFileStorageSvc.hpp"

using namespace tafsvc;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

le_result_t taf_mngdStorSecFile_CreateStorage
(
    const char* storageNamePtr,
    taf_mngdStorSecFile_ManagedCapMask_t capMask
)
{
    auto &mss = tafMngdSecFileStorageSvc::GetInstance();
    return mss.CreateStorageRefImpl(storageNamePtr, capMask, false);
}

taf_mngdStorSecFile_StorageRef_t taf_mngdStorSecFile_GetStorageRef
(
    const char* storageName
)
{
    auto &mss = tafMngdSecFileStorageSvc::GetInstance();
    return mss.GetStorageRefImpl(storageName);
}

le_result_t taf_mngdStorSecFile_UnlockStorage
(
    taf_mngdStorSecFile_StorageRef_t storageRef
)
{
    auto &mss = tafMngdSecFileStorageSvc::GetInstance();
    return mss.UnlockStorageImpl(storageRef);
}

le_result_t taf_mngdStorSecFile_LockStorage
(
    taf_mngdStorSecFile_StorageRef_t storageRef
)
{
    auto &mss = tafMngdSecFileStorageSvc::GetInstance();
    return mss.LockStorageImpl(storageRef);
}

le_result_t taf_mngdStorSecFile_ImportFile
(
    taf_mngdStorSecFile_StorageRef_t storageRef,
    const char* sourceFilePath,
    const char* targetFilePath
)
{
    auto &mss = tafMngdSecFileStorageSvc::GetInstance();
    return mss.ImportFileImpl(storageRef, sourceFilePath, targetFilePath);
}

le_result_t taf_mngdStorSecFile_ReadFile
(
    taf_mngdStorSecFile_StorageRef_t storageRef,
    const char* filePath,
    uint8_t* bufPtr,
    size_t* bufSize
)
{
    auto &mss = tafMngdSecFileStorageSvc::GetInstance();
    return mss.ReadFileImpl(storageRef, filePath, bufPtr, bufSize);
}

le_result_t taf_mngdStorSecFile_DeleteFile
(
    taf_mngdStorSecFile_StorageRef_t storageRef,
    const char* filePath
)
{
    auto &mss = tafMngdSecFileStorageSvc::GetInstance();
    return mss.DeleteFileImpl(storageRef, filePath);
}

le_result_t taf_mngdStorSecFile_GetBasePath
(
    taf_mngdStorSecFile_StorageRef_t storageRef,
    char* basePath,
    size_t pathSize
)
{
    auto &mss = tafMngdSecFileStorageSvc::GetInstance();
    return mss.GetBasePathImpl(storageRef, basePath, pathSize);
}

le_result_t taf_mngdStorSecFile_DeleteStorage
(
    taf_mngdStorSecFile_StorageRef_t storageRef
)
{
    auto &mss = tafMngdSecFileStorageSvc::GetInstance();
    return mss.DeleteStorageImpl(storageRef);
}


COMPONENT_INIT
{
    LE_INFO("tafMngdStorSecFile COMPONENT init...");

    auto &mss = tafMngdSecFileStorageSvc::GetInstance();

    mss.Init();

    LE_INFO("tafMngdStorSecFile COMPONENT init finished");
}
