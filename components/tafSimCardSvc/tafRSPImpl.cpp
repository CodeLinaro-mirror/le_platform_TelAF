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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include <telux/tel/PhoneFactory.hpp>
#include "tafRSP.hpp"

using namespace telux::tel;
using namespace telux::common;
using namespace tafsvc;

LE_MEM_DEFINE_STATIC_POOL(tafProfileListPool, TAF_SIMRSP_MAX_PROFILE,
                           sizeof(taf_simRsp_ProfileListNode_t));
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
void tafRspListener::onDownloadStatus(SlotId slotId, telux::tel::DownloadStatus status,
    telux::tel::DownloadErrorCause cause) {

    LE_INFO("onDownloadStatus");
    auto &rsp = taf_simRsp::GetInstance();
    taf_simRsp_DownloadEvent_t profileDownloadEvent;
    profileDownloadEvent.slotId = (taf_sim_Id_t) slotId;
    profileDownloadEvent.downloadStatus = (taf_simRsp_DownloadStatus_t) status;
    profileDownloadEvent.downloadErrorCause = (taf_simRsp_DownloadErrorCause_t) cause;

    le_event_Report(rsp.ProfileDownloadEventId, &profileDownloadEvent, sizeof(taf_simRsp_DownloadEvent_t));
}


void tafRspListener::onUserDisplayInfo(SlotId slotId, bool userConsentRequired,
    telux::tel::PolicyRuleMask mask) {

    LE_INFO("Is user consent required:");
    auto &rsp = taf_simRsp::GetInstance();
    taf_simRsp_UserConsentEvent_t profileUserConsentEvent;
    profileUserConsentEvent.slotId = (taf_sim_Id_t) slotId;
    profileUserConsentEvent.userConsentRequired = userConsentRequired;
    profileUserConsentEvent.mask = mask.to_ulong();
    le_event_Report(rsp.ProfileUserConsentEventId, &profileUserConsentEvent, sizeof(taf_simRsp_UserConsentEvent_t));
}

void tafRspListener::onConfirmationCodeRequired(SlotId slotId, std::string profileName) {


    LE_INFO(" Confirmation Code Required");
    auto &rsp = taf_simRsp::GetInstance();
    taf_simRsp_ConfirmationCodeEvent_t profileConfirmationCodeEvent;
    profileConfirmationCodeEvent.slotId = (taf_sim_Id_t) slotId;
    le_utf8_Copy(profileConfirmationCodeEvent.profileName, profileName.c_str(), TAF_SIMRSP_NAME_BYTES, NULL);
    le_event_Report(rsp.ProfileConfirmationCodeEventId, &profileConfirmationCodeEvent, sizeof(taf_simRsp_ConfirmationCodeEvent_t));
}
#endif

void taf_simRsp::Init(void)
{
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    simProfileManager = phoneFactory.getSimProfileManager();
    if (!simProfileManager)
    {
        LE_FATAL("Failed to get sim profile manager.");
    }
    else
    {
        telux::common::ServiceStatus simProfileMgrStatus = simProfileManager->getServiceStatus();
        if (simProfileMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
        {
            LE_DEBUG("Sim profile subsystem is not ready, waiting for it to be ready...");
            std::promise<telux::common::ServiceStatus> simProfileMgrProm;
            simProfileManager = phoneFactory.getSimProfileManager([&](telux::common::ServiceStatus status) {
                LE_INFO("Getting status:%d from sim profile manager", (int)status);
                if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    simProfileMgrProm.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                }
                else {
                    simProfileMgrProm.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
                }
            });
            std::future<telux::common::ServiceStatus> initFuture = simProfileMgrProm.get_future();
            std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                TAF_SIM_SUBSYSTEM_TIMEOUT));
            if (std::future_status::timeout == waitStatus)
            {
                LE_FATAL ("Timeout waiting for sim profile susbsytem");
            }
            else
            {
                simProfileMgrStatus = initFuture.get();
            }
        }
        if (simProfileMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
        {
            LE_DEBUG("Sim profile subsystem is ready.");
        }
        else
        {
            LE_FATAL("Fail to init sim profile subsystem");
        }
    }

    ProfileListPool = le_mem_InitStaticPool(tafProfileListPool,
                    TAF_SIMRSP_MAX_PROFILE, sizeof(taf_simRsp_ProfileListNode_t));
    ProfileListNodeRefMap = le_ref_CreateMap("tafRspProfileRefMap", TAF_SIMRSP_MAX_PROFILE);
    ProfileDownloadEventId = le_event_CreateId("ProfileDownloadEventId", sizeof(taf_simRsp_DownloadEvent_t));
    ProfileUserConsentEventId = le_event_CreateId("ProfileUserConsentEventId", sizeof(taf_simRsp_UserConsentEvent_t));
    ProfileConfirmationCodeEventId = le_event_CreateId("ProfileConfirmationCodeEventId", sizeof(taf_simRsp_ConfirmationCodeEvent_t));
    char eidPtr[TAF_SIM_EID_BYTES];
    GetEID((taf_sim_Id_t)TAF_SIM_EXTERNAL_SLOT_2, eidPtr, TAF_SIM_EID_BYTES);
    LE_INFO(" EID = %s", eidPtr);
}

taf_simRsp &taf_simRsp::GetInstance()
{
    static taf_simRsp instance;
    return instance;
}

