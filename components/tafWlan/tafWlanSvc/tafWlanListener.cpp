/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanListener.cpp
 *
 * @brief      Implemenation of TelAF WLAN Listener class.
 *
 */

#include "tafWlan.hpp"

using namespace telux::tafsvc;
//--------------------------------------------------------------------------------------------------
/**
 * WLAN device enable/disable state handler
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanListener::onEnableChanged(bool enable) {
    // Set promise value
    LE_INFO ("WLAN Device State : %d", (int) enable);
    promise_.set_value(enable);
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    myWlan.SetDeviceState (enable);
}

//--------------------------------------------------------------------------------------------------
/**
 * Access function for promise
 */
//--------------------------------------------------------------------------------------------------
bool taf_WlanListener::getEnableStatus() {
    // Return promise
    return promise_.get_future().get();
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset onEnableChanged promise
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanListener::resetPromise() {
    // Reset promise
    promise_ = std::promise<bool>();
}

//--------------------------------------------------------------------------------------------------
/**
 * WLAN Subsystem state change handler
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanListener::onServiceStatusChange(telux::common::ServiceStatus status) {
    LE_INFO ("Subsystem state: %d", (int) status);
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    myWlan.SetSubsystemState (status);
}
