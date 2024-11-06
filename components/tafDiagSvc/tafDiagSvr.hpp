/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_DIAG_SVR_HPP
#define TAF_DIAG_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafDiagBackend.hpp"
#include "tafEventSvr.hpp"

#define DEFAULT_SVC_REF_CNT 16
#define DEFAULT_RX_TESTER_STATE_REF_CNT 16
#define DEFAULT_RX_HANDLER_REF_CNT 16

//-------------------------------------------------------------------------------------------------
/**
 * Enable condition status structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t  enableConditionID;
    bool  conditionFulfilled = false;
    le_dls_Link_t link;
}taf_DiagEnableStatus_t;

//-------------------------------------------------------------------------------------------------
/**
 * Service structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diag_ServiceRef_t svcRef;           ///< Own reference.
    le_msg_SessionRef_t sessionRef;         ///< Reference to a client-server session.
    le_dls_List_t testerStateHandlerList;   ///< Handler list.
}taf_DiagSvc_t;

//-------------------------------------------------------------------------------------------------
/**
 * Tester present Rx state.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diag_TesterStateRef_t rxStateRef; ///< Own reference.
    taf_uds_AddrInfo_t addrInfo;          ///< Rx logical address information structure.
    taf_diag_State_t preTesterState;      ///< Tester previous state.
    taf_diag_State_t currentTesterState;  ///< Tester current state.
}taf_RxTesterStateMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * Tester present state handler structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diag_TesterStateHandlerRef_t handlerRef;  ///< Own reference.
    taf_diag_ServiceRef_t svcRef;                 ///< Service reference.
    uint16_t vlanId;                              ///< VLAN Id
    taf_diag_StateChangeHandlerFunc_t func;       ///< Handler function.
    void* ctxPtr;                                 ///< Handler context.
    le_dls_Link_t  link;
}taf_TesterStateHandler_t;

// Diag service class
namespace telux {
    namespace tafsvc {
        class taf_DiagSvr : public ITafSvc, public taf_UDSInterface
        {
            public:
                taf_DiagSvr() {};
                ~taf_DiagSvr() {};
                void Init();
                static taf_DiagSvr& GetInstance();

                static void OnClientDisconnection(le_msg_SessionRef_t sessionRef,
                        void *contextPtr);

                le_result_t SetEnableCondition(uint8_t enableConditionID, bool conditionFulfilled);
                bool GetEnableConditionStatus(uint8_t enableConditionID);

                // UDS message handler.
                void UDSMsgHandler(const taf_uds_AddrInfo_t* addrPtr, uint8_t sid, uint8_t* msgPtr,
                        size_t msgLen) override;

                taf_diag_ServiceRef_t GetService();
                static void TesterStateEventHandler(void* reportPtr);
                taf_diag_TesterStateHandlerRef_t AddTesterStateHandler(
                        taf_diag_ServiceRef_t svcRef,  uint16_t vlanId,
                                taf_diag_StateChangeHandlerFunc_t handlerPtr, void* contextPtr);
                void RemoveTesterStateHandler(taf_diag_TesterStateHandlerRef_t handlerRef);
                le_result_t ReleaseTesterStateMsg(taf_diag_TesterStateRef_t stateRef);

                le_result_t RemoveSvc(taf_diag_ServiceRef_t svcRef);

            private:
                le_dls_List_t enableStatusList = LE_DLS_LIST_INIT;
                le_mem_PoolRef_t EnableMemPool;

                // Internal search function.
                taf_DiagSvc_t* GetServiceObj(le_msg_SessionRef_t sessionRef);
                // To clear handler list.
                void ClearHandlerList(taf_DiagSvc_t* servicePtr);

                uint8_t stateChangeId = 0xFD;   // Tester state change ID.

                // Service and event object
                le_mem_PoolRef_t SvcPool;
                le_ref_MapRef_t SvcRefMap;

                // Rx message resource
                le_mem_PoolRef_t RxMsgPool;
                le_ref_MapRef_t RxTesterStateRefMap;

                // Rx request handler object
                le_mem_PoolRef_t ReqHandlerPool;
                le_ref_MapRef_t ReqHandlerRefMap;

                // Event for service.
                le_event_Id_t TesterStateEvent;
                le_event_HandlerRef_t TesterStateEventHandlerRef;
        };
    }
}
#endif /* #ifndef TAF_DIAG_SVR_HPP */