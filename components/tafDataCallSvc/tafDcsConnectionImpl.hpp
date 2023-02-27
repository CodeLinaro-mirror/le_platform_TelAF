/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
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
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include "telux/data/DataConnectionManager.hpp"
#include "telux/data/DataDefines.hpp"
#include "telux/data/DataFactory.hpp"
#include "telux/data/DataProfile.hpp"
#include "telux/tel/PhoneFactory.hpp"
#include "telux/common/CommonDefines.hpp"

#ifdef TARGET_SA515M
#include <telux/tel/ServingSystemManager.hpp>
#endif
#include "tafSvcIF.hpp"

using namespace telux::data;
using namespace telux::common;

#define SLOT_ID_1 1

#define SESSION_TIMEOUT 60

namespace telux {
namespace tafsvc {

    typedef enum
    {
        CALL_FUNCTION_UNKNOWN,
        CALL_FUNCTION_SYNC_START,
        CALL_FUNCTION_SYNC_STOP,
        CALL_FUNCTION_ASYNC_START,
        CALL_FUNCTION_ASYNC_STOP
    }taf_CallFuncType_t;

    typedef enum
    {
        EVT_START_CALLBACK = 0,
        EVT_STOP_CALLBACK,
        EVT_STATUS_CHANGED,
        EVT_SET_DEFAULT_PROFILE,
        EVT_GET_DEFAULT_PROFILE,
    }EventType_t;

    typedef struct
    {
        taf_dcs_CallRef_t       callRef;
        taf_dcs_ConState_t      callEvent;
        taf_dcs_StateInfo_t     info;
        void*                   ptr;
    } DataCallState_t;

    typedef struct
    {
        le_msg_SessionRef_t sessionRef;
        le_dls_Link_t       link;
    } taf_SessionRef_t;

    typedef struct tag_taf_dcs_Call_Ctx
    {
        taf_dcs_CallRef_t                       callRef;
        char                                    intfName[TAF_DCS_NAME_MAX_LEN];
        int32_t                                 profileId;      // Profile id
        le_dls_List_t                           sessionRefList; // The list of clients
        le_dls_Link_t                           link;           // Link to data call list
        bool                                    isCallActionInProgress;// Is a data call in progress
        taf_dcs_ConState_t                      latestConState; // The latest connection state
        taf_CallFuncType_t                      funcType;       // The data call type
        pthread_mutex_t                         callActionMutex;// Mutex for variable isCallAction.
        pthread_cond_t                          callActionCond; // Condition variable
        pthread_mutex_t                         sessionListMutex; // Mutex for sessionRefList
        telux::data::DataCallStatus             callStatus;
        telux::data::DataCallStatus             ipv4Status;
        telux::data::DataCallStatus             ipv6Status;
        telux::data::IpFamilyType               ipType;
        char                                    ipv4Addr[TAF_DCS_IPV4_ADDR_MAX_LEN];
        char                                    ipv4Gw[TAF_DCS_IPV4_ADDR_MAX_LEN];
        char                                    ipv4Dns1[TAF_DCS_IPV4_ADDR_MAX_LEN];
        char                                    ipv4Dns2[TAF_DCS_IPV4_ADDR_MAX_LEN];
        char                                    ipv6Addr[TAF_DCS_IPV6_ADDR_MAX_LEN];
        char                                    ipv6Gw[TAF_DCS_IPV6_ADDR_MAX_LEN];
        char                                    ipv6Dns1[TAF_DCS_IPV6_ADDR_MAX_LEN];
        char                                    ipv6Dns2[TAF_DCS_IPV6_ADDR_MAX_LEN];
        taf_dcs_DataBearerTechnology_t          dataBearerTech;
        le_event_Id_t                           sessionStateEvent;
    } taf_dcs_CallCtx_t;

    typedef struct
    {
        EventType_t                             event;
        int32_t                                 profileId;
        telux::common::ErrorCode                errorCode;
        std::string                             ifName;
        telux::data::IpFamilyType               ipType;
        telux::data::DataCallStatus             callStatus;
        telux::data::DataCallStatus             ipv4Status;
        telux::data::DataCallStatus             ipv6Status;
        telux::data::IpAddrInfo                 ipv4AddrInfo;
        telux::data::IpAddrInfo                 ipv6AddrInfo;
        telux::data::DataBearerTechnology       dataBearerTech;
    } dataCallEvent_t;

    typedef struct
    {
        int32_t profileId;                ///< profile id
        void *contextPtr;
        le_msg_SessionRef_t sessionRef;   ///< handler's owner session's reference
        taf_dcs_AsyncSessionHandlerFunc_t asyncHandler;  ///< async handler
        le_dls_Link_t handlerLink;   ///< double link list's link element
    }HandlerSessionMapping_t;

