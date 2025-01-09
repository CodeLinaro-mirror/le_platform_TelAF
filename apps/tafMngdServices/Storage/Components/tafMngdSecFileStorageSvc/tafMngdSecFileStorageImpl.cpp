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

#include "tafMngdSecFileStorageSvc.hpp"

using namespace telux::tafsvc;
#include <unistd.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

/**
 * Get MSS instance
 */
tafMngdSecFileStorageSvc &tafMngdSecFileStorageSvc::GetInstance()
{
   static tafMngdSecFileStorageSvc instance;
   return instance;
}

le_result_t tafMngdSecFileStorageSvc::PreCheckExtensionJson()
{
    std::ifstream jfile(DEFAULT_MSS_CONFIG_NAME);
    if(!jfile.is_open()){
        LE_WARN ("Unable to open %s", DEFAULT_MSS_CONFIG_NAME);
        return LE_FAULT;
    }
    // Create a root
    pt::ptree root;
    std::string version = "";
    // Load the json file in this ptree
    try
    {
        pt::read_json(DEFAULT_MSS_CONFIG_NAME, root);
        version = root.get<std::string>("Version");
        LE_INFO("Version of Json is %s",version.c_str());
        std::string extension = root.get<std::string>("Extension");
        if (extension != ""){
            char extensionPath[LIMIT_MAX_PATH_BYTES];
            snprintf(extensionPath,LIMIT_MAX_PATH_BYTES,"%s%s",extension.c_str(),
                DEFAULT_MSS_CONFIG_NAME);
            if(IsFileExisting(extensionPath)){
                le_result_t result = ParseServiceJsonConfig(extensionPath);
                if(result == LE_OK){
                    LE_INFO("Service Initialize with extension json %s",extensionPath);
                    return result;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        LE_WARN ("read_json exception: %s. Check validity of JSON.", e.what());
        return LE_FAULT;
    }
    LE_INFO("Unable to intialize with extension json ,Intializing with default json");
    //Initializing with default json
    char configPath[LIMIT_MAX_PATH_BYTES];
    snprintf(configPath,LIMIT_MAX_PATH_BYTES,"%s",DEFAULT_MSS_CONFIG_NAME);
    le_result_t res = ParseServiceJsonConfig(configPath);
    return res;
}

le_result_t tafMngdSecFileStorageSvc::ParseServiceJsonConfig(char* configPath)
{
    LE_INFO("Parsing %s", configPath);
    std::ifstream jfile(configPath);
    if(!jfile.is_open()){
        LE_WARN ("Unable to open %s", configPath);
        return LE_FAULT;
    }

    // Create a root
    pt::ptree root;
    // Load the json file in this ptree
    try
    {
        pt::read_json(configPath, root);
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
        std::string svcJsonVersion = root.get<std::string>("Version");
        LE_INFO("Version of Json is %s",svcJsonVersion.c_str());
        for (const auto& item :
            root.get_child("MSS Secure File Storage.Configuration.StoragePath")) {
            const boost::property_tree::ptree& uPath = item.second;
            std::string basePath = uPath.get<std::string>("BasePath");
            snprintf(secFileStorage,sizeof(secFileStorage),"%s",basePath.c_str());
            std::string backupPath = uPath.get<std::string>("BackupPath");
            LE_INFO("Base path is %s",secFileStorage);
            snprintf(secFileRfsStorage,sizeof(secFileRfsStorage),"%s",backupPath.c_str());
            LE_INFO("Backup path is %s",secFileRfsStorage);
        }
    }
    catch (const std::exception &e)
    {
        LE_WARN ("read_json exception: %s. Check validity of JSON.", e.what());
        return LE_FAULT;
    }
    return LE_OK;
}

bool tafMngdSecFileStorageSvc::IsFileExisting(const char *path)
{
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

le_result_t tafMngdSecFileStorageSvc::CreateDirectory(const char *path)
{
    return le_dir_MakePath(path, 0644);
}

/**
 * tafMngdSecFileStorageSvc initialization
 */
void tafMngdSecFileStorageSvc::Init(void)
{
    le_result_t result = PreCheckExtensionJson();
    if(result != LE_OK){
        LE_FATAL("Failed to read service json");
    }

    result = CreateDirectory(secFileStorage);
    if(result != LE_OK){
        LE_FATAL("Unable to create directory %s",secFileStorage);
    }

    result = CreateDirectory(secFileRfsStorage);
    if(result != LE_OK){
        LE_FATAL("Unable to create directory %s",secFileRfsStorage);
    }

    // Create memory pools
    DirPool = le_mem_CreatePool("DirPool", sizeof(tafMngdSecFileStorage_Dir_t));

    // Create reference maps
    DirRefMap = le_ref_CreateMap("DirRefMap", SECFILE_MAX_NUM_OF_STORAGE);

    // Create memory pools
    ClientPool = le_mem_CreatePool("ClientPool", sizeof(tafMngdSecFileStorage_ClientCxt_t));

    // Create reference maps
    ClientRefMap = le_ref_CreateMap("ClientRefMap", SECFILE_MAX_NUM_OF_CLIENT);

    taf_rfs_Init(true, nullptr);

    taf_rfs_SetBackupStorage(secFileRfsStorage);
}

bool tafMngdSecFileStorageSvc::IsDirExisting(const char *path)
{
    struct stat buffer;
    if (stat(path, &buffer) != 0)
    {
        return false;
    }
    return S_ISDIR(buffer.st_mode);
}

le_result_t tafMngdSecFileStorageSvc::CheckValidPosixFileName(const char *fileName)
{
    // POSIX file name must not be empty
    if (fileName == nullptr || strlen(fileName) == 0)
    {
        return LE_BAD_PARAMETER;
    }

    // Iterate through each character in the fileName
    for (size_t i = 0; i < strlen(fileName); i++)
    {
        char ch = fileName[i];
        // Check if the character is alphanumeric or one of the allowed special characters
        if (!(isalnum((unsigned char)ch) || ch == '-' || ch == '_' || ch == '.'))
        {
            return LE_OUT_OF_RANGE;
        }
    }

    return LE_OK;
}

le_result_t tafMngdSecFileStorageSvc::GetAppNameBySessionRef
(
    le_msg_SessionRef_t clientSessionRef,           ///< [IN]  client session reference.
    char    *appNameStr,                            ///< [OUT] Application name buffer.
    size_t   appNameSize                            ///< [IN]  Buffer size.
)
{
    pid_t pid;
    const char* namePtr = nullptr;
    char procPath[LIMIT_MAX_PATH_BYTES] = {0};
    char appPath[LIMIT_MAX_PATH_BYTES] = {0};

    // Parameter check.
    if ((clientSessionRef == nullptr) || (appNameStr == nullptr) || (appNameSize == 0))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    // Get pid from the sessionRef.
    if (le_msg_GetClientProcessId(clientSessionRef, &pid) != LE_OK)
    {
        LE_ERROR("Failed to get the pid from client session reference.");
        return LE_FAULT;
    }

    // Get the app name from the pid.
    if (le_appInfo_GetName(pid, appPath, sizeof(appPath)) != LE_OK)
    {
        // It's not a telaf app but should a legacy app.
        // Read the program name from the softlink of /proc/<pid>/exe .
        LE_ASSERT(snprintf(procPath, sizeof(procPath), "/proc/%d/exe", pid)
                  < static_cast<int>(sizeof(procPath)));

        memset(appPath, 0, sizeof(appPath));
        if (readlink(procPath, appPath, sizeof(appPath)) < 0)
        {
            LE_ERROR("readlink(%s) failed %s", procPath, LE_ERRNO_TXT(errno));
            return LE_FAULT;
        }

        // Get the program name from the executable Path.
        namePtr = le_path_GetBasenamePtr(appPath, "/");
    }
    else
    {
        // It's a telaf app.
        namePtr = appPath;
    }

    snprintf(appNameStr, appNameSize, "%s", namePtr);

    LE_INFO("Get appName: %s", appNameStr);

    return LE_OK;
}
le_result_t tafMngdSecFileStorageSvc::GetStoragePath
(
    const char* basePathPtr,
    const char* storageNamePtr,
    char* bufferPtr,
    size_t bufferSize
)
{
    // Construct the storage path by concatenating base path and storage name
    snprintf(bufferPtr, bufferSize, "%s%s", basePathPtr, storageNamePtr);

    // Log the constructed storage path
    LE_INFO("Storage path is: %s", bufferPtr);

    return LE_OK;
}

bool tafMngdSecFileStorageSvc::IsDirectoryEmpty
(
    const char *dirname
)
{
    int n = 0;
    struct dirent *d;
    DIR *dir = opendir(dirname);

    // Check if the directory can be opened
    if (dir == nullptr)
    {
        LE_ERROR("Failed to open directory: %s", dirname);
        return false;
    }

    // Count the number of entries in the directory
    while ((d = readdir(dir)) != nullptr)
    {
        if (++n > 2)
            break;
    }
    closedir(dir);

    // If there are only '.' and '..', the directory is empty
    bool isEmpty = (n <= 2);
    LE_DEBUG("Directory %s is empty: %d", dirname, isEmpty);

    return isEmpty;
}

tafMngdSecFileStorage_DirRef_t tafMngdSecFileStorageSvc::CreateDirRef
(
    const char* basePathPtr,
    const char* storageNamePtr
)
{
    LE_INFO("CreateDirRef for '%s'", storageNamePtr);

    tafMngdSecFileStorage_Dir_t *dirPtr = nullptr;
    tafMngdSecFileStorage_DirRef_t dirRef = nullptr;
    char myAppName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };
    le_result_t res;

    // Allocate memory for the directory structure
    dirPtr = (tafMngdSecFileStorage_Dir_t*)le_mem_ForceAlloc(DirPool);
    if (dirPtr == nullptr)
    {
        LE_ERROR("Memory allocation failed");
        return nullptr;
    }

    memset((void*)dirPtr, 0, sizeof(tafMngdSecFileStorage_Dir_t));

    // Create a reference for the directory
    dirRef = (tafMngdSecFileStorage_DirRef_t)le_ref_CreateRef(DirRefMap, dirPtr);
    if (dirRef == nullptr)
    {
        LE_ERROR("Reference creation failed");
        goto cleanup;
    }

    snprintf(dirPtr->storageName, sizeof(dirPtr->storageName), "%s", storageNamePtr);

    // Get appName from client session
    if (LE_OK != GetAppNameBySessionRef(taf_mngdStorSecFile_GetClientSessionRef(),
                                        myAppName, sizeof(myAppName)))
    {
        LE_ERROR("Failed to get client appName.");
        goto cleanup;
    }

    snprintf(dirPtr->masterAppName, sizeof(dirPtr->masterAppName), "%s", myAppName);

    // Get the default storage path
    if (GetStoragePath(secFileStorage, storageNamePtr,
                        dirPtr->path, sizeof(dirPtr->path)) != LE_OK)
    {
        LE_ERROR("Cannot get storage path");
        goto cleanup;
    }

    // Get the FSC storage reference
    dirPtr->fscStorageRef = taf_fsc_GetStorageRef(dirPtr->path, &res);
    if (res != LE_OK || dirPtr->fscStorageRef == nullptr)
    {
        LE_ERROR("Cannot get FSC storage");
        goto cleanup;
    }

    // Lock the FSC storage
    if (taf_fsc_LockStorage(dirPtr->fscStorageRef) != LE_OK)
    {
        LE_ERROR("Cannot lock FSC storage");
        goto cleanup;
    }

    // Get the RFS storage path
    if (GetStoragePath(secFileRfsStorage, storageNamePtr,
                        dirPtr->rfsPath, sizeof(dirPtr->rfsPath)) != LE_OK)
    {
        LE_ERROR("Cannot get RFS storage path");
        goto cleanup;
    }

    // Get the RFS FSC storage reference
    dirPtr->rfs_fscStorageRef = taf_fsc_GetStorageRef(dirPtr->rfsPath, &res);
    if (res != LE_OK || dirPtr->rfs_fscStorageRef == nullptr)
    {
        LE_ERROR("Cannot get RFS FSC storage");
        goto cleanup;
    }

    // Lock the RFS FSC storage
    if (taf_fsc_LockStorage(dirPtr->rfs_fscStorageRef) != LE_OK)
    {
        LE_ERROR("Cannot lock RFS FSC storage");
        goto cleanup;
    }

    return dirRef;

cleanup:
    // Clean up resources in case of failure
    le_ref_DeleteRef(DirRefMap, dirRef);
    le_mem_Release(dirPtr);
    return nullptr;
}
/**
 * Find secure dir reference
 */
le_result_t tafMngdSecFileStorageSvc::FindDirRef
(
    const char* storageNamePtr,
    tafMngdSecFileStorage_DirRef_t* dirRef
)
{
    LE_DEBUG("FindDirRef");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(DirRefMap);

    // Scan all the data nodes
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        tafMngdSecFileStorage_Dir_t* dirPtr = (tafMngdSecFileStorage_Dir_t*)le_ref_GetValue(iterRef);
        if (dirPtr == nullptr)
        {
            LE_ERROR("dirPtr is nullptr");
            return LE_FAULT;
        }

        // Find the node that matches the current data
        if (strcmp(dirPtr->storageName, storageNamePtr) == 0)
        {
            *dirRef = (tafMngdSecFileStorage_DirRef_t)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Found storage '%s'", dirPtr->storageName);
            return LE_OK;
        }
    }

    return LE_NOT_FOUND;
}

le_result_t tafMngdSecFileStorageSvc::CreateStorageRefImpl
(
    const char* storageNamePtr,
    taf_mngdStorSecFile_ManagedCapMask_t capMask
)
{
    // Validate the storage name
    TAF_ERROR_IF_RET_VAL(CheckValidPosixFileName(storageNamePtr) != LE_OK,
                         LE_BAD_PARAMETER,
                         "Invalid storage string");

    char dirPath[LIMIT_MAX_PATH_BYTES] = {0};

    // Get the storage path
    if (GetStoragePath(secFileStorage, storageNamePtr, dirPath, sizeof(dirPath)) != LE_OK)
    {
        LE_ERROR("Cannot get storage path");
        return LE_FAULT;
    }

    // Check if the directory already exists
    if (IsDirExisting(dirPath))
    {
        LE_INFO("The directory '%s' already exists", dirPath);

        // Check if the directory is empty
        if (!IsDirectoryEmpty(dirPath))
        {
            LE_ERROR("The directory '%s' is not empty", dirPath);
            return LE_NOT_PERMITTED;
        }
    }

    tafMngdSecFileStorage_DirRef_t dirRef = nullptr;

    // Find or create the directory reference
    if (FindDirRef(storageNamePtr, &dirRef) == LE_NOT_FOUND)
    {
        dirRef = CreateDirRef(secFileStorage, storageNamePtr);
    }
    else
    {
        LE_ERROR("The directory reference '%s' already exists", dirPath);
        return LE_DUPLICATE;
    }

    TAF_ERROR_IF_RET_VAL(dirRef == nullptr, LE_FAULT, "Cannot create directory reference");

    return LE_OK;
}

/**
 * Find client data reference
 */
le_result_t tafMngdSecFileStorageSvc::FindClientCxtRef
(
    const char* storageNamePtr,
    taf_mngdStorSecFile_StorageRef_t* storageRefPtr
)
{
    LE_DEBUG("FindClientCxtRef");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(ClientRefMap);

    // Scan all the data nodes
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        tafMngdSecFileStorage_ClientCxt_t* clientPtr =
            (tafMngdSecFileStorage_ClientCxt_t*)le_ref_GetValue(iterRef);

        if (clientPtr == nullptr)
        {
            LE_ERROR("clientPtr is nullptr");
            return LE_FAULT;
        }

        // Find the node that matches the current client
        if ((strcmp(clientPtr->storageName, storageNamePtr) == 0) &&
            clientPtr->clientSessionRef == taf_mngdStorSecFile_GetClientSessionRef())
        {
            *storageRefPtr = (taf_mngdStorSecFile_StorageRef_t)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Found client context for client session (%p)", clientPtr->clientSessionRef);
            return LE_OK;
        }
    }

    return LE_NOT_FOUND;
}

taf_mngdStorSecFile_StorageRef_t tafMngdSecFileStorageSvc::GetStorageRefImpl
(
    const char* storageNamePtr
)
{
    // Validate the storage name
    TAF_ERROR_IF_RET_VAL(CheckValidPosixFileName(storageNamePtr) != LE_OK,
                         nullptr,
                         "Invalid storage string");

    char dirPath[LIMIT_MAX_PATH_BYTES] = {0};

    // Get the storage path
    if (GetStoragePath(secFileStorage, storageNamePtr, dirPath, sizeof(dirPath)) != LE_OK)
    {
        LE_ERROR("Cannot get storage path");
        return nullptr;
    }

    // Check if the directory already exists
    if (IsDirExisting(dirPath) == false)
    {
        LE_ERROR("The directory '%s' does not exist", dirPath);
        return nullptr;
    }

    taf_mngdStorSecFile_StorageRef_t storageRef;
    tafMngdSecFileStorage_ClientCxt_t* clientCtxPtr = nullptr;

    // Find or create the client context reference
    if (FindClientCxtRef(storageNamePtr, &storageRef) == LE_NOT_FOUND)
    {
        LE_INFO("Create new client context reference for '%s'", storageNamePtr);

        clientCtxPtr = (tafMngdSecFileStorage_ClientCxt_t*)le_mem_ForceAlloc(ClientPool);

        if (clientCtxPtr == nullptr)
        {
            LE_ERROR("Memory allocation failed");
            return nullptr;
        }

        memset((void*)clientCtxPtr, 0, sizeof(tafMngdSecFileStorage_ClientCxt_t));

        clientCtxPtr->storageRef =
            (taf_mngdStorSecFile_StorageRef_t)le_ref_CreateRef(ClientRefMap, clientCtxPtr);

        clientCtxPtr->clientSessionRef = taf_mngdStorSecFile_GetClientSessionRef();

        snprintf(clientCtxPtr->storageName, sizeof(clientCtxPtr->storageName),
                 "%s", storageNamePtr);

        storageRef = clientCtxPtr->storageRef;
    }
    else
    {
        clientCtxPtr = (tafMngdSecFileStorage_ClientCxt_t*)le_ref_Lookup(ClientRefMap, storageRef);

    }

    if (clientCtxPtr == nullptr)
    {
        LE_ERROR("Memory allocation failed");
        return nullptr;
    }

    // Find or create the directory reference
    if (FindDirRef(storageNamePtr, &(clientCtxPtr->dirRef)) == LE_NOT_FOUND)
    {
        clientCtxPtr->dirRef = CreateDirRef(secFileStorage, storageNamePtr);
        if (clientCtxPtr->dirRef == nullptr)
        {
            LE_ERROR("Cannot create directory context reference");
            le_mem_Release(clientCtxPtr);
            return nullptr;
        }
    }

    LE_INFO("Successfully obtained storage reference for '%s'", storageNamePtr);
    return storageRef;
}

le_result_t tafMngdSecFileStorageSvc::UnlockStorageImpl
(
    taf_mngdStorSecFile_StorageRef_t storageRef
)
{
    // Check if the storage reference is valid
    TAF_ERROR_IF_RET_VAL(storageRef == nullptr, LE_BAD_PARAMETER,
                         "Invalid storage reference");

    // Lookup the client context using the storage reference
    tafMngdSecFileStorage_ClientCxt_t* clienCxtPtr =
        (tafMngdSecFileStorage_ClientCxt_t*)le_ref_Lookup(ClientRefMap, storageRef);

    // Check if the client context is valid
    TAF_ERROR_IF_RET_VAL(clienCxtPtr == nullptr, LE_NOT_FOUND, "Invalid client data reference");

    // Lookup the directory using the client context's directory reference
    tafMngdSecFileStorage_Dir_t* dirPtr =
        (tafMngdSecFileStorage_Dir_t*)le_ref_Lookup(DirRefMap, clienCxtPtr->dirRef);

    // Check if the directory reference is valid
    TAF_ERROR_IF_RET_VAL(dirPtr == nullptr, LE_NOT_FOUND, "Invalid secure directory reference");

    // Unlock the RFS storage
    LE_DEBUG("Unlocking RFS storage for directory: %s", dirPtr->rfsPath);
    taf_fsc_UnlockStorage(dirPtr->rfs_fscStorageRef);

    // Unlock the FSC storage
    LE_DEBUG("Unlocking FSC storage for directory: %s", dirPtr->path);
    return taf_fsc_UnlockStorage(dirPtr->fscStorageRef);
}

le_result_t tafMngdSecFileStorageSvc::LockStorageImpl
(
    taf_mngdStorSecFile_StorageRef_t storageRef
)
{
    // Check if the storage reference is valid
    TAF_ERROR_IF_RET_VAL(storageRef == nullptr, LE_BAD_PARAMETER,
                         "Invalid storage reference");

    // Lookup the client context using the storage reference
    tafMngdSecFileStorage_ClientCxt_t* clienCxtPtr =
        (tafMngdSecFileStorage_ClientCxt_t*)le_ref_Lookup(ClientRefMap, storageRef);

    // Check if the client context is valid
    TAF_ERROR_IF_RET_VAL(clienCxtPtr == nullptr, LE_NOT_FOUND, "Invalid client data reference");

    // Lookup the directory using the client context's directory reference
    tafMngdSecFileStorage_Dir_t* dirPtr =
        (tafMngdSecFileStorage_Dir_t*)le_ref_Lookup(DirRefMap, clienCxtPtr->dirRef);

    // Check if the directory reference is valid
    TAF_ERROR_IF_RET_VAL(dirPtr == nullptr, LE_NOT_FOUND, "Invalid secure directory reference");

    // Lock the RFS storage
    LE_DEBUG("Locking RFS storage for directory: %s", dirPtr->rfsPath);
    taf_fsc_LockStorage(dirPtr->rfs_fscStorageRef);

    // Lock the FSC storage
    LE_DEBUG("Locking FSC storage for directory: %s", dirPtr->path);
    return taf_fsc_LockStorage(dirPtr->fscStorageRef);
}

le_result_t tafMngdSecFileStorageSvc::ImportFileImpl
(
    taf_mngdStorSecFile_StorageRef_t storageRef,
    const char* sourceFilePathPtr,
    const char* targetFilePathPtr
)
{
    // Check if the storage reference is valid
    TAF_ERROR_IF_RET_VAL(storageRef == nullptr,
                         LE_BAD_PARAMETER,
                         "Invalid storage reference");

    // Lookup the client context using the storage reference
    tafMngdSecFileStorage_ClientCxt_t* clienCxtPtr =
        (tafMngdSecFileStorage_ClientCxt_t*)le_ref_Lookup(ClientRefMap, storageRef);

    // Check if the client context is valid
    TAF_ERROR_IF_RET_VAL(clienCxtPtr == nullptr, LE_NOT_FOUND, "Invalid client data reference");

    // Lookup the directory using the client context's directory reference
    tafMngdSecFileStorage_Dir_t* dirPtr =
        (tafMngdSecFileStorage_Dir_t*)le_ref_Lookup(DirRefMap, clienCxtPtr->dirRef);

    // Check if the directory reference is valid
    TAF_ERROR_IF_RET_VAL(dirPtr == nullptr, LE_NOT_FOUND, "Invalid secure directory reference");

    // Check if the source file path is valid
    TAF_ERROR_IF_RET_VAL(sourceFilePathPtr == nullptr || strlen(sourceFilePathPtr) == 0,
                         LE_BAD_PARAMETER,
                         "Invalid source file path");

    // Check if the target file path is valid
    TAF_ERROR_IF_RET_VAL(targetFilePathPtr == nullptr || strlen(targetFilePathPtr) == 0,
                         LE_BAD_PARAMETER,
                         "Invalid target file path");

    char storageTargetFilePath[LIMIT_MAX_PATH_BYTES] = {0};

    // Construct the target file path within the storage directory
    snprintf(storageTargetFilePath, sizeof(storageTargetFilePath),
             "%s/%s", dirPtr->path, targetFilePathPtr);

    // Unlock the storage before performing the file operation
    LE_DEBUG("Unlocking FSC storage for directory: %s", dirPtr->path);
    taf_fsc_UnlockStorage(dirPtr->fscStorageRef);
    LE_DEBUG("Unlocking RFS storage for directory: %s", dirPtr->rfsPath);
    taf_fsc_UnlockStorage(dirPtr->rfs_fscStorageRef);

    // Set the backup storage path
    taf_rfs_SetBackupStorage(dirPtr->rfsPath);

    // Copy the file from the source to the target path
    if (taf_rfs_Copy(sourceFilePathPtr, storageTargetFilePath) != 0)
    {
        LE_ERROR("Failed to import file from %s to %s", sourceFilePathPtr, storageTargetFilePath);
        return LE_FAULT;
    }

    // Lock the storage after the file operation
    LE_DEBUG("Locking FSC storage for directory: %s", dirPtr->path);
    taf_fsc_LockStorage(dirPtr->fscStorageRef);
    LE_DEBUG("Locking RFS storage for directory: %s", dirPtr->rfsPath);
    taf_fsc_LockStorage(dirPtr->rfs_fscStorageRef);

    LE_INFO("Successfully imported file from %s to %s", sourceFilePathPtr, storageTargetFilePath);
    return LE_OK;
}

le_result_t tafMngdSecFileStorageSvc::ReadFileImpl
(
    taf_mngdStorSecFile_StorageRef_t storageRef,
    const char* filePath,
    uint8_t* bufPtr,
    size_t* bufSize
)
{
    // Check if the storage reference is valid
    TAF_ERROR_IF_RET_VAL(storageRef == nullptr,
                         LE_BAD_PARAMETER,
                         "Invalid storage reference");

    // Lookup the client context using the storage reference
    tafMngdSecFileStorage_ClientCxt_t* clienCxtPtr =
        (tafMngdSecFileStorage_ClientCxt_t*)le_ref_Lookup(ClientRefMap, storageRef);

    // Check if the client context is valid
    TAF_ERROR_IF_RET_VAL(clienCxtPtr == nullptr, LE_NOT_FOUND, "Invalid client data reference");

    // Lookup the directory using the client context's directory reference
    tafMngdSecFileStorage_Dir_t* dirPtr =
        (tafMngdSecFileStorage_Dir_t*)le_ref_Lookup(DirRefMap, clienCxtPtr->dirRef);

    // Check if the directory reference is valid
    TAF_ERROR_IF_RET_VAL(dirPtr == nullptr, LE_NOT_FOUND, "Invalid secure directory reference");

    // Check if the file path is valid
    TAF_ERROR_IF_RET_VAL(filePath == nullptr || strlen(filePath) == 0,
                         LE_BAD_PARAMETER,
                         "Invalid file path");

    // Check if the buffer pointer is valid
    TAF_ERROR_IF_RET_VAL(bufPtr == nullptr,
                         LE_BAD_PARAMETER,
                         "Invalid buffer pointer");

    char storageTargetFilePath[LIMIT_MAX_PATH_BYTES] = {0};

    // Construct the target file path within the storage directory
    snprintf(storageTargetFilePath, sizeof(storageTargetFilePath),
             "%s/%s", dirPtr->path, filePath);

    // Unlock the storage before performing the file operation
    LE_DEBUG("Unlocking FSC storage for directory: %s", dirPtr->path);
    taf_fsc_UnlockStorage(dirPtr->fscStorageRef);
    LE_DEBUG("Unlocking RFS storage for directory: %s", dirPtr->rfsPath);
    taf_fsc_UnlockStorage(dirPtr->rfs_fscStorageRef);

    // Set the backup storage path
    taf_rfs_SetBackupStorage(dirPtr->rfsPath);

    // Open the file for reading
    int fd = taf_rfs_Open(storageTargetFilePath, O_RDONLY, 0);
    if (fd < 0)
    {
        LE_ERROR("Failed to open file '%s'", storageTargetFilePath);
        taf_fsc_LockStorage(dirPtr->fscStorageRef);
        taf_fsc_LockStorage(dirPtr->rfs_fscStorageRef);
        return LE_FAULT;
    }

    // Read the file content into the buffer
    taf_rfs_Read(fd, (uint8_t*)bufPtr, bufSize);

    // Close the file
    taf_rfs_Close(fd);

    // Lock the storage after the file operation
    LE_DEBUG("Locking FSC storage for directory: %s", dirPtr->path);
    taf_fsc_LockStorage(dirPtr->fscStorageRef);
    LE_DEBUG("Locking RFS storage for directory: %s", dirPtr->rfsPath);
    taf_fsc_LockStorage(dirPtr->rfs_fscStorageRef);

    LE_INFO("Successfully read file '%s'", storageTargetFilePath);
    return LE_OK;
}

le_result_t tafMngdSecFileStorageSvc::DeleteFileImpl
(
    taf_mngdStorSecFile_StorageRef_t storageRef,
    const char* filePath
)
{
    // Check if the storage reference is valid
    TAF_ERROR_IF_RET_VAL(storageRef == nullptr, LE_BAD_PARAMETER,
                         "Invalid storage reference");

    // Lookup the client context using the storage reference
    tafMngdSecFileStorage_ClientCxt_t* clienCxtPtr =
        (tafMngdSecFileStorage_ClientCxt_t*)le_ref_Lookup(ClientRefMap, storageRef);

    // Check if the client context is valid
    TAF_ERROR_IF_RET_VAL(clienCxtPtr == nullptr, LE_NOT_FOUND, "Invalid client data reference");

    // Lookup the directory using the client context's directory reference
    tafMngdSecFileStorage_Dir_t* dirPtr =
        (tafMngdSecFileStorage_Dir_t*)le_ref_Lookup(DirRefMap, clienCxtPtr->dirRef);

    // Check if the directory reference is valid
    TAF_ERROR_IF_RET_VAL(dirPtr == nullptr, LE_NOT_FOUND, "Invalid secure directory reference");

    // Check if the file path is valid
    TAF_ERROR_IF_RET_VAL(filePath == nullptr || strlen(filePath) == 0, LE_BAD_PARAMETER,
                         "Invalid file path");

    char storageTargetFilePath[LIMIT_MAX_PATH_BYTES] = {0};

    // Construct the target file path within the storage directory
    snprintf(storageTargetFilePath, sizeof(storageTargetFilePath),
             "%s/%s", dirPtr->path, filePath);

    // Unlock the storage before performing the file operation
    LE_DEBUG("Unlocking FSC storage for directory: %s", dirPtr->path);
    taf_fsc_UnlockStorage(dirPtr->fscStorageRef);
    LE_DEBUG("Unlocking RFS storage for directory: %s", dirPtr->rfsPath);
    taf_fsc_UnlockStorage(dirPtr->rfs_fscStorageRef);

    // Set the backup storage path
    taf_rfs_SetBackupStorage(dirPtr->rfsPath);

    // Delete the file
    LE_DEBUG("Deleting file: %s", storageTargetFilePath);
    taf_rfs_Delete(storageTargetFilePath);

    // Lock the storage after the file operation
    LE_DEBUG("Locking FSC storage for directory: %s", dirPtr->path);
    taf_fsc_LockStorage(dirPtr->fscStorageRef);
    LE_DEBUG("Locking RFS storage for directory: %s", dirPtr->rfsPath);
    taf_fsc_LockStorage(dirPtr->rfs_fscStorageRef);

    LE_INFO("Successfully deleted file: %s", storageTargetFilePath);
    return LE_OK;
}

le_result_t tafMngdSecFileStorageSvc::GetBasePathImpl
(
    taf_mngdStorSecFile_StorageRef_t storageRef,
    char* basePath,
    size_t pathSize
)
{
    // Check if the storage reference is valid
    TAF_ERROR_IF_RET_VAL(storageRef == nullptr, LE_BAD_PARAMETER,
                         "Invalid storage reference");

    // Lookup the client context using the storage reference
    tafMngdSecFileStorage_ClientCxt_t* clienCxtPtr =
        (tafMngdSecFileStorage_ClientCxt_t*)le_ref_Lookup(ClientRefMap, storageRef);

    // Check if the client context is valid
    TAF_ERROR_IF_RET_VAL(clienCxtPtr == nullptr, LE_NOT_FOUND, "Invalid client data reference");

    // Lookup the directory using the client context's directory reference
    tafMngdSecFileStorage_Dir_t* dirPtr =
        (tafMngdSecFileStorage_Dir_t*)le_ref_Lookup(DirRefMap, clienCxtPtr->dirRef);

    // Check if the directory reference is valid
    TAF_ERROR_IF_RET_VAL(dirPtr == nullptr, LE_NOT_FOUND, "Invalid secure directory reference");

    // Check if the base path pointer is valid
    TAF_ERROR_IF_RET_VAL(basePath == nullptr, LE_BAD_PARAMETER,
                         "Invalid base path pointer");

    // Copy the base path from the directory path
    snprintf(basePath, pathSize, "%s", dirPtr->path);

    LE_INFO("Base path for storage reference: %s", basePath);
    return LE_OK;
}

