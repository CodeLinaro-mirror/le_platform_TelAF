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

#define SECURE_STORAGE "/data/secStorage/"
#define SECURE_MAX_NUM_OF_STORAGE    25
#define SECURE_MAX_NUM_OF_DATA      100

/*
 * Macros for config storage
 */
#define CONFIG_STORAGE "/persist/configStorage/"
#define CONFIG_RFS "/persist/rfs/"
#define CONFIG_RFS_STORAGE CONFIG_RFS"configStorage/"
#define MSS_CONFIG_PATH "/data/ManagedServices/tafMngdStorageConfig.json"
#define DEFAULT_MSS_CONFIG_PATH "/legato/systems/current/appsWriteable/tafMngdStorageSvc/data/ManagedServices/tafMngdStorageConfig.json"
#define MAX_NUM_OF_CONFIG_STORAGE    20
#define MAX_FILE_NAME_LEN 255

namespace telux {
namespace tafsvc {

typedef struct
{
    // Key reference
    taf_ks_KeyRef_t keyRef;

    // Crypto session reference
    taf_ks_CryptoSessionRef_t sessionRef;

    uint8_t encryptedData[TAF_KS_MAX_PACKET_SIZE];

    size_t encryptedSize;

    int outputFd;
}
WriteOp_t;

typedef struct
{
    // Key reference
    taf_ks_KeyRef_t keyRef;

    // Crypto session reference
    taf_ks_CryptoSessionRef_t sessionRef;

    uint8_t decryptedData[TAF_KS_MAX_PACKET_SIZE];

    size_t decryptedSize;

    size_t ReadDecryptedDataSize;

    uint32_t fileSize;

    uint readIterator;

    int outputFd;
}
ReadOp_t;

typedef struct
{
    // Reference to the secure storage
    taf_mngdStorSec_DataRef_t dataRef;

    // Data name
    char dataLabel[TAF_MNGDSTORSEC_MAX_DATA_LABLE_BYTES];

    // Client session reference
    le_msg_SessionRef_t clientSessionRef;

    // Whether the data is in writing process
    bool isInWritingProcess;

    // Whether the data is in reading process
    bool isInReadingProcess;

    // File path
    char path[LIMIT_MAX_PATH_BYTES];

    WriteOp_t writeOp;

    ReadOp_t readOp;
}
tafMngdStorage_SecData_t;

typedef struct{
    // JSON file path
    char updatePath[LIMIT_MAX_PATH_BYTES];

    // Golden copy file path
    char goldenCopyPath[LIMIT_MAX_PATH_BYTES];

    // File name
    char fileName[MAX_FILE_NAME_LEN];

    // Owner App id
    char ownerId[TAF_MNGDSTORCFG_MAX_STR_LEN];

    // Major version of file
    int majorVersion;

    // Minor version of file
    int minorVersion;

    // Patch version of file
    int patchVersion;

    // Client session reference
    le_msg_SessionRef_t clientSessionRef;

    // Reference of configuration file
    taf_mngdStorCfg_ConfigRef_t fileRef;

    size_t dataSize;

    int outputFd;
}
tafMngdStorage_ConfigFileData_t;

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

        le_result_t GetStoragePath(char* bufferPtr, size_t bufferSize);

        le_result_t CreateStorageDir();

        le_result_t GetClientNamespace(char* namespaceStr, size_t strSize);

        uint32_t GetStorageMaxSize();

        uint32_t GetStorageUsedSize();

        uint32_t GetStorageFreeSpace();

        le_result_t CheckStorageSizeLimit(size_t inputSize);

        /**
         * Functions for secure data
         */

        taf_mngdStorSec_DataRef_t CreateData(const char* dataLable);

        taf_mngdStorSec_DataRef_t GetDataRef(const char* dataLable);

        le_result_t FindDataRef(const char* dataLabel,
                                            taf_mngdStorSec_DataRef_t* dataRef);

        le_result_t GetDataPath(const char* dataLabel, char* bufferPtr,
                                            size_t bufferSize);

        le_result_t CreateDataItem(taf_mngdStorSec_DataRef_t dataRef);

        static void ReleaseDataRef(le_msg_SessionRef_t sessionRef, void* contextPtr);

        le_result_t WriteDataStart(taf_mngdStorSec_DataRef_t dataRef);

        le_result_t WriteDataChunk(taf_mngdStorSec_DataRef_t dataRef,
                                                const uint8_t *bufferPtr,
                                                size_t bufferSize);

        le_result_t WriteDataEnd(taf_mngdStorSec_DataRef_t dataRef);

        le_result_t ReadDataFirstChunk(taf_mngdStorSec_DataRef_t dataRef,
                                                    uint8_t *bufferPtr,
                                                    size_t *readSize);

        le_result_t ReadDataNextChunk(taf_mngdStorSec_DataRef_t dataRef,
                                                    uint8_t *bufferPtr,
                                                    size_t *readSize);

        le_result_t GetDataSize(taf_mngdStorSec_DataRef_t dataRef, uint32_t *size);

        le_result_t DeleteData(taf_mngdStorSec_DataRef_t dataRef);

        le_result_t CheckSize(uint32_t writeSize);

        /**
         * Resources for secure data
         */
        le_ref_MapRef_t SecDataRefMap;
        le_mem_PoolRef_t SecDataPool;

        /**
         * Internal functions
         */
        static size_t GetFilesSizeInDirectory(const char *dirPath);

        static bool IsDirectoryEmpty(const char *path);

        static bool IsDirectoryExisting(const char *path);

        static bool IsFileExisting(const char *path);

        static le_result_t CheckValidPosixFileName(const char *fileName);

        static inline size_t memscpy(void *dst, size_t dst_size, const void *src, size_t src_size)
        {
            size_t  copy_size = (dst_size <= src_size) ? dst_size : src_size;
            memcpy(dst, src, copy_size);
            return copy_size;
        }

        /**
         * Resources for  config storage
         */
        le_ref_MapRef_t configStorageRefMap;
        le_mem_PoolRef_t configStoragePool;

        /**
         * Functions for Config storage
         */

        void InitConfigStorage();

        le_result_t ParseServiceJsonConfig();

        le_result_t UpdateFile();

        le_result_t Sync();

        le_result_t GetConfigStoragePath(char* storagePtr, size_t storageSize,
            tafMngdStorage_ConfigFileData_t* configPtr);

        le_result_t GetConfigFilePath(char* bufferPtr, size_t bufferSize,
            tafMngdStorage_ConfigFileData_t* configPtr);

};
}
}
