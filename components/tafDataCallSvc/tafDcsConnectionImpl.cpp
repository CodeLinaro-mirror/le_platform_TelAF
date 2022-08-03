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

le_event_Id_t taf_DataConnection::connectionAsyncCmdEvId = nullptr;

#ifdef TARGET_SA515M
taf_DataConnServingSystemListener::taf_DataConnServingSystemListener(SlotId slot) :
   slotId(slot) {
}

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

void taf_DataConnRequestServiceStatusCallback::requestServiceStatus
(
    telux::data::ServiceStatus serviceStatus,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> taf_DataConnRequestServiceStatusCallback --> requestServiceStatus");

    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("Error(%d)", (int)error);
    }

    status = serviceStatus;
    le_sem_Post(semaphore);
}
#endif
void taf_DataConnectionListener::onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &iCall)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    TAF_ERROR_IF_RET_NIL(iCall == nullptr, "iCall is null");
    dataConnection.LogDataCallInfo(iCall, __func__);

    dataCallEvent_t callEvent;
    int32_t profileId = iCall->getProfileId();
    telux::data::DataCallStatus callStatus = iCall->getDataCallStatus();

    taf_dcs_CallCtx_t* callCtxPtr = dataConnection.GetCallCtx(profileId);
    TAF_ERROR_IF_RET_NIL(callCtxPtr == NULL, "cannot found call context, skip this profile[%d] & event[%s]",
        profileId, dataConnection.CallStatusToString(callStatus));

    callEvent.event         = EVT_STATUS_CHANGED;
    callEvent.profileId     = profileId;
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

void taf_DataConnection::LogDataCallInfo(const std::shared_ptr<telux::data::IDataCall> &dataCall, const char *fromPtr)
{
    int32_t profileId = dataCall->getProfileId();

    LE_DEBUG("data callback details from: %s", fromPtr);
    LE_DEBUG("profile id:           %d", profileId);
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
    LE_DEBUG("call end reason:      %s", CallEndReasonToString(dataCall->getDataCallEndReason().type));
    LE_DEBUG("tech preference:      %s", TechPreferenceToString(dataCall->getTechPreference()));
    LE_DEBUG("DataBearerTechnology: %s", DataBearerToString(dataCall->getCurrentBearerTech()));

    return;
}

void taf_DataConnection::StartDataCallCallback(const std::shared_ptr<telux::data::IDataCall> &iCall, telux::common::ErrorCode errorCode)
{
    dataCallEvent_t callEvent;
    auto &dataConnection = taf_DataConnection::GetInstance();
    int32_t profileId = iCall->getProfileId();

    dataConnection.LogDataCallInfo(iCall, __func__);

    if (errorCode != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("start data session is failed with profile: %d, error code: %d", profileId, (uint32_t)errorCode);
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
    callEvent.ipType        = iCall->getIpFamilyType();
    callEvent.ipv4Status    = iCall->getIpv4Info().status;
    callEvent.ipv6Status    = iCall->getIpv6Info().status;
    LE_DEBUG("event=%d,errcode=%d, callstatus=%s,profileId=%d,ipType=%d",
            (int)callEvent.event,(int)callEvent.errorCode,dataConnection.CallStatusToString(callEvent.callStatus),
            (int)callEvent.profileId,(int)callEvent.ipType);
    le_event_Report(dataConnection.CallEvent, &callEvent,sizeof(dataCallEvent_t));

    return;
}

void taf_DataConnection::StopDataCallCallback(const std::shared_ptr<telux::data::IDataCall> &iCall, telux::common::ErrorCode errorCode)
{
    dataCallEvent_t callEvent;
    auto &dataConnection = taf_DataConnection::GetInstance();
    //Check if iCall is a null pointer to avoid crashing
    TAF_ERROR_IF_RET_NIL(iCall == NULL, "iCall is NULL, drop this event");
    int32_t profileId = iCall->getProfileId();

    dataConnection.LogDataCallInfo(iCall, __func__);

    if (errorCode != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("stopping data call is failed with profile: %d, error code: %d", profileId, (uint32_t)errorCode);
    }

    LE_INFO("receiving StopDataCallCallback!");
    callEvent.event         = EVT_STOP_CALLBACK;
    callEvent.errorCode     = errorCode;
    callEvent.callStatus    = iCall->getDataCallStatus();
    callEvent.profileId     = iCall->getProfileId();
    callEvent.ipType        = iCall->getIpFamilyType();
    callEvent.ipv4Status    = iCall->getIpv4Info().status;
    callEvent.ipv6Status    = iCall->getIpv6Info().status;
    le_event_Report(dataConnection.CallEvent, &callEvent,sizeof(dataCallEvent_t));
    return;
}

void taf_DataConnection::SetDefaultProfileCallCallback(telux::common::ErrorCode errorCode)
{
    dataCallEvent_t callEvent;
    auto &dataConnection = taf_DataConnection::GetInstance();

    if (errorCode != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("setting default id is not finished, error code: %d", (uint32_t)errorCode);
    }

    callEvent.event         = EVT_SET_DEFAULT_PROFILE;
    callEvent.errorCode     = errorCode;
    le_event_Report(dataConnection.CallEvent, &callEvent,sizeof(dataCallEvent_t));

    return;
}

void taf_DataConnection::GetDefaultProfileCallCallback(int profileId, SlotId slotId, telux::common::ErrorCode errorCode)
{
    dataCallEvent_t callEvent;
    auto &dataConnection = taf_DataConnection::GetInstance();

    if (errorCode != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("getting default id is failed, error code: %d", (uint32_t)errorCode);
    }

    callEvent.event         = EVT_GET_DEFAULT_PROFILE;
    callEvent.errorCode     = errorCode;
    callEvent.profileId     = profileId;
    le_event_Report(dataConnection.CallEvent, &callEvent,sizeof(dataCallEvent_t));

    return;
}

taf_dcs_CallCtx_t* taf_DataConnection::CreateDataCallCtx(int32_t profileId)
{
    taf_dcs_CallCtx_t* callCtxPtr = NULL;
    char name[16] = {0};

    callCtxPtr = (taf_dcs_CallCtx_t*)le_mem_ForceAlloc(DataCallCtxPool);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, NULL, "cannot alloc callCtr");

    callCtxPtr->isInProgress = false;
    callCtxPtr->ipv4Status = telux::data::DataCallStatus::INVALID;
    callCtxPtr->ipv6Status = telux::data::DataCallStatus::INVALID;
    callCtxPtr->ipType = telux::data::IpFamilyType::UNKNOWN;
    callCtxPtr->profileId = profileId;
    callCtxPtr->sessionRefList = LE_DLS_LIST_INIT;
    callCtxPtr->link = LE_DLS_LINK_INIT;
    memset(callCtxPtr->intfName, 0, sizeof(callCtxPtr->intfName));
    le_dls_Queue(&DataCallCtxList, &callCtxPtr->link);
    callCtxPtr->callRef = (taf_dcs_CallRef_t)le_ref_CreateRef(DataCallRefMap, (void *)callCtxPtr);
    TAF_ERROR_IF_RET_VAL(callCtxPtr->callRef == NULL, NULL, "cannot alloc call reference");

    snprintf(name, sizeof(name)-1, "callCtx-%d", profileId);
    callCtxPtr->sessionStateEvent = le_event_CreateId(name, sizeof(DataCallState_t));

    return callCtxPtr;
}

