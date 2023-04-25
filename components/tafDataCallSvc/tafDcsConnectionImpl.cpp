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
 * @file       tafDcsConnectionImpl.cpp
 * @brief      This file provides the implementation of taf data connection component.
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

LE_MEM_DEFINE_STATIC_POOL(tafDataCall, TAF_DCS_MAX_CALL_OBJ, sizeof(taf_dcs_CallCtx_t));
LE_MEM_DEFINE_STATIC_POOL(tafSessionRef, TAF_DCS_MAX_SESSION_REF, sizeof(taf_SessionRef_t));
LE_MEM_DEFINE_STATIC_POOL(HandlerSessionMappingPool,
                                    TAF_DCS_MAX_ASYNC_HANDLER_MAPPING,
                                    sizeof(HandlerSessionMapping_t));
LE_MEM_DEFINE_STATIC_POOL(RoamingStatusPool, TAF_DCS_MAX_SESSION_REF,
                          sizeof(taf_dcs_RoamingStatusInd_t));


#ifdef TARGET_SA515M
taf_DataConnServingSystemListener::taf_DataConnServingSystemListener(SlotId slot) : slotId(slot) {}

void taf_DataConnServingSystemListener::onServiceStateChanged(telux::data::ServiceStatus status)
{
    std::lock_guard<std::mutex> lock(cv_mutex);
    LE_DEBUG("<SDK Listener> taf_DataConnServingSystemListener --> onServiceStateChanged");

    dsStatus = status.serviceState;
    LE_DEBUG("status = %d", (int)dsStatus);
    if (dsStatus == telux::data::DataServiceState::IN_SERVICE) {
        conVar.notify_all();
    }
}

void taf_DataConnServingSystemListener::onRoamingStatusChanged(telux::data::RoamingStatus status)
{
    le_result_t result = LE_OK;
    taf_dcs_RoamingStatusInd_t *reportPtr = NULL;
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();
    reportPtr = (taf_dcs_RoamingStatusInd_t*)le_mem_ForceAlloc(dataConnection.RoamingStatusPool);

    result = dataProfile.getPhoneIdFromSlotId(slotId, &(reportPtr->phoneId) );
    if(result != LE_OK)
    {
        LE_ERROR("Failed to get phone id from slot id");
        reportPtr->phoneId = SLOT_ID_1;
    }

    reportPtr->isRoaming = status.isRoaming;
    reportPtr->type = (taf_dcs_RoamingType_t)status.type;

    le_event_ReportWithRefCounting(dataConnection.RoamingStatusEvtId, (void*)reportPtr);

}

void taf_DataConnRequestRoamingStatusCallback::requestRoamingStatus
(
    telux::data::RoamingStatus roamingStatus,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> taf_DataConnRequestRoamingStatusCallback --> requestRoamingStatus");

    errorCode=error;
    status = roamingStatus;
    le_sem_Post(semaphore);
}

#endif
void taf_DataConnectionListener::onDataCallInfoChanged
(
    const std::shared_ptr<telux::data::IDataCall> &iCall
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    TAF_ERROR_IF_RET_NIL(iCall == nullptr, "iCall is null");
    dataConnection.LogDataCallInfo(iCall, __func__);

    dataCallEvent_t callEvent;
    int32_t profileId = iCall->getProfileId();
    uint8_t slotId = (uint8_t)iCall->getSlotId();
    telux::data::DataCallStatus callStatus = iCall->getDataCallStatus();

    taf_dcs_CallCtx_t* callCtxPtr = dataConnection.GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_NIL(callCtxPtr == NULL,
                         "Cannot find call context from slotId(%d) profileId(%d), event(%s)",
                          slotId, profileId, dataConnection.CallStatusToString(callStatus));

    callEvent.event         = EVT_STATUS_CHANGED;
    callEvent.profileId     = profileId;
    callEvent.slotId        = slotId;
    callEvent.callStatus    = callStatus;
    callEvent.ipType        = iCall->getIpFamilyType();
    callEvent.ipv4Status    = iCall->getIpv4Info().status;
    if (callEvent.ipv4Status == telux::data::DataCallStatus::NET_CONNECTED)
    {
        callEvent.ipv4AddrInfo  = iCall->getIpv4Info().addr;
    }
    callEvent.ipv6Status        = iCall->getIpv6Info().status;
    if (callEvent.ipv6Status == telux::data::DataCallStatus::NET_CONNECTED)
    {
        callEvent.ipv6AddrInfo  = iCall->getIpv6Info().addr;
    }
    callEvent.ifName            = iCall->getInterfaceName();
    callEvent.dataBearerTech    = iCall->getCurrentBearerTech();

    le_event_Report(dataConnection.CallEvent, &callEvent, sizeof(dataCallEvent_t));
};

const char* taf_DataConnection::CallStatusToString(telux::data::DataCallStatus status)
{
    const char *statusPtr = "";

    switch (status)
    {
        case telux::data::DataCallStatus::INVALID:
            statusPtr = "INVALID";
        break;
        case telux::data::DataCallStatus::NET_CONNECTED:
            statusPtr = "NET_CONNECTED";
        break;
        case telux::data::DataCallStatus::NET_NO_NET:
            statusPtr = "NET_NO_NET";
        break;
        case telux::data::DataCallStatus::NET_IDLE:
            statusPtr = "NET_IDLE";
        break;
        case telux::data::DataCallStatus::NET_CONNECTING:
            statusPtr = "NET_CONNECTING";
        break;
        case telux::data::DataCallStatus::NET_DISCONNECTING:
            statusPtr = "NET_DISCONNECTING";
        break;
        case telux::data::DataCallStatus::NET_RECONFIGURED:
            statusPtr = "NET_RECONFIGURED";
        break;
        case telux::data::DataCallStatus::NET_NEWADDR:
            statusPtr = "NET_NEWADDR";
        break;
        case telux::data::DataCallStatus::NET_DELADDR:
            statusPtr = "NET_DELADDR";
        break;
        default:
            LE_ERROR("call(%d) status error", (int32_t)status);
            statusPtr = "";
        break;
    }

    return statusPtr;
}

const char*  taf_DataConnection::CallEndReasonToString(EndReasonType type)
{
    const char *reasonPtr = "";

    switch (type) {
        case EndReasonType::CE_MOBILE_IP:
            reasonPtr = "CE_MOBILE_IP";
        break;
        case EndReasonType::CE_INTERNAL:
            reasonPtr =  "CE_INTERNAL";
        break;
        case EndReasonType::CE_CALL_MANAGER_DEFINED:
            reasonPtr =  "CE_CALL_MANAGER_DEFINED";
        break;
        case EndReasonType::CE_3GPP_SPEC_DEFINED:
            reasonPtr =  "CE_3GPP_SPEC_DEFINED";
        break;
        case EndReasonType::CE_PPP:
            reasonPtr =  "CE_PPP";
        break;
        case EndReasonType::CE_EHRPD:
            reasonPtr =  "CE_EHRPD";
        break;
        case EndReasonType::CE_IPV6:
            reasonPtr =  "CE_IPV6";
        break;
        case EndReasonType::CE_UNKNOWN:
            reasonPtr =  "CE_UNKNOWN";
        break;
        default:
            LE_ERROR("end reason(%d) error", (int32_t)type);
            reasonPtr = "";
        break;
    }

    return reasonPtr;
}

const char* taf_DataConnection::IpFamilyTypeToString(telux::data::IpFamilyType ipType)
{
    const char *ipPtr = "";

    switch(ipType) {
        case telux::data::IpFamilyType::IPV4:
            ipPtr = "IPv4";
        break;
        case telux::data::IpFamilyType::IPV6:
            ipPtr = "IPv6";
        break;
        case telux::data::IpFamilyType::IPV4V6:
            ipPtr = "IPv4v6";
        break;
        case telux::data::IpFamilyType::UNKNOWN:
        default:
            LE_ERROR("unknown ip(%d)", (int32_t)ipType);
            ipPtr = "";
        break;
    }

    return ipPtr;
}

const char* taf_DataConnection::TechPreferenceToString(telux::data::TechPreference techPref)
{
    const char *techPrefPtr = "";

    switch(techPref) {
        case telux::data::TechPreference::TP_3GPP:
            techPrefPtr = "3GPP";
        break;
        case telux::data::TechPreference::TP_3GPP2:
            techPrefPtr = "3GPP2";
        break;
        case telux::data::TechPreference::TP_ANY:
            techPrefPtr = "TP_ANY";
        break;
        case telux::data::TechPreference::UNKNOWN:
        default:
            LE_ERROR("unknown tech preference(%d)", (int32_t)techPref);
            techPrefPtr = "";
        break;
    }

    return techPrefPtr;
}

const char* taf_DataConnection::DataBearerToString(telux::data::DataBearerTechnology dataBearer)
{
    const char *dataBearerPtr = "";

    switch(dataBearer) {
        case telux::data::DataBearerTechnology::CDMA_1X:
            dataBearerPtr = "1X technology";
        break;
        case telux::data::DataBearerTechnology::EVDO_REV0:
            dataBearerPtr = "CDMA Rev 0";
        break;
        case telux::data::DataBearerTechnology::EVDO_REVA:
            dataBearerPtr = "CDMA Rev A";
        break;
        case telux::data::DataBearerTechnology::EVDO_REVB:
            dataBearerPtr = "CDMA Rev B";
        break;
        case telux::data::DataBearerTechnology::EHRPD:
            dataBearerPtr = "EHRPD";
        break;
        case telux::data::DataBearerTechnology::FMC:
            dataBearerPtr = "Fixed mobile convergence";
        break;
        case telux::data::DataBearerTechnology::HRPD:
            dataBearerPtr = "HRPD";
        break;
        case telux::data::DataBearerTechnology::BEARER_TECH_3GPP2_WLAN:
            dataBearerPtr = "3GPP2 IWLAN";
        break;
        case telux::data::DataBearerTechnology::WCDMA:
            dataBearerPtr = "WCDMA";
        break;
        case telux::data::DataBearerTechnology::GPRS:
            dataBearerPtr = "GPRS";
        break;
        case telux::data::DataBearerTechnology::HSDPA:
            dataBearerPtr = "HSDPA";
        break;
        case telux::data::DataBearerTechnology::HSUPA:
            dataBearerPtr = "HSUPA";
        break;
        case telux::data::DataBearerTechnology::EDGE:
            dataBearerPtr = "EDGE";
        break;
        case telux::data::DataBearerTechnology::LTE:
            dataBearerPtr = "LTE";
        break;
        case telux::data::DataBearerTechnology::HSDPA_PLUS:
            dataBearerPtr = "HSDPA+";
        break;
        case telux::data::DataBearerTechnology::DC_HSDPA_PLUS:
            dataBearerPtr = "DC HSDPA+.";
        break;
        case telux::data::DataBearerTechnology::HSPA:
            dataBearerPtr = "HSPA";
        break;
        case telux::data::DataBearerTechnology::BEARER_TECH_64_QAM:
            dataBearerPtr = "64 QAM";
        break;
        case telux::data::DataBearerTechnology::TDSCDMA:
            dataBearerPtr = "TDSCDMA";
        break;
        case telux::data::DataBearerTechnology::GSM:
            dataBearerPtr = "GSM";
        break;
        case telux::data::DataBearerTechnology::BEARER_TECH_3GPP_WLAN:
            dataBearerPtr = "3GPP WLAN";
        break;
        case telux::data::DataBearerTechnology::BEARER_TECH_5G:
            dataBearerPtr = "5G";
        break;
        default:
            LE_ERROR("unknown data bearer type(%d)", (int32_t)dataBearer);
            dataBearerPtr = "UNKNOWN";
        break;
    }

    return dataBearerPtr;
}

void taf_DataConnection::LogDataCallInfo
(
    const std::shared_ptr<telux::data::IDataCall> &dataCall,
     const char *fromPtr
)
{
    int32_t profileId = dataCall->getProfileId();
    uint8_t slotId = (uint8_t)dataCall->getSlotId();

    LE_DEBUG("data callback details from: %s", fromPtr);
    LE_DEBUG("profile id:           %d", profileId);
    LE_DEBUG("slot id:           %d", slotId);
    LE_DEBUG("interface name:       %s", dataCall->getInterfaceName().c_str());
    LE_DEBUG("call status:          %s", CallStatusToString(dataCall->getDataCallStatus()));
    LE_DEBUG("ip type:              %s", IpFamilyTypeToString(dataCall->getIpFamilyType()));
    LE_DEBUG("ipv4 status:          %s", CallStatusToString(dataCall->getIpv4Info().status));
    LE_DEBUG("ipv6 status:          %s", CallStatusToString(dataCall->getIpv6Info().status));
    std::list<telux::data::IpAddrInfo> ipAddrList = dataCall->getIpAddressInfo();
    for(auto &it : ipAddrList) {
        LE_DEBUG("interface addr:       %s", it.ifAddress.c_str());
        LE_DEBUG("gateway   addr:       %s", it.gwAddress.c_str());
        LE_DEBUG("primary dns addr:     %s", it.primaryDnsAddress.c_str());
        LE_DEBUG("secondary dns addr:   %s", it.secondaryDnsAddress.c_str());
    }
    LE_DEBUG("call end reason:   %s", CallEndReasonToString(dataCall->getDataCallEndReason().type));
    LE_DEBUG("tech preference:      %s", TechPreferenceToString(dataCall->getTechPreference()));
    LE_DEBUG("DataBearerTechnology: %s", DataBearerToString(dataCall->getCurrentBearerTech()));

    return;
}

