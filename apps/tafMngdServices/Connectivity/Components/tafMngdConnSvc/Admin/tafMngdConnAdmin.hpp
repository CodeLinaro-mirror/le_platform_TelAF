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
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
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

#define MAX_MNGD_CONN_FILE_PATH_LEN    256
#define POWERON_STATE_MAX_CHECKING_TIMES  5
#define RECONNECT_TIME_INTERVAL 3000
#define RECONNECT_RETRY_COUNT 1

namespace telux {
namespace tafsvc {

    typedef enum
    {
        TAF_MNGD_CONN_INIT,                                         ///< Init.
        TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_READY,                 ///< Sim ready.
        TAF_MNGD_CONN_DATA_NOT_CONNECTED_SIM_NOT_READY,             ///< Sim not ready.
        TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_REGISTERED,             ///< Network registered.
        TAF_MNGD_CONN_DATA_NOT_CONNECTED_NW_NOT_REGISTERED,         ///< Network unregistered.
        TAF_MNGD_CONN_DATA_NOT_CONNECTED_AWAITING_USER_COMMAND,     ///< Awaiting user command.
        TAF_MNGD_CONN_DATA_NOT_CONNECTED_RETRYING,                  ///< Retry to connect.
        TAF_MNGD_CONN_DATA_CONNECTED_ACTIVE,                        ///< Active.
        TAF_MNGD_CONN_DATA_CONNECTED_IDLE,                          ///< Idle.
        TAF_MNGD_CONN_ERROR                                         ///< Error.
    }taf_mngd_Conn_State_t;

    typedef enum
    {
        EVT_INIT = 0,
        EVT_SET_POLICY_CONF,
        EVT_SIM_READY,
        EVT_SIM_NOT_READY,
        EVT_NETWORK_REG_STATE,
        EVT_NETWORK_UNREG_STATE,
        EVT_DATA_START,
        EVT_DATA_STOP,
        EVT_DATA_CONNECTION_CONNECTED,
        EVT_DATA_CONNECTION_DISCONNECTED,
        EVT_GET_CONNECTION_INFO
    }EventType_t;

    typedef struct
    {
        EventType_t                             event;
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
        uint8_t                                 dataId;         // Data ID
        uint8_t                                 slotId;         // Slot ID
        uint8_t                                 phoneId;        // Phone ID
        uint32_t                                profileNumber;  // Profile number
        bool                                    autoStart;      // Auto start or not
        char                                    intfName[TAF_DCS_NAME_MAX_LEN]; // Interface name
        le_dls_Link_t                           link;           // Link to data list
        taf_mngd_Conn_State_t                   state; // The Managed Connectivity state
        taf_mngd_Conn_DataState_t               dataState; // The data state for notification
        le_timer_Ref_t                          reconnTimerRef; // The timer reference
        le_event_Id_t                           dataStateEvent; //Data state event
        taf_dcs_Pdp_t                           ipType; // Ip type
        taf_mngd_Conn_DataRef_t                 dataRef;
        char                                    ipv4Addr[TAF_DCS_IPV4_ADDR_MAX_LEN];
        char                                    ipv6Addr[TAF_DCS_IPV6_ADDR_MAX_LEN];
    } taf_mngd_Conn_Ctx_t;

    class tafMngdConnAdmin: public ITafSvc
    {
        public:
            tafMngdConnAdmin() {};
            ~tafMngdConnAdmin() {};

            void Init(void);
            static tafMngdConnAdmin &GetInstance();
            le_result_t SetPolicyConfigurationJSONs(const char* ConfigFileNamePtr,
                                                    const char* PolicyFileNamePtr);
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
            le_result_t EventSetPolicyConfigJSONs(const char* ConfigFileNamePtr,
                                                  const char* PolicyFileNamePtr);
            le_result_t EventStartData(uint8_t dataId);
            le_result_t EventStopData(uint8_t dataId);
            le_result_t EventGetConnectionInfo(uint8_t dataId);
            le_result_t EventSimReadyState(uint8_t slotId);
            le_result_t EventSimNotReadyState(uint8_t slotId);
            le_result_t EventNetworkRegState(uint8_t phoneId);
            le_result_t EventNetworkUnregState(uint8_t phoneId);
            void EventDataConnected(uint8_t dataId);
            void EventDataDisconnected(uint8_t dataId);
            static void ReconnectTimerHandler(le_timer_Ref_t timerRef);
            le_result_t CreateConnectionsBasedPolicy();
            le_dls_List_t ConnectionCtxList = LE_DLS_LIST_INIT;
            le_mem_PoolRef_t ConnCtxPool = NULL;
            le_mutex_Ref_t connCtxMutex = NULL; // Mutex for ConnectionCtxList
            le_ref_MapRef_t DataRefMap = NULL;

            taf_mngd_Conn_Ctx_t* GetConnCtx(uint8_t dataId);
            taf_mngd_Conn_Ctx_t* CreateConnCtx(uint8_t dataId, uint8_t slotId, uint8_t phoneId,
                                               uint32_t profileId, bool autoStart);
            le_result_t getProfileList( profileInfo_t *profileNumberList, int *listSize);
            bool IsStateConnected();

            // Policy and Configuration to use
            taf_mngd_Conn_Policy_t Policy;
            taf_mngd_Conn_Configuration_t Configuration;

            // Policy and Config file name
            char PolicyFileName[MAX_MNGD_CONN_FILE_PATH_LEN];
            char ConfigFileName[MAX_MNGD_CONN_FILE_PATH_LEN];

            bool ReadJSONFileNamesFromConfigTree (
                                    char *PolicyFileNamePtr,
                                    char *ConfigurationFileNamePtr);

            const char * EventToString(EventType_t event);
            const char * StateToString(taf_mngd_Conn_State_t state);
            bool IsJsonValid = false;
    };
}
}

