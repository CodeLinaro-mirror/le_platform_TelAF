/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       wlanIntTest.cpp
 * @brief      Integration test functions for WLAN device manager
 */

#include "interfaces.h"
#include "legato.h"
#include <future>
#include <iostream>

#define MAX_SYSTEM_CMD_LENGTH 200

void PrintUsage(void) {
    puts("\n"
         "app runProc tafWlanIntTest tafWlanIntTest -- On\n"
         "app runProc tafWlanIntTest tafWlanIntTest -- Off\n"
         "app runProc tafWlanIntTest tafWlanIntTest -- GetState\n"
         "app runProc tafWlanIntTest tafWlanIntTest -- GetMode\n"
         "app runProc tafWlanIntTest tafWlanIntTest -- SetMode <mode>\n"
         "            Modes: 1-AP 2-STA 3-AP+STA 4-AP+AP 5-AP+AP+STA\n"
         "app runProc tafWlanIntTest tafWlanIntTest -- GetInterfaces\n"
         "app runProc tafWlanIntTest tafWlanIntTest -- GetBandIntState\n"
         "app runProc tafWlanIntTest tafWlanIntTest -- SetBandIntState <0|1>\n"
         "app runProc tafWlanIntTest tafWlanIntTest -- GetBandIntPriority\n"
         "app runProc tafWlanIntTest tafWlanIntTest -- SetBandIntPriority <band>\n"
         "app runProc tafWlanIntTest tafWlanIntTest -- GetBandIntWaitTime\n"
         "app runProc tafWlanIntTest tafWlanIntTest -- SetBandIntWaitTime <band> <WaitTime>\n"
         "\n");
}

inline void CheckNumArgs(size_t NumArgs, size_t ExpectedNumArgs)
{
    // If num args is less than the number of expected args, print usage and exit FATAL.
    if (NumArgs < ExpectedNumArgs)
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }
}

static le_result_t wlanTestOn() {
    le_result_t result = taf_wlan_TurnOn(NULL);
    fprintf(stderr, "taf_wlan_TurnOn Return:%d\n", result);
    return result;
}

static le_result_t wlanTestOff() {
    le_result_t result = taf_wlan_TurnOff(NULL);
    fprintf(stderr, "taf_wlan_TurnOff Return:%d\n", result);
    return result;
}

static void PrintState(taf_wlan_DeviceState_t state) {
    if (TAF_WLAN_OFF == state)
        LE_TEST_INFO("State: TAF_WLAN_OFF(%d)", state);
    else if (TAF_WLAN_ON == state)
        LE_TEST_INFO("State: TAF_WLAN_ON(%d)", state);
    else if (TAF_WLAN_UNAVAILABLE == state)
        LE_TEST_INFO("State: TAF_WLAN_UNAVAILABLE(%d)", state);
    else {
        // Control should not reach here
        LE_TEST_INFO("*ERR* Unsupported State: %d", state);
    }
    return;
}

static le_result_t wlanTestGetState() {
    taf_wlan_DeviceState_t state;
    le_result_t result = taf_wlan_GetState(NULL, &state);
    fprintf(stderr, "taf_wlan_GetState Return:%d State: %d\n", result, state);
    PrintState(state);
    return result;
}

static void PrintMode(taf_wlan_DeviceMode_t mode) {
    if (TAF_WLAN_MODE_UNKNOWN == mode)
        LE_TEST_INFO("Mode: TAF_WLAN_MODE_UNKNOWN(%d)", mode);
    else if (TAF_WLAN_MODE_AP == mode)
        LE_TEST_INFO("Mode: TAF_WLAN_MODE_AP(%d)", mode);
    else if (TAF_WLAN_MODE_STA == mode)
        LE_TEST_INFO("Mode: TAF_WLAN_MODE_STA(%d)", mode);
    else if (TAF_WLAN_MODE_STA_AP == mode)
        LE_TEST_INFO("Mode: TAF_WLAN_MODE_STA_AP(%d)", mode);
    else if (TAF_WLAN_MODE_AP_AP == mode)
        LE_TEST_INFO("Mode: TAF_WLAN_MODE_AP_AP(%d)", mode);
    else if (TAF_WLAN_MODE_AP_AP_STA == mode)
        LE_TEST_INFO("Mode: TAF_WLAN_MODE_AP_AP_STA(%d)", mode);
    else {
        // Control should not reach here
        LE_TEST_INFO("*ERR* Unsupported Mode: %d", mode);
    }
}

