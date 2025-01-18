/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       wlanIntTest.cpp
 * @brief      Integration test functions for WLAN device manager
 */

#include "interfaces.h"
#include "legato.h"

#define MAX_SYSTEM_CMD_LENGTH 200

void PrintUsage(void) {
    puts("\n"
         "app runProc tafWLANIntTest wlanTest -- On\n"
         "app runProc tafWLANIntTest wlanTest -- Off\n"
         "app runProc tafWLANIntTest wlanTest -- GetState\n"
         "app runProc tafWLANIntTest wlanTest -- GetMode\n"
         "app runProc tafWLANIntTest wlanTest -- SetMode <mode>\n"
         "            Modes: 1-AP 2-STA 3-AP+STA 4-AP+AP 5-AP+AP+STA\n"
         "app runProc tafWLANIntTest wlanTest -- GetInterfaces\n"
         "\n");
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
    taf_wlan_APIntfInfo_t APIntf[TAF_WLAN_MAX_NUM_AP] = { 0 };
    taf_wlan_STAIntfInfo_t STAIntf[TAF_WLAN_MAX_NUM_STA] = { 0 };
    size_t APIntfSize = TAF_WLAN_MAX_NUM_AP, STAIntfSize = TAF_WLAN_MAX_NUM_STA;
    le_result_t result = taf_wlan_GetIntfInfo(NULL, APIntf, &APIntfSize, STAIntf, &STAIntfSize);
    fprintf(stderr, "taf_wlan_GetIntfInfo Return:%d\n", result);
    LE_TEST_INFO("----------------------------------");
    LE_TEST_INFO("Num AP : %" PRIuS "", APIntfSize);
    for (int i = 0; i < APIntfSize; i++) {
        LE_TEST_INFO("AP ID        : %d", APIntf[i].id);
        LE_TEST_INFO("AP Intf Name : %s", APIntf[i].IntfName);
    }
    LE_TEST_INFO("----------------------------------");
    LE_TEST_INFO("Num STA: %" PRIuS "", STAIntfSize);
    for (int i = 0; i < STAIntfSize; i++) {
        LE_TEST_INFO("STA ID        : %d", STAIntf[i].id);
        LE_TEST_INFO("STA Intf Name : %s", STAIntf[i].IntfName);
    }
    LE_TEST_INFO("----------------------------------");
    return result;
}

inline void CheckNumArgs(size_t NumArgs, size_t ExpectedNumArgs) {
    if (NumArgs != ExpectedNumArgs) {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }
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
    } else {
        PrintUsage();
        LE_TEST_FATAL("Invalid test type %s", testType);
    }

    LE_TEST_EXIT;
}