le_result_t taf_DataConnection::IsProfileUsing(int32_t profileId, bool *isUsingPtr)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&DataCallCtxList);
    while (linkPtr)
    {
        taf_dcs_CallCtx_t* callCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_CallCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCallCtxList, linkPtr);
        if (callCtxPtr->profileId == profileId)
        {
            *isUsingPtr = callCtxPtr->isInProgress;
            return LE_OK;
        }
    }

    return LE_NOT_FOUND;
}

taf_dcs_CallCtx_t* taf_DataConnection::GetCallCtx(int32_t profileId)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&DataCallCtxList);
    while (linkPtr)
    {
        taf_dcs_CallCtx_t* callCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_CallCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCallCtxList, linkPtr);
        if (callCtxPtr->profileId == profileId)
        {
            return callCtxPtr;
        }
    }

    return NULL;
}

taf_dcs_CallCtx_t* taf_DataConnection::GetCallCtx(taf_dcs_CallRef_t reference)
{
    taf_dcs_CallCtx_t* callCtxPtr = (taf_dcs_CallCtx_t* )le_ref_Lookup(DataCallRefMap, (void*)reference);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, NULL, "cannot get callCtx from ref(%p)", reference);

    return callCtxPtr;
}

int32_t taf_DataConnection::GetProfileId(taf_dcs_CallRef_t reference)
{
    taf_dcs_CallCtx_t* callCtxPtr = GetCallCtx(reference);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "cannot get callCtx from ref(%p)", reference);
    return callCtxPtr->profileId;
}

le_result_t taf_DataConnection::GetConnectionState(int32_t profileId, taf_dcs_ConState_t* statePtr)
{
    TAF_ERROR_IF_RET_VAL(statePtr == NULL, LE_NOT_FOUND, "statePtr is NULL");

    taf_dcs_CallCtx_t* callCtxPtr = GetCallCtx(profileId);
    if (callCtxPtr == NULL)
    {
        LE_WARN("cannot found context from profile(%d), set to DISCONNECTED", profileId);
        *statePtr = TAF_DCS_DISCONNECTED;
        return LE_OK;
    }

    *statePtr = callCtxPtr->latestConState;
    return LE_OK;
}

le_result_t taf_DataConnection::SendSettingDefaultProfileIdCmd(int32_t profileId)
{
    telux::common::Status status = ConnectionMgr->setDefaultProfile(telux::data::OperationType::DATA_LOCAL, profileId, SetDefaultProfileCallCallback);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT, "start call failed, ret: %d", (int32_t)status);

    return LE_OK;
}

le_result_t taf_DataConnection::SendGettingDefaultProfileIdCmd()
{
#ifdef TARGET_SA515M
    telux::common::Status status = ConnectionMgr->getDefaultProfile(telux::data::OperationType::DATA_LOCAL, GetDefaultProfileCallCallback);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT, "start call failed, ret: %d", (int32_t)status);
#endif
    return LE_OK;
}

le_result_t taf_DataConnection::MakeCall(int32_t profileId, telux::data::IpFamilyType ipType)
{
    telux::common::Status status;
#ifdef TARGET_SA515M
    auto reqSvcStateCbFunc = std::bind(&taf_DataConnRequestServiceStatusCallback::requestServiceStatus, reqSvcStateCb, std::placeholders::_1, std::placeholders::_2);

    status = dataServingSystemManagers[(SlotId)SLOT_ID_1]->requestServiceStatus(reqSvcStateCbFunc);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT, "Call sdk function failed.");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(reqSvcStateCb->semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Wait semaphore timeout.");
    LE_INFO("Current data service status: %d.", (int)reqSvcStateCb->status.serviceState);
    if (reqSvcStateCb->status.serviceState != telux::data::DataServiceState::IN_SERVICE) {
        std::unique_lock<std::mutex> uLock(connectionServingSystemlisteners[(SlotId)SLOT_ID_1]->cv_mutex);
        connectionServingSystemlisteners[(SlotId)SLOT_ID_1]->conVar.wait_for(uLock, std::chrono::seconds(10));
        if (connectionServingSystemlisteners[(SlotId)SLOT_ID_1]->dsStatus != telux::data::DataServiceState::IN_SERVICE) {
            LE_ERROR("Wait for data in service time out.");
            return LE_FAULT;
        }
    }
#endif
    status = ConnectionMgr->startDataCall(profileId, ipType, StartDataCallCallback);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT, "start call failed, ret: %d", (int32_t)status);

    return LE_OK;
}

le_result_t taf_DataConnection::StopCall(int32_t profileId, telux::data::IpFamilyType ipType)
{
    telux::common::Status status;
#ifdef TARGET_SA515M
    auto reqSvcStateCbFunc = std::bind(&taf_DataConnRequestServiceStatusCallback::requestServiceStatus, reqSvcStateCb, std::placeholders::_1, std::placeholders::_2);

    status = dataServingSystemManagers[(SlotId)SLOT_ID_1]->requestServiceStatus(reqSvcStateCbFunc);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT, "Call sdk function failed.");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(reqSvcStateCb->semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Wait semaphore timeout.");
    LE_INFO("Current data service status: %d.", (int)reqSvcStateCb->status.serviceState);
    if (reqSvcStateCb->status.serviceState != telux::data::DataServiceState::IN_SERVICE) {
        std::unique_lock<std::mutex> uLock(connectionServingSystemlisteners[(SlotId)SLOT_ID_1]->cv_mutex);
        connectionServingSystemlisteners[(SlotId)SLOT_ID_1]->conVar.wait_for(uLock, std::chrono::seconds(10));
        if (connectionServingSystemlisteners[(SlotId)SLOT_ID_1]->dsStatus != telux::data::DataServiceState::IN_SERVICE) {
            LE_ERROR("Wait for data in service time out.");
            return LE_FAULT;
        }
    }
#endif
    status = ConnectionMgr->stopDataCall(profileId, ipType, StopDataCallCallback);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT, "stop call failed, ret: %d", (int32_t)status);

    return LE_OK;
}

le_result_t taf_DataConnection::IsCallCtxCreated(int32_t profileId, bool *isCreatedPtr)
{
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&DataCallCtxList);
    while (linkPtr)
    {
        taf_dcs_CallCtx_t* callCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_CallCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCallCtxList, linkPtr);
        if (callCtxPtr->profileId == profileId)
        {
            *isCreatedPtr = true;
            return LE_OK;
        }
    }

    *isCreatedPtr = false;
    return LE_OK;
}

