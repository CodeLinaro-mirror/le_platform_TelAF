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
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS`
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include <future>
#include "tafMngdConn_Common.hpp"
#include "tafMngdConnSvcJSONParser.hpp"
#include <set>

#define MCS_MAX_FILE_PATH_LEN    256
#define MCS_MAX_DATA_OBJ 16

// Radio off time for L1 recovery
#define MCS_L1_RECOVERY_RADIO_OFF_TIME 5

// Maximum nmber of client sessions
#define MCS_MAX_SESSIONS 16

namespace telux {
namespace tafsvc {
    //----------------------------------------------------------------------------------------------
    /**
     * States as recognized by the Admin component
     */
    //----------------------------------------------------------------------------------------------
    typedef enum
    {
        MCS_ADMIN_INIT,                           ///< Init.
        MCS_ADMIN_ERROR,                          ///< Error.
        MCS_DATA_NOT_CONNECTED_SIM_READY,         ///< Sim ready.
        MCS_DATA_NOT_CONNECTED_SIM_NOT_READY,     ///< Sim not ready.
        MCS_DATA_NOT_CONNECTED_NW_REGISTERED,     ///< Network registered.
        MCS_DATA_NOT_CONNECTED_NW_NOT_REGISTERED, ///< Network unregistered.
        MCS_DATA_NOT_CONNECTED,                   ///< Awaiting user command.
        MCS_DATA_NOT_CONNECTED_RETRYING,          ///< Retry to connect.
        MCS_DATA_NOT_CONNECTED_FAILED,            ///< Data connection failure.
        MCS_DATA_CONNECTED_ACTIVE,                ///< Active.
        MCS_DATA_CONNECTED_INACTIVE,              ///< Inactive.
        MCS_DATA_CONNECTED_INACTIVE_RETRYING,     ///< Retry to connect when Inactive.
        MCS_DATA_NOT_CONNECTED_INACTIVE_RETRYING, ///< Retry when data not connected
        MCS_DATA_CONNECTED_IDLE,                  ///< Idle.
        MCS_DATA_START_CONNECTIONTEST_START,      ///< DataStartConnectionTest Started
        MCS_DATA_START_CONNECTIONTEST_FAILED,     ///< DataStartConnectionTest failed
        MCS_RECOVERY_SCHEDULED_L1,                ///< L1 connectivity recovery scheduled.
        MCS_RECOVERY_STARTED_L1,                  ///< L1 connectivity recovery started.
        MCS_RECOVERY_CANCELED_L1,                 ///< L1 connectivity recovery canceled.
        MCS_RECOVERY_FAILED_L1,                   ///< L1 connectivity recovery failed.
        MCS_RECOVERY_SCHEDULED_L2,                ///< L2 connectivity recovery scheduled.
        MCS_RECOVERY_STARTED_L2,                  ///< L2 connectivity recovery started.
        MCS_RECOVERY_CANCELED_L2,                 ///< L2 connectivity recovery canceled.
        MCS_RECOVERY_FAILED_L2                    ///< L2 connectivity recovery failed.
    } mcs_Admin_State_t;

    /**
     * Events that can be sent to the state machine to act upon.
     * Events that end with _SYNC are synchronous (API call from an application) and on completion
     * return is provided via promise.
     */
    typedef enum
    {
        MCS_EVT_INIT = 0,
        MCS_EVT_SET_POLICY_CONF_SYNC,
        MCS_EVT_SIM_READY,
        MCS_EVT_SIM_NOT_READY,
        MCS_RADIO_POWER_ON,
        MCS_EVT_NETWORK_REG_STATE,
        MCS_EVT_NETWORK_UNREG_STATE,
        MCS_EVT_DATA_START,
        MCS_EVT_DATA_START_SYNC,
        MCS_EVT_DATA_START_RETRY,
        MCS_EVT_DATA_START_RETRY_APP_REQ,
        MCS_EVT_DATA_STOP_SYNC,
        MCS_EVT_DATA_STOP,
        MCS_EVT_DATA_CONNECTION_CONNECTED,
        MCS_EVT_DATA_CONNECTION_CONNECTED_ACTIVE,
        MCS_EVT_DATA_CONNECTION_DISCONNECTED,
        MCS_EVT_GET_CONNECTION_INFO_SYNC,
        MCS_EVT_DATA_START_CONNECTIONTEST,
        MCS_EVT_DATA_PERIODIC_CONNECTIONTEST,
        MCS_EVT_CONN_RECOVERY_SCHEDULE_L1, // Schedule L1 connectivity recovery
        MCS_EVT_CONN_RECOVERY_CANCEL_L1,
        MCS_EVT_CONN_RECOVERY_CANCEL_L1_SYNC,
        MCS_EVT_CONN_RECOVERY_CANCEL_L1_SEND_IND,
        MCS_EVT_CONN_RECOVERY_INTERRUPTED_L1,
        MCS_EVT_CONN_RECOVERY_START_L1,
        MCS_EVT_CONN_RECOVERY_SCHEDULE_L2, // Schedule L2 connectivity recovery
        MCS_EVT_CONN_RECOVERY_CANCEL_L2,
        MCS_EVT_CONN_RECOVERY_CANCEL_L2_SYNC,
        MCS_EVT_CONN_RECOVERY_CANCEL_L2_SEND_IND,
        MCS_EVT_CONN_RECOVERY_INTERRUPTED_L2,
        MCS_EVT_CONN_RECOVERY_START_L2

    } mcs_EventType_t;

    /**
     * Data start retry intervals in milli seconds.
    */
    typedef enum
    {
        MCS_RETRY_INTERVAL_1 = 30000,  // 30 seconds
        MCS_RETRY_INTERVAL_2 = 120000, // 2 minutes
        MCS_RETRY_INTERVAL_3 = 240000, // 4 minutes
        MCS_RETRY_INTERVAL_4 = 480000, // 8 minutes
        MCS_RETRY_INTERVAL_LAST = 960000, // 16 minutes
    } mcs_Data_Start_Retry_Intervals_t;

    typedef struct
    {
        mcs_EventType_t                   event;
        union
        {
            uint8_t                                 dataId;
            uint8_t                                 slotId;
            uint8_t                                 phoneId;
        };
    } stateMachineEvent_t;

    /* Internal structure to report Data State */
    typedef struct
    {
        taf_mngdConn_DataRef_t                 dataRef;
        taf_mngdConn_DataState_t               dataState;
    } DataState_t;

    /* Internal structure to report Recovery State */
    typedef struct
    {
        taf_mngdConn_RecoveryState_t recoveryState;
        taf_mngdConn_DataRef_t       dataRef;
    } RecoveryState_t;

    typedef struct
    {
        uint8_t                                 phoneId;
        uint32_t                                profileNumber;
        taf_mngdConn_DataState_t               dataState;
    } profileInfo_t;

    typedef struct
    {
        le_msg_SessionRef_t sessionRef;
        pid_t pid;
    } mcs_ClientNode_t;

    typedef struct
    {
        le_hashmap_Ref_t hashMap;
        le_mem_PoolRef_t memPool;
    } mcs_Clients_t;

//Context to maintain the state for each data id.
    typedef struct tag_mcs_DataCtx_t
    {
        uint8_t                       dataId;                 // JSON Data ID
        uint8_t                       slotId;                 // JSON Slot ID
        uint8_t                       phoneId;                // JSON Phone ID
        uint8_t                       dataStartRetryCount;    // Data Start retry count
        uint8_t                       maxdataRetryCount;      // User provided max retry count
        uint8_t                       dataConnTestFailedRetryCount; // ConnTest failed retry count
        bool                          dataRetry;              // DataRetry enabled/disabled
        uint32_t                      profileNumber;          // Profile number
        char                          dataName[MCS_MAX_NAME_LEN]; //DataName
        bool                          autoStart;              // Auto start or not
        bool                          needReConn;             //Need to reconnect for manualStart
        char                          intfName[TAF_DCS_NAME_MAX_LEN]; // Interface name
        char                          dns1Addr[MCS_MAX_IPV4_LEN]; // First dns Address
        char                          dns2Addr[MCS_MAX_IPV4_LEN]; // Second dns Address
        le_dls_Link_t                 link;                   // Link to data list
        mcs_Admin_State_t             adminState;               // Internal MCS state
        taf_mngdConn_DataState_t      dataState;              // The data state for notification
        le_timer_Ref_t periodicConnectivityTestTimerRef;
                                                // periodicConnectivityTestTimerRef timer reference
        le_timer_Ref_t                dataStartRetryTimerRef; // Data start retry timer reference
        le_timer_Ref_t                recoveryScheduleTimerRef; // Recovery schedule timer reference
        le_timer_Ref_t                recoveryRetryTimerRef; // Recovery schedule timer reference
        le_event_Id_t                 dataStateEvent;         //Data state event
        taf_dcs_Pdp_t                 ipType;                 // Ip type
        taf_mngdConn_DataRef_t        dataRef;
        taf_dcs_ConState_t            dcsConState;            // DCS Data State
        char                          conn_test_url[MCS_MAX_CONNECTION_URL_LEN];
                                      //URL to be used for DataStartConnectionTest
        char                          conn_test_ipv4Addr[MCS_MAX_IPV4_LEN];
                                      //IPv4 address to be used for DataStartConnectionTest
        char                          conn_test_ipv6Addr[MCS_MAX_IPV6_LEN];
                                      //IPv6 address to be used for DataStartConnectionTest
        char                          ipv4Addr[MCS_MAX_IPV4_LEN];
        char                          ipv6Addr[MCS_MAX_IPV6_LEN];
        bool                          isConnectivityRecoveryScheduled;
                                      //PeriodicConnectivityTest URL
        char                          conn_periodic_test_url[MCS_MAX_CONNECTION_URL_LEN];
                                      //PeriodicConnectivityTest Interval
        uint8_t                       conn_periodic_test_interval;
                                      //PeriodicConnectivityTest RetryCount
        uint8_t                       conn_periodic_test_retryCount;
                                      //PeriodicConnectivityTest MaxRetryCount
        uint8_t                       conn_periodic_test_maxRetryCount;
	    bool                          wasL1ConnectivityRecoveryDone;
        // Clients that have called Data Start
        std::set<le_msg_SessionRef_t> clients;
    } mcs_DataCtx_t;

    class tafMngdConnAdmin: public ITafSvc
    {
        public:
            tafMngdConnAdmin() {};
            ~tafMngdConnAdmin() {};

            void Init(void);
            static tafMngdConnAdmin &GetInstance();
            taf_mngdConn_DataRef_t GetRefByDataId(uint8_t dataId);
            taf_mngdConn_DataRef_t GetRefByName(const char *dataName);
            le_result_t GetDataIdByRef(taf_mngdConn_DataRef_t dataRef, uint8_t* dataIdPtr);
            le_result_t GetDataNameByRef(taf_mngdConn_DataRef_t dataRef,
                                    char *dataName, size_t dataNameSize);
            le_result_t Startdata(taf_mngdConn_DataRef_t dataRef);
            le_result_t Stopdata(taf_mngdConn_DataRef_t dataRef);
            le_result_t GetConnectionState(taf_mngdConn_DataRef_t dataRef,
                                           taf_mngdConn_DataState_t *statePtr);
            le_result_t GetConnectionIPAddresses( taf_mngdConn_DataRef_t dataRef,
                                                  char *ipv4AddrPtr, size_t ipv4AddrSize,
                                                  char *ipv6AddrPtr, size_t ipv6AddrSize);
            le_result_t StartDataRetry(taf_mngdConn_DataRef_t dataRef);
            le_result_t CancelL1Recovery(taf_mngdConn_DataRef_t dataRef);
            le_result_t CancelL2Recovery(taf_mngdConn_DataRef_t dataRef);

            taf_mngdConn_DataStateHandlerRef_t AddDataStateHandler(
                taf_mngdConn_DataRef_t dataRef,
                taf_mngdConn_DataStateHandlerFunc_t handlerPtr,
                void *contextPtr);
            taf_mngdConn_RecoveryStateHandlerRef_t AddRecoveryStateHandler(
                taf_mngdConn_RecoveryStateHandlerFunc_t handlerPtr,
                void *contextPtr);

            // Accessor functions
            mcs_DataCtx_t *GetDataCtx(uint8_t phoneId, uint32_t profileNumber);
            le_event_Id_t  GetStateMachineEventId();

        private:

            le_event_Id_t StateMachineEventId;

            le_thread_Ref_t tafMngd_event_thread=NULL;
            le_thread_Ref_t StateMachineEventThreadRef = NULL;

            // Policy and Configuration to use
            mcs_Policy_t Policy;
            mcs_Configuration_t Configuration;

            // Config file name
            char ConfigFileName[MCS_MAX_FILE_PATH_LEN];

            bool IsJsonValid = false;

            le_mem_PoolRef_t dataStatePool;
            std::promise<le_result_t> CmdSynchronousPromise;

            le_dls_List_t DataCtxList = LE_DLS_LIST_INIT;
            le_mem_PoolRef_t DataCtxPool = NULL;
            le_mutex_Ref_t DataCtxMutex = NULL; // Mutex for DataCtxList
            le_ref_MapRef_t DataRefMap = NULL;

            le_mem_PoolRef_t recoveryStatePool;
            le_event_Id_t recoveryStateEvent; // Recovery state event

            // resources for multi-client management
            static mcs_Clients_t ConnectedClients;

            void EventInit();
            le_result_t EventSetPolicyConfigJSONs(const char* ConfigFileNamePtr);
            void EventSetRadioPowerOn();
            le_result_t EventStartData(uint8_t dataId);
            le_result_t EventStartDataRetry(uint8_t dataId);
            le_result_t EventStartDataRetryAppReq(uint8_t dataId);
            le_result_t EventStopData(uint8_t dataId);
            le_result_t EventGetConnectionInfo(uint8_t dataId);
            le_result_t EventSimReadyState(uint8_t slotId);
            le_result_t EventSimNotReadyState(uint8_t slotId);
            le_result_t EventNetworkRegState(uint8_t phoneId);
            le_result_t EventNetworkUnregState(uint8_t phoneId);
            le_result_t SetPolicyConfigurationJSONs(const char *ConfigFileNamePtr);

            void EventDataConnected(uint8_t dataId);
            void EventDataConnectedActive(uint8_t dataId);
            void EventDataDisconnected(uint8_t dataId);


            void ReportAndUpdateDataState(mcs_DataCtx_t *dataCtxPtr,
                                          taf_mngdConn_DataState_t newstate);
            void ReportRecoveryStateEvent(taf_mngdConn_RecoveryState_t recoveryState,
                                          mcs_DataCtx_t *dataCtxPtr);
            le_result_t InitializeStates();
            le_event_Id_t GetDataStateEvent(taf_mngdConn_DataRef_t dataRef);
            mcs_DataCtx_t* GetDataCtx(uint8_t dataId);
            mcs_DataCtx_t *GetDataCtx(const char *dataName);
            mcs_DataCtx_t* CreateDataCtx(uint8_t dataId, uint8_t slotId, uint8_t phoneId,
                                               uint32_t profileId,
                                               char dataName[MCS_MAX_NAME_LEN],
                                               bool autoStart,
                                               char* conn_test_url,
                                               char* conn_test_ipv4Addr);
            le_result_t getProfileList( profileInfo_t *profileNumberList, int *listSize);
            bool IsStateConnected();

            void ResetDataRetryPeriodicConnCheckValues();
            void ResetDataRetryPeriodicConnCheckValues(uint8_t dataId);

            //Connectiontest
            void EventDataStartConnectionTest(uint8_t dataId);
            void EventDataPeriodicConnectivityTest(uint8_t dataId);
            bool DataConnectivityTest_URL(std::string url, std::string interfaceName);
            bool DataConnectivityTest_IPv4(std::string ipv4, std::string interfaceName);

            // L1 Connectivity Recovery
            void EventL1ConnRecoverySchedule(uint8_t dataId);
            void EventL1ConnRecoveryCancel(uint8_t dataId);
            void EventL1ConnRecoveryCancelSync(uint8_t dataId);
            void EventL1ConnRecoveryCancelSendInd(uint8_t dataId);
            void EventL1ConnRecoveryInterrupted(uint8_t dataId);
            void EventL1ConnRecoveryStart(uint8_t dataId);

            // L2 Connectivity Recovery
            void EventL2ConnRecoverySchedule(uint8_t dataId);
            void EventL2ConnRecoveryCancel(uint8_t dataId);
            void EventL2ConnRecoveryCancelSync(uint8_t dataId);
            void EventL2ConnRecoveryCancelSendInd(uint8_t dataId);
            void EventL2ConnRecoveryInterrupted(uint8_t dataId);
            void EventL2ConnRecoveryStart(uint8_t dataId);

            const char *EventToString(mcs_EventType_t event);
            const char *StateToString(mcs_Admin_State_t state);
            const char *DataStateToString(taf_mngdConn_DataState_t state);

            bool ReadJSONFileNamesFromConfigTree(char *ConfigurationFileNamePtr);


            // TelAF event handler and callback functions
            // Entry function for thread that receives events from TelAF services.
            static void *callback_thread_func(void *contextPtr);
            // State machine thread entry function
            static void *StateMachineEventThreadFunc(void *contextPtr);
            // State machine event handler function
            static void StateMachineEvtHandlerFunc(void *reqPtr);

            // Timer handler
            static void PeriodicConnectivityTestTimerHandler(le_timer_Ref_t timerRef);
            static void DataRetryTimerHandler(le_timer_Ref_t timerRef);
            static void RecoveryScheduleTimerHandler(le_timer_Ref_t timerRef);
            static void RecoveryRetryTimerHandler(le_timer_Ref_t timerRef);

            // Client connect/disconnect handlers
            static void OnClientConnect(le_msg_SessionRef_t sessionRef, void *ctxPtr);
            static void OnClientDisconnect(le_msg_SessionRef_t sessionRef, void *ctxPtr);

            // Layered handlers to send events to applications
            static void FirstLayerDataStateHandler(void *reportPtr, void *secondLayerHandlerFunc);
            static void FirstLayerRecoveryStateHandler(void *reportPtr,
                                                       void *secondLayerHandlerFunc);
#ifndef LE_CONFIG_TARGET_SIMULATION
            //Async APIs callback handler
            static void RestartReqAsyncCallBack(taf_mngdPm_RestartMode_t RestartMode,
                                                taf_mngdPm_ResponseMode_t ResponseMode,
                                                void *contextPtr);
#endif // LE_CONFIG_TARGET_SIMULATION
    };
}
}
