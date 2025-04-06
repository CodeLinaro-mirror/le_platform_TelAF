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
 *  Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafDcsProfileImpl.cpp
 * @brief      This file provides the implementation of taf data profile management.
 */
#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include <telux/tel/PhoneFactory.hpp>
#include "telux/common/CommonDefines.hpp"
#include "tafSvcIF.hpp"
#include "tafDcsConnectionImpl.hpp"
#include "tafDcsProfileImpl.hpp"

using namespace telux::data;
using namespace telux::common;
using namespace tafsvc;

LE_MEM_DEFINE_STATIC_POOL(tafProfilePool, TAF_DCS_PROFILE_LIST_MAX_ENTRY, sizeof(taf_dcs_ProfileCtx_t));
LE_MEM_DEFINE_STATIC_POOL(tafProfileEvent, TAF_DCS_PROFILE_LIST_MAX_ENTRY, sizeof(taf_dcs_ProfileCtxs_t));


// require profile list handler
void taf_ProfileListCallback::onProfileListResponse(
    const std::vector<std::shared_ptr<telux::data::DataProfile>> &profiles,
    telux::common::ErrorCode error)
{
    auto &myProfile = taf_DataProfile::GetInstance();
    int num = 0;
    taf_dcs_ProfileCtxs_t *contexts = (taf_dcs_ProfileCtxs_t *)le_mem_ForceAlloc(myProfile.getListEventPool());
    memset(contexts, 0, sizeof(taf_dcs_ProfileCtxs_t));
    Profile_List_Event_t listEvent;

    for (auto &profile : profiles) {
        if (profile) {
            LE_DEBUG("id: %d, name: %s, apn: %s, username: %s, password: %s",
                      profile->getId(), profile->getName().c_str(), profile->getApn().c_str(),
                      profile->getUserName().c_str(), profile->getPassword().c_str());
            LE_DEBUG("IP Family: %d, Tech Perf: %d, Auth Type: %d",
                     (uint32_t)profile->getIpFamilyType(), (uint32_t)profile->getTechPreference(),
                     (uint32_t)profile->getAuthProtocolType());

            contexts->item[num].info.index = profile->getId();
            contexts->item[num].info.tech  = myProfile.MapTechPreference(profile->getTechPreference());
            le_utf8_Copy(contexts->item[num].info.name, profile->getName().c_str(), TAF_DCS_NAME_MAX_LEN, NULL);
            le_utf8_Copy(contexts->item[num].apn, profile->getApn().c_str(), TAF_DCS_NAME_MAX_LEN, NULL);

            contexts->item[num].apnType =0;
            telux::data::ApnTypes ApnTypes = profile->getApnTypes();
            uint16_t apnValue = (uint16_t)(ApnTypes.to_ulong());
            if( (apnValue & TAF_DCS_APN_TYPE_DEFAULT) == TAF_DCS_APN_TYPE_DEFAULT )
                contexts->item[num].apnType |=TAF_DCS_APN_TYPE_DEFAULT;
            if( (apnValue & TAF_DCS_APN_TYPE_IMS) == TAF_DCS_APN_TYPE_IMS )
                contexts->item[num].apnType |=TAF_DCS_APN_TYPE_IMS;
            if( (apnValue & TAF_DCS_APN_TYPE_MMS) == TAF_DCS_APN_TYPE_MMS )
                contexts->item[num].apnType |=TAF_DCS_APN_TYPE_MMS;
            if( (apnValue & TAF_DCS_APN_TYPE_DUN) == TAF_DCS_APN_TYPE_DUN )
                contexts->item[num].apnType |=TAF_DCS_APN_TYPE_DUN;
            if( (apnValue & TAF_DCS_APN_TYPE_SUPL) == TAF_DCS_APN_TYPE_SUPL )
                contexts->item[num].apnType |=TAF_DCS_APN_TYPE_SUPL;
            if( (apnValue & TAF_DCS_APN_TYPE_HIPRI) == TAF_DCS_APN_TYPE_HIPRI )
                contexts->item[num].apnType |=TAF_DCS_APN_TYPE_HIPRI;
            if( (apnValue & TAF_DCS_APN_TYPE_FOTA) == TAF_DCS_APN_TYPE_FOTA )
                contexts->item[num].apnType |=TAF_DCS_APN_TYPE_FOTA;
            if( (apnValue & TAF_DCS_APN_TYPE_CBS) == TAF_DCS_APN_TYPE_CBS )
                contexts->item[num].apnType |=TAF_DCS_APN_TYPE_CBS;
            if( (apnValue & TAF_DCS_APN_TYPE_IA) == TAF_DCS_APN_TYPE_IA )
                contexts->item[num].apnType |=TAF_DCS_APN_TYPE_IA;
            if( (apnValue & TAF_DCS_APN_TYPE_EMERGENCY) == TAF_DCS_APN_TYPE_EMERGENCY )
                contexts->item[num].apnType |=TAF_DCS_APN_TYPE_EMERGENCY;
            if( (apnValue & TAF_DCS_APN_TYPE_UT) == TAF_DCS_APN_TYPE_UT )
                contexts->item[num].apnType |=TAF_DCS_APN_TYPE_UT;
            if( (apnValue & TAF_DCS_APN_TYPE_MCX) == TAF_DCS_APN_TYPE_MCX )
                contexts->item[num].apnType |=TAF_DCS_APN_TYPE_MCX;

            contexts->item[num].pdp        = myProfile.MapIpFamily(profile->getIpFamilyType());
            contexts->item[num].auth       = myProfile.MapAuthProtocol(profile->getAuthProtocolType());
            le_utf8_Copy(contexts->item[num].authUsername, profile->getUserName().c_str(), TAF_DCS_NAME_MAX_LEN, NULL);
            le_utf8_Copy(contexts->item[num].authPassword, profile->getPassword().c_str(), TAF_DCS_NAME_MAX_LEN, NULL);
            contexts->item[num].slotId     = this->slotId;
            num++;
        }
    }

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("response error for slot id: %d! error code: %d", this->slotId, (int32_t)error);
        listEvent.ret = LE_FAULT;
    }
    else
    {
        LE_INFO(" Response ok for slot id %d",this->slotId);
        listEvent.ret = LE_OK;
    }

    listEvent.num = num;
    listEvent.slotId = this->slotId;
    listEvent.profilesListPtr = contexts;
    le_event_Report(myProfile.getListReqEvent(), (void *)&listEvent, sizeof(Profile_List_Event_t));
}

void taf_ProfileModifyCallback::commandResponse(telux::common::ErrorCode error)
{
    auto &myProfile = taf_DataProfile::GetInstance();
    le_result_t result;

    if (error == telux::common::ErrorCode::SUCCESS)
    {
        LE_DEBUG("profile modification is OK");
        result = LE_OK;
    }
    else
    {
        LE_ERROR("profile modification is failed, error code: %d", (int32_t)error);
        result = LE_FAULT;
    }

    myProfile.CmdSynchronousPromise.set_value(result);
}

taf_DataProfile &taf_DataProfile::GetInstance()
{
    static taf_DataProfile instance;
    return instance;
}