void taf_DataConnection::StartDataCallCallback
(
    const std::shared_ptr<telux::data::IDataCall> &iCall,
    telux::common::ErrorCode errorCode
)
{
    dataCallEvent_t callEvent;
    auto &dataConnection = taf_DataConnection::GetInstance();
    //Check if iCall is a null pointer to avoid crashing
    TAF_ERROR_IF_RET_NIL(iCall == NULL, "iCall is NULL, drop this event");
    int32_t profileId = iCall->getProfileId();
    uint8_t slotId = (uint8_t)iCall->getSlotId();

    dataConnection.LogDataCallInfo(iCall, __func__);

    if (errorCode != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Starting data session is failed with slotId(%d) profileId(%d), error code: %d",
                  slotId, profileId, (uint32_t)errorCode);
    }

    callEvent.event         = EVT_START_CALLBACK;
    callEvent.errorCode     = errorCode;

    // In SA415M with old telsdk version, when call telsdk startDataCall function,
    // the callback handler will return errorCode with SUCCESS value and
    // callStatus with INVALID value,not CONNECTING value
#ifdef TARGET_SA415M
    if(errorCode == telux::common::ErrorCode::SUCCESS)
        callEvent.callStatus = telux::data::DataCallStatus::NET_CONNECTING;
    else
        callEvent.callStatus    = iCall->getDataCallStatus();
#else
    callEvent.callStatus    = iCall->getDataCallStatus();
#endif

    callEvent.profileId     = iCall->getProfileId();
    callEvent.slotId        = slotId;
    callEvent.ipType        = iCall->getIpFamilyType();
    callEvent.ipv4Status    = iCall->getIpv4Info().status;
    callEvent.ipv6Status    = iCall->getIpv6Info().status;
    LE_DEBUG("Start callback:event=%d,errcode=%d, callstatus=%s, slotId=%d, profileId=%d, ipType=%d",
             (int)callEvent.event, (int)callEvent.errorCode,
             dataConnection.CallStatusToString(callEvent.callStatus), callEvent.slotId,
             (int)callEvent.profileId,(int)callEvent.ipType);
    le_event_Report(dataConnection.CallEvent, &callEvent,sizeof(dataCallEvent_t));

    return;
}

void taf_DataConnection::StopDataCallCallback
(
    const std::shared_ptr<telux::data::IDataCall> &iCall,
    telux::common::ErrorCode errorCode
)
{
    dataCallEvent_t callEvent;
    auto &dataConnection = taf_DataConnection::GetInstance();
    //Check if iCall is a null pointer to avoid crashing
    TAF_ERROR_IF_RET_NIL(iCall == NULL, "iCall is NULL, drop this event");
    int32_t profileId = iCall->getProfileId();
    uint8_t slotId = (uint8_t)iCall->getSlotId();

    dataConnection.LogDataCallInfo(iCall, __func__);

    if (errorCode != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Stopping data call is failed with slotId(%d) profileId(%d), error code: %d",
                  slotId, profileId, (uint32_t)errorCode);
    }

    LE_INFO("Receiving StopDataCallCallback!");
    callEvent.event         = EVT_STOP_CALLBACK;
    callEvent.errorCode     = errorCode;
    callEvent.callStatus    = iCall->getDataCallStatus();
    callEvent.profileId     = iCall->getProfileId();
    callEvent.slotId        = slotId;
    callEvent.ipType        = iCall->getIpFamilyType();
    callEvent.ipv4Status    = iCall->getIpv4Info().status;
    callEvent.ipv6Status    = iCall->getIpv6Info().status;
    LE_DEBUG("stop callback:event=%d, errcode=%d, callstatus=%s, slotId=%d, profileId=%d, ipType=%d",
             (int)callEvent.event,(int)callEvent.errorCode,
            dataConnection.CallStatusToString(callEvent.callStatus), callEvent.slotId,
            (int)callEvent.profileId,(int)callEvent.ipType);

    le_event_Report(dataConnection.CallEvent, &callEvent,sizeof(dataCallEvent_t));
    return;
}

void taf_DataConnection::SetDefaultProfileCallCallback(telux::common::ErrorCode errorCode)
{
    dataCallEvent_t callEvent;
    auto &dataConnection = taf_DataConnection::GetInstance();

    if (errorCode != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Setting default id is not finished, error code: %d", (uint32_t)errorCode);
    }

    callEvent.event         = EVT_SET_DEFAULT_PROFILE;
    callEvent.errorCode     = errorCode;
    le_event_Report(dataConnection.CallEvent, &callEvent,sizeof(dataCallEvent_t));

    return;
}

void taf_DataConnection::GetDefaultProfileCallCallback
(
    int profileId, SlotId slotId,
    telux::common::ErrorCode errorCode
)
{
    dataCallEvent_t callEvent;
    auto &dataConnection = taf_DataConnection::GetInstance();

    if (errorCode != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Getting default id is failed, error code: %d", (uint32_t)errorCode);
    }

    callEvent.event         = EVT_GET_DEFAULT_PROFILE;
    callEvent.errorCode     = errorCode;
    callEvent.profileId     = profileId;
    callEvent.slotId     = (uint8_t)slotId;
    le_event_Report(dataConnection.CallEvent, &callEvent,sizeof(dataCallEvent_t));

    return;
}

taf_dcs_CallCtx_t* taf_DataConnection::CreateDataCallCtx(uint8_t slotId, int32_t profileId)
{
    taf_dcs_CallCtx_t* callCtxPtr = NULL;
    char name[16] = {0};

    callCtxPtr = (taf_dcs_CallCtx_t*)le_mem_ForceAlloc(DataCallCtxPool);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, NULL, "cannot alloc callCtr");

    callCtxPtr->funcType = CALL_FUNCTION_UNKNOWN;
    callCtxPtr->isCallActionInProgress = false;
    callCtxPtr->callActionMutex = PTHREAD_MUTEX_INITIALIZER;
    callCtxPtr->callActionCond = PTHREAD_COND_INITIALIZER;
    callCtxPtr->sessionListMutex = PTHREAD_MUTEX_INITIALIZER;
    callCtxPtr->ipv4Status = telux::data::DataCallStatus::INVALID;
    callCtxPtr->ipv6Status = telux::data::DataCallStatus::INVALID;
    callCtxPtr->ipType = telux::data::IpFamilyType::UNKNOWN;
    callCtxPtr->profileId = profileId;
    callCtxPtr->slotId = slotId;
    callCtxPtr->sessionRefList = LE_DLS_LIST_INIT;
    callCtxPtr->link = LE_DLS_LINK_INIT;
    memset(callCtxPtr->intfName, 0, sizeof(callCtxPtr->intfName));
    le_mutex_Lock(callCtxMutex);
    le_dls_Queue(&DataCallCtxList, &callCtxPtr->link);
    le_mutex_Unlock(callCtxMutex);
    callCtxPtr->callRef = (taf_dcs_CallRef_t)le_ref_CreateRef(DataCallRefMap, (void *)callCtxPtr);
    TAF_ERROR_IF_RET_VAL(callCtxPtr->callRef == NULL, NULL, "cannot alloc call reference");

    snprintf(name, sizeof(name)-1, "callCtx-%d-%d", slotId, profileId);
    callCtxPtr->sessionStateEvent = le_event_CreateId(name, sizeof(DataCallState_t));

    return callCtxPtr;
}

taf_dcs_CallCtx_t* taf_DataConnection::GetCallCtx(uint8_t slotId, int32_t profileId)
{
    le_dls_Link_t* linkPtr = NULL;

    le_mutex_Lock(callCtxMutex);
    linkPtr = le_dls_Peek(&DataCallCtxList);
    while (linkPtr)
    {
        taf_dcs_CallCtx_t* callCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_CallCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCallCtxList, linkPtr);
        if (callCtxPtr->slotId == slotId && callCtxPtr->profileId == profileId)
        {
            le_mutex_Unlock(callCtxMutex);
            return callCtxPtr;
        }
    }

    le_mutex_Unlock(callCtxMutex);
    return NULL;
}

taf_dcs_CallCtx_t* taf_DataConnection::GetCallCtx(taf_dcs_CallRef_t reference)
{
    taf_dcs_CallCtx_t* callCtxPtr = (taf_dcs_CallCtx_t* )le_ref_Lookup(DataCallRefMap,
                                                                       (void*)reference);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, NULL, "cannot get callCtx from ref(%p)", reference);

    return callCtxPtr;
}

le_result_t taf_DataConnection::GetSlotIdAndProfileId
(
    taf_dcs_CallRef_t reference,
    uint8_t *slotId,
    int32_t *profileId
)
{
    TAF_ERROR_IF_RET_VAL(reference == NULL || slotId == NULL || profileId == NULL, LE_BAD_PARAMETER,
                         "Null pointer");
    taf_dcs_CallCtx_t* callCtxPtr = GetCallCtx(reference);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND,
                         "Cannot get callCtx from ref(%p)", reference);

    *slotId = callCtxPtr->slotId;
    *profileId = callCtxPtr->profileId;
    return LE_OK;
}

le_result_t taf_DataConnection::GetConnectionState
(
    uint8_t slotId,
    int32_t profileId,
    taf_dcs_ConState_t* statePtr
)
{
    TAF_ERROR_IF_RET_VAL(statePtr == NULL, LE_NOT_FOUND, "statePtr is NULL");

    taf_dcs_CallCtx_t* callCtxPtr = GetCallCtx(slotId, profileId);
    if (callCtxPtr == NULL)
    {
        LE_WARN("Cannot find call context slotId(%d) profileId(%d), set to DISCONNECTED",
                 slotId, profileId);
        *statePtr = TAF_DCS_DISCONNECTED;
        return LE_OK;
    }

    *statePtr = callCtxPtr->latestConState;
    return LE_OK;
}

le_result_t taf_DataConnection::SendSettingDefaultProfileIdCmd(uint8_t slotId, int32_t profileId)
{

    if(dataConnectionManagers.find((SlotId)slotId) == dataConnectionManagers.end())
    {
        LE_ERROR("Connection manager is not init for slot %d", slotId);
        return LE_FAULT;
    }

    telux::common::Status status =
                          dataConnectionManagers[static_cast<SlotId>(slotId)]->setDefaultProfile(
                                                            telux::data::OperationType::DATA_LOCAL,
                                                            profileId,
                                                            SetDefaultProfileCallCallback);

    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT,
                         "Setting profile is failed, ret: %d", (int32_t)status);

    return LE_OK;
}

le_result_t taf_DataConnection::SendGettingDefaultProfileIdCmd()
{
#ifdef TARGET_SA515M

    if(dataConnectionManagers.find((SlotId)SLOT_ID_1) == dataConnectionManagers.end())
    {
        LE_ERROR("Connection manager is not init for slot %d", SLOT_ID_1);
        return LE_FAULT;
    }

    telux::common::Status status =
                          dataConnectionManagers[static_cast<SlotId>(SLOT_ID_1)]->getDefaultProfile(
                                                            telux::data::OperationType::DATA_LOCAL,
                                                            GetDefaultProfileCallCallback);

    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT,
                         "Getting default profile id failed, ret: %d", (int32_t)status);
#endif
    return LE_OK;
}

le_result_t taf_DataConnection::MakeCall
(
    uint8_t slotId,
    int32_t profileId,
    telux::data::IpFamilyType ipType
)
{
    telux::common::Status status;

    if(dataConnectionManagers.find((SlotId)slotId) == dataConnectionManagers.end())
    {
        LE_ERROR("Connection manager is not init for slot %d", slotId);
        return LE_FAULT;
    }

    status = dataConnectionManagers[static_cast<SlotId>(slotId)]->startDataCall(profileId, ipType,
                                                                            StartDataCallCallback);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT,
                         "Starting call failed, ret: %d", (int32_t)status);

    return LE_OK;
}

le_result_t taf_DataConnection::StopCall
(
    uint8_t slotId,
    int32_t profileId,
    telux::data::IpFamilyType ipType
)
{
    telux::common::Status status;

    if(dataConnectionManagers.find((SlotId)slotId) == dataConnectionManagers.end())
    {
        LE_ERROR("Connection manager is not init for slot %d", slotId);
        return LE_FAULT;
    }

    status = dataConnectionManagers[static_cast<SlotId>(slotId)]->stopDataCall(profileId, ipType,
                                                                           StopDataCallCallback);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT,
                         "Stopping call failed, ret: %d", (int32_t)status);

    return LE_OK;
}

bool taf_DataConnection::IsCallCtxCreated(uint8_t slotId, int32_t profileId)
{
    le_dls_Link_t* linkPtr = NULL;

    le_mutex_Lock(callCtxMutex);
    linkPtr = le_dls_Peek(&DataCallCtxList);
    while (linkPtr)
    {
        taf_dcs_CallCtx_t* callCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_CallCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCallCtxList, linkPtr);
        if (callCtxPtr->slotId == slotId && callCtxPtr->profileId == profileId)
        {
            le_mutex_Unlock(callCtxMutex);
            return true;
        }

    }

    le_mutex_Unlock(callCtxMutex);
    return false;
}