    typedef void (*taf_dcs_SessionStateFunc_t)(taf_dcs_ConState_t event,
                                               taf_dcs_StateInfo_t *infoPtr,
                                               taf_dcs_CallCtx_t *callCtxPtr);

    class taf_DataConnectionListener : public telux::data::IDataConnectionListener
    {
        public:
          void onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &iCall) override;
    };
#ifdef TARGET_SA515M
    class taf_DataConnServingSystemListener : public telux::data::IServingSystemListener
    {
        public:
            std::mutex cv_mutex;
            std::condition_variable conVar;
            telux::data::DataServiceState dsStatus;

            taf_DataConnServingSystemListener(SlotId slot);
            void onServiceStateChanged(telux::data::ServiceStatus status) override;

        private:
            SlotId slotId;
    };

    class taf_DataConnRequestServiceStatusCallback
    {
        public:
            le_sem_Ref_t semaphore;
            telux::data::ServiceStatus status;
            void requestServiceStatus(telux::data::ServiceStatus serviceStatus,
                                      telux::common::ErrorCode error);
    };
#endif
    // Data connection component implementation
    class taf_DataConnection: public ITafSvc
    {
        public:
            taf_DataConnection() {};
            ~taf_DataConnection() {};
        #ifdef TARGET_SA515M
            void onInitCompleted(telux::common::ServiceStatus status);
        #endif
            void Init(void);
            static taf_DataConnection &GetInstance();
            le_result_t PreProcessDataCall( int32_t profileId, taf_dcs_CallCtx_t* callCtxPtr,
                                                     taf_CallFuncType_t funcType,
                                                     le_msg_SessionRef_t sessionRef);
            void AddHandlerSessionMapping(int32_t profileId, void *contextPtr,
                                                    le_msg_SessionRef_t sessionRef,
                                                    taf_dcs_AsyncSessionHandlerFunc_t asyncHandler);
            HandlerSessionMapping_t* FindAsyncHandler( int32_t profileId);
            void DeleteHandlerInfo(int32_t profileId,
                                   taf_dcs_AsyncSessionHandlerFunc_t asyncHandler);
            static void StartDataCallCallback(const std::shared_ptr<telux::data::IDataCall> &iCall,
                                              telux::common::ErrorCode errorCode);
            static void StopDataCallCallback(const std::shared_ptr<telux::data::IDataCall> &iCall,
                                             telux::common::ErrorCode errorCode);
            static void SetDefaultProfileCallCallback(telux::common::ErrorCode errorCode);
            static void GetDefaultProfileCallCallback(int profileId, SlotId slotId,
                                                      telux::common::ErrorCode error);
            void SendNotificationStateEvent(taf_dcs_ConState_t conState,
                                            taf_dcs_StateInfo_t *infoPtr,
                                            taf_dcs_CallCtx_t *callCtxPtr);

            le_result_t StartSessionCallSync(int32_t profileId, taf_dcs_Pdp_t pdpType);
            le_result_t StartSessionCmdSync(int32_t profileId, taf_dcs_Pdp_t pdpType,
                                            le_msg_SessionRef_t sessionRef);
            void StartSessionCmdAsync(taf_dcs_ProfileRef_t profileRef,
                                      taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
                                      void* contextPtr,
                                      le_msg_SessionRef_t sessionRef);
            le_result_t StopSessionCallSync(int32_t profileId, taf_dcs_Pdp_t pdpType);
            le_result_t StopSessionCmdSync(int32_t profileId, taf_dcs_Pdp_t pdpType,
                                           le_msg_SessionRef_t sessionRef);
            void StopSessionCmdAsync(taf_dcs_ProfileRef_t profileRef,
                                     taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
                                     void* contextPtr,
                                     le_msg_SessionRef_t sessionRef);
            le_result_t SetDefaultProfileIdSync(uint32_t profileId);
            le_result_t GetDefaultProfileIdSync(uint32_t &profileId);

