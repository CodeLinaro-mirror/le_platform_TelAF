/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       wlanAPIntTest.cpp
 * @brief      Integration test functions for WLAN Access Point service.
 */

#include "interfaces.h"
#include "legato.h"

#define MAX_SYSTEM_CMD_LENGTH 200

void PrintUsage(void) {
    puts("\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- Start\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- Stop\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- Restart\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- GetStatus\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- SetConfig\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- GetConfig\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- SetSecurityConfig\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- GetSecurityConfig\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- GetConnectedDevices\n"
         "\n");
}

static le_result_t wlanAPTestStart() {
    le_result_t result = taf_wlanAp_Start(NULL);
    fprintf(stderr, "taf_wlanAp_Start Return:%d\n", result);
    return result;
}
static le_result_t wlanAPTestStop() {
    le_result_t result = taf_wlanAp_Stop(NULL);
    fprintf(stderr, "taf_wlanAp_Stop Return:%d\n", result);
    return result;
}

static le_result_t wlanAPTestRestart() {
    le_result_t result = taf_wlanAp_Restart(NULL);
    fprintf(stderr, "taf_wlanAp_Restart Return:%d\n", result);
    return result;
}

static le_result_t wlanAPTestGetStatus() {
    taf_wlanAp_WlanAPStatus_t Status = { 0, { 0 }, { 0 }, { 0 }, { 0 } };
    le_result_t result = taf_wlanAp_GetStatus(NULL, &Status);
    fprintf(stderr, "taf_wlanAp_GetStatus Return:%d\n", result);
    if (LE_OK != result)
        return result;

    LE_TEST_INFO("AP Enabled    : %s", ((Status.bEnabled) ? "Yes" : "No"));
    if (Status.bEnabled) {
        LE_TEST_INFO("Interface Name: %s", Status.IntfName);
        LE_TEST_INFO("IPv4 Address  : %s", Status.IPv4Address);
        LE_TEST_INFO("MAC Address   : %s", Status.MACAddress);
    }
    return result;
}

static le_result_t wlanAPTestGetConfig() {
    taf_wlanAp_WlanAPConfig_t Config;
    le_result_t result = taf_wlanAp_GetConfig(NULL, &Config);
    fprintf(stderr, "taf_wlanAp_GetConfig Return:%d\n", result);
    if (LE_OK != result)
        return result;

    LE_TEST_INFO("SSID   : %s", Config.SSID);
    LE_TEST_INFO("Visible: %s", ((Config.bSSIDVisible) ? "Yes" : "No"));
    return result;
}

static le_result_t wlanAPTestSetConfig() {
    taf_wlanAp_WlanAPConfig_t config;
    config.bSSIDVisible = true;
    le_utf8_Copy(config.SSID, "testSSID", TAF_WLAN_MAX_SSID_LENGTH + 1, NULL);
    le_result_t result = taf_wlanAp_SetConfig(NULL, &config);
    if (LE_OK != result) {
        fprintf(stderr, "taf_wlanAp_SetConfig failed with %d\n", result);
        return result;
    }

    fprintf(stderr, "taf_wlanAp_SetConfig passed\n");
    return result;
}

static void PrintSecMode(taf_wlan_SecurityMode_t SecMode) {
    if (TAF_WLAN_SEC_MODE_UNKNOWN == SecMode)
        LE_TEST_INFO("SecMode: TAF_WLAN_SEC_MODE_UNKNOWN(%d)", SecMode);
    else if (TAF_WLAN_SEC_MODE_OPEN == SecMode)
        LE_TEST_INFO("SecMode: TAF_WLAN_SEC_MODE_OPEN(%d)", SecMode);
    else if (TAF_WLAN_SEC_MODE_WEP == SecMode)
        LE_TEST_INFO("SecMode: TAF_WLAN_SEC_MODE_WEP(%d)", SecMode);
    else if (TAF_WLAN_SEC_MODE_WPA == SecMode)
        LE_TEST_INFO("SecMode: TAF_WLAN_SEC_MODE_WPA(%d)", SecMode);
    else if (TAF_WLAN_SEC_MODE_WPA2 == SecMode)
        LE_TEST_INFO("SecMode: TAF_WLAN_SEC_MODE_WPA2(%d)", SecMode);
    else if (TAF_WLAN_SEC_MODE_WPA3 == SecMode)
        LE_TEST_INFO("SecMode: TAF_WLAN_SEC_MODE_WPA3(%d)", SecMode);
    else {
        // Control should not reach here
        LE_TEST_INFO("*ERR* Unsupported SecMode: %d", SecMode);
    }
}

