/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       wlanInteractive.cpp
 * @brief      WLAN device manager interactive tests.
 */

#include "wlanInteractiveTest.hpp"

static const char* wlanGetStateString(taf_wlan_DeviceState_t state)
{
    switch (state)
    {
    case TAF_WLAN_OFF:
        return "TAF_WLAN_OFF";
    case TAF_WLAN_ON:
        return "TAF_WLAN_ON";
    default:
        return "TAF_WLAN_UNAVAILABLE";
    }
}

static void wlanDeviceStateHandlerFunc(
    taf_wlan_WlanRef_t WlanRef,
    taf_wlan_DeviceState_t state,
    void *contextPtr)
{
    LE_TEST_INFO("Device State: %d(%s)", state, wlanGetStateString(state));
    std::cout << "Device State: " << state << "(" << wlanGetStateString(state) << ")" << std::endl;
}

static taf_wlan_DeviceStateHandlerRef_t wlanDeviceStateHandlerRef = NULL;

le_result_t WlanTestAddDeviceStateCb()
{
    wlanDeviceStateHandlerRef = taf_wlan_AddDeviceStateHandler(NULL, wlanDeviceStateHandlerFunc,
                                                                                            NULL);
    LE_TEST_OK(nullptr != wlanDeviceStateHandlerRef, "taf_wlan_AddDeviceStateHandler");
    if (wlanDeviceStateHandlerRef)
    {
        return LE_OK;
    }
    return LE_FAULT;
}

le_result_t WlanTestRemoveDeviceStateCb()
{
    if (!wlanDeviceStateHandlerRef)
    {
        LE_TEST_INFO("ERR: Handler is not registered.");
        return LE_FAULT;
    }
    taf_wlan_RemoveDeviceStateHandler(wlanDeviceStateHandlerRef);
    LE_TEST_OK(true, "taf_wlan_RemoveDeviceStateHandler");
    return LE_OK;
}

le_result_t WlanTestOn()
{
    le_result_t result = taf_wlan_TurnOn(NULL);
    LE_TEST_OK(LE_OK == result, "taf_wlan_TurnOn result: %d", result);
    return result;
}

le_result_t WlanTestOff()
{
    le_result_t result = taf_wlan_TurnOff(NULL);
    LE_TEST_OK(LE_OK == result, "taf_wlan_TurnOff result: %d", result);
    return result;
}

le_result_t WlanTestGetState()
{
    taf_wlan_DeviceState_t state;
    le_result_t result = taf_wlan_GetState(NULL, &state);
    LE_TEST_OK(LE_OK == result, "taf_wlan_GetState result: %d", result);
    if (LE_OK == result)
    {
        LE_TEST_INFO("Device State: %d(%s)", state,wlanGetStateString(state));
        std::cout << "Device State: " << state << "(" << wlanGetStateString(state) << ")"
                                                                                    << std::endl;
    }
    return result;
}

static const char* wlanGetModeStr(taf_wlan_DeviceMode_t mode)
{
    if (TAF_WLAN_MODE_UNKNOWN == mode)
        return "TAF_WLAN_MODE_UNKNOWN";
    else if (TAF_WLAN_MODE_AP == mode)
        return "TAF_WLAN_MODE_AP";
    else if (TAF_WLAN_MODE_STA == mode)
        return "TAF_WLAN_MODE_STA";
    else if (TAF_WLAN_MODE_STA_AP == mode)
        return "TAF_WLAN_MODE_STA_AP";
    else if (TAF_WLAN_MODE_AP_AP == mode)
        return "TAF_WLAN_MODE_AP_AP";
    else if (TAF_WLAN_MODE_AP_AP_STA == mode)
        return "TAF_WLAN_MODE_AP_AP_STA";
    else
    {
        return "TAF_WLAN_MODE_UNKNOWN";
    }
}

le_result_t WlanTestGetMode()
{
    taf_wlan_DeviceMode_t mode = TAF_WLAN_MODE_UNKNOWN;
    le_result_t result = taf_wlan_GetMode(NULL, &mode);
    LE_TEST_OK(LE_OK == result, "taf_wlan_GetMode result: %d", result);
    if (LE_OK == result)
    {
        LE_TEST_INFO("Mode: %s(%d)", wlanGetModeStr(mode), mode);
        std::cout << "Mode: " << wlanGetModeStr(mode) << "(" << mode << ")" << std::endl;
    }
    return result;
}

