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

#define SECURE_MAX_NUM_OF_STORAGE       25
#define SECURE_MAX_NUM_OF_DATA          100
#define SECURE_MAX_NUM_OF_CLIENT_DATA   100
#define SECURE_MAX_NUM_OF_DATA_HANDLER  20
#define SECURE_DATA_TEMP_EXTENSION ".temp"
#define DEFAULT_MSS_CONFIG_NAME "tafMngdStorageSvc.json"

namespace telux {
namespace tafsvc {

typedef struct
{
    // Crypto session reference
    taf_ks_CryptoSessionRef_t sessionRef;

    uint8_t encryptedData[TAF_KS_MAX_PACKET_SIZE];

    size_t encryptedSize;

    int outputFd;

    // Client session reference
    le_msg_SessionRef_t clientSessionRef;
}
WriteOp_t;

typedef struct
{
    // Crypto session reference
    taf_ks_CryptoSessionRef_t sessionRef;

    uint8_t decryptedData[TAF_KS_MAX_PACKET_SIZE];

    size_t decryptedSize;

    size_t ReadDecryptedDataSize;

    uint32_t fileSize;

    uint readIterator;

    int outputFd;

    // Client session reference
    le_msg_SessionRef_t clientSessionRef;
}
ReadOp_t;

typedef struct
{
    taf_mngdStorSecData_DataUsage_t usage;    ///< Data usage.
    char appName[LIMIT_MAX_APP_NAME_LEN + 1]; ///< Shared app name.
}
SharedApp_t;

typedef struct
{
    SharedApp_t appInfo[TAF_MNGDSTORSECDATA_MAX_SHARED_APP_NUM]; ///< Shared app list.
    uint8_t getIterIndex;
    uint8_t appCount;
}
SharedAppList_t;

typedef struct
{
    // Data name
    char dataName[TAF_MNGDSTORSECDATA_MAX_DATA_LABEL_BYTES];

    // Owner app name
    char ownerAppName[LIMIT_MAX_APP_NAME_LEN + 1];

    // Key reference
    taf_ks_KeyRef_t keyRef;

    // Whether the data is in writing process
    bool isInWritingProcess;

    // Whether the data is in reading process
    bool isInReadingProcess;

    // File path
    char path[LIMIT_MAX_PATH_BYTES];

    WriteOp_t writeOp;

    ReadOp_t readOp;

    SharedAppList_t sharedAppList;
}
tafMngdStorage_SecData_t;

using tafMngdStorage_SecDataRef_t = tafMngdStorage_SecData_t*;

typedef struct
{
    // Reference to the client secure storage
    taf_mngdStorSecData_DataRef_t dataRef;

    // Reference to secure data item
    tafMngdStorage_SecDataRef_t secDataRef;

    // Client session reference
    le_msg_SessionRef_t clientSessionRef;

    // Shared client
    bool sharedClient;

    // Data name
    char dataLabel[TAF_MNGDSTORSECDATA_MAX_DATA_LABEL_BYTES];
}
tafMngdStorage_ClientData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data structure for data change event in service layer.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    tafMngdStorage_SecDataRef_t secDataRef;         ///< Secure data reference
    taf_mngdStorSecData_DataState_t state;          ///< Data state
    char sharedAppName[LIMIT_MAX_APP_NAME_LEN + 1]; ///< Sharing state change application
}
tafMngdStorage_DataChangeEvent_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data structure for data change handler.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
   le_msg_SessionRef_t clientSessionRef;                        ///< Client session reference
   char dataName[TAF_MNGDSTORSECDATA_MAX_DATA_LABEL_BYTES];     ///< Data name
   char ownerAppName[LIMIT_MAX_APP_NAME_LEN + 1];               ///< Key owner application
   taf_mngdStorSecData_DataState_t state;                       ///< Data change state
   taf_mngdStorSecData_DataStateChangeHandlerFunc_t handleFunc; ///< Handler function
   void* context;                                               ///< Handler context
   taf_mngdStorSecData_DataStateChangeHandlerRef_t handlerRef;  ///< Handler reference
}
tafMngdStorage_DataChangeHandler_t;

class tafMngdStorageSvc: public ITafSvc
{
    public:
        tafMngdStorageSvc() {};
        ~tafMngdStorageSvc() {};

        void Init(void);
        static tafMngdStorageSvc &GetInstance();

        /**
         * Functions for secure storages
         */
        void InitStorage();

        le_result_t ParseServiceJsonConfig();

        le_result_t GetStoragePath(char* bufferPtr, size_t bufferSize);

        le_result_t CreateStorageDir();

        le_result_t GetClientNamespace(char* namespaceStr, size_t strSize);

        uint32_t GetStorageMaxSize();

        uint32_t GetStorageUsedSize(const char* appNamePtr);

        uint32_t GetStorageFreeSpace(const char* appNamePtr);

        void LoadAllSharedAppData();

        void LoadSecDataRefForDataSharedKey(const char* namespacePtr, const char* dataNamePtr);

        /**
         * Functions for secure data
         */

        tafMngdStorage_SecDataRef_t CreateSecDataRef(const char* dataNamePtr,
                                                        const char* appNamePtr);

        le_result_t CreateData(const char* dataName);

        taf_mngdStorSecData_DataRef_t GetDataRef(const char* dataLable);

