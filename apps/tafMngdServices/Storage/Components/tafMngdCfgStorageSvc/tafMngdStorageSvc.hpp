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
#include "tafHalLib.hpp"
#include "limit.h"
#include <vector>
#include <float.h>
#include "tafRFSLib.h"
#include "TafPiCfgStor.h"

/*
 * Macros for config storage
 */
#define CONFIG_FILE_NAME "Config.json"
#define CONFIG_FILE_NAME_BAK "Config.json.bak"
#define DEFAULT_MSS_CONFIG_NAME "tafMngdStorageSvc.json"
#define TAF_MNGD_CFG_STORAGE_SVC_PATH "tafMngdCfgStorageSvc:/configuration/"
#define MAX_OEM_CONFIG_FILES 5
#define MAX_QCM_CONFIG_FILES 1
#define MAX_NUM_OF_CONFIG_STORAGE 20
#define MAX_FILE_PATH_LEN 256
#define CFG_NODE_MAJORVERSION "MajorVersion"
#define CFG_NODE_MINORVERSION "MinorVersion"
#define CFG_NODE_PATCHVERSION "PatchVersion"
#define CFG_NODE_MAJORVERSION_FULLPATH TAF_MNGD_CFG_STORAGE_SVC_PATH CFG_NODE_MAJORVERSION
#define CFG_NODE_MINORVERSION_FULLPATH TAF_MNGD_CFG_STORAGE_SVC_PATH CFG_NODE_MINORVERSION
#define CFG_NODE_PATCHVERSION_FULLPATH TAF_MNGD_CFG_STORAGE_SVC_PATH CFG_NODE_PATCHVERSION

namespace telux
{
    namespace tafsvc
    {

typedef struct{
    // File name
    char fileName[MAX_FILE_PATH_LEN];
}
tafMngdStorage_ConfigFileData_t;

typedef struct{
    // Major version of file
    uint32_t majorVersion;

    // Minor version of file
    uint32_t minorVersion;

    // Patch version of file
    uint32_t patchVersion;
}
tafMngdStorage_ConfigVersionInfo_t;

typedef struct{
    // Client session reference
    le_msg_SessionRef_t clientSessionRef;

    // Reference of configuration file
    taf_mngdStorCfg_ConfigRef_t configRef;

}
tafMngdStorage_ConfigStorage_t;

class tafMngdStorageSvc: public ITafSvc
{
    public:
        tafMngdStorageSvc() {};
        ~tafMngdStorageSvc() {};

        void Init(void);
        static tafMngdStorageSvc &GetInstance();

        /**
         * Resources for  config storage
         */
        char configStorage[MAX_FILE_PATH_LEN];
        char configRfsStorage[MAX_FILE_PATH_LEN];
        le_ref_MapRef_t configStorageRefMap;
        le_mem_PoolRef_t configStoragePool;
        le_mem_PoolRef_t versionStoragePool;
        le_mem_PoolRef_t cfgTempStoragePool;
        cfgStor_Inf_t* cfgStorInf;
        taf_fsc_StorageRef_t cfgFscRef;
        taf_fsc_StorageRef_t cfgRfsFscRef;

        // JSON file update path
        char updatePath[MAX_FILE_PATH_LEN];

        // check for json format type
        bool isQcmFormat;

        /**
         * Resources for holding all config files data.
         */
        tafMngdStorage_ConfigFileData_t* configFileData[MAX_OEM_CONFIG_FILES];

        /**
         * Resources for holding version info for master config file.
         */
        tafMngdStorage_ConfigVersionInfo_t* versionInfo;

        // Max limit of clients
        int32_t mClientRefCount;

        /**
         * Functions for Config storage
         */

        void InitConfigStorage();

        le_result_t ParseServiceJsonConfig(char* configPath);

        // Check the extension json if not valid, then intialized service with base json
        le_result_t PreCheckExtensionJson();

        bool IsFileExisting(const char *path);

        bool IsDirectoryExisting(const char *path);

        le_result_t CreateDirectory(const char *path);

        le_result_t Update(taf_mngdStorCfg_ConfigRef_t,const char*);

        le_result_t Activate(taf_mngdStorCfg_ConfigRef_t);

        le_result_t GetConfigStoragePath(char* storagePtr, size_t storageSize);

        le_result_t GetFiles(const char *path,uint32_t maxFiles);

        le_result_t ConvertToSingleMssJson(uint32_t maxFiles);

        le_result_t AuthenticateFile(uint32_t maxFiles);

        le_result_t GetConfigFilePath(char* bufferPtr, size_t bufferSize,
            tafMngdStorage_ConfigFileData_t* configPtr);

        le_result_t ValidateJsonSchema(taf_mngdStorCfg_ConfigRef_t configStor,
            char* filePath,const char* version);

        le_result_t Cancel(taf_mngdStorCfg_ConfigRef_t);

        le_result_t Rollback(taf_mngdStorCfg_ConfigRef_t);

        taf_mngdStorCfg_ConfigRef_t GetRef();

        le_result_t ReleaseRef(taf_mngdStorCfg_ConfigRef_t);

        le_result_t Commit(taf_mngdStorCfg_ConfigRef_t);

        le_result_t ClearTree();

        le_result_t ImportTree(char* filePath);

        le_result_t GetVersion(taf_mngdStorCfg_ConfigRef_t ConfigRef,
            uint32_t *MajorVersionPtr,
            uint32_t *MinorVersionPtr,
            uint32_t *PatchVersionPtr);

        le_result_t GetType(taf_mngdStorCfg_ConfigRef_t ConfigRef,
            const char *LE_NONNULL groupName,
            const char *LE_NONNULL nodeName,
            taf_mngdStorCfg_NodeType_t *typePtr);

        le_result_t GetString(taf_mngdStorCfg_ConfigRef_t ConfigRef,
            const char *LE_NONNULL groupName,
            const char *LE_NONNULL nodeName,
            char *nodeValue,
            size_t nodeValueSize);

        le_result_t GetInt(taf_mngdStorCfg_ConfigRef_t ConfigRef,
            const char *LE_NONNULL groupName,
            const char *LE_NONNULL nodeName,
            int32_t *nodeValuePtr);

        le_result_t GetFloat(taf_mngdStorCfg_ConfigRef_t ConfigRef,
            const char *LE_NONNULL groupName,
            const char *LE_NONNULL nodeName,
            double *nodeValuePtr);

        le_result_t GetBool(taf_mngdStorCfg_ConfigRef_t ConfigRef,
            const char *LE_NONNULL groupName,
            const char *LE_NONNULL nodeName,
            int32_t *nodeValuePtr);

        le_result_t GetValue(taf_mngdStorCfg_ConfigRef_t ConfigRef,
            const char *LE_NONNULL groupName,
            const char *LE_NONNULL nodeName,
            taf_mngdStorCfg_NodeType_t *typePtr,
            char *nodeValue,
            size_t nodeValueSize
        );

        le_result_t
            CheckNodeExsist(const char *LE_NONNULL groupName,const char *LE_NONNULL nodeName);

        le_result_t LockStorage();

        le_result_t UnlockStorage();

};
}
}
