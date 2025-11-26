/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file tafDcsUtils.cpp
 * @brief TelAF Data Call Service's utility functions.
 *
 */

#include "tafDcsUtils.hpp"

using namespace taf::svc::datacall;

taf_dcs_Tech_t TafDcsUtils::ConvertTechPref(taf::pa::data::TechPref_e techPref)
{
    using namespace taf::pa::data;
    switch (techPref)
    {
    case TechPref_e::TP_3GPP:
        return TAF_DCS_TECH_3GPP;
    case TechPref_e::TP_3GPP2:
        return TAF_DCS_TECH_3GPP2;
    case TechPref_e::TP_ANY:
        return TAF_DCS_TECH_ANY;
    default:
        break;
    };
    return TAF_DCS_TECH_UNKNOWN;
}

taf::pa::data::TechPref_e TafDcsUtils::ConvertTechPref(taf_dcs_Tech_t techPref)
{
    using namespace taf::pa::data;
    switch (techPref)
    {
    case TAF_DCS_TECH_3GPP:
        return TechPref_e::TP_3GPP;
    case TAF_DCS_TECH_3GPP2:
        return TechPref_e::TP_3GPP2;
    case TAF_DCS_TECH_ANY:
        return TechPref_e::TP_ANY;
    default:
        break;
    };
    return TechPref_e::TP_UNKNOWN;
}

taf_dcs_Auth_t TafDcsUtils::ConvertAuthType(taf::pa::data::AuthType_e authType)
{
    using namespace taf::pa::data;
    switch (authType)
    {
    case AuthType_e::PAP:
        return TAF_DCS_AUTH_PAP;
    case AuthType_e::CHAP:
        return TAF_DCS_AUTH_CHAP;
    case AuthType_e::PAP_CHAP:
        return TAF_DCS_AUTH_PAP | TAF_DCS_AUTH_CHAP;
    default:
        break;
    };
    return TAF_DCS_AUTH_NONE;
}

taf::pa::data::AuthType_e TafDcsUtils::ConvertAuthType(taf_dcs_Auth_t authType)
{
    using namespace taf::pa::data;
    switch (authType)
    {
    case TAF_DCS_AUTH_PAP:
        return AuthType_e::PAP;
    case TAF_DCS_AUTH_CHAP:
        return AuthType_e::CHAP;
    case TAF_DCS_AUTH_PAP | TAF_DCS_AUTH_CHAP:
        return AuthType_e::PAP_CHAP;
    default:
        break;
    };
    return AuthType_e::NONE;
}

void TafDcsUtils::ConvertCallEndReason
(
    const taf::pa::data::DataCallEndReason_t &reason,
    taf_dcs_CallEndReasonType_t &type,
    int32_t &code)
{
    using namespace taf::pa::data;
    switch(reason.reason)
    {
    case CallEndReason_e::CE_REASON_MOBILE_IP:
        type = TAF_DCS_CE_TYPE_MOBILE_IP;
        code = static_cast<int32_t>(reason.mipCode);
        break;
    case CallEndReason_e::CE_REASON_INTERNAL:
        type = TAF_DCS_CE_TYPE_INTERNAL;
        code = static_cast<int32_t>(reason.internalCode);
        break;
    case CallEndReason_e::CE_REASON_CALL_MANAGER_DEFINED:
        type = TAF_DCS_CE_TYPE_CALL_MANAGER_DEFINED;
        code = static_cast<int32_t>(reason.cmCode);
        break;
    case CallEndReason_e::CE_REASON_3GPP_SPEC_DEFINED:
        type = TAF_DCS_CE_TYPE_3GPP_SPEC_DEFINED;
        code = static_cast<int32_t>(reason.specCode);
        break;
    case CallEndReason_e::CE_REASON_PPP:
        type = TAF_DCS_CE_TYPE_PPP;
        code = static_cast<int32_t>(reason.pppCode);
        break;
    case CallEndReason_e::CE_REASON_EHRPD:
        type = TAF_DCS_CE_TYPE_EHRPD;
        code = static_cast<int32_t>(reason.ehrpdCode);
        break;
    case CallEndReason_e::CE_REASON_IPV6:
        type = TAF_DCS_CE_TYPE_IPV6;
        code = static_cast<int32_t>(reason.ipv6Code);
        break;
    case CallEndReason_e::CE_REASON_HANDOFF:
        type = TAF_DCS_CE_TYPE_HANDOFF;
        code = static_cast<int32_t>(reason.handOffCode);
        break;
    case CallEndReason_e::CE_REASON_UNKNOWN:
    default:
        type = TAF_DCS_CE_TYPE_UNKNOWN;
        code = -1;
        break;
    }
    return;
}

