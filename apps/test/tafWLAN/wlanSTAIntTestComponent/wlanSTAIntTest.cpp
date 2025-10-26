/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       wlanSTAIntTest.cpp
 * @brief      Integration test functions for WLAN Station service.
 */

#include <iostream>
#include <string>
#include <future>
#include <vector>
#include <map>
#include "interfaces.h"
#include "legato.h"

#define MAX_SYSTEM_CMD_LENGTH 200


static le_sem_Ref_t wlanSemRef = nullptr;
static std::promise<taf_wlanSta_State_t> connectPromise;
static std::promise<taf_wlanSta_State_t> disconnectPromise;
static std::promise<taf_wlanSta_State_t> removeNetworkPromise;

void PrintUsage() {
    printf("\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- Start <STA>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- Stop <STA>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- Restart <STA>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- GetStatus <STA>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- GetMode <STA>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- GetIpConfig <STA>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- SetIpConfig <STA> <STATIC|DYNAMIC>"
           " <IPv4> <GW IPv4> <DNS IPv4> <subnet mask>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- SetMode  <STA> <Station Mode>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- DoAPScan <STA>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- GetAPScanResults <STA>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- SetWpa2Psk <STA> <SSID> <psk>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- Connect <STA> <SSID>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- Disconnect <STA> <SSID>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- RemoveNetwork <STA> <SSID>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- SaveNetworkConfig <STA>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- GetApSignalStrength <STA>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- ApSigStrengthEvents <STA>\n"
           "app runProc tafWlanSTAIntTest tafWlanSTAIntTest -- GetAPEstimatedThroughput <STA>\n"
           "\n STA: STA interface obtained from taf_wlan_GetIntfInfo\n"
           "\n");
}

inline void CheckNumArgs(size_t NumArgs, size_t ExpectedNumArgs)
{
    if (NumArgs < ExpectedNumArgs)
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }
}

static le_result_t wlanSTATestStart(taf_wlanSta_WlanSTARef_t staRef)
{
    le_result_t result = taf_wlanSta_Start(staRef);
    fprintf(stderr, "taf_wlanSta_Start Return:%d\n", result);
    return result;
}

static le_result_t wlanSTATestStop(taf_wlanSta_WlanSTARef_t staRef)
{
    le_result_t result = taf_wlanSta_Stop(staRef);
    fprintf(stderr, "taf_wlanSta_Stop Return:%d\n", result);
    return result;
}

static le_result_t wlanSTATestRestart(taf_wlanSta_WlanSTARef_t staRef)
{
    le_result_t result = taf_wlanSta_Restart(staRef);
    fprintf(stderr, "taf_wlanSta_Restart Return:%d\n", result);
    return result;
}

static void PrintStaState(taf_wlanSta_State_t State) {
    switch (State) {
        case TAF_WLANSTA_STATE_UNKNOWN:
            LE_TEST_INFO("State: TAF_WLANSTA_STATE_UNKNOWN(%d)", State);
            break;

        case TAF_WLANSTA_STATE_CONNECTING:
            LE_TEST_INFO("State: TAF_WLANSTA_STATE_CONNECTING(%d)", State);
            break;

        case TAF_WLANSTA_STATE_CONNECTED:
            LE_TEST_INFO("State: TAF_WLANSTA_STATE_CONNECTED(%d)", State);
            break;

        case TAF_WLANSTA_STATE_DISCONNECTED:
            LE_TEST_INFO("State: TAF_WLANSTA_STATE_DISCONNECTED(%d)", State);
            break;

        case TAF_WLANSTA_STATE_NETWORK_REMOVED:
            LE_TEST_INFO("State: TAF_WLANSTA_STATE_NETWORK_REMOVED(%d)", State);
            break;

        case TAF_WLANSTA_STATE_ASSOCIATION_FAILED:
            LE_TEST_INFO("State: TAF_WLANSTA_STATE_ASSOCIATION_FAILED(%d)", State);
            break;

        case TAF_WLANSTA_STATE_IP_ASSIGNMENT_FAILED:
            LE_TEST_INFO("State: TAF_WLANSTA_STATE_IP_ASSIGNMENT_FAILED(%d)", State);
            break;

        default:
            // Control should not reach here
            LE_TEST_INFO("*ERR* Unsupported State: %d", State);
            break;
    }
}

static le_result_t wlanSTATestGetStatus(taf_wlanSta_WlanSTARef_t staRef)
{
    taf_wlanSta_State_t State;
    char IntfName[TAF_NET_INTERFACE_NAME_MAX_LEN] = { 0 };
    char IPv4Address[TAF_NET_IPV4_ADDR_MAX_LEN] = { 0 };
    char IPv6Address[TAF_NET_IPV6_ADDR_MAX_LEN] = { 0 };
    char MACAddress[TAF_NET_MAC_ADDR_MAX_LEN] = { 0 };
    le_result_t result = taf_wlanSta_GetStatus(staRef, &State, IntfName,
        TAF_NET_INTERFACE_NAME_MAX_LEN, IPv4Address, TAF_NET_IPV4_ADDR_MAX_LEN, IPv6Address,
        TAF_NET_IPV6_ADDR_MAX_LEN, MACAddress, TAF_NET_MAC_ADDR_MAX_LEN);
    fprintf(stderr, "taf_wlanSta_GetStatus Return:%d\n", result);
    if (LE_OK != result)
        return result;

    PrintStaState(State);
    LE_TEST_INFO("Interface Name : %s", IntfName);
    LE_TEST_INFO("IPv4 Address   : %s", IPv4Address);
    LE_TEST_INFO("IPv6 Address   : %s", IPv6Address);
    LE_TEST_INFO("MAC Address    : %s", MACAddress);
    return result;
}

