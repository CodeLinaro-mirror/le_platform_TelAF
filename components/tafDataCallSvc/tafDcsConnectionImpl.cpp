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
 *  Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>

using namespace telux::data;
using namespace telux::common;
using namespace tafsvc;

LE_MEM_DEFINE_STATIC_POOL(tafDataCall, TAF_DCS_MAX_CALL_OBJ, sizeof(taf_dcs_CallCtx_t));
LE_MEM_DEFINE_STATIC_POOL(tafSessionRef, TAF_DCS_MAX_SESSION_REF, sizeof(taf_SessionRef_t));
LE_MEM_DEFINE_STATIC_POOL(HandlerSessionMappingPool,
                                    TAF_DCS_MAX_ASYNC_HANDLER_MAPPING,
                                    sizeof(HandlerSessionMapping_t));
LE_MEM_DEFINE_STATIC_POOL(RoamingStatusPool, TAF_DCS_MAX_SESSION_REF,
                          sizeof(taf_dcs_RoamingStatusInd_t));

LE_MEM_DEFINE_STATIC_POOL(QosStatusPool, TAF_DCS_MAX_SESSION_REF,sizeof(QOSFlowCtxStatus_t));

LE_REF_DEFINE_STATIC_MAP(QosStatusRefMap, TAF_DCS_MAX_SESSION_REF);

taf_DataConnServingSystemListener::taf_DataConnServingSystemListener(SlotId slot) : slotId(slot) {}

void taf_DataConnServingSystemListener::onServiceStateChanged(telux::data::ServiceStatus status)
{
    std::lock_guard<std::mutex> lock(cv_mutex);
    LE_DEBUG("<SDK Listener> taf_DataConnServingSystemListener --> onServiceStateChanged");

    dsStatus = status.serviceState;
    LE_DEBUG("Status = %d", (int)dsStatus);
    LE_DEBUG("RAT    = %d", (int)status.networkRat);
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

// TelSDK RequestServiceStatusResponseCb
void taf_DataConnRequestServiceStatusCallback::requestServiceStatus(
    telux::data::ServiceStatus serviceStatus,
    telux::common::ErrorCode error)
{
    LE_DEBUG("<SDK Callback> taf_DataConnRequestServiceStatusCallback --> requestServiceStatus");
    LE_DEBUG("Error code       : %d", static_cast<int>(error));
    LE_DEBUG("DataServiceState : %d", static_cast<int>(serviceStatus.serviceState));
    LE_DEBUG("NetworkRat       : %d", static_cast<int>(serviceStatus.networkRat));
    errorCode = error;
    status = serviceStatus;
    le_sem_Post(semaphore);
}

taf_DataConnectionListener::taf_DataConnectionListener(SlotId slot) : slotId(slot) {}

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
                          slotId, profileId, taf_DCSHelper::CallStatusToString(callStatus));

    callEvent.event         = EVT_STATUS_CHANGED;
    callEvent.profileId     = profileId;
    callEvent.slotId        = slotId;
    callEvent.callStatus    = callStatus;
    callEvent.ipType        = iCall->getIpFamilyType();
    callEvent.ipv4Status    = iCall->getIpv4Info().status;
    callEvent.ipv6Status    = iCall->getIpv6Info().status;
    callEvent.maxRxBitRate  = 0;
    callEvent.maxTxBitRate  = 0;
    callEvent.callEndReasonIPv4.callEndReasonType = TAF_DCS_CE_TYPE_UNKNOWN;
    callEvent.callEndReasonIPv4.reasonInternal    = TAF_DCS_CE_INTERNAL_UNKNOWN;
    callEvent.callEndReasonIPv6.callEndReasonType = TAF_DCS_CE_TYPE_UNKNOWN;
    callEvent.callEndReasonIPv6.reasonInternal    = TAF_DCS_CE_INTERNAL_UNKNOWN;

    if (callEvent.ipv4Status == telux::data::DataCallStatus::NET_CONNECTED)
    {
        le_utf8_Copy(callEvent.ipv4AddrInfo.ifAddress, iCall->getIpv4Info().addr.ifAddress.c_str(),
                     TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);

        callEvent.ipv4AddrInfo.ifMask = iCall->getIpv4Info().addr.ifMask;

        le_utf8_Copy(callEvent.ipv4AddrInfo.gwAddress, iCall->getIpv4Info().addr.gwAddress.c_str(),
                     TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);

        callEvent.ipv4AddrInfo.gwMask = iCall->getIpv4Info().addr.gwMask;

        le_utf8_Copy(callEvent.ipv4AddrInfo.primaryDnsAddress,
                     iCall->getIpv4Info().addr.primaryDnsAddress.c_str(),
                     TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);

        le_utf8_Copy(callEvent.ipv4AddrInfo.secondaryDnsAddress,
                     iCall->getIpv4Info().addr.secondaryDnsAddress.c_str(),
                     TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
    }

    if (callEvent.ipv6Status == telux::data::DataCallStatus::NET_CONNECTED)
    {
        le_utf8_Copy(callEvent.ipv6AddrInfo.ifAddress, iCall->getIpv6Info().addr.ifAddress.c_str(),
                     TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);

        callEvent.ipv6AddrInfo.ifMask = iCall->getIpv6Info().addr.ifMask;

        le_utf8_Copy(callEvent.ipv6AddrInfo.gwAddress, iCall->getIpv6Info().addr.gwAddress.c_str(),
                     TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);

        callEvent.ipv6AddrInfo.gwMask = iCall->getIpv6Info().addr.gwMask;

        le_utf8_Copy(callEvent.ipv6AddrInfo.primaryDnsAddress,
                     iCall->getIpv6Info().addr.primaryDnsAddress.c_str(),
                     TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);

        le_utf8_Copy(callEvent.ipv6AddrInfo.secondaryDnsAddress,
                     iCall->getIpv6Info().addr.secondaryDnsAddress.c_str(),
                     TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);

    }

    // If this is a connected event, update the max Tx and Rx bit rates.
    if (telux::data::DataCallStatus::NET_CONNECTED == callEvent.callStatus ||
        telux::data::DataCallStatus::NET_CONNECTED == callEvent.ipv4Status ||
        telux::data::DataCallStatus::NET_CONNECTED == callEvent.ipv6Status)
    {
        LE_DEBUG("Get max data bit rate");
        // Promise and future used for synchronization.
        std::promise<bool> p;
        std::future<bool> f = p.get_future();

        // requestDataCallBitRate callback lambda
        auto respCb = [&callEvent, &p](telux::data::BitRateInfo &bitRate,
                                              telux::common::ErrorCode errorCode)
        {
            if (telux::common::ErrorCode::SUCCESS == errorCode)
            {
                // Success
                LE_DEBUG("maxRxRate: %" PRIu64 "", bitRate.maxRxRate);
                LE_DEBUG("maxTxRate: %" PRIu64 "", bitRate.maxTxRate);
                callEvent.maxRxBitRate = bitRate.maxRxRate;
                callEvent.maxTxBitRate = bitRate.maxTxRate;
                p.set_value(true);
            }
            else
            {
                LE_WARN("requestDataCallBitRateCb error: %d", static_cast<int>(errorCode));
                p.set_value(true);
            }
        };
        telux::common::Status status = iCall->requestDataCallBitRate(respCb);
        if (telux::common::Status::SUCCESS == status)
        {
            LE_DEBUG("requestDataCallBitRate SUCCESS. Wait for cbk");
            f.get();
        }
        else
        {
            LE_WARN("requestDataCallBitRate failed: %d", static_cast<int>(status));
        }
    }

    // If this is a data disconnected event, store the call end reason type and reason for IPv4.
    if (telux::data::DataCallStatus::NET_NO_NET == callEvent.ipv4Status ||
        telux::data::DataCallStatus::INVALID    == callEvent.ipv4Status )
    {
        telux::common::DataCallEndReason reason = iCall->getDataCallEndReason();
        callEvent.callEndReasonIPv4.callEndReasonType =
                                        taf_DCSHelper::ConvertCallEndReasonType(reason.type);

        switch (reason.type)
        {
        case telux::common::EndReasonType::CE_MOBILE_IP:
            callEvent.callEndReasonIPv4.reasonMIP =
                            taf_DCSHelper::ConvertCallEndMobileIpReasonCode(reason.IpCode);
            break;
        case telux::common::EndReasonType::CE_INTERNAL:
            callEvent.callEndReasonIPv4.reasonInternal =
                            taf_DCSHelper::ConvertCallEndInternalReasonCode(reason.internalCode);
            break;
        case telux::common::EndReasonType::CE_CALL_MANAGER_DEFINED:
            callEvent.callEndReasonIPv4.reasonCallManager =
                            taf_DCSHelper::ConvertCallEndCallManagerReasonCode(reason.cmCode);
            break;
        case telux::common::EndReasonType::CE_3GPP_SPEC_DEFINED:
            callEvent.callEndReasonIPv4.reasonSpec =
                            taf_DCSHelper::ConvertCallEnd3GPPSpecReasonCode(reason.specCode);
            break;
        case telux::common::EndReasonType::CE_PPP:
            callEvent.callEndReasonIPv4.reasonPPP =
                            taf_DCSHelper::ConvertCallEndPPPReasonCode(reason.pppCode);
            break;
        case telux::common::EndReasonType::CE_EHRPD:
            callEvent.callEndReasonIPv4.reasonEHRPD =
                            taf_DCSHelper::ConvertCallEndEHRPDReasonCode(reason.ehrpdCode);
            break;
        case telux::common::EndReasonType::CE_IPV6:
            callEvent.callEndReasonIPv4.reasonIPv6 =
                            taf_DCSHelper::ConvertCallEndIPv6ReasonCode(reason.ipv6Code);
            break;
        case telux::common::EndReasonType::CE_HANDOFF:
            callEvent.callEndReasonIPv4.reasonHandOff =
                            taf_DCSHelper::ConvertCallEndHandoffReasonCode(reason.handOffCode);
            break;
        default:
            LE_WARN("Invalid Reason code: %d", static_cast<int32_t>(reason.type));
            callEvent.callEndReasonIPv4.reasonInternal = TAF_DCS_CE_INTERNAL_UNKNOWN;
            break;
        }
    }

    // If this is a data disconnected event, store the call end reason type and reason for IPv6.
    if (telux::data::DataCallStatus::NET_NO_NET == callEvent.ipv6Status ||
        telux::data::DataCallStatus::INVALID    == callEvent.ipv6Status )
    {
        telux::common::DataCallEndReason reason = iCall->getDataCallEndReason();
        callEvent.callEndReasonIPv6.callEndReasonType =
                                        taf_DCSHelper::ConvertCallEndReasonType(reason.type);

        switch (reason.type)
        {
        case telux::common::EndReasonType::CE_MOBILE_IP:
            callEvent.callEndReasonIPv6.reasonMIP =
                            taf_DCSHelper::ConvertCallEndMobileIpReasonCode(reason.IpCode);
            break;
        case telux::common::EndReasonType::CE_INTERNAL:
            callEvent.callEndReasonIPv6.reasonInternal =
                            taf_DCSHelper::ConvertCallEndInternalReasonCode(reason.internalCode);
            break;
        case telux::common::EndReasonType::CE_CALL_MANAGER_DEFINED:
            callEvent.callEndReasonIPv6.reasonCallManager =
                            taf_DCSHelper::ConvertCallEndCallManagerReasonCode(reason.cmCode);
            break;
        case telux::common::EndReasonType::CE_3GPP_SPEC_DEFINED:
            callEvent.callEndReasonIPv6.reasonSpec =
                            taf_DCSHelper::ConvertCallEnd3GPPSpecReasonCode(reason.specCode);
            break;
        case telux::common::EndReasonType::CE_PPP:
            callEvent.callEndReasonIPv6.reasonPPP =
                            taf_DCSHelper::ConvertCallEndPPPReasonCode(reason.pppCode);
            break;
        case telux::common::EndReasonType::CE_EHRPD:
            callEvent.callEndReasonIPv6.reasonEHRPD =
                            taf_DCSHelper::ConvertCallEndEHRPDReasonCode(reason.ehrpdCode);
            break;
        case telux::common::EndReasonType::CE_IPV6:
            callEvent.callEndReasonIPv6.reasonIPv6 =
                            taf_DCSHelper::ConvertCallEndIPv6ReasonCode(reason.ipv6Code);
            break;
        case telux::common::EndReasonType::CE_HANDOFF:
            callEvent.callEndReasonIPv6.reasonHandOff =
                            taf_DCSHelper::ConvertCallEndHandoffReasonCode(reason.handOffCode);
            break;
        default:
            LE_WARN("Invalid Reason code: %d", static_cast<int32_t>(reason.type));
            callEvent.callEndReasonIPv6.reasonInternal = TAF_DCS_CE_INTERNAL_UNKNOWN;
            break;
        }
    }

    LE_DEBUG("maxRxBitRate: %" PRIu64 "", callEvent.maxRxBitRate);
    LE_DEBUG("maxTxBitRate: %" PRIu64 "", callEvent.maxTxBitRate);

    le_utf8_Copy(callEvent.ifName, iCall->getInterfaceName().c_str(), TAF_DCS_NAME_MAX_LEN, NULL);

    callEvent.dataBearerTech    = iCall->getCurrentBearerTech();

    le_event_Report(dataConnection.CallEvent, &callEvent, sizeof(dataCallEvent_t));
}

