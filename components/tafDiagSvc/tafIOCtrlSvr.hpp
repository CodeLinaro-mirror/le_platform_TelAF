/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_IO_CTRL_SVR_HPP
#define TAF_IO_CTRL_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafDiagBackend.hpp"

#ifndef LE_CONFIG_DIAG_FEATURE_A
#include "tafDiagSvr.hpp"
#endif

#define DEFAULT_SVC_REF_CNT 16
#define DEFAULT_RX_MSG_REF_CNT 16
#define DEFAULT_RX_HANDLER_REF_CNT 16

// NRC lowest value for IOCtrl service
#define IO_CONTROL_NRC_RANGE_LOW_VALUE 0x80U

//--------------------------------------------------------------------------------------------------
/**
 * Diag IOCtrl service structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagIOCtrl_ServiceRef_t svcRef;            ///< Own reference.
    uint16_t dataID;                               ///< Data identifier
    le_dls_List_t reqMsgList;                      ///< Rx msg list.
    taf_diagIOCtrl_RxMsgHandlerRef_t handlerRef;   ///< Handler reference.
    le_msg_SessionRef_t sessionRef;                ///< Ref to a client-svr session.
    le_dls_List_t supportedVlanList;
}taf_IOCtrlSvc_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag IOCtrl service Rx message structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagIOCtrl_RxMsgRef_t rxMsgRef;                           ///< Own reference.
    uint8_t serviceId;                                            ///< Service Identifier.
    uint16_t dataID;                                              ///< Data identifier.
    uint8_t ioCtrlParameter;                                      ///< IO control parameter.
    uint8_t controlState[TAF_DIAGIOCTRL_MAX_CONTROL_RECORD_SIZE]; ///< Control state.
    uint16_t controlStateSize;                                    ///< Control state length.
    uint8_t maskRecord[TAF_DIAGIOCTRL_MAX_CONTROL_RECORD_SIZE];   ///< Mask record.
    uint16_t maskRecordSize;                                      ///< Mask record length.
    taf_uds_AddrInfo_t addrInfo;                                  ///< Rx LA information struct.
    le_dls_Link_t link;                                           ///< Link to the Rx msg list.
}taf_IOCtrlRxMsg_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag IOCtrl service handler structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagIOCtrl_RxMsgHandlerRef_t handlerRef; ///< Own reference.
    taf_diagIOCtrl_ServiceRef_t svcRef;          ///< Service reference.
    taf_diagIOCtrl_RxMsgHandlerFunc_t func;      ///< Handler function.
    void* ctxPtr;                                ///< Handler context.
}taf_IOCtrlHandler_t;

//-------------------------------------------------------------------------------------------------
/**
 * VLAN ID structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t   link;
    uint16_t        vlanId;
}taf_IOCtrlVlanIdNode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag IOCtrl Server Service Class
 */
//--------------------------------------------------------------------------------------------------
    namespace tafsvc
    {
        class taf_IOCtrlSvr : public ITafSvc, public taf_UDSInterface
        {
            public:
                taf_IOCtrlSvr() {};
                ~taf_IOCtrlSvr() {}
                void Init();
                static taf_IOCtrlSvr& GetInstance();

                static void OnClientDisconnection(le_msg_SessionRef_t sessionRef,
                        void *contextPtr);

                // UDS message handler.
                void UDSMsgHandler(const taf_uds_AddrInfo_t* addrPtr, uint8_t sid, uint8_t* msgPtr,
                        size_t msgLen) override;

                taf_diagIOCtrl_ServiceRef_t GetService(uint16_t dataId);

                static void RxIOCtrlEventHandler(void* reportPtr);
                taf_diagIOCtrl_RxMsgHandlerRef_t AddRxMsgHandler(
                        taf_diagIOCtrl_ServiceRef_t svcRef,
                                taf_diagIOCtrl_RxMsgHandlerFunc_t handlerPtr,
                                        void* contextPtr);
                void RemoveRxMsgHandler(taf_diagIOCtrl_RxMsgHandlerRef_t handlerRef);
                le_result_t GetCtrlState(taf_diagIOCtrl_RxMsgRef_t rxMsgRef,
                        uint8_t* controlStatePtr, size_t* controlStateSizePtr);
                le_result_t GetCtrlEnableMaskRecd(taf_diagIOCtrl_RxMsgRef_t rxMsgRef,
                        uint8_t* maskRecordPtr, size_t* maskRecordSizePtr);
                le_result_t SendResp(taf_diagIOCtrl_RxMsgRef_t rxMsgRef,
                        uint8_t errCode, const uint8_t* dataPtr, size_t dataSize);

                le_result_t RemoveSvc(taf_diagIOCtrl_ServiceRef_t svcRef);
                le_result_t SetVlanId(taf_diagIOCtrl_ServiceRef_t svcRef, uint16_t vlanId);
                le_result_t GetVlanIdFromMsg(taf_diagIOCtrl_RxMsgRef_t reqMsgRef,
                        uint16_t* vlanIdPtr);
            private:
                // Internal search function.
                taf_IOCtrlSvc_t* GetServiceObj(uint16_t dataId, le_msg_SessionRef_t sessionRef);
                taf_IOCtrlSvc_t* GetServiceObj(uint16_t dataId, uint16_t vlanId);
                // Send NRC response msg.
                le_result_t SendNRCResp(uint8_t sid, const taf_uds_AddrInfo_t*  addrInfoPtr,
                        uint8_t errCode);

                // To clear message list.
                void ClearMsgList(taf_IOCtrlSvc_t* servicePtr);
                void ClearVlanList(taf_IOCtrlSvc_t* servicePtr);

                uint8_t reqIOCtrlSvcId = 0x2F;   // IOCtrl request service ID.
                uint8_t respIOCtrlSvcId = 0x6F;  // IOCtrl response service ID.
                uint16_t logAddr;                // Service logic address.

                // Service and event object
                le_mem_PoolRef_t SvcPool;
                le_ref_MapRef_t SvcRefMap;

                // Rx message resource
                le_mem_PoolRef_t RxMsgPool;
                le_ref_MapRef_t RxMsgRefMap;

                // Rx request handler object
                le_mem_PoolRef_t ReqHandlerPool;
                le_ref_MapRef_t ReqHandlerRefMap;
                le_mem_PoolRef_t vlanPool;

                // Event for service.
                le_event_Id_t ReqEvent;
                le_event_HandlerRef_t ReqEventHandlerRef;
        };
    }
#endif /* #ifndef TAF_IO_CTRL_SVR_HPP */
