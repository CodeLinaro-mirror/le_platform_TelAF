/*
*
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#include "tafMngdSecFileStorageSvc.hpp"

using namespace tafsvc;
#include <unistd.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <vector>
#include <cstring>

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <sys/stat.h>
#include <sys/vfs.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

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

inline le_result_t CheckValidPath(std::string& str){
    if(str.size() == 0){
        LE_ERROR("Storage Path len is 0");
        return LE_FAULT;
    }
    if(str[0] != '/'){
        str = '/'+str;
    }
    if(str[str.size()-1] != '/'){
        str = str+'/';
    }
    return LE_OK;
}

le_result_t tafMngdSecFileStorageSvc::ParseServiceJsonConfig(char* configPath)
{
    LE_INFO("Parsing %s", configPath);
    std::ifstream jfile(configPath);
    if (!jfile.is_open())
    {
        LE_WARN("Unable to open %s", configPath);
        return LE_FAULT;
    }

    pt::ptree root;
    try
    {
        pt::read_json(configPath, root);
        std::string product = root.get<std::string>("Product");
        if (product != "TelAF")
        {
            LE_WARN("Invalid JSON property value");
            return LE_FAULT;
        }
        std::string name = root.get<std::string>("Name");
        if (name != "MSS")
        {
            LE_WARN("Invalid JSON property value");
            return LE_FAULT;
        }
        std::string svcJsonVersion = root.get<std::string>("Version");
        LE_INFO("Version of Json is %s", svcJsonVersion.c_str());

        // Parse "MSS Secure File Storage" section
        auto mssSecureFileStorage = root.get_child("MSS Secure File Storage");
        if (!mssSecureFileStorage.empty())
        {
            auto configuration = mssSecureFileStorage.get_child("Configuration");
            if (!configuration.empty())
            {
                // Parse "StoragePath" section
                auto storagePath = configuration.get_child("StoragePath");
                if (!storagePath.empty())
                {
                    for (const auto& item : storagePath)
                    {
                        const boost::property_tree::ptree& uPath = item.second;
                        std::string basePath = uPath.get<std::string>("BasePath");
                        if(CheckValidPath(basePath) != LE_OK){
                            return LE_BAD_PARAMETER;
                        }
                        snprintf(secFileStorage,sizeof(secFileStorage),"%s",basePath.c_str());
                        std::string backupPath = uPath.get<std::string>("BackupPath");
                        if(CheckValidPath(backupPath) != LE_OK){
                            return LE_BAD_PARAMETER;
                        }
                        LE_INFO("Base path is %s",secFileStorage);
                        snprintf(secFileRfsStorage,sizeof(secFileRfsStorage),"%s",backupPath.c_str());
                        LE_INFO("Backup path is %s",secFileRfsStorage);
                    }
                }

                // Parse "Storages" section
                auto storages = configuration.get_child("Storages");
                if (!storages.empty())
                {
                    for (const auto& item : storages)
                    {
                        tafMngdSecFileStorage_StorageCfg_t storage;
                        std::string storageName = item.second.get<std::string>("StorageName");
                        snprintf(storage.StorageName, sizeof(storage.StorageName), "%s", storageName.c_str());

                        // Parse "AccessibleApps" section with "Read" and "Write" permissions
                        auto accessibleApps = item.second.get_child("AccessibleApps");
                        if (!accessibleApps.empty())
                        {
                            auto readApps = accessibleApps.get_child("Read");
                            if (!readApps.empty())
                            {
                                for (const auto& app : readApps)
                                {
                                    std::string appName = app.second.get_value<std::string>();
                                    char* appCStr = new char[LIMIT_MAX_APP_NAME_LEN];
                                    snprintf(appCStr, LIMIT_MAX_APP_NAME_LEN, "%s", appName.c_str());
                                    storage.ReadAccessibleApps.push_back(appCStr);
                                }
                            }

                            auto writeApps = accessibleApps.get_child("Write");
                            if (!writeApps.empty())
                            {
                                for (const auto& app : writeApps)
                                {
                                    std::string appName = app.second.get_value<std::string>();
                                    char* appCStr = new char[LIMIT_MAX_APP_NAME_LEN];
                                    snprintf(appCStr, LIMIT_MAX_APP_NAME_LEN, "%s", appName.c_str());
                                    storage.WriteAccessibleApps.push_back(appCStr);
                                }
                            }
                        }

                        storageAccessCfg.push_back(storage);
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        LE_WARN("read_json exception: %s. Check validity of JSON.", e.what());
        return LE_FAULT;
    }
    return LE_OK;
}

/**
 * Check if an application has read access to a storage.
 */
