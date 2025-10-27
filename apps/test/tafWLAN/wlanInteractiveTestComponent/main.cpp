/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       main.cpp
 * @brief      Interactive tests.
 */

#include <future>
#include "wlanInteractiveTest.hpp"

const std::string COLOR_RESET       = "\033[0m";        // Reset to default
const std::string COLOR_BRIGHT_BLUE = "\033[94m"; // Bright Blue

static void printWlanUsage(void)
{
    std::cout << "\n"
                 "WLAN Device Management Test Menu:\n"
              << std::setw(3) << WLAN_DEV_ADD_DEV_STATE_CB
              << "  - Add device state events handler\n"
              << std::setw(3) << WLAN_DEV_REMOVE_DEV_STATE_CB
              << "  - Remove device state events handler\n"
              << std::setw(3) << WLAN_DEV_ON
              << "  - Turn On WLAN\n"
              << std::setw(3) << WLAN_DEV_OFF
              << "  - Turn Off WLAN\n"
              << std::setw(3) << WLAN_DEV_GET_STATE
              << "  - Get WLAN State\n"
              << std::setw(3) << WLAN_DEV_GET_MODE
              << "  - Get WLAN Mode\n"
              << std::setw(3) << WLAN_DEV_SET_MODE
              << "  - Set WLAN Mode <mode>\n"
              << "        Modes: 1-AP 2-STA 3-AP+STA 4-AP+AP 5-AP+AP+STA\n"
              << std::setw(3) << WLAN_DEV_GET_INTERFACES
              << "  - Get WLAN Interfaces\n"
              << std::setw(3) << WLAN_DEV_GET_BAND_INT_STATE
              << "  - Get WLAN Band Interference State\n"
              << std::setw(3) << WLAN_DEV_SET_BAND_INT_STATE
              << "  - Set WLAN Band Interference State <0|1>\n"
              << std::setw(3) << WLAN_DEV_GET_BAND_INT_PRIORITY
              << "  - Get WLAN Band Interference Priority\n"
              << std::setw(3) << WLAN_DEV_SET_BAND_INT_PRIORITY
              << "  - Set WLAN Band Interference Priority <band>\n"
              << std::setw(3) << WLAN_DEV_GET_BAND_INT_WAIT_TIME
              << "  - Get WLAN Band Interference Wait Time\n"
              << std::setw(3) << WLAN_DEV_SET_BAND_INT_WAIT_TIME
              << "  - Set WLAN Band Interference Wait Time <band> <WaitTime>\n"
              << std::endl;
}

static void printApUsage(void)
{
    std::cout << "\n"
                 "WLAN Access Point Management Test Menu:\n"
              << std::setw(3) << WLAN_AP_ADD_DEVICE_CONNECTION_EVENT_CB
              << "  - Add client connection events handler\n"
              << std::setw(3) << WLAN_AP_REMOVE_DEVICE_CONNECTION_EVENT_CB
              << "  - Remove client connection events handler\n"
              << std::setw(3) << WLAN_AP_START
              << "  - Start AP\n"
              << std::setw(3) << WLAN_AP_STOP
              << "  - Stop AP\n"
              << std::setw(3) << WLAN_AP_RESTART
              << "  - Restart AP\n"
              << std::setw(3) << WLAN_AP_GET_STATUS
              << "  - Get AP Status\n"
              << std::setw(3) << WLAN_AP_GET_CONFIG
              << "  - Get AP Config\n"
              << std::setw(3) << WLAN_AP_SET_CONFIG
              << "  - Set AP Config\n"
              << std::setw(3) << WLAN_AP_GET_SECURITY_CONFIG
              << "  - Get AP Security Config\n"
              << std::setw(3) << WLAN_AP_SET_SECURITY_CONFIG
              << "  - Set AP Security Config\n"
              << std::setw(3) << WLAN_AP_GET_CONNECTED_DEVICES_LIST
              << "  - Get Connected Devices List\n"
              << std::endl;
}

