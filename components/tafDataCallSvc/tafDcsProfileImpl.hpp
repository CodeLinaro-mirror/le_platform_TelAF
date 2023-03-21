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

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include "telux/data/DataFactory.hpp"
#include "telux/data/DataConnectionManager.hpp"
#include "telux/data/DataProfile.hpp"
#include "telux/data/DataProfileManager.hpp"
#include "telux/common/CommonDefines.hpp"
#include "tafSvcIF.hpp"

using namespace telux::data;
using namespace telux::common;

typedef struct
{
    le_msg_SessionRef_t                     sessionRef;
    taf_dcs_ProfileListHandlerFunc_t        handlerPtr;
    void                                    *contextPtr;
    le_dls_Link_t                           link;
} taf_dcs_ProfileListHandler_t;

typedef struct
{
    bool                                     isValid;
    taf_dcs_ProfileRef_t                     reference;
    taf_dcs_ProfileInfo_t                    info;
    taf_dcs_Pdp_t                            pdp;
    char                                     apn[TAF_DCS_NAME_MAX_LEN];
    taf_dcs_ApnType_t                        apnType;
    taf_dcs_Auth_t                           auth;
    char                                     authUsername[TAF_DCS_USER_NAME_MAX_LEN];
    char                                     authPassword[TAF_DCS_PASSWORD_NAME_MAX_LEN];
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
    taf_dcs_ProfileCtxs_t                    *profilesListPtr;
} Profile_List_Event_t;

namespace telux {
namespace tafsvc {
    class taf_ProfileListCallback : public telux::data::IDataProfileListCallback
    {
        public:
            taf_ProfileListCallback(std::string cmdCbName):cmdCbName_(cmdCbName) {};
            ~taf_ProfileListCallback() {};
            void onProfileListResponse(const std::vector<std::shared_ptr<telux::data::DataProfile>> &profiles, telux::common::ErrorCode error) override;
         private:
            std::string cmdCbName_;
    };

    class taf_ProfileModifyCallback : public telux::common::ICommandResponseCallback {
       void commandResponse(telux::common::ErrorCode error) override;
    };

    // Data profile component implementation
    class taf_DataProfile: public ITafSvc
    {
        public:
            void Init(void);

            le_result_t ListProfileAsync(taf_dcs_ProfileListHandlerFunc_t handlerPtr, void *contextPtr);
            le_result_t ListProfile(taf_dcs_ProfileInfo_t *profileList, size_t *listSize);
            le_result_t SetDefaultProfile(int32_t index);
            taf_dcs_ProfileRef_t GetProfileRef(int32_t index);
            le_result_t GetProfileId(taf_dcs_ProfileRef_t profileRef, int32_t *profileIdPtr);
            le_result_t MapProfileCtxToParams(taf_dcs_ProfileCtx_t *ctxPtr, telux::data::ProfileParams &params);
            le_result_t SetApn(taf_dcs_ProfileRef_t profileRef, const char *apnPtr);
            le_result_t GetApn(taf_dcs_ProfileRef_t profileRef, char *apnPtr, size_t apnSize);
            le_result_t GetApnTypes(taf_dcs_ProfileRef_t profileRef, taf_dcs_ApnType_t *apnTypePtr);
            le_result_t SetPdp(taf_dcs_ProfileRef_t profileRef, taf_dcs_Pdp_t pdp);
            le_result_t SetAuth(taf_dcs_ProfileRef_t profileRef, taf_dcs_Auth_t type, const char *userName, const char *password);
            taf_dcs_Pdp_t GetPdp(taf_dcs_ProfileRef_t profileRef);
            le_result_t GetAuthentication(taf_dcs_ProfileRef_t profileRef, taf_dcs_Auth_t *typePtr, char *userNamePtr, size_t userNameSize, char *passwordPtr, size_t passwordSize);
            bool IsListHandlerBound(le_msg_SessionRef_t sessionRef);
            le_result_t AddListHandler(le_msg_SessionRef_t sessionRef, taf_dcs_ProfileListHandlerFunc_t handlerPtr, void *contextPtr);
            void CleanupAllProfiles(Profile_List_Event_t *listEvent);
            void CreateIndividualProfile(taf_dcs_ProfileCtx_t *info);
            taf_dcs_ProfileCtx_t *GetProfileCtx(uint32_t index);
            void UpdateIndividualProfile(taf_dcs_ProfileCtx_t *distPtr, taf_dcs_ProfileCtx_t *srcPtr);
            le_result_t UpdateAllProfilesFromListEvent(Profile_List_Event_t *listEvent);
            void NotifyProfileListHandler(void *listEvent);
            le_result_t SendProfileListReq();
            le_result_t SendProfileModificationReq(int32_t profileId, telux::data::ProfileParams &params);

            std::shared_ptr<IDataProfileManager>        ProfileMgr;
            std::shared_ptr<taf_ProfileListCallback>    ListProfileCb;
            std::shared_ptr<taf_ProfileModifyCallback>  ModifyProfileCb;
            void show();

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
            uint32_t GetProfileNum();
        private:
            le_mem_PoolRef_t ListEventPool = NULL;
            le_mem_PoolRef_t ProfilePool = NULL;
            le_mem_PoolRef_t ListHandlerPool = NULL;
            le_ref_MapRef_t  ProfileRefMap = NULL;
            le_event_Id_t    ListReqEvent;
            le_dls_List_t    ProfileCtxList;
            taf_dcs_ProfileCtxs_t ProfilesListPtr = { 0 };
            le_thread_Ref_t ProfileEventThreadRef = NULL;
            uint32_t ProfileNum = 0;
    };

}
}