le_result_t taf_DataProfile::getPhoneIdFromSlotId(uint8_t slotId, uint8_t *phoneIdPtr)
{
    int retPhoneId;
    le_result_t result = LE_OK;

    TAF_ERROR_IF_RET_VAL(phoneIdPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(phoneIdPtr)");

    if(PhoneMgr)
    {
        retPhoneId = PhoneMgr->getPhoneIdFromSlotId(slotId);
        if(retPhoneId < 0)
        {
            LE_ERROR("Invalid phone id");
            result = LE_FAULT;
        }
        else
        {
            *phoneIdPtr = (uint8_t)retPhoneId;
            result = LE_OK;
        }
    }
    else
    {
        LE_ERROR("Phone manager is NULL");
        result = LE_FAULT;
    }

    LE_DEBUG("result =%d, slotId = %d, phoneId = %d", result, slotId, *phoneIdPtr);

    return result;
}

le_result_t taf_DataProfile::getSlotIdFromPhoneId(uint8_t phoneId, uint8_t *slotIdPtr)
{
    int retSlotId;
    le_result_t result = LE_OK;

    TAF_ERROR_IF_RET_VAL(slotIdPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(slotIdPtr)");

    if(PhoneMgr)
    {
        retSlotId = PhoneMgr->getSlotIdFromPhoneId(phoneId);
        if(retSlotId < 0)
        {
            LE_ERROR("Invalid slot id");
            result = LE_FAULT;
        }
        else
        {
            *slotIdPtr = (uint8_t)retSlotId;
            result = LE_OK;
        }
    }
    else
    {
        LE_ERROR("Phone manager is NULL");
        result = LE_FAULT;
    }

    LE_DEBUG("result =%d, slotId = %d, phoneId = %d",result, *slotIdPtr, phoneId);

    return result;
}

le_event_Id_t taf_DataProfile::GetThrottleStateEvent(uint8_t slotId, int32_t profileId)
{
    taf_dcs_ProfileCtx_t *profileCtxPtr = GetProfileCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, NULL,
                         "Cannot find profile context slotId(%d) profileId(%d)",
                         slotId, profileId);

    return profileCtxPtr->throttleStateEvent;
}

void taf_DataProfile::ProcessThrottledApnInfoChanged(const std::vector<telux::data::APNThrottleInfo>  &throttleInfoList,uint8_t slotId)
{
    ThrottleStatus_t stateEvent;
    le_dls_Link_t* linkPtr = NULL;

    if(throttleInfoList.size() == 0)
    {
        //List is empty browse through profile list cache's value for each profile and send event
        //if throttleState was true.
        LE_INFO("onThrottledApnInfoChanged 0 APNs throttled!");

        linkPtr = le_dls_Peek(&ProfileCtxList);
        while (linkPtr)
        {
          //const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
          //taf_dcs_ProfileCtx_t* profileCtx = GetProfileCtx(slotId, profileInfoPtr->index);
          taf_dcs_ProfileCtx_t* profileCtx = CONTAINER_OF(linkPtr, taf_dcs_ProfileCtx_t, link);
          linkPtr = le_dls_PeekNext(&ProfileCtxList, linkPtr);
          if(profileCtx == NULL)
          {
            LE_DEBUG("Profile context not found slot %d", slotId);
            continue;
          }
          if((slotId == profileCtx->slotId) && (profileCtx->throttleInfo.isThrottled == true))
          {
            //If throttle is true in profile context then send notification with false.
            stateEvent.throttleState = false;
            stateEvent.slotId = slotId;
            stateEvent.profileId = profileCtx->info.index;
            stateEvent.ipv4Time = 0;
            stateEvent.ipv6Time = 0;
            LE_INFO("%-6d""%-6d""%-12s", profileCtx->info.index, profileCtx->info.tech,
                                         profileCtx->info.name);

            le_event_Report(profileCtx->throttleStateEvent, &stateEvent, sizeof(stateEvent));
            //reset the values in profile's cache.
            memset(&(profileCtx->throttleInfo), 0, sizeof(throttleInfo_t));
          }
        }
        return;
    }

    bool profileFound = false;
    linkPtr = NULL;
    linkPtr = le_dls_Peek(&ProfileCtxList);
    while (linkPtr)
    {
      taf_dcs_ProfileCtx_t* profileCtxFromList = CONTAINER_OF(linkPtr, taf_dcs_ProfileCtx_t, link);
      linkPtr = le_dls_PeekNext(&ProfileCtxList, linkPtr);
      profileFound = false;
      // Traverse the throttleInfoList to find your slotId and profile ID from profile context
      for (auto throttleInfo : throttleInfoList)
      {
        for (uint8_t tProfId : throttleInfo.profileIds)
        {
          // Absence of profile id in throttle info considered as the profile is not throttled
          taf_dcs_ProfileCtx_t* profileCtx = GetProfileCtx(slotId, tProfId);
          if(profileCtx == NULL)
          {
            LE_DEBUG("Profile context not found slot %d Id %d", slotId,tProfId);
            continue;
          }
          //cache throttle info in profile context.
          if((profileCtxFromList->info.index == tProfId) && (slotId == profileCtx->slotId))
          {
            profileFound = true;
            profileCtx->throttleInfo.isThrottled = true;
            stateEvent.throttleState = true;
            stateEvent.slotId = slotId;
            stateEvent.profileId = tProfId;
            le_utf8_Copy(profileCtx->throttleInfo.mnc, throttleInfo.mnc.c_str(),
                                                     TAF_DCS_MNC_BYTES, NULL);
            le_utf8_Copy(profileCtx->throttleInfo.mcc, throttleInfo.mcc.c_str(),
                                                     TAF_DCS_MCC_BYTES, NULL);
            if (throttleInfo.isBlocked)
            {
              LE_DEBUG("APN blocked on all plmns slot %d Id %d", slotId,tProfId);
              profileCtx->throttleInfo.isBlocked = throttleInfo.isBlocked;
            }
            stateEvent.ipv4Time = throttleInfo.ipv4Time;
            stateEvent.ipv6Time = throttleInfo.ipv6Time;
            le_event_Report(profileCtx->throttleStateEvent, &stateEvent, sizeof(stateEvent));
          }
        }
      }
      if(profileFound == false) //check for throttleState. If yes send event with false.
      {
        if(profileCtxFromList->throttleInfo.isThrottled == true )
        {
          stateEvent.throttleState = false;
          stateEvent.slotId = slotId;
          stateEvent.profileId = profileCtxFromList->info.index;
          stateEvent.ipv4Time = 0;
          stateEvent.ipv6Time = 0;
          le_event_Report(profileCtxFromList->throttleStateEvent, &stateEvent, sizeof(stateEvent));
          memset(&(profileCtxFromList->throttleInfo), 0, sizeof(throttleInfo_t));
        }
      }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the PLMN on which the APN is throttled.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Failed.
 *  - LE_NOT_POSSIBLE -- Data profile is not created.
 *  - LE_UNAVAILABLE  -- Data profile is not throttled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DataProfile::GetAPNThrottledPLMN(taf_dcs_ProfileRef_t    profileRef,
                                                 bool       *areAllPLMNsThrottled,
                                                 char                 *mccPtr,
                                                 size_t                mccSize,
                                                 char                 *mncPtr,
                                                 size_t                mncSize)

{
    TAF_ERROR_IF_RET_VAL((profileRef == NULL) || (areAllPLMNsThrottled == NULL) ||
                         (mccPtr == NULL) || (mncPtr == NULL), LE_BAD_PARAMETER,
                          "some pointers may be null");

    taf_dcs_ProfileCtx_t* profileCtx = (taf_dcs_ProfileCtx_t* )le_ref_Lookup(ProfileRefMap,
                                                                           (void*)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtx == NULL, LE_NOT_FOUND,
                                      "cannot get profile context from reference(%p)", profileRef);

    TAF_ERROR_IF_RET_VAL(profileCtx->throttleInfo.isThrottled == false, LE_UNAVAILABLE,
                                      "profile is not throttled");

    if(profileCtx->throttleInfo.mcc != NULL)
      le_utf8_Copy(mccPtr,profileCtx->throttleInfo.mcc,TAF_DCS_MCC_BYTES, NULL);

    if(profileCtx->throttleInfo.mnc != NULL)
      le_utf8_Copy(mncPtr,profileCtx->throttleInfo.mnc,TAF_DCS_MNC_BYTES, NULL);

    *areAllPLMNsThrottled = profileCtx->throttleInfo.isBlocked;

    LE_DEBUG("GetAPNThrottledPLMN mccPtr %s mncPtr %s", mccPtr,mncPtr);
    LE_DEBUG("GetAPNThrottledPLMN areAllPLMNsThrottled %d", *areAllPLMNsThrottled);

    return LE_OK;
}


le_result_t taf_DataProfile::SendAPNThrottledInfo(uint8_t slotId, int32_t profileId, bool *isThrottledPtr)
{
    taf_dcs_ProfileCtx_t* profileCtxPtr = GetProfileCtx(slotId, profileId);
    le_result_t result = LE_OK;
    bool         isThrottled = false;
    uint32_t     ipv4RemainingTime = 0;
    uint32_t     ipv6RemainingTime = 0;
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                         "Cannot find profile context slotId(%d) profileId(%d)",
                         slotId, profileId);

    *isThrottledPtr = profileCtxPtr->throttleInfo.isThrottled;

    if(*isThrottledPtr == true)
    {
      //send APN throttle event
      ThrottleStatus_t stateEvent;

      result = taf_dcs_GetAPNThrottledStatus(profileCtxPtr->reference,
                                             &isThrottled,&ipv4RemainingTime,
                                             &ipv6RemainingTime);
      if(result == LE_OK)
      {
        stateEvent.throttleState = true;
        stateEvent.slotId = slotId;
        stateEvent.profileId = profileId;
        stateEvent.ipv4Time = ipv4RemainingTime;
        stateEvent.ipv6Time = ipv6RemainingTime;
        le_event_Report(profileCtxPtr->throttleStateEvent, &stateEvent, sizeof(stateEvent));
      }
    }
    return LE_OK;
}

le_result_t taf_DataProfile::IsAPNThrottled(uint8_t slotId, int32_t profileId, bool *isThrottledPtr)
{
    taf_dcs_ProfileCtx_t* profileCtxPtr = GetProfileCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                         "Cannot find profile context slotId(%d) profileId(%d)",
                         slotId, profileId);

    *isThrottledPtr = profileCtxPtr->throttleInfo.isThrottled;
    LE_DEBUG("Profile APN throttled %d", *isThrottledPtr);
    return LE_OK;
}


taf_dcs_Pdp_t taf_DataProfile::MapIpFamily(telux::data::IpFamilyType ipFamily)
{
    if (ipFamily == telux::data::IpFamilyType::IPV4)
    {
        return TAF_DCS_PDP_IPV4;
    }
    else if (ipFamily == telux::data::IpFamilyType::IPV6)
    {
        return TAF_DCS_PDP_IPV6;
    }
    else if (ipFamily == telux::data::IpFamilyType::IPV4V6)
    {
        return TAF_DCS_PDP_IPV4V6;
    }

    return TAF_DCS_PDP_UNKNOWN;
}

telux::data::IpFamilyType taf_DataProfile::MapIpFamily(taf_dcs_Pdp_t ipFamily)
{
    if (ipFamily == TAF_DCS_PDP_IPV4)
    {
        return telux::data::IpFamilyType::IPV4;
    }
    else if (ipFamily == TAF_DCS_PDP_IPV6)
    {
        return telux::data::IpFamilyType::IPV6;
    }
    else if (ipFamily == TAF_DCS_PDP_IPV4V6)
    {
        return telux::data::IpFamilyType::IPV4V6;
    }

    return telux::data::IpFamilyType::UNKNOWN;
}

taf_dcs_Auth_t taf_DataProfile::MapAuthProtocol(telux::data::AuthProtocolType authType)
{
    if (authType == telux::data::AuthProtocolType::AUTH_PAP)
    {
        return TAF_DCS_AUTH_PAP;
    }
    else if (authType == telux::data::AuthProtocolType::AUTH_CHAP)
    {
        return TAF_DCS_AUTH_CHAP;
    }
    else if (authType == telux::data::AuthProtocolType::AUTH_PAP_CHAP)
    {
        return TAF_DCS_AUTH_PAP | TAF_DCS_AUTH_CHAP;
    }

    return TAF_DCS_AUTH_NONE;
}

telux::data::AuthProtocolType taf_DataProfile::MapAuthProtocol(taf_dcs_Auth_t authType)
{
    if (authType == TAF_DCS_AUTH_PAP)
    {
        return telux::data::AuthProtocolType::AUTH_PAP;
    }
    else if (authType == TAF_DCS_AUTH_CHAP)
    {
        return telux::data::AuthProtocolType::AUTH_CHAP;
    }
    else if (authType == (TAF_DCS_AUTH_PAP | TAF_DCS_AUTH_CHAP))
    {
        return telux::data::AuthProtocolType::AUTH_PAP_CHAP;
    }

    return telux::data::AuthProtocolType::AUTH_NONE;
}

taf_dcs_Tech_t taf_DataProfile::MapTechPreference(telux::data::TechPreference techPref)
{
    if (techPref == telux::data::TechPreference::TP_3GPP)
    {
        return TAF_DCS_TECH_3GPP;
    }
    else if (techPref == telux::data::TechPreference::TP_3GPP2)
    {
        return TAF_DCS_TECH_3GPP2;
    }
    else if (techPref == telux::data::TechPreference::TP_ANY)
    {
        return TAF_DCS_TECH_ANY;
    }

    return TAF_DCS_TECH_UNKNOWN;
}

telux::data::TechPreference taf_DataProfile::MapTechPreference(taf_dcs_Tech_t techPref)
{
    if (techPref == TAF_DCS_TECH_3GPP)
    {
        return telux::data::TechPreference::TP_3GPP;
    }
    else if (techPref == TAF_DCS_TECH_3GPP2)
    {
        return telux::data::TechPreference::TP_3GPP2;
    }
    else if (techPref == TAF_DCS_TECH_ANY)
    {
        return telux::data::TechPreference::TP_ANY;
    }

    return telux::data::TechPreference::UNKNOWN;
}

le_result_t taf_DataProfile::ListProfile
(
    uint8_t slotId,
    taf_dcs_ProfileInfo_t *profileList,
    size_t *listSize
)
{
    le_result_t result;
    le_dls_Link_t* linkPtr = NULL;
    int profileCnt = 0;

    result = SendProfileListReq(slotId);

    if (result != LE_OK)
    {
        LE_ERROR("getting profile list is failed, result: %d", result);
        return result;
    }

    linkPtr = le_dls_Peek(&ProfileCtxList);
    while (linkPtr)
    {
        taf_dcs_ProfileCtx_t* profileCtx = CONTAINER_OF(linkPtr, taf_dcs_ProfileCtx_t, link);
        linkPtr = le_dls_PeekNext(&ProfileCtxList, linkPtr);

        if(slotId == profileCtx->slotId)
        {
            memcpy((char *)&profileList[profileCnt], (const char *)&profileCtx->info, sizeof(taf_dcs_ProfileInfo_t));
            profileCnt++;
        }
    }

    *listSize = profileCnt;

    return LE_OK;
}

le_result_t taf_DataProfile::SendProfileListReq(uint8_t slotId)
{
    telux::common::Status status;
    CmdSynchronousPromise = std::promise<le_result_t>();

    if(dataProfileManagers.find((SlotId)slotId) == dataProfileManagers.end())
    {
        LE_ERROR("Profile manager is not init for slot id %d", slotId);
        return LE_FAULT;
    }

    status = dataProfileManagers[static_cast<SlotId>(slotId)]->requestProfileList(
        ListProfileCb[static_cast<SlotId>(slotId)]);

    if (status != telux::common::Status::SUCCESS)
    {
        return LE_FAULT;
    }

    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    le_result_t result = futResult.get();

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * The createProfile completion callback
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_CreateProfileCallback::onResponse(int profileId, telux::common::ErrorCode error)
{
    LE_DEBUG("Status: %d, profileId %d", static_cast<int>(error), profileId);
    auto &myProfile = taf_DataProfile::GetInstance();
    std::tuple<telux::common::ErrorCode, int> params = std::make_tuple(error, profileId);
    myProfile.CreateProfileSyncPromise.set_value(params);
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Creates a new profile on the device. Client should have set at least the IP family type for the
 * profile using ::taf_dcs_SetPDP before calling this API.<br>
 * On success creation of profile, the profile reference will be updated with the created profile
 * ID. Clients can use ::taf_dcs_GetProfileId to get the created profile ID.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DataProfile::CreateProfile(taf_dcs_ProfileRef_t profileRef)
{
    TAF_ERROR_IF_RET_VAL((profileRef == NULL), LE_BAD_PARAMETER, "profileRef is null");
    taf_dcs_ProfileCtx_t *profileCtxPtr = (taf_dcs_ProfileCtx_t *)le_ref_Lookup(ProfileRefMap,
                                                                                (void *)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                         "cannot get profile context from reference(%p)", profileRef);

    telux::data::ProfileParams params;
    telux::common::Status status;
    le_result_t result;

    if (TAF_DCS_UNDEFINED_PROFILE_ID != profileCtxPtr->info.index)
    {
        LE_WARN("Profile with id %d already created", profileCtxPtr->info.index);
        return LE_DUPLICATE;
    }

    result = MapProfileCtxToParams(profileCtxPtr, params);
    if (LE_OK != result)
    {
        LE_ERROR("Unable to map profile ctx to params");
        return result;
    }
    LE_INFO("SIM slot ID: %d", profileCtxPtr->slotId);

    if (dataProfileManagers.find(static_cast<SlotId>(profileCtxPtr->slotId)) ==
                                                                  dataProfileManagers.end())
    {
        LE_ERROR("Profile manager not initialized for slot id %d", profileCtxPtr->slotId);
        return LE_FAULT;
    }

    // initialize the create profile promise
    CreateProfileSyncPromise = std::promise<std::tuple<telux::common::ErrorCode, int>>();

    // Create a future to wait for callback
    std::future <std::tuple<telux::common::ErrorCode, int>> CreateProfilefuture =
                                                        CreateProfileSyncPromise.get_future();

    // Create the profile.
    status = dataProfileManagers[static_cast<SlotId>(profileCtxPtr->slotId)]->createProfile(params,
                                                                                CreateProfileCb);

    if (telux::common::Status::SUCCESS != status )
    {
        LE_ERROR("createProfile failed: %d", static_cast<int>(status));
        return LE_FAULT;
    }

    // Wait for profile creation confirmation
    auto futResult = CreateProfilefuture.get();
    telux::common::ErrorCode errCode = static_cast<telux::common::ErrorCode>(std::get<0>(futResult));
    int profileId = static_cast<int>(std::get<1>(futResult));
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_ERROR("Profile creation failed with error : %d", static_cast<int>(errCode));
        return LE_FAULT;
    }
    LE_INFO("Created profile id: %d", profileId);

    //Update the context with the created profile id.
    profileCtxPtr->info.index = profileId;

    // Update the profile
    ProfileNum[(SlotId)(profileCtxPtr->slotId)] = profileId;
    show(profileCtxPtr->slotId);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes the profile that is referenced. Once a profile is deleted, the profile reference is not
 * valid anymore.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Profile reference was not found.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DataProfile::DeleteProfile(taf_dcs_ProfileRef_t profileRef)
{
    TAF_ERROR_IF_RET_VAL((profileRef == NULL), LE_BAD_PARAMETER, "profileRef is null");
    taf_dcs_ProfileCtx_t *profileCtxPtr = (taf_dcs_ProfileCtx_t *)le_ref_Lookup(ProfileRefMap,
                                                                                (void *)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                         "cannot get profile context from reference(%p)", profileRef);

    le_result_t result;
    telux::common::Status status;
    uint8_t slotId = 0;
    int32_t profileId = 0;
    taf_dcs_ConState_t dataState;
    auto &myProfile = taf_DataProfile::GetInstance();
    auto &dataConnection = taf_DataConnection::GetInstance();

    result = GetSlotIdAndProfileId(profileRef, &slotId, &profileId);
    if (LE_OK != result)
    {
        LE_ERROR("Unable to get slot ID and profile ID");
        return LE_FAULT;
    }
    LE_INFO("To delete: slot ID %d, profile ID: %d", slotId, profileId);

    // Check if the profile has been created or not.
    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileId)
    {
        LE_WARN("Profile still to be created.");
        return LE_NOT_POSSIBLE;
    }

    // Check if a data connection is active with that profile
    result = dataConnection.GetConnectionState(slotId, profileId, &dataState);
    if (LE_OK != result)
    {
        LE_ERROR("Unable to get connection state of the profile");
        return LE_FAULT;
    }
    if (TAF_DCS_DISCONNECTED != dataState)
    {
        // Data connection state with this profile is not in DISCONNETED state
        LE_ERROR("Profile is not in TAF_DCS_DISCONNECTED state");
        return LE_BUSY;
    }

    // initialize the synchronous promise
    CmdSynchronousPromise = std::promise<le_result_t>();

    // Call the API to delete profile.
    status = dataProfileManagers[static_cast<SlotId>(slotId)]->deleteProfile(profileId,
                            myProfile.MapTechPreference(profileCtxPtr->info.tech), ModifyProfileCb);

    if (telux::common::Status::SUCCESS != status)
    {
        LE_ERROR("Unable to delete profile: %d. deleteProfile API error: %d", profileId,
                                                            static_cast<uint32_t>(status));
        return LE_FAULT;
    }

    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();
    if (LE_OK != result)
    {
        LE_ERROR("Unable to delete profile: %d. deleteProfile cb error.", profileId);
        return LE_FAULT;
    }

    // Remove references to this profile from DCS
    le_ref_DeleteRef(ProfileRefMap, profileCtxPtr->reference);
    le_dls_Remove(&ProfileCtxList, &profileCtxPtr->link);
    le_mem_Release(profileCtxPtr);
    return LE_OK;
}

taf_dcs_ProfileRef_t taf_DataProfile::GetProfileRef(uint8_t slotId, int32_t index)
{
    if (TAF_DCS_UNDEFINED_PROFILE_ID != index)
    {
        LE_INFO("Profile ID is %d. Checking available profiles", index);
        taf_dcs_ProfileCtx_t *profileCtxPtr = GetProfileCtx(slotId, index);
        TAF_ERROR_IF_RET_VAL(profileCtxPtr == nullptr, nullptr,
                             "cannot get profile context from index[%d]", index);
        TAF_ERROR_IF_RET_VAL(profileCtxPtr->reference == nullptr, nullptr,
                             "reference is invalid from index[%d]", index);
        return profileCtxPtr->reference;
    }

    LE_INFO("Profile ID is TAF_DCS_UNDEFINED_PROFILE_ID.");
    // if TAF_DCS_UNDEFINED_PROFILE_ID is the provided profile index, check if another profile is
    // already in line to be created
    taf_dcs_ProfileCtx_t *profileCtxPtr = GetProfileCtx(slotId, index);
    if (nullptr != profileCtxPtr)
    {
        LE_WARN("Profile context with profile ID TAF_DCS_UNDEFINED_PROFILE_ID already present.");
        return profileCtxPtr->reference;
    }

    LE_INFO("Creating a profile reference.");
    le_result_t result;
    taf_dcs_ProfileCtx_t profileCtx = {0};
    profileCtx.slotId = slotId;
    profileCtx.info.index = index;
    result = CreateIndividualProfile(&profileCtx);
    if (LE_OK != result)
    {
        // Client should delete a profile, free up memory and then create profile again.
        LE_ERROR("Unable to create profile reference");
        return nullptr;
    }

    // Get the created reference
    profileCtxPtr = nullptr;
    profileCtxPtr = GetProfileCtx(slotId, index);
    if (nullptr == profileCtxPtr)
    {
        LE_ERROR("Created profile reference is NULL");
        return nullptr;
    }
    LE_INFO("Profile reference created and added to DCS ProfileCtxList");

    // Set techinology and PDP to reduce IPC calls for clients. Clients can update these values
    // using relevant APIs later.
    profileCtxPtr->info.tech = TAF_DCS_TECH_3GPP;
    profileCtxPtr->pdp = TAF_DCS_PDP_IPV4;
    return profileCtxPtr->reference;
}

le_result_t taf_DataProfile::GetSlotIdAndProfileId
(
    taf_dcs_ProfileRef_t profileRef,
    uint8_t *slotId,
    int32_t *profileId
)
{
    TAF_ERROR_IF_RET_VAL(profileRef == NULL || profileId == NULL || slotId == NULL,
                         LE_BAD_PARAMETER, "Null pointer");
    taf_dcs_ProfileCtx_t* profileCtxPtr = (taf_dcs_ProfileCtx_t* )le_ref_Lookup(ProfileRefMap,
                                                                                (void*)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                         "cannot get profile context from reference(%p)", profileRef);

    *slotId = profileCtxPtr->slotId;
    *profileId = profileCtxPtr->info.index;

    return LE_OK;
}

le_result_t taf_DataProfile::GetAuthentication(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_Auth_t *typePtr,
    char *userNamePtr,
    size_t userNameSize,
    char *passwordPtr,
    size_t passwordSize)
{
    TAF_ERROR_IF_RET_VAL(profileRef == NULL, LE_NOT_FOUND, "reference is invalid");
    TAF_ERROR_IF_RET_VAL(typePtr == NULL || userNamePtr == NULL || passwordPtr == NULL,
        LE_OUT_OF_RANGE, "some pointers are null");
    TAF_ERROR_IF_RET_VAL(userNameSize < TAF_DCS_USER_NAME_MAX_LEN || passwordSize < TAF_DCS_PASSWORD_NAME_MAX_LEN,
        LE_OUT_OF_RANGE, "the size of user name or password is invalid");

    taf_dcs_ProfileCtx_t* profileCtxPtr = (taf_dcs_ProfileCtx_t* )le_ref_Lookup(ProfileRefMap, (void*)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND, "cannot get profile context from reference(%p)", profileRef);

    *typePtr = profileCtxPtr->auth;
    LE_INFO("*typePtr is :%d", *typePtr);
    if (*typePtr != TAF_DCS_AUTH_NONE)
    {
        le_utf8_Copy(userNamePtr, profileCtxPtr->authUsername, userNameSize, NULL);
        le_utf8_Copy(passwordPtr, profileCtxPtr->authPassword, passwordSize, NULL);
    }
    return LE_OK;
}


le_result_t taf_DataProfile::GetApn(taf_dcs_ProfileRef_t profileRef, char *apnPtr, size_t apnSize)
{
    TAF_ERROR_IF_RET_VAL(apnSize < TAF_DCS_APN_NAME_MAX_LEN, LE_OVERFLOW, "apnSize(%zu) is smaller than NAME_MAX_BYTES(%d)", apnSize, TAF_DCS_APN_NAME_MAX_LEN);
    TAF_ERROR_IF_RET_VAL((profileRef == NULL) || (apnPtr == NULL), LE_NOT_FOUND, "some pointers may be null");
    taf_dcs_ProfileCtx_t* profileCtxPtr = (taf_dcs_ProfileCtx_t* )le_ref_Lookup(ProfileRefMap, (void*)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND, "cannot get profile context from reference(%p)", profileRef);
    LE_INFO("apn: %s...slot id: %d, profile id: %d",
             apnPtr, profileCtxPtr->slotId, profileCtxPtr->info.index);
    le_utf8_Copy(apnPtr, profileCtxPtr->apn, apnSize, NULL);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the data profile name.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Profile reference was not found.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DataProfile::SetProfileName(taf_dcs_ProfileRef_t profileRef, const char * namePtr)
{
    telux::data::ProfileParams params;
    le_result_t result;

    TAF_ERROR_IF_RET_VAL((profileRef == NULL), LE_BAD_PARAMETER, "profileRef is null");
    TAF_ERROR_IF_RET_VAL((namePtr == NULL), LE_BAD_PARAMETER, "namePtr is null");

    taf_dcs_ProfileCtx_t* profileCtxPtr = (taf_dcs_ProfileCtx_t* )le_ref_Lookup(ProfileRefMap,
                                                                                 (void*)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                                       "cannot get profile context from reference(%p)", profileRef);

    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileCtxPtr->info.index)
    {
        // The profile is still to be created. Just update context with provided profile name.
        LE_INFO("Profile still to be created. Update context alone with name: %s", namePtr);
        le_utf8_Copy(profileCtxPtr->info.name, namePtr, TAF_DCS_NAME_MAX_LEN, NULL);
        return LE_OK;
    }

    MapProfileCtxToParams(profileCtxPtr, params);
    params.profileName = namePtr;
    LE_DEBUG ("Profile name to set: %s", params.profileName.c_str());
    result = SendProfileModificationReq(profileCtxPtr->slotId, profileCtxPtr->info.index, params);
    if (result != LE_OK)
    {
        LE_ERROR("updating profile infomation failed, result: %d", result);
        return result;
    }

    // update new value
    le_utf8_Copy(profileCtxPtr->info.name, params.profileName.c_str(), TAF_DCS_NAME_MAX_LEN, NULL);
    LE_DEBUG ("Profile name set successfully");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data profile name.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Failed.
 *  - LE_OVERFLOW -- Apn size is smaller than APN_NAME_MAX_BYTES.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DataProfile::GetProfileName(taf_dcs_ProfileRef_t profileRef, char *namePtr,
                                                                                size_t nameSize)
{
    TAF_ERROR_IF_RET_VAL((profileRef == NULL), LE_BAD_PARAMETER, "profileRef is null");
    TAF_ERROR_IF_RET_VAL((namePtr == NULL), LE_BAD_PARAMETER, "namePtr is null");
    TAF_ERROR_IF_RET_VAL(nameSize < TAF_DCS_NAME_MAX_LEN, LE_OVERFLOW,
                                            "nameSize(%zu) is smaller than NAME_MAX_BYTES(%d)",
                                            nameSize, TAF_DCS_NAME_MAX_LEN);

    taf_dcs_ProfileCtx_t* profileCtxPtr = (taf_dcs_ProfileCtx_t* )le_ref_Lookup(ProfileRefMap,
                                                                                 (void*)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                                       "cannot get profile context from reference(%p)", profileRef);

    le_utf8_Copy(namePtr, profileCtxPtr->info.name, nameSize, NULL);
    LE_INFO("name: %s...slot id: %d, profile id: %d",
                                  namePtr, profileCtxPtr->slotId, profileCtxPtr->info.index);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data profile APN types.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DataProfile::GetApnTypes
(
    taf_dcs_ProfileRef_t profileRef,
     taf_dcs_ApnType_t *apnTypePtr
)
{
    TAF_ERROR_IF_RET_VAL((profileRef == NULL) || (apnTypePtr == NULL), LE_NOT_FOUND,
                         "some pointers may be null");
    taf_dcs_ProfileCtx_t* profileCtxPtr = (taf_dcs_ProfileCtx_t* )le_ref_Lookup(ProfileRefMap,
                                                                                (void*)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                         "can't get profile context from reference(%p)", profileRef);
    *apnTypePtr = profileCtxPtr->apnType;
    LE_INFO("apntype: %d...slot id: %d, profile id: %d",
            (int)*apnTypePtr, profileCtxPtr->slotId, profileCtxPtr->info.index);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the data profile APN type.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DataProfile::SetApnTypes
(
    taf_dcs_ProfileRef_t profileRef,
     taf_dcs_ApnType_t apnType
)
{
    telux::data::ProfileParams params;
    le_result_t result;
    int32_t profileId;
    uint8_t slotId;

    TAF_ERROR_IF_RET_VAL((profileRef == NULL), LE_BAD_PARAMETER, "profileRef is null");
    TAF_ERROR_IF_RET_VAL(0 == apnType, LE_BAD_PARAMETER, "0 is not valid apnType");
    taf_dcs_ProfileCtx_t* profileCtxPtr = (taf_dcs_ProfileCtx_t* )le_ref_Lookup(ProfileRefMap,
                                                                                (void*)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                         "can't get profile context from reference(%p)", profileRef);
    TAF_ERROR_IF_RET_VAL(GetSlotIdAndProfileId(profileRef, &slotId, &profileId) != LE_OK,
                         LE_NOT_FOUND, "cannot get profile id from reference(%p)", profileRef);

    LE_DEBUG("APN Type to update: %d", apnType);
    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileCtxPtr->info.index)
    {
        // The profile is still to be created. Just update context with provided APN type.
        LE_INFO("Profile still to be created. Update context alone with APN type: %d", apnType);
        profileCtxPtr->apnType = apnType;
        return LE_OK;
    }

    MapProfileCtxToParams(profileCtxPtr, params);
    params.apnTypes = apnType;

    result = SendProfileModificationReq(slotId, profileId, params);
    if (result != LE_OK)
    {
        LE_ERROR("updating profile infomation failed, result: %d", result);
        return result;
    }

    // update new value
    profileCtxPtr->apnType = apnType;
    LE_DEBUG("APN Type updated");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the data profile technology preference.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Profile reference was not found.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DataProfile::SetTechPreference
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_Tech_t techPref
        ///< [IN] The technology preference.
)
{
    telux::data::ProfileParams params;
    le_result_t result;
    int32_t profileId;
    uint8_t slotId;

    TAF_ERROR_IF_RET_VAL((profileRef == NULL), LE_BAD_PARAMETER, "profileRef is null");
    TAF_ERROR_IF_RET_VAL(TAF_DCS_TECH_UNKNOWN == techPref, LE_BAD_PARAMETER,
                                                            "TAF_DCS_TECH_UNKNOWN not allowed");
    taf_dcs_ProfileCtx_t *profileCtxPtr = (taf_dcs_ProfileCtx_t *)le_ref_Lookup(ProfileRefMap,
                                                                                (void *)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                         "can't get profile context from reference(%p)", profileRef);
    TAF_ERROR_IF_RET_VAL(GetSlotIdAndProfileId(profileRef, &slotId, &profileId) != LE_OK,
                         LE_NOT_FOUND, "cannot get profile id from reference(%p)", profileRef);

    LE_DEBUG("Tech pref to update: %d", techPref);
    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileCtxPtr->info.index)
    {
        // The profile is still to be created. Just update context with provided tech pref.
        LE_INFO("Profile still to be created. Update context alone with tech pref: %d", techPref);
        profileCtxPtr->info.tech = techPref;
        return LE_OK;
    }

    MapProfileCtxToParams(profileCtxPtr, params);
    params.techPref = MapTechPreference(techPref);

    result = SendProfileModificationReq(slotId, profileId, params);
    if (result != LE_OK)
    {
        LE_ERROR("updating profile infomation failed, result: %d", result);
        return result;
    }

    // update new value
    profileCtxPtr->info.tech = techPref;
    LE_DEBUG("Tech pref updated");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data profile technology preference.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Profile reference was not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DataProfile::GetTechPreference
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_Tech_t* techPrefPtr
        ///< [OUT] The technology preference.
)
{
    TAF_ERROR_IF_RET_VAL((profileRef == NULL), LE_BAD_PARAMETER, "profileRef is null");
    TAF_ERROR_IF_RET_VAL((techPrefPtr == NULL), LE_BAD_PARAMETER, "techPrefPtr is null");

    taf_dcs_ProfileCtx_t* profileCtxPtr = (taf_dcs_ProfileCtx_t* )le_ref_Lookup(ProfileRefMap,
                                                                                (void*)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                         "can't get profile context from reference(%p)", profileRef);
    *techPrefPtr = profileCtxPtr->info.tech;
    LE_INFO("Tech pref: %d...slot id: %d, profile id: %d",
            profileCtxPtr->info.tech, profileCtxPtr->slotId, profileCtxPtr->info.index);

    return LE_OK;
}

le_result_t taf_DataProfile::MapProfileCtxToParams(taf_dcs_ProfileCtx_t *ctxPtr, telux::data::ProfileParams &params)
{
    params.profileName = ctxPtr->info.name;
    params.techPref = MapTechPreference(ctxPtr->info.tech);
    params.authType = MapAuthProtocol(ctxPtr->auth);
    params.ipFamilyType = MapIpFamily(ctxPtr->pdp);
    params.apn = ctxPtr->apn;
    params.userName = ctxPtr->authUsername;
    params.password = ctxPtr->authPassword;

    return LE_OK;
}

le_result_t taf_DataProfile::SendProfileModificationReq
(
    uint8_t slotId,
    int32_t profileId,
    telux::data::ProfileParams &params
)
{
    // need reset promise.
    CmdSynchronousPromise = std::promise<le_result_t>();

    if(dataProfileManagers.find((SlotId)slotId) == dataProfileManagers.end())
    {
        LE_ERROR("Profile manager is not init for slot id %d", slotId);
        return LE_FAULT;
    }

    telux::common::Status status =
                   dataProfileManagers[static_cast<SlotId>(slotId)]->modifyProfile(profileId,
                                                                          params, ModifyProfileCb);

    if (status != telux::common::Status::SUCCESS)
    {
        return LE_FAULT;
    }

    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    le_result_t result = futResult.get();

    return result;
}

le_result_t taf_DataProfile::SetApn(taf_dcs_ProfileRef_t profileRef, const char *apnPtr)
{
    int32_t profileId;
    uint8_t slotId;
    taf_dcs_ProfileCtx_t * profileCtxPtr;
    telux::data::ProfileParams params;
    le_result_t result;

    TAF_ERROR_IF_RET_VAL((profileRef == NULL) || (apnPtr == NULL), LE_NOT_FOUND,
                          "some pointers may be null");
    TAF_ERROR_IF_RET_VAL(GetSlotIdAndProfileId(profileRef, &slotId, &profileId) != LE_OK,
                         LE_NOT_FOUND, "cannot get profile id from reference(%p)", profileRef);

    profileCtxPtr = GetProfileCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                        "cannot get profile context from reference(%p)", profileRef);

    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileCtxPtr->info.index)
    {
        // The profile is still to be created. Just update context with provided APN.
        LE_INFO("Profile still to be created. Update context alone with APN: %s", apnPtr);
        le_utf8_Copy(profileCtxPtr->apn, apnPtr, TAF_DCS_NAME_MAX_LEN, NULL);
        return LE_OK;
    }

    MapProfileCtxToParams(profileCtxPtr, params);
    params.apn = apnPtr;

    result = SendProfileModificationReq(slotId, profileId, params);
    if (result != LE_OK)
    {
        LE_ERROR("updating profile infomation is failed, result: %d", result);
        return result;
    }

    // update new value
    le_utf8_Copy(profileCtxPtr->apn, params.apn.c_str(), TAF_DCS_NAME_MAX_LEN, NULL);
    return LE_OK;
}

le_result_t taf_DataProfile::SetPdp(taf_dcs_ProfileRef_t profileRef, taf_dcs_Pdp_t pdp)
{
    int32_t profileId;
    uint8_t slotId;
    taf_dcs_ProfileCtx_t * profileCtxPtr;
    telux::data::ProfileParams params;
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(TAF_DCS_PDP_UNKNOWN == pdp, LE_BAD_PARAMETER,
                                                        "TAF_DCS_PDP_UNKNOWN not allowed");
    TAF_ERROR_IF_RET_VAL(profileRef == NULL, LE_NOT_FOUND, "some pointers may be null");
    TAF_ERROR_IF_RET_VAL(GetSlotIdAndProfileId(profileRef, &slotId, &profileId) != LE_OK,
                         LE_NOT_FOUND, "cannot get profile id from reference(%p)", profileRef);

    profileCtxPtr = GetProfileCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                         "cannot get profile context from reference(%p)", profileRef);

    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileCtxPtr->info.index)
    {
        // The profile is still to be created. Just update context with provided APN.
        LE_INFO("Profile still to be created. Update context alone with PDP: %d", pdp);
        profileCtxPtr->pdp = pdp;
        return LE_OK;
    }

    MapProfileCtxToParams(profileCtxPtr, params);
    params.ipFamilyType = MapIpFamily(pdp);

    result = SendProfileModificationReq(slotId, profileId, params);
    if (result != LE_OK)
    {
        LE_ERROR("updating profile infomation is failed, result: %d", result);
        return result;
    }

    // update new value
    profileCtxPtr->pdp = MapIpFamily(params.ipFamilyType);
    return LE_OK;
}

