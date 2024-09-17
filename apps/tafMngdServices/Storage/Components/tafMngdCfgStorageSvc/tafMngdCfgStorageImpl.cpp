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
    InitConfigStorage();
    taf_rfs_Init(true, nullptr);
}

void tafMngdStorageSvc::InitConfigStorage(){
    auto &mss = tafMngdStorageSvc::GetInstance();
    struct stat sb;

    //assuming config rfs directory exist.
    if(stat(CONFIG_RFS,&sb) == -1)
    {
        LE_INFO("Config storage directory not exists");
        if(mkdir(CONFIG_RFS, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", CONFIG_RFS);
            exit(-1);
        }
    }
    else if((sb.st_mode & S_IFMT) == S_IFDIR)
    {
        if(stat(CONFIG_RFS_STORAGE,&sb) == -1)
        {
            LE_INFO("RFS storage was found!");
            if(mkdir(CONFIG_RFS_STORAGE, 0755) != 0)
            {
                LE_ERROR("Failed to create the dir %s", CONFIG_RFS_STORAGE);
                exit(-1);
            }
        }
    }
    else
    {
        // some other file objects. delete first
        LE_ERROR("Delete the file object, then create the storage");

        // try to delete it
        unlink(CONFIG_RFS);

        // Create the directory
        if(mkdir(CONFIG_RFS, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", CONFIG_RFS);
            exit(-1);
        }

        if(mkdir(CONFIG_RFS_STORAGE, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", CONFIG_RFS_STORAGE);
            exit(-1);
        }
    }

    //assuming config storage directory exist.
    if(stat(CONFIG_STORAGE,&sb) == -1){

        LE_INFO("Config storage directory not exists");
        if(mkdir(CONFIG_STORAGE, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", CONFIG_STORAGE);
            exit(-1);
        }
    }
    else if((sb.st_mode & S_IFMT) == S_IFDIR)
    {
        // assume the sub dir was created as well. do nothing
        LE_INFO("RFS storage was found!");
    }
    else
    {
        // some other file objects. delete first
        LE_ERROR("Delete the file object, then create the storage");

        // try to delete it
        unlink(CONFIG_STORAGE);

        // Create the directory
        if(mkdir(CONFIG_STORAGE, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", CONFIG_STORAGE);
            exit(-1);
        }
    }

    // Set up RFS backup storage to the specified path
    // if(taf_rfs_SetBackupStorage(CONFIG_RFS_STORAGE) != LE_OK)
    // {
    //     LE_ERROR("Failed to set rfs backup storage %s", CONFIG_RFS_STORAGE);
    //     exit(-1);
    // }

    le_result_t result = LE_OK;

    // Initialize FSC storage for CONFIG_STORAGE and CONFIG_RFS_STORAGE
    cfgFscRef = taf_fsc_GetStorageRef(CONFIG_STORAGE, &result);
    if(cfgFscRef == nullptr || result != LE_OK)
    {
        LE_ERROR("Failed to get fsc storage %s", CONFIG_STORAGE);
        exit(-1);
    }

    result = taf_fsc_LockStorage(cfgFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", CONFIG_STORAGE);
        exit(-1);
    }

    cfgRfsFscRef = taf_fsc_GetStorageRef(CONFIG_RFS_STORAGE, &result);
    if(cfgFscRef == nullptr || result != LE_OK)
    {
        LE_ERROR("Failed to get fsc storage %s", CONFIG_RFS_STORAGE);
        exit(-1);
    }

    result = taf_fsc_LockStorage(cfgRfsFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", CONFIG_RFS_STORAGE);
        exit(-1);
    }

    configStoragePool = le_mem_CreatePool("configStoragePool",
        sizeof(tafMngdStorage_ConfigFileData_t));

    configStorageRefMap = le_ref_CreateMap("ConfigStorageRefMap", MAX_NUM_OF_CONFIG_STORAGE);

    result =  mss.ParseServiceJsonConfig();
    if(result != LE_OK){
        LE_ERROR("Unable to parse json %s",DEFAULT_MSS_CONFIG_PATH);
        exit(-1);
    }

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
    LE_INFO("Parsing %s", DEFAULT_MSS_CONFIG_PATH);
    std::ifstream jfile(DEFAULT_MSS_CONFIG_PATH);
    if(!jfile.is_open()){
        LE_WARN ("Unable to open %s", DEFAULT_MSS_CONFIG_PATH);
        return LE_FAULT;
    }

    // Create a root
    pt::ptree root;
    // Load the json file in this ptree
    try
    {
        pt::read_json(DEFAULT_MSS_CONFIG_PATH, root);
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
            LE_INFO("update path is %s",updatePath);
        }
    } catch (const boost::property_tree::ptree_error& e) {
        LE_ERROR("Error accessing JSON data");
        return LE_FAULT;
    }
    return LE_OK;
}

bool tafMngdStorageSvc::IsFileExisting(const char *path)
{
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

le_result_t tafMngdStorageSvc::UpdateFile(){

    le_result_t result;
    // Unlock CONFIG_STORAGE and CONFIG_RFS_STORAGE
    result = taf_fsc_UnlockStorage(cfgFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to unlock fsc storage %s", CONFIG_STORAGE);
        return LE_FAULT;
    }

    result = taf_fsc_UnlockStorage(cfgRfsFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to unlock fsc storage %s", CONFIG_RFS_STORAGE);
        return LE_FAULT;
    }

    //Getting file from update path
    result = GetFiles(updatePath);
    if(result !=LE_OK){
        return result;
    }
    LE_INFO("Successfully found files.");

    //Authenticate Json File using Plugin Module.
    result = AuthenticateFile();
    if(result != LE_OK){
        LE_ERROR("unable to Authenticate all files.");
        return result;
    }

    LE_INFO("Authenticated all File Successfully");

    //Convert all json file to single mss json file.
    result  =  ConvertToSingleMssJson();
    if(result != LE_OK){
        LE_ERROR("unable to convert all files to single JSON file");
        return result;
    }

    LE_INFO("Successfully Convert to single json File");
    //Getting Active Storage Path.
    char storagePath[LIMIT_MAX_PATH_BYTES] =  {0};
    result =
        GetConfigStoragePath(storagePath,sizeof(storagePath));
    if(result != LE_OK){
        LE_ERROR("Unable to get Active Storage path");
        return result;
    }

    //Validate JSON schema
    result = ValidateJsonSchema(storagePath,sizeof(storagePath));
    if(result != LE_OK){
        LE_ERROR("Failed to validate json schema for file %s",storagePath);
        if(result ==LE_FORMAT_ERROR) return LE_FORMAT_ERROR;
        return result;
    }

    LE_INFO("Successfully validate json schema for file %s",storagePath);

    // Lock CONFIG_STORAGE and CONFIG_RFS_STORAGE
    result = taf_fsc_LockStorage(cfgFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", CONFIG_STORAGE);
        return LE_FAULT;
    }

    result = taf_fsc_LockStorage(cfgRfsFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", CONFIG_RFS_STORAGE);
        return LE_FAULT;
    }

    return LE_OK;
}

le_result_t tafMngdStorageSvc::Sync(){

    // Unlock CONFIG_STORAGE and CONFIG_RFS_STORAGE
    le_result_t result = taf_fsc_UnlockStorage(cfgFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to unlock fsc storage %s", CONFIG_STORAGE);
        return LE_FAULT;
    }

    result = taf_fsc_UnlockStorage(cfgRfsFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to unlock fsc storage %s", CONFIG_RFS_STORAGE);
        return LE_FAULT;
    }

    char filePath[LIMIT_MAX_PATH_BYTES] =  {0};
    result = GetConfigStoragePath(filePath,sizeof(filePath));
    if(result != LE_OK){
        LE_ERROR("Unable to get Active Storage path");
        return result;
    }

    //checking if file exists at path.
    if(!IsFileExisting(filePath)){
        LE_ERROR("Unable to find file at %s",filePath);
        return LE_FAULT;
    }

    char renamePath[LIMIT_MAX_PATH_BYTES] = {0};
    snprintf(renamePath,sizeof(renamePath),"%s%s",CONFIG_STORAGE,CONFIG_FILE_NAME);

    // creating .bak file.
    if(IsFileExisting(renamePath)){
        LE_INFO("Creating backup of file %s",renamePath);
        char backUpPath[LIMIT_MAX_PATH_BYTES] = {0};
        snprintf(backUpPath,sizeof(backUpPath),"%s%s",CONFIG_STORAGE,CONFIG_FILE_NAME_BAK);
        int output = taf_rfs_Rename(renamePath,backUpPath);
        if(output != 0){
            LE_ERROR("unable to create backup for %s at %s with %d",renamePath,backUpPath,output);
            return LE_FAULT;
        }
        LE_INFO("successfully created backup file at path %s",backUpPath);
    }

    int output = taf_rfs_Rename(filePath,renamePath);
    if(output != 0){
        LE_ERROR("unable to sync the file to path %s with error %d",renamePath,output);
        return LE_FAULT;
    }
    LE_INFO("successfully sync the file to path %s",renamePath);

    //Clear Config Tree
    result = ClearTree();
    if(result != LE_OK){
        return result;
    }

    //Import config Tree
    result = ImportTree(renamePath);
    if(result != LE_OK){
        return result;
    }

    // Lock CONFIG_STORAGE and CONFIG_RFS_STORAGE
    result = taf_fsc_LockStorage(cfgFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", CONFIG_STORAGE);
        return LE_FAULT;
    }

    result = taf_fsc_LockStorage(cfgRfsFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", CONFIG_RFS_STORAGE);
        return LE_FAULT;
    }

    return LE_OK;
}

le_result_t tafMngdStorageSvc::Cancel(){
    LE_DEBUG("Cancel the update campaign");

    // Unlock CONFIG_STORAGE and CONFIG_RFS_STORAGE
    le_result_t result = taf_fsc_UnlockStorage(cfgFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to unlock fsc storage %s", CONFIG_STORAGE);
        return LE_FAULT;
    }

    result = taf_fsc_UnlockStorage(cfgRfsFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to unlock fsc storage %s", CONFIG_RFS_STORAGE);
        return LE_FAULT;
    }

    char storagePath[LIMIT_MAX_PATH_BYTES] =  {0};
    result =
        GetConfigStoragePath(storagePath,sizeof(storagePath));
    if(result != LE_OK){
        LE_ERROR("Unable to get Active Storage path");
        return LE_FAULT;
    }
    if(!IsFileExisting(storagePath)){
        LE_ERROR("Unable to find file at %s",storagePath);
        return LE_OK;
    }
    taf_rfs_Delete(storagePath);
    LE_INFO("Successfully deleted file from path %s",storagePath);

    // Lock CONFIG_STORAGE and CONFIG_RFS_STORAGE
    result = taf_fsc_LockStorage(cfgFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", CONFIG_STORAGE);
        return LE_FAULT;
    }

    result = taf_fsc_LockStorage(cfgRfsFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", CONFIG_RFS_STORAGE);
        return LE_FAULT;
    }

    return LE_OK;
}

le_result_t tafMngdStorageSvc::Rollback(){
    LE_DEBUG("Rolling back config file to pervious version of file");

    // Unlock CONFIG_STORAGE and CONFIG_RFS_STORAGE
    le_result_t result = taf_fsc_UnlockStorage(cfgFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to unlock fsc storage %s", CONFIG_STORAGE);
        return LE_FAULT;
    }

    result = taf_fsc_UnlockStorage(cfgRfsFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to unlock fsc storage %s", CONFIG_RFS_STORAGE);
        return LE_FAULT;
    }

    char backUpPath[LIMIT_MAX_PATH_BYTES] = {0};
    snprintf(backUpPath,sizeof(backUpPath),"%s%s",CONFIG_STORAGE,CONFIG_FILE_NAME_BAK);
    char ConfigFilePath[LIMIT_MAX_PATH_BYTES] = {0};
    snprintf(ConfigFilePath,sizeof(ConfigFilePath),"%s%s",CONFIG_STORAGE,CONFIG_FILE_NAME);
    //checks if backup file exists in directory.
    if(IsFileExisting(backUpPath)){
        //delete Config.json before rollback, if it exsist.
        if(IsFileExisting(ConfigFilePath)){
            taf_rfs_Delete(ConfigFilePath);
        }
        int output =  taf_rfs_Rename(backUpPath,ConfigFilePath);
        if(output != 0){
            LE_ERROR("unable to rollback the file to path %s with error %d",ConfigFilePath,output);
            return LE_FAULT;
        }

        //Clearing the tree.
        result =  ClearTree();
        if(result != LE_OK){
            return result;
        }

        //Importing Tree For reverted File;
        result = ImportTree(ConfigFilePath);
        if(result != LE_OK){
            return result;
        }

    }
    else{
        LE_ERROR("File not exist at path %s",backUpPath);
        return LE_FAULT;
    }

    // Lock CONFIG_STORAGE and CONFIG_RFS_STORAGE
    result = taf_fsc_LockStorage(cfgFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", CONFIG_STORAGE);
        return LE_FAULT;
    }

    result = taf_fsc_LockStorage(cfgRfsFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", CONFIG_RFS_STORAGE);
        return LE_FAULT;
    }

    return LE_OK;
}

le_result_t tafMngdStorageSvc::Commit(){
    LE_DEBUG("Commiting data to config storage");

    // Unlock CONFIG_STORAGE and CONFIG_RFS_STORAGE
    le_result_t result = taf_fsc_UnlockStorage(cfgFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to unlock fsc storage %s", CONFIG_STORAGE);
        return LE_FAULT;
    }

    result = taf_fsc_UnlockStorage(cfgRfsFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to unlock fsc storage %s", CONFIG_RFS_STORAGE);
        return LE_FAULT;
    }

    char backUpPath[LIMIT_MAX_PATH_BYTES] = {0};
    snprintf(backUpPath,sizeof(backUpPath),"%s%s",CONFIG_STORAGE,CONFIG_FILE_NAME_BAK);
    char ConfigFilePath[LIMIT_MAX_PATH_BYTES] = {0};
    snprintf(ConfigFilePath,sizeof(ConfigFilePath),"%s%s",CONFIG_STORAGE,CONFIG_FILE_NAME);
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
    }
    else{
        LE_ERROR("Configuation file not exists at %s",ConfigFilePath);
        return LE_FAULT;
    }

    // Lock CONFIG_STORAGE and CONFIG_RFS_STORAGE
    result = taf_fsc_LockStorage(cfgFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", CONFIG_STORAGE);
        return LE_FAULT;
    }

    result = taf_fsc_LockStorage(cfgRfsFscRef);
    if(result != LE_OK)
    {
        LE_ERROR("Failed to lock fsc storage %s", CONFIG_RFS_STORAGE);
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t tafMngdStorageSvc::ClearTree(){
    char cfgRootDir[LE_CFG_STR_LEN_BYTES] = "";
    snprintf(cfgRootDir, LE_CFG_STR_LEN_BYTES, "%s","configuration");
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(cfgRootDir);
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
    static char pathBuffer[LE_CFG_STR_LEN_BYTES] = "";
    snprintf(pathBuffer, LE_CFG_STR_LEN_BYTES, "/%s","configuration");
    le_cfg_IteratorRef_t itrRef = le_cfg_CreateWriteTxn("");
    result = le_cfgAdmin_ImportTree(itrRef, filePath, pathBuffer);
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
    snprintf(storagePtr, storageSize, "%s%s%s", CONFIG_STORAGE,CONFIG_FILE_NAME,".update");
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

le_result_t tafMngdStorageSvc::GetFiles(const char *path)
{
    struct dirent *d;
    DIR *dir = opendir(path);

    if (dir == NULL)
    {
        LE_ERROR("Unable to open directory");
        return LE_FAULT; // Return false if the directory cannot be opened
    }
    bool isEmpty = true;
    int i=0;
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

        if(i>=MAX_CONFIG_FILES){
            LE_ERROR("Configuration files reached max limit of %d",MAX_CONFIG_FILES);
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

le_result_t tafMngdStorageSvc::ConvertToSingleMssJson(){
    LE_INFO("Converting All files to single MSS json file");
    le_result_t result;
    char storagePath[LIMIT_MAX_PATH_BYTES] =  {0};
    result =
        GetConfigStoragePath(storagePath,sizeof(storagePath));
    if(result != LE_OK){
        LE_ERROR("Unable to get Active Storage path");
        return LE_FAULT;
    }
    for(int i=0;i<MAX_CONFIG_FILES;i++){
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
    }
    return LE_OK;
}

le_result_t tafMngdStorageSvc::AuthenticateFile(){
    //Authenticate each file one by one.
    le_result_t result;
    for(int i=0;i<MAX_CONFIG_FILES;i++){
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

le_result_t tafMngdStorageSvc::ValidateJsonSchema(char* filePath, size_t fileSize){
    int majVersion=0;
    int minVersion=0;
    int patchVersion=0;
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

    if(versionInfo == NULL){
        versionInfo =
            (tafMngdStorage_ConfigVersionInfo_t*)malloc(sizeof(tafMngdStorage_ConfigVersionInfo_t));
        memset(versionInfo,0,sizeof(tafMngdStorage_ConfigVersionInfo_t));
    }
    LE_DEBUG("version of file %d %d %d",versionInfo->majorVersion,versionInfo->minorVersion,versionInfo->patchVersion );

    if(versionInfo->majorVersion == 0 && versionInfo->minorVersion == 0
        && versionInfo->patchVersion == 0){
        versionInfo->majorVersion = majVersion;
        versionInfo->minorVersion = minVersion;
        versionInfo->patchVersion = patchVersion;
    }
    else if(versionInfo->majorVersion <= majVersion && versionInfo->minorVersion <= minVersion
        && versionInfo->patchVersion < patchVersion){
        versionInfo->majorVersion = majVersion;
        versionInfo->minorVersion = minVersion;
        versionInfo->patchVersion = patchVersion;
    }
    else{
        LE_ERROR("Failed to validate json schema for %s",filePath);
        return LE_FORMAT_ERROR;
    }
return LE_OK;
}