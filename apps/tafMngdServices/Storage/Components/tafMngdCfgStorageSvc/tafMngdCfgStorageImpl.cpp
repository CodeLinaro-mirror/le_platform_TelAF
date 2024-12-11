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
#include <sys/stat.h>
#include <unordered_map>
#include <string>
#include <dirent.h>
#include <limits.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <fstream>

using namespace telux::tafsvc;
namespace pt = boost::property_tree;

/**
 * Get MSS instance
 */
tafMngdStorageSvc &tafMngdStorageSvc::GetInstance()
{
   static tafMngdStorageSvc instance;
   return instance;
}

/**
 * tafMngdStorageSvc initialization
 */
void tafMngdStorageSvc::Init(void)
{
    mClientRefCount=0;
    InitConfigStorage();
    taf_rfs_Init(true, nullptr);
    // Set up RFS backup storage to the specified path
    if(taf_rfs_SetBackupStorage(configRfsStorage) != LE_OK)
    {
        LE_ERROR("Failed to set rfs backup storage %s", configRfsStorage);
    }
}

bool tafMngdStorageSvc::IsDirectoryExisting(const char *path)
{
    struct stat sb;
    //will check if path exists and is directory
    if(stat(path,&sb) == 0 && S_ISDIR(sb.st_mode)){
        return true;
    }
    return false;
}

le_result_t tafMngdStorageSvc::CreateDirectory(const char *path)
{
    return le_dir_MakePath(path, 0644);
}

void tafMngdStorageSvc::InitConfigStorage()
{
    auto &mss = tafMngdStorageSvc::GetInstance();
    le_result_t res =  mss.ParseServiceJsonConfig();
    if(res != LE_OK){
        LE_FATAL("Unable to parse json %s",DEFAULT_MSS_CONFIG_NAME);
    }

    res = CreateDirectory(configStorage);
    if(res != LE_OK){
        LE_FATAL("Unable to create directory %s",configStorage);
    }

    res = CreateDirectory(configRfsStorage);
    if(res != LE_OK){
        LE_FATAL("Unable to create directory %s",configRfsStorage);
    }

    le_result_t result = LE_OK;

    // Initialize FSC storage for configStorage and configRfsStorage
    cfgFscRef = taf_fsc_GetStorageRef(configStorage, &result);
    if(cfgFscRef == nullptr || result != LE_OK)
    {
        LE_FATAL("Failed to get fsc storage %s", configStorage);
    }

    result = taf_fsc_LockStorage(cfgFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", configStorage);
    }

    cfgRfsFscRef = taf_fsc_GetStorageRef(configRfsStorage, &result);
    if(cfgFscRef == nullptr || result != LE_OK)
    {
        LE_FATAL("Failed to get fsc storage %s", configRfsStorage);
    }

    result = taf_fsc_LockStorage(cfgRfsFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", configRfsStorage);
    }

    configStoragePool = le_mem_CreatePool("configStoragePool",
        sizeof(tafMngdStorage_ConfigFileData_t));

    versionStoragePool = le_mem_CreatePool("versionStoragePool",
        sizeof(tafMngdStorage_ConfigVersionInfo_t));

    configStorageRefMap = le_ref_CreateMap("ConfigStorageRefMap", MAX_NUM_OF_CONFIG_STORAGE);

    //Loads Plugin Module.
    cfgStorInf = (cfgStor_Inf_t*)taf_devMgr_LoadDrv(TAF_CFGSTOR_MODULE_NAME, NULL);

    if (cfgStorInf == NULL)
    {
        LE_WARN("Config storage PI module is not loaded.");
    }
    else
    {
        if (cfgStorInf->init != NULL)
        {
            LE_INFO("Config storage PI init...");
            (*(cfgStorInf->init))();
        }
    }
}

le_result_t tafMngdStorageSvc::ParseServiceJsonConfig(){
    LE_INFO("Parsing %s", DEFAULT_MSS_CONFIG_NAME);
    std::ifstream jfile(DEFAULT_MSS_CONFIG_NAME);
    if(!jfile.is_open()){
        LE_WARN ("Unable to open %s", DEFAULT_MSS_CONFIG_NAME);
        return LE_FAULT;
    }

    // Create a root
    pt::ptree root;
    // Load the json file in this ptree
    try
    {
        pt::read_json(DEFAULT_MSS_CONFIG_NAME, root);
    }
    catch (const std::exception &e)
    {
        LE_WARN ("read_json exception: %s. Check validity of JSON.", e.what());
        return LE_FAULT;
    }
    std::string product = root.get<std::string>("Product");
     if (product != "TelAF"){
        LE_WARN("Invalid JSON property value");
        return LE_FAULT;
    }
    std::string name = root.get<std::string>("Name");
    if(name != "MSS"){
        LE_WARN("Invalid JSON property value");
        return LE_FAULT;
    }
    try {
        for (const auto& item : root.get_child("MSS Config Storage.Configuration.UpdatePath")) {
            const boost::property_tree::ptree& uPath = item.second;
            std::string fPath = uPath.get<std::string>("Path");
            snprintf(updatePath,sizeof(updatePath),"%s",fPath.c_str());
            bool format  =  uPath.get<bool>("QcmFormat");
            isQcmFormat =  format;
            LE_INFO("update path is %s and qc format is %d",updatePath,format);
        }

        for (const auto& item : root.get_child("MSS Config Storage.Configuration.StoragePath")) {
            const boost::property_tree::ptree& uPath = item.second;
            std::string basePath = uPath.get<std::string>("BasePath");
            snprintf(configStorage,sizeof(configStorage),"%s",basePath.c_str());
            std::string backupPath = uPath.get<std::string>("BackupPath");
            LE_INFO("Base path is %s",configStorage);
            snprintf(configRfsStorage,sizeof(configRfsStorage),"%s",backupPath.c_str());
            LE_INFO("Backup path is %s",configRfsStorage);
        }
    } catch (const boost::property_tree::ptree_error& e) {
        LE_ERROR("Error accessing JSON data with error %s",e.what());
        return LE_FAULT;
    }
    return LE_OK;
}

