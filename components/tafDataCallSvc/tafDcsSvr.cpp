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

/*
 * @file       tafDcsSvr.cpp
 * @brief      This file provides the telematics application framework data call service APIs.
 */
#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include "tafSvcIF.hpp"
#include "tafDcsConnectionImpl.hpp"
#include "tafDcsProfileImpl.hpp"

using namespace telux::data;
using namespace telux::common;
using namespace telux::tafsvc;

/**
 * The initialization of TelAF data profile component.
 */
void taf_dcs_profile_init()
{
    LE_INFO("taf data profile component init start...\n");
    auto &dataProfile = taf_DataProfile::GetInstance();
    dataProfile.Init();
    LE_INFO("taf data profile component init done...\n");

    return;
}

/**
 * Get the data profile list.
 *
 * This function call is synchronous, the profile list will be returned by handlerPtr.
 *
 * @param [in] handlerPtr               The handler when proflie list comes back.
 * @param [in] contextPtr               The handler function context.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to get profile list.
 */
le_result_t taf_dcs_GetProfileList(taf_dcs_ProfileInfo_t *profileList, size_t *listSize)
{
    auto &dataProfile = taf_DataProfile::GetInstance();

    return dataProfile.ListProfile(profileList, listSize);
}

/**
 * Get the total data profile number.
 *
 * @returns profile number
 */
uint32_t taf_dcs_GetNum()
{
    auto &dataProfile = taf_DataProfile::GetInstance();

    return dataProfile.GetProfileNum();
}

/**
 * Get the profile reference corresponding to specified profile index.
 *
 *
 * @param [in] index                    The profile index.
 *
 * @returns reference                   The profile reference, should not be NULL.
 */
taf_dcs_ProfileRef_t taf_dcs_GetProfile(uint32_t index)
{
    auto &dataProfile = taf_DataProfile::GetInstance();

    return dataProfile.GetProfileRef(index);
}

/**
 * Get data profile index corresponding to specified profile reference.
 *
 * @param [in] profileRef               The profile want to be used.
 *
 * @returns profile index.
 */
uint32_t taf_dcs_GetProfileIndex
(
    taf_dcs_ProfileRef_t profileRef
)
{
    int32_t profileId;
    auto &dataProfile = taf_DataProfile::GetInstance();
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);

    if (result == LE_OK)
    {
        return profileId;
    }
    else
    {
        LE_ERROR("getting profile id from reference(%p) is failed, will use default id", profileRef);
        return TAF_DCS_DEFAULT_PROFILE;
    }

}

/**
 * Set data profile APN corresponding to specified profile reference.
 *
 * This function call is asynchronous, the result will be returned by data event handler if failed.
 *
 * @param [in] profileRef               The profile which want to be updated.
 * @param [in] apnPtr                   The new APN string.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to update profile infomation.
 */
le_result_t taf_dcs_SetAPN(taf_dcs_ProfileRef_t profileRef, const char *apnPtr)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.SetApn(profileRef, apnPtr);
}

/**
 * Set data profile PDP corresponding to specified profile reference.
 *
 * This function call is asynchronous, the result will be returned by data event handler if failed.
 *
 * @param [in] profileRef               The profile which want to be updated.
 * @param [in] pdp                      The new PDP type.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to update profile infomation.
 */
le_result_t taf_dcs_SetPDP
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_Pdp_t        pdp
)
{
    int32_t profileId;
    taf_dcs_ConState_t state = TAF_DCS_CONNECTED;
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();


    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    // Check if the data session is currently not disconnected for the given profile.
    if(dataConnection.GetConnectionState(profileId, &state) == LE_OK &&
       state != TAF_DCS_DISCONNECTED)
        return LE_FAULT;

    return dataProfile.SetPdp(profileRef, pdp);
}

/**
 * Set data profile authentication type, username and password corresponding to specified profile reference.
 *
 * This function call is asynchronous, the result will be returned by data event handler if failed.
 *
 * @param [in] profileRef               The profile which want to be updated.
 * @param [in] type                     The authentication type.
 * @param [in] userName                 The authentication username string.
 * @param [in] password                 The authentication password string
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to update profile infomation.
 */