le_result_t taf_DataConnection::AddSessionToCallCtx
(
    taf_dcs_CallCtx_t* callCtxPtr,
    le_msg_SessionRef_t sessionRef
)
{
    le_dls_Link_t* linkPtr = NULL;

    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_BAD_PARAMETER, "this call context is NULL");

    pthread_mutex_lock(&callCtxPtr->sessionListMutex);
    linkPtr = le_dls_Peek(&(callCtxPtr->sessionRefList));
    while (linkPtr)
    {
        taf_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkPtr, taf_SessionRef_t, link);
        linkPtr = le_dls_PeekNext(&(callCtxPtr->sessionRefList), linkPtr);

        if (sessionRefPtr->sessionRef == sessionRef)
        {
            LE_DEBUG("Session(%p) has been added to callctx slotId(%d) profileId(%d)",
                     sessionRef, callCtxPtr->slotId, callCtxPtr->profileId);
            pthread_mutex_unlock(&callCtxPtr->sessionListMutex);
            return LE_DUPLICATE;
        }
    }

    taf_SessionRef_t* newSessionRefPtr = (taf_SessionRef_t *)le_mem_ForceAlloc(SessionRefPool);

    newSessionRefPtr->sessionRef = sessionRef;
    newSessionRefPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&callCtxPtr->sessionRefList, &(newSessionRefPtr->link));
    le_mem_AddRef(callCtxPtr);

    pthread_mutex_unlock(&callCtxPtr->sessionListMutex);
    return LE_OK;
}

le_result_t taf_DataConnection::RemoveSessionFromCallCtx
(
    taf_dcs_CallCtx_t* callCtxPtr,
    le_msg_SessionRef_t sessionRef
)
{
    le_dls_Link_t* linkPtr = NULL;

    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_BAD_PARAMETER, "this call context is NULL");

    pthread_mutex_lock(&callCtxPtr->sessionListMutex);
    linkPtr = le_dls_Peek(&(callCtxPtr->sessionRefList));
    while (linkPtr)
    {
        taf_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkPtr, taf_SessionRef_t, link);
        linkPtr = le_dls_PeekNext(&(callCtxPtr->sessionRefList), linkPtr);

        if (sessionRefPtr->sessionRef == sessionRef)
        {
            le_dls_Remove(&(callCtxPtr->sessionRefList), &(sessionRefPtr->link));
            le_mem_Release(sessionRefPtr);
            pthread_mutex_unlock(&callCtxPtr->sessionListMutex);
            return LE_OK;
        }
    }

    LE_ERROR("Cannot found session context with ref(%p) from slotId(%d) profileId(%d)",
             sessionRef, callCtxPtr->slotId, callCtxPtr->profileId);
    pthread_mutex_unlock(&callCtxPtr->sessionListMutex);
    return LE_NOT_FOUND;
}

le_result_t taf_DataConnection::StartSessionCallSync
(
    uint8_t slotId,
    int32_t profileId,
    taf_dcs_Pdp_t pdpType
)
{
    // initialize the synchronous promise
    CmdSynchronousPromise = std::promise<le_result_t>();

    telux::data::IpFamilyType ipType = telux::data::IpFamilyType::IPV4V6;
    if (pdpType == TAF_DCS_PDP_IPV4)
    {
        ipType = telux::data::IpFamilyType::IPV4;
    }
    else if (pdpType == TAF_DCS_PDP_IPV6)
    {
        ipType = telux::data::IpFamilyType::IPV6;
    }

    le_result_t result = MakeCall(slotId, profileId, ipType);

    if (result != LE_OK)
    {
        LE_ERROR("Starting session is failed");
        return result;
    }

    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();

    return result;
}

le_result_t taf_DataConnection::PreProcessDataCall
(
    uint8_t slotId,
    int32_t profileId,
    taf_dcs_CallCtx_t* callCtxPtr,
    taf_CallFuncType_t funcType,
    le_msg_SessionRef_t sessionRef
)
{
    le_result_t result = LE_OK;

    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "Cannot get call context");

    switch(funcType)
    {
        case CALL_FUNCTION_SYNC_START:
        case CALL_FUNCTION_ASYNC_START:
            pthread_mutex_lock(&callCtxPtr->callActionMutex);
            // An async call action(starting or stopping) is in progress
            if(callCtxPtr->isCallActionInProgress == true)
            {
                LE_INFO("SlotId(%d) profileId(%d) is in use and async data call is in progress",
                         slotId, profileId);
                pthread_mutex_unlock(&callCtxPtr->callActionMutex);
                return LE_IN_PROGRESS;
            }
            pthread_mutex_unlock(&callCtxPtr->callActionMutex);

            // add session into call ctx

            AddSessionToCallCtx(callCtxPtr, sessionRef);

            // Already connected, return LE_DUPLICATE
            if(callCtxPtr->latestConState == TAF_DCS_CONNECTED)
            {
                LE_INFO("SlotId(%d) profileId(%d) for data call is already connected",
                         slotId, profileId);
                return LE_DUPLICATE;
            }

        break;
        case CALL_FUNCTION_SYNC_STOP:
        case CALL_FUNCTION_ASYNC_STOP:
            pthread_mutex_lock(&callCtxPtr->callActionMutex);
            // An async call action(starting or stopping) is in progress
            if(callCtxPtr->isCallActionInProgress == true)
            {
                LE_INFO("SlotId(%d) profileId(%d) is in use and async data call is in progress",
                         slotId, profileId);
                pthread_mutex_unlock(&callCtxPtr->callActionMutex);
                return LE_IN_PROGRESS;
            }
            pthread_mutex_unlock(&callCtxPtr->callActionMutex);

            result = RemoveSessionFromCallCtx(callCtxPtr, sessionRef);
            if(result != LE_OK)
            {
                LE_INFO("Cannot remove session ref(%p) from this call ctx", sessionRef);
                return result;
            }

        break;

        default:
            return LE_FAULT;
    }

    return LE_OK;
}

le_result_t taf_DataConnection::StartSessionCmdSync
(
    uint8_t slotId,
    int32_t profileId,
    taf_dcs_Pdp_t pdpType,
    le_msg_SessionRef_t sessionRef
)
{
    bool  isCreated;
    le_result_t result = LE_OK;
    taf_dcs_CallCtx_t* callCtxPtr;
    EventSynchronousPromise = std::promise<le_result_t>();
    std::chrono::seconds span(SESSION_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(pdpType == TAF_DCS_PDP_UNKNOWN, LE_OUT_OF_RANGE, "pdpType is unknown");
    LE_INFO("Sync start sessionRef=%p, slotId(%d) profileId(%d)", sessionRef, slotId, profileId);
    isCreated = IsCallCtxCreated(slotId, profileId);

    if (isCreated == true)
    {
        callCtxPtr = GetCallCtx(slotId, profileId);
        TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "cannot get call context");

        result = PreProcessDataCall(slotId, profileId, callCtxPtr, CALL_FUNCTION_SYNC_START,
                                    sessionRef);
        if(result != LE_OK)
            return result;

        callCtxPtr->funcType = CALL_FUNCTION_SYNC_START;
    }
    else
    {
        callCtxPtr = CreateDataCallCtx(slotId, profileId);
        callCtxPtr->funcType = CALL_FUNCTION_SYNC_START;
        TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "cannot create call context");
        result = AddSessionToCallCtx(callCtxPtr, sessionRef);
        TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "addSessionToCallCtx return(%d) error",
                             result);
    }

    pthread_mutex_lock(&callCtxPtr->callActionMutex);
    callCtxPtr->isCallActionInProgress = true;
    pthread_mutex_unlock(&callCtxPtr->callActionMutex);

    result = StartSessionCallSync(slotId, profileId, pdpType);

    if (result != LE_OK)
    {
        LE_ERROR("Starting synchronous session cmd failed, result: %d, slotId(%d) profileId(%d)",
                  result, slotId, profileId);
        //if start session failed ,remove sessionRef from callCtxPtr
        pthread_mutex_lock(&callCtxPtr->callActionMutex);
        callCtxPtr->isCallActionInProgress = false;
        pthread_mutex_unlock(&callCtxPtr->callActionMutex);

        RemoveSessionFromCallCtx(callCtxPtr,sessionRef);
        return result;
    }

    // blocking here to get call event response
    std::future<le_result_t> futResult = EventSynchronousPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("waiting promise timeout for %d seconds", SESSION_TIMEOUT);
        LE_INFO("Err slotId(%d) profileId(%d) for Type[%s] IPv4[%s] IPv6[%s]", slotId, profileId,
                 IpFamilyTypeToString(callCtxPtr->ipType),
                 CallStatusToString(callCtxPtr->ipv4Status),
                 CallStatusToString(callCtxPtr->ipv6Status));
                 result = LE_TIMEOUT;
    }
    else
    {
        result = futResult.get();
    }

    pthread_mutex_lock(&callCtxPtr->callActionMutex);
    callCtxPtr->isCallActionInProgress = false;
    pthread_mutex_unlock(&callCtxPtr->callActionMutex);

    //If call status is NET_NO_NET(disconnected), telsdk will return a NULL pointer for the iCall
    //parameter of the StopDataCallCallback when invoke stopCall function.
    //Remove session since there is no need to call stopCall
    if(result == LE_OK)
    {
        callCtxPtr = GetCallCtx(slotId, profileId);
        if(callCtxPtr->callStatus == telux::data::DataCallStatus::NET_NO_NET)
        {
            LE_ERROR("callStatus is disconnected");
            RemoveSessionFromCallCtx(callCtxPtr, sessionRef);
            return LE_TERMINATED;
        }
    }
    LE_INFO("Sync starting session is done, result: %s", LE_RESULT_TXT(result));

    return result;
}

void taf_DataConnection::StartSessionCmdAsync
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
    void* contextPtr,
    le_msg_SessionRef_t sessionRef
)
{
    bool isCreated = false;
    int32_t profileId;
    uint8_t slotId;
    taf_dcs_CallCtx_t* callCtxPtr;
    le_result_t result = LE_OK;
    taf_dcs_Pdp_t pdpType;
    telux::data::IpFamilyType ipType = telux::data::IpFamilyType::IPV4V6;
    auto &dataProfile = taf_DataProfile::GetInstance();

    TAF_ERROR_IF_RET_NIL(handlerPtr == NULL, "Handler function is NULL");

    result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);
    // Check if profileRef is valid
    if (result != LE_OK)
    {
        LE_ERROR("profile reference(%p) is invalid", profileRef);
        handlerPtr(profileRef, LE_NOT_FOUND, contextPtr);
        return;
    }

    LE_INFO("Async starting sessionRef=%p, slotId(%d) profileId(%d)",
             sessionRef, slotId, profileId);

    pdpType = dataProfile.GetPdp(profileRef);

    // Check if pdpType is valid
    switch(pdpType)
    {
        case TAF_DCS_PDP_IPV4:
            ipType = telux::data::IpFamilyType::IPV4;
        break;
        case TAF_DCS_PDP_IPV6:
            ipType = telux::data::IpFamilyType::IPV6;
        break;
        case TAF_DCS_PDP_IPV4V6:
            ipType = telux::data::IpFamilyType::IPV4V6;
        break;
        default:
            LE_ERROR("pdpType type is unknown");
            handlerPtr(profileRef, LE_OUT_OF_RANGE, contextPtr);
            return;
    }

    isCreated = IsCallCtxCreated(slotId, profileId);

    if (isCreated == true)
    {
        callCtxPtr = GetCallCtx(slotId, profileId);
        // Check if callCtxPtr is valid
        if (callCtxPtr == NULL)
        {
            LE_ERROR("cannot get call context for slotId(%d) profile(%d)", slotId, profileId);
            handlerPtr(profileRef, LE_NOT_FOUND, contextPtr);
            return;
        }

        result = PreProcessDataCall(slotId, profileId, callCtxPtr, CALL_FUNCTION_ASYNC_START,
                                     sessionRef);
        if(result != LE_OK)
        {
            handlerPtr(profileRef, result, contextPtr);
            return;
        }

        callCtxPtr->funcType = CALL_FUNCTION_ASYNC_START;
    }
    else
    {
        callCtxPtr = CreateDataCallCtx(slotId, profileId);
        callCtxPtr->funcType = CALL_FUNCTION_ASYNC_START;
        // Check if call context can be created
        if (callCtxPtr == NULL)
        {
            LE_ERROR("cannot create call context");
            handlerPtr(profileRef, LE_FAULT, contextPtr);
            return;
        }

        result = AddSessionToCallCtx(callCtxPtr, sessionRef);
        // Check if session can be added into call context
        if (result != LE_OK)
        {
            LE_ERROR("addSessionToCallCtx return(%d) error", result);
            handlerPtr(profileRef, LE_FAULT, contextPtr);
            return;
        }

    }

    pthread_mutex_lock(&callCtxPtr->callActionMutex);
    callCtxPtr->isCallActionInProgress = true;
    pthread_mutex_unlock(&callCtxPtr->callActionMutex);

    AddHandlerSessionMapping(slotId, profileId, contextPtr, sessionRef, handlerPtr);

    result = MakeCall(slotId, profileId, ipType);

    if (result != LE_OK)
    {
        LE_ERROR("starting session failed");
        RemoveSessionFromCallCtx(callCtxPtr,sessionRef);
        DeleteHandlerInfo(slotId, profileId, handlerPtr);
        pthread_mutex_lock(&callCtxPtr->callActionMutex);
        callCtxPtr->isCallActionInProgress = false;
        pthread_mutex_unlock(&callCtxPtr->callActionMutex);

        handlerPtr(profileRef, result, contextPtr);
        return;
    }

}