le_result_t taf_DataProfile::SetAuth(taf_dcs_ProfileRef_t profileRef, taf_dcs_Auth_t type, const char *userName, const char *password)
{
    int32_t profileId;
    uint8_t slotId;
    taf_dcs_ProfileCtx_t * profileCtxPtr;
    telux::data::ProfileParams params;
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(profileRef == NULL, LE_NOT_FOUND, "some pointers may be null");
    TAF_ERROR_IF_RET_VAL(GetSlotIdAndProfileId(profileRef, &slotId, &profileId) != LE_OK,
                        LE_NOT_FOUND, "cannot get profile id from reference(%p)", profileRef);

    profileCtxPtr = GetProfileCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                         "cannot get profile context from reference(%p)", profileRef);

    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileCtxPtr->info.index)
    {
        // The profile is still to be created. Just update context with provided auth params.
        LE_INFO("Profile still to be created. Update context alone with authtype:%d, UN:%s, PW:%s",
                                                                        type,userName, password);
        profileCtxPtr->auth = type;
        le_utf8_Copy(profileCtxPtr->authUsername, userName, TAF_DCS_USER_NAME_MAX_LEN, NULL);
        le_utf8_Copy(profileCtxPtr->authPassword, password, TAF_DCS_PASSWORD_NAME_MAX_LEN, NULL);
        return LE_OK;
    }

    MapProfileCtxToParams(profileCtxPtr, params);
    params.authType = MapAuthProtocol(type);
    params.userName = userName;
    params.password = password;

    result = SendProfileModificationReq(slotId, profileId, params);
    if (result != LE_OK)
    {
        LE_ERROR("updating profile infomation is failed, result: %d", result);
        return result;
    }

    // update new value
    profileCtxPtr->auth = MapAuthProtocol(params.authType);
    le_utf8_Copy(profileCtxPtr->authUsername, params.userName.c_str(), TAF_DCS_USER_NAME_MAX_LEN, NULL);
    le_utf8_Copy(profileCtxPtr->authPassword, params.password.c_str(), TAF_DCS_PASSWORD_NAME_MAX_LEN, NULL);
    return LE_OK;
}


