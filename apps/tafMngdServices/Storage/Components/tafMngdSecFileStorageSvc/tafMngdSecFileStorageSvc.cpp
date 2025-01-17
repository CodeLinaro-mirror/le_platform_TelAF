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

#include "tafMngdSecFileStorageSvc.hpp"

using namespace telux::tafsvc;

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
    return mss.CreateStorageRefImpl(storageNamePtr, capMask);
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