bool tafMngdSecFileStorageSvc::IsReadable(const char* storageName, const char* appName)
{
    for (const auto& storage : storageAccessCfg)
    {
        if (std::strcmp(storage.StorageName, storageName) == 0)
        {
            for (const auto& app : storage.ReadAccessibleApps)
            {
                if (std::strcmp(app, appName) == 0)
                {
                    LE_INFO("%s has read access to storage: %s", appName, storageName);
                    return true;
                }
            }
        }
    }
    return false;
}

/**
 * Check if an application has write access to a storage.
 */
bool tafMngdSecFileStorageSvc::IsWritable(const char* storageName, const char* appName)
{
    for (const auto& storage : storageAccessCfg)
    {
        if (std::strcmp(storage.StorageName, storageName) == 0)
        {
            for (const auto& app : storage.WriteAccessibleApps)
            {
                if (std::strcmp(app, appName) == 0)
                {
                    LE_INFO("%s has write access to storage: %s", appName, storageName);
                    return true;
                }
            }
        }
    }
    return false;
}

bool tafMngdSecFileStorageSvc::IsAppAccessible(const char* storageName, const char* appName)
{
    return IsReadable(storageName, appName) || IsWritable(storageName, appName);
}

bool tafMngdSecFileStorageSvc::IsServiceStorage(const char* storageName)
{
    for (const auto& storage : storageAccessCfg)
    {
        if (std::strcmp(storage.StorageName, storageName) == 0)
        {
            LE_INFO("%s is a service-created storage", storageName);
            return true;
        }
    }
    return false;
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

    // Set session close handlers
    le_msg_AddServiceCloseHandler(taf_mngdStorSecFile_GetServiceRef(), SessionCloseHandler, nullptr);

    taf_rfs_Init(true, nullptr);

    taf_rfs_SetBackupStorage(secFileRfsStorage);

    CreateServiceStorages();
}

void tafMngdSecFileStorageSvc::SessionCloseHandler
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    LE_DEBUG("SessionCloseHandler for session (%p)", sessionRef);

    auto &mss = tafMngdSecFileStorageSvc::GetInstance();

    le_ref_IterRef_t iterRef = le_ref_GetIterator(mss.ClientRefMap);

    // Scan all the data nodes
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        tafMngdSecFileStorage_ClientCxt_t* clientPtr =
            (tafMngdSecFileStorage_ClientCxt_t*)le_ref_GetValue(iterRef);

        if (clientPtr == nullptr)
        {
            LE_ERROR("clientPtr is nullptr");
            return;
        }

        // Find the node that matches the current session
        if (clientPtr->clientSessionRef == sessionRef)
        {
            // Lock the storage
            tafMngdSecFileStorage_Dir_t* dirPtr =
                (tafMngdSecFileStorage_Dir_t*)le_ref_Lookup(mss.DirRefMap, clientPtr->dirRef);

            // Check if the directory reference is valid
            if (dirPtr == nullptr)
            {
                LE_ERROR( "Invalid secure directory reference");
            }

            if(clientPtr->lockState == false)
            {
                dirPtr->userCount--;
                if(dirPtr->userCount == 0)
                {
                    LE_DEBUG("Locking FSC storage for directory: %s", dirPtr->path);
                    taf_fsc_LockStorage(dirPtr->fscStorageRef);
                    LE_DEBUG("Locking RFS storage for directory: %s", dirPtr->rfsPath);
                    taf_fsc_LockStorage(dirPtr->rfs_fscStorageRef);
                }
            }

            // Release the client context
            le_ref_DeleteRef(mss.ClientRefMap, clientPtr->storageRef);
            le_mem_Release(clientPtr);
        }
    }
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
    const char* storageNamePtr,
    bool internal
)
{
    LE_INFO("CreateDirRef for '%s'", storageNamePtr);

    tafMngdSecFileStorage_Dir_t *dirPtr = nullptr;
    tafMngdSecFileStorage_DirRef_t dirRef = nullptr;
    char myAppName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };
    le_result_t res = LE_OK;

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
    if(internal == true)
    {
        snprintf(myAppName, sizeof(myAppName), "%s", SECFILE_CREATOR_NAME);
    }
    else
    {
        if (LE_OK != GetAppNameBySessionRef(taf_mngdStorSecFile_GetClientSessionRef(),
                                        myAppName, sizeof(myAppName)))
        {
            LE_ERROR("Failed to get client appName.");
            goto cleanup;
        }
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
    dirPtr->userCount = 0;
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

//--------------------------------------------------------------------------------------------------
/**
 * Calculates file path SHA1
 */
//--------------------------------------------------------------------------------------------------
static void CalculateInputStrSHA1(const char* inputPtr, char* outputHashPtr)
{
    LE_DEBUG("%s", __FUNCTION__);

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)inputPtr, strlen(inputPtr), hash);
    // transfer SHA1 to a hex string
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        snprintf(outputHashPtr + (i * 2), sizeof(outputHashPtr), "%02x", hash[i]);
    }
    outputHashPtr[SHA_DIGEST_LENGTH * 2] = '\0';
}

