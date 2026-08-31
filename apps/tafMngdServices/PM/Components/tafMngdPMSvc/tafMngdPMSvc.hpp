/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafHalPM.h"
#include "tafHalLib.hpp"
#include <vector>
#include <sys/reboot.h>
#include <bitset>
#include <unordered_set>

#define VEHICHLE_WAKEUP_REASON_DEFAULT 0
#define VEHICHLE_WAKEUP_STATUS_AWAKE  0
#define VEHICHLE_WAKEUP_STATUS_INVALID_REQ 1
#define VEHICHLE_WAKEUP_STATUS_UNKNOWN 2
#define TAF_MNGDPM_VM_HASH_SIZE 10
#define NODE_PRIMARY_NAD 0
#define NODE_ID 0
#define WAKELOCK_WITHOUT_REF 0
#define MAX_SESSION 50
#define TAF_REF_POOL_SIZE 32
#define STAYAWAKE "STAYAWAKE"
#define RELAX "RELAX"
#define SHUTDOWN "SHUTDOWN"
#define INFO_REPORT_MASK_BUB 1
#define AUTHORIZE_ALL_STAY_AWAKE_REASON 0xFFFFFFFF
#define TAF_MNGDPM_PROCNAME_LEN 30

#define WAKE_SOURCE_ACQUIRED 1
#define WAKE_SOURCE_NOT_ACQUIRED 0

#define MAIN_THREAD_KICK_INTERVAL 13
#define MONITOR_MAIN_THREAD_LOOP 0

#define TAF_MNGDPM_MACHINE_NAME_LEN 32
#define TAF_MNGDPM_STATE_UNKNOWN 0
#define TAF_MNGDPM_STATE_RESUME 1
#define TAF_MNGDPM_STATE_SUSPEND 2
#define TAF_MNGDPM_STATE_SHUTDOWN 3
#define TAF_MNGDPM_STATE_RESTART 4
#define TAF_MNGDPM_STATE_RESTARTING 5
#define TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE 6
#define TAF_MNGDPM_STATE_SUSPENDING 7
#define TAF_MNGDPM_STATE_SHUTTING_DOWN 8
#define TAF_MNGDPM_STATE_WAKING_UP 9
#define WS_DUMP_TIMER_INTERVAL 10000