le_result_t taf_simRsp::GetEID(taf_sim_Id_t slotId, char* eidPtr, size_t eidLen) {
    if (!simProfileManager) {
        LE_ERROR("GetEID: SimProfileManager is null");
        return LE_FAULT;
    }
    auto EidSynchronousPromise = std::make_shared<std::promise<std::string>>();
    SlotId slot = SlotId::DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slot = (SlotId)slotId;
    }
   auto eidCallback = [EidSynchronousPromise](std::string eid, telux::common::ErrorCode errorCode) {
    try {
        if (errorCode == telux::common::ErrorCode::SUCCESS) {
            LE_DEBUG("Request for GetEID sent successfully");
            EidSynchronousPromise->set_value(eid);
        } else {
            EidSynchronousPromise->set_value("");  // Set empty string on error
            LE_INFO("Request for GetEID failed with errorCode: %d", static_cast<int>(errorCode));
        }
    }
    catch (const std::future_error &e) {
        // Only occurs if promise already satisfied
        if (e.code() == std::make_error_code(std::future_errc::promise_already_satisfied)) {
            LE_WARN(" eidcallback: Promise already satisfied  %s", e.what());
        } else {
            LE_ERROR("eidcallback: Unexpected future_error: %s", e.what());
        }
    }
    catch (const std::exception& e) {
        LE_ERROR("Exception in eidCallback: %s", e.what());
    }
    catch (...) {
        LE_ERROR("Unknown error in eidCallback");
    }
};
    Status status = simProfileManager->requestEid(slot, eidCallback);
    if (status != telux::common::Status::SUCCESS) {
        LE_ERROR("GetEID: Failed to send request, Status = %d", static_cast<int>(status));
        return LE_FAULT;
    }
    std::future<std::string> eidResult = EidSynchronousPromise->get_future();
    std::chrono::seconds span(SESSION_TIMEOUT);
    std::future_status waitStatus = eidResult.wait_for(span);
    if (waitStatus == std::future_status::timeout) {
        LE_ERROR("GetEID: Timeout waiting for EID result");
        return LE_TIMEOUT;
    }
    std::string eidValue = eidResult.get();
    if (eidValue.empty()) {
        LE_ERROR("GetEID: Failed to retrieve EID");
        return LE_FAULT;
    }
    le_result_t copyResult = le_utf8_Copy(eidPtr, eidValue.c_str(), eidLen, NULL);
    if (copyResult != LE_OK) {
        LE_ERROR("GetEID: Failed to copy EID, buffer too small");
        return LE_OVERFLOW;
    }
    LE_INFO("GetEID: Request sent successfully");
    return LE_OK;
}

le_result_t taf_simRsp::AddProfile(taf_sim_Id_t slotId, const char* activationCode, const char* confirmationCode,
        bool userConsentSupported) {

    SlotId slot = (SlotId) slotId;
    if (!simProfileManager) {
        LE_ERROR("AddProfile: SimProfileManager is null");
        return LE_FAULT;
    }
    if( taf_sim_SelectCard(slotId)!= LE_OK) {
        slot = SlotId::DEFAULT_SLOT_ID;
    }

    if (activationCode == nullptr || activationCode[0] == '\0') {
        LE_ERROR("AddProfile: activationCode is null or empty");
        return LE_BAD_PARAMETER;
    }
    auto profilePromisePtr = std::make_shared<std::promise<le_result_t>>();
    auto responseCb = [profilePromisePtr](telux::common::ErrorCode errorCode) {
        le_result_t result = (errorCode == telux::common::ErrorCode::SUCCESS) ? LE_OK : LE_FAULT;
        try{
            profilePromisePtr->set_value(result);
        }
        catch (const std::future_error &e) {
            if (e.code() == std::make_error_code(std::future_errc::promise_already_satisfied)) {
                LE_WARN("AddProfile callback: Promise already satisfied  %s", e.what());
            } else {
                LE_ERROR("AddProfile callback: Unexpected future_error: %s", e.what());
            }
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in callback: %s", e.what());
        }
        catch (...)
        {
            LE_ERROR("Unknown error in AddProfile callback");
        }
    };
    Status status = simProfileManager->addProfile(slot, activationCode, confirmationCode,
            userConsentSupported, responseCb);

    if (status != Status::SUCCESS) {
        LE_ERROR("AddProfile: Failed to send request. Status = %d", static_cast<int>(status));
        return LE_FAULT;
    }
    std::chrono::seconds span(SESSION_TIMEOUT);
    std::future<le_result_t> futureResult = profilePromisePtr->get_future();
    std::future_status waitStatus = futureResult.wait_for(span);
    if (waitStatus == std::future_status::timeout) {
        LE_ERROR("AddProfile: Timeout waiting for AddProfile result");
        return LE_TIMEOUT;
    }
    return futureResult.get();
}

le_result_t taf_simRsp::DeleteProfile( taf_sim_Id_t slotId, uint32_t profileId) {
    SlotId slot = (SlotId) slotId;
    if (!simProfileManager) {
        LE_ERROR("DeleteProfile: SimProfileManager is null");
        return LE_FAULT;
    }
    if( taf_sim_SelectCard(slotId)!= LE_OK) {
        slot = SlotId::DEFAULT_SLOT_ID;
    }
    auto deleteProfilePromisePtr = std::make_shared<std::promise<le_result_t>>();
    auto responseCb = [deleteProfilePromisePtr](telux::common::ErrorCode errorCode) {
        le_result_t result = (errorCode == telux::common::ErrorCode::SUCCESS) ? LE_OK : LE_FAULT;
        try
        {
            deleteProfilePromisePtr->set_value(result);
        }
        catch (const std::future_error &e) {
            if (e.code() == std::make_error_code(std::future_errc::promise_already_satisfied)) {
                LE_WARN("DeleteProfile callback: Promise already satisfied  %s", e.what());
            } else {
                LE_ERROR("DeleteProfile callback: Unexpected future_error: %s", e.what());
            }
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in DeleteProfile callback: %s", e.what());
        }
        catch (...)
        {
            LE_ERROR("Unknown error in DeleteProfile callback");
        }
    };
    Status status = simProfileManager->deleteProfile(slot, profileId, responseCb);
    if (status != Status::SUCCESS) {
        LE_ERROR("Failed to send delete profile request. Status: %d", static_cast<int>(status));
        return LE_FAULT;
    }
    std::chrono::seconds span(SESSION_TIMEOUT);
    std::future<le_result_t> futureResult = deleteProfilePromisePtr ->get_future();
    std::future_status waitStatus = futureResult.wait_for(span);
    if (waitStatus == std::future_status::timeout) {
        LE_ERROR("DeleteProfile: Timeout waiting for DeleteProfile result");
        return LE_TIMEOUT;
    }
    LE_INFO("Delete profile request sent successfully");
    return futureResult.get();
}

