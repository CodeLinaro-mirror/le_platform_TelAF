/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file   tafDcsHelper.cpp
 * @brief  Helper functions for the data call service.
 */

#include "tafDcsHelper.hpp"
using namespace telux::tafsvc;

const char *taf_DCSHelper::DataBearerTechnologyToString(taf_dcs_DataBearerTechnology_t tech)
{
    switch (tech)
    {
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_1X:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_1X";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EVDO:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EVDO";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EVDO_REVA:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EVDO_REVA";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EVDO_REVB:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EVDO_REVB";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EHRPD:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EHRPD";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_HRPD:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_HRPD";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA_EVDO_FMC:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA_EVDO_FMC";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_3GPP2_WLAN:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_3GPP2_WLAN";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA_1X:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA_1X";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA_EVDO:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA_EVDO";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_WCDMA:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_WCDMA";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_GPRS:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_GPRS";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_HSDPA:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_HSDPA";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_HSUPA:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_HSUPA";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_EDGE:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_EDGE";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_LTE:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_LTE";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_HSDPA_PLUS:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_HSDPA_PLUS";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_DC_HSDPA_PLUS:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_DC_HSDPA_PLUS";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_HSPA:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_HSPA";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_64_QAM:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_64_QAM";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_TD_SCDMA:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_TD_SCDMA";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_GSM:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_GSM";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_3GPP_WLAN:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_3GPP_WLAN";
    case TAF_DCS_DATA_BEARER_TECHNOLOGY_5G:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_5G";
    default:
        return "TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN";
    }
}

const char *taf_DCSHelper::RoamingTypeToString(taf_dcs_RoamingType_t roamingType)
{
    switch (roamingType)
    {
    case TAF_DCS_ROAMING_UNKNOWN:
        return "TAF_DCS_ROAMING_UNKNOWN";
    case TAF_DCS_ROAMING_DOMESTIC:
        return "TAF_DCS_ROAMING_DOMESTIC";
    case TAF_DCS_ROAMING_INTERNATIONAL:
        return "TAF_DCS_ROAMING_INTERNATIONAL";
    default:
        return "TAF_DCS_ROAMING_TYPE_UNKNOWN";
    }
}

const char *taf_DCSHelper::CallEventToString(taf_dcs_ConState_t callEvent)
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
}

const char *taf_DCSHelper::CallStatusToString(telux::data::DataCallStatus status)
{
    switch (status)
    {
    case telux::data::DataCallStatus::INVALID:
        return "INVALID";
    case telux::data::DataCallStatus::NET_CONNECTED:
        return "NET_CONNECTED";
    case telux::data::DataCallStatus::NET_NO_NET:
        return "NET_NO_NET";
    case telux::data::DataCallStatus::NET_IDLE:
        return "NET_IDLE";
    case telux::data::DataCallStatus::NET_CONNECTING:
        return "NET_CONNECTING";
    case telux::data::DataCallStatus::NET_DISCONNECTING:
        return "NET_DISCONNECTING";
    case telux::data::DataCallStatus::NET_RECONFIGURED:
        return "NET_RECONFIGURED";
    case telux::data::DataCallStatus::NET_NEWADDR:
        return "NET_NEWADDR";
    case telux::data::DataCallStatus::NET_DELADDR:
        return "NET_DELADDR";
    default:
        LE_ERROR("call(%d) status error", static_cast<int32_t> (status));
        return "INVALID";
    }
}

const char *taf_DCSHelper::CallEndReasonTypeToString(telux::common::EndReasonType endReasontype)
{
    switch (endReasontype)
    {
    case telux::common::EndReasonType::CE_MOBILE_IP:
        return "CE_MOBILE_IP";
    case telux::common::EndReasonType::CE_INTERNAL:
        return "CE_INTERNAL";
    case telux::common::EndReasonType::CE_CALL_MANAGER_DEFINED:
        return "CE_CALL_MANAGER_DEFINED";
    case telux::common::EndReasonType::CE_3GPP_SPEC_DEFINED:
        return "CE_3GPP_SPEC_DEFINED";
    case telux::common::EndReasonType::CE_PPP:
        return "CE_PPP";
    case telux::common::EndReasonType::CE_EHRPD:
        return "CE_EHRPD";
    case telux::common::EndReasonType::CE_IPV6:
        return "CE_IPV6";
    case telux::common::EndReasonType::CE_UNKNOWN:
        return "CE_UNKNOWN";
    default:
        LE_ERROR("end reason(%d) error", static_cast<int32_t>(endReasontype));
        return "CE_UNKNOWN";
    }
}

taf_dcs_CallEndReasonType_t taf_DCSHelper::ConvertCallEndReasonType
(
    telux::common::EndReasonType endReasontype
)
{
    using telux::common::EndReasonType;
    switch (endReasontype)
    {
    case EndReasonType::CE_MOBILE_IP:
        return TAF_DCS_CE_TYPE_MOBILE_IP;
    case EndReasonType::CE_INTERNAL:
        return TAF_DCS_CE_TYPE_INTERNAL;
    case EndReasonType::CE_CALL_MANAGER_DEFINED:
        return TAF_DCS_CE_TYPE_CALL_MANAGER_DEFINED;
    case EndReasonType::CE_3GPP_SPEC_DEFINED:
        return TAF_DCS_CE_TYPE_3GPP_SPEC_DEFINED;
    case EndReasonType::CE_PPP:
        return TAF_DCS_CE_TYPE_PPP;
    case EndReasonType::CE_EHRPD:
        return TAF_DCS_CE_TYPE_EHRPD;
    case EndReasonType::CE_IPV6:
        return TAF_DCS_CE_TYPE_IPV6;
    case EndReasonType::CE_UNKNOWN:
        return TAF_DCS_CE_TYPE_UNKNOWN;
    default:
        LE_ERROR("Invalid call end reason type: %d", static_cast<int32_t>(endReasontype));
        return TAF_DCS_CE_TYPE_UNKNOWN;
    }
}

const char *taf_DCSHelper::CallEndReasonTypeToString(taf_dcs_CallEndReasonType_t endReasonType)
{
    switch (endReasonType)
    {
    case TAF_DCS_CE_TYPE_UNKNOWN:              return "TAF_DCS_CE_TYPE_UNKNOWN";
    case TAF_DCS_CE_TYPE_MOBILE_IP:            return "TAF_DCS_CE_TYPE_MOBILE_IP";
    case TAF_DCS_CE_TYPE_INTERNAL:             return "TAF_DCS_CE_TYPE_INTERNAL";
    case TAF_DCS_CE_TYPE_CALL_MANAGER_DEFINED: return "TAF_DCS_CE_TYPE_CALL_MANAGER_DEFINED";
    case TAF_DCS_CE_TYPE_3GPP_SPEC_DEFINED:    return "TAF_DCS_CE_TYPE_3GPP_SPEC_DEFINED";
    case TAF_DCS_CE_TYPE_PPP:                  return "TAF_DCS_CE_TYPE_PPP";
    case TAF_DCS_CE_TYPE_EHRPD:                return "TAF_DCS_CE_TYPE_EHRPD";
    case TAF_DCS_CE_TYPE_IPV6:                 return "TAF_DCS_CE_TYPE_IPV6";
    case TAF_DCS_CE_TYPE_HANDOFF:              return "TAF_DCS_CE_TYPE_HANDOFF";
    default:
        LE_WARN("Invalid Reason Type: %d", static_cast<int>(endReasonType));
        return "Invalid Reason Type";
    }
}

taf_dcs_CallEndMobileIpReasonCode_t taf_DCSHelper::ConvertCallEndMobileIpReasonCode(
    telux::common::MobileIpReasonCode endReasonCode)
{
    using telux::common::MobileIpReasonCode;
    taf_dcs_CallEndMobileIpReasonCode_t result;

    switch (endReasonCode)
    {
    case MobileIpReasonCode::CE_MIP_FA_ERR_REASON_UNSPECIFIED:
        result = TAF_DCS_CE_MIP_FA_ERR_REASON_UNSPECIFIED;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_ADMINISTRATIVELY_PROHIBITED:
        result = TAF_DCS_CE_MIP_FA_ERR_ADMINISTRATIVELY_PROHIBITED;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_INSUFFICIENT_RESOURCES:
        result = TAF_DCS_CE_MIP_FA_ERR_INSUFFICIENT_RESOURCES;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE:
        result = TAF_DCS_CE_MIP_FA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_HA_AUTHENTICATION_FAILURE:
        result = TAF_DCS_CE_MIP_FA_ERR_HA_AUTHENTICATION_FAILURE;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_REQUESTED_LIFETIME_TOO_LONG:
        result = TAF_DCS_CE_MIP_FA_ERR_REQUESTED_LIFETIME_TOO_LONG;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_MALFORMED_REQUEST:
        result = TAF_DCS_CE_MIP_FA_ERR_MALFORMED_REQUEST;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_MALFORMED_REPLY:
        result = TAF_DCS_CE_MIP_FA_ERR_MALFORMED_REPLY;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_ENCAPSULATION_UNAVAILABLE:
        result = TAF_DCS_CE_MIP_FA_ERR_ENCAPSULATION_UNAVAILABLE;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_VJHC_UNAVAILABLE:
        result = TAF_DCS_CE_MIP_FA_ERR_VJHC_UNAVAILABLE;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_REVERSE_TUNNEL_UNAVAILABLE:
        result = TAF_DCS_CE_MIP_FA_ERR_REVERSE_TUNNEL_UNAVAILABLE;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET:
        result = TAF_DCS_CE_MIP_FA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_DELIVERY_STYLE_NOT_SUPPORTED:
        result = TAF_DCS_CE_MIP_FA_ERR_DELIVERY_STYLE_NOT_SUPPORTED;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_MISSING_NAI:
        result = TAF_DCS_CE_MIP_FA_ERR_MISSING_NAI;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_MISSING_HA:
        result = TAF_DCS_CE_MIP_FA_ERR_MISSING_HA;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_MISSING_HOME_ADDR:
        result = TAF_DCS_CE_MIP_FA_ERR_MISSING_HOME_ADDR;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_UNKNOWN_CHALLENGE:
        result = TAF_DCS_CE_MIP_FA_ERR_UNKNOWN_CHALLENGE;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_MISSING_CHALLENGE:
        result = TAF_DCS_CE_MIP_FA_ERR_MISSING_CHALLENGE;
        break;
    case MobileIpReasonCode::CE_MIP_FA_ERR_STALE_CHALLENGE:
        result = TAF_DCS_CE_MIP_FA_ERR_STALE_CHALLENGE;
        break;
    case MobileIpReasonCode::CE_MIP_HA_ERR_REASON_UNSPECIFIED:
        result = TAF_DCS_CE_MIP_HA_ERR_REASON_UNSPECIFIED;
        break;
    case MobileIpReasonCode::CE_MIP_HA_ERR_ADMINISTRATIVELY_PROHIBITED:
        result = TAF_DCS_CE_MIP_HA_ERR_ADMINISTRATIVELY_PROHIBITED;
        break;
    case MobileIpReasonCode::CE_MIP_HA_ERR_INSUFFICIENT_RESOURCES:
        result = TAF_DCS_CE_MIP_HA_ERR_INSUFFICIENT_RESOURCES;
        break;
    case MobileIpReasonCode::CE_MIP_HA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE:
        result = TAF_DCS_CE_MIP_HA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE;
        break;
    case MobileIpReasonCode::CE_MIP_HA_ERR_FA_AUTHENTICATION_FAILURE:
        result = TAF_DCS_CE_MIP_HA_ERR_FA_AUTHENTICATION_FAILURE;
        break;
    case MobileIpReasonCode::CE_MIP_HA_ERR_REGISTRATION_ID_MISMATCH:
        result = TAF_DCS_CE_MIP_HA_ERR_REGISTRATION_ID_MISMATCH;
        break;
    case MobileIpReasonCode::CE_MIP_HA_ERR_MALFORMED_REQUEST:
        result = TAF_DCS_CE_MIP_HA_ERR_MALFORMED_REQUEST;
        break;
    case MobileIpReasonCode::CE_MIP_HA_ERR_UNKNOWN_HA_ADDR:
        result = TAF_DCS_CE_MIP_HA_ERR_UNKNOWN_HA_ADDR;
        break;
    case MobileIpReasonCode::CE_MIP_HA_ERR_REVERSE_TUNNEL_UNAVAILABLE:
        result = TAF_DCS_CE_MIP_HA_ERR_REVERSE_TUNNEL_UNAVAILABLE;
        break;
    case MobileIpReasonCode::CE_MIP_HA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET:
        result = TAF_DCS_CE_MIP_HA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET;
        break;
    case MobileIpReasonCode::CE_MIP_HA_ERR_ENCAPSULATION_UNAVAILABLE:
        result = TAF_DCS_CE_MIP_HA_ERR_ENCAPSULATION_UNAVAILABLE;
        break;
    default:
        result = TAF_DCS_CE_MIP_UNKNOWN;
        break;
    }
    LE_DEBUG("TelSDK code: %d, TelAF code: %d",
             static_cast<uint32_t>(endReasonCode), static_cast<uint32_t>(result));
    return result;
}

const char *taf_DCSHelper::CallEndMobileIpReasonCodeToString
(
    taf_dcs_CallEndMobileIpReasonCode_t endCode
)
{
    switch (endCode)
    {
    case TAF_DCS_CE_MIP_FA_ERR_REASON_UNSPECIFIED:
        return "TAF_DCS_CE_MIP_FA_ERR_REASON_UNSPECIFIED";
    case TAF_DCS_CE_MIP_FA_ERR_ADMINISTRATIVELY_PROHIBITED:
        return "TAF_DCS_CE_MIP_FA_ERR_ADMINISTRATIVELY_PROHIBITED";
    case TAF_DCS_CE_MIP_FA_ERR_INSUFFICIENT_RESOURCES:
        return "TAF_DCS_CE_MIP_FA_ERR_INSUFFICIENT_RESOURCES";
    case TAF_DCS_CE_MIP_FA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE:
        return "TAF_DCS_CE_MIP_FA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE";
    case TAF_DCS_CE_MIP_FA_ERR_HA_AUTHENTICATION_FAILURE:
        return "TAF_DCS_CE_MIP_FA_ERR_HA_AUTHENTICATION_FAILURE";
    case TAF_DCS_CE_MIP_FA_ERR_REQUESTED_LIFETIME_TOO_LONG:
        return "TAF_DCS_CE_MIP_FA_ERR_REQUESTED_LIFETIME_TOO_LONG";
    case TAF_DCS_CE_MIP_FA_ERR_MALFORMED_REQUEST:
        return "TAF_DCS_CE_MIP_FA_ERR_MALFORMED_REQUEST";
    case TAF_DCS_CE_MIP_FA_ERR_MALFORMED_REPLY:
        return "TAF_DCS_CE_MIP_FA_ERR_MALFORMED_REPLY";
    case TAF_DCS_CE_MIP_FA_ERR_ENCAPSULATION_UNAVAILABLE:
        return "TAF_DCS_CE_MIP_FA_ERR_ENCAPSULATION_UNAVAILABLE";
    case TAF_DCS_CE_MIP_FA_ERR_VJHC_UNAVAILABLE:
        return "TAF_DCS_CE_MIP_FA_ERR_VJHC_UNAVAILABLE";
    case TAF_DCS_CE_MIP_FA_ERR_REVERSE_TUNNEL_UNAVAILABLE:
        return "TAF_DCS_CE_MIP_FA_ERR_REVERSE_TUNNEL_UNAVAILABLE";
    case TAF_DCS_CE_MIP_FA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET:
        return "TAF_DCS_CE_MIP_FA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET";
    case TAF_DCS_CE_MIP_FA_ERR_DELIVERY_STYLE_NOT_SUPPORTED:
        return "TAF_DCS_CE_MIP_FA_ERR_DELIVERY_STYLE_NOT_SUPPORTED";
    case TAF_DCS_CE_MIP_FA_ERR_MISSING_NAI:
        return "TAF_DCS_CE_MIP_FA_ERR_MISSING_NAI";
    case TAF_DCS_CE_MIP_FA_ERR_MISSING_HA:
        return "TAF_DCS_CE_MIP_FA_ERR_MISSING_HA";
    case TAF_DCS_CE_MIP_FA_ERR_MISSING_HOME_ADDR:
        return "TAF_DCS_CE_MIP_FA_ERR_MISSING_HOME_ADDR";
    case TAF_DCS_CE_MIP_FA_ERR_UNKNOWN_CHALLENGE:
        return "TAF_DCS_CE_MIP_FA_ERR_UNKNOWN_CHALLENGE";
    case TAF_DCS_CE_MIP_FA_ERR_MISSING_CHALLENGE:
        return "TAF_DCS_CE_MIP_FA_ERR_MISSING_CHALLENGE";
    case TAF_DCS_CE_MIP_FA_ERR_STALE_CHALLENGE:
        return "TAF_DCS_CE_MIP_FA_ERR_STALE_CHALLENGE";
    case TAF_DCS_CE_MIP_HA_ERR_REASON_UNSPECIFIED:
        return "TAF_DCS_CE_MIP_HA_ERR_REASON_UNSPECIFIED";
    case TAF_DCS_CE_MIP_HA_ERR_ADMINISTRATIVELY_PROHIBITED:
        return "TAF_DCS_CE_MIP_HA_ERR_ADMINISTRATIVELY_PROHIBITED";
    case TAF_DCS_CE_MIP_HA_ERR_INSUFFICIENT_RESOURCES:
        return "TAF_DCS_CE_MIP_HA_ERR_INSUFFICIENT_RESOURCES";
    case TAF_DCS_CE_MIP_HA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE:
        return "TAF_DCS_CE_MIP_HA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE";
    case TAF_DCS_CE_MIP_HA_ERR_FA_AUTHENTICATION_FAILURE:
        return "TAF_DCS_CE_MIP_HA_ERR_FA_AUTHENTICATION_FAILURE";
    case TAF_DCS_CE_MIP_HA_ERR_REGISTRATION_ID_MISMATCH:
        return "TAF_DCS_CE_MIP_HA_ERR_REGISTRATION_ID_MISMATCH";
    case TAF_DCS_CE_MIP_HA_ERR_MALFORMED_REQUEST:
        return "TAF_DCS_CE_MIP_HA_ERR_MALFORMED_REQUEST";
    case TAF_DCS_CE_MIP_HA_ERR_UNKNOWN_HA_ADDR:
        return "TAF_DCS_CE_MIP_HA_ERR_UNKNOWN_HA_ADDR";
    case TAF_DCS_CE_MIP_HA_ERR_REVERSE_TUNNEL_UNAVAILABLE:
        return "TAF_DCS_CE_MIP_HA_ERR_REVERSE_TUNNEL_UNAVAILABLE";
    case TAF_DCS_CE_MIP_HA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET:
        return "TAF_DCS_CE_MIP_HA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET";
    case TAF_DCS_CE_MIP_HA_ERR_ENCAPSULATION_UNAVAILABLE:
        return "TAF_DCS_CE_MIP_HA_ERR_ENCAPSULATION_UNAVAILABLE";
    case TAF_DCS_CE_MIP_UNKNOWN:
        return "TAF_DCS_CE_MIP_UNKNOWN";
    default:
        LE_WARN("Invalid MIP reason code: %d", static_cast<int>(endCode));
        return "Invalid MIP reason code";
    }
}