le_result_t taf_DataConnection::StopSessionCallSync
(
    uint8_t slotId,
    int32_t profileId,
    taf_dcs_Pdp_t pdpType
)
{
    // initialize the synchronous promise
    CmdSynchronousPromise = std::promise<le_result_t>();
    telux::data::IpFamilyType ipType = telux::data::IpFamilyType::IPV4V6;

    if (pdpType == TAF_DCS_PDP_IPV4)
    {
        ipType = telux::data::IpFamilyType::IPV4;
    }
    else if (pdpType == TAF_DCS_PDP_IPV6)
    {
        ipType = telux::data::IpFamilyType::IPV6;
    }

    le_result_t result = StopCall(slotId, profileId, ipType);

    if (result != LE_OK)
    {
        LE_ERROR("stopping session command is failed, result: %d", result);
        return result;
    }

    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();

    return result;
}

le_result_t taf_DataConnection::StopSessionCmdSync
(
    uint8_t slotId,
    int32_t profileId,
    taf_dcs_Pdp_t pdpType,
    le_msg_SessionRef_t sessionRef
)
{
    // initialize the synchronous promise
    EventSynchronousPromise = std::promise<le_result_t>();
    std::chrono::seconds span(SESSION_TIMEOUT);
    taf_dcs_CallCtx_t* callCtxPtr;
    le_result_t result = LE_OK;
    LE_INFO("Sync stopping sessionRef=%p, slotId(%d) profileId(%d)", sessionRef, slotId, profileId);
    TAF_ERROR_IF_RET_VAL(pdpType == TAF_DCS_PDP_UNKNOWN, LE_OUT_OF_RANGE, "pdpType is unknown");

    callCtxPtr = GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "cannot get call context");

    result = PreProcessDataCall(slotId, profileId, callCtxPtr, CALL_FUNCTION_SYNC_STOP,
                                 sessionRef);
    if(result != LE_OK)
        return result;

    pthread_mutex_lock(&callCtxPtr->sessionListMutex);
    size_t numLinks = le_dls_NumLinks(&callCtxPtr->sessionRefList);
    pthread_mutex_unlock(&callCtxPtr->sessionListMutex);

    LE_INFO("numlink=%d", numLinks);

    if (numLinks > 0)
    {
        LE_INFO("slotId(%d) profile(%d) is used by (%d) clients, nothing to do in this operation",
                 slotId, profileId, numLinks);
        return LE_OK;
    }

    if(callCtxPtr->latestConState == TAF_DCS_DISCONNECTED)
    {
        LE_INFO("slotId(%d) profile(%d) for data call is already disconnected",slotId, profileId);
        return LE_OK;
    }

    pthread_mutex_lock(&callCtxPtr->callActionMutex);
    callCtxPtr->isCallActionInProgress = true;
    pthread_mutex_unlock(&callCtxPtr->callActionMutex);

    callCtxPtr->funcType = CALL_FUNCTION_SYNC_STOP;

    result = StopSessionCallSync(slotId, profileId, pdpType);
    if (result != LE_OK)
    {
        LE_ERROR("stopping session command failed, ret: %d", result);
        pthread_mutex_lock(&callCtxPtr->callActionMutex);
        callCtxPtr->isCallActionInProgress = false;
        pthread_mutex_unlock(&callCtxPtr->callActionMutex);
        // If stopping session failed, add the session into call ctx.
        AddSessionToCallCtx(callCtxPtr, sessionRef);

        return result;
    }

    // blocking here to get response
    std::future<le_result_t> futResult = EventSynchronousPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("waiting promise timeout");
        //if start session failed ,remove sessionRef from callCtxPtr
        result = LE_TIMEOUT;
    }
    else
    {
        result = futResult.get();
    }

    pthread_mutex_lock(&callCtxPtr->callActionMutex);
    callCtxPtr->isCallActionInProgress = false;
    pthread_mutex_unlock(&callCtxPtr->callActionMutex);

    LE_INFO("Sync stopping session is done, result: %s", LE_RESULT_TXT(result));

    return result;
}

void taf_DataConnection::StopSessionCmdAsync
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
    void* contextPtr,
    le_msg_SessionRef_t sessionRef
)
{
    int32_t profileId;
    uint8_t slotId;
    taf_dcs_CallCtx_t* callCtxPtr;
    le_result_t result = LE_OK;
    taf_dcs_Pdp_t pdpType;
    telux::data::IpFamilyType ipType = telux::data::IpFamilyType::IPV4V6;
        pid_t pid;
    uid_t uid;

   result = le_msg_GetClientUserCreds(sessionRef, &uid, &pid);
   LE_INFO("result =%d, uid=%d,pid=%d",result, (int)uid, (int)pid);
    auto &dataProfile = taf_DataProfile::GetInstance();

    TAF_ERROR_IF_RET_NIL(handlerPtr == NULL, "Handler function is NULL");

    result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);
    // Check if profileRef is valid
    if (result != LE_OK)
    {
        LE_ERROR("profile reference(%p) is invalid", profileRef);
        handlerPtr(profileRef, LE_NOT_FOUND, contextPtr);
        return;
    }

    LE_INFO("Async stopping sessionRef=%p, slotId(%d) profileId(%d)",
             sessionRef, slotId, profileId);

    pdpType = dataProfile.GetPdp(profileRef);

    // Check if pdpType is valid
    switch(pdpType)
    {
        case TAF_DCS_PDP_IPV4:
            ipType = telux::data::IpFamilyType::IPV4;
        break;
        case TAF_DCS_PDP_IPV6:
            ipType = telux::data::IpFamilyType::IPV6;
        break;
        case TAF_DCS_PDP_IPV4V6:
            ipType = telux::data::IpFamilyType::IPV4V6;
        break;
        default:
            LE_ERROR("pdpType type is unknown");
            handlerPtr(profileRef, LE_OUT_OF_RANGE, contextPtr);
            return;
    }

    callCtxPtr = GetCallCtx(slotId, profileId);
    if (callCtxPtr == NULL)
    {
        LE_ERROR("cannot get call context for slotId(%d) profileId(%d)", slotId, profileId);
        handlerPtr(profileRef, LE_NOT_FOUND, contextPtr);
        return;
    }

    result = PreProcessDataCall(slotId, profileId, callCtxPtr, CALL_FUNCTION_ASYNC_STOP,
                                 sessionRef);
    if(result != LE_OK)
    {
        handlerPtr(profileRef, result, contextPtr);
        return;
    }

    pthread_mutex_lock(&callCtxPtr->sessionListMutex);
    size_t numLinks = le_dls_NumLinks(&callCtxPtr->sessionRefList);
    pthread_mutex_unlock(&callCtxPtr->sessionListMutex);

    LE_INFO("numlink=%d",numLinks);

    // Check if more than one session uses this data connection
    if (numLinks > 0)
    {
        LE_INFO("slotId(%d) profile(%d) is used by (%d) clients, nothing to do in this operation",
                 slotId, profileId, numLinks);
        handlerPtr(profileRef, LE_OK, contextPtr);
        return;
    }

    if(callCtxPtr->latestConState == TAF_DCS_DISCONNECTED)
    {
        LE_INFO("slotId(%d) profile(%d) for data call is already disconnected", slotId, profileId);
        handlerPtr(profileRef, LE_OK, contextPtr);
        return;
    }

    pthread_mutex_lock(&callCtxPtr->callActionMutex);
    callCtxPtr->isCallActionInProgress = true;
    pthread_mutex_unlock(&callCtxPtr->callActionMutex);

    callCtxPtr->funcType = CALL_FUNCTION_ASYNC_STOP;

    AddHandlerSessionMapping(slotId, profileId, contextPtr, sessionRef, handlerPtr);

    result = StopCall(slotId, profileId, ipType);

    if (result != LE_OK)
    {
        LE_ERROR("stopping session failed");
        AddSessionToCallCtx(callCtxPtr, sessionRef);
        DeleteHandlerInfo(slotId, profileId, handlerPtr);
        pthread_mutex_lock(&callCtxPtr->callActionMutex);
        callCtxPtr->isCallActionInProgress = false;
        pthread_mutex_unlock(&callCtxPtr->callActionMutex);

        handlerPtr(profileRef, result, contextPtr);
        return;
    }

}

le_result_t taf_DataConnection::SetDefaultProfileIdSync(uint8_t slotId, uint32_t profileId)
{
    // initialize the synchronous promise
    CmdSynchronousPromise = std::promise<le_result_t>();

    le_result_t result = SendSettingDefaultProfileIdCmd(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result,
                         "Setting default profile is failed, slotId(%d) profileId(%d)",
                         slotId, profileId);

    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();

    LE_INFO("Setting default profile(%d) for slotId(%d) is finished, result: %d",
             profileId, slotId, result);

    return result;
}

le_result_t taf_DataConnection::GetDefaultProfileIdSync(uint8_t *slotId, uint32_t *profileId)
{
    TAF_ERROR_IF_RET_VAL(slotId == NULL || profileId == NULL, LE_BAD_PARAMETER, "Null pointer");

    // initialize the synchronous promise
    CmdSynchronousPromise = std::promise<le_result_t>();

    le_result_t result = SendGettingDefaultProfileIdCmd();
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Setting default profile is failed");

// In SA415M with old telsdk version, there is no getDefaultProfile function which will not set
// CmdSynchronousPromise value
#ifdef TARGET_SA515M
    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();
#else
    result = LE_OK;
#endif

    if (result == LE_OK)
    {
        *profileId = DefaultProfileId;
        *slotId = DefaultSlotId;
    }
    else
    {
        LE_ERROR("Getting default profile is failed, use default slotId(%d) profileId(%d)",
                  SLOT_ID_1, TAF_DCS_DEFAULT_PROFILE);
        *profileId = TAF_DCS_DEFAULT_PROFILE;
        *slotId = SLOT_ID_1;
    }

    return result;
}

le_result_t taf_DataConnection::GetSlotIdAndProfileIdByIfName
(
    const char* namePtr,
    uint8_t *slotId,
    uint32_t* profileId
)
{
    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_BAD_PARAMETER, "namePtr is null");
    TAF_ERROR_IF_RET_VAL(slotId == NULL, LE_BAD_PARAMETER, "slotId is null");
    TAF_ERROR_IF_RET_VAL(profileId == NULL, LE_BAD_PARAMETER, "profileId is null");

    le_dls_Link_t* linkPtr = NULL;

    le_mutex_Lock(callCtxMutex);
    linkPtr = le_dls_Peek(&DataCallCtxList);

    while (linkPtr)
    {
        taf_dcs_CallCtx_t* callCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_CallCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCallCtxList, linkPtr);

        if (strncmp(callCtxPtr->intfName,namePtr,TAF_DCS_NAME_MAX_LEN)== 0)
        {
            *profileId = callCtxPtr->profileId;
            *slotId = callCtxPtr->slotId;
            le_mutex_Unlock(callCtxMutex);
            return LE_OK;
        }
    }

    le_mutex_Unlock(callCtxMutex);
    return LE_NOT_FOUND;
}

le_result_t taf_DataConnection::GetInterfaceName
(
    uint8_t slotId,
    int32_t profileId,
    char* namePtr,
    size_t nameSize
)
{
    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_NOT_FOUND, "namePtr is null");
    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND,
                         "Cannot find call context from slotId(%d) profileId(%d)",
                         slotId, profileId);

    if ((callCtxPtr->callStatus == telux::data::DataCallStatus::NET_CONNECTED) &&
        ((callCtxPtr->ipv4Status == telux::data::DataCallStatus::NET_CONNECTED) ||
         (callCtxPtr->ipv6Status == telux::data::DataCallStatus::NET_CONNECTED)))
    {
        le_utf8_Copy(namePtr, callCtxPtr->intfName, nameSize, NULL);
        return LE_OK;
    }

    LE_DEBUG("Invalid connection status, callstatus: %s, ipv4: %s, ipv6: %s",
             CallStatusToString(callCtxPtr->callStatus), CallStatusToString(callCtxPtr->ipv4Status),
             CallStatusToString(callCtxPtr->ipv6Status));
    return LE_NOT_POSSIBLE;
}

le_result_t taf_DataConnection::GetIpv4Address
(
    uint8_t slotId,
    int32_t profileId,
    char* addrPtr,
    size_t addrSize
)
{
    TAF_ERROR_IF_RET_VAL(addrPtr == NULL, LE_NOT_FOUND, "addrPtr is null");
    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND,
                         "Cannot find call context from slotId(%d) profileId(%d)",
                         slotId, profileId);

    le_utf8_Copy(addrPtr, callCtxPtr->ipv4Addr, addrSize, NULL);

    return LE_OK;
}

le_result_t taf_DataConnection::GetIpv4Gateway
(
    uint8_t slotId,
    int32_t profileId,
    char* addrPtr,
    size_t addrSize
)
{
    TAF_ERROR_IF_RET_VAL(addrPtr == NULL, LE_NOT_FOUND, "addrPtr is null");
    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND,
                         "Cannot find call context slotId(%d) profileId(%d)",
                         slotId, profileId);

    le_utf8_Copy(addrPtr, callCtxPtr->ipv4Gw, addrSize, NULL);

    return LE_OK;
}

