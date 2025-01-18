/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDidStore.hpp"

using namespace telux::tafsvc;

le_sem_Ref_t read_semaphore;
le_sem_Ref_t write_semaphore;
le_mem_PoolRef_t WriteRequestPool;
le_mem_PoolRef_t ReadRequestPool;
le_thread_Ref_t ReadThreadRef;
le_thread_Ref_t WriteThreadRef;

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

        // Initialize semaphores
        servicePtr->readSemaphore = le_sem_Create("ReadSemaphore", 0);
        servicePtr->writeSemaphore = le_sem_Create("WriteSemaphore", 0);

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


void taf_diagDidStore::didReadCb
(
    uint16_t dataID,
    uint8_t *value,
    size_t len,
    uint8_t result
)
{
    LE_DEBUG("didReadCb!");

    auto &didStore = taf_diagDidStore::GetInstance();

    // Get the session reference of the client that initiated the read request
    le_msg_SessionRef_t clientSessionRef = taf_diagDidStore_GetClientSessionRef();
    if (!clientSessionRef)
    {
        LE_ERROR("Unable to retrieve client session reference.");
        return;
    }

    // Find the client-specific service object
    taf_DidStore_t* servicePtr = (taf_DidStore_t*)didStore.GetServiceObj(clientSessionRef);

    if (!servicePtr)
    {
        LE_ERROR("Service object not found for client session %p", clientSessionRef);
        return;
    }

     // Store the read result in the client-specific structure
    servicePtr->readStrg.readDID = dataID;

    if (value == NULL || len == 0 || len > sizeof(servicePtr->readStrg.didData))
    {
        LE_ERROR("Invalid read response: dataID %u, len %zu", dataID, len);
        servicePtr->readStrg.result = TAF_REQ_OUT_OF_RANGE;
        le_sem_Post(servicePtr->readSemaphore); // Signal client-specific semaphore
        return;
    }

    // Copy data to the client-specific structure
    memcpy(servicePtr->readStrg.didData, value, len);
    servicePtr->readStrg.didDataLen = len;
    servicePtr->readStrg.result = result;

    // Signal the client-specific semaphore
    le_sem_Post(servicePtr->readSemaphore);
}


le_result_t taf_diagDidStore::Read
(
    uint16_t dataId,
    uint8_t* dataRecordPtr,
    size_t* dataRecordSizePtr
)
{
    LE_DEBUG("taf_diagDidStore_Read! DataID: 0x%04X", dataId);

    // Check if dataRecordPtr or dataRecordSizePtr is NULL
    if(dataRecordPtr == NULL || dataRecordSizePtr == NULL)
    {
        LE_ERROR("Invalid parameter: dataRecordPtr or dataRecordSizePtr is NULL");
        return LE_FAULT;
    }

    // Get the session reference of the current client
    le_msg_SessionRef_t clientSessionRef = taf_diagDidStore_GetClientSessionRef();
    if (!clientSessionRef)
    {
        LE_ERROR("Unable to retrieve client session reference.");
        return LE_FAULT;
    }

    // Find the client-specific service object
    taf_DidStore_t* servicePtr = taf_diagDidStore::GetServiceObj(clientSessionRef);
    if (!servicePtr)
    {
        LE_ERROR("Service object not found for client session %p", clientSessionRef);
        return LE_FAULT;
    }

    // Prepare a read request
    ReadWriteRequest_t* requestPtr = (ReadWriteRequest_t*)le_mem_ForceAlloc(ReadRequestPool);
    if (!requestPtr)
    {
        LE_ERROR("Failed to allocate memory for read request.");
        return LE_FAULT;
    }

    // Initialize read request structure
    requestPtr->request = READ_REQUEST_PI;
    requestPtr->dataId = dataId;
    requestPtr->dataRecordPtr = dataRecordPtr;
    requestPtr->dataRecordSizePtr = dataRecordSizePtr;
    requestPtr->requestingThreadRef = le_thread_GetCurrent();

    // Queue the request to the read thread
    le_event_QueueFunctionToThread(ReadThreadRef, HandleReadWriteReq, requestPtr, NULL);

    // Wait for the read operation to complete
    le_clk_Time_t time = {SEM_TIME_TO_WAIT, 0};
    le_result_t ret = le_sem_WaitWithTimeOut(servicePtr->readSemaphore, time);
    if (ret != LE_OK)
    {
        LE_ERROR("Read operation timeout");
        return ret;
    }

    // Retrieve the result from the client-specific structure
    if (servicePtr->readStrg.result != 0) // Assuming 0 indicates success
    {
        LE_ERROR("Read operation failed for dataID %u, result %d", dataId,
                servicePtr->readStrg.result);
        return LE_FAULT;
    }

    // Copy the read data to the caller's buffer
    memcpy(dataRecordPtr, servicePtr->readStrg.didData, servicePtr->readStrg.didDataLen);
    *dataRecordSizePtr = servicePtr->readStrg.didDataLen;
    LE_DEBUG("Successfully read from DID 0x%04X, length: %zu", dataId, *dataRecordSizePtr);

    return LE_OK;
}