le_result_t taf_DataConnection::AddSessionToCallCtx(taf_dcs_CallCtx_t* callCtxPtr, le_msg_SessionRef_t sessionRef)
{
    le_dls_Link_t* linkPtr = NULL;

    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_BAD_PARAMETER, "this call context is NULL");

    linkPtr = le_dls_Peek(&(callCtxPtr->sessionRefList));
    while (linkPtr)
    {
        taf_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkPtr, taf_SessionRef_t, link);
        linkPtr = le_dls_PeekNext(&(callCtxPtr->sessionRefList), linkPtr);

        if (sessionRefPtr->sessionRef == sessionRef)
        {
            LE_DEBUG("session(%p) has been added to callctx(profile %d)", sessionRef, callCtxPtr->profileId);
            return LE_DUPLICATE;
        }
    }

    taf_SessionRef_t* newSessionRefPtr = (taf_SessionRef_t *)le_mem_ForceAlloc(SessionRefPool);
    TAF_ERROR_IF_RET_VAL(newSessionRefPtr == NULL, LE_NO_MEMORY, "Cannot alloc mem for sessionRefNode");
    newSessionRefPtr->sessionRef = sessionRef;
    newSessionRefPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&callCtxPtr->sessionRefList, &(newSessionRefPtr->link));
    le_mem_AddRef(callCtxPtr);

    return LE_OK;
}

le_result_t taf_DataConnection::RemoveSessionFromCallCtx(taf_dcs_CallCtx_t* callCtxPtr, le_msg_SessionRef_t sessionRef)
{
    le_dls_Link_t* linkPtr = NULL;

    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_BAD_PARAMETER, "this call context is NULL");

    linkPtr = le_dls_Peek(&(callCtxPtr->sessionRefList));
    while (linkPtr)
    {
        taf_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkPtr, taf_SessionRef_t, link);
        linkPtr = le_dls_PeekNext(&(callCtxPtr->sessionRefList), linkPtr);

        if (sessionRefPtr->sessionRef == sessionRef)
        {
            le_dls_Remove(&(callCtxPtr->sessionRefList), &(sessionRefPtr->link));
            le_mem_Release(sessionRefPtr);
            return LE_OK;
        }
    }

    LE_ERROR("cannot found session context with ref(%p) from profile(%d)", sessionRef, callCtxPtr->profileId);
    return LE_NOT_FOUND;
}

le_result_t taf_DataConnection::StartSession(int32_t profileId, taf_dcs_Pdp_t pdpType, le_msg_SessionRef_t sessionRef)
{
    bool isUsing, isCreated;
    le_result_t ret;
    taf_dcs_CallCtx_t* callCtxPtr;

    TAF_ERROR_IF_RET_VAL(pdpType == TAF_DCS_PDP_UNKNOWN, LE_OUT_OF_RANGE, "pdpType type is unknown");

    ret = IsCallCtxCreated(profileId, &isCreated);
    TAF_ERROR_IF_RET_VAL(ret != LE_OK, LE_NOT_FOUND, "profile(%d) is invalid, ret: %d", profileId, ret);
    if (isCreated == true)
    {
        ret = IsProfileUsing(profileId, &isUsing);
        TAF_ERROR_IF_RET_VAL(ret != LE_OK, LE_NOT_FOUND, "profile(%d) is invalid, ret: %d", profileId, ret);
        TAF_ERROR_IF_RET_VAL(isUsing == true, LE_BUSY, "profile(%d) is in using(%d)", profileId, isUsing);
        callCtxPtr = GetCallCtx(profileId);
        TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "cannot get call context");
        ret = AddSessionToCallCtx(callCtxPtr, sessionRef);
        TAF_ERROR_IF_RET_VAL(!(ret == LE_OK || ret == LE_DUPLICATE), LE_NOT_FOUND, "addSessionToCallCtx return(%d) error", ret);
    }
    else
    {
        callCtxPtr = CreateDataCallCtx(profileId);
        TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "cannot create call context");
        ret = AddSessionToCallCtx(callCtxPtr, sessionRef);
        TAF_ERROR_IF_RET_VAL(!(ret == LE_OK || ret == LE_DUPLICATE), LE_NOT_FOUND, "addSessionToCallCtx return(%d) error", ret);
    }

    telux::data::IpFamilyType ipType = telux::data::IpFamilyType::IPV4V6;
    if (pdpType == TAF_DCS_PDP_IPV4)
    {
        ipType = telux::data::IpFamilyType::IPV4;
    }
    else if (pdpType == TAF_DCS_PDP_IPV6)
    {
        ipType = telux::data::IpFamilyType::IPV6;
    }
    ret = MakeCall(profileId, ipType);
    TAF_ERROR_IF_RET_VAL(ret != LE_OK, LE_FAULT, "making data call failed, ret: %d", ret);

    callCtxPtr = GetCallCtx(profileId);
    TAF_ERROR_IF_RET_VAL((callCtxPtr == NULL) || (callCtxPtr->callRef == NULL), LE_NOT_FOUND, "call context(%p) is invalid", callCtxPtr);

    return LE_OK;
}

le_result_t taf_DataConnection::StartSessionCmdSync(int32_t profileId, taf_dcs_Pdp_t pdpType, le_msg_SessionRef_t sessionRef)
{
    // initialize the synchronous promise
    CmdSynchronousPromise = std::promise<le_result_t>();

    le_result_t result = StartSession(profileId, pdpType, sessionRef);
    if (result != LE_OK)
    {
        LE_ERROR("start session is failed");
        return result;
    }

    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();

    return result;
}

le_result_t taf_DataConnection::StartSessionAllSync(int32_t profileId, taf_dcs_Pdp_t pdpType, le_msg_SessionRef_t sessionRef)
{
    // initialize the synchronous promise
    EventSynchronousPromise = std::promise<le_result_t>();
    std::chrono::seconds span(SESSION_TIMEOUT);
    taf_dcs_CallCtx_t* callCtxPtr;

    le_result_t result = StartSessionCmdSync(profileId, pdpType, sessionRef);
    if (result != LE_OK)
    {
        LE_ERROR("start synchronous session cmd is failed, result: %d", result);
        //if start session failed ,remove sessionRef from callCtxPtr
        callCtxPtr = GetCallCtx(profileId);
        RemoveSessionFromCallCtx(callCtxPtr,sessionRef);
        return result;
    }

    IsOnSynchronousAction = true;

    // blocking here to get call event response
    std::future<le_result_t> futResult = EventSynchronousPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("waiting promise timeout for %d seconds", SESSION_TIMEOUT);
        callCtxPtr = GetCallCtx(profileId);
        LE_INFO("Err profile[%d] for Type[%s] IPv4[%s] IPv6[%s]", profileId,
            IpFamilyTypeToString(callCtxPtr->ipType),
            CallStatusToString(callCtxPtr->ipv4Status),
            CallStatusToString(callCtxPtr->ipv6Status));
        result = LE_TIMEOUT;
    }
    else
    {
        result = futResult.get();
    }

    IsOnSynchronousAction = false;
    //If call status is NET_NO_NET(disconnected), telsdk will return a NULL pointer for the iCall
    //parameter of the StopDataCallCallback when invoke stopCall function.
    //Remove session since there is no need to call stopCall
    if(result == LE_OK)
    {
        callCtxPtr = GetCallCtx(profileId);
        if(callCtxPtr->callStatus == telux::data::DataCallStatus::NET_NO_NET)
        {
            LE_ERROR("callStatus is disconnected");
            RemoveSessionFromCallCtx(callCtxPtr, sessionRef);
            return LE_TERMINATED;
        }
    }
    LE_INFO("start synchronous session is done, result: %s", LE_RESULT_TXT(result));

    return result;
}

