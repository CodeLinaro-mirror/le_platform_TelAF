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
#include "tafRFSLib.h"
#include "TafPiCfgStor.h"

/*
 * Macros for config storage
 */
#define CONFIG_STORAGE "/persist/configStorage/"
#define CONFIG_RFS "/persist/rfs/"
#define CONFIG_RFS_STORAGE CONFIG_RFS"configStorage/"
#define CONFIG_FILE_NAME "Config.json"
#define CONFIG_FILE_NAME_BAK "Config.json.bak"
#define DEFAULT_MSS_CONFIG_PATH "/legato/systems/current/appsWriteable/tafMngdStorageSvc/data/ManagedServices/tafMngdStorageConfig.json"
#define MAX_CONFIG_FILES 5
#define MAX_NUM_OF_CONFIG_STORAGE 20
#define MAX_FILE_NAME_LEN 256

namespace telux {
namespace tafsvc {

typedef struct{
    // File name
    char fileName[MAX_FILE_NAME_LEN];
}
tafMngdStorage_ConfigFileData_t;

typedef struct{
    // Major version of file
    int majorVersion;

    // Minor version of file
    int minorVersion;

    // Patch version of file
    int patchVersion;
}
tafMngdStorage_ConfigVersionInfo_t;

typedef struct{
    // Client session reference
    le_msg_SessionRef_t clientSessionRef;

    // Reference of configuration file
    taf_mngdStorCfg_ConfigRef_t fileRef;

    // Config Tree Iterator
    le_cfg_IteratorRef_t iterator;

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
        le_ref_MapRef_t configStorageRefMap;
        le_mem_PoolRef_t configStoragePool;
        cfgStor_Inf_t* cfgStorInf;
        taf_fsc_StorageRef_t cfgFscRef;
        taf_fsc_StorageRef_t cfgRfsFscRef;

        // JSON file update path
        char updatePath[LIMIT_MAX_PATH_BYTES];

        /**
         * Resources for holding all config files data.
         */
        tafMngdStorage_ConfigFileData_t* configFileData[MAX_CONFIG_FILES];

        /**
         * Resources for holding version info for master config file.
         */
        tafMngdStorage_ConfigVersionInfo_t* versionInfo;

        /**
         * Functions for Config storage
         */

        void InitConfigStorage();

        le_result_t ParseServiceJsonConfig();

        bool IsFileExisting(const char *path);

        le_result_t UpdateFile();

        le_result_t Sync();

        le_result_t GetConfigStoragePath(char* storagePtr, size_t storageSize);

        le_result_t GetFiles(const char *path);

        le_result_t ConvertToSingleMssJson();

        le_result_t AuthenticateFile();

        le_result_t GetConfigFilePath(char* bufferPtr, size_t bufferSize,
            tafMngdStorage_ConfigFileData_t* configPtr);

        le_result_t ValidateJsonSchema(char* filePath,size_t fileSize);

        le_result_t Cancel();

        le_result_t Rollback();

        le_result_t Commit();

        le_result_t ClearTree();

        le_result_t ImportTree(char* filePath);

};
}
}
