/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDidStore.hpp"

namespace pt = boost::property_tree;
using namespace tafsvc;

le_sem_Ref_t read_semaphore = NULL;
le_sem_Ref_t write_semaphore = NULL;

// Diag RDBI/WDBI
static taf_diagDataID_ServiceRef_t DiagDataIDSvcRef = NULL;
static taf_diagDataID_RxReadDIDMsgHandlerRef_t DiagReadDataIDMsgRef = NULL;
static taf_diagDataID_RxWriteDIDMsgHandlerRef_t DiagWriteDataIDMsgRef = NULL;

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
    LE_INFO("Gets the DataIDStor service!");
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
    LE_DEBUG("find the service object!");

    auto &didStore = taf_diagDidStore::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(didStore.SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_DidStore_t* servicePtr = (taf_DidStore_t *)le_ref_GetValue(iterRef);
        if ((servicePtr != NULL) && (servicePtr->sessionRef == sessionRef))
        {
            LE_INFO("Found existing svcRef %p for client session %p", servicePtr->svcRef,
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
    LE_INFO("ParseDidStoreJsonConfig");

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
            LE_DEBUG("App name is %s", appName.c_str());
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

    LE_INFO("PID : %d", pid);

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
    LE_DEBUG("IsReadAppAccessible");

    for (const auto& didStore : dataIdStoreAccessCfg)
    {
        if (std::strcmp(didStore.AppName, appName) == 0)
        {
            for (const auto& readDID : didStore.ReadAccessDID)
            {
                if (readDID == dataId)
                {
                    LE_INFO("dataId %x is in the access list of app %s", dataId, appName);
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
    LE_DEBUG("IsWriteAppAccessible");

    for (const auto& didStore : dataIdStoreAccessCfg)
    {
        if (std::strcmp(didStore.AppName, appName) == 0)
        {
            for (const auto& writeDID : didStore.WriteAccessDID)
            {
                if (writeDID == dataId)
                {
                    LE_INFO("dataId %x is in the access list of app %s", dataId, appName);
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
    LE_DEBUG("didReadCb!");

    auto &didStore = taf_diagDidStore::GetInstance();

    // Store the read result in the client-specific structure
    didStore.readStrg.readDID = dataID;

    if (contextPtr == NULL)
    {
        LE_ERROR("contextPtr is NULL");
        didStore.readStrg.result = LE_FAULT;
        if (read_semaphore != NULL)
        {
            le_sem_Post(read_semaphore); // Signal client-specific semaphore
        }
        return;
    }

    ReadWriteRequest_t* requestPtr =
            (ReadWriteRequest_t*)le_ref_Lookup(didStore.ReadDIDRefMap, contextPtr);
    if (requestPtr == NULL)
    {
        LE_ERROR("Invalid requestPtr");
        didStore.readStrg.result = LE_FAULT;
        if (read_semaphore != NULL)
        {
            le_sem_Post(read_semaphore); // Signal client-specific semaphore
        }
        return;
    }

    if (dataRecPtr == NULL || len == 0 || len > sizeof(didStore.readStrg.didData))
    {
        LE_ERROR("Invalid read response: dataID %u, len %zu", dataID, len);
        didStore.readStrg.result = TAF_REQ_OUT_OF_RANGE;
        if (read_semaphore != NULL)
        {
            le_sem_Post(read_semaphore); // Signal client-specific semaphore
        }
        return;
    }

    // Copy data to the client-specific structure
    memcpy(didStore.readStrg.didData, dataRecPtr, len);
    didStore.readStrg.didDataLen = len;
    didStore.readStrg.result = result;

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
    LE_DEBUG("Read! DataID: 0x%04X", dataId);

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
    LE_DEBUG("Successfully read from DID 0x%04X, length: %zu", dataId, *dataRecordSizePtr);

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
    LE_DEBUG("didWriteCb!");
    auto &didStore = taf_diagDidStore::GetInstance();

    if (contextPtr == NULL)
    {
        LE_ERROR("contextPtr is NULL");
        didStore.writeDIDPIResult = LE_FAULT;
        if (write_semaphore != NULL)
        {
            le_sem_Post(write_semaphore);
        }

        return;
    }

    ReadWriteRequest_t* requestPtr = (ReadWriteRequest_t*)le_ref_Lookup(didStore.WriteDIDRefMap,
            contextPtr);
    if (requestPtr == NULL)
    {
        LE_ERROR("Invalid requestPtr");
        didStore.writeDIDPIResult = LE_FAULT;
        if (write_semaphore != NULL)
        {
            le_sem_Post(write_semaphore);
        }

        return;
    }

    // Store the result in the client-specific structure
    didStore.writeDIDPIResult = result;
    LE_DEBUG("Write operation for DataID %u completed with result %d", dataID, result);

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
    LE_DEBUG("Write DataID: 0x%04X", dataId);

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
    LE_DEBUG("HandleReadWriteReq!");
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
                    didStore.readStrg.result = result;
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
                    LE_ERROR("Fail to get from DID Storage Plugin");
                    didStore.writeDIDPIResult = result;
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


void taf_diagDidStore::DidMsgPluginCB
(
    uint16_t dataID,
    uint8_t *value,
    size_t len
)
{
    LE_DEBUG("DidMsgPluginCB!");

    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(didStorageNotifyList));
    while (linkHandlerPtr)
    {
        taf_DIDStorgNotifyHandler_t * handlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_DIDStorgNotifyHandler_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(didStorageNotifyList), linkHandlerPtr);
        if (handlerCtxPtr->func && (handlerCtxPtr->did == dataID))
        {
            LE_INFO("Notifying to clients");
            handlerCtxPtr->func(dataID, value, len, handlerCtxPtr->ctxPtr);
        }
    }

}


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
    LE_DEBUG("AddDataIdChangeHandler!");

    taf_DidStore_t* servicePtr = (taf_DidStore_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, NULL, "Invalid service reference provided");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    if (servicePtr->msgDIDStorgHandlerRef != NULL)
    {
        LE_ERROR("DID notification handler is already registered");
        return NULL;
    }

    taf_DIDStorgNotifyHandler_t* handlerObjPtr = NULL;
    handlerObjPtr = (taf_DIDStorgNotifyHandler_t*)le_mem_ForceAlloc(MsgDIDStorgHandlerPool);
    memset(handlerObjPtr, 0, sizeof(taf_DIDStorgNotifyHandler_t));

    // Initialize the Handler object.
    handlerObjPtr->svcRef     = svcRef;
    handlerObjPtr->did = dataId;
    handlerObjPtr->func       = handlerPtr;
    handlerObjPtr->ctxPtr     = contextPtr;
    handlerObjPtr->handlerRef = (taf_diagDidStore_DataIdChangeHandlerRef_t)le_ref_CreateRef(
            MsgDIDStorgHandlerRefMap, handlerObjPtr);
    handlerObjPtr->link = LE_DLS_LINK_INIT;
    // Attach handler to service.
    servicePtr->msgDIDStorgHandlerRef = handlerObjPtr->handlerRef;

    LE_INFO("DID Msg: Registered Handler");

    le_dls_Queue(&(didStorageNotifyList),&handlerObjPtr->link);
    return handlerObjPtr->handlerRef;
}

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
        LE_DEBUG("Reading for requested DataID!");
        if(sendBufLen + DID_LEN > TAF_DIAGDATAID_MAX_READ_DID_PAYLOAD_SIZE)
        {
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
            LE_DEBUG("Requested DataID is read!");
            if (totalBufLen + sendBufLen + DID_LEN > TAF_DIAGDATAID_MAX_READ_DID_PAYLOAD_SIZE)
            {
                result = taf_diagDataID_SendReadDIDResp( rxMsgRef,
                        TAF_DIAGDATAID_READ_DID_RESPONSE_TOO_LONG, NULL, 0);
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
        LE_DEBUG("Sending NRC to readDID req!");
        result = taf_diagDataID_SendReadDIDResp( rxMsgRef,
                TAF_REQ_OUT_OF_RANGE, NULL, 0);
    }
    else
    {
        LE_DEBUG("Sending positive response for readDID req!");
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
        LE_DEBUG("Sending NRC for writeDID req!");
        // send NRC
        result = taf_diagDataID_SendWriteDIDResp( rxMsgRef,
                TAF_REQ_OUT_OF_RANGE, dataId);
    }
    else
    {
        LE_DEBUG("Sending NRC for writeDID req!");
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
                    // Print the address before removal
                    LE_DEBUG("Removing handler object at address: %p", handlerCtxPtr);
                    // Remove the handler from the list and release memory
                    le_dls_Remove(&(didStorageNotifyList), &handlerCtxPtr->link);
                    le_mem_Release(handlerCtxPtr);
                    handlerCtxPtr = NULL; // Set the pointer to NULL after release

                    // Print the address after removal
                    LE_DEBUG("Handler object at address: %p removed", handlerCtxPtr);
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
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_diagDidStore::Init
(
    void
)
{
    LE_INFO("taf_diagDidStore Init!");

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

    //Read DID Plugin thread
    ReadRequestPool = le_mem_CreatePool("Read plugin Request", sizeof(ReadWriteRequest_t));
    ReadDIDRefMap = le_ref_CreateMap("ReadDIDRefMap", DEFAULT_READ_DID_REF_CNT);
    ReadThreadRef = le_thread_Create("Background Thread", ReadThreadMain, NULL);
    le_thread_SetPriority(ReadThreadRef, LE_THREAD_PRIORITY_IDLE);
    le_thread_Start(ReadThreadRef);

    //write DID Plugin thread
    WriteRequestPool = le_mem_CreatePool("Write plugin Request", sizeof(ReadWriteRequest_t));
    WriteDIDRefMap = le_ref_CreateMap("WriteDIDRefMap", DEFAULT_WRITE_DID_REF_CNT);
    WriteThreadRef = le_thread_Create("Background Thread", WriteThreadMain, NULL);
    le_thread_SetPriority(WriteThreadRef, LE_THREAD_PRIORITY_IDLE);
    le_thread_Start(WriteThreadRef);

    // Load plugin.
    didStorInf = (diagDID_Inf_t*)taf_devMgr_LoadDrv(TAF_DIAGDID_MODULE_NAME, NULL);
    if (didStorInf == NULL)
    {
        LE_WARN("DID storage PI module is not loaded.");
    }
    else
    {
        if (didStorInf->init != NULL)
        {
            LE_INFO("DID storage PI init...");
            (*(didStorInf->init))();
        }
    }

    if (didStorInf)
    {
        LE_WARN("DID storage PI addDataChangeHandler.");
        (*(didStorInf->addDataChangeHandler))(DidMsgPluginCB);
    }

    // Diag service Client request-response handling
    // Get diag Data ID reference
    DiagDataIDSvcRef = taf_diagDataID_GetService();
    if(DiagDataIDSvcRef == NULL)
    {
        LE_ERROR("Get diagDataID service error");
        return;
    }

    DiagReadDataIDMsgRef = taf_diagDataID_AddRxReadDIDMsgHandler(DiagDataIDSvcRef,
            readDataIDMsgHandler, NULL);
    TAF_ERROR_IF_RET_NIL(DiagReadDataIDMsgRef == NULL,
            "Not Registered successfully for readDataIDMsgHandler");

    DiagWriteDataIDMsgRef = taf_diagDataID_AddRxWriteDIDMsgHandler(DiagDataIDSvcRef,
            writeDataIDMsgHandler, NULL);
    TAF_ERROR_IF_RET_NIL(DiagWriteDataIDMsgRef == NULL,
            "Not Registered successfully for writeDataIDMsgHandler");

    // Set session close handler
    le_msg_AddServiceCloseHandler(taf_diagDidStore_GetServiceRef(), OnClientDisconnection, NULL);

    LE_INFO("taf_diagDidStore Init completed!");

}