taf_dcs_Pdp_t taf_DataProfile::GetPdp(taf_dcs_ProfileRef_t profileRef)
{
    TAF_ERROR_IF_RET_VAL(profileRef == NULL, TAF_DCS_PDP_UNKNOWN, "reference is invalid");
    taf_dcs_ProfileCtx_t* profileCtxPtr = (taf_dcs_ProfileCtx_t* )le_ref_Lookup(ProfileRefMap, (void*)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, TAF_DCS_PDP_UNKNOWN, "cannot get profile context from reference(%p)", profileRef);

    return profileCtxPtr->pdp;
}

void taf_DataProfile::CleanupAllProfiles(Profile_List_Event_t *listEvent)
{
    le_dls_Link_t* linkPtr = NULL;
    uint32_t i;

    linkPtr = le_dls_Peek(&ProfileCtxList);
    while (linkPtr)
    {
        taf_dcs_ProfileCtx_t* profileCtx = CONTAINER_OF(linkPtr, taf_dcs_ProfileCtx_t, link);
        linkPtr = le_dls_PeekNext(&ProfileCtxList, linkPtr);

        // Only clean the profiles with the same slot id.
        if(profileCtx->slotId != listEvent->slotId)
            continue;

        for (i = 0; i < listEvent->num; i++)
        {
            if (profileCtx->info.index == listEvent->profilesListPtr->item[i].info.index)
            {
                break;
            }
        }
        if (i >= listEvent->num)
        {
            le_ref_DeleteRef(ProfileRefMap, profileCtx->reference);
            le_dls_Remove(&ProfileCtxList, &profileCtx->link);
            le_mem_Release(profileCtx);
        }
    }

    return;
}

