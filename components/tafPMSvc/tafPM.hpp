/*
 *  Copyright (c) 2021-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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
#include "taf_prop_pms.h"

using namespace telux::power;
using namespace telux::common;
using namespace tafsvc;
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
#define TAF_MNGD_PM_SVC "tafMngdPMSvc"
#define TAF_RPC_PROXY "tafRpcProxy"
#define PMS_CLNTS_ACK_TIMEOUT 350
#define TAF_CONSILATED_ACK_CLNT_SIZE 100
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

typedef struct
{
    taf_pm_ConsolidatedAckInfoRef_t ref;
    taf_pm_State_t state;
    bool isAllAcked;
    taf_pm_ClientInfo_t unResponsedClntData[TAF_CONSILATED_ACK_CLNT_SIZE];
    taf_pm_ClientInfo_t nackResponsedClntData[TAF_CONSILATED_ACK_CLNT_SIZE];
    unsigned int unresponsiveClientsSize;
    unsigned int nackResponseClientsSize;
}
taf_FinalAckStatus_t;

#if defined(LE_CONFIG_ENABLE_MULTI_VM_SUPPORT)

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
    bool             ismpm;                          // variable to find MPM handler
    void *           contextPtr;                     // client context data
    le_dls_Link_t    link;                           // link to handler list
} taf_PStateHandlerCtx_t;

/**
 * @brief Consolidated state ack context struct
 *
 */
typedef struct
{
    taf_pm_ConsolidatedAckInfoHandlerRef_t handlerRef;
    taf_pm_ConsolidatedAckInfoHandlerFunc_t handlerPtr;
    void* contextPtr;
    le_msg_SessionRef_t sessionRef;
    le_dls_Link_t link;
}
taf_ConsolidatedAckInfoHandler_t;

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
            #ifndef LE_CONFIG_ENABLE_MULTI_VM_SUPPORT
            void onTcuActivityStateUpdate(telux::power::TcuActivityState state) override;
            void onSlaveAckStatusUpdate(telux::common::Status status) override;
            #endif

            #if defined(LE_CONFIG_ENABLE_MULTI_VM_SUPPORT)
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

    class taf_PmsPa {
    private:

        taf_prop_pms_MpssRef_t paPmsObject;

        taf_PmsPa() { paPmsObject = nullptr; }
        ~taf_PmsPa() { }

        taf_PmsPa(const taf_PmsPa&) = delete;
        taf_PmsPa& operator=(const taf_PmsPa&) = delete;

        static void PaPmsErrCallback
        (
            taf_prop_pms_ErrCode_t errCode,
            void * cbCtx
        );

    public:

        static taf_PmsPa* GetInstance()
        {
            static taf_PmsPa* instance = new taf_PmsPa();

            return instance;
        }

        le_result_t Init(void);
        le_result_t Deinit(void);

        le_result_t SetModemWakeupFilter
        (
            taf_pm_NodeModemWsBitMask_t bitset
        );

        le_result_t GetModemWakeupFilter
        (
            taf_pm_NodeModemWsBitMask_t* bitset
        );
    };

    // define our class to handler the call with telsdk
    class taf_PM : public ITafSvc {
    private:
        taf_pm_State_t tcuStateToTafPowerState(telux::power::TcuActivityState state);
        taf_pm_Status_t teluxStatustoTafStatus(telux::common::Status status);
        telux::power::TcuActivityState tafStateToTcuState(taf_pm_State_t tafState);
    public:
        taf_PmsPa * paRef;
        taf_PM(): paRef(taf_PmsPa::GetInstance()) {}
        ~taf_PM() { /* paRef->Deinit(); */ }
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
        static void PaInit(void *p1, void *p2);
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

        #if defined(LE_CONFIG_ENABLE_MULTI_VM_SUPPORT)
        taf_pm_State_t curTcuState;
        taf_FinalAckStatus_t consolidateStateAckInfo;
        le_mem_PoolRef_t vmListPool;
        le_mem_PoolRef_t vmInfoPool;
        le_ref_MapRef_t vmListRefMap;
        le_result_t SetPowerState(taf_pm_State_t state, const char *machineName);
        taf_pm_VMListRef_t GetMachineList();
        le_result_t GetFirstMachineName(taf_pm_VMListRef_t vmListRef,
                char* vmNamePtr, size_t vmNamePtrSize);
        le_result_t GetNextMachineName(taf_pm_VMListRef_t vmListRef,
                char* vmNamePtr, size_t vmNamePtrSize);
        le_result_t DeleteMachineList(taf_pm_VMListRef_t vmListRef);
        le_mem_PoolRef_t powerStateRefPool;
        le_ref_MapRef_t powerStateRefMap;
        le_event_Id_t stateChangeExEvent;

        le_mem_PoolRef_t ConsolidatedStateAckHandlerPool;
        le_dls_List_t ConsolidatedStateAckHandlerList;
        le_ref_MapRef_t ConsolidatedStateAckHandlerRefMap;
        le_mem_PoolRef_t ConsolidatedStateAckPool;
        le_ref_MapRef_t ConsolidatedStateAckRefMap;
        le_event_Id_t ConsolidatedAckInfoEvent;

        static void PowerStateChanged(void* reportPtr);
        void CallClientHandlerFunc(taf_pm_State_t state);
        le_mem_PoolRef_t powerStateHandlerPool;
        le_dls_List_t powerStateHandlerList;
        le_ref_MapRef_t powerStateHandlerRefMap;
        void DeletePowerStateRefs();
        void SendNackToPmd(taf_pm_State_t state);
        void SendAckToPmd(taf_pm_State_t state);
        void SendStateChangeAck(taf_pm_PowerStateRef_t powerStateRef,
        taf_pm_State_t state, taf_pm_NadVm_t vm_id, taf_pm_ClientAck_t ackType);
        taf_pm_StateChangeExHandlerRef_t AddStateChangeExHandler
                (taf_pm_StateChangeExHandlerFunc_t handlerPtr,void* contextPtr);
        void RemoveStateChangeExHandler(taf_pm_StateChangeExHandlerRef_t handlerRef);
        le_timer_Ref_t pmClientsAckTimerRef;
        taf_pm_State_t statePtr;
        static void PmsClntsAckTimerHandler(le_timer_Ref_t timerRef);
        //ConsolidatedAckInfoHandler
        taf_pm_ConsolidatedAckInfoHandlerRef_t AddConsolidatedAckInfoHandler
                (taf_pm_ConsolidatedAckInfoHandlerFunc_t handlerPtr, void* contextPtr);
        void RemoveConsolidatedAckInfoHandler(taf_pm_ConsolidatedAckInfoHandlerRef_t handlerRef);
        le_result_t GetNackClientInfo
        (
            taf_pm_ConsolidatedAckInfoRef_t consolidatedAckInfoRef,
            taf_pm_ClientInfo_t* nackClientsPtr,
            size_t* nackClientsSizePtr
        );
        le_result_t GetUnrespClientInfo
        (
            taf_pm_ConsolidatedAckInfoRef_t consolidatedAckInfoRef,
            taf_pm_ClientInfo_t* unrespClientsPtr,
            size_t* unrespClientsSizePtr
        );
        static void ConsolidatedAckInfo(void* reportPtr);
        bool IsLowPowerMode;
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

#endif /* #ifndef TAFPM_HPP */