static void printStaUsage(void)
{
    std::cout << "\n"
                 "WLAN Station Management Test Menu:\n"
              << std::setw(3) << WLAN_STA_ADD_EVENT_CB
              << "  - Add STA events handler\n"
              << std::setw(3) << WLAN_STA_REMOVE_EVENT_CB
              << "  - Remove STA events handler\n"
              << std::setw(3) << WLAN_STA_START
              << "  - Start STA\n"
              << std::setw(3) << WLAN_STA_STOP
              << "  - Stop STA\n"
              << std::setw(3) << WLAN_STA_RESTART
              << "  - Restart STA\n"
              << std::setw(3) << WLAN_STA_GET_STATUS
              << "  - Get STA Status\n"
              << std::setw(3) << WLAN_STA_GET_MODE
              << "  - Get STA Mode\n"
              << std::setw(3) << WLAN_STA_SET_MODE
              << "  - Set STA Mode <mode>\n"
              << "        Modes: 0-Router 1-Bridge\n"
              << std::setw(3) << WLAN_STA_GET_IP_CONFIG
              << "  - Get STA IP Config\n"
              << std::setw(3) << WLAN_STA_SET_IP_CONFIG
              << "  - Set STA IP Config\n"
              << std::setw(3) << WLAN_STA_DO_AP_SCAN
              << "  - Do AP Scan\n"
              << std::setw(3) << WLAN_STA_GET_AP_SCAN_RESULTS
              << "  - Get AP Scan Results\n"
              << std::setw(3) << WLAN_STA_SET_WPA2_PSK
              << "  - Set WPA2-PSK for a SSID\n"
              << std::setw(3) << WLAN_STA_CONNECT
              << "  - Connect to AP\n"
              << std::setw(3) << WLAN_STA_DISCONNECT
              << "  - Disconnect from AP\n"
              << std::setw(3) << WLAN_STA_GET_AP_SIG_STRENGTH
              << "  - Get Connected AP Signal Strength\n"
              << std::setw(3) << WLAN_STA_ADD_AP_SIG_STRENGTH_CB
              << "  - Add Connected AP Signal Strength CB\n"
              << std::setw(3) << WLAN_STA_REMOVE_AP_SIG_STRENGTH_CB
              << "  - Remove Connected AP Signal Strength CB\n"
              << std::setw(3) << WLAN_STA_GET_AP_EST_THROUGHPUT
              << "  - Get AP Estimated Throughput\n"
              << std::endl;
}

static void printUsage(void)
{
    std::cout << "\n"
                 "Interactive Test Menu:\n"
                 "  0       : Show this menu\n"
                 "  1-50    : WLAN Device Management tests\n"
                 "  51-100  : WLAN Access Point Management tests\n"
                 "  101-150 : WLAN Station Management tests\n"
                 "  999     : Exit\n"
                 "\n";
    printWlanUsage();
    printApUsage();
    printStaUsage();
}

/**
 * Global variables
 */
std::promise<void> wlanTestPromise;
static le_thread_Ref_t wlanTestEventsThreadRef = NULL;

// The event handler thread.
static void *wlanTestEventsThreadHandlerFunc (void *context)
{
    taf_wlan_ConnectService();
    taf_wlanAp_ConnectService();
    taf_wlanSta_ConnectService();
    LE_TEST_INFO("EVent handler thread");
    le_event_RunLoop();
}

// Functions that will be queued to the event handler thread.
static void wlanEventThreadWlanAddDeviceStateCb(void *param1, void *param2)
{
    LE_TEST_INFO("WLAN_DEV_ADD_DEV_STATE_CB");
    le_result_t result = WlanTestAddDeviceStateCb();
    std::cout << "wlanTestAddDeviceStateCb result: " << result << std::endl;
    wlanTestPromise.set_value();
}

static void wlanEventThreadWlanRemoveDeviceStateCb(void *param1, void *param2)
{
    LE_TEST_INFO("WLAN_DEV_REMOVE_DEV_STATE_CB");
    le_result_t result = WlanTestRemoveDeviceStateCb();
    std::cout << "WlanTestRemoveDeviceStateCb result: " << result << std::endl;
    wlanTestPromise.set_value();
}