le_result_t taf_DataProfile::CreateIndividualProfile(taf_dcs_ProfileCtx_t *info)
{
    taf_dcs_ProfileCtx_t* profileCtx = NULL;
    char throttlename[18] = {0};

    profileCtx = (taf_dcs_ProfileCtx_t *)le_mem_ForceAlloc(ProfilePool);
    TAF_ERROR_IF_RET_VAL(profileCtx == NULL, LE_NO_MEMORY, "cannot alloc profileCtx");
    // Copy the profile info into the Ctx
    memcpy((char *)profileCtx, (char *)info, sizeof(taf_dcs_ProfileCtx_t));

    // create reference for this profile context
    taf_dcs_ProfileRef_t profileRef = (taf_dcs_ProfileRef_t)le_ref_CreateRef(ProfileRefMap, (void *)profileCtx);
    TAF_ERROR_IF_RET_VAL(profileRef == NULL, LE_NO_MEMORY, "cannot alloc profileRef");

    profileCtx->reference = profileRef;

    snprintf(throttlename, sizeof(throttlename)-1, "Throttle-%d-%d", profileCtx->slotId,
                                                                     profileCtx->info.index);
    profileCtx->throttleStateEvent = le_event_CreateId(throttlename, sizeof(ThrottleStatus_t));

    // add this profile context to list
    le_dls_Queue(&ProfileCtxList, &profileCtx->link);

    return LE_OK;
}