le_result_t WlanTestSetMode()
{
    int mode = -1;
    std::cout << "Enter mode (1-AP 2-STA 3-AP+STA 4-AP+AP 5-AP+AP+STA): ";
    std::cin.clear();
    std::cin >> mode;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    LE_TEST_INFO("Mode: %d", mode);

    le_result_t result = taf_wlan_SetMode(NULL, (taf_wlan_DeviceMode_t)mode);
    LE_TEST_OK(LE_OK == result, "taf_wlan_SetMode result: %d", result);
    return result;
}

le_result_t WlanTestGetInterfaces()
{
    taf_wlan_APIntfInfo_t APIntf[TAF_WLAN_MAX_NUM_AP] = {TAF_WLAN_AP_ID1, TAF_WLAN_AP_ID2};
    taf_wlan_STAIntfInfo_t STAIntf[TAF_WLAN_MAX_NUM_STA] = {TAF_WLAN_STA_ID1};
    size_t APIntfSize = TAF_WLAN_MAX_NUM_AP, STAIntfSize = TAF_WLAN_MAX_NUM_STA;
    le_result_t result = taf_wlan_GetIntfInfo(NULL, APIntf, &APIntfSize, STAIntf, &STAIntfSize);
    LE_TEST_OK(LE_OK == result, "taf_wlan_GetIntfInfo result: %d", result);

    if (LE_OK == result)
    {
        LE_TEST_INFO("----------------------------------");
        LE_TEST_INFO("Num AP : %zu", APIntfSize);
        for (size_t i = 0; i < APIntfSize; i++)
        {
            LE_TEST_INFO("AP ID        : %d", APIntf[i].id);
            LE_TEST_INFO("AP Intf Name : %s", APIntf[i].IntfName);
        }
        LE_TEST_INFO("----------------------------------");
        LE_TEST_INFO("Num STA: %zu", STAIntfSize);
        for (size_t i = 0; i < STAIntfSize; i++)
        {
            LE_TEST_INFO("STA ID        : %d", STAIntf[i].id);
            LE_TEST_INFO("STA Intf Name : %s", STAIntf[i].IntfName);
        }
        LE_TEST_INFO("----------------------------------");

        std::cout << "----------------------------------" << std::endl;
        std::cout << "Num AP : " << APIntfSize << std::endl;
        for (size_t i = 0; i < APIntfSize; i++)
        {
            std::cout << "AP ID        : " << APIntf[i].id << std::endl;
            std::cout << "AP Intf Name : " << APIntf[i].IntfName << std::endl;
        }
        std::cout << "----------------------------------" << std::endl;
        std::cout << "Num STA: " << STAIntfSize << std::endl;
        for (size_t i = 0; i < STAIntfSize; i++)
        {
            std::cout << "STA ID        : " << STAIntf[i].id << std::endl;
            std::cout << "STA Intf Name : " << STAIntf[i].IntfName << std::endl;
        }
        std::cout << "----------------------------------" << std::endl;
    }
    return result;
}

static const char *wlanGetGetBandIntStateStr(taf_wlan_BandIntState_t state)
{
    if (TAF_WLAN_BAND_INT_ENABLED == state)
    {
        return "ENABLED";
    }
    return "DISABLED";
}

le_result_t WlanTestGetBandIntState()
{
    taf_wlan_BandIntState_t state;
    le_result_t result = taf_wlan_GetBandIntState(NULL, &state);

    LE_TEST_OK(LE_OK == result, "taf_wlan_GetBandIntState result: %d", result);
    if (LE_OK == result)
    {
        LE_TEST_INFO("taf_wlan_GetBandIntState succeeded");
        LE_TEST_INFO("State: %d(%s)", state, wlanGetGetBandIntStateStr(state));
        std::cout << "State: " << state << "(" << wlanGetGetBandIntStateStr(state) << ")"
                                                                                      << std::endl;
    }
    return result;
}

le_result_t WlanTestSetBandIntState()
{
    int intState = 0;
    std::cout << "Enter band interference state (0|1): ";
    std::cin.clear();
    std::cin >> intState;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    LE_TEST_INFO("State: %d", intState);

    taf_wlan_BandIntState_t state = static_cast<taf_wlan_BandIntState_t>(intState);
    le_result_t result = taf_wlan_SetBandIntState(NULL, state);
    LE_TEST_OK(LE_OK == result, "taf_wlan_SetBandIntState result: %d", result);
    return result;
}

static const char *wlanGetBandIntPriorityStr(const taf_wlan_BandIntPriority_t priority)
{
    if (TAF_WLAN_PRIO_BAND_N79 == priority)
    {
        return "5G N79";
    }
    if (TAF_WLAN_PRIO_BAND_WLAN_5_GHZ == priority)
    {
        return "WLAN 5GHz";
    }
    return "UNKNOWN";
}