static le_result_t wlanTestGetMode() {
    taf_wlan_DeviceMode_t mode = TAF_WLAN_MODE_UNKNOWN;
    le_result_t result = taf_wlan_GetMode(NULL, &mode);
    fprintf(stderr, "taf_wlan_GetMode Return:%d Mode: %d\n", result, mode);
    PrintMode(mode);
    return result;
}

static le_result_t wlanTestSetMode() {
    const char *SetModeStr = le_arg_GetArg(1);
    if (NULL == SetModeStr)
    {
        PrintUsage();
        LE_TEST_FATAL("Mode value is NULL");
    }

    char modeStr[10]="";   // NULL appended string
    le_utf8_Copy(modeStr,SetModeStr,10,NULL);

    int mode = strtol(modeStr, NULL, 10);
    LE_TEST_INFO("Mode: %d", mode);
    le_result_t result = taf_wlan_SetMode(NULL, (taf_wlan_DeviceMode_t)mode);
    fprintf(stderr, "taf_wlan_SetMode Return:%d\n", result);
    return result;
}

static le_result_t wlanTestGetInterfaces() {
    taf_wlan_APIntfInfo_t APIntf[TAF_WLAN_MAX_NUM_AP] = {TAF_WLAN_AP_ID1, TAF_WLAN_AP_ID2};
    taf_wlan_STAIntfInfo_t STAIntf[TAF_WLAN_MAX_NUM_STA] = {TAF_WLAN_STA_ID1};
    size_t APIntfSize = TAF_WLAN_MAX_NUM_AP, STAIntfSize = TAF_WLAN_MAX_NUM_STA;
    le_result_t result = taf_wlan_GetIntfInfo(NULL, APIntf, &APIntfSize, STAIntf, &STAIntfSize);
    fprintf(stderr, "taf_wlan_GetIntfInfo Return:%d\n", result);
    LE_TEST_INFO("----------------------------------");
    LE_TEST_INFO("Num AP : %" PRIuS "", APIntfSize);
    for (size_t i = 0; i < APIntfSize; i++) {
        LE_TEST_INFO("AP ID        : %d", APIntf[i].id);
        LE_TEST_INFO("AP Intf Name : %s", APIntf[i].IntfName);
    }
    LE_TEST_INFO("----------------------------------");
    LE_TEST_INFO("Num STA: %" PRIuS "", STAIntfSize);
    for (size_t i = 0; i < STAIntfSize; i++) {
        LE_TEST_INFO("STA ID        : %d", STAIntf[i].id);
        LE_TEST_INFO("STA Intf Name : %s", STAIntf[i].IntfName);
    }
    LE_TEST_INFO("----------------------------------");
    return result;
}

static le_result_t wlanTestGetBandIntState()
{
    taf_wlan_BandIntState_t state;
    le_result_t result = taf_wlan_GetBandIntState(NULL, &state);

    if (LE_OK == result)
    {
        LE_TEST_INFO("taf_wlan_GetBandIntState succeeded");
        LE_TEST_INFO("State: %d", state);
    }
    else
    {
        LE_TEST_INFO("taf_wlan_GetBandIntState failed: %d", result);
    }
    std::cout << "taf_wlan_GetBandIntState return: " << result
              << " State: " << state << std::endl;
    return result;
}

static le_result_t wlanTestSetBandIntState()
{
    taf_wlan_BandIntState_t state = TAF_WLAN_BAND_INT_DISABLED;

    const char *stateStr = le_arg_GetArg(1);
    if (NULL == stateStr)
    {
        PrintUsage();
        LE_TEST_FATAL("<enable> is NULL");
    }

    state = static_cast<taf_wlan_BandIntState_t>(atoi(stateStr));

    le_result_t result = taf_wlan_SetBandIntState(NULL, state);
    if (LE_OK == result)
    {
        LE_TEST_INFO("taf_wlan_SetBandIntState succeeded");
    }
    else
    {
        LE_TEST_INFO("taf_wlan_SetBandIntState failed: %d", result);
    }
    std::cout << "taf_wlan_SetBandIntState Return: " << result
              << std::endl;
    return result;
}

