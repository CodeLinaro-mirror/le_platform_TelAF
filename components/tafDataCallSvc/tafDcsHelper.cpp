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
