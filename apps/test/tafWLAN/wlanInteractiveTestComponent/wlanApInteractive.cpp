/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       wlanApInteractive.cpp
 * @brief      WLAN AP interactive tests.
 */

#include <map>
#include "wlanInteractiveTest.hpp"

// Static reference to the event handler
static std::map<taf_wlan_APid_t, taf_wlanAp_DeviceConnectionEventHandlerRef_t>
    wlanApDevConnHandlerMap =
    {
        {TAF_WLAN_AP_ID1, nullptr},
        {TAF_WLAN_AP_ID2, nullptr},
    };
static int wlanApDevConnContexts[TAF_WLAN_MAX_NUM_AP + 1] = {-1,1,2};

static taf_wlanAp_WlanAPRef_t wlanApGetRef()
{
    int intApId = 1;
    char interfaceStr[24] = {0};

    std::cout << "Enter AP Id (1 or 2): ";
    std::cin.clear();
    std::cin >> intApId;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter AP interface:  ";
    std::cin.getline(interfaceStr, 24);

    LE_TEST_INFO("AP ID  : %d", intApId);
    LE_TEST_INFO("AP Intf: %s", interfaceStr);

    taf_wlanAp_WlanAPRef_t ref = taf_wlanAp_GetWlanAP((taf_wlan_APid_t)intApId, interfaceStr);
    LE_TEST_OK(nullptr != ref, "AP reference should be valid.");
    return ref;
}

static const char *wlanApGetConnectionEventString(taf_wlanAp_DeviceConnectionEvent_t event)
{
    if (TAF_WLANAP_DEVICE_CONNECTED == event)
    {
        return "CONNECTED";
    }
    return "DISCONNECTED";
}

static void wlanDeviceConnectionEventHandlerFunc(
    taf_wlanAp_WlanAPRef_t wlanAPRef,
    taf_wlanAp_DeviceConnectionEvent_t event,
    const char *LE_NONNULL clientMACAddress,
    void *contextPtr
)
{
    int intApId = -1;
    if (contextPtr)
    {
        intApId = *(static_cast<int*>(contextPtr));
    }
    LE_TEST_INFO("ConnectionEventHandler(AP %d): deviceConnectionEvent = %d(%s), MACAddress = %s",
                 intApId, event, wlanApGetConnectionEventString(event), clientMACAddress);
    std::cout << "Device ConnectionEventHandler(AP " << intApId << ") :" << std ::endl;
    std::cout << "   Event: " << wlanApGetConnectionEventString(event) << std ::endl;
    std::cout << "   MAC  : " << clientMACAddress << std ::endl;
}


le_result_t WlanApTestAddDeviceConnectionEventCb()
{
    int intApId = 1;
    char interfaceStr[24] = {0};

    std::cout << "Enter AP Id (1 or 2): ";
    std::cin.clear();
    std::cin >> intApId;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_wlan_APid_t apId = static_cast<taf_wlan_APid_t>(intApId);

    if (nullptr == wlanApDevConnHandlerMap[apId])
    {
        std::cout << "Enter AP interface:  ";
        std::cin.getline(interfaceStr, 24);

        LE_TEST_INFO("AP ID  : %d", intApId);
        LE_TEST_INFO("AP Intf: %s", interfaceStr);

        taf_wlanAp_WlanAPRef_t apRef = taf_wlanAp_GetWlanAP((taf_wlan_APid_t)intApId, interfaceStr);
        if (nullptr == apRef)
        {
            LE_TEST_INFO("ERR: Unable to get AP ref");
            return LE_FAULT;
        }
        wlanApDevConnHandlerMap[apId] = taf_wlanAp_AddDeviceConnectionEventHandler(apRef,
                            wlanDeviceConnectionEventHandlerFunc, &wlanApDevConnContexts[apId]);
        LE_TEST_OK(nullptr != wlanApDevConnHandlerMap[apId],
                                                "taf_wlanAp_AddDeviceConnectionEventHandler");
        if (nullptr == wlanApDevConnHandlerMap[apId])
        {
            return LE_FAULT;
        }
    }
    else
    {
        LE_TEST_INFO("Device connection event handler already added for AP ID: %d", apId);
    }
    return LE_OK;
}

le_result_t WlanApTestRemoveDeviceConnectionEventCb()
{
    LE_TEST_INFO("Removing device connection event handler");
    int intApId = 1;
    std::cout << "Enter AP Id (1 or 2): ";
    std::cin.clear();
    std::cin >> intApId;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    taf_wlan_APid_t apId = static_cast<taf_wlan_APid_t>(intApId);

    if (nullptr != wlanApDevConnHandlerMap[apId])
    {
        taf_wlanAp_RemoveDeviceConnectionEventHandler(wlanApDevConnHandlerMap[apId]);
        wlanApDevConnHandlerMap[apId] = nullptr;
    }
    else
    {
        LE_TEST_INFO("ERR: No device connection event handler to remove.");
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t WlanApTestStart()
{
    LE_TEST_INFO("Starting WLAN AP");
    taf_wlanAp_WlanAPRef_t apRef = wlanApGetRef();
    if (apRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get AP reference for starting.");
        return LE_FAULT;
    }

    le_result_t result = taf_wlanAp_Start(apRef);
    LE_TEST_OK(LE_OK == result, "taf_wlanAp_Start result: %d", result);

    return result;
}

le_result_t WlanApTestStop()
{
    LE_TEST_INFO("Stopping WLAN AP");
    taf_wlanAp_WlanAPRef_t apRef = wlanApGetRef();
    if (apRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get AP reference for stopping.");
        return LE_FAULT;
    }

    le_result_t result = taf_wlanAp_Stop(apRef);
    LE_TEST_OK(LE_OK == result, "taf_wlanAp_Stop result: %d", result);
    return result;
}

le_result_t WlanApTestRestart()
{
    LE_TEST_INFO("Restarting WLAN AP");
    taf_wlanAp_WlanAPRef_t apRef = wlanApGetRef();
    if (apRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get AP reference for restarting.");
        return LE_FAULT;
    }

    le_result_t result = taf_wlanAp_Restart(apRef);
    LE_TEST_OK(LE_OK == result, "taf_wlanAp_Restart result: %d", result);
    return result;
}

le_result_t WlanApTestGetStatus()
{
    LE_TEST_INFO("Getting WLAN AP Status");
    taf_wlanAp_WlanAPRef_t apRef = wlanApGetRef();
    if (apRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get AP reference for getting status.");
        return LE_FAULT;
    }

    taf_wlanAp_WlanAPStatus_t status = { 0, { 0 }, { 0 }, { 0 }, { 0 } };
    le_result_t result = taf_wlanAp_GetStatus(apRef, &status);
    LE_TEST_OK(LE_OK == result, "taf_wlanAp_GetStatus result: %d", result);
    if (LE_OK == result)
    {
        LE_TEST_INFO("AP Status retrieved successfully.");
        LE_TEST_INFO("AP Enabled    : %s", ((status.bEnabled) ? "Yes" : "No"));
        if (status.bEnabled) {
            LE_TEST_INFO("Interface Name: %s", status.IntfName);
            LE_TEST_INFO("IPv4 Address  : %s", status.IPv4Address);
            LE_TEST_INFO("MAC Address   : %s", status.MACAddress);
        }

        std::cout << "AP Enabled    : " << (status.bEnabled ? "Yes" : "No") << std::endl;
        if (status.bEnabled)
        {
            std::cout << "Interface Name: " << status.IntfName << std::endl;
            std::cout << "IPv4 Address  : " << status.IPv4Address << std::endl;
            std::cout << "MAC Address   : " << status.MACAddress << std::endl;
        }
    }
    return result;
}

le_result_t WlanApTestGetConfig()
{
    LE_TEST_INFO("Getting WLAN AP Configuration");
    taf_wlanAp_WlanAPRef_t apRef = wlanApGetRef();
    if (apRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get AP reference for getting configuration.");
        return LE_FAULT;
    }

    taf_wlanAp_WlanAPConfig_t config;
    le_result_t result = taf_wlanAp_GetConfig(apRef, &config);
    LE_TEST_OK(LE_OK == result, "taf_wlanAp_GetConfig result: %d", result);
    if (LE_OK == result)
    {
        LE_TEST_INFO("AP Configuration retrieved successfully.");
        LE_TEST_INFO("SSID   : %s", config.SSID);
        LE_TEST_INFO("Visible: %s", ((config.bSSIDVisible) ? "Yes" : "No"));

        std::cout << "SSID   : " << config.SSID << std::endl;
        std::cout << "Visible: " << (config.bSSIDVisible ? "Yes" : "No") << std::endl;
    }
    return result;
}

le_result_t WlanApTestSetConfig()
{
    LE_TEST_INFO("Setting WLAN AP Configuration");
    taf_wlanAp_WlanAPRef_t apRef = wlanApGetRef();
    if (apRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get AP reference for setting configuration.");
        return LE_FAULT;
    }

    taf_wlanAp_WlanAPConfig_t config;
    std::string ssid;
    int bSSIDVisible_int;

    std::cout << "Enter new SSID: ";
    std::cin.clear();
    std::getline(std::cin, ssid);
    le_utf8_Copy(config.SSID, ssid.c_str(), TAF_WLAN_MAX_SSID_LENGTH, NULL);

    std::cout << "Is SSID visible? (1 for Yes, 0 for No): ";
    std::cin.clear();
    std::cin >> bSSIDVisible_int;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    config.bSSIDVisible = (bSSIDVisible_int == 1);

    le_result_t result = taf_wlanAp_SetConfig(apRef, &config);
    LE_TEST_OK(LE_OK == result, "taf_wlanAp_SetConfig result: %d", result);
    return result;
}

static const char* wlanApGetSecModeStr(taf_wlan_SecurityMode_t SecMode) {
    switch (SecMode) {
        case TAF_WLAN_SEC_MODE_UNKNOWN: return "TAF_WLAN_SEC_MODE_UNKNOWN";
        case TAF_WLAN_SEC_MODE_OPEN:    return "TAF_WLAN_SEC_MODE_OPEN";
        case TAF_WLAN_SEC_MODE_WEP:     return "TAF_WLAN_SEC_MODE_WEP";
        case TAF_WLAN_SEC_MODE_WPA:     return "TAF_WLAN_SEC_MODE_WPA";
        case TAF_WLAN_SEC_MODE_WPA2:    return "TAF_WLAN_SEC_MODE_WPA2";
        case TAF_WLAN_SEC_MODE_WPA3:    return "TAF_WLAN_SEC_MODE_WPA3";
        default:                        return "*ERR* Unsupported SecMode";
    }
}

static const char* wlanApGetAuthMethodStr(taf_wlan_SecurityAuthMethod_t AuthMethod) {
    switch (AuthMethod) {
        case TAF_WLAN_SEC_AUTH_METHOD_UNKNOWN:  return "TAF_WLAN_SEC_AUTH_METHOD_UNKNOWN";
        case TAF_WLAN_SEC_AUTH_METHOD_NONE:     return "TAF_WLAN_SEC_AUTH_METHOD_NONE";
        case TAF_WLAN_SEC_AUTH_METHOD_PSK:      return "TAF_WLAN_SEC_AUTH_METHOD_PSK";
        case TAF_WLAN_SEC_AUTH_METHOD_EAP_SIM:  return "TAF_WLAN_SEC_AUTH_METHOD_EAP_SIM";
        case TAF_WLAN_SEC_AUTH_METHOD_EAP_AKA:  return "TAF_WLAN_SEC_AUTH_METHOD_EAP_AKA";
        case TAF_WLAN_SEC_AUTH_METHOD_EAP_LEAP: return "TAF_WLAN_SEC_AUTH_METHOD_EAP_LEAP";
        case TAF_WLAN_SEC_AUTH_METHOD_EAP_TLS:  return "TAF_WLAN_SEC_AUTH_METHOD_EAP_TLS";
        case TAF_WLAN_SEC_AUTH_METHOD_EAP_TTLS: return "TAF_WLAN_SEC_AUTH_METHOD_EAP_TTLS";
        case TAF_WLAN_SEC_AUTH_METHOD_EAP_PEAP: return "TAF_WLAN_SEC_AUTH_METHOD_EAP_PEAP";
        case TAF_WLAN_SEC_AUTH_METHOD_EAP_FAST: return "TAF_WLAN_SEC_AUTH_METHOD_EAP_FAST";
        case TAF_WLAN_SEC_AUTH_METHOD_EAP_PSK:  return "TAF_WLAN_SEC_AUTH_METHOD_EAP_PSK";
        case TAF_WLAN_SEC_AUTH_METHOD_SAE:      return "TAF_WLAN_SEC_AUTH_METHOD_SAE";
        default:                                return "*ERR* Unsupported AuthMethod";
    }
}

static const char* wlanApGetSecEncryptMethodStr(taf_wlan_SecurityEncryptionMethod_t SecEncryptMethod) {
    switch (SecEncryptMethod) {
        case TAF_WLAN_SEC_ENCRYPT_METHOD_UNKNOWN: return "TAF_WLAN_SEC_ENCRYPT_METHOD_UNKNOWN";
        case TAF_WLAN_SEC_ENCRYPT_METHOD_RC4:     return "TAF_WLAN_SEC_ENCRYPT_METHOD_RC4";
        case TAF_WLAN_SEC_ENCRYPT_METHOD_TKIP:    return "TAF_WLAN_SEC_ENCRYPT_METHOD_TKIP";
        case TAF_WLAN_SEC_ENCRYPT_METHOD_AES:     return "TAF_WLAN_SEC_ENCRYPT_METHOD_AES";
        case TAF_WLAN_SEC_ENCRYPT_METHOD_GCMP:    return "TAF_WLAN_SEC_ENCRYPT_METHOD_GCMP";
        default:                                  return "*ERR* Unsupported SecEncryptMethod";
    }
}

le_result_t WlanApTestGetSecurityConfig()
{
    LE_TEST_INFO("Getting WLAN AP Security Configuration");
    taf_wlanAp_WlanAPRef_t apRef = wlanApGetRef();
    if (apRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get AP reference for getting security configuration.");
        return LE_FAULT;
    }

    taf_wlanAp_WlanAPSecurityConfig_t secConfig;
    le_result_t result = taf_wlanAp_GetSecurityConfig(apRef, &secConfig);
    LE_TEST_OK(LE_OK == result, "taf_wlanAp_SetConfig result: %d", result);
    if (LE_OK == result)
    {
        LE_TEST_INFO("AP Security Configuration retrieved successfully.");
        LE_TEST_INFO("Sec Mode      : %s(%d)", wlanApGetSecModeStr(secConfig.SecMode),
                                                                            secConfig.SecMode);
        LE_TEST_INFO("Auth Method   : %s(%d)", wlanApGetAuthMethodStr(secConfig.SecAuthMethod),
                                                                        secConfig.SecAuthMethod);
        LE_TEST_INFO("Encrypt Method: %s(%d)",
            wlanApGetSecEncryptMethodStr(secConfig.SecEncryptMethod), secConfig.SecEncryptMethod);
        LE_TEST_INFO("Passphrase    : %s", secConfig.PassPhrase);

        std::cout << "Sec Mode      : " << wlanApGetSecModeStr(secConfig.SecMode)
                                                    << "(" << secConfig.SecMode << ")" << std::endl;
        std::cout << "Auth Method   : " << wlanApGetAuthMethodStr(secConfig.SecAuthMethod)
                                              << "(" << secConfig.SecAuthMethod << ")" << std::endl;
        std::cout << "Encrypt Method: " << wlanApGetSecEncryptMethodStr(secConfig.SecEncryptMethod)
                                           << "(" << secConfig.SecEncryptMethod << ")" << std::endl;
        std::cout << "Passphrase    : " << secConfig.PassPhrase << std::endl;
    }
    return result;
}

le_result_t WlanApTestSetSecurityConfig()
{
    LE_TEST_INFO("Setting WLAN AP Security Configuration");
    taf_wlanAp_WlanAPRef_t apRef = wlanApGetRef();
    if (apRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get AP reference for setting security configuration.");
        return LE_FAULT;
    }

    taf_wlanAp_WlanAPSecurityConfig_t SecConfig;
    int secMode_int, authMethod_int, encryptMethod_int;
    std::string passPhrase;

    std::cout << "Enter Security Mode (0:UNKNOWN, 1:OPEN, 2:WEP, 3:WPA, 4:WPA2, 5:WPA3): ";
    std::cin.clear();
    std::cin >> secMode_int;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    SecConfig.SecMode = (taf_wlan_SecurityMode_t)secMode_int;

    std::cout << "Enter Authentication Method (0:UNKNOWN, 1:NONE, 2:PSK, 3:EAP_SIM, 4:EAP_AKA, "
              << "5:EAP_LEAP, 6:EAP_TLS, 7:EAP_TTLS, 8:EAP_PEAP, 9:EAP_FAST, 10:EAP_PSK, 11:SAE): ";
    std::cin.clear();
    std::cin >> authMethod_int;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    SecConfig.SecAuthMethod = (taf_wlan_SecurityAuthMethod_t)authMethod_int;

    std::cout << "Enter Encryption Method (0:UNKNOWN, 1:RC4, 2:TKIP, 3:AES, 4:GCMP): ";
    std::cin.clear();
    std::cin >> encryptMethod_int;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    SecConfig.SecEncryptMethod = (taf_wlan_SecurityEncryptionMethod_t)encryptMethod_int;

    std::cout << "Enter Passphrase: ";
    std::cin.clear();
    std::getline(std::cin, passPhrase);
    le_utf8_Copy(SecConfig.PassPhrase, passPhrase.c_str(), TAF_WLAN_MAX_PASSPHRASE_LENGTH, NULL);

    le_result_t result = taf_wlanAp_SetSecurityConfig(apRef, &SecConfig);
    LE_TEST_OK(LE_OK == result, "taf_wlanAp_SetSecurityConfig result: %d", result);
    return result;
}

le_result_t WlanApTestGetConnectedDevicesList()
{
    LE_TEST_INFO("Getting WLAN AP Connected Devices List");
    taf_wlanAp_WlanAPRef_t apRef = wlanApGetRef();
    if (apRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get AP reference for getting connected devices list.");
        return LE_FAULT;
    }

    taf_wlanAp_WlanAPConnectedDeviceInfo_t DevInfo[TAF_WLANAP_MAX_CONNECTED_DEVICES];
    uint16_t numDevices = 0;
    size_t DevInfoSize = TAF_WLANAP_MAX_CONNECTED_DEVICES;
    memset(DevInfo, 0,
        (sizeof(taf_wlanAp_WlanAPConnectedDeviceInfo_t) * TAF_WLANAP_MAX_CONNECTED_DEVICES));

    le_result_t result = taf_wlanAp_GetConnectedDevices(apRef, &numDevices, DevInfo, &DevInfoSize);
    LE_TEST_OK(LE_OK == result, "taf_wlanAp_GetConnectedDevices result: %d", result);
    if (LE_OK == result)
    {
        LE_TEST_INFO("Connected Devices List retrieved successfully.");
        LE_TEST_INFO("Number of devices connected             : %d", numDevices);
        LE_TEST_INFO("Number of device info elements populated: %zu", DevInfoSize);
        for (size_t i = 0; i < DevInfoSize; i++) {
            LE_TEST_INFO("Device : %zu", (i + 1));
            LE_TEST_INFO("   Name        : %s", DevInfo[i].Name);
            LE_TEST_INFO("   MACAddress  : %s", DevInfo[i].MACAddress);
            LE_TEST_INFO("   IPv4Address : %s", DevInfo[i].IPv4Address);
        }

        std::cout << "Number of devices connected             : " << numDevices << std::endl;
        std::cout << "Number of device info elements populated: " << DevInfoSize << std::endl;
        for (size_t i = 0; i < DevInfoSize; i++)
        {
            std::cout << "Device : " << (i + 1) << std::endl;
            std::cout << "   Name        : " << DevInfo[i].Name << std::endl;
            std::cout << "   MACAddress  : " << DevInfo[i].MACAddress << std::endl;
            std::cout << "   IPv4Address : " << DevInfo[i].IPv4Address << std::endl;
        }
    }
    return result;
}