taf_dcs_CallEndInternalReasonCode_t taf_DCSHelper::ConvertCallEndInternalReasonCode
(
    telux::common::InternalReasonCode endReasonCode
)
{
    using telux::common::InternalReasonCode;
    taf_dcs_CallEndInternalReasonCode_t result;

    switch (endReasonCode)
    {
    case InternalReasonCode::CE_RETRY:
        result = TAF_DCS_CE_INTERNAL_RETRY;
        break;
    case InternalReasonCode::CE_INTERNAL_ERROR:
        result = TAF_DCS_CE_INTERNAL_ERROR;
        break;
    case InternalReasonCode::CE_CALL_ENDED:
        result = TAF_DCS_CE_INTERNAL_CALL_ENDED;
        break;
    case InternalReasonCode::CE_INTERNAL_UNKNOWN_CAUSE_CODE:
        result = TAF_DCS_CE_INTERNAL_UNKNOWN_CAUSE_CODE203;
        break;
    case InternalReasonCode::CE_UNKNOWN_CAUSE_CODE:
        result = TAF_DCS_CE_INTERNAL_UNKNOWN_CAUSE_CODE204;
        break;
    case InternalReasonCode::CE_CLOSE_IN_PROGRESS:
        result = TAF_DCS_CE_INTERNAL_CLOSE_IN_PROGRESS;
        break;
    case InternalReasonCode::CE_NW_INITIATED_TERMINATION:
        result = TAF_DCS_CE_INTERNAL_NW_INITIATED_TERMINATION;
        break;
    case InternalReasonCode::CE_APP_PREEMPTED:
        result = TAF_DCS_CE_INTERNAL_APP_PREEMPTED;
        break;
    case InternalReasonCode::CE_ERR_PDN_IPV4_CALL_DISALLOWED:
        result = TAF_DCS_CE_INTERNAL_ERR_PDN_IPV4_CALL_DISALLOWED;
        break;
    case InternalReasonCode::CE_ERR_PDN_IPV4_CALL_THROTTLED:
        result = TAF_DCS_CE_INTERNAL_ERR_PDN_IPV4_CALL_THROTTLED;
        break;
    case InternalReasonCode::CE_ERR_PDN_IPV6_CALL_DISALLOWED:
        result = TAF_DCS_CE_INTERNAL_ERR_PDN_IPV6_CALL_DISALLOWED;
        break;
    case InternalReasonCode::CE_ERR_PDN_IPV6_CALL_THROTTLED:
        result = TAF_DCS_CE_INTERNAL_ERR_PDN_IPV6_CALL_THROTTLED;
        break;
    case InternalReasonCode::CE_MODEM_RESTART:
        result = TAF_DCS_CE_INTERNAL_MODEM_RESTART;
        break;
    case InternalReasonCode::CE_PDP_PPP_NOT_SUPPORTED:
        result = TAF_DCS_CE_INTERNAL_PDP_PPP_NOT_SUPPORTED;
        break;
    case InternalReasonCode::CE_UNPREFERRED_RAT:
        result = TAF_DCS_CE_INTERNAL_UNPREFERRED_RAT;
        break;
    case InternalReasonCode::CE_PHYS_LINK_CLOSE_IN_PROGRESS:
        result = TAF_DCS_CE_INTERNAL_PHYS_LINK_CLOSE_IN_PROGRESS;
        break;
    case InternalReasonCode::CE_APN_PENDING_HANDOVER:
        result = TAF_DCS_CE_INTERNAL_APN_PENDING_HANDOVER;
        break;
    case InternalReasonCode::CE_PROFILE_BEARER_INCOMPATIBLE:
        result = TAF_DCS_CE_INTERNAL_PROFILE_BEARER_INCOMPATIBLE;
        break;
    case InternalReasonCode::CE_MMGSDI_CARD_EVT:
        result = TAF_DCS_CE_INTERNAL_MMGSDI_CARD_EVT;
        break;
    case InternalReasonCode::CE_LPM_OR_PWR_DOWN:
        result = TAF_DCS_CE_INTERNAL_LPM_OR_PWR_DOWN;
        break;
    case InternalReasonCode::CE_APN_DISABLED:
        result = TAF_DCS_CE_INTERNAL_APN_DISABLED;
        break;
    case InternalReasonCode::CE_MPIT_EXPIRED:
        result = TAF_DCS_CE_INTERNAL_MPIT_EXPIRED;
        break;
    case InternalReasonCode::CE_IPV6_ADDR_TRANSFER_FAILED:
        result = TAF_DCS_CE_INTERNAL_IPV6_ADDR_TRANSFER_FAILED;
        break;
    case InternalReasonCode::CE_TRAT_SWAP_FAILED:
        result = TAF_DCS_CE_INTERNAL_TRAT_SWAP_FAILED;
        break;
    case InternalReasonCode::CE_EHRPD_TO_HRPD_FALLBACK:
        result = TAF_DCS_CE_INTERNAL_EHRPD_TO_HRPD_FALLBACK;
        break;
    case InternalReasonCode::CE_MANDATORY_APN_DISABLED:
        result = TAF_DCS_CE_INTERNAL_MANDATORY_APN_DISABLED;
        break;
    case InternalReasonCode::CE_MIP_CONFIG_FAILURE:
        result = TAF_DCS_CE_INTERNAL_MIP_CONFIG_FAILURE;
        break;
    case InternalReasonCode::CE_INTERNAL_PDN_INACTIVITY_TIMER_EXPIRED:
        result = TAF_DCS_CE_INTERNAL_INTERNAL_PDN_INACTIVITY_TIMER_EXPIRED;
        break;
    case InternalReasonCode::CE_MAX_V4_CONNECTIONS:
        result = TAF_DCS_CE_INTERNAL_MAX_V4_CONNECTIONS;
        break;
    case InternalReasonCode::CE_MAX_V6_CONNECTIONS:
        result = TAF_DCS_CE_INTERNAL_MAX_V6_CONNECTIONS;
        break;
    case InternalReasonCode::CE_APN_MISMATCH:
        result = TAF_DCS_CE_INTERNAL_APN_MISMATCH;
        break;
    case InternalReasonCode::CE_IP_VERSION_MISMATCH:
        result = TAF_DCS_CE_INTERNAL_IP_VERSION_MISMATCH;
        break;
    case InternalReasonCode::CE_DUN_CALL_DISALLOWED:
        result = TAF_DCS_CE_INTERNAL_DUN_CALL_DISALLOWED;
        break;
    case InternalReasonCode::CE_INVALID_PROFILE:
        result = TAF_DCS_CE_INTERNAL_INVALID_PROFILE;
        break;
    case InternalReasonCode::CE_INTERNAL_EPC_NONEPC_TRANSITION:
        result = TAF_DCS_CE_INTERNAL_EPC_NONEPC_TRANSITION;
        break;
    case InternalReasonCode::CE_INVALID_PROFILE_ID:
        result = TAF_DCS_CE_INTERNAL_INVALID_PROFILE_ID;
        break;
    case InternalReasonCode::CE_INTERNAL_CALL_ALREADY_PRESENT:
        result = TAF_DCS_CE_INTERNAL_CALL_ALREADY_PRESENT;
        break;
    case InternalReasonCode::CE_IFACE_IN_USE:
        result = TAF_DCS_CE_INTERNAL_IFACE_IN_USE;
        break;
    case InternalReasonCode::CE_IP_PDP_MISMATCH:
        result = TAF_DCS_CE_INTERNAL_IP_PDP_MISMATCH;
        break;
    case InternalReasonCode::CE_APN_DISALLOWED_ON_ROAMING:
        result = TAF_DCS_CE_INTERNAL_APN_DISALLOWED_ON_ROAMING;
        break;
    case InternalReasonCode::CE_APN_PARAM_CHANGE:
        result = TAF_DCS_CE_INTERNAL_APN_PARAM_CHANGE;
        break;
    case InternalReasonCode::CE_IFACE_IN_USE_CFG_MATCH:
        result = TAF_DCS_CE_INTERNAL_IFACE_IN_USE_CFG_MATCH;
        break;
    case InternalReasonCode::CE_NULL_APN_DISALLOWED:
        result = TAF_DCS_CE_INTERNAL_NULL_APN_DISALLOWED;
        break;
    case InternalReasonCode::CE_THERMAL_MITIGATION:
        result = TAF_DCS_CE_INTERNAL_THERMAL_MITIGATION;
        break;
    case InternalReasonCode::CE_SUBS_ID_MISMATCH:
        result = TAF_DCS_CE_INTERNAL_SUBS_ID_MISMATCH;
        break;
    case InternalReasonCode::CE_DATA_SETTINGS_DISABLED:
        result = TAF_DCS_CE_INTERNAL_DATA_SETTINGS_DISABLED;
        break;
    case InternalReasonCode::CE_DATA_ROAMING_SETTINGS_DISABLED:
        result = TAF_DCS_CE_INTERNAL_DATA_ROAMING_SETTINGS_DISABLED;
        break;
    case InternalReasonCode::CE_APN_FORMAT_INVALID:
        result = TAF_DCS_CE_INTERNAL_APN_FORMAT_INVALID;
        break;
    case InternalReasonCode::CE_DDS_CALL_ABORT:
        result = TAF_DCS_CE_INTERNAL_DDS_CALL_ABORT;
        break;
    case InternalReasonCode::CE_VALIDATION_FAILURE:
        result = TAF_DCS_CE_INTERNAL_VALIDATION_FAILURE;
        break;
    case InternalReasonCode::CE_PROFILES_NOT_COMPATIBLE:
        result = TAF_DCS_CE_INTERNAL_PROFILES_NOT_COMPATIBLE;
        break;
    case InternalReasonCode::CE_NULL_RESOLVED_APN_NO_MATCH:
        result = TAF_DCS_CE_INTERNAL_NULL_RESOLVED_APN_NO_MATCH;
        break;
    case InternalReasonCode::CE_INVALID_APN_NAME:
        result = TAF_DCS_CE_INTERNAL_INVALID_APN_NAME;
        break;
    default:
        result = TAF_DCS_CE_INTERNAL_UNKNOWN;
        break;
    }
    LE_DEBUG("TelSDK code: %d, TelAF code: %d",
             static_cast<uint32_t>(endReasonCode), static_cast<uint32_t>(result));
    return result;
}

const char *taf_DCSHelper::CallEndInternalReasonCodeToString
(
    taf_dcs_CallEndInternalReasonCode_t endCode
)
{
    switch(endCode)
    {
    case TAF_DCS_CE_INTERNAL_UNKNOWN:
        return "TAF_DCS_CE_INTERNAL_UNKNOWN";
    case TAF_DCS_CE_INTERNAL_RETRY:
        return "TAF_DCS_CE_INTERNAL_RETRY";
    case TAF_DCS_CE_INTERNAL_ERROR:
        return "TAF_DCS_CE_INTERNAL_ERROR";
    case TAF_DCS_CE_INTERNAL_CALL_ENDED:
        return "TAF_DCS_CE_INTERNAL_CALL_ENDED";
    case TAF_DCS_CE_INTERNAL_UNKNOWN_CAUSE_CODE203:
        return "TAF_DCS_CE_INTERNAL_UNKNOWN_CAUSE_CODE203";
    case TAF_DCS_CE_INTERNAL_UNKNOWN_CAUSE_CODE204:
        return "TAF_DCS_CE_INTERNAL_UNKNOWN_CAUSE_CODE204";
    case TAF_DCS_CE_INTERNAL_CLOSE_IN_PROGRESS:
        return "TAF_DCS_CE_INTERNAL_CLOSE_IN_PROGRESS";
    case TAF_DCS_CE_INTERNAL_NW_INITIATED_TERMINATION:
        return "TAF_DCS_CE_INTERNAL_NW_INITIATED_TERMINATION";
    case TAF_DCS_CE_INTERNAL_APP_PREEMPTED:
        return "TAF_DCS_CE_INTERNAL_APP_PREEMPTED";
    case TAF_DCS_CE_INTERNAL_ERR_PDN_IPV4_CALL_DISALLOWED:
        return "TAF_DCS_CE_INTERNAL_ERR_PDN_IPV4_CALL_DISALLOWED";
    case TAF_DCS_CE_INTERNAL_ERR_PDN_IPV4_CALL_THROTTLED:
        return "TAF_DCS_CE_INTERNAL_ERR_PDN_IPV4_CALL_THROTTLED";
    case TAF_DCS_CE_INTERNAL_ERR_PDN_IPV6_CALL_DISALLOWED:
        return "TAF_DCS_CE_INTERNAL_ERR_PDN_IPV6_CALL_DISALLOWED";
    case TAF_DCS_CE_INTERNAL_ERR_PDN_IPV6_CALL_THROTTLED:
        return "TAF_DCS_CE_INTERNAL_ERR_PDN_IPV6_CALL_THROTTLED";
    case TAF_DCS_CE_INTERNAL_MODEM_RESTART:
        return "TAF_DCS_CE_INTERNAL_MODEM_RESTART";
    case TAF_DCS_CE_INTERNAL_PDP_PPP_NOT_SUPPORTED:
        return "TAF_DCS_CE_INTERNAL_PDP_PPP_NOT_SUPPORTED";
    case TAF_DCS_CE_INTERNAL_UNPREFERRED_RAT:
        return "TAF_DCS_CE_INTERNAL_UNPREFERRED_RAT";
    case TAF_DCS_CE_INTERNAL_PHYS_LINK_CLOSE_IN_PROGRESS:
        return "TAF_DCS_CE_INTERNAL_PHYS_LINK_CLOSE_IN_PROGRESS";
    case TAF_DCS_CE_INTERNAL_APN_PENDING_HANDOVER:
        return "TAF_DCS_CE_INTERNAL_APN_PENDING_HANDOVER";
    case TAF_DCS_CE_INTERNAL_PROFILE_BEARER_INCOMPATIBLE:
        return "TAF_DCS_CE_INTERNAL_PROFILE_BEARER_INCOMPATIBLE";
    case TAF_DCS_CE_INTERNAL_MMGSDI_CARD_EVT:
        return "TAF_DCS_CE_INTERNAL_MMGSDI_CARD_EVT";
    case TAF_DCS_CE_INTERNAL_LPM_OR_PWR_DOWN:
        return "TAF_DCS_CE_INTERNAL_LPM_OR_PWR_DOWN";
    case TAF_DCS_CE_INTERNAL_APN_DISABLED:
        return "TAF_DCS_CE_INTERNAL_APN_DISABLED";
    case TAF_DCS_CE_INTERNAL_MPIT_EXPIRED:
        return "TAF_DCS_CE_INTERNAL_MPIT_EXPIRED";
    case TAF_DCS_CE_INTERNAL_IPV6_ADDR_TRANSFER_FAILED:
        return "TAF_DCS_CE_INTERNAL_IPV6_ADDR_TRANSFER_FAILED";
    case TAF_DCS_CE_INTERNAL_TRAT_SWAP_FAILED:
        return "TAF_DCS_CE_INTERNAL_TRAT_SWAP_FAILED";
    case TAF_DCS_CE_INTERNAL_EHRPD_TO_HRPD_FALLBACK:
        return "TAF_DCS_CE_INTERNAL_EHRPD_TO_HRPD_FALLBACK";
    case TAF_DCS_CE_INTERNAL_MANDATORY_APN_DISABLED:
        return "TAF_DCS_CE_INTERNAL_MANDATORY_APN_DISABLED";
    case TAF_DCS_CE_INTERNAL_MIP_CONFIG_FAILURE:
        return "TAF_DCS_CE_INTERNAL_MIP_CONFIG_FAILURE";
    case TAF_DCS_CE_INTERNAL_INTERNAL_PDN_INACTIVITY_TIMER_EXPIRED:
        return "TAF_DCS_CE_INTERNAL_INTERNAL_PDN_INACTIVITY_TIMER_EXPIRED";
    case TAF_DCS_CE_INTERNAL_MAX_V4_CONNECTIONS:
        return "TAF_DCS_CE_INTERNAL_MAX_V4_CONNECTIONS";
    case TAF_DCS_CE_INTERNAL_MAX_V6_CONNECTIONS:
        return "TAF_DCS_CE_INTERNAL_MAX_V6_CONNECTIONS";
    case TAF_DCS_CE_INTERNAL_APN_MISMATCH:
        return "TAF_DCS_CE_INTERNAL_APN_MISMATCH";
    case TAF_DCS_CE_INTERNAL_IP_VERSION_MISMATCH:
        return "TAF_DCS_CE_INTERNAL_IP_VERSION_MISMATCH";
    case TAF_DCS_CE_INTERNAL_DUN_CALL_DISALLOWED:
        return "TAF_DCS_CE_INTERNAL_DUN_CALL_DISALLOWED";
    case TAF_DCS_CE_INTERNAL_INVALID_PROFILE:
        return "TAF_DCS_CE_INTERNAL_INVALID_PROFILE";
    case TAF_DCS_CE_INTERNAL_EPC_NONEPC_TRANSITION:
        return "TAF_DCS_CE_INTERNAL_EPC_NONEPC_TRANSITION";
    case TAF_DCS_CE_INTERNAL_INVALID_PROFILE_ID:
        return "TAF_DCS_CE_INTERNAL_INVALID_PROFILE_ID";
    case TAF_DCS_CE_INTERNAL_CALL_ALREADY_PRESENT:
        return "TAF_DCS_CE_INTERNAL_CALL_ALREADY_PRESENT";
    case TAF_DCS_CE_INTERNAL_IFACE_IN_USE:
        return "TAF_DCS_CE_INTERNAL_IFACE_IN_USE";
    case TAF_DCS_CE_INTERNAL_IP_PDP_MISMATCH:
        return "TAF_DCS_CE_INTERNAL_IP_PDP_MISMATCH";
    case TAF_DCS_CE_INTERNAL_APN_DISALLOWED_ON_ROAMING:
        return "TAF_DCS_CE_INTERNAL_APN_DISALLOWED_ON_ROAMING";
    case TAF_DCS_CE_INTERNAL_APN_PARAM_CHANGE:
        return "TAF_DCS_CE_INTERNAL_APN_PARAM_CHANGE";
    case TAF_DCS_CE_INTERNAL_IFACE_IN_USE_CFG_MATCH:
        return "TAF_DCS_CE_INTERNAL_IFACE_IN_USE_CFG_MATCH";
    case TAF_DCS_CE_INTERNAL_NULL_APN_DISALLOWED:
        return "TAF_DCS_CE_INTERNAL_NULL_APN_DISALLOWED";
    case TAF_DCS_CE_INTERNAL_THERMAL_MITIGATION:
        return "TAF_DCS_CE_INTERNAL_THERMAL_MITIGATION";
    case TAF_DCS_CE_INTERNAL_SUBS_ID_MISMATCH:
        return "TAF_DCS_CE_INTERNAL_SUBS_ID_MISMATCH";
    case TAF_DCS_CE_INTERNAL_DATA_SETTINGS_DISABLED:
        return "TAF_DCS_CE_INTERNAL_DATA_SETTINGS_DISABLED";
    case TAF_DCS_CE_INTERNAL_DATA_ROAMING_SETTINGS_DISABLED:
        return "TAF_DCS_CE_INTERNAL_DATA_ROAMING_SETTINGS_DISABLED";
    case TAF_DCS_CE_INTERNAL_APN_FORMAT_INVALID:
        return "TAF_DCS_CE_INTERNAL_APN_FORMAT_INVALID";
    case TAF_DCS_CE_INTERNAL_DDS_CALL_ABORT:
        return "TAF_DCS_CE_INTERNAL_DDS_CALL_ABORT";
    case TAF_DCS_CE_INTERNAL_VALIDATION_FAILURE:
        return "TAF_DCS_CE_INTERNAL_VALIDATION_FAILURE";
    case TAF_DCS_CE_INTERNAL_PROFILES_NOT_COMPATIBLE:
        return "TAF_DCS_CE_INTERNAL_PROFILES_NOT_COMPATIBLE";
    case TAF_DCS_CE_INTERNAL_NULL_RESOLVED_APN_NO_MATCH:
        return "TAF_DCS_CE_INTERNAL_NULL_RESOLVED_APN_NO_MATCH";
    case TAF_DCS_CE_INTERNAL_INVALID_APN_NAME:
        return "TAF_DCS_CE_INTERNAL_INVALID_APN_NAME";
    default:
        LE_WARN("Invalid internal reason code: %d", static_cast<int>(endCode));
        return "Invalid internal reason code";
    }
}