bool tafMngdStorageSvc::IsFileExisting(const char *path)
{
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

taf_mngdStorCfg_ConfigRef_t tafMngdStorageSvc::GetRef(){
    LE_DEBUG("GetRef for config storage");
    if(mClientRefCount >= MAX_NUM_OF_CONFIG_STORAGE){
        LE_ERROR("Client Count reached max limit.");
        return NULL;
    }
    tafMngdStorage_ConfigStorage_t* configStorPtr =
        (tafMngdStorage_ConfigStorage_t*)le_mem_ForceAlloc(configStoragePool);
    memset(configStorPtr,0,sizeof(tafMngdStorage_ConfigStorage_t));
    configStorPtr->clientSessionRef = taf_mngdStorCfg_GetClientSessionRef();
    configStorPtr->configRef =
        (taf_mngdStorCfg_ConfigRef_t)le_ref_CreateRef(configStorageRefMap,configStorPtr);
    mClientRefCount++;
    return configStorPtr->configRef;
}

le_result_t tafMngdStorageSvc::Update(taf_mngdStorCfg_ConfigRef_t configStor,
    const char* version){

    le_result_t result;
    // Unlock configStorage and configRfsStorage
    result = UnlockStorage();
    if(result != LE_OK)
    {
        return result;
    }
    LE_INFO("Storage Unlocked");

    // For OEM format max 5 OEM json files supported.
    // if its QCM format only 1 config json file supported.

    uint32_t maxFiles=0;
    if(isQcmFormat){
        maxFiles = MAX_QCM_CONFIG_FILES;
    }else{
        maxFiles = MAX_OEM_CONFIG_FILES;
    }

    //Getting file from update path
    result = GetFiles(updatePath,maxFiles);
    if(result !=LE_OK){
        LockStorage();
        return result;
    }
    LE_INFO("Successfully found files.");

    //Authenticate Json File using Plugin Module.
    result = AuthenticateFile(maxFiles);
    if(result != LE_OK){
        LE_ERROR("unable to Authenticate all files.");
        LockStorage();
        return result;
    }

    LE_INFO("Authenticated all File Successfully");

    //Convert all json file to single mss json file.
    result  =  ConvertToSingleMssJson(maxFiles);
    if(result != LE_OK){
        LE_ERROR("unable to convert all files to single JSON file");
        LockStorage();
        return result;
    }
    LE_INFO("Successfully Convert to single json File");

    //Getting Active Storage Path.
    char storagePath[LIMIT_MAX_PATH_BYTES] =  {0};
    result =
        GetConfigStoragePath(storagePath,sizeof(storagePath));
    if(result != LE_OK){
        LockStorage();
        LE_ERROR("Unable to get Active Storage path");
        return result;
    }

    //Validate JSON schema
    result = ValidateJsonSchema(configStor,storagePath,version);
    if(result != LE_OK){
        LE_ERROR("Failed to validate json schema for file %s",storagePath);
        taf_rfs_Delete(storagePath);
        LockStorage();
        if(result ==LE_FORMAT_ERROR) return LE_FORMAT_ERROR;
        return result;
    }

    LE_INFO("Successfully validate json schema for file %s",storagePath);

    char MasterFilePath[LIMIT_MAX_PATH_BYTES] = {0};
    snprintf(MasterFilePath,sizeof(MasterFilePath),"%s%s",configStorage,CONFIG_FILE_NAME);

    // creating .bak file.
    if(IsFileExisting(MasterFilePath)){
        LE_INFO("Creating backup of file %s",MasterFilePath);
        char backUpPath[LIMIT_MAX_PATH_BYTES] = {0};
        snprintf(backUpPath,sizeof(backUpPath),"%s%s",configStorage,CONFIG_FILE_NAME_BAK);
        int output = taf_rfs_Copy(MasterFilePath,backUpPath);
        if(output != 0){
            LockStorage();
            LE_ERROR("unable to create backup for %s at %s with %d",MasterFilePath,backUpPath,output);
            return LE_FAULT;
        }
        LE_INFO("successfully created backup file at path %s",backUpPath);
    }

    // Lock configStorage and configRfsStorage
    result = LockStorage();
    if(result != LE_OK)
    {
        return result;
    }
    LE_INFO("Storage locked");
    return LE_OK;
}

le_result_t tafMngdStorageSvc::Activate(taf_mngdStorCfg_ConfigRef_t configRef){

    // Unlock configStorage and configRfsStorage
    le_result_t result = UnlockStorage();
    if(result != LE_OK)
    {
        return result;
    }
    LE_INFO("Storage unlocked");

    char filePath[LIMIT_MAX_PATH_BYTES] =  {0};
    result = GetConfigStoragePath(filePath,sizeof(filePath));
    if(result != LE_OK){
        LockStorage();
        LE_ERROR("Unable to get Active Storage path");
        return result;
    }

    //checking if file exists at path.
    if(!IsFileExisting(filePath)){
        LockStorage();
        LE_ERROR("Unable to find file at %s",filePath);
        return LE_FAULT;
    }

    char renamePath[LIMIT_MAX_PATH_BYTES] = {0};
    snprintf(renamePath,sizeof(renamePath),"%s%s",configStorage,CONFIG_FILE_NAME);

    int output = taf_rfs_Copy(filePath,renamePath);
    if(output != 0){
        LockStorage();
        LE_ERROR("unable to sync the file to path %s with error %d",renamePath,output);
        return LE_FAULT;
    }
    LE_INFO("successfully sync the file to path %s",renamePath);

    //Clear Config Tree
    result = ClearTree();
    if(result != LE_OK){
        LockStorage();
        return result;
    }

    //Import config Tree
    result = ImportTree(renamePath);
    if(result != LE_OK){
        LockStorage();
        return result;
    }

    // Lock configStorage and configRfsStorage
    result = LockStorage();
    if(result != LE_OK)
    {
        return LE_FAULT;
    }
    LE_INFO("Storage Locked");

    return LE_OK;
}

le_result_t tafMngdStorageSvc::Cancel(taf_mngdStorCfg_ConfigRef_t configRef){
    LE_DEBUG("Cancel the update campaign");

    // Unlock configStorage and configRfsStorage
    le_result_t result = UnlockStorage();
    if(result != LE_OK)
    {
        return LE_FAULT;
    }
    LE_INFO("Storage Locked");

    // Delete config.json.update file
     char storagePath[LIMIT_MAX_PATH_BYTES] =  {0};
    result = GetConfigStoragePath(storagePath,sizeof(storagePath));
    if(IsFileExisting(storagePath)){
        taf_rfs_Delete(storagePath);
        LE_INFO("Successfully deleted file from path %s",storagePath);
    }
    else{
        LE_ERROR("Unable to find file at %s",storagePath);
    }

    // Delete .bak file also.
    char bakFilePath[LIMIT_MAX_PATH_BYTES] =  {0};
    snprintf(bakFilePath,sizeof(bakFilePath),"%s%s",configStorage,CONFIG_FILE_NAME_BAK);
    if(IsFileExisting(bakFilePath)){
        taf_rfs_Delete(bakFilePath);
        LE_INFO("Successfully deleted file from path %s",bakFilePath);
    }
    else{
        LE_ERROR("Unable to find file at %s",bakFilePath);
    }

    // Release Version Info ptr
    if(versionInfo != NULL){
        le_mem_Release(versionInfo);
        versionInfo = NULL;
    }

    // Lock configStorage and configRfsStorage
    result = LockStorage();
    if(result != LE_OK)
    {
        return LE_FAULT;
    }
    LE_INFO("Storage Locked");

    return LE_OK;
}

le_result_t tafMngdStorageSvc::Rollback(taf_mngdStorCfg_ConfigRef_t configRef){
    LE_DEBUG("Rolling back config file to pervious version of file");

    // Unlock configStorage and configRfsStorage
    le_result_t  result = UnlockStorage();
    if(result != LE_OK)
    {
        return LE_FAULT;
    }
    LE_INFO("Storage unlocked");

    char backUpPath[LIMIT_MAX_PATH_BYTES] = {0};
    snprintf(backUpPath,sizeof(backUpPath),"%s%s",configStorage,CONFIG_FILE_NAME_BAK);
    char ConfigFilePath[LIMIT_MAX_PATH_BYTES] = {0};
    snprintf(ConfigFilePath,sizeof(ConfigFilePath),"%s%s",configStorage,CONFIG_FILE_NAME);
    //checks if backup file exists in directory.
    if(IsFileExisting(backUpPath)){
        int output =  taf_rfs_Copy(backUpPath,ConfigFilePath);
        if(output != 0){
            LockStorage();
            LE_ERROR("unable to rollback the file to path %s with error %d",ConfigFilePath,output);
            return LE_FAULT;
        }

        //Clearing the tree.
        result =  ClearTree();
        if(result != LE_OK){
            LockStorage();
            return result;
        }

        //Importing Tree For reverted File;
        result = ImportTree(ConfigFilePath);
        if(result != LE_OK){
            LockStorage();
            return result;
        }

        //update version Ptr
        if(versionInfo == NULL){
            versionInfo =
            (tafMngdStorage_ConfigVersionInfo_t*)le_mem_ForceAlloc(versionStoragePool);
            memset(versionInfo,0,sizeof(tafMngdStorage_ConfigVersionInfo_t));
        }
        le_result_t result = taf_mngdStorCfg_GetVersion(configRef,
            &versionInfo->majorVersion,&versionInfo->minorVersion,&versionInfo->patchVersion);
        if(result != LE_OK){
            LockStorage();
            LE_ERROR("Unable to get version");
            return result;
        }
    }
    else{
        LockStorage();
        LE_ERROR("File not exist at path %s",backUpPath);
        return LE_FAULT;
    }

    // Lock configStorage and configRfsStorage
    result = LockStorage();
    if(result != LE_OK)
    {
        return LE_FAULT;
    }
    LE_INFO("Storage Locked");

    return LE_OK;
}

le_result_t tafMngdStorageSvc::Commit(taf_mngdStorCfg_ConfigRef_t configRef){
    LE_DEBUG("Commiting data to config storage");

    // Unlock configStorage and configRfsStorage
    le_result_t result = UnlockStorage();
    if(result != LE_OK)
    {
        return LE_FAULT;
    }
    LE_INFO("Storage unlocked");

    char backUpPath[LIMIT_MAX_PATH_BYTES] = {0};
    snprintf(backUpPath,sizeof(backUpPath),"%s%s",configStorage,CONFIG_FILE_NAME_BAK);
    char ConfigFilePath[LIMIT_MAX_PATH_BYTES] = {0};
    snprintf(ConfigFilePath,sizeof(ConfigFilePath),"%s%s",configStorage,CONFIG_FILE_NAME);
    //checks if Configuration file exists
    if(IsFileExisting(ConfigFilePath)){
        //deletes .bak file if exists
        if(IsFileExisting(backUpPath)){
            taf_rfs_Delete(backUpPath);
            LE_INFO("Successfully deleted backup file at %s",backUpPath);
        }
        else{
            LE_ERROR("File not exists at %s",backUpPath);
        }
        char storagePath[LIMIT_MAX_PATH_BYTES] =  {0};
        result = GetConfigStoragePath(storagePath,sizeof(storagePath));
        if(!IsFileExisting(storagePath)){
            LockStorage();
            LE_ERROR("Unable to find file at %s",storagePath);
            return LE_OK;
        }
        taf_rfs_Delete(storagePath);
        LE_INFO("Successfully deleted file from path %s",storagePath);
    }
    else{
        LockStorage();
        LE_ERROR("Configuation file not exists at %s",ConfigFilePath);
        return LE_FAULT;
    }

    // Lock configStorage and configRfsStorage
    result = LockStorage();
    if(result != LE_OK)
    {
        return LE_FAULT;
    }
    LE_INFO("Storage Locked");
    return LE_OK;
}

le_result_t tafMngdStorageSvc::ReleaseRef(taf_mngdStorCfg_ConfigRef_t configRef){
    TAF_ERROR_IF_RET_VAL(configRef == nullptr,LE_BAD_PARAMETER,"Null reference(config storage)");
    tafMngdStorage_ConfigStorage_t* strPtr =
        (tafMngdStorage_ConfigStorage_t*)le_ref_Lookup(configStorageRefMap,configRef);
    TAF_ERROR_IF_RET_VAL(strPtr == nullptr, LE_BAD_PARAMETER, "Invalid para(null reference ptr)");
    le_ref_DeleteRef(configStorageRefMap, configRef);
    le_mem_Release(strPtr);
    mClientRefCount--;
    return LE_OK;
}

le_result_t tafMngdStorageSvc::LockStorage()
{
    le_result_t result = taf_fsc_LockStorage(cfgFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", configStorage);
        return LE_FAULT;
    }

    result = taf_fsc_LockStorage(cfgRfsFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", configRfsStorage);
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t tafMngdStorageSvc::UnlockStorage()
{
   le_result_t result = taf_fsc_UnlockStorage(cfgFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to unlock fsc storage %s", configStorage);
        return LE_FAULT;
    }

    result = taf_fsc_UnlockStorage(cfgRfsFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to unlock fsc storage %s", configRfsStorage);
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t tafMngdStorageSvc::ClearTree(){
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(TAF_MNGD_CFG_STORAGE_SVC_PATH);
    if(iterRef == NULL){
        LE_ERROR("unable to create iterator for tree");
        return LE_FAULT;
    }
    le_cfg_DeleteNode(iterRef, "");
    le_cfg_CommitTxn(iterRef);
    LE_INFO("Tree Cleared SuccessFully");
    return LE_OK;
}

le_result_t tafMngdStorageSvc::ImportTree(char* filePath){
    le_result_t result;
    le_cfg_IteratorRef_t itrRef = le_cfg_CreateWriteTxn(TAF_MNGD_CFG_STORAGE_SVC_PATH);
    result = le_cfgAdmin_ImportTree(itrRef, filePath, "");
    if(result != LE_OK){
        LE_ERROR("Not able to import tree from %s",filePath);
        le_cfg_CancelTxn(itrRef);
        return LE_FAULT;
    }

    LE_INFO("successfully import tree from %s",filePath);
    le_cfg_CommitTxn(itrRef);
    return LE_OK;
}

le_result_t tafMngdStorageSvc::GetConfigStoragePath(char* storagePtr, size_t storageSize){
    snprintf(storagePtr, storageSize, "%s%s%s", configStorage,CONFIG_FILE_NAME,".update");
    LE_INFO("Storage Path is %s",storagePtr);
    return LE_OK;
}

le_result_t tafMngdStorageSvc::GetConfigFilePath(char* filePtr, size_t fileSize,
    tafMngdStorage_ConfigFileData_t* configPtr){
    if(configPtr == NULL) return LE_FAULT;
    snprintf(filePtr, fileSize, "%s%s",updatePath,configPtr->fileName);
    LE_INFO("Storage Path is %s",filePtr);
    return LE_OK;
}

le_result_t tafMngdStorageSvc::GetFiles(const char *path,uint32_t maxFiles)
{
    struct dirent *d;
    DIR *dir = opendir(path);

    if (dir == NULL)
    {
        LE_ERROR("Unable to open directory");
        return LE_FAULT; // Return false if the directory cannot be opened
    }
    bool isEmpty = true;
    uint32_t i=0;
    while ((d = readdir(dir)) != NULL) {
        // ignoring '.' and '..'
        if(d->d_name[0] == '.' &&
            (d->d_name[1] =='\0' || (d->d_name[1] == '.' && d->d_name[2] == '\0'))) {
            continue;
        }
        // skip directory also.
        if(d->d_type == DT_DIR){
            continue;
        }
        //check files extension, .json only allowed.
        const char* ext = strrchr(d->d_name,'.');
        if(ext == NULL || strncmp(ext,".json",strlen(ext)) == 1){
            continue;
        }

        if(i>=maxFiles){
            LE_ERROR("Configuration files reached max limit of %d",maxFiles);
            closedir(dir);
            return LE_OUT_OF_RANGE;
        }
        configFileData[i] =
                (tafMngdStorage_ConfigFileData_t*)malloc(sizeof(tafMngdStorage_ConfigFileData_t));
        if(configFileData[i] != NULL){
            snprintf(configFileData[i]->fileName,
                        sizeof(configFileData[i]->fileName),"%s",d->d_name);
            LE_INFO("Found file %s",configFileData[i]->fileName);
        }
        i++;
        isEmpty = false;
    }
    closedir(dir);
    if(isEmpty == true){
        LE_ERROR("Files Not Found");
        return LE_NOT_FOUND;
    }
    return LE_OK;
}

le_result_t tafMngdStorageSvc::ConvertToSingleMssJson(uint32_t maxFiles){
    LE_INFO("Converting All files to single MSS json file");
    le_result_t result;
    char storagePath[LIMIT_MAX_PATH_BYTES] =  {0};
    result =
        GetConfigStoragePath(storagePath,sizeof(storagePath));
    if(result != LE_OK){
        LE_ERROR("Unable to get Active Storage path");
        return LE_FAULT;
    }
    for(uint32_t i=0;i<maxFiles;i++){
        tafMngdStorage_ConfigFileData_t* configFilePtr = configFileData[i];
        if(configFilePtr){
            LE_INFO("Updating file %s",configFilePtr->fileName);
            //Getting Config File Path.
            char FilePath[LIMIT_MAX_PATH_BYTES] =  {0};
            result = GetConfigFilePath(FilePath,sizeof(FilePath),configFilePtr);
            if(result != LE_OK){
                LE_ERROR("Unable to get Storage path");
                return LE_FAULT;
            }

            //checking if file exists at path.
            if(!IsFileExisting(FilePath)){
                LE_ERROR("Unable to find file at %s",FilePath);
                return LE_FAULT;
            }
            if(!isQcmFormat){
                if(!cfgStorInf){
                    LE_ERROR("plugin Module not initialized..");
                    return LE_FAULT;
                }
                result = (*(cfgStorInf->merge))(storagePath,FilePath);
                if(result != LE_OK){
                    LE_ERROR("Unable to merge confile file from %s to %s", FilePath,storagePath);
                    return LE_FAULT;
                }
            }
            else{
                int output = taf_rfs_Copy(FilePath,storagePath);
                if(output != 0){
                    LE_ERROR("unable to Copy File from %s to %s",FilePath,storagePath);
                    return LE_FAULT;
                }
            }
        }
    }
    return LE_OK;
}

le_result_t tafMngdStorageSvc::AuthenticateFile(uint32_t maxFiles){
    //Authenticate each file one by one.
    le_result_t result;
    for(uint32_t i=0;i<maxFiles;i++){
        tafMngdStorage_ConfigFileData_t* configFilePtr = configFileData[i];
        if(configFilePtr){
            LE_INFO("Authenticating file %s",configFilePtr->fileName);

            //Getting Config File Path.
            char FilePath[LIMIT_MAX_PATH_BYTES] =  {0};
            result = GetConfigFilePath(FilePath,sizeof(FilePath),configFilePtr);
            if(result != LE_OK){
                LE_ERROR("Unable to get Storage path");
                return LE_FAULT;
            }

            //checking if file exists at path.
            if(!IsFileExisting(FilePath)){
                LE_ERROR("Unable to find file at %s",FilePath);
                return LE_FAULT;
            }

            //authentication with plugin module.
            if(!cfgStorInf){
                LE_ERROR("plugin Module not initialized..");
                return LE_FAULT;
            }
            result = (*(cfgStorInf->auth))(FilePath);
            if(result != LE_OK){
                LE_ERROR("Unable to auth file");
                return LE_FAULT;
            }
        }
    }
    return LE_OK;
}

le_result_t tafMngdStorageSvc::ValidateJsonSchema(taf_mngdStorCfg_ConfigRef_t configStor,
    char* filePath,const char* version){
    uint32_t majVersion=0;
    uint32_t minVersion=0;
    uint32_t patchVersion=0;
    std::ifstream jsonFile(filePath);
    if (!jsonFile.is_open())
    {
        LE_WARN ("Unable to open %s",filePath);
        return LE_FAULT;
    }

    // Create a root
    pt::ptree root;
    // Load the json file in this ptree
    try
    {
        pt::read_json(filePath, root);
        for (const auto& element : root.get_child("children")) {
            const boost::property_tree::ptree& item = element.second;
            if(item.get< std::string>("name") == "MajorVersion"
                && item.get<std::string>("type") == "int"){
                majVersion= item.get<int>("value");
                LE_DEBUG("version of file %d",majVersion);
            }
            else if(item.get< std::string>("name") == "MinorVersion"
                && item.get<std::string>("type") == "int"){
                minVersion= item.get<int>("value");
                LE_DEBUG("version of file %d",minVersion);
            }
            else if(item.get< std::string>("name") == "PatchVersion"
                && item.get<std::string>("type") == "int"){
                patchVersion= item.get<int>("value");
                LE_DEBUG("version of file %d",patchVersion);
            }
            else{
                if(element.second.count("children")>0){
                    for(const auto& childnode : element.second.get_child("children")){
                        if(childnode.second.count("children")>0){
                            LE_ERROR("More than 2 level of steming found!");
                            return LE_FORMAT_ERROR;
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        LE_WARN ("read_json exception: %s. Check validity of JSON.", e.what());
        return LE_FAULT;
    }

    char updateVersion[LIMIT_MAX_PATH_BYTES] ={0};
    snprintf(updateVersion,sizeof(updateVersion),"%d.%d.%d",majVersion,minVersion,patchVersion);

    if(strcmp(version,updateVersion)!=0){
        LE_ERROR("Json file %s version mismatch with input version",filePath);
        return LE_FAULT;
    }

    LE_INFO("VERSION: %s updateVersion: %s",version,updateVersion);

    if(versionInfo == NULL){
        versionInfo =
            (tafMngdStorage_ConfigVersionInfo_t*)le_mem_ForceAlloc(versionStoragePool);
        memset(versionInfo,0,sizeof(tafMngdStorage_ConfigVersionInfo_t));
    }

    le_result_t result = GetVersion(configStor,
        &versionInfo->majorVersion,&versionInfo->minorVersion,&versionInfo->patchVersion);

    if(result != LE_OK){
       memset(versionInfo,0,sizeof(tafMngdStorage_ConfigVersionInfo_t));
    }
    LE_DEBUG("version of file %d %d %d",versionInfo->majorVersion,versionInfo->minorVersion,versionInfo->patchVersion );

    if(versionInfo->majorVersion < majVersion || versionInfo->minorVersion < minVersion
        || versionInfo->patchVersion < patchVersion){
        versionInfo->majorVersion = majVersion;
        versionInfo->minorVersion = minVersion;
        versionInfo->patchVersion = patchVersion;
    }
    else{
        LE_ERROR("Failed to check version, current version: %d, %d, %d, update version: %s",
                    versionInfo->majorVersion,versionInfo->minorVersion,versionInfo->patchVersion,
                    updateVersion);
        return LE_FORMAT_ERROR;
    }
    return LE_OK;
}

le_result_t tafMngdStorageSvc::GetVersion(taf_mngdStorCfg_ConfigRef_t ConfigRef,
    uint32_t *MajorVersionPtr,
    uint32_t *MinorVersionPtr,
    uint32_t *PatchVersionPtr)
{
    le_cfg_IteratorRef_t itrRef = le_cfg_CreateReadTxn(TAF_MNGD_CFG_STORAGE_SVC_PATH);
    le_result_t res = LE_FAULT;
    if (!itrRef)
    {
        LE_ERROR("Failed to create a read transcation");
        le_cfg_CancelTxn(itrRef);
        return LE_FAULT;
    }
    if (le_cfg_NodeExists(itrRef, CFG_NODE_MAJORVERSION) &&
        le_cfg_NodeExists(itrRef, CFG_NODE_MINORVERSION) &&
        le_cfg_NodeExists(itrRef, CFG_NODE_PATCHVERSION))
    {
        *MajorVersionPtr = le_cfg_QuickGetInt(CFG_NODE_MAJORVERSION_FULLPATH, INT_MAX);
        *MinorVersionPtr = le_cfg_QuickGetInt(CFG_NODE_MINORVERSION_FULLPATH, INT_MAX);
        *PatchVersionPtr = le_cfg_QuickGetInt(CFG_NODE_PATCHVERSION_FULLPATH, INT_MAX);
        if ((*MajorVersionPtr != INT_MAX) && (*MinorVersionPtr != INT_MAX) &&
            (*PatchVersionPtr != INT_MAX))
        {
            res = LE_OK;
        }
        LE_INFO("MajorVersion: %d MinorVersion: %d PatchVersion: %d", *MajorVersionPtr, *MinorVersionPtr, *PatchVersionPtr);
    }
    else
    {
        LE_ERROR("Node not exist");
    }
    le_cfg_CancelTxn(itrRef);
    return res;
}

le_result_t tafMngdStorageSvc::GetType(taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char *LE_NONNULL groupName,
    const char *LE_NONNULL nodeName,
    taf_mngdStorCfg_NodeType_t *typePtr)
{
    le_cfg_IteratorRef_t itrRef = le_cfg_CreateReadTxn(TAF_MNGD_CFG_STORAGE_SVC_PATH);
    if (!itrRef)
    {
        LE_ERROR("Failed to create a read transcation");
        le_cfg_CancelTxn(itrRef);
        return LE_FAULT;
    }
    if (le_cfg_NodeExists(itrRef,groupName))
    {
        le_cfg_GoToNode(itrRef, groupName);
        if(!le_cfg_NodeExists(itrRef,nodeName)){
            LE_ERROR("Unable to find node name %s",nodeName);
            le_cfg_CancelTxn(itrRef);
            return LE_NOT_FOUND;
        }
        le_cfg_nodeType_t nodeType = le_cfg_GetNodeType(itrRef, nodeName);
        LE_INFO("nodeType:%d", nodeType);
        switch (nodeType)
        {
        case LE_CFG_TYPE_STRING:
            *typePtr = TAF_MNGDSTORCFG_TYPE_STRING;
            break;
        case LE_CFG_TYPE_BOOL:
            *typePtr = TAF_MNGDSTORCFG_TYPE_BOOL;
            break;
        case LE_CFG_TYPE_INT:
            *typePtr = TAF_MNGDSTORCFG_TYPE_INT;
            break;
        case LE_CFG_TYPE_FLOAT:
            *typePtr = TAF_MNGDSTORCFG_TYPE_FLOAT;
            break;
        default:
            le_cfg_CancelTxn(itrRef);
            return LE_UNAVAILABLE;
        }
    }
    le_cfg_CancelTxn(itrRef);
    return LE_OK;
}

le_result_t tafMngdStorageSvc::GetString(taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char *LE_NONNULL groupName,
    const char *LE_NONNULL nodeName,
    char *nodeValue,
    size_t nodeValueSize)
{
    if(CheckNodeExsist(groupName,nodeName) != LE_OK){
        return LE_NOT_FOUND;
    }
    le_result_t result = LE_NOT_FOUND;
    char nodePathVal[LIMIT_MAX_PATH_BYTES];
    snprintf(nodePathVal, LIMIT_MAX_PATH_BYTES, "%s%s/%s",
                TAF_MNGD_CFG_STORAGE_SVC_PATH, groupName, nodeName);
    result = le_cfg_QuickGetString(nodePathVal, nodeValue, nodeValueSize, "");
    if (result == LE_OK && strlen(nodeValue) != 0)
    {
        LE_INFO("nodeValue:%s",nodeValue);
    }
    else{
        result = LE_FAULT;
    }
    return result;
}

le_result_t tafMngdStorageSvc::GetInt(taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char *LE_NONNULL groupName,
    const char *LE_NONNULL nodeName,
    int32_t *nodeValuePtr)
{
    if(CheckNodeExsist(groupName,nodeName) != LE_OK){
        return LE_NOT_FOUND;
    }
    le_result_t result = LE_OK;
    char nodePathVal[LIMIT_MAX_PATH_BYTES];
    snprintf(nodePathVal, LIMIT_MAX_PATH_BYTES, "%s%s/%s",
                TAF_MNGD_CFG_STORAGE_SVC_PATH, groupName, nodeName);
    *nodeValuePtr = le_cfg_QuickGetInt(nodePathVal, INT_MAX);
    if (*nodeValuePtr == INT_MAX)
    {
        LE_ERROR("Unable to get value for node %s",nodePathVal);
        result = LE_FAULT;
    }
    return result;
}

le_result_t tafMngdStorageSvc::GetFloat(taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char *LE_NONNULL groupName,
    const char *LE_NONNULL nodeName,
    double *nodeValuePtr)
{
    if(CheckNodeExsist(groupName,nodeName) != LE_OK){
        return LE_NOT_FOUND;
    }
    le_result_t result = LE_OK;
    char nodePathVal[LIMIT_MAX_PATH_BYTES];
    snprintf(nodePathVal, LIMIT_MAX_PATH_BYTES, "%s%s/%s",
                TAF_MNGD_CFG_STORAGE_SVC_PATH, groupName, nodeName);
    *nodeValuePtr = le_cfg_QuickGetFloat(nodePathVal, FLT_MAX);
    if (*nodeValuePtr == FLT_MAX)
    {
        LE_ERROR("Unable to get value for node %s",nodePathVal);
        result = LE_FAULT;
    }
    return result;
}

le_result_t tafMngdStorageSvc::GetBool(taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char *LE_NONNULL groupName,
    const char *LE_NONNULL nodeName,
    int32_t *nodeValuePtr)
{
    if(CheckNodeExsist(groupName,nodeName) != LE_OK){
        return LE_NOT_FOUND;
    }
    le_result_t result = LE_OK;
    char nodePathVal[LIMIT_MAX_PATH_BYTES];
    snprintf(nodePathVal, LIMIT_MAX_PATH_BYTES, "%s%s/%s",
                TAF_MNGD_CFG_STORAGE_SVC_PATH, groupName, nodeName);
    if (le_cfg_QuickGetBool(nodePathVal, false))
    {
        *nodeValuePtr = 1;
    }
    else
    {
        *nodeValuePtr = 0;
    }

    return result;
}

le_result_t tafMngdStorageSvc::GetValue(taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char *LE_NONNULL groupName,
    const char *LE_NONNULL nodeName,
    taf_mngdStorCfg_NodeType_t *typePtr,
    char *nodeValue,
    size_t nodeValueSize)
{
    le_result_t result = LE_NOT_FOUND;
    int32_t nodeBoolValue = 0;
    int32_t nodeIntValue = 0;
    double nodeDoubleValue = 0.000;
    result = tafMngdStorageSvc::GetType(ConfigRef, groupName, nodeName, typePtr);
    if (result == LE_OK)
    {
        switch (*typePtr)
        {
        case TAF_MNGDSTORCFG_TYPE_STRING:
            result = tafMngdStorageSvc::GetString(ConfigRef, groupName, nodeName,
                nodeValue, nodeValueSize);
            if (result != LE_OK)
            {
                LE_ERROR("GetString Failed");
                return result;
            }
            LE_INFO("type as String value: %s", nodeValue);
            break;
        case TAF_MNGDSTORCFG_TYPE_BOOL:
            result = tafMngdStorageSvc::GetBool(ConfigRef, groupName, nodeName, &nodeBoolValue);
            if (result != LE_OK)
            {
                LE_ERROR("GetBool Failed");
                return result;
            }
            snprintf(nodeValue,nodeValueSize,"%s", nodeBoolValue==1 ? "true" : "false");
            LE_INFO("type as bool value: %s", nodeValue);
            break;
        case TAF_MNGDSTORCFG_TYPE_INT:
            result = tafMngdStorageSvc::GetInt(ConfigRef, groupName, nodeName, &nodeIntValue);
            if (result != LE_OK)
            {
                LE_ERROR("GetInt Failed");
                return result;
            }
            snprintf(nodeValue,nodeValueSize,"%d",nodeIntValue);
            LE_INFO("type as Int value: %s", nodeValue);
            break;
        case LE_CFG_TYPE_FLOAT:
            result = tafMngdStorageSvc::GetFloat(ConfigRef, groupName, nodeName, &nodeDoubleValue);
            if (result != LE_OK)
            {
               LE_ERROR("Getfloat Failed");
               return result;
            }
            snprintf(nodeValue,nodeValueSize,"%f",nodeDoubleValue);
            LE_INFO("type as Int value: %s", nodeValue);
            break;
        default:
            return LE_UNAVAILABLE;
        }
    }
    else
    {
        LE_INFO("Failed with GetType");
    }
    return result;
}

le_result_t tafMngdStorageSvc::CheckNodeExsist(const char *LE_NONNULL groupName,
    const char *LE_NONNULL nodeName){
    le_cfg_IteratorRef_t itrRef = le_cfg_CreateReadTxn(TAF_MNGD_CFG_STORAGE_SVC_PATH);
    if (!itrRef)
    {
        LE_ERROR("Failed to create a read transcation");
        le_cfg_CancelTxn(itrRef);
        return LE_FAULT;
    }
    if(le_cfg_NodeExists(itrRef,groupName)){
        le_cfg_GoToNode(itrRef,groupName);
        if(le_cfg_NodeExists(itrRef,nodeName)){
            LE_INFO("Found groupName %s and nodeName %s",groupName,nodeName);
        }
        else{
            le_cfg_CancelTxn(itrRef);
            LE_ERROR("Unable to find node %s",nodeName);
            return LE_NOT_FOUND;
        }
    }
    else{
        le_cfg_CancelTxn(itrRef);
        LE_ERROR("Unable to find node %s",groupName);
        return LE_NOT_FOUND;
    }
    le_cfg_CancelTxn(itrRef);
    return LE_OK;
}