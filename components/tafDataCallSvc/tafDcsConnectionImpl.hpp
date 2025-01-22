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
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include "tafDcsHelper.hpp"

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
#include <telux/tel/ServingSystemManager.hpp>
#endif
#include "tafSvcIF.hpp"

using namespace telux::data;
using namespace telux::common;

#define MIN_SLOT_COUNT 1
#define MAX_SLOT_COUNT 2
#define DEFAULT_PHONE_ID_1 1
#define SLOT_ID_1 1
#define SLOT_ID_2 2

#define SESSION_TIMEOUT 60
#define DATA_SUBSYSTEM_INIT_TIMEOUT 5
#define TELSDK_ASYNC_REQ_TIMEOUT 2

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

    typedef struct
    {
        taf_dcs_CallEndReasonType_t callEndReasonType;
        union
        {
            taf_dcs_CallEndMobileIpReasonCode_t    reasonMIP;
            taf_dcs_CallEndInternalReasonCode_t    reasonInternal;
            taf_dcs_CallEndCallManagerReasonCode_t reasonCallManager;
            taf_dcs_CallEnd3GPPSpecReasonCode_t    reasonSpec;
            taf_dcs_CallEndPPPReasonCode_t         reasonPPP;
            taf_dcs_CallEndEHRPDReasonCode_t       reasonEHRPD;
            taf_dcs_CallEndIPv6ReasonCode_t        reasonIPv6;
            taf_dcs_CallEndHandoffReasonCode_t     reasonHandOff;
        };
    }taf_dcs_callEndReason_t;

    typedef struct tag_taf_dcs_Call_Ctx
    {
        taf_dcs_CallRef_t                       callRef;
        char                                    intfName[TAF_DCS_NAME_MAX_LEN];
        int32_t                                 profileId;      // Profile id
        uint8_t                                 slotId;         // Slot id
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
        uint32_t                                ipv4Mask;
        uint32_t                                ipv6Mask;
        uint64_t                                maxRxBitRate;
        uint64_t                                maxTxBitRate;
        taf_dcs_callEndReason_t                 callEndReasonIPv4;
        taf_dcs_callEndReason_t                 callEndReasonIPv6;
    } taf_dcs_CallCtx_t;

    typedef struct IpAddrInfo
    {
        char                                    ifAddress[TAF_DCS_IPV6_ADDR_MAX_LEN];
        uint32_t                                ifMask;
        char                                    gwAddress[TAF_DCS_IPV6_ADDR_MAX_LEN];
        uint32_t                                gwMask = 0;
        char                                    primaryDnsAddress[TAF_DCS_IPV6_ADDR_MAX_LEN];
        char                                    secondaryDnsAddress[TAF_DCS_IPV6_ADDR_MAX_LEN];
    }taf_dcs_IpAddrInfo_t;

    typedef struct
    {
        EventType_t                             event;
        int32_t                                 profileId;
        uint8_t                                 slotId;
        telux::common::ErrorCode                errorCode;
        char                                    ifName[TAF_DCS_NAME_MAX_LEN];
        telux::data::IpFamilyType               ipType;
        telux::data::DataCallStatus             callStatus;
        telux::data::DataCallStatus             ipv4Status;
        telux::data::DataCallStatus             ipv6Status;
        taf_dcs_IpAddrInfo_t                    ipv4AddrInfo;
        taf_dcs_IpAddrInfo_t                    ipv6AddrInfo;
        telux::data::DataBearerTechnology       dataBearerTech;
        uint64_t                                maxRxBitRate;
        uint64_t                                maxTxBitRate;
        taf_dcs_callEndReason_t                 callEndReasonIPv4;
        taf_dcs_callEndReason_t                 callEndReasonIPv6;
    } dataCallEvent_t;

    typedef struct
    {
        uint8_t slotId;                   ///< slot id
        int32_t profileId;                ///< profile id
        void *contextPtr;
        le_msg_SessionRef_t sessionRef;   ///< handler's owner session's reference
        taf_dcs_AsyncSessionHandlerFunc_t asyncHandler;  ///< async handler
        le_dls_Link_t handlerLink;   ///< double link list's link element
    }HandlerSessionMapping_t;

    typedef struct
    {
      int32_t      profileId;
      uint8_t      slotId;
      bool         throttleState;
      uint32_t     ipv4Time;
      uint32_t     ipv6Time;
    } ThrottleStatus_t;

    typedef void (*taf_dcs_SessionStateFunc_t)(taf_dcs_ConState_t event,
                                               taf_dcs_StateInfo_t *infoPtr,
                                               taf_dcs_CallCtx_t *callCtxPtr);

    class taf_DataConnectionListener : public telux::data::IDataConnectionListener
    {
        public:
          taf_DataConnectionListener(SlotId slot);

          void onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &iCall) override;
          void onThrottledApnInfoChanged(const std::vector<telux::data::APNThrottleInfo> &throttleInfoList) override;

        private:
            SlotId slotId;
    };
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    class taf_DataConnServingSystemListener : public telux::data::IServingSystemListener
    {
        public:
            std::mutex cv_mutex;
            std::condition_variable conVar;
            telux::data::DataServiceState dsStatus;

            taf_DataConnServingSystemListener(SlotId slot);
            void onServiceStateChanged(telux::data::ServiceStatus status) override;
            void onRoamingStatusChanged(telux::data::RoamingStatus status) override;

        private:
            SlotId slotId;
    };

    class taf_DataConnRequestServiceStatusCallback
    {
        public:
            le_sem_Ref_t semaphore;
            telux::data::ServiceStatus status;
            telux::common::ErrorCode errorCode;
            void requestServiceStatus(telux::data::ServiceStatus serviceStatus,
                                      telux::common::ErrorCode error);
    };

    class taf_DataConnRequestRoamingStatusCallback
    {
        public:
            le_sem_Ref_t semaphore;
            telux::data::RoamingStatus status;
            telux::common::ErrorCode errorCode;
            void requestRoamingStatus(telux::data::RoamingStatus roamingStatus,
                                      telux::common::ErrorCode error);
    };

        /*
     * @brief A apn throttle information callback class must be provided when requesting throttle
     * information.
     */
    class taf_DataAPNThrottleInfoCallback {
    public:
        le_sem_Ref_t semaphore;
        le_result_t result;
        ThrottleStatus_t throttleStatus;
        /*
         * This function is called after requesting apn throttle information.
         *
         * @param [in] throttleInfoList    Pointer of apn throttle information list.
         * @param [in] error               The error code of the result.
         */
        void apnThrottleListResponse(
        const std::vector<telux::data::APNThrottleInfo> &throttleInfoList,
        telux::common::ErrorCode error);
    };