static void PrintAuthMethod(taf_wlan_SecurityAuthMethod_t AuthMethod) {
    if (TAF_WLAN_SEC_AUTH_METHOD_UNKNOWN == AuthMethod)
        LE_TEST_INFO("AuthMethod: TAF_WLAN_SEC_AUTH_METHOD_UNKNOWN(%d)", AuthMethod);
    else if (TAF_WLAN_SEC_AUTH_METHOD_NONE == AuthMethod)
        LE_TEST_INFO("AuthMethod: TAF_WLAN_SEC_AUTH_METHOD_NONE(%d)", AuthMethod);
    else if (TAF_WLAN_SEC_AUTH_METHOD_PSK == AuthMethod)
        LE_TEST_INFO("AuthMethod: TAF_WLAN_SEC_AUTH_METHOD_PSK(%d)", AuthMethod);
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_SIM == AuthMethod)
        LE_TEST_INFO("AuthMethod: TAF_WLAN_SEC_AUTH_METHOD_EAP_SIM(%d)", AuthMethod);
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_AKA == AuthMethod)
        LE_TEST_INFO("AuthMethod: TAF_WLAN_SEC_AUTH_METHOD_EAP_AKA(%d)", AuthMethod);
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_LEAP == AuthMethod)
        LE_TEST_INFO("AuthMethod: TAF_WLAN_SEC_AUTH_METHOD_EAP_LEAP(%d)", AuthMethod);
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_TLS == AuthMethod)
        LE_TEST_INFO("AuthMethod: TAF_WLAN_SEC_AUTH_METHOD_EAP_TLS(%d)", AuthMethod);
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_TTLS == AuthMethod)
        LE_TEST_INFO("AuthMethod: TAF_WLAN_SEC_AUTH_METHOD_EAP_TTLS(%d)", AuthMethod);
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_PEAP == AuthMethod)
        LE_TEST_INFO("AuthMethod: TAF_WLAN_SEC_AUTH_METHOD_EAP_PEAP(%d)", AuthMethod);
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_FAST == AuthMethod)
        LE_TEST_INFO("AuthMethod: TAF_WLAN_SEC_AUTH_METHOD_EAP_FAST(%d)", AuthMethod);
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_PSK == AuthMethod)
        LE_TEST_INFO("AuthMethod: TAF_WLAN_SEC_AUTH_METHOD_EAP_PSK(%d)", AuthMethod);
    else if (TAF_WLAN_SEC_AUTH_METHOD_SAE == AuthMethod)
        LE_TEST_INFO("AuthMethod: TAF_WLAN_SEC_AUTH_METHOD_SAE(%d)", AuthMethod);
    else {
        // Control should not reach here
        LE_TEST_INFO("*ERR* Unsupported AuthMethod: %d", AuthMethod);
    }
}

static void PrintSecEncryptMethod(taf_wlan_SecurityEncryptionMethod_t SecEncryptMethod) {
    if (TAF_WLAN_SEC_ENCRYPT_METHOD_UNKNOWN == SecEncryptMethod)
        LE_TEST_INFO("EncryptMethod: TAF_WLAN_SEC_ENCRYPT_METHOD_UNKNOWN(%d)", SecEncryptMethod);
    else if (TAF_WLAN_SEC_ENCRYPT_METHOD_RC4 == SecEncryptMethod)
        LE_TEST_INFO("EncryptMethod: TAF_WLAN_SEC_ENCRYPT_METHOD_RC4(%d)", SecEncryptMethod);
    else if (TAF_WLAN_SEC_ENCRYPT_METHOD_TKIP == SecEncryptMethod)
        LE_TEST_INFO("EncryptMethod: TAF_WLAN_SEC_ENCRYPT_METHOD_TKIP(%d)", SecEncryptMethod);
    else if (TAF_WLAN_SEC_ENCRYPT_METHOD_AES == SecEncryptMethod)
        LE_TEST_INFO("EncryptMethod: TAF_WLAN_SEC_ENCRYPT_METHOD_AES(%d)", SecEncryptMethod);
    else if (TAF_WLAN_SEC_ENCRYPT_METHOD_GCMP == SecEncryptMethod)
        LE_TEST_INFO("EncryptMethod: TAF_WLAN_SEC_ENCRYPT_METHOD_GCMP(%d)", SecEncryptMethod);
    else {
        // Control should not reach here
        LE_TEST_INFO("*ERR* Unsupported SecEncryptMethod: %d", SecEncryptMethod);
    }
}

static le_result_t wlanAPTestGetSecurityConfig() {
    taf_wlanAp_WlanAPSecurityConfig_t SecConfig;
    le_result_t result = taf_wlanAp_GetSecurityConfig(NULL, &SecConfig);
    fprintf(stderr, "taf_wlanAp_GetSecurityConfig Return:%d\n", result);
    if (LE_OK != result)
        return result;

    PrintSecMode(SecConfig.SecMode);
    PrintAuthMethod(SecConfig.SecAuthMethod);
    PrintSecEncryptMethod(SecConfig.SecEncryptMethod);
    LE_TEST_INFO("Passphrase            : %s", SecConfig.PassPhrase);

    return result;
}

