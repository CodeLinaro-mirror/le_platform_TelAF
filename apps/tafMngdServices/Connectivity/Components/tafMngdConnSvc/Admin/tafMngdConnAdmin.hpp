/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#define TAF_MNGD_CONN_MAX_FILE_PATH_LEN    256
#define TAF_MNGD_CONN_MAX_DATA_OBJ 16

namespace telux {
namespace tafsvc {

    // States as recognized by the Admin component
    typedef enum
    {
        TAF_MNGD_CONN_ADMIN_INIT,                               ///< Init.
        TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_READY,             ///< Sim ready.
        TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_NOT_READY,         ///< Sim not ready.
        TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_REGISTERED,         ///< Network registered.
        TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_NOT_REGISTERED,     ///< Network unregistered.
        TAF_MNGD_CONN_DATA_NOT_CONNECTED, ///< Awaiting user command.
        TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING,              ///< Retry to connect.
        TAF_MNGD_CONN_DATA_NOT_CONNECTED_FAILED,                ///< Data connection failure.
        TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE, ///< Active.
        TAF_MNGD_CONN_DATA_CONNECTED_INACTIVE,                  ///< Inactive.
        TAF_MNGD_CONN_DATA_CONNECTED_INACTIVE_RETRYING,         ///< Retry to connect when Inactive.
        TAF_MNGD_CONN_DATA_NOT_CONNECTED_INACTIVE_RETRYING,     ///< Retry when data not connected
        TAF_MNGD_CONN_DATA_CONNECTED_IDLE,   ///< Idle.
        TAF_MNGD_CONN_ADMIN_ERROR,            ///< Error.
        TAF_MNGD_CONN_DATA_CONNECTIONTEST_START,                ///<ConnectionTest Started
        TAF_MNGD_CONN_DATA_CONNECTIONTEST_FAILED                ///<ConnectionTest failed
    } taf_mngd_Conn_Admin_State_t;

    /**
     * Events that can be sent to the state machine to act upon.
     * Events that end with _SYNC are synchronous (API call from an application) and on completion
     * return is provided via promise.
     */
    typedef enum
    {
        TAF_MNGD_CONN_EVT_INIT = 0,
        TAF_MNGD_CONN_EVT_SET_POLICY_CONF_SYNC,
        TAF_MNGD_CONN_EVT_SIM_READY,
        TAF_MNGD_CONN_EVT_SIM_NOT_READY,
        TAF_MNGD_CONN_EVT_RADIO_POWER_ON,
        TAF_MNGD_CONN_EVT_NETWORK_REG_STATE,
        TAF_MNGD_CONN_EVT_NETWORK_UNREG_STATE,
        TAF_MNGD_CONN_EVT_DATA_START,
        TAF_MNGD_CONN_EVT_DATA_START_SYNC,
        TAF_MNGD_CONN_EVT_DATA_START_RETRY,
        TAF_MNGD_CONN_EVT_DATA_STOP_SYNC,
        TAF_MNGD_CONN_EVT_DATA_STOP,
        TAF_MNGD_CONN_EVT_DATA_CONNECTION_CONNECTED,
        TAF_MNGD_CONN_EVT_DATA_CONNECTION_DISCONNECTED,
        TAF_MNGD_CONN_EVT_GET_CONNECTION_INFO_SYNC,
        TAF_MNGD_CONN_EVT_CONNECTIONTEST
    } taf_mngd_Conn_EventType_t;

    /**
     * Data start retry intervals in milli seconds.
    */
    typedef enum
    {
        TAF_MNGD_CONN_RETRY_INTERVAL_1 = 30000,  // 30 seconds
        TAF_MNGD_CONN_RETRY_INTERVAL_2 = 120000, // 2 minutes
        TAF_MNGD_CONN_RETRY_INTERVAL_3 = 240000, // 4 minutes
        TAF_MNGD_CONN_RETRY_INTERVAL_4 = 480000, // 8 minutes
        TAF_MNGD_CONN_RETRY_INTERVAL_LAST = 960000, // 16 minutes
    } taf_mngd_Conn_Data_Start_Retry_Intervals_t;

