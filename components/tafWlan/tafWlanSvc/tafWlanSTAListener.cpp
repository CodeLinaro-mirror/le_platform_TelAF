/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanSTAListener.cpp
 *
 * @brief      Implemenation of TelAF WLAN STA Listener class.
 *
 */

#include "tafWlan.hpp"

using namespace telux::tafsvc;
//--------------------------------------------------------------------------------------------------
/**
 * WLAN STA band changed handler
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTAListener::onStationBandChanged(telux::wlan::BandType radio)
{
    LE_INFO ("WLAN STA band changed to: %d", (int) radio);
}

static void PrintStaState(taf_wlanSta_State_t State)
{
    if (TAF_WLANSTA_STATE_UNKNOWN==State)
        LE_INFO("State: TAF_WLANSTA_STATE_UNKNOWN(%d)", State);
    else if (TAF_WLANSTA_STATE_CONNECTING==State)
        LE_INFO("State: TAF_WLANSTA_STATE_CONNECTING(%d)", State);
    else if (TAF_WLANSTA_STATE_CONNECTED==State)
        LE_INFO("State: TAF_WLANSTA_STATE_CONNECTED(%d)", State);
    else if (TAF_WLANSTA_STATE_DISCONNECTED==State)
        LE_INFO("State: TAF_WLANSTA_STATE_DISCONNECTED(%d)", State);
    else if (TAF_WLANSTA_STATE_ASSOCIATION_FAILED==State)
        LE_INFO("State: TAF_WLANSTA_STATE_ASSOCIATION_FAILED(%d)", State);
    else if (TAF_WLANSTA_STATE_IP_ASSIGNMENT_FAILED==State)
        LE_INFO("State: TAF_WLANSTA_STATE_IP_ASSIGNMENT_FAILED(%d)", State);
    else {
        // Control should not reach here
        LE_WARN("*ERR* Unsupported State: %d", State);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * WLAN STA state changed handler
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTAListener::onStationStatusChanged(std::vector<telux::wlan::StaStatus> staStatus)
{
    for (auto element : staStatus) {
        LE_INFO ("STA Id             : %d", (int) element.id);
        PrintStaState (taf_WlanHelper::StaIntfStatusToTAF(element.status));
        //LE_INFO ("STA State          : %d", (int) element.status);
        LE_INFO ("STA Interface Name : %s", element.name.c_str());
        LE_INFO ("STA MAC Address    : %s", element.macAddress.c_str());
        LE_INFO ("STA IPv4 Address   : %s", element.ipv4Address.c_str());
        LE_INFO ("STA IPv6 Address   : %s", element.ipv6Address.c_str());
    }
}