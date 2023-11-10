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

#ifndef TAF_SECURITY_SVR_HPP
#define TAF_SECURITY_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafDiagBackend.hpp"

#define DEFAULT_SVC_REF_CNT 16
#define DEFAULT_RX_MSG_REF_CNT 16
#define DEFAULT_RX_HANDLER_REF_CNT 16

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic SecurityAccess service structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagSecurity_ServiceRef_t svcRef;                   ///< Own reference.
    le_dls_List_t rxMsgList;                                ///< Rx message list of the svc.
    taf_diagSecurity_RxSecAccessMsgHandlerRef_t handlerRef; ///< Rx Message handler ref of the svc.
    le_msg_SessionRef_t sessionRef;                         ///< Client-server session reference.
}taf_SecuritySvc_t;

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic SecurityAccess service Rx message.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagSecurity_RxSecAccessMsgRef_t rxMsgRef;                 ///< Own reference.
    taf_uds_AddrInfo_t addrInfo;                                   ///< Rx logical address.
    uint8_t subFunc;                                               ///< Rx subFunction.
    uint16_t PayloadLen;                                           ///< Rx payload length.
    uint8_t Payload[TAF_DIAGSECURITY_MAX_SEC_ACCESS_PAYLOAD_SIZE]; ///< Rx payload.
    le_dls_Link_t link;                                            ///< Link to the Rx msg list.
}taf_SecAccessRxMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic SecurityAccess service handler structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagSecurity_RxSecAccessMsgHandlerRef_t handlerRef; ///< Own reference.
    taf_diagSecurity_ServiceRef_t svcRef;                   ///< Service reference.
    taf_diagSecurity_RxSecAccessMsgHandlerFunc_t func;      ///< Handler function.
    void* ctxPtr;                                           ///< Handler context.
}taf_SecAccessReqHandler_t;

// Security access service class
namespace telux {
    namespace tafsvc {
        class taf_SecuritySvr : public ITafSvc, public taf_UDSInterface
        {
            public:
                taf_SecuritySvr() {};
                ~taf_SecuritySvr() {};
                void Init();
                static taf_SecuritySvr& GetInstance();

                static void OnClientDisconnection(le_msg_SessionRef_t sessionRef,
                        void *contextPtr);

                // UDS message handler.
                void UDSMsgHandler(const taf_uds_AddrInfo_t* addrPtr, uint8_t sid, uint8_t* msgPtr,
                        size_t msgLen) override;

                taf_diagSecurity_ServiceRef_t GetService();

                static void RxSecAccessEventHandler(void* reportPtr);
                taf_diagSecurity_RxSecAccessMsgHandlerRef_t AddRxSecAccessMsgHandler(
                        taf_diagSecurity_ServiceRef_t svcRef,
                                taf_diagSecurity_RxSecAccessMsgHandlerFunc_t handlerPtr,
                                        void* contextPtr);
                void RemoveRxSecAccessMsgHandler(taf_diagSecurity_RxSecAccessMsgHandlerRef_t
                        handlerRef);

                le_result_t GetSecAccessPayloadLen(taf_diagSecurity_RxSecAccessMsgRef_t rxMsgRef,
                        uint16_t* payloadLenPtr);
                le_result_t GetSecAccessPayload(taf_diagSecurity_RxSecAccessMsgRef_t rxMsgRef,
                        uint8_t* payloadPtr, size_t* payloadSizePtr);

                le_result_t SendSecAccessResp( taf_diagSecurity_RxSecAccessMsgRef_t rxMsgRef,
                        taf_diagSecurity_SecAccessErrorCode_t errCode, const uint8_t* dataPtr,
                                size_t dataSize);

                le_result_t RemoveSvc(taf_diagSecurity_ServiceRef_t svcRef);

            private:
                // Internal search function.
                taf_SecuritySvc_t* GetServiceObj();
                // Send NRC response msg.
                le_result_t SendNRCResp(uint8_t sid, taf_uds_AddrInfo_t*  addrInfoPtr,
                        uint8_t errCode);

                uint8_t reqSecAccessSvcId = 0x27;   // Security access request service ID.
                uint8_t respSecAccessSvcId = 0x67;  // Security access response service ID.
                uint16_t logAddr;                   // Service logic address.

                // Service and event object
                le_mem_PoolRef_t SvcPool;
                le_ref_MapRef_t SvcRefMap;

                // Rx message resource
                le_mem_PoolRef_t RxSecAccessMsgPool;
                le_ref_MapRef_t RxSecAccessMsgRefMap;

                // Rx request handler object
                le_mem_PoolRef_t ReqSecAccessHandlerPool;
                le_ref_MapRef_t ReqSecAccessHandlerRefMap;

                // Event for service.
                le_event_Id_t SecAccessEvent;
                le_event_HandlerRef_t SecAccessEventHandlerRef;
        };
    }
} /* #ifndef TAF_SECURITY_SVR_HPP */
#endif