le_result_t taf_dcs_SetAuthentication
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_Auth_t       type,
    const char         *userName,
    const char         *password
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.SetAuth(profileRef, type, userName, password);
}

/**
 * Get data profile APN string corresponding to specified profile reference.
 *
 * @param [in] profileRef               The profile which want to be updated.
 * @param [in] apnSize                  The input APN string max size.
 * @param [out] apnPtr                  The APN string, the size of this string should not exceed apnSize.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to get profile infomation.
 */
le_result_t taf_dcs_GetAPN
(
    taf_dcs_ProfileRef_t  profileRef,
    char                 *apnPtr,
    size_t                apnSize
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.GetApn(profileRef, apnPtr, apnSize);
}

/**
 * Get data profile APN type corresponding to specified profile reference.
 *
 * @param [in] profileRef               The profile which want to be updated.
 * @param [out] apnType                 The APN type.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to get APN type.
 */
le_result_t taf_dcs_GetApnTypes
(
    taf_dcs_ProfileRef_t  profileRef,
    taf_dcs_ApnType_t     *apnTypePtr
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.GetApnTypes(profileRef, apnTypePtr);
}
/**
 * Get data profile PDP corresponding to specified profile reference.
 *
 * @param [in] profileRef               The profile which want to be updated.
 *
 * @returns DDP type.
 */
taf_dcs_Pdp_t taf_dcs_GetPDP
(
    taf_dcs_ProfileRef_t profileRef
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.GetPdp(profileRef);
}

/**
 * Get data profile authentication type, username and password corresponding to specified profile reference.
 *
 * @param [in] profileRef               The profile which want to be updated.
 * @param [out] typePtr                 The authentication type.
 * @param [out] userNamePtr             The authentication username string.
 * @param [in] userNameSize             The authentication username max size, userNamePtr should not exceed this value.
 * @param [out] passwordPtr             The authentication password string.
 * @param [in] passwordSize             The authentication password max size, passwordPtr should not exceed this value.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to get profile infomation.
 */
le_result_t taf_dcs_GetAuthentication
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_Auth_t      *typePtr,
    char                *userNamePtr,
    size_t              userNameSize,
    char                *passwordPtr,
    size_t              passwordSize
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.GetAuthentication(profileRef, typePtr, userNamePtr, userNameSize, passwordPtr, passwordSize);
}

/**
 * Start to make a synchronous call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 * This is a synchronous function call.
 *
 * @param [in] profileRef               The profile reference to be started.
 *
 * @returns LE_OK                       Success to start this data session.
 *          OTHER                       Failed to start this data session.
 */
le_result_t taf_dcs_StartSession(taf_dcs_ProfileRef_t profileRef)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    taf_dcs_Pdp_t pdpType = dataProfile.GetPdp(profileRef);
    return dataConnection.StartSessionCmdSync(profileId, pdpType, taf_dcs_GetClientSessionRef());
}

/**
 * Start to make a asynchronous call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 * This is an asynchronous function call, the handlerPtr will be called after getting the result.
 *
 * @param [in] profileRef               The profile reference to be started.
 *
 * @returns None.
 */
void taf_dcs_StartSessionAsync
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();

    return dataConnection.StartSessionCmdAsync(profileRef, handlerPtr, contextPtr,
                                               taf_dcs_GetClientSessionRef());
}

/**
 * Synchronouly stop a call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 * This is a synchronous function call, the state events will be reported by session state handler.
 *
 * @param [in] profileRef               The profile reference to be stopped.
 *
 * @returns LE_OK                       Success to stop this data session.
 *          OTHER                       Failed to stop this data session.
 */
le_result_t taf_dcs_StopSession(taf_dcs_ProfileRef_t profileRef)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    taf_dcs_Pdp_t pdpType = dataProfile.GetPdp(profileRef);
    return dataConnection.StopSessionCmdSync(profileId, pdpType, taf_dcs_GetClientSessionRef());
}