le_result_t taf_DataConnection::GetIpv4Dns
(
    uint8_t slotId,
    int32_t profileId,
    char* dns1Ptr,
    size_t dns1Size,
    char* dns2Ptr,
    size_t dns2Size
)
{
    TAF_ERROR_IF_RET_VAL(dns1Ptr == NULL || dns2Ptr == NULL, LE_NOT_FOUND, "addrPtr is null");
    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND,
                         "Cannot find call context slotId(%d) profileId(%d)", slotId, profileId);

    le_utf8_Copy(dns1Ptr, callCtxPtr->ipv4Dns1, dns1Size, NULL);
    le_utf8_Copy(dns2Ptr, callCtxPtr->ipv4Dns2, dns2Size, NULL);

    return LE_OK;
}

le_result_t taf_DataConnection::GetIpv6Address
(
    uint8_t slotId,
    int32_t profileId,
    char* addrPtr,
    size_t addrSize
)
{
    TAF_ERROR_IF_RET_VAL(addrPtr == NULL, LE_NOT_FOUND, "addrPtr is null");
    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND,
                         "Cannot find call context slotId(%d) profileId(%d)", slotId, profileId);

    le_utf8_Copy(addrPtr, callCtxPtr->ipv6Addr, addrSize, NULL);

    return LE_OK;
}

le_result_t taf_DataConnection::GetIpv6Gateway
(
    uint8_t slotId,
    int32_t profileId,
    char* addrPtr,
    size_t addrSize
)
{
    TAF_ERROR_IF_RET_VAL(addrPtr == NULL, LE_NOT_FOUND, "addrPtr is null");
    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND,
                         "Cannot find call context slotId(%d) profileId(%d)",
                         slotId, profileId);

    le_utf8_Copy(addrPtr, callCtxPtr->ipv6Gw, addrSize, NULL);

    return LE_OK;
}

le_result_t taf_DataConnection::GetIpv6Dns
(
    uint8_t slotId,
    int32_t profileId,
    char* dns1Ptr,
    size_t dns1Size,
    char* dns2Ptr,
    size_t dns2Size
)
{
    TAF_ERROR_IF_RET_VAL(dns1Ptr == NULL || dns2Ptr == NULL, LE_NOT_FOUND, "addrPtr is null");
    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND,
                         "Cannot find call context from slotId(%d) profileId(%d)",
                         slotId, profileId);

    le_utf8_Copy(dns1Ptr, callCtxPtr->ipv6Dns1, dns1Size, NULL);
    le_utf8_Copy(dns2Ptr, callCtxPtr->ipv6Dns2, dns2Size, NULL);

    return LE_OK;
}

le_result_t taf_DataConnection::GetDataBearerTechnology
(
    uint8_t slotId,
    int32_t profileId,
    taf_dcs_DataBearerTechnology_t* downDataBearerTechPtr,
    taf_dcs_DataBearerTechnology_t* upDataBearerTechPtr
)
{
    TAF_ERROR_IF_RET_VAL(downDataBearerTechPtr == NULL || upDataBearerTechPtr == NULL,
                         LE_NOT_FOUND, "ptr is null");
    taf_dcs_CallCtx_t* callCtxPtr;

    callCtxPtr = GetCallCtx(slotId, profileId);
    if (callCtxPtr == NULL)
    {
        LE_ERROR("Cannot find call context, use unknown data bearer");
        *downDataBearerTechPtr = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
        *upDataBearerTechPtr   = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
        return LE_OK;
    }

    *downDataBearerTechPtr = callCtxPtr->dataBearerTech;
    *upDataBearerTechPtr = callCtxPtr->dataBearerTech;
    return LE_OK;
}

le_result_t taf_DataConnection::GetRoamingStatus
(
    uint8_t slotId,
    bool* isRoamingPtr,
    taf_dcs_RoamingType_t* typePtr
)
{
    TAF_ERROR_IF_RET_VAL(isRoamingPtr == NULL || typePtr == NULL, LE_BAD_PARAMETER, "ptr is null");

#ifdef TARGET_SA515M
        auto reqRoamingStatusCbFunc = std::bind(
                                    &taf_DataConnRequestRoamingStatusCallback::requestRoamingStatus,
                                    reqRoamingStatusCb,
                                    std::placeholders::_1,
                                    std::placeholders::_2);

        telux::common::Status status =
            dataServingSystemManagers[(SlotId)slotId]->requestRoamingStatus(
                                                                            reqRoamingStatusCbFunc);
        TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT,
                             "Call sdk function failed.");

        le_clk_Time_t timeToWait = {2, 0};
        le_result_t res = le_sem_WaitWithTimeOut(reqRoamingStatusCb->semaphore, timeToWait);
        TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_TIMEOUT, "Wait semaphore timeout.");

        TAF_ERROR_IF_RET_VAL(reqRoamingStatusCb->errorCode != telux::common::ErrorCode::SUCCESS,
                             LE_FAULT, "Telsdk returns error.");

        *isRoamingPtr = reqRoamingStatusCb->status.isRoaming;
        *typePtr = (taf_dcs_RoamingType_t)reqRoamingStatusCb->status.type;
#endif

    return LE_OK;
}

le_event_Id_t taf_DataConnection::GetSessionStateEvent(uint8_t slotId, int32_t profileId)
{
    taf_dcs_CallCtx_t* callCtxPtr;

    callCtxPtr = GetCallCtx(slotId, profileId);
    if (callCtxPtr == NULL)
    {
        callCtxPtr = CreateDataCallCtx(slotId, profileId);
    }
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, NULL,
                         "Cannot find call context slotId(%d) profileId(%d)",
                         slotId, profileId);

    return callCtxPtr->sessionStateEvent;
}

le_result_t taf_DataConnection::SendStatusChangedNotification
(
    taf_dcs_CallCtx_t *callCtxPtr,
    dataCallEvent_t *eventPtr
)
{
    taf_dcs_StateInfo_t stateInfo;
    stateInfo.ipType = TAF_DCS_PDP_UNKNOWN;

    if (callCtxPtr->callStatus == telux::data::DataCallStatus::NET_CONNECTING)
    {
        stateInfo.ipType = GetEvtInfoFromConnStatus(callCtxPtr, callCtxPtr->callStatus);
        SendNotificationStateEvent(TAF_DCS_CONNECTING, &stateInfo, callCtxPtr);
    }
    else if (callCtxPtr->callStatus == telux::data::DataCallStatus::NET_CONNECTED)
    {
        stateInfo.ipType = GetEvtInfoFromConnStatus(callCtxPtr, callCtxPtr->callStatus);
        SendNotificationStateEvent(TAF_DCS_CONNECTED, &stateInfo, callCtxPtr);
    }
    else if (callCtxPtr->callStatus == telux::data::DataCallStatus::NET_NO_NET)
    {
        stateInfo.ipType = GetEvtInfoFromConnStatus(callCtxPtr, callCtxPtr->callStatus);
        SendNotificationStateEvent(TAF_DCS_DISCONNECTED, &stateInfo, callCtxPtr);
    }
    else if (callCtxPtr->callStatus == telux::data::DataCallStatus::NET_DISCONNECTING)
    {
        stateInfo.ipType = GetEvtInfoFromConnStatus(callCtxPtr, callCtxPtr->callStatus);
        SendNotificationStateEvent(TAF_DCS_DISCONNECTING, &stateInfo, callCtxPtr);
    }
    else
    {
        LE_INFO("skip this event for type[%s] status[%s] IPv4[%s] IPv6[%s]",
            IpFamilyTypeToString(callCtxPtr->ipType),
            CallStatusToString(callCtxPtr->callStatus),
            CallStatusToString(callCtxPtr->ipv4Status),
            CallStatusToString(callCtxPtr->ipv6Status));
    }

    return LE_OK;
}

taf_dcs_DataBearerTechnology_t taf_DataConnection::updateDataBearerTech
(
    telux::data::DataBearerTechnology dataBearerTech
)
{
    taf_dcs_DataBearerTechnology_t result;

    switch(dataBearerTech) {
        case telux::data::DataBearerTechnology::CDMA_1X:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_1X;
        break;
        case telux::data::DataBearerTechnology::EVDO_REV0:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EVDO;
        break;
        case telux::data::DataBearerTechnology::EVDO_REVA:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EVDO_REVA;
        break;
        case telux::data::DataBearerTechnology::EVDO_REVB:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EVDO_REVB;
        break;
        case telux::data::DataBearerTechnology::EHRPD:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EHRPD;
        break;
        case telux::data::DataBearerTechnology::FMC:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA_EVDO_FMC;
        break;
        case telux::data::DataBearerTechnology::HRPD:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_HRPD;
        break;
        case telux::data::DataBearerTechnology::BEARER_TECH_3GPP2_WLAN:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_3GPP2_WLAN;
        break;
        case telux::data::DataBearerTechnology::WCDMA:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_WCDMA;
        break;
        case telux::data::DataBearerTechnology::GPRS:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_GPRS;
        break;
        case telux::data::DataBearerTechnology::HSDPA:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_HSDPA;
        break;
        case telux::data::DataBearerTechnology::HSUPA:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_HSUPA;
        break;
        case telux::data::DataBearerTechnology::EDGE:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_EDGE;
        break;
        case telux::data::DataBearerTechnology::LTE:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_LTE;
        break;
        case telux::data::DataBearerTechnology::HSDPA_PLUS:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_HSDPA_PLUS;
        break;
        case telux::data::DataBearerTechnology::DC_HSDPA_PLUS:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_DC_HSDPA_PLUS;
        break;
        case telux::data::DataBearerTechnology::HSPA:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_HSPA;
        break;
        case telux::data::DataBearerTechnology::BEARER_TECH_64_QAM:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_64_QAM;
        break;
        case telux::data::DataBearerTechnology::TDSCDMA:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_TD_SCDMA;
        break;
        case telux::data::DataBearerTechnology::GSM:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_GSM;
        break;
        case telux::data::DataBearerTechnology::BEARER_TECH_3GPP_WLAN:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_3GPP_WLAN;
        break;
        case telux::data::DataBearerTechnology::BEARER_TECH_5G:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_5G;
        break;
        default:
            result = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
        break;
    }

    return result;
}

