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

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "limit.h"
#include <vector>
#include "tafRFSLib.h"

/*
 * Macros for secure storage
 */

#define SECFILE_MAX_NUM_OF_STORAGE       25
#define SECFILE_MAX_NUM_OF_FILE          30
#define SECFILE_MAX_NUM_OF_CLIENT        30
#define DEFAULT_MSS_CONFIG_NAME "tafMngdStorageSvc.json"

namespace telux {
namespace tafsvc {

typedef struct
{
    // Storage name
    char storageName[TAF_MNGDSTORSECFILE_MAX_STORAGE_NAME_SIZE];

    // Master app name
    char masterAppName[LIMIT_MAX_APP_NAME_LEN + 1];

    // FSC storage path
    char path[TAF_MNGDSTORSECFILE_MAX_STORAGE_NAME_SIZE];

    // RFS FSC storage path
    char rfsPath[TAF_MNGDSTORSECFILE_MAX_STORAGE_NAME_SIZE];

    taf_fsc_StorageRef_t fscStorageRef;

    taf_fsc_StorageRef_t rfs_fscStorageRef;
}
tafMngdSecFileStorage_Dir_t;

using tafMngdSecFileStorage_DirRef_t = tafMngdSecFileStorage_Dir_t*;

typedef struct
{
    // Reference to the client secure storage
    taf_mngdStorSecFile_StorageRef_t storageRef;

    // Reference to secure data item
    tafMngdSecFileStorage_DirRef_t dirRef;

    // Client session reference
    le_msg_SessionRef_t clientSessionRef;

    // Shared client
    bool masterClient;

    // Storage name
    char storageName[TAF_MNGDSTORSECFILE_MAX_STORAGE_NAME_SIZE];
}
tafMngdSecFileStorage_ClientCxt_t;

class tafMngdSecFileStorageSvc: public ITafSvc
{
    public:
        tafMngdSecFileStorageSvc() {};
        ~tafMngdSecFileStorageSvc() {};

        void Init(void);
        static tafMngdSecFileStorageSvc &GetInstance();

        /**
         * Resources for secure data
         */

        char secFileStorage[TAF_MNGDSTORSECFILE_MAX_STORAGE_NAME_SIZE];
        char secFileRfsStorage[TAF_MNGDSTORSECFILE_MAX_STORAGE_NAME_SIZE];

        le_ref_MapRef_t DirRefMap;
        le_mem_PoolRef_t DirPool;

        le_ref_MapRef_t ClientRefMap;
        le_mem_PoolRef_t ClientPool;

        /**
         * Functions for secure storages
         */

        tafMngdSecFileStorage_DirRef_t CreateDirRef(const char* basePathPtr,
                                                    const char* storageNamePtr);

        le_result_t FindDirRef(const char* storageNamePtr,
                                tafMngdSecFileStorage_DirRef_t* dirRef);

        le_result_t FindClientCxtRef(const char* storageNamePtr,
                                        taf_mngdStorSecFile_StorageRef_t* dataRefPtr);

        le_result_t CreateStorageRefImpl(const char* storageName,
                                        taf_mngdStorSecFile_ManagedCapMask_t capMask);

        taf_mngdStorSecFile_StorageRef_t GetStorageRefImpl(const char* storageName);

        le_result_t UnlockStorageImpl(taf_mngdStorSecFile_StorageRef_t storageRef);

        le_result_t LockStorageImpl(taf_mngdStorSecFile_StorageRef_t storageRef);

        le_result_t ImportFileImpl(taf_mngdStorSecFile_StorageRef_t storageRef,
                                const char* sourceFilePath,
                                const char* targetFilePath);

        le_result_t ReadFileImpl(taf_mngdStorSecFile_StorageRef_t storageRef,
                                const char* filePath,
                                uint8_t* bufPtr,
                                size_t* bufSize);

        le_result_t DeleteFileImpl(taf_mngdStorSecFile_StorageRef_t storageRef,
                                const char* filePath);

        le_result_t GetBasePathImpl(taf_mngdStorSecFile_StorageRef_t storageRef,
                                    char* basePath, size_t pathSize);

        le_result_t DeleteStorageImpl(taf_mngdStorSecFile_StorageRef_t storageRef);

        /**
         * Internal functions
         */
        le_result_t ParseServiceJsonConfig(char* configPath);

        // Check the extension json if not valid, then intialized service with base json
        le_result_t PreCheckExtensionJson();

        bool IsFileExisting(const char *path);

        le_result_t CreateDirectory(const char *path);

        static bool IsDirExisting(const char *path);

        static le_result_t CheckValidPosixFileName(const char *fileName);

        static le_result_t GetAppNameBySessionRef(le_msg_SessionRef_t clientSessionRef,
                                                                char *appNameStr,
                                                                size_t appNameSize);

        static le_result_t GetStoragePath(const char* basePathPtr,const char* storageNamePtr,
                                    char* bufferPtr, size_t bufferSize);

        static bool IsDirectoryEmpty(const char *dirname);

        static inline size_t memscpy(void *dst, size_t dst_size, const void *src, size_t src_size)
        {
            size_t  copy_size = (dst_size <= src_size) ? dst_size : src_size;
            memcpy(dst, src, copy_size);
            return copy_size;
        }

};
}
}
