/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       wlanUnitTest.cpp
 * @brief      Unit test for WLAN device manager
 */

#include <iostream>
#include <string>
#include <future>
#include "legato.h"
#include "interfaces.h"

static le_sem_Ref_t wlanSemRef = nullptr;
static taf_wlanSta_WlanSTARef_t wlanSTARef = nullptr;

static std::promise<taf_wlanSta_State_t> connectPromise;
static std::promise<taf_wlanSta_State_t> disconnectPromise;

//static const taf_wlan_STAid_t STAid = TAF_WLAN_STA_ID1;
//static const char* wlanIntf = "wlan0";

static const char *StaEventsToStr(taf_wlanSta_State_t State)
{
    if (TAF_WLANSTA_STATE_UNKNOWN == State)
        return "TAF_WLANSTA_STATE_UNKNOWN";
    else if (TAF_WLANSTA_STATE_CONNECTING == State)
        return "TAF_WLANSTA_STATE_CONNECTING";
    else if (TAF_WLANSTA_STATE_CONNECTED == State)
        return "TAF_WLANSTA_STATE_CONNECTED";
    else if (TAF_WLANSTA_STATE_DISCONNECTED == State)
        return "TAF_WLANSTA_STATE_DISCONNECTED";
    else if (TAF_WLANSTA_STATE_ASSOCIATION_FAILED == State)
        return "TAF_WLANSTA_STATE_ASSOCIATION_FAILED";
    else if (TAF_WLANSTA_STATE_IP_ASSIGNMENT_FAILED == State)
        return "TAF_WLANSTA_STATE_IP_ASSIGNMENT_FAILED";
    else if (TAF_WLANSTA_STATE_SCAN_STARTED == State)
        return "TAF_WLANSTA_STATE_SCAN_STARTED";
    else if (TAF_WLANSTA_STATE_SCAN_COMPLETED == State)
        return "TAF_WLANSTA_STATE_SCAN_COMPLETED";
    else if (TAF_WLANSTA_STATE_SCAN_FAILED == State)
        return "TAF_WLANSTA_STATE_SCAN_FAILED";

    // Control should not reach here
    LE_TEST_INFO("*ERR* Unsupported State: %d", State);
    return nullptr;
}

static const char *SecModeToStr(taf_wlan_SecurityMode_t mode)
{
    if (TAF_WLAN_SEC_MODE_OPEN == mode)
        return "TAF_WLAN_SEC_MODE_OPEN";
    else if (TAF_WLAN_SEC_MODE_WEP == mode)
        return "TAF_WLAN_SEC_MODE_WEP";
    else if (TAF_WLAN_SEC_MODE_WPA == mode)
        return "TAF_WLAN_SEC_MODE_WPA";
    else if (TAF_WLAN_SEC_MODE_WPA2 == mode)
        return "TAF_WLAN_SEC_MODE_WPA2";
    else if (TAF_WLAN_SEC_MODE_WPA3 == mode)
        return "TAF_WLAN_SEC_MODE_WPA3";

    // Control should not reach here
    LE_TEST_INFO("*ERR* Unknown mode: %d", mode);
    return "TAF_WLAN_SEC_MODE_UNKNOWN";
}

static const char *SecAuthToStr(taf_wlan_SecurityAuthMethod_t Auth)
{
    if (TAF_WLAN_SEC_AUTH_METHOD_NONE == Auth)
        return "TAF_WLAN_SEC_AUTH_METHOD_NONE";
    else if (TAF_WLAN_SEC_AUTH_METHOD_PSK == Auth)
        return "TAF_WLAN_SEC_AUTH_METHOD_PSK";
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_SIM == Auth)
        return "TAF_WLAN_SEC_AUTH_METHOD_EAP_SIM";
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_AKA == Auth)
        return "TAF_WLAN_SEC_AUTH_METHOD_EAP_AKA";
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_LEAP == Auth)
        return "TAF_WLAN_SEC_AUTH_METHOD_EAP_LEAP";
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_TLS == Auth)
        return "TAF_WLAN_SEC_AUTH_METHOD_EAP_TLS";
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_TTLS == Auth)
        return "TAF_WLAN_SEC_AUTH_METHOD_EAP_TTLS";
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_PEAP == Auth)
        return "TAF_WLAN_SEC_AUTH_METHOD_EAP_PEAP";
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_FAST == Auth)
        return "TAF_WLAN_SEC_AUTH_METHOD_EAP_FAST";
    else if (TAF_WLAN_SEC_AUTH_METHOD_EAP_PSK == Auth)
        return "TAF_WLAN_SEC_AUTH_METHOD_EAP_PSK";
    else if (TAF_WLAN_SEC_AUTH_METHOD_SAE == Auth)
        return "TAF_WLAN_SEC_AUTH_METHOD_SAE";

    // Control should not reach here
    LE_TEST_INFO("*ERR* Unknown auth method: %d", Auth);
    return "TAF_WLAN_SEC_AUTH_METHOD_UNKNOWN";
}

