/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAF_DID_STOR_HPP
#define TAF_DID_STOR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafHalLib.hpp"
#include "tafPiDiagDID.h"

#define MAX_DID_STORAGE 256
#define MAX_DATA_SIZE 256

#define DEFAULT_SVC_REF_CNT 16
#define DEFAULT_DID_HANDLER_REF_CNT 16

// DID length definition
#define DID_LEN  2

//--------------------------------------------------------------------------------------------------
/**
 * Diag DID storage service structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagDidStore_ServiceRef_t svcRef;                            ///< own reference.
    taf_diagDidStore_DataIdChangeHandlerRef_t msgDIDStorgHandlerRef; ///< Handler ref of DID notify.
    le_msg_SessionRef_t sessionRef;                                  ///< Ref to a client-svr session.
}taf_DidStore_t;

//--------------------------------------------------------------------------------------------------
/**
 * Read DidStorage structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t readDID;
    uint8_t didData;
    uint16_t didDataLen;
    uint8_t result;
}taf_ReadDidStorg_t;

//--------------------------------------------------------------------------------------------------
/**
 * Did storage notification service handler structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagDidStore_DataIdChangeHandlerRef_t handlerRef; ///< Own reference.
    taf_diagDidStore_ServiceRef_t svcRef;                 ///< Service reference.
    uint16_t did;                                         ///< Data identifier.
    taf_diagDidStore_DataIdChangeHandlerFunc_t func;      ///< Handler function.
    void* ctxPtr;                                         ///< Handler context.
    le_dls_Link_t link;                                   ///< Link to the Rx message list.
}taf_DIDStorgNotifyHandler_t;


//--------------------------------------------------------------------------------------------------
/**
 * Did storage Read/Write request structure.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    READ_REQUEST_PI,
    WRITE_REQUEST_PI
} ProcessRequest;

typedef struct {
    ProcessRequest request;
    uint16_t dataId;
    uint8_t* dataRecordPtr;
    size_t* dataRecordSizePtr;
    le_result_t result;
    le_thread_Ref_t requestingThreadRef;
} ReadWriteRequest_t;

namespace telux {
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
            static void didReadCb( uint16_t dataID,uint8_t *value,size_t len,uint8_t result);
            static void didWriteCb(uint16_t dataID, uint8_t result);
            static void DidMsgPluginCB(uint16_t dataID,uint8_t *value,size_t len);

            le_result_t Read(uint16_t dataId,
                uint8_t* dataRecordPtr,size_t* dataRecordSizePtr);
            le_result_t Write(uint16_t dataId,
                const uint8_t* dataPtr, size_t dataSize);
            static void HandleReadWriteReq(void* param1Ptr, void* param2Ptr);
            void DidMsgEventHandler(void* reportPtr);
            taf_diagDidStore_DataIdChangeHandlerRef_t AddDataIdChangeHandler(
                    taf_diagDidStore_ServiceRef_t svcRef, uint16_t dataId,
                        taf_diagDidStore_DataIdChangeHandlerFunc_t handlerPtr, void* contextPtr);
            void RemoveDataIdChangeHandler(taf_diagDidStore_DataIdChangeHandlerRef_t handlerRef);
            taf_ReadDidStorg_t readStrg;
            diagDID_Inf_t* didStorInf;
            le_ref_MapRef_t SvcRefMap;

            static void writeDataIDMsgHandler(taf_diagDataID_RxWriteDIDMsgRef_t rxMsgRef,
                uint16_t dataId, void* contextPtr);
            static void readDataIDMsgHandler(taf_diagDataID_RxReadDIDMsgRef_t rxMsgRef,
                const uint16_t* dataIdPtr, size_t dataIdSize,void* contextPtr);

        private:

            taf_DidStore_t* GetServiceObj();

            uint8_t writeDIDPIResult;

            // Service and event object
            le_mem_PoolRef_t SvcPool;

            le_mem_PoolRef_t MsgDIDStorgHandlerPool;
            le_ref_MapRef_t MsgDIDStorgHandlerRefMap;

            le_dls_List_t didStorageNotifyList = LE_DLS_LIST_INIT;

    };
}
}
#endif /* #ifndef TAF_DID_STOR_HPP */