taf_dcs_CallEndCallManagerReasonCode_t taf_DCSHelper::ConvertCallEndCallManagerReasonCode
(
    telux::common::CallManagerReasonCode endReasonCode
)
{
    taf_dcs_CallEndCallManagerReasonCode_t result;
    using telux::common::CallManagerReasonCode;

    switch (endReasonCode)
    {
    case CallManagerReasonCode::CE_CDMA_LOCK:
        result = TAF_DCS_CE_CM_CDMA_LOCK;
        break;
    case CallManagerReasonCode::CE_INTERCEPT:
        result = TAF_DCS_CE_CM_INTERCEPT;
        break;
    case CallManagerReasonCode::CE_REORDER:
        result = TAF_DCS_CE_CM_REORDER;
        break;
    case CallManagerReasonCode::CE_REL_SO_REJ:
        result = TAF_DCS_CE_CM_REL_SO_REJ;
        break;
    case CallManagerReasonCode::CE_INCOM_CALL:
        result = TAF_DCS_CE_CM_INCOM_CALL;
        break;
    case CallManagerReasonCode::CE_ALERT_STOP:
        result = TAF_DCS_CE_CM_ALERT_STOP;
        break;
    case CallManagerReasonCode::CE_ACTIVATION:
        result = TAF_DCS_CE_CM_ACTIVATION;
        break;
    case CallManagerReasonCode::CE_MAX_ACCESS_PROBE:
        result = TAF_DCS_CE_CM_MAX_ACCESS_PROBE;
        break;
    case CallManagerReasonCode::CE_CCS_NOT_SUPPORTED_BY_BS:
        result = TAF_DCS_CE_CM_CCS_NOT_SUPPORTED_BY_BS;
        break;
    case CallManagerReasonCode::CE_NO_RESPONSE_FROM_BS:
        result = TAF_DCS_CE_CM_NO_RESPONSE_FROM_BS;
        break;
    case CallManagerReasonCode::CE_REJECTED_BY_BS:
        result = TAF_DCS_CE_CM_REJECTED_BY_BS;
        break;
    case CallManagerReasonCode::CE_INCOMPATIBLE:
        result = TAF_DCS_CE_CM_INCOMPATIBLE;
        break;
    case CallManagerReasonCode::CE_ALREADY_IN_TC:
        result = TAF_DCS_CE_CM_ALREADY_IN_TC;
        break;
    case CallManagerReasonCode::CE_USER_CALL_ORIG_DURING_GPS:
        result = TAF_DCS_CE_CM_USER_CALL_ORIG_DURING_GPS;
        break;
    case CallManagerReasonCode::CE_USER_CALL_ORIG_DURING_SMS:
        result = TAF_DCS_CE_CM_USER_CALL_ORIG_DURING_SMS;
        break;
    case CallManagerReasonCode::CE_NO_CDMA_SRV:
        result = TAF_DCS_CE_CM_NO_CDMA_SRV;
        break;
    case CallManagerReasonCode::CE_MC_ABORT:
        result = TAF_DCS_CE_CM_MC_ABORT;
        break;
    case CallManagerReasonCode::CE_PSIST_NG:
        result = TAF_DCS_CE_CM_PSIST_NG;
        break;
    case CallManagerReasonCode::CE_UIM_NOT_PRESENT:
        result = TAF_DCS_CE_CM_UIM_NOT_PRESENT;
        break;
    case CallManagerReasonCode::CE_RETRY_ORDER:
        result = TAF_DCS_CE_CM_RETRY_ORDER;
        break;
    case CallManagerReasonCode::CE_ACCESS_BLOCK:
        result = TAF_DCS_CE_CM_ACCESS_BLOCK;
        break;
    case CallManagerReasonCode::CEACCESS_BLOCK_ALL:
        result = TAF_DCS_CE_CM_ACCESS_BLOCK_ALL;
        break;
    case CallManagerReasonCode::CE_IS707B_MAX_ACC:
        result = TAF_DCS_CE_CM_IS707B_MAX_ACC;
        break;
    case CallManagerReasonCode::CE_THERMAL_EMERGENCY:
        result = TAF_DCS_CE_CM_THERMAL_EMERGENCY;
        break;
    case CallManagerReasonCode::CE_CALL_ORIG_THROTTLED:
        result = TAF_DCS_CE_CM_CALL_ORIG_THROTTLED;
        break;
    case CallManagerReasonCode::CE_USER_CALL_ORIG_DURING_VOICE_CALL:
        result = TAF_DCS_CE_CM_USER_CALL_ORIG_DURING_VOICE_CALL;
        break;
    case CallManagerReasonCode::CE_CONF_FAILED:
        result = TAF_DCS_CE_CM_CONF_FAILED;
        break;
    case CallManagerReasonCode::CE_INCOM_REJ:
        result = TAF_DCS_CE_CM_INCOM_REJ;
        break;
    case CallManagerReasonCode::CE_NEW_NO_GW_SRV:
        result = TAF_DCS_CE_CM_NEW_NO_GW_SRV;
        break;
    case CallManagerReasonCode::CE_NEW_NO_GPRS_CONTEXT:
        result = TAF_DCS_CE_CM_NEW_NO_GPRS_CONTEXT;
        break;
    case CallManagerReasonCode::CE_NEW_ILLEGAL_MS:
        result = TAF_DCS_CE_CM_NEW_ILLEGAL_MS;
        break;
    case CallManagerReasonCode::CE_NEW_ILLEGAL_ME:
        result = TAF_DCS_CE_CM_NEW_ILLEGAL_ME;
        break;
    case CallManagerReasonCode::CE_NEW_GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED:
        result = TAF_DCS_CE_CM_NEW_GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED;
        break;
    case CallManagerReasonCode::CE_NEW_GPRS_SERVICES_NOT_ALLOWED:
        result = TAF_DCS_CE_CM_NEW_GPRS_SERVICES_NOT_ALLOWED;
        break;
    case CallManagerReasonCode::CE_NEW_MS_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK:
        result = TAF_DCS_CE_CM_NEW_MS_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK;
        break;
    case CallManagerReasonCode::CE_NEW_IMPLICITLY_DETACHED:
        result = TAF_DCS_CE_CM_NEW_IMPLICITLY_DETACHED;
        break;
    case CallManagerReasonCode::CE_NEW_PLMN_NOT_ALLOWED:
        result = TAF_DCS_CE_CM_NEW_PLMN_NOT_ALLOWED;
        break;
    case CallManagerReasonCode::CE_NEW_LA_NOT_ALLOWED:
        result = TAF_DCS_CE_CM_NEW_LA_NOT_ALLOWED;
        break;
    case CallManagerReasonCode::CE_NEW_GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN:
        result = TAF_DCS_CE_CM_NEW_GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN;
        break;
    case CallManagerReasonCode::CE_NEW_PDP_DUPLICATE:
        result = TAF_DCS_CE_CM_NEW_PDP_DUPLICATE;
        break;
    case CallManagerReasonCode::CE_NEW_UE_RAT_CHANGE:
        result = TAF_DCS_CE_CM_NEW_UE_RAT_CHANGE;
        break;
    case CallManagerReasonCode::CE_NEW_CONGESTION:
        result = TAF_DCS_CE_CM_NEW_CONGESTION;
        break;
    case CallManagerReasonCode::CE_NEW_NO_PDP_CONTEXT_ACTIVATED:
        result = TAF_DCS_CE_CM_NEW_NO_PDP_CONTEXT_ACTIVATED;
        break;
    case CallManagerReasonCode::CE_NEW_ACCESS_CLASS_DSAC_REJECTION:
        result = TAF_DCS_CE_CM_NEW_ACCESS_CLASS_DSAC_REJECTION;
        break;
    case CallManagerReasonCode::CE_PDP_ACTIVATE_MAX_RETRY_FAILED:
        result = TAF_DCS_CE_CM_PDP_ACTIVATE_MAX_RETRY_FAILED;
        break;
    case CallManagerReasonCode::CE_RAB_FAILURE:
        result = TAF_DCS_CE_CM_RAB_FAILURE;
        break;
    case CallManagerReasonCode::CE_ESM_UNKNOWN_EPS_BEARER_CONTEXT:
        result = TAF_DCS_CE_CM_ESM_UNKNOWN_EPS_BEARER_CONTEXT;
        break;
    case CallManagerReasonCode::CE_DRB_RELEASED_AT_RRC:
        result = TAF_DCS_CE_CM_DRB_RELEASED_AT_RRC;
        break;
    case CallManagerReasonCode::CE_NAS_SIG_CONN_RELEASED:
        result = TAF_DCS_CE_CM_NAS_SIG_CONN_RELEASED;
        break;
    case CallManagerReasonCode::CE_REASON_EMM_DETACHED:
        result = TAF_DCS_CE_CM_REASON_EMM_DETACHED;
        break;
    case CallManagerReasonCode::CE_EMM_ATTACH_FAILED:
        result = TAF_DCS_CE_CM_EMM_ATTACH_FAILED;
        break;
    case CallManagerReasonCode::CE_EMM_ATTACH_STARTED:
        result = TAF_DCS_CE_CM_EMM_ATTACH_STARTED;
        break;
    case CallManagerReasonCode::CE_LTE_NAS_SERVICE_REQ_FAILED:
        result = TAF_DCS_CE_CM_LTE_NAS_SERVICE_REQ_FAILED;
        break;
    case CallManagerReasonCode::CE_ESM_ACTIVE_DEDICATED_BEARER_REACTIVATED_BY_NW:
        result = TAF_DCS_CE_CM_ESM_ACTIVE_DEDICATED_BEARER_REACTIVATED_BY_NW;
        break;
    case CallManagerReasonCode::CE_ESM_LOWER_LAYER_FAILURE:
        result = TAF_DCS_CE_CM_ESM_LOWER_LAYER_FAILURE;
        break;
    case CallManagerReasonCode::CE_ESM_SYNC_UP_WITH_NW:
        result = TAF_DCS_CE_CM_ESM_SYNC_UP_WITH_NW;
        break;
    case CallManagerReasonCode::CE_ESM_NW_ACTIVATED_DED_BEARER_WITH_ID_OF_DEF_BEARER:
        result = TAF_DCS_CE_CM_ESM_NW_ACTIVATED_DED_BEARER_WITH_ID_OF_DEF_BEARER;
        break;
    case CallManagerReasonCode::CE_ESM_BAD_OTA_MESSAGE:
        result = TAF_DCS_CE_CM_ESM_BAD_OTA_MESSAGE;
        break;
    case CallManagerReasonCode::CE_ESM_DS_REJECTED_THE_CALL:
        result = TAF_DCS_CE_CM_ESM_DS_REJECTED_THE_CALL;
        break;
    case CallManagerReasonCode::CE_ESM_CONTEXT_TRANSFERED_DUE_TO_IRAT:
        result = TAF_DCS_CE_CM_ESM_CONTEXT_TRANSFERED_DUE_TO_IRAT;
        break;
    case CallManagerReasonCode::CE_DS_EXPLICIT_DEACT:
        result = TAF_DCS_CE_CM_DS_EXPLICIT_DEACT;
        break;
    case CallManagerReasonCode::CE_ESM_LOCAL_CAUSE_NONE:
        result = TAF_DCS_CE_CM_ESM_LOCAL_CAUSE_NONE;
        break;
    case CallManagerReasonCode::CE_LTE_NAS_SERVICE_REQ_FAILED_NO_THROTTLE:
        result = TAF_DCS_CE_CM_LTE_NAS_SERVICE_REQ_FAILED_NO_THROTTLE;
        break;
    case CallManagerReasonCode::CE_ACL_FAILURE:
        result = TAF_DCS_CE_CM_ACL_FAILURE;
        break;
    case CallManagerReasonCode::CE_LTE_NAS_SERVICE_REQ_FAILED_DS_DISALLOW:
        result = TAF_DCS_CE_CM_LTE_NAS_SERVICE_REQ_FAILED_DS_DISALLOW;
        break;
    case CallManagerReasonCode::CE_EMM_T3417_EXPIRED:
        result = TAF_DCS_CE_CM_EMM_T3417_EXPIRED;
        break;
    case CallManagerReasonCode::CE_EMM_T3417_EXT_EXPIRED:
        result = TAF_DCS_CE_CM_EMM_T3417_EXT_EXPIRED;
        break;
    case CallManagerReasonCode::CE_LRRC_UL_DATA_CNF_FAILURE_TXN:
        result = TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_TXN;
        break;
    case CallManagerReasonCode::CE_LRRC_UL_DATA_CNF_FAILURE_HO:
        result = TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_HO;
        break;
    case CallManagerReasonCode::CE_LRRC_UL_DATA_CNF_FAILURE_CONN_REL:
        result = TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_CONN_REL;
        break;
    case CallManagerReasonCode::CE_LRRC_UL_DATA_CNF_FAILURE_RLF:
        result = TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_RLF;
        break;
    case CallManagerReasonCode::CE_LRRC_UL_DATA_CNF_FAILURE_CTRL_NOT_CONN:
        result = TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_CTRL_NOT_CONN;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_EST_FAILURE:
        result = TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_EST_FAILURE_ABORTED:
        result = TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_ABORTED;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_EST_FAILURE_ACCESS_BARRED:
        result = TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_ACCESS_BARRED;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_EST_FAILURE_CELL_RESEL:
        result = TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_CELL_RESEL;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_EST_FAILURE_CONFIG_FAILURE:
        result = TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_CONFIG_FAILURE;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_EST_FAILURE_TIMER_EXPIRED:
        result = TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_TIMER_EXPIRED;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_EST_FAILURE_LINK_FAILURE:
        result = TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_LINK_FAILURE;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_EST_FAILURE_NOT_CAMPED:
        result = TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_NOT_CAMPED;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_EST_FAILURE_SI_FAILURE:
        result = TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_SI_FAILURE;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_EST_FAILURE_CONN_REJECT:
        result = TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_CONN_REJECT;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_REL_NORMAL:
        result = TAF_DCS_CE_CM_LRRC_CONN_REL_NORMAL;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_REL_RLF:
        result = TAF_DCS_CE_CM_LRRC_CONN_REL_RLF;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_REL_CRE_FAILURE:
        result = TAF_DCS_CE_CM_LRRC_CONN_REL_CRE_FAILURE;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_REL_OOS_DURING_CRE:
        result = TAF_DCS_CE_CM_LRRC_CONN_REL_OOS_DURING_CRE;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_REL_ABORTED:
        result = TAF_DCS_CE_CM_LRRC_CONN_REL_ABORTED;
        break;
    case CallManagerReasonCode::CE_LRRC_CONN_REL_SIB_READ_ERROR:
        result = TAF_DCS_CE_CM_LRRC_CONN_REL_SIB_READ_ERROR;
        break;
    case CallManagerReasonCode::CE_DETACH_WITH_REATTACH_LTE_NW_DETACH:
        result = TAF_DCS_CE_CM_DETACH_WITH_REATTACH_LTE_NW_DETACH;
        break;
    case CallManagerReasonCode::CE_DETACH_WITH_OUT_REATTACH_LTE_NW_DETACH:
        result = TAF_DCS_CE_CM_DETACH_WITH_OUT_REATTACH_LTE_NW_DETACH;
        break;
    case CallManagerReasonCode::CE_ESM_PROC_TIME_OUT:
        result = TAF_DCS_CE_CM_ESM_PROC_TIME_OUT;
        break;
    case CallManagerReasonCode::CE_INVALID_CONNECTION_ID:
        result = TAF_DCS_CE_CM_INVALID_CONNECTION_ID;
        break;
    case CallManagerReasonCode::CE_INVALID_NSAPI:
        result = TAF_DCS_CE_CM_INVALID_NSAPI;
        break;
    case CallManagerReasonCode::CE_INVALID_PRI_NSAPI:
        result = TAF_DCS_CE_CM_INVALID_PRI_NSAPI;
        break;
    case CallManagerReasonCode::CE_INVALID_FIELD:
        result = TAF_DCS_CE_CM_INVALID_FIELD;
        break;
    case CallManagerReasonCode::CE_RAB_SETUP_FAILURE:
        result = TAF_DCS_CE_CM_RAB_SETUP_FAILURE;
        break;
    case CallManagerReasonCode::CE_PDP_ESTABLISH_MAX_TIMEOUT:
        result = TAF_DCS_CE_CM_PDP_ESTABLISH_MAX_TIMEOUT;
        break;
    case CallManagerReasonCode::CE_PDP_MODIFY_MAX_TIMEOUT:
        result = TAF_DCS_CE_CM_PDP_MODIFY_MAX_TIMEOUT;
        break;
    case CallManagerReasonCode::CE_PDP_INACTIVE_MAX_TIMEOUT:
        result = TAF_DCS_CE_CM_PDP_INACTIVE_MAX_TIMEOUT;
        break;
    case CallManagerReasonCode::CE_PDP_LOWERLAYER_ERROR:
        result = TAF_DCS_CE_CM_PDP_LOWERLAYER_ERROR;
        break;
    case CallManagerReasonCode::CE_PPD_UNKNOWN_REASON:
        result = TAF_DCS_CE_CM_PPD_UNKNOWN_REASON;
        break;
    case CallManagerReasonCode::CE_PDP_MODIFY_COLLISION:
        result = TAF_DCS_CE_CM_PDP_MODIFY_COLLISION;
        break;
    case CallManagerReasonCode::CE_PDP_MBMS_REQUEST_COLLISION:
        result = TAF_DCS_CE_CM_PDP_MBMS_REQUEST_COLLISION;
        break;
    case CallManagerReasonCode::CE_MBMS_DUPLICATE:
        result = TAF_DCS_CE_CM_MBMS_DUPLICATE;
        break;
    case CallManagerReasonCode::CE_SM_PS_DETACHED:
        result = TAF_DCS_CE_CM_SM_PS_DETACHED;
        break;
    case CallManagerReasonCode::CE_SM_NO_RADIO_AVAILABLE:
        result = TAF_DCS_CE_CM_SM_NO_RADIO_AVAILABLE;
        break;
    case CallManagerReasonCode::CE_SM_ABORT_SERVICE_NOT_AVAILABLE:
        result = TAF_DCS_CE_CM_SM_ABORT_SERVICE_NOT_AVAILABLE;
        break;
    case CallManagerReasonCode::CE_MESSAGE_EXCEED_MAX_L2_LIMIT:
        result = TAF_DCS_CE_CM_MESSAGE_EXCEED_MAX_L2_LIMIT;
        break;
    case CallManagerReasonCode::CE_SM_NAS_SRV_REQ_FAILURE:
        result = TAF_DCS_CE_CM_SM_NAS_SRV_REQ_FAILURE;
        break;
    case CallManagerReasonCode::CE_RRC_CONN_EST_FAILURE_REQ_ERROR:
        result = TAF_DCS_CE_CM_RRC_CONN_EST_FAILURE_REQ_ERROR;
        break;
    case CallManagerReasonCode::CE_RRC_CONN_EST_FAILURE_TAI_CHANGE:
        result = TAF_DCS_CE_CM_RRC_CONN_EST_FAILURE_TAI_CHANGE;
        break;
    case CallManagerReasonCode::CE_RRC_CONN_EST_FAILURE_RF_UNAVAILABLE:
        result = TAF_DCS_CE_CM_RRC_CONN_EST_FAILURE_RF_UNAVAILABLE;
        break;
    case CallManagerReasonCode::CE_RRC_CONN_REL_ABORTED_IRAT_SUCCESS:
        result = TAF_DCS_CE_CM_RRC_CONN_REL_ABORTED_IRAT_SUCCESS;
        break;
    case CallManagerReasonCode::CE_RRC_CONN_REL_RLF_SEC_NOT_ACTIVE:
        result = TAF_DCS_CE_CM_RRC_CONN_REL_RLF_SEC_NOT_ACTIVE;
        break;
    case CallManagerReasonCode::CE_RRC_CONN_REL_IRAT_TO_LTE_ABORTED:
        result = TAF_DCS_CE_CM_RRC_CONN_REL_IRAT_TO_LTE_ABORTED;
        break;
    case CallManagerReasonCode::CE_RRC_CONN_REL_IRAT_FROM_LTE_TO_G_CCO_SUCCESS:
        result = TAF_DCS_CE_CM_RRC_CONN_REL_IRAT_FROM_LTE_TO_G_CCO_SUCCESS;
        break;
    case CallManagerReasonCode::CE_RRC_CONN_REL_IRAT_FROM_LTE_TO_G_CCO_ABORTED:
        result = TAF_DCS_CE_CM_RRC_CONN_REL_IRAT_FROM_LTE_TO_G_CCO_ABORTED;
        break;
    case CallManagerReasonCode::CE_IMSI_UNKNOWN_IN_HSS:
        result = TAF_DCS_CE_CM_IMSI_UNKNOWN_IN_HSS;
        break;
    case CallManagerReasonCode::CE_IMEI_NOT_ACCEPTED:
        result = TAF_DCS_CE_CM_IMEI_NOT_ACCEPTED;
        break;
    case CallManagerReasonCode::CE_EPS_SERVICES_AND_NON_EPS_SERVICES_NOT_ALLOWED:
        result = TAF_DCS_CE_CM_EPS_SERVICES_AND_NON_EPS_SERVICES_NOT_ALLOWED;
        break;
    case CallManagerReasonCode::CE_EPS_SERVICES_NOT_ALLOWED_IN_PLMN:
        result = TAF_DCS_CE_CM_EPS_SERVICES_NOT_ALLOWED_IN_PLMN;
        break;
    case CallManagerReasonCode::CE_MSC_TEMPORARILY_NOT_REACHABLE:
        result = TAF_DCS_CE_CM_MSC_TEMPORARILY_NOT_REACHABLE;
        break;
    case CallManagerReasonCode::CE_CS_DOMAIN_NOT_AVAILABLE:
        result = TAF_DCS_CE_CM_CS_DOMAIN_NOT_AVAILABLE;
        break;
    case CallManagerReasonCode::CE_ESM_FAILURE:
        result = TAF_DCS_CE_CM_ESM_FAILURE;
        break;
    case CallManagerReasonCode::CE_MAC_FAILURE:
        result = TAF_DCS_CE_CM_MAC_FAILURE;
        break;
    case CallManagerReasonCode::CE_SYNCH_FAILURE:
        result = TAF_DCS_CE_CM_SYNCH_FAILURE;
        break;
    case CallManagerReasonCode::CE_UE_SECURITY_CAPABILITIES_MISMATCH:
        result = TAF_DCS_CE_CM_UE_SECURITY_CAPABILITIES_MISMATCH;
        break;
    case CallManagerReasonCode::CE_SECURITY_MODE_REJ_UNSPECIFIED:
        result = TAF_DCS_CE_CM_SECURITY_MODE_REJ_UNSPECIFIED;
        break;
    case CallManagerReasonCode::CE_NON_EPS_AUTH_UNACCEPTABLE:
        result = TAF_DCS_CE_CM_NON_EPS_AUTH_UNACCEPTABLE;
        break;
    case CallManagerReasonCode::CE_CS_FALLBACK_CALL_EST_NOT_ALLOWED:
        result = TAF_DCS_CE_CM_CS_FALLBACK_CALL_EST_NOT_ALLOWED;
        break;
    case CallManagerReasonCode::CE_NO_EPS_BEARER_CONTEXT_ACTIVATED:
        result = TAF_DCS_CE_CM_NO_EPS_BEARER_CONTEXT_ACTIVATED;
        break;
    case CallManagerReasonCode::CE_EMM_INVALID_STATE:
        result = TAF_DCS_CE_CM_EMM_INVALID_STATE;
        break;
    case CallManagerReasonCode::CE_NAS_LAYER_FAILURE:
        result = TAF_DCS_CE_CM_NAS_LAYER_FAILURE;
        break;
    case CallManagerReasonCode::CE_MULTI_PDN_NOT_ALLOWED:
        result = TAF_DCS_CE_CM_MULTI_PDN_NOT_ALLOWED;
        break;
    case CallManagerReasonCode::CE_EMBMS_NOT_ENABLED:
        result = TAF_DCS_CE_CM_EMBMS_NOT_ENABLED;
        break;
    case CallManagerReasonCode::CE_PENDING_REDIAL_CALL_CLEANUP:
        result = TAF_DCS_CE_CM_PENDING_REDIAL_CALL_CLEANUP;
        break;
    case CallManagerReasonCode::CE_EMBMS_REGULAR_DEACTIVATION:
        result = TAF_DCS_CE_CM_EMBMS_REGULAR_DEACTIVATION;
        break;
    case CallManagerReasonCode::CE_TLB_REGULAR_DEACTIVATION:
        result = TAF_DCS_CE_CM_TLB_REGULAR_DEACTIVATION;
        break;
    case CallManagerReasonCode::CE_LOWER_LAYER_REGISTRATION_FAILURE:
        result = TAF_DCS_CE_CM_LOWER_LAYER_REGISTRATION_FAILURE;
        break;
    case CallManagerReasonCode::CE_DETACH_EPS_SERVICES_NOT_ALLOWED:
        result = TAF_DCS_CE_CM_DETACH_EPS_SERVICES_NOT_ALLOWED;
        break;
    case CallManagerReasonCode::CE_SM_INTERNAL_PDP_DEACTIVATION:
        result = TAF_DCS_CE_CM_SM_INTERNAL_PDP_DEACTIVATION;
        break;
    case CallManagerReasonCode::CE_UNSUPPORTED_1X_PREV:
        result = TAF_DCS_CE_CM_UNSUPPORTED_1X_PREV;
        break;
    case CallManagerReasonCode::CE_CD_GEN_OR_BUSY:
        result = TAF_DCS_CE_CM_CD_GEN_OR_BUSY;
        break;
    case CallManagerReasonCode::CE_CD_BILL_OR_AUTH:
        result = TAF_DCS_CE_CM_CD_BILL_OR_AUTH;
        break;
    case CallManagerReasonCode::CE_CHG_HDR:
        result = TAF_DCS_CE_CM_CHG_HDR;
        break;
    case CallManagerReasonCode::CE_EXIT_HDR:
        result = TAF_DCS_CE_CM_EXIT_HDR;
        break;
    case CallManagerReasonCode::CE_HDR_NO_SESSION:
        result = TAF_DCS_CE_CM_HDR_NO_SESSION;
        break;
    case CallManagerReasonCode::CE_HDR_ORIG_DURING_GPS_FIX:
        result = TAF_DCS_CE_CM_HDR_ORIG_DURING_GPS_FIX;
        break;
    case CallManagerReasonCode::CE_HDR_CS_TIMEOUT:
        result = TAF_DCS_CE_CM_HDR_CS_TIMEOUT;
        break;
    case CallManagerReasonCode::CE_HDR_RELEASED_BY_CM:
        result = TAF_DCS_CE_CM_HDR_RELEASED_BY_CM;
        break;
    case CallManagerReasonCode::CE_COLLOC_ACQ_FAIL:
        result = TAF_DCS_CE_CM_COLLOC_ACQ_FAIL;
        break;
    case CallManagerReasonCode::CE_OTASP_COMMIT_IN_PROG:
        result = TAF_DCS_CE_CM_OTASP_COMMIT_IN_PROG;
        break;
    case CallManagerReasonCode::CE_NO_HYBR_HDR_SRV:
        result = TAF_DCS_CE_CM_NO_HYBR_HDR_SRV;
        break;
    case CallManagerReasonCode::CE_HDR_NO_LOCK_GRANTED:
        result = TAF_DCS_CE_CM_HDR_NO_LOCK_GRANTED;
        break;
    case CallManagerReasonCode::CE_HOLD_OTHER_IN_PROG:
        result = TAF_DCS_CE_CM_HOLD_OTHER_IN_PROG;
        break;
    case CallManagerReasonCode::CE_HDR_FADE:
        result = TAF_DCS_CE_CM_HDR_FADE;
        break;
    case CallManagerReasonCode::CE_HDR_ACC_FAIL:
        result = TAF_DCS_CE_CM_HDR_ACC_FAIL;
        break;
    case CallManagerReasonCode::CE_CLIENT_END:
        result = TAF_DCS_CE_CM_CLIENT_END;
        break;
    case CallManagerReasonCode::CE_NO_SRV:
        result = TAF_DCS_CE_CM_NO_SRV;
        break;
    case CallManagerReasonCode::CE_FADE:
        result = TAF_DCS_CE_CM_FADE;
        break;
    case CallManagerReasonCode::CE_REL_NORMAL:
        result = TAF_DCS_CE_CM_REL_NORMAL;
        break;
    case CallManagerReasonCode::CE_ACC_IN_PROG:
        result = TAF_DCS_CE_CM_ACC_IN_PROG;
        break;
    case CallManagerReasonCode::CE_ACC_FAIL:
        result = TAF_DCS_CE_CM_ACC_FAIL;
        break;
    case CallManagerReasonCode::CE_REDIR_OR_HANDOFF:
        result = TAF_DCS_CE_CM_REDIR_OR_HANDOFF;
        break;
    case CallManagerReasonCode::CE_CM_UNKNOWN_ERROR:
        result = TAF_DCS_CE_CM_CM_UNKNOWN_ERROR;
        break;
    case CallManagerReasonCode::CE_OFFLINE:
        result = TAF_DCS_CE_CM_OFFLINE;
        break;
    case CallManagerReasonCode::CE_EMERGENCY_MODE:
        result = TAF_DCS_CE_CM_EMERGENCY_MODE;
        break;
    case CallManagerReasonCode::CE_PHONE_IN_USE:
        result = TAF_DCS_CE_CM_PHONE_IN_USE;
        break;
    case CallManagerReasonCode::CE_INVALID_MODE:
        result = TAF_DCS_CE_CM_INVALID_MODE;
        break;
    case CallManagerReasonCode::CE_INVALID_SIM_STATE:
        result = TAF_DCS_CE_CM_INVALID_SIM_STATE;
        break;
    case CallManagerReasonCode::CE_NO_COLLOC_HDR:
        result = TAF_DCS_CE_CM_NO_COLLOC_HDR;
        break;
    case CallManagerReasonCode::CE_CALL_CONTROL_REJECTED:
        result = TAF_DCS_CE_CM_CALL_CONTROL_REJECTED;
        break;
    default:
        result = TAF_DCS_CE_CM_UNKNOWN;
        break;
    }
    LE_DEBUG("TelSDK code: %d, TelAF code: %d",
             static_cast<uint32_t>(endReasonCode), static_cast<uint32_t>(result));
    return result;
}