le_result_t taf_DataConnection::StopSession(int32_t profileId, taf_dcs_Pdp_t pdpType, le_msg_SessionRef_t sessionRef)
{
    taf_dcs_CallCtx_t* callCtxPtr;

    TAF_ERROR_IF_RET_VAL(pdpType == TAF_DCS_PDP_UNKNOWN, LE_OUT_OF_RANGE, "pdpType type is unknown");

    callCtxPtr = GetCallCtx(profileId);
    le_result_t result = RemoveSessionFromCallCtx(callCtxPtr, sessionRef);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "cannot remove session ref(%p) from this call ctx", sessionRef);

    size_t numLinks = le_dls_NumLinks(&callCtxPtr->sessionRefList);
    if (numLinks > 0)
    {
        LE_INFO("profile(%d) is used by (%d) clients, nothing to do in this operation", profileId, numLinks);
        //don't return LE_OK, if return LE_OK ,the caller StopSessionCmdSync will block, because CmdSynchronousPromise value is not set
        return LE_DUPLICATE;
    }

    telux::data::IpFamilyType ipType = telux::data::IpFamilyType::IPV4V6;
    if (pdpType == TAF_DCS_PDP_IPV4)
    {
        ipType = telux::data::IpFamilyType::IPV4;
    }
    else if (pdpType == TAF_DCS_PDP_IPV6)
    {
        ipType = telux::data::IpFamilyType::IPV6;
    }
    return StopCall(callCtxPtr->profileId, ipType);
}

le_result_t taf_DataConnection::StopSessionCmdSync(int32_t profileId, taf_dcs_Pdp_t pdpType, le_msg_SessionRef_t sessionRef)
{
    // initialize the synchronous promise
    CmdSynchronousPromise = std::promise<le_result_t>();

    le_result_t result = StopSession(profileId, pdpType, sessionRef);

    //When result is equal to LE_DUPLICATE, return OK and don't block
    if (result == LE_DUPLICATE)
        return LE_OK;
    else if (result != LE_OK)
    {
        LE_ERROR("stopping session command is failed, result: %d", result);
        return result;
    }

    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();

    return result;
}

le_result_t taf_DataConnection::StopSessionAllSync(int32_t profileId, taf_dcs_Pdp_t pdpType, le_msg_SessionRef_t sessionRef)
{
    // initialize the synchronous promise
    EventSynchronousPromise = std::promise<le_result_t>();
    std::chrono::seconds span(SESSION_TIMEOUT);

    le_result_t result = StopSessionCmdSync(profileId, pdpType, sessionRef);
    if (result != LE_OK)
    {
        LE_ERROR("stopping session command is failed, result: %d", result);
        return result;
    }

    IsOnSynchronousAction = true;

    // blocking here to get response
    std::future<le_result_t> futResult = EventSynchronousPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("waiting promise timeout");
        result = LE_TIMEOUT;
    }
    else
    {
        result = futResult.get();
    }

    IsOnSynchronousAction = false;
    LE_DEBUG("stop synchronous session is done, result: %s", LE_RESULT_TXT(result));

    return result;
}

le_result_t taf_DataConnection::SetDefaultProfileIdSync(uint32_t profileId)
{
    // initialize the synchronous promise
    CmdSynchronousPromise = std::promise<le_result_t>();

    le_result_t result = SendSettingDefaultProfileIdCmd(profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "setting default profile is failed, profile: %d", profileId);

    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();

    LE_INFO("setting default profile(%d) is finished, result: %d", profileId, result);

    return result;
}

le_result_t taf_DataConnection::GetDefaultProfileIdSync(uint32_t &profileId)
{
    // initialize the synchronous promise
    CmdSynchronousPromise = std::promise<le_result_t>();

    le_result_t result = SendGettingDefaultProfileIdCmd();
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "setting default profile is failed");

//In SA415M with old telsdk version, there is no getDefaultProfile function which will not set CmdSynchronousPromise value
#ifdef TARGET_SA515M
    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();
#else
    result = LE_OK;
#endif

    if (result == LE_OK)
    {
        profileId = DefaultProfileId;
    }
    else
    {
        LE_ERROR("getting default profile is failed, use default profile: %d", TAF_DCS_DEFAULT_PROFILE);
        profileId = TAF_DCS_DEFAULT_PROFILE;
    }

    return result;
}

le_result_t taf_DataConnection::GetProfileIdByInterfaceName(const char* namePtr,uint32_t* profileId)
{
    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_NOT_FOUND, "namePtr is null");
    TAF_ERROR_IF_RET_VAL(profileId == NULL, LE_NOT_FOUND, "profileId is null");

    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&DataCallCtxList);

    while (linkPtr)
    {
        taf_dcs_CallCtx_t* callCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_CallCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCallCtxList, linkPtr);

        if (strncmp(callCtxPtr->intfName,namePtr,TAF_DCS_NAME_MAX_LEN)== 0)
        {
            *profileId = callCtxPtr->profileId;
            return LE_OK;
        }
    }

    return LE_NOT_FOUND;
}

le_result_t taf_DataConnection::GetInterfaceName(int32_t profileId, char* namePtr, size_t nameSize)
{
    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_NOT_FOUND, "namePtr is null");
    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "cannot find call context from profile Id: %d", profileId);

    if ((callCtxPtr->isInProgress == true) &&
        ((callCtxPtr->ipv4Status == telux::data::DataCallStatus::NET_CONNECTED) ||
         (callCtxPtr->ipv6Status == telux::data::DataCallStatus::NET_CONNECTED)))
    {
        le_utf8_Copy(namePtr, callCtxPtr->intfName, nameSize, NULL);
        return LE_OK;
    }

    LE_ERROR("invalid connection status, inProgress: %d, ipv4: %s, ipv6: %s",
        callCtxPtr->isInProgress, CallStatusToString(callCtxPtr->ipv4Status), CallStatusToString(callCtxPtr->ipv6Status));
    return LE_NOT_POSSIBLE;
}

le_result_t taf_DataConnection::GetIpv4Address(int32_t profileId, char* addrPtr, size_t addrSize)
{
    TAF_ERROR_IF_RET_VAL(addrPtr == NULL, LE_NOT_FOUND, "addrPtr is null");
    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "cannot find call context from profile Id: %d", profileId);

    le_utf8_Copy(addrPtr, callCtxPtr->ipv4Addr, addrSize, NULL);

    return LE_OK;
}

