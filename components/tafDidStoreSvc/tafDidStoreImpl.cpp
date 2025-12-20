/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDidStore.hpp"
#include <setjmp.h>

namespace pt = boost::property_tree;
using namespace tafsvc;

#define MAX_NUM_OF_ATTEMPTS   10
#define RETRY_TIMER_INTERVAL  3000
#define TIMER_SAFECALL 5
DECLARE_SAFE_CALL();

le_sem_Ref_t read_semaphore = NULL;
le_sem_Ref_t write_semaphore = NULL;

// Diag RDBI/WDBI
taf_diagDataID_ServiceRef_t taf_diagDidStore::DiagDataIDSvcRef = NULL;
taf_diagDataID_RxReadDIDMsgHandlerRef_t taf_diagDidStore::DiagReadDataIDMsgRef = NULL;
taf_diagDataID_RxWriteDIDMsgHandlerRef_t taf_diagDidStore::DiagWriteDataIDMsgRef = NULL;

le_dls_List_t didStorageNotifyList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of TelAF DID storage server.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDidStore &taf_diagDidStore::GetInstance
(
)
{
    static taf_diagDidStore instance;

    return instance;
}


//-------------------------------------------------------------------------------------------------
/**
 * Create a reference for the service, or get the reference of a service if the reference already
 * exist.
 */
//-------------------------------------------------------------------------------------------------
taf_diagDidStore_ServiceRef_t taf_diagDidStore::GetService
(
)
{
    LE_DEBUG("Gets the DataIDStor service!");
    le_msg_SessionRef_t sessionRef = taf_diagDidStore_GetClientSessionRef();
    if (!sessionRef)
    {
        LE_ERROR("Client session reference is NULL.");
        return NULL;
    }

    taf_DidStore_t* servicePtr = GetServiceObj(sessionRef);

    if (!servicePtr)
    {
        // Allocate and initialize the service object
        servicePtr = (taf_DidStore_t*)le_mem_ForceAlloc(SvcPool);
        memset(servicePtr, 0, sizeof(taf_DidStore_t));
        servicePtr->sessionRef = sessionRef;

        // Add to the map
        servicePtr->svcRef = (taf_diagDidStore_ServiceRef_t)le_ref_CreateRef(SvcRefMap, servicePtr);

        LE_DEBUG("Created svcRef %p for client session %p", servicePtr->svcRef,
                servicePtr->sessionRef);
    }

    return servicePtr->svcRef;
}


//-------------------------------------------------------------------------------------------------
/**
 * Get the service instacnce object, if the service reference already created and return the
 * object pointer.
 */
