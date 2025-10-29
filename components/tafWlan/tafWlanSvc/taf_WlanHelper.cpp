/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanCommon.cpp
 *
 * @brief      Helper functions
 *
 */

#include "tafWlan.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Split string based on delimiter
 */
//--------------------------------------------------------------------------------------------------
std::vector<std::string> taf_WlanHelper::StrSplit(const std::string &str, char delim)
{
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;

    while (getline(ss, item, delim))
    {
        result.push_back(item);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert StaBridgeMode from TelSDK to TelAF value
 */
//--------------------------------------------------------------------------------------------------
taf_wlanSta_Mode_t taf_WlanHelper::StaModeToTAF(telux::wlan::StaBridgeMode Mode)
{
    if (Mode == telux::wlan::StaBridgeMode::ROUTER)
    {
        return TAF_WLANSTA_MODE_ROUTER;
    }
    else if (Mode == telux::wlan::StaBridgeMode::BRIDGE)
    {
        return TAF_WLANSTA_MODE_BRIDGE;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return TAF_WLANSTA_MODE_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert StaBridgeMode from TelAF to TelSDK value
 */
//--------------------------------------------------------------------------------------------------
telux::wlan::StaBridgeMode taf_WlanHelper::StaModeToTelux(taf_wlanSta_Mode_t Mode)
{
    if (Mode == TAF_WLANSTA_MODE_ROUTER)
    {
        return telux::wlan::StaBridgeMode::ROUTER;
    }
    else if (Mode == TAF_WLANSTA_MODE_BRIDGE)
    {
        return telux::wlan::StaBridgeMode::BRIDGE;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return telux::wlan::StaBridgeMode::ROUTER;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert StaIpConfig from TelSDK to TelAF value
 */
//--------------------------------------------------------------------------------------------------
taf_wlanSta_IPType_t taf_WlanHelper::StaIPTypeToTAF(telux::wlan::StaIpConfig IPType)
{
    if (IPType == telux::wlan::StaIpConfig::DYNAMIC_IP)
    {
        return TAF_WLANSTA_IPTYPE_DYNAMIC;
    }
    else if (IPType == telux::wlan::StaIpConfig::STATIC_IP)
    {
        return TAF_WLANSTA_IPTYPE_STATIC;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return TAF_WLANSTA_IPTYPE_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert StaIpConfig from TelAF to TelSDK value
 */
//--------------------------------------------------------------------------------------------------
telux::wlan::StaIpConfig taf_WlanHelper::StaIPTypeToTelux(taf_wlanSta_IPType_t IPType)
{
    if (IPType == TAF_WLANSTA_IPTYPE_DYNAMIC)
    {
        return telux::wlan::StaIpConfig::DYNAMIC_IP;
    }
    else if (IPType == TAF_WLANSTA_IPTYPE_STATIC)
    {
        return telux::wlan::StaIpConfig::STATIC_IP;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return telux::wlan::StaIpConfig::DYNAMIC_IP;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert StaInterfaceStatus from TelSDK to TelAF value
 */
//--------------------------------------------------------------------------------------------------
taf_wlanSta_State_t taf_WlanHelper::StaIntfStatusToTAF(telux::wlan::StaInterfaceStatus State)
{
    if (State == telux::wlan::StaInterfaceStatus::UNKNOWN)
    {
        return TAF_WLANSTA_STATE_UNKNOWN;
    }
    else if (State == telux::wlan::StaInterfaceStatus::CONNECTING)
    {
        return TAF_WLANSTA_STATE_CONNECTING;
    }
    else if (State == telux::wlan::StaInterfaceStatus::CONNECTED)
    {
        return TAF_WLANSTA_STATE_CONNECTED;
    }
    else if (State == telux::wlan::StaInterfaceStatus::DISCONNECTED)
    {
        return TAF_WLANSTA_STATE_DISCONNECTED;
    }
    else if (State == telux::wlan::StaInterfaceStatus::ASSOCIATION_FAILED)
    {
        return TAF_WLANSTA_STATE_ASSOCIATION_FAILED;
    }
    else if (State == telux::wlan::StaInterfaceStatus::IP_ASSIGNMENT_FAILED)
    {
        return TAF_WLANSTA_STATE_IP_ASSIGNMENT_FAILED;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return TAF_WLANSTA_STATE_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert TelAF STA ID to Telux WLAN ID
 */
//--------------------------------------------------------------------------------------------------
telux::wlan::Id taf_WlanHelper::TAFSTAidtoTeluxId(taf_wlan_STAid_t id)
{
    if (TAF_WLAN_STA_ID1 == id)
    {
        return telux::wlan::Id::PRIMARY;
    }

    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return telux::wlan::Id::PRIMARY;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert TelAF band priority to Telux band priority
 */
//--------------------------------------------------------------------------------------------------
telux::data::BandPriority taf_WlanHelper::ConvertInterferenceBand(taf_wlan_BandIntPriority_t band)
{
    if (TAF_WLAN_PRIO_BAND_N79 == band)
        return telux::data::BandPriority::N79;

    return telux::data::BandPriority::WLAN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert Telux band priority to TelAF band priority
 */
//--------------------------------------------------------------------------------------------------
taf_wlan_BandIntPriority_t taf_WlanHelper::ConvertInterferenceBand(telux::data::BandPriority band)
{
    if (telux::data::BandPriority::N79 == band)
        return TAF_WLAN_PRIO_BAND_N79;

    return TAF_WLAN_PRIO_BAND_WLAN_5_GHZ;
}