void taf_DataAPNThrottleInfoCallback::apnThrottleListResponse(
        const std::vector<telux::data::APNThrottleInfo> &throttleInfoList,
        telux::common::ErrorCode error)
{
    LE_DEBUG("<SDK Callback> taf_DataAPNThrottleInfoCallback --> apnThrottleListResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        result = LE_NOT_FOUND;
        le_sem_Post(semaphore);
        return;
    }

    LE_DEBUG("throttleInfoList size %d", (int)throttleInfoList.size());

    if(throttleInfoList.size() == 0)
    {
      result = LE_OK;
      LE_DEBUG("APN throttled is 0 hence return");
      throttleStatus.throttleState = false;
      throttleStatus.ipv4Time = 0;
      throttleStatus.ipv6Time = 0;
      le_sem_Post(semaphore);
      return;
    }

    auto &dataProfile = taf_DataProfile::GetInstance();

    // Traverse the throttleInfoList to find your slotId and profile ID from profile context
    // and store in cache
    for (auto throttleInfo : throttleInfoList)
    {
      for (int tProfId : throttleInfo.profileIds)
      {
        if(tProfId == throttleStatus.profileId)
        {
          //cache throttle info in profile context.
          // Absence of profile id in throttle info considered as the profile is not throttled
          taf_dcs_ProfileCtx_t* profileCtx = dataProfile.GetProfileCtx(throttleStatus.slotId,
                                                                 throttleStatus.profileId);
          if(profileCtx == NULL)
          {
            break;
          }
          profileCtx->throttleInfo.isThrottled = true;
          throttleStatus.throttleState = true;

          le_utf8_Copy(profileCtx->throttleInfo.mnc, throttleInfo.mnc.c_str(),
                                                     TAF_DCS_MNC_BYTES, NULL);
          le_utf8_Copy(profileCtx->throttleInfo.mcc, throttleInfo.mcc.c_str(),
                                                     TAF_DCS_MCC_BYTES, NULL);
          if (throttleInfo.isBlocked)
          {
            LE_DEBUG("APN blocked on all plmns slot %d Id %d", throttleStatus.slotId,tProfId);
            profileCtx->throttleInfo.isBlocked = throttleInfo.isBlocked;
          }
          throttleStatus.ipv4Time = throttleInfo.ipv4Time;
          throttleStatus.ipv6Time = throttleInfo.ipv6Time;
          break;
        }
      }
    }

    le_sem_Post(semaphore);
}


void taf_DataConnectionListener::onThrottledApnInfoChanged
(
const std::vector<telux::data::APNThrottleInfo>  &throttleInfoList
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();

    LE_DEBUG("<SDK Callback> taf_DataConnectionListener --> onThrottledApnInfoChanged");

    LE_INFO("Number of throttled APN: %d",(uint8_t)throttleInfoList.size());

    dataProfile.ProcessThrottledApnInfoChanged(throttleInfoList, slotId);
}
taf_dcs_HwAccelerationState_t taf_DataConnection::ConvertHwAccelSate(
    const telux::data::ServiceState state)
{
    // If active, return TAF_DCS_HW_ACCELERATION_ACTIVE
    if (telux::data::ServiceState::ACTIVE == state)
    {
        return TAF_DCS_HW_ACCELERATION_ACTIVE;
    }

    // Return TAF_DCS_HW_ACCELERATION_INACTIVE in all other cases
    return TAF_DCS_HW_ACCELERATION_INACTIVE;
}

void taf_DataConnectionListener::onHwAccelerationChanged(const telux::data::ServiceState state)
{
    LE_DEBUG("HW acceleration state: %d", static_cast<int>(state));
    // Send event to taf_DataProfile to handle this event
    HwAccelStatus_t event = {nullptr, TAF_DCS_HW_ACCELERATION_INACTIVE};
    auto &dataProfile = taf_DataProfile::GetInstance();
    event.state = taf_DataConnection::ConvertHwAccelSate(state);
    le_event_Report(dataProfile.GetHwAccelStatusEvent(), &event, sizeof(event));
}

taf_dcs_QosFlowBitMask_t taf_DataConnection::fillQosFlowMask(
    telux::data::QosFlowMask mask)
{
    taf_dcs_QosFlowBitMask_t qosFlowMask = 0x0;
    if (mask.test(telux::data::QosFlowMaskType::MASK_FLOW_NONE))
    {
        qosFlowMask = qosFlowMask | TAF_DCS_QOS_BIT_MASK_FLOW_NONE;
        LE_DEBUG("No QOS flow mask installed");
        return qosFlowMask;
    }
    if (mask.test(telux::data::QosFlowMaskType::MASK_FLOW_TX_GRANTED))
    {
        qosFlowMask = qosFlowMask | TAF_DCS_QOS_BIT_MASK_FLOW_TX_GRANTED;
        LE_DEBUG("MASK_FLOW_TX_GRANTED");
    }
    if (mask.test(telux::data::QosFlowMaskType::MASK_FLOW_RX_GRANTED))
    {
        qosFlowMask = qosFlowMask | TAF_DCS_QOS_BIT_MASK_FLOW_RX_GRANTED;
        LE_DEBUG("MASK_FLOW_RX_GRANTED");
    }
    if (mask.test(telux::data::QosFlowMaskType::MASK_FLOW_TX_FILTERS))
    {
        qosFlowMask = qosFlowMask | TAF_DCS_QOS_BIT_MASK_FLOW_TX_FILTERS;
        LE_DEBUG("MASK_FLOW_TX_FILTERS");
    }
    if (mask.test(telux::data::QosFlowMaskType::MASK_FLOW_RX_FILTERS))
    {
        qosFlowMask = qosFlowMask | TAF_DCS_QOS_BIT_MASK_FLOW_RX_FILTERS;
        LE_DEBUG("MASK_FLOW_RX_FILTERS");
    }
    return qosFlowMask;
}