static le_result_t wlanAPTestSetSecurityConfig() {
    taf_wlanAp_WlanAPSecurityConfig_t SecConfig;
    SecConfig.SecMode = TAF_WLAN_SEC_MODE_OPEN;
    SecConfig.SecAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_NONE;
    SecConfig.SecEncryptMethod = TAF_WLAN_SEC_ENCRYPT_METHOD_AES;
    le_utf8_Copy(SecConfig.PassPhrase, "testPassPhrase", TAF_WLAN_MAX_PASSPHRASE_LENGTH + 1, NULL);

    le_result_t result = taf_wlanAp_SetSecurityConfig(NULL, &SecConfig);
    fprintf(stderr, "taf_wlanAp_SetSecurityConfig Return:%d\n", result);
    if (LE_OK != result) {
        fprintf(stderr, "taf_wlanAp_SetSecurityConfig failed\n");
        return result;
    }

    fprintf(stderr, "taf_wlanAp_SetSecurityConfig passed\n");
    return result;
}

static le_result_t wlanAPTestGetConnectedDevices() {
    taf_wlanAp_WlanAPConnectedDeviceInfo_t DevInfo[TAF_WLANAP_MAX_CONNECTED_DEVICES];
    uint16_t numDevices = 0;
    size_t DevInfoSize = TAF_WLANAP_MAX_CONNECTED_DEVICES;
    memset(DevInfo, 0,
        (sizeof(taf_wlanAp_WlanAPConnectedDeviceInfo_t) * TAF_WLANAP_MAX_CONNECTED_DEVICES));

    le_result_t result = taf_wlanAp_GetConnectedDevices(NULL, &numDevices, DevInfo, &DevInfoSize);
    fprintf(stderr, "taf_wlanAp_GetConnectedDevices Return:%d\n", result);
    if (LE_OK != result)
        return result;

    LE_TEST_INFO("Num devices connected             : %d", numDevices);
    LE_TEST_INFO("Num device info elements populated: %ld", DevInfoSize);
    for (int i = 0; i < DevInfoSize; i++) {
        LE_TEST_INFO("Device : %d", (i + 1));
        LE_TEST_INFO("   Name        : %s", DevInfo[i].Name);
        LE_TEST_INFO("   MACAddress  : %s", DevInfo[i].MACAddress);
        LE_TEST_INFO("   IPv4Address : %s", DevInfo[i].IPv4Address);
    }

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
    size_t numArgs = le_arg_NumArgs();
    const char* testType = le_arg_GetArg(0);

    LE_TEST_INIT;
    LE_TEST_INFO("======== WLAN Access Point Integration Test ========");

    if (strncasecmp(testType, "Start", strlen("Start")) == 0) {
        LE_TEST_INFO("======== WLAN AP Test: Start ========");
        CheckNumArgs(numArgs, 1);
        status = wlanAPTestStart();
        LE_TEST_OK(LE_OK == status, "WLAN AP Test: Start");
    } else if (strncasecmp(testType, "Stop", strlen("Stop")) == 0) {
        LE_TEST_INFO("======== WLAN Test: Stop ========");
        CheckNumArgs(numArgs, 1);
        status = wlanAPTestStop();
        LE_TEST_OK(LE_OK == status, "WLAN Test: Stop");
    } else if (strncasecmp(testType, "Restart", strlen("Restart")) == 0) {
        LE_TEST_INFO("======== WLAN Test: Restart ========");
        CheckNumArgs(numArgs, 1);
        status = wlanAPTestRestart();
        LE_TEST_OK(LE_OK == status, "WLAN Test: Restart");
    } else if (strncasecmp(testType, "GetStatus", strlen("GetStatus")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetStatus ========");
        CheckNumArgs(numArgs, 1);
        status = wlanAPTestGetStatus();
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetStatus");
    } else if (strncasecmp(testType, "GetConfig", strlen("GetConfig")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetConfig ========");
        CheckNumArgs(numArgs, 1);
        status = wlanAPTestGetConfig();
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetConfig");
    } else if (strncasecmp(testType, "SetConfig", strlen("SetConfig")) == 0) {
        LE_TEST_INFO("======== WLAN Test: SetConfig ========");
        CheckNumArgs(numArgs, 1);
        status = wlanAPTestSetConfig();
        LE_TEST_OK(LE_OK == status, "WLAN Test: SetConfig");
    } else if (strncasecmp(testType, "GetSecurityConfig", strlen("GetSecurityConfig")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetSecurityConfig ========");
        CheckNumArgs(numArgs, 1);
        status = wlanAPTestGetSecurityConfig();
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetSecurityConfig");
    } else if (strncasecmp(testType, "SetSecurityConfig", strlen("SetSecurityConfig")) == 0) {
        LE_TEST_INFO("======== WLAN Test: SetSecurityConfig ========");
        CheckNumArgs(numArgs, 1);
        status = wlanAPTestSetSecurityConfig();
        LE_TEST_OK(LE_OK == status, "WLAN Test: SetSecurityConfig");
    } else if (strncasecmp(testType, "GetConnectedDevices", strlen("GetConnectedDevices")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetConnectedDevices ========");
        CheckNumArgs(numArgs, 1);
        status = wlanAPTestGetConnectedDevices();
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetConnectedDevices");
    } else {
        PrintUsage();
        LE_TEST_FATAL("Invalid test type %s", testType);
    }

    LE_TEST_EXIT;
}