/**
 * Asynchronouly stop a call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 * This is an asynchronous function call, the handlerPtr will be called after getting the result.
 *
 * @param [in] profileRef               The profile reference to be stopped.
 *
 * @returns None.
 */
void taf_dcs_StopSessionAsync
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();

    return dataConnection.StopSessionCmdAsync(profileRef, handlerPtr, contextPtr,
                                              taf_dcs_GetClientSessionRef());
}

/**
 * First session state handler used by le_event_AddLayeredHandler().
 *
 * @param [in] reportPtr               event pointer.
 * @param [in] subHandlerFunc          The secondary handler pointer, i.e. handlerPtr() from taf_dcs_AddSessionStateHandler().
 */
static void FirstSessionStateHandler(void* reportPtr, void* subHandlerFunc)
{
    DataCallState_t* stateEvent = (DataCallState_t *)reportPtr;
    taf_dcs_SessionStateHandlerFunc_t handlerFunc = (taf_dcs_SessionStateHandlerFunc_t)subHandlerFunc;
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    LE_INFO("send callback to callRef: %p, callEvent: %s\n", stateEvent->callRef,
        dataConnection.CallEventToString(stateEvent->callEvent));

    int32_t profileId = dataConnection.GetProfileId(stateEvent->callRef);
    taf_dcs_ProfileRef_t profileRef = dataProfile.GetProfileRef(profileId);
    TAF_ERROR_IF_RET_NIL(profileRef == NULL, "cannot get profile reference from profile(%d)", profileId);

    handlerFunc(profileRef, stateEvent->callEvent, &stateEvent->info, le_event_GetContextPtr());
}

/**
 * Add a session state handler to monitor the specified data call.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [in] handlerPtr               The handler function.
 * @param [in] contextPtr               The handler context.
 *
 * @returns reference                   Success to add session state handler.
 *          NULL                        Failed to add session state handler.
 */
taf_dcs_SessionStateHandlerRef_t taf_dcs_AddSessionStateHandler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_SessionStateHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, NULL, "profile reference(%p) is invalid", profileRef);

    le_event_Id_t sessionStateEvent = dataConnection.GetSessionStateEvent(profileId);
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                    "DataCallState",
                                                    sessionStateEvent,
                                                    FirstSessionStateHandler,
                                                    (void *)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_dcs_SessionStateHandlerRef_t)(handlerRef);
}

/**
 * Remove session state handler.
 *
 * @param [in] handlerRef               The state handler reference returned by taf_dcs_AddSessionStateHandler().
 *
 * @returns NA
 *
 * @note    NA
 */
void taf_dcs_RemoveSessionStateHandler
(
    taf_dcs_SessionStateHandlerRef_t handlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t) handlerRef);
    return;
}

/**
 * First roaming status handler used by taf_dcs_AddRoamingStatusHandler().
 *
 * @param [in] reportPtr               event pointer.
 * @param [in] subHandlerFunc          The secondary handler pointer, i.e. handlerPtr() from taf_dcs_AddRoamingStatusHandler().
 */
static void FirstRoamingStatusHandler(void* reportPtr, void* subHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(subHandlerFunc == nullptr, "Null ptr(subHandlerFunc)");

    taf_dcs_RoamingStatusHandlerFunc_t handlerFunc =
                                                 (taf_dcs_RoamingStatusHandlerFunc_t)subHandlerFunc;
    handlerFunc((taf_dcs_RoamingStatusInd_t*)reportPtr, le_event_GetContextPtr());

    le_mem_Release(reportPtr);

}

/**
 * Add a roaming status handler to monitor the roaming status.
 *
 * @param [in] handlerPtr               The handler function.
 * @param [in] contextPtr               The handler context.
 *
 * @returns reference                   Success to add roaming status handler.
 *          NULL                        Failed to add roaming status handler.
 */
taf_dcs_RoamingStatusHandlerRef_t taf_dcs_AddRoamingStatusHandler
(
    taf_dcs_RoamingStatusHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                    "RoamingStatus",
                                                    dataConnection.RoamingStatusEvtId,
                                                    FirstRoamingStatusHandler,
                                                    (void *)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_dcs_RoamingStatusHandlerRef_t)(handlerRef);
}