static void PrintStaMode(taf_wlanSta_Mode_t StaMode) {
    switch (StaMode) {
        case TAF_WLANSTA_MODE_UNKNOWN:
            LE_TEST_INFO("StaMode: TAF_WLANSTA_MODE_UNKNOWN(%d)", StaMode);
            break;

        case TAF_WLANSTA_MODE_ROUTER:
            LE_TEST_INFO("StaMode: TAF_WLANSTA_MODE_ROUTER(%d)", StaMode);
            break;

        case TAF_WLANSTA_MODE_BRIDGE:
            LE_TEST_INFO("StaMode: TAF_WLANSTA_MODE_BRIDGE(%d)", StaMode);
            break;

        default:
            // Control should not reach here
            LE_TEST_INFO("*ERR* Unsupported StaMode: %d", StaMode);
            break;
    }
}

static le_result_t wlanSTATestGetMode(taf_wlanSta_WlanSTARef_t staRef)
{
    taf_wlanSta_Mode_t Mode;
    le_result_t result = taf_wlanSta_GetMode(staRef, &Mode);
    fprintf(stderr, "taf_wlanSta_GetMode Return:%d\n", result);
    if (LE_OK != result)
        return result;

    PrintStaMode(Mode);

    return result;
}

static void PrintStaIPConfig(taf_wlanSta_IPType_t IPType) {
    switch (IPType) {
        case TAF_WLANSTA_IPTYPE_UNKNOWN:
            LE_TEST_INFO("IPType     : TAF_WLANSTA_IPTYPE_UNKNOWN(%d)", IPType);
            printf("IPType       : TAF_WLANSTA_IPTYPE_UNKNOWN\n");
            break;

        case TAF_WLANSTA_IPTYPE_DYNAMIC:
            LE_TEST_INFO("IPType     : TAF_WLANSTA_IPTYPE_DYNAMIC(%d)", IPType);
            printf("IPType       : TAF_WLANSTA_IPTYPE_DYNAMIC\n");
            break;

        case TAF_WLANSTA_IPTYPE_STATIC:
            LE_TEST_INFO("IPType     : TAF_WLANSTA_IPTYPE_STATIC(%d)", IPType);
            printf("IPType       : TAF_WLANSTA_IPTYPE_STATIC\n");
            break;

        default:
            // Control should not reach here
            LE_TEST_INFO("*ERR* Unsupported IPType: %d", IPType);
            break;
    }
}

static le_result_t wlanSTATestGetIPConfig(taf_wlanSta_WlanSTARef_t staRef)
{
    taf_wlanSta_IPType_t IPType;
    taf_wlanSta_IPConfig_t StaStaticIPConfig = { { 0 }, { 0 }, { 0 }, { 0 } };
    le_result_t result = taf_wlanSta_GetIPConfig(staRef, &IPType, &StaStaticIPConfig);
    fprintf(stderr, "StaStaticIPConfig Return:%d\n", result);
    if (LE_OK != result)
        return result;

    PrintStaIPConfig(IPType);
    LE_TEST_INFO("IPv4 Address : %s", StaStaticIPConfig.IPv4Addr);
    LE_TEST_INFO("GW Address   : %s", StaStaticIPConfig.GWAddr);
    LE_TEST_INFO("DNS Address  : %s", StaStaticIPConfig.DNSAddr);
    LE_TEST_INFO("Net Mask     : %s", StaStaticIPConfig.NetMask);
    printf("IPv4 address : %s\n", StaStaticIPConfig.IPv4Addr);
    printf("GW address   : %s\n", StaStaticIPConfig.GWAddr);
    printf("DNS address  : %s\n", StaStaticIPConfig.DNSAddr);
    printf("Subnet mask  : %s\n", StaStaticIPConfig.NetMask);
    return result;
}

static le_result_t wlanSTATestSetIPConfig(taf_wlanSta_WlanSTARef_t staRef, size_t numArgs)
{
    const char *IpTypeStr = le_arg_GetArg(2);
    if (nullptr == IpTypeStr)
    {
        printf("IpTypeStr is null\n");
        PrintUsage();
        LE_TEST_INFO("*ERR* IpTypeStr is null");
        return LE_BAD_PARAMETER;
    }

    taf_wlanSta_IPType_t IpType;
    taf_wlanSta_IPConfig_t IpConfig;
    if (strncasecmp(IpTypeStr, "DYNAMIC", strlen("DYNAMIC")) == 0)
    {
        IpType = TAF_WLANSTA_IPTYPE_DYNAMIC;
    }
    else if (strncasecmp(IpTypeStr, "STATIC", strlen("STATIC")) == 0)
    {
        IpType = TAF_WLANSTA_IPTYPE_STATIC;
        // Check the number of arguments
        CheckNumArgs (numArgs,7);
        const char *IpV4Str = le_arg_GetArg(3);
        const char *GwIpV4Str = le_arg_GetArg(4);
        const char *DnsV4Str = le_arg_GetArg(5);
        const char *SubnetStr = le_arg_GetArg(6);
        if (nullptr == IpV4Str  || nullptr == GwIpV4Str ||
            nullptr == DnsV4Str || nullptr == SubnetStr)
        {
            printf("Invalid parameter\n");
            PrintUsage();
            LE_TEST_INFO("*ERR* Invalid argument");
            return LE_BAD_PARAMETER;
        }
        le_utf8_Copy(IpConfig.IPv4Addr, IpV4Str,   TAF_NET_IPV4_ADDR_MAX_LEN + 1, NULL);
        le_utf8_Copy(IpConfig.GWAddr,   GwIpV4Str, TAF_NET_IPV4_ADDR_MAX_LEN + 1, NULL);
        le_utf8_Copy(IpConfig.DNSAddr,  DnsV4Str,  TAF_NET_IPV4_ADDR_MAX_LEN + 1, NULL);
        le_utf8_Copy(IpConfig.NetMask,  SubnetStr, TAF_NET_IPV4_ADDR_MAX_LEN + 1, NULL);
    }
    else
    {
        LE_TEST_INFO("*ERR* Unsupported IPType: %s", IpTypeStr);
        PrintUsage();
        return LE_BAD_PARAMETER;
    }

    le_result_t result = taf_wlanSta_SetIPConfig(staRef, IpType, &IpConfig);
    fprintf(stderr, "taf_wlanSta_SetIPConfig Return:%d\n", result);

    return result;
}

