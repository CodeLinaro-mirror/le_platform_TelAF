/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_ROUTINE_CTRL_SVR_HPP
#define TAF_ROUTINE_CTRL_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafDiagBackend.hpp"

#ifndef LE_CONFIG_DIAG_FEATURE_A
#include "tafDiagSvr.hpp"
#endif

namespace tafsvc {
    #define TAF_DIAG_ROUTINE_CTRL_SVC_CNT   32
    #define TAF_DIAG_ROUTINE_CTRL_REQHANDLER_CNT   32
    #define TAF_DIAG_ROUTINE_CTRL_REQMSG_CNT 8
    #define TAF_DIAG_ROUTINE_CTRL_RECORD_LEN    1024
    #define TAF_DIAG_ROUTINE_CTRL_NRC_RANGE_LOW_VALUE 0x80U

    typedef struct
    {
        uint8_t type;
        void* ref;
    }taf_UdsEvent_t;

    typedef struct
    {
        taf_diagRoutineCtrl_ServiceRef_t svcRef;
        taf_diagRoutineCtrl_RxMsgHandlerFunc_t func;
        void* safeRef;
        void* ctxPtr;
    }taf_RoutineCtrlReqHandler_t;

    typedef struct
    {
        uint16_t identifier;
        //taf_diagRoutineCtrl_Type_t routineCtrlType;
        le_dls_List_t reqMsgList;
        taf_diagRoutineCtrl_RxMsgHandlerRef_t rxHandlerRef;
        le_msg_SessionRef_t sessionRef;
        taf_diagRoutineCtrl_ServiceRef_t ref;
        le_dls_List_t supportedVlanList;
    }taf_RoutineCtrlSvc_t;

    typedef struct
    {
        le_dls_Link_t link;
        taf_diagRoutineCtrl_RxMsgRef_t ref;
        taf_uds_AddrInfo_t addrInfo;
        uint8_t subFunc;
        uint16_t routineId;  // Routine identifier
        size_t recordSize;
        uint8_t recordData[TAF_DIAG_ROUTINE_CTRL_RECORD_LEN];
    }taf_RoutineCtrlReqMsg_t;

    //-------------------------------------------------------------------------------------------------
    /**
     * VLAN ID structure.
     */
    //-------------------------------------------------------------------------------------------------
    typedef struct
    {
        le_dls_Link_t   link;
        uint16_t        vlanId;
    }taf_RoutineCtrlVlanIdNode_t;

    // Implementation of Routine control service
    class taf_RoutinCtrlSvr : public ITafSvc, public taf_UDSInterface
    {
        public:
            taf_RoutinCtrlSvr() {};
            ~taf_RoutinCtrlSvr() {};
            static taf_RoutinCtrlSvr& GetInstance();
            void Init();

            static void ServiceObjDestructor(void* objPtr);
            static void RxReqEventHandler(void* reportPtr);
            static void OnClientDisconnection(le_msg_SessionRef_t sessionRef,
                                                       void *contextPtr);

            // UDS message handler. It will be called if receive this service message.
            void UDSMsgHandler(const taf_uds_AddrInfo_t* addrPtr,
                    uint8_t sid, uint8_t* msgPtr, size_t msgLen) override;

            taf_diagRoutineCtrl_ServiceRef_t FindOrCreateService(uint16_t identifier);
            le_result_t RemoveRoutineCtrlSvc(taf_diagRoutineCtrl_ServiceRef_t svcRef);
            taf_diagRoutineCtrl_RxMsgHandlerRef_t AddRxReqMsgHandler(
                    taf_diagRoutineCtrl_ServiceRef_t svcRef,
                    taf_diagRoutineCtrl_RxMsgHandlerFunc_t handlerPtr,
                    void* contextPtr);
            void RemoveRxReqMsgHandler(taf_diagRoutineCtrl_RxMsgHandlerRef_t handlerRef);
            le_result_t GetRoutineCtrlRec(taf_diagRoutineCtrl_RxMsgRef_t reqMsgRef,
                    uint8_t* optionRecPtr, size_t* optionRecSizePtr);
            le_result_t SendRoutineCtrlResp(taf_diagRoutineCtrl_RxMsgRef_t reqMsgRef,
                    uint8_t nrc, const uint8_t* dataPtr, size_t dataSize);
            le_result_t ReleaseRoutineCtrlMsg(taf_diagRoutineCtrl_RxMsgRef_t reqMsgRef);

            // VLAN ID setting/getting.
            le_result_t SetVlanId(taf_diagRoutineCtrl_ServiceRef_t svcRef, uint16_t vlanId);
            le_result_t GetVlanIdFromMsg(taf_diagRoutineCtrl_RxMsgRef_t reqMsgRef,
                uint16_t* vlanIdPtr);
        private:
            uint8_t svcId = 0x31;
            uint16_t logAddr;   // This service logic address.

            // For routine control service object
            le_mem_PoolRef_t svcPool;
            le_ref_MapRef_t svcRefMap;

            // For routine control request handler object
            le_mem_PoolRef_t reqHandlerPool;
            le_ref_MapRef_t reqHandlerRefMap;

            // Rx message resource
            le_mem_PoolRef_t reqMsgPool;
            le_ref_MapRef_t reqMsgRefMap;

            le_event_Id_t rxReqEvent;
            le_event_HandlerRef_t rxReqEventHandlerRef;

            le_mem_PoolRef_t vlanPool;

            taf_RoutineCtrlSvc_t* GetServiceObj(uint16_t identifier, uint16_t vlanId);
            taf_RoutineCtrlSvc_t* GetServiceObj(uint16_t identifier, le_msg_SessionRef_t sessionRef);
            le_result_t SendNRCResp(uint8_t sid, const taf_uds_AddrInfo_t*  addrInfoPtr,
                    uint8_t errCode);
            void ClearVlanList(taf_RoutineCtrlSvc_t* servicePtr);
    };
}
#endif