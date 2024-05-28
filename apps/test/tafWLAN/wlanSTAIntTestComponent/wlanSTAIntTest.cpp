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

/**
 * With current releases, only 1 STA is supported. So hardcode the following
 * STA ID = TAF_WLAN_STA_ID1
 * STA Interface = wlan0
 */
static const taf_wlan_STAid_t g_StaID = TAF_WLAN_STA_ID1;
static const char* g_StaIntf = "wlan0";
// TBD: Make STA ID and Interface user provided values. as done for "GetRef".

static le_sem_Ref_t wlanSemRef = nullptr;
static taf_wlanSta_WlanSTARef_t wlanSTARef = nullptr;
static std::promise<taf_wlanSta_State_t> connectPromise;
static std::promise<taf_wlanSta_State_t> disconnectPromise;

void PrintUsage() {
    printf("\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- GetRef <STA ID> <Interface>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- Restart\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- GetStatus\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- GetMode\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- GetIPConfig\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- SetMode <Station Mode>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- DoAPScan\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- GetAPScanResults\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- SetWpa2Psk <SSID> <psk>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- Connect <SSID>\n"
           "app runProc tafWLANSTAIntTest wlanSTATest -- Disconnect <SSID>\n"
           "\n");
}

static le_result_t wlanSTATestStart() {
    taf_wlanSta_WlanSTARef_t staRef = taf_wlanSta_GetWlanSTA(g_StaID, g_StaIntf);
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }
    le_result_t result = taf_wlanSta_Start(staRef);
    fprintf(stderr, "taf_wlanSta_Start Return:%d\n", result);
    return result;
}

static le_result_t wlanSTATestStop() {
    taf_wlanSta_WlanSTARef_t staRef = taf_wlanSta_GetWlanSTA(g_StaID, g_StaIntf);
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }
    le_result_t result = taf_wlanSta_Stop(staRef);
    fprintf(stderr, "taf_wlanSta_Stop Return:%d\n", result);
    return result;
}

static le_result_t wlanSTATestRestart() {
    taf_wlanSta_WlanSTARef_t staRef = taf_wlanSta_GetWlanSTA(g_StaID, g_StaIntf);
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }
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

static le_result_t wlanSTATestGetStatus() {
    taf_wlanSta_WlanSTARef_t staRef = taf_wlanSta_GetWlanSTA(g_StaID, g_StaIntf);
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }
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

static le_result_t wlanSTATestGetMode() {
    taf_wlanSta_WlanSTARef_t staRef = taf_wlanSta_GetWlanSTA(g_StaID, g_StaIntf);
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }
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

static le_result_t wlanSTATestGetIPConfig() {
    taf_wlanSta_WlanSTARef_t staRef = taf_wlanSta_GetWlanSTA(g_StaID, g_StaIntf);
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }
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

static le_result_t wlanSTATestSetMode() {
    taf_wlanSta_WlanSTARef_t staRef = taf_wlanSta_GetWlanSTA(g_StaID, g_StaIntf);
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }
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

static le_result_t wlanSTATestDoAPScan() {
    taf_wlanSta_WlanSTARef_t staRef = taf_wlanSta_GetWlanSTA(g_StaID, g_StaIntf);
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }
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

static le_result_t wlanSTATestGetAPScanResults() {
    taf_wlanSta_WlanSTARef_t staRef = taf_wlanSta_GetWlanSTA(g_StaID, g_StaIntf);
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }
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

static le_result_t wlanSTATestGetRef() {
    taf_wlan_STAid_t staID = (taf_wlan_STAid_t)strtol((const char*)le_arg_GetArg(1), nullptr, 10);
    LE_TEST_INFO("Getting Ref for ID: %d, Intf: %s", staID, le_arg_GetArg(2));
    taf_wlanSta_WlanSTARef_t staRef = taf_wlanSta_GetWlanSTA(staID, (const char*)le_arg_GetArg(2));
    if (!staRef) {
        fprintf(stderr, "taf_wlanSta_GetWlanSTA failed\n");
        return LE_FAULT;
    }
    return LE_OK;
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
    wlanSTARef = taf_wlanSta_GetWlanSTA(STAInfo[0].id, STAInfo[0].IntfName);

    taf_wlanSta_EventHandlerRef_t staHdlrRef =
        taf_wlanSta_AddEventHandler(wlanSTARef, StationEventHandler, nullptr);
    LE_UNUSED(staHdlrRef);

    // Allow main thread to proceed.
    le_sem_Post(wlanSemRef);

    // Service events
    le_event_RunLoop();

    return nullptr;
}