namespace tafsvc {

typedef struct
{
    uint8_t nad;
    char vmName[TAF_MNGDPM_MACHINE_NAME_LEN];
    uint8_t state;
}taf_mngdPm_vmState_t;

typedef struct
{
    le_msg_SessionRef_t sessionRef;
    void* shutdownCBCtxPtr;
    taf_mngdPm_AsyncShutdownReqHandlerFunc_t shutdownCallbackFunc;
}taf_mngdPm_ShutdownCb_t;

typedef struct
{
    le_msg_SessionRef_t sessionRef;
    void* restartCBCtxPtr;
    taf_mngdPm_AsyncRestartReqHandlerFunc_t restartCallbackFunc;
}taf_mngdPm_RestartCb_t;

typedef struct
{
    le_msg_SessionRef_t sessionRef;
    void* wakeupVehicleCBCtxPtr;
    taf_mngdPm_AsyncWakeupVehicleReqHandlerFunc_t wakeupVehicleCallbackFunc;
}taf_mngdPm_WakeupVehicleCb_t;

typedef enum
{
    WAKEUP_VEHICHLE_REQ_DEFAULT
}taf_mngdPm_RequestedWakeupVehicle_t;

typedef enum
{
    SYSTEM_NORMAL_SHUTDOWN,
    RESTART_WITH_NAD_POWER_OFF_ON,
    RESTART_WITH_NAD_REBOOT
}taf_mngdPm_RequestedState_t;

typedef struct
{
   le_msg_SessionRef_t sessionRef;
   pid_t               procId;
   char name[TAF_MNGDPM_PROCNAME_LEN + 1];
}taf_mngdPm_SessionNode_t;

typedef struct
{
    le_hashmap_Ref_t    clients;
    le_mem_PoolRef_t    SessionNodePool = NULL;
}taf_mngdPm_Client_t;

typedef struct
{
    const char* wsTag;                     // VhalTag to be sent to VHAL
    taf_mngdPm_wsNodeRef_t wsRef;          // New wakeup source reference
    uint8_t pmNodeId;                      // NodeId given
    taf_mngdPm_WsOpt_t option;             // wake source option for calling stay awake and relax
    le_dls_Link_t link;                    // Link to handler list
    le_msg_SessionRef_t sessionRef;        // Session reference of a client
    bool isAcquiredLock;                   // boolean to check wakelock acquired
}taf_nodeWsRefCtx_t;

typedef struct
{
    const char* wsTag;                     // wsTag to be sent to VHAL
    taf_mngdPm_wsRef_t wsRef;              // New wakeup source reference
    taf_mngdPm_StayAwakeReason_t reason;   // stay awake reason for the wakeup source
    le_dls_Link_t link;                    // Link to handler list
    le_msg_SessionRef_t sessionRef;        // Session reference of a client
    int8_t wakeSourceState;                // Wake source acquiring state
    taf_mngdPm_WsOpt_t option;             // wake source option for calling stay awake and relax
}taf_wsRefCtx_t;

typedef struct
{
    bool isGraceful;
    bool isForceful;
    bool isShutDown;
    bool isRestart;
    bool isSuspend;
    bool isWsAcquired;
    bool isWsNodeAcquired;
}taf_powerMode_t;

typedef struct
{
    uint8_t currentState;
    uint8_t prevState;
}taf_stateMachine_t;

typedef struct
{
    taf_mngdPm_InfoReportHandlerFunc_t handlerPtr;
    le_dls_Link_t link;                     // Link to handler list
    taf_mngdPm_InfoReportHandlerRef_t handlerRef;
    void* infoReportHandlerCtxPtr;
    le_msg_SessionRef_t sessionRef;
}taf_mngdPm_InfoReportCb_t;

typedef struct
{
    int32_t status;
}bubStatusEvent_t;

typedef struct
{
    taf_mngdPm_nodePowerStateRef_t nodeStateRef;
    le_msg_SessionRef_t sessionRef;
    taf_mngdPm_NodePowerState_t state;
    bool isAcked;
}taf_mngdPm_NodePowerStateChangeCtxt_t;

typedef struct
{
    taf_mngdPm_NodePowerStateChangeHandlerFunc_t handlerPtr;
    uint8_t pmNodeId;
    le_dls_Link_t link;               // Link to handler list
    le_msg_SessionRef_t sessionRef;
    taf_mngdPm_NodePowerStateChangeBitMask_t powerStateMask;
    taf_mngdPm_NodePowerStateChangeHandlerRef_t handlerRef;
    void* nodePowerStateHandlerCtxPtr;

    // Per-handler snapshot of the current node power state (used for immediate notify)
    taf_mngdPm_NodePowerStateChangeCtxt_t initialNodePowerState;
}taf_mngdPm_NodePowerStateCtxt_t;

typedef struct
{
    taf_mngdPm_NodePowerState_t state;
}taf_mngdPm_NodePowerStateChange_t;

typedef struct
{
    taf_mngdPm_StayAwakeReason_t stayAwakeReason;
    le_msg_SessionRef_t sessionRef;
}taf_mngdPm_StayAwakeReasonCtxt_t;

/*
 * @brief The struct of Power state Ref list.
 */
typedef struct
{
    taf_mngdPm_nodePowerStateRef_t nodeStateRef;
} taf_NodePowerStateRef_t;

/*
 * @brief The struct of Power state Ref list.
 */
typedef struct
{
    long int bootup_awake_time;
    long int hal_state_prepare_timeout;
    long int hal_wakeup_vehicle_timeout;
    long int state_change_ack_timeout;
    bool hal_enabled;
} taf_mngdPm_config_t;

/*
 * @brief The struct of the client callback context
 */
typedef struct
{
    taf_mngdPm_NodeModemAwakeHandlerFunc_t callback;
    void* context;
    uint8_t nodeId;
    taf_mngdPm_NodeModemWsBitMask_t bitset;
    taf_mngdPm_NodeModemAwakeHandlerRef_t ref;
}
CallbackHandlerCombo_t;

typedef enum
{
    EVT_LOAD_PMVHAL_READY = 0,
} mngdPmEventType_Ready_t;

typedef struct
{
    mngdPmEventType_Ready_t type;
}taf_mngdPm_readyEvtType_t;

typedef enum
{
    EVT_NODE_EVENT_STAYAWAKE,
    EVT_NODE_EVENT_RELAX,
    EVT_NODE_EVENT_SHUTDOWN
} taf_mngdPm_InternalEventType_t;

typedef struct
{
    taf_mngdPm_InternalEventType_t type;
} taf_mngdPm_NodeEventData_t;


class tafMngdPMSvc: public ITafSvc
{
    public:
        tafMngdPMSvc() {};
        ~tafMngdPMSvc() {};

