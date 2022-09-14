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
#include <telux/tel/PhoneFactory.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/tel/SimProfile.hpp>
#include <telux/tel/SimProfileManager.hpp>
#include <telux/tel/SimProfileListener.hpp>
#include <telux/tel/SimProfileDefines.hpp>
#include <telux/common/DeviceConfig.hpp>
#include "tafSvcIF.hpp"

#define SESSION_TIMEOUT 60

using namespace telux::tel;
using namespace telux::common;
using namespace std;

typedef struct taf_rsp_SimProfileInfo
{
    uint32_t         profileId;
    taf_rsp_ProfileType_t    profileType;
    char         iccid[TAF_RSP_ICCID_LEN];
    bool           isActive;
    char         nickName[TAF_RSP_NICKNAME_LEN];
    char         name[TAF_RSP_NAME_LEN];
    char         spn[TAF_RSP_SPN_LEN];
    taf_rsp_IconType_t       iconType;
    taf_rsp_ProfileClass_t   profileClass;
    uint8_t          mask;
}taf_rsp_SimProfileInfo_t;

typedef struct taf_rsp_ProfileListNode
{
    taf_rsp_ProfileListNodeRef_t profileListRef;
    taf_rsp_SimProfileInfo_t    profileInfo;
    le_dls_Link_t               link;
}taf_rsp_ProfileListNode_t;

typedef struct
{
    taf_rsp_SimProfileInfo_t    simProfileInfo[TAF_RSP_MAX_PROFILE];
    int                         profileCount;
    le_result_t                 result;
}taf_rsp_ProfileListEvent_t;

typedef struct
{
    taf_sim_Id_t                   slotId;
    taf_rsp_DownloadStatus_t       downloadStatus;
    taf_rsp_DownloadErrorCause_t   downloadErrorCause;
}taf_rsp_DownloadEvent_t;

typedef struct
{
    taf_sim_Id_t                   slotId;
    bool                           userConsentRequired;
    uint8_t                        mask;
}taf_rsp_UserConsentEvent_t;

typedef struct
{
    taf_sim_Id_t                   slotId;
    char                           profileName[100];
}taf_rsp_ConfirmationCodeEvent_t;

namespace telux {
    namespace tafsvc {
        class tafRspListener : public telux::tel::ISimProfileListener {
            public:
#ifdef TARGET_SA515M

            void onDownloadStatus(SlotId slotId, telux::tel::DownloadStatus status,
                    telux::tel::DownloadErrorCause cause) override;
            void onUserDisplayInfo(SlotId slotId, bool userConsentRequired,
                    telux::tel::PolicyRuleMask mask) override;
            void onConfirmationCodeRequired(SlotId slotId, std::string profileName) override;
#endif
        };
        class tafRspCallback {
            public:
                void onProfileListResponse(
                        const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
                        telux::common::ErrorCode error);
                tafRspCallback(){};
                ~tafRspCallback(){};

                static void onEidResponse(std::string eid, telux::common::ErrorCode errorCode);
                void onResponseCallback(telux::common::ErrorCode error);
                void onServerAddressResponse(std::string smdpAddress,
                        std::string smdsAddress, telux::common::ErrorCode error);

        };

        class taf_rsp :public ITafSvc {
            public:
                void Init(void);
                static taf_rsp &GetInstance();
                taf_rsp() {};
                ~taf_rsp() {};
                le_event_Id_t ProfileListEventId;
                le_event_Id_t ProfileDownloadEventId;
                le_event_Id_t ProfileUserConsentEventId;
                le_event_Id_t ProfileConfirmationCodeEventId;
            private:
                std::shared_ptr<telux::tel::ISimProfileManager> simProfileManager = nullptr;
                std::shared_ptr<telux::tel::ISimProfileListener> rspListener;

                le_mem_PoolRef_t ProfileListPool = NULL;
                le_mem_PoolRef_t ProfileListEventPool = NULL;

                le_dls_List_t    ProfileList;
                le_thread_Ref_t ProfileListEventThreadRef = NULL;

                std::string SmdpAddress;
                std::string SmdsAddress;
                le_result_t CreateProfileListNode();
                le_ref_MapRef_t ProfileListNodeRefMap = NULL;