const char *taf_DCSHelper::CallEndCallManagerReasonCodeToString
(
    taf_dcs_CallEndCallManagerReasonCode_t endCode
)
{
    switch (endCode)
    {
    case TAF_DCS_CE_CM_CDMA_LOCK:
        return "TAF_DCS_CE_CM_CDMA_LOCK";
    case TAF_DCS_CE_CM_INTERCEPT:
        return "TAF_DCS_CE_CM_INTERCEPT";
    case TAF_DCS_CE_CM_REORDER:
        return "TAF_DCS_CE_CM_REORDER";
    case TAF_DCS_CE_CM_REL_SO_REJ:
        return "TAF_DCS_CE_CM_REL_SO_REJ";
    case TAF_DCS_CE_CM_INCOM_CALL:
        return "TAF_DCS_CE_CM_INCOM_CALL";
    case TAF_DCS_CE_CM_ALERT_STOP:
        return "TAF_DCS_CE_CM_ALERT_STOP";
    case TAF_DCS_CE_CM_ACTIVATION:
        return "TAF_DCS_CE_CM_ACTIVATION";
    case TAF_DCS_CE_CM_MAX_ACCESS_PROBE:
        return "TAF_DCS_CE_CM_MAX_ACCESS_PROBE";
    case TAF_DCS_CE_CM_CCS_NOT_SUPPORTED_BY_BS:
        return "TAF_DCS_CE_CM_CCS_NOT_SUPPORTED_BY_BS";
    case TAF_DCS_CE_CM_NO_RESPONSE_FROM_BS:
        return "TAF_DCS_CE_CM_NO_RESPONSE_FROM_BS";
    case TAF_DCS_CE_CM_REJECTED_BY_BS:
        return "TAF_DCS_CE_CM_REJECTED_BY_BS";
    case TAF_DCS_CE_CM_INCOMPATIBLE:
        return "TAF_DCS_CE_CM_INCOMPATIBLE";
    case TAF_DCS_CE_CM_ALREADY_IN_TC:
        return "TAF_DCS_CE_CM_ALREADY_IN_TC";
    case TAF_DCS_CE_CM_USER_CALL_ORIG_DURING_GPS:
        return "TAF_DCS_CE_CM_USER_CALL_ORIG_DURING_GPS";
    case TAF_DCS_CE_CM_USER_CALL_ORIG_DURING_SMS:
        return "TAF_DCS_CE_CM_USER_CALL_ORIG_DURING_SMS";
    case TAF_DCS_CE_CM_NO_CDMA_SRV:
        return "TAF_DCS_CE_CM_NO_CDMA_SRV";
    case TAF_DCS_CE_CM_MC_ABORT:
        return "TAF_DCS_CE_CM_MC_ABORT";
    case TAF_DCS_CE_CM_PSIST_NG:
        return "TAF_DCS_CE_CM_PSIST_NG";
    case TAF_DCS_CE_CM_UIM_NOT_PRESENT:
        return "TAF_DCS_CE_CM_UIM_NOT_PRESENT";
    case TAF_DCS_CE_CM_RETRY_ORDER:
        return "TAF_DCS_CE_CM_RETRY_ORDER";
    case TAF_DCS_CE_CM_ACCESS_BLOCK:
        return "TAF_DCS_CE_CM_ACCESS_BLOCK";
    case TAF_DCS_CE_CM_ACCESS_BLOCK_ALL:
        return "TAF_DCS_CE_CM_ACCESS_BLOCK_ALL";
    case TAF_DCS_CE_CM_IS707B_MAX_ACC:
        return "TAF_DCS_CE_CM_IS707B_MAX_ACC";
    case TAF_DCS_CE_CM_THERMAL_EMERGENCY:
        return "TAF_DCS_CE_CM_THERMAL_EMERGENCY";
    case TAF_DCS_CE_CM_CALL_ORIG_THROTTLED:
        return "TAF_DCS_CE_CM_CALL_ORIG_THROTTLED";
    case TAF_DCS_CE_CM_USER_CALL_ORIG_DURING_VOICE_CALL:
        return "TAF_DCS_CE_CM_USER_CALL_ORIG_DURING_VOICE_CALL";
    case TAF_DCS_CE_CM_CONF_FAILED:
        return "TAF_DCS_CE_CM_CONF_FAILED";
    case TAF_DCS_CE_CM_INCOM_REJ:
        return "TAF_DCS_CE_CM_INCOM_REJ";
    case TAF_DCS_CE_CM_NEW_NO_GW_SRV:
        return "TAF_DCS_CE_CM_NEW_NO_GW_SRV";
    case TAF_DCS_CE_CM_NEW_NO_GPRS_CONTEXT:
        return "TAF_DCS_CE_CM_NEW_NO_GPRS_CONTEXT";
    case TAF_DCS_CE_CM_NEW_ILLEGAL_MS:
        return "TAF_DCS_CE_CM_NEW_ILLEGAL_MS";
    case TAF_DCS_CE_CM_NEW_ILLEGAL_ME:
        return "TAF_DCS_CE_CM_NEW_ILLEGAL_ME";
    case TAF_DCS_CE_CM_NEW_GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED:
        return "TAF_DCS_CE_CM_NEW_GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED";
    case TAF_DCS_CE_CM_NEW_GPRS_SERVICES_NOT_ALLOWED:
        return "TAF_DCS_CE_CM_NEW_GPRS_SERVICES_NOT_ALLOWED";
    case TAF_DCS_CE_CM_NEW_MS_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK:
        return "TAF_DCS_CE_CM_NEW_MS_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK";
    case TAF_DCS_CE_CM_NEW_IMPLICITLY_DETACHED:
        return "TAF_DCS_CE_CM_NEW_IMPLICITLY_DETACHED";
    case TAF_DCS_CE_CM_NEW_PLMN_NOT_ALLOWED:
        return "TAF_DCS_CE_CM_NEW_PLMN_NOT_ALLOWED";
    case TAF_DCS_CE_CM_NEW_LA_NOT_ALLOWED:
        return "TAF_DCS_CE_CM_NEW_LA_NOT_ALLOWED";
    case TAF_DCS_CE_CM_NEW_GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN:
        return "TAF_DCS_CE_CM_NEW_GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN";
    case TAF_DCS_CE_CM_NEW_PDP_DUPLICATE:
        return "TAF_DCS_CE_CM_NEW_PDP_DUPLICATE";
    case TAF_DCS_CE_CM_NEW_UE_RAT_CHANGE:
        return "TAF_DCS_CE_CM_NEW_UE_RAT_CHANGE";
    case TAF_DCS_CE_CM_NEW_CONGESTION:
        return "TAF_DCS_CE_CM_NEW_CONGESTION";
    case TAF_DCS_CE_CM_NEW_NO_PDP_CONTEXT_ACTIVATED:
        return "TAF_DCS_CE_CM_NEW_NO_PDP_CONTEXT_ACTIVATED";
    case TAF_DCS_CE_CM_NEW_ACCESS_CLASS_DSAC_REJECTION:
        return "TAF_DCS_CE_CM_NEW_ACCESS_CLASS_DSAC_REJECTION";
    case TAF_DCS_CE_CM_PDP_ACTIVATE_MAX_RETRY_FAILED:
        return "TAF_DCS_CE_CM_PDP_ACTIVATE_MAX_RETRY_FAILED";
    case TAF_DCS_CE_CM_RAB_FAILURE:
        return "TAF_DCS_CE_CM_RAB_FAILURE";
    case TAF_DCS_CE_CM_ESM_UNKNOWN_EPS_BEARER_CONTEXT:
        return "TAF_DCS_CE_CM_ESM_UNKNOWN_EPS_BEARER_CONTEXT";
    case TAF_DCS_CE_CM_DRB_RELEASED_AT_RRC:
        return "TAF_DCS_CE_CM_DRB_RELEASED_AT_RRC";
    case TAF_DCS_CE_CM_NAS_SIG_CONN_RELEASED:
        return "TAF_DCS_CE_CM_NAS_SIG_CONN_RELEASED";
    case TAF_DCS_CE_CM_REASON_EMM_DETACHED:
        return "TAF_DCS_CE_CM_REASON_EMM_DETACHED";
    case TAF_DCS_CE_CM_EMM_ATTACH_FAILED:
        return "TAF_DCS_CE_CM_EMM_ATTACH_FAILED";
    case TAF_DCS_CE_CM_EMM_ATTACH_STARTED:
        return "TAF_DCS_CE_CM_EMM_ATTACH_STARTED";
    case TAF_DCS_CE_CM_LTE_NAS_SERVICE_REQ_FAILED:
        return "TAF_DCS_CE_CM_LTE_NAS_SERVICE_REQ_FAILED";
    case TAF_DCS_CE_CM_ESM_ACTIVE_DEDICATED_BEARER_REACTIVATED_BY_NW:
        return "TAF_DCS_CE_CM_ESM_ACTIVE_DEDICATED_BEARER_REACTIVATED_BY_NW";
    case TAF_DCS_CE_CM_ESM_LOWER_LAYER_FAILURE:
        return "TAF_DCS_CE_CM_ESM_LOWER_LAYER_FAILURE";
    case TAF_DCS_CE_CM_ESM_SYNC_UP_WITH_NW:
        return "TAF_DCS_CE_CM_ESM_SYNC_UP_WITH_NW";
    case TAF_DCS_CE_CM_ESM_NW_ACTIVATED_DED_BEARER_WITH_ID_OF_DEF_BEARER:
        return "TAF_DCS_CE_CM_ESM_NW_ACTIVATED_DED_BEARER_WITH_ID_OF_DEF_BEARER";
    case TAF_DCS_CE_CM_ESM_BAD_OTA_MESSAGE:
        return "TAF_DCS_CE_CM_ESM_BAD_OTA_MESSAGE";
    case TAF_DCS_CE_CM_ESM_DS_REJECTED_THE_CALL:
        return "TAF_DCS_CE_CM_ESM_DS_REJECTED_THE_CALL";
    case TAF_DCS_CE_CM_ESM_CONTEXT_TRANSFERED_DUE_TO_IRAT:
        return "TAF_DCS_CE_CM_ESM_CONTEXT_TRANSFERED_DUE_TO_IRAT";
    case TAF_DCS_CE_CM_DS_EXPLICIT_DEACT:
        return "TAF_DCS_CE_CM_DS_EXPLICIT_DEACT";
    case TAF_DCS_CE_CM_ESM_LOCAL_CAUSE_NONE:
        return "TAF_DCS_CE_CM_ESM_LOCAL_CAUSE_NONE";
    case TAF_DCS_CE_CM_LTE_NAS_SERVICE_REQ_FAILED_NO_THROTTLE:
        return "TAF_DCS_CE_CM_LTE_NAS_SERVICE_REQ_FAILED_NO_THROTTLE";
    case TAF_DCS_CE_CM_ACL_FAILURE:
        return "TAF_DCS_CE_CM_ACL_FAILURE";
    case TAF_DCS_CE_CM_LTE_NAS_SERVICE_REQ_FAILED_DS_DISALLOW:
        return "TAF_DCS_CE_CM_LTE_NAS_SERVICE_REQ_FAILED_DS_DISALLOW";
    case TAF_DCS_CE_CM_EMM_T3417_EXPIRED:
        return "TAF_DCS_CE_CM_EMM_T3417_EXPIRED";
    case TAF_DCS_CE_CM_EMM_T3417_EXT_EXPIRED:
        return "TAF_DCS_CE_CM_EMM_T3417_EXT_EXPIRED";
    case TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_TXN:
        return "TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_TXN";
    case TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_HO:
        return "TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_HO";
    case TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_CONN_REL:
        return "TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_CONN_REL";
    case TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_RLF:
        return "TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_RLF";
    case TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_CTRL_NOT_CONN:
        return "TAF_DCS_CE_CM_LRRC_UL_DATA_CNF_FAILURE_CTRL_NOT_CONN";
    case TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE:
        return "TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE";
    case TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_ABORTED:
        return "TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_ABORTED";
    case TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_ACCESS_BARRED:
        return "TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_ACCESS_BARRED";
    case TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_CELL_RESEL:
        return "TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_CELL_RESEL";
    case TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_CONFIG_FAILURE:
        return "TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_CONFIG_FAILURE";
    case TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_TIMER_EXPIRED:
        return "TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_TIMER_EXPIRED";
    case TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_LINK_FAILURE:
        return "TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_LINK_FAILURE";
    case TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_NOT_CAMPED:
        return "TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_NOT_CAMPED";
    case TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_SI_FAILURE:
        return "TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_SI_FAILURE";
    case TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_CONN_REJECT:
        return "TAF_DCS_CE_CM_LRRC_CONN_EST_FAILURE_CONN_REJECT";
    case TAF_DCS_CE_CM_LRRC_CONN_REL_NORMAL:
        return "TAF_DCS_CE_CM_LRRC_CONN_REL_NORMAL";
    case TAF_DCS_CE_CM_LRRC_CONN_REL_RLF:
        return "TAF_DCS_CE_CM_LRRC_CONN_REL_RLF";
    case TAF_DCS_CE_CM_LRRC_CONN_REL_CRE_FAILURE:
        return "TAF_DCS_CE_CM_LRRC_CONN_REL_CRE_FAILURE";
    case TAF_DCS_CE_CM_LRRC_CONN_REL_OOS_DURING_CRE:
        return "TAF_DCS_CE_CM_LRRC_CONN_REL_OOS_DURING_CRE";
    case TAF_DCS_CE_CM_LRRC_CONN_REL_ABORTED:
        return "TAF_DCS_CE_CM_LRRC_CONN_REL_ABORTED";
    case TAF_DCS_CE_CM_LRRC_CONN_REL_SIB_READ_ERROR:
        return "TAF_DCS_CE_CM_LRRC_CONN_REL_SIB_READ_ERROR";
    case TAF_DCS_CE_CM_DETACH_WITH_REATTACH_LTE_NW_DETACH:
        return "TAF_DCS_CE_CM_DETACH_WITH_REATTACH_LTE_NW_DETACH";
    case TAF_DCS_CE_CM_DETACH_WITH_OUT_REATTACH_LTE_NW_DETACH:
        return "TAF_DCS_CE_CM_DETACH_WITH_OUT_REATTACH_LTE_NW_DETACH";
    case TAF_DCS_CE_CM_ESM_PROC_TIME_OUT:
        return "TAF_DCS_CE_CM_ESM_PROC_TIME_OUT";
    case TAF_DCS_CE_CM_INVALID_CONNECTION_ID:
        return "TAF_DCS_CE_CM_INVALID_CONNECTION_ID";
    case TAF_DCS_CE_CM_INVALID_NSAPI:
        return "TAF_DCS_CE_CM_INVALID_NSAPI";
    case TAF_DCS_CE_CM_INVALID_PRI_NSAPI:
        return "TAF_DCS_CE_CM_INVALID_PRI_NSAPI";
    case TAF_DCS_CE_CM_INVALID_FIELD:
        return "TAF_DCS_CE_CM_INVALID_FIELD";
    case TAF_DCS_CE_CM_RAB_SETUP_FAILURE:
        return "TAF_DCS_CE_CM_RAB_SETUP_FAILURE";
    case TAF_DCS_CE_CM_PDP_ESTABLISH_MAX_TIMEOUT:
        return "TAF_DCS_CE_CM_PDP_ESTABLISH_MAX_TIMEOUT";
    case TAF_DCS_CE_CM_PDP_MODIFY_MAX_TIMEOUT:
        return "TAF_DCS_CE_CM_PDP_MODIFY_MAX_TIMEOUT";
    case TAF_DCS_CE_CM_PDP_INACTIVE_MAX_TIMEOUT:
        return "TAF_DCS_CE_CM_PDP_INACTIVE_MAX_TIMEOUT";
    case TAF_DCS_CE_CM_PDP_LOWERLAYER_ERROR:
        return "TAF_DCS_CE_CM_PDP_LOWERLAYER_ERROR";
    case TAF_DCS_CE_CM_PPD_UNKNOWN_REASON:
        return "TAF_DCS_CE_CM_PPD_UNKNOWN_REASON";
    case TAF_DCS_CE_CM_PDP_MODIFY_COLLISION:
        return "TAF_DCS_CE_CM_PDP_MODIFY_COLLISION";
    case TAF_DCS_CE_CM_PDP_MBMS_REQUEST_COLLISION:
        return "TAF_DCS_CE_CM_PDP_MBMS_REQUEST_COLLISION";
    case TAF_DCS_CE_CM_MBMS_DUPLICATE:
        return "TAF_DCS_CE_CM_MBMS_DUPLICATE";
    case TAF_DCS_CE_CM_SM_PS_DETACHED:
        return "TAF_DCS_CE_CM_SM_PS_DETACHED";
    case TAF_DCS_CE_CM_SM_NO_RADIO_AVAILABLE:
        return "TAF_DCS_CE_CM_SM_NO_RADIO_AVAILABLE";
    case TAF_DCS_CE_CM_SM_ABORT_SERVICE_NOT_AVAILABLE:
        return "TAF_DCS_CE_CM_SM_ABORT_SERVICE_NOT_AVAILABLE";
    case TAF_DCS_CE_CM_MESSAGE_EXCEED_MAX_L2_LIMIT:
        return "TAF_DCS_CE_CM_MESSAGE_EXCEED_MAX_L2_LIMIT";
    case TAF_DCS_CE_CM_SM_NAS_SRV_REQ_FAILURE:
        return "TAF_DCS_CE_CM_SM_NAS_SRV_REQ_FAILURE";
    case TAF_DCS_CE_CM_RRC_CONN_EST_FAILURE_REQ_ERROR:
        return "TAF_DCS_CE_CM_RRC_CONN_EST_FAILURE_REQ_ERROR";
    case TAF_DCS_CE_CM_RRC_CONN_EST_FAILURE_TAI_CHANGE:
        return "TAF_DCS_CE_CM_RRC_CONN_EST_FAILURE_TAI_CHANGE";
    case TAF_DCS_CE_CM_RRC_CONN_EST_FAILURE_RF_UNAVAILABLE:
        return "TAF_DCS_CE_CM_RRC_CONN_EST_FAILURE_RF_UNAVAILABLE";
    case TAF_DCS_CE_CM_RRC_CONN_REL_ABORTED_IRAT_SUCCESS:
        return "TAF_DCS_CE_CM_RRC_CONN_REL_ABORTED_IRAT_SUCCESS";
    case TAF_DCS_CE_CM_RRC_CONN_REL_RLF_SEC_NOT_ACTIVE:
        return "TAF_DCS_CE_CM_RRC_CONN_REL_RLF_SEC_NOT_ACTIVE";
    case TAF_DCS_CE_CM_RRC_CONN_REL_IRAT_TO_LTE_ABORTED:
        return "TAF_DCS_CE_CM_RRC_CONN_REL_IRAT_TO_LTE_ABORTED";
    case TAF_DCS_CE_CM_RRC_CONN_REL_IRAT_FROM_LTE_TO_G_CCO_SUCCESS:
        return "TAF_DCS_CE_CM_RRC_CONN_REL_IRAT_FROM_LTE_TO_G_CCO_SUCCESS";
    case TAF_DCS_CE_CM_RRC_CONN_REL_IRAT_FROM_LTE_TO_G_CCO_ABORTED:
        return "TAF_DCS_CE_CM_RRC_CONN_REL_IRAT_FROM_LTE_TO_G_CCO_ABORTED";
    case TAF_DCS_CE_CM_IMSI_UNKNOWN_IN_HSS:
        return "TAF_DCS_CE_CM_IMSI_UNKNOWN_IN_HSS";
    case TAF_DCS_CE_CM_IMEI_NOT_ACCEPTED:
        return "TAF_DCS_CE_CM_IMEI_NOT_ACCEPTED";
    case TAF_DCS_CE_CM_EPS_SERVICES_AND_NON_EPS_SERVICES_NOT_ALLOWED:
        return "TAF_DCS_CE_CM_EPS_SERVICES_AND_NON_EPS_SERVICES_NOT_ALLOWED";
    case TAF_DCS_CE_CM_EPS_SERVICES_NOT_ALLOWED_IN_PLMN:
        return "TAF_DCS_CE_CM_EPS_SERVICES_NOT_ALLOWED_IN_PLMN";
    case TAF_DCS_CE_CM_MSC_TEMPORARILY_NOT_REACHABLE:
        return "TAF_DCS_CE_CM_MSC_TEMPORARILY_NOT_REACHABLE";
    case TAF_DCS_CE_CM_CS_DOMAIN_NOT_AVAILABLE:
        return "TAF_DCS_CE_CM_CS_DOMAIN_NOT_AVAILABLE";
    case TAF_DCS_CE_CM_ESM_FAILURE:
        return "TAF_DCS_CE_CM_ESM_FAILURE";
    case TAF_DCS_CE_CM_MAC_FAILURE:
        return "TAF_DCS_CE_CM_MAC_FAILURE";
    case TAF_DCS_CE_CM_SYNCH_FAILURE:
        return "TAF_DCS_CE_CM_SYNCH_FAILURE";
    case TAF_DCS_CE_CM_UE_SECURITY_CAPABILITIES_MISMATCH:
        return "TAF_DCS_CE_CM_UE_SECURITY_CAPABILITIES_MISMATCH";
    case TAF_DCS_CE_CM_SECURITY_MODE_REJ_UNSPECIFIED:
        return "TAF_DCS_CE_CM_SECURITY_MODE_REJ_UNSPECIFIED";
    case TAF_DCS_CE_CM_NON_EPS_AUTH_UNACCEPTABLE:
        return "TAF_DCS_CE_CM_NON_EPS_AUTH_UNACCEPTABLE";
    case TAF_DCS_CE_CM_CS_FALLBACK_CALL_EST_NOT_ALLOWED:
        return "TAF_DCS_CE_CM_CS_FALLBACK_CALL_EST_NOT_ALLOWED";
    case TAF_DCS_CE_CM_NO_EPS_BEARER_CONTEXT_ACTIVATED:
        return "TAF_DCS_CE_CM_NO_EPS_BEARER_CONTEXT_ACTIVATED";
    case TAF_DCS_CE_CM_EMM_INVALID_STATE:
        return "TAF_DCS_CE_CM_EMM_INVALID_STATE";
    case TAF_DCS_CE_CM_NAS_LAYER_FAILURE:
        return "TAF_DCS_CE_CM_NAS_LAYER_FAILURE";
    case TAF_DCS_CE_CM_MULTI_PDN_NOT_ALLOWED:
        return "TAF_DCS_CE_CM_MULTI_PDN_NOT_ALLOWED";
    case TAF_DCS_CE_CM_EMBMS_NOT_ENABLED:
        return "TAF_DCS_CE_CM_EMBMS_NOT_ENABLED";
    case TAF_DCS_CE_CM_PENDING_REDIAL_CALL_CLEANUP:
        return "TAF_DCS_CE_CM_PENDING_REDIAL_CALL_CLEANUP";
    case TAF_DCS_CE_CM_EMBMS_REGULAR_DEACTIVATION:
        return "TAF_DCS_CE_CM_EMBMS_REGULAR_DEACTIVATION";
    case TAF_DCS_CE_CM_TLB_REGULAR_DEACTIVATION:
        return "TAF_DCS_CE_CM_TLB_REGULAR_DEACTIVATION";
    case TAF_DCS_CE_CM_LOWER_LAYER_REGISTRATION_FAILURE:
        return "TAF_DCS_CE_CM_LOWER_LAYER_REGISTRATION_FAILURE";
    case TAF_DCS_CE_CM_DETACH_EPS_SERVICES_NOT_ALLOWED:
        return "TAF_DCS_CE_CM_DETACH_EPS_SERVICES_NOT_ALLOWED";
    case TAF_DCS_CE_CM_SM_INTERNAL_PDP_DEACTIVATION:
        return "TAF_DCS_CE_CM_SM_INTERNAL_PDP_DEACTIVATION";
    case TAF_DCS_CE_CM_UNSUPPORTED_1X_PREV:
        return "TAF_DCS_CE_CM_UNSUPPORTED_1X_PREV";
    case TAF_DCS_CE_CM_CD_GEN_OR_BUSY:
        return "TAF_DCS_CE_CM_CD_GEN_OR_BUSY";
    case TAF_DCS_CE_CM_CD_BILL_OR_AUTH:
        return "TAF_DCS_CE_CM_CD_BILL_OR_AUTH";
    case TAF_DCS_CE_CM_CHG_HDR:
        return "TAF_DCS_CE_CM_CHG_HDR";
    case TAF_DCS_CE_CM_EXIT_HDR:
        return "TAF_DCS_CE_CM_EXIT_HDR";
    case TAF_DCS_CE_CM_HDR_NO_SESSION:
        return "TAF_DCS_CE_CM_HDR_NO_SESSION";
    case TAF_DCS_CE_CM_HDR_ORIG_DURING_GPS_FIX:
        return "TAF_DCS_CE_CM_HDR_ORIG_DURING_GPS_FIX";
    case TAF_DCS_CE_CM_HDR_CS_TIMEOUT:
        return "TAF_DCS_CE_CM_HDR_CS_TIMEOUT";
    case TAF_DCS_CE_CM_HDR_RELEASED_BY_CM:
        return "TAF_DCS_CE_CM_HDR_RELEASED_BY_CM";
    case TAF_DCS_CE_CM_COLLOC_ACQ_FAIL:
        return "TAF_DCS_CE_CM_COLLOC_ACQ_FAIL";
    case TAF_DCS_CE_CM_OTASP_COMMIT_IN_PROG:
        return "TAF_DCS_CE_CM_OTASP_COMMIT_IN_PROG";
    case TAF_DCS_CE_CM_NO_HYBR_HDR_SRV:
        return "TAF_DCS_CE_CM_NO_HYBR_HDR_SRV";
    case TAF_DCS_CE_CM_HDR_NO_LOCK_GRANTED:
        return "TAF_DCS_CE_CM_HDR_NO_LOCK_GRANTED";
    case TAF_DCS_CE_CM_HOLD_OTHER_IN_PROG:
        return "TAF_DCS_CE_CM_HOLD_OTHER_IN_PROG";
    case TAF_DCS_CE_CM_HDR_FADE:
        return "TAF_DCS_CE_CM_HDR_FADE";
    case TAF_DCS_CE_CM_HDR_ACC_FAIL:
        return "TAF_DCS_CE_CM_HDR_ACC_FAIL";
    case TAF_DCS_CE_CM_CLIENT_END:
        return "TAF_DCS_CE_CM_CLIENT_END";
    case TAF_DCS_CE_CM_NO_SRV:
        return "TAF_DCS_CE_CM_NO_SRV";
    case TAF_DCS_CE_CM_FADE:
        return "TAF_DCS_CE_CM_FADE";
    case TAF_DCS_CE_CM_REL_NORMAL:
        return "TAF_DCS_CE_CM_REL_NORMAL";
    case TAF_DCS_CE_CM_ACC_IN_PROG:
        return "TAF_DCS_CE_CM_ACC_IN_PROG";
    case TAF_DCS_CE_CM_ACC_FAIL:
        return "TAF_DCS_CE_CM_ACC_FAIL";
    case TAF_DCS_CE_CM_REDIR_OR_HANDOFF:
        return "TAF_DCS_CE_CM_REDIR_OR_HANDOFF";
    case TAF_DCS_CE_CM_CM_UNKNOWN_ERROR:
        return "TAF_DCS_CE_CM_CM_UNKNOWN_ERROR";
    case TAF_DCS_CE_CM_OFFLINE:
        return "TAF_DCS_CE_CM_OFFLINE";
    case TAF_DCS_CE_CM_EMERGENCY_MODE:
        return "TAF_DCS_CE_CM_EMERGENCY_MODE";
    case TAF_DCS_CE_CM_PHONE_IN_USE:
        return "TAF_DCS_CE_CM_PHONE_IN_USE";
    case TAF_DCS_CE_CM_INVALID_MODE:
        return "TAF_DCS_CE_CM_INVALID_MODE";
    case TAF_DCS_CE_CM_INVALID_SIM_STATE:
        return "TAF_DCS_CE_CM_INVALID_SIM_STATE";
    case TAF_DCS_CE_CM_NO_COLLOC_HDR:
        return "TAF_DCS_CE_CM_NO_COLLOC_HDR";
    case TAF_DCS_CE_CM_CALL_CONTROL_REJECTED:
        return "TAF_DCS_CE_CM_CALL_CONTROL_REJECTED";
    case TAF_DCS_CE_CM_UNKNOWN:
        return "TAF_DCS_CE_CM_UNKNOWN";
    default:
        LE_WARN("Invalid call manager reason code: %d", static_cast<int>(endCode));
        return "Invalid call manager reason code";
    }
}