le_result_t WlanTestGetBandIntPriority()
{
    taf_wlan_BandIntPriority_t bandPriority;
    le_result_t result = taf_wlan_GetBandIntPriority(NULL, &bandPriority);
    LE_TEST_OK(LE_OK == result, "taf_wlan_GetBandIntPriority result: %d", result);
    if (LE_OK == result)
    {
        LE_TEST_INFO("taf_wlan_GetBandIntPriority succeeded");
        LE_TEST_INFO("Priority: %d(%s)", bandPriority, wlanGetBandIntPriorityStr(bandPriority));
        std::cout << "Priority: " << bandPriority << "(" << wlanGetBandIntPriorityStr(bandPriority)
                                                                                << ")" << std::endl;
    }
    return result;
}

le_result_t WlanTestSetBandIntPriority()
{
    int intBand = 0;
    std::cout << "Enter band priority (1 - N74, 2 - WLAN 5G): ";
    std::cin.clear();
    std::cin >> intBand;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    LE_TEST_INFO("Priority Band: %d", intBand);

    taf_wlan_BandIntPriority_t bandPriority = static_cast<taf_wlan_BandIntPriority_t>(intBand);
    le_result_t result = taf_wlan_SetBandIntPriority(NULL, bandPriority);
    LE_TEST_OK(LE_OK == result, "taf_wlan_SetBandIntPriority result: %d", result);
    return result;
}

le_result_t WlanTestGetBandIntWaitTime()
{
    taf_wlan_BandIntPriority_t bandPriority = TAF_WLAN_PRIO_BAND_N79;
    uint32_t waitTime = 0;
    le_result_t result = taf_wlan_GetBandIntWaitTime(NULL, bandPriority, &waitTime);
    LE_TEST_OK(LE_OK == result, "taf_wlan_GetBandIntWaitTime N79 5G result: %d", result);

    if (LE_OK == result)
    {
        LE_TEST_INFO("taf_wlan_GetBandIntWaitTime for N79 5G:");
        LE_TEST_INFO("  Priority    : %d", bandPriority);
        LE_TEST_INFO("  Wait time(s): %d", waitTime);

        std::cout << "taf_wlan_GetBandIntWaitTime for N79 5G:" << std::endl;
        std::cout << "  Priority    : " << bandPriority << "("
                                    << wlanGetBandIntPriorityStr(bandPriority) << ")" << std::endl;
        std::cout << "  Wait time(s): " << waitTime << std::endl;
    }

    bandPriority = TAF_WLAN_PRIO_BAND_WLAN_5_GHZ;
    waitTime = 0;
    result = taf_wlan_GetBandIntWaitTime(NULL, bandPriority, &waitTime);
    LE_TEST_OK(LE_OK == result, "taf_wlan_GetBandIntWaitTime WLAN 5G result: %d", result);
    if (LE_OK == result)
    {
        LE_TEST_INFO("taf_wlan_GetBandIntWaitTime for WLAN 5G:");
        LE_TEST_INFO("  Priority    : %d", bandPriority);
        LE_TEST_INFO("  Wait time(s): %d", waitTime);

        std::cout << "taf_wlan_GetBandIntWaitTime for WLAN 5G:" << std::endl;
        std::cout << "  Priority    : " << bandPriority << "("
                                    << wlanGetBandIntPriorityStr(bandPriority) << ")" << std::endl;
        std::cout << "  Wait time(s): " << waitTime << std::endl;
    }
    return result;
}

le_result_t WlanTestSetBandIntWaitTime()
{
    int intBand = 0;
    std::cout << "Enter band priority (1 - N74, 2 - WLAN 5G): ";
    std::cin.clear();
    std::cin >> intBand;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    int intWaitTime;
    std::cout << "Enter wait time (sec): ";
    std::cin.clear();
    std::cin >> intWaitTime;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    LE_TEST_INFO("Priority Band: %d", intBand);
    LE_TEST_INFO("Wait Time(s) : %d", intBand);

    taf_wlan_BandIntPriority_t bandPriority = static_cast<taf_wlan_BandIntPriority_t>((intBand));

    uint32_t waitTime = static_cast<uint32_t>(intWaitTime);

    le_result_t result = taf_wlan_SetBandIntWaitTime(NULL, bandPriority, waitTime);
    LE_TEST_OK(LE_OK == result, "taf_wlan_SetBandIntWaitTime result: %d", result);
    return result;
}