#endif
    // Data connection component implementation
    class taf_DataConnection: public ITafSvc
    {
        public:
            taf_DataConnection() {};
            ~taf_DataConnection() {};
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
            void onInitCompleted(telux::common::ServiceStatus status);
        #endif
            void Init(void);
            static taf_DataConnection &GetInstance();
            le_result_t PreProcessDataCall( uint8_t slotId, int32_t profileId,
                                            taf_dcs_CallCtx_t* callCtxPtr,
                                            taf_CallFuncType_t funcType,
                                            le_msg_SessionRef_t sessionRef);
            void AddHandlerSessionMapping( uint8_t slotId, int32_t profileId, void *contextPtr,
                                           le_msg_SessionRef_t sessionRef,
                                           taf_dcs_AsyncSessionHandlerFunc_t asyncHandler);
            HandlerSessionMapping_t* FindAsyncHandler( uint8_t slotId, int32_t profileId);
            void DeleteHandlerInfo(uint8_t slotId, int32_t profileId,
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

            le_result_t StartSessionCallSync(uint8_t slotId, int32_t profileId,
                                             taf_dcs_Pdp_t pdpType);
            le_result_t StartSessionCmdSync(uint8_t slotId, int32_t profileId,
                                            taf_dcs_Pdp_t pdpType,
                                            le_msg_SessionRef_t sessionRef);
            void StartSessionCmdAsync(taf_dcs_ProfileRef_t profileRef,
                                      taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
                                      void* contextPtr,
                                      le_msg_SessionRef_t sessionRef);
            le_result_t StopSessionCallSync(uint8_t slotId, int32_t profileId,
                                            taf_dcs_Pdp_t pdpType);
            le_result_t StopSessionCmdSync(uint8_t slotId, int32_t profileId,
                                           taf_dcs_Pdp_t pdpType,
                                           le_msg_SessionRef_t sessionRef);
            void StopSessionCmdAsync(taf_dcs_ProfileRef_t profileRef,
                                     taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
                                     void* contextPtr,
                                     le_msg_SessionRef_t sessionRef);
            le_result_t SetDefaultProfileIdSync(uint8_t slotId, uint32_t profileId);
            le_result_t GetDefaultProfileIdSync(uint8_t *slotId, uint32_t *profileId);

            static void DataCnxCallEventHandler(void *reportPtr);
            void InternalDataCallEventHandler(void* reportPtr);
            taf_dcs_CallCtx_t* CreateDataCallCtx(uint8_t slotId, int32_t profileId);
            taf_dcs_CallCtx_t* GetCallCtx(uint8_t slotId, int32_t profileId);
            taf_dcs_CallCtx_t* GetCallCtx(taf_dcs_CallRef_t reference);
            le_result_t GetSlotIdAndProfileId(taf_dcs_CallRef_t reference, uint8_t *slotId,
                                              int32_t *profileId);
            le_result_t GetSlotIdAndProfileIdByIfName(const char* namePtr, uint8_t *slotId,
                                              uint32_t* profileId);
            le_result_t MakeCall(uint8_t slotId, int32_t profileId,
                                 telux::data::IpFamilyType ipType);
            le_result_t StopCall(uint8_t slotId, int32_t profileId,
                                 telux::data::IpFamilyType ipType);
            le_result_t SendSettingDefaultProfileIdCmd(uint8_t slotId, int32_t profileId);
            le_result_t SendGettingDefaultProfileIdCmd();
            bool IsCallCtxCreated(uint8_t slotId, int32_t profileId);
            le_result_t AddSessionToCallCtx(taf_dcs_CallCtx_t* callCtxPtr,
                                            le_msg_SessionRef_t sessionRef);
            le_result_t RemoveSessionFromCallCtx(taf_dcs_CallCtx_t* callCtxPtr,
                                                 le_msg_SessionRef_t sessionRef);
            void LogDataCallInfo(const std::shared_ptr<telux::data::IDataCall> &iCall,
                                 const char *fromPtr);
            le_result_t GetInterfaceName(uint8_t slotId, int32_t profileId, char* namePtr,
                                         size_t nameSize);
            le_result_t GetIpv4Address(uint8_t slotId, int32_t profileId, char* addrPtr,
                                       size_t addrSize);
            le_result_t GetIpv4SubnetMask(uint8_t slotId, int32_t profileId, uint32_t* mask);
            le_result_t GetIpv4Gateway(uint8_t slotId, int32_t profileId, char* addrPtr,
                                       size_t addrSize);
            le_result_t GetIpv4Dns(uint8_t slotId, int32_t profileId, char* dns1Ptr,
                                   size_t dns1Size, char* dns2Ptr, size_t dns2Size);
            le_result_t GetIpv6Address(uint8_t slotId, int32_t profileId, char* addrPtr,
                                       size_t addrSize);
            le_result_t GetIpv6SubnetMask(uint8_t slotId, int32_t profileId, uint32_t* mask);
            le_result_t GetIpv6Gateway(uint8_t slotId, int32_t profileId, char* addrPtr,
                                       size_t addrSize);
            le_result_t GetIpv6Dns(uint8_t slotId, int32_t profileId, char* dns1Ptr,
                                   size_t dns1Size, char* dns2Ptr, size_t dns2Size);
            le_result_t GetMtu(uint8_t slotId, int32_t profileId, uint16_t *mtuPtr);
            le_result_t GetConnectionState(uint8_t slotId, int32_t profileId,
                                           taf_dcs_ConState_t* statePtr);
            le_result_t GetDataBearerTechnology(uint8_t slotId, int32_t profileId,
                                            taf_dcs_DataBearerTechnology_t* downDataBearerTechPtr,
                                            taf_dcs_DataBearerTechnology_t* upDataBearerTechPtr);
            le_result_t GetRoamingStatus(uint8_t slotId, bool* isRoamingPtr,
                                         taf_dcs_RoamingType_t* typePtr);
            bool updateStatus(taf_dcs_CallCtx_t *callCtxPtr, dataCallEvent_t *eventPtr);
            le_result_t SendStatusChangedNotification(taf_dcs_CallCtx_t *callCtxPtr,
                                                      dataCallEvent_t *eventPtr);
            taf_dcs_DataBearerTechnology_t updateDataBearerTech(
                                                  telux::data::DataBearerTechnology dataBearerTech);
            le_result_t GetAPNThrottledStatus(taf_dcs_ProfileRef_t    profileRef,
                                                      bool         *isThrottledPtr,
                                                      uint32_t     *ipv4RemainingTimePtr,
                                                      uint32_t     *ipv6RemainingTimePtr);
            le_result_t GetMaxDataBitRates( taf_dcs_ProfileRef_t profileRef,
                                            uint64_t *maxRxBitRatePtr,
                                            uint64_t *maxTxBitRatePtr);
            le_result_t GetCallEndReason( taf_dcs_ProfileRef_t profileRef,
                                          taf_dcs_Pdp_t pdpType,
                                          taf_dcs_CallEndReasonType_t *callEndReasonTypePtr,
                                          int32_t *callEndReasonPtr);

            le_event_Id_t CallEvent;
            bool IsIpv4(uint8_t slotId, int32_t profileId);
            bool IsIpv6(uint8_t slotId, int32_t profileId);
            void RegisterSessionStateHandler(taf_dcs_SessionStateFunc_t func);
            le_event_Id_t GetSessionStateEvent(uint8_t slotId, int32_t profileId);
            le_event_Id_t RoamingStatusEvtId;
            le_mem_PoolRef_t RoamingStatusPool;
            std::promise<le_result_t> CmdSynchronousPromise;
            std::promise<le_result_t> EventSynchronousPromise;
            static void* ConnectionEventThread(void* contextPtr);
            taf_dcs_Pdp_t GetEvtInfoFromConnStatus(taf_dcs_CallCtx_t *callCtxPtr,
                                                   telux::data::DataCallStatus callStatus);
            static void CloseEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
            bool subSystemStatusUpdated;
            std::mutex mtx;
            std::condition_variable conVar;
            std::map<SlotId, std::shared_ptr<telux::data::IServingSystemManager>>
                                                                          dataServingSystemManagers;
            std::map<SlotId, std::shared_ptr<telux::data::IServingSystemListener>>
                                                                         dataServingSystemListeners;
            std::map<SlotId, std::shared_ptr<taf_DataConnServingSystemListener>>
                                                                tafDataConnServingSystemListeners;
            std::shared_ptr<taf_DataConnRequestServiceStatusCallback> reqServiceStatusCb;
            std::shared_ptr<taf_DataConnRequestRoamingStatusCallback> reqRoamingStatusCb;
            std::shared_ptr<taf_DataAPNThrottleInfoCallback> reqAPNThrottlingStatusCb;

            // Function to get data service status from TelSDK.
            le_result_t GetServiceStatusFromTelSDK( const uint8_t slotId,
                                                    telux::data::ServiceStatus &serviceStatus);
            taf_dcs_DataBearerTechnology_t MapNwRatToDataBearerTech(telux::data::NetworkRat nwRAT);
#endif
            std::map<SlotId, std::shared_ptr<telux::data::IDataConnectionManager>>
                                                                            dataConnectionManagers;
            std::map<SlotId, std::shared_ptr<telux::data::IDataConnectionListener>>
                                                                            dataConnectionListeners;
            std::map<SlotId, std::shared_ptr<taf_DataConnectionListener>>
                                                                        tafDataConnectionListeners;

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
            int32_t DefaultSlotId = SLOT_ID_1;
            int32_t ConvertCEReason(taf_dcs_callEndReason_t ceReason);
    };
}
}
