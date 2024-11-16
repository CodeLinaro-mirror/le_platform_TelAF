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

/*  Changes from Qualcomm Innovation Center are provided under the following license:
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <tuple>
#include "telux/data/DataFactory.hpp"
#include "telux/data/DataConnectionManager.hpp"
#include "telux/data/DataProfile.hpp"
#include "telux/data/DataProfileManager.hpp"
#include <telux/tel/PhoneFactory.hpp>
#include <telux/common/DeviceConfig.hpp>
#include "telux/common/CommonDefines.hpp"
#include "tafSvcIF.hpp"

using namespace telux::data;
using namespace telux::common;

typedef struct
{
    bool isThrottled;                               /**< Is APN throttled */
    bool isBlocked;                                 /**< Is APN blocked on all plmns */
    char mcc[TAF_DCS_MCC_BYTES];                  /**< Mobile Country Code */
    char mnc[TAF_DCS_MNC_BYTES];                  /**< Mobile Network Code */
}throttleInfo_t;

typedef struct
{
    bool                                     isValid;
    uint8_t                                  slotId;
    taf_dcs_ProfileRef_t                     reference;
    taf_dcs_ProfileInfo_t                    info;
    taf_dcs_Pdp_t                            pdp;
    char                                     apn[TAF_DCS_NAME_MAX_LEN];
    taf_dcs_ApnType_t                        apnType;
    taf_dcs_Auth_t                           auth;
    char                                     authUsername[TAF_DCS_USER_NAME_MAX_LEN];
    char                                     authPassword[TAF_DCS_PASSWORD_NAME_MAX_LEN];
    le_event_Id_t                            throttleStateEvent;
    throttleInfo_t                           throttleInfo;
    le_dls_Link_t                            link;
} taf_dcs_ProfileCtx_t;

