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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <fstream>

using namespace telux::tafsvc;
namespace pt = boost::property_tree;

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

    configStoragePool = le_mem_CreatePool("configStoragePool",
        sizeof(tafMngdStorage_ConfigFileData_t));

    configStorageRefMap = le_ref_CreateMap("ConfigStorageRefMap", MAX_NUM_OF_CONFIG_STORAGE);

    le_result_t result =  mss.ParseServiceJsonConfig();
    if(result != LE_OK){
        LE_ERROR("Unable to parse json %s",MSS_CONFIG_PATH);
        exit(-1);
    }
}

le_result_t tafMngdStorageSvc::ParseServiceJsonConfig(){
    LE_INFO("Parsing %s", MSS_CONFIG_PATH);
    std::string Config_path = MSS_CONFIG_PATH;
    std::ifstream jsonFile(Config_path);
    if (!jsonFile.is_open())
    {
        LE_WARN ("Unable to open %s", MSS_CONFIG_PATH);
        LE_INFO("Trying to open default file");
        Config_path =  DEFAULT_MSS_CONFIG_PATH;
        std::ifstream jfile(Config_path);
        if(!jfile.is_open()){
            LE_WARN ("Unable to open %s", Config_path.c_str());
            return LE_FAULT;
        }

    }

    // Create a root
    pt::ptree root;
    // Load the json file in this ptree
    try
    {
        pt::read_json(Config_path, root);
    }
    catch (const std::exception &e)
    {
        LE_WARN ("read_json exception: %s. Check validity of JSON.", e.what());
        return LE_FAULT;
    }
    std::unordered_map<std::string,std::string> umap;
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
            const boost::property_tree::ptree& updatePath = item.second;
            std::string fPath = updatePath.get<std::string>("Path");
            std::string fName = updatePath.get<std::string>("Name");
            umap[fName] = fPath;
            LE_INFO("update Path for %s is %s",fName.c_str(),fPath.c_str());

        }

        for (const auto& item :
            root.get_child("MSS Config Storage.Configuration.FactoryGoldenCopyPath")) {
            const boost::property_tree::ptree& factoryPath = item.second;
            std::string fPath = factoryPath.get<std::string>("Path");
            std::string fName = factoryPath.get<std::string>("Name");
            if(umap.count(fName)>0){
                tafMngdStorage_ConfigFileData_t* configPtr =
                    (tafMngdStorage_ConfigFileData_t*)
                    le_mem_ForceAlloc(configStoragePool);

                memset((void*)configPtr, 0,
                    sizeof(tafMngdStorage_ConfigFileData_t));

                configPtr->fileRef = (taf_mngdStorCfg_ConfigRef_t)
                    le_ref_CreateRef(configStorageRefMap,configPtr);

                snprintf(configPtr->updatePath,
                    sizeof(configPtr->updatePath),"%s",umap[fName].c_str());

                snprintf(configPtr->goldenCopyPath,
                    sizeof(configPtr->goldenCopyPath),"%s",fPath.c_str());

                snprintf(configPtr->fileName, sizeof(configPtr->fileName),"%s",
                    fName.c_str());

                configPtr->clientSessionRef =
                    taf_mngdStorCfg_GetClientSessionRef();

                LE_INFO("update Path for %s is %s",configPtr->updatePath,
                    configPtr->fileName);
        }
        }
    } catch (const boost::property_tree::ptree_error& e) {
        LE_ERROR("Error accessing JSON data");
        return LE_FAULT;
    }

    umap.clear();
    return LE_OK;
}

