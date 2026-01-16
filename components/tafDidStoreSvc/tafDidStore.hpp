/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAF_DID_STOR_HPP
#define TAF_DID_STOR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafHalLib.hpp"
#include "tafPiDiagDID.h"

#include <atomic>

#include "limit.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <vector>
#include <cstring>

#define DEFAULT_DID_STORE_CONFIG_PATH "./tafDidStore.json"
#define MAX_DID_STORAGE 256
#define MAX_DATA_SIZE 256

#define DEFAULT_SVC_REF_CNT 16
#define DEFAULT_DID_HANDLER_REF_CNT 16
#define DEFAULT_READ_DID_REF_CNT 16
#define DEFAULT_WRITE_DID_REF_CNT 16

// DID length definition
#define DID_LEN  2

#define TAF_REQ_OUT_OF_RANGE 0x31
#define TAF_CONDITION_NOT_CORRECT 0x22
#define TAF_GENERAL_PROGRAMMING_FAILURE 0x72

// Semaphore wait time
#define SEM_TIME_TO_WAIT 5

//--------------------------------------------------------------------------------------------------
/**
 * Read DidStorage structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t readDID;
    uint8_t didData[TAF_DIAGDIDSTORE_MAX_DID_DATA_RECORD_SIZE];
    uint16_t didDataLen;
    uint8_t result;
}taf_ReadDidStorg_t;


//--------------------------------------------------------------------------------------------------
/**
 * Diag DID storage service structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagDidStore_ServiceRef_t svcRef;                            ///< own reference.
    taf_diagDidStore_DataIdChangeHandlerRef_t msgDIDStorgHandlerRef; ///< HandlerRef of DID notify.
    le_msg_SessionRef_t sessionRef;                                  ///< Client-svr session ref.
    le_dls_List_t list;                                              ///< Link for dynamic list.
}taf_DidStore_t;


//--------------------------------------------------------------------------------------------------
/**
 * Did storage notification service handler structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagDidStore_DataIdChangeHandlerRef_t handlerRef; ///< Own reference.
    taf_diagDidStore_ServiceRef_t svcRef;                 ///< Service reference.
    le_dls_List_t dataIdList;                             ///< Data ID list.
    taf_diagDidStore_DataIdChangeHandlerFunc_t func;      ///< Handler function.
    void* ctxPtr;                                         ///< Handler context.
    le_dls_Link_t link;                                   ///< Link to the Rx message list.
}taf_DIDStorgNotifyHandler_t;

//--------------------------------------------------------------------------------------------------
/**
 * DID list to notify on change.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;
    uint16_t      dataId;
}taf_DataID_t;

//--------------------------------------------------------------------------------------------------
/**
 * DID request type.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    READ_REQUEST_PI,
    WRITE_REQUEST_PI
} ProcessRequest;

typedef enum
{
    EVT_LOAD_PLUG_IN_READY = 0,
    EVT_CONNECT_DIAG_SVC_READY = 1
} EventType_Ready_t;

typedef struct
{
    EventType_Ready_t type;
}taf_didStore_ReadyEvtType_t;

typedef struct {
    ProcessRequest request;
    uint16_t dataId;
    uint8_t* dataRecordPtr;
    size_t* dataRecordSizePtr;
    le_result_t result;
    void* readDIDRef;
    void* writeDIDRef;
    le_thread_Ref_t requestingThreadRef;
} ReadWriteRequest_t;

//--------------------------------------------------------------------------------------------------
/**
 * DID Storage access configuration.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char AppName[LIMIT_MAX_APP_NAME_LEN];
    std::vector<uint16_t> WriteAccessDID;
    std::vector<uint16_t> ReadAccessDID;
}tafDidStore_Config_t;

namespace tafsvc {
class taf_diagDidStore: public ITafSvc
    {
        public:
            taf_diagDidStore() {};
            ~taf_diagDidStore() {};
            static taf_diagDidStore& GetInstance();
            void Init();
            static void OnClientDisconnection(le_msg_SessionRef_t sessionRef,
                        void *contextPtr);
            taf_diagDidStore_ServiceRef_t GetService(void);

            static void didReadCb( uint16_t dataID, uint8_t* dataRecPtr, size_t len,
                    uint8_t result, void* contextPtr);
            static void didWriteCb(uint16_t dataID, uint8_t result, void* contextPtr);
            static void DidMsgPluginCB(uint16_t dataID,uint8_t *value, size_t len);

            le_result_t Read(uint16_t dataId, uint8_t* dataRecordPtr, size_t* dataRecordSizePtr);
            le_result_t Write(uint16_t dataId, const uint8_t* dataPtr, size_t dataSize);

            static void HandleReadWriteReq(void* param1Ptr, void* param2Ptr);
            void DidMsgEventHandler(void* reportPtr);
            taf_diagDidStore_DataIdChangeHandlerRef_t AddDataIdChangeHandler(
                    taf_diagDidStore_ServiceRef_t svcRef, uint16_t dataId,
                        taf_diagDidStore_DataIdChangeHandlerFunc_t handlerPtr, void* contextPtr);
            void RemoveDataIdChangeHandler(taf_diagDidStore_DataIdChangeHandlerRef_t handlerRef);
            taf_diagDidStore_DIDChangeHandlerRef_t GetDIDHandlerRef(
                    taf_diagDidStore_ServiceRef_t svcRef);
            le_result_t AddDIDToHandler(taf_diagDidStore_DIDChangeHandlerRef_t handlerRef,
                    uint16_t dataId);
            le_result_t RemoveDIDFromHandler(taf_diagDidStore_DIDChangeHandlerRef_t handlerRef,
                    uint16_t dataId);

            le_result_t ParseDidStoreJsonConfig(const char* configPathPtr);
            std::vector<tafDidStore_Config_t> dataIdStoreAccessCfg;

            le_result_t GetAppNameBySessionRef(le_msg_SessionRef_t clientSessionRef,
                    char *appNameStr, size_t appNameSize);
            bool IsReadAppAccessible(uint16_t dataId, const char* appName);
            bool IsWriteAppAccessible(uint16_t dataId, const char* appName);

            diagDID_Inf_t* didStorInf;
            le_ref_MapRef_t SvcRefMap;

            // DTOOL read/write DID request handler
            static void writeDataIDMsgHandler(taf_diagDataID_RxWriteDIDMsgRef_t rxMsgRef,
                    uint16_t dataId, void* contextPtr);
            static void readDataIDMsgHandler(taf_diagDataID_RxReadDIDMsgRef_t rxMsgRef,
                    const uint16_t* dataIdPtr, size_t dataIdSize,void* contextPtr);

            le_event_Id_t evtReady;
            bool isPluginReady = false;
            bool isDiagSvcReady = false;
            static void GetSvcReady(void *p1, void *p2);
            void LoadPlugin();
            void ConnectDiagSvc();
            static void RetryHandler(le_timer_Ref_t timerRef);
            static void ReadyEvtHandler(void * reportPtr);

            static taf_diagDataID_ServiceRef_t DiagDataIDSvcRef;
            static taf_diagDataID_RxReadDIDMsgHandlerRef_t DiagReadDataIDMsgRef;
            static taf_diagDataID_RxWriteDIDMsgHandlerRef_t DiagWriteDataIDMsgRef;

        private:
            // Internal search function.
            taf_DidStore_t* GetServiceObj(le_msg_SessionRef_t sessionRef);

            // Store Read/write plugin result data.
            taf_ReadDidStorg_t readStrg;
            uint8_t writeDIDPIResult;

            // Service and event object
            le_mem_PoolRef_t SvcPool;

            std::atomic<bool> isReadDIDLock = {false};
            std::atomic<bool> isWriteDIDLock = {false};

            le_ref_MapRef_t ReadDIDRefMap;
            le_ref_MapRef_t WriteDIDRefMap;

            le_mem_PoolRef_t MsgDIDStorgHandlerPool;
            le_ref_MapRef_t MsgDIDStorgHandlerRefMap;
            le_mem_PoolRef_t dataIdPool;

            le_mem_PoolRef_t WriteRequestPool;
            le_mem_PoolRef_t ReadRequestPool;

            le_thread_Ref_t ReadThreadRef;
            le_thread_Ref_t WriteThreadRef;
    };
}
#endif /* #ifndef TAF_DID_STOR_HPP */
