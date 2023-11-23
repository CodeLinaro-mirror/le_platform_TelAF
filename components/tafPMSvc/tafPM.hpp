/*
 * Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef TAFPM_HPP
#define TAFPM_HPP

#include "legato.h"
#include "interfaces.h"
#include <telux/power/PowerFactory.hpp>
#include <telux/power/TcuActivityDefines.hpp>
#include <telux/power/TcuActivityListener.hpp>
#include <telux/power/TcuActivityManager.hpp>
#include "tafSvcIF.hpp"

using namespace telux::power;
using namespace telux::common;
using namespace telux::tafsvc;
using namespace std;

/**
 * Telaf's prefix for wakeup source names
 */
#define TAF_TAG_PREFIX   "taf"

#define TAF_WS_NAME_FORMAT TAF_TAG_PREFIX"_%s_%p"
#define TAF_WS_PROCNAME_LEN 30
#define TAF_WS_NAME_LEN (sizeof(TAF_TAG_PREFIX) + TAF_PM_TAG_LEN + TAF_WS_PROCNAME_LEN + 3)

/**
 * Memory pool sizes
 */
#define TAF_PM_CLIENT_DEFAULT_POOL_SIZE 8
#define TAF_PM_CLIENT_DEFAULT_HASH_SIZE 31
#define TAF_WAKEUP_SOURCE_DEFAULT_POOL_SIZE 64
#define TAF_PM_REFERENCE_DEFAULT_POOL_SIZE   31
#define TAF_POWER_SOURCE_DEFAULT_POOL_SIZE 64
#define TAF_MNGD_PM_SERVICE 13
/**
 * Wakeup source record definition
 */
typedef struct
{
    uint32_t      cookie;
    char          name[TAF_WS_NAME_LEN];
    pid_t         clientPid;
    le_msg_SessionRef_t sessionRef;
    uint32_t      acquired;
    bool          isRef;
    void          *wsRef;
}
taf_ws_t;

#if defined(TARGET_SA525M)

#define TAF_PM_VM_LIST_POOL_SIZE   5
#define TAF_PM_VM_INFO_POOL_SIZE   5
#define SET_STATE_TIMEOUT 5
#define SIZE_OF_STRING_ALL_MACHINES 13

/*
 * @brief The struct of virtual machine info.
 */
typedef struct
{
    char          name[TAF_PM_MACHINE_NAME_LEN];
    le_sls_Link_t link;
} taf_PMVmInfo_t;

/*
 * @brief The struct of virtual machine list.
 */
typedef struct
{
    le_sls_List_t VmInfoList;
    le_msg_SessionRef_t sessionRef;
    le_sls_Link_t* currPtr;
    taf_pm_VMListRef_t ref;
} taf_PMVmList_t;

/*
 * @brief The struct of Power state Ref list.
 */
typedef struct
{
    taf_pm_PowerStateRef_t pStateRef;
} taf_PmPowerStateRef_t;

/*
 * @brief To store the handlerptr of clients .
 */
typedef struct
{
    taf_pm_StateChangeExHandlerRef_t handlerRef;     // this handler ref
    taf_pm_StateChangeExHandlerFunc_t handlerPtr;    // this function ptr
    bool             ismpm;                          // Var to find MPM handler
    void *           contextPtr;                     // clientt context data
    le_dls_Link_t    link;                           // link to handler list
} taf_PStateHandlerCtx_t;
#endif

#define TAF_PM_WAKEUP_SOURCE_COOKIE 0xa1f6337b

/**
 * Client record definition
 */
typedef struct
{
    uint32_t cookie;
    pid_t procId;
    le_msg_SessionRef_t sessionRef;
    char name[TAF_WS_PROCNAME_LEN + 1];
}
taf_Client_t;

#define TAF_PM_CLIENT_COOKIE 0x7732c691

#define TAF_PM_TIMEOUT 10 * 60         // 10 mins

namespace telux {
namespace tafsvc {

    typedef struct
    {
        taf_pm_State_t state;
    }
    stateEvent_t;

    /**
     * Global power manager record
     */
    typedef struct taf_powerManager_record
    {
        int                 wsAcquired;
        le_ref_MapRef_t     refs;
        le_mem_PoolRef_t    clientpool;
        le_hashmap_Ref_t    clients;
        le_mem_PoolRef_t    lockpool;
        le_hashmap_Ref_t    locks;
    }
    taf_powerManager_t;

    // define the callback class for TCU state change of local proc
    class tafTcuStateListener : public telux::power::ITcuActivityListener {
        public :
            #if defined(TARGET_SA515M)
            void onTcuActivityStateUpdate(telux::power::TcuActivityState state) override;
            void onSlaveAckStatusUpdate(telux::common::Status status) override;
            #endif

            #if defined(TARGET_SA525M)
            void onTcuActivityStateUpdate(TcuActivityState state, string machineName) override;
            void onMachineUpdate(const string machineName,
                    const MachineEvent machineEvent) override;
            void onSlaveAckStatusUpdate(const Status status,
                    const string machineName, const vector<ClientInfo> unresponsiveClients,
                    const vector<ClientInfo> nackResponseClients) override;
            #endif
    };

