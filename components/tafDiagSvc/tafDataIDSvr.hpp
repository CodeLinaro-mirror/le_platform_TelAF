/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef TAF_RWDID_SVR_HPP
#define TAF_RWDID_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafDiagBackend.hpp"

#define DEFAULT_SVC_REF_CNT 16
#define DEFAULT_RX_MSG_REF_CNT 16
#define DEFAULT_RX_HANDLER_REF_CNT 16

#define MAX_READ_DID_REQ_LEN 4092
#define MIN_WRITE_DID_REQ_LEN 3
#define WRITE_DID_RESP_DATA_LEN 2
#define reqReadDIDSvcId 0x22   // ReadDID request service ID.
#define respReadDIDSvcId 0x62  // ReadDID response service ID.
#define reqWriteDIDSvcId 0x2E  // WriteDID request service ID.
#define respWriteDIDSvcId 0x6E // WriteDID response service ID.

//--------------------------------------------------------------------------------------------------
/**
 * Diag read/write DID service structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagDataID_ServiceRef_t svcRef;                          ///< own reference.
    le_dls_List_t readDIDMsgList;                                ///< Rx msg list of ReadDID svc.
    le_dls_List_t writeDIDMsgList;                               ///< Rx msg list of WriteDID svc.
    taf_diagDataID_RxReadDIDMsgHandlerRef_t readDIDHandlerRef;   ///< Handler ref of ReadDID svc.
    taf_diagDataID_RxWriteDIDMsgHandlerRef_t writeDIDHandlerRef; ///< Handler ref of WriteDID svc.
    le_msg_SessionRef_t sessionRef;                              ///< Ref to a client-svr session.
}taf_DataIDSvc_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag ReadDID service Rx message structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagDataID_RxReadDIDMsgRef_t readDIDRxMsgRef;   ///< Own reference.
    uint8_t serviceId;                                  ///< Service Identifier.
    uint16_t readDID[TAF_DIAGDATAID_MAX_READ_DID_SIZE]; ///< Read DID.
    uint16_t readDIDLen;                                ///< Read DID length.
    taf_uds_AddrInfo_t addrInfo;                        ///< Rx logical address information struct.
    le_dls_Link_t link;                                 ///< Link to the Rx message list.
}taf_ReadDIDRxMsg_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag Read DID service handler structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagDataID_RxReadDIDMsgHandlerRef_t handlerRef; ///< Own reference.
    taf_diagDataID_ServiceRef_t svcRef;                 ///< Service reference.
    taf_diagDataID_RxReadDIDMsgHandlerFunc_t func;      ///< Handler function.
    void* ctxPtr;                                       ///< Handler context.
}taf_ReadDIDHandler_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag WriteDID service Rx message structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagDataID_RxWriteDIDMsgRef_t writeDIDRxMsgRef;       ///< Own reference.
    uint8_t serviceId;                                        ///< Service Identifier.
    uint16_t writeDID;                                        ///< Write data identifier.
    uint16_t dataRecLen;                                      ///< Rx dataRec length.
    uint8_t dataRec[TAF_DIAGDATAID_MAX_DID_DATA_RECORD_SIZE]; ///< Data record.
    taf_uds_AddrInfo_t addrInfo;                              ///< Rx logical address information.
    le_dls_Link_t link;                                       ///< Link to the Rx message list.
}taf_WriteDIDRxMsg_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag WriteDID service handler structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagDataID_RxWriteDIDMsgHandlerRef_t handlerRef; ///< Own reference.
    taf_diagDataID_ServiceRef_t svcRef;                  ///< Service reference.
    taf_diagDataID_RxWriteDIDMsgHandlerFunc_t func;      ///< Handler function.
    void* ctxPtr;                                        ///< Handler context.
}taf_WriteDIDHandler_t;


//--------------------------------------------------------------------------------------------------
/**
 * Diag DID Server Service Class
 */
