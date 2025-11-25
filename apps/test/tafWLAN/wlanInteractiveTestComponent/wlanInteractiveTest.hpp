/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       wlanInteractiveTest.hpp
 * @brief      WLAN AP interactive tests header.
 */

#ifndef _WLAN_INTERACTIVE_TEST_HPP_
#define _WLAN_INTERACTIVE_TEST_HPP_

#include <iostream>
#include <iomanip>
#include <limits>
#include <string>
#include <stdexcept>

#include "legato.h"
#include "interfaces.h"

/**
 *   0       : Show this menu
 *   1-50    : WLAN Device Management tests
 *   51-100  : WLAN Access Point Management tests
 *   101-150 : WLAN Station Management tests
 *   999     : Exit
 */

// WLAN device management tests 1 t0 50
enum InteractiveWlanDevTestType
{
    WLAN_DEV_ADD_DEV_STATE_CB = 1,
    WLAN_DEV_REMOVE_DEV_STATE_CB,
    WLAN_DEV_ON,
    WLAN_DEV_OFF,
    WLAN_DEV_GET_STATE,
    WLAN_DEV_GET_MODE,
    WLAN_DEV_SET_MODE,
    WLAN_DEV_GET_INTERFACES,
    WLAN_DEV_GET_BAND_INT_STATE,
    WLAN_DEV_SET_BAND_INT_STATE,
    WLAN_DEV_GET_BAND_INT_PRIORITY,
    WLAN_DEV_SET_BAND_INT_PRIORITY,
    WLAN_DEV_GET_BAND_INT_WAIT_TIME,
    WLAN_DEV_SET_BAND_INT_WAIT_TIME,
    WLAN_DEV_TEST_MAX = 50
};

// WLAN Access point tests 51 to 100
enum InteractiveWlanApTestType
{
    WLAN_AP_ADD_DEVICE_CONNECTION_EVENT_CB = 51,
    WLAN_AP_REMOVE_DEVICE_CONNECTION_EVENT_CB,
    WLAN_AP_START,
    WLAN_AP_STOP,
    WLAN_AP_RESTART,
    WLAN_AP_GET_STATUS,
    WLAN_AP_GET_CONFIG,
    WLAN_AP_SET_CONFIG,
    WLAN_AP_GET_SECURITY_CONFIG,
    WLAN_AP_SET_SECURITY_CONFIG,
    WLAN_AP_GET_CONNECTED_DEVICES_LIST,
    WLAN_AP_TEST_MAX = 100
};

// WLAN Station tests 101 to 150
enum InteractiveWlanStaTestType
{
    WLAN_STA_ADD_EVENT_CB = 101,
    WLAN_STA_REMOVE_EVENT_CB,
    WLAN_STA_START,
    WLAN_STA_STOP,
    WLAN_STA_RESTART,
    WLAN_STA_GET_STATUS,
    WLAN_STA_GET_MODE,
    WLAN_STA_SET_MODE,
    WLAN_STA_GET_IP_CONFIG,
    WLAN_STA_SET_IP_CONFIG,
    WLAN_STA_DO_AP_SCAN,
    WLAN_STA_GET_AP_SCAN_RESULTS,
    WLAN_STA_SET_WPA2_PSK,
    WLAN_STA_CONNECT,
    WLAN_STA_DISCONNECT,
    WLAN_STA_REMOVE_NETWORK,
    WLAN_STA_SAVE_NETWORK_CONFIG,
    WLAN_STA_GET_AP_SIG_STRENGTH,
    WLAN_STA_ADD_AP_SIG_STRENGTH_CB,
    WLAN_STA_REMOVE_AP_SIG_STRENGTH_CB,
    WLAN_STA_GET_AP_EST_THROUGHPUT,
    WLAN_STA_TEST_MAX = 150
};

// Declarations for WLAN device manager test functions
le_result_t WlanTestAddDeviceStateCb();
le_result_t WlanTestRemoveDeviceStateCb();
le_result_t WlanTestOn();
le_result_t WlanTestOff();
le_result_t WlanTestGetState();
le_result_t WlanTestGetMode();
le_result_t WlanTestSetMode();
le_result_t WlanTestGetInterfaces();
le_result_t WlanTestGetBandIntState();
le_result_t WlanTestSetBandIntState();
le_result_t WlanTestGetBandIntPriority();
le_result_t WlanTestSetBandIntPriority();
le_result_t WlanTestGetBandIntWaitTime();
le_result_t WlanTestSetBandIntWaitTime();

// Declarations for WLAN AP test functions
le_result_t WlanApTestAddDeviceConnectionEventCb();
le_result_t WlanApTestRemoveDeviceConnectionEventCb();
le_result_t WlanApTestStart();
le_result_t WlanApTestStop();
le_result_t WlanApTestRestart();
le_result_t WlanApTestGetStatus();
le_result_t WlanApTestGetConfig();
le_result_t WlanApTestSetConfig();
le_result_t WlanApTestGetSecurityConfig();
le_result_t WlanApTestSetSecurityConfig();
le_result_t WlanApTestGetConnectedDevicesList();

// Declarations for WLAN STA test functions
le_result_t WlanStaTestAddEventCb();
le_result_t WlanStaTestRemoveEventCb();
le_result_t WlanStaTestStart();
le_result_t WlanStaTestStop();
le_result_t WlanStaTestRestart();
le_result_t WlanStaTestGetStatus();
le_result_t WlanStaTestGetMode();
le_result_t WlanStaTestSetMode();
le_result_t WlanStaTestGetIpConfig();
le_result_t WlanStaTestSetIpConfig();
le_result_t WlanStaTestDoApScan();
le_result_t WlanStaTestGetApScanResults();
le_result_t WlanStaTestSetWpa2Psk();
le_result_t WlanStaTestConnect();
le_result_t WlanStaTestDisconnect();
le_result_t WlanStaTestRemoveNetwork();
le_result_t WlanStaTestSaveNetworkConfig();
le_result_t WlanStaTestGetConnectedApSignalStrength();
le_result_t WlanStaTestAddConnectedApSignalStrengthCb();
le_result_t WlanStaTestRemoveConnectedApSignalStrengthCb();
le_result_t WlanStaTestGetApEstimatedThroughput();

#endif