le_result_t tafMngdSecFileStorageSvc::SetStorageCreator
(
    const char* storageNamePtr,
    const char* creatorAppPtr
)
{
    TAF_ERROR_IF_RET_VAL(storageNamePtr == nullptr,
                         LE_BAD_PARAMETER,
                         "Invalid storage string");

    TAF_ERROR_IF_RET_VAL(creatorAppPtr == nullptr,
                         LE_BAD_PARAMETER,
                         "Invalid creatorApp string");

    LE_INFO("Set creator for storage %s", storageNamePtr);

    char sha1Hash[SHA_DIGEST_LENGTH * 2 + 1] = {0};
    char dataName[TAF_MNGDSTORSECDATA_MAX_DATA_LABEL_BYTES] = {0};

    CalculateInputStrSHA1(storageNamePtr, sha1Hash);

    TAF_ERROR_IF_RET_VAL(strlen(sha1Hash) == 0, LE_FAULT, "Get SHA1 failed");

    snprintf(dataName, sizeof(dataName), "%s", sha1Hash);

    le_result_t res = LE_OK;

    res = taf_mngdStorSecData_CreateData(dataName);

    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Create data failed");

    taf_mngdStorSecData_DataRef_t dataRef = taf_mngdStorSecData_GetDataRef(dataName);

    TAF_ERROR_IF_RET_VAL(dataRef == nullptr, LE_FAULT, "Invalid data reference");

    res = taf_mngdStorSecData_WriteDataStart(dataRef);

    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Write data failed");

    res = taf_mngdStorSecData_WriteDataChunk(dataRef,
                                                (uint8_t*)creatorAppPtr,
                                                strlen(creatorAppPtr) + 1);

    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Write data failed");

    res = taf_mngdStorSecData_WriteDataEnd(dataRef);

    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Write data failed");

    return LE_OK;
}

le_result_t tafMngdSecFileStorageSvc::CheckStorageCreator
(
    const char* storageNamePtr,
    const char* checkAppPtr
)
{
    TAF_ERROR_IF_RET_VAL(storageNamePtr == nullptr,
                         LE_BAD_PARAMETER,
                         "Invalid storage string");

    TAF_ERROR_IF_RET_VAL(checkAppPtr == nullptr,
                         LE_BAD_PARAMETER,
                         "Invalid checkApp string");

    char sha1Hash[SHA_DIGEST_LENGTH * 2 + 1] = {0};
    char dataName[TAF_MNGDSTORSECDATA_MAX_DATA_LABEL_BYTES] = {0};

    CalculateInputStrSHA1(storageNamePtr, sha1Hash);

    TAF_ERROR_IF_RET_VAL(strlen(sha1Hash) == 0, LE_FAULT, "Get SHA1 failed");

    snprintf(dataName, sizeof(dataName), "%s", sha1Hash);

    taf_mngdStorSecData_DataRef_t dataRef = taf_mngdStorSecData_GetDataRef(dataName);
    TAF_ERROR_IF_RET_VAL(dataRef == nullptr, LE_FAULT, "Invalid data reference");

    uint8_t buffer[TAF_MNGDSTORSECDATA_MAX_DATA_LABEL_BYTES] = {0};
    size_t readSize = sizeof(buffer);

    le_result_t res = taf_mngdStorSecData_ReadDataFirstChunk(dataRef, buffer, &readSize);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Read data failed");

    if (strcmp((char*)buffer, checkAppPtr) == 0)
    {
        LE_INFO("%s is the creator of storage: %s", checkAppPtr, storageNamePtr);
        return LE_OK; // The app matches
    }
    else
    {
        return LE_NOT_FOUND; // The app does not match
    }
}