//-------------------------------------------------------------------------------------------------
taf_DidStore_t* taf_diagDidStore::GetServiceObj
(
    le_msg_SessionRef_t sessionRef
)
{
    auto &didStore = taf_diagDidStore::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(didStore.SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_DidStore_t* servicePtr = (taf_DidStore_t *)le_ref_GetValue(iterRef);
        if ((servicePtr != NULL) && (servicePtr->sessionRef == sessionRef))
        {
            LE_DEBUG("Found existing svcRef %p for client session %p", servicePtr->svcRef,
                    servicePtr->sessionRef);
            return servicePtr;
        }
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * Parse json configuration.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_diagDidStore::ParseDidStoreJsonConfig
(
    const char* configPathPtr
)
{
    LE_DEBUG("ParseDidStoreJsonConfig");

    if (configPathPtr == NULL)
    {
        LE_ERROR("configPathPtr is null!");
    }

    std::ifstream jfile(configPathPtr);
    if (!jfile.is_open())
    {
        LE_WARN("Unable to open %s", configPathPtr);
        return LE_FAULT;
    }

    // Read json config file
    try{
        // Create a root
        pt::ptree root;

        // Load the json file in this ptree
        pt::read_json(configPathPtr, root);

        for (const auto& item : root.get_child("did_access"))
        {
            tafDidStore_Config_t didConfigStore;

            // App name
            std::string appName = item.second.get<std::string>("AppName");
            snprintf(didConfigStore.AppName, LIMIT_MAX_APP_NAME_LEN, "%s", appName.c_str());

            // Write DID accessible list
            for (const auto& writeAccessDID : item.second.get_child("WriteAccessibleDID"))
            {
                std::string writableDID = writeAccessDID.second.get_value<std::string>();
                uint16_t writeDID;
                writeDID = std::stoul(writableDID, nullptr, 16);
                didConfigStore.WriteAccessDID.push_back(writeDID);
            }

            // Read DID accessable list
            for (const auto& readAccessDID : item.second.get_child("ReadAccessibleDID"))
            {
                std::string readableDID = readAccessDID.second.get_value<std::string>();
                uint16_t readDID;
                readDID = std::stoul(readableDID, nullptr, 16);
                didConfigStore.ReadAccessDID.push_back(readDID);
            }

            dataIdStoreAccessCfg.push_back(didConfigStore);
        }
    }
    catch (std::exception const& exp)
    {
        LE_WARN("Exception caught while reading json file: %s", exp.what());
        return LE_FAULT;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the application name of the process with the specified client session reference.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_diagDidStore::GetAppNameBySessionRef
(
    le_msg_SessionRef_t clientSessionRef,
    char *appNameStr,
    size_t appNameSize
)
{
    LE_DEBUG("GetAppNameBySessionRef");

    pid_t pid;
    const char* namePtr = NULL;
    char procPath[LIMIT_MAX_PATH_BYTES] = {0};
    char appPath[LIMIT_MAX_PATH_BYTES] = {0};

    // Parameter check.
    if ((clientSessionRef == NULL) || (appNameStr == NULL) || (appNameSize == 0))
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

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Check the read DID is accessible by the application.
 */
//-------------------------------------------------------------------------------------------------
bool taf_diagDidStore::IsReadAppAccessible
(
    uint16_t dataId,
    const char* appName
)
{
    for (const auto& didStore : dataIdStoreAccessCfg)
    {
        if (std::strcmp(didStore.AppName, appName) == 0)
        {
            for (const auto& readDID : didStore.ReadAccessDID)
            {
                if (readDID == dataId)
                {
                    LE_DEBUG("dataId %x is in the access list of app %s", dataId, appName);
                    return true;
                }
            }
        }
    }

    LE_DEBUG("dataId %x is not in the access list of app %s", dataId, appName);
    return false;
}

//-------------------------------------------------------------------------------------------------
/**
 * Check the write DID is accessible by the application.
 */
//-------------------------------------------------------------------------------------------------
bool taf_diagDidStore::IsWriteAppAccessible
(
    uint16_t dataId,
    const char* appName
)
{
    for (const auto& didStore : dataIdStoreAccessCfg)
    {
        if (std::strcmp(didStore.AppName, appName) == 0)
        {
            for (const auto& writeDID : didStore.WriteAccessDID)
            {
                if (writeDID == dataId)
                {
                    LE_DEBUG("dataId %x is in the access list of app %s", dataId, appName);
                    return true;
                }
            }
        }
    }

    LE_DEBUG("dataId %x is not in the access list of app %s", dataId, appName);
    return false;
}

//-------------------------------------------------------------------------------------------------
/**
 * ReadDID callback.
 */
//-------------------------------------------------------------------------------------------------
void taf_diagDidStore::didReadCb
(
    uint16_t dataID,
    uint8_t* dataRecPtr,
    size_t len,
    uint8_t result,
    void* contextPtr
)
{
    auto &didStore = taf_diagDidStore::GetInstance();

    // Store the read result in the client-specific structure
    didStore.readStrg.readDID = dataID;
    ReadWriteRequest_t* requestPtr = NULL;

    // Check the result.
    if (result != 0)
    {
        LE_ERROR("Read DID error code : %x", result);
        if (result != TAF_CONDITION_NOT_CORRECT && result != TAF_REQ_OUT_OF_RANGE)
        {
            didStore.readStrg.result = TAF_CONDITION_NOT_CORRECT;
        }
        else
        {
            didStore.readStrg.result = result;
        }
        goto semPostOut;
    }

    if (contextPtr == NULL)
    {
        LE_ERROR("contextPtr is NULL");
        didStore.readStrg.result = TAF_CONDITION_NOT_CORRECT;
        goto semPostOut;
    }

    requestPtr =(ReadWriteRequest_t*)le_ref_Lookup(didStore.ReadDIDRefMap, contextPtr);
    if (requestPtr == NULL)
    {
        LE_ERROR("Invalid requestPtr");
        didStore.readStrg.result = TAF_CONDITION_NOT_CORRECT;
        goto semPostOut;
    }

    if (dataRecPtr == NULL || len == 0 || len > sizeof(didStore.readStrg.didData))
    {
        LE_ERROR("Invalid read response: dataID %u, len %zu", dataID, len);
        didStore.readStrg.result = TAF_CONDITION_NOT_CORRECT;
        goto semPostOut;
    }

    // Copy data to the client-specific structure
    memcpy(didStore.readStrg.didData, dataRecPtr, len);
    didStore.readStrg.didDataLen = len;
    didStore.readStrg.result = result;
    goto semPostOut;

semPostOut:
    // Signal the client-specific semaphore
    if (read_semaphore != NULL)
    {
        le_sem_Post(read_semaphore);
    }
    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Read the data record for data ID.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_diagDidStore::Read
(
    uint16_t dataId,
    uint8_t* dataRecordPtr,
    size_t* dataRecordSizePtr
)
{
    // Check if dataRecordPtr or dataRecordSizePtr is NULL
    if(dataRecordPtr == NULL || dataRecordSizePtr == NULL)
    {
        LE_ERROR("Invalid parameter: dataRecordPtr or dataRecordSizePtr is NULL");
        return LE_FAULT;
    }

    // Create semaphore.
    if (read_semaphore == NULL)
    {
        read_semaphore = le_sem_Create("ReadDID Semaphore", 0);
    }

    // Prepare a read request
    ReadWriteRequest_t* requestPtr = (ReadWriteRequest_t*)le_mem_ForceAlloc(ReadRequestPool);
    if (requestPtr == NULL)
    {
        LE_ERROR("Failed to allocate memory for read request.");
        return LE_FAULT;
    }

    // Initialize read request structure
    requestPtr->request = READ_REQUEST_PI;
    requestPtr->dataId = dataId;
    requestPtr->dataRecordPtr = dataRecordPtr;
    requestPtr->dataRecordSizePtr = dataRecordSizePtr;
    requestPtr->readDIDRef = le_ref_CreateRef(ReadDIDRefMap, requestPtr);
    requestPtr->writeDIDRef = NULL;
    requestPtr->requestingThreadRef = le_thread_GetCurrent();

    // Queue the request to the read thread
    le_event_QueueFunctionToThread(ReadThreadRef, HandleReadWriteReq, requestPtr, NULL);

    // Wait for the read operation to complete for the defined time period
    le_clk_Time_t time = {SEM_TIME_TO_WAIT, 0};
    le_result_t ret = le_sem_WaitWithTimeOut(read_semaphore, time);
    isReadDIDLock.store(true);
    if (ret != LE_OK)
    {
        LE_ERROR("Read operation timeout");

        le_sem_Delete(read_semaphore);
        read_semaphore = NULL;
        isReadDIDLock.store(false);
        readStrg.result = TAF_DIAGDATAID_READ_DID_CONDITIONS_NOT_CORRECT;

        le_ref_DeleteRef(ReadDIDRefMap, requestPtr->readDIDRef);
        le_mem_Release(requestPtr);

        return LE_FAULT;
    }
    else
    {
        le_ref_DeleteRef(ReadDIDRefMap, requestPtr->readDIDRef);
        le_mem_Release(requestPtr);
    }

    isReadDIDLock.store(false);

    // Retrieve the result from the client-specific structure
    if (readStrg.result != 0) // Assuming 0 indicates success
    {
        LE_ERROR("Read operation failed for dataID %u, result %d", dataId,
                readStrg.result);
        return LE_FAULT;
    }

    // Copy the read data to the caller's buffer
    memcpy(dataRecordPtr, readStrg.didData, readStrg.didDataLen);
    *dataRecordSizePtr = readStrg.didDataLen;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * WriteDID callback.
 */
//-------------------------------------------------------------------------------------------------
void taf_diagDidStore::didWriteCb
(
    uint16_t dataID,
    uint8_t result,
    void* contextPtr
)
{
    auto &didStore = taf_diagDidStore::GetInstance();
    ReadWriteRequest_t* requestPtr = NULL;

    // Check the result.
    if (result != 0)
    {
        LE_ERROR("Write DID error code : %x", result);
        if (result != TAF_CONDITION_NOT_CORRECT && result != TAF_REQ_OUT_OF_RANGE
                && result != TAF_GENERAL_PROGRAMMING_FAILURE)
        {
            didStore.writeDIDPIResult = TAF_CONDITION_NOT_CORRECT;
        }
        else
        {
            didStore.writeDIDPIResult = result;
        }
        goto semPostOut;
    }

    if (contextPtr == NULL)
    {
        LE_ERROR("contextPtr is NULL");
        didStore.writeDIDPIResult = TAF_CONDITION_NOT_CORRECT;
        goto semPostOut;
    }

    requestPtr = (ReadWriteRequest_t*)le_ref_Lookup(didStore.WriteDIDRefMap, contextPtr);
    if (requestPtr == NULL)
    {
        LE_ERROR("Invalid requestPtr");
        didStore.writeDIDPIResult = TAF_CONDITION_NOT_CORRECT;
        goto semPostOut;
    }

    // Store the result in the client-specific structure
    didStore.writeDIDPIResult = result;
    goto semPostOut;

semPostOut:
    // Signal the client-specific semaphore
    if (write_semaphore != NULL)
    {
        le_sem_Post(write_semaphore);
    }
    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Write the data record to data ID.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_diagDidStore::Write
(
    uint16_t dataId,
        ///< [IN] data identifier.
    const uint8_t* dataPtr,
        ///< [IN] Data payload.
    size_t dataSize
        ///< [IN]
)
{
    // Prepare a write request
    ReadWriteRequest_t* requestPtr = (ReadWriteRequest_t*)le_mem_ForceAlloc(WriteRequestPool);

    if (!requestPtr)
    {
        LE_ERROR("Failed to allocate memory for write request.");
        return LE_FAULT;
    }

    // Create semaphore.
    if (write_semaphore == NULL)
    {
        write_semaphore = le_sem_Create("WriteDID Semaphore", 0);
    }

    requestPtr->request = WRITE_REQUEST_PI;
    requestPtr->dataId = dataId;
    requestPtr->dataRecordPtr = (uint8_t*)dataPtr;
    requestPtr->dataRecordSizePtr = &dataSize;
    requestPtr->writeDIDRef = le_ref_CreateRef(WriteDIDRefMap, requestPtr);
    requestPtr->readDIDRef = NULL;
    requestPtr->requestingThreadRef = le_thread_GetCurrent();

    // Queue the request to the write thread
    le_event_QueueFunctionToThread(WriteThreadRef, HandleReadWriteReq, requestPtr, NULL);

    // Wait for the write operation to complete
    le_clk_Time_t time = {SEM_TIME_TO_WAIT, 0};
    le_result_t ret = le_sem_WaitWithTimeOut(write_semaphore, time);
    isWriteDIDLock.store(true);
    if (ret != LE_OK)
    {
        LE_ERROR("Write operation timeout");

        le_sem_Delete(write_semaphore);
        write_semaphore = NULL;
        isWriteDIDLock.store(false);
        writeDIDPIResult = TAF_DIAGDATAID_WRITE_DID_CONDITIONS_NOT_CORRECT;

        le_ref_DeleteRef(WriteDIDRefMap, requestPtr->writeDIDRef);
        le_mem_Release(requestPtr);

        return LE_FAULT;
    }
    else
    {
        le_ref_DeleteRef(WriteDIDRefMap, requestPtr->writeDIDRef);
        le_mem_Release(requestPtr);
    }

    isWriteDIDLock.store(false);

    // Retrieve the result from the client-specific structure
    if (writeDIDPIResult != 0)
    {
        LE_ERROR("Write operation failed for dataID %u, res %d", dataId, writeDIDPIResult);
        return LE_FAULT;
    }

    return LE_OK;
}

void taf_diagDidStore::HandleReadWriteReq
(
    void* param1Ptr,
    void* param2Ptr
)
{
    auto &didStore = taf_diagDidStore::GetInstance();

    ReadWriteRequest_t* reqPtr = (ReadWriteRequest_t*)(param1Ptr);
    TAF_ERROR_IF_RET_NIL(reqPtr == NULL, "reqPtr is Null");

    switch (reqPtr->request)
    {
        case READ_REQUEST_PI:
        {
            LE_DEBUG("ReadDID Request!");
            if(!didStore.didStorInf)
            {
                LE_ERROR("Plugin not initialized");
                didStore.readStrg.result = LE_FAULT;
                return;
            }

            if (!didStore.isReadDIDLock.load())
            {
                le_result_t result = (*(didStore.didStorInf->diagDIDGetAsync))
                        (reqPtr->dataId, didStore.didReadCb, (void *)reqPtr->readDIDRef);

                if (result != LE_OK)
                {
                    LE_ERROR("Fail to get from DID Storage Plugin");
                    didStore.readStrg.result = TAF_DIAGDATAID_READ_DID_CONDITIONS_NOT_CORRECT;
                }
            }

            break;
        }
        case WRITE_REQUEST_PI:
        {
            LE_DEBUG("WriteDID Request!");
            if(!didStore.didStorInf)
            {
                LE_ERROR("Plugin not initialized");
                didStore.writeDIDPIResult = LE_FAULT;
                return;
            }

            if (!didStore.isWriteDIDLock.load())
            {
                le_result_t result = (*(didStore.didStorInf->diagDIDSetAsync))
                        (reqPtr->dataId, const_cast<uint8_t*>(reqPtr->dataRecordPtr),
                                *(reqPtr->dataRecordSizePtr), didStore.didWriteCb,
                                        (void *)reqPtr->writeDIDRef);
                if (result != LE_OK)
                {
                    LE_ERROR("Fail to set to DID Storage Plugin");
                    didStore.writeDIDPIResult = TAF_DIAGDATAID_WRITE_DID_CONDITIONS_NOT_CORRECT;
                }
            }

            break;
        }
        default:
            LE_ERROR("Invalid request type: %u", reqPtr->request);
            le_mem_Release(reqPtr);
            return;
    }

    return;
}

static void* ReadThreadMain
(
    void* contextPtr
)
{
    le_event_RunLoop();
}


static void* WriteThreadMain
(
    void* contextPtr
)
{
    le_event_RunLoop();
}

//-------------------------------------------------------------------------------------------------
/**
 * Plugin Callabck notification on DID change.
 */
//-------------------------------------------------------------------------------------------------
void taf_diagDidStore::DidMsgPluginCB
(
    uint16_t dataID,
    uint8_t *value,
    size_t len
)
{
    if (value == NULL || len == 0){
        LE_ERROR("Received value is NULL!");
        return;
    }
    bool isDIDAvailable = false;

    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(didStorageNotifyList));
    while (linkHandlerPtr)
    {
        taf_DIDStorgNotifyHandler_t* handlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_DIDStorgNotifyHandler_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(didStorageNotifyList), linkHandlerPtr);
        if (handlerCtxPtr && handlerCtxPtr->func)
        {
            le_dls_Link_t* linkDataIdPtr = le_dls_PeekTail(&(handlerCtxPtr->dataIdList));
            while (linkDataIdPtr)
            {
                taf_DataID_t* dataIdPtr = CONTAINER_OF(linkDataIdPtr, taf_DataID_t, link);
                linkDataIdPtr = le_dls_PeekPrev(&(handlerCtxPtr->dataIdList), linkDataIdPtr);
                if (dataIdPtr != NULL && dataIdPtr->dataId == dataID)
                {
                    LE_DEBUG("Notify DataID %x as changed", dataIdPtr->dataId);
                    isDIDAvailable = true;
                    handlerCtxPtr->func(dataID, value, len, handlerCtxPtr->ctxPtr);
                }
            }
        }
    }

    if(!isDIDAvailable)
    {
        LE_DEBUG("Remove data change notification for DID %x", dataID);
        auto &didStore = taf_diagDidStore::GetInstance();
        didStore.didStorInf->diagDIDRemoveDataChangeNotification(dataID);
    }
    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a handler for DID change notification.
 */
//-------------------------------------------------------------------------------------------------
taf_diagDidStore_DataIdChangeHandlerRef_t taf_diagDidStore::AddDataIdChangeHandler
(
    taf_diagDidStore_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint16_t dataId,
    taf_diagDidStore_DataIdChangeHandlerFunc_t handlerPtr,
        ///< [IN] Received message handler.
    void* contextPtr
        ///< [IN]
)
{
    taf_DidStore_t* servicePtr = (taf_DidStore_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, NULL, "Invalid service reference provided");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    if (servicePtr->msgDIDStorgHandlerRef != NULL)
    {
        LE_ERROR("DID notification handler is already registered");
        return NULL;
    }

    // Add the DID to plugin change notification list
    if(didStorInf)
    {
        le_result_t res = didStorInf->diagDIDAddDataChangeNotification(dataId);
        if (res == LE_FAULT)
        {
            LE_ERROR("DID %x not registered to plugin, error with return %d",dataId, res);
            return NULL;
        }
    }
    else
    {
        LE_ERROR("Plugin not initialized");
        return NULL;
    }

    taf_DIDStorgNotifyHandler_t* handlerObjPtr = NULL;
    handlerObjPtr = (taf_DIDStorgNotifyHandler_t*)le_mem_ForceAlloc(MsgDIDStorgHandlerPool);
    memset(handlerObjPtr, 0, sizeof(taf_DIDStorgNotifyHandler_t));

    // Initialize the Handler object.
    handlerObjPtr->svcRef     = svcRef;
    handlerObjPtr->func       = handlerPtr;
    handlerObjPtr->ctxPtr     = contextPtr;
    handlerObjPtr->handlerRef = (taf_diagDidStore_DataIdChangeHandlerRef_t)le_ref_CreateRef(
            MsgDIDStorgHandlerRefMap, handlerObjPtr);
    handlerObjPtr->link = LE_DLS_LINK_INIT;

    // Add DataID to the list
    taf_DataID_t* dataIdPtr = (taf_DataID_t *)le_mem_ForceAlloc(dataIdPool);
    memset(dataIdPtr, 0, sizeof(taf_DataID_t));

    dataIdPtr->dataId = dataId;
    dataIdPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&handlerObjPtr->dataIdList, &dataIdPtr->link);

    // Attach handler to service.
    servicePtr->msgDIDStorgHandlerRef = handlerObjPtr->handlerRef;

    le_dls_Queue(&(didStorageNotifyList),&handlerObjPtr->link);
    return handlerObjPtr->handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the reference of a DID change handler.
 */
//-------------------------------------------------------------------------------------------------
taf_diagDidStore_DIDChangeHandlerRef_t taf_diagDidStore::GetDIDHandlerRef
(
    taf_diagDidStore_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    LE_DEBUG("GetDIDHandlerRef!");

    taf_DidStore_t* servicePtr = (taf_DidStore_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, NULL, "Invalid service reference");

    return (taf_diagDidStore_DIDChangeHandlerRef_t)servicePtr->msgDIDStorgHandlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Adds the DID to handler for change notification.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_diagDidStore::AddDIDToHandler
(
    taf_diagDidStore_DIDChangeHandlerRef_t handlerRef,
        ///< [IN] Handler reference.
    uint16_t dataId
        ///< [IN] Data identifier.
)
{
    LE_DEBUG("AddDIDToHandler!");
    TAF_ERROR_IF_RET_VAL(handlerRef == NULL, LE_BAD_PARAMETER, "Invalid handlerRef");

    taf_DIDStorgNotifyHandler_t* handlerObjPtr = NULL;
    handlerObjPtr = (taf_DIDStorgNotifyHandler_t*)le_ref_Lookup(MsgDIDStorgHandlerRefMap,
            (taf_diagDidStore_DataIdChangeHandlerRef_t)handlerRef);
    TAF_ERROR_IF_RET_VAL(handlerObjPtr == NULL, LE_BAD_PARAMETER, "Invalid handlerObjPtr");

    // Check DataID was already added before or not
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&handlerObjPtr->dataIdList);
    while (linkPtr)
    {
        taf_DataID_t* dataIdPtr = CONTAINER_OF(linkPtr, taf_DataID_t, link);
        if (dataIdPtr != NULL && dataIdPtr->dataId == dataId)
        {
            return LE_DUPLICATE;
        }
        linkPtr = le_dls_PeekNext(&handlerObjPtr->dataIdList, linkPtr);
    }

    // Add the DID to plugin change notification list
    if(didStorInf)
    {
        le_result_t res = didStorInf->diagDIDAddDataChangeNotification(dataId);
        if (res == LE_FAULT)
        {
            LE_ERROR("DID %x not registered to plugin, error with return %d",dataId, res);
            return LE_FAULT;
        }
    }
    else
    {
        LE_ERROR("Plugin not initialized");
        return LE_FAULT;
    }

    // Add DataID to the list
    taf_DataID_t* dataIdPtr = (taf_DataID_t *)le_mem_ForceAlloc(dataIdPool);
    memset(dataIdPtr, 0, sizeof(taf_DataID_t));

    dataIdPtr->dataId = dataId;
    dataIdPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&handlerObjPtr->dataIdList, &dataIdPtr->link);
    LE_DEBUG("DataID (0x%x) added", dataIdPtr->dataId);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Removes DID from handler.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_diagDidStore::RemoveDIDFromHandler
(
    taf_diagDidStore_DIDChangeHandlerRef_t handlerRef,
        ///< [IN] Handler reference.
    uint16_t dataId
        ///< [IN] Data identifier.
)
{
    TAF_ERROR_IF_RET_VAL(handlerRef == NULL, LE_BAD_PARAMETER, "Invalid handlerRef");

    taf_DIDStorgNotifyHandler_t* handlerObjPtr = NULL;
    handlerObjPtr = (taf_DIDStorgNotifyHandler_t*)le_ref_Lookup(MsgDIDStorgHandlerRefMap,
            (taf_diagDidStore_DataIdChangeHandlerRef_t)handlerRef);
    TAF_ERROR_IF_RET_VAL(handlerObjPtr == NULL, LE_BAD_PARAMETER, "Invalid handlerObjPtr");

    // Remove the requested DataID from the list.
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&handlerObjPtr->dataIdList);
    while (linkPtr)
    {
        taf_DataID_t* dataIdPtr = CONTAINER_OF(linkPtr, taf_DataID_t, link);
        if (dataIdPtr != NULL && dataIdPtr->dataId == dataId)
        {
            // Release the dataIdPtr
            le_dls_Remove(&handlerObjPtr->dataIdList, &dataIdPtr->link);
            le_mem_Release(dataIdPtr);
            return LE_OK;
        }
        linkPtr = le_dls_PeekNext(&handlerObjPtr->dataIdList, linkPtr);
    }

    LE_DEBUG("DataID (0x%x) not found", dataId);
    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagDataIDStor_MsgNotify'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagDidStore::RemoveDataIdChangeHandler
(
    taf_diagDidStore_DataIdChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_DEBUG("RemoveDataIdChangeHandler!");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");

    taf_DidStore_t* servicePtr = NULL;
    taf_DIDStorgNotifyHandler_t* handlerObjPtr = NULL;

    handlerObjPtr = (taf_DIDStorgNotifyHandler_t*)le_ref_Lookup(MsgDIDStorgHandlerRefMap,
            handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerObjPtr == NULL, "Invalid handlerObjPtr");

    // Release all the dataId list.
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Pop(&handlerObjPtr->dataIdList);
    while (linkPtr)
    {
        taf_DataID_t* dataIdPtr = CONTAINER_OF(linkPtr, taf_DataID_t, link);
        if (dataIdPtr != NULL)
        {
            // Release the dataIdPtr
            le_mem_Release(dataIdPtr);
        }
        linkPtr = le_dls_Pop(&handlerObjPtr->dataIdList);
    }

    servicePtr = (taf_DidStore_t*)le_ref_Lookup(SvcRefMap, handlerObjPtr->svcRef);
    if (servicePtr == NULL)
    {
        LE_WARN("The handler is not belong to this service.");
        le_ref_DeleteRef(MsgDIDStorgHandlerRefMap, handlerRef);
        le_mem_Release(handlerObjPtr);

        return;
    }

    // Detach the handler from service.
    servicePtr->msgDIDStorgHandlerRef = NULL;

    // Remove the handler from the double-linked list
    le_dls_Remove(&(didStorageNotifyList), &(handlerObjPtr->link));

    // Clear Handler resources
    handlerObjPtr->handlerRef = NULL;
    handlerObjPtr->svcRef     = NULL;
    handlerObjPtr->func       = NULL;
    handlerObjPtr->ctxPtr     = NULL;

    if(le_dls_NumLinks(&didStorageNotifyList) == 0)
    {
        didStorageNotifyList = LE_DLS_LIST_INIT;
    }

    // Free the handler.
    le_ref_DeleteRef(MsgDIDStorgHandlerRefMap, handlerRef);
    le_mem_Release(handlerObjPtr);

    return;
}


/**
 * Callback function for read dataID request message
 */
void taf_diagDidStore::readDataIDMsgHandler
(
    taf_diagDataID_RxReadDIDMsgRef_t rxMsgRef,
    const uint16_t* dataIdPtr,
    size_t dataIdSize,
    void* contextPtr
)
{
    LE_DEBUG("readDataIDMsgHandler!");
    auto &didStore = taf_diagDidStore::GetInstance();

    uint8_t sendBuf[TAF_DIAGDATAID_MAX_READ_DID_PAYLOAD_SIZE];
    size_t sendBufLen = 0;
    uint8_t totalBuf[TAF_DIAGDATAID_MAX_READ_DID_PAYLOAD_SIZE];
    size_t totalBufLen = 0;

    le_result_t result;
    le_result_t ret;

    if(dataIdSize == 0 || dataIdPtr == NULL)
    {
        if(taf_diagDataID_SendReadDIDResp( rxMsgRef, TAF_REQ_OUT_OF_RANGE,
                NULL, 0 ) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    for( int i=0; i< (int)dataIdSize; i++)
    {
        if(sendBufLen + DID_LEN > TAF_DIAGDATAID_MAX_READ_DID_PAYLOAD_SIZE)
        {
            LE_WARN("Send NRC %x to readDID req!", TAF_DIAGDATAID_READ_DID_RESPONSE_TOO_LONG);
            if(taf_diagDataID_SendReadDIDResp( rxMsgRef,
                    TAF_DIAGDATAID_READ_DID_RESPONSE_TOO_LONG, NULL, 0 ) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

        // Get record data for dataId[i] and fill record data into sendBuf
        ret = didStore.Read(dataIdPtr[i], sendBuf, &sendBufLen);
        if (ret == LE_OK)
        {
            if (totalBufLen + sendBufLen + DID_LEN > TAF_DIAGDATAID_MAX_READ_DID_PAYLOAD_SIZE)
            {
                LE_WARN("Send NRC %x to readDID req!", TAF_DIAGDATAID_READ_DID_RESPONSE_TOO_LONG);
                if(taf_diagDataID_SendReadDIDResp( rxMsgRef,
                    TAF_DIAGDATAID_READ_DID_RESPONSE_TOO_LONG, NULL, 0 ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
                return;
            }

            // Fill data id
            totalBuf[totalBufLen] = (dataIdPtr[i] & 0xff00) >> 8;
            totalBuf[totalBufLen+1] = dataIdPtr[i] & 0xff;
            totalBufLen = totalBufLen + DID_LEN;

            memcpy(totalBuf + totalBufLen, sendBuf, sendBufLen);
            totalBufLen += sendBufLen;
        }
    }

    if ( totalBufLen == 0 )
    {
        LE_WARN("Send NRC %x to readDID req!", didStore.readStrg.result);
        result = taf_diagDataID_SendReadDIDResp( rxMsgRef,
                didStore.readStrg.result, NULL, 0);
    }
    else
    {
        result = taf_diagDataID_SendReadDIDResp( rxMsgRef,
                TAF_DIAGDATAID_READ_DID_NO_ERROR, totalBuf, totalBufLen);
    }

    if (result != LE_OK)
    {
        LE_ERROR("send response error");
    }
    return;
}

/**
 * Callback function for writeDataID request message
 */
void taf_diagDidStore::writeDataIDMsgHandler
(
    taf_diagDataID_RxWriteDIDMsgRef_t rxMsgRef,
    uint16_t dataId,
    void* contextPtr
)
{
    LE_DEBUG("writeDataIDMsgHandler!");
    auto &didStore = taf_diagDidStore::GetInstance();

    uint8_t recordData[TAF_DIAGDATAID_MAX_DID_DATA_RECORD_SIZE];
    size_t dataLen = 0;
    le_result_t result;
    le_result_t ret;

    result = taf_diagDataID_GetWriteDataRecord(rxMsgRef, recordData, &dataLen);
    if( result != LE_OK)
    {
        LE_ERROR("Getting data record error");
        if(taf_diagDataID_SendWriteDIDResp(rxMsgRef,
                TAF_DIAGDATAID_WRITE_DID_CONDITIONS_NOT_CORRECT, dataId) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    ret = didStore.Write(dataId, recordData, dataLen);
    if (ret != LE_OK)
    {
        LE_WARN("Send NRC %x for writeDID req!", didStore.writeDIDPIResult);
        // send NRC
        result = taf_diagDataID_SendWriteDIDResp(rxMsgRef,
                didStore.writeDIDPIResult, dataId);
    }
    else
    {
        result = taf_diagDataID_SendWriteDIDResp( rxMsgRef,
                TAF_DIAGDATAID_WRITE_DID_NO_ERROR, dataId);
    }

    if(result != LE_OK)
    {
        LE_ERROR("Send response error");
    }

}


void taf_diagDidStore::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    LE_DEBUG("OnClientDisconnection");
    auto &didStore = taf_diagDidStore::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(didStore.SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_DidStore_t* servicePtr = (taf_DidStore_t *)le_ref_GetValue(iterRef);

        if (servicePtr == NULL || servicePtr->svcRef != le_ref_GetSafeRef(iterRef))
        {
            LE_ERROR("Service pointer is NULL or mismatched safe reference.");
            continue;
        }

        if (servicePtr->sessionRef == sessionRef)
        {
            // Clearing the registered readDID handler
            if (servicePtr->msgDIDStorgHandlerRef != NULL)
            {
                didStore.RemoveDataIdChangeHandler(servicePtr->msgDIDStorgHandlerRef);
                servicePtr->msgDIDStorgHandlerRef = NULL;
            }

            // Remove the service from the double-linked list
            le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(didStorageNotifyList));
            while (linkHandlerPtr)
            {
                taf_DIDStorgNotifyHandler_t* handlerCtxPtr =
                    CONTAINER_OF(linkHandlerPtr, taf_DIDStorgNotifyHandler_t, link);
                if (handlerCtxPtr != NULL && handlerCtxPtr->svcRef == servicePtr->svcRef)
                {
                    // Remove the handler from the list and release memory
                    le_dls_Remove(&(didStorageNotifyList), &handlerCtxPtr->link);
                    le_mem_Release(handlerCtxPtr);
                    handlerCtxPtr = NULL; // Set the pointer to NULL after release
                }
                else
                {
                    linkHandlerPtr = le_dls_PeekPrev(&(didStorageNotifyList), linkHandlerPtr);
                }
            }

            // Deleting the reference and releasing the service object
            le_ref_DeleteRef(didStore.SvcRefMap, (void*)servicePtr->svcRef);
            le_mem_Release(servicePtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Load plugin. If succeeded, send EVT_LOAD_PLUG_IN_READY event.
 */
 //-------------------------------------------------------------------------------------------------
void taf_diagDidStore::LoadPlugin()
{
    didStorInf = (diagDID_Inf_t*)taf_devMgr_LoadDrv(TAF_DIAGDID_MODULE_NAME, NULL);
    if (didStorInf == NULL)
    {
        LE_ERROR("Failed to load plugin: %s, try it later", TAF_DIAGDID_MODULE_NAME);
        return;
    }

    if (didStorInf->init != NULL)
    {
        int ret = 0;
        LE_DEBUG("Before safe call init");
        ENTER_SAFE_CALL(TIMER_SAFECALL, ret, (*(didStorInf->init)));
        EXIT_SAFE_CALL();

        if (ret == -1)
        {
            LE_CRIT("Failed to init plugin: %s", TAF_DIAGDID_MODULE_NAME);
            exit(EXIT_SUCCESS);
        }
    }

    if (didStorInf->addDataChangeHandler != NULL)
    {
        int ret = 0;
        LE_DEBUG("Before safe call addDataChangeHandler");
        ENTER_SAFE_CALL(TIMER_SAFECALL, ret, (*(didStorInf->addDataChangeHandler)), DidMsgPluginCB);
        EXIT_SAFE_CALL();

        if (ret == -1)
        {
            LE_CRIT("Failed to add handler for plugin: %s", TAF_DIAGDID_MODULE_NAME);
            exit(EXIT_SUCCESS);
        }
    }

    //Load plugin successfully.
    LE_INFO("Load plugin successfully");
    taf_didStore_ReadyEvtType_t readyType;
    readyType.type = EVT_LOAD_PLUG_IN_READY;
    isPluginReady = true;
    le_event_Report(evtReady, &readyType, sizeof(readyType));

}

//--------------------------------------------------------------------------------------------------
/**
 * Connect diag service. If succeeded, send EVT_CONNECT_DIAG_SVC_READY event.
 */
 //-------------------------------------------------------------------------------------------------
void taf_diagDidStore::ConnectDiagSvc()
{
    le_result_t rst = taf_diagDataID_TryConnectService();

    if (rst != LE_OK)
    {
        LE_ERROR("Failed to connect diag service, try it later");
        return;
    }

    //Connect Diag service successfully.
    LE_INFO("Connect Diag service successfully");
    taf_didStore_ReadyEvtType_t readyType;
    readyType.type = EVT_CONNECT_DIAG_SVC_READY;
    isDiagSvcReady = true;
    le_event_Report(evtReady, &readyType, sizeof(readyType));

}
//--------------------------------------------------------------------------------------------------
/**
 * Handle the event when loading plugin or connecting diag service successfully.
 */
 //-------------------------------------------------------------------------------------------------
void taf_diagDidStore::ReadyEvtHandler(void * reportPtr)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");
    taf_didStore_ReadyEvtType_t* evtType =(taf_didStore_ReadyEvtType_t*)reportPtr;
    auto & inst = GetInstance();

    switch(evtType->type)
    {
        case EVT_LOAD_PLUG_IN_READY:
            // After plugin loaded, advertise the service to client sides
            LE_INFO("Advertise didStore service");
            taf_diagDidStore_AdvertiseService();

            // Set session close handler
            le_msg_AddServiceCloseHandler(taf_diagDidStore_GetServiceRef(), OnClientDisconnection,
                    NULL);
        break;
        case EVT_CONNECT_DIAG_SVC_READY:
            // Diag service Client request-response handling
            // Get diag Data ID reference
            inst.DiagDataIDSvcRef = taf_diagDataID_GetService();
            if(inst.DiagDataIDSvcRef == NULL)
            {
                LE_ERROR("Get diagDataID service error");
                return;
            }

            inst.DiagReadDataIDMsgRef =
                taf_diagDataID_AddRxReadDIDMsgHandler(
                    inst.DiagDataIDSvcRef,
                    inst.readDataIDMsgHandler,
                    NULL);

            TAF_ERROR_IF_RET_NIL(
                inst.DiagReadDataIDMsgRef == NULL,
                "Not Registered successfully for readDataIDMsgHandler");

            inst.DiagWriteDataIDMsgRef =
                taf_diagDataID_AddRxWriteDIDMsgHandler(
                    inst.DiagDataIDSvcRef,
                    inst.writeDataIDMsgHandler,
                    NULL);

            TAF_ERROR_IF_RET_NIL(
                inst.DiagWriteDataIDMsgRef == NULL,
                "Not Registered successfully for writeDataIDMsgHandler");
        break;
        default:
            LE_ERROR("Wrong event type");
        break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Load plugin and connect diag service until retry count has reaches the maximum value.
 */
 //-------------------------------------------------------------------------------------------------
void taf_diagDidStore::RetryHandler(le_timer_Ref_t timerRef)
{
    auto & inst = GetInstance();

    uint32_t expiryCount = le_timer_GetExpiryCount(timerRef);
    LE_DEBUG("expiryCount = %d", expiryCount);
    if(expiryCount < MAX_NUM_OF_ATTEMPTS)
    {
        // Load plugin. IF failed to load plugin, try again later.
        if(!inst.isPluginReady)
        {
            inst.LoadPlugin();
        }

        //Connect diag service, if failed to connect, try it later.
        if(!inst.isDiagSvcReady)
        {
            inst.ConnectDiagSvc();
        }

        // Load plugin and connect diag service successfully. Delete the timer.
        if(inst.isPluginReady && inst.isDiagSvcReady)
        {
            LE_INFO("Load plugin and connect diag service successfully");
            le_timer_Delete(timerRef);
        }

        return;
    }

    if(!inst.isPluginReady)
    {
        LE_CRIT("Load plugin timeout");
        //Load plugin timeout.
        exit(EXIT_SUCCESS);
    }

    if(!inst.isDiagSvcReady)
    {
        LE_CRIT("Connect diag service timeout");
    }

    le_timer_Delete(timerRef);

}

//--------------------------------------------------------------------------------------------------
/**
 * Load plugin and try to connect diag service, if cannot load plugin or connect diag service
 * successfully, start a timer to try again later.
 */
 //-------------------------------------------------------------------------------------------------
void taf_diagDidStore::GetSvcReady(void *p1, void *p2)
{
    LE_UNUSED(p1);
    LE_UNUSED(p2);

    auto & inst = GetInstance();

    // Load plugin.
    inst.LoadPlugin();

    //Connect diag service.
    inst.ConnectDiagSvc();

    if( inst.isPluginReady && inst.isDiagSvcReady)
    {
        LE_INFO("Load plugin and connect diag service successfully");
        return;
    }

    // Start one timer for the retry-action
    le_timer_Ref_t retryTimer = le_timer_Create("retry-timer-diagsvc");

    if (retryTimer == NULL)
    {
        LE_ERROR("Failed to le_timer_Create for the retry-timer");
        return;
    }

    le_timer_SetRepeat(retryTimer, MAX_NUM_OF_ATTEMPTS);
    le_timer_SetHandler(retryTimer, RetryHandler);
    le_timer_SetWakeup(retryTimer, false);
    le_timer_SetMsInterval(retryTimer, RETRY_TIMER_INTERVAL);
    le_timer_Start(retryTimer);
    LE_DEBUG("Retry timer start ...");
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_diagDidStore::Init
(
    void
)
{
    // parse json configuration
    le_result_t result = ParseDidStoreJsonConfig(DEFAULT_DID_STORE_CONFIG_PATH);
    if(result != LE_OK){
        LE_FATAL("Failed to read json");
    }

    // Create memory pools.
    SvcPool = le_mem_CreatePool("didStoreSvcPool", sizeof(taf_DidStore_t));
    // Create reference maps
    SvcRefMap = le_ref_CreateMap("didStoreSvcRefMap", DEFAULT_SVC_REF_CNT);

    MsgDIDStorgHandlerPool = le_mem_CreatePool("MsgDIDStorgHandlerPool",
        sizeof(taf_DIDStorgNotifyHandler_t));
    MsgDIDStorgHandlerRefMap = le_ref_CreateMap("MsgDIDStorgHandlerRefMap",
        DEFAULT_DID_HANDLER_REF_CNT);
    dataIdPool = le_mem_CreatePool("DataIDPool", sizeof(taf_DataID_t));

    //Read DID Plugin thread
    ReadRequestPool = le_mem_CreatePool("Read plugin Request", sizeof(ReadWriteRequest_t));
    ReadDIDRefMap = le_ref_CreateMap("ReadDIDRefMap", DEFAULT_READ_DID_REF_CNT);
    ReadThreadRef = le_thread_Create("didstoreThR", ReadThreadMain, NULL);
    le_thread_SetPriority(ReadThreadRef, LE_THREAD_PRIORITY_IDLE);
    le_thread_Start(ReadThreadRef);

    //write DID Plugin thread
    WriteRequestPool = le_mem_CreatePool("Write plugin Request", sizeof(ReadWriteRequest_t));
    WriteDIDRefMap = le_ref_CreateMap("WriteDIDRefMap", DEFAULT_WRITE_DID_REF_CNT);
    WriteThreadRef = le_thread_Create("didstoreThW", WriteThreadMain, NULL);
    le_thread_SetPriority(WriteThreadRef, LE_THREAD_PRIORITY_IDLE);
    le_thread_Start(WriteThreadRef);

    // Load plugin and connect diag service to get service ready.
    le_event_QueueFunction(GetSvcReady, NULL, NULL);

    evtReady = le_event_CreateId("readyEvt", sizeof(taf_didStore_ReadyEvtType_t));
    le_event_AddHandler("readyEvtHdlr", evtReady, ReadyEvtHandler);
}