bool taf_DataConnection::updateStatus(taf_dcs_CallCtx_t *callCtxPtr, dataCallEvent_t *eventPtr)
{
    bool isSendEvent = false;

    callCtxPtr->callStatus = eventPtr->callStatus;
    callCtxPtr->ipv4Status = eventPtr->ipv4Status;
    callCtxPtr->ipv6Status = eventPtr->ipv6Status;

    switch (eventPtr->callStatus)
    {
        case  telux::data::DataCallStatus::NET_CONNECTING:
            callCtxPtr->ipType     = eventPtr->ipType;
            isSendEvent = true;
        break;

        case telux::data::DataCallStatus::NET_CONNECTED:
            le_utf8_Copy(callCtxPtr->intfName, eventPtr->ifName.c_str(),
                         sizeof(callCtxPtr->intfName), NULL);
            if (eventPtr->ipv4Status == telux::data::DataCallStatus::NET_CONNECTED)
            {
                le_utf8_Copy(callCtxPtr->ipv4Addr, eventPtr->ipv4AddrInfo.ifAddress.c_str(),
                             TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv4Gw, eventPtr->ipv4AddrInfo.gwAddress.c_str(),
                             TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv4Dns1, eventPtr->ipv4AddrInfo.primaryDnsAddress.c_str(),
                             TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv4Dns2,
                             eventPtr->ipv4AddrInfo.secondaryDnsAddress.c_str(),
                             TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
            }

            if (eventPtr->ipv6Status == telux::data::DataCallStatus::NET_CONNECTED)
            {
                le_utf8_Copy(callCtxPtr->ipv6Addr, eventPtr->ipv6AddrInfo.ifAddress.c_str(),
                             TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv6Gw, eventPtr->ipv6AddrInfo.gwAddress.c_str(),
                             TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv6Dns1, eventPtr->ipv6AddrInfo.primaryDnsAddress.c_str(),
                             TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv6Dns2,
                             eventPtr->ipv6AddrInfo.secondaryDnsAddress.c_str(),
                             TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
            }

            callCtxPtr->dataBearerTech = updateDataBearerTech(eventPtr->dataBearerTech);
            isSendEvent = true;
        break;

        case telux::data::DataCallStatus::NET_DISCONNECTING:
            isSendEvent = true;
        break;

        case telux::data::DataCallStatus::NET_NO_NET:
            memset(callCtxPtr->intfName, 0, sizeof(callCtxPtr->intfName));
            isSendEvent = true;
        break;

        default:
            LE_ERROR("cannot handle this event: %s", CallStatusToString(eventPtr->callStatus));
        break;

    }

    return isSendEvent;
}

taf_dcs_Pdp_t taf_DataConnection::GetEvtInfoFromConnStatus
(
    taf_dcs_CallCtx_t *callCtxPtr,
    telux::data::DataCallStatus callStatus
)
{
    taf_dcs_Pdp_t ipType = TAF_DCS_PDP_UNKNOWN;

    if ((callCtxPtr->ipv4Status == callStatus) && (callCtxPtr->ipv6Status == callStatus))
    {
        ipType = TAF_DCS_PDP_IPV4V6;
    }
    else if (callCtxPtr->ipv4Status == callStatus)
    {
        ipType = TAF_DCS_PDP_IPV4;
    }
    else if (callCtxPtr->ipv6Status == callStatus)
    {
        ipType = TAF_DCS_PDP_IPV6;
    }

    return ipType;
}

void taf_DataConnection::InternalEventHandler(void* reportPtr)
{
    le_result_t result = LE_OK, clientRet = LE_OK;
    bool isSendNotification;
    dataCallEvent_t *eventPtr = (dataCallEvent_t *)reportPtr;
    int32_t profileId = eventPtr->profileId;
    uint8_t slotId = eventPtr->slotId;
    taf_dcs_CallCtx_t *callCtxPtr;
    taf_dcs_StateInfo_t stateInfo = {TAF_DCS_PDP_UNKNOWN};
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();
    HandlerSessionMapping_t *connHandlerMappingPtr = NULL;
    taf_dcs_ProfileRef_t profileRef = NULL;
    pid_t pid;
    uid_t uid;

    LE_DEBUG("----event=%d",eventPtr->event);

    switch (eventPtr->event)
    {
        // this event came from telsdk start_call callback handler
        case EVT_START_CALLBACK:
            callCtxPtr = GetCallCtx(slotId, profileId);
            TAF_ERROR_IF_RET_NIL(callCtxPtr == NULL,
                                 "Cannot get call context from slotId(%d) profileId(%d)",
                                 slotId, profileId);

            if (eventPtr->errorCode != telux::common::ErrorCode::SUCCESS)
            {
                result = LE_FAULT;
                if (callCtxPtr->funcType == CALL_FUNCTION_ASYNC_START)
                {
                    //Find the handler function and then call it
                    connHandlerMappingPtr = dataConnection.FindAsyncHandler(slotId, profileId);

                    if (connHandlerMappingPtr != NULL &&
                        connHandlerMappingPtr->asyncHandler != NULL)
                    {
                        //Call handler function, and then remove it from mapping list
                        profileRef = dataProfile.GetProfileRef(callCtxPtr->slotId,
                                                               callCtxPtr->profileId);
                        clientRet = le_msg_GetClientUserCreds(connHandlerMappingPtr->sessionRef,
                                                              &uid, &pid);
                        if (profileRef != NULL && clientRet == LE_OK)
                            connHandlerMappingPtr->asyncHandler(profileRef, LE_FAULT,
                                                                connHandlerMappingPtr->contextPtr);

                        dataConnection.DeleteHandlerInfo(callCtxPtr->slotId, callCtxPtr->profileId,
                                                         connHandlerMappingPtr->asyncHandler);
                    }
                    // If starting session failed, remove the session from call ctx.
                    RemoveSessionFromCallCtx(callCtxPtr, connHandlerMappingPtr->sessionRef);
                    pthread_mutex_lock(&callCtxPtr->callActionMutex);
                    callCtxPtr->isCallActionInProgress = false;
                    pthread_mutex_unlock(&callCtxPtr->callActionMutex);
                    pthread_cond_signal(&callCtxPtr->callActionCond);

                }
                else if (callCtxPtr->funcType == CALL_FUNCTION_SYNC_START)
                {
                    pthread_mutex_lock(&callCtxPtr->callActionMutex);
                    callCtxPtr->isCallActionInProgress = false;
                    pthread_mutex_unlock(&callCtxPtr->callActionMutex);
                    CmdSynchronousPromise.set_value(result);
                }

            }
            else
            {
                isSendNotification = updateStatus(callCtxPtr, eventPtr);
                stateInfo.ipType = GetEvtInfoFromConnStatus(callCtxPtr,
                                                    telux::data::DataCallStatus::NET_CONNECTING);
                TAF_ERROR_IF_RET_NIL(isSendNotification != true,
                                     "won't send notification to listener");
                SendNotificationStateEvent(TAF_DCS_CONNECTING, &stateInfo, callCtxPtr);

                // If the call back is from synchronous data call, need to set the result
                if (callCtxPtr->funcType == CALL_FUNCTION_SYNC_START)
                {
                    LE_INFO(" SYNC SET PROMISE");
                    CmdSynchronousPromise.set_value(result);
                }
            }

        break;

        // this event came from telsdk stop_call callback handler
        case EVT_STOP_CALLBACK:
            callCtxPtr = GetCallCtx(slotId, profileId);
            TAF_ERROR_IF_RET_NIL(callCtxPtr == NULL,
                                 "Cannot get call context from slotId(%d) profileId(%d)",
                                 slotId, profileId);
            if (eventPtr->errorCode != telux::common::ErrorCode::SUCCESS)
            {
                result = LE_FAULT;
                if (callCtxPtr->funcType == CALL_FUNCTION_ASYNC_STOP)
                {
                    //Find the handler function and then call it
                    connHandlerMappingPtr = dataConnection.FindAsyncHandler(slotId, profileId);

                    // If stopping session failed, add the session into call ctx.
                    AddSessionToCallCtx(callCtxPtr, connHandlerMappingPtr->sessionRef);
                    pthread_mutex_lock(&callCtxPtr->callActionMutex);
                    callCtxPtr->isCallActionInProgress = false;
                    pthread_mutex_unlock(&callCtxPtr->callActionMutex);
                    pthread_cond_signal(&callCtxPtr->callActionCond);

                    if (connHandlerMappingPtr != NULL &&
                        connHandlerMappingPtr->asyncHandler != NULL)
                    {
                        //Call handler function, and then remove it from mapping list
                        profileRef = dataProfile.GetProfileRef(callCtxPtr->slotId,
                                                               callCtxPtr->profileId);
                        clientRet = le_msg_GetClientUserCreds(connHandlerMappingPtr->sessionRef,
                                                              &uid, &pid);
                        if (profileRef != NULL && clientRet == LE_OK)
                            connHandlerMappingPtr->asyncHandler(profileRef, LE_FAULT,
                                                                connHandlerMappingPtr->contextPtr);

                        dataConnection.DeleteHandlerInfo(callCtxPtr->slotId, callCtxPtr->profileId,
                                                         connHandlerMappingPtr->asyncHandler);
                    }

                }
                else if (callCtxPtr->funcType == CALL_FUNCTION_SYNC_STOP)
                {
                    pthread_mutex_lock(&callCtxPtr->callActionMutex);
                    callCtxPtr->isCallActionInProgress = false;
                    pthread_mutex_unlock(&callCtxPtr->callActionMutex);
                    CmdSynchronousPromise.set_value(result);
                }
            }
            else
            {
                isSendNotification = updateStatus(callCtxPtr, eventPtr);
                stateInfo.ipType = GetEvtInfoFromConnStatus(callCtxPtr,
                                                    telux::data::DataCallStatus::NET_CONNECTING);
                TAF_ERROR_IF_RET_NIL(isSendNotification != true,
                                     "won't send notification to listener");
                SendNotificationStateEvent(TAF_DCS_DISCONNECTING, &stateInfo, callCtxPtr);

                // If the call back is from synchronous data call, need to set the result
                if (callCtxPtr->funcType == CALL_FUNCTION_SYNC_STOP)
                    CmdSynchronousPromise.set_value(result);
            }

        break;

        // the event from telsdk status changed handler
        case EVT_STATUS_CHANGED:
            callCtxPtr = GetCallCtx(slotId, profileId);
            TAF_ERROR_IF_RET_NIL(callCtxPtr == NULL,
                                 "Cannot get call context from slotId(%d) profileId(%d)",
                                 slotId, profileId);
            isSendNotification = updateStatus(callCtxPtr, eventPtr);
            TAF_ERROR_IF_RET_NIL(isSendNotification != true, "Won't send notification to listener");
            result = SendStatusChangedNotification(callCtxPtr, eventPtr);
            TAF_ERROR_IF_RET_NIL(result != LE_OK, "Cannot sent notification, ret: %d", result);
        break;

        // the event from telsdk setting default profile callback
        case EVT_SET_DEFAULT_PROFILE:
            if ((eventPtr->errorCode == telux::common::ErrorCode::SUCCESS) ||
                (eventPtr->errorCode == telux::common::ErrorCode::NO_EFFECT))
            {
                LE_INFO("Setting profile finished, errorCode: %d", (int32_t)eventPtr->errorCode);
            }
            else
            {
                LE_ERROR("Setting profile is failed from callback, errorCode: %d",
                          (uint32_t)eventPtr->errorCode);
                result = LE_FAULT;
            }
            CmdSynchronousPromise.set_value(result);
        break;

        // the event from telsdk getting default profile callback
        case EVT_GET_DEFAULT_PROFILE:
            if (eventPtr->errorCode != telux::common::ErrorCode::SUCCESS)
            {
                LE_ERROR("Setting profile is failed from callback, errorCode: %d",
                          (uint32_t)eventPtr->errorCode);
                result = LE_FAULT;
            }
            else
            {
                DefaultProfileId = profileId;
                DefaultSlotId = slotId;
            }
            CmdSynchronousPromise.set_value(result);
        break;

        default:
            LE_ERROR("invalid event(%d)", eventPtr->event);
        break;
    }

    return;
}

void taf_DataConnection::EventHandler(void* reportPtr)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    return dataConnection.InternalEventHandler(reportPtr);
}

void taf_DataConnection::SendNotificationStateEvent
(
    taf_dcs_ConState_t conState,
    taf_dcs_StateInfo_t *infoPtr,
    taf_dcs_CallCtx_t *callCtxPtr
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();
    taf_dcs_ProfileRef_t profileRef = NULL;
    le_result_t ret = LE_OK;
    pid_t pid;
    uid_t uid;
    callCtxPtr->latestConState = conState;
    TAF_ERROR_IF_RET_NIL(SessionStateFunc == NULL, "SessionStateFunc is NULL, drop this event");
    LE_INFO("Sending connection status: %d, isCallActionInProgress=%d, slotId(%d) profileId(%d)",
             conState,callCtxPtr->isCallActionInProgress,callCtxPtr->slotId, callCtxPtr->profileId);
    SessionStateFunc(callCtxPtr->latestConState, infoPtr, callCtxPtr);

    // wakeup sync API
    if ( ((conState == TAF_DCS_CONNECTED) || (conState == TAF_DCS_DISCONNECTED)) &&
         (callCtxPtr->isCallActionInProgress == true) )
    {
        // When starts 2 data calls almost at the same time with ipv4 or ipv6, sometimes the
        // ipv4Status or ipv6Status will be INVALID, don't process this event
        if ( ((callCtxPtr->ipType == telux::data::IpFamilyType::IPV4) &&
              (callCtxPtr->ipv4Status != telux::data::DataCallStatus::INVALID) ) ||
             ((callCtxPtr->ipType == telux::data::IpFamilyType::IPV6) &&
              (callCtxPtr->ipv6Status != telux::data::DataCallStatus::INVALID) ) )
        {
            LE_INFO("promise for Type[%s] IPv4[%s] IPv6[%s]",
                     IpFamilyTypeToString(callCtxPtr->ipType),
                     CallStatusToString(callCtxPtr->ipv4Status),
                     CallStatusToString(callCtxPtr->ipv6Status));

            if (callCtxPtr->funcType == CALL_FUNCTION_SYNC_START ||
                callCtxPtr->funcType == CALL_FUNCTION_SYNC_STOP)
            {
                //set isCallActionInProgress here to avoid setting EventSynchronousPromise twice in some cases
                pthread_mutex_lock(&callCtxPtr->callActionMutex);
                callCtxPtr->isCallActionInProgress = false;
                pthread_mutex_unlock(&callCtxPtr->callActionMutex);
                EventSynchronousPromise.set_value(LE_OK);
            }
            else if (callCtxPtr->funcType == CALL_FUNCTION_ASYNC_START ||
                     callCtxPtr->funcType == CALL_FUNCTION_ASYNC_STOP)
            {
                HandlerSessionMapping_t *connHandlerMappingPtr = NULL;
                //Find the handler function and then call it
                connHandlerMappingPtr = dataConnection.FindAsyncHandler(callCtxPtr->slotId,
                                                                        callCtxPtr->profileId);

                // Set isCallActionInProgress to be false before calling handler to process
                // CloseEventHandler from main thread
                pthread_mutex_lock(&callCtxPtr->callActionMutex);
                callCtxPtr->isCallActionInProgress = false;
                pthread_mutex_unlock(&callCtxPtr->callActionMutex);
                pthread_cond_signal(&callCtxPtr->callActionCond);

                if (connHandlerMappingPtr != NULL && connHandlerMappingPtr->asyncHandler != NULL)
                {
                    profileRef = dataProfile.GetProfileRef(callCtxPtr->slotId,
                                                           callCtxPtr->profileId);
                    //Call handler function, and then remove it from mapping list
                    if( profileRef != NULL)
                    {
                        //Exception handle: If starts a data call with profile which has ims APN,
                        //the status will be NET_NO_NET(disconnected), and telsdk will return a NULL
                        //pointer for the iCall parameter of the StopDataCallCallback when invoke
                        // stopCall function at this situlation. So needs to remove session since
                        // there is no need to call stopCall later
                        if(callCtxPtr->callStatus == telux::data::DataCallStatus::NET_NO_NET)
                        {
                            if(callCtxPtr->funcType == CALL_FUNCTION_ASYNC_START)
                                RemoveSessionFromCallCtx(callCtxPtr,
                                                         connHandlerMappingPtr->sessionRef);

                            ret = le_msg_GetClientUserCreds(connHandlerMappingPtr->sessionRef,
                                                            &uid, &pid);
                            LE_INFO("result =%d, uid=%d,pid=%d",ret, (int)uid, (int)pid);
                            // Handler can only be called when the client doesn't exit
                            if(ret == LE_OK)
                            {
                                if(callCtxPtr->funcType == CALL_FUNCTION_ASYNC_START)
                                    connHandlerMappingPtr->asyncHandler(profileRef, LE_TERMINATED,
                                                                connHandlerMappingPtr->contextPtr);
                                else
                                    connHandlerMappingPtr->asyncHandler(profileRef, LE_OK,
                                                                connHandlerMappingPtr->contextPtr);
                            }
                        }
                        else
                        {
                            ret = le_msg_GetClientUserCreds(connHandlerMappingPtr->sessionRef,
                                                            &uid, &pid);
                            LE_INFO("result =%d, uid=%d,pid=%d",ret, (int)uid, (int)pid);
                            if(ret == LE_OK)
                            {
                                connHandlerMappingPtr->asyncHandler(profileRef, LE_OK,
                                                                connHandlerMappingPtr->contextPtr);
                            }
                        }
                    }

                    dataConnection.DeleteHandlerInfo(callCtxPtr->slotId, callCtxPtr->profileId,
                                                     connHandlerMappingPtr->asyncHandler);
                }

            }
        }
        else if (callCtxPtr->ipType == telux::data::IpFamilyType::IPV4V6)
        {
            if ( ( (callCtxPtr->ipv4Status == telux::data::DataCallStatus::NET_CONNECTED) ||
                   (callCtxPtr->ipv4Status == telux::data::DataCallStatus::NET_NO_NET) ) &&
                 ( (callCtxPtr->ipv6Status == telux::data::DataCallStatus::NET_CONNECTED) ||
                   (callCtxPtr->ipv6Status == telux::data::DataCallStatus::NET_NO_NET) ) )
            {
                LE_INFO("promise for Conn[%d] Type[%s] IPv4[%s] IPv6[%s]", conState,
                         IpFamilyTypeToString(callCtxPtr->ipType),
                         CallStatusToString(callCtxPtr->ipv4Status),
                         CallStatusToString(callCtxPtr->ipv6Status));

                if (callCtxPtr->funcType == CALL_FUNCTION_SYNC_START ||
                    callCtxPtr->funcType == CALL_FUNCTION_SYNC_STOP)
                {
                    pthread_mutex_lock(&callCtxPtr->callActionMutex);
                    callCtxPtr->isCallActionInProgress = false;
                    pthread_mutex_unlock(&callCtxPtr->callActionMutex);
                    EventSynchronousPromise.set_value(LE_OK);
                }
                else if (callCtxPtr->funcType == CALL_FUNCTION_ASYNC_START ||
                        callCtxPtr->funcType == CALL_FUNCTION_ASYNC_STOP)
                {
                    HandlerSessionMapping_t *connHandlerMappingPtr = NULL;
                    //Find the handler function and then call it
                    connHandlerMappingPtr = dataConnection.FindAsyncHandler(callCtxPtr->slotId,
                                                                            callCtxPtr->profileId);
                    pthread_mutex_lock(&callCtxPtr->callActionMutex);
                    LE_INFO("current  action %d , set it to false",
                             callCtxPtr->isCallActionInProgress);
                    callCtxPtr->isCallActionInProgress = false;
                    pthread_mutex_unlock(&callCtxPtr->callActionMutex);
                    pthread_cond_signal(&callCtxPtr->callActionCond);

                    if ( connHandlerMappingPtr != NULL &&
                        connHandlerMappingPtr->asyncHandler != NULL )
                    {
                        profileRef = dataProfile.GetProfileRef(callCtxPtr->slotId,
                                                               callCtxPtr->profileId);
                        //Call handler function, and then remove it from mapping list
                        if( profileRef != NULL)
                        {
                            //Exception handle: See the upper comments
                            if(callCtxPtr->callStatus == telux::data::DataCallStatus::NET_NO_NET)
                            {
                                if(callCtxPtr->funcType == CALL_FUNCTION_ASYNC_START)
                                    RemoveSessionFromCallCtx(callCtxPtr,
                                                             connHandlerMappingPtr->sessionRef);

                                ret = le_msg_GetClientUserCreds(connHandlerMappingPtr->sessionRef,
                                                                &uid, &pid);
                                LE_INFO("result =%d, uid=%d,pid=%d",ret, (int)uid, (int)pid);
                                if(ret == LE_OK)
                                {
                                    if(callCtxPtr->funcType == CALL_FUNCTION_ASYNC_START)
                                        connHandlerMappingPtr->asyncHandler(profileRef,
                                                                            LE_TERMINATED,
                                                                connHandlerMappingPtr->contextPtr);
                                    else
                                        connHandlerMappingPtr->asyncHandler(profileRef, LE_OK,
                                                                connHandlerMappingPtr->contextPtr);
                                }
                            }
                            else
                            {
                                ret = le_msg_GetClientUserCreds(connHandlerMappingPtr->sessionRef,
                                                                &uid, &pid);
                                LE_INFO("result =%d, uid=%d,pid=%d",ret, (int)uid, (int)pid);
                                if(ret == LE_OK)
                                {
                                    connHandlerMappingPtr->asyncHandler(profileRef, LE_OK,
                                                                connHandlerMappingPtr->contextPtr);
                                }
                            }
                        }

                        dataConnection.DeleteHandlerInfo(callCtxPtr->slotId, callCtxPtr->profileId,
                                                         connHandlerMappingPtr->asyncHandler);
                    }

                }
            }
            else
            {
                LE_INFO("no promise for Conn[%d] Type[%s] IPv4[%s] IPv6[%s]", conState,
                         IpFamilyTypeToString(callCtxPtr->ipType),
                         CallStatusToString(callCtxPtr->ipv4Status),
                         CallStatusToString(callCtxPtr->ipv6Status));
            }
        }
        else
        {
            LE_ERROR("invalid IP type: %s or status IPv4[%s] IPv6[%s]",
                      IpFamilyTypeToString(callCtxPtr->ipType),
                      CallStatusToString(callCtxPtr->ipv4Status),
                      CallStatusToString(callCtxPtr->ipv6Status));
        }
    }
    else
    {
        LE_INFO("no promise for Conn[%d] Type[%s] IPv4[%s] IPv6[%s]", conState,
                 IpFamilyTypeToString(callCtxPtr->ipType),
                 CallStatusToString(callCtxPtr->ipv4Status),
                  CallStatusToString(callCtxPtr->ipv6Status));
    }

    return;
}

void taf_DataConnection::RegisterSessionStateHandler(taf_dcs_SessionStateFunc_t func)
{
    TAF_ERROR_IF_RET_NIL(func == NULL, "the registered session state func is null");
    SessionStateFunc = func;
    return;
}

const char * taf_DataConnection::CallEventToString(taf_dcs_ConState_t callEvent)
{
    switch (callEvent)
    {
        case TAF_DCS_DISCONNECTED:
            return "disconnect";
        case TAF_DCS_CONNECTING:
            return "connecting";
        case TAF_DCS_CONNECTED:
            return "connected";
        case TAF_DCS_DISCONNECTING:
            return "disconnecting";
        default:
            LE_ERROR("unknown status: %d", callEvent);
            return "unknow status";
    }
    return "unknow status";
}

bool taf_DataConnection::IsIpv4(uint8_t slotId, int32_t profileId)
{
    taf_dcs_CallCtx_t* callCtxPtr = GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, false,
                         "Cannot get call context form slotId(%d) profileId(%d)",
                         slotId, profileId);
    return (callCtxPtr->ipv4Status == telux::data::DataCallStatus::NET_CONNECTED);
}