static const char *SecEncryptToStr(taf_wlan_SecurityEncryptionMethod_t encrypt)
{
    if (TAF_WLAN_SEC_ENCRYPT_METHOD_RC4 == encrypt)
        return "TAF_WLAN_SEC_ENCRYPT_METHOD_RC4";
    else if (TAF_WLAN_SEC_ENCRYPT_METHOD_TKIP == encrypt)
        return "TAF_WLAN_SEC_ENCRYPT_METHOD_TKIP";
    else if (TAF_WLAN_SEC_ENCRYPT_METHOD_AES == encrypt)
        return "TAF_WLAN_SEC_ENCRYPT_METHOD_AES";
    else if (TAF_WLAN_SEC_ENCRYPT_METHOD_GCMP == encrypt)
        return "TAF_WLAN_SEC_ENCRYPT_METHOD_GCMP";

    // Control should not reach here
    LE_TEST_INFO("*ERR* Unknown encryption: %d", encrypt);
    return "TAF_WLAN_SEC_ENCRYPT_METHOD_UNKNOWN";
}

static void PrintAPInfo(taf_wlanSta_APInfo_t *APInfoPtr)
{
    LE_TEST_INFO("  BSSID        : %s", APInfoPtr->BSSID);
    LE_TEST_INFO("  SSID         : %s", APInfoPtr->SSID);
    LE_TEST_INFO("  Frequency    : %d MHz", APInfoPtr->Frequency);
    LE_TEST_INFO("  Signal Level : %d dBm", APInfoPtr->SignalLevel);
    LE_TEST_INFO("  WPS          : %s", APInfoPtr->WPSEnabled ? "Enabled" : "Disabled");
    LE_TEST_INFO("  Service Set  : %s", (APInfoPtr->SS == TAF_WLAN_SS_BASIC) ? "BSS" : "ESS");
    LE_TEST_INFO("  Security");
    LE_TEST_INFO("    Proto      : %s", SecModeToStr(APInfoPtr->secMode));
    LE_TEST_INFO("    AuthMode   : %s", SecAuthToStr(APInfoPtr->secAuthMethod));
    LE_TEST_INFO("    Encryption : %s", SecEncryptToStr(APInfoPtr->secEncryptionMethod));
    return;
}

static void PrintAllAPInfoOnConsole(taf_wlanSta_APInfo_t *APInfoPtr, size_t APInfoSize)
{
    printf("Available APs found during Scan: \n\n");
    printf("SSID\tWPS\tAuthMode\n");
    for(size_t n = 0; n < APInfoSize; ++n)
    {
        printf("%s\t%s\t%s\n", APInfoPtr[n].SSID,
                                   APInfoPtr[n].WPSEnabled ? "Enabled" : "Disabled",
                                   SecAuthToStr(APInfoPtr[n].secAuthMethod));
    }
}

static void DeviceStateHandler(taf_wlan_WlanRef_t wlanRef,
                   taf_wlan_DeviceState_t state,
                   void *CtxPtr)
{
    LE_UNUSED (wlanRef);
    LE_UNUSED (CtxPtr);
    LE_TEST_INFO("DeviceStateHandler: State: %d", state);
}

static void StationEventHandler(taf_wlanSta_WlanSTARef_t wlanSTARef,
                                taf_wlanSta_State_t staState,
                                void *CtxPtr)
{
    LE_UNUSED(wlanSTARef);
    LE_UNUSED(CtxPtr);
    LE_TEST_INFO("Station Event: %s(%d)", StaEventsToStr(staState), staState);
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

    taf_wlanSta_EventHandlerRef_t staHdlrRef =
        taf_wlanSta_AddEventHandler(wlanSTARef, StationEventHandler, nullptr);
    LE_UNUSED(staHdlrRef);

    // Allow main thread to proceed.
    le_sem_Post(wlanSemRef);

    // Service events
    le_event_RunLoop();

    return nullptr;
}