le_result_t taf_DataConnection::GetIpv4Gateway(int32_t profileId, char* addrPtr, size_t addrSize)
{
    TAF_ERROR_IF_RET_VAL(addrPtr == NULL, LE_NOT_FOUND, "addrPtr is null");
    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "cannot find call context from profile Id: %d", profileId);

    le_utf8_Copy(addrPtr, callCtxPtr->ipv4Gw, addrSize, NULL);

    return LE_OK;
}

le_result_t taf_DataConnection::GetIpv4Dns(int32_t profileId, char* dns1Ptr, size_t dns1Size, char* dns2Ptr, size_t dns2Size)
{
    TAF_ERROR_IF_RET_VAL(dns1Ptr == NULL || dns2Ptr == NULL, LE_NOT_FOUND, "addrPtr is null");
    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "cannot find call context from profile Id: %d", profileId);

    le_utf8_Copy(dns1Ptr, callCtxPtr->ipv4Dns1, dns1Size, NULL);
    le_utf8_Copy(dns2Ptr, callCtxPtr->ipv4Dns2, dns2Size, NULL);

    return LE_OK;
}

le_result_t taf_DataConnection::GetIpv6Address(int32_t profileId, char* addrPtr, size_t addrSize)
{
    TAF_ERROR_IF_RET_VAL(addrPtr == NULL, LE_NOT_FOUND, "addrPtr is null");
    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "cannot find call context from profile Id: %d", profileId);

    le_utf8_Copy(addrPtr, callCtxPtr->ipv6Addr, addrSize, NULL);

    return LE_OK;
}

le_result_t taf_DataConnection::GetIpv6Gateway(int32_t profileId, char* addrPtr, size_t addrSize)
{
    TAF_ERROR_IF_RET_VAL(addrPtr == NULL, LE_NOT_FOUND, "addrPtr is null");
    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "cannot find call context from profile Id: %d", profileId);

    le_utf8_Copy(addrPtr, callCtxPtr->ipv6Gw, addrSize, NULL);

    return LE_OK;
}

le_result_t taf_DataConnection::GetIpv6Dns(int32_t profileId, char* dns1Ptr, size_t dns1Size, char* dns2Ptr, size_t dns2Size)
{
    TAF_ERROR_IF_RET_VAL(dns1Ptr == NULL || dns2Ptr == NULL, LE_NOT_FOUND, "addrPtr is null");
    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "cannot find call context from profile Id: %d", profileId);

    le_utf8_Copy(dns1Ptr, callCtxPtr->ipv6Dns1, dns1Size, NULL);
    le_utf8_Copy(dns2Ptr, callCtxPtr->ipv6Dns2, dns2Size, NULL);

    return LE_OK;
}

le_result_t taf_DataConnection::GetDataBearerTechnology(int32_t profileId, taf_dcs_DataBearerTechnology_t* downDataBearerTechPtr, taf_dcs_DataBearerTechnology_t* upDataBearerTechPtr)
{
    TAF_ERROR_IF_RET_VAL(downDataBearerTechPtr == NULL || upDataBearerTechPtr == NULL, LE_NOT_FOUND, "ptr is null");
    taf_dcs_CallCtx_t* callCtxPtr;

    callCtxPtr = GetCallCtx(profileId);
    if (callCtxPtr == NULL)
    {
        LE_ERROR("cannot found call context, use unknown data bearer");
        *downDataBearerTechPtr = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
        *upDataBearerTechPtr   = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
        return LE_OK;
    }

    *downDataBearerTechPtr = callCtxPtr->dataBearerTech;
    *upDataBearerTechPtr = callCtxPtr->dataBearerTech;
    return LE_OK;
}

le_event_Id_t taf_DataConnection::GetSessionStateEvent(int32_t profileId)
{
    taf_dcs_CallCtx_t* callCtxPtr;

    callCtxPtr = GetCallCtx(profileId);
    if (callCtxPtr == NULL)
    {
        callCtxPtr = CreateDataCallCtx(profileId);
    }
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, NULL, "cannot find call context from profile Id: %d", profileId);

    return callCtxPtr->sessionStateEvent;
}

le_result_t taf_DataConnection::SendStatusChangedNotification(taf_dcs_CallCtx_t *callCtxPtr, dataCallEvent_t *eventPtr)
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

taf_dcs_DataBearerTechnology_t taf_DataConnection::updateDataBearerTech(telux::data::DataBearerTechnology dataBearerTech)
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
            callCtxPtr->isInProgress = true;
            callCtxPtr->ipType     = eventPtr->ipType;
            isSendEvent = true;
        break;

        case telux::data::DataCallStatus::NET_CONNECTED:
            le_utf8_Copy(callCtxPtr->intfName, eventPtr->ifName.c_str(), sizeof(callCtxPtr->intfName), NULL);
            if (eventPtr->ipv4Status == telux::data::DataCallStatus::NET_CONNECTED)
            {
                le_utf8_Copy(callCtxPtr->ipv4Addr, eventPtr->ipv4AddrInfo.ifAddress.c_str(), TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv4Gw, eventPtr->ipv4AddrInfo.gwAddress.c_str(), TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv4Dns1, eventPtr->ipv4AddrInfo.primaryDnsAddress.c_str(), TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv4Dns2, eventPtr->ipv4AddrInfo.secondaryDnsAddress.c_str(), TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
            }

            if (eventPtr->ipv6Status == telux::data::DataCallStatus::NET_CONNECTED)
            {
                le_utf8_Copy(callCtxPtr->ipv6Addr, eventPtr->ipv6AddrInfo.ifAddress.c_str(), TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv6Gw, eventPtr->ipv6AddrInfo.gwAddress.c_str(), TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv6Dns1, eventPtr->ipv6AddrInfo.primaryDnsAddress.c_str(), TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv6Dns2, eventPtr->ipv6AddrInfo.secondaryDnsAddress.c_str(), TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
            }

            callCtxPtr->dataBearerTech = updateDataBearerTech(eventPtr->dataBearerTech);
            isSendEvent = true;
        break;

        case telux::data::DataCallStatus::NET_DISCONNECTING:
            isSendEvent = true;
        break;

        case telux::data::DataCallStatus::NET_NO_NET:
            callCtxPtr->isInProgress = false;
            memset(callCtxPtr->intfName, 0, sizeof(callCtxPtr->intfName));
            isSendEvent = true;
        break;

        default:
            LE_ERROR("cannot handle this event: %s", CallStatusToString(eventPtr->callStatus));
        break;

    }

    return isSendEvent;
}