taf_dcs_CallEnd3GPPSpecReasonCode_t taf_DCSHelper::ConvertCallEnd3GPPSpecReasonCode
(
    telux::common::SpecReasonCode endReasonCode
)
{
    taf_dcs_CallEnd3GPPSpecReasonCode_t result;
    using telux::common::SpecReasonCode;

    switch (endReasonCode)
    {
    case SpecReasonCode::CE_OPERATOR_DETERMINED_BARRING:
        result = TAF_DCS_CE_3GPPSPEC_OPERATOR_DETERMINED_BARRING;
        break;
    case SpecReasonCode::CE_NAS_SIGNALLING_ERROR:
        result = TAF_DCS_CE_3GPPSPEC_NAS_SIGNALLING_ERROR;
        break;
    case SpecReasonCode::CE_LLC_SNDCP_FAILURE:
        result = TAF_DCS_CE_3GPPSPEC_LLC_SNDCP_FAILURE;
        break;
    case SpecReasonCode::CE_INSUFFICIENT_RESOURCES:
        result = TAF_DCS_CE_3GPPSPEC_INSUFFICIENT_RESOURCES;
        break;
    case SpecReasonCode::CE_UNKNOWN_APN:
        result = TAF_DCS_CE_3GPPSPEC_UNKNOWN_APN;
        break;
    case SpecReasonCode::CE_UNKNOWN_PDP:
        result = TAF_DCS_CE_3GPPSPEC_UNKNOWN_PDP;
        break;
    case SpecReasonCode::CE_AUTH_FAILED:
        result = TAF_DCS_CE_3GPPSPEC_AUTH_FAILED;
        break;
    case SpecReasonCode::CE_GGSN_REJECT:
        result = TAF_DCS_CE_3GPPSPEC_GGSN_REJECT;
        break;
    case SpecReasonCode::CE_ACTIVATION_REJECT:
        result = TAF_DCS_CE_3GPPSPEC_ACTIVATION_REJECT;
        break;
    case SpecReasonCode::CE_OPTION_NOT_SUPPORTED:
        result = TAF_DCS_CE_3GPPSPEC_OPTION_NOT_SUPPORTED;
        break;
    case SpecReasonCode::CE_OPTION_UNSUBSCRIBED:
        result = TAF_DCS_CE_3GPPSPEC_OPTION_UNSUBSCRIBED;
        break;
    case SpecReasonCode::CE_OPTION_TEMP_OOO:
        result = TAF_DCS_CE_3GPPSPEC_OPTION_TEMP_OOO;
        break;
    case SpecReasonCode::CE_NSAPI_ALREADY_USED:
        result = TAF_DCS_CE_3GPPSPEC_NSAPI_ALREADY_USED;
        break;
    case SpecReasonCode::CE_REGULAR_DEACTIVATION:
        result = TAF_DCS_CE_3GPPSPEC_REGULAR_DEACTIVATION;
        break;
    case SpecReasonCode::CE_QOS_NOT_ACCEPTED:
        result = TAF_DCS_CE_3GPPSPEC_QOS_NOT_ACCEPTED;
        break;
    case SpecReasonCode::CE_NETWORK_FAILURE:
        result = TAF_DCS_CE_3GPPSPEC_NETWORK_FAILURE;
        break;
    case SpecReasonCode::CE_UMTS_REACTIVATION_REQ:
        result = TAF_DCS_CE_3GPPSPEC_UMTS_REACTIVATION_REQ;
        break;
    case SpecReasonCode::CE_FEATURE_NOT_SUPPORTED:
        result = TAF_DCS_CE_3GPPSPEC_FEATURE_NOT_SUPPORTED;
        break;
    case SpecReasonCode::CE_TFT_SEMANTIC_ERROR:
        result = TAF_DCS_CE_3GPPSPEC_TFT_SEMANTIC_ERROR;
        break;
    case SpecReasonCode::CE_TFT_SYNTAX_ERROR:
        result = TAF_DCS_CE_3GPPSPEC_TFT_SYNTAX_ERROR;
        break;
    case SpecReasonCode::CE_UNKNOWN_PDP_CONTEXT:
        result = TAF_DCS_CE_3GPPSPEC_UNKNOWN_PDP_CONTEXT;
        break;
    case SpecReasonCode::CE_FILTER_SEMANTIC_ERROR:
        result = TAF_DCS_CE_3GPPSPEC_FILTER_SEMANTIC_ERROR;
        break;
    case SpecReasonCode::CE_FILTER_SYNTAX_ERROR:
        result = TAF_DCS_CE_3GPPSPEC_FILTER_SYNTAX_ERROR;
        break;
    case SpecReasonCode::CE_PDP_WITHOUT_ACTIVE_TFT:
        result = TAF_DCS_CE_3GPPSPEC_PDP_WITHOUT_ACTIVE_TFT;
        break;
    case SpecReasonCode::CE_IP_V4_ONLY_ALLOWED:
        result = TAF_DCS_CE_3GPPSPEC_IP_V4_ONLY_ALLOWED;
        break;
    case SpecReasonCode::CE_IP_V6_ONLY_ALLOWED:
        result = TAF_DCS_CE_3GPPSPEC_IP_V6_ONLY_ALLOWED;
        break;
    case SpecReasonCode::CE_SINGLE_ADDR_BEARER_ONLY:
        result = TAF_DCS_CE_3GPPSPEC_SINGLE_ADDR_BEARER_ONLY;
        break;
    case SpecReasonCode::CE_ESM_INFO_NOT_RECEIVED:
        result = TAF_DCS_CE_3GPPSPEC_ESM_INFO_NOT_RECEIVED;
        break;
    case SpecReasonCode::CE_PDN_CONN_DOES_NOT_EXIST:
        result = TAF_DCS_CE_3GPPSPEC_PDN_CONN_DOES_NOT_EXIST;
        break;
    case SpecReasonCode::CE_MULTI_CONN_TO_SAME_PDN_NOT_ALLOWED:
        result = TAF_DCS_CE_3GPPSPEC_MULTI_CONN_TO_SAME_PDN_NOT_ALLOWED;
        break;
    case SpecReasonCode::CE_MAX_ACTIVE_PDP_CONTEXT_REACHED:
        result = TAF_DCS_CE_3GPPSPEC_MAX_ACTIVE_PDP_CONTEXT_REACHED;
        break;
    case SpecReasonCode::CE_UNSUPPORTED_APN_IN_CURRENT_PLMN:
        result = TAF_DCS_CE_3GPPSPEC_UNSUPPORTED_APN_IN_CURRENT_PLMN;
        break;
    case SpecReasonCode::CE_INVALID_TRANSACTION_ID:
        result = TAF_DCS_CE_3GPPSPEC_INVALID_TRANSACTION_ID;
        break;
    case SpecReasonCode::CE_MESSAGE_INCORRECT_SEMANTIC:
        result = TAF_DCS_CE_3GPPSPEC_MESSAGE_INCORRECT_SEMANTIC;
        break;
    case SpecReasonCode::CE_INVALID_MANDATORY_INFO:
        result = TAF_DCS_CE_3GPPSPEC_INVALID_MANDATORY_INFO;
        break;
    case SpecReasonCode::CE_MESSAGE_TYPE_UNSUPPORTED:
        result = TAF_DCS_CE_3GPPSPEC_MESSAGE_TYPE_UNSUPPORTED;
        break;
    case SpecReasonCode::CE_MSG_TYPE_NONCOMPATIBLE_STATE:
        result = TAF_DCS_CE_3GPPSPEC_MSG_TYPE_NONCOMPATIBLE_STATE;
        break;
    case SpecReasonCode::CE_UNKNOWN_INFO_ELEMENT:
        result = TAF_DCS_CE_3GPPSPEC_UNKNOWN_INFO_ELEMENT;
        break;
    case SpecReasonCode::CE_CONDITIONAL_IE_ERROR:
        result = TAF_DCS_CE_3GPPSPEC_CONDITIONAL_IE_ERROR;
        break;
    case SpecReasonCode::CE_MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE:
        result = TAF_DCS_CE_3GPPSPEC_MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE;
        break;
    case SpecReasonCode::CE_PROTOCOL_ERROR:
        result = TAF_DCS_CE_3GPPSPEC_PROTOCOL_ERROR;
        break;
    case SpecReasonCode::CE_APN_TYPE_CONFLICT:
        result = TAF_DCS_CE_3GPPSPEC_APN_TYPE_CONFLICT;
        break;
    case SpecReasonCode::CE_INVALID_PCSCF_ADDRESS:
        result = TAF_DCS_CE_3GPPSPEC_INVALID_PCSCF_ADDRESS;
        break;
    case SpecReasonCode::CE_INTERNAL_CALL_PREEMPT_BY_HIGH_PRIO_APN:
        result = TAF_DCS_CE_3GPPSPEC_INTERNAL_CALL_PREEMPT_BY_HIGH_PRIO_APN;
        break;
    case SpecReasonCode::CE_EMM_ACCESS_BARRED:
        result = TAF_DCS_CE_3GPPSPEC_EMM_ACCESS_BARRED;
        break;
    case SpecReasonCode::CE_EMERGENCY_IFACE_ONLY:
        result = TAF_DCS_CE_3GPPSPEC_EMERGENCY_IFACE_ONLY;
        break;
    case SpecReasonCode::CE_IFACE_MISMATCH:
        result = TAF_DCS_CE_3GPPSPEC_IFACE_MISMATCH;
        break;
    case SpecReasonCode::CE_COMPANION_IFACE_IN_USE:
        result = TAF_DCS_CE_3GPPSPEC_COMPANION_IFACE_IN_USE;
        break;
    case SpecReasonCode::CE_IP_ADDRESS_MISMATCH:
        result = TAF_DCS_CE_3GPPSPEC_IP_ADDRESS_MISMATCH;
        break;
    case SpecReasonCode::CE_IFACE_AND_POL_FAMILY_MISMATCH:
        result = TAF_DCS_CE_3GPPSPEC_IFACE_AND_POL_FAMILY_MISMATCH;
        break;
    case SpecReasonCode::CE_EMM_ACCESS_BARRED_INFINITE_RETRY:
        result = TAF_DCS_CE_3GPPSPEC_EMM_ACCESS_BARRED_INFINITE_RETRY;
        break;
    case SpecReasonCode::CE_AUTH_FAILURE_ON_EMERGENCY_CALL:
        result = TAF_DCS_CE_3GPPSPEC_AUTH_FAILURE_ON_EMERGENCY_CALL;
        break;
    case SpecReasonCode::CE_INVALID_DNS_ADDR:
        result = TAF_DCS_CE_3GPPSPEC_INVALID_DNS_ADDR;
        break;
    case SpecReasonCode::CE_INVALID_PCSCF_DNS_ADDR:
        result = TAF_DCS_CE_3GPPSPEC_INVALID_PCSCF_DNS_ADDR;
        break;
    case SpecReasonCode::CE_TEST_LOOPBACK_MODE_A_OR_B_ENABLED:
        result = TAF_DCS_CE_3GPPSPEC_TEST_LOOPBACK_MODE_A_OR_B_ENABLED;
        break;
    default:
        result = TAF_DCS_CE_3GPPSPEC_UNKNOWN;
        break;
    }
    LE_DEBUG("TelSDK code: %d, TelAF code: %d",
                static_cast<uint32_t>(endReasonCode), static_cast<uint32_t>(result));
    return result;
}