typedef struct
{
    taf_dcs_ProfileCtx_t                     item[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
} taf_dcs_ProfileCtxs_t;

typedef struct
{
    le_result_t                              ret;
    uint32_t                                 num;
    uint8_t                                  slotId;
    taf_dcs_ProfileCtxs_t                    *profilesListPtr;
} Profile_List_Event_t;

namespace telux {
namespace tafsvc {
    class taf_ProfileListCallback : public telux::data::IDataProfileListCallback
    {
        public:
            taf_ProfileListCallback(SlotId slotId):slotId(slotId) {};
            ~taf_ProfileListCallback() {};
            void onProfileListResponse(const std::vector<std::shared_ptr<telux::data::DataProfile>>
                                       &profiles, telux::common::ErrorCode error) override;
         private:
            SlotId slotId;
    };

    class taf_CreateProfileCallback : public telux::data::IDataCreateProfileCallback
    {
    public:
        void onResponse(int profileId, telux::common::ErrorCode error) override;
    };

    class taf_ProfileModifyCallback : public telux::common::ICommandResponseCallback {
       void commandResponse(telux::common::ErrorCode error) override;
    };

    // Data profile component implementation
    class taf_DataProfile: public ITafSvc
    {
        public:
            void Init(void);
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
                void onInitCompleted(telux::common::ServiceStatus status);
#endif
            le_result_t getPhoneIdFromSlotId(uint8_t slotId, uint8_t *phoneIdPtr);
            le_result_t getSlotIdFromPhoneId(uint8_t phoneId, uint8_t *slotIdPtr);
            le_result_t ListProfile(uint8_t slotId, taf_dcs_ProfileInfo_t *profileList,
                                    size_t *listSize);
            taf_dcs_ProfileRef_t GetProfileRef(uint8_t slotId, int32_t index);
            le_result_t GetSlotIdAndProfileId(taf_dcs_ProfileRef_t profileRef, uint8_t *slotId,
                                              int32_t *profileId);
            le_event_Id_t GetThrottleStateEvent(uint8_t slotId, int32_t profileId);

            le_result_t MapProfileCtxToParams(taf_dcs_ProfileCtx_t *ctxPtr,
                                              telux::data::ProfileParams &params);
            le_result_t CreateProfile(taf_dcs_ProfileRef_t profileRef);
            le_result_t DeleteProfile(taf_dcs_ProfileRef_t profileRef);
            le_result_t SetApn(taf_dcs_ProfileRef_t profileRef, const char *apnPtr);
            le_result_t GetApn(taf_dcs_ProfileRef_t profileRef, char *apnPtr, size_t apnSize);
            le_result_t SetProfileName(taf_dcs_ProfileRef_t profileRef, const char *namePtr);
            le_result_t GetProfileName(taf_dcs_ProfileRef_t profileRef, char *namePtr,
                                                                                   size_t nameSize);
            le_result_t GetApnTypes(taf_dcs_ProfileRef_t profileRef, taf_dcs_ApnType_t *apnTypePtr);
            le_result_t SetApnTypes(taf_dcs_ProfileRef_t profileRef, taf_dcs_ApnType_t apnType);
            le_result_t SetTechPreference(taf_dcs_ProfileRef_t profileRef, taf_dcs_Tech_t techPref);
            le_result_t GetTechPreference(taf_dcs_ProfileRef_t profileRef,
                                                                       taf_dcs_Tech_t *techPrefPtr);
            le_result_t SetPdp(taf_dcs_ProfileRef_t profileRef, taf_dcs_Pdp_t pdp);
            le_result_t SetAuth(taf_dcs_ProfileRef_t profileRef, taf_dcs_Auth_t type,
                                const char *userName, const char *password);
            taf_dcs_Pdp_t GetPdp(taf_dcs_ProfileRef_t profileRef);
            le_result_t GetAuthentication(taf_dcs_ProfileRef_t profileRef, taf_dcs_Auth_t *typePtr,
                                          char *userNamePtr, size_t userNameSize, char *passwordPtr,
                                          size_t passwordSize);
            void CleanupAllProfiles(Profile_List_Event_t *listEvent);
            le_result_t CreateIndividualProfile(taf_dcs_ProfileCtx_t *info);
            taf_dcs_ProfileCtx_t *GetProfileCtx(uint8_t slotId, uint32_t index);
            void UpdateIndividualProfile(taf_dcs_ProfileCtx_t *distPtr,
                                         taf_dcs_ProfileCtx_t *srcPtr);
            le_result_t UpdateAllProfilesFromListEvent(Profile_List_Event_t *listEvent);
            void NotifyProfileListHandler(void *listEvent);
            le_result_t SendProfileListReq(uint8_t slotId);
            le_result_t SendProfileModificationReq(uint8_t slotId, int32_t profileId,
                                                   telux::data::ProfileParams &params);

            le_result_t GetAPNThrottledPLMN(taf_dcs_ProfileRef_t    profileRef,
                                            bool       *areAllPLMNsThrottledPtr,
                                            char                 *mccPtrPtr,
                                            size_t                mccSize,
                                            char                 *mncPtrPtr,
                                            size_t                mncSize);

            le_result_t IsAPNThrottled(uint8_t slotId, int32_t profileId, bool *isThrottledPtr);
            le_result_t SendAPNThrottledInfo(uint8_t slotId, int32_t profileId,
                                             bool *isThrottledPtr);

            void ProcessThrottledApnInfoChanged(const std::vector<telux::data::APNThrottleInfo>
                                                &throttleInfoList,uint8_t slotId);

            std::map<SlotId, std::shared_ptr<telux::data::IDataProfileManager>>
                                                                          dataProfileManagers;
            std::map<SlotId, std::shared_ptr<taf_ProfileListCallback>> ListProfileCb;
            std::shared_ptr<taf_ProfileModifyCallback>  ModifyProfileCb;
            std::shared_ptr<taf_CreateProfileCallback>  CreateProfileCb;
            void show(uint8_t slotId);

            taf_DataProfile() {};
            ~taf_DataProfile() {};

            static void ProcessListReq(void* callReq);
            static taf_DataProfile &GetInstance();
            static void* ProfileEventThread(void* contextPtr);

            le_event_Id_t getListReqEvent() { return ListReqEvent; };

            le_mem_PoolRef_t getListEventPool()
            {
                return ListEventPool;
            }

            le_dls_List_t                     ProfileReqHandlerList = LE_DLS_LIST_INIT;
            taf_dcs_ProfileInfo_t             ProfileInfo[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
            taf_dcs_Pdp_t  MapIpFamily(telux::data::IpFamilyType ipFamily);
            telux::data::IpFamilyType MapIpFamily(taf_dcs_Pdp_t ipFamily);
            taf_dcs_Auth_t MapAuthProtocol(telux::data::AuthProtocolType authType);
            telux::data::AuthProtocolType MapAuthProtocol(taf_dcs_Auth_t authType);
            taf_dcs_Tech_t MapTechPreference(telux::data::TechPreference techPref);
            telux::data::TechPreference MapTechPreference(taf_dcs_Tech_t techPref);
            std::promise<le_result_t> CmdSynchronousPromise;
            std::promise<std::tuple<telux::common::ErrorCode, int>> CreateProfileSyncPromise;

        private:
            le_mem_PoolRef_t ListEventPool = NULL;
            le_mem_PoolRef_t ProfilePool = NULL;
            le_mem_PoolRef_t ListHandlerPool = NULL;
            le_ref_MapRef_t  ProfileRefMap = NULL;
            le_event_Id_t    ListReqEvent;
            le_dls_List_t    ProfileCtxList;
            taf_dcs_ProfileCtxs_t ProfilesListPtr = { 0 };
            le_thread_Ref_t ProfileEventThreadRef = NULL;
            std::map<SlotId, uint32_t> ProfileNum;

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
            bool subSystemStatusUpdated;
            std::mutex mtx;
            std::condition_variable conVar;
#endif

            std::shared_ptr<telux::tel::IPhoneManager> PhoneMgr;

    };

}
}