    typedef struct
    {
        taf_mngd_Conn_EventType_t                   event;
        union
        {
            uint8_t                                 dataId;
            uint8_t                                 slotId;
            uint8_t                                 phoneId;
        };
    } stateMachineEvent_t;

    typedef struct
    {
        taf_mngd_Conn_DataRef_t                 dataRef;
        taf_mngd_Conn_DataState_t               dataState;
    } DataState_t;

    typedef struct
    {
        uint8_t                                 phoneId;
        uint32_t                                profileNumber;
        taf_mngd_Conn_DataState_t               dataState;
    } profileInfo_t;

//Context to maintain the state for each data id.
    typedef struct tag_taf_mngd_Conn_Ctx
    {
        uint8_t                       dataId;                 // JSON Data ID
        uint8_t                       slotId;                 // JSON Slot ID
        uint8_t                       phoneId;                // JSON Phone ID
        uint8_t                       dataStartRetryCount;    // Data Start retry count
        uint8_t                       maxdataRetryCount;      // User provided max retry count
        uint8_t                       dataConnTestFailedRetryCount; // ConnTest failed retry count
        bool                          dataRetry;              // DataRetry enabled/disabled
        uint32_t                      profileNumber;          // Profile number
        bool                          autoStart;              // Auto start or not
        bool                          needReConn;             //Need to reconnect for manualStart
        char                          intfName[TAF_DCS_NAME_MAX_LEN]; // Interface name
        char                          dns1Addr[TAF_MNGD_CONN_MAX_IPV4_LEN]; // First dns Address
        char                          dns2Addr[TAF_MNGD_CONN_MAX_IPV4_LEN]; // Second dns Address
        le_dls_Link_t                 link;                   // Link to data list
        taf_mngd_Conn_Admin_State_t   state;                  // The Managed Connectivity state
        taf_mngd_Conn_DataState_t     dataState;              // The data state for notification
        le_timer_Ref_t                dataStartRetryTimerRef; // Data start retry timer reference
        le_event_Id_t                 dataStateEvent;         //Data state event
        taf_dcs_Pdp_t                 ipType;                 // Ip type
        taf_mngd_Conn_DataRef_t       dataRef;
        taf_dcs_ConState_t            dcsConState;            // DCS Data State
        char                          conn_test_url[TAF_MNGD_CONN_MAX_CONNECTION_URL_LEN];
                                      //URL to be used for ConnectionTest
        char                          conn_test_ipv4Addr[TAF_MNGD_CONN_MAX_IPV4_LEN];
                                      //IPv4 address to be used for ConnectionTest
        char                          conn_test_ipv6Addr[TAF_MNGD_CONN_MAX_IPV6_LEN];
                                      //IPv6 address to be used for ConnectionTest
        char                          ipv4Addr[TAF_MNGD_CONN_MAX_IPV4_LEN];
        char                          ipv6Addr[TAF_MNGD_CONN_MAX_IPV6_LEN];
    } taf_mngd_Conn_Ctx_t;

    class tafMngdConnAdmin: public ITafSvc
    {
        public:
            tafMngdConnAdmin() {};
            ~tafMngdConnAdmin() {};

