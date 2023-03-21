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
LE_MEM_DEFINE_STATIC_POOL(tafProfileListHandler, TAF_DCS_PROFILE_LIST_MAX_ENTRY, sizeof(taf_dcs_ProfileListHandler_t));

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
                profile->getId(), profile->getName().c_str(), profile->getApn().c_str(), profile->getUserName().c_str(), profile->getPassword().c_str());
            LE_DEBUG("IP Family: %d, Tech Perf: %d, Auth Type: %d",
                (uint32_t)profile->getIpFamilyType(), (uint32_t)profile->getTechPreference(), (uint32_t)profile->getAuthProtocolType());

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
            num++;
        }
    }

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("response error! error code: %d", (int32_t)error);
        listEvent.ret = LE_FAULT;
    }
    else
    {

        listEvent.ret = LE_OK;
    }

    listEvent.num = num;
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

bool taf_DataProfile::IsListHandlerBound(le_msg_SessionRef_t sessionRef)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&ProfileReqHandlerList);
    while (linkPtr)
    {
        taf_dcs_ProfileListHandler_t* handlerCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_ProfileListHandler_t, link);
        linkPtr = le_dls_PeekNext(&ProfileReqHandlerList, linkPtr);
        if (handlerCtxPtr && (handlerCtxPtr->sessionRef == sessionRef))
        {
            return true;
        }
    }

    return false;
}

le_result_t taf_DataProfile::AddListHandler(le_msg_SessionRef_t sessionRef, taf_dcs_ProfileListHandlerFunc_t handlerPtr, void *contextPtr)
{
    // add this handler link to handler list
    taf_dcs_ProfileListHandler_t* handlerCtxPtr = (taf_dcs_ProfileListHandler_t *)le_mem_ForceAlloc(ListHandlerPool);
    TAF_ERROR_IF_RET_VAL(handlerCtxPtr == NULL, LE_NO_MEMORY, "cannot alloc memory for handler context!");

    handlerCtxPtr->sessionRef = sessionRef;
    handlerCtxPtr->link = LE_DLS_LINK_INIT;
    handlerCtxPtr->handlerPtr = handlerPtr;
    handlerCtxPtr->contextPtr = contextPtr;
    le_dls_Queue(&ProfileReqHandlerList, &(handlerCtxPtr->link));

    return LE_OK;
}

le_result_t taf_DataProfile::ListProfileAsync(taf_dcs_ProfileListHandlerFunc_t handlerPtr, void *contextPtr)
{
    if (IsListHandlerBound(taf_dcs_GetClientSessionRef()) == false)
    {
        (void)AddListHandler(taf_dcs_GetClientSessionRef(), handlerPtr, contextPtr);
    }

    return SendProfileListReq();
}

le_result_t taf_DataProfile::ListProfile(taf_dcs_ProfileInfo_t *profileList, size_t *listSize)
{
    le_result_t result;
    le_dls_Link_t* linkPtr = NULL;
    int profileCnt = 0;

    // initialize the synchronous promise
    CmdSynchronousPromise = std::promise<le_result_t>();

    //Remove IsOnSynchronousAction, otherwise CmdSynchronousPromise will not be set and program
    //will be stuck.

    result = SendProfileListReq();

    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();

    if (result != LE_OK)
    {
        LE_ERROR("getting profile list is failed, result: %d", result);
    }

    linkPtr = le_dls_Peek(&ProfileCtxList);
    while (linkPtr)
    {
        taf_dcs_ProfileCtx_t* profileCtx = CONTAINER_OF(linkPtr, taf_dcs_ProfileCtx_t, link);
        linkPtr = le_dls_PeekNext(&ProfileCtxList, linkPtr);

        memcpy((char *)&profileList[profileCnt], (const char *)&profileCtx->info, sizeof(taf_dcs_ProfileInfo_t));
        profileCnt++;
    }

    *listSize = profileCnt;

    return LE_OK;
}

le_result_t taf_DataProfile::SendProfileListReq()
{
    telux::common::Status status;

    status = ProfileMgr->requestProfileList(ListProfileCb);
    if (status != telux::common::Status::SUCCESS)
    {
        return LE_FAULT;
    }

    return LE_OK;
}

taf_dcs_ProfileRef_t taf_DataProfile::GetProfileRef(int32_t index)
{
    taf_dcs_ProfileCtx_t * profileCtxPtr = GetProfileCtx(index);

    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, NULL, "cannot get reference from index[%d]", index);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr->reference == NULL, NULL, "reference is invalid from index[%d]", index);

    return profileCtxPtr->reference;
}