    // define the callback class for TCU state change of remote proc
    class tafRemoteTcuStateListener : public telux::power::ITcuActivityListener {
        public :
            void onTcuActivityStateUpdate(telux::power::TcuActivityState state) override;
            void onSlaveAckStatusUpdate(telux::common::Status status) override;
    };

    // define the callback class for Service status for TelSDK
    class tafTcuServiceStatusListener : public telux::common::IServiceStatusListener {
        public:
            void onServiceStatusChange(telux::common::ServiceStatus status) override;
    };

    // define our class to handler the call with telsdk
    class taf_PM : public ITafSvc {
    private:
        taf_pm_State_t tcuStateToTafPowerState(telux::power::TcuActivityState state);
        taf_pm_Status_t teluxStatustoTafStatus(telux::common::Status status);
        telux::power::TcuActivityState tafStateToTcuState(taf_pm_State_t tafState);
    public:
        taf_PM() {};
        ~taf_PM() {};
        std::shared_ptr<telux::power::ITcuActivityManager> tcuActivityMgr;
        std::shared_ptr<telux::power::ITcuActivityManager> tcuSlaveActivityMgr;
        std::shared_ptr<telux::power::ITcuActivityManager> RemoteTcuActivityMgr = nullptr;
        std::shared_ptr<telux::power::ITcuActivityListener> tcuStateListener;
        std::shared_ptr<telux::power::ITcuActivityListener> tcuSlaveStateListener;
        std::shared_ptr<telux::power::ITcuActivityListener> remoteTcuStateListener;
        std::shared_ptr<telux::common::IServiceStatusListener> tcuServiceStatusListener;
        le_event_Id_t StateChangeEvent;
        le_event_Id_t AckEvent;
        static taf_PM &GetInstance();
        static void StateChanged(void* reportPtr, void* SecondLayeredHandlerFunc);
        static void TafSigTermEventHandler(int tafSigNum);
        static void sendAck(void* reportPtr);
        void Init(void);
        static taf_Client_t *to_taf_Client_t(void *c);
        static taf_ws_t *ToTafWakeupSource(taf_pm_WakeupSourceRef_t w);
        taf_pm_WakeupSourceRef_t NewWakeupSource( uint32_t opts, const char *tag);
        le_result_t StayAwake( taf_pm_WakeupSourceRef_t w);
        le_result_t Relax( taf_pm_WakeupSourceRef_t w);
        taf_pm_State_t GetPowerState();
        const char* tcuStateToString(telux::power::TcuActivityState state);
        taf_pm_StateChangeHandlerRef_t AddStateChangeHandler
                (taf_pm_StateChangeHandlerFunc_t handlerPtr, void* contextPtr);
        void RemoveStateChangeHandler(taf_pm_StateChangeHandlerRef_t handlerRef);
        #if defined(TARGET_SA525M)
        taf_pm_State_t curTcuState;
        le_mem_PoolRef_t vmListPool;
        le_mem_PoolRef_t vmInfoPool;
        le_ref_MapRef_t vmListRefMap;
        le_mem_PoolRef_t powerStateRefPool;
        le_ref_MapRef_t powerStateRefMap;
        le_event_Id_t stateChangeExEvent;
        static void PowerStateChanged(void* reportPtr);
        void CallClientHandlerFunc(taf_pm_State_t state);
        le_mem_PoolRef_t powerStateHandlerPool;
        le_dls_List_t powerStateHandlerList;
        le_ref_MapRef_t powerStateHandlerRefMap;
        le_result_t SetPowerState(taf_pm_State_t state, const char *machineName);
        taf_pm_VMListRef_t GetMachineList();
        le_result_t GetFirstMachineName(taf_pm_VMListRef_t vmListRef,
                char* vmNamePtr, size_t vmNamePtrSize);
        le_result_t GetNextMachineName(taf_pm_VMListRef_t vmListRef,
                char* vmNamePtr, size_t vmNamePtrSize);
        le_result_t DeleteMachineList(taf_pm_VMListRef_t vmListRef);
        void DeletePowerStateRefs();
        void SendNackToPmd(taf_pm_State_t state);
        void SendAckToPmd(taf_pm_State_t state);
        void SendStateChangeAck(taf_pm_PowerStateRef_t powerStateRef,
        taf_pm_State_t state, taf_pm_NadVm_t vm_id, taf_pm_ClientAck_t AckType);
        taf_pm_StateChangeExHandlerRef_t AddStateChangeExHandler
                (taf_pm_StateChangeExHandlerFunc_t handlerPtr,void* contextPtr);
        void RemoveStateChangeExHandler(taf_pm_StateChangeExHandlerRef_t handlerRef);
        #endif
    };

    class taf_Handler : public ITafSvc {
        public:
            void Init(void);

            taf_Handler();
            ~taf_Handler();

            //taf service handlers
            static void OnClientConnection(le_msg_SessionRef_t sessionRef, void *contextPtr);
            static void OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *contextPtr);

            //callback for TCU cmd status
            static void commandCallback(telux::common::ErrorCode errorCode);
    };
}
}

#endif /* #ifndef TAFPM_HPP */