static void wlanEventThreadWlanApAddDeviceConnectionEventCb(void *param1, void *param2)
{
    LE_TEST_INFO("WLAN_AP_ADD_DEVICE_CONNECTION_EVENT_CB");
    le_result_t result = WlanApTestAddDeviceConnectionEventCb();
    std::cout << "WlanApTestAddDeviceConnectionEventCb result: " << result << std::endl;
    wlanTestPromise.set_value();
}

static void wlanEventThreadWlanApRemoveDeviceConnectionEventCb(void *param1, void *param2)
{
    LE_TEST_INFO("WLAN_AP_REMOVE_DEVICE_CONNECTION_EVENT_CB");
    le_result_t result = WlanApTestRemoveDeviceConnectionEventCb();
    std::cout << "WlanApTestRemoveDeviceConnectionEventCb result: " << result << std::endl;
    wlanTestPromise.set_value();
}

static void wlanEventThreadWlanStaAddEventCb(void *param1, void *param2)
{
    LE_TEST_INFO("WLAN_STA_ADD_EVENT_CB");
    le_result_t result = WlanStaTestAddEventCb();
    std::cout << "WlanStaTestAddEventCb result: " << result << std::endl;
    wlanTestPromise.set_value();
}

static void wlanEventThreadWlanStaRemoveEventCb(void *param1, void *param2)
{
    LE_TEST_INFO("WLAN_STA_REMOVE_EVENT_CB");
    le_result_t result = WlanStaTestRemoveEventCb();
    std::cout << "WlanStaTestRemoveEventCb result: " << result << std::endl;
    wlanTestPromise.set_value();
}

static void wlanEventThreadWlanStaAddConnectedApSignalStrengthCb(void *param1, void *param2)
{
    LE_TEST_INFO("WLAN_STA_ADD_AP_SIG_STRENGTH_CB");
    le_result_t result = WlanStaTestAddConnectedApSignalStrengthCb();
    std::cout << "WlanStaTestAddConnectedApSignalStrengthCb result: " << result << std::endl;
    wlanTestPromise.set_value();
}

static void wlanEventThreadWlanStaRemoveConnectedApSignalStrengthCb(void *param1, void *param2)
{
    LE_TEST_INFO("WLAN_STA_REMOVE_AP_SIG_STRENGTH_CB");
    le_result_t result = WlanStaTestRemoveConnectedApSignalStrengthCb();
    std::cout << "WlanStaTestRemoveConnectedApSignalStrengthCb result: " << result << std::endl;
    wlanTestPromise.set_value();
}

