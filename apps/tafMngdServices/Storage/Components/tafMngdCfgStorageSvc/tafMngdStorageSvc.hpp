/*
*
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause-Clear
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
#define TAF_MNGD_CFG_STORAGE_UPDATE_TREE_PATH "tafMngdCfgUpdateState:/UpdateState/"
#define MAX_OEM_CONFIG_FILES 5
#define MAX_QCM_CONFIG_FILES 1
#define MAX_NUM_OF_CONFIG_STORAGE 20
#define MAX_FILE_PATH_LEN 256
#define CFG_NODE_MAJORVERSION "MajorVersion"
#define CFG_NODE_MINORVERSION "MinorVersion"
#define CFG_NODE_PATCHVERSION "PatchVersion"
#define CFG_NODE_STATE "State"
#define CFG_NODE_MAJORVERSION_FULLPATH TAF_MNGD_CFG_STORAGE_SVC_PATH CFG_NODE_MAJORVERSION
#define CFG_NODE_MINORVERSION_FULLPATH TAF_MNGD_CFG_STORAGE_SVC_PATH CFG_NODE_MINORVERSION
#define CFG_NODE_PATCHVERSION_FULLPATH TAF_MNGD_CFG_STORAGE_SVC_PATH CFG_NODE_PATCHVERSION
#define DATA_LABEL_ACTIVATED_CONFIG_HASH "ActivatedConfigHash"

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

typedef enum{
    UPDATE_COMPLETED,
    UPDATE_FAILED,
    ACTIVATE_COMPLETED,
    ACTIVATE_FAILED,
    CANCEL_COMPLETED,
    CANCEL_FAILED,
    ROLLBACK_COMPLETED,
    ROLLBACK_FAILED,
    COMMIT_COMPLETED,
    COMMIT_FAILED
}
tafMngdStorage_UpdateState_t;

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

        le_result_t VerifyActivation(taf_mngdStorCfg_ConfigRef_t);

        void CalculateSHA256(const char* filePath, unsigned char* hash);

        le_result_t SetActivatedConfigHash();

        le_result_t CheckActivatedConfigHash();

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

        le_result_t GetUpdateCampaignState(tafMngdStorage_UpdateState_t*);

        le_result_t SetUpdateCampaignState(tafMngdStorage_UpdateState_t);

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