taf_dcs_DataBearerTechnology_t TafDcsUtils::ConvertDataBearerTech
(
    taf::pa::data::DataBearerTechnology_e tech
)
{
    using namespace taf::pa::data;
    switch (tech)
    {
    case DataBearerTechnology_e::BEARER_CDMA_1X:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_1X;
    case DataBearerTechnology_e::BEARER_EVDO_REV0:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EVDO;
    case DataBearerTechnology_e::BEARER_EVDO_REVA:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EVDO_REVA;
    case DataBearerTechnology_e::BEARER_EVDO_REVB:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EVDO_REVB;
    case DataBearerTechnology_e::BEARER_EHRPD:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_EHRPD;
    case DataBearerTechnology_e::BEARER_FMC:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA_EVDO_FMC;
    case DataBearerTechnology_e::BEARER_HRPD:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_CDMA2000_HRPD;
    case DataBearerTechnology_e::BEARER_3GPP2_WLAN:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_3GPP2_WLAN;
    case DataBearerTechnology_e::BEARER_WCDMA:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_WCDMA;
    case DataBearerTechnology_e::BEARER_GPRS:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_GPRS;
    case DataBearerTechnology_e::BEARER_HSDPA:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_HSDPA;
    case DataBearerTechnology_e::BEARER_HSUPA:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_HSUPA;
    case DataBearerTechnology_e::BEARER_EDGE:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_EDGE;
    case DataBearerTechnology_e::BEARER_LTE:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_LTE;
    case DataBearerTechnology_e::BEARER_HSDPA_PLUS:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_HSDPA_PLUS;
    case DataBearerTechnology_e::BEARER_DC_HSDPA_PLUS:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_DC_HSDPA_PLUS;
    case DataBearerTechnology_e::BEARER_HSPA:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_HSPA;
    case DataBearerTechnology_e::BEARER_64_QAM:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_64_QAM;
    case DataBearerTechnology_e::BEARER_TDSCDMA:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_TD_SCDMA;
    case DataBearerTechnology_e::BEARER_GSM:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_GSM;
    case DataBearerTechnology_e::BEARER_3GPP_WLAN:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_3GPP_WLAN;
    case DataBearerTechnology_e::BEARER_5G:
        return TAF_DCS_DATA_BEARER_TECHNOLOGY_5G;
    default:
    return TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
    };
}

taf::pa::data::IpType_e TafDcsUtils::ConvertPDP(taf_dcs_Pdp_t ipType)
{
    using namespace taf::pa::data;
    switch (ipType)
    {
        case TAF_DCS_PDP_IPV4:
            return IpType_e::IPV4;
        case TAF_DCS_PDP_IPV6:
            return IpType_e::IPV6;
        case TAF_DCS_PDP_IPV4V6:
            return IpType_e::IPV4V6;
        default:
            break;
    };
    return IpType_e::UNKNOWN;
}

taf::pa::data::DataCallStatus_e TafDcsUtils::ConvertDataCallStatus(taf_dcs_ConState_t state)
{
    switch (state)
    {
        case TAF_DCS_DISCONNECTED:
            return taf::pa::data::DataCallStatus_e::DISCONNECTED;
        case TAF_DCS_CONNECTING:
            return taf::pa::data::DataCallStatus_e::CONNECTING;
        case TAF_DCS_CONNECTED:
            return taf::pa::data::DataCallStatus_e::CONNECTED;
        case TAF_DCS_DISCONNECTING:
            return taf::pa::data::DataCallStatus_e::DISCONNECTING;
        default:
            break;
    }
    return taf::pa::data::DataCallStatus_e::UNKNOWN;
}
taf_dcs_ConState_t TafDcsUtils::ConvertDataCallStatus(taf::pa::data::DataCallStatus_e status)
{
    switch (status)
    {
        case taf::pa::data::DataCallStatus_e::CONNECTING:
            return TAF_DCS_CONNECTING;
        case taf::pa::data::DataCallStatus_e::CONNECTED:
            return TAF_DCS_CONNECTED;
        case taf::pa::data::DataCallStatus_e::DISCONNECTING:
            return TAF_DCS_DISCONNECTING;
        default:
            break;
    }
    return TAF_DCS_DISCONNECTED;
}

taf_dcs_Pdp_t TafDcsUtils::ConvertPDP(taf::pa::data::IpType_e ipType)
{
    using namespace taf::pa::data;
    switch (ipType)
    {
    case IpType_e::IPV4:
        return TAF_DCS_PDP_IPV4;
    case IpType_e::IPV6:
        return TAF_DCS_PDP_IPV6;
    case IpType_e::IPV4V6:
        return TAF_DCS_PDP_IPV4V6;
    default:
        break;
    };
    return TAF_DCS_PDP_UNKNOWN;
}

taf_dcs_ApnType_t TafDcsUtils::ConvertApnTypeMask
(
    taf::pa::data::ApnTypeBitmask_e apnTypeMask
)
{
    return static_cast<taf_dcs_ApnType_t>(apnTypeMask);
}

