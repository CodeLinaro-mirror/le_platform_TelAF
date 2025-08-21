/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       wlanStaInteractive.cpp
 * @brief      WLAN STA interactive tests.
 */

#include <map>
#include "wlanInteractiveTest.hpp"

// Global variable to hold the event handler reference
static std::map<taf_wlan_STAid_t, taf_wlanSta_EventHandlerRef_t>
    wlanStaEventHandlerMap =
    {
        {TAF_WLAN_STA_ID1, nullptr},
    };

// Global variable to hold the event handler contexts
static int wlanStaEventContexts[TAF_WLAN_MAX_NUM_STA + 1] = {-1, 1};

// Connected AP signal strength handler references. 2 are supported. 3 is for testing.
static std::map<int, taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t>
    wlanStaConnectedApSigStrengthHdlRef =
    {
        {1, nullptr},
        {2, nullptr},
        {3, nullptr},
    };
static int wlanStaConnectedApSigStrengthContexts[4] = {-1,1,2,3};

static taf_wlanSta_WlanSTARef_t wlanStaGetRef()
{
    int intStaId = 1;
    char interfaceStr[24] = {0};

    std::cout << "Enter STA Id (1): ";
    std::cin.clear();
    std::cin >> intStaId;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (1 != intStaId)
    {
        LE_TEST_INFO("ERR: Only STA id 1 is supported.");
        return nullptr;
    }

    std::cout << "Enter STA interface:  ";
    std::cin.getline(interfaceStr, 24);

    LE_TEST_INFO("STA ID  : %d", intStaId);
    LE_TEST_INFO("STA Intf: %s", interfaceStr);

    taf_wlan_STAid_t staId = static_cast<taf_wlan_STAid_t>(intStaId);

    taf_wlanSta_WlanSTARef_t ref = taf_wlanSta_GetWlanSTA((taf_wlan_STAid_t)staId, interfaceStr);
    LE_TEST_OK(nullptr != ref, "STA reference should be valid.");
    return ref;
}

static const char *wlanStaGetStateStr(taf_wlanSta_State_t State)
{
    switch (State)
    {
    case TAF_WLANSTA_STATE_UNKNOWN:
        return "TAF_WLANSTA_STATE_UNKNOWN";
    case TAF_WLANSTA_STATE_CONNECTING:
        return "TAF_WLANSTA_STATE_CONNECTING";
    case TAF_WLANSTA_STATE_CONNECTED:
        return "TAF_WLANSTA_STATE_CONNECTED";
    case TAF_WLANSTA_STATE_DISCONNECTED:
        return "TAF_WLANSTA_STATE_DISCONNECTED";
    case TAF_WLANSTA_STATE_ASSOCIATION_FAILED:
        return "TAF_WLANSTA_STATE_ASSOCIATION_FAILED";
    case TAF_WLANSTA_STATE_IP_ASSIGNMENT_FAILED:
        return "TAF_WLANSTA_STATE_IP_ASSIGNMENT_FAILED";
    case TAF_WLANSTA_STATE_SCAN_STARTED:
        return "TAF_WLANSTA_STATE_SCAN_STARTED";
    case TAF_WLANSTA_STATE_SCAN_COMPLETED:
        return "TAF_WLANSTA_STATE_SCAN_COMPLETED";
    case TAF_WLANSTA_STATE_SCAN_FAILED:
        return "TAF_WLANSTA_STATE_SCAN_FAILED";
    default:
        return "Unsupported State";
    }
}

static void StationEventHandler(taf_wlanSta_WlanSTARef_t wlanSTARef,
                                taf_wlanSta_State_t staState,
                                void *contextPtr)
{
    LE_UNUSED(wlanSTARef);

    int intStaId = -1;
    if (contextPtr)
    {
        intStaId = *(static_cast<int *>(contextPtr));
    }

    std::cout << "WLAN STA(" << intStaId << ") Event: " << wlanStaGetStateStr(staState)
                                                                                    << std::endl;
    LE_TEST_INFO("WLAN STA(%d) Event: %s (%d)", intStaId, wlanStaGetStateStr(staState), staState);
}

le_result_t WlanStaTestAddEventCb()
{
    int intStaId = 1;
    char interfaceStr[24] = {0};

    std::cout << "Enter STA Id (1): ";
    std::cin.clear();
    std::cin >> intStaId;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (1 != intStaId)
    {
        LE_TEST_INFO("ERR: Only STA id 1 is supported.");
        return LE_FAULT;
    }

    std::cout << "Enter STA interface:  ";
    std::cin.getline(interfaceStr, 24);

    LE_TEST_INFO("STA ID  : %d", intStaId);
    LE_TEST_INFO("STA Intf: %s", interfaceStr);

    taf_wlan_STAid_t staId = static_cast<taf_wlan_STAid_t>(intStaId);

    taf_wlanSta_WlanSTARef_t staRef = taf_wlanSta_GetWlanSTA((taf_wlan_STAid_t)staId, interfaceStr);
    if (nullptr == staRef)
    {
        LE_TEST_INFO("ERR: Unable to get STA ref");
        return LE_FAULT;
    }

    // Check if handler is already added
    if (wlanStaEventHandlerMap[staId] != nullptr)
    {
        LE_TEST_INFO("WLAN STA Event Callback is already registered.");
        return LE_DUPLICATE;
    }

    wlanStaEventHandlerMap[staId] = taf_wlanSta_AddEventHandler(staRef,
                                            StationEventHandler, &wlanStaEventContexts[staId]);
    LE_TEST_OK(nullptr != wlanStaEventHandlerMap[staId], "taf_wlanSta_AddEventHandler");
    if (wlanStaEventHandlerMap[staId])
    {
        return LE_OK;
    }
    return LE_FAULT;
}

le_result_t WlanStaTestRemoveEventCb()
{
    int intStaId = 1;

    std::cout << "Enter STA Id (1): ";
    std::cin.clear();
    std::cin >> intStaId;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (1 != intStaId)
    {
        LE_TEST_INFO("ERR: Only STA id 1 is supported.");
        return LE_FAULT;
    }

    taf_wlan_STAid_t staId = static_cast<taf_wlan_STAid_t>(intStaId);
    if (wlanStaEventHandlerMap[staId] != nullptr)
    {
        taf_wlanSta_RemoveEventHandler(wlanStaEventHandlerMap[staId]);
        LE_TEST_OK(true, "taf_wlanSta_RemoveEventHandler");
        wlanStaEventHandlerMap[staId] = nullptr;
    }
    else
    {
        LE_TEST_INFO("ERR: No device connection event handler to remove.");
        return LE_NOT_FOUND;
    }
    return LE_OK;
}

le_result_t WlanStaTestStart()
{
    taf_wlanSta_WlanSTARef_t staRef = wlanStaGetRef();
    if (staRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get STA reference.");
        return LE_FAULT;
    }

    le_result_t result = taf_wlanSta_Start(staRef);
    LE_TEST_OK(LE_OK == result, "taf_wlanSta_Start result: %d", result);
    return result;
}

le_result_t WlanStaTestStop()
{
    taf_wlanSta_WlanSTARef_t staRef = wlanStaGetRef();
    if (staRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get STA reference.");
        return LE_FAULT;
    }

    le_result_t result = taf_wlanSta_Stop(staRef);
    LE_TEST_OK(LE_OK == result, "taf_wlanSta_Stop result: %d", result);
    return result;
}

le_result_t WlanStaTestRestart()
{
    taf_wlanSta_WlanSTARef_t staRef = wlanStaGetRef();
    if (staRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get STA reference.");
        return LE_FAULT;
    }

    le_result_t result = taf_wlanSta_Restart(staRef);
    LE_TEST_OK(LE_OK == result, "taf_wlanSta_Restart result: %d", result);
    return result;
}

le_result_t WlanStaTestGetStatus()
{
    taf_wlanSta_WlanSTARef_t staRef = wlanStaGetRef();
    if (staRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get STA reference.");
        return LE_FAULT;
    }

    taf_wlanSta_State_t state;
    char intfName[TAF_NET_INTERFACE_NAME_MAX_LEN] = { 0 };
    char ipv4Address[TAF_NET_IPV4_ADDR_MAX_LEN] = { 0 };
    char ipv6Address[TAF_NET_IPV6_ADDR_MAX_LEN] = { 0 };
    char macAddress[TAF_NET_MAC_ADDR_MAX_LEN] = { 0 };

    le_result_t result = taf_wlanSta_GetStatus(staRef, &state, intfName,
        TAF_NET_INTERFACE_NAME_MAX_LEN, ipv4Address, TAF_NET_IPV4_ADDR_MAX_LEN, ipv6Address,
        TAF_NET_IPV6_ADDR_MAX_LEN, macAddress, TAF_NET_MAC_ADDR_MAX_LEN);

    LE_TEST_OK(LE_OK == result, "taf_wlanSta_GetStatus result: %d", result);

    if (LE_OK == result )
    {
        LE_TEST_INFO("State          : %s (%d)", wlanStaGetStateStr(state), state);
        LE_TEST_INFO("Interface Name : %s", intfName);
        LE_TEST_INFO("IPv4 Address   : %s", ipv4Address);
        LE_TEST_INFO("IPv6 Address   : %s", ipv6Address);
        LE_TEST_INFO("MAC Address    : %s", macAddress);

        std::cout << "State          : " << wlanStaGetStateStr(state) << " (" << state << ")"
                                                                                    << std::endl;
        std::cout << "Interface Name : " << intfName << std::endl;
        std::cout << "IPv4 Address   : " << ipv4Address << std::endl;
        std::cout << "IPv6 Address   : " << ipv6Address << std::endl;
        std::cout << "MAC Address    : " << macAddress << std::endl;
    }
    return result;
}

static const char* wlanStaGetModeStr(taf_wlanSta_Mode_t Mode) {
    switch (Mode) {
        case TAF_WLANSTA_MODE_UNKNOWN:
            return "TAF_WLANSTA_MODE_UNKNOWN";
        case TAF_WLANSTA_MODE_ROUTER:
            return "TAF_WLANSTA_MODE_ROUTER";
        case TAF_WLANSTA_MODE_BRIDGE:
            return "TAF_WLANSTA_MODE_BRIDGE";
        default:
            return "Unsupported Mode";
    }
}

le_result_t WlanStaTestGetMode()
{
    taf_wlanSta_WlanSTARef_t staRef = wlanStaGetRef();
    if (staRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get STA reference.");
        return LE_FAULT;
    }

    taf_wlanSta_Mode_t mode;
    le_result_t result = taf_wlanSta_GetMode(staRef, &mode);

    LE_TEST_OK(LE_OK == result, "taf_wlanSta_GetMode result: %d", result);
    if (LE_OK == result)
    {
        LE_TEST_INFO("Mode: %s (%d)", wlanStaGetModeStr(mode), mode);
        std::cout << "Mode: " << wlanStaGetModeStr(mode) << " (" << mode << ")" << std::endl;
    }
    return result;
}

le_result_t WlanStaTestSetMode()
{
    taf_wlanSta_WlanSTARef_t staRef = wlanStaGetRef();
    if (staRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get STA reference.");
        return LE_FAULT;
    }

    int modeVal;
    std::cout << "Enter STA mode (1 for ROUTER, 2 for BRIDGE): ";
    std::cin.clear();
    std::cin >> modeVal;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_wlanSta_Mode_t mode = static_cast<taf_wlanSta_Mode_t>(modeVal);

    if (mode != TAF_WLANSTA_MODE_ROUTER && mode != TAF_WLANSTA_MODE_BRIDGE) {
        LE_TEST_INFO("ERR: Invalid StaMode value entered.");
        return LE_BAD_PARAMETER;
    }

    le_result_t result = taf_wlanSta_SetMode(staRef, mode);
    LE_TEST_OK(LE_OK == result, "taf_wlanSta_SetMode result: %d", result);

    return result;
}

static const char* wlanStaGetIPTypeStr(taf_wlanSta_IPType_t IPType) {
    switch (IPType) {
        case TAF_WLANSTA_IPTYPE_UNKNOWN:
            return "TAF_WLANSTA_IPTYPE_UNKNOWN";
        case TAF_WLANSTA_IPTYPE_DYNAMIC:
            return "TAF_WLANSTA_IPTYPE_DYNAMIC";
        case TAF_WLANSTA_IPTYPE_STATIC:
            return "TAF_WLANSTA_IPTYPE_STATIC";
        default:
            return "Unsupported IPType";
    }
}

le_result_t WlanStaTestGetIpConfig()
{
    taf_wlanSta_WlanSTARef_t staRef = wlanStaGetRef();
    if (staRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get STA reference.");
        return LE_FAULT;
    }

    taf_wlanSta_IPType_t ipType;
    taf_wlanSta_IPConfig_t staticIpConfig = { { 0 }, { 0 }, { 0 }, { 0 } };
    le_result_t result = taf_wlanSta_GetIPConfig(staRef, &ipType, &staticIpConfig);

    LE_TEST_OK(LE_OK == result, "taf_wlanSta_GetIPConfig result: %d", result);
    if (LE_OK == result)
    {
        LE_TEST_INFO("IPType       : %s (%d)", wlanStaGetIPTypeStr(ipType), ipType);
        LE_TEST_INFO("IPv4 Address : %s", staticIpConfig.IPv4Addr);
        LE_TEST_INFO("GW Address   : %s", staticIpConfig.GWAddr);
        LE_TEST_INFO("DNS Address  : %s", staticIpConfig.DNSAddr);
        LE_TEST_INFO("Net Mask     : %s", staticIpConfig.NetMask);

        std::cout << "IPType       : " << wlanStaGetIPTypeStr(ipType) << " (" << ipType << ")"
                                                                                    << std::endl;
        std::cout << "IPv4 Address : " << staticIpConfig.IPv4Addr << std::endl;
        std::cout << "GW Address   : " << staticIpConfig.GWAddr << std::endl;
        std::cout << "DNS Address  : " << staticIpConfig.DNSAddr << std::endl;
        std::cout << "Net Mask     : " << staticIpConfig.NetMask << std::endl;
    }
    return result;
}

le_result_t WlanStaTestSetIpConfig()
{
    taf_wlanSta_WlanSTARef_t staRef = wlanStaGetRef();
    if (staRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get STA reference.");
        return LE_FAULT;
    }

    int ipTypeVal;
    std::cout << "Enter IP Type (1 for DYNAMIC, 2 for STATIC): ";
    std::cin.clear();
    std::cin >> ipTypeVal;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    taf_wlanSta_IPType_t ipType = static_cast<taf_wlanSta_IPType_t>(ipTypeVal);
    taf_wlanSta_IPConfig_t ipConfig;
    memset(&ipConfig, 0, sizeof(ipConfig));

    if (ipType == TAF_WLANSTA_IPTYPE_STATIC) {
        std::cout << "Enter IPv4 Address: ";
        std::cin.getline(ipConfig.IPv4Addr, TAF_NET_IPV4_ADDR_MAX_LEN);
        std::cout << "Enter Gateway Address: ";
        std::cin.getline(ipConfig.GWAddr, TAF_NET_IPV4_ADDR_MAX_LEN);
        std::cout << "Enter DNS Address: ";
        std::cin.getline(ipConfig.DNSAddr, TAF_NET_IPV4_ADDR_MAX_LEN);
        std::cout << "Enter Subnet Mask: ";
        std::cin.getline(ipConfig.NetMask, TAF_NET_IPV4_ADDR_MAX_LEN);
    } else if (ipType != TAF_WLANSTA_IPTYPE_DYNAMIC) {
        LE_TEST_INFO("ERR: Invalid IPType value entered.");
        return LE_BAD_PARAMETER;
    }

    le_result_t result = taf_wlanSta_SetIPConfig(staRef, ipType, &ipConfig);
    LE_TEST_OK(LE_OK == result, "taf_wlanSta_SetIPConfig result: %d", result);

    return result;
}

le_result_t WlanStaTestDoApScan()
{
    taf_wlanSta_WlanSTARef_t staRef = wlanStaGetRef();
    if (staRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get STA reference.");
        return LE_FAULT;
    }

    le_result_t result = taf_wlanSta_DoAPScan(staRef);
    LE_TEST_OK(LE_OK == result, "taf_wlanSta_DoAPScan result: %d", result);

    return result;
}

// Helper function to get the service set type string
static const char *getServiceSetType(taf_wlan_ServiceSet_t ss)
{
    switch (ss)
    {
    case TAF_WLAN_SS_UNKNOWN:
        return "UNKNOWN";
    case TAF_WLAN_SS_BASIC:
        return "BSS";
    case TAF_WLAN_SS_EXTENDED:
        return "ESS";
    default:
        return "UNKNOWN";
    }
}

// Helper function to get the security mode string
static const char *getSecurityMode(taf_wlan_SecurityMode_t mode)
{
    switch (mode)
    {
    case TAF_WLAN_SEC_MODE_UNKNOWN:
        return "UNKNOWN";
    case TAF_WLAN_SEC_MODE_OPEN:
        return "OPEN";
    case TAF_WLAN_SEC_MODE_WEP:
        return "WEP";
    case TAF_WLAN_SEC_MODE_WPA:
        return "WPA";
    case TAF_WLAN_SEC_MODE_WPA2:
        return "WPA2";
    case TAF_WLAN_SEC_MODE_WPA3:
        return "WPA3";
    default:
        return "UNKNOWN";
    }
}

// Helper function to get the security authentication method string
static const char *getSecurityAuthMethod(taf_wlan_SecurityAuthMethod_t method)
{
    switch (method)
    {
    case TAF_WLAN_SEC_AUTH_METHOD_UNKNOWN:
        return "UNKNOWN";
    case TAF_WLAN_SEC_AUTH_METHOD_NONE:
        return "NONE";
    case TAF_WLAN_SEC_AUTH_METHOD_PSK:
        return "PSK";
    case TAF_WLAN_SEC_AUTH_METHOD_EAP_SIM:
        return "EAP-SIM";
    case TAF_WLAN_SEC_AUTH_METHOD_EAP_AKA:
        return "EAP-AKA";
    case TAF_WLAN_SEC_AUTH_METHOD_EAP_LEAP:
        return "EAP-LEAP";
    case TAF_WLAN_SEC_AUTH_METHOD_EAP_TLS:
        return "EAP-TLS";
    case TAF_WLAN_SEC_AUTH_METHOD_EAP_TTLS:
        return "EAP-TTLS";
    case TAF_WLAN_SEC_AUTH_METHOD_EAP_PEAP:
        return "EAP-PEAP";
    case TAF_WLAN_SEC_AUTH_METHOD_EAP_FAST:
        return "EAP-FAST";
    case TAF_WLAN_SEC_AUTH_METHOD_EAP_PSK:
        return "EAP-PSK";
    case TAF_WLAN_SEC_AUTH_METHOD_SAE:
        return "SAE";
    default:
        return "UNKNOWN";
    }
}

// Helper function to get the security encryption method string
static const char *getSecurityEncryptionMethod(taf_wlan_SecurityEncryptionMethod_t method)
{
    switch (method)
    {
    case TAF_WLAN_SEC_ENCRYPT_METHOD_UNKNOWN:
        return "UNKNOWN";
    case TAF_WLAN_SEC_ENCRYPT_METHOD_RC4:
        return "RC4";
    case TAF_WLAN_SEC_ENCRYPT_METHOD_TKIP:
        return "TKIP";
    case TAF_WLAN_SEC_ENCRYPT_METHOD_AES:
        return "AES";
    case TAF_WLAN_SEC_ENCRYPT_METHOD_GCMP:
        return "GCMP";
    default:
        return "UNKNOWN";
    }
}

static void printAPInfo(const taf_wlanSta_APInfo_t &apInfo)
{
    LE_TEST_INFO("  BSSID          : %s", apInfo.BSSID);
    LE_TEST_INFO("  SSID           : %s", apInfo.SSID);
    LE_TEST_INFO("  Signal Level   : %d dBm", apInfo.SignalLevel);
    LE_TEST_INFO("  Frequency      : %d MHz", apInfo.Frequency);
    LE_TEST_INFO("  Service Set    : %s", getServiceSetType(apInfo.SS));
    LE_TEST_INFO("  Sec Mode       : %s", getSecurityMode(apInfo.secMode));
    LE_TEST_INFO("  Auth Method    : %s", getSecurityAuthMethod(apInfo.secAuthMethod));
    LE_TEST_INFO("  Encrypt Method : %s", getSecurityEncryptionMethod(apInfo.secEncryptionMethod));
    LE_TEST_INFO("  WPS            : %s", (apInfo.WPSEnabled ? "ENABLED" : "DISABLED"));
    LE_TEST_INFO("  ---------------- ");

    std::cout << "  BSSID          : " << apInfo.BSSID << std::endl;
    std::cout << "  SSID           : " << apInfo.SSID << std::endl;
    std::cout << "  Signal Level   : " << apInfo.SignalLevel << " dBm" << std::endl;
    std::cout << "  Frequency      : " << apInfo.Frequency << " MHz" << std::endl;
    std::cout << "  Service Set    : " << getServiceSetType(apInfo.SS) << std::endl;
    std::cout << "  Sec Mode       : " << getSecurityMode(apInfo.secMode) << std::endl;
    std::cout << "  Auth Method    : " << getSecurityAuthMethod(apInfo.secAuthMethod) << std::endl;
    std::cout << "  Encrypt Method : " << getSecurityEncryptionMethod(apInfo.secEncryptionMethod)
                                                                                       << std::endl;
    std::cout << "  WPS            : " << (apInfo.WPSEnabled ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "  ---------------- " << std::endl;
}

le_result_t WlanStaTestGetApScanResults()
{
    taf_wlanSta_WlanSTARef_t staRef = wlanStaGetRef();
    if (staRef == nullptr)
    {
        LE_TEST_INFO("ERR: Failed to get STA reference.");
        return LE_FAULT;
    }

    uint16_t numScannedAPs = 0;
    size_t apInfoSize = TAF_WLANSTA_MAX_APSCAN_RESULT_NUM;
    taf_wlanSta_APInfo_t apInfo[TAF_WLANSTA_MAX_APSCAN_RESULT_NUM] = { 0 };

    le_result_t result = taf_wlanSta_GetAPScanResults(staRef, &numScannedAPs, apInfo, &apInfoSize);
    LE_TEST_OK(LE_OK == result, "taf_wlanSta_GetAPScanResults result: %d", result);
    if (LE_OK == result)
    {
        LE_TEST_INFO("\nNum APs available     : %d", numScannedAPs);
        LE_TEST_INFO("Num elements populated: %zu", apInfoSize);
        LE_TEST_INFO("");

        for (size_t i = 0; i < numScannedAPs; ++i)
        {
            LE_TEST_INFO("AP %zu:", i + 1);
            printAPInfo(apInfo[i]);
        }
    }
    return result;
}

le_result_t WlanStaTestSetWpa2Psk()
{
    taf_wlanSta_WlanSTARef_t staRef = wlanStaGetRef();
    if (!staRef) {
        LE_TEST_INFO("ERR: Failed to get STA reference.");
        return LE_FAULT;
    }

    constexpr uint8_t ssidSizeBytes = TAF_WLAN_MAX_SSID_LENGTH + 1;
    constexpr uint8_t pskSizeBytes  = TAF_WLAN_MAX_PASSPHRASE_LENGTH + 1;
    char ssid[ssidSizeBytes] = {0};
    char psk[pskSizeBytes] = {0};

    std::cout << "Enter SSID: ";
    std::cin.clear();
    std::cin.getline(ssid, ssidSizeBytes);

    if (0 == strlen(ssid))
    {
        LE_TEST_INFO("ERR: SSID cannot be empty.");
        return LE_BAD_PARAMETER;
    }

    std::cout << "Enter PSK (password): ";
    std::cin.clear();
    std::cin.getline(psk, pskSizeBytes);
    if (0 == strlen(psk))
    {
        LE_TEST_INFO("ERR: PSK cannot be empty for PSK authentication type.");
        return LE_BAD_PARAMETER;
    }

    taf_wlanSta_APInfo_t apInfoConnect;
    memset(&apInfoConnect, 0, sizeof(apInfoConnect));
    le_utf8_Copy(apInfoConnect.SSID, ssid, ssidSizeBytes, NULL);
    apInfoConnect.secMode       = TAF_WLAN_SEC_MODE_WPA2;
    apInfoConnect.secAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_PSK;

    LE_TEST_INFO("SSID: %s, PSK: %s", ssid, psk);
    le_result_t result = taf_wlanSta_SetWpa2Psk(staRef, &apInfoConnect, psk);
    LE_TEST_OK(LE_OK == result, "taf_wlanSta_SetWpa2Psk result: %d", result);

    return result;
}

le_result_t WlanStaTestConnect()
{
    taf_wlanSta_WlanSTARef_t staRef = wlanStaGetRef();
    if (!staRef)
    {
        LE_TEST_INFO("ERR: Failed to get STA reference.");
        return LE_FAULT;
    }

    constexpr uint8_t ssidSizeBytes = TAF_WLAN_MAX_SSID_LENGTH + 1;
    char ssid[ssidSizeBytes] = {0};

    std::cout << "Enter SSID: ";
    std::cin.clear();
    std::cin.getline(ssid, ssidSizeBytes);

    if (0 == strlen(ssid))
    {
        LE_TEST_INFO("ERR: SSID cannot be empty.");
        return LE_BAD_PARAMETER;
    }

    taf_wlanSta_APInfo_t apInfoConnect;
    memset(&apInfoConnect, 0, sizeof(apInfoConnect));
    le_utf8_Copy(apInfoConnect.SSID, ssid, ssidSizeBytes, NULL);
    apInfoConnect.secMode       = TAF_WLAN_SEC_MODE_WPA2;
    apInfoConnect.secAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_PSK;

    LE_TEST_INFO("WPA2-PSK for SSID: %s", ssid);
    le_result_t result = taf_wlanSta_Connect(staRef, &apInfoConnect);
    LE_TEST_OK(LE_OK == result, "taf_wlanSta_Connect result: %d", result);

    return result;
}

le_result_t WlanStaTestDisconnect()
{
    taf_wlanSta_WlanSTARef_t staRef = wlanStaGetRef();
    if (!staRef)
    {
        LE_TEST_INFO("ERR: Failed to get STA reference.");
        return LE_FAULT;
    }

    constexpr uint8_t ssidSizeBytes = TAF_WLAN_MAX_SSID_LENGTH + 1;
    char ssid[ssidSizeBytes] = {0};

    std::cout << "Enter SSID: ";
    std::cin.clear();
    std::cin.getline(ssid, ssidSizeBytes);

    if (0 == strlen(ssid))
    {
        LE_TEST_INFO("ERR: SSID cannot be empty.");
        return LE_BAD_PARAMETER;
    }

    taf_wlanSta_APInfo_t apInfoConnect;
    memset(&apInfoConnect, 0, sizeof(apInfoConnect));
    le_utf8_Copy(apInfoConnect.SSID, ssid, ssidSizeBytes, NULL);

    LE_TEST_INFO("SSID: %s", ssid);
    le_result_t result = taf_wlanSta_Disconnect(staRef, &apInfoConnect);
    LE_TEST_OK(LE_OK == result, "taf_wlanSta_Disconnect result: %d", result);

    return result;
}

le_result_t WlanStaTestGetConnectedApSignalStrength()
{
    taf_wlanSta_WlanSTARef_t staRef = wlanStaGetRef();
    if (!staRef)
    {
        LE_TEST_INFO("ERR: Failed to get STA reference.");
        return LE_FAULT;
    }
    int16_t sigStrength;
    le_result_t result = taf_wlanSta_GetConnectedApSignalStrength(staRef, &sigStrength);
    LE_TEST_OK(LE_OK == result, "taf_wlanSta_GetConnectedApSignalStrength result: %d", result);
    if (result == LE_OK)
    {
        LE_TEST_INFO("Signal strength of connected AP is %ddBm", sigStrength);
        std:: cout << "Signal strength of connected AP is "<< sigStrength << " dbm" << std::endl;
    }
    return result;
}

static void wlaStaConnectedApSignalStrengthHandler(
    taf_wlanSta_WlanSTARef_t staRef,
    int16_t signalStrength,
    void *contextPtr)
{
    LE_UNUSED(staRef);
    int intHandlerId = -1;
    if (contextPtr)
    {
        intHandlerId = *(static_cast<int *>(contextPtr));
    }
    LE_TEST_INFO("Id: %d, Signal Strength: %d dBm", intHandlerId, signalStrength);
    std::cout << "Id: " << intHandlerId << ", Signal Strength: " << signalStrength << " dBm"
                                                                                    << std::endl;
}

le_result_t WlanStaTestAddConnectedApSignalStrengthCb()
{
    int intStaId     = 1;
    int intHandlerId = 1;
    int intThreshold = -75;
    int intFrequency = 10;
    int intAverage   = 0;

    char interfaceStr[24] = {0};

    std::cout << "Enter STA Id (1): ";
    std::cin.clear();
    std::cin >> intStaId;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (1 != intStaId)
    {
        LE_TEST_INFO("ERR: Only STA id 1 is supported.");
        return LE_FAULT;
    }

    std::cout << "Enter STA interface:  ";
    std::cin.getline(interfaceStr, 24);

    LE_TEST_INFO("STA ID  : %d", intStaId);
    LE_TEST_INFO("STA Intf: %s", interfaceStr);

    taf_wlan_STAid_t staId = static_cast<taf_wlan_STAid_t>(intStaId);

    taf_wlanSta_WlanSTARef_t staRef = taf_wlanSta_GetWlanSTA((taf_wlan_STAid_t)staId, interfaceStr);
    if (nullptr == staRef)
    {
        LE_TEST_INFO("ERR: Unable to get STA ref");
        return LE_FAULT;
    }

    // Get the handler ID that is used internally.
    std::cout << "Max " << TAF_WLANSTA_MAX_CONNECTED_AP_SIGNAL_STRENGTH_CLIENTS
                        << " connected AP signal strength handlers are supported" << std::endl;
    std::cout << "Enter handler ID (1|2|3):";
    std::cin.clear();
    std::cin >> intHandlerId;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (1 != intHandlerId && 2 != intHandlerId && 3 != intHandlerId)
    {
        LE_TEST_INFO("ERR: Handler IDs can only be 1, 2 or 3.");
        return LE_FAULT;
    }

    // Threshold
    std::cout << "Enter signal threshold below which handler will be triggered(dBm): ";
    std::cin.clear();
    std::cin >> intThreshold;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // Frequency
    std::cout << "Enter frequency at which handler will be triggered(0 to 300): ";
    std::cin.clear();
    std::cin >> intFrequency;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (intFrequency< 0)
    {
        LE_TEST_INFO("ERR: Frequency cannot be negative.");
        return LE_FAULT;
    }

    // Average
    std::cout << "Should average over frequency be reported (0|1): ";
    std::cin.clear();
    std::cin >> intAverage;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (intAverage != 0 && intAverage != 1)
    {
        LE_TEST_INFO("ERR: Average should be 0 or 1");
        return LE_FAULT;
    }

    LE_TEST_INFO("Id: %d, Threshold: %d, Frequency: %d, Average: %d", intHandlerId, intThreshold,
                                                                          intFrequency, intAverage);
    taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef = nullptr;
    wlanStaConnectedApSigStrengthContexts[intHandlerId] = intHandlerId;
    handlerRef = taf_wlanSta_AddConnectedApSignalStrengthHandler(staRef, intThreshold, intFrequency,
                                            intAverage, wlaStaConnectedApSignalStrengthHandler,
                                            &wlanStaConnectedApSigStrengthContexts[intHandlerId]);
    LE_TEST_OK(nullptr != handlerRef, "taf_wlanSta_AddConnectedApSignalStrengthHandler");
    if (nullptr == handlerRef)
    {
        return LE_FAULT;
    }
    wlanStaConnectedApSigStrengthHdlRef[intHandlerId] = handlerRef;
    return LE_OK;
}

le_result_t WlanStaTestRemoveConnectedApSignalStrengthCb()
{
    int intHandlerId = 1;
    std::cout << "Enter handler ID (1|2|3):";
    std::cin.clear();
    std::cin >> intHandlerId;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (1 != intHandlerId && 2 != intHandlerId && 3 != intHandlerId)
    {
        LE_TEST_INFO("ERR: Handler IDs can only be 1, 2 or 3.");
        return LE_FAULT;
    }

    if (wlanStaConnectedApSigStrengthHdlRef[intHandlerId] != nullptr)
    {
        taf_wlanSta_RemoveConnectedApSignalStrengthHandler(
                                                wlanStaConnectedApSigStrengthHdlRef[intHandlerId]);
        LE_TEST_OK(true, "taf_wlanSta_RemoveEventHandler");
        wlanStaConnectedApSigStrengthHdlRef[intHandlerId] = nullptr;
    }
    else
    {
        LE_TEST_INFO("ERR: No connected AP signal strength handler to remove.");
        return LE_NOT_FOUND;
    }
    return LE_OK;
}