COMPONENT_INIT {
    le_result_t status = LE_FAULT;

    size_t numArgs = le_arg_NumArgs();
    if (numArgs == 0) {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }

    const char* testType = le_arg_GetArg(0);

    LE_TEST_INFO("======== WLAN Station Integration Test ========");
    LE_TEST_INIT;

    // Register for events
    LE_TEST_INFO("======== Register for events ========");
    wlanSemRef = le_sem_Create("wlanSem", 0);
    le_thread_Ref_t wlanThreadRef = le_thread_Create("wlanThread", wlanThreadHdlr, nullptr);
    le_thread_Start(wlanThreadRef);
    le_sem_Wait(wlanSemRef);

    if (strncasecmp(testType, "GetRef", strlen("GetRef")) == 0) {
        LE_TEST_INFO("======== WLAN AP Test: GetRef ========");
        CheckNumArgs(numArgs, 3);
        status = wlanSTATestGetRef();
        LE_TEST_OK(LE_OK == status, "WLAN AP Test: GetRef");
    } else if (strncasecmp(testType, "Start", strlen("Start")) == 0) {
        LE_TEST_INFO("======== WLAN AP Test: Start ========");
        CheckNumArgs(numArgs, 1);
        status = wlanSTATestStart();
        LE_TEST_OK(LE_OK == status, "WLAN AP Test: Start");
    } else if (strncasecmp(testType, "Stop", strlen("Stop")) == 0) {
        LE_TEST_INFO("======== WLAN Test: Stop ========");
        CheckNumArgs(numArgs, 1);
        status = wlanSTATestStop();
        LE_TEST_OK(LE_OK == status, "WLAN Test: Stop");
    } else if (strncasecmp(testType, "Restart", strlen("Restart")) == 0) {
        LE_TEST_INFO("======== WLAN Test: Restart ========");
        CheckNumArgs(numArgs, 1);
        status = wlanSTATestRestart();
        LE_TEST_OK(LE_OK == status, "WLAN Test: Restart");
    } else if (strncasecmp(testType, "GetStatus", strlen("GetStatus")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetStatus ========");
        CheckNumArgs(numArgs, 1);
        status = wlanSTATestGetStatus();
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetStatus");
    } else if (strncasecmp(testType, "GetMode", strlen("GetMode")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetMode ========");
        CheckNumArgs(numArgs, 1);
        status = wlanSTATestGetMode();
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetMode");
    } else if (strncasecmp(testType, "GetIPConfig", strlen("GetIPConfig")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetIPConfig ========");
        CheckNumArgs(numArgs, 1);
        status = wlanSTATestGetIPConfig();
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetIPConfig");
    } else if (strncasecmp(testType, "SetMode", strlen("SetMode")) == 0) {
        LE_TEST_INFO("======== WLAN Test: SetMode ========");
        CheckNumArgs(numArgs, 2);
        status = wlanSTATestSetMode();
        LE_TEST_OK(LE_OK == status, "WLAN Test: SetMode");
    } else if (strncasecmp(testType, "DoAPScan", strlen("DoAPScan")) == 0) {
        LE_TEST_INFO("======== WLAN Test: DoAPScan ========");
        CheckNumArgs(numArgs, 1);
        status = wlanSTATestDoAPScan();
        LE_TEST_OK(LE_OK == status, "WLAN Test: DoAPScan");
    } else if (strncasecmp(testType, "GetAPScanResults", strlen("GetAPScanResults")) == 0) {
        LE_TEST_INFO("======== WLAN Test: GetAPScanResults ========");
        CheckNumArgs(numArgs, 1);
        status = wlanSTATestGetAPScanResults();
        LE_TEST_OK(LE_OK == status, "WLAN Test: GetAPScanResults");
    } else {
        PrintUsage();
        LE_TEST_FATAL("Invalid test type %s", testType);
    }

    LE_TEST_EXIT;
}
