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

    // Search the service.
    taf_DidStore_t* servicePtr = GetServiceObj();

    // Create a service object if it doesn't exist in the list.
    if (servicePtr == NULL)
    {
        servicePtr = (taf_DidStore_t *)le_mem_ForceAlloc(SvcPool);
        memset(servicePtr, 0, sizeof(taf_DidStore_t));

        // Attach the service to the client.
        servicePtr->sessionRef = taf_diagDidStore_GetClientSessionRef();

        // Create a Safe Reference for this service object
        servicePtr->svcRef = (taf_diagDidStore_ServiceRef_t)le_ref_CreateRef(SvcRefMap,
                servicePtr);

        LE_DEBUG("svcRef %p of client %p is created for DataIdStor.",
                servicePtr->svcRef, servicePtr->sessionRef);
    }
    else
    {
        // Only the service owner app can get the service reference for subsequent operations.
        if (servicePtr->sessionRef != taf_diagDidStore_GetClientSessionRef())
        {
            LE_ERROR("The service is created by other client.");
            return NULL;
        }
    }

    LE_INFO("Get serviceRef %p for Diag DataId service.", servicePtr->svcRef);

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
)
{
    LE_DEBUG("find the service object!");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_DidStore_t* servicePtr = (taf_DidStore_t *)le_ref_GetValue(iterRef);
        if (servicePtr != NULL)
        {
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
    auto &didStore = taf_diagDidStore::GetInstance();

    didStore.readStrg.readDID = dataID;

    if(value == NULL)
    {
        LE_INFO("No value found to read from DID");
        return;
    }

    if (len == 0 || len > sizeof(didStore.readStrg.didData))
    {
        LE_ERROR("Invalid length");
        return;
    }

    memcpy(didStore.readStrg.didData, value, len);
    didStore.readStrg.didDataLen = len;
    didStore.readStrg.result = result;

    le_sem_Post(read_semaphore);
}


le_result_t taf_diagDidStore::Read
(
    uint16_t dataId,
        ///< [IN] Data identifier.
    uint8_t* dataRecordPtr,
        ///< [OUT] Data record.
    size_t* dataRecordSizePtr
        ///< [INOUT]
)
{
    LE_DEBUG("taf_diagDidStore_Read!");

    ReadWriteRequest_t* requestPtr = (ReadWriteRequest_t*)le_mem_ForceAlloc(ReadRequestPool);

    if (!requestPtr) {

        return LE_FAULT;

    }

    requestPtr->request = READ_REQUEST_PI;
    requestPtr->dataId = dataId;
    requestPtr->dataRecordPtr = dataRecordPtr;
    requestPtr->dataRecordSizePtr = dataRecordSizePtr;
    requestPtr->requestingThreadRef = le_thread_GetCurrent();
    le_event_QueueFunctionToThread(ReadThreadRef, HandleReadWriteReq, requestPtr, NULL);
    le_sem_Wait(read_semaphore);
    memcpy(dataRecordPtr, readStrg.didData, readStrg.didDataLen);
    *dataRecordSizePtr = readStrg.didDataLen;

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

    didStore.writeDIDPIResult = result;
    LE_DEBUG("result %d", result);

    le_sem_Post(write_semaphore);
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

    ReadWriteRequest_t* requestPtr = (ReadWriteRequest_t*)le_mem_ForceAlloc(WriteRequestPool);

    if (!requestPtr) {

        return LE_FAULT;

    }

    requestPtr->request = WRITE_REQUEST_PI;
    requestPtr->dataId = dataId;
    requestPtr->dataRecordPtr = (uint8_t*)dataPtr;
    requestPtr->dataRecordSizePtr = &dataSize;
    requestPtr->requestingThreadRef = le_thread_GetCurrent();

    le_event_QueueFunctionToThread(WriteThreadRef, HandleReadWriteReq, requestPtr, NULL);
    le_sem_Wait(write_semaphore);

    return (le_result_t)writeDIDPIResult;

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
        if(taf_diagDataID_SendReadDIDResp( rxMsgRef, TAF_DIAGDATAID_READ_DID_CONDITIONS_NOT_CORRECT,
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
                TAF_DIAGDATAID_READ_DID_CONDITIONS_NOT_CORRECT, NULL, 0);
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