static le_result_t wlanTestGetBandIntPriority()
{
    taf_wlan_BandIntPriority_t bandPriority;
    le_result_t result = taf_wlan_GetBandIntPriority(NULL, &bandPriority);
    if (LE_OK == result)
    {
        LE_TEST_INFO("taf_wlan_GetBandIntPriority succeeded");
        LE_TEST_INFO("Priority: %d", bandPriority);
    }
    else
    {
        LE_TEST_INFO("taf_wlan_GetBandIntPriority failed: %d", result);
    }
    std::cout << "taf_wlan_GetBandIntPriority Return: " << result
              << " Band: " << bandPriority << std::endl;
    return result;
}

static le_result_t wlanTestSetBandIntPriority()
{
    taf_wlan_BandIntPriority_t bandPriority;

    const char *bandStr = le_arg_GetArg(1);
    if (NULL == bandStr)
    {
        PrintUsage();
        LE_TEST_FATAL("<band> is NULL");
    }

    bandPriority = static_cast<taf_wlan_BandIntPriority_t>(atoi(bandStr));

    le_result_t result = taf_wlan_SetBandIntPriority(NULL, bandPriority);
    if (LE_OK == result)
    {
        LE_TEST_INFO("taf_wlan_SetBandIntPriority succeeded");
    }
    else
    {
        LE_TEST_INFO("taf_wlan_SetBandIntPriority failed: %d", result);
    }
    std::cout << "taf_wlan_SetBandIntPriority Return: " << result << std::endl;
    return result;
}

static le_result_t wlanTestGetBandIntWaitTime()
{
    taf_wlan_BandIntPriority_t bandPriority = TAF_WLAN_PRIO_BAND_N79;
    uint32_t waitTime=0;
    le_result_t result = taf_wlan_GetBandIntWaitTime(NULL, bandPriority, &waitTime);
    std::cout << "taf_wlan_GetBandIntWaitTime Return: " << result
              << " Band: " << bandPriority
              << " Wait Time: " << waitTime << std::endl;
    if (LE_OK == result)
    {
        LE_TEST_INFO("taf_wlan_GetBandIntWaitTime for N79 5G succeeded");
        LE_TEST_INFO("Priority : %d", bandPriority);
        LE_TEST_INFO("Wait time: %d", waitTime);
    }
    else
    {
        LE_TEST_INFO("taf_wlan_GetBandIntWaitTime for N79 5G failed: %d", result);
        return result;
    }

    bandPriority = TAF_WLAN_PRIO_BAND_WLAN_5_GHZ;
    waitTime = 0;
    result = taf_wlan_GetBandIntWaitTime(NULL, bandPriority, &waitTime);
    if (LE_OK == result)
    {
        LE_TEST_INFO("taf_wlan_GetBandIntWaitTime for WLAN 5G succeeded");
        LE_TEST_INFO("Priority : %d", bandPriority);
        LE_TEST_INFO("Wait time: %d", waitTime);
    }
    else
    {
        LE_TEST_INFO("taf_wlan_GetBandIntWaitTime for WLAN 5G failed: %d", result);
    }
    std::cout << "taf_wlan_GetBandIntWaitTime Return: "<< result
              << " Band: " << bandPriority
              << " Wait Time: " << waitTime << std::endl;
    return result;
}

static le_result_t wlanTestSetBandIntWaitTime()
{
    taf_wlan_BandIntPriority_t bandPriority;
    uint32_t waitTime = 0;

    const char *bandStr = le_arg_GetArg(1);
    if (NULL == bandStr)
    {
        PrintUsage();
        LE_TEST_FATAL("<band> is NULL");
    }
    bandPriority = static_cast<taf_wlan_BandIntPriority_t>(atoi(bandStr));

    const char *waitTimeStr = le_arg_GetArg(2);
    if (NULL == waitTimeStr)
    {
        PrintUsage();
        LE_TEST_FATAL("<waitTime> is NULL");
    }
    waitTime = static_cast<uint32_t>(atoi(waitTimeStr));

    le_result_t result = taf_wlan_SetBandIntWaitTime(NULL, bandPriority, waitTime);
    if (LE_OK == result)
    {
        LE_TEST_INFO("taf_wlan_SetBandIntWaitTime succeeded");
    }
    else
    {
        LE_TEST_INFO("taf_wlan_SetBandIntWaitTime failed: %d", result);
    }
    std::cout << "taf_wlan_SetBandIntWaitTime Return: " << result << std::endl;
    return result;
}