void taf_DataConnection::fillQosDetails(std::shared_ptr<telux::data::TrafficFlowTemplate> &tft,
                                        telux::data::QosFlowStateChangeEvent stateChange,
                                        int32_t profileId, uint8_t slotId)
{
    LE_DEBUG(" QoS Flow Identifier : %d ", tft->qosId);
    LE_DEBUG(" QOS Flow Old State : %d ", static_cast<int>(tft->stateChange));

    switch (stateChange)
    {
    case telux::data::QosFlowStateChangeEvent::ACTIVATED:
    {
        LE_DEBUG("QOS FLOW ACTIVATED");

        taf_dcs_CallCtx_t *callCtxPtr = NULL;

        callCtxPtr = GetCallCtx(slotId, profileId);
        if (callCtxPtr == NULL)
        {
            LE_ERROR("Cannot find call context slotId(%d) profileId(%d)", slotId, profileId);
            return;
        }

        taf_dcs_QosFlowRef_t qosRef = callCtxPtr->qosFlowRef;
        if (qosRef)
        {
            // only 1 QOS ID is supported . TBD : multiple QOS IDs.
            LE_DEBUG("QOS ID already exists hence return");
            return;
        }
        QOSFlowCtxStatus_t *qosStatusPtr = NULL;
        qosStatusPtr = (QOSFlowCtxStatus_t *)le_mem_ForceAlloc(QosStatusPool);
        qosStatusPtr->profileId = profileId;
        qosStatusPtr->slotId = slotId;
        qosStatusPtr->qosID = tft->qosId;
        qosStatusPtr->qosState = TAF_DCS_QOS_ACTIVATED;
        qosStatusPtr->qosMask = fillQosFlowMask(tft->mask);
        qosRef = (taf_dcs_QosFlowRef_t)le_ref_CreateRef(QosStatusRefMap, (void *)qosStatusPtr);
        if (qosRef)
        {
            callCtxPtr->qosFlowRef = qosRef;
            // send event to client app with qosRef
            QOSFlowStatus_t stateEvent;
            stateEvent.profileId = profileId;
            stateEvent.slotId = slotId;
            stateEvent.qosID = tft->qosId;
            stateEvent.qosState = TAF_DCS_QOS_ACTIVATED;
            stateEvent.qosRef = qosRef;
            le_event_Report(callCtxPtr->qosStateEvent, &stateEvent, sizeof(stateEvent));
        }
    }
    break;
    case telux::data::QosFlowStateChangeEvent::MODIFIED:
        LE_DEBUG("QOS FLOW MODIFIED");
        break;
    case telux::data::QosFlowStateChangeEvent::DELETED:
    {
        LE_DEBUG("QOS FLOW DELETED");
        taf_dcs_CallCtx_t *callCtxPtr = NULL;

        callCtxPtr = GetCallCtx(slotId, profileId);
        if (callCtxPtr == NULL)
        {
            LE_ERROR("Cannot find call context slotId(%d) profileId(%d)", slotId, profileId);
            return;
        }
        taf_dcs_QosFlowRef_t qosRef = callCtxPtr->qosFlowRef;
        QOSFlowCtxStatus_t *qosStatus = (QOSFlowCtxStatus_t *)le_ref_Lookup(QosStatusRefMap,
                                                                            qosRef);

        if (qosStatus)
        {
            if (qosStatus->qosID == tft->qosId)
            {
                // send event to client app
                QOSFlowStatus_t stateEvent;
                stateEvent.profileId = profileId;
                stateEvent.slotId = slotId;
                stateEvent.qosID = tft->qosId;
                stateEvent.qosState = TAF_DCS_QOS_DELETED;
                stateEvent.qosRef = qosRef;

                le_event_Report(callCtxPtr->qosStateEvent, &stateEvent, sizeof(stateEvent));
                // delete the qosRef and the pointer associated
                le_ref_DeleteRef(QosStatusRefMap, qosRef);
                le_mem_Release(qosStatus);
                callCtxPtr->qosFlowRef = NULL;
            }
            else
            {
                LE_DEBUG("QOS ID Not found in Data call list");
                return;
            }
        }
        else
        {
            LE_DEBUG("QOS ID Not found in Data call list");
            return;
        }
    }
    break;
    default:
    {
        LE_DEBUG("UNKNOWN");
    }
    }

    LE_DEBUG("QosFlowMaskType Mask size %ld ", tft->mask.size());
    LE_DEBUG("QosFlowMaskType Mask size %s ", tft->mask.to_string().c_str());

    if (tft->mask.test(telux::data::QosFlowMaskType::MASK_FLOW_TX_GRANTED) &&
        (tft->txGrantedFlow.mask.test(
             telux::data::QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS) ||
         tft->txGrantedFlow.mask.test(
             telux::data::QosIPFlowMaskType::MASK_IP_FLOW_DATA_RATE_MIN_MAX)))
    {
        LE_DEBUG(" TX QOS FLow Granted: ");

        if (tft->txGrantedFlow.mask.test(
                telux::data::QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS))
        {
            LE_DEBUG("IP FLow Traffic class: ");
            // trafficClassToString(tft->txGrantedFlow.tfClass);
        }
    }

    if (tft->mask.test(telux::data::QosFlowMaskType::MASK_FLOW_RX_GRANTED) &&
        (tft->rxGrantedFlow.mask.test(
             telux::data::QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS) ||
         tft->rxGrantedFlow.mask.test(
             telux::data::QosIPFlowMaskType::MASK_IP_FLOW_DATA_RATE_MIN_MAX)))
    {
        LE_DEBUG(" RX QOS FLow Granted: ");

        if (tft->rxGrantedFlow.mask.test(
                telux::data::QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS))
        {
            LE_DEBUG("IP FLow Traffic class: ");
            // trafficClassToString(tft->rxGrantedFlow.tfClass);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the QOS flow Id.
 *
 * @instaging
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- QOS flow not found.
 *  - LE_OK -- Succeeded.
 */
le_result_t taf_DataConnection::GetQosID(taf_dcs_QosFlowRef_t qosFlowRef, uint32_t *qosFlowIdPtr)
{
    TAF_ERROR_IF_RET_VAL((qosFlowIdPtr == NULL), LE_BAD_PARAMETER, "qosFlowIdPtr is null");

    TAF_ERROR_IF_RET_VAL((qosFlowRef == NULL), LE_BAD_PARAMETER, "qosFlowRef is null");

    QOSFlowCtxStatus_t *qosStatus = (QOSFlowCtxStatus_t *)le_ref_Lookup(QosStatusRefMap,
                                                                        (void *)qosFlowRef);
    TAF_ERROR_IF_RET_VAL(qosStatus == NULL, LE_NOT_FOUND, "cannot get qos from ref(%p)",
                         qosFlowRef);

    *qosFlowIdPtr = qosStatus->qosID;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the details of the given QOS flow parameter mask.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- QOS flow not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DataConnection::GetQosMask(taf_dcs_QosFlowRef_t qosFlowRef,
                                           taf_dcs_QosFlowBitMask_t *qosFlowMaskPtr)
{
    TAF_ERROR_IF_RET_VAL((qosFlowMaskPtr == NULL), LE_BAD_PARAMETER, "qosFlowMaskPtr is null");

    TAF_ERROR_IF_RET_VAL((qosFlowRef == NULL), LE_BAD_PARAMETER, "qosFlowRef is null");

    QOSFlowCtxStatus_t *qosStatus = (QOSFlowCtxStatus_t *)le_ref_Lookup(QosStatusRefMap,
                                                                        (void *)qosFlowRef);
    TAF_ERROR_IF_RET_VAL(qosStatus == NULL, LE_NOT_FOUND, "cannot get qos from ref(%p)", qosFlowRef);

    *qosFlowMaskPtr = qosStatus->qosMask;

    return LE_OK;
}

/**
 * Mutex to protect data received via onTrafficFlowTemplateChange. This is declared globally here
 * instead of as a class variable because taf_DataConnectionListener can have 2 objects (multi sim)
 * and there could be a condition where the appropriate mutex is not locked.
 */
static std::mutex TftMtx;

/**
 * The QoS TFT callback implementation.
 */

void taf_DataConnectionListener::onTrafficFlowTemplateChange(
    const std::shared_ptr<telux::data::IDataCall> &iCall,
    const std::vector<std::shared_ptr<telux::data::TftChangeInfo>> &tfts)
{

    LE_DEBUG("<SDK Callback> taf_DataConnectionListener --> onTrafficFlowTemplateChange");
    TAF_ERROR_IF_RET_NIL(iCall == nullptr, "iCall is null");
    int32_t profileId = iCall->getProfileId();
    uint8_t slotId = (uint8_t)iCall->getSlotId();

    // Get the data connection object.
    auto &dataConnection = taf_DataConnection::GetInstance();

    // Lock the mutex
    std::lock_guard<std::mutex> lock(TftMtx);

    for (auto tft_iter : tfts)
    {
        LE_DEBUG(" ** TFT Details ** ");

        LE_DEBUG(" QOS Flow ID %d ", tft_iter->tft->qosId);
        LE_DEBUG(" QOS Flow New State %d ", static_cast<int>(tft_iter->stateChange));

        dataConnection.fillQosDetails(tft_iter->tft, tft_iter->stateChange, profileId, slotId);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns whether the APN is throttled or unthrottled.
 * It gets the remaining throttled time for IPv4 and IPv6 in milliseconds if isThrottled is true
 * otherwise returns 0.
 * If profile belongs to only one ipType then FFFF is returned for not supported ipType.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Failed.
 *  - LE_NOT_POSSIBLE -- Data profile is not created.
 *  - LE_UNAVAILABLE  -- Data profile is not throttled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_DataConnection::GetAPNThrottledStatus
(
  taf_dcs_ProfileRef_t    profileRef,
  bool         *isThrottled,
  uint32_t     *ipv4RemainingTime,
  uint32_t     *ipv6RemainingTime
)
{

    le_result_t result = LE_OK;
    int32_t profileId;
    uint8_t slotId;
    TAF_ERROR_IF_RET_VAL((profileRef == NULL) || (isThrottled == NULL) ||
                         (ipv4RemainingTime == NULL) || (ipv6RemainingTime == NULL),
    LE_BAD_PARAMETER, "some pointers may be null");

    auto &dataProfile = taf_DataProfile::GetInstance();

    auto reqAPNStatusCbFunc = std::bind(
                                    &taf_DataAPNThrottleInfoCallback::apnThrottleListResponse,
                                    reqAPNThrottlingStatusCb,
                                    std::placeholders::_1,
                                    std::placeholders::_2);

    result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(LE_OK != result,result,
                        "Unable to get slot Id and profile Id. result = %d", result);

    LE_DEBUG("Slot Id: %d, Profile Id: %d", slotId, profileId);

    // If the proifle ID is TAF_DCS_UNDEFINED_PROFILE_ID, it means the profile has not been created.
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId,LE_NOT_POSSIBLE,
                        "Profile has not been created yet.");

    reqAPNThrottlingStatusCb->throttleStatus.profileId = profileId;
    reqAPNThrottlingStatusCb->throttleStatus.slotId = slotId;
    reqAPNThrottlingStatusCb->throttleStatus.throttleState = 0;
    reqAPNThrottlingStatusCb->throttleStatus.ipv4Time = 0;
    reqAPNThrottlingStatusCb->throttleStatus.ipv6Time = 0;

    telux::common::Status status =
    dataConnectionManagers[(SlotId)slotId]->requestThrottledApnInfo(reqAPNStatusCbFunc);

    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT,
                         "Fail to get apn throttle info, ret: %d", (int32_t)status);

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(reqAPNThrottlingStatusCb->semaphore, timeToWait);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_TIMEOUT, "Wait semaphore timeout.");

    TAF_ERROR_IF_RET_VAL(reqAPNThrottlingStatusCb->result != LE_OK,
        reqAPNThrottlingStatusCb->result, "Fail to get apn throttle info.");

    *isThrottled = reqAPNThrottlingStatusCb->throttleStatus.throttleState;
    *ipv4RemainingTime = reqAPNThrottlingStatusCb->throttleStatus.ipv4Time;
    *ipv6RemainingTime = reqAPNThrottlingStatusCb->throttleStatus.ipv6Time;

    LE_DEBUG("GetAPNThrottledStatus ipv4 %d ipv6 %d", *ipv4RemainingTime,*ipv6RemainingTime);
    LE_DEBUG("GetAPNThrottledStatus isThrottled %d", *isThrottled);

    return result;
}

/**
 * Function to get the amx data bit rates.
 */
le_result_t taf_DataConnection::GetMaxDataBitRates(taf_dcs_ProfileRef_t profileRef,
                                                   uint64_t *maxRxBitRatePtr,
                                                   uint64_t *maxTxBitRatePtr)
{
    TAF_ERROR_IF_RET_VAL(NULL == profileRef,      LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(NULL == maxRxBitRatePtr, LE_BAD_PARAMETER, "maxRxBitRatePtr is NULL");
    TAF_ERROR_IF_RET_VAL(NULL == maxTxBitRatePtr, LE_BAD_PARAMETER, "maxTxBitRatePtr is NULL");

    auto &dataProfile = taf_DataProfile::GetInstance();
    int32_t profileId;
    uint8_t slotId;
    taf_dcs_CallCtx_t *callCtxPtr = NULL;
    le_result_t result;

    result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);
    if (LE_OK != result)
    {
        LE_ERROR("Unable to get slot Id and profile Id. result = %d", result);
        return result;
    }
    LE_DEBUG("Slot Id: %d, Profile Id: %d", slotId, profileId);

    // If the proifle ID is TAF_DCS_UNDEFINED_PROFILE_ID, it means the profile has not been created.
    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileId)
    {
        LE_ERROR("Profile has not been created yet.");
        return LE_NOT_POSSIBLE;
    }

    // Get the call context.
    callCtxPtr = GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND,
                        "Cannot get call context from slotId(%d) profileId(%d)", slotId, profileId);

    if (callCtxPtr->callStatus != telux::data::DataCallStatus::NET_CONNECTED &&
        callCtxPtr->callStatus != telux::data::DataCallStatus::NET_IDLE &&
        callCtxPtr->ipv4Status != telux::data::DataCallStatus::NET_CONNECTED &&
        callCtxPtr->ipv4Status != telux::data::DataCallStatus::NET_IDLE &&
        callCtxPtr->ipv6Status != telux::data::DataCallStatus::NET_CONNECTED &&
        callCtxPtr->ipv6Status != telux::data::DataCallStatus::NET_IDLE)
    {
        // Call is not connected. Return LE_UNAVAILABLE
        LE_WARN("Data call is not active for profile id %d", profileId);
        *maxTxBitRatePtr = 0;
        *maxRxBitRatePtr = 0;
        return LE_UNAVAILABLE;
    }

    // Get the max bit rates
    *maxRxBitRatePtr = callCtxPtr->maxRxBitRate;
    *maxTxBitRatePtr = callCtxPtr->maxTxBitRate;

    return LE_OK;
}

/**
 * Function to convert call end reason to int32_t. This is a helper to GetCallEndReason function.
 */
int32_t taf_DataConnection::ConvertCEReason(taf_dcs_callEndReason_t ceReason)
{
    switch (ceReason.callEndReasonType)
    {
    case TAF_DCS_CE_TYPE_UNKNOWN:
        LE_DEBUG("Unknown type");
        return TAF_DCS_CE_REASON_UNKNOWN;
    case TAF_DCS_CE_TYPE_MOBILE_IP:
        return static_cast<int32_t>(ceReason.reasonMIP);
    case TAF_DCS_CE_TYPE_INTERNAL:
        return static_cast<int32_t>(ceReason.reasonInternal);
    case TAF_DCS_CE_TYPE_CALL_MANAGER_DEFINED:
        return static_cast<int32_t>(ceReason.reasonCallManager);
    case TAF_DCS_CE_TYPE_3GPP_SPEC_DEFINED:
        return static_cast<int32_t>(ceReason.reasonSpec);
    case TAF_DCS_CE_TYPE_PPP:
        return static_cast<int32_t>(ceReason.reasonPPP);
    case TAF_DCS_CE_TYPE_EHRPD:
        return static_cast<int32_t>(ceReason.reasonEHRPD);
    case TAF_DCS_CE_TYPE_IPV6:
        return static_cast<int32_t>(ceReason.reasonIPv6);
    case TAF_DCS_CE_TYPE_HANDOFF:
        return static_cast<int32_t>(ceReason.reasonHandOff);
    default:
        LE_WARN("Invalid/Unknown reason type: %d",
                static_cast<int32_t>(ceReason.callEndReasonType));
        return TAF_DCS_CE_REASON_UNKNOWN;
    }
}

/**
 * Function to get call end reason.
 */
le_result_t taf_DataConnection::GetCallEndReason(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_Pdp_t pdpType,
    taf_dcs_CallEndReasonType_t *callEndReasonTypePtr,
    int32_t *callEndReasonPtr)
{
    TAF_ERROR_IF_RET_VAL(NULL == profileRef, LE_BAD_PARAMETER, "profileRef is NULL");
    TAF_ERROR_IF_RET_VAL(NULL == callEndReasonTypePtr, LE_BAD_PARAMETER,
                                                                "callEndReasonTypePtr is NULL");
    TAF_ERROR_IF_RET_VAL(NULL == callEndReasonPtr, LE_BAD_PARAMETER, "callEndReasonPtr is NULL");
    TAF_ERROR_IF_RET_VAL(TAF_DCS_PDP_UNKNOWN == pdpType, LE_BAD_PARAMETER, "pdpType is invalid");
    TAF_ERROR_IF_RET_VAL(TAF_DCS_PDP_IPV4V6 == pdpType, LE_BAD_PARAMETER, "Specify IPv4 or IPv6");

    auto &dataProfile = taf_DataProfile::GetInstance();
    int32_t profileId;
    uint8_t slotId;
    taf_dcs_CallCtx_t *callCtxPtr = NULL;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);
    if (LE_OK != result)
    {
        LE_ERROR("Unable to get slot Id and profile Id. result = %d", result);
        return result;
    }
    LE_DEBUG("Slot Id: %d, Profile Id: %d", slotId, profileId);

    // If the proifle ID is TAF_DCS_UNDEFINED_PROFILE_ID, it means the profile has not been created.
    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileId)
    {
        LE_ERROR("Profile has not been created yet.");
        return LE_NOT_POSSIBLE;
    }

    // Get the call context.
    callCtxPtr = GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND,
                        "Cannot get call context from slotId(%d) profileId(%d)", slotId, profileId);

    // Check if a data call has been setup yet.
    if (telux::data::DataCallStatus::INVALID == callCtxPtr->callStatus)
    {
        // The call end type is unknown. No data call has been setup yet. Return LE_UNAVAILABLE.
        LE_WARN("Data call has not been setup yet");
        return LE_UNAVAILABLE;
    }

    if (TAF_DCS_PDP_IPV4 == pdpType)
    {
        // Check if a IPv4 data call is active
        if (callCtxPtr->ipv4Status != telux::data::DataCallStatus::NET_NO_NET &&
            callCtxPtr->ipv4Status != telux::data::DataCallStatus::INVALID)
        {
            // Call is connected. Return LE_UNAVAILABLE
            LE_WARN("IPv4 call is active for profile id %d", profileId);
            return LE_UNAVAILABLE;
        }
        LE_DEBUG("IPv4 Reason type: %d", callCtxPtr->callEndReasonIPv4.callEndReasonType);
        // Assign the call end reason type
        *callEndReasonTypePtr = callCtxPtr->callEndReasonIPv4.callEndReasonType;
        *callEndReasonPtr     = ConvertCEReason(callCtxPtr->callEndReasonIPv4);
    }
    if (TAF_DCS_PDP_IPV6 == pdpType)
    {
        // Check if a IPv6 data call is active
        if (callCtxPtr->ipv6Status != telux::data::DataCallStatus::NET_NO_NET &&
            callCtxPtr->ipv6Status != telux::data::DataCallStatus::INVALID)
        {
            // Call is connected. Return LE_UNAVAILABLE
            LE_WARN("IPv6 call is active for profile id %d", profileId);
            return LE_UNAVAILABLE;
        }
        LE_DEBUG("IPv6 Reason type: %d", callCtxPtr->callEndReasonIPv6.callEndReasonType);
        // Assign the call end reason type
        *callEndReasonTypePtr = callCtxPtr->callEndReasonIPv6.callEndReasonType;
        *callEndReasonPtr     = ConvertCEReason(callCtxPtr->callEndReasonIPv6);
    }
    LE_DEBUG("Reason code: %d", *callEndReasonPtr);
    return LE_OK;
}