//--------------------------------------------------------------------------------------------------
namespace telux
{
    namespace tafsvc
    {
        class taf_DataIDSvr : public ITafSvc, public taf_UDSInterface
        {
            public:
                taf_DataIDSvr() {};
                ~taf_DataIDSvr() {}
                void Init();
                static taf_DataIDSvr& GetInstance();

                static void OnClientDisconnection(le_msg_SessionRef_t sessionRef,
                        void *contextPtr);

                // UDS message handler.
                void UDSMsgHandler(const taf_uds_AddrInfo_t* addrPtr, uint8_t sid, uint8_t* msgPtr,
                        size_t msgLen) override;

                taf_diagDataID_ServiceRef_t GetService();

                static void RxReadDIDEventHandler(void* reportPtr);
                taf_diagDataID_RxReadDIDMsgHandlerRef_t AddRxReadDIDMsgHandler(
                        taf_diagDataID_ServiceRef_t svcRef,
                                taf_diagDataID_RxReadDIDMsgHandlerFunc_t handlerPtr,
                                        void* contextPtr);
                void RemoveRxReadDIDMsgHandler(taf_diagDataID_RxReadDIDMsgHandlerRef_t handlerRef);
                le_result_t SendReadDIDResp(taf_diagDataID_RxReadDIDMsgRef_t rxMsgRef,
                        taf_diagDataID_ReadDIDErrorCode_t errCode, const uint8_t* dataPtr,
                                size_t dataSize);

                static void RxWriteDIDEventHandler(void* reportPtr);
                taf_diagDataID_RxWriteDIDMsgHandlerRef_t AddRxWriteDIDMsgHandler(
                        taf_diagDataID_ServiceRef_t svcRef,
                                taf_diagDataID_RxWriteDIDMsgHandlerFunc_t handlerPtr,
                                        void* contextPtr);
                void RemoveRxWriteDIDMsgHandler(
                        taf_diagDataID_RxWriteDIDMsgHandlerRef_t handlerRef);
                le_result_t GetWriteDataRecord(taf_diagDataID_RxWriteDIDMsgRef_t rxMsgRef,
                        uint8_t* dataRecordPtr, size_t* dataRecordSizePtr);
                le_result_t SendWriteDIDResp(taf_diagDataID_RxWriteDIDMsgRef_t rxMsgRef,
                        taf_diagDataID_WriteDIDErrorCode_t errCode, uint16_t dataId);

                le_result_t RemoveSvc(taf_diagDataID_ServiceRef_t svcRef);

            private:
                // Internal search function.
                taf_DataIDSvc_t* GetServiceObj();
                // Send NRC response msg.
                le_result_t SendNRCResp(uint8_t sid, taf_uds_AddrInfo_t*  addrInfoPtr,
                        uint8_t errCode);

                // To clear message list.
                void ClearReadDIDMsgList(taf_DataIDSvc_t* servicePtr);
                void ClearWriteDIDMsgList(taf_DataIDSvc_t* servicePtr);

                uint16_t logAddr;                 // Service logic address.

                uint8_t writeDataId[WRITE_DID_RESP_DATA_LEN];

                // Service and event object
                le_mem_PoolRef_t SvcPool;
                le_ref_MapRef_t SvcRefMap;

                // Rx message resource
                le_mem_PoolRef_t RxReadDIDMsgPool;
                le_ref_MapRef_t RxReadDIDMsgRefMap;
                le_mem_PoolRef_t RxWriteDIDMsgPool;
                le_ref_MapRef_t RxWriteDIDMsgRefMap;

                // Rx request handler object
                le_mem_PoolRef_t ReqReadDIDHandlerPool;
                le_ref_MapRef_t ReqReadDIDHandlerRefMap;
                le_mem_PoolRef_t ReqWriteDIDHandlerPool;
                le_ref_MapRef_t ReqWriteDIDHandlerRefMap;

                // Event for service.
                le_event_Id_t ReadDIDEvent;
                le_event_HandlerRef_t ReadDIDEventHandlerRef;
                le_event_Id_t WriteDIDEvent;
                le_event_HandlerRef_t WriteDIDEventHandlerRef;
        };
    }
}
#endif /* #ifndef TAF_RWDID_SVR_HPP */
