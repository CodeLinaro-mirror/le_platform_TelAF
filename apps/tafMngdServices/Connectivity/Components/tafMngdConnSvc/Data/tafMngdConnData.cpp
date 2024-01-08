/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#include "tafMngdConnData.hpp"
#include "tafMngdConnAdmin.hpp"

#define MIN_PHONE_ID 1
#define MAX_PHONE_ID 2

using namespace telux::tafsvc;

void tafMngdConnData::Init(void)
{
     LE_INFO("tafMngdConnData: init");
}

tafMngdConnData &tafMngdConnData::GetInstance()
{
    static tafMngdConnData instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for data session state change.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnData::SessionStateChangeHandler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_ConState_t state,
    const taf_dcs_StateInfo_t *stateInfoPtr,
    void *contextPtr
)
{
    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();
    uint32_t profileId;
    uint8_t phoneId;
    stateMachineEvent_t stateMachineEvt = {TAF_MNGD_CONN_EVT_INIT,0};
    taf_mngd_Conn_Ctx_t* connCtxPtr = NULL;
    le_result_t result;

    profileId = taf_dcs_GetProfileIndex(profileRef);
    result = taf_dcs_GetPhoneId(profileRef, &phoneId);

    LE_DEBUG("Data Connection State: %d, phoneid: %d, profileId: %d, PDP: %d",
             state, phoneId, profileId, stateInfoPtr->ipType);
    if(state == TAF_DCS_CONNECTED && stateInfoPtr->ipType!=TAF_DCS_PDP_IPV4)
    {
        //Only IPv4 is supported
        //TODO: Add support for IPv6
        LE_DEBUG("Returning from here as only IPv4 is supported");
        return;
    }

    if(result != LE_OK)
    {
        LE_ERROR("Can't find the phoneId for profileRef(%p)", profileRef);
        return;
    }

    LE_DEBUG ("Data Connection State: %d, phoneid: %d, profileId: %d", state, phoneId, profileId);
    connCtxPtr = mngdConnAdmin.GetConnCtx(phoneId, profileId);
    //The connection is not created by tafMngdConnSvc, don't report the event.
    if(connCtxPtr == NULL)
    {
        LE_DEBUG("Can't find the context for phoneId(%d), profileId(%d)", phoneId, profileId);
        return;
    }
    switch (state)
    {
        case TAF_DCS_DISCONNECTED:
            LE_DEBUG ("Data Disconnected Event called for dataID  %d", connCtxPtr->dataId);
            stateMachineEvt.event = TAF_MNGD_CONN_EVT_DATA_CONNECTION_DISCONNECTED;
            stateMachineEvt.dataId = connCtxPtr->dataId;
            break;
        case TAF_DCS_CONNECTED:
            LE_DEBUG ("Data connected Event called for dataID  %d", connCtxPtr->dataId);
            stateMachineEvt.event=TAF_MNGD_CONN_EVT_DATA_CONNECTION_CONNECTED;
            stateMachineEvt.dataId = connCtxPtr->dataId;
            break;
        default:
            return;
    }

    le_event_Report(mngdConnAdmin.StateMachineEventId, &stateMachineEvt,
                    sizeof(stateMachineEvent_t));

}

//--------------------------------------------------------------------------------------------------
/**
 * Register data connection event handler.
 */