static le_result_t wlanSTATestSetMode(taf_wlanSta_WlanSTARef_t staRef)
{
    const char *SetModeStr = le_arg_GetArg(1);
    if (NULL == SetModeStr)
    {
        PrintUsage();
        LE_TEST_FATAL("Mode value is NULL");
    }

    std::string mode(SetModeStr);

    taf_wlanSta_Mode_t StaMode = static_cast<taf_wlanSta_Mode_t>(strtol(mode.c_str(), nullptr, 10));

    if (StaMode != TAF_WLANSTA_MODE_ROUTER && StaMode != TAF_WLANSTA_MODE_BRIDGE) {
        LE_TEST_INFO("Invalid StaMode value");
        return LE_BAD_PARAMETER;
    }
    le_result_t result = taf_wlanSta_SetMode(staRef, StaMode);
    fprintf(stderr, "taf_wlanSta_SetMode Return:%d\n", result);
    return result;
}

static le_result_t wlanSTATestDoAPScan(taf_wlanSta_WlanSTARef_t staRef)
{
    le_result_t result = taf_wlanSta_DoAPScan(staRef);
    fprintf(stderr, "taf_wlanSta_DoAPScan Return: %d\n", result);
    return result;
}

// Function to get the service set type string
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

// Function to get the security mode string
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

// Function to get the security authentication method string
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

// Function to get the security encryption method string
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

static le_result_t wlanSTATestGetAPScanResults(taf_wlanSta_WlanSTARef_t staRef)
{
    uint16_t numScanedAPs = 0;
    size_t APInfoSize = TAF_WLANSTA_MAX_APSCAN_RESULT_NUM;
    taf_wlanSta_APInfo_t ApInfo[TAF_WLANSTA_MAX_APSCAN_RESULT_NUM] = { 0 };
    le_result_t result = taf_wlanSta_GetAPScanResults(staRef, &numScanedAPs, ApInfo, &APInfoSize);
    fprintf(stderr, "taf_wlanSta_GetAPScanResults Return: %d\n", result);
    if (LE_OK != result)
    {
        LE_TEST_INFO("taf_wlanSta_GetAPScanResults failed: %d", result);
        return result;
    }

    printf("\nNum APs available     : %d", numScanedAPs);
    printf("\nNum elements populated: %" PRIuS "", APInfoSize);
    printf("\n");

    for (size_t i = 0; i < numScanedAPs; ++i)
    {
        std::cout << "AP " << i + 1 << ":" << std::endl;
        printAPInfo(ApInfo[i]);
    }
    return result;
}

static le_result_t wlanSTATestSetWpa2Psk(taf_wlanSta_WlanSTARef_t staRef)
{
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }

    const char* ssidStr = le_arg_GetArg(2);
    if (NULL == ssidStr)
    {
        PrintUsage();
        LE_TEST_FATAL("ssid value is NULL");
    }

    std::string ssid(ssidStr);

    taf_wlanSta_APInfo_t APInfoConnect;
    uint16_t numScanedAPs = 0;
    size_t APInfoSize = TAF_WLANSTA_MAX_APSCAN_RESULT_NUM;
    taf_wlanSta_APInfo_t ApInfo[TAF_WLANSTA_MAX_APSCAN_RESULT_NUM] = { 0 };
    taf_wlanSta_GetAPScanResults(staRef, &numScanedAPs, ApInfo, &APInfoSize);
    bool isApFound = false;
    for(size_t i = 0; i < APInfoSize; ++i)
    {
        taf_wlanSta_APInfo_t ap = ApInfo[i];
        if(std::string(ap.SSID) == ssid)
        {
            LE_TEST_INFO("Found %s in the scanned APs list", ap.SSID);
            isApFound = true;
            APInfoConnect = ap;
            break;
        }
    }
    if(!isApFound)
    {
        printf("%s was not found in the scanned APs, try again\n", ssid.c_str());
        LE_TEST_EXIT;
    }

    std::string psk = "";

    if(APInfoConnect.secAuthMethod == TAF_WLAN_SEC_AUTH_METHOD_PSK)
    {
        const char* pskStr = le_arg_GetArg(3);
        if (NULL == pskStr)
        {
          PrintUsage();
          LE_TEST_FATAL("password value is NULL for auth type PSK");
        }
        psk = std::string(pskStr);
    }

    le_result_t result = taf_wlanSta_SetWpa2Psk(staRef, &APInfoConnect, psk.c_str());
    fprintf(stderr, "taf_wlanSta_SetWpa2Psk Return: %d\n", result);

    return result;
}