taf_dcs_Pdp_t taf_DataConnection::GetEvtInfoFromConnStatus(taf_dcs_CallCtx_t *callCtxPtr, telux::data::DataCallStatus callStatus)
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
    le_result_t result = LE_OK;
    bool isSendNotification;
    dataCallEvent_t *eventPtr = (dataCallEvent_t *)reportPtr;
    int32_t profileId = eventPtr->profileId;
    taf_dcs_CallCtx_t *callCtxPtr;
    taf_dcs_StateInfo_t stateInfo = {TAF_DCS_PDP_UNKNOWN};

    switch (eventPtr->event)
    {
        // this event came from telsdk start_call callback handler
        case EVT_START_CALLBACK:
            callCtxPtr = GetCallCtx(profileId);
            TAF_ERROR_IF_RET_NIL(callCtxPtr == NULL, "cannot get call context from profile(%d)", profileId);
            if (eventPtr->errorCode != telux::common::ErrorCode::SUCCESS)
            {
                result = LE_FAULT;
            }
            else
            {
                isSendNotification = updateStatus(callCtxPtr, eventPtr);
                stateInfo.ipType = GetEvtInfoFromConnStatus(callCtxPtr, telux::data::DataCallStatus::NET_CONNECTING);
                TAF_ERROR_IF_RET_NIL(isSendNotification != true, "won't send notification to listener");
                SendNotificationStateEvent(TAF_DCS_CONNECTING, &stateInfo, callCtxPtr);
            }
            CmdSynchronousPromise.set_value(result);
            break;

        // this event came from telsdk stop_call callback handler
        case EVT_STOP_CALLBACK:
            callCtxPtr = GetCallCtx(profileId);
            TAF_ERROR_IF_RET_NIL(callCtxPtr == NULL, "cannot get call context from profile(%d)", profileId);
            if (eventPtr->errorCode != telux::common::ErrorCode::SUCCESS)
            {
                result = LE_FAULT;
            }
            else
            {
                isSendNotification = updateStatus(callCtxPtr, eventPtr);
                stateInfo.ipType = GetEvtInfoFromConnStatus(callCtxPtr, telux::data::DataCallStatus::NET_CONNECTING);
                TAF_ERROR_IF_RET_NIL(isSendNotification != true, "won't send notification to listener");
                SendNotificationStateEvent(TAF_DCS_DISCONNECTING, &stateInfo, callCtxPtr);
            }
            CmdSynchronousPromise.set_value(result);
        break;

        // the event from telsdk status changed handler
        case EVT_STATUS_CHANGED:
            callCtxPtr = GetCallCtx(profileId);
            TAF_ERROR_IF_RET_NIL(callCtxPtr == NULL, "cannot get call context from profile(%d)", profileId);
            isSendNotification = updateStatus(callCtxPtr, eventPtr);
            TAF_ERROR_IF_RET_NIL(isSendNotification != true, "won't send notification to listener");
            result = SendStatusChangedNotification(callCtxPtr, eventPtr);
            TAF_ERROR_IF_RET_NIL(result != LE_OK, "cannot sent notification, ret: %d", result);
        break;

        // the event from telsdk setting default profile callback
        case EVT_SET_DEFAULT_PROFILE:
            if ((eventPtr->errorCode == telux::common::ErrorCode::SUCCESS) || (eventPtr->errorCode == telux::common::ErrorCode::NO_EFFECT))
            {
                LE_INFO("setting profile finished, errorCode: %d", (int32_t)eventPtr->errorCode);
            }
            else
            {
                LE_ERROR("Setting profile is failed from callback, errorCode: %d", (uint32_t)eventPtr->errorCode);
                result = LE_FAULT;
            }
            CmdSynchronousPromise.set_value(result);
        break;

        // the event from telsdk getting default profile callback
        case EVT_GET_DEFAULT_PROFILE:
            if (eventPtr->errorCode != telux::common::ErrorCode::SUCCESS)
            {
                LE_ERROR("Setting profile is failed from callback, errorCode: %d", (uint32_t)eventPtr->errorCode);
                result = LE_FAULT;
            }
            else
            {
                DefaultProfileId = profileId;
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

void taf_DataConnection::SendNotificationStateEvent(taf_dcs_ConState_t conState, taf_dcs_StateInfo_t *infoPtr, taf_dcs_CallCtx_t *callCtxPtr)
{
    callCtxPtr->latestConState = conState;
    TAF_ERROR_IF_RET_NIL(SessionStateFunc == NULL, "SessionStateFunc is NULL, drop this event");
    LE_INFO("send connection status: %d", conState);
    SessionStateFunc(callCtxPtr->latestConState, infoPtr, callCtxPtr);

    // wakeup sync API
    if (((conState == TAF_DCS_CONNECTED) || (conState == TAF_DCS_DISCONNECTED)) && (IsOnSynchronousAction == true))
    {
        if ((callCtxPtr->ipType == telux::data::IpFamilyType::IPV4) || (callCtxPtr->ipType == telux::data::IpFamilyType::IPV6))
        {
            LE_INFO("promise for Type[%s] IPv4[%s] IPv6[%s]", IpFamilyTypeToString(callCtxPtr->ipType),
                CallStatusToString(callCtxPtr->ipv4Status),
                CallStatusToString(callCtxPtr->ipv6Status));
            EventSynchronousPromise.set_value(LE_OK);
        }
        else if (callCtxPtr->ipType == telux::data::IpFamilyType::IPV4V6)
        {
            if (((callCtxPtr->ipv4Status == telux::data::DataCallStatus::NET_CONNECTED) || (callCtxPtr->ipv4Status == telux::data::DataCallStatus::NET_NO_NET)) &&
                ((callCtxPtr->ipv6Status == telux::data::DataCallStatus::NET_CONNECTED) || (callCtxPtr->ipv6Status == telux::data::DataCallStatus::NET_NO_NET)))
            {
                LE_INFO("promise for Conn[%d] Type[%s] IPv4[%s] IPv6[%s]", conState, IpFamilyTypeToString(callCtxPtr->ipType),
                    CallStatusToString(callCtxPtr->ipv4Status),
                    CallStatusToString(callCtxPtr->ipv6Status));
                EventSynchronousPromise.set_value(LE_OK);
            }
            else
            {
                LE_INFO("no promise for Conn[%d] Type[%s] IPv4[%s] IPv6[%s]", conState, IpFamilyTypeToString(callCtxPtr->ipType),
                    CallStatusToString(callCtxPtr->ipv4Status),
                    CallStatusToString(callCtxPtr->ipv6Status));
            }
        }
        else
        {
            LE_ERROR("invalid IP type: %s", IpFamilyTypeToString(callCtxPtr->ipType));
        }
    }
    else
    {
        LE_INFO("no promise for Conn[%d] Type[%s] IPv4[%s] IPv6[%s]", conState, IpFamilyTypeToString(callCtxPtr->ipType),
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

bool taf_DataConnection::IsIpv4(int32_t profileId)
{
    taf_dcs_CallCtx_t* callCtxPtr = GetCallCtx(profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, false, "cannot get call context form profile(%d)", profileId);
    return (callCtxPtr->ipv4Status == telux::data::DataCallStatus::NET_CONNECTED);
}

bool taf_DataConnection::IsIpv6(int32_t profileId)
{
    taf_dcs_CallCtx_t* callCtxPtr = GetCallCtx(profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, false, "cannot get call context form profile(%d)", profileId);
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

/*======================================================================
 FUNCTION        taf_DataConnection::ConnectionProcAsyncCmdHandler
 DESCRIPTION     Asyncrous connection command handler.

 DEPENDENCIES    The initialization of data connection command thread.

 PARAMETERS      [IN] void* cmdReqPtr: Command request pointer.

 RETURN VALUE    None

======================================================================*/
void taf_DataConnection::ConnectionProcAsyncCmdHandler(void* cmdReqPtr)
{
    taf_ConnectionCmdReq_t* cmdReq = (taf_ConnectionCmdReq_t*)cmdReqPtr;
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();
    int32_t profileId;

    TAF_ERROR_IF_RET_NIL(cmdReqPtr == NULL, "Parameter is NULL");

    le_result_t result = dataProfile.GetProfileId(cmdReq->profileRef, &profileId);
    TAF_ERROR_IF_RET_NIL(result != LE_OK, "profile reference is invalid");

    taf_dcs_Pdp_t pdpType = dataProfile.GetPdp(cmdReq->profileRef);

    switch(cmdReq->cmdType)
    {
        case ASYNC_START_SESSION:
            LE_DEBUG("-ASYNC_START_SESSION-");
            result = dataConnection.StartSessionAllSync(profileId,
                                                        pdpType,
                                                        cmdReq->sessionRef);
            if (result != LE_OK)
            {
                LE_ERROR("StartSession error %d", result);
            }
        break;
        case ASYNC_STOP_SESSION:
            LE_DEBUG("-ASYNC_STOP_SESSION-");
            result = dataConnection.StopSessionAllSync(profileId,
                                                       pdpType,
                                                       cmdReq->sessionRef);
            if (result != LE_OK)
            {
                LE_ERROR("StopSession error %d", result);
            }
        break;
        default:
                LE_ERROR("Command error");
        break;
    }

    if (cmdReq->handlerFuncPtr)
    {
        // Check if handler is in the mapping list before calling an async handler
        HandlerSessionMapping_t *asyncHandlerDb =  dataConnection.FindAsyncHandler(
                                                                           cmdReq->handlerFuncPtr);
        if (!asyncHandlerDb)
        {
            LE_DEBUG("Don't call Async handler %p since the client session is already closed",
            cmdReq->handlerFuncPtr);
            //If a client session starts a data call and the client session is closed, stop the
            //data call
            if(cmdReq->cmdType == ASYNC_START_SESSION && result == LE_OK)
            {
                LE_DEBUG("--stop the data call since the client session is closed--");
                dataConnection.StopSessionCmdSync(profileId, pdpType, cmdReq->sessionRef);
            }
            return;
        }
        LE_DEBUG("Calling async handler %p with status %d", cmdReq->handlerFuncPtr, result);
        cmdReq->handlerFuncPtr(cmdReq->profileRef, result, cmdReq->contextPtr);
        dataConnection.DeleteHandlerInfo(asyncHandlerDb->asyncHandler);
    }
    else
    {
        LE_WARN("No handler function, result %d!!", result);
    }

}

/*======================================================================

 FUNCTION        taf_DataConnection::ConnectionAsyncCmdThread

 DESCRIPTION     Data connection command thread for handling asynchronous request.

 DEPENDENCIES    The initialization of DataConnection.

 PARAMETERS      [IN] void* contextPtr: Context pointer.

 RETURN VALUE    void*
                     NULL: Success.

======================================================================*/
void* taf_DataConnection::ConnectionAsyncCmdThread(void* contextPtr)
{
    le_event_AddHandler("ConnectionProcAsyncCmdHandler", connectionAsyncCmdEvId,
                                                         ConnectionProcAsyncCmdHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return nullptr;
}

void taf_DataConnection::AddHandlerSessionMapping
(
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
    handlerSessionMapping->asyncHandler = asyncHandler;
    handlerSessionMapping->sessionRef = sessionRef;
    handlerSessionMapping->handlerLink = LE_DLS_LINK_INIT;
    le_dls_Queue(&HandlerSessionMappingList, &handlerSessionMapping->handlerLink);

    LE_DEBUG("Added async handler %p for session reference %p", asyncHandler, sessionRef);
}

HandlerSessionMapping_t* taf_DataConnection::FindAsyncHandler
(
    taf_dcs_AsyncSessionHandlerFunc_t asyncHandler
)
{
    HandlerSessionMapping_t *handlerSessionInfo;
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&HandlerSessionMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, HandlerSessionMapping_t, handlerLink);
        if (handlerSessionInfo->asyncHandler == asyncHandler)
        {
            LE_DEBUG("Found async handler %p for session reference %p", asyncHandler,
                     handlerSessionInfo->sessionRef);
            return handlerSessionInfo;
        }
        handlerLinkPtr = le_dls_PeekNext(&HandlerSessionMappingList, handlerLinkPtr);
    }

    return NULL;
}

bool taf_DataConnection::IsSessionPresentInMappingList
(
    le_msg_SessionRef_t sessionRef
)
{
    HandlerSessionMapping_t *handlerSessionInfo;
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&HandlerSessionMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, HandlerSessionMapping_t, handlerLink);
        handlerLinkPtr = le_dls_PeekNext(&HandlerSessionMappingList, handlerLinkPtr);
        if (handlerSessionInfo->sessionRef == sessionRef)
        {
            return true;
        }
    }

    return false;
}

void taf_DataConnection::DeleteSessionHandlersInfo
(
    le_msg_SessionRef_t sessionRef
)
{
    HandlerSessionMapping_t *handlerSessionInfo;
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&HandlerSessionMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, HandlerSessionMapping_t, handlerLink);
        handlerLinkPtr = le_dls_PeekNext(&HandlerSessionMappingList, handlerLinkPtr);
        if (handlerSessionInfo->sessionRef == sessionRef)
        {
            le_dls_Remove(&HandlerSessionMappingList, &handlerSessionInfo->handlerLink);
            le_mem_Release(handlerSessionInfo);
        }
    }
}

void taf_DataConnection::DeleteHandlerInfo(taf_dcs_AsyncSessionHandlerFunc_t asyncHandler)
{
    HandlerSessionMapping_t *handlerSessionInfo;
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&HandlerSessionMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, HandlerSessionMapping_t, handlerLink);
        handlerLinkPtr = le_dls_PeekNext(&HandlerSessionMappingList, handlerLinkPtr);
        if (handlerSessionInfo->asyncHandler == asyncHandler)
        {
            le_dls_Remove(&HandlerSessionMappingList, &handlerSessionInfo->handlerLink);

            le_mem_Release(handlerSessionInfo);
            break;
        }
    }
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

    LE_DEBUG("SessionRef (%p) has been closed", sessionRef);

    // Find the data calls brought up by the sessionRef , and then stop them one by one
    linkPtr = le_dls_Peek(&dataConnection.DataCallCtxList);
    while (linkPtr)
    {
        taf_dcs_CallCtx_t* callCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_CallCtx_t, link);

        TAF_ERROR_IF_RET_NIL(callCtxPtr == NULL, "this call context is NULL");

        linkRefPtr = le_dls_Peek(&(callCtxPtr->sessionRefList));
        while (linkRefPtr)
        {
            taf_SessionRef_t* sessionRefPtr = CONTAINER_OF(linkRefPtr, taf_SessionRef_t, link);
            linkRefPtr = le_dls_PeekNext(&(callCtxPtr->sessionRefList), linkRefPtr);

            if (sessionRefPtr->sessionRef == sessionRef)
            {
                // If the client starts a data call with async api and loses connection before the
                // data call is completed, need to stop the data call after it is completed, see
                // function ConnectionProcAsyncCmdHandler()
                if(dataConnection.IsSessionPresentInMappingList(sessionRef))
                {
                    LE_DEBUG("---async api is called,don't stop data call here ---");
                    dataConnection.DeleteSessionHandlersInfo(sessionRef);
                }
                else
                {
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
                        profileRef = dataProfile.GetProfileRef(callCtxPtr->profileId);

                        pdpType = dataProfile.GetPdp(profileRef);

                        LE_DEBUG("---setting pdpType=%d",pdpType);
                        break;
                    }

                    LE_DEBUG("stop data call profileId=%d, pdpType=%d", callCtxPtr->profileId, pdpType);

                    dataConnection.StopSessionAllSync(callCtxPtr->profileId, pdpType, sessionRef);
                }

                break;
            }
        }

        linkPtr = le_dls_PeekNext(&dataConnection.DataCallCtxList, linkPtr);

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
#endif
void taf_DataConnection::Init(void)
{
    auto &dataFactory = DataFactory::getInstance();
    ConnectionMgr = dataFactory.getDataConnectionManager((SlotId)SLOT_ID_1);

    bool isReady = ConnectionMgr->isSubsystemReady();
    if(isReady == false)
    {
        LE_INFO("data connection component is not ready, wait for it unconditionally...");
        std::future<bool> readyFunc = ConnectionMgr->onSubsystemReady();
        isReady = readyFunc.get();
    }

    if(isReady)
    {
        LE_INFO("data connection component is ready...");
    }
    else
    {
        LE_CRIT("unable to init data connection component!");
    }

    /* register data connection status listener */
    DataConnectionListener = std::make_shared<taf_DataConnectionListener>();
    telux::common::Status status =  ConnectionMgr->registerListener(DataConnectionListener);
    TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS, "register listener failed, status: %d", (int32_t)status);