/**
 * Remove roaming status handler.
 *
 * @param [in] handlerRef The state handler reference returned by taf_dcs_AddRoamingStatusHandler().
 *
 * @returns NA
 *
 * @note    NA
 */
void taf_dcs_RemoveRoamingStatusHandler
(
    taf_dcs_RoamingStatusHandlerRef_t handlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t) handlerRef);
    return;
}

/**
 * Get the roaming status.
 *
 * @param [in] phoneId                  The phone id.
 * @param [out] isRoamingPtr            Is roaming on or off.
 * @param [out] typePtr                 The roaming type.
 *
 * @returns LE_OK                       Success to get the roaming status.
 *          OTHER                       Failed to get the roaming status.
 *
 */

le_result_t taf_dcs_GetRoamingStatus
(
    uint8_t phoneId,
    bool* isRoamingPtr,
    taf_dcs_RoamingType_t* typePtr
)
{
    TAF_ERROR_IF_RET_VAL(isRoamingPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(isRoamingPtr)");
    TAF_ERROR_IF_RET_VAL(typePtr == nullptr, LE_BAD_PARAMETER, "Null ptr(typePtr)");

    auto &dataConnection = taf_DataConnection::GetInstance();

    return dataConnection.GetRoamingStatus(isRoamingPtr, typePtr);
}

/**
 * Get the interface address corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] addrPtr                 The interface name address.
 * @param [in] addrSize                 The max size of interface name string, specified in taf_dcs.api.
 *
 * @returns LE_OK                       Success to interface name.
 *          OTHER                       Failed to interface name.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetInterfaceName(taf_dcs_ProfileRef_t profileRef, char* namePtr, size_t nameSize)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    return dataConnection.GetInterfaceName(profileId, namePtr, nameSize);
}

/**
 * Get the IPv4 interface address corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] addrPtr                 The IP address.
 * @param [in] addrSize                 The max size of gateway string, specified in taf_dcs.api.
 *
 * @returns LE_OK                       Success to get IP address.
 *          OTHER                       Failed to get IP address.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetIPv4Address(taf_dcs_ProfileRef_t profileRef, char *addrPtr, size_t addrSize)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    return dataConnection.GetIpv4Address(profileId, addrPtr, addrSize);
}

/**
 * Get the IPv4 gateway corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] addrPtr                 The gateway address.
 * @param [in] addrSize                 The max size of gateway string, specified in taf_dcs.api.
 *
 * @returns LE_OK                       Success to get gateway address.
 *          OTHER                       Failed to get gateway address.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetIPv4GatewayAddress
(
    taf_dcs_ProfileRef_t    profileRef,
    char*   addrPtr,
    size_t  addrSize
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    return dataConnection.GetIpv4Gateway(profileId, addrPtr, addrSize);
}

/**
 * Get the IPv4 DNS corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] dns1AddrPtr             The first DNS address.
 * @param [in] dns1AddrSize             The max size of DNS address, specified in taf_dcs.api.
 * @param [out] dns2AddrPtr             The secondary DNS address.
 * @param [in] dns2AddrSize             The max size of DNS address, specified in taf_dcs.api..
 *
 * @returns LE_OK                       Success to get DNS address.
 *          OTHER                       Failed to get DNS address.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetIPv4DNSAddresses
(
    taf_dcs_ProfileRef_t    profileRef,
    char*   dns1AddrPtr,
    size_t  dns1AddrSize,
    char*   dns2AddrPtr,
    size_t  dns2AddrSize
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    return dataConnection.GetIpv4Dns(profileId, dns1AddrPtr, dns1AddrSize, dns2AddrPtr, dns2AddrSize);
}

/**
 * Get the IPv6 interface address corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] addrPtr                 The IP address.
 * @param [in] addrSize                 The max size of gateway string, specified in taf_dcs.api.
 *
 * @returns LE_OK                       Success to get IP address.
 *          OTHER                       Failed to get IP address.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetIPv6Address(taf_dcs_ProfileRef_t profileRef, char *addrPtr, size_t addrSize)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    return dataConnection.GetIpv6Address(profileId, addrPtr, addrSize);
}

/**
 * Get the IPv6 gateway corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] addrPtr                 The gateway address.
 * @param [in] addrSize                 The max size of gateway string, specified in taf_dcs.api.
 *
 * @returns LE_OK                       Success to get gateway address.
 *          OTHER                       Failed to get gateway address.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetIPv6GatewayAddress
(
    taf_dcs_ProfileRef_t    profileRef,
    char*   addrPtr,
    size_t  addrSize
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    return dataConnection.GetIpv6Gateway(profileId, addrPtr, addrSize);
}

/**
 * Get the IPv6 DNS corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] dns1AddrPtr             The first DNS address.
 * @param [in] dns1AddrSize             The max size of DNS address, specified in taf_dcs.api.
 * @param [out] dns2AddrPtr             The secondary DNS address.
 * @param [in] dns2AddrSize             The max size of DNS address, specified in taf_dcs.api..
 *
 * @returns LE_OK                       Success to get DNS address.
 *          OTHER                       Failed to get DNS address.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetIPv6DNSAddresses
(
    taf_dcs_ProfileRef_t    profileRef,
    char*   dns1AddrPtr,
    size_t  dns1AddrSize,
    char*   dns2AddrPtr,
    size_t  dns2AddrSize
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    return dataConnection.GetIpv6Dns(profileId, dns1AddrPtr, dns1AddrSize, dns2AddrPtr, dns2AddrSize);
}

/**
 * Get the session state corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] statePtr                The session state enumeration, check ConState in taf_dcs.api.
 *
 * @returns LE_OK                       Success to get session state.
 *          OTHER                       Failed to get session state.
 *
 * @note    NA
 */
le_result_t taf_dcs_GetSessionState
(
    taf_dcs_ProfileRef_t    profileRef,
    taf_dcs_ConState_t*     statePtr
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    return dataConnection.GetConnectionState(profileId, statePtr);
}

/**
 * Get the data bearer technology corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked
 * @param [out] dlDataBearerTechPtr     The down link data bearer technology
 * @param [out] ulDataBearerTechPtr     The up link data bearer technology
 *
 * @returns LE_OK                       Success to get the data bearer technology
 *          OTHER                       Failed to get data bearer technology
 *
 * @note    NA
 */
le_result_t taf_dcs_GetDataBearerTechnology(taf_dcs_ProfileRef_t profileRef, taf_dcs_DataBearerTechnology_t* downDataBearerTechPtr, taf_dcs_DataBearerTechnology_t* upDataBearerTechPtr)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    return dataConnection.GetDataBearerTechnology(profileId, downDataBearerTechPtr, upDataBearerTechPtr);
}