            void Init(void);
            static tafMngdConnAdmin &GetInstance();
            le_result_t SetPolicyConfigurationJSONs(const char* ConfigFileNamePtr);
            taf_mngd_Conn_DataRef_t GetRefByDataId(uint8_t dataId);
            le_result_t Startdata(taf_mngd_Conn_DataRef_t dataRef);
            le_result_t Stopdata(taf_mngd_Conn_DataRef_t dataRef);
            le_result_t GetConnectionState(taf_mngd_Conn_DataRef_t dataRef,
                                           uint8_t* dataIdPtr,
                                           taf_mngd_Conn_DataState_t *statePtr);
            le_result_t GetConnectionIPAddresses( taf_mngd_Conn_DataRef_t dataRef,
                                                  char *ipv4AddrPtr, size_t ipv4AddrSize,
                                                  char *ipv6AddrPtr, size_t ipv6AddrSize);
            static void* callback_thread(void* contextPtr);
            static void* StateMachineEventThread(void* contextPtr);
            le_event_Id_t StateMachineEventId;
            le_mem_PoolRef_t connStatePool;
            static void StateMachineHandler(void* reqPtr);
            std::promise<le_result_t> CmdSynchronousPromise;
            taf_mngd_Conn_Ctx_t* GetConnCtx(uint8_t phoneId, uint32_t profileNumber);
            le_event_Id_t GetDataStateEvent(taf_mngd_Conn_DataRef_t dataRef);
            static void FirstLayerConnStateHandler(void* reportPtr, void* secondLayerHandlerFunc);
            void ReportAndUpdateDataState(taf_mngd_Conn_Ctx_t* connCtxPtr,
                                          taf_mngd_Conn_DataState_t newstate);

        private:
            le_thread_Ref_t tafMngd_event_thread=NULL;
            le_thread_Ref_t StateMachineEventThreadRef = NULL;
            void EventInit();
            le_result_t EventSetPolicyConfigJSONs(const char* ConfigFileNamePtr);
            void EventSetRadioPowerOn();
            le_result_t EventStartData(uint8_t dataId);
            le_result_t EventStartDataRetry(uint8_t dataId);
            le_result_t EventStopData(uint8_t dataId);
            le_result_t EventGetConnectionInfo(uint8_t dataId);
            le_result_t EventSimReadyState(uint8_t slotId);
            le_result_t EventSimNotReadyState(uint8_t slotId);
            le_result_t EventNetworkRegState(uint8_t phoneId);
            le_result_t EventNetworkUnregState(uint8_t phoneId);
            void EventDataConnected(uint8_t dataId);
            void EventDataDisconnected(uint8_t dataId);
            static void DataRetryTimerHandler(le_timer_Ref_t timerRef);
            le_result_t InitializeStates();
            le_dls_List_t ConnectionCtxList = LE_DLS_LIST_INIT;
            le_mem_PoolRef_t ConnCtxPool = NULL;
            le_mutex_Ref_t connCtxMutex = NULL; // Mutex for ConnectionCtxList
            le_ref_MapRef_t DataRefMap = NULL;

            taf_mngd_Conn_Ctx_t* GetConnCtx(uint8_t dataId);
            taf_mngd_Conn_Ctx_t* CreateConnCtx(uint8_t dataId, uint8_t slotId, uint8_t phoneId,
                                               uint32_t profileId, bool autoStart,
                                               char* conn_test_url,
                                               char* conn_test_ipv4Addr);
            le_result_t getProfileList( profileInfo_t *profileNumberList, int *listSize);
            bool IsStateConnected();

            void ResetDataRetryValues();
            void ResetDataRetryValues(uint8_t dataId);

            //Connectiontest
            void ConnectionTest(uint8_t dataId);
            bool ConnectionTest_URL(std::string url, std::string interfaceName);
            bool ConnectionTest_IPv4(std::string ipv4, std::string interfaceName);

            // Policy and Configuration to use
            taf_mngd_Conn_Policy_t Policy;
            taf_mngd_Conn_Configuration_t Configuration;

            //Config file name
            char ConfigFileName[TAF_MNGD_CONN_MAX_FILE_PATH_LEN];

            bool ReadJSONFileNamesFromConfigTree (char *ConfigurationFileNamePtr);

            const char * EventToString(taf_mngd_Conn_EventType_t event);
            const char * StateToString(taf_mngd_Conn_Admin_State_t state);
            bool IsJsonValid = false;
    };
}
}
