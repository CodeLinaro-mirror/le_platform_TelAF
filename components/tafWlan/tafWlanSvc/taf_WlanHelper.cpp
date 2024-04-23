/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Trim trailing space from a string.
 * This helper function needed because the WPA_EVENT_ defines have a trailing space.
 */
//--------------------------------------------------------------------------------------------------
std::string taf_WlanHelper::StrTrimEndSpace(const std::string &str)
{
    size_t endpos = str.find_last_not_of(" \t");
    if (std::string::npos != endpos)
    {
        return str.substr(0, endpos + 1);
    }
    return str;
}

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
 * Convert BandType from TelSDK to TelAF value
 */
//--------------------------------------------------------------------------------------------------
taf_wlan_Band_t taf_WlanHelper::BandTypeToTAF (telux::wlan::BandType bandType)
{
    if (telux::wlan::BandType::BAND_5GHZ == bandType) {
        return TAF_WLAN_BAND_5_GHZ;
    }
    else if (telux::wlan::BandType::BAND_2GHZ == bandType)
    {
        return TAF_WLAN_BAND_2P4_GHZ;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN ("Control should not reach here");
    return TAF_WLAN_BAND_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert BandType from TelAF to TelSDK value
 */
//--------------------------------------------------------------------------------------------------
telux::wlan::BandType taf_WlanHelper::BandTypeToTelux (taf_wlan_Band_t bandType)
{
    if (TAF_WLAN_BAND_5_GHZ == bandType){
        return telux::wlan::BandType::BAND_5GHZ;
    }
    else if (TAF_WLAN_BAND_2P4_GHZ == bandType){
        return telux::wlan::BandType::BAND_2GHZ;
    }

    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN ("Control should not reach here");
    return telux::wlan::BandType::BAND_2GHZ;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert APType from TelSDK to TelAF value
 */
//--------------------------------------------------------------------------------------------------
taf_wlan_APType_t taf_WlanHelper::APTypeToTAF(telux::wlan::ApType APType)
{
    if (APType==telux::wlan::ApType::UNKNOWN ){
        return TAF_WLAN_UNKNOWN_AP;
    }
    else if (APType == telux::wlan::ApType::PRIVATE)
    {
        return TAF_WLAN_PRIVATE_AP;
    }
    else if (APType == telux::wlan::ApType::GUEST)
    {
        return TAF_WLAN_GUEST_AP;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN ("Control should not reach here");
    return TAF_WLAN_UNKNOWN_AP;
}
//--------------------------------------------------------------------------------------------------
/**
 * Convert APType from TelAF to TelSDK value
 */
//--------------------------------------------------------------------------------------------------
telux::wlan::ApType taf_WlanHelper::APTypeToTelux(taf_wlan_APType_t APType)
{
    if (APType==TAF_WLAN_UNKNOWN_AP ){
        return telux::wlan::ApType::UNKNOWN;
    }
    else if (APType == TAF_WLAN_PRIVATE_AP)
    {
        return telux::wlan::ApType::PRIVATE;
    }
    else if (APType == TAF_WLAN_GUEST_AP)
    {
        return telux::wlan::ApType::GUEST;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return telux::wlan::ApType::UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert SecMode from TelSDK to TelAF value
 */
//--------------------------------------------------------------------------------------------------
taf_wlan_SecurityMode_t taf_WlanHelper::SecModeToTAF(telux::wlan::SecMode SecMode)
{
    if (SecMode == telux::wlan::SecMode::OPEN)
    {
        return TAF_WLAN_SEC_MODE_OPEN;
    }
    else if (SecMode == telux::wlan::SecMode::WEP)
    {
        return TAF_WLAN_SEC_MODE_WEP;
    }
    else if (SecMode == telux::wlan::SecMode::WPA)
    {
        return TAF_WLAN_SEC_MODE_WPA;
    }
    else if (SecMode == telux::wlan::SecMode::WPA2)
    {
        return TAF_WLAN_SEC_MODE_WPA2;
    }
    else if (SecMode == telux::wlan::SecMode::WPA3)
    {
        return TAF_WLAN_SEC_MODE_WPA3;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return TAF_WLAN_SEC_MODE_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert SecMode from TelAF to TelSDK value
 */
//--------------------------------------------------------------------------------------------------
telux::wlan::SecMode taf_WlanHelper::SecModeToTelux(taf_wlan_SecurityMode_t SecMode)
{
    if (SecMode==TAF_WLAN_SEC_MODE_OPEN ){
        return telux::wlan::SecMode::OPEN;
    }
    else if (SecMode== TAF_WLAN_SEC_MODE_WEP){
        return telux::wlan::SecMode::WEP;
    }
    else if (SecMode ==TAF_WLAN_SEC_MODE_WPA)
    {
        return telux::wlan::SecMode::WPA;
    }
    else if (SecMode ==TAF_WLAN_SEC_MODE_WPA2)
    {
        return telux::wlan::SecMode::WPA2;
    }
    else if (SecMode == TAF_WLAN_SEC_MODE_WPA3)
    {
        return telux::wlan::SecMode::WPA3;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return telux::wlan::SecMode::OPEN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert SecAuth from TelSDK to TelAF value
 */
//--------------------------------------------------------------------------------------------------
taf_wlan_SecurityAuthMethod_t taf_WlanHelper::SecAuthToTAF(telux::wlan::SecAuth SecAuth)
{
    if (SecAuth == telux::wlan::SecAuth::NONE)
    {
        return TAF_WLAN_SEC_AUTH_METHOD_NONE;
    }
    else if (SecAuth == telux::wlan::SecAuth::PSK)
    {
        return TAF_WLAN_SEC_AUTH_METHOD_PSK;
    }
    else if (SecAuth == telux::wlan::SecAuth::EAP_SIM)
    {
        return TAF_WLAN_SEC_AUTH_METHOD_EAP_SIM;
    }
    else if (SecAuth == telux::wlan::SecAuth::EAP_AKA)
    {
        return TAF_WLAN_SEC_AUTH_METHOD_EAP_AKA;
    }
    else if (SecAuth == telux::wlan::SecAuth::EAP_LEAP)
    {
        return TAF_WLAN_SEC_AUTH_METHOD_EAP_LEAP;
    }
    else if (SecAuth == telux::wlan::SecAuth::EAP_TLS)
    {
        return TAF_WLAN_SEC_AUTH_METHOD_EAP_TLS;
    }
    else if (SecAuth == telux::wlan::SecAuth::EAP_TTLS)
    {
        return TAF_WLAN_SEC_AUTH_METHOD_EAP_TTLS;
    }
    else if (SecAuth == telux::wlan::SecAuth::EAP_PEAP)
    {
        return TAF_WLAN_SEC_AUTH_METHOD_EAP_PEAP;
    }
    else if (SecAuth == telux::wlan::SecAuth::EAP_FAST)
    {
        return TAF_WLAN_SEC_AUTH_METHOD_EAP_FAST;
    }
    else if (SecAuth == telux::wlan::SecAuth::EAP_PSK)
    {
        return TAF_WLAN_SEC_AUTH_METHOD_EAP_PSK;
    }
    else if (SecAuth == telux::wlan::SecAuth::SAE)
    {
        return TAF_WLAN_SEC_AUTH_METHOD_SAE;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return TAF_WLAN_SEC_AUTH_METHOD_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert SecAuth from TelAF to TelSDK value
 */
//--------------------------------------------------------------------------------------------------
telux::wlan::SecAuth taf_WlanHelper::SecAuthToTelux(taf_wlan_SecurityAuthMethod_t SecAuth)
{
    if (SecAuth == TAF_WLAN_SEC_AUTH_METHOD_NONE)
    {
        return telux::wlan::SecAuth::NONE;
    }
    else if (SecAuth == TAF_WLAN_SEC_AUTH_METHOD_PSK)
    {
        return telux::wlan::SecAuth::PSK;
    }
    else if (SecAuth == TAF_WLAN_SEC_AUTH_METHOD_EAP_SIM)
    {
        return telux::wlan::SecAuth::EAP_SIM;
    }
    else if (SecAuth == TAF_WLAN_SEC_AUTH_METHOD_EAP_AKA)
    {
        return telux::wlan::SecAuth::EAP_AKA;
    }
    else if (SecAuth == TAF_WLAN_SEC_AUTH_METHOD_EAP_LEAP)
    {
        return telux::wlan::SecAuth::EAP_LEAP;
    }
    else if (SecAuth == TAF_WLAN_SEC_AUTH_METHOD_EAP_TLS)
    {
        return telux::wlan::SecAuth::EAP_TLS;
    }
    else if (SecAuth == TAF_WLAN_SEC_AUTH_METHOD_EAP_TTLS)
    {
        return telux::wlan::SecAuth::EAP_TTLS;
    }
    else if (SecAuth == TAF_WLAN_SEC_AUTH_METHOD_EAP_PEAP)
    {
        return telux::wlan::SecAuth::EAP_PEAP;
    }
    else if (SecAuth == TAF_WLAN_SEC_AUTH_METHOD_EAP_FAST)
    {
        return telux::wlan::SecAuth::EAP_FAST;
    }
    else if (SecAuth == TAF_WLAN_SEC_AUTH_METHOD_EAP_PSK)
    {
        return telux::wlan::SecAuth::EAP_PSK;
    }
    else if (SecAuth == TAF_WLAN_SEC_AUTH_METHOD_SAE)
    {
        return telux::wlan::SecAuth::SAE;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return telux::wlan::SecAuth::NONE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert SecEncrypt from TelSDK to TelAF value
 */
//--------------------------------------------------------------------------------------------------
taf_wlan_SecurityEncryptionMethod_t
taf_WlanHelper::SecEncryptToTAF(telux::wlan::SecEncrypt SecEncrypt)
{
    if (SecEncrypt == telux::wlan::SecEncrypt::RC4)
    {
        return TAF_WLAN_SEC_ENCRYPT_METHOD_RC4;
    }
    else if (SecEncrypt == telux::wlan::SecEncrypt::TKIP)
    {
        return TAF_WLAN_SEC_ENCRYPT_METHOD_TKIP;
    }
    else if (SecEncrypt == telux::wlan::SecEncrypt::AES)
    {
        return TAF_WLAN_SEC_ENCRYPT_METHOD_AES;
    }
    else if (SecEncrypt == telux::wlan::SecEncrypt::GCMP)
    {
        return TAF_WLAN_SEC_ENCRYPT_METHOD_GCMP;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return TAF_WLAN_SEC_ENCRYPT_METHOD_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert SecAuth from TelAF to TelSDK value
 */
//--------------------------------------------------------------------------------------------------
telux::wlan::SecEncrypt
taf_WlanHelper::SecEncryptToTelux(taf_wlan_SecurityEncryptionMethod_t SecEncrypt)
{
    if (SecEncrypt == TAF_WLAN_SEC_ENCRYPT_METHOD_RC4)
    {
        return telux::wlan::SecEncrypt::RC4;
    }
    else if (SecEncrypt == TAF_WLAN_SEC_ENCRYPT_METHOD_TKIP)
    {
        return telux::wlan::SecEncrypt::TKIP;
    }
    else if (SecEncrypt == TAF_WLAN_SEC_ENCRYPT_METHOD_AES)
    {
        return telux::wlan::SecEncrypt::AES;
    }
    else if (SecEncrypt == TAF_WLAN_SEC_ENCRYPT_METHOD_GCMP)
    {
        return telux::wlan::SecEncrypt::GCMP;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return telux::wlan::SecEncrypt::RC4;
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

////--------------------------------------------------------------------------------------------------
/**
 * Convert Telux WLAN ID to TelAF AP ID
 */
//--------------------------------------------------------------------------------------------------
taf_wlan_APid_t taf_WlanHelper::TeluxIdtoTAFAPId(telux::wlan::Id id)
{
    if (telux::wlan::Id::PRIMARY == id)
    {
        return TAF_WLAN_AP_ID1;
    }
    else if (telux::wlan::Id::SECONDARY == id)
    {
        return TAF_WLAN_AP_ID2;
    }

    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return TAF_WLAN_AP_ID1;
}

////--------------------------------------------------------------------------------------------------
/**
 * Convert TelAF AP ID to Telux WLAN ID
 */
//--------------------------------------------------------------------------------------------------
telux::wlan::Id taf_WlanHelper::TAFAPidtoTeluxId(taf_wlan_APid_t id)
{
    if (TAF_WLAN_AP_ID1 == id)
    {
        return telux::wlan::Id::PRIMARY;
    }
    else if (TAF_WLAN_AP_ID2 == id)
    {
        return telux::wlan::Id::SECONDARY;
    }
    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return telux::wlan::Id::PRIMARY;
}

////--------------------------------------------------------------------------------------------------
/**
 * Convert Telux WLAN ID to TelAF STA ID
 */
//--------------------------------------------------------------------------------------------------
taf_wlan_STAid_t taf_WlanHelper::TeluxIdtoTAFSTAId(telux::wlan::Id id)
{
    if (telux::wlan::Id::PRIMARY == id)
    {
        return TAF_WLAN_STA_ID1;
    }

    // To avoid error "control reaches end of non-void function", return some value with warning
    LE_WARN("Control should not reach here");
    return TAF_WLAN_STA_ID1;
}

////--------------------------------------------------------------------------------------------------
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