/**
 * Check whether this data profile supports IPv6 or not.
 *
 * @param [in] profileRef     The profile reference to be checked
 *
 * @returns false             NOT support IPv6 or profile reference is invalid
 *          true              Support IPv4
 *
 * @note    NA
 */
bool taf_dcs_IsIPv4(taf_dcs_ProfileRef_t profileRef)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    return dataConnection.IsIpv4(profileId);
}

/**
 * Check whether this data profile supports IPv6 or not.
 *
 * @param [in] profileRef     The profile reference to be checked
 *
 * @returns false             NOT support IPv6 or profile reference is invalid
 *          true              Support IP6v
 *
 * @note    NA
 */
bool taf_dcs_IsIPv6(taf_dcs_ProfileRef_t profileRef)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, false, "profile reference(%p) is invalid", profileRef);

    return dataConnection.IsIpv6(profileId);
}

/**
 * Use this method to send call state event to registered clients.
 */
void SendSessionStateEvent(taf_dcs_ConState_t event, taf_dcs_StateInfo_t *infoPtr, taf_dcs_CallCtx_t *callCtxPtr)
{
    DataCallState_t stateEvent;

    stateEvent.callRef   = callCtxPtr->callRef;
    stateEvent.callEvent = event;
    memcpy((char *)&stateEvent.info, (char *)infoPtr, sizeof(taf_dcs_StateInfo_t));
    le_event_Report(callCtxPtr->sessionStateEvent, &stateEvent, sizeof(stateEvent));
    return;
}