le_result_t taf_simRsp::SetProfile( taf_sim_Id_t slotId, uint32_t profileId, bool enable) {
    SlotId slot = (SlotId) slotId;
    if (!simProfileManager) {
        LE_ERROR("SetProfile: SimProfileManager is null");
        return LE_FAULT;
    }
    auto setProfilePromisePtr = std::make_shared<std::promise<le_result_t>>();
    if( taf_sim_SelectCard(slotId)!= LE_OK) {
        slot = SlotId::DEFAULT_SLOT_ID;
    }
    auto responseCb = [setProfilePromisePtr](telux::common::ErrorCode errorCode) {
        le_result_t result = (errorCode == telux::common::ErrorCode::SUCCESS) ? LE_OK : LE_FAULT;
        try
        {
            setProfilePromisePtr->set_value(result);
        }
        catch (const std::future_error &e) {
            if (e.code() == std::make_error_code(std::future_errc::promise_already_satisfied)) {
                LE_WARN("SetProfile callback: Promise already satisfied  %s", e.what());
            } else {
                LE_ERROR("SetProfile Unexpected future_error: %s", e.what());
            }
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in SetProfile callback: %s", e.what());
        }
        catch (...)
        {
            LE_ERROR("Unknown error in SetProfile callback");
        }
    };

    Status status = simProfileManager->setProfile(slot, profileId, enable, responseCb);
    if (status != Status::SUCCESS) {
        LE_ERROR("Failed to send set profile request. Status: %d", static_cast<int>(status));
        return LE_FAULT;
    }
    std::future<le_result_t> futureResult =  setProfilePromisePtr->get_future();
    std::chrono::seconds span(SESSION_TIMEOUT);
    std::future_status waitStatus = futureResult.wait_for(span);
    if (waitStatus == std::future_status::timeout) {
        LE_ERROR("setProfile: Timeout waiting for setProfile result");
        return LE_TIMEOUT;
    }
    LE_INFO("Set profile request sent successfully");
    return futureResult.get();
}

le_result_t taf_simRsp::UpdateNickName( taf_sim_Id_t slotId, uint32_t profileId,const char* nickName){
    if (nickName == nullptr || strlen(nickName) == 0) {
        LE_ERROR("Nickname is null or empty");
        return LE_FAULT;
    }
    if (!simProfileManager) {
        LE_ERROR("UpdateNickName: SimProfileManager is null");
        return LE_FAULT;
    }
    SlotId slot = (SlotId) slotId;
    if( taf_sim_SelectCard(slotId)!= LE_OK) {
        slot = SlotId::DEFAULT_SLOT_ID;
    }
    auto updateNickNamePromise = std::make_shared<std::promise<le_result_t>>();
    auto responseCb = [updateNickNamePromise](telux::common::ErrorCode errorCode) {
        le_result_t result = (errorCode == telux::common::ErrorCode::SUCCESS) ? LE_OK : LE_FAULT;
        try{
            updateNickNamePromise->set_value(result);
        }
        catch (const std::future_error &e) {
           if (e.code() == std::make_error_code(std::future_errc::promise_already_satisfied)) {
                LE_WARN("UpdateNickName callback: Promise already satisfied  %s", e.what());
            } else {
                LE_ERROR("UpdateNickName Unexpected future_error: %s", e.what());
            }
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in callback: %s", e.what());
        }
        catch (...)
        {
            LE_ERROR("Unknown error in UpdateNickName callback");
        }
    };
    Status status = simProfileManager->updateNickName(slot, profileId, nickName, responseCb);
    if (status != Status::SUCCESS) {
        LE_ERROR("Failed to send update nickname request. Status: %d", static_cast<int>(status));
        return LE_FAULT;
    }
    std::chrono::seconds span(SESSION_TIMEOUT);
    std::future<le_result_t> futureResult =  updateNickNamePromise->get_future();
    std::future_status waitStatus = futureResult.wait_for(span);
    if (waitStatus == std::future_status::timeout) {
        LE_ERROR("UpdateNickName: Timeout waiting for UpdateNickName result");
        return LE_TIMEOUT;
    }
    LE_INFO("Update nick name request sent successfully");
    return futureResult.get();
}

le_result_t taf_simRsp::RequestProfileList(taf_sim_Id_t slotId, taf_simRsp_ProfileListNodeRef_t* profileListPtr, size_t *profileCount) {
    SlotId slot = (SlotId) slotId;
    auto promise = std::make_shared<std::promise<le_result_t>>();
    std::future<le_result_t> futureResult = promise->get_future();
    std::chrono::seconds span(SESSION_TIMEOUT);
    le_dls_Link_t* linkPtr = NULL;
    uint8_t count = 0;

    if (simProfileManager) {
        if (taf_sim_SelectCard(slotId) != LE_OK) {
            slot = SlotId::DEFAULT_SLOT_ID;
        }
        auto responseCb = [promise](const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
                telux::common::ErrorCode errorCode){
            taf_simRsp_ProfileListEvent_t profileListEvent;
            if (errorCode == telux::common::ErrorCode::SUCCESS && !profiles.empty()) {
                int i = 0;
                for (const auto &profile : profiles) {
                    if (profile) {
                        profileListEvent.simProfileInfo[i].profileId = profile->getProfileId();
                        profileListEvent.simProfileInfo[i].profileType = static_cast<taf_simRsp_ProfileType_t>(profile->getType());
                        le_utf8_Copy(profileListEvent.simProfileInfo[i].iccid, profile->getIccid().c_str(), TAF_SIM_ICCID_BYTES, NULL);
                        profileListEvent.simProfileInfo[i].isActive = profile->isActive();
                        le_utf8_Copy(profileListEvent.simProfileInfo[i].nickName, profile->getNickName().c_str(), TAF_SIMRSP_NAME_BYTES, NULL);
                        le_utf8_Copy(profileListEvent.simProfileInfo[i].name, profile->getName().c_str(), TAF_SIMRSP_NAME_BYTES, NULL);
                        le_utf8_Copy(profileListEvent.simProfileInfo[i].spn, profile->getSPN().c_str(), TAF_SIMRSP_SPN_LEN, NULL);
                        profileListEvent.simProfileInfo[i].iconType = static_cast<taf_simRsp_IconType_t>(profile->getIconType());
                        profileListEvent.simProfileInfo[i].profileClass = static_cast<taf_simRsp_ProfileClass_t>(profile->getClass());
                        profileListEvent.simProfileInfo[i].mask = profile->getPolicyRule().to_ulong();
                        i++;
                    }
                }
                profileListEvent.profileCount = i;
                profileListEvent.result = LE_OK;
            } else {
                profileListEvent.result = LE_FAULT;
            }
            auto &rsp = taf_simRsp::GetInstance();
            rsp.UpdateProfileList(&profileListEvent);
            try {
                promise->set_value(profileListEvent.result);
            } catch (const std::future_error &e) {
                LE_ERROR("Exception setting promise value: %s", e.what());
            }
        };
        telux::common::Status status = simProfileManager->requestProfileList(slot,responseCb);
        if (status == telux::common::Status::SUCCESS) {
            std::future_status waitStatus = futureResult.wait_for(span);
            if (waitStatus == std::future_status::timeout) {
                LE_INFO("Timeout waiting for profile list");
                return LE_TIMEOUT;
            }

            if (futureResult.get() == LE_OK) {
                linkPtr = le_dls_Peek(&ProfileList);
                while (linkPtr) {
                    taf_simRsp_ProfileListNode_t* profileNode = CONTAINER_OF(linkPtr, taf_simRsp_ProfileListNode_t, link);
                    memcpy((char *)&profileListPtr[count], (const char *)&profileNode->profileListRef, sizeof(taf_simRsp_ProfileListNodeRef_t));
                    count++;
                    linkPtr = le_dls_PeekNext(&ProfileList, linkPtr);
                }

                *profileCount = count;
                return LE_OK;
            }
        } else {
            LE_ERROR("Request profile list failed, status: %d", int(status));
        }
    } else {
        LE_ERROR("SimProfileManager is null");
    }
    return LE_FAULT;
}

