/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       wlanAPIntTest.cpp
 * @brief      Integration test functions for WLAN Access Point service.
 */

#include "legato.h"
#include "interfaces.h"

#define MAX_SYSTEM_CMD_LENGTH 200

static void PrintUsage(void)
{
    puts("\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- Start\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- Stop\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- Restart\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- GetStatus\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- GetConfig\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- GetSecurityConfig\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- GetConnectedDevices\n"
         "\n");
}

static le_result_t wlanAPTestStart()
{
    le_result_t result = taf_wlanAp_Start(NULL);
    fprintf(stderr, "taf_wlanAp_Start Return:%d\n", result);
    return result;
}
static le_result_t wlanAPTestStop()
{
    le_result_t result = taf_wlanAp_Stop(NULL);
    fprintf(stderr, "taf_wlanAp_Stop Return:%d\n", result);
    return result;
}

static le_result_t wlanAPTestRestart()
{
    le_result_t result = taf_wlanAp_Restart(NULL);
    fprintf(stderr, "taf_wlanAp_Restart Return:%d\n", result);
    return result;
}

static le_result_t wlanAPTestGetStatus()
{
    taf_wlanAp_WlanAPStatus_t Status = {0, {0}, {0}, {0}, {0}};
    le_result_t result = taf_wlanAp_GetStatus(NULL, &Status);
    fprintf(stderr, "taf_wlanAp_GetStatus Return:%d\n", result);
    if (LE_OK != result)
        return result;

    LE_TEST_INFO("AP Enabled    : %s", ((Status.bEnabled) ? "Yes" : "No"));
    if (Status.bEnabled){
        LE_TEST_INFO("Interface Name: %s", Status.IntfName);
        LE_TEST_INFO("IPv4 Address  : %s", Status.IPv4Address);
        LE_TEST_INFO("MAC Address   : %s", Status.MACAddress);
    }
    return result;
}

static le_result_t wlanAPTestGetConfig()
{
    taf_wlanAp_WlanAPConfig_t Config;
    le_result_t result = taf_wlanAp_GetConfig(NULL, &Config);
    fprintf(stderr, "taf_wlanAp_GetConfig Return:%d\n", result);
    if (LE_OK != result)
        return result;

    LE_TEST_INFO("SSID   : %s", Config.SSID);
    LE_TEST_INFO("Visible: %s", ((Config.bSSIDVisible) ? "Yes" : "No"));
    return result;
}

static le_result_t wlanAPTestGetSecurityConfig()
{
    taf_wlanAp_WlanAPSecurityConfig_t SecConfig;
    le_result_t result = taf_wlanAp_GetSecurityConfig(NULL, &SecConfig);
    fprintf(stderr, "taf_wlanAp_GetSecurityConfig Return:%d\n", result);
    if (LE_OK != result)
        return result;

    LE_TEST_INFO("Sec Mode              : %d", SecConfig.SecMode);
    LE_TEST_INFO("Sec Auth Method       : %d", SecConfig.SecAuthMethod);
    LE_TEST_INFO("Sec Encryption Method : %d", SecConfig.SecEncryptMethod);
    LE_TEST_INFO("Passphrase            : %s", SecConfig.PassPhrase);

    return result;
}

static le_result_t wlanAPTestGetConnectedDevices()
{
    taf_wlanAp_WlanAPConnectedDeviceInfo_t DevInfo[TAF_WLANAP_MAX_CONNECTED_DEVICES];
    uint16_t numDevices = 0;
    size_t DevInfoSize = TAF_WLANAP_MAX_CONNECTED_DEVICES;
    memset(DevInfo, 0,
           (sizeof(taf_wlanAp_WlanAPConnectedDeviceInfo_t) * TAF_WLANAP_MAX_CONNECTED_DEVICES));

    le_result_t result = taf_wlanAp_GetConnectedDevices(NULL, &numDevices,
                                                         DevInfo, &DevInfoSize);
    fprintf(stderr, "taf_wlanAp_GetConnectedDevices Return:%d\n", result);
    if (LE_OK != result)
        return result;

    LE_TEST_INFO("Num devices connected             : %d",  numDevices);
    LE_TEST_INFO("Num device info elements populated: %ld", DevInfoSize);
    for (int i = 0; i < DevInfoSize;i++){
        LE_TEST_INFO("Device : %d", (i+1));
        LE_TEST_INFO("   Name        : %s", DevInfo[i].Name);
        LE_TEST_INFO("   MACAddress  : %s", DevInfo[i].MACAddress);
        LE_TEST_INFO("   IPv4Address : %s", DevInfo[i].IPv4Address);
    }

    return result;
}

COMPONENT_INIT
{
    le_result_t status = LE_FAULT;

    LE_TEST_INIT;

    LE_TEST_INFO("======== WLAN Access Point Integration Test ========");
    size_t numArgs = le_arg_NumArgs();

    // Number of arguments should be 1 or 2.
    if ((1 == numArgs))
    {
        const char *testType = le_arg_GetArg(0);
        if (strncmp(testType, "Start", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== WLAN AP Test: Start ========");
            status = wlanAPTestStart();
            LE_TEST_OK(LE_OK == status, "WLAN AP Test: Start");
        }
        else if (strncmp(testType, "Stop", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== WLAN Test: Stop ========");
            status = wlanAPTestStop();
            LE_TEST_OK(LE_OK == status, "WLAN Test: Stop");
        }
        else if (strncmp(testType, "Restart", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== WLAN Test: Restart ========");
            status = wlanAPTestRestart();
            LE_TEST_OK(LE_OK == status, "WLAN Test: Restart");
        }
        else if (strncmp(testType, "GetStatus", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== WLAN Test: GetStatus ========");
            status = wlanAPTestGetStatus();
            LE_TEST_OK(LE_OK == status, "WLAN Test: GetStatus");
        }
        else if (strncmp(testType, "GetConfig", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== WLAN Test: GetConfig ========");
            status = wlanAPTestGetConfig();
            LE_TEST_OK(LE_OK == status, "WLAN Test: GetConfig");
        }
        else if (strncmp(testType, "GetSecurityConfig", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== WLAN Test: GetSecurityConfig ========");
            status = wlanAPTestGetSecurityConfig();
            LE_TEST_OK(LE_OK == status, "WLAN Test: GetSecurityConfig");
        }
        else if (strncmp(testType, "GetConnectedDevices", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== WLAN Test: GetConnectedDevices ========");
            status = wlanAPTestGetConnectedDevices();
            LE_TEST_OK(LE_OK == status, "WLAN Test: GetConnectedDevices");
        }
        else
        {
            PrintUsage();
            LE_TEST_FATAL("Invalid test type %s", testType);
        }
    }
    else
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }

    LE_TEST_EXIT;
}