taf_dcs_ProfileCtx_t * taf_DataProfile::GetProfileCtx(uint8_t slotId, uint32_t index)
{
    le_dls_Link_t* linkPtr = NULL;
    LE_INFO("Slot ID: %d, Profile ID: %d", slotId, index);

    linkPtr = le_dls_Peek(&ProfileCtxList);
    while (linkPtr)
    {
        taf_dcs_ProfileCtx_t* profileCtx = CONTAINER_OF(linkPtr, taf_dcs_ProfileCtx_t, link);
        linkPtr = le_dls_PeekNext(&ProfileCtxList, linkPtr);
        LE_DEBUG("Ctx: Slot ID: %d, Profile ID: %d", profileCtx->slotId, profileCtx->info.index);
        if (profileCtx->info.index == index && profileCtx->slotId == slotId)
        {
            LE_DEBUG("Get profileCtx %p", profileCtx);
            return profileCtx;
        }
    }

    return NULL;
}

void taf_DataProfile::UpdateIndividualProfile(taf_dcs_ProfileCtx_t *distPtr, taf_dcs_ProfileCtx_t *srcPtr)
{
    TAF_ERROR_IF_RET_NIL(distPtr == NULL, "distPtr is NULL!");
    TAF_ERROR_IF_RET_NIL(srcPtr == NULL, "srcPtr is NULL!");
    TAF_ERROR_IF_RET_NIL(distPtr->info.index != srcPtr->info.index, "profile index is different!")

    distPtr->info.tech = srcPtr->info.tech;
    le_utf8_Copy(distPtr->info.name, srcPtr->info.name, sizeof(distPtr->info.name), NULL);
    le_utf8_Copy(distPtr->apn, srcPtr->apn, sizeof(distPtr->apn), NULL);
    distPtr->pdp = srcPtr->pdp;
    distPtr->auth = srcPtr->auth;
    le_utf8_Copy(distPtr->authUsername, srcPtr->authUsername, sizeof(distPtr->authUsername), NULL);
    le_utf8_Copy(distPtr->authPassword, srcPtr->authPassword, sizeof(distPtr->authPassword), NULL);

    return;
}