le_result_t taf_simRsp::GetServerAddress( taf_sim_Id_t slotId, char* smdpAddress, size_t smdpLength,char* smdsAddress,
        size_t smdsLength) {
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    SlotId slot = (SlotId) slotId;
    auto profilePromise = std::make_shared<std::promise<le_result_t>>();
    std::future<le_result_t> futureResult = profilePromise->get_future();
    std::chrono::seconds span(SESSION_TIMEOUT);
    if( taf_sim_SelectCard(slotId)!= LE_OK) {
        slot = SlotId::DEFAULT_SLOT_ID;
    }
#endif
    auto  responseCb = [profilePromise](std::string smdpAddress,std::string smdsAddress, telux::common::ErrorCode errorCode)
    {
        le_result_t result = (errorCode == telux::common::ErrorCode::SUCCESS) ? LE_OK : LE_FAULT;
        try{
            auto &rsp = taf_simRsp::GetInstance();
            if (result == LE_OK) {
                LE_INFO("Request processed successfully \n");
                rsp.SetSmdpAddress(smdpAddress);
                rsp.SetSmdsAddress(smdsAddress);
            }
            profilePromise->set_value(result);
        }
        catch (const std::future_error &e) {
            if (e.code() == std::make_error_code(std::future_errc::promise_already_satisfied)) {
                LE_WARN("GetServerAddress callback: Promise already satisfied  %s", e.what());
            } else {
                LE_ERROR("GetServerAddress Unexpected future_error: %s", e.what());
            }
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in callback: %s", e.what());
        }
        catch (...)
        {
            LE_ERROR("Unknown error in GetServerAddress callback");
        }
    };
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    Status status = simProfileManager->requestServerAddress(slot,responseCb);
    if (status == Status::SUCCESS) {
        LE_INFO("Get server address request sent successfully");
        std::future_status waitStatus = futureResult.wait_for(span);
        if (waitStatus == std::future_status::timeout) {
            LE_INFO("Timeout waiting for ServerAddress");
            return LE_TIMEOUT;
        }
        if(futureResult.get() == LE_OK) {
                le_utf8_Copy(smdpAddress, SmdpAddress.c_str(), smdpLength, NULL);
                le_utf8_Copy(smdsAddress, SmdsAddress.c_str(), smdsLength, NULL);
                return LE_OK;
        }
    } else {
        LE_INFO("ERROR - Failed to get server address request, Status:%d", static_cast<int>(status));
        return LE_FAULT;
    }
    #else
        LE_ERROR("GetServerAddress: Not supported on this platform");
        return LE_UNSUPPORTED;
#endif
    return LE_FAULT;
}

le_result_t taf_simRsp::SetServerAddress( taf_sim_Id_t slotId, const char* smdpAddress) {
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    SlotId slot = (SlotId) slotId;
    auto profilePromise = std::make_shared<std::promise<le_result_t>>();
    if( taf_sim_SelectCard(slotId)!= LE_OK) {
        slot = SlotId::DEFAULT_SLOT_ID;
    }
    auto responseCb = [profilePromise](telux::common::ErrorCode errorCode) {
        le_result_t result = (errorCode == telux::common::ErrorCode::SUCCESS) ? LE_OK : LE_FAULT;
        try{
            profilePromise->set_value(result);
        }
        catch(const std::future_error &e)
        {
            if (e.code() == std::make_error_code(std::future_errc::promise_already_satisfied)) {
                LE_WARN("SetServerAddress callback: Promise already satisfied  %s", e.what());
            } else {
                LE_ERROR("SetServerAddress Unexpected future_error: %s", e.what());
            }
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in callback: %s", e.what());
        }
        catch (...)
        {
            LE_ERROR("Unknown error in RSIM callback");
        }
    };
    Status status = simProfileManager->setServerAddress(slot, smdpAddress, responseCb);
    if (status == Status::SUCCESS) {
        LE_INFO("Set server address request sent successfully");
        std::chrono::seconds span(SESSION_TIMEOUT);
        std::future<le_result_t> futureResult = profilePromise->get_future();
        std::future_status waitStatus = futureResult.wait_for(span);
        if (waitStatus == std::future_status::timeout) {
            LE_INFO("Timeout waiting for ServerAddress");
            return LE_TIMEOUT;
        }
        LE_DEBUG("Request processed successfully \n");
        return futureResult.get();  // Consider adding timeout if needed
    }
    else {
        LE_INFO("ERROR - Failed to set server address , Status:%d", static_cast<int>(status));
        return LE_FAULT;
    }
#endif
    return LE_FAULT;
}