le_result_t tafMngdSecFileStorageSvc::ClearStorageCreator
(
    const char* storageNamePtr,
    const char* checkAppPtr
)
{
    TAF_ERROR_IF_RET_VAL(storageNamePtr == nullptr,
                         LE_BAD_PARAMETER,
                         "Invalid storage string");

    TAF_ERROR_IF_RET_VAL(checkAppPtr == nullptr,
                         LE_BAD_PARAMETER,
                         "Invalid checkApp string");

    char sha1Hash[SHA_DIGEST_LENGTH * 2 + 1] = {0};
    char dataName[TAF_MNGDSTORSECDATA_MAX_DATA_LABEL_BYTES] = {0};

    CalculateInputStrSHA1(storageNamePtr, sha1Hash);

    TAF_ERROR_IF_RET_VAL(strlen(sha1Hash) == 0, LE_FAULT, "Get SHA1 failed");

    snprintf(dataName, sizeof(dataName), "%s", sha1Hash);

    taf_mngdStorSecData_DataRef_t dataRef = taf_mngdStorSecData_GetDataRef(dataName);
    TAF_ERROR_IF_RET_VAL(dataRef == nullptr, LE_FAULT, "Invalid data reference");

    le_result_t res = taf_mngdStorSecData_DeleteData(dataRef);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Delete data failed");

    return LE_OK;
}

