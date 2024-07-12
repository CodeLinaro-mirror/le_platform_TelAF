/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include <vector>

namespace telux {
namespace tafsvc {

typedef struct
{
    taf_mngdPm_nodePowerStateRef_t ref;
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
        static le_result_t SuspendRpcNAD();
        static bool IsRpcConnected;
        static le_result_t StayAwakeRpcNode(taf_mngdPm_wsRef_t wsRef);
        static taf_mngdPm_wsRef_t NewRpcNodeWakeupSource( uint8_t pmNodeId, taf_mngdPm_WakeupType_t wakeupType);
        static le_result_t AcquireRpcNodeWakeLock();
        static le_result_t ReleaseRpcNodeWakeLock();
        static le_result_t RelaxRpcNode(taf_mngdPm_wsRef_t wsRef);

        static void RpcPmStateChangeExHandler(taf_rpcPm_PowerStateRef_t rpcPowerStateRef,
                taf_rpcPm_NadVm_t rpcVm_id, taf_rpcPm_State_t rpcState, void* contextPtr);

        static taf_mngdPm_NodePowerStateChangeHandlerRef_t AddRpcNodePowerStateChangeHandler(taf_mngdPm_NodePowerStateChangeHandlerFunc_t handlerFuncPtr,
               void* contextPtr, uint8_t pmNodeId,taf_mngdPm_NodePowerStateChangeBitMask_t stateMask);

        static le_result_t SendRpcNodePowerStateChangeAck (uint8_t pmNodeId,
                taf_mngdPm_nodePowerStateRef_t Ref, taf_mngdPm_NodePowerState_t state, taf_mngdPm_NodeClientAck_t ack);

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

        static taf_rpcPm_StateChangeExHandlerRef_t rpcHandlerExRef;
        static taf_rpcPm_PowerStateRef_t rpcPowerStateRef;
        std::vector<taf_mngdPm_nodePowerStateRef_t>rpcAckClientrecrd;
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
};
}
}