le_result_t taf_simRsp::CreateProfileListNode() {

    // Create the node.  Get the memory from a memory pool previously created.
    taf_simRsp_ProfileListNode_t* profileNodePtr = (taf_simRsp_ProfileListNode_t*)le_mem_ForceAlloc(ProfileListPool);

    // Initialize the node's link.
    profileNodePtr->link = LE_DLS_LINK_INIT;

    // Add the node to the tail of the list by passing in the node's link.
    le_dls_Queue(&ProfileList, &(profileNodePtr->link));

    return LE_OK;
}

void taf_simRsp::UpdateProfileList(taf_simRsp_ProfileListEvent_t *profileListEvent)
{
    LE_INFO("UpdateProfileList");
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&ProfileList);
    uint8_t i = 0;
    uint8_t profileCount = profileListEvent->profileCount;
    uint8_t profileUpdateCount = 0;
    uint8_t profileId[profileCount] = {0};
    while (linkPtr != NULL) {
        taf_simRsp_ProfileListNode_t *profileNode = CONTAINER_OF(linkPtr, taf_simRsp_ProfileListNode_t, link);
        for (i = 0; i < profileCount; i++) {
            if (profileNode->profileInfo.profileId == profileListEvent->simProfileInfo[i].profileId) {
                //Update existing profile Id profile
                profileNode->profileInfo.profileType = profileListEvent->simProfileInfo[i].profileType;
                le_utf8_Copy(profileNode->profileInfo.iccid, profileListEvent->simProfileInfo[i].iccid, TAF_SIM_ICCID_BYTES, NULL);
                profileNode->profileInfo.isActive = profileListEvent->simProfileInfo[i].isActive;
                le_utf8_Copy(profileNode->profileInfo.nickName, profileListEvent->simProfileInfo[i].nickName, TAF_SIMRSP_NAME_BYTES, NULL);
                le_utf8_Copy(profileNode->profileInfo.name, profileListEvent->simProfileInfo[i].name, TAF_SIMRSP_NAME_BYTES, NULL);
                le_utf8_Copy(profileNode->profileInfo.spn, profileListEvent->simProfileInfo[i].spn, TAF_SIMRSP_SPN_LEN, NULL);
                profileNode->profileInfo.iconType = profileListEvent->simProfileInfo[i].iconType;
                profileNode->profileInfo.profileClass = profileListEvent->simProfileInfo[i].profileClass;
                profileId[profileUpdateCount] = profileNode->profileInfo.profileId;
                le_ref_DeleteRef(ProfileListNodeRefMap, profileNode->profileListRef);
                profileNode->profileListRef = (taf_simRsp_ProfileListNodeRef_t)le_ref_CreateRef(ProfileListNodeRefMap, profileNode);
                profileUpdateCount++;
                break;
            }
        }
        if (i >= profileCount) {
          le_dls_Link_t* nextLinkPtr = le_dls_PeekNext(&ProfileList, linkPtr);
          le_ref_DeleteRef(ProfileListNodeRefMap, profileNode->profileListRef);
          le_dls_Remove(&ProfileList, &profileNode->link);
          le_mem_Release(profileNode);
          linkPtr = nextLinkPtr;
          continue;
        }
        linkPtr = le_dls_PeekNext(&ProfileList, linkPtr);
    }
    if (profileUpdateCount < profileCount) {
        for (i = 0; i < profileCount; i++) {
            uint8_t j;
            for (j = 0; j < profileUpdateCount; j++) {
                if (profileId[j] == profileListEvent->simProfileInfo[i].profileId) {
                    break;
                }
            }
            if (j == profileUpdateCount) {
                // Create the node.  Get the memory from a memory pool previously created.
                taf_simRsp_ProfileListNode_t* profileNode = (taf_simRsp_ProfileListNode_t*)le_mem_ForceAlloc(ProfileListPool);

                profileNode->profileInfo.profileId = profileListEvent->simProfileInfo[i].profileId;
                profileNode->profileInfo.profileType = profileListEvent->simProfileInfo[i].profileType;
                le_utf8_Copy(profileNode->profileInfo.iccid, profileListEvent->simProfileInfo[i].iccid, TAF_SIM_ICCID_BYTES, NULL);
                profileNode->profileInfo.isActive = profileListEvent->simProfileInfo[i].isActive;
                le_utf8_Copy(profileNode->profileInfo.nickName, profileListEvent->simProfileInfo[i].nickName, TAF_SIMRSP_NAME_BYTES, NULL);
                le_utf8_Copy(profileNode->profileInfo.name, profileListEvent->simProfileInfo[i].name, TAF_SIMRSP_NAME_BYTES, NULL);
                le_utf8_Copy(profileNode->profileInfo.spn, profileListEvent->simProfileInfo[i].spn, TAF_SIMRSP_SPN_LEN, NULL);
                profileNode->profileInfo.iconType = profileListEvent->simProfileInfo[i].iconType;
                profileNode->profileInfo.profileClass = profileListEvent->simProfileInfo[i].profileClass;
                // Initialize the node's link.
                profileNode->link = LE_DLS_LINK_INIT;
                profileNode->profileListRef = (taf_simRsp_ProfileListNodeRef_t)le_ref_CreateRef(ProfileListNodeRefMap, profileNode);

                // Add the node to the tail of the list by passing in the node's link.
                le_dls_Queue(&ProfileList, &(profileNode->link));

            }
        }
    }
}

taf_simRsp_ProfileDownloadHandlerRef_t taf_simRsp::AddProfileDownloadHandler(taf_simRsp_ProfileDownloadHandlerFunc_t handlerPtr,
        void* contextPtr){

    le_event_HandlerRef_t handlerRef;
    LE_INFO("Add Profile download Handler");

    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler pointer is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("ProfileDownloadHandler", ProfileDownloadEventId,
            FirstLayerProfileDownloadHandler, (void*)handlerPtr);

    return (taf_simRsp_ProfileDownloadHandlerRef_t)(handlerRef);

}