void taf_diagDidStore::didWriteCb
(
    uint16_t dataID,
    uint8_t result
)
{
    LE_DEBUG("didWriteCb!");
    auto &didStore = taf_diagDidStore::GetInstance();

    // Get the session reference of the client that initiated the write request
    le_msg_SessionRef_t clientSessionRef = taf_diagDidStore_GetClientSessionRef();
    if (!clientSessionRef)
    {
        LE_ERROR("Unable to retrieve client session reference.");
        return;
    }

    // Find the client-specific service object
    taf_DidStore_t* servicePtr = (taf_DidStore_t*)didStore.GetServiceObj(clientSessionRef);

    if (!servicePtr)
    {
        LE_ERROR("Service object not found for client session %p", clientSessionRef);
        return;
    }

    // Store the result in the client-specific structure
    servicePtr->writeResult = result;
    // Check for failure condition
    if (result != 0)
    {
        LE_ERROR("Write operation failed for DataID %u with result %d", dataID, result);
    }

    LE_DEBUG("Write operation for DataID %u completed with result %d", dataID, result);

    // Signal the client-specific semaphore
    le_sem_Post(servicePtr->writeSemaphore);
}


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
    LE_DEBUG("taf_diagDidStore_Write!");

    // Get the session reference of the current client
    le_msg_SessionRef_t clientSessionRef = taf_diagDidStore_GetClientSessionRef();
    if (!clientSessionRef)
    {
        LE_ERROR("Unable to retrieve client session reference.");
        return LE_FAULT;
    }

    // Find the client-specific service object
    taf_DidStore_t* servicePtr = GetServiceObj(clientSessionRef);

    if (!servicePtr)
    {
        LE_ERROR("Service object not found for client session %p", clientSessionRef);
        return LE_FAULT;
    }

    // Prepare a write request
    ReadWriteRequest_t* requestPtr = (ReadWriteRequest_t*)le_mem_ForceAlloc(WriteRequestPool);

    if (!requestPtr)
    {
        LE_ERROR("Failed to allocate memory for write request.");
        return LE_FAULT;
    }

    requestPtr->request = WRITE_REQUEST_PI;
    requestPtr->dataId = dataId;
    requestPtr->dataRecordPtr = (uint8_t*)dataPtr;
    requestPtr->dataRecordSizePtr = &dataSize;
    requestPtr->requestingThreadRef = le_thread_GetCurrent();

    // Queue the request to the write thread
    le_event_QueueFunctionToThread(WriteThreadRef, HandleReadWriteReq, requestPtr, NULL);

    // Wait for the write operation to complete
    le_clk_Time_t time = {SEM_TIME_TO_WAIT, 0};
    le_result_t ret = le_sem_WaitWithTimeOut(servicePtr->writeSemaphore, time);
    if (ret != LE_OK)
    {
        LE_ERROR("Write operation timeout");
        return ret;
    }

    // Retrieve the result from the client-specific structure
    if (servicePtr->writeResult != 0)
    {
        LE_ERROR("Write operation failed for dataID %u, res %d", dataId, servicePtr->writeResult);
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
    ReadWriteRequest_t* req = (ReadWriteRequest_t*)(param1Ptr);

    switch (req->request)
    {
        case READ_REQUEST_PI:
        {
            if(!didStore.didStorInf)
            {
                LE_ERROR("Plugin not initialized");
                return;
            }

            le_result_t result = (*(didStore.didStorInf->diagDIDGetAsync))
                (req->dataId, didStore.didReadCb);

            if (result != LE_OK)
            {
                LE_ERROR("Fail to get from DID Storage Plugin");
                didStore.readStrg.result = result;
            }
            break;
        }
        case WRITE_REQUEST_PI:
        {
            if(!didStore.didStorInf)
            {
                LE_ERROR("Plugin not initialized");
                return;
            }

            le_result_t result = (*(didStore.didStorInf->diagDIDSetAsync))
                (req->dataId, const_cast<uint8_t*>(req->dataRecordPtr),
                    *(req->dataRecordSizePtr), didStore.didWriteCb);
            if (result != LE_OK)
            {
                LE_ERROR("Fail to get from DID Storage Plugin");
            }
            break;
        }
        default:
            LE_ERROR("Invalid request type: %u", req->request);
            le_mem_Release(req);
            return;
    }

    le_mem_Release(req);
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
    if(servicePtr == NULL)
    {
        return NULL;
    }

    if(handlerPtr == NULL)
    {
        return NULL;
    }

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
    LE_INFO("readDataIDMsgHandler!");
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
    LE_INFO("writeDataIDMsgHandler!");
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
        LE_DEBUG("Sending positive response for writeDID req!");
        // send NRC
        result = taf_diagDataID_SendWriteDIDResp( rxMsgRef,
                TAF_DIAGDATAID_WRITE_DID_CONDITIONS_NOT_CORRECT, dataId);
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
            le_sem_Delete(servicePtr->readSemaphore);
            le_sem_Delete(servicePtr->writeSemaphore);
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

    read_semaphore = le_sem_Create("Did read Semaphore", 0);
    write_semaphore = le_sem_Create("Did write Semaphore", 0);

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
    ReadThreadRef = le_thread_Create("Background Thread", ReadThreadMain, NULL);
    le_thread_SetPriority(ReadThreadRef, LE_THREAD_PRIORITY_IDLE);
    le_thread_Start(ReadThreadRef);

    //write DID Plugin thread
    WriteRequestPool = le_mem_CreatePool("Write plugin Request", sizeof(ReadWriteRequest_t));
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