        void Init(void);
        static tafMngdPMSvc &GetInstance();
        static le_result_t ParseJsonConfig(std::string configPath);
        static le_result_t ParseJsonConfiguration(std::string configPath);
        static const char* TafStateToString(uint8_t tafState);
        static void OnClientConnection(le_msg_SessionRef_t sessionRef, void *ctxPtr);
        static void OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *ctxPtr);
        static bool IsClientValid();
        static void StateChangeHandler(taf_pm_State_t state, void* contextPtr);
        static le_result_t InitVHalModule();
        static void StateChangeExHandler(taf_pm_PowerStateRef_t powerStateRef,
                taf_pm_NadVm_t vm_id, taf_pm_State_t state, void* contextPtr);

        static void VhalAckTimerHandler(le_timer_Ref_t timerRef);
        static void WakeSourceTimerHandler(le_timer_Ref_t timerRef);
        static void WaitWakeSourceTimer();
        static void VehichleWakeupTimerHandler(le_timer_Ref_t timerRef);

        static le_result_t ShutdownNAD();
        static le_result_t RestartNAD();
        static le_result_t SuspendNAD();
        static void ShutdownPrepareRespCB(uint8_t pmNodeId, hal_pm_NodeState_t state,
                hal_pm_PowerMode_t mode, const uint8_t shutdownReason, hal_pm_RspReason_t reason);
        static void NodeStateChangeReqRespCB(uint8_t pmNodeId, hal_pm_NodeState_t state,
                hal_pm_PowerMode_t mode);
        static void RestartPrepareRespCB(uint8_t pmNodeId, hal_pm_NodeState_t state,
                hal_pm_PowerMode_t mode, const uint8_t restartReason, hal_pm_RspReason_t reason);
        static void WakeupVehicleCB(int32_t reason, int32_t response);

        static void NodeStateChangeNotificationCB(uint8_t pm_node_id,
                                            hal_pm_NodeState_t state,
                                            hal_pm_ConfirmStatus_t status);
        static void NodeEventCB(uint8_t pm_node_id, const char* pm_node_event_info);

        static le_result_t AcquireWakeLock();
        static le_result_t ReleaseWakeLock();

        static le_result_t AcquireNodeWakeLock();
        static le_result_t ReleaseNodeWakeLock();

        static le_result_t RequestStateChange(uint8_t requestedState);
        static void ProcessStateChange(uint8_t toState);

        static void StateLayeredHandler(void* reportPtr, void* layerHandlerFunc);

        static le_mem_PoolRef_t vmStatePool;
        static le_hashmap_Ref_t vmStateHashmap;

        static int8_t vhalWsState; // VHAL-held wake source state

        //List for system level wake sources
        static le_mem_PoolRef_t wsRefPool;
        static le_dls_List_t wsRefList;
        static le_ref_MapRef_t wsRefMap;

        //List for node level wake sources
        static le_mem_PoolRef_t nodeWsRefPool;
        static le_dls_List_t nodeWsRefList;
        static le_ref_MapRef_t nodeWsRefMap;

        // resources to communicate with PMS
        static taf_pm_StateChangeHandlerRef_t handlerRef;
        static taf_pm_StateChangeExHandlerRef_t handlerExRef;
        static taf_pm_PowerStateRef_t powerStateRef;
        static taf_pm_WakeupSourceRef_t ws;
        static taf_pm_WakeupSourceRef_t wsNode;

        // resources to manage power state change requests
        static taf_mngdPm_TargetedPowerMode_t targetedPowerMode;
        static taf_mngdPm_RestartCb_t restartCB;
        static taf_mngdPm_ShutdownCb_t shutdownCB;
        static uint8_t wsCount;
        static uint8_t wsNodeCount;
        static taf_powerMode_t powerMode;
        static taf_stateMachine_t stateMachine;

        // resource to call VHAL module
        static le_timer_Ref_t vhalAckTimerRef;
        static hal_pm_Inf_t *pmInf;
        static taf_mngdPm_RequestedState_t statePtr;
        static le_timer_Ref_t wakeSourceTimerRef;
        static taf_mngdPm_RequestedWakeupVehicle_t wakeupModePtr;
        // resources for multi-client management
        static taf_mngdPm_Client_t mngdPmClientInfo;

        // resources to manage state change handler
        static taf_mngdPm_WakeupVehicleCb_t wakeupVehicleCB;
        static le_timer_Ref_t wakeupVehicleTimerRef;