static void PrintAPIntfInfo(taf_wlan_APIntfInfo_t *APIntfInfoPtr, size_t APIntfinfoSize)
{
    for (unsigned int iCount = 0; iCount < APIntfinfoSize; iCount++)
    {
        LE_TEST_INFO("AP[%d] ID  : %d", (iCount + 1), APIntfInfoPtr[iCount].id);
        LE_TEST_INFO("AP[%d] Intf: %s", (iCount + 1), APIntfInfoPtr[iCount].IntfName);
    }
    return;
}

static void PrintSTAIntfInfo(taf_wlan_STAIntfInfo_t *STAIntfInfoPtr, size_t STAIntfinfoSize)
{
    for (unsigned int iCount = 0; iCount < STAIntfinfoSize; iCount++)
    {
        LE_TEST_INFO("STA[%d] ID  : %d", (iCount + 1), STAIntfInfoPtr[iCount].id);
        LE_TEST_INFO("STA[%d] Intf: %s", (iCount + 1), STAIntfInfoPtr[iCount].IntfName);
    }
    return;
}

/*
    Command to run this test:
        app runProc tafWLANUnitTest wlanUnitTest
*/
COMPONENT_INIT
{
    le_result_t result = LE_FAULT;
    taf_wlan_DeviceState_t state;
    taf_wlan_DeviceMode_t mode = TAF_WLAN_MODE_UNKNOWN;
    bool bSetStaMode = false;
    bool bRestartWLAN = false;

    LE_TEST_INIT;
    LE_TEST_INFO("======== WLAN Unit Test ========");

    // Register device state handler
    taf_wlan_DeviceStateHandlerRef_t devHdlrRef =
        taf_wlan_AddDeviceStateHandler(nullptr, DeviceStateHandler, nullptr);
    LE_UNUSED(devHdlrRef);

    // Get Mode
    LE_TEST_INFO("======== WLAN Unit Test: taf_wlan_GetMode ========");
    result = taf_wlan_GetMode(nullptr, &mode);
    LE_TEST_INFO("Mode: %d", mode);
    LE_TEST_OK(LE_OK == result, "WLAN Unit Test: taf_wlan_GetMode");
    // Check if STA is enabled
    if (TAF_WLAN_MODE_STA != mode && TAF_WLAN_MODE_STA_AP != mode)
    {
        // Need to enable STA
        LE_TEST_INFO("STA mode is not set");
        bSetStaMode = true;
    }

    // Get State
    LE_TEST_INFO("======== WLAN Unit Test: taf_wlan_GetState ========");
    result = taf_wlan_GetState(nullptr, &state);
    LE_TEST_INFO("State: %d", state);
    LE_TEST_OK(LE_OK == result, "WLAN Unit Test: taf_wlan_GetState");
    // Ensure WLAN is ON.
    if (TAF_WLAN_ON == state) {
        LE_TEST_INFO("WLAN is ON");
        if (bSetStaMode)
        {
            LE_TEST_INFO("Need to restart WLAN to set STA");
            bRestartWLAN = true;
        }
    }
    else {
        LE_TEST_INFO("WLAN is OFF");
    }

    // Turn OFF WLAN if needed
    if (bRestartWLAN)
    {
        LE_TEST_INFO("======== WLAN Unit Test: taf_wlan_TurnOff ========");
        result = taf_wlan_TurnOff(nullptr);
        LE_TEST_OK(LE_OK == result, "WLAN Unit Test: taf_wlan_TurnOff");
    }

    // Set STA Mode
    LE_TEST_INFO("======== WLAN Unit Test: taf_wlan_SetMode ========");
    mode = TAF_WLAN_MODE_STA_AP;
    result = taf_wlan_SetMode(nullptr, mode);
    LE_TEST_OK(LE_OK == result, "WLAN Unit Test: taf_wlan_SetMode");

    // Turn ON WLAN if needed
    // Get State
    LE_TEST_INFO("======== WLAN Unit Test: taf_wlan_GetState ========");
    result = taf_wlan_GetState(nullptr, &state);
    LE_TEST_INFO("State: %d", state);
    LE_TEST_OK(LE_OK == result, "WLAN Unit Test: taf_wlan_GetState");
    // Ensure WLAN is ON.
    if (TAF_WLAN_ON == state)
    {
        LE_TEST_INFO("WLAN is ON");
    }
    else
    {
        LE_TEST_INFO("WLAN is OFF");
        LE_TEST_INFO("======== WLAN Unit Test: taf_wlan_TurnOn ========");
        result = taf_wlan_TurnOn(nullptr);
        LE_TEST_OK(LE_OK == result, "WLAN Unit Test: taf_wlan_TurnOn");
    }

    // Get Interface information
    LE_TEST_INFO("======== WLAN Unit Test: taf_wlan_GetIntfInfo ========");
    taf_wlan_APIntfInfo_t APIntInfos[TAF_WLAN_MAX_NUM_AP] = {};
    size_t APIntfInfoSize = TAF_WLAN_MAX_NUM_AP;
    taf_wlan_STAIntfInfo_t STAInfo[TAF_WLAN_MAX_NUM_STA] = {};
    size_t STAIntfInfoSize = TAF_WLAN_MAX_NUM_STA;
    result = taf_wlan_GetIntfInfo(nullptr, APIntInfos, &APIntfInfoSize, STAInfo, &STAIntfInfoSize);
    LE_TEST_OK(LE_OK == result, "WLAN Unit Test: taf_wlan_GetIntfInfo");
    PrintAPIntfInfo(APIntInfos, APIntfInfoSize);
    PrintSTAIntfInfo(STAInfo, STAIntfInfoSize);

    // Get WLAN STA Reference
    LE_TEST_INFO("======== Get WLAN STA Reference ========");
    wlanSTARef = taf_wlanSta_GetWlanSTA(STAInfo[0].id, STAInfo[0].IntfName);
    LE_TEST_OK(nullptr != wlanSTARef, "WLAN Unit Test: taf_wlanSta_GetWlanSTA");

    // Register for events
    LE_TEST_INFO("======== Register for events ========");
    wlanSemRef = le_sem_Create("wlanSem", 0);
    le_thread_Ref_t wlanThreadRef = le_thread_Create("wlanThread", wlanThreadHdlr, nullptr);
    le_thread_Start(wlanThreadRef);
    le_sem_Wait(wlanSemRef);

    // Start WLAN STA scan
    LE_TEST_INFO("======== Do WLAN STA AP Scan ========");
    result = taf_wlanSta_DoAPScan(wlanSTARef);
    LE_TEST_ASSERT(LE_OK == result, "WLAN Unit Test: taf_wlanSta_DoAPScan");

    LE_TEST_INFO("Waiting for SCAN_COMPLETED event...");
    le_sem_Wait(wlanSemRef);

    LE_TEST_INFO("======== Do WLAN STA AP Get Scan Results ========");
    uint16_t numScanedAPs = 0;
    size_t APInfoSize = TAF_WLANSTA_MAX_APSCAN_RESULT_NUM;
    taf_wlanSta_APInfo_t ApInfos[TAF_WLANSTA_MAX_APSCAN_RESULT_NUM] = {0};
    result = taf_wlanSta_GetAPScanResults(wlanSTARef, &numScanedAPs, ApInfos, &APInfoSize);
    LE_TEST_ASSERT(LE_OK == result, "WLAN Unit Test: taf_wlanSta_GetAPScanResults");
    LE_TEST_INFO("Num APs available     : %d", numScanedAPs);
    LE_TEST_INFO("Num elements populated: %" PRIuS "", APInfoSize);

    // Ensure the number of APs scanned and elements populated are equal
    LE_TEST_OK(numScanedAPs == APInfoSize,
        "num APs scanned and elements populated should be equal");
    for (unsigned int iCount = 0; iCount < static_cast<unsigned int>(APInfoSize); iCount++)
    {
        LE_TEST_INFO("AP Number : %d", (iCount + 1));
        PrintAPInfo(&ApInfos[iCount]);
    }

    PrintAllAPInfoOnConsole(ApInfos, APInfoSize);

    LE_TEST_INFO("======== Do WLAN STA AP Get Scan Results(one element) ========");
    numScanedAPs = 0;
    size_t APInfoSizeOne = 1;
    taf_wlanSta_APInfo_t ApInfo = {};
    result = taf_wlanSta_GetAPScanResults(wlanSTARef, &numScanedAPs, &ApInfo, &APInfoSizeOne);
    LE_TEST_ASSERT(LE_OK == result, "WLAN Unit Test: taf_wlanSta_GetAPScanResults");
    LE_TEST_INFO("Num APs available     : %d", numScanedAPs);
    LE_TEST_INFO("Num elements populated: %" PRIuS "", APInfoSizeOne);

    // Ensure the number of APs scanned and elements populated are NOT equal
    LE_TEST_OK(APInfoSizeOne <= numScanedAPs, "APInfoSize is lesser than or equal to numScanedAPs");
    LE_TEST_OK(1 == APInfoSizeOne, "APInfoSizeOne should be 1");
    LE_TEST_INFO("AP Number : 1");
    PrintAPInfo(&ApInfo);

    LE_TEST_INFO("======== ALL WLAN UNIT TESTS PASSED ========");
    LE_TEST_EXIT;
}