/**
 * Set the deafult data profile index which data call is using.
 *
 * @param [in] index                    The default profile index.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to set default profile.
 */
le_result_t taf_dcs_SetDefaultProfileIndex(uint32_t profileId)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    uint32_t profileIdGet;

    le_result_t result = dataConnection.GetDefaultProfileIdSync(profileIdGet);
    if (result != LE_OK)
    {
        LE_ERROR("getting default profile is failed, result: %d", result);
        return result;
    }

    if (profileIdGet == profileId)
    {
        LE_INFO("profile id(%d) does not change", profileIdGet);
        return LE_OK;
    }

    return dataConnection.SetDefaultProfileIdSync(profileId);
}

/**
 * Get the deafult data profile index which data call is using.
 *
 * @returns default profile index
 */
uint32_t taf_dcs_GetDefaultProfileIndex()
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    uint32_t profileId = TAF_DCS_DEFAULT_PROFILE;

    dataConnection.GetDefaultProfileIdSync(profileId);

    return profileId;
}

/**
 * Get the deafult data profile index which data call is using.
 *
 * @returns default profile index
 */
le_result_t taf_dcs_GetProfileIdByInterfaceName(const char* intfName,uint32_t* profileId)
{
    auto &dataConnection = taf_DataConnection::GetInstance();

    le_result_t result = dataConnection.GetProfileIdByInterfaceName(intfName,profileId);

    return result;
}

/**
 * The init function of TelAF data connection component.
 */
void taf_dcs_connection_init()
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    dataConnection.Init();

    dataConnection.RegisterSessionStateHandler(SendSessionStateEvent);

    LE_INFO("taf data profile component init done...\n");
}

/**
 * Change route corresponding to specified profile reference and network address.
 *
 * The data call which relates to this profile need to be brought up before this API.
 *
 * @param [in] profileRef               The profile reference to be checked
 * @param [in] destAddr                 The network address
 * @param [in] prefixLength             The prefix length
 * @param [in] isAdd                    Add or delete operation
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed.
 *
 * @note    NA
 */
le_result_t taf_dcs_ChangeRoute(taf_dcs_ProfileRef_t profileRef, const char *destAddr, const char *prefixLength, bool isAdd)
{
    return LE_OK;
}

/**
 * Set the default gateway.
 *
 * The data call which relates to this profile need to be brought up before this API.
 *
 * @param [in] profileRef               The profile reference to be checked
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed.
 *
 * @note    NA
 */
le_result_t  taf_dcs_SetDefaultGW(taf_dcs_ProfileRef_t profileRef)
{
    return LE_OK;
}

/**
 * Get the default gateway.
 *
 * The data call which relates to this profile need to be brought up before this API.
 *
 * @param [in] profileRef               The profile reference to be checked
 * @param [out] addr                    The default gateway address
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed.
 *
 * @note    NA
 */
le_result_t taf_dcs_GetDefaultGW(taf_dcs_ProfileRef_t profileRef, taf_dcs_DefaultGatewayAddresses_t* addr)
{
    return LE_OK;
}

/**
 * Backup the default gateway.
 *
 * @param [in] profileRef               The profile reference to be checked
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed.
 *
 * @note    NA
 */
void taf_dcs_BackupDefaultGW(void)
{
    return;
}

/**
 * Restore a backup gateway.
 *
 * The data call which relates to this profile need to be brought up before this API.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed.
 *
 * @note    NA
 */
le_result_t taf_dcs_RestoreDefaultGW(void)
{
    return LE_OK;
}

/**
 * Set the default DNS address.
 *
 * The data call which relates to this profile need to be brought up before this API.
 *
 * @param [in] profileRef               The profile reference to be checked
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed.
 *
 * @note    NA
 */
le_result_t taf_dcs_SetDNS(taf_dcs_ProfileRef_t profileRef)
{
    return LE_OK;
}

