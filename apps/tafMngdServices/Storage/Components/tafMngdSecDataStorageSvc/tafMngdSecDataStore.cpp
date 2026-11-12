/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafMngdStorageSvc.hpp"
#include "limit.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace tafsvc;

namespace pt = boost::property_tree;

// Defines secure storage size for each client
#ifndef MSS_SEC_STORE_SIZE
#define MSS_SEC_STORE_SIZE (1024 * 8)
#endif

le_result_t tafMngdStorageSvc::PreCheckExtensionJson()
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

le_result_t tafMngdStorageSvc::ParseServiceJsonConfig(char* configPath)
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
        for (const auto &item : root.get_child("MSS Secure Data Storage.Configuration.StoragePath"))
        {
            const boost::property_tree::ptree &uPath = item.second;
            std::string basePath = uPath.get<std::string>("BasePath");
            if(CheckValidPath(basePath) != LE_OK){
                return LE_BAD_PARAMETER;
            }
            snprintf(secDataStorage, sizeof(secDataStorage), "%s", basePath.c_str());
            std::string backupPath = uPath.get<std::string>("BackupPath");
            if(CheckValidPath(backupPath) != LE_OK){
                return LE_BAD_PARAMETER;
            }
            LE_INFO("Base path is %s", secDataStorage);
            snprintf(secDataRfsStorage, sizeof(secDataRfsStorage), "%s", backupPath.c_str());
            LE_INFO("Backup path is %s", secDataRfsStorage);
        }
    }
    catch (const std::exception &e)
    {
        LE_WARN ("read_json exception: %s. Check validity of JSON.", e.what());
        return LE_FAULT;
    }
    return LE_OK;
}

void tafMngdStorageSvc::InitStorage
(
)
{
    le_result_t res = PreCheckExtensionJson();
    if(res != LE_OK){
        LE_FATAL("unable to parse service json");
    }

    res = CreateDirectory(secDataStorage);
    if(res != LE_OK){
        LE_FATAL("unable to create directory %s",secDataStorage);
    }

    res = CreateDirectory(secDataRfsStorage);
    if(res != LE_OK){
        LE_FATAL("unable to create directory %s",secDataRfsStorage);
    }

    // Create memory pools
    SecDataPool = le_mem_CreatePool("SecDataPool", sizeof(tafMngdStorage_SecData_t));

    // Create reference maps
    SecDataRefMap = le_ref_CreateMap("SecDataRefMap", SECURE_MAX_NUM_OF_DATA);

    // Create memory pools
    ClientDataPool = le_mem_CreatePool("ClientDataPool", sizeof(tafMngdStorage_ClientData_t));

    // Create reference maps
    ClientDataRefMap = le_ref_CreateMap("ClientDataRefMap", SECURE_MAX_NUM_OF_CLIENT_DATA);

    // Create memory pools
    DataChangeHandlerPool =
        le_mem_CreatePool("DataChangeHandlerPool", sizeof(tafMngdStorage_DataChangeHandler_t));

    // Create reference maps
    DataChangeHandlerRefMap =
        le_ref_CreateMap("DataChangeHandlerRefMap", SECURE_MAX_NUM_OF_DATA_HANDLER);

    // Release storage reference for disconnected session
    le_msg_AddServiceCloseHandler(taf_mngdStorSecData_GetServiceRef(),
                                    ReleaseDataRef,
                                    nullptr);

    // Create key event
    DataChangeEventId =
        le_event_CreateId("DataChangeEventId", sizeof(tafMngdStorage_DataChangeEvent_t));

    le_event_AddHandler("DataChangeEventhandler", DataChangeEventId, DataChangeEventHandler);

    // KeyStore is an optional dependency. Load the shared app data once it is connected.
    if (KsEnsureConnected() != LE_OK)
    {
        LE_WARN("KeyStore is not available, shared app data will be loaded after it is connected");
        StartKsRetryTimer();
    }
}