const char *taf_DCSHelper::CallEnd3GPPSpecReasonCodeToString
(
    taf_dcs_CallEnd3GPPSpecReasonCode_t endCode)
{
    switch (endCode)
    {
    case TAF_DCS_CE_3GPPSPEC_OPERATOR_DETERMINED_BARRING:
        return "TAF_DCS_CE_3GPPSPEC_OPERATOR_DETERMINED_BARRING";
    case TAF_DCS_CE_3GPPSPEC_NAS_SIGNALLING_ERROR:
        return "TAF_DCS_CE_3GPPSPEC_NAS_SIGNALLING_ERROR";
    case TAF_DCS_CE_3GPPSPEC_LLC_SNDCP_FAILURE:
        return "TAF_DCS_CE_3GPPSPEC_LLC_SNDCP_FAILURE";
    case TAF_DCS_CE_3GPPSPEC_INSUFFICIENT_RESOURCES:
        return "TAF_DCS_CE_3GPPSPEC_INSUFFICIENT_RESOURCES";
    case TAF_DCS_CE_3GPPSPEC_UNKNOWN_APN:
        return "TAF_DCS_CE_3GPPSPEC_UNKNOWN_APN";
    case TAF_DCS_CE_3GPPSPEC_UNKNOWN_PDP:
        return "TAF_DCS_CE_3GPPSPEC_UNKNOWN_PDP";
    case TAF_DCS_CE_3GPPSPEC_AUTH_FAILED:
        return "TAF_DCS_CE_3GPPSPEC_AUTH_FAILED";
    case TAF_DCS_CE_3GPPSPEC_GGSN_REJECT:
        return "TAF_DCS_CE_3GPPSPEC_GGSN_REJECT";
    case TAF_DCS_CE_3GPPSPEC_ACTIVATION_REJECT:
        return "TAF_DCS_CE_3GPPSPEC_ACTIVATION_REJECT";
    case TAF_DCS_CE_3GPPSPEC_OPTION_NOT_SUPPORTED:
        return "TAF_DCS_CE_3GPPSPEC_OPTION_NOT_SUPPORTED";
    case TAF_DCS_CE_3GPPSPEC_OPTION_UNSUBSCRIBED:
        return "TAF_DCS_CE_3GPPSPEC_OPTION_UNSUBSCRIBED";
    case TAF_DCS_CE_3GPPSPEC_OPTION_TEMP_OOO:
        return "TAF_DCS_CE_3GPPSPEC_OPTION_TEMP_OOO";
    case TAF_DCS_CE_3GPPSPEC_NSAPI_ALREADY_USED:
        return "TAF_DCS_CE_3GPPSPEC_NSAPI_ALREADY_USED";
    case TAF_DCS_CE_3GPPSPEC_REGULAR_DEACTIVATION:
        return "TAF_DCS_CE_3GPPSPEC_REGULAR_DEACTIVATION";
    case TAF_DCS_CE_3GPPSPEC_QOS_NOT_ACCEPTED:
        return "TAF_DCS_CE_3GPPSPEC_QOS_NOT_ACCEPTED";
    case TAF_DCS_CE_3GPPSPEC_NETWORK_FAILURE:
        return "TAF_DCS_CE_3GPPSPEC_NETWORK_FAILURE";
    case TAF_DCS_CE_3GPPSPEC_UMTS_REACTIVATION_REQ:
        return "TAF_DCS_CE_3GPPSPEC_UMTS_REACTIVATION_REQ";
    case TAF_DCS_CE_3GPPSPEC_FEATURE_NOT_SUPPORTED:
        return "TAF_DCS_CE_3GPPSPEC_FEATURE_NOT_SUPPORTED";
    case TAF_DCS_CE_3GPPSPEC_TFT_SEMANTIC_ERROR:
        return "TAF_DCS_CE_3GPPSPEC_TFT_SEMANTIC_ERROR";
    case TAF_DCS_CE_3GPPSPEC_TFT_SYNTAX_ERROR:
        return "TAF_DCS_CE_3GPPSPEC_TFT_SYNTAX_ERROR";
    case TAF_DCS_CE_3GPPSPEC_UNKNOWN_PDP_CONTEXT:
        return "TAF_DCS_CE_3GPPSPEC_UNKNOWN_PDP_CONTEXT";
    case TAF_DCS_CE_3GPPSPEC_FILTER_SEMANTIC_ERROR:
        return "TAF_DCS_CE_3GPPSPEC_FILTER_SEMANTIC_ERROR";
    case TAF_DCS_CE_3GPPSPEC_FILTER_SYNTAX_ERROR:
        return "TAF_DCS_CE_3GPPSPEC_FILTER_SYNTAX_ERROR";
    case TAF_DCS_CE_3GPPSPEC_PDP_WITHOUT_ACTIVE_TFT:
        return "TAF_DCS_CE_3GPPSPEC_PDP_WITHOUT_ACTIVE_TFT";
    case TAF_DCS_CE_3GPPSPEC_IP_V4_ONLY_ALLOWED:
        return "TAF_DCS_CE_3GPPSPEC_IP_V4_ONLY_ALLOWED";
    case TAF_DCS_CE_3GPPSPEC_IP_V6_ONLY_ALLOWED:
        return "TAF_DCS_CE_3GPPSPEC_IP_V6_ONLY_ALLOWED";
    case TAF_DCS_CE_3GPPSPEC_SINGLE_ADDR_BEARER_ONLY:
        return "TAF_DCS_CE_3GPPSPEC_SINGLE_ADDR_BEARER_ONLY";
    case TAF_DCS_CE_3GPPSPEC_ESM_INFO_NOT_RECEIVED:
        return "TAF_DCS_CE_3GPPSPEC_ESM_INFO_NOT_RECEIVED";
    case TAF_DCS_CE_3GPPSPEC_PDN_CONN_DOES_NOT_EXIST:
        return "TAF_DCS_CE_3GPPSPEC_PDN_CONN_DOES_NOT_EXIST";
    case TAF_DCS_CE_3GPPSPEC_MULTI_CONN_TO_SAME_PDN_NOT_ALLOWED:
        return "TAF_DCS_CE_3GPPSPEC_MULTI_CONN_TO_SAME_PDN_NOT_ALLOWED";
    case TAF_DCS_CE_3GPPSPEC_MAX_ACTIVE_PDP_CONTEXT_REACHED:
        return "TAF_DCS_CE_3GPPSPEC_MAX_ACTIVE_PDP_CONTEXT_REACHED";
    case TAF_DCS_CE_3GPPSPEC_UNSUPPORTED_APN_IN_CURRENT_PLMN:
        return "TAF_DCS_CE_3GPPSPEC_UNSUPPORTED_APN_IN_CURRENT_PLMN";
    case TAF_DCS_CE_3GPPSPEC_INVALID_TRANSACTION_ID:
        return "TAF_DCS_CE_3GPPSPEC_INVALID_TRANSACTION_ID";
    case TAF_DCS_CE_3GPPSPEC_MESSAGE_INCORRECT_SEMANTIC:
        return "TAF_DCS_CE_3GPPSPEC_MESSAGE_INCORRECT_SEMANTIC";
    case TAF_DCS_CE_3GPPSPEC_INVALID_MANDATORY_INFO:
        return "TAF_DCS_CE_3GPPSPEC_INVALID_MANDATORY_INFO";
    case TAF_DCS_CE_3GPPSPEC_MESSAGE_TYPE_UNSUPPORTED:
        return "TAF_DCS_CE_3GPPSPEC_MESSAGE_TYPE_UNSUPPORTED";
    case TAF_DCS_CE_3GPPSPEC_MSG_TYPE_NONCOMPATIBLE_STATE:
        return "TAF_DCS_CE_3GPPSPEC_MSG_TYPE_NONCOMPATIBLE_STATE";
    case TAF_DCS_CE_3GPPSPEC_UNKNOWN_INFO_ELEMENT:
        return "TAF_DCS_CE_3GPPSPEC_UNKNOWN_INFO_ELEMENT";
    case TAF_DCS_CE_3GPPSPEC_CONDITIONAL_IE_ERROR:
        return "TAF_DCS_CE_3GPPSPEC_CONDITIONAL_IE_ERROR";
    case TAF_DCS_CE_3GPPSPEC_MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE:
        return "TAF_DCS_CE_3GPPSPEC_MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE";
    case TAF_DCS_CE_3GPPSPEC_PROTOCOL_ERROR:
        return "TAF_DCS_CE_3GPPSPEC_PROTOCOL_ERROR";
    case TAF_DCS_CE_3GPPSPEC_APN_TYPE_CONFLICT:
        return "TAF_DCS_CE_3GPPSPEC_APN_TYPE_CONFLICT";
    case TAF_DCS_CE_3GPPSPEC_INVALID_PCSCF_ADDRESS:
        return "TAF_DCS_CE_3GPPSPEC_INVALID_PCSCF_ADDRESS";
    case TAF_DCS_CE_3GPPSPEC_INTERNAL_CALL_PREEMPT_BY_HIGH_PRIO_APN:
        return "TAF_DCS_CE_3GPPSPEC_INTERNAL_CALL_PREEMPT_BY_HIGH_PRIO_APN";
    case TAF_DCS_CE_3GPPSPEC_EMM_ACCESS_BARRED:
        return "TAF_DCS_CE_3GPPSPEC_EMM_ACCESS_BARRED";
    case TAF_DCS_CE_3GPPSPEC_EMERGENCY_IFACE_ONLY:
        return "TAF_DCS_CE_3GPPSPEC_EMERGENCY_IFACE_ONLY";
    case TAF_DCS_CE_3GPPSPEC_IFACE_MISMATCH:
        return "TAF_DCS_CE_3GPPSPEC_IFACE_MISMATCH";
    case TAF_DCS_CE_3GPPSPEC_COMPANION_IFACE_IN_USE:
        return "TAF_DCS_CE_3GPPSPEC_COMPANION_IFACE_IN_USE";
    case TAF_DCS_CE_3GPPSPEC_IP_ADDRESS_MISMATCH:
        return "TAF_DCS_CE_3GPPSPEC_IP_ADDRESS_MISMATCH";
    case TAF_DCS_CE_3GPPSPEC_IFACE_AND_POL_FAMILY_MISMATCH:
        return "TAF_DCS_CE_3GPPSPEC_IFACE_AND_POL_FAMILY_MISMATCH";
    case TAF_DCS_CE_3GPPSPEC_EMM_ACCESS_BARRED_INFINITE_RETRY:
        return "TAF_DCS_CE_3GPPSPEC_EMM_ACCESS_BARRED_INFINITE_RETRY";
    case TAF_DCS_CE_3GPPSPEC_AUTH_FAILURE_ON_EMERGENCY_CALL:
        return "TAF_DCS_CE_3GPPSPEC_AUTH_FAILURE_ON_EMERGENCY_CALL";
    case TAF_DCS_CE_3GPPSPEC_INVALID_DNS_ADDR:
        return "TAF_DCS_CE_3GPPSPEC_INVALID_DNS_ADDR";
    case TAF_DCS_CE_3GPPSPEC_INVALID_PCSCF_DNS_ADDR:
        return "TAF_DCS_CE_3GPPSPEC_INVALID_PCSCF_DNS_ADDR";
    case TAF_DCS_CE_3GPPSPEC_TEST_LOOPBACK_MODE_A_OR_B_ENABLED:
        return "TAF_DCS_CE_3GPPSPEC_TEST_LOOPBACK_MODE_A_OR_B_ENABLED";
    case TAF_DCS_CE_3GPPSPEC_UNKNOWN:
        return "TAF_DCS_CE_3GPPSPEC_UNKNOWN";
    default:
        LE_WARN("Invalid 3GPP spec reason code: %d", static_cast<int>(endCode));
        return "Invalid 3GPP spec reason code";
    }
}