le_result_t taf_DataProfile::GetProfileId(taf_dcs_ProfileRef_t profileRef, int32_t *profileId)
{
    TAF_ERROR_IF_RET_VAL(profileRef == NULL, LE_NOT_FOUND, "reference is invalid");
    taf_dcs_ProfileCtx_t* profileCtxPtr = (taf_dcs_ProfileCtx_t* )le_ref_Lookup(ProfileRefMap, (void*)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND, "cannot get profile context from reference(%p)", profileRef);

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
    TAF_ERROR_IF_RET_VAL(apnSize < TAF_DCS_APN_NAME_MAX_LEN, LE_OVERFLOW, "apnSize(%d) is smaller than NAME_MAX_BYTES(%d)", apnSize, TAF_DCS_APN_NAME_MAX_LEN);
    TAF_ERROR_IF_RET_VAL((profileRef == NULL) || (apnPtr == NULL), LE_NOT_FOUND, "some pointers may be null");
    taf_dcs_ProfileCtx_t* profileCtxPtr = (taf_dcs_ProfileCtx_t* )le_ref_Lookup(ProfileRefMap, (void*)profileRef);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND, "cannot get profile context from reference(%p)", profileRef);
    LE_INFO("apn: %s...profile id: %d", apnPtr, profileCtxPtr->info.index);
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
    LE_INFO("apntype: %d...profile id: %d", (int)*apnTypePtr, profileCtxPtr->info.index);

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

le_result_t taf_DataProfile::SendProfileModificationReq(int32_t profileId, telux::data::ProfileParams &params)
{
    // need reset promise.
    CmdSynchronousPromise = std::promise<le_result_t>();

    telux::common::Status status = ProfileMgr->modifyProfile(profileId, params, ModifyProfileCb);
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
    taf_dcs_ProfileCtx_t * profileCtxPtr;
    telux::data::ProfileParams params;
    le_result_t result;

    TAF_ERROR_IF_RET_VAL((profileRef == NULL) || (apnPtr == NULL), LE_NOT_FOUND, "some pointers may be null");
    TAF_ERROR_IF_RET_VAL(GetProfileId(profileRef, &profileId) != LE_OK, LE_NOT_FOUND, "cannot get profile id from reference(%p)", profileRef);

    profileCtxPtr = GetProfileCtx(profileId);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND, "cannot get profile context from reference(%p)", profileRef);

    MapProfileCtxToParams(profileCtxPtr, params);
    params.apn = apnPtr;

    result = SendProfileModificationReq(profileId, params);
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
    taf_dcs_ProfileCtx_t * profileCtxPtr;
    telux::data::ProfileParams params;
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(profileRef == NULL, LE_NOT_FOUND, "some pointers may be null");
    TAF_ERROR_IF_RET_VAL(GetProfileId(profileRef, &profileId) != LE_OK, LE_NOT_FOUND, "cannot get profile id from reference(%p)", profileRef);

    profileCtxPtr = GetProfileCtx(profileId);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND, "cannot get profile context from reference(%p)", profileRef);

    MapProfileCtxToParams(profileCtxPtr, params);
    params.ipFamilyType = MapIpFamily(pdp);

    result = SendProfileModificationReq(profileId, params);
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
    taf_dcs_ProfileCtx_t * profileCtxPtr;
    telux::data::ProfileParams params;
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(profileRef == NULL, LE_NOT_FOUND, "some pointers may be null");
    TAF_ERROR_IF_RET_VAL(GetProfileId(profileRef, &profileId) != LE_OK, LE_NOT_FOUND, "cannot get profile id from reference(%p)", profileRef);

    profileCtxPtr = GetProfileCtx(profileId);
    TAF_ERROR_IF_RET_VAL(profileCtxPtr == NULL, LE_NOT_FOUND, "cannot get profile context from reference(%p)", profileRef);

    MapProfileCtxToParams(profileCtxPtr, params);
    params.authType = MapAuthProtocol(type);
    params.userName = userName;
    params.password = password;

    result = SendProfileModificationReq(profileId, params);
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

taf_dcs_ProfileCtx_t * taf_DataProfile::GetProfileCtx(uint32_t index)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&ProfileCtxList);
    while (linkPtr)
    {
        taf_dcs_ProfileCtx_t* profileCtx = CONTAINER_OF(linkPtr, taf_dcs_ProfileCtx_t, link);
        linkPtr = le_dls_PeekNext(&ProfileCtxList, linkPtr);

        if (profileCtx->info.index == index)
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
        profilePtr = GetProfileCtx(listEvent->profilesListPtr->item[i].info.index);
        if (profilePtr != NULL)
        {
            UpdateIndividualProfile(profilePtr, &listEvent->profilesListPtr->item[i]);
        }
        else
        {
            CreateIndividualProfile(&listEvent->profilesListPtr->item[i]);
        }
    }

    ProfileNum = listEvent->num;

    return LE_OK;
}

uint32_t taf_DataProfile::GetProfileNum()
{
    return ProfileNum;
}