le_result_t tafMngdStorageSvc::GetStoragePath
(
    char* bufferPtr,
    size_t bufferSize
)
{
    char nsStr[LIMIT_MAX_PATH_BYTES] = {0};

    TAF_ERROR_IF_RET_VAL(
        GetClientNamespace(nsStr,
                            sizeof(nsStr)) != LE_OK,
                            LE_FAULT, "Cannot get namespace");

    snprintf(bufferPtr, bufferSize, "%s%s", secDataStorage, nsStr);

    LE_DEBUG("storage path is: %s", bufferPtr);

    return LE_OK;
}

le_result_t tafMngdStorageSvc::CreateStorageDir
(
)
{
    char storagePath[LIMIT_MAX_PATH_BYTES] = {0};
    struct stat sb;

    TAF_ERROR_IF_RET_VAL(GetStoragePath(storagePath,
                            sizeof(storagePath)) != LE_OK,
                            LE_FAULT, "Cannot get storage path");

    if(stat(storagePath, &sb) == -1)
    {
        LE_INFO("%s does not exist, create it.", storagePath);

        if(mkdir(storagePath, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", storagePath);
            return LE_FAULT;
        }
    }
    else if((sb.st_mode & S_IFMT) == S_IFDIR)
    {
        // assume the sub dir was created as well. do nothing
        LE_INFO("storage was found!");
    }
    else
    {
        // some other file objects. delete first
        LE_ERROR("Delete the file object, then create the storage");

        // try to delete it
        unlink(storagePath);

        // Create the directory
        if(mkdir(storagePath, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", storagePath);
            return LE_FAULT;
        }
    }

    return LE_OK;
}

le_result_t tafMngdStorageSvc::GetClientNamespace
(
    char* namespaceStr,
    size_t strSize
)
{
    pid_t pid;
    char appPath[LIMIT_MAX_PATH_BYTES] = {0};

    le_msg_SessionRef_t sessionRef = taf_mngdStorSecData_GetClientSessionRef();

    if (sessionRef != nullptr)
    {
        // Get the client pid.
        if (le_msg_GetClientProcessId(sessionRef, &pid) != LE_OK)
        {
            LE_ERROR("Failed to get the pid from client session reference.");
            return LE_FAULT;
        }
    }
    else
    {
        // Get self pid.
        pid = getpid();
    }

    // Retrieve the app name from the pid.
    if (le_appInfo_GetName(pid, appPath, sizeof(appPath)) == LE_OK)
    {
        snprintf(namespaceStr, strSize, "%s", appPath);
        LE_DEBUG("Get namespaceStr: %s.", namespaceStr);
    }
    else
    {
        return LE_FAULT;
    }

    return LE_OK;
}

uint32_t tafMngdStorageSvc::GetStorageFreeSpace
(
    const char* appNamePtr
)
{
    if(GetStorageUsedSize(appNamePtr) >= GetStorageMaxSize())
    {
        return 0;
    }
    return (GetStorageMaxSize() - GetStorageUsedSize(appNamePtr));
}

uint32_t tafMngdStorageSvc::GetStorageMaxSize
(
)
{
    uint32_t value = MSS_SEC_STORE_SIZE;

    return value;
}

uint32_t tafMngdStorageSvc::GetStorageUsedSize
(
    const char* appNamePtr
)
{
    char storagePath[LIMIT_MAX_PATH_BYTES] = {0};

    if(appNamePtr != nullptr)
    {
        snprintf(storagePath, sizeof(storagePath), "%s%s", secDataStorage, appNamePtr);
    }
    else
    {
        TAF_ERROR_IF_RET_VAL(
            GetStoragePath(storagePath,
                                sizeof(storagePath)) != LE_OK,
                                0, "Cannot get storage path");
    }

    return GetFilesSizeInDirectory(storagePath);
}

void tafMngdStorageSvc::LoadAllSharedAppData
(
    void
)
{
    struct dirent *namespace_entry;
    DIR *namespace_dir = opendir(secDataStorage);

    if (namespace_dir == nullptr)
    {
        LE_ERROR("Unable to open base directory");
        return;
    }

    while ((namespace_entry = readdir(namespace_dir)) != nullptr)
    {
        if (namespace_entry->d_type == DT_DIR) {
            // Skip "." and ".." directories
            if (strcmp(namespace_entry->d_name, ".") == 0 ||
                strcmp(namespace_entry->d_name, "..") == 0)
            {
                continue;
            }

            char namespace_path[1024];
            snprintf(namespace_path, sizeof(namespace_path), "%s/%s",
                        secDataStorage, namespace_entry->d_name);

            struct dirent *data_entry;
            DIR *data_dir = opendir(namespace_path);

            if (data_dir == nullptr)
            {
                LE_ERROR("Unable to open namespace directory");
                continue;
            }

            while ((data_entry = readdir(data_dir)) != nullptr)
            {
                // Check if the entry is a regular file
                if (data_entry->d_type == DT_REG)
                {
                    const char* filename = data_entry->d_name;
                    const char* temp_ext = SECURE_DATA_TEMP_EXTENSION;
                    size_t len = strlen(filename);
                    size_t ext_len = strlen(temp_ext);

                    // If the file has a .temp extension
                    if (len >= ext_len && strcmp(filename + len - ext_len, temp_ext) == 0)
                    {
                        // Construct the full path to the file for deletion
                        std::string full_path = std::string(namespace_path) + "/" + filename;

                        // Attempt to remove the .temp file
                        if (remove(full_path.c_str()) != 0)
                        {
                            // Print error message if deletion fails
                            LE_ERROR("Failed to remove temp file: %s", full_path.c_str());
                        }

                        // Skip further processing for this file
                        continue;
                    }

                    // Load secure data reference for non-temp files
                    LoadSecDataRefForDataSharedKey(namespace_entry->d_name, filename);
                }
            }

            closedir(data_dir);
        }
    }

    closedir(namespace_dir);
}

void tafMngdStorageSvc::LoadSecDataRefForDataSharedKey
(
    const char* namespacePtr,
    const char* dataNamePtr
)
{
    LE_INFO("CreateSecDataRefForDataSharedKey");

    char keyId[TAF_KS_MAX_KEY_ID_SIZE] = {0};
    snprintf(keyId, TAF_KS_MAX_KEY_ID_SIZE, "%s%s", namespacePtr, dataNamePtr);

    LE_INFO("Check keyId: %s", keyId);

    // Skip the data which already has a reference (e.g. created by a client before KeyStore
    // became available).
    tafMngdStorage_SecDataRef_t existingRef = nullptr;
    if (FindSecDataRef(dataNamePtr, namespacePtr, &existingRef) == LE_OK)
    {
        LE_INFO("Data '%s::%s' is already loaded", namespacePtr, dataNamePtr);
        return;
    }

    taf_ks_KeyRef_t keyRef;
    taf_ks_KeyUsage_t keyCap;
    taf_ks_AppCapMask_t appCap;

    TAF_ERROR_IF_RET_NIL(LE_OK != taf_ks_GetKey(keyId, &keyRef),
                            "Failed to get key with key id: %s", keyId);

    char appName[LIMIT_MAX_APP_NAME_LEN + 1] = { 0 };

    le_result_t res = taf_ks_GetFirstSharedApp(keyRef, appName, sizeof(appName), &keyCap, &appCap);

    if(res == LE_OK)
    {
        tafMngdStorage_SecData_t *dataPtr = nullptr;
        tafMngdStorage_SecDataRef_t secDataRef = nullptr;

        dataPtr = (tafMngdStorage_SecData_t*)le_mem_ForceAlloc(SecDataPool);

        memset((void*)dataPtr, 0, sizeof(tafMngdStorage_SecData_t));

        secDataRef =
            (tafMngdStorage_SecDataRef_t)le_ref_CreateRef(SecDataRefMap, dataPtr);

        snprintf(dataPtr->dataName, sizeof(dataPtr->dataName), "%s", dataNamePtr);
        snprintf(dataPtr->ownerAppName, sizeof(dataPtr->ownerAppName), "%s", namespacePtr);

        GetDataPath(dataPtr->ownerAppName,
                        dataPtr->dataName,
                        dataPtr->path,
                        LIMIT_MAX_PATH_BYTES);

        dataPtr->isInWritingProcess = false;
        dataPtr->isInReadingProcess = false;

        dataPtr->keyRef = keyRef;

        RefreshSharedAppInfo(secDataRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Retry intervals (ms) for connecting to KeyStore.
 */
//--------------------------------------------------------------------------------------------------
const uint32_t tafMngdStorageSvc::KS_RETRY_INTERVALS_MS[] =
{
    10000, 30000, 60000, 120000
};

const uint8_t tafMngdStorageSvc::KS_MAX_RETRIES =
    sizeof(tafMngdStorageSvc::KS_RETRY_INTERVALS_MS) /
    sizeof(tafMngdStorageSvc::KS_RETRY_INTERVALS_MS[0]);

//--------------------------------------------------------------------------------------------------
/**
 * Connects to KeyStore if it is not connected yet. On the first successful connection, the shared
 * app data saved in the storage is loaded.
 *
 * @return
 *  - LE_OK          KeyStore is connected.
 *  - LE_UNAVAILABLE KeyStore is not available.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdStorageSvc::KsEnsureConnected()
{
    if (ksConnected)
    {
        return LE_OK;
    }

    le_result_t res = taf_ks_TryConnectService();
    if (res != LE_OK)
    {
        LE_WARN("Failed to connect to KeyStore service, res = %d", res);
        return LE_UNAVAILABLE;
    }

    LE_INFO("Connected to KeyStore service");

    ksConnected = true;
    taf_ks_SetNonExitServerDisconnectHandler(KsDisconnectHandler, nullptr);

    if (!sharedDataLoaded)
    {
        sharedDataLoaded = true;
        LoadAllSharedAppData();
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Ensures KeyStore is connected and the key reference of the secure data is valid. The key
 * reference is dropped when KeyStore is disconnected, so it is retrieved again here.
 *
 * @return
 *  - LE_OK          KeyStore is ready to be used for the secure data.
 *  - LE_UNAVAILABLE KeyStore is not available.
 *  - LE_NOT_FOUND   The secure data reference is invalid.
 *  - LE_FAULT       Failed to get the key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdStorageSvc::PrepareKs
(
    tafMngdStorage_SecDataRef_t secDataRef
)
{
    if (KsEnsureConnected() != LE_OK)
    {
        return LE_UNAVAILABLE;
    }

    tafMngdStorage_SecData_t* dataPtr =
        (tafMngdStorage_SecData_t*)le_ref_Lookup(SecDataRefMap, secDataRef);

    TAF_ERROR_IF_RET_VAL(dataPtr == nullptr, LE_NOT_FOUND, "invalid secure data ref");

    if (dataPtr->keyRef == nullptr)
    {
        char keyId[TAF_KS_MAX_KEY_ID_SIZE] = {0};

        TAF_ERROR_IF_RET_VAL(GetDataKeyId(secDataRef, keyId, sizeof(keyId)) != LE_OK,
                                LE_FAULT, "cannot get data keyId");

        TAF_ERROR_IF_RET_VAL(taf_ks_GetKey(keyId, &(dataPtr->keyRef)) != LE_OK,
                                LE_FAULT, "Failed to get key with key id: %s", keyId);

        RefreshSharedAppInfo(secDataRef);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Drops all the KeyStore references and terminates the ongoing read/write operations, because the
 * crypto sessions and key references are invalid after KeyStore is disconnected.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdStorageSvc::ResetKsReferences()
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SecDataRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        tafMngdStorage_SecData_t* dataPtr = (tafMngdStorage_SecData_t*)le_ref_GetValue(iterRef);
        if (dataPtr == nullptr)
        {
            continue;
        }

        if (dataPtr->isInWritingProcess)
        {
            LE_WARN("Abort writing '%s::%s' since KeyStore is disconnected",
                    dataPtr->ownerAppName, dataPtr->dataName);

            if (dataPtr->writeOp.outputFd >= 0)
            {
                taf_rfs_Close(dataPtr->writeOp.outputFd);
            }
            DeleteTempFile(dataPtr->path);

            dataPtr->isInWritingProcess = false;
            dataPtr->writeOp.outputFd = -1;
            dataPtr->writeOp.sessionRef = nullptr;
            dataPtr->writeOp.clientSessionRef = nullptr;
        }

        if (dataPtr->isInReadingProcess)
        {
            LE_WARN("Abort reading '%s::%s' since KeyStore is disconnected",
                    dataPtr->ownerAppName, dataPtr->dataName);

            if (dataPtr->readOp.outputFd >= 0)
            {
                taf_rfs_Close(dataPtr->readOp.outputFd);
            }

            dataPtr->isInReadingProcess = false;
            dataPtr->readOp.outputFd = -1;
            dataPtr->readOp.sessionRef = nullptr;
            dataPtr->readOp.clientSessionRef = nullptr;
        }

        dataPtr->keyRef = nullptr;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for the unexpected disconnection of KeyStore.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdStorageSvc::KsDisconnectHandler(void* contextPtr)
{
    LE_WARN("KeyStore service disconnected");

    auto& mss = tafMngdStorageSvc::GetInstance();

    mss.ksConnected = false;
    mss.ResetKsReferences();
    mss.StartKsRetryTimer();
}

//--------------------------------------------------------------------------------------------------
/**
 * Starts the timer to reconnect to KeyStore.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdStorageSvc::StartKsRetryTimer()
{
    if (ksRetryTimer == nullptr)
    {
        ksRetryTimer = le_timer_Create("ksRetryTimer");
        if (ksRetryTimer == nullptr)
        {
            LE_ERROR("Failed to create KeyStore retry timer");
            return;
        }

        le_timer_SetHandler(ksRetryTimer, KsRetryHandler);
        le_timer_SetWakeup(ksRetryTimer, false);
    }
    else if (le_timer_IsRunning(ksRetryTimer))
    {
        return;
    }

    ksRetryIndex = 0;

    le_timer_SetMsInterval(ksRetryTimer, KS_RETRY_INTERVALS_MS[0]);
    le_timer_Start(ksRetryTimer);

    LE_INFO("KeyStore retry mechanism started");
}

//--------------------------------------------------------------------------------------------------
/**
 * Timer handler to reconnect to KeyStore. If all the retries fail, KeyStore is connected on demand
 * when a client requests an operation that needs it.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdStorageSvc::KsRetryHandler(le_timer_Ref_t timerRef)
{
    auto& mss = tafMngdStorageSvc::GetInstance();

    LE_INFO("KeyStore retry attempt %d/%d", mss.ksRetryIndex + 1, KS_MAX_RETRIES);

    if (mss.KsEnsureConnected() == LE_OK)
    {
        le_timer_Delete(timerRef);
        mss.ksRetryTimer = nullptr;
        mss.ksRetryIndex = 0;
        return;
    }

    mss.ksRetryIndex++;

    if (mss.ksRetryIndex < KS_MAX_RETRIES)
    {
        uint32_t interval = KS_RETRY_INTERVALS_MS[mss.ksRetryIndex];

        LE_WARN("KeyStore is still unavailable, retry in %u ms", interval);

        le_timer_SetMsInterval(timerRef, interval);
        le_timer_Start(timerRef);
    }
    else
    {
        LE_ERROR("Max KeyStore retries reached, will connect on demand");

        le_timer_Delete(timerRef);
        mss.ksRetryTimer = nullptr;
        mss.ksRetryIndex = 0;
    }
}