taf_dcs_CallEndPPPReasonCode_t taf_DCSHelper::ConvertCallEndPPPReasonCode
(
    telux::common::PPPReasonCode endReasonCode
)
{
    using telux::common::PPPReasonCode;
    taf_dcs_CallEndPPPReasonCode_t result;
    switch (endReasonCode) {
        case PPPReasonCode::CE_PPP_TIMEOUT:
            result = TAF_DCS_CE_PPP_TIMEOUT;
            break;
        case PPPReasonCode::CE_PPP_AUTH_FAILURE:
            result = TAF_DCS_CE_PPP_AUTH_FAILURE;
            break;
        case PPPReasonCode::CE_PPP_OPTION_MISMATCH:
            result = TAF_DCS_CE_PPP_OPTION_MISMATCH;
            break;
        case PPPReasonCode::CE_PPP_PAP_FAILURE:
            result = TAF_DCS_CE_PPP_PAP_FAILURE;
            break;
        case PPPReasonCode::CE_PPP_CHAP_FAILURE:
            result = TAF_DCS_CE_PPP_CHAP_FAILURE;
            break;
        case PPPReasonCode::CE_PPP_CLOSE_IN_PROGRESS:
            result = TAF_DCS_CE_PPP_CLOSE_IN_PROGRESS;
            break;
        case PPPReasonCode::CE_PPP_NV_REFRESH_IN_PROGRESS:
            result = TAF_DCS_CE_PPP_NV_REFRESH_IN_PROGRESS;
            break;
        case PPPReasonCode::CE_PPP_UNKNOWN:
            result = TAF_DCS_CE_PPP_UNKNOWN;
            break;
        default:
            result = TAF_DCS_CE_PPP_UNKNOWN;
            break;
    }
    LE_DEBUG("TelSDK code: %d, TelAF code: %d",
             static_cast<uint32_t>(endReasonCode), static_cast<uint32_t>(result));
    return result;
}