bool taf_DataConnection::IsIpv6(uint8_t slotId, int32_t profileId)
{
    taf_dcs_CallCtx_t* callCtxPtr = GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, false,
                         "Cannot get call context form slotId(%d) profileId(%d)",
                         slotId, profileId);
    return (callCtxPtr->ipv6Status == telux::data::DataCallStatus::NET_CONNECTED);
}

taf_DataConnection &taf_DataConnection::GetInstance()
{
    static taf_DataConnection instance;
    return instance;
}

void* taf_DataConnection::ConnectionEventThread(void* contextPtr)
{
    le_sem_Ref_t semRef = (le_sem_Ref_t)contextPtr;

    auto &dataConnection = taf_DataConnection::GetInstance();

    // internal event handler
    dataConnection.CallEvent = le_event_CreateId("Internal Event", sizeof(dataCallEvent_t));
    le_event_AddHandler("Internal Event Handler", dataConnection.CallEvent, EventHandler);

    le_sem_Post(semRef);

    LE_INFO("Create event loop for connection event");
    // start event loop
    le_event_RunLoop();
    return NULL;
}

void taf_DataConnection::AddHandlerSessionMapping
(
    uint8_t slotId,
    int32_t profileId,
    void *contextPtr,
    le_msg_SessionRef_t sessionRef,
    taf_dcs_AsyncSessionHandlerFunc_t asyncHandler
)
{
    HandlerSessionMapping_t *handlerSessionMapping;

    TAF_ERROR_IF_RET_NIL(sessionRef == nullptr || asyncHandler == nullptr, "Null ptr");

    handlerSessionMapping = (HandlerSessionMapping_t *)le_mem_ForceAlloc(HandlerSessionMappingPool);

    TAF_ERROR_IF_RET_NIL(handlerSessionMapping == nullptr ,
                         "Failed to alloc memory for handlerSessionMapping");

    memset(handlerSessionMapping, 0, sizeof(HandlerSessionMapping_t));
    handlerSessionMapping->slotId = slotId;
    handlerSessionMapping->profileId = profileId;
    handlerSessionMapping->contextPtr = contextPtr;
    handlerSessionMapping->asyncHandler = asyncHandler;
    handlerSessionMapping->sessionRef = sessionRef;
    handlerSessionMapping->handlerLink = LE_DLS_LINK_INIT;
    le_mutex_Lock(handlerlistMutex);
    le_dls_Queue(&HandlerSessionMappingList, &handlerSessionMapping->handlerLink);
    le_mutex_Unlock(handlerlistMutex);
    LE_INFO("Add handler for slotId(%d) profileId(%d), session reference %p successfully",
            slotId, profileId, sessionRef);
}

HandlerSessionMapping_t* taf_DataConnection::FindAsyncHandler
(
    uint8_t slotId,
    int32_t profileId
)
{
    HandlerSessionMapping_t *handlerSessionInfo;

    le_mutex_Lock(handlerlistMutex);
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&HandlerSessionMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, HandlerSessionMapping_t, handlerLink);
        if (handlerSessionInfo->profileId == profileId && handlerSessionInfo->slotId == slotId)
        {
            LE_INFO("Found async handler for session reference %p, slotId(%d), profileId(%d)",
                     handlerSessionInfo->sessionRef, slotId, profileId);
            le_mutex_Unlock(handlerlistMutex);
            return handlerSessionInfo;
        }
        handlerLinkPtr = le_dls_PeekNext(&HandlerSessionMappingList, handlerLinkPtr);
    }

    le_mutex_Unlock(handlerlistMutex);
    return NULL;
}

void taf_DataConnection::DeleteHandlerInfo
(
    uint8_t slotId,
    int32_t profileId,
    taf_dcs_AsyncSessionHandlerFunc_t asyncHandler
)
{
    HandlerSessionMapping_t *handlerSessionInfo;

    le_mutex_Lock(handlerlistMutex);
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&HandlerSessionMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, HandlerSessionMapping_t, handlerLink);
        handlerLinkPtr = le_dls_PeekNext(&HandlerSessionMappingList, handlerLinkPtr);
        if (handlerSessionInfo->asyncHandler == asyncHandler &&
            handlerSessionInfo->profileId == profileId &&
            handlerSessionInfo->slotId == slotId)
        {
            LE_INFO("Delete handler for slotId(%d), profileId(%d), session ref %p successfully",
                     slotId, profileId, handlerSessionInfo->sessionRef );
            le_dls_Remove(&HandlerSessionMappingList, &handlerSessionInfo->handlerLink);

            le_mem_Release(handlerSessionInfo);
            break;
        }
    }

    le_mutex_Unlock(handlerlistMutex);
}