taf::pa::data::ApnTypeBitmask_e TafDcsUtils::ConvertApnTypeMask(taf_dcs_ApnType_t apnTypeMask)
{
    return static_cast<taf::pa::data::ApnTypeBitmask_e>(apnTypeMask);
}

bool TafDcsUtils::ConvertEmergencyCallSupport
(
    taf::pa::data::EmergencyCapability_e emergencyCallSupport
)
{
    using namespace taf::pa::data;
    if (EmergencyCapability_e::ALLOWED == emergencyCallSupport)
    {
        return true;
    }
    return false;
}

taf::pa::data::EmergencyCapability_e TafDcsUtils::ConvertEmergencyCallSupport(bool bEmergencyCap)
{
    using namespace taf::pa::data;
    if (bEmergencyCap)
    {
        return taf::pa::data::EmergencyCapability_e::ALLOWED;
    }
    return taf::pa::data::EmergencyCapability_e::NOT_ALLOWED;
}

taf_dcs_RoamingType_t TafDcsUtils::ConvertRoamingType(taf::pa::data::RoamingType_e type)
{
    using namespace taf::pa::data;
    switch (type)
    {
    case RoamingType_e::DOMESTIC:
        return TAF_DCS_ROAMING_DOMESTIC;
    case RoamingType_e::INTERNATIONAL:
        return TAF_DCS_ROAMING_INTERNATIONAL;
    default:
        break;
    }
    return TAF_DCS_ROAMING_UNKNOWN;
}

taf_dcs_HwAccelerationState_t TafDcsUtils::ConvertHwAccelerationState
(
    taf::pa::data::HwAccelerationState_e state)
{
    if (taf::pa::data::HwAccelerationState_e::ACTIVE == state)
    {
        return TAF_DCS_HW_ACCELERATION_ACTIVE;
    }
    return TAF_DCS_HW_ACCELERATION_INACTIVE;
}

taf_dcs_QosFlowState_t TafDcsUtils::ConvertQoSFlowState(taf::pa::data::QosFlowState_e state)
{
    if (taf::pa::data::QosFlowState_e::ACTIVATED == state) {
        return TAF_DCS_QOS_ACTIVATED;
    }
    if (taf::pa::data::QosFlowState_e::MODIFIED == state)
    {
        return TAF_DCS_QOS_MODIFIED;
    }
    if (taf::pa::data::QosFlowState_e::DELETED == state)
    {
        return TAF_DCS_QOS_DELETED;
    }
    return TAF_DCS_QOS_UNKNOWN;
}

taf_dcs_QosFlowBitMask_t TafDcsUtils::ConvertQoSFlowBitMask(taf::pa::data::QosFlowMask_e mask)
{
    // They are both 32 bit.
    return static_cast<taf_dcs_QosFlowBitMask_t>(static_cast<std::bitset<32>>(mask).to_ulong());
}

/***************************************************************************************************
 * To string functions
 **************************************************************************************************/
const char *TafDcsUtils::ToString(taf::pa::data::TechPref_e techPref)
{
    switch (techPref)
    {
    case taf::pa::data::TechPref_e::TP_3GPP:
        return "3GPP";
    case taf::pa::data::TechPref_e::TP_3GPP2:
        return "3GPP2";
    case taf::pa::data::TechPref_e::TP_ANY:
        return "TP_ANY";
    case taf::pa::data::TechPref_e::TP_UNKNOWN:
    default:
        {
            LE_WARN("Unknown techPref:  %d", TO_INT(techPref));
            return "UNKNOWN";
        }
    };
}

const char *TafDcsUtils::ToString(taf::pa::data::Subsystem_e subsystem)
{
    using namespace taf::pa::data;
    switch (subsystem)
    {
    case Subsystem_e::PHONE_MANAGER:
        return "PHONE_MANAGER";
    case Subsystem_e::PROFILE_MANAGER:
        return "PROFILE_MANAGER";
    case Subsystem_e::DATACALL_MANAGER:
        return "DATACALL_MANAGER";
    case Subsystem_e::SERVING_SYSTEM_MANAGER:
        return "SERVING_SYSTEM_MANAGER";
    default:
        LE_WARN("Unknown subsystem:  %d", TO_INT(subsystem));
        return "UNKNOWN";
    };
}
const char *TafDcsUtils::ToString(taf::pa::data::SubsystemState_e state)
{
    using namespace taf::pa::data;
    switch (state)
    {
    case SubsystemState_e::AVAILABLE:
        return "AVAILABLE";
    case SubsystemState_e::UNAVAILABLE:
        return "UNAVAILABLE";
    case SubsystemState_e::FAILED:
        return "FAILED";
    default:
        LE_WARN("Unknown subsystem state:  %d", TO_INT(state));
        return "FAILED";
    };
}