const char *taf_DCSHelper::CallEndPPPReasonCodeToString(taf_dcs_CallEndPPPReasonCode_t endCode)
{
    switch (endCode)
    {
    case TAF_DCS_CE_PPP_TIMEOUT:
        return "TAF_DCS_CE_PPP_TIMEOUT";
    case TAF_DCS_CE_PPP_AUTH_FAILURE:
        return "TAF_DCS_CE_PPP_AUTH_FAILURE";
    case TAF_DCS_CE_PPP_OPTION_MISMATCH:
        return "TAF_DCS_CE_PPP_OPTION_MISMATCH";
    case TAF_DCS_CE_PPP_PAP_FAILURE:
        return "TAF_DCS_CE_PPP_PAP_FAILURE";
    case TAF_DCS_CE_PPP_CHAP_FAILURE:
        return "TAF_DCS_CE_PPP_CHAP_FAILURE";
    case TAF_DCS_CE_PPP_CLOSE_IN_PROGRESS:
        return "TAF_DCS_CE_PPP_CLOSE_IN_PROGRESS";
    case TAF_DCS_CE_PPP_NV_REFRESH_IN_PROGRESS:
        return "TAF_DCS_CE_PPP_NV_REFRESH_IN_PROGRESS";
    case TAF_DCS_CE_PPP_UNKNOWN:
        return "TAF_DCS_CE_PPP_UNKNOWN";
    default:
        LE_WARN("Invalid PPP reason code: %d", static_cast<int>(endCode));
        return "Invalid PPP reason code";
    }
}

taf_dcs_CallEndEHRPDReasonCode_t taf_DCSHelper::ConvertCallEndEHRPDReasonCode
(
    telux::common::EHRPDReasonCode endReasonCode
)
{
    using telux::common::EHRPDReasonCode;
    taf_dcs_CallEndEHRPDReasonCode_t result;
    switch (endReasonCode)
    {
    case EHRPDReasonCode::CE_EHRPD_SUBS_LIMITED_TO_V4:
        result = TAF_DCS_CE_EHRPD_SUBS_LIMITED_TO_V4;
        break;
    case EHRPDReasonCode::CE_EHRPD_SUBS_LIMITED_TO_V6:
        result = TAF_DCS_CE_EHRPD_SUBS_LIMITED_TO_V6;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_TIMEOUT:
        result = TAF_DCS_CE_EHRPD_VSNCP_TIMEOUT;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_FAILURE:
        result = TAF_DCS_CE_EHRPD_VSNCP_FAILURE;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_3GPP2I_GEN_ERROR:
        result = TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_GEN_ERROR;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_3GPP2I_UNAUTH_APN:
        result = TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_UNAUTH_APN;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_3GPP2I_PDN_LIMIT_EXCEED:
        result = TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_LIMIT_EXCEED;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_3GPP2I_NO_PDN_GW:
        result = TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_NO_PDN_GW;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_3GPP2I_PDN_GW_UNREACH:
        result = TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_GW_UNREACH;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_3GPP2I_PDN_GW_REJ:
        result = TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_GW_REJ;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_3GPP2I_INSUFF_PARAM:
        result = TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_INSUFF_PARAM;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_3GPP2I_RESOURCE_UNAVAIL:
        result = TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_RESOURCE_UNAVAIL;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_3GPP2I_ADMIN_PROHIBIT:
        result = TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_ADMIN_PROHIBIT;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_3GPP2I_PDN_ID_IN_USE:
        result = TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_ID_IN_USE;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_3GPP2I_SUBSCR_LIMITATION:
        result = TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_SUBSCR_LIMITATION;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_3GPP2I_PDN_EXISTS_FOR_THIS_APN:
        result = TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_EXISTS_FOR_THIS_APN;
        break;
    case EHRPDReasonCode::CE_EHRPD_VSNCP_3GPP2I_RECONNECT_NOT_ALLOWED:
        result = TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_RECONNECT_NOT_ALLOWED;
        break;
    case EHRPDReasonCode::CE_EHRPD_UNKNOWN:
        result = TAF_DCS_CE_EHRPD_UNKNOWN;
        break;
    default:
        result = TAF_DCS_CE_EHRPD_UNKNOWN;
        break;
    }
    LE_DEBUG("TelSDK code: %d, TelAF code: %d",
                static_cast<uint32_t>(endReasonCode), static_cast<uint32_t>(result));
    return result;
}

const char *taf_DCSHelper::CallEndEHRPDReasonCodeToString(taf_dcs_CallEndEHRPDReasonCode_t endCode)
{
    switch (endCode)
    {
    case TAF_DCS_CE_EHRPD_SUBS_LIMITED_TO_V4:
        return "TAF_DCS_CE_EHRPD_SUBS_LIMITED_TO_V4";
    case TAF_DCS_CE_EHRPD_SUBS_LIMITED_TO_V6:
        return "TAF_DCS_CE_EHRPD_SUBS_LIMITED_TO_V6";
    case TAF_DCS_CE_EHRPD_VSNCP_TIMEOUT:
        return "TAF_DCS_CE_EHRPD_VSNCP_TIMEOUT";
    case TAF_DCS_CE_EHRPD_VSNCP_FAILURE:
        return "TAF_DCS_CE_EHRPD_VSNCP_FAILURE";
    case TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_GEN_ERROR:
        return "TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_GEN_ERROR";
    case TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_UNAUTH_APN:
        return "TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_UNAUTH_APN";
    case TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_LIMIT_EXCEED:
        return "TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_LIMIT_EXCEED";
    case TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_NO_PDN_GW:
        return "TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_NO_PDN_GW";
    case TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_GW_UNREACH:
        return "TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_GW_UNREACH";
    case TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_GW_REJ:
        return "TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_GW_REJ";
    case TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_INSUFF_PARAM:
        return "TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_INSUFF_PARAM";
    case TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_RESOURCE_UNAVAIL:
        return "TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_RESOURCE_UNAVAIL";
    case TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_ADMIN_PROHIBIT:
        return "TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_ADMIN_PROHIBIT";
    case TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_ID_IN_USE:
        return "TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_ID_IN_USE";
    case TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_SUBSCR_LIMITATION:
        return "TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_SUBSCR_LIMITATION";
    case TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_EXISTS_FOR_THIS_APN:
        return "TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_PDN_EXISTS_FOR_THIS_APN";
    case TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_RECONNECT_NOT_ALLOWED:
        return "TAF_DCS_CE_EHRPD_VSNCP_3GPP2I_RECONNECT_NOT_ALLOWED";
    case TAF_DCS_CE_EHRPD_UNKNOWN:
        return "TAF_DCS_CE_EHRPD_UNKNOWN";
    default:
        LE_WARN("Invalid EHRPD reason code: %d", static_cast<int>(endCode));
        return "Invalid EHRPD reason code";
    }
}

taf_dcs_CallEndIPv6ReasonCode_t taf_DCSHelper::ConvertCallEndIPv6ReasonCode
(
    telux::common::Ipv6ReasonCode endReasonCode
)
{
    using telux::common::Ipv6ReasonCode;
    taf_dcs_CallEndIPv6ReasonCode_t result;
    switch (endReasonCode)
    {
    case Ipv6ReasonCode::CE_PREFIX_UNAVAILABLE:
        result = TAF_DCS_CE_IPV6_PREFIX_UNAVAILABLE;
        break;
    case Ipv6ReasonCode::CE_IPV6_ERR_HRPD_IPV6_DISABLED:
        result = TAF_DCS_CE_IPV6_ERR_HRPD_IPV6_DISABLED;
        break;
    case Ipv6ReasonCode::CE_IPV6_DISABLED:
        result = TAF_DCS_CE_IPV6_DISABLED;
        break;
    default:
        result = TAF_DCS_CE_IPV6_UNKNOWN;
        break;
    }
    LE_DEBUG("TelSDK code: %d, TelAF code: %d",
                static_cast<uint32_t>(endReasonCode), static_cast<uint32_t>(result));
    return result;
}

const char *taf_DCSHelper::CallEndIPv6ReasonCodeToString(taf_dcs_CallEndIPv6ReasonCode_t endCode)
{
    switch (endCode)
    {
    case TAF_DCS_CE_IPV6_UNKNOWN:
        return "TAF_DCS_CE_IPV6_UNKNOWN";
    case TAF_DCS_CE_IPV6_PREFIX_UNAVAILABLE:
        return "TAF_DCS_CE_IPV6_PREFIX_UNAVAILABLE";
    case TAF_DCS_CE_IPV6_ERR_HRPD_IPV6_DISABLED:
        return "TAF_DCS_CE_IPV6_ERR_HRPD_IPV6_DISABLED";
    case TAF_DCS_CE_IPV6_DISABLED:
        return "TAF_DCS_CE_IPV6_DISABLED";
    default:
        LE_WARN("Invalid IPv6 reason code: %d", static_cast<int>(endCode));
        return "Invalid IPv6 reason code";
    }
}

taf_dcs_CallEndHandoffReasonCode_t taf_DCSHelper::ConvertCallEndHandoffReasonCode
(
    telux::common::HandoffReasonCode endReasonCode
)
{
    using telux::common::HandoffReasonCode;
    taf_dcs_CallEndHandoffReasonCode_t result;
    switch (endReasonCode)
    {
    case HandoffReasonCode::CE_VCER_HANDOFF_PREF_SYS_BACK_TO_SRAT:
        result = TAF_DCS_CE_HANDOFF_PREF_SYS_BACK_TO_SRAT;
        break;
    default:
        result = TAF_DCS_CE_HANDOFF_UNKNOWN;
        break;
    }
    LE_DEBUG("TelSDK code: %d, TelAF code: %d",
             static_cast<uint32_t>(endReasonCode), static_cast<uint32_t>(result));
    return result;
}

const char *taf_DCSHelper::CallEndHandoffReasonCodeToString
(
    taf_dcs_CallEndHandoffReasonCode_t endCode
)
{
    switch (endCode)
    {
    case TAF_DCS_CE_HANDOFF_UNKNOWN:
        return "TAF_DCS_CE_HANDOFF_UNKNOWN";
    case TAF_DCS_CE_HANDOFF_PREF_SYS_BACK_TO_SRAT:
        return "TAF_DCS_CE_HANDOFF_PREF_SYS_BACK_TO_SRAT";
    default:
        LE_WARN("Invalid hand off reason code: %d", static_cast<int>(endCode));
        return "Invalid hand off reason code";
    }
}

const char *taf_DCSHelper::CallEndReasonCodeToString
(
    taf_dcs_CallEndReasonType_t endReasonType,
    int32_t endCode
)
{
    switch (endReasonType)
    {
    case TAF_DCS_CE_TYPE_UNKNOWN:
        LE_WARN("Unknown Reason Type");
        return "TAF_DCS_CE_REASON_UNKNOWN";
    case TAF_DCS_CE_TYPE_MOBILE_IP:
        return CallEndMobileIpReasonCodeToString(
            static_cast<taf_dcs_CallEndMobileIpReasonCode_t>(endCode));
    case TAF_DCS_CE_TYPE_INTERNAL:
        return CallEndInternalReasonCodeToString(
            static_cast<taf_dcs_CallEndInternalReasonCode_t>(endCode));
    case TAF_DCS_CE_TYPE_CALL_MANAGER_DEFINED:
        return CallEndCallManagerReasonCodeToString(
            static_cast<taf_dcs_CallEndCallManagerReasonCode_t>(endCode));
    case TAF_DCS_CE_TYPE_3GPP_SPEC_DEFINED:
        return CallEnd3GPPSpecReasonCodeToString(
            static_cast<taf_dcs_CallEnd3GPPSpecReasonCode_t>(endCode));
    case TAF_DCS_CE_TYPE_PPP:
        return CallEndPPPReasonCodeToString(
            static_cast<taf_dcs_CallEndPPPReasonCode_t>(endCode));
    case TAF_DCS_CE_TYPE_EHRPD:
        return CallEndEHRPDReasonCodeToString(
            static_cast<taf_dcs_CallEndEHRPDReasonCode_t>(endCode));
    case TAF_DCS_CE_TYPE_IPV6:
        return CallEndIPv6ReasonCodeToString(
            static_cast<taf_dcs_CallEndIPv6ReasonCode_t>(endCode));
    case TAF_DCS_CE_TYPE_HANDOFF:
        return CallEndHandoffReasonCodeToString(
            static_cast<taf_dcs_CallEndHandoffReasonCode_t>(endCode));
    default:
        LE_WARN("Invalid Reason Type: %d", static_cast<int>(endReasonType));
        return NULL;
    }
}

const char *taf_DCSHelper::IpFamilyTypeToString(taf_dcs_Pdp_t ipType)
{
    switch (ipType)
    {
    case TAF_DCS_PDP_UNKNOWN:
        return "TAF_DCS_PDP_UNKNOWN";
    case TAF_DCS_PDP_IPV4:
        return "TAF_DCS_PDP_IPV4";
    case TAF_DCS_PDP_IPV6:
        return "TAF_DCS_PDP_IPV6";
    case TAF_DCS_PDP_IPV4V6:
        return "TAF_DCS_PDP_IPV4V6";
    default:
        LE_ERROR("unknown ip: %d", ipType);
        return "TAF_DCS_PDP_UNKNOWN";
    }
}

const char *taf_DCSHelper::IpFamilyTypeToString(telux::data::IpFamilyType ipType)
{
    switch (ipType)
    {
    case telux::data::IpFamilyType::IPV4:
        return "IPv4";
    case telux::data::IpFamilyType::IPV6:
        return "IPv6";
    case telux::data::IpFamilyType::IPV4V6:
        return "IPv4v6";
    case telux::data::IpFamilyType::UNKNOWN:
    default:
        LE_ERROR("unknown ip: %d", static_cast<int32_t>(ipType));
        return "UNKNOWN";
    }
}

const char *taf_DCSHelper::TechPreferenceToString(telux::data::TechPreference techPref)
{
    switch (techPref)
    {
    case telux::data::TechPreference::TP_3GPP:
        return "3GPP";
    case telux::data::TechPreference::TP_3GPP2:
        return "3GPP2";
    case telux::data::TechPreference::TP_ANY:
        return "TP_ANY";
    case telux::data::TechPreference::UNKNOWN:
    default:
        LE_ERROR("unknown tech preference(%d)", static_cast<int32_t>(techPref));
        return "UNKNOWN";
    }
}

const char *taf_DCSHelper::DataBearerToString(telux::data::DataBearerTechnology dataBearer)
{
    switch (dataBearer)
    {
    case telux::data::DataBearerTechnology::CDMA_1X:
        return "1X technology";
    case telux::data::DataBearerTechnology::EVDO_REV0:
        return "CDMA Rev 0";
    case telux::data::DataBearerTechnology::EVDO_REVA:
        return "CDMA Rev A";
    case telux::data::DataBearerTechnology::EVDO_REVB:
        return "CDMA Rev B";
    case telux::data::DataBearerTechnology::EHRPD:
        return "EHRPD";
    case telux::data::DataBearerTechnology::FMC:
        return "Fixed mobile convergence";
    case telux::data::DataBearerTechnology::HRPD:
        return "HRPD";
    case telux::data::DataBearerTechnology::BEARER_TECH_3GPP2_WLAN:
        return "3GPP2 IWLAN";
    case telux::data::DataBearerTechnology::WCDMA:
        return "WCDMA";
    case telux::data::DataBearerTechnology::GPRS:
        return "GPRS";
    case telux::data::DataBearerTechnology::HSDPA:
        return "HSDPA";
    case telux::data::DataBearerTechnology::HSUPA:
        return "HSUPA";
    case telux::data::DataBearerTechnology::EDGE:
        return "EDGE";
    case telux::data::DataBearerTechnology::LTE:
        return "LTE";
    case telux::data::DataBearerTechnology::HSDPA_PLUS:
        return "HSDPA+";
    case telux::data::DataBearerTechnology::DC_HSDPA_PLUS:
        return "DC HSDPA+.";
    case telux::data::DataBearerTechnology::HSPA:
        return "HSPA";
    case telux::data::DataBearerTechnology::BEARER_TECH_64_QAM:
        return "64 QAM";
    case telux::data::DataBearerTechnology::TDSCDMA:
        return "TDSCDMA";
    case telux::data::DataBearerTechnology::GSM:
        return "GSM";
    case telux::data::DataBearerTechnology::BEARER_TECH_3GPP_WLAN:
        return "3GPP WLAN";
    case telux::data::DataBearerTechnology::BEARER_TECH_5G:
        return "5G";
    default:
        LE_ERROR("unknown data bearer type(%d)", static_cast<int32_t> (dataBearer));
        return "UNKNOWN";
    }
}
