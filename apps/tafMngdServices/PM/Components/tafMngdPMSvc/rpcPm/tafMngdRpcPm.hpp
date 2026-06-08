/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include <vector>

#define RPC_CONNECT_TIMEOUT 5000

namespace tafsvc {

typedef struct
{
    taf_mngdPm_nodePowerStateRef_t nodeStateRef;
    le_msg_SessionRef_t sessionRef;
    taf_mngdPm_NodePowerState_t state;
}taf_rpcPm_NodePowerStateChange_t;

class tafMngdRpcPm: public ITafSvc
{
    public:
        tafMngdRpcPm() {};
        ~tafMngdRpcPm() {};

        void Init(void);
        static tafMngdRpcPm &GetInstance();
        static le_result_t ShutdownRpcNAD();
        static le_result_t  RestartRpcNAD();
        static le_result_t SuspendRpcNAD();
        static bool IsRpcConnected;
        static le_result_t StayAwakeRpcNode(taf_mngdPm_wsNodeRef_t wsRef);
        static taf_mngdPm_wsNodeRef_t CreateRpcNodeWakeupSource(uint8_t pmNodeId, taf_mngdPm_WsOpt_t option, const char* wsTag);
        static le_result_t DeleteRpcNodeWakeupSource(taf_mngdPm_wsNodeRef_t wsRef);
        static le_result_t AcquireRpcNodeWakeLock();
        static le_result_t ReleaseRpcNodeWakeLock();
        static le_result_t RelaxRpcNode(taf_mngdPm_wsNodeRef_t wsRef);

        static void RpcPmStateChangeExHandler(taf_rpcPm_PowerStateRef_t rpcPowerStateRef,
                taf_rpcPm_NadVm_t rpcVm_id, taf_rpcPm_State_t rpcState, void* contextPtr);

        static taf_mngdPm_NodePowerStateChangeHandlerRef_t AddRpcNodePowerStateChangeHandler(taf_mngdPm_NodePowerStateChangeHandlerFunc_t handlerFuncPtr,
               void* contextPtr, uint8_t pmNodeId,taf_mngdPm_NodePowerStateChangeBitMask_t stateMask);

        static le_result_t SendRpcNodePowerStateChangeAck (uint8_t pmNodeId,
                taf_mngdPm_nodePowerStateRef_t Ref, taf_mngdPm_NodeClientAck_t ack);

        static void SendAckToRpcPms(taf_mngdPm_NodePowerState_t state, taf_rpcPm_ClientAck_t ackType);

        static void DeleteRpcNodePowerStateRefs();

        static void RpcNodePowerStateChanged(void* reportPtr);

        static void RemoveRpcNodePowerStateChangeHandler(taf_mngdPm_NodePowerStateChangeHandlerRef_t handlerRef);

        // resources to communicate with rpc PMS
        static taf_rpcPm_WakeupSourceRef_t rpcWs;
        static le_mem_PoolRef_t rpcWsRefPool;
        static le_dls_List_t rpcWsRefList;
        static le_ref_MapRef_t rpcWsRefMap;
        static uint8_t rpcWsCount;

        //Rpc StateChangeExHandler
        static taf_rpcPm_StateChangeExHandlerRef_t rpcHandlerExRef;
        static taf_rpcPm_PowerStateRef_t rpcPowerStateRef;
        static int8_t ackRpcClientrecrdSize;
        std::vector<taf_rpcPm_NodePowerStateChange_t>rpcRegClientrecrd;

        //Node Power State change handler
        static le_event_Id_t rpcNodePowerStateChange;
        static le_mem_PoolRef_t rpcNodePowerStateHandlerPool;
        static le_dls_List_t rpcNodePowerStateHandlerList;
        static le_ref_MapRef_t rpcNodePowerStateHandlerMap;
        static le_mem_PoolRef_t rpcNodePowerStateRefPool;
        static le_ref_MapRef_t rpcNodePowerStateRefMap;
        static int8_t clientSize;
        static taf_mngdPm_TargetedPowerMode_t rpcTargetedPowerMode;

        //RPC Retry
        static le_timer_Ref_t RpcConnectTimerRef;
        static int RpcRetryCount;
        static void RpcConnectTimerHandler(le_timer_Ref_t timerRef);
        static void TryConnectService();
        static void RpcDisconnectHandler(void * contextptr);
        //Multiclient data
         static void OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *ctxPtr);
};
}