void taf_DataProfile::show()
{
    le_dls_Link_t* linkPtr = NULL;

    LE_DEBUG("total profile number: %d", ProfileNum);
    LE_DEBUG("%-6s""%-6s""%-6s""%-12s""%-6s""%-10s""%-6s", "Index", "Refs", "type", "Name", "Pdp", "Apn", "Auth");

    linkPtr = le_dls_Peek(&ProfileCtxList);
    while (linkPtr)
    {
        taf_dcs_ProfileCtx_t* profileCtx = CONTAINER_OF(linkPtr, taf_dcs_ProfileCtx_t, link);
        linkPtr = le_dls_PeekNext(&ProfileCtxList, linkPtr);

        LE_DEBUG("%-6d""%-6p""%-6d""%-12s""%-6d""%-10s""%-6d",
            profileCtx->info.index, profileCtx->reference, profileCtx->info.tech,
            profileCtx->info.name,  profileCtx->pdp, profileCtx->apn, profileCtx->auth);

    }

    return;
}

void taf_DataProfile::NotifyProfileListHandler(void *listEvent)
{
    le_dls_Link_t* linkPtr = NULL;

    uint32_t i, profileNum;

    profileNum = ((Profile_List_Event_t *)listEvent)->num;
    for (i = 0; i < profileNum; i++)
    {
        taf_dcs_ProfileCtx_t *itemPtr = &(((Profile_List_Event_t *)listEvent)->profilesListPtr->item[i]);
        memcpy((char *)&ProfileInfo[i], (const char *)&itemPtr->info, sizeof(taf_dcs_ProfileInfo_t));
    }

    linkPtr = le_dls_Peek(&ProfileReqHandlerList);
    while (linkPtr)
    {
        taf_dcs_ProfileListHandler_t* handlerCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_ProfileListHandler_t, link);
        linkPtr = le_dls_PeekNext(&ProfileReqHandlerList, linkPtr);
        if (handlerCtxPtr)
        {
            LE_INFO("notify session: %p", handlerCtxPtr->sessionRef);
            handlerCtxPtr->handlerPtr(
                ((Profile_List_Event_t *)listEvent)->ret,
                ProfileInfo,
                profileNum,
                handlerCtxPtr->contextPtr);
            le_dls_Remove(&ProfileReqHandlerList, &handlerCtxPtr->link);
            le_mem_Release(handlerCtxPtr);
        }
        else
        {
            LE_ERROR("handlerCtxPtr is NULL!");
            continue;
        }
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

    myProfile.show();

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

void taf_DataProfile::Init(void)
{
    auto &dataFactory = DataFactory::getInstance();
    ProfileMgr = dataFactory.getDataProfileManager();

    bool isReady = ProfileMgr->isSubsystemReady();
    if(isReady == false)
    {
        LE_INFO("data profile component is not ready, wait for it unconditionally...");
        std::future<bool> readyFunc = ProfileMgr->onSubsystemReady();
        isReady = readyFunc.get();
    }

    if(isReady)
    {
        LE_INFO("data profile component is ready...");
    }
    else
    {
        LE_CRIT("unable to init data profile component!");
    }

    ListProfileCb = std::make_shared<taf_ProfileListCallback>("TELAF_REQ_PROFILE_LIST");

    ModifyProfileCb = std::make_shared<taf_ProfileModifyCallback>();

    // this pool is for allocing profile items, supports up to 32 profiles
    ProfilePool = le_mem_InitStaticPool(tafProfilePool, TAF_DCS_PROFILE_LIST_MAX_ENTRY, sizeof(taf_dcs_ProfileCtx_t));

    // this pool is for allocing profile list events, support up to 32 profile list events
    ListEventPool   = le_mem_InitStaticPool(tafProfileEvent, TAF_DCS_PROFILE_LIST_MAX_ENTRY, sizeof(taf_dcs_ProfileCtxs_t));

    // this pool is for allocing profile list handler, support up to 32 handlers
    ListHandlerPool = le_mem_InitStaticPool(tafProfileListHandler, TAF_DCS_PROFILE_LIST_MAX_ENTRY, sizeof(taf_dcs_ProfileListHandler_t));

    // this reference map is for profile context
    ProfileRefMap = le_ref_CreateMap("tafDataProfileRef", TAF_DCS_PROFILE_LIST_MAX_ENTRY);

    le_sem_Ref_t semRef = le_sem_Create("ProfileThreadSem", 0);
    ProfileEventThreadRef = le_thread_Create("DcsProfileThread", ProfileEventThread, (void*)semRef);
    le_thread_Start(ProfileEventThreadRef);
    le_sem_Wait(semRef);
    le_sem_Delete(semRef);

    // send a profile Req to get default profiles
    SendProfileListReq();

    return;
}