#ifdef TARGET_SA515M
    /* register data serving system manager */
    connectionServingSystemlisteners[(SlotId)SLOT_ID_1] = std::make_shared<taf_DataConnServingSystemListener>((SlotId)SLOT_ID_1);
    dataServingSystemListeners[(SlotId)SLOT_ID_1] = connectionServingSystemlisteners[(SlotId)SLOT_ID_1];

    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated = false;
    auto initCb = std::bind(&taf_DataConnection::onInitCompleted, this, std::placeholders::_1);
    auto servingSystemMgr = dataFactory.getServingSystemManager((SlotId)SLOT_ID_1, initCb);
    bool subSysReady = false;

    if (servingSystemMgr) {
        std::unique_lock<std::mutex> uLock(mtx);
        conVar.wait(uLock, [this]{return this->subSystemStatusUpdated;});
        subSystemStatus = servingSystemMgr->getServiceStatus();

        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LE_INFO("Serving system manager on slot %d is ready.", (int)SLOT_ID_1);
            subSysReady = true;
        } else {
            LE_ERROR("Serving system manager on slot %d is not ready.", (int)SLOT_ID_1);
            //If manager exist, deregister and remove it
            if (dataServingSystemManagers.find((SlotId)SLOT_ID_1) != dataServingSystemManagers.end()) {
                dataServingSystemManagers[(SlotId)SLOT_ID_1]->deregisterListener(dataServingSystemListeners[(SlotId)SLOT_ID_1]);
                dataServingSystemManagers.erase((SlotId)SLOT_ID_1);
            }
            subSysReady = false;
        }

        //If it is new manager and initialization passed
        if (subSysReady && (dataServingSystemManagers.find((SlotId)SLOT_ID_1) == dataServingSystemManagers.end())) {
            dataServingSystemManagers.emplace((SlotId)SLOT_ID_1, servingSystemMgr);
            dataServingSystemManagers[(SlotId)SLOT_ID_1]->registerListener(dataServingSystemListeners[(SlotId)SLOT_ID_1]);
        }
    }

    reqSvcStateCb = std::make_shared<taf_DataConnRequestServiceStatusCallback>();
    reqSvcStateCb->semaphore = le_sem_Create("taf_ConnectionReqSvcStateCbSem", 0);