le_result_t taf_DataProfile::UpdateAllProfilesFromListEvent(Profile_List_Event_t *listEvent)
{
    uint32_t i;
    taf_dcs_ProfileCtx_t *profilePtr;

    // Cleanup the ProfilesListPtr first, i.e. delete the
    // profiles that exist in ProfilesListPtr, but not in listEvent
    CleanupAllProfiles(listEvent);

    for (i = 0; i < listEvent->num; i++)
    {
        profilePtr = GetProfileCtx(listEvent->slotId, listEvent->profilesListPtr->item[i].info.index);
        if (profilePtr != NULL)
        {
            UpdateIndividualProfile(profilePtr, &listEvent->profilesListPtr->item[i]);
        }
        else
        {
            CreateIndividualProfile(&listEvent->profilesListPtr->item[i]);
        }
    }

    ProfileNum[(SlotId)(listEvent->slotId)] = listEvent->num;

    return LE_OK;
}

void taf_DataProfile::show(uint8_t slotId)
{
    le_dls_Link_t* linkPtr = NULL;

    LE_DEBUG("total profile number: %d", ProfileNum[(SlotId)slotId]);
    LE_DEBUG("%-6s""%-6s""%-6s""%-12s""%-6s""%-10s""%-6s", "Index", "Refs", "type", "Name", "Pdp", "Apn", "Auth");

    linkPtr = le_dls_Peek(&ProfileCtxList);
    while (linkPtr)
    {
        taf_dcs_ProfileCtx_t* profileCtx = CONTAINER_OF(linkPtr, taf_dcs_ProfileCtx_t, link);
        linkPtr = le_dls_PeekNext(&ProfileCtxList, linkPtr);

        if(slotId != profileCtx->slotId)
            continue;

        LE_DEBUG("%-6d""%-6p""%-6d""%-12s""%-6d""%-10s""%-6d",
            profileCtx->info.index, profileCtx->reference, profileCtx->info.tech,
            profileCtx->info.name,  profileCtx->pdp, profileCtx->apn, profileCtx->auth);

    }

    return;
}