void taf_simRsp::RemoveProfileDownloadHandler(taf_simRsp_ProfileDownloadHandlerRef_t handlerRef) {
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

void taf_simRsp::FirstLayerProfileDownloadHandler(void* reportPtr,
        void* secondLayerHandlerFunc)
{
    taf_simRsp_DownloadEvent_t* downloadEventPtr = (taf_simRsp_DownloadEvent_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(downloadEventPtr == NULL,"downloadEventPtr is NULL");

    taf_simRsp_ProfileDownloadHandlerFunc_t clientHandlerFunc =
        (taf_simRsp_ProfileDownloadHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(downloadEventPtr->slotId, downloadEventPtr->downloadStatus,  downloadEventPtr->downloadErrorCause,
                                         le_event_GetContextPtr());
}

le_result_t taf_simRsp::ProvideUserConsent(taf_sim_Id_t slotId, bool userConsent, taf_simRsp_UserConsentReasonType_t reason) {

    SlotId slot = (SlotId) slotId;
    if( taf_sim_SelectCard(slotId)!= LE_OK) {
        slot = SlotId::DEFAULT_SLOT_ID;
    }
    auto profilePromise = std::make_shared<std::promise<le_result_t>>();
    auto  responseCb =[profilePromise](telux::common::ErrorCode errorCode)
    {
        le_result_t result = (errorCode == telux::common::ErrorCode::SUCCESS) ? LE_OK : LE_FAULT;
        try{
            profilePromise->set_value(result);
        }
        catch(const std::future_error &e)
        {
            if (e.code() == std::make_error_code(std::future_errc::promise_already_satisfied)) {
                LE_WARN("ProvideUserConsent callback: Promise already satisfied  %s", e.what());
            } else {
                LE_ERROR("ProvideUserConsent Unexpected future_error: %s", e.what());
            }
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in callback: %s", e.what());
        }
        catch (...)
        {
            LE_ERROR("Unknown error in ProvideUserConsent callback");
        }
    };
    Status status = Status::FAILED;
    if (!simProfileManager) {
        LE_ERROR("simProfileManager is null");
        return LE_FAULT;
    }
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    status = simProfileManager->provideUserConsent(slot, userConsent,
            static_cast<telux::tel::UserConsentReasonType>(reason), responseCb);
#endif
#ifdef TARGET_SA415M
    status = simProfileManager->provideUserConsent(slot, userConsent,responseCb);
#endif
    if (status == Status::SUCCESS) {
        std::future<le_result_t> futureResult = profilePromise->get_future();
        std::chrono::seconds span(SESSION_TIMEOUT);
        std::future_status waitStatus = futureResult.wait_for(span);
        LE_DEBUG("ProvideUserConsent request sent successfully");
        if (std::future_status::timeout == waitStatus) {
            LE_INFO("Unable to read profile list");
            return LE_TIMEOUT;
        }
        return futureResult.get();
    } else {
        LE_INFO( "ERROR - Failed to send user consent request, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }
}

le_result_t taf_simRsp::ProvideConfirmationCode(taf_sim_Id_t slotId, const char* code, size_t codeLength) {

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    if (code == nullptr || codeLength == 0 || code[0] == '\0') {
        LE_ERROR("ProvideConfirmationCode: code is null or empty");
        return LE_BAD_PARAMETER;
    }
    SlotId slot = (SlotId) slotId;
    if( taf_sim_SelectCard(slotId)!= LE_OK) {
        slot = SlotId::DEFAULT_SLOT_ID;
    }
#endif
    if (!simProfileManager) {
        LE_ERROR("simProfileManager is null");
        return LE_FAULT;
    }
    auto confirmationPromise = std::make_shared<std::promise<le_result_t>>();
    auto  responseCb = [confirmationPromise](telux::common::ErrorCode errorCode)
    {
        try
        {
            le_result_t result = (errorCode == telux::common::ErrorCode::SUCCESS) ? LE_OK : LE_FAULT;
            confirmationPromise->set_value(result);
        }
        catch(const std::future_error &e)
        {
         if (e.code() == std::make_error_code(std::future_errc::promise_already_satisfied)) {
                LE_WARN("ProvideConfirmationCode callback: Promise already satisfied  %s", e.what());
            } else {
                LE_ERROR("ProvideConfirmationCode Unexpected future_error: %s", e.what());
            }
        }
        catch (const std::exception &e) {
            LE_ERROR("Exception in ProvideConfirmationCode: %s", e.what());
        }
        catch (...)
        {
            LE_ERROR("Unknown error in RSIM callback");
        }
    };

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    Status status = simProfileManager->provideConfirmationCode(slot, code, responseCb);
    if (status == Status::SUCCESS) {
        LE_INFO("ProvideConfirmationCode request sent successfully");
        std::future<le_result_t> futureResult = confirmationPromise->get_future();
        std::chrono::seconds span(SESSION_TIMEOUT);
        std::future_status waitStatus = futureResult.wait_for(span);
        if (std::future_status::timeout == waitStatus) {
            LE_INFO("Unable to read profile list");
            return LE_TIMEOUT;
        }
        return futureResult.get();

    } else {
        LE_INFO( "ERROR - Failed to send confirmation code request, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }
#else
    LE_ERROR("ProvideConfirmationCode: Not supported on this platform");
    return LE_UNSUPPORTED;
#endif
}

taf_simRsp_ProfileUserConsentHandlerRef_t taf_simRsp::AddProfileUserConsentHandler(taf_simRsp_ProfileUserConsentHandlerFunc_t handlerPtr,
        void* contextPtr){

    le_event_HandlerRef_t handlerRef;
    LE_INFO("Add Profile User consent Handler");

    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler pointer is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("ProfileUserConsentHandler", ProfileUserConsentEventId,
            FirstLayerProfileUserConsentHandler, (void*)handlerPtr);

    return (taf_simRsp_ProfileUserConsentHandlerRef_t)(handlerRef);

}

void taf_simRsp::RemoveProfileUserConsentHandler(taf_simRsp_ProfileUserConsentHandlerRef_t handlerRef) {
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

void taf_simRsp::FirstLayerProfileUserConsentHandler(void* reportPtr,
        void* secondLayerHandlerFunc)
{
    taf_simRsp_UserConsentEvent_t* userConsentEventPtr = (taf_simRsp_UserConsentEvent_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(userConsentEventPtr == NULL,"UserConsentEventPtr is NULL");

    taf_simRsp_ProfileUserConsentHandlerFunc_t clientHandlerFunc =
        (taf_simRsp_ProfileUserConsentHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc( userConsentEventPtr->slotId,  userConsentEventPtr->userConsentRequired, userConsentEventPtr->mask,
                                         le_event_GetContextPtr());
}

taf_simRsp_ProfileConfirmationCodeHandlerRef_t taf_simRsp::AddProfileConfirmationCodeHandler(taf_simRsp_ProfileConfirmationCodeHandlerFunc_t handlerPtr,
        void* contextPtr){

    le_event_HandlerRef_t handlerRef;
    LE_INFO("Add Profile User consent Handler");

    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler pointer is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("ProfileConfirmationCodeHandler", ProfileConfirmationCodeEventId,
            FirstLayerProfileConfirmationCodeHandler, (void*)handlerPtr);

    return (taf_simRsp_ProfileConfirmationCodeHandlerRef_t)(handlerRef);

}

void taf_simRsp::RemoveProfileConfirmationCodeHandler(taf_simRsp_ProfileConfirmationCodeHandlerRef_t handlerRef) {
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

void taf_simRsp::FirstLayerProfileConfirmationCodeHandler(void* reportPtr,
        void* secondLayerHandlerFunc)
{
    taf_simRsp_ConfirmationCodeEvent_t* confirmationCodeEventPtr = (taf_simRsp_ConfirmationCodeEvent_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(confirmationCodeEventPtr == NULL,"confirmationCodeEventPtr is NULL");

    taf_simRsp_ProfileConfirmationCodeHandlerFunc_t clientHandlerFunc =
        (taf_simRsp_ProfileConfirmationCodeHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc( confirmationCodeEventPtr->slotId,  confirmationCodeEventPtr->profileName,
                                         le_event_GetContextPtr());
}

taf_simRsp_ProfileListNodeRef_t taf_simRsp::GetProfileListNodeRef(uint32_t index)
{
    //Use requestProfileList from telsdk to update ProfileList.
    //Traverse and look for id, if present, return the reference type pointer.
    //If not present, add a node with that index, with default information.
    auto slot = taf_sim_GetSelectedCard();
    auto promise = std::make_shared<std::promise<le_result_t>>();
    std::future<le_result_t> futureResult = promise->get_future();
    std::chrono::seconds span(SESSION_TIMEOUT);
    le_dls_Link_t* linkPtr = NULL;
    if (simProfileManager) {

         auto responseCb = [promise](const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
            telux::common::ErrorCode errorCode){
            taf_simRsp_ProfileListEvent_t profileListEvent;
            memset(&profileListEvent, 0, sizeof(profileListEvent));
            if (errorCode == telux::common::ErrorCode::SUCCESS && !profiles.empty()) {
                int i = 0;
                for (const auto &profile : profiles) {
                    if (profile) {
                        profileListEvent.simProfileInfo[i].profileId = profile->getProfileId();
                        profileListEvent.simProfileInfo[i].profileType = static_cast<taf_simRsp_ProfileType_t>(profile->getType());
                        le_utf8_Copy(profileListEvent.simProfileInfo[i].iccid, profile->getIccid().c_str(), TAF_SIM_ICCID_BYTES, NULL);
                        profileListEvent.simProfileInfo[i].isActive = profile->isActive();
                        le_utf8_Copy(profileListEvent.simProfileInfo[i].nickName, profile->getNickName().c_str(), TAF_SIMRSP_NAME_BYTES, NULL);
                        le_utf8_Copy(profileListEvent.simProfileInfo[i].name, profile->getName().c_str(), TAF_SIMRSP_NAME_BYTES, NULL);
                        le_utf8_Copy(profileListEvent.simProfileInfo[i].spn, profile->getSPN().c_str(), TAF_SIMRSP_SPN_LEN, NULL);
                        profileListEvent.simProfileInfo[i].iconType = static_cast<taf_simRsp_IconType_t>(profile->getIconType());
                        profileListEvent.simProfileInfo[i].profileClass = static_cast<taf_simRsp_ProfileClass_t>(profile->getClass());
                        profileListEvent.simProfileInfo[i].mask = profile->getPolicyRule().to_ulong();
                        i++;
                    }
                }
                profileListEvent.profileCount = i;
                profileListEvent.result = LE_OK;
            } else {
                profileListEvent.result = LE_FAULT;
            }
            auto &rsp = taf_simRsp::GetInstance();
            rsp.UpdateProfileList(&profileListEvent);
            try {
                promise->set_value(profileListEvent.result);
            }
            catch (const std::future_error &e) {
                LE_ERROR("Exception setting promise value: %s", e.what());
            }
        };

        telux::common::Status status = simProfileManager->requestProfileList(SlotId(slot), responseCb);
        if (status == telux::common::Status::SUCCESS) {
            std::future_status waitStatus = futureResult.wait_for(span);
            if (waitStatus == std::future_status::timeout) {
                LE_INFO("Timeout waiting for profile list");
                return NULL;
            }
            if (futureResult.get() == LE_OK) {
                linkPtr = le_dls_Peek(&ProfileList);
                while (linkPtr) {
                    taf_simRsp_ProfileListNode_t* profileNode = CONTAINER_OF(linkPtr, taf_simRsp_ProfileListNode_t, link);
                    if (profileNode->profileInfo.profileId == index) {
                        return (taf_simRsp_ProfileListNodeRef_t)(profileNode->profileListRef);
                    }
                    linkPtr = le_dls_PeekNext(&ProfileList, linkPtr);
                }
            }
        } else {
            LE_ERROR("Request profile list failed, status: %d", int(status));
        }
    } else {
        LE_ERROR("SimProfileManager is null");
    }
    return NULL;
}

uint32_t taf_simRsp::GetProfileIndex
(
    taf_simRsp_ProfileListNodeRef_t profileRef
)
{
    taf_simRsp_ProfileListNode_t* profilePtr = (taf_simRsp_ProfileListNode_t*)le_ref_Lookup(ProfileListNodeRefMap, profileRef);
    if(profilePtr == NULL)
    {
        LE_INFO("Profile not found");
        return 0;
    }
    return (uint32_t)(profilePtr->profileInfo.profileId);
}

taf_simRsp_ProfileType_t taf_simRsp::GetProfileType
(
    taf_simRsp_ProfileListNodeRef_t profileRef
)
{
    taf_simRsp_ProfileListNode_t* profilePtr = (taf_simRsp_ProfileListNode_t*)le_ref_Lookup(ProfileListNodeRefMap, profileRef);
    if(profilePtr == NULL)
    {
        LE_INFO("Profile not found");
        return (taf_simRsp_ProfileType_t)(-1);
    }
    return (taf_simRsp_ProfileType_t)(profilePtr->profileInfo.profileType);
}

le_result_t taf_simRsp::GetIccid
(
    taf_simRsp_ProfileListNodeRef_t profileRef,
    char* iccidPtr,
    size_t iccidLen
)
{
    taf_simRsp_ProfileListNode_t* profilePtr = (taf_simRsp_ProfileListNode_t*)le_ref_Lookup(ProfileListNodeRefMap, profileRef);
    if(profilePtr == NULL)
    {
        LE_INFO("Profile not found");
        return LE_FAULT;
    }
    if(iccidLen != TAF_SIM_ICCID_BYTES)
        return LE_FAULT;
    le_utf8_Copy(iccidPtr, profilePtr->profileInfo.iccid, TAF_SIM_ICCID_BYTES, NULL);
    return LE_OK;
}

bool taf_simRsp::GetProfileActiveStatus
(
    taf_simRsp_ProfileListNodeRef_t profileRef
)
{
    taf_simRsp_ProfileListNode_t* profilePtr = (taf_simRsp_ProfileListNode_t*)le_ref_Lookup(ProfileListNodeRefMap, profileRef);
    if(profilePtr == NULL)
    {
        LE_INFO("Profile not found");
        return false;
    }
    return (bool)(profilePtr->profileInfo.isActive);
}

le_result_t taf_simRsp::GetNickName
(
    taf_simRsp_ProfileListNodeRef_t profileRef,
    char* nickNamePtr,
    size_t nickNameLen
)
{
    taf_simRsp_ProfileListNode_t* profilePtr = (taf_simRsp_ProfileListNode_t*)le_ref_Lookup(ProfileListNodeRefMap, profileRef);
    if(profilePtr == NULL)
    {
        LE_INFO("Profile not found");
        return LE_FAULT;
    }
    if(nickNameLen != TAF_SIMRSP_NAME_BYTES)
        return LE_FAULT;
    le_utf8_Copy(nickNamePtr, profilePtr->profileInfo.nickName, TAF_SIMRSP_NAME_BYTES, NULL);
    return LE_OK;
}

le_result_t taf_simRsp::GetName
(
    taf_simRsp_ProfileListNodeRef_t profileRef,
    char * namePtr,
    size_t nameLen
)
{
    taf_simRsp_ProfileListNode_t* profilePtr = (taf_simRsp_ProfileListNode_t*)le_ref_Lookup(ProfileListNodeRefMap, profileRef);
    if(profilePtr == NULL)
    {
        LE_INFO("Profile not found");
        return LE_FAULT;
    }
    if(nameLen != TAF_SIMRSP_NAME_BYTES)
        return LE_FAULT;
    le_utf8_Copy(namePtr, profilePtr->profileInfo.name, TAF_SIMRSP_NAME_BYTES, NULL);
    return LE_OK;
}

le_result_t taf_simRsp::GetSpn
(
    taf_simRsp_ProfileListNodeRef_t profileRef,
    char * spnPtr,
    size_t spnLen
)
{
    taf_simRsp_ProfileListNode_t* profilePtr = (taf_simRsp_ProfileListNode_t*)le_ref_Lookup(ProfileListNodeRefMap, profileRef);
    if(profilePtr == NULL)
    {
        LE_INFO("Profile not found");
        return LE_FAULT;
    }
    if(spnLen != TAF_SIMRSP_NAME_BYTES)
        return LE_FAULT;
    le_utf8_Copy(spnPtr, profilePtr->profileInfo.spn, TAF_SIMRSP_SPN_LEN, NULL);
    return LE_OK;
}

taf_simRsp_IconType_t taf_simRsp::GetIconType
(
    taf_simRsp_ProfileListNodeRef_t profileRef
)
{
    taf_simRsp_ProfileListNode_t* profilePtr = (taf_simRsp_ProfileListNode_t*)le_ref_Lookup(ProfileListNodeRefMap, profileRef);
    if(profilePtr == NULL)
    {
        LE_INFO("Profile not found");
        return (taf_simRsp_IconType_t)(-1);
    }
    return (taf_simRsp_IconType_t)(profilePtr->profileInfo.iconType);
}

taf_simRsp_ProfileClass_t taf_simRsp::GetProfileClass
(
    taf_simRsp_ProfileListNodeRef_t profileRef
)
{
    taf_simRsp_ProfileListNode_t* profilePtr = (taf_simRsp_ProfileListNode_t*)le_ref_Lookup(ProfileListNodeRefMap, profileRef);
    if(profilePtr == NULL)
    {
        LE_INFO("Profile not found");
        return (taf_simRsp_ProfileClass_t)(0);
    }
    return (taf_simRsp_ProfileClass_t)(profilePtr->profileInfo.profileClass);
}

uint32_t taf_simRsp::GetMask
(
    taf_simRsp_ProfileListNodeRef_t profileRef
)
{
    taf_simRsp_ProfileListNode_t* profilePtr = (taf_simRsp_ProfileListNode_t*)le_ref_Lookup(ProfileListNodeRefMap, profileRef);
    if(profilePtr == NULL)
    {
        LE_INFO("Profile not found");
        return (uint32_t)(0);
    }
    return (uint32_t)(profilePtr->profileInfo.mask);
}