            static void EventHandler(void* reportPtr);
            void InternalEventHandler(void* reportPtr);
            taf_dcs_CallCtx_t* CreateDataCallCtx(int32_t profileId);
            const char * CallEventToString(taf_dcs_ConState_t callEvent);
            const char* CallStatusToString(telux::data::DataCallStatus status);
            const char* CallEndReasonToString(EndReasonType type);
            const char* IpFamilyTypeToString(telux::data::IpFamilyType ipType);
            const char* TechPreferenceToString(telux::data::TechPreference techPref);
            const char* DataBearerToString(telux::data::DataBearerTechnology techPref);
            taf_dcs_CallCtx_t* GetCallCtx(int32_t profileId);
            taf_dcs_CallCtx_t* GetCallCtx(taf_dcs_CallRef_t reference);
            int32_t GetProfileId(taf_dcs_CallRef_t reference);
            le_result_t GetProfileIdByInterfaceName(const char* namePtr,uint32_t* profileId);
            le_result_t MakeCall(int32_t profileId,
                                 telux::data::IpFamilyType ipType);
            le_result_t StopCall(int32_t profileId,
                                 telux::data::IpFamilyType ipType);
            le_result_t SendSettingDefaultProfileIdCmd(int32_t profileId);
            le_result_t SendGettingDefaultProfileIdCmd();
            bool IsCallCtxCreated(int32_t profileId);
            le_result_t AddSessionToCallCtx(taf_dcs_CallCtx_t* callCtxPtr,
                                            le_msg_SessionRef_t sessionRef);
            le_result_t RemoveSessionFromCallCtx(taf_dcs_CallCtx_t* callCtxPtr,
                                                 le_msg_SessionRef_t sessionRef);
            void LogDataCallInfo(const std::shared_ptr<telux::data::IDataCall> &iCall,
                                 const char *fromPtr);
            le_result_t GetInterfaceName(int32_t profileId, char* namePtr, size_t nameSize);
            le_result_t GetIpv4Address(int32_t profileId, char* addrPtr, size_t addrSize);
            le_result_t GetIpv4Gateway(int32_t profileId, char* addrPtr, size_t addrSize);
            le_result_t GetIpv4Dns(int32_t profileId, char* dns1Ptr, size_t dns1Size, char* dns2Ptr,
                                   size_t dns2Size);
            le_result_t GetIpv6Address(int32_t profileId, char* addrPtr, size_t addrSize);
            le_result_t GetIpv6Gateway(int32_t profileId, char* addrPtr, size_t addrSize);
            le_result_t GetIpv6Dns(int32_t profileId, char* dns1Ptr, size_t dns1Size, char* dns2Ptr,
                                   size_t dns2Size);
            le_result_t GetConnectionState(int32_t profileId, taf_dcs_ConState_t* statePtr);
            le_result_t GetDataBearerTechnology(int32_t profileId,
                                            taf_dcs_DataBearerTechnology_t* downDataBearerTechPtr,
                                            taf_dcs_DataBearerTechnology_t* upDataBearerTechPtr);
            bool updateStatus(taf_dcs_CallCtx_t *callCtxPtr, dataCallEvent_t *eventPtr);
            le_result_t SendStatusChangedNotification(taf_dcs_CallCtx_t *callCtxPtr,
                                                      dataCallEvent_t *eventPtr);
            taf_dcs_DataBearerTechnology_t updateDataBearerTech(
                                                  telux::data::DataBearerTechnology dataBearerTech);
            le_event_Id_t CallEvent;
            bool IsIpv4(int32_t profileId);
            bool IsIpv6(int32_t profileId);
            void RegisterSessionStateHandler(taf_dcs_SessionStateFunc_t func);
            le_event_Id_t GetSessionStateEvent(int32_t profileId);
            std::promise<le_result_t> CmdSynchronousPromise;
            std::promise<le_result_t> EventSynchronousPromise;
            static void* ConnectionEventThread(void* contextPtr);
            taf_dcs_Pdp_t GetEvtInfoFromConnStatus(taf_dcs_CallCtx_t *callCtxPtr,
                                                   telux::data::DataCallStatus callStatus);
            static void CloseEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);
        #ifdef TARGET_SA515M
            bool subSystemStatusUpdated;
            std::mutex mtx;
            std::condition_variable conVar;
            std::map<SlotId, std::shared_ptr<telux::data::IServingSystemManager>>
                                                                          dataServingSystemManagers;
            std::map<SlotId, std::shared_ptr<telux::data::IServingSystemListener>>
                                                                         dataServingSystemListeners;
            std::map<SlotId, std::shared_ptr<taf_DataConnServingSystemListener>>
                                                                   connectionServingSystemlisteners;
            std::shared_ptr<taf_DataConnRequestServiceStatusCallback> reqSvcStateCb;
        #endif
            std::shared_ptr<telux::data::IDataConnectionManager> ConnectionMgr;
            std::shared_ptr<telux::data::IDataConnectionListener> DataConnectionListener;
        private:
            le_dls_List_t    DataCallCtxList = LE_DLS_LIST_INIT;
            le_dls_List_t HandlerSessionMappingList = LE_DLS_LIST_INIT;
            le_mem_PoolRef_t HandlerSessionMappingPool = NULL;
            le_mem_PoolRef_t SessionRefPool = NULL;
            le_mem_PoolRef_t DataCallCtxPool = NULL;
            le_ref_MapRef_t  DataCallRefMap = NULL;
            le_mutex_Ref_t callCtxMutex = NULL; // Mutex for DataCallCtxList
            le_mutex_Ref_t handlerlistMutex = NULL; // Mutex for HandlerSessionMappingList
            taf_dcs_SessionStateFunc_t SessionStateFunc = NULL;
            le_thread_Ref_t ConnectionEventThreadRef = NULL;
            int32_t DefaultProfileId = TAF_DCS_DEFAULT_PROFILE;
    };

}
}