//--------------------------------------------------------------------------------------------------
void tafMngdConnData::RegisterEvents()
{
    taf_dcs_ProfileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
    size_t listSize = 0;
    le_result_t result;
    taf_dcs_ProfileRef_t profileRef = NULL;

    taf_dcs_ConnectService();

    for(int phoneId = MIN_PHONE_ID; phoneId <= MAX_PHONE_ID; phoneId++)
    {
        result = taf_dcs_GetProfileListEx(phoneId, profilesInfoPtr, &listSize);
        if(result != LE_OK)
        {
            LE_DEBUG("Getting profile list for phone id %d failed", phoneId);
            continue;
        }

        for (size_t i = 0; i < listSize; i++)
        {
            const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
            profileRef = taf_dcs_GetProfileEx(phoneId, profileInfoPtr->index);

            if (profileRef == NULL)
            {
                LE_ERROR("Invalid profile %p\n", profileRef);
                continue;
            }

            SessionStateHandlerRef =
                        taf_dcs_AddSessionStateHandler(profileRef, SessionStateChangeHandler, NULL);

            if (SessionStateHandlerRef == NULL)
            {
                LE_ERROR("Adding session state handler failed");
                continue;
            }
        }
    }

    LE_INFO ("Data Event callback is set");
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnData::Startdata(uint8_t phoneId, uint32_t profileId)
{
    le_result_t result;
    taf_dcs_ProfileRef_t profileRef = NULL;
    uint8_t defaultPhoneId = 0;
    uint32_t defaultProfileId = 0;

    result = taf_dcs_GetDefaultPhoneIdAndProfileId(&defaultPhoneId, &defaultProfileId);

    if(result == LE_OK)
    {
        if(defaultProfileId != profileId)
        {
            LE_INFO("Profile %d is not a default profile", profileId);
        }
        if (defaultPhoneId != phoneId)
        {
            LE_WARN("Phone ID %d is not default phone ID", phoneId);
            return LE_UNSUPPORTED;
        }
    }
    else
    {
        LE_ERROR("Getting default profile failed");
        return LE_FAULT;
    }

    profileRef = taf_dcs_GetProfileEx (phoneId, profileId);
    result = taf_dcs_StartSession(profileRef);

    LE_INFO("Startdata: result =%d " ,result);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop a data session.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnData::Stopdata(uint8_t phoneId, uint32_t profileId)
{
    taf_dcs_ProfileRef_t profileRef = NULL;

    profileRef = taf_dcs_GetProfileEx (phoneId, profileId);

    return taf_dcs_StopSession(profileRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get connection information for specified connection context.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnData::GetConnectionInfo(taf_mngd_Conn_Ctx_t* connCtxPtr)
{
    le_result_t result;
    taf_dcs_ProfileRef_t profileRef = NULL;
    bool isIpv4 = false, isIpv6 = false;

    TAF_ERROR_IF_RET_VAL(connCtxPtr == NULL, LE_BAD_PARAMETER, "Null ptr(connCtxPtr)");
    profileRef = taf_dcs_GetProfileEx (connCtxPtr->phoneId, connCtxPtr->profileNumber);

    result = taf_dcs_GetInterfaceName(profileRef, connCtxPtr->intfName, TAF_DCS_NAME_MAX_LEN);
    if(result != LE_OK)
    {
        LE_ERROR("Getting interface name failed for dataID %d",connCtxPtr->dataId);
        return result;
    }

    isIpv4 = taf_dcs_IsIPv4(profileRef);
    isIpv6 = taf_dcs_IsIPv6(profileRef);

    if(isIpv4 && !isIpv6)         //Ipv4 only
    {
        connCtxPtr->ipType = TAF_DCS_PDP_IPV4;
        result=taf_dcs_GetIPv4Address(profileRef, connCtxPtr->ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN);

        if(result != LE_OK)
        {
            LE_ERROR("Getting IPV4 address failed for dataID %d ",connCtxPtr->dataId);
            return result;
        }
    }
    else if(!isIpv4 && isIpv6)    //Ipv6 only
    {
        connCtxPtr->ipType = TAF_DCS_PDP_IPV6;
        result=taf_dcs_GetIPv6Address(profileRef, connCtxPtr->ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN);

        if(result != LE_OK)
        {
            LE_ERROR("Getting IPV6 address failed for dataID %d ",connCtxPtr->dataId);
            return result;
        }
    }
    else if(isIpv4 && isIpv6)     //Ipv4v6
    {
        connCtxPtr->ipType = TAF_DCS_PDP_IPV4V6;
        result=taf_dcs_GetIPv4Address(profileRef, connCtxPtr->ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN);

        if(result != LE_OK)
        {
            LE_ERROR("Getting IPV4 address failed for dataID %d ",connCtxPtr->dataId);
            return result;
        }

        result=taf_dcs_GetIPv6Address(profileRef, connCtxPtr->ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN);

        if(result != LE_OK)
        {
            LE_ERROR("Getting IPV6 address failed for dataID %d ",connCtxPtr->dataId);
            return result;
        }
    }
    else
    {
        connCtxPtr->ipType = TAF_DCS_PDP_UNKNOWN;
        result = LE_FAULT;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the connection information for the specified list.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafMngdConnData::GetAllConnectionInfo(profileInfo_t *profileNumberList, int listSize)
{
    bool isIpv4 = false, isIpv6 = false;
    taf_dcs_ProfileRef_t profileRef = NULL;
    taf_mngd_Conn_Ctx_t* connCtxPtr = NULL;
    le_result_t result;

    auto &mngdConnAdmin = tafMngdConnAdmin::GetInstance();

    TAF_ERROR_IF_RET_VAL(profileNumberList == NULL, LE_BAD_PARAMETER,
                        "Null ptr(profileNumberList)");

    for(int i = 0; i < listSize; i++)
    {
        connCtxPtr = mngdConnAdmin.GetConnCtx(profileNumberList[i].phoneId,
                                              profileNumberList[i].profileNumber);

        if(connCtxPtr == NULL)
            continue;

        if(connCtxPtr->dataState != TAF_MNGD_CONN_DATA_CONNECTED)
            continue;

        profileRef = taf_dcs_GetProfileEx (connCtxPtr->phoneId, connCtxPtr->profileNumber);

        result = taf_dcs_GetInterfaceName(profileRef, connCtxPtr->intfName, TAF_DCS_NAME_MAX_LEN);
        if(result != LE_OK)
        {
            LE_ERROR("Getting interface name failed");
            continue;
        }

        isIpv4 = taf_dcs_IsIPv4(profileRef);
        isIpv6 = taf_dcs_IsIPv6(profileRef);

        if(isIpv4 && !isIpv6)
        {
            connCtxPtr->ipType = TAF_DCS_PDP_IPV4;
            result=taf_dcs_GetIPv4Address(profileRef, connCtxPtr->ipv4Addr,
                                          TAF_DCS_IPV4_ADDR_MAX_LEN);

            if(result != LE_OK)
            {
                LE_ERROR("Getting IPV4 address failed");
                continue;
            }
        }
        else if(!isIpv4 && isIpv6)
        {
            connCtxPtr->ipType = TAF_DCS_PDP_IPV6;
            result=taf_dcs_GetIPv6Address(profileRef, connCtxPtr->ipv6Addr,
                                          TAF_DCS_IPV6_ADDR_MAX_LEN);

            if(result != LE_OK)
            {
                LE_ERROR("Getting IPV6 address failed");
                continue;
            }
        }
        else if(isIpv4 && isIpv6)
        {
            connCtxPtr->ipType = TAF_DCS_PDP_IPV4V6;
            result=taf_dcs_GetIPv4Address(profileRef, connCtxPtr->ipv4Addr,
                                          TAF_DCS_IPV4_ADDR_MAX_LEN);

            if(result != LE_OK)
            {
                LE_ERROR("Getting IPV4 address failed");
                continue;
            }

            result=taf_dcs_GetIPv6Address(profileRef, connCtxPtr->ipv6Addr,
                                          TAF_DCS_IPV6_ADDR_MAX_LEN);

            if(result != LE_OK)
            {
                LE_ERROR("Getting IPV6 address failed");
                continue;
            }
        }
        else
        {
            connCtxPtr->ipType = TAF_DCS_PDP_UNKNOWN;
        }
    }

    if(listSize <= 0)
    {
        LE_ERROR("No context");
        return LE_FAULT;
    }

    return LE_OK;
}