le_result_t tafMngdSecFileStorageSvc::CreateStorageRefImpl
(
    const char* storageNamePtr,
    taf_mngdStorSecFile_ManagedCapMask_t capMask,
    bool internal
)
{
    // Validate the storage name
    TAF_ERROR_IF_RET_VAL(CheckValidPosixFileName(storageNamePtr) != LE_OK,
                         LE_BAD_PARAMETER,
                         "Invalid storage string");

    char dirPath[LIMIT_MAX_PATH_BYTES] = {0};
    char myAppName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };

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

    // Set up creator for the new storage
    if(internal == true)
    {
        snprintf(myAppName, sizeof(myAppName), "%s", SECFILE_CREATOR_NAME);
    }
    else
    {
        if (LE_OK != GetAppNameBySessionRef(taf_mngdStorSecFile_GetClientSessionRef(),
                                        myAppName, sizeof(myAppName)))
        {
            LE_ERROR("Failed to get client appName.");
            return LE_FAULT;
        }
    }

    if(SetStorageCreator(storageNamePtr, myAppName) != LE_OK)
    {
        LE_ERROR("Failed to set creator for storage %s.", storageNamePtr);
        return LE_FAULT;
    }

    tafMngdSecFileStorage_DirRef_t dirRef = nullptr;

    // Find or create the directory reference
    if (FindDirRef(storageNamePtr, &dirRef) == LE_NOT_FOUND)
    {
        dirRef = CreateDirRef(storageNamePtr, internal);
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
    char myAppName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };
    bool isCreator = false;

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

    if (LE_OK != GetAppNameBySessionRef(taf_mngdStorSecFile_GetClientSessionRef(),
                                        myAppName, sizeof(myAppName)))
    {
        LE_ERROR("Failed to get client appName.");
        return nullptr;
    }

    if(CheckStorageCreator(storageNamePtr, myAppName) == LE_OK)
    {
        LE_INFO("Calling app is the creator");
        isCreator = true;
    }
    else
    {
        LE_INFO("Calling app is not the creator, check if it is accessible");
        if(IsAppAccessible(storageNamePtr, myAppName) == false)
        {
            LE_ERROR("Calling app is not in the access list");
            return nullptr;
        }
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

        clientCtxPtr->lockState= true;

        clientCtxPtr->IsReadable = false;

        clientCtxPtr->IsWritable = false;

        if(IsReadable(storageNamePtr, myAppName) == true)
        {
            LE_DEBUG("This client has read access");
            clientCtxPtr->IsReadable = true;
        }

        if(IsWritable(storageNamePtr, myAppName) == true)
        {
            LE_DEBUG("This client has write access");
            clientCtxPtr->IsWritable = true;
        }

        if(isCreator == true)
        {
            clientCtxPtr->IsCreator = true;
            clientCtxPtr->IsReadable = true;
            clientCtxPtr->IsWritable = true;
        }

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
        clientCtxPtr->dirRef = CreateDirRef(storageNamePtr, false);
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

    if(dirPtr->userCount == 0)
    {
        // Unlock the RFS storage
        LE_DEBUG("Unlocking RFS storage for directory: %s", dirPtr->rfsPath);
        taf_fsc_UnlockStorage(dirPtr->rfs_fscStorageRef);

        // Unlock the FSC storage
        LE_DEBUG("Unlocking FSC storage for directory: %s", dirPtr->path);
        taf_fsc_UnlockStorage(dirPtr->fscStorageRef);
    }
    dirPtr->userCount++;
    clienCxtPtr->lockState = false;
    LE_DEBUG("Unlocking  with userCount and lockstate: %d,%d", dirPtr->userCount,clienCxtPtr->lockState);
    return LE_OK;
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

    if(clienCxtPtr->lockState  == true)
    {
        LE_ERROR("Locking Failed: %s", dirPtr->rfsPath);
        return LE_NOT_PERMITTED;
    }

    if(dirPtr->userCount != 0)
    {
        dirPtr->userCount --;
    }
    else
    {
        LE_ERROR("Locking Failed: %s", dirPtr->rfsPath);
        return LE_NOT_PERMITTED;
    }

    clienCxtPtr->lockState = true;
    if(dirPtr->userCount == 0)
    {
        // Lock the RFS storage
        LE_DEBUG("Locking RFS storage for directory: %s", dirPtr->rfsPath);
        taf_fsc_LockStorage(dirPtr->rfs_fscStorageRef);

        // Lock the FSC storage
        LE_DEBUG("Locking FSC storage for directory: %s", dirPtr->path);
        taf_fsc_LockStorage(dirPtr->fscStorageRef);
    }
    LE_DEBUG("Locking with userCount lockState : %d %d", dirPtr->userCount,clienCxtPtr->lockState);
    return LE_OK;
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

    // Check client access
    TAF_ERROR_IF_RET_VAL(clienCxtPtr->IsWritable != true, LE_NOT_PERMITTED,
                            "Client doesn't hava write access");

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

    size_t availableSize = GetAvailableSpace(dirPtr->path);
    size_t fileSize = GetFileSize(sourceFilePathPtr);

    LE_INFO("Storage size %" PRIuS ", file size %" PRIuS, availableSize, fileSize);

    if(availableSize < fileSize)
    {
        LE_ERROR("Storage size %" PRIuS " is not enough for the file size %" PRIuS,
                    availableSize, fileSize);

        return LE_NO_MEMORY;
    }

    char storageTargetFilePath[LIMIT_MAX_PATH_BYTES] = {0};
    char tmpStorageTargetFilePath[LIMIT_MAX_PATH_BYTES] = {0};

    // Construct the target file path within the storage directory
    snprintf(storageTargetFilePath, sizeof(storageTargetFilePath),
             "%s/%s", dirPtr->path, targetFilePathPtr);

    // Construct the target file path within the storage directory
    snprintf(tmpStorageTargetFilePath, sizeof(tmpStorageTargetFilePath),
             "%s/%s%s", dirPtr->path, targetFilePathPtr, SECFILE_TMP_FILE_NAME_EXTENSION);

    // Unlock the storage before performing the file operation
    LE_DEBUG("Unlocking FSC storage for directory: %s", dirPtr->path);
    taf_fsc_UnlockStorage(dirPtr->fscStorageRef);
    LE_DEBUG("Unlocking RFS storage for directory: %s", dirPtr->rfsPath);
    taf_fsc_UnlockStorage(dirPtr->rfs_fscStorageRef);

    // Set the backup storage path
    taf_rfs_SetBackupStorage(dirPtr->rfsPath);

    // For atomic operation, copy the file from the source to the tmp file
    if(taf_rfs_Copy(sourceFilePathPtr, tmpStorageTargetFilePath) != 0)
    {
        LE_ERROR("Failed to import file from %s to %s", sourceFilePathPtr, storageTargetFilePath);
        goto cleanup;
    }

    // Rename the file if the copy is successful
    if(taf_rfs_Rename(tmpStorageTargetFilePath, storageTargetFilePath) != 0)
    {
        LE_ERROR("Failed to rename file from %s to %s", tmpStorageTargetFilePath, storageTargetFilePath);
        goto cleanup;
    }

    // Delete the tmp file
    taf_rfs_Delete(tmpStorageTargetFilePath);

    // Lock the storage after the file operation
    LE_DEBUG("Locking FSC storage for directory: %s", dirPtr->path);
    taf_fsc_LockStorage(dirPtr->fscStorageRef);
    LE_DEBUG("Locking RFS storage for directory: %s", dirPtr->rfsPath);
    taf_fsc_LockStorage(dirPtr->rfs_fscStorageRef);

    LE_INFO("Successfully imported file from %s to %s", sourceFilePathPtr, storageTargetFilePath);
    return LE_OK;

cleanup:

    // Delete the tmp file
    taf_rfs_Delete(tmpStorageTargetFilePath);

    // Lock the storage after the file operation
    LE_DEBUG("Locking FSC storage for directory: %s", dirPtr->path);
    taf_fsc_LockStorage(dirPtr->fscStorageRef);
    LE_DEBUG("Locking RFS storage for directory: %s", dirPtr->rfsPath);
    taf_fsc_LockStorage(dirPtr->rfs_fscStorageRef);

    return LE_FAULT;
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

    // Check client access
    TAF_ERROR_IF_RET_VAL(clienCxtPtr->IsReadable != true, LE_NOT_PERMITTED,
                            "Client doesn't hava read access");

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

    // Check client access
    TAF_ERROR_IF_RET_VAL(clienCxtPtr->IsWritable != true, LE_NOT_PERMITTED,
        "Client doesn't hava write access");

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

le_result_t tafMngdSecFileStorageSvc::DeleteStorageImpl
(
    taf_mngdStorSecFile_StorageRef_t storageRef
)
{
    le_result_t result;

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

    // Check if the directory is empty
    if (!IsDirectoryEmpty(dirPtr->path))
    {
        LE_ERROR("The directory '%s' is not empty", dirPtr->path);
        return LE_NOT_PERMITTED;
    }

    if(IsServiceStorage(dirPtr->storageName) == true)
    {
        LE_ERROR("The service storage '%s' cannot be deleted", dirPtr->storageName);
        return LE_NOT_PERMITTED;
    }

    result = taf_fsc_DeleteStorage(dirPtr->rfs_fscStorageRef);
    if(result == LE_OK)
    {
        LE_INFO("Successfully deleted Storage: %s", dirPtr->rfsPath);
    }
    else
    {
        LE_ERROR("Failed to delete rfsstorage refernce");
        return LE_FAULT;
    }
    result = taf_fsc_DeleteStorage(dirPtr->fscStorageRef);
    if(result == LE_OK)
    {
        LE_INFO("Successfully deleted Storage: %s", dirPtr->path);
    }
    else
    {
        LE_ERROR("Failed to delete fscstorage refernce");
        return LE_FAULT;
    }

    result = ClearStorageCreator(dirPtr->storageName, dirPtr->masterAppName);
    if(result == LE_OK)
    {
        LE_INFO("Successfully clear storage creator");
    }
    else
    {
        LE_ERROR("Failed to clear storage creator");
        return LE_FAULT;
    }

    // Free the storage object and reference
    le_ref_DeleteRef(DirRefMap, clienCxtPtr->dirRef);
    le_mem_Release(dirPtr);
    le_ref_DeleteRef(ClientRefMap, storageRef);
    le_mem_Release(clienCxtPtr);
    return result;

}

void tafMngdSecFileStorageSvc::CreateServiceStorages()
{
    taf_mngdStorSecFile_ManagedCapMask_t capMask = TAF_MNGDSTORSECFILE_CAP_RELIABILITY;
    bool internal = true;
    le_result_t res = LE_OK;

    for (const auto& storage : storageAccessCfg)
    {
        res = CreateStorageRefImpl(storage.StorageName, capMask, internal);
        if (res != LE_OK)
        {
            LE_WARN("Failed to create storage: %s", storage.StorageName);
        }
        LE_INFO("Successfully created storage: %s", storage.StorageName);
    }
}

size_t tafMngdSecFileStorageSvc::GetFileSize
(
    const char *filePath
)
{
    struct stat fileStat;

    // Get file statistics
    if (stat(filePath, &fileStat) == -1)
    {
        LE_ERROR("Failed to get file status for %s", filePath);
        return 0;
    }

    // Check if the entry is a regular file
    if (S_ISREG(fileStat.st_mode))
    {
        return (size_t)fileStat.st_size;
    }
    else
    {
        LE_ERROR("%s is not a regular file", filePath);
        return 0;
    }
}

size_t tafMngdSecFileStorageSvc::GetAvailableSpace
(
    const char *path
)
{
    struct statfs stat;
    if (statfs(path, &stat) == 0)
    {
        return (size_t)stat.f_bsize * stat.f_bavail;
    }
    return (size_t)-1;
}
