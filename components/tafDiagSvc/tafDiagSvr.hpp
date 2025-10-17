/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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
    taf_diag_ServiceRef_t svcRef;                       ///< Own reference.
    le_msg_SessionRef_t sessionRef;                     ///< Reference to a client-server session.
    taf_diag_TesterStateHandlerRef_t testerHandlerRef;  ///< Tester state handler ref.
    le_dls_List_t supportedVlanList;                    ///< VLAN ID list.
    uint16_t targetVlanId;                              ///< Target VLAN ID
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
    taf_diag_StateChangeHandlerFunc_t func;       ///< Handler function.
    void* ctxPtr;                                 ///< Handler context.
}taf_TesterStateHandler_t;

//-------------------------------------------------------------------------------------------------
/**
 * Asynchronous callback function structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diag_ServiceRef_t svcRef;                 ///< Service reference.
    uint16_t vlanId;                              ///< VLAN Id
    taf_diag_CancelFileXferCallbackFunc_t func;   ///< Callback function.
    void* ctxPtr;                                 ///< Handler context.
    le_dls_Link_t link;                          ///< Link
}taf_CancelFileXferHandler_t;

//-------------------------------------------------------------------------------------------------
/**
 * VLAN ID structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t vlanId;                             ///< VLAN Id
    le_dls_Link_t link;                          ///< Link
}taf_DiagVlanIdNode_t;

// Diag service class
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
                        taf_diag_ServiceRef_t svcRef,
                                taf_diag_StateChangeHandlerFunc_t handlerPtr, void* contextPtr);
                void RemoveTesterStateHandler(taf_diag_TesterStateHandlerRef_t handlerRef);
                le_result_t ReleaseTesterStateMsg(taf_diag_TesterStateRef_t stateRef);

                // VLAN ID setting and selecting.
                le_result_t SetVlanId(taf_diag_ServiceRef_t svcRef, uint16_t vlanId);
                le_result_t SelectTargetVlanID(taf_diag_ServiceRef_t svcRef, uint16_t vlanId);

                // Pause/Resume diag service to receive UDS requests.
                le_result_t Pause(taf_diag_ServiceRef_t svcRef);
                le_result_t Resume(taf_diag_ServiceRef_t svcRef);
                le_result_t Shutdown(taf_diag_ServiceRef_t svcRef);

                //Asynchrous function from API direction
                void CancelFileXferAsync(taf_diag_ServiceRef_t svcRef,
                    taf_diag_CancelFileXferCallbackFunc_t callbackFuncPtr, void* contextPtr);
                //Handler function from UDS indication direction
                void CancelFileXferMsgHandler(uint16_t vlanId, le_result_t result, void* userPtr);
                //Add CancelFileXfer callback into list
                static le_result_t AddCancelFileXferCb(taf_diag_ServiceRef_t svcRef,
                        uint16_t vlanId, taf_diag_CancelFileXferCallbackFunc_t callbackFuncPtr,
                        void* contextPtr);
                //Delete CancelFileXfer callback from list
                static void DeleteCancelFileXferCb(uint16_t vlanId,
                        taf_diag_CancelFileXferCallbackFunc_t callbackFunc);
                //Find CancelFileXfer callback from list
                static taf_CancelFileXferHandler_t* FindCancelFileXferCb(uint16_t vlanId);
                static taf_CancelFileXferHandler_t* FindCancelFileXferCb(
                        taf_diag_ServiceRef_t svcRef, uint16_t vlanId);

                le_result_t RemoveSvc(taf_diag_ServiceRef_t svcRef);

                static le_dls_List_t cancelFileXferCbList;   ///< Callback list.
                static le_mutex_Ref_t cancelFileXferListCbMtx;
                static le_mem_PoolRef_t CancelFileXferCbPool;
            private:
                le_dls_List_t enableStatusList = LE_DLS_LIST_INIT;
                le_mem_PoolRef_t EnableMemPool;

                // Internal search function.
                taf_DiagSvc_t* GetServiceObj(le_msg_SessionRef_t sessionRef);
                taf_DiagSvc_t* GetServiceObj(uint16_t vlanId);

                // To clear handler list.
                void ClearHandlerList(taf_DiagSvc_t* servicePtr);
                void ClearCancelFileXferCbList(taf_DiagSvc_t* servicePtr);
                void ClearVlanList(taf_DiagSvc_t* servicePtr);

                void ClearFileXferStateList(le_dls_List_t* fileXferStateListPtr);
                le_result_t GetIfNameByVlanIdAndStateList(uint16_t vlanId,
                        le_dls_List_t* fileXferStateListPtr, char* ifNamePtr);

                uint8_t cancelFileXferRetId = 0xFC;   // CancelFileXfer result.
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

                le_mem_PoolRef_t VlanPool;

                // Event for service.
                le_event_Id_t TesterStateEvent;
                le_event_HandlerRef_t TesterStateEventHandlerRef;
        };
    }
#endif /* #ifndef TAF_DIAG_SVR_HPP */
