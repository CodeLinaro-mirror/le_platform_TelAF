/*
*
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause-Clear
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
#define SECFILE_CREATOR_NAME             "MSS_SECFILE"
#define SECFILE_TMP_FILE_NAME_EXTENSION  ".tmp"
#define DEFAULT_MSS_CONFIG_NAME "tafMngdStorageSvc.json"

namespace tafsvc {

// Storage access configuration
typedef struct
{
    char StorageName[TAF_MNGDSTORSECFILE_MAX_STORAGE_NAME_SIZE];
    std::vector<char*> ReadAccessibleApps;
    std::vector<char*> WriteAccessibleApps;
}
tafMngdSecFileStorage_StorageCfg_t;

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

    uint32_t userCount;

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

    // Storage name
    char storageName[TAF_MNGDSTORSECFILE_MAX_STORAGE_NAME_SIZE];

    // Lock state
    bool lockState;

    // Is creator
    bool IsCreator;

    // Can read the file
    bool IsReadable;

    // Can write
    bool IsWritable;
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
         * Resources for secure file
         */

        char secFileStorage[TAF_MNGDSTORSECFILE_MAX_STORAGE_NAME_SIZE];
        char secFileRfsStorage[TAF_MNGDSTORSECFILE_MAX_STORAGE_NAME_SIZE];

        le_ref_MapRef_t DirRefMap;
        le_mem_PoolRef_t DirPool;

        le_ref_MapRef_t ClientRefMap;
        le_mem_PoolRef_t ClientPool;

        std::vector<tafMngdSecFileStorage_StorageCfg_t> storageAccessCfg;

        /**
         * Functions for secure storages
         */

        void CreateServiceStorages();

        tafMngdSecFileStorage_DirRef_t CreateDirRef(const char* storageNamePtr, bool internal);

        le_result_t FindDirRef(const char* storageNamePtr,
                                tafMngdSecFileStorage_DirRef_t* dirRef);

        le_result_t FindClientCxtRef(const char* storageNamePtr,
                                        taf_mngdStorSecFile_StorageRef_t* dataRefPtr);

        le_result_t CreateStorageRefImpl(const char* storageName,
                                        taf_mngdStorSecFile_ManagedCapMask_t capMask,
                                        bool internal);

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

        static void SessionCloseHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);

        le_result_t SetStorageCreator(const char* storageNamePtr,
                                        const char* creatorAppPtr);

        le_result_t GetStorageCreator(const char* storageNamePtr,
                                        char *appNameStr,
                                        size_t appNameSize);

        le_result_t ClearStorageCreator(const char* storageNamePtr);

        le_result_t ParseServiceJsonConfig(char* configPath);

        // Check the extension json if not valid, then intialized service with base json
        le_result_t PreCheckExtensionJson();

        bool IsReadable(const char* storageName, const char* appName);

        bool IsWritable(const char* storageName, const char* appName);

        bool IsAppAccessible(const char* storageName, const char* appName);

        bool IsServiceStorage(const char* storageName);

        bool IsFileExisting(const char *path);

        le_result_t CreateDirectory(const char *path);

        size_t GetFileSize(const char *filePath);

        size_t GetAvailableSpace(const char *path);

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
