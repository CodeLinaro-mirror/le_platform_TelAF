/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       wlanSTAIntTest.cpp
 * @brief      Integration test functions for WLAN Station service.
 */

#include <iostream>
#include <string>
#include <future>
#include "interfaces.h"
#include "legato.h"

#define MAX_SYSTEM_CMD_LENGTH 200


static le_sem_Ref_t wlanSemRef = nullptr;
static std::promise<taf_wlanSta_State_t> connectPromise;
static std::promise<taf_wlanSta_State_t> disconnectPromise;

void PrintUsage() {
    printf("\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- Start <STA>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- Stop <STA>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- Restart <STA>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- GetStatus <STA>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- GetMode <STA>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- GetIPConfig <STA>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- SetMode  <STA> <Station Mode>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- DoAPScan <STA>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- GetAPScanResults <STA>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- SetWpa2Psk <STA> <SSID> <psk>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- Connect <STA> <SSID>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- Disconnect <STA> <SSID>\n"
           "\n STA: STA interface obtained from taf_wlan_GetIntfInfo\n"
           "\n");
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
            break;

        case TAF_WLANSTA_IPTYPE_DYNAMIC:
            LE_TEST_INFO("IPType     : TAF_WLANSTA_IPTYPE_DYNAMIC(%d)", IPType);
            break;

        case TAF_WLANSTA_IPTYPE_STATIC:
            LE_TEST_INFO("IPType     : TAF_WLANSTA_IPTYPE_STATIC(%d)", IPType);
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

    return result;
}

static le_result_t wlanSTATestSetMode(taf_wlanSta_WlanSTARef_t staRef)
{
    taf_wlanSta_Mode_t StaMode
        = (taf_wlanSta_Mode_t)strtol((const char*)le_arg_GetArg(1), nullptr, 10);

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

static void PrintAllAPInfoOnConsole(taf_wlanSta_APInfo_t* APInfoPtr, size_t APInfoSize) {
    printf("\nAvailable APs found during Scan: \n\n");
    printf("SSID\tWPS\n");
    for (size_t n = 0; n < APInfoSize; ++n) {
        printf("%s\t%s\n", APInfoPtr[n].SSID, APInfoPtr[n].WPSEnabled ? "Enabled" : "Disabled");
    }
}

static le_result_t wlanSTATestGetAPScanResults(taf_wlanSta_WlanSTARef_t staRef)
{
    uint16_t numScanedAPs = 0;
    size_t APInfoSize = TAF_WLANSTA_MAX_APSCAN_RESULT_NUM;
    taf_wlanSta_APInfo_t ApInfo[TAF_WLANSTA_MAX_APSCAN_RESULT_NUM] = { 0 };
    le_result_t result = taf_wlanSta_GetAPScanResults(staRef, &numScanedAPs, ApInfo, &APInfoSize);
    fprintf(stderr, "taf_wlanSta_GetAPScanResults Return: %d\n", result);
    printf("\nNum APs available     : %d", numScanedAPs);
    printf("\nNum elements populated: %" PRIuS "", APInfoSize);

    PrintAllAPInfoOnConsole(ApInfo, APInfoSize);
    return result;
}

static le_result_t wlanSTATestSetWpa2Psk(taf_wlanSta_WlanSTARef_t staRef)
{
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }

    std::string ssid(le_arg_GetArg(2));

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
        psk = std::string(le_arg_GetArg(3));
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

    std::string ssid(le_arg_GetArg(2));
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
    std::string ssid(le_arg_GetArg(2));
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
            printf("Disconnection from AP %s was successful..", APInfo.SSID);
        }
    }
    if (status == std::future_status::timeout)
    {
        LE_TEST_INFO("Timeout waiting for taf_wlanSta_Disconnect");
        result = LE_TIMEOUT;
    }
    return result;
}

inline void CheckNumArgs(size_t NumArgs, size_t ExpectedNumArgs) {
    if (NumArgs < ExpectedNumArgs) {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }
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

COMPONENT_INIT {
    le_result_t status = LE_FAULT;

    size_t numArgs = le_arg_NumArgs();
    if (numArgs == 0) {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }

    const char* testType = le_arg_GetArg(0);
    const char *staIntfName = le_arg_GetArg(1);

    LE_TEST_INFO("======== WLAN Station Integration Test ========");
    LE_TEST_INIT;

    if (NULL == staIntfName)
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid STA interface name is NULL");
    }
    LE_TEST_INFO("STA Interface to use: %s", staIntfName);

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
    } else if (strncasecmp(testType, "GetIPConfig", strlen("GetIPConfig")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetIPConfig ========");
        CheckNumArgs(numArgs, 2);
        status = wlanSTATestGetIPConfig(getSTARef(staIntfName));
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetIPConfig");
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
    } else {
        PrintUsage();
        LE_TEST_FATAL("Invalid test type %s", testType);
    }

    LE_TEST_EXIT;
}