COMPONENT_INIT {
    LE_TEST_INFO("Data Call Interactive Test App");
    le_result_t result;

    // Start events thread
    wlanTestEventsThreadRef = le_thread_Create("wlanTstEvtThr",
                                                        wlanTestEventsThreadHandlerFunc, NULL);
    le_thread_Start(wlanTestEventsThreadRef);

    printUsage();

    while (true) {
        std::cout << std::endl << COLOR_BRIGHT_BLUE << "Select a test from the menu: "
                                                                                << COLOR_RESET;
        std::string line;
        if (!std::getline(std::cin, line)) {
            LE_TEST_INFO("Failed to read input, exiting.");
            break;
        }

        // Check for empty input
        if (line.empty()) {
            printUsage();
            continue;
        }

        int testType = 0;
        try {
            testType = std::stoi(line);
        } catch (const std::invalid_argument& e) {
            LE_TEST_INFO("ERR: Invalid input: %s", e.what());
            std::cout << "Invalid input. Please enter a number." << std::endl;
            continue;
        } catch (const std::out_of_range& e) {
            LE_TEST_INFO("ERR: Input out of range: %s", e.what());
            std::cout << "Input out of range. Please enter a valid number." << std::endl;
            continue;
        }

        if (0  == testType)
        {
            printUsage();
            continue;
        }

        if (testType == 999) {
            LE_TEST_INFO("Exiting interactive test.");
            exit(0);
        }

        if (testType >= WLAN_DEV_ADD_DEV_STATE_CB && testType <= WLAN_DEV_TEST_MAX)
        {
            LE_TEST_INFO("Running WLAN test type: %d", testType);
            switch (testType)
            {
            case WLAN_DEV_ADD_DEV_STATE_CB:
                {
                    wlanTestPromise = std::promise<void>();
                    std::future<void> f = wlanTestPromise.get_future();
                    // Queue to events handler thread.
                    le_event_QueueFunctionToThread(wlanTestEventsThreadRef,
                                                wlanEventThreadWlanAddDeviceStateCb, NULL, NULL);
                    f.get();
                }
                break;
            case WLAN_DEV_REMOVE_DEV_STATE_CB:
                {
                    wlanTestPromise = std::promise<void>();
                    std::future<void> f = wlanTestPromise.get_future();
                    // Queue to events handler thread.
                    le_event_QueueFunctionToThread(wlanTestEventsThreadRef,
                                                wlanEventThreadWlanRemoveDeviceStateCb, NULL, NULL);
                    f.get();
                }
                break;
            case WLAN_DEV_ON:
                result = WlanTestOn();
                std::cout << "wlanTestOn result: " << result << std::endl;
                break;
            case WLAN_DEV_OFF:
                result = WlanTestOff();
                std::cout << "wlanTestOff result: " << result << std::endl;
                break;
            case WLAN_DEV_GET_STATE:
                result = WlanTestGetState();
                std::cout << "wlanTestGetState result: " << result << std::endl;
                break;
            case WLAN_DEV_GET_MODE:
                result = WlanTestGetMode();
                std::cout << "wlanTestGetMode result: " << result << std::endl;
                break;
            case WLAN_DEV_SET_MODE:
                result = WlanTestSetMode();
                std::cout << "wlanTestSetMode result: " << result << std::endl;
                break;
            case WLAN_DEV_GET_INTERFACES:
                result = WlanTestGetInterfaces();
                std::cout << "wlanTestGetInterfaces result: " << result << std::endl;
                break;
            case WLAN_DEV_GET_BAND_INT_STATE:
                result = WlanTestGetBandIntState();
                std::cout << "wlanTestGetBandIntState result: " << result << std::endl;
                break;
            case WLAN_DEV_SET_BAND_INT_STATE:
                result = WlanTestSetBandIntState();
                std::cout << "wlanTestSetBandIntState result: " << result << std::endl;
                break;
            case WLAN_DEV_GET_BAND_INT_PRIORITY:
                result = WlanTestGetBandIntPriority();
                std::cout << "wlanTestGetBandIntPriority result: " << result << std::endl;
                break;
            case WLAN_DEV_SET_BAND_INT_PRIORITY:
                result = WlanTestSetBandIntPriority();
                std::cout << "wlanTestSetBandIntPriority result: " << result << std::endl;
                break;
            case WLAN_DEV_GET_BAND_INT_WAIT_TIME:
                result = WlanTestGetBandIntWaitTime();
                std::cout << "wlanTestGetBandIntWaitTime result: " << result << std::endl;
                break;
            case WLAN_DEV_SET_BAND_INT_WAIT_TIME:
                result = WlanTestSetBandIntWaitTime();
                std::cout << "wlanTestSetBandIntWaitTime result: " << result << std::endl;
                break;
            default:
                LE_TEST_INFO("Invalid WLAN test type.");
                std::cout << "Invalid WLAN test type." << std::endl;
                break;
            };
        }
        else if (testType >= WLAN_AP_ADD_DEVICE_CONNECTION_EVENT_CB && testType <= WLAN_AP_TEST_MAX)
        {
            LE_TEST_INFO("Running AP test type: %d", testType);
            switch (testType)
            {
            case WLAN_AP_ADD_DEVICE_CONNECTION_EVENT_CB:
                {
                    wlanTestPromise = std::promise<void>();
                    std::future<void> f = wlanTestPromise.get_future();
                    // Queue to events handler thread.
                    le_event_QueueFunctionToThread(wlanTestEventsThreadRef,
                                       wlanEventThreadWlanApAddDeviceConnectionEventCb, NULL, NULL);
                    f.get();
                }
                break;
            case WLAN_AP_REMOVE_DEVICE_CONNECTION_EVENT_CB:
                {
                    wlanTestPromise = std::promise<void>();
                    std::future<void> f = wlanTestPromise.get_future();
                    // Queue to events handler thread.
                    le_event_QueueFunctionToThread(wlanTestEventsThreadRef,
                                    wlanEventThreadWlanApRemoveDeviceConnectionEventCb, NULL, NULL);
                    f.get();
                }
                break;
            case WLAN_AP_START:
                result = WlanApTestStart();
                std::cout << "wlanApTestStart result: " << result << std::endl;
                break;
            case WLAN_AP_STOP:
                result = WlanApTestStop();
                std::cout << "wlanApTestStop result: " << result << std::endl;
                break;
            case WLAN_AP_RESTART:
                result = WlanApTestRestart();
                std::cout << "wlanApTestRestart result: " << result << std::endl;
                break;
            case WLAN_AP_GET_STATUS:
                result = WlanApTestGetStatus();
                std::cout << "wlanApTestGetStatus result: " << result << std::endl;
                break;
            case WLAN_AP_GET_CONFIG:
                result = WlanApTestGetConfig();
                std::cout << "wlanApTestGetConfig result: " << result << std::endl;
                break;
            case WLAN_AP_SET_CONFIG:
                result = WlanApTestSetConfig();
                std::cout << "wlanApTestSetConfig result: " << result << std::endl;
                break;
            case WLAN_AP_GET_SECURITY_CONFIG:
                result = WlanApTestGetSecurityConfig();
                std::cout << "wlanApTestGetSecurityConfig result: " << result << std::endl;
                break;
            case WLAN_AP_SET_SECURITY_CONFIG:
                result = WlanApTestSetSecurityConfig();
                std::cout << "wlanApTestSetSecurityConfig result: " << result << std::endl;
                break;
            case WLAN_AP_GET_CONNECTED_DEVICES_LIST:
                result = WlanApTestGetConnectedDevicesList();
                std::cout << "wlanApTestGetConnectedDevicesList result: " << result << std::endl;
                break;
            default:
                LE_TEST_INFO("Invalid AP test type.");
                std::cout << "Invalid AP test type." << std::endl;
                break;
            };
        }
        else if (testType >= WLAN_STA_ADD_EVENT_CB && testType <= WLAN_STA_TEST_MAX)
        {
            LE_TEST_INFO("Running STA test type: %d", testType);
            switch (testType)
            {
            case WLAN_STA_ADD_EVENT_CB:
                {
                    wlanTestPromise = std::promise<void>();
                    std::future<void> f = wlanTestPromise.get_future();
                    // Queue to events handler thread.
                    le_event_QueueFunctionToThread(wlanTestEventsThreadRef,
                                                    wlanEventThreadWlanStaAddEventCb, NULL, NULL);
                    f.get();
                }
                break;
            case WLAN_STA_REMOVE_EVENT_CB:
                {
                    wlanTestPromise = std::promise<void>();
                    std::future<void> f = wlanTestPromise.get_future();
                    // Queue to events handler thread.
                    le_event_QueueFunctionToThread(wlanTestEventsThreadRef,
                                                   wlanEventThreadWlanStaRemoveEventCb, NULL, NULL);
                    f.get();
                }
                break;
            case WLAN_STA_START:
                result = WlanStaTestStart();
                std::cout << "wlanStaTestStart result: " << result << std::endl;
                break;
            case WLAN_STA_STOP:
                result = WlanStaTestStop();
                std::cout << "wlanStaTestStop result: " << result << std::endl;
                break;
            case WLAN_STA_RESTART:
                result = WlanStaTestRestart();
                std::cout << "wlanStaTestRestart result: " << result << std::endl;
                break;
            case WLAN_STA_GET_STATUS:
                result = WlanStaTestGetStatus();
                std::cout << "wlanStaTestGetStatus result: " << result << std::endl;
                break;
            case WLAN_STA_GET_MODE:
                result = WlanStaTestGetMode();
                std::cout << "wlanStaTestGetMode result: " << result << std::endl;
                break;
            case WLAN_STA_SET_MODE:
                result = WlanStaTestSetMode();
                std::cout << "wlanStaTestSetMode result: " << result << std::endl;
                break;
            case WLAN_STA_GET_IP_CONFIG:
                result = WlanStaTestGetIpConfig();
                std::cout << "wlanStaTestGetIpConfig result: " << result << std::endl;
                break;
            case WLAN_STA_SET_IP_CONFIG:
                result = WlanStaTestSetIpConfig();
                std::cout << "wlanStaTestSetIpConfig result: " << result << std::endl;
                break;
            case WLAN_STA_DO_AP_SCAN:
                result = WlanStaTestDoApScan();
                std::cout << "wlanStaTestDoApScan result: " << result << std::endl;
                break;
            case WLAN_STA_GET_AP_SCAN_RESULTS:
                result = WlanStaTestGetApScanResults();
                std::cout << "wlanStaTestGetApScanResults result: " << result << std::endl;
                break;
            case WLAN_STA_SET_WPA2_PSK:
                result = WlanStaTestSetWpa2Psk();
                std::cout << "WlanStaTestSetWpa2Psk result: " << result << std::endl;
                break;
            case WLAN_STA_CONNECT:
                result = WlanStaTestConnect();
                std::cout << "wlanStaTestConnect result: " << result << std::endl;
                break;
            case WLAN_STA_DISCONNECT:
                result = WlanStaTestDisconnect();
                std::cout << "wlanStaTestDisconnect result: " << result << std::endl;
                break;
            case WLAN_STA_GET_AP_SIG_STRENGTH:
                result = WlanStaTestGetConnectedApSignalStrength();
                std::cout << "wlanStaTestGetConnectedApSignalStrength result: " << result
                                                                                    << std::endl;
                break;
            case WLAN_STA_GET_AP_EST_THROUGHPUT:
                result = WlanStaTestGetApEstimatedThroughput();
                std::cout << "WlanStaTestGetApEstimatedThroughput result: " << result << std::endl;
                break;
            case WLAN_STA_ADD_AP_SIG_STRENGTH_CB:
                {
                    wlanTestPromise = std::promise<void>();
                    std::future<void> f = wlanTestPromise.get_future();
                    // Queue to events handler thread.
                    le_event_QueueFunctionToThread(wlanTestEventsThreadRef,
                                  wlanEventThreadWlanStaAddConnectedApSignalStrengthCb, NULL, NULL);
                    f.get();
                }
                break;
            case WLAN_STA_REMOVE_AP_SIG_STRENGTH_CB:
                {
                    wlanTestPromise = std::promise<void>();
                    std::future<void> f = wlanTestPromise.get_future();
                    // Queue to events handler thread.
                    le_event_QueueFunctionToThread(wlanTestEventsThreadRef,
                                wlanEventThreadWlanStaRemoveConnectedApSignalStrengthCb, NULL, NULL);
                    f.get();
                }
                break;
            default:
                LE_TEST_INFO("Invalid STA test type.");
                std::cout << "Invalid STA test type." << std::endl;
                break;
            };
        }
        else
        {
            LE_TEST_INFO("Invalid test type. Please refer to the menu.");
            std::cout << "Invalid test type. Please enter a number from the menu." << std::endl;
            printUsage();
        }

        // Add a small delay to prevent rapid looping in case of issues
        usleep(100000); // 100ms delay
    }
}