/**
 * Get the DNS address.
 *
 * The data call which relates to this profile need to be brought up before this API.
 *
 * @param [in] profileRef               The profile reference to be checked
 * @param [out] addr                    The DNS address
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed.
 *
 * @note    NA
 */
le_result_t taf_dcs_GetDNS(taf_dcs_ProfileRef_t profileRef, taf_dcs_DnsServerAddresses_t* addr)
{
    return LE_OK;
}

/**
 * Restore a DNS address.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed.
 *
 * @note    NA
 */
void taf_dcs_RestoreDNS(void)
{
    return;
}


/**
 * Start to make a synchronous permanent call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to
 * specified profile index.
 * This is a synchronous function call, the state events will be reported by session state handler.
 *
 * @param [in] profileRef               The profile reference to be started.
 *
 * @returns LE_OK                       Success to start this data session.
 *          OTHER                       Failed to start this data session.
 */
le_result_t taf_mdc_StartSession(taf_dcs_ProfileRef_t profileRef)
{
    LE_DEBUG("-----------taf_mdc_StartSession------------");
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    taf_dcs_Pdp_t pdpType = dataProfile.GetPdp(profileRef);
    // Start a data call with a fixed value 0 for sessionRef, and when the client loses the
    // connection with data call service,the data call will not be stopped
    return dataConnection.StartSessionCmdSync(profileId, pdpType, 0);

}

/**
 * Start to make a asynchronous call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to
 * specified profile index.
 * This is an asynchronous function call, the state events will be reported by session state handler
 *
 * @param [in] profileRef               The profile reference to be started.
 *
 * @returns LE_OK                       Success to start this data session.
 *          OTHER                       Failed to start this data session.
 */
le_result_t taf_mdc_StartSessionAsync(taf_dcs_ProfileRef_t profileRef)
{
    LE_DEBUG("-----------taf_mdc_StartSessionAsync------------");
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    taf_dcs_Pdp_t pdpType = dataProfile.GetPdp(profileRef);
    // Start a data call with a fixed value 0 for sessionRef, and when the client loses the
    // connection with data call service,the data call will not be stopped
    return dataConnection.StartSessionCmdSync(profileId, pdpType, 0);

}

/**
 * Synchronouly stop a permanent call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to
 * specified profile index.
 * This is an asynchronous function call, the state events will be reported by session state handler
 *
 * @param [in] profileRef               The profile reference to be stopped.
 *
 * @returns LE_OK                       Success to stop this data session.
 *          OTHER                       Failed to stop this data session.
 */
le_result_t taf_mdc_StopSession(taf_dcs_ProfileRef_t profileRef)
{
    LE_DEBUG("-----------taf_mdc_StopSession------------");
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    taf_dcs_Pdp_t pdpType = dataProfile.GetPdp(profileRef);
    // When the application calls taf_mdc_StartSession() to start a data call, this function
    // can stop that data call
    return dataConnection.StopSessionCmdSync(profileId, pdpType, 0);
}

/**
 * Asynchronouly stop a call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to
 * specified profile index.
 * This is an asynchronous function call, the state events will be reported by session state handler
 *
 * @param [in] profileRef               The profile reference to be stopped.
 *
 * @returns LE_OK                       Success to stop this data session.
 *          OTHER                       Failed to stop this data session.
 */
le_result_t taf_mdc_StopSessionAsync(taf_dcs_ProfileRef_t profileRef)
{
    LE_DEBUG("-----------taf_mdc_StopSessionAsync------------");
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    le_result_t result = dataProfile.GetProfileId(profileRef, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    taf_dcs_Pdp_t pdpType = dataProfile.GetPdp(profileRef);
    // When the application calls taf_mdc_StartSessionAsync() to start a data call, this function
    // can stop that data call
    return dataConnection.StopSessionCmdSync(profileId, pdpType, 0);

}

COMPONENT_INIT
{
    taf_dcs_profile_init();
    taf_dcs_connection_init();
    LE_INFO("the initialization of TelAf data call service is finished\n");
}