        // resources to manage infoReport change handler
        static le_event_Id_t infoReport;
        static le_mem_PoolRef_t infoReportHandlerPool;
        static le_dls_List_t infoReportHandlerList;
        static le_ref_MapRef_t infoReportHandlerRefMap;
        static void InfoReportCB(void* reportPtr);
        static void InfoReportVhalCB(int32_t* reportPtr);

        //Node Power State change handler
        static void NodePowerStateChanged(void* reportPtr);
        static le_event_Id_t nodePowerStateChange;
        static le_mem_PoolRef_t nodePowerStateHandlerPool;
        static le_dls_List_t nodePowerStateHandlerList;
        static le_ref_MapRef_t nodePowerStateHandlerMap;
        static void CallNodePowerStateHandlerFunc(taf_mngdPm_NodePowerState_t state);
        static le_mem_PoolRef_t nodePowerStateRefPool;
        static le_ref_MapRef_t nodePowerStateRefMap;
        static void DeleteNodePowerStateRefs();
        std::vector<taf_mngdPm_NodePowerStateChangeCtxt_t>regClientrecrd;
        static int8_t ackClientrecrdSize;
        static int8_t clientSize;

        static void SendAckToPms(taf_mngdPm_NodePowerState_t state, taf_pm_ClientAck_t ackType);
        bool IsSameAsCurrentState(taf_mngdPm_NodePowerState_t nodeState, uint8_t tafState);
        bool IsConfiguredBitMask(taf_mngdPm_NodePowerState_t state, taf_mngdPm_NodePowerStateChangeBitMask_t stateMask);
        static taf_mngdPm_SessionNode_t* To_taf_mngdPm_SessionNode_t(void *c);

        //MPM configuration
        static taf_mngdPm_config_t config;

        //authorize stayawake reason
        static std::bitset<32> stayAwakeReasonMask;
        static std::bitset<32> previousStayAwakeReasonMask;
        bool IsAuthorizedStayAwakeReason(taf_mngdPm_StayAwakeReason_t stayAwakeReason, std::bitset<32> mask);
        void RefreshWakeSources();
        static le_result_t ReleaseWakeSource(taf_wsRefCtx_t * wsRefCtxPtr);
        le_result_t AcquireWakeSource(taf_wsRefCtx_t * wsRefCtxPtr);

        //resources for clients state change acknowledgement
        static le_timer_Ref_t stateChangeAckTimerRef;
        static void StateChangeAckTimerHandler(le_timer_Ref_t timerRef);

        // periodic wake source dump timer
        static le_timer_Ref_t wsDumpTimerRef;
        static void WsDumpTimerHandler(le_timer_Ref_t timerRef);
        static taf_mngdPm_NodePowerState_t currentStateChangePtr;

        static le_mem_PoolRef_t cbHandlerPool;

        static le_ref_MapRef_t cbLocalMap;
        static le_ref_MapRef_t cbRemoteMap;
        static bool enableLocal;
        static bool enableRemote;
        static taf_pm_ModemAwakeHandlerRef_t pmsLocalWakeupHandler;
        static taf_pm_ModemAwakeHandlerRef_t pmsRemoteWakeupHandler;

        static void ClientCallbackDispatcher
        (
            taf_pm_ModemAwakeEventRef_t ref,
            taf_pm_NodeModemWsBitMask_t wsBitmask,
            void * contextPtr
        );

        static void EnableLocalOnce(void);
        static void EnableRemoteOnce(void);

        static le_event_Id_t pmEvtReady;
        bool isPmVhalReady = false;
        static void GetPmVhalReady(void *p1, void *p2);
        static void RetryHandler(le_timer_Ref_t timerRef);
        static void PMVhalReadyEvtHandler(void * reportPtr);

        // Single snapshot used for immediate notification to newly registered handlers.
        // Initialized at service start via taf_pm_GetPowerState and updated on nodePowerStateChange events.
        static taf_mngdPm_NodePowerState_t currentNodePowerState;

        // Helpers for snapshot mapping and initialization
        static taf_mngdPm_NodePowerState_t ToNodePowerStateFromPm(taf_pm_State_t s);
        static void InitializeCurrentNodePowerState();
        // Internal event ID for node events, to be processed on the main thread
        static le_event_Id_t nodeInternalEvent;

        // Handler for node internal events
        static void NodeInternalEventHandler(void *reportPtr);
};
}
