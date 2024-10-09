/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       wlanAPIntTest.cpp
 * @brief      Integration test functions for WLAN Access Point service.
 */

#include "interfaces.h"
#include "legato.h"

typedef struct
{
    taf_wlanAp_WlanAPRef_t apRef;
    le_sem_Ref_t semRef;
} DevCnxEvtData_t;

/**
 * Example commands:
 * Single AP:
 *       app runProc tafWLANAPIntTest wlanAPTest -- DeviceConnectionEvents wlan0
 * Dual AP: Run the command with different proc names and point to the correct wlan interface
 *       app runProc tafWLANAPIntTest wlanAPTest1 --exe=wlanAPTest -- DeviceConnectionEvents wlan1
 *       app runProc tafWLANAPIntTest wlanAPTest2 --exe=wlanAPTest -- DeviceConnectionEvents wlan2
 */

void PrintUsage(void) {
    puts("\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- help\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- Start <AP>\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- Stop <AP>\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- Restart <AP>\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- GetStatus <AP>\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- SetConfig <AP>\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- GetConfig <AP>\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- SetSecurityConfig <AP>\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- GetSecurityConfig <AP>\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- GetConnectedDevices <AP>\n"
         "app runProc tafWLANAPIntTest wlanAPTest -- DeviceConnectionEvents <AP>\n"
         "\n AP: AP interface obtained from taf_wlan_GetIntfInfo\n"
         "\n");
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

static le_result_t wlanAPTestStart(taf_wlanAp_WlanAPRef_t apRef) {
    le_result_t result = taf_wlanAp_Start(apRef);
    fprintf(stderr, "taf_wlanAp_Start Return:%d\n", result);
    return result;
}

static le_result_t wlanAPTestStop(taf_wlanAp_WlanAPRef_t apRef) {
    le_result_t result = taf_wlanAp_Stop(apRef);
    fprintf(stderr, "taf_wlanAp_Stop Return:%d\n", result);
    return result;
}

static le_result_t wlanAPTestRestart(taf_wlanAp_WlanAPRef_t apRef) {
    le_result_t result = taf_wlanAp_Restart(apRef);
    fprintf(stderr, "taf_wlanAp_Restart Return:%d\n", result);
    return result;
}

static le_result_t wlanAPTestGetStatus(taf_wlanAp_WlanAPRef_t apRef) {
    taf_wlanAp_WlanAPStatus_t Status = { 0, { 0 }, { 0 }, { 0 }, { 0 } };
    le_result_t result = taf_wlanAp_GetStatus(apRef, &Status);
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

static le_result_t wlanAPTestGetConfig(taf_wlanAp_WlanAPRef_t apRef) {
    taf_wlanAp_WlanAPConfig_t Config;
    le_result_t result = taf_wlanAp_GetConfig(apRef, &Config);
    fprintf(stderr, "taf_wlanAp_GetConfig Return:%d\n", result);
    if (LE_OK != result)
        return result;

    LE_TEST_INFO("SSID   : %s", Config.SSID);
    LE_TEST_INFO("Visible: %s", ((Config.bSSIDVisible) ? "Yes" : "No"));
    return result;
}

static le_result_t wlanAPTestSetConfig(taf_wlanAp_WlanAPRef_t apRef) {
    taf_wlanAp_WlanAPConfig_t config;
    config.bSSIDVisible = true;
    le_utf8_Copy(config.SSID, "testSSID", TAF_WLAN_MAX_SSID_LENGTH + 1, NULL);
    le_result_t result = taf_wlanAp_SetConfig(apRef, &config);
    if (LE_OK != result) {
        fprintf(stderr, "taf_wlanAp_SetConfig failed with %d\n", result);
        return result;
    }

    fprintf(stderr, "taf_wlanAp_SetConfig passed\n");
    return result;
}

static le_result_t wlanAPTestGetSecurityConfig(taf_wlanAp_WlanAPRef_t apRef) {
    taf_wlanAp_WlanAPSecurityConfig_t SecConfig;
    le_result_t result = taf_wlanAp_GetSecurityConfig(apRef, &SecConfig);
    fprintf(stderr, "taf_wlanAp_GetSecurityConfig Return:%d\n", result);
    if (LE_OK != result)
        return result;

    PrintSecMode(SecConfig.SecMode);
    PrintAuthMethod(SecConfig.SecAuthMethod);
    PrintSecEncryptMethod(SecConfig.SecEncryptMethod);
    LE_TEST_INFO("Passphrase            : %s", SecConfig.PassPhrase);

    return result;
}

static le_result_t wlanAPTestSetSecurityConfig(taf_wlanAp_WlanAPRef_t apRef) {
    taf_wlanAp_WlanAPSecurityConfig_t SecConfig;
    SecConfig.SecMode = TAF_WLAN_SEC_MODE_OPEN;
    SecConfig.SecAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_NONE;
    SecConfig.SecEncryptMethod = TAF_WLAN_SEC_ENCRYPT_METHOD_AES;
    le_utf8_Copy(SecConfig.PassPhrase, "testPassPhrase", TAF_WLAN_MAX_PASSPHRASE_LENGTH + 1, NULL);

    le_result_t result = taf_wlanAp_SetSecurityConfig(apRef, &SecConfig);
    fprintf(stderr, "taf_wlanAp_SetSecurityConfig Return:%d\n", result);
    if (LE_OK != result) {
        fprintf(stderr, "taf_wlanAp_SetSecurityConfig failed\n");
        return result;
    }

    fprintf(stderr, "taf_wlanAp_SetSecurityConfig passed\n");
    return result;
}

static le_result_t wlanAPTestGetConnectedDevices(taf_wlanAp_WlanAPRef_t apRef) {
    taf_wlanAp_WlanAPConnectedDeviceInfo_t DevInfo[TAF_WLANAP_MAX_CONNECTED_DEVICES];
    uint16_t numDevices = 0;
    size_t DevInfoSize = TAF_WLANAP_MAX_CONNECTED_DEVICES;
    memset(DevInfo, 0,
        (sizeof(taf_wlanAp_WlanAPConnectedDeviceInfo_t) * TAF_WLANAP_MAX_CONNECTED_DEVICES));

    le_result_t result = taf_wlanAp_GetConnectedDevices(apRef, &numDevices, DevInfo, &DevInfoSize);
    fprintf(stderr, "taf_wlanAp_GetConnectedDevices Return:%d\n", result);
    if (LE_OK != result)
        return result;

    LE_TEST_INFO("Num devices connected             : %d", numDevices);
    LE_TEST_INFO("Num device info elements populated: %" PRIuS "", DevInfoSize);
    for (int i = 0; i < DevInfoSize; i++) {
        LE_TEST_INFO("Device : %d", (i + 1));
        LE_TEST_INFO("   Name        : %s", DevInfo[i].Name);
        LE_TEST_INFO("   MACAddress  : %s", DevInfo[i].MACAddress);
        LE_TEST_INFO("   IPv4Address : %s", DevInfo[i].IPv4Address);
    }

    return result;
}

void DeviceConnectionEventHandlerFunc (taf_wlanAp_WlanAPRef_t wlanAPRef,
                                            taf_wlanAp_DeviceConnectionEvent_t event,
                                            const char *LE_NONNULL MACAddress,
                                            ///< The MAC address of the device.
                                            void *contextPtr
)
{
    le_sem_Ref_t semRef = (le_sem_Ref_t)contextPtr;
    if (TAF_WLANAP_DEVICE_CONNECTED == event)
    {
        LE_TEST_INFO("Device Connection Event: CONNECTED %s", MACAddress);
        le_sem_Post(semRef);
    }
    else if (TAF_WLANAP_DEVICE_DISCONNECTED == event)
    {
        LE_TEST_INFO("Device Connection Event: DISCONNECTED %s", MACAddress);
        le_sem_Post(semRef);
    }
    else
    {
        LE_TEST_FATAL("Unknown event received");
    }
}

// Remove handler from thread destructor.
static void DevCnxEvtThreadDestructor (void *context)
{
    LE_TEST_INFO("Remove device connection events handler");
    taf_wlanAp_DeviceConnectionEventHandlerRef_t devConnHandlerRef =
        (taf_wlanAp_DeviceConnectionEventHandlerRef_t)context;
    taf_wlanAp_RemoveDeviceConnectionEventHandler(devConnHandlerRef);
}

static void* DevCnxEvtThreadHdlr (void *context)
{
    taf_wlanAp_DeviceConnectionEventHandlerRef_t devConnHandlerRef = NULL;

    taf_wlanAp_ConnectService();
    DevCnxEvtData_t *ctxDataPtr = (DevCnxEvtData_t *)context;

    devConnHandlerRef = taf_wlanAp_AddDeviceConnectionEventHandler(ctxDataPtr->apRef,
                                                                   DeviceConnectionEventHandlerFunc,
                                                                   ctxDataPtr->semRef);
    LE_TEST_ASSERT(NULL != devConnHandlerRef, "taf_wlanAp_AddDeviceConnectionEventHandler");

    le_thread_AddDestructor(DevCnxEvtThreadDestructor, (void *)devConnHandlerRef);

    le_event_RunLoop();
}

// Test waits for device connected event, then waits for device disconencted event, clean up and
// then returns LE_OK.
static le_result_t wlanAPTestDeviceConnectionEvents(taf_wlanAp_WlanAPRef_t apRef)
{
    DevCnxEvtData_t ctxData = {NULL, NULL};
    le_sem_Ref_t semRef = le_sem_Create("DevCnxEvtSem", 0);
    LE_TEST_ASSERT(NULL != semRef, "le_sem_Create");

    ctxData.semRef = semRef;
    ctxData.apRef  = apRef;
    le_thread_Ref_t threadRef = le_thread_Create("DevCnxEvtThread", DevCnxEvtThreadHdlr,
                                                 (void *)&ctxData);
    LE_TEST_ASSERT(NULL != threadRef, "le_thread_Create");
    le_thread_SetJoinable(threadRef);
    le_thread_Start(threadRef);

    LE_TEST_INFO("Waiting for CONNECTED event");
    le_sem_Wait(semRef);
    LE_TEST_INFO("CONNECTED event received");
    LE_TEST_INFO("Waiting for DISCONNECTED event");
    le_sem_Wait(semRef);
    LE_TEST_INFO("DISCONNECTED event received");
    le_sem_Delete(semRef);
    le_thread_Cancel(threadRef);
    le_thread_Join(threadRef,NULL);
    return LE_OK;
}

inline void CheckNumArgs(size_t NumArgs, size_t ExpectedNumArgs) {
    if ((NumArgs == 1) && (strncasecmp(le_arg_GetArg(0), "help", strlen("help")) == 0)) {
        PrintUsage();
        LE_TEST_EXIT;
    }

    if (NumArgs != ExpectedNumArgs) {
        printf("Invalid command, please select one of these commands: \n");
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }
}

static taf_wlanAp_WlanAPRef_t getAPRef(const char *apIntfNameStr)
{
    taf_wlan_APIntfInfo_t APIntf[TAF_WLAN_MAX_NUM_AP] = {0};
    taf_wlan_STAIntfInfo_t STAIntf[TAF_WLAN_MAX_NUM_STA] = {0};
    size_t APIntfSize = TAF_WLAN_MAX_NUM_AP, STAIntfSize = TAF_WLAN_MAX_NUM_STA;
    le_result_t status = taf_wlan_GetIntfInfo(NULL, APIntf, &APIntfSize, STAIntf, &STAIntfSize);
    if (LE_OK != status)
    {
        LE_TEST_FATAL("taf_wlan_GetIntfInfo failed %d", status);
    }

    if (0==APIntfSize)
    {
        LE_TEST_FATAL("AP is not enabled");
    }
    int APIdx = -1;
    for (int i = 0; i < APIntfSize; i++)
    {
        if (strncasecmp(apIntfNameStr, APIntf[i].IntfName, strlen(apIntfNameStr)) == 0)
        {
            APIdx = i;
            break;
        }
    }
    if (-1 == APIdx)
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid AP interface name: %s", apIntfNameStr);
    }

    taf_wlanAp_WlanAPRef_t apRef = taf_wlanAp_GetWlanAP(APIntf[APIdx].id, APIntf[APIdx].IntfName);
    if (apRef == NULL)
    {
        LE_TEST_FATAL("taf_wlanAp_GetWlanAPReference failed");
    }
    return apRef;
}

COMPONENT_INIT {
    LE_TEST_INIT;

    le_result_t status = LE_FAULT;

    size_t numArgs = le_arg_NumArgs();
    CheckNumArgs(numArgs, 2);

    const char* testType = le_arg_GetArg(0);
    const char* apIntfName = le_arg_GetArg(1);

    LE_TEST_INFO("======== WLAN Access Point Integration Test ========");
    if (NULL == apIntfName)
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid AP interface name is NULL");
    }
    LE_TEST_INFO("AP Interface to use: %s", apIntfName);

    if (strncasecmp(testType, "Start", strlen("Start")) == 0) {
        LE_TEST_INFO("======== WLAN AP Test: Start ========");
        status = wlanAPTestStart(getAPRef(apIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN AP Test: Start");
    } else if (strncasecmp(testType, "Stop", strlen("Stop")) == 0) {
        LE_TEST_INFO("======== WLAN Test: Stop ========");
        status = wlanAPTestStop(getAPRef(apIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: Stop");
    } else if (strncasecmp(testType, "Restart", strlen("Restart")) == 0) {
        LE_TEST_INFO("======== WLAN Test: Restart ========");
        status = wlanAPTestRestart(getAPRef(apIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: Restart");
    } else if (strncasecmp(testType, "GetStatus", strlen("GetStatus")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetStatus ========");
        status = wlanAPTestGetStatus(getAPRef(apIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetStatus");
    } else if (strncasecmp(testType, "GetConfig", strlen("GetConfig")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetConfig ========");
        status = wlanAPTestGetConfig(getAPRef(apIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetConfig");
    } else if (strncasecmp(testType, "SetConfig", strlen("SetConfig")) == 0) {
        LE_TEST_INFO("======== WLAN Test: SetConfig ========");
        status = wlanAPTestSetConfig(getAPRef(apIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: SetConfig");
    } else if (strncasecmp(testType, "GetSecurityConfig", strlen("GetSecurityConfig")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetSecurityConfig ========");
        status = wlanAPTestGetSecurityConfig(getAPRef(apIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetSecurityConfig");
    } else if (strncasecmp(testType, "SetSecurityConfig", strlen("SetSecurityConfig")) == 0) {
        LE_TEST_INFO("======== WLAN Test: SetSecurityConfig ========");
        status = wlanAPTestSetSecurityConfig(getAPRef(apIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: SetSecurityConfig");
    } else if (strncasecmp(testType, "GetConnectedDevices", strlen("GetConnectedDevices")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetConnectedDevices ========");
        status = wlanAPTestGetConnectedDevices(getAPRef(apIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetConnectedDevices");
    } else if (strncasecmp(testType, "DeviceConnectionEvents",strlen("DeviceConnectionEvents"))==0){
        LE_TEST_INFO("======== WLAN Test: DeviceConnectionEvents ========");
        status = wlanAPTestDeviceConnectionEvents(getAPRef(apIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: DeviceConnectionEvents");
    }
    else
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid test type %s", testType);
    }

    LE_TEST_EXIT;
}