void taf_DataConnection::LogDataCallInfo
(
    const std::shared_ptr<telux::data::IDataCall> &dataCall,
     const char *fromPtr
)
{
    int32_t profileId = dataCall->getProfileId();
    uint8_t slotId = (uint8_t)dataCall->getSlotId();
    telux::data::DataCallStatus callStatus;

    LE_DEBUG("data callback details from: %s", fromPtr);
    LE_DEBUG("profile id:           %d", profileId);
    LE_DEBUG("slot id:           %d", slotId);
    LE_DEBUG("interface name:       %s", dataCall->getInterfaceName().c_str());
    callStatus = dataCall->getDataCallStatus();
    LE_DEBUG("call status:          %s", taf_DCSHelper::CallStatusToString(callStatus));
    LE_DEBUG("ip type:              %s", taf_DCSHelper::IpFamilyTypeToString(
                                                                dataCall->getIpFamilyType()));
    LE_DEBUG("ipv4 status:          %s", taf_DCSHelper::CallStatusToString(
                                                                dataCall->getIpv4Info().status));
    LE_DEBUG("ipv6 status:          %s", taf_DCSHelper::CallStatusToString(
                                                                dataCall->getIpv6Info().status));
    std::list<telux::data::IpAddrInfo> ipAddrList = dataCall->getIpAddressInfo();
    for(auto &it : ipAddrList) {
        LE_DEBUG("interface addr:       %s", it.ifAddress.c_str());
        LE_DEBUG("gateway   addr:       %s", it.gwAddress.c_str());
        LE_DEBUG("primary dns addr:     %s", it.primaryDnsAddress.c_str());
        LE_DEBUG("secondary dns addr:   %s", it.secondaryDnsAddress.c_str());
    }
    telux::common::DataCallEndReason reason = dataCall->getDataCallEndReason();
    LE_DEBUG("call end reason type:   %s", taf_DCSHelper::CallEndReasonTypeToString(reason.type));
    if ( telux::data::DataCallStatus::NET_NO_NET        == callStatus ||
         telux::data::DataCallStatus::NET_DISCONNECTING == callStatus )
    {
        switch (reason.type)
        {
        case telux::data::EndReasonType::CE_MOBILE_IP:
            LE_DEBUG("call end MIP reason code: %d(%s)",
                        static_cast<int32_t>(reason.IpCode),
                        taf_DCSHelper::CallEndMobileIpReasonCodeToString(
                            taf_DCSHelper::ConvertCallEndMobileIpReasonCode(reason.IpCode)));
            break;
        case telux::data::EndReasonType::CE_INTERNAL:
            LE_DEBUG("call end internal reason code: %d(%s)",
                     static_cast<int32_t>(reason.internalCode),
                     taf_DCSHelper::CallEndInternalReasonCodeToString(
                         taf_DCSHelper::ConvertCallEndInternalReasonCode(reason.internalCode)));
            break;
        case telux::data::EndReasonType::CE_CALL_MANAGER_DEFINED:
            LE_DEBUG("call end CM reason code: %d(%s)",
                     static_cast<int32_t>(reason.cmCode),
                     taf_DCSHelper::CallEndCallManagerReasonCodeToString(
                         taf_DCSHelper::ConvertCallEndCallManagerReasonCode(reason.cmCode)));
            break;
        case telux::data::EndReasonType::CE_3GPP_SPEC_DEFINED:
            LE_DEBUG("call end 3GPP spec reason code: %d(%s)",
                     static_cast<int32_t>(reason.specCode),
                     taf_DCSHelper::CallEnd3GPPSpecReasonCodeToString(
                         taf_DCSHelper::ConvertCallEnd3GPPSpecReasonCode(reason.specCode)));
            break;
        case telux::data::EndReasonType::CE_PPP:
            LE_DEBUG("call end PPP reason code: %d(%s)",
                     static_cast<int32_t>(reason.pppCode),
                     taf_DCSHelper::CallEndPPPReasonCodeToString(
                         taf_DCSHelper::ConvertCallEndPPPReasonCode(reason.pppCode)));
            break;
        case telux::data::EndReasonType::CE_EHRPD:
            LE_DEBUG("call end EHRPD reason code: %d(%s)",
                     static_cast<int32_t>(reason.ehrpdCode),
                     taf_DCSHelper::CallEndEHRPDReasonCodeToString(
                         taf_DCSHelper::ConvertCallEndEHRPDReasonCode(reason.ehrpdCode)));
            break;
        case telux::data::EndReasonType::CE_IPV6:
            LE_DEBUG("call end IPv6 reason code: %d(%s)",
                     static_cast<int32_t>(reason.ipv6Code),
                     taf_DCSHelper::CallEndIPv6ReasonCodeToString(
                         taf_DCSHelper::ConvertCallEndIPv6ReasonCode(reason.ipv6Code)));
            break;
        case telux::data::EndReasonType::CE_HANDOFF:
            LE_DEBUG("call end handodd reason code: %d(%s)",
                     static_cast<int32_t>(reason.handOffCode),
                     taf_DCSHelper::CallEndHandoffReasonCodeToString(
                         taf_DCSHelper::ConvertCallEndHandoffReasonCode(reason.handOffCode)));
            break;
        default:
            LE_DEBUG("Invalid Reason code: %d", static_cast<int32_t>(reason.type));
            break;
        }
    }

    LE_DEBUG("tech preference:      %s", taf_DCSHelper::TechPreferenceToString(
                                                            dataCall->getTechPreference()));
    LE_DEBUG("DataBearerTechnology: %s", taf_DCSHelper::DataBearerToString(
                                                            dataCall->getCurrentBearerTech()));

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
    if (callEvent.ipv4Status == telux::data::DataCallStatus::NET_CONNECTED)
    {
        le_utf8_Copy(callEvent.ipv4AddrInfo.ifAddress, iCall->getIpv4Info().addr.ifAddress.c_str(),
                     TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);

        callEvent.ipv4AddrInfo.ifMask = iCall->getIpv4Info().addr.ifMask;

        le_utf8_Copy(callEvent.ipv4AddrInfo.gwAddress, iCall->getIpv4Info().addr.gwAddress.c_str(),
                     TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);

        callEvent.ipv4AddrInfo.gwMask = iCall->getIpv4Info().addr.gwMask;

        le_utf8_Copy(callEvent.ipv4AddrInfo.primaryDnsAddress,
                     iCall->getIpv4Info().addr.primaryDnsAddress.c_str(),
                     TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);

        le_utf8_Copy(callEvent.ipv4AddrInfo.secondaryDnsAddress,
                     iCall->getIpv4Info().addr.secondaryDnsAddress.c_str(),
                     TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
    }

    callEvent.ipv6Status    = iCall->getIpv6Info().status;

    if (callEvent.ipv6Status == telux::data::DataCallStatus::NET_CONNECTED)
    {
        le_utf8_Copy(callEvent.ipv6AddrInfo.ifAddress, iCall->getIpv6Info().addr.ifAddress.c_str(),
                     TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);

        callEvent.ipv6AddrInfo.ifMask = iCall->getIpv6Info().addr.ifMask;

        le_utf8_Copy(callEvent.ipv6AddrInfo.gwAddress, iCall->getIpv6Info().addr.gwAddress.c_str(),
                     TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);

        callEvent.ipv6AddrInfo.gwMask = iCall->getIpv6Info().addr.gwMask;

        le_utf8_Copy(callEvent.ipv6AddrInfo.primaryDnsAddress,
                     iCall->getIpv6Info().addr.primaryDnsAddress.c_str(),
                     TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);

        le_utf8_Copy(callEvent.ipv6AddrInfo.secondaryDnsAddress,
                     iCall->getIpv6Info().addr.secondaryDnsAddress.c_str(),
                     TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
    }
    if (callEvent.callStatus == telux::data::DataCallStatus::NET_CONNECTED)
    {
        le_utf8_Copy(callEvent.ifName, iCall->getInterfaceName().c_str(), TAF_DCS_NAME_MAX_LEN, NULL);
    }
    callEvent.dataBearerTech    = iCall->getCurrentBearerTech();
    LE_DEBUG("ipv4 status=%s, ipv6 status=%s",
              taf_DCSHelper::CallStatusToString(callEvent.ipv4Status),
              taf_DCSHelper::CallStatusToString(callEvent.ipv6Status));
    LE_DEBUG("Start callback:event=%d,errcode=%d, callstatus=%s, slotId=%d, profileId=%d, ipType=%d",
             (int)callEvent.event, (int)callEvent.errorCode,
             taf_DCSHelper::CallStatusToString(callEvent.callStatus), callEvent.slotId,
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
    if (callEvent.ipv4Status == telux::data::DataCallStatus::NET_CONNECTED)
    {
        le_utf8_Copy(callEvent.ipv4AddrInfo.ifAddress, iCall->getIpv4Info().addr.ifAddress.c_str(),
                     TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);

        callEvent.ipv4AddrInfo.ifMask = iCall->getIpv4Info().addr.ifMask;

        le_utf8_Copy(callEvent.ipv4AddrInfo.gwAddress, iCall->getIpv4Info().addr.gwAddress.c_str(),
                     TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);

        callEvent.ipv4AddrInfo.gwMask = iCall->getIpv4Info().addr.gwMask;

        le_utf8_Copy(callEvent.ipv4AddrInfo.primaryDnsAddress,
                     iCall->getIpv4Info().addr.primaryDnsAddress.c_str(),
                     TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);

        le_utf8_Copy(callEvent.ipv4AddrInfo.secondaryDnsAddress,
                     iCall->getIpv4Info().addr.secondaryDnsAddress.c_str(),
                     TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
    }
    callEvent.ipv6Status    = iCall->getIpv6Info().status;
    if (callEvent.ipv6Status == telux::data::DataCallStatus::NET_CONNECTED)
    {
        le_utf8_Copy(callEvent.ipv6AddrInfo.ifAddress, iCall->getIpv6Info().addr.ifAddress.c_str(),
                     TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);

        callEvent.ipv6AddrInfo.ifMask = iCall->getIpv6Info().addr.ifMask;

        le_utf8_Copy(callEvent.ipv6AddrInfo.gwAddress, iCall->getIpv6Info().addr.gwAddress.c_str(),
                     TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);

        callEvent.ipv6AddrInfo.gwMask = iCall->getIpv6Info().addr.gwMask;

        le_utf8_Copy(callEvent.ipv6AddrInfo.primaryDnsAddress,
                     iCall->getIpv6Info().addr.primaryDnsAddress.c_str(),
                     TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);

        le_utf8_Copy(callEvent.ipv6AddrInfo.secondaryDnsAddress,
                     iCall->getIpv6Info().addr.secondaryDnsAddress.c_str(),
                     TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
    }
    if (callEvent.callStatus == telux::data::DataCallStatus::NET_CONNECTED)
    {
        le_utf8_Copy(callEvent.ifName, iCall->getInterfaceName().c_str(), TAF_DCS_NAME_MAX_LEN, NULL);
        callEvent.dataBearerTech    = iCall->getCurrentBearerTech();
    }
    LE_DEBUG("ipv4 status=%s, ipv6 status=%s",
              taf_DCSHelper::CallStatusToString(callEvent.ipv4Status),
              taf_DCSHelper::CallStatusToString(callEvent.ipv6Status));
    LE_DEBUG("stop callback:event=%d, errcode=%d, callstatus=%s, slotId=%d, profileId=%d, ipType=%d",
             (int)callEvent.event,(int)callEvent.errorCode,
            taf_DCSHelper::CallStatusToString(callEvent.callStatus), callEvent.slotId,
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
    callCtxPtr->callStatus = telux::data::DataCallStatus::INVALID;
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

    snprintf(name, sizeof(name)-1, "callQos-%d-%d", slotId, profileId);
    callCtxPtr->qosStateEvent = le_event_CreateId(name, sizeof(QOSFlowStatus_t));

    callCtxPtr->maxRxBitRate = 0;
    callCtxPtr->maxTxBitRate = 0;
    callCtxPtr->qosFlowRef = NULL;
    callCtxPtr->callEndReasonIPv4.callEndReasonType = TAF_DCS_CE_TYPE_UNKNOWN;
    callCtxPtr->callEndReasonIPv4.reasonInternal    = TAF_DCS_CE_INTERNAL_UNKNOWN;
    callCtxPtr->callEndReasonIPv6.callEndReasonType = TAF_DCS_CE_TYPE_UNKNOWN;
    callCtxPtr->callEndReasonIPv6.reasonInternal    = TAF_DCS_CE_INTERNAL_UNKNOWN;

    return callCtxPtr;
}

taf_dcs_CallCtx_t* taf_DataConnection::GetCallCtx(uint8_t slotId, int32_t profileId)
{
    le_dls_Link_t* linkPtr = NULL;
    LE_DEBUG("Slot ID: %d, Profile ID: %d", slotId, profileId);
    le_mutex_Lock(callCtxMutex);
    linkPtr = le_dls_Peek(&DataCallCtxList);
    while (linkPtr)
    {
        taf_dcs_CallCtx_t* callCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_CallCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCallCtxList, linkPtr);
        LE_DEBUG("Ctx: Slot ID: %d, Profile ID: %d", callCtxPtr->slotId, callCtxPtr->profileId);
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

le_result_t taf_DataConnection::SendGettingDefaultProfileIdCmd(uint8_t slotId)
{
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)

    if (dataConnectionManagers.find((SlotId)slotId) == dataConnectionManagers.end())
    {
        LE_ERROR("Connection manager is not init for slot %d", slotId);
        return LE_FAULT;
    }

    telux::common::Status status =
                          dataConnectionManagers[static_cast<SlotId>(slotId)]->getDefaultProfile(
                                                            telux::data::OperationType::DATA_LOCAL,
                                                            GetDefaultProfileCallCallback);

    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT,
                         "Getting default profile id failed, ret: %d", (int32_t)status);
#endif
    return LE_OK;
}


/**
 * @brief Builds a data-call response callback used for start/stop operations.
 *
 * @param slotId          - Slot associated with the data call (captured for logging/reporting).
 * @param profileId       - Data profile identifier used when initiating or stopping the call.
 * @param eventType       - Event type dispatched if the call object is missing or invalid.
 * @param successCallback - Handler invoked when TelSDK returns a valid IDataCall pointer.
 *
 * @return telux::data::DataCallResponseCb  A lambda safe for asynchronous invocation that
 *         reports failures through taf_DataConnection events and forwards successes
 *         to the supplied handler.
 */
template <typename CallbackFunc>
telux::data::DataCallResponseCb CreateStartStopDataCallCb
(
    uint8_t slotId,
    int32_t profileId,
    EventType_t eventType,
    CallbackFunc &&successCallback
)
{
    using CallbackT = typename std::decay<CallbackFunc>::type;
    auto cbHolder = std::make_shared<CallbackT>(std::forward<CallbackFunc>(successCallback));

    return [slotId, profileId, eventType, cbHolder]
    (
        const std::shared_ptr<telux::data::IDataCall> &iCall,
        telux::common::ErrorCode err
    )
    {
        auto &dc = taf_DataConnection::GetInstance();
        if (!dc.IsActive())
        {
            LE_WARN("Ignoring TelSDK callback for slotId(%d) profileId(%d) after deinit",
                slotId, profileId);
            return;
        }

        if (iCall == nullptr)
        {
            LE_WARN("NULL iCall for slotId(%d) profileId(%d), errorCode: %d",
                slotId, profileId, static_cast<int>(err));

            dataCallEvent_t callEvent{}; // value-initialized to zero all fields
            callEvent.event = eventType;
            callEvent.errorCode = telux::common::ErrorCode::SUCCESS;
            callEvent.profileId = profileId;
            callEvent.slotId = slotId;
            callEvent.callStatus = telux::data::DataCallStatus::NET_NO_NET;
            callEvent.ipType = telux::data::IpFamilyType::UNKNOWN;
            callEvent.ipv4Status = telux::data::DataCallStatus::NET_NO_NET;
            callEvent.ipv6Status = telux::data::DataCallStatus::NET_NO_NET;
            callEvent.dataBearerTech = telux::data::DataBearerTechnology::UNKNOWN;
            callEvent.maxRxBitRate = 0;
            callEvent.maxTxBitRate = 0;
            callEvent.callEndReasonIPv4.callEndReasonType = TAF_DCS_CE_TYPE_UNKNOWN;
            callEvent.callEndReasonIPv4.reasonInternal = TAF_DCS_CE_INTERNAL_UNKNOWN;
            callEvent.callEndReasonIPv6.callEndReasonType = TAF_DCS_CE_TYPE_UNKNOWN;
            callEvent.callEndReasonIPv6.reasonInternal = TAF_DCS_CE_INTERNAL_UNKNOWN;

            le_event_Report(dc.CallEvent, &callEvent, sizeof(dataCallEvent_t));
            return;
        }

        // Calling Start/Stop cb
        (*cbHolder)(iCall, err);
    };
}

le_result_t taf_DataConnection::MakeCall
(
    uint8_t slotId,
    int32_t profileId,
    telux::data::IpFamilyType ipType
)
{
    auto &dc = taf_DataConnection::GetInstance();
    if (!dc.IsActive())
    {
        LE_WARN("startDataCall ignored for slotId(%d) profileId(%d); taf_DataConnection inactive",
            slotId, profileId);
        return LE_FAULT;
    }

    std::shared_ptr<telux::data::IDataConnectionManager> manager;
    {
        std::lock_guard<std::mutex> lock(managersMutex_);
        const auto it = dataConnectionManagers.find(static_cast<SlotId>(slotId));
        if (it != dataConnectionManagers.end())
        {
            manager = it->second;
        }
    }

    if (!manager)
    {
        LE_ERROR("Connection manager is not initialized for slot %d", slotId);
        return LE_FAULT;
    }

    auto cb = CreateStartStopDataCallCb(slotId, profileId, EVT_START_CALLBACK,
        StartDataCallCallback);

    const auto status = manager->startDataCall(profileId, ipType, cb);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS,
                         LE_FAULT,
                         "Starting call failed for slot %d profile %d, ret: %d",
                         slotId,
                         profileId,
                         static_cast<int32_t>(status));
    LE_DEBUG("startDataCall was successful for slot %d profile %d", slotId, profileId);
    return LE_OK;
}

le_result_t taf_DataConnection::StopCall
(
    uint8_t slotId,
    int32_t profileId,
    telux::data::IpFamilyType ipType
)
{
    auto &dc = taf_DataConnection::GetInstance();
    if (!dc.IsActive())
    {
        LE_WARN("stopDataCall ignored for slotId(%d) profileId(%d); taf_DataConnection inactive",
            slotId, profileId);
        return LE_FAULT;
    }

    std::shared_ptr<telux::data::IDataConnectionManager> manager;
    {
        std::lock_guard<std::mutex> lock(managersMutex_);
        const auto it = dataConnectionManagers.find(static_cast<SlotId>(slotId));
        if (it != dataConnectionManagers.end())
        {
            manager = it->second;
        }
    }

    if (!manager)
    {
        LE_ERROR("Connection manager is not initialized for slot %d", slotId);
        return LE_FAULT;
    }

    auto cb = CreateStartStopDataCallCb(slotId, profileId, EVT_STOP_CALLBACK,
        StopDataCallCallback);

    const auto status = manager->stopDataCall(profileId, ipType, cb);
    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS,
                         LE_FAULT,
                         "Stopping call failed for slot %d profile %d, ret: %d",
                         slotId,
                         profileId,
                         static_cast<int32_t>(status));
    LE_DEBUG("stopDataCall was successful for slot %d profile %d", slotId, profileId);
    return LE_OK;
}

bool taf_DataConnection::IsCallCtxCreated(uint8_t slotId, int32_t profileId)
{
    le_dls_Link_t* linkPtr = NULL;
    LE_DEBUG("Slot ID: %d, Profile ID: %d", slotId, profileId);
    le_mutex_Lock(callCtxMutex);
    linkPtr = le_dls_Peek(&DataCallCtxList);

    while (linkPtr)
    {
        taf_dcs_CallCtx_t* callCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_CallCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCallCtxList, linkPtr);
        LE_DEBUG("Ctx: Slot ID: %d, Profile ID: %d", callCtxPtr->slotId, callCtxPtr->profileId);
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
    size_t numSessionRefs = 0;

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
            numSessionRefs = le_dls_NumLinks(&callCtxPtr->sessionRefList);
            LE_INFO("Slot Id: %d. Profile Id: %d, numSessionRefs=%" PRIuS, callCtxPtr->slotId,
                                                            callCtxPtr->profileId, numSessionRefs);
            pthread_mutex_unlock(&callCtxPtr->sessionListMutex);
            return LE_DUPLICATE;
        }
    }

    taf_SessionRef_t* newSessionRefPtr = (taf_SessionRef_t *)le_mem_ForceAlloc(SessionRefPool);

    newSessionRefPtr->sessionRef = sessionRef;
    newSessionRefPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&callCtxPtr->sessionRefList, &(newSessionRefPtr->link));
    le_mem_AddRef(callCtxPtr);

    numSessionRefs = le_dls_NumLinks(&callCtxPtr->sessionRefList);
    LE_INFO("Slot Id: %d. Profile Id: %d, numSessionRefs=%" PRIuS, callCtxPtr->slotId,
                                                        callCtxPtr->profileId, numSessionRefs);
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
    size_t numSessionRefs = 0;

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
            numSessionRefs = le_dls_NumLinks(&callCtxPtr->sessionRefList);
            LE_INFO("Slot Id: %d. Profile Id: %d, numSessionRefs=%" PRIuS, callCtxPtr->slotId,
                                                            callCtxPtr->profileId, numSessionRefs);
            pthread_mutex_unlock(&callCtxPtr->sessionListMutex);
            return LE_OK;
        }
    }

    LE_ERROR("Cannot find session context with ref(%p) from slotId(%d) profileId(%d)",
             sessionRef, callCtxPtr->slotId, callCtxPtr->profileId);
    numSessionRefs = le_dls_NumLinks(&callCtxPtr->sessionRefList);
    LE_INFO("Slot Id: %d. Profile Id: %d, numSessionRefs=%" PRIuS, callCtxPtr->slotId,
                                                            callCtxPtr->profileId, numSessionRefs);
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
        TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND, "cannot create call context");
        callCtxPtr->funcType = CALL_FUNCTION_SYNC_START;
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
                taf_DCSHelper::IpFamilyTypeToString(callCtxPtr->ipType),
                taf_DCSHelper::CallStatusToString(callCtxPtr->ipv4Status),
                taf_DCSHelper::CallStatusToString(callCtxPtr->ipv6Status));
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

    // If the proifle ID is TAF_DCS_UNDEFINED_PROFILE_ID, it means the profile has not been created.
    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileId)
    {
        LE_ERROR("Profile has not been created yet.");
        handlerPtr(profileRef, LE_NOT_POSSIBLE, contextPtr);
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
        // Check if call context can be created
        if (callCtxPtr == NULL)
        {
            LE_ERROR("cannot create call context");
            handlerPtr(profileRef, LE_FAULT, contextPtr);
            return;
        }

        callCtxPtr->funcType = CALL_FUNCTION_ASYNC_START;

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

    LE_INFO("numlink=%" PRIuS, numLinks);

    if (numLinks > 0)
    {
        LE_INFO("slotId(%d) profile(%d) is used by (%" PRIuS ") clients, nothing to do in this operation",
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

    // If the proifle ID is TAF_DCS_UNDEFINED_PROFILE_ID, it means the profile has not been created.
    if (TAF_DCS_UNDEFINED_PROFILE_ID == profileId)
    {
        LE_ERROR("Profile has not been created yet.");
        handlerPtr(profileRef, LE_NOT_POSSIBLE, contextPtr);
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

    LE_INFO("numlink=%" PRIuS, numLinks);

    // Check if more than one session uses this data connection
    if (numLinks > 0)
    {
        LE_INFO("slotId(%d) profile(%d) is used by (%" PRIuS ") clients, nothing to do in this operation",
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
    // Ensure slot id is valid
    if (bMultiSimSupported)
    {
        TAF_ERROR_IF_RET_VAL(slotId != SLOT_ID_1 && slotId != SLOT_ID_2,
                                                        LE_BAD_PARAMETER, "Invalid slot id.");
    }
    else
    {
        TAF_ERROR_IF_RET_VAL(slotId != SLOT_ID_1, LE_BAD_PARAMETER, "Invalid slot id.");
    }
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

// This function returns the default profile ID for the default slot ID 1
le_result_t taf_DataConnection::GetDefaultProfileIdSync(uint8_t *slotId, uint32_t *profileId)
{
    TAF_ERROR_IF_RET_VAL(slotId == NULL || profileId == NULL, LE_BAD_PARAMETER, "Null pointer");

    // initialize the synchronous promise
    CmdSynchronousPromise = std::promise<le_result_t>();

    le_result_t result = SendGettingDefaultProfileIdCmd(SLOT_ID_1);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Getting default profile failed");

// In SA415M with old telsdk version, there is no getDefaultProfile function which will not set
// CmdSynchronousPromise value
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();
#else
    result = LE_OK;
#endif

    if (result == LE_OK)
    {
        *profileId = DefaultProfileIdMap[SLOT_ID_1];
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

le_result_t taf_DataConnection::GetDefaultProfileIdForSlotIdSync
(
    uint8_t slotId,
    uint32_t& profileIdGet
)
{
    // Ensure slot id is valid
    if (bMultiSimSupported)
    {
        TAF_ERROR_IF_RET_VAL(slotId != SLOT_ID_1 && slotId != SLOT_ID_2,
                                                        LE_BAD_PARAMETER, "Invalid slot id.");
    }
    else
    {
        TAF_ERROR_IF_RET_VAL(slotId != SLOT_ID_1, LE_BAD_PARAMETER, "Invalid slot id.");
    }

    // initialize the synchronous promise
    CmdSynchronousPromise = std::promise<le_result_t>();

    le_result_t result = SendGettingDefaultProfileIdCmd(slotId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Getting default profile failed");
    // blocking here to get response
    std::future<le_result_t> futResult = CmdSynchronousPromise.get_future();
    result = futResult.get();
    if (result == LE_OK)
    {
        profileIdGet = DefaultProfileIdMap[slotId];
    }
    else
    {
        LE_ERROR("Getting default profile failed, set default profileId(%d)",
                                                                    TAF_DCS_DEFAULT_PROFILE);
        profileIdGet = TAF_DCS_DEFAULT_PROFILE;
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
    TAF_ERROR_IF_RET_VAL(namePtr[0] == '\0', LE_BAD_PARAMETER, "namePtr is empty");
    TAF_ERROR_IF_RET_VAL(slotId == NULL, LE_BAD_PARAMETER, "slotId is null");
    TAF_ERROR_IF_RET_VAL(profileId == NULL, LE_BAD_PARAMETER, "profileId is null");

    size_t inputLen = strnlen(namePtr, TAF_DCS_NAME_MAX_LEN);

    le_dls_Link_t* linkPtr = NULL;
    le_mutex_Lock(callCtxMutex);
    linkPtr = le_dls_Peek(&DataCallCtxList);
    while (linkPtr)
    {
        taf_dcs_CallCtx_t* callCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_CallCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCallCtxList, linkPtr);

        // Skip entries with empty or uninitialized interface name
        size_t storedLen = strnlen(callCtxPtr->intfName, TAF_DCS_NAME_MAX_LEN);
        if (storedLen == 0)
        {
            continue;
        }

        // Exact match: length and content must match
        if (storedLen == inputLen &&
            strncmp(callCtxPtr->intfName, namePtr, inputLen) == 0)
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

    LE_WARN("Invalid connection status, callstatus: %s, ipv4: %s, ipv6: %s",
             taf_DCSHelper::CallStatusToString(callCtxPtr->callStatus),
             taf_DCSHelper::CallStatusToString(callCtxPtr->ipv4Status),
             taf_DCSHelper::CallStatusToString(callCtxPtr->ipv6Status));
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

    if ((callCtxPtr->callStatus == telux::data::DataCallStatus::NET_CONNECTED) &&
        (callCtxPtr->ipv4Status == telux::data::DataCallStatus::NET_CONNECTED))
    {
        le_utf8_Copy(addrPtr, callCtxPtr->ipv4Addr, addrSize, NULL);
        return LE_OK;
    }

    LE_WARN("Invalid connection status, callstatus: %s, ipv4: %s",
             taf_DCSHelper::CallStatusToString(callCtxPtr->callStatus),
             taf_DCSHelper::CallStatusToString(callCtxPtr->ipv4Status));

    return LE_UNAVAILABLE;
}

le_result_t taf_DataConnection::GetIpv4SubnetMask
(
    uint8_t slotId,
    int32_t profileId,
    uint32_t* mask
)
{
    TAF_ERROR_IF_RET_VAL(mask == NULL, LE_BAD_PARAMETER, "mask is null");

    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND,
                         "Cannot find call context from slotId(%d) profileId(%d)",
                         slotId, profileId);

    if ((callCtxPtr->callStatus == telux::data::DataCallStatus::NET_CONNECTED) &&
        (callCtxPtr->ipv4Status == telux::data::DataCallStatus::NET_CONNECTED))
    {
        *mask = callCtxPtr->ipv4Mask;
        return LE_OK;
    }

    LE_WARN("Invalid connection status, callstatus: %s, ipv4: %s",
             taf_DCSHelper::CallStatusToString(callCtxPtr->callStatus),
             taf_DCSHelper::CallStatusToString(callCtxPtr->ipv4Status));

    return LE_UNAVAILABLE;
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

    if ((callCtxPtr->callStatus == telux::data::DataCallStatus::NET_CONNECTED) &&
        (callCtxPtr->ipv4Status == telux::data::DataCallStatus::NET_CONNECTED))
    {
        le_utf8_Copy(addrPtr, callCtxPtr->ipv4Gw, addrSize, NULL);
        return LE_OK;
    }

    LE_WARN("Invalid connection status, callstatus: %s, ipv4: %s",
             taf_DCSHelper::CallStatusToString(callCtxPtr->callStatus),
             taf_DCSHelper::CallStatusToString(callCtxPtr->ipv4Status));

    return LE_UNAVAILABLE;
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


    if ((callCtxPtr->callStatus == telux::data::DataCallStatus::NET_CONNECTED) &&
        (callCtxPtr->ipv4Status == telux::data::DataCallStatus::NET_CONNECTED))
    {
        le_utf8_Copy(dns1Ptr, callCtxPtr->ipv4Dns1, dns1Size, NULL);
        le_utf8_Copy(dns2Ptr, callCtxPtr->ipv4Dns2, dns2Size, NULL);

        return LE_OK;
    }

    LE_WARN("Invalid connection status, callstatus: %s, ipv4: %s",
             taf_DCSHelper::CallStatusToString(callCtxPtr->callStatus),
             taf_DCSHelper::CallStatusToString(callCtxPtr->ipv4Status));

    return LE_UNAVAILABLE;
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

    if ((callCtxPtr->callStatus == telux::data::DataCallStatus::NET_CONNECTED) &&
        (callCtxPtr->ipv6Status == telux::data::DataCallStatus::NET_CONNECTED))
    {
        le_utf8_Copy(addrPtr, callCtxPtr->ipv6Addr, addrSize, NULL);
        return LE_OK;
    }

    LE_WARN("Invalid connection status, callstatus: %s, ipv6: %s",
             taf_DCSHelper::CallStatusToString(callCtxPtr->callStatus),
             taf_DCSHelper::CallStatusToString(callCtxPtr->ipv6Status));

    return LE_UNAVAILABLE;
}

le_result_t taf_DataConnection::GetIpv6SubnetMask
(
    uint8_t slotId,
    int32_t profileId,
    uint32_t* mask
)
{
    TAF_ERROR_IF_RET_VAL(mask == NULL, LE_BAD_PARAMETER, "mask is null");

    taf_dcs_CallCtx_t* callCtxPtr;
    callCtxPtr = GetCallCtx(slotId, profileId);
    TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, LE_NOT_FOUND,
                         "Cannot find call context slotId(%d) profileId(%d)", slotId, profileId);

    if ((callCtxPtr->callStatus == telux::data::DataCallStatus::NET_CONNECTED) &&
        (callCtxPtr->ipv6Status == telux::data::DataCallStatus::NET_CONNECTED))
    {
        *mask = callCtxPtr->ipv6Mask;
        return LE_OK;
    }

    LE_WARN("Invalid connection status, callstatus: %s, ipv6: %s",
             taf_DCSHelper::CallStatusToString(callCtxPtr->callStatus),
             taf_DCSHelper::CallStatusToString(callCtxPtr->ipv6Status));

    return LE_UNAVAILABLE;
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
    if ((callCtxPtr->callStatus == telux::data::DataCallStatus::NET_CONNECTED) &&
        (callCtxPtr->ipv6Status == telux::data::DataCallStatus::NET_CONNECTED))
    {
        le_utf8_Copy(addrPtr, callCtxPtr->ipv6Gw, addrSize, NULL);
        return LE_OK;
    }

    LE_WARN("Invalid connection status, callstatus: %s, ipv6: %s",
             taf_DCSHelper::CallStatusToString(callCtxPtr->callStatus),
             taf_DCSHelper::CallStatusToString(callCtxPtr->ipv6Status));

    return LE_UNAVAILABLE;

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

    if ((callCtxPtr->callStatus == telux::data::DataCallStatus::NET_CONNECTED) &&
        (callCtxPtr->ipv6Status == telux::data::DataCallStatus::NET_CONNECTED))
    {
        le_utf8_Copy(dns1Ptr, callCtxPtr->ipv6Dns1, dns1Size, NULL);
        le_utf8_Copy(dns2Ptr, callCtxPtr->ipv6Dns2, dns2Size, NULL);

        return LE_OK;
    }

    LE_WARN("Invalid connection status, callstatus: %s, ipv6: %s",
             taf_DCSHelper::CallStatusToString(callCtxPtr->callStatus),
             taf_DCSHelper::CallStatusToString(callCtxPtr->ipv6Status));

    return LE_UNAVAILABLE;
}

le_result_t taf_DataConnection::GetMtu
(
    uint8_t slotId,
    int32_t profileId,
    uint16_t *mtuPtr
)
{
    struct ifreq ifr;
    int8_t sock;
    le_result_t result;
    char interfaceName[TAF_DCS_NAME_MAX_BYTES];

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    TAF_ERROR_IF_RET_VAL(sock < 0, LE_FAULT,"socket error %d",sock);

    memset(&ifr, 0, sizeof(struct ifreq));
    result = GetInterfaceName(slotId, profileId, interfaceName, sizeof(interfaceName));
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "GetInterfaceName error %d",result);

    result = le_utf8_Copy(ifr.ifr_name,interfaceName, sizeof(ifr.ifr_name), NULL);
    TAF_ERROR_IF_RET_VAL(result == LE_OVERFLOW, LE_OVERFLOW,
                                               "IOCTL interface name length is smaller");

    if(ioctl(sock, SIOCGIFMTU, &ifr) < 0)
    {
        LE_ERROR("ioctl get error %d error:%s",errno,strerror ( errno ));
        close(sock);
        return LE_IO_ERROR;
    }

    *mtuPtr = static_cast<uint16_t>(ifr.ifr_mtu);
    LE_DEBUG("GetMtu MTU %d", static_cast<uint16_t>(*mtuPtr));

    close(sock);

    return LE_OK;
}

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
le_result_t taf_DataConnection::GetServiceStatusFromTelSDK(const uint8_t slotId,
                                                    telux::data::ServiceStatus &serviceStatus)
{
    le_clk_Time_t timeToWait = {TELSDK_ASYNC_REQ_TIMEOUT, 0};
    le_result_t result;

    LE_INFO("Slot ID: %d", slotId);
    auto reqServiceStatusCbFunc = std::bind(
                            &taf_DataConnRequestServiceStatusCallback::requestServiceStatus,
                            reqServiceStatusCb,
                            std::placeholders::_1,
                            std::placeholders::_2);

    telux::common::Status status =
                    dataServingSystemManagers[(SlotId)slotId]->requestServiceStatus(
                                                                        reqServiceStatusCbFunc);
    if (telux::common::Status::SUCCESS != status)
    {
        LE_WARN("<TelSDK> requestServiceStatus failed: %d", static_cast<int>(status));
        return LE_FAULT;
    }

    // Wait for the callback to be called
    result = le_sem_WaitWithTimeOut(reqServiceStatusCb->semaphore, timeToWait);
    if (LE_OK != result)
    {
        LE_WARN("le_sem_WaitWithTimeOut failed: %d", result);
        return LE_TIMEOUT;
    }

    // Check the error code from the SDK's RequestServiceStatusResponseCb
    if (telux::common::ErrorCode::SUCCESS != reqServiceStatusCb->errorCode)
    {
        LE_WARN("<TelSDK>  RequestServiceStatusResponseCb failed: %d",
                static_cast<int>(reqServiceStatusCb->errorCode));
        return LE_FAULT;
    }

    serviceStatus.serviceState = reqServiceStatusCb->status.serviceState;
    serviceStatus.networkRat = reqServiceStatusCb->status.networkRat;

    return LE_OK;
}

taf_dcs_DataBearerTechnology_t taf_DataConnection::MapNwRatToDataBearerTech
(
    telux::data::NetworkRat nwRAT
)
{
    switch (nwRAT)
    {
        case telux::data::NetworkRat::UNKNOWN:   return TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
        case telux::data::NetworkRat::CDMA_1X:   return TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA_1X;
        case telux::data::NetworkRat::CDMA_EVDO: return TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA_EVDO;
        case telux::data::NetworkRat::GSM:       return TAF_DCS_DATA_BEARER_TECHNOLOGY_GSM;
        case telux::data::NetworkRat::WCDMA:     return TAF_DCS_DATA_BEARER_TECHNOLOGY_WCDMA;
        case telux::data::NetworkRat::TDSCDMA:   return TAF_DCS_DATA_BEARER_TECHNOLOGY_TD_SCDMA;
        case telux::data::NetworkRat::LTE:       return TAF_DCS_DATA_BEARER_TECHNOLOGY_LTE;
        case telux::data::NetworkRat::NR5G:      return TAF_DCS_DATA_BEARER_TECHNOLOGY_5G;
        default:
            LE_WARN("Unknown RAT: %d", static_cast<int>(nwRAT));
            return TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
    }
}
#endif // if defined(TARGET_SA515M) || defined(TARGET_SA525M)

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

    // Set default value
    *downDataBearerTechPtr = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
    *upDataBearerTechPtr = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;

    callCtxPtr = GetCallCtx(slotId, profileId);
    if (callCtxPtr == NULL)
    {
        // Just return if call context cannot be found
        LE_ERROR("Cannot find call context, use unknown data bearer");
        return LE_NOT_FOUND;
    }

    if (callCtxPtr->callStatus == telux::data::DataCallStatus::NET_CONNECTED ||
        callCtxPtr->callStatus == telux::data::DataCallStatus::NET_IDLE ||
        callCtxPtr->ipv4Status == telux::data::DataCallStatus::NET_CONNECTED ||
        callCtxPtr->ipv4Status == telux::data::DataCallStatus::NET_IDLE ||
        callCtxPtr->ipv6Status == telux::data::DataCallStatus::NET_CONNECTED ||
        callCtxPtr->ipv6Status == telux::data::DataCallStatus::NET_IDLE)
    {
        // If call is connected, update call data bearer tech
        *downDataBearerTechPtr = callCtxPtr->dataBearerTech;
        *upDataBearerTechPtr = callCtxPtr->dataBearerTech;
    }
    else
    {
        // Call is not connected. Set unknown bearer
        LE_WARN("Data call is not active for profile id %d", profileId);
        *downDataBearerTechPtr = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
        *upDataBearerTechPtr = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
        return LE_UNAVAILABLE;
    }

    // Check if the network is IN SERVICE
    // The service still has cached data bearer technology info. Here the service checks if the
    // NAD is in service with the network. In case of issues in getting the service state, the
    // API will return TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN.
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    telux::data::ServiceStatus serviceStatus;
    le_result_t result = GetServiceStatusFromTelSDK(slotId,serviceStatus);
    if (result != LE_OK)
    {
        LE_WARN("Failed to get service status. Set TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN");
        // Set TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN
        *downDataBearerTechPtr = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
        *upDataBearerTechPtr = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
        return LE_OK;
    }

    // Check if the network is in service
    if (telux::data::DataServiceState::IN_SERVICE != serviceStatus.serviceState)
    {
        LE_WARN("NAD is not IN_SERVICE: %d. Set TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN",
                                                static_cast<int>(serviceStatus.serviceState));
        // Set TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN
        *downDataBearerTechPtr = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
        *upDataBearerTechPtr = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
        return LE_OK;
    }

    // Update the correct RAT
    *downDataBearerTechPtr = MapNwRatToDataBearerTech(serviceStatus.networkRat);
    *upDataBearerTechPtr   = MapNwRatToDataBearerTech(serviceStatus.networkRat);

#endif // if defined(TARGET_SA515M) || defined(TARGET_SA525M)
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

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
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

le_event_Id_t taf_DataConnection::GetQosStateEvent(uint8_t slotId, int32_t profileId)
{
    taf_dcs_CallCtx_t* callCtxPtr;

    callCtxPtr = GetCallCtx(slotId, profileId);
    if (callCtxPtr == NULL)
    {
        TAF_ERROR_IF_RET_VAL(callCtxPtr == NULL, NULL,
                         "Cannot find call context slotId(%d) profileId(%d)",
                         slotId, profileId);
    }

    return callCtxPtr->qosStateEvent;
}

le_result_t taf_DataConnection::SendStatusChangedNotification
(
    taf_dcs_CallCtx_t *callCtxPtr,
    dataCallEvent_t *eventPtr
)
{
    taf_dcs_StateInfo_t stateInfo;
    stateInfo.ipType = TAF_DCS_PDP_UNKNOWN;

    LE_UNUSED(eventPtr);

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
                taf_DCSHelper::IpFamilyTypeToString(callCtxPtr->ipType),
                taf_DCSHelper::CallStatusToString(callCtxPtr->callStatus),
                taf_DCSHelper::CallStatusToString(callCtxPtr->ipv4Status),
                taf_DCSHelper::CallStatusToString(callCtxPtr->ipv6Status));
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
    taf_dcs_QosFlowRef_t qosRef = NULL;
    QOSFlowCtxStatus_t* qosStatus = NULL;

    callCtxPtr->callStatus = eventPtr->callStatus;
    callCtxPtr->ipv4Status = eventPtr->ipv4Status;
    callCtxPtr->ipv6Status = eventPtr->ipv6Status;

    switch (eventPtr->callStatus)
    {
        case  telux::data::DataCallStatus::NET_CONNECTING:
            LE_DEBUG("NET_CONNECTING");
            callCtxPtr->ipType = eventPtr->ipType;
            isSendEvent = true;
        break;

        case telux::data::DataCallStatus::NET_CONNECTED:
            LE_DEBUG("NET_CONNECTED");
            callCtxPtr->ipType     = eventPtr->ipType;
            le_utf8_Copy(callCtxPtr->intfName, eventPtr->ifName,
                         sizeof(callCtxPtr->intfName), NULL);
            if (eventPtr->ipv4Status == telux::data::DataCallStatus::NET_CONNECTED)
            {
                le_utf8_Copy(callCtxPtr->ipv4Addr, eventPtr->ipv4AddrInfo.ifAddress,
                             TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv4Gw, eventPtr->ipv4AddrInfo.gwAddress,
                             TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv4Dns1, eventPtr->ipv4AddrInfo.primaryDnsAddress,
                             TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv4Dns2,
                             eventPtr->ipv4AddrInfo.secondaryDnsAddress,
                             TAF_DCS_IPV4_ADDR_MAX_LEN, NULL);
                callCtxPtr->ipv4Mask = eventPtr->ipv4AddrInfo.ifMask;
            }

            if (eventPtr->ipv6Status == telux::data::DataCallStatus::NET_CONNECTED)
            {
                le_utf8_Copy(callCtxPtr->ipv6Addr, eventPtr->ipv6AddrInfo.ifAddress,
                             TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv6Gw, eventPtr->ipv6AddrInfo.gwAddress,
                             TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv6Dns1, eventPtr->ipv6AddrInfo.primaryDnsAddress,
                             TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(callCtxPtr->ipv6Dns2,
                             eventPtr->ipv6AddrInfo.secondaryDnsAddress,
                             TAF_DCS_IPV6_ADDR_MAX_LEN, NULL);
                callCtxPtr->ipv6Mask = eventPtr->ipv6AddrInfo.ifMask;
            }

            callCtxPtr->dataBearerTech = updateDataBearerTech(eventPtr->dataBearerTech);
            callCtxPtr->maxRxBitRate = eventPtr->maxRxBitRate;
            callCtxPtr->maxTxBitRate = eventPtr->maxTxBitRate;
            isSendEvent = true;
        break;

        case telux::data::DataCallStatus::NET_DISCONNECTING:
            LE_DEBUG("NET_DISCONNECTING");
            callCtxPtr->maxRxBitRate = 0;
            callCtxPtr->maxTxBitRate = 0;
            isSendEvent = true;
        break;

        case telux::data::DataCallStatus::NET_NO_NET:
            LE_DEBUG("NET_NO_NET");
            memset(callCtxPtr->intfName, 0, sizeof(callCtxPtr->intfName));
            callCtxPtr->maxRxBitRate = 0;
            callCtxPtr->maxTxBitRate = 0;

            //delete QOS flow for this data call
            qosRef = callCtxPtr->qosFlowRef;
            qosStatus = (QOSFlowCtxStatus_t*)le_ref_Lookup(QosStatusRefMap, qosRef);
            if(qosStatus)
            {
              //when dataCall is disconnected , it implicitly means QOS is deleted.
              LE_DEBUG("QOS flow released ID %d",qosStatus->qosID);
              le_ref_DeleteRef(QosStatusRefMap, qosRef);
              le_mem_Release(qosStatus);
              callCtxPtr->qosFlowRef = NULL;
            }

            // IPv4 call end reason
            callCtxPtr->callEndReasonIPv4.callEndReasonType =
                                                    eventPtr->callEndReasonIPv4.callEndReasonType;
            switch (eventPtr->callEndReasonIPv4.callEndReasonType)
            {
            case TAF_DCS_CE_TYPE_MOBILE_IP:
                callCtxPtr->callEndReasonIPv4.reasonMIP =
                                                    eventPtr->callEndReasonIPv4.reasonMIP;
                break;
            case TAF_DCS_CE_TYPE_INTERNAL:
                callCtxPtr->callEndReasonIPv4.reasonInternal =
                                                    eventPtr->callEndReasonIPv4.reasonInternal;
                break;
            case TAF_DCS_CE_TYPE_CALL_MANAGER_DEFINED:
                callCtxPtr->callEndReasonIPv4.reasonCallManager =
                                                    eventPtr->callEndReasonIPv4.reasonCallManager;
                break;
            case TAF_DCS_CE_TYPE_3GPP_SPEC_DEFINED:
                callCtxPtr->callEndReasonIPv4.reasonSpec =
                                                    eventPtr->callEndReasonIPv4.reasonSpec;
                break;
            case TAF_DCS_CE_TYPE_PPP:
                callCtxPtr->callEndReasonIPv4.reasonPPP =
                                                    eventPtr->callEndReasonIPv4.reasonPPP;
                break;
            case TAF_DCS_CE_TYPE_EHRPD:
                callCtxPtr->callEndReasonIPv4.reasonEHRPD =
                                                    eventPtr->callEndReasonIPv4.reasonEHRPD;
                break;
            case TAF_DCS_CE_TYPE_IPV6:
                callCtxPtr->callEndReasonIPv4.reasonIPv6 =
                                                    eventPtr->callEndReasonIPv4.reasonIPv6;
                break;
            case TAF_DCS_CE_TYPE_HANDOFF:
                callCtxPtr->callEndReasonIPv4.reasonHandOff =
                                                    eventPtr->callEndReasonIPv4.reasonHandOff;
                break;
            default:
                LE_WARN("Invalid Reason type: %d",
                        static_cast<int32_t>(eventPtr->callEndReasonIPv4.callEndReasonType));
                callCtxPtr->callEndReasonIPv4.reasonInternal = TAF_DCS_CE_INTERNAL_UNKNOWN;
                break;
            }

            // IPv6 call end reason
            callCtxPtr->callEndReasonIPv6.callEndReasonType =
                                                eventPtr->callEndReasonIPv6.callEndReasonType;
            switch (eventPtr->callEndReasonIPv6.callEndReasonType)
            {
            case TAF_DCS_CE_TYPE_MOBILE_IP:
                callCtxPtr->callEndReasonIPv6.reasonMIP =
                                                eventPtr->callEndReasonIPv6.reasonMIP;
                break;
            case TAF_DCS_CE_TYPE_INTERNAL:
                callCtxPtr->callEndReasonIPv6.reasonInternal =
                                                eventPtr->callEndReasonIPv6.reasonInternal;
                break;
            case TAF_DCS_CE_TYPE_CALL_MANAGER_DEFINED:
                callCtxPtr->callEndReasonIPv6.reasonCallManager =
                                                eventPtr->callEndReasonIPv6.reasonCallManager;
                break;
            case TAF_DCS_CE_TYPE_3GPP_SPEC_DEFINED:
                callCtxPtr->callEndReasonIPv6.reasonSpec =
                                                eventPtr->callEndReasonIPv6.reasonSpec;
                break;
            case TAF_DCS_CE_TYPE_PPP:
                callCtxPtr->callEndReasonIPv6.reasonPPP =
                                                eventPtr->callEndReasonIPv6.reasonPPP;
                break;
            case TAF_DCS_CE_TYPE_EHRPD:
                callCtxPtr->callEndReasonIPv6.reasonEHRPD =
                                                eventPtr->callEndReasonIPv6.reasonEHRPD;
                break;
            case TAF_DCS_CE_TYPE_IPV6:
                callCtxPtr->callEndReasonIPv6.reasonIPv6 =
                                                eventPtr->callEndReasonIPv6.reasonIPv6;
                break;
            case TAF_DCS_CE_TYPE_HANDOFF:
                callCtxPtr->callEndReasonIPv6.reasonHandOff =
                                                eventPtr->callEndReasonIPv6.reasonHandOff;
                break;
            default:
                LE_WARN("Invalid Reason type: %d",
                        static_cast<int32_t>(eventPtr->callEndReasonIPv6.callEndReasonType));
                callCtxPtr->callEndReasonIPv6.reasonInternal = TAF_DCS_CE_INTERNAL_UNKNOWN;
                break;
            }
            isSendEvent = true;
            break;

        default:
            LE_ERROR("cannot handle this event: %s",
                            taf_DCSHelper::CallStatusToString(eventPtr->callStatus));
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

void taf_DataConnection::InternalDataCallEventHandler(void *reportPtr)
{
    if (!IsActive())
    {
        LE_WARN("Dropping internal data call event because taf_DataConnection is deinitialized");
        return;
    }
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
                    if(connHandlerMappingPtr != NULL)
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
                TAF_ERROR_IF_RET_NIL(isSendNotification != true,
                                     "won't send notification to listener");
                LE_DEBUG("STARTCALLBACK:callStatus = %s, ipType=%d, ipv4status=%s, ipv6status=%s",
                taf_DCSHelper::CallStatusToString(callCtxPtr->callStatus),(int)callCtxPtr->ipType,
                taf_DCSHelper::CallStatusToString(callCtxPtr->ipv4Status),
                taf_DCSHelper::CallStatusToString(callCtxPtr->ipv6Status));
                SendStatusChangedNotification(callCtxPtr,eventPtr);

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
                    if(connHandlerMappingPtr != NULL)
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
                TAF_ERROR_IF_RET_NIL(isSendNotification != true,
                                     "won't send notification to listener");
                LE_DEBUG("STOPCALLBACK:callStatus = %s, ipType=%d, ipv4status=%s, ipv6status=%s",
                          taf_DCSHelper::CallStatusToString(callCtxPtr->callStatus),(int)stateInfo.ipType,
                          taf_DCSHelper::CallStatusToString(callCtxPtr->ipv4Status),
                          taf_DCSHelper::CallStatusToString(callCtxPtr->ipv6Status));
                SendStatusChangedNotification(callCtxPtr,eventPtr);

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
                LE_ERROR("Getting profile is failed from callback, errorCode: %d",
                          (uint32_t)eventPtr->errorCode);
                result = LE_FAULT;
            }
            else
            {
                LE_INFO("Default profile ID for slotId(%d) is %d", slotId, profileId);
                DefaultProfileIdMap[slotId] = profileId;
            }
            CmdSynchronousPromise.set_value(result);
        break;

        default:
            LE_ERROR("invalid event(%d)", eventPtr->event);
        break;
    }

    return;
}

// The handler for dataConnection.CallEvent
void taf_DataConnection::DataCnxCallEventHandler(void *reportPtr)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    return dataConnection.InternalDataCallEventHandler(reportPtr);
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
                    taf_DCSHelper::IpFamilyTypeToString(callCtxPtr->ipType),
                    taf_DCSHelper::CallStatusToString(callCtxPtr->ipv4Status),
                    taf_DCSHelper::CallStatusToString(callCtxPtr->ipv6Status));

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
                        taf_DCSHelper::IpFamilyTypeToString(callCtxPtr->ipType),
                        taf_DCSHelper::CallStatusToString(callCtxPtr->ipv4Status),
                        taf_DCSHelper::CallStatusToString(callCtxPtr->ipv6Status));

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
                        taf_DCSHelper::IpFamilyTypeToString(callCtxPtr->ipType),
                        taf_DCSHelper::CallStatusToString(callCtxPtr->ipv4Status),
                        taf_DCSHelper::CallStatusToString(callCtxPtr->ipv6Status));
            }
        }
        else
        {
            LE_ERROR("invalid IP type: %s or status IPv4[%s] IPv6[%s]",
                     taf_DCSHelper::IpFamilyTypeToString(callCtxPtr->ipType),
                     taf_DCSHelper::CallStatusToString(callCtxPtr->ipv4Status),
                     taf_DCSHelper::CallStatusToString(callCtxPtr->ipv6Status));
        }
    }
    else
    {
        LE_INFO("no promise for Conn[%d] Type[%s] IPv4[%s] IPv6[%s]", conState,
                taf_DCSHelper::IpFamilyTypeToString(callCtxPtr->ipType),
                taf_DCSHelper::CallStatusToString(callCtxPtr->ipv4Status),
                taf_DCSHelper::CallStatusToString(callCtxPtr->ipv6Status));
    }

    return;
}

