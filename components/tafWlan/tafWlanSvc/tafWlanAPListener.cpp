/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanAPListener.cpp
 *
 * @brief      Implemenation of TelAF WLAN AP Listener class.
 *
 */

#include "tafWlan.hpp"

using namespace telux::tafsvc;
//--------------------------------------------------------------------------------------------------
/**
 * WLAN AP config changed handler
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPListener::onApConfigChanged(telux::wlan::Id apId)
{
    LE_INFO ("WLAN Config changed for AP ID : %d", (int) apId);
}

//--------------------------------------------------------------------------------------------------
/**
 * WLAN AP band changed handler
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPListener::onApBandChanged(telux::wlan::BandType radio)
{
    LE_INFO ("WLAN AP band changed to: %d", (int) radio);
}

//--------------------------------------------------------------------------------------------------
/**
 * WLAN AP connected device state changed handler
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPListener::onApDeviceStatusChanged(
    telux::wlan::ApDeviceConnectionEvent event,
    std::vector<telux::wlan::DeviceIndInfo> info)
{
    LE_INFO ("WLAN AP connected device changed event: %d", (int) event);
    LE_INFO ("Device info size : %ld", info.size());
    for (auto element : info) {
        LE_INFO ("Device Id : %d", (int) element.id);
        LE_INFO ("Device MAC: %s", element.macAddress.c_str());
    }
}