#endif
    DataCallCtxPool = le_mem_InitStaticPool(tafDataCall, TAF_DCS_MAX_CALL_OBJ, sizeof(taf_dcs_CallCtx_t));
    // le_mem_SetDestructor(DataCallCtxPool, taf_Handler::ReleaseCallCtrlHandler);
    SessionRefPool = le_mem_InitStaticPool(tafSessionRef, TAF_DCS_MAX_SESSION_REF, sizeof(taf_SessionRef_t));
    HandlerSessionMappingPool = le_mem_InitStaticPool(HandlerSessionMappingPool,
                                                      TAF_DCS_MAX_ASYNC_HANDLER_MAPPING,
                                                      sizeof(HandlerSessionMapping_t));

    DataCallRefMap = le_ref_CreateMap("Call Context Reference", TAF_DCS_MAX_CALL_OBJ);

    le_sem_Ref_t semRef = le_sem_Create("ConnThreadSem", 0);
    ConnectionEventThreadRef = le_thread_Create("DcsConnThread", ConnectionEventThread, (void*)semRef);
    le_thread_Start(ConnectionEventThreadRef);
    le_sem_Wait(semRef);
    le_sem_Delete(semRef);

    // Create and start connection command thread.
    le_sem_Ref_t connectionCmdThreadSem = le_sem_Create("connectionCmdThreadSem", 0);
    connectionAsyncCmdEvId = le_event_CreateId("connectionCmd", sizeof(taf_ConnectionCmdReq_t));
    le_thread_Ref_t connectionCmdThreadRef = le_thread_Create("connectionCmdThread", ConnectionAsyncCmdThread, (void*)connectionCmdThreadSem);
    le_thread_Start(connectionCmdThreadRef);
    le_sem_Wait(connectionCmdThreadSem);
    le_sem_Delete(connectionCmdThreadSem);

    le_msg_AddServiceCloseHandler(taf_dcs_GetServiceRef(), CloseEventHandler, NULL);

    return;
}