// When the client closed, this event handler will process the data calls
void taf_DataConnection::CloseEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    taf_dcs_Pdp_t pdpType=TAF_DCS_PDP_IPV4V6;
    le_dls_Link_t* linkPtr = NULL;
    le_dls_Link_t* linkRefPtr = NULL;
    taf_dcs_ProfileRef_t profileRef = NULL;

    TAF_ERROR_IF_RET_NIL( sessionRef == NULL, "sessionRef is nullptr!");

    auto &dataConnection = taf_DataConnection::GetInstance();

    auto &dataProfile = taf_DataProfile::GetInstance();

    LE_INFO("SessionRef (%p) has been closed", sessionRef);

    // Find the data calls brought up by the sessionRef , and then stop them one by one
    le_mutex_Lock(dataConnection.callCtxMutex);
    linkPtr = le_dls_Peek(&dataConnection.DataCallCtxList);
    le_mutex_Unlock(dataConnection.callCtxMutex);
    while (linkPtr)
    {
        taf_dcs_CallCtx_t* callCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_CallCtx_t, link);

        TAF_ERROR_IF_RET_NIL(callCtxPtr == NULL, "this call context is NULL");

        pthread_mutex_lock(&callCtxPtr->sessionListMutex);
        linkRefPtr = le_dls_Peek(&(callCtxPtr->sessionRefList));
        pthread_mutex_unlock(&callCtxPtr->sessionListMutex);
        while (linkRefPtr)
        {
            taf_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkRefPtr, taf_SessionRef_t, link);
            pthread_mutex_lock(&callCtxPtr->sessionListMutex);
            linkRefPtr = le_dls_PeekNext(&(callCtxPtr->sessionRefList), linkRefPtr);
            pthread_mutex_unlock(&callCtxPtr->sessionListMutex);

            if (sessionRefPtr->sessionRef == sessionRef)
            {
                // If the client starts a data call with async api and loses connection before the
                // data call is completed, needs to stop the data call after it is completed

                LE_INFO(" isCallActionInProgress=%d,status=%d ",callCtxPtr->isCallActionInProgress,
                        (int)callCtxPtr->latestConState);

                pthread_mutex_lock(&callCtxPtr->callActionMutex);
                while (callCtxPtr->isCallActionInProgress == true)
                {
                    pthread_cond_wait(&callCtxPtr->callActionCond, &callCtxPtr->callActionMutex);
                }
                pthread_mutex_unlock(&callCtxPtr->callActionMutex);

                LE_INFO(" isCallActionInProgress=%d,status=%d ",callCtxPtr->isCallActionInProgress,
                        (int)callCtxPtr->latestConState);
                switch(callCtxPtr->ipType)
                {
                    case telux::data::IpFamilyType::IPV4:
                        pdpType = TAF_DCS_PDP_IPV4;
                    break;
                    case telux::data::IpFamilyType::IPV6:
                        pdpType = TAF_DCS_PDP_IPV6;
                    break;
                    case telux::data::IpFamilyType::IPV4V6:
                        pdpType = TAF_DCS_PDP_IPV4V6;
                    break;
                    default:
                    // In this case, when the client starts a data call and loses connection at
                    // once, the callCtxPtr->ipType is not updated at this time, so get the pdp
                    // type from the setting value.
                    profileRef = dataProfile.GetProfileRef(callCtxPtr->slotId,
                                                           callCtxPtr->profileId);

                    pdpType = dataProfile.GetPdp(profileRef);

                    LE_INFO("setting pdpType=%d",pdpType);
                    break;
                }

                LE_INFO("Stop data call slotId(%d) profileId(%d), pdpType=%d",
                        callCtxPtr->slotId, callCtxPtr->profileId, pdpType);
                dataConnection.StopSessionCmdSync(callCtxPtr->slotId, callCtxPtr->profileId,
                                                  pdpType, sessionRef);
                break;
            }
        }

        le_mutex_Lock(dataConnection.callCtxMutex);
        linkPtr = le_dls_PeekNext(&dataConnection.DataCallCtxList, linkPtr);
        le_mutex_Unlock(dataConnection.callCtxMutex);
    }

    return;
}

#ifdef TARGET_SA515M
void taf_DataConnection::onInitCompleted(telux::common::ServiceStatus status)
{
    std::lock_guard<std::mutex> lock(mtx);
    subSystemStatusUpdated = true;
    conVar.notify_all();
}

//--------------------------------------------------------------------------------------------------

/**
 * Mutex used to protect shared data structures in this module.
 */
//--------------------------------------------------------------------------------------------------

static pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;   // POSIX "Fast" mutex.

/// Locks the mutex.
#define LOCK    LE_ASSERT(pthread_mutex_lock(&Mutex) == 0);

/// Unlocks the mutex.
#define UNLOCK  LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0);

#define MAX_SLOT_NUM   2

static bool registered[MAX_SLOT_NUM] = {false};

//--------------------------------------------------------------------------------------------------

/**
 * Register listeners.
 */
//--------------------------------------------------------------------------------------------------
void RegisterListeners()
{

    LE_INFO("Registering listeners.");
    auto &dataConnection = taf_DataConnection::GetInstance();

    for(auto slotIdx = 1; slotIdx <= MAX_SLOT_NUM; slotIdx++)
    {
        LOCK
        if(registered[slotIdx-1])
        {
            LE_INFO("Listeners already registered.");
            UNLOCK
            continue;
        }

        if ( dataConnection.dataServingSystemManagers.find((SlotId)slotIdx) !=
                                                     dataConnection.dataServingSystemManagers.end())
        {
            if( dataConnection.dataServingSystemManagers[(SlotId)slotIdx]->registerListener(
                     dataConnection.dataServingSystemListeners[(SlotId)slotIdx]) ==
                                                                     telux::common::Status::SUCCESS)
            {
                LE_INFO("Serving system listener %d registered.", slotIdx);
                registered[slotIdx-1] = true;
            }
            else
            {
                LE_ERROR("Fail to register serving system listener %d.", slotIdx);
            }
        }

        UNLOCK
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Deregister listeners.
 */
//--------------------------------------------------------------------------------------------------

void DeregisterListeners()
{
    LE_INFO("Deregistering listeners.");
    auto &dataConnection = taf_DataConnection::GetInstance();

    for(auto slotIdx = 1; slotIdx <= MAX_SLOT_NUM; slotIdx++)
    {
        LOCK
        if(!registered[slotIdx-1])
        {
            LE_INFO("Listeners already deregistered.");
            UNLOCK
            continue;
        }

        if ( dataConnection.dataServingSystemManagers.find((SlotId)slotIdx) !=
                                                     dataConnection.dataServingSystemManagers.end())
        {
            if( dataConnection.dataServingSystemManagers[(SlotId)slotIdx]->deregisterListener(
                     dataConnection.dataServingSystemListeners[(SlotId)slotIdx]) ==
                                                                     telux::common::Status::SUCCESS)
            {
                LE_INFO("Serving system listener %d deregistered.", slotIdx);
                registered[slotIdx-1] = false;
            }
            else
            {
                LE_ERROR("Fail to deregister serving system listener %d.", slotIdx);
            }
        }

        UNLOCK
    }
}

void PowerStateChangeHandler(taf_pm_State_t state, void* contextPtr)
{
    if (state == TAF_PM_STATE_RESUME)
    {
        LE_INFO("Power state change to RESUME");
        RegisterListeners();
    }
    else if (state == TAF_PM_STATE_SUSPEND)
    {
        LE_INFO("Power state change to SUSPEND");
        DeregisterListeners();
    }

}

#endif

void taf_DataConnection::Init(void)
{
    auto &dataFactory = telux::data::DataFactory::getInstance();

#ifdef TARGET_SA515M

    int noOfSlots = MIN_SLOT_COUNT;
    if(telux::common::DeviceConfig::isMultiSimSupported())
    {
       noOfSlots = MAX_SLOT_COUNT;
       LE_INFO("MultiSim supported");
    }

    for(auto slotIdx = 1; slotIdx <= noOfSlots; slotIdx++)
    {
        telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
        subSystemStatusUpdated = false;
        auto initConnCb = std::bind(&taf_DataConnection::onInitCompleted, this,
                                    std::placeholders::_1);
        auto conneMgr = dataFactory.getDataConnectionManager((SlotId)slotIdx, initConnCb);
        bool subSysReady = false;

        if (conneMgr)
        {
            std::unique_lock<std::mutex> uLock(mtx);
            conVar.wait(uLock, [this]{return this->subSystemStatusUpdated;});
            subSystemStatus = conneMgr->getServiceStatus();

            if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
            {
                LE_INFO("Connection manager on slot %d is ready.", (int)slotIdx);
                subSysReady = true;
            }
            else
            {
                LE_ERROR("Connection manager on slot %d is not ready.", (int)slotIdx);
                subSysReady = false;
            }
            //If it is new manager and initialization passed
            if ( subSysReady &&
               (dataConnectionManagers.find((SlotId)slotIdx) == dataConnectionManagers.end()))
            {
               dataConnectionManagers.emplace((SlotId)slotIdx, conneMgr);
            }
        }
        else
        {
            LE_ERROR("Failed to get connection Manager instance ");
        }

        if(subSysReady)
        {
            LE_INFO("Data connection component is ready for slot %d...",(int)slotIdx);
        }
        else
        {
            LE_CRIT("Unable to init data connection component for slot %d !",(int)slotIdx);
        }

        /* register data connection status listener */
        DataConnectionListener = std::make_shared<taf_DataConnectionListener>();
        telux::common::Status status =  conneMgr->registerListener(DataConnectionListener);
        TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS,
                             "register listener failed, status: %d", (int32_t)status);

        /* register data serving system manager */
        connectionServingSystemlisteners[(SlotId)slotIdx] =
                              std::make_shared<taf_DataConnServingSystemListener>((SlotId)slotIdx);
        dataServingSystemListeners[(SlotId)slotIdx] =
                                                 connectionServingSystemlisteners[(SlotId)slotIdx];

        subSystemStatusUpdated = false;
            auto initSvrCb = std::bind(&taf_DataConnection::onInitCompleted, this,
                                       std::placeholders::_1);
            auto servingSystemMgr = dataFactory.getServingSystemManager((SlotId)slotIdx, initSvrCb);

        if (servingSystemMgr)
        {
            std::unique_lock<std::mutex> uLock(mtx);
            conVar.wait(uLock, [this]{return this->subSystemStatusUpdated;});
            subSystemStatus = servingSystemMgr->getServiceStatus();

            if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
            {
                LE_INFO("Serving system manager on slot %d is ready.", (int)slotIdx);
                subSysReady = true;
            }
            else
            {
                LE_ERROR("Serving system manager on slot %d is not ready.", (int)slotIdx);
                //If manager exist, deregister and remove it
                if (dataServingSystemManagers.find((SlotId)slotIdx) !=
                    dataServingSystemManagers.end())
                {
                    dataServingSystemManagers[(SlotId)slotIdx]->deregisterListener(
                                                    dataServingSystemListeners[(SlotId)slotIdx]);
                    dataServingSystemManagers.erase((SlotId)slotIdx);
                }
                subSysReady = false;
            }

            //If it is new manager and initialization passed
            if ( subSysReady &&
               (dataServingSystemManagers.find((SlotId)slotIdx) == dataServingSystemManagers.end()))
            {
                dataServingSystemManagers.emplace((SlotId)slotIdx, servingSystemMgr);
            }
        }
        else
        {
            LE_ERROR("Failed to get serving system Manager instance ");
        }

    }

    reqSvcStateCb = std::make_shared<taf_DataConnRequestServiceStatusCallback>();
    reqSvcStateCb->semaphore = le_sem_Create("taf_ConnectionReqSvcStateCbSem", 0);

    reqRoamingStatusCb = std::make_shared<taf_DataConnRequestRoamingStatusCallback>();
    reqRoamingStatusCb->semaphore = le_sem_Create("taf_ConnReqRoamingStatusCbSem", 0);

#else

    auto ConnectionMgr = dataFactory.getDataConnectionManager();

    bool isReady = ConnectionMgr->isSubsystemReady();
    if(isReady == false)
    {
        LE_INFO("data connection component is not ready, wait for it unconditionally...");
        std::future<bool> readyFunc = ConnectionMgr->onSubsystemReady();
        isReady = readyFunc.get();
    }

    if ( isReady &&
        (dataConnectionManagers.find((SlotId)SLOT_ID_1) == dataConnectionManagers.end()))
    {
        dataConnectionManagers.emplace((SlotId)SLOT_ID_1, ConnectionMgr);
    }
    else
    {
        LE_CRIT("unable to init data connection component!");
    }

    /* register data connection status listener */
    DataConnectionListener = std::make_shared<taf_DataConnectionListener>();
    telux::common::Status status =  ConnectionMgr->registerListener(DataConnectionListener);
    TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS,
                         "register listener failed, status: %d", (int32_t)status);

#endif

    RoamingStatusEvtId = le_event_CreateIdWithRefCounting("RoamingStatus");

    DataCallCtxPool = le_mem_InitStaticPool(tafDataCall, TAF_DCS_MAX_CALL_OBJ,
                                            sizeof(taf_dcs_CallCtx_t));
    // le_mem_SetDestructor(DataCallCtxPool, taf_Handler::ReleaseCallCtrlHandler);
    SessionRefPool = le_mem_InitStaticPool(tafSessionRef, TAF_DCS_MAX_SESSION_REF,
                                           sizeof(taf_SessionRef_t));
    HandlerSessionMappingPool = le_mem_InitStaticPool(HandlerSessionMappingPool,
                                                      TAF_DCS_MAX_ASYNC_HANDLER_MAPPING,
                                                      sizeof(HandlerSessionMapping_t));
    RoamingStatusPool = le_mem_InitStaticPool(RoamingStatusPool,
                                                      TAF_DCS_MAX_SESSION_REF,
                                                      sizeof(taf_dcs_RoamingStatusInd_t));

    DataCallRefMap = le_ref_CreateMap("Call Context Reference", TAF_DCS_MAX_CALL_OBJ);

    le_sem_Ref_t semRef = le_sem_Create("ConnThreadSem", 0);
    ConnectionEventThreadRef = le_thread_Create("DcsEvtThread", ConnectionEventThread,
                                                (void*)semRef);
    le_thread_Start(ConnectionEventThreadRef);
    le_sem_Wait(semRef);
    le_sem_Delete(semRef);

    le_msg_AddServiceCloseHandler(taf_dcs_GetServiceRef(), CloseEventHandler, NULL);
    callCtxMutex = le_mutex_CreateNonRecursive("callCtxMutex");
    handlerlistMutex = le_mutex_CreateNonRecursive("handlerlistMutex");

#ifdef TARGET_SA515M
    // Add power state change handler
    taf_pm_AddStateChangeHandler(PowerStateChangeHandler, NULL);
    if (taf_pm_GetPowerState() != TAF_PM_STATE_SUSPEND)
    {
        RegisterListeners();
    }
#endif

    return;
}