void taf_DataProfile::NotifyProfileListHandler(void *listEvent)
{
    uint32_t i, profileNum;

    profileNum = ((Profile_List_Event_t *)listEvent)->num;
    for (i = 0; i < profileNum; i++)
    {
        taf_dcs_ProfileCtx_t *itemPtr = &(((Profile_List_Event_t *)listEvent)->profilesListPtr->item[i]);
        memcpy((char *)&ProfileInfo[i], (const char *)&itemPtr->info, sizeof(taf_dcs_ProfileInfo_t));
    }

    le_mem_Release(((Profile_List_Event_t *)listEvent)->profilesListPtr);
}

void taf_DataProfile::ProcessListReq(void *listEvent)
{
    TAF_ERROR_IF_RET_NIL(listEvent == NULL, "listEvent is NULL!");
    Profile_List_Event_t * profileListEvtPtr = (Profile_List_Event_t *)listEvent;

    auto &myProfile = taf_DataProfile::GetInstance();

    myProfile.UpdateAllProfilesFromListEvent(profileListEvtPtr);

    myProfile.NotifyProfileListHandler(profileListEvtPtr);

    myProfile.show(profileListEvtPtr->slotId);

    LE_DEBUG("getting profile list, profileListEvtPtr->ret: %d", profileListEvtPtr->ret);
    myProfile.CmdSynchronousPromise.set_value(profileListEvtPtr->ret);

    return;
}

void* taf_DataProfile::ProfileEventThread(void* contextPtr)
{
    auto &myProfile = taf_DataProfile::GetInstance();
    le_sem_Ref_t semRef = (le_sem_Ref_t)contextPtr;

    // register profile list request event
    myProfile.ListReqEvent = le_event_CreateId("Profile List Request Event", sizeof(Profile_List_Event_t));
    le_event_AddHandler("Profile List Request Event", myProfile.ListReqEvent, ProcessListReq);

    le_sem_Post(semRef);

    LE_INFO("Create event loop for connection event");
    // start event loop
    le_event_RunLoop();
    return NULL;
}

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
void taf_DataProfile::onInitCompleted(telux::common::ServiceStatus status)
{
    std::lock_guard<std::mutex> lock(mtx);
    subSystemStatusUpdated = true;
    conVar.notify_all();
}
#endif

void taf_DataProfile::Init(void)
{
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    PhoneMgr = phoneFactory.getPhoneManager();
    //  Check if telephony subsystem is ready
    bool PhSubSystemStatus = PhoneMgr->isSubsystemReady();

    if (!PhSubSystemStatus) {
        LE_INFO("Wait telephony subsystem  to be ready...");
        std::future<bool> f = PhoneMgr->onSubsystemReady();
        //  Wait until the subsystem is ready.
        PhSubSystemStatus = f.get();
    }

    LE_INFO("Waiting result is OK");
    if(!PhSubSystemStatus)
        LE_ERROR("Failed to init telephony subsystem");

    int noOfSlots = MIN_SLOT_COUNT;

    auto &dataFactory = telux::data::DataFactory::getInstance();

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)

    if(telux::common::DeviceConfig::isMultiSimSupported())
    {
       noOfSlots = MAX_SLOT_COUNT;
       LE_INFO("MultiSim supported");
    }

    for(auto slotIdx = 1; slotIdx <= noOfSlots; slotIdx++)
    {
        telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
        subSystemStatusUpdated = false;
        auto initCb = std::bind(&taf_DataProfile::onInitCompleted, this, std::placeholders::_1);
        auto profMgr = dataFactory.getDataProfileManager((SlotId)slotIdx, initCb);
        bool subSysReady = false;

        if(profMgr)
        {
            std::unique_lock<std::mutex> lck(mtx);
            conVar.wait(lck, [this]{return this->subSystemStatusUpdated;});
            subSystemStatus = profMgr->getServiceStatus();
            if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
            {
                LE_INFO("Profile manager on slot id %d is ready.", (int)slotIdx);
                subSysReady = true;
            }
            else
            {
                LE_ERROR("Profile manager on slot id %d is not ready.", (int)slotIdx);
                subSysReady = false;
            }
            //If it is new manager and initialization passed
            if ( subSysReady &&
                (dataProfileManagers.find((SlotId)slotIdx) == dataProfileManagers.end()))
            {
                dataProfileManagers.emplace((SlotId)slotIdx, profMgr);
                ListProfileCb.emplace((SlotId)slotIdx, std::make_shared<taf_ProfileListCallback>((SlotId)slotIdx));
            }
        }
        else
        {
            LE_CRIT("Failed to get profile Manager instance ");
        }

        if(subSysReady)
        {
            LE_INFO("Data profile component is ready for slot id %d...",(int)slotIdx);
        }
        else
        {
            LE_FATAL("Unable to init data profile component for slot id %d !",(int)slotIdx);
        }
    }
#else
    auto profMgr = dataFactory.getDataProfileManager();
    bool ProfileSubSystemStatus = profMgr->isSubsystemReady();

    // If data subsystem is not ready, wait for it to be ready
    if (!ProfileSubSystemStatus)
    {
        LE_INFO("Data profile manager subsystem is not ready, Please wait");
        std::future<bool> f = profMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        ProfileSubSystemStatus = f.get();
    }

    if ( ProfileSubSystemStatus &&
        (dataProfileManagers.find((SlotId)SLOT_ID_1) == dataProfileManagers.end()))
    {
        dataProfileManagers.emplace((SlotId)SLOT_ID_1, profMgr);
        ListProfileCb.emplace((SlotId)SLOT_ID_1, std::make_shared<taf_ProfileListCallback>((SlotId)SLOT_ID_1));
    }
    else
    {
        LE_FATAL("Unable to init data profile component for slot id !");
    }
#endif

    ModifyProfileCb = std::make_shared<taf_ProfileModifyCallback>();
    // Add callback for create profile command.
    CreateProfileCb = std::make_shared<taf_CreateProfileCallback>();

    // this pool is for allocing profile items, supports up to 32 profiles
    ProfilePool = le_mem_InitStaticPool(tafProfilePool, TAF_DCS_PROFILE_LIST_MAX_ENTRY, sizeof(taf_dcs_ProfileCtx_t));

    // this pool is for allocing profile list events, support up to 32 profile list events
    ListEventPool   = le_mem_InitStaticPool(tafProfileEvent, TAF_DCS_PROFILE_LIST_MAX_ENTRY, sizeof(taf_dcs_ProfileCtxs_t));

    // this reference map is for profile context
    ProfileRefMap = le_ref_CreateMap("tafDataProfileRef", TAF_DCS_PROFILE_LIST_MAX_ENTRY);

    le_sem_Ref_t semRef = le_sem_Create("ProfileThreadSem", 0);
    ProfileEventThreadRef = le_thread_Create("DcsProfileThread", ProfileEventThread, (void*)semRef);
    le_thread_Start(ProfileEventThreadRef);
    le_sem_Wait(semRef);
    le_sem_Delete(semRef);

    // send a profile Req to get default profiles
    for(auto slotIdx = 1; slotIdx <= noOfSlots; slotIdx++){

        SendProfileListReq(slotIdx);
    }

    return;
}