        le_result_t FindClientDataRef(const char* dataLabel,
                                        taf_mngdStorSecData_DataRef_t* dataRef);

        le_result_t FindSecDataRef(const char* dataLabel,const char* ownerAppName,
                                    tafMngdStorage_SecDataRef_t* dataRef);

        le_result_t GetDataPath(const char* storageName, const char* dataLabel,
                                char* bufferPtr, size_t bufferSize);

        int OpenTempFile(const char *filename);

        int RenameTempFile(const char *filename);

        void DeleteTempFile(const char *filename);

        le_result_t GetDataKeyId(tafMngdStorage_SecDataRef_t dataRef,
                                    char* keyId,size_t keySize);

        le_result_t CreateDataItem(tafMngdStorage_SecDataRef_t dataRef);

        static void ReleaseDataRef(le_msg_SessionRef_t sessionRef, void* contextPtr);

        le_result_t WriteDataStart(taf_mngdStorSecData_DataRef_t dataRef);

        le_result_t WriteDataChunk(taf_mngdStorSecData_DataRef_t dataRef,
                                                const uint8_t *bufferPtr,
                                                size_t bufferSize);

        le_result_t WriteDataEnd(taf_mngdStorSecData_DataRef_t dataRef);

        le_result_t ReadDataFirstChunk(taf_mngdStorSecData_DataRef_t dataRef,
                                                    uint8_t *bufferPtr,
                                                    size_t *readSize);

        le_result_t ReadDataNextChunk(taf_mngdStorSecData_DataRef_t dataRef,
                                                    uint8_t *bufferPtr,
                                                    size_t *readSize);

        le_result_t GetDataSize(taf_mngdStorSecData_DataRef_t dataRef, uint32_t *size);

        le_result_t DeleteData(taf_mngdStorSecData_DataRef_t dataRef);

        void ClearData(tafMngdStorage_SecDataRef_t dataRef);

        le_result_t CheckSize(uint32_t writeSize, const char *appNamePtr, const char *fileNamePtr);

        /**
         * Functions for secure data sharing
         */

        le_result_t ShareData(taf_mngdStorSecData_DataRef_t dataRef,
                                const char* appName,
                                taf_mngdStorSecData_DataUsage_t usage);

        le_result_t CancelDataSharing(taf_mngdStorSecData_DataRef_t dataRef,
                                        const char* appName);

        le_result_t RefreshSharedAppInfo(tafMngdStorage_SecDataRef_t dataRef);

        taf_mngdStorSecData_DataUsage_t GetSharedAppUsage(taf_mngdStorSecData_DataRef_t dataRef,
                                                            const char* appName);

        bool IsInSharedAppList(taf_mngdStorSecData_DataRef_t dataRef, const char* appName);

        le_result_t GetFirstSharedApp(taf_mngdStorSecData_DataRef_t dataRef,
                                        char* appName,
                                        size_t appNameSize,
                                        taf_mngdStorSecData_DataUsage_t* usage);

        le_result_t GetNextSharedApp(taf_mngdStorSecData_DataRef_t dataRef,
                                        char* appName,
                                        size_t appNameSize,
                                        taf_mngdStorSecData_DataUsage_t* usage);

        taf_mngdStorSecData_DataStateChangeHandlerRef_t AddDataStateChangeHandler(
                const char* dataName,
                const char* appName,
                taf_mngdStorSecData_DataStateChangeHandlerFunc_t handlerPtr,
                void* contextPtr);

        void RemoveDataStateChangeHandler(
                taf_mngdStorSecData_DataStateChangeHandlerRef_t handlerRef);

        static void DataChangeEventHandler(void* reportPtr);

        static void NotifyClientsForDataChange(const char* dataNamePtr,
                                                const char* ownerAppNamePtr,
                                                const char* sharedAppNamePtr,
                                                taf_mngdStorSecData_DataState_t state);

        /**
         * Resources for secure data
         */

        char secDataStorage [LIMIT_MAX_PATH_BYTES];
        char secDataRfsStorage [LIMIT_MAX_PATH_BYTES];

        le_ref_MapRef_t SecDataRefMap;
        le_mem_PoolRef_t SecDataPool;

        le_ref_MapRef_t ClientDataRefMap;
        le_mem_PoolRef_t ClientDataPool;

        le_ref_MapRef_t DataChangeHandlerRefMap;
        le_mem_PoolRef_t DataChangeHandlerPool;

        le_event_Id_t DataChangeEventId;

        /**
         * Internal functions
         */

        static size_t GetFileSize(const char *filePath);

        static le_result_t CreateDirectory(const char *path);

        static size_t GetFilesSizeInDirectory(const char *dirPath);

        static bool IsDirectoryEmpty(const char *path);

        static bool IsDirectoryExisting(const char *path);

        static bool IsFileExisting(const char *path);

        static le_result_t CheckValidPosixFileName(const char *fileName);

        static le_result_t GetAppNameBySessionRef(le_msg_SessionRef_t clientSessionRef,
                                                                char *appNameStr,
                                                                size_t appNameSize);

        static inline size_t memscpy(void *dst, size_t dst_size, const void *src, size_t src_size)
        {
            size_t  copy_size = (dst_size <= src_size) ? dst_size : src_size;
            memcpy(dst, src, copy_size);
            return copy_size;
        }

};
}
}