COMPONENT_INIT {
    le_result_t status = LE_FAULT;

    LE_TEST_INIT;

    LE_TEST_INFO("======== WLAN Device Manager Integration Test ========");
    size_t numArgs = le_arg_NumArgs();
    if (numArgs == 0) {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }

    const char* testType = le_arg_GetArg(0);
    if (NULL == testType)
    {
        PrintUsage();
        LE_TEST_FATAL("Test type is NULL");
    }

    if (strncasecmp(testType, "On", strlen("On")) == 0) {
        LE_TEST_INFO("======== WLAN Test: On ========");
        CheckNumArgs(numArgs, 1);
        status = wlanTestOn();
        LE_TEST_OK(LE_OK == status, "WLAN Test: On");
    } else if (strncasecmp(testType, "Off", strlen("Off")) == 0) {
        LE_TEST_INFO("======== WLAN Test: Off ========");
        CheckNumArgs(numArgs, 1);
        status = wlanTestOff();
        LE_TEST_OK(LE_OK == status, "WLAN Test: Off");
    } else if (strncasecmp(testType, "GetState", strlen("GetState")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetState ========");
        CheckNumArgs(numArgs, 1);
        status = wlanTestGetState();
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetState");
    } else if (strncasecmp(testType, "GetMode", strlen("GetMode")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetMode========");
        CheckNumArgs(numArgs, 1);
        status = wlanTestGetMode();
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetMode");
    } else if (strncasecmp(testType, "SetMode", strlen("SetMode")) == 0) {
        LE_TEST_INFO("======== WLAN Test: SetMode========");
        CheckNumArgs(numArgs, 2);
        status = wlanTestSetMode();
        LE_TEST_OK(LE_OK == status, "WLAN Test: SetMode");
    } else if (strncasecmp(testType, "GetInterfaces", strlen("GetInterfaces")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetInterfaces========");
        CheckNumArgs(numArgs, 1);
        status = wlanTestGetInterfaces();
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetInterfaces");
    } else if (strncasecmp(testType, "GetBandIntState", strlen("GetBandIntState")) == 0)
    {
        LE_TEST_INFO("======== WLAN Test: GetBandIntState========");
        CheckNumArgs(numArgs, 1);
        status = wlanTestGetBandIntState();
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetBandIntState");
    } else if (strncasecmp(testType, "SetBandIntState", strlen("SetBandIntState")) == 0)
    {
        LE_TEST_INFO("======== WLAN Test: SetBandIntState========");
        CheckNumArgs(numArgs, 2);
        status = wlanTestSetBandIntState();
        LE_TEST_OK(LE_OK == status, "WLAN Test: SetBandIntState");
    } else if (strncasecmp(testType, "GetBandIntPriority", strlen("GetBandIntPriority")) == 0)
    {
        LE_TEST_INFO("======== WLAN Test: GetBandIntPriority========");
        CheckNumArgs(numArgs, 1);
        status = wlanTestGetBandIntPriority();
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetBandIntPriority");
    } else if (strncasecmp(testType, "SetBandIntPriority", strlen("SetBandIntPriority")) == 0)
    {
        LE_TEST_INFO("======== WLAN Test: SetBandIntPriority========");
        CheckNumArgs(numArgs, 2);
        status = wlanTestSetBandIntPriority();
        LE_TEST_OK(LE_OK == status, "WLAN Test: SetBandIntPriority");
    } else if (strncasecmp(testType, "GetBandIntWaitTime", strlen("GetBandIntWaitTime")) == 0)
    {
        LE_TEST_INFO("======== WLAN Test: GetBandIntWaitTime========");
        CheckNumArgs(numArgs, 1);
        status = wlanTestGetBandIntWaitTime();
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetBandIntWaitTime");
    } else if (strncasecmp(testType, "SetBandIntWaitTime", strlen("SetBandIntWaitTime")) == 0)
    {
        LE_TEST_INFO("======== WLAN Test: SetBandIntWaitTime========");
        CheckNumArgs(numArgs, 3);
        status = wlanTestSetBandIntWaitTime();
        LE_TEST_OK(LE_OK == status, "WLAN Test: SetBandIntWaitTime");
    }
    else
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid test type %s", testType);
    }

    LE_TEST_EXIT;
}