            public:
                std::promise<std:: string> EidSynchronousPromise;
                std::promise<le_result_t> ProfileSyncPromise;
                le_result_t  GetEID( taf_sim_Id_t slotId, char* eidPtr, size_t eidLen);
                le_result_t AddProfile(taf_sim_Id_t slotId, const char* activationCode,
                     const char* confirmationCode, bool userConsentSupported);
                le_result_t DeleteProfile( taf_sim_Id_t slotId, uint32_t profileId);
                le_result_t SetProfile( taf_sim_Id_t slotId, uint32_t profileId, bool enable);
                le_result_t UpdateNickName( taf_sim_Id_t slotId, uint32_t profileId,
                         const char* nickName);
                le_result_t RequestProfileList( taf_sim_Id_t slotId, taf_rsp_ProfileListNodeRef_t* profileListPtr, size_t *profileCount);
                le_result_t GetServerAddress( taf_sim_Id_t slotId, char* smdpAddress, size_t smdpLength,char* smdsAddress,
                                             size_t smdsLength);
                le_result_t SetServerAddress( taf_sim_Id_t slotId, const char* smdpAddress);

                le_result_t ProvideUserConsent(taf_sim_Id_t slot, bool userConsent, taf_rsp_UserConsentReasonType_t reason);
                le_result_t ProvideConfirmationCode( taf_sim_Id_t slotId,const char* code, size_t codeLength);
                le_mem_PoolRef_t getProfileListEventPool() {
                    return ProfileListEventPool;
                };
                le_event_Id_t getProfileListEventId() {
                    return ProfileListEventId;
                }
                void SetSmdpAddress(string smdpAddress) {
                    SmdpAddress = smdpAddress;
                }
                void SetSmdsAddress(string smdsAddress) {
                    SmdsAddress = smdsAddress;
                }
                static void UpdateProfileHandler(void *profileEvent);
                void UpdateProfileList(taf_rsp_ProfileListEvent_t *profileListEvent);
                static void* ProfileAddHandlerThread(void* contextPtr);

                taf_rsp_ProfileDownloadHandlerRef_t AddProfileDownloadHandler(taf_rsp_ProfileDownloadHandlerFunc_t handlerPtr, void* contextPtr);
                void RemoveProfileDownloadHandler(taf_rsp_ProfileDownloadHandlerRef_t handlerRef);
                static void FirstLayerProfileDownloadHandler(void* reportPtr, void* secondLayerHandlerFunc);

                taf_rsp_ProfileUserConsentHandlerRef_t AddProfileUserConsentHandler(taf_rsp_ProfileUserConsentHandlerFunc_t handlerPtr,void* contextPtr);
                void RemoveProfileUserConsentHandler(taf_rsp_ProfileUserConsentHandlerRef_t handlerRef);
                static void FirstLayerProfileUserConsentHandler(void* reportPtr, void* secondLayerHandlerFunc);

               taf_rsp_ProfileConfirmationCodeHandlerRef_t AddProfileConfirmationCodeHandler(taf_rsp_ProfileConfirmationCodeHandlerFunc_t handlerPtr,
                                                        void* contextPtr);
                void RemoveProfileConfirmationCodeHandler(taf_rsp_ProfileConfirmationCodeHandlerRef_t handlerRef);
                static void FirstLayerProfileConfirmationCodeHandler(void* reportPtr,void* secondLayerHandlerFunc);
                taf_rsp_ProfileListNodeRef_t GetProfileListNodeRef(uint32_t index);
                uint32_t GetProfileIndex(taf_rsp_ProfileListNodeRef_t profileRef);
                taf_rsp_ProfileType_t GetProfileType(taf_rsp_ProfileListNodeRef_t profileRef);
                le_result_t GetIccid(taf_rsp_ProfileListNodeRef_t profileRef, char* iccidPtr, size_t iccidLen);
                bool GetProfileActiveStatus(taf_rsp_ProfileListNodeRef_t profileRef);
                le_result_t GetNickName(taf_rsp_ProfileListNodeRef_t profileRef, char* nickNamePtr, size_t nickNameLen);
                le_result_t GetName(taf_rsp_ProfileListNodeRef_t profileRef,char * namePtr,size_t nameLen);
                le_result_t GetSpn(taf_rsp_ProfileListNodeRef_t profileRef, char * spnPtr, size_t spnLen);
                taf_rsp_IconType_t GetIconType(taf_rsp_ProfileListNodeRef_t profileRef);
                taf_rsp_ProfileClass_t GetProfileClass(taf_rsp_ProfileListNodeRef_t profileRef);
                uint32_t GetMask(taf_rsp_ProfileListNodeRef_t profileRef);
        };
    }
}