le_result_t tafMngdStorageSvc::UpdateFile(){
    le_ref_IterRef_t iterRef = le_ref_GetIterator(configStorageRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK){
        tafMngdStorage_ConfigFileData_t* configFilePtr =
            (tafMngdStorage_ConfigFileData_t*)le_ref_GetValue(iterRef);
        if(configFilePtr){
            LE_INFO("Updating file %s",configFilePtr->fileName);
            //Getting Active Storage Path.
            char storagePath[LIMIT_MAX_PATH_BYTES] =  {0};
            le_result_t result =
                GetConfigStoragePath(storagePath,sizeof(storagePath),configFilePtr);
            if(result != LE_OK){
                LE_ERROR("Unable to get Active Storage path");
                return LE_FAULT;
            }

            //Getting Config File Path.
            char FilePath[LIMIT_MAX_PATH_BYTES] =  {0};
            result = GetConfigFilePath(FilePath,sizeof(FilePath),configFilePtr);
            if(result != LE_OK){
                LE_ERROR("Unable to get Storage path");
                return LE_FAULT;
            }
            //Needs to authenticate above User file before Writing it to storage.

            //checking if file exists at path.
            if(!IsFileExisting(FilePath)){
                LE_ERROR("Unable to find file at %s",FilePath);
                return LE_FAULT;
            }
        taf_rfs_Copy(FilePath, storagePath);
        }
    }
    return LE_OK;
}

le_result_t tafMngdStorageSvc::Sync(){
    le_ref_IterRef_t iterRef = le_ref_GetIterator(configStorageRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK){
        tafMngdStorage_ConfigFileData_t* configFilePtr =
            (tafMngdStorage_ConfigFileData_t*)le_ref_GetValue(iterRef);
        if(configFilePtr){
            LE_INFO("Syncing file %s",configFilePtr->fileName);
            char filePath[LIMIT_MAX_PATH_BYTES] =  {0};
            le_result_t result =
                GetConfigStoragePath(filePath,sizeof(filePath),configFilePtr);
            if(result != LE_OK){
                LE_ERROR("Unable to get Active Storage path");
                return LE_FAULT;
            }

            //checking if file exists at path.
            if(!IsFileExisting(filePath)){
                LE_ERROR("Unable to find file at %s",filePath);
                return LE_FAULT;
            }

            char renamePath[LIMIT_MAX_PATH_BYTES] = {0};
            snprintf(renamePath,sizeof(renamePath),"%s%s",CONFIG_STORAGE,configFilePtr->fileName);

            int output = taf_rfs_Rename(filePath,renamePath);
            if(output != 0){
                LE_ERROR("unable to sync the file to path %s",renamePath);
                return LE_FAULT;
            }
            LE_INFO("successfully sync the file to path %s",renamePath);

            //Import config Tree

            static char pathBuffer[LE_CFG_STR_LEN_BYTES] = "";
            snprintf(pathBuffer, LE_CFG_STR_LEN_BYTES, "/%s","configuration");
            le_cfg_IteratorRef_t itrRef = le_cfg_CreateWriteTxn("");
            result = le_cfgAdmin_ImportTree(itrRef, renamePath, pathBuffer);
            if(result != LE_OK){
                LE_ERROR("Not able to import tree from %s",renamePath);
                le_cfg_CancelTxn(itrRef);
                return LE_FAULT;
            }

            LE_INFO("successfully import tree from %s",renamePath);
            le_cfg_CommitTxn(itrRef);
        }
    }
    return LE_OK;
}

le_result_t tafMngdStorageSvc::GetConfigStoragePath(char* storagePtr, size_t storageSize,
    tafMngdStorage_ConfigFileData_t* configPtr){
        snprintf(storagePtr, storageSize, "%s%s%s", CONFIG_STORAGE,configPtr->fileName,".update");
        LE_INFO("Storage Path is %s",storagePtr);
    return LE_OK;
}

le_result_t tafMngdStorageSvc::GetConfigFilePath(char* filePtr, size_t fileSize,
    tafMngdStorage_ConfigFileData_t* configPtr){
    snprintf(filePtr, fileSize, "%s%s", configPtr->updatePath,configPtr->fileName);
    LE_INFO("Storage Path is %s",filePtr);
    return LE_OK;
}