void taf_DataConnection::RegisterSessionStateHandler(taf_dcs_SessionStateFunc_t func)
{
    TAF_ERROR_IF_RET_NIL(func == NULL, "the registered session state func is null");
    SessionStateFunc = func;
    return;
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
    le_event_AddHandler("Internal Event Handler", dataConnection.CallEvent, DataCnxCallEventHandler);

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

    LE_UNUSED(contextPtr);

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

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
void taf_DataConnection::onInitCompleted(telux::common::ServiceStatus status)
{
    LE_INFO("Received service status: %d", (int)status);
    std::lock_guard<std::mutex> lock(mtx);
    if (status != telux::common::ServiceStatus::SERVICE_UNAVAILABLE)
    {
        subSystemStatusUpdated = true;
        conVar.notify_all();
    }
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

static bool bServingSystemListenersRegistered [MAX_SLOT_NUM] = {false};
static bool bDataConnectionListenersRegistered[MAX_SLOT_NUM] = {false};

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
        // Register serving system listener for each slot it
        if(bServingSystemListenersRegistered[slotIdx-1])
        {
            LE_INFO("Serving System listeners already registered.");
        }
        else
        {
            if ( dataConnection.dataServingSystemManagers.find((SlotId)slotIdx) !=
                                                    dataConnection.dataServingSystemManagers.end())
            {
                if( dataConnection.dataServingSystemManagers[(SlotId)slotIdx]->registerListener(
                        dataConnection.dataServingSystemListeners[(SlotId)slotIdx]) ==
                                                                    telux::common::Status::SUCCESS)
                {
                    LE_INFO("Serving system listener for slot ID %d registered.", slotIdx);
                    bServingSystemListenersRegistered[slotIdx-1] = true;
                }
                else
                {
                    LE_ERROR("Fail to register serving system listener %d.", slotIdx);
                }
            }
        }

        // Register data connection listener for each slot it
        if (bDataConnectionListenersRegistered[slotIdx - 1])
        {
            LE_INFO("Data connection listeners already registered.");
        }
        else
        {
            if (dataConnection.dataConnectionManagers.find((SlotId)slotIdx) !=
                dataConnection.dataConnectionManagers.end())
            {
                if (dataConnection.dataConnectionManagers[(SlotId)slotIdx] -> registerListener(
                                    dataConnection.dataConnectionListeners[(SlotId)slotIdx]) ==
                                                                    telux::common::Status::SUCCESS)
                {
                    LE_INFO("Data connection listener for slot ID %d registered.", slotIdx);
                    bDataConnectionListenersRegistered[slotIdx - 1] = true;
                }
                else
                {
                    LE_ERROR("Fail to register serving system listener %d.", slotIdx);
                }
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
        // Deregister serving system listeners
        if(!bServingSystemListenersRegistered[slotIdx-1])
        {
            LE_INFO("Serving system listeners already deregistered.");
        }
        else
        {
            if ( dataConnection.dataServingSystemManagers.find((SlotId)slotIdx) !=
                                                    dataConnection.dataServingSystemManagers.end())
            {
                if( dataConnection.dataServingSystemManagers[(SlotId)slotIdx]->deregisterListener(
                        dataConnection.dataServingSystemListeners[(SlotId)slotIdx]) ==
                                                                    telux::common::Status::SUCCESS)
                {
                    LE_INFO("Serving system listener %d deregistered.", slotIdx);
                    bServingSystemListenersRegistered[slotIdx - 1] = false;
                }
                else
                {
                    LE_ERROR("Fail to deregister serving system listener %d.", slotIdx);
                }
            }
        }

        // Deregister data connection listener for each slot it
        if (!bDataConnectionListenersRegistered[slotIdx - 1])
        {
            LE_INFO("Data connection listeners already deregistered.");
        }
        else
        {
            if (dataConnection.dataConnectionManagers.find((SlotId)slotIdx) !=
                dataConnection.dataConnectionManagers.end())
            {
                if (dataConnection.dataConnectionManagers[(SlotId)slotIdx] -> deregisterListener(
                                    dataConnection.dataConnectionListeners[(SlotId)slotIdx]) ==
                                                                    telux::common::Status::SUCCESS)
                {
                    LE_INFO("Data connection listener for slot ID %d registered.", slotIdx);
                    bDataConnectionListenersRegistered[slotIdx - 1] = false;
                }
                else
                {
                    LE_ERROR("Fail to register serving system listener %d.", slotIdx);
                }
            }
        }
        UNLOCK
    }
}

void PowerStateChangeHandler(taf_pm_State_t state, void* contextPtr)
{
    LE_UNUSED(contextPtr);
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
    bMultiSimSupported = false;

    // Initialize the default profile ID map
    DefaultProfileIdMap.clear();
    DefaultProfileIdMap.insert(std::pair<uint8_t, uint32_t>(SLOT_ID_1, TAF_DCS_DEFAULT_PROFILE));

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)

    int noOfSlots = MIN_SLOT_COUNT;
    if(telux::common::DeviceConfig::isMultiSimSupported())
    {
        bMultiSimSupported = true;
        // Update the default profile ID map with the second slot ID
        DefaultProfileIdMap.insert(std::pair<uint8_t,uint32_t>(SLOT_ID_2, TAF_DCS_DEFAULT_PROFILE));
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
        auto timeout = std::chrono::seconds(DATA_SUBSYSTEM_INIT_TIMEOUT);

        if (conneMgr)
        {
            std::unique_lock<std::mutex> uLock(mtx);
            if (!conVar.wait_for(uLock, timeout, [this]{return this->subSystemStatusUpdated;}))
            {
                LE_FATAL("Timeout to wait data connection component initialization for slot %d !",
                            (int)slotIdx);
            }
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
            LE_FATAL("Failed to get connection Manager instance ");
        }

        if(subSysReady)
        {
            LE_INFO("Data connection component is ready for slot %d...",(int)slotIdx);
        }
        else
        {
            LE_FATAL("Unable to init data connection component for slot %d !",(int)slotIdx);
        }

        /* register data connection status listener */
        tafDataConnectionListeners[(SlotId)slotIdx] =
                                std::make_shared<taf_DataConnectionListener>((SlotId)slotIdx);
        dataConnectionListeners[(SlotId)slotIdx] = tafDataConnectionListeners[(SlotId)slotIdx];

        /* register data serving system manager */
        tafDataConnServingSystemListeners[(SlotId)slotIdx] =
                              std::make_shared<taf_DataConnServingSystemListener>((SlotId)slotIdx);
        dataServingSystemListeners[(SlotId)slotIdx] =
                                                 tafDataConnServingSystemListeners[(SlotId)slotIdx];

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
            LE_FATAL("Failed to get serving system Manager instance ");
        }

    }

    reqServiceStatusCb = std::make_shared<taf_DataConnRequestServiceStatusCallback>();
    reqServiceStatusCb->semaphore = le_sem_Create("taf_DataConnRequestServiceStatusCallbackSem", 0);

    reqRoamingStatusCb = std::make_shared<taf_DataConnRequestRoamingStatusCallback>();
    reqRoamingStatusCb->semaphore = le_sem_Create("taf_ConnReqRoamingStatusCbSem", 0);

    reqAPNThrottlingStatusCb = std::make_shared<taf_DataAPNThrottleInfoCallback>();
    reqAPNThrottlingStatusCb->semaphore = le_sem_Create("taf_ConnReqRoamingStatusCbSem", 0);

#else // #if defined(TARGET_SA515M) || defined(TARGET_SA525M)

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
        LE_FATAL("unable to init data connection component!");
    }

    /* register data connection status listener */
    dataConnectionListeners[(SlotId)SLOT_ID_1] =
                                    std::make_shared<taf_DataConnectionListener>((SlotId)SLOT_ID_1);
    telux::common::Status status =
                        ConnectionMgr->registerListener(dataConnectionListeners[(SlotId)SLOT_ID_1]);
    TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS,
                         "register listener for SLOT_ID_1 failed, status: %d", (int32_t)status);

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
    //TFT QOS Flow Reference
    QosStatusPool = le_mem_InitStaticPool(QosStatusPool,TAF_DCS_MAX_SESSION_REF,
                                                      sizeof(QOSFlowCtxStatus_t));
    QosStatusRefMap = le_ref_InitStaticMap(QosStatusRefMap, TAF_DCS_MAX_SESSION_REF);

    DataCallRefMap = le_ref_CreateMap("Call Context Reference", TAF_DCS_MAX_CALL_OBJ);

    le_sem_Ref_t semRef = le_sem_Create("ConnThreadSem", 0);
    ConnectionEventThreadRef = le_thread_Create("DcsEvtThread", ConnectionEventThread,
                                                (void*)semRef);
    le_thread_SetJoinable(ConnectionEventThreadRef);
    le_thread_Start(ConnectionEventThreadRef);
    le_sem_Wait(semRef);
    le_sem_Delete(semRef);

    le_msg_AddServiceCloseHandler(taf_dcs_GetServiceRef(), CloseEventHandler, NULL);
    callCtxMutex = le_mutex_CreateNonRecursive("callCtxMutex");
    handlerlistMutex = le_mutex_CreateNonRecursive("handlerlistMutex");

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    // Add power state change handler
    taf_pm_AddStateChangeHandler(PowerStateChangeHandler, NULL);
    if (taf_pm_GetPowerState() != TAF_PM_STATE_SUSPEND)
    {
        RegisterListeners();
    }
#endif

    isActive_.store(true, std::memory_order_release);
    return;
}

void taf_DataConnection::ClearHandlerMappingList(void)
{
    HandlerSessionMapping_t *handlerSessionInfo;

    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&HandlerSessionMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, HandlerSessionMapping_t, handlerLink);
        handlerLinkPtr = le_dls_PeekNext(&HandlerSessionMappingList, handlerLinkPtr);
        le_dls_Remove(&HandlerSessionMappingList, &handlerSessionInfo->handlerLink);
        le_mem_Release(handlerSessionInfo);
    }
}

void taf_DataConnection::ClearDataCallCtxList(void)
{
    LE_INFO("Clear data call contexts list");
    le_dls_Link_t *linkPtr = NULL;
    linkPtr = le_dls_Peek(&DataCallCtxList);

    while (linkPtr)
    {
        taf_dcs_CallCtx_t *callCtxPtr = CONTAINER_OF(linkPtr, taf_dcs_CallCtx_t, link);
        linkPtr = le_dls_PeekNext(&DataCallCtxList, linkPtr);

        {
            // Clear session within each call context
            le_dls_Link_t* sessionLinkPtr = NULL;
            sessionLinkPtr = le_dls_Peek(&(callCtxPtr->sessionRefList));
            while (sessionLinkPtr)
            {
                taf_SessionRef_t *sessionRefPtr = CONTAINER_OF(sessionLinkPtr, taf_SessionRef_t,
                                                                                            link);
                sessionLinkPtr = le_dls_PeekNext(&(callCtxPtr->sessionRefList), sessionLinkPtr);
                le_dls_Remove(&(callCtxPtr->sessionRefList), &sessionRefPtr->link);
                le_mem_Release(sessionRefPtr);
            }
        }

        le_dls_Remove(&DataCallCtxList, &(callCtxPtr->link));
        le_mem_Release(callCtxPtr);
    }
    return;
}

void taf_DataConnection::Deinit(void)
{
    isActive_.store(false, std::memory_order_release);

    // Deregister TelSDK listeners
    DeregisterListeners();

    // Clean up all handlers
    ClearHandlerMappingList();

    // Clear all call contexts
    ClearDataCallCtxList();

    // Stop the connection event thread
    le_thread_Cancel(ConnectionEventThreadRef);
    le_thread_Join(ConnectionEventThreadRef, NULL);
}
