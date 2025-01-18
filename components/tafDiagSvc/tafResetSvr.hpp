/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef TAF_RESET_SVR_HPP
#define TAF_RESET_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafDiagBackend.hpp"

#ifndef LE_CONFIG_DIAG_FEATURE_A
#include "tafDiagSvr.hpp"
#endif

#include <string>

#define DEFAULT_SVC_REF_CNT 16
#define DEFAULT_RX_MSG_REF_CNT 16
#define DEFAULT_RX_HANDLER_REF_CNT 16

// NRC lowest value for reset service
#define ECURESET_NRC_RANGE_LOW_VALUE 0x80U

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic ECUReset service structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagReset_ServiceRef_t svcRef;           ///< Own reference.
    uint8_t resetType;                           ///< Reset type.
    le_dls_List_t rxMsgList;                     ///< Rx message list of the service.
    taf_diagReset_RxMsgHandlerRef_t handlerRef;  ///< Rx Message handler ref of the service.
    le_msg_SessionRef_t sessionRef;              ///< Reference to a client-server session.
    le_dls_List_t supportedVlanList;             ///< VLAN ID list.
}taf_ResetSvc_t;

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic ECUReset service Rx message.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagReset_RxMsgRef_t rxMsgRef;  ///< Own reference.
    taf_uds_AddrInfo_t addrInfo;        ///< Rx logical address information structure.
    uint8_t subFunc;                    ///< Rx subFunction.
    le_dls_Link_t link;                 ///< Link to the Rx message list.
}taf_ResetRxMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic ECUReset service handler structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagReset_RxMsgHandlerRef_t handlerRef;  ///< Own reference.
    taf_diagReset_ServiceRef_t svcRef;           ///< Service reference.
    taf_diagReset_RxMsgHandlerFunc_t func;       ///< Handler function.
    void* ctxPtr;                                ///< Handler context.
}taf_ResetReqHandler_t;

//-------------------------------------------------------------------------------------------------
/**
 * VLAN ID structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t   link;
    uint16_t        vlanId;
}taf_ResetVlanIdNode_t;

// ECU Reset service class
namespace telux {
    namespace tafsvc {
        class taf_ResetSvr : public ITafSvc, public taf_UDSInterface
        {
            public:
                taf_ResetSvr() {};
                ~taf_ResetSvr() {};
                void Init();
                static taf_ResetSvr& GetInstance();

                static void OnClientDisconnection(le_msg_SessionRef_t sessionRef,
                        void *contextPtr);

                // UDS message handler.
                void UDSMsgHandler(const taf_uds_AddrInfo_t* addrPtr, uint8_t sid, uint8_t* msgPtr,
                        size_t msgLen) override;

                taf_diagReset_ServiceRef_t GetService(uint8_t resetType);

                static void RxReqEventHandler(void* reportPtr);
                taf_diagReset_RxMsgHandlerRef_t AddRxMsgHandler(taf_diagReset_ServiceRef_t svcRef,
                        taf_diagReset_RxMsgHandlerFunc_t handlerPtr, void* contextPtr);
                void RemoveRxMsgHandler(taf_diagReset_RxMsgHandlerRef_t handlerRef);

                le_result_t SendResp(taf_diagReset_RxMsgRef_t rxMsgRef, uint8_t errCode);

                le_result_t RemoveSvc(taf_diagReset_ServiceRef_t svcRef);

                // VLAN ID setting/getting.
                le_result_t SetVlanId(taf_diagReset_ServiceRef_t svcRef, uint16_t vlanId);
                le_result_t GetVlanIdFromMsg(taf_diagReset_RxMsgRef_t rxMsgRef,
                    uint16_t* vlanIdPtr);
            private:
                // Internal search function.
                taf_ResetSvc_t* GetServiceObj(uint8_t resetType, uint16_t vlanId);
                taf_ResetSvc_t* GetServiceObj(uint8_t resetType, le_msg_SessionRef_t sessionRef);
                // Send NRC response msg.
                le_result_t SendNRCResp(taf_uds_AddrInfo_t*  addrInfoPtr, uint8_t errCode);

                // To clear message list.
                void ClearResetMsgList(taf_ResetSvc_t* servicePtr);
                void ClearVlanList(taf_ResetSvc_t* servicePtr);

                uint8_t reqSvcId = 0x11;   // ECUReset request service ID.
                uint8_t respSvcId = 0x51;  // ECUReset response service ID.
                uint16_t logAddr;          // Service logic address.

                // Service and event object
                le_mem_PoolRef_t SvcPool;
                le_ref_MapRef_t SvcRefMap;

                // Rx message resource
                le_mem_PoolRef_t RxMsgPool;
                le_ref_MapRef_t RxMsgRefMap;

                // Rx request handler object
                le_mem_PoolRef_t ReqHandlerPool;
                le_ref_MapRef_t ReqHandlerRefMap;

                le_mem_PoolRef_t VlanPool;

                // Event for service.
                le_event_Id_t ResetEvent;
                le_event_HandlerRef_t ResetEventHandlerRef;
        };
    }
}
#endif /* #ifndef TAF_RESET_SVR_HPP */