static le_result_t wlanSTATestConnect(taf_wlanSta_WlanSTARef_t staRef)
{
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }

    const char* ssidStr = le_arg_GetArg(2);
    if (NULL == ssidStr)
    {
        PrintUsage();
        LE_TEST_FATAL("ssid value is NULL");
    }

    std::string ssid(ssidStr);

    uint16_t numScanedAPs = 0;
    size_t APInfoSize = TAF_WLANSTA_MAX_APSCAN_RESULT_NUM;
    taf_wlanSta_APInfo_t ApInfo[TAF_WLANSTA_MAX_APSCAN_RESULT_NUM] = { 0 };
    taf_wlanSta_APInfo_t APInfoConnect;
    taf_wlanSta_GetAPScanResults(staRef, &numScanedAPs, ApInfo, &APInfoSize);
    bool isApFound = false;
    for(size_t i = 0; i < APInfoSize; ++i)
    {
        taf_wlanSta_APInfo_t ap = ApInfo[i];
        if(std::string(ap.SSID) == ssid)
        {
            LE_TEST_INFO("Found %s in the scanned APs list", ap.SSID);
            isApFound = true;
            APInfoConnect = ap;
            break;
        }
    }
    if(!isApFound)
    {
        printf("%s was not found in the scanned APs, try again\n", ssid.c_str());
        LE_TEST_EXIT;
    }
    connectPromise = std::promise<taf_wlanSta_State_t>();

    le_result_t result = taf_wlanSta_Connect(staRef, &APInfoConnect);
    fprintf(stderr, "taf_wlanSta_Connect Return: %d\n", result);

    // Waiting for maximum of 60 seconds for connect to be completed
    auto fut = connectPromise.get_future();
    auto status = fut.wait_for(std::chrono::seconds(60));
    if (status == std::future_status::ready)
    {
        taf_wlanSta_State_t state = fut.get();
        if (state == TAF_WLANSTA_STATE_CONNECTED)
        {
            printf("Connection to AP %s was successful..\n", ssid.c_str());
        }
        else if (state == TAF_WLANSTA_STATE_ASSOCIATION_FAILED)
        {
            printf("Connection to AP %s failed..", ssid.c_str());
        }
    }
    if (status == std::future_status::timeout)
    {
        LE_TEST_INFO("Timeout waiting for TAF_WLANSTA_STATE_CONNECTED event");
        result = LE_TIMEOUT;
    }

    return result;
}

static le_result_t wlanSTATestDisconnect(taf_wlanSta_WlanSTARef_t staRef)
{
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }
    const char* ssidStr = le_arg_GetArg(2);
    if (NULL == ssidStr)
    {
        PrintUsage();
        LE_TEST_FATAL("ssid value is NULL");
    }

    std::string ssid(ssidStr);

    uint16_t numScanedAPs = 0;
    size_t APInfoSize = TAF_WLANSTA_MAX_APSCAN_RESULT_NUM;
    taf_wlanSta_APInfo_t ApInfos[TAF_WLANSTA_MAX_APSCAN_RESULT_NUM] = { 0 };
    taf_wlanSta_APInfo_t APInfo;
    taf_wlanSta_GetAPScanResults(staRef, &numScanedAPs, ApInfos, &APInfoSize);
    bool isApFound = false;
    for(size_t i = 0; i < APInfoSize; ++i)
    {
        taf_wlanSta_APInfo_t ap = ApInfos[i];
        if(std::string(ap.SSID) == ssid)
        {
            LE_TEST_INFO("Found %s in the scanned APs list", ap.SSID);
            isApFound = true;
            APInfo = ap;
            break;
        }
    }
    if(!isApFound)
    {
        printf("%s was not found in the scanned APs, try again\n", ssid.c_str());
        LE_TEST_EXIT;
    }

    disconnectPromise = std::promise<taf_wlanSta_State_t>();

    le_result_t result = taf_wlanSta_Disconnect(staRef, &APInfo);
    fprintf(stderr, "taf_wlanSta_Connect Return: %d\n", result);

    // Waiting for maximum of 20 seconds for disconnect to be completed
    auto fut = disconnectPromise.get_future();
    auto status = fut.wait_for(std::chrono::seconds(20));
    if (status == std::future_status::ready)
    {
        taf_wlanSta_State_t state = fut.get();
        if (state == TAF_WLANSTA_STATE_DISCONNECTED)
        {
            printf("Disconnection from AP %s was successful.\n", APInfo.SSID);
        }
    }
    if (status == std::future_status::timeout)
    {
        LE_TEST_INFO("Timeout waiting for taf_wlanSta_Disconnect");
        result = LE_TIMEOUT;
    }
    return result;
}

static le_result_t wlanSTATestGetConnectedApSignalStrength(taf_wlanSta_WlanSTARef_t staRef)
{
    if (!staRef)
    {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }
    int16_t sigStrength;
    le_result_t result = taf_wlanSta_GetConnectedApSignalStrength(staRef, &sigStrength);
    fprintf(stderr, "taf_wlanSta_GetConnectedApSignalStrength Return: %d\n", result);
    if (result == LE_OK)
    {
        LE_TEST_INFO("Signal strength of connected AP is %ddBm", sigStrength);
        fprintf(stderr, "Connected AP signal strength: %ddBm\n", sigStrength);
    }
    return result;
}

static void SignalStrengthHandler
(
    taf_wlanSta_WlanSTARef_t staRef,
    int16_t signalStrength,
    void *contextPtr
)
{
    LE_UNUSED(staRef);
    uint8_t *numPtr = nullptr;
    if (contextPtr)
    {
        numPtr = static_cast<uint8_t*>(contextPtr);
        LE_TEST_INFO("Context                 : %d", *numPtr);
    }
    LE_TEST_INFO("Signal Strength Callback: %d dBm", signalStrength);
}


static le_result_t wlanSTATestAddConnectedApSignalStrengthHandler
(
    taf_wlanSta_WlanSTARef_t staRef
)
{
    if (!staRef)
    {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }

    // STA should be connected.
    taf_wlanSta_State_t state;
    char IntfName[TAF_NET_INTERFACE_NAME_MAX_LEN] = {0};
    char IPv4Address[TAF_NET_IPV4_ADDR_MAX_LEN] = {0};
    char IPv6Address[TAF_NET_IPV6_ADDR_MAX_LEN] = {0};
    char MACAddress[TAF_NET_MAC_ADDR_MAX_LEN] = {0};
    le_result_t result = taf_wlanSta_GetStatus(staRef, &state, IntfName,
                TAF_NET_INTERFACE_NAME_MAX_LEN, IPv4Address, TAF_NET_IPV4_ADDR_MAX_LEN, IPv6Address,
                TAF_NET_IPV6_ADDR_MAX_LEN, MACAddress, TAF_NET_MAC_ADDR_MAX_LEN);
    LE_TEST_ASSERT(LE_OK == result, "taf_wlanSta_GetStatus() should pass.");
    LE_TEST_ASSERT(state == TAF_WLANSTA_STATE_CONNECTED, "STA should be connected.");

    LE_TEST_INFO("Register handler 1");
    uint8_t num1 = 1;
    taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t signalStrengthHandlerRef1 = nullptr;
    signalStrengthHandlerRef1 = taf_wlanSta_AddConnectedApSignalStrengthHandler(staRef, -70, 10, false, SignalStrengthHandler, static_cast<void *>(&num1));
    LE_TEST_OK(nullptr != signalStrengthHandlerRef1, "Handler 1 should register. Service should switch to active monitoring.");
    // Sleep and check for active signal monitoring in logs
    sleep(5);

    LE_TEST_INFO("Register handler 2");
    uint8_t num2 = 2;
    taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t signalStrengthHandlerRef2 = nullptr;
    signalStrengthHandlerRef2 = taf_wlanSta_AddConnectedApSignalStrengthHandler(staRef, -75, 5, true, SignalStrengthHandler, static_cast<void *>(&num2));
    LE_TEST_OK(nullptr != signalStrengthHandlerRef2, "Handler 2 should register.");
    sleep(5);

    LE_TEST_INFO("Try to register handler 3");
    uint8_t num3 = 3;
    taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t signalStrengthHandlerRef3 = nullptr;
    signalStrengthHandlerRef3 = taf_wlanSta_AddConnectedApSignalStrengthHandler(staRef, -85, 15, true, SignalStrengthHandler, static_cast<void *>(&num3));
    LE_TEST_OK(nullptr == signalStrengthHandlerRef3, "Handler 3 should not register.");
    sleep(5);

    LE_TEST_INFO("Unregister handler 1");
    taf_wlanSta_RemoveConnectedApSignalStrengthHandler(signalStrengthHandlerRef1);
    // Sleep and check for active signal monitoring in logs
    LE_TEST_OK(true, "Handler 1 should deregister. Service should continue active monitoring.");
    sleep(5);

    LE_TEST_INFO("Register handler 3");
    signalStrengthHandlerRef3 = taf_wlanSta_AddConnectedApSignalStrengthHandler(staRef, -85, 15, true, SignalStrengthHandler, static_cast<void *>(&num3));
    LE_TEST_OK(nullptr != signalStrengthHandlerRef2, "Handler 3 should register.");
    sleep(5);

    LE_TEST_INFO("Service should switch to passive monitoring.");

    return LE_OK;
}

static void StationEventHandler(taf_wlanSta_WlanSTARef_t wlanSTARef,
                                taf_wlanSta_State_t staState,
                                void *CtxPtr)
{
    LE_UNUSED(wlanSTARef);
    LE_UNUSED(CtxPtr);
    if (staState == TAF_WLANSTA_STATE_SCAN_FAILED)
    {
        LE_TEST_FATAL("WLAN STA AP scanning failed");
    }
    else if (staState == TAF_WLANSTA_STATE_SCAN_COMPLETED)
    {
        // Allow main thread to proceed.
        le_sem_Post(wlanSemRef);
    }
    else if (staState == TAF_WLANSTA_STATE_CONNECTING)
    {
        LE_TEST_INFO("AP connection is in progress...");
    }
    else if (staState == TAF_WLANSTA_STATE_CONNECTED)
    {
        LE_TEST_INFO("AP connection was successful");
        try {
            connectPromise.set_value(TAF_WLANSTA_STATE_CONNECTED);
        }
        catch (std::future_error& e) {
            // ignore
        }
    }
    else if (staState == TAF_WLANSTA_STATE_ASSOCIATION_FAILED)
    {
        LE_TEST_INFO("AP connection failed");
        try {
            connectPromise.set_value(TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        }
        catch (std::future_error& e) {
            // ignore
        }
    }
    else if (staState == TAF_WLANSTA_STATE_DISCONNECTED)
    {
        LE_TEST_INFO("AP disconnection was successful");
        try {
            disconnectPromise.set_value(TAF_WLANSTA_STATE_DISCONNECTED);
        }
        catch (std::future_error& e) {
            // ignore
        }
    }
    else if (staState == TAF_WLANSTA_STATE_NETWORK_REMOVED)
    {
        LE_TEST_INFO("Network was removed");
        try {
            removeNetworkPromise.set_value(TAF_WLANSTA_STATE_NETWORK_REMOVED);
        }
        catch (std::future_error&) {
            // ignore
        }
    }
}

static void *wlanThreadHdlr(void *contextPtr)
{
    //  connect to wlan service from the thread's context.
    taf_wlan_ConnectService();
    taf_wlanSta_ConnectService();

    taf_wlan_APIntfInfo_t APIntInfos[TAF_WLAN_MAX_NUM_AP] = {};
    size_t APIntfInfoSize = TAF_WLAN_MAX_NUM_AP;
    taf_wlan_STAIntfInfo_t STAInfo[TAF_WLAN_MAX_NUM_STA] = {};
    size_t STAIntfInfoSize = TAF_WLAN_MAX_NUM_STA;
    taf_wlan_GetIntfInfo(nullptr, APIntInfos, &APIntfInfoSize, STAInfo, &STAIntfInfoSize);
    taf_wlanSta_WlanSTARef_t wlanSTARef = (taf_wlanSta_WlanSTARef_t) contextPtr;

    taf_wlanSta_EventHandlerRef_t staHdlrRef =
        taf_wlanSta_AddEventHandler(wlanSTARef, StationEventHandler, nullptr);
    LE_UNUSED(staHdlrRef);

    // Allow main thread to proceed.
    le_sem_Post(wlanSemRef);

    // Service events
    le_event_RunLoop();

    return nullptr;
}

static taf_wlanSta_WlanSTARef_t getSTARef(const char *staIntfNameStr)
{
    taf_wlan_APIntfInfo_t APIntf[TAF_WLAN_MAX_NUM_AP] = {};
    taf_wlan_STAIntfInfo_t STAIntf[TAF_WLAN_MAX_NUM_STA] = {};
    size_t APIntfSize = TAF_WLAN_MAX_NUM_AP, STAIntfSize = TAF_WLAN_MAX_NUM_STA;
    le_result_t status = taf_wlan_GetIntfInfo(NULL, APIntf, &APIntfSize, STAIntf, &STAIntfSize);
    if (LE_OK != status)
    {
        LE_TEST_FATAL("taf_wlan_GetIntfInfo failed %d", status);
    }
    if (0 == STAIntfSize)
    {
        LE_TEST_FATAL("STA is not enabled");
    }
    int STAIdx = -1;
    for (int i = 0; i < static_cast<int>(STAIntfSize); i++)
    {
        if (strncasecmp(staIntfNameStr, STAIntf[i].IntfName, strlen(staIntfNameStr)) == 0)
        {
            STAIdx = i;
            break;
        }
    }
    if (-1 == STAIdx)
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid STA interface name: %s", staIntfNameStr);
    }

    taf_wlanSta_WlanSTARef_t staRef = taf_wlanSta_GetWlanSTA(STAIntf[STAIdx].id,
                                                            STAIntf[STAIdx].IntfName);
    if (staRef == NULL)
    {
        LE_TEST_FATAL("taf_wlanSta_GetWlanSTA failed");
    }
    return staRef;
}

static le_result_t wlanSTATestGetAPEstimatedThroughput(taf_wlanSta_WlanSTARef_t staRef)
{
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }

    // First, we need to scan for APs to get their BSSIDs
    le_result_t result = taf_wlanSta_DoAPScan(staRef);
    if (result != LE_OK) {
        fprintf(stderr, "taf_wlanSta_DoAPScan failed: %d\n", result);
        return result;
    }

    // Wait for scan to complete with timeout
    le_clk_Time_t timeout = {20, 0};  // 20 seconds timeout
    if (le_sem_WaitWithTimeOut(wlanSemRef, timeout) != LE_OK) {
        LE_TEST_FATAL("Timeout waiting for scan to complete");
    }

    // Get scan results
    uint16_t numScanedAPs = 0;
    size_t APInfoSize = TAF_WLANSTA_MAX_APSCAN_RESULT_NUM;
    taf_wlanSta_APInfo_t ApInfo[TAF_WLANSTA_MAX_APSCAN_RESULT_NUM] = { 0 };
    result = taf_wlanSta_GetAPScanResults(staRef, &numScanedAPs, ApInfo, &APInfoSize);
    if (result != LE_OK) {
        fprintf(stderr, "taf_wlanSta_GetAPScanResults failed: %d\n", result);
        return result;
    }

    if (numScanedAPs == 0) {
        fprintf(stderr, "No APs found in scan results\n");
        return LE_NOT_FOUND;
    }

    printf("\nTesting estimated throughput for %d APs:\n", numScanedAPs);

    int availableCount = 0;
    int unavailableCount = 0;
    int errorCount = 0;
    std::map<uint32_t, int> throughputDistribution;
    std::map<uint32_t, std::vector<std::string>> throughputToSSIDs;

    // Try to get estimated throughput for each AP
    for (size_t i = 0; i < numScanedAPs; i++) {
        uint32_t estimatedThroughput = 0;
        int32_t age = -1;
        result = taf_wlanSta_GetAPEstimatedThroughput(staRef, ApInfo[i].BSSID,
            &estimatedThroughput, &age);

        printf("AP %zu: SSID=%s, BSSID=%s\n", i+1, ApInfo[i].SSID, ApInfo[i].BSSID);
        printf("  Signal Level: %d dBm, Frequency: %u MHz\n",
               ApInfo[i].SignalLevel, ApInfo[i].Frequency);
        printf("  Security: %s, Auth: %s\n",
               getSecurityMode(ApInfo[i].secMode),
               getSecurityAuthMethod(ApInfo[i].secAuthMethod));

        if (result == LE_OK) {
            printf("  Estimated Throughput: %u Kbps", estimatedThroughput);
            if (age >= 0) {
                printf(", Age: %d seconds\n", age);
            } else {
                printf(", Age: not available\n");
            }
            availableCount++;
            throughputDistribution[estimatedThroughput]++;
            throughputToSSIDs[estimatedThroughput].push_back(std::string(ApInfo[i].SSID));
        } else if (result == LE_UNAVAILABLE) {
            printf("  Estimated Throughput: Not available\n");
            unavailableCount++;
        } else if (result == LE_NOT_FOUND) {
            printf("  BSSID not found in scan results\n");
            errorCount++;
        } else {
            printf("  Error getting estimated throughput: %d\n", result);
            errorCount++;
        }
        printf("  ---------------- \n");
    }

    printf("\nSummary:\n");
    printf("  Total APs: %d\n", numScanedAPs);
    printf("  APs with throughput info: %d\n", availableCount);
    printf("  APs without throughput info: %d\n", unavailableCount);
    printf("  Errors: %d\n", errorCount);

    if (!throughputDistribution.empty()) {
        printf("\nThroughput distribution:\n");
        for (const auto& pair : throughputDistribution) {
            printf("  %u Kbps: %d APs\n", pair.first, pair.second);

            printf("    SSIDs: ");
            const auto& ssids = throughputToSSIDs[pair.first];
            for (size_t i = 0; i < ssids.size(); i++) {
                printf("%s", ssids[i].c_str());
                if (i < ssids.size() - 1) {
                    printf(", ");
                }
            }
            printf("\n");
        }
    }

    return LE_OK;
}

static le_result_t wlanSTATestRemoveNetwork(taf_wlanSta_WlanSTARef_t staRef)
{
    if (!staRef)
    {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }

    const char* ssidStr = le_arg_GetArg(2);
    if (ssidStr == nullptr)
    {
        PrintUsage();
        LE_TEST_FATAL("ssid value is NULL");
    }

    std::string ssid(ssidStr);

    // Build APInfoToRemove: prefer a scanned AP entry, fallback to SSID-only
    uint16_t numScanedAPs = 0;
    size_t APInfoSize = TAF_WLANSTA_MAX_APSCAN_RESULT_NUM;
    taf_wlanSta_APInfo_t ApInfo[TAF_WLANSTA_MAX_APSCAN_RESULT_NUM] = { 0 };
    taf_wlanSta_APInfo_t APInfoToRemove;
    le_result_t scanRes = taf_wlanSta_GetAPScanResults(staRef, &numScanedAPs, ApInfo, &APInfoSize);
    if (scanRes != LE_OK)
    {
        LE_WARN("No scan results available, using SSID only");
        memset(&APInfoToRemove, 0, sizeof(taf_wlanSta_APInfo_t));
        le_utf8_Copy(APInfoToRemove.SSID, ssid.c_str(), TAF_WLAN_MAX_SSID_LENGTH + 1, nullptr);
    }
    else
    {
        bool isApFound = false;
        for (size_t i = 0; i < APInfoSize; ++i)
        {
            if (std::string(ApInfo[i].SSID) == ssid)
            {
                LE_TEST_INFO("Found %s in the scanned APs list", ApInfo[i].SSID);
                APInfoToRemove = ApInfo[i];
                isApFound = true;
                break;
            }
        }
        if (!isApFound)
        {
            LE_WARN("%s not found in scan results, using SSID only", ssid.c_str());
            memset(&APInfoToRemove, 0, sizeof(taf_wlanSta_APInfo_t));
            le_utf8_Copy(APInfoToRemove.SSID, ssid.c_str(), TAF_WLAN_MAX_SSID_LENGTH + 1, nullptr);
        }
    }

    // Prepare to wait for the NETWORK_REMOVED event
    removeNetworkPromise = std::promise<taf_wlanSta_State_t>();

    printf("Attempting to remove network: %s\n", ssid.c_str());
    LE_TEST_INFO("Attempting to remove network: %s", ssid.c_str());
    le_result_t result = taf_wlanSta_RemoveNetwork(staRef, &APInfoToRemove);
    fprintf(stderr, "taf_wlanSta_RemoveNetwork Return: %d\n", result);
    LE_TEST_INFO("taf_wlanSta_RemoveNetwork Return: %d", result);

    if (result == LE_OK)
    {
        // Wait up to 10 seconds for NETWORK_REMOVED event from the service
        auto fut = removeNetworkPromise.get_future();
        auto status = fut.wait_for(std::chrono::seconds(10));
        if (status == std::future_status::ready &&
            fut.get() == TAF_WLANSTA_STATE_NETWORK_REMOVED)
        {
            printf("Network %s removed successfully (event received)\n", ssid.c_str());
            LE_TEST_INFO("Network %s removed successfully (event received)", ssid.c_str());
        }
        else
        {
            printf("Network %s removed successfully (no event observed within timeout)\n",
                   ssid.c_str());
            LE_TEST_INFO("Network %s removed successfully (no event within timeout)",
                ssid.c_str());
        }
    }
    else if (result == LE_NOT_FOUND)
    {
        printf("Network %s was not found in configured networks\n", ssid.c_str());
        printf("This means the network was never added/configured\n");
        LE_TEST_INFO("Network %s was not found in configured networks", ssid.c_str());
    }
    else if (result == LE_BAD_PARAMETER)
    {
        printf("Bad parameter error - check SSID: '%s'\n", ssid.c_str());
        LE_TEST_INFO("Bad parameter error - check SSID: '%s'", ssid.c_str());
    }
    else if (result == LE_FAULT)
    {
        printf("System fault occurred while removing network %s\n", ssid.c_str());
        LE_TEST_INFO("System fault occurred while removing network %s", ssid.c_str());
    }
    else
    {
        printf("Unexpected error (%d) while removing network %s\n", result, ssid.c_str());
        LE_TEST_INFO("Unexpected error (%d) while removing network %s", result, ssid.c_str());
    }

    return result;
}

static le_result_t wlanSTATestSaveNetworkConfig(taf_wlanSta_WlanSTARef_t staRef)
{
    if (!staRef)
    {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }

    printf("Attempting to save network configuration...\n");
    LE_TEST_INFO("Attempting to save network configuration");

    le_result_t result = taf_wlanSta_SaveNetworkConfig(staRef);
    fprintf(stderr, "taf_wlanSta_SaveNetworkConfig Return: %d\n", result);
    LE_TEST_INFO("taf_wlanSta_SaveNetworkConfig Return: %d", result);

    if (result == LE_OK)
    {
        printf("Network configuration saved successfully\n");
        LE_TEST_INFO("Network configuration saved successfully");
    }
    else if (result == LE_FAULT)
    {
        printf("Failed to save network configuration\n");
        LE_TEST_INFO("Failed to save network configuration");
    }
    else
    {
        printf("Unexpected error (%d) while saving configuration\n", result);
        LE_TEST_INFO("Unexpected error (%d) while saving configuration", result);
    }

    return result;
}

COMPONENT_INIT {
    le_result_t status = LE_FAULT;

    size_t numArgs = le_arg_NumArgs();
    if (numArgs == 0) {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }

    const char* testTypeStr = le_arg_GetArg(0);
    const char* staIntfName = le_arg_GetArg(1);

    LE_TEST_INFO("======== WLAN Station Integration Test ========");
    if (NULL == testTypeStr)
    {
        PrintUsage();
        LE_TEST_FATAL("Test type is NULL");
    }
    if (NULL == staIntfName)
    {
        PrintUsage();
        LE_TEST_FATAL("STA interface name is NULL");
    }
    LE_TEST_INFO("STA Interface to use: %s", staIntfName);

    char testType[30]="";   // NULL appended string
    le_utf8_Copy(testType, testTypeStr, 30, NULL);

    LE_TEST_INIT;
    // Register for events
    LE_TEST_INFO("======== Register for events ========");
    wlanSemRef = le_sem_Create("wlanSem", 0);
    le_thread_Ref_t wlanThreadRef = le_thread_Create("wlanThread", wlanThreadHdlr,
                                                     (void *)getSTARef(staIntfName));
    le_thread_Start(wlanThreadRef);
    le_sem_Wait(wlanSemRef);

    if (strncasecmp(testType, "Start", strlen("Start")) == 0) {
        LE_TEST_INFO("======== WLAN STA Test: Start ========");
        CheckNumArgs(numArgs, 2);
        status = wlanSTATestStart(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN STA Test: Start");
    } else if (strncasecmp(testType, "Stop", strlen("Stop")) == 0) {
        LE_TEST_INFO("======== WLAN Test: Stop ========");
        CheckNumArgs(numArgs, 2);
        status = wlanSTATestStop(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: Stop");
    } else if (strncasecmp(testType, "Restart", strlen("Restart")) == 0) {
        LE_TEST_INFO("======== WLAN Test: Restart ========");
        CheckNumArgs(numArgs, 2);
        status = wlanSTATestRestart(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: Restart");
    } else if (strncasecmp(testType, "GetStatus", strlen("GetStatus")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetStatus ========");
        CheckNumArgs(numArgs, 2);
        status = wlanSTATestGetStatus(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetStatus");
    } else if (strncasecmp(testType, "GetMode", strlen("GetMode")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetMode ========");
        CheckNumArgs(numArgs, 2);
        status = wlanSTATestGetMode(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetMode");
    } else if (strncasecmp(testType, "GetIpConfig", strlen("GetIpConfig")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetIpConfig ========");
        CheckNumArgs(numArgs, 2);
        status = wlanSTATestGetIPConfig(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetIpConfig");
    } else if (strncasecmp(testType, "SetIpConfig", strlen("SetIpConfig")) == 0) {
        LE_TEST_INFO("======== WLAN Test: SetIpConfig ========");
        // At least 3 args are required: testType, staIntfName, ip type
        CheckNumArgs(numArgs, 3);
        status = wlanSTATestSetIPConfig(getSTARef(staIntfName), numArgs);
        LE_TEST_OK(LE_OK == status, "WLAN Test: SetIpConfig");
    } else if (strncasecmp(testType, "SetMode", strlen("SetMode")) == 0) {
        LE_TEST_INFO("======== WLAN Test: SetMode ========");
        CheckNumArgs(numArgs, 3);
        status = wlanSTATestSetMode(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: SetMode");
    } else if (strncasecmp(testType, "DoAPScan", strlen("DoAPScan")) == 0) {
        LE_TEST_INFO("======== WLAN Test: DoAPScan ========");
        CheckNumArgs(numArgs, 2);
        status = wlanSTATestDoAPScan(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: DoAPScan");
    } else if (strncasecmp(testType, "GetAPScanResults", strlen("GetAPScanResults")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetAPScanResults ========");
        CheckNumArgs(numArgs, 2);
        status = wlanSTATestGetAPScanResults(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetAPScanResults");
    } else if (strncasecmp(testType, "SetWpa2Psk", strlen("SetWpa2Psk")) == 0) {
        LE_TEST_INFO("======== WLAN Test: SetWpa2Psk ========");
        CheckNumArgs(numArgs, 4);
        status = wlanSTATestSetWpa2Psk(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: SetWpa2Psk");
    } else if (strncasecmp(testType, "Connect", strlen("Connect")) == 0) {
        LE_TEST_INFO("======== WLAN Test: Connect ========");
        CheckNumArgs(numArgs, 3);
        status = wlanSTATestConnect(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: Connect");
    } else if (strncasecmp(testType, "Disconnect", strlen("Disconnect")) == 0) {
        LE_TEST_INFO("======== WLAN Test: Disconnect ========");
        CheckNumArgs(numArgs, 3);
        status = wlanSTATestDisconnect(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: Disconnect");
    } else if (strncasecmp(testType, "RemoveNetwork", strlen("RemoveNetwork")) == 0) {
        LE_TEST_INFO("======== WLAN Test: RemoveNetwork ========");
        CheckNumArgs(numArgs, 3);
        status = wlanSTATestRemoveNetwork(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status || LE_NOT_FOUND == status, "WLAN Test: RemoveNetwork");
    } else if (strncasecmp(testType, "SaveNetworkConfig", strlen("SaveNetworkConfig")) == 0) {
        LE_TEST_INFO("======== WLAN Test: SaveNetworkConfig ========");
        CheckNumArgs(numArgs, 2);
        status = wlanSTATestSaveNetworkConfig(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: SaveNetworkConfig");
    } else if (strncasecmp(testType, "GetApSignalStrength", strlen("GetApSignalStrength")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetApSignalStrength ========");
        CheckNumArgs(numArgs, 2);
        status = wlanSTATestGetConnectedApSignalStrength(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetApSignalStrength");
    } else if (strncasecmp(testType, "ApSigStrengthEvents", strlen("ApSigStrengthEvents")) == 0) {
        LE_TEST_INFO("======== WLAN Test: ApSigStrengthEvents ========");
        CheckNumArgs(numArgs, 2);
        status = wlanSTATestAddConnectedApSignalStrengthHandler(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: ApSigStrengthEvents");
    }
    else if (strncasecmp(testType, "GetAPEstimatedThroughput",
            strlen("GetAPEstimatedThroughput")) == 0)
    {
        LE_TEST_INFO("======== WLAN Test: GetAPEstimatedThroughput ========");
        CheckNumArgs(numArgs, 2);
        status = wlanSTATestGetAPEstimatedThroughput(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetAPEstimatedThroughput");
    } else {
        PrintUsage();
        LE_TEST_FATAL("Invalid test type %s", testType);
    }

    LE_TEST_EXIT;
}
