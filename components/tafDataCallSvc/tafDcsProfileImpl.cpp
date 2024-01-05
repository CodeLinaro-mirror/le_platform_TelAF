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
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
using namespace telux::tafsvc;

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

taf_dcs_ProfileRef_t taf_DataProfile::GetProfileRef(uint8_t slotId, int32_t index)
{
    taf_dcs_ProfileCtx_t * profileCtxPtr = GetProfileCtx(slotId, index);

    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, NULL, "cannot get reference from index[%d]", index);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr->reference == NULL, NULL, "reference is invalid from index[%d]", index);

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

    TAF_ERROR_IF_RET_VAL(profileRef == NULL, LE_NOT_FOUND, "some pointers may be null");
    TAF_ERROR_IF_RET_VAL(GetSlotIdAndProfileId(profileRef, &slotId, &profileId) != LE_OK,
                         LE_NOT_FOUND, "cannot get profile id from reference(%p)", profileRef);

    profileCtxPtr = GetProfileCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND,
                         "cannot get profile context from reference(%p)", profileRef);

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

void taf_DataProfile::CreateIndividualProfile(taf_dcs_ProfileCtx_t *info)
{
    taf_dcs_ProfileCtx_t* profileCtx = NULL;

    profileCtx = (taf_dcs_ProfileCtx_t *)le_mem_ForceAlloc(ProfilePool);
    TAF_ERROR_IF_RET_NIL(profileCtx == NULL, "cannot alloc profileCtx");
    memcpy((char *)profileCtx, (char *)info, sizeof(taf_dcs_ProfileCtx_t));

    // create reference for this profile context
    taf_dcs_ProfileRef_t profileRef = (taf_dcs_ProfileRef_t)le_ref_CreateRef(ProfileRefMap, (void *)profileCtx);
    TAF_ERROR_IF_RET_NIL(profileRef == NULL, "cannot alloc profileRef");

    profileCtx->reference = profileRef;

    // add this profile context to list
    le_dls_Queue(&ProfileCtxList, &profileCtx->link);

    return;
}

taf_dcs_ProfileCtx_t * taf_DataProfile::GetProfileCtx(uint8_t slotId, uint32_t index)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&ProfileCtxList);
    while (linkPtr)
    {
        taf_dcs_ProfileCtx_t* profileCtx = CONTAINER_OF(linkPtr, taf_dcs_ProfileCtx_t, link);
        linkPtr = le_dls_PeekNext(&ProfileCtxList, linkPtr);

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
            LE_CRIT("Unable to init data profile component for slot id %d !",(int)slotIdx);
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
        LE_CRIT("Unable to init data profile component for slot id !");
    }
#endif

    ModifyProfileCb = std::make_shared<taf_ProfileModifyCallback>();

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


