/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanSTASvcImpl.cpp
 *
 * @brief      Implementation of TelAF WLAN Station Service APIs.
 *
 */


#include "tafWlan.hpp"
#include <wpa_ctrl.h>
#include <errno.h>

using namespace tafsvc;

// Memory pool for STA events for applications
LE_MEM_DEFINE_STATIC_POOL(StaEventsPool,
                          TAF_WLAN_MAX_SESSION_REF,
                          (sizeof(StaEvents_t)));

// Memory pool for STA contexts
LE_MEM_DEFINE_STATIC_POOL ( tafWlanStaCtxPool,
                            TAF_WLAN_MAX_NUM_STA,
                            (sizeof(StaCtx_t)) );

//--------------------------------------------------------------------------------------------------
/**
 * First layer STA event handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::FirstLayerEventHandler(void *reportPtr, void *secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "reportPtr is NULL!");
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == NULL,"secondLayerHandlerFunc is NULL!");

    StaEvents_t *StaEventPtr = (StaEvents_t *)reportPtr;

    taf_wlanSta_HandlerFunc_t clientHandlerFunc = (taf_wlanSta_HandlerFunc_t)secondLayerHandlerFunc;

    LE_DEBUG("Call client callback");
    clientHandlerFunc(StaEventPtr->staRef, StaEventPtr->state, le_event_GetContextPtr());
    // Release the pointer that was allocated when the event was sent.
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Report station state events for registered applications
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::ReportStaState(
    StaCtx_t *staCtxPtr,      // Sta Context
    taf_wlanSta_State_t state // State to report
)
{
    // If STA is connected/disconnected, start/stop link monitoring respectively.
    if (TAF_WLANSTA_STATE_CONNECTED == state)
    {
        StaCmd_t staCmd = {staCtxPtr, CMD_WPA_START_LINK_MONITORING, nullptr};
        LE_DEBUG("Sta connected. Send command to start link monitoring.");
        le_event_Report(staCommand_, &staCmd, sizeof(StaCmd_t));
    }
    else if (TAF_WLANSTA_STATE_DISCONNECTED == state)
    {
        StaCmd_t staCmd = {staCtxPtr, CMD_WPA_STOP_LINK_MONITORING, nullptr};
        LE_DEBUG("Sta disconnected. Send command to stop link monitoring.");
        le_event_Report(staCommand_, &staCmd, sizeof(StaCmd_t));
    }

    LE_DEBUG("Send STA events to registered clients.");
    // Report STA state event to clients.
    StaEvents_t *StaEventPtr = (StaEvents_t *)le_mem_ForceAlloc(StaEventsPoolRef);
    TAF_ERROR_IF_RET_NIL(StaEventPtr == NULL, "StaEventPtr is NULL!");
    StaEventPtr->staRef = staCtxPtr->staRef;
    StaEventPtr->state = state;
    le_event_ReportWithRefCounting(staCtxPtr->StaEvent, (void *)StaEventPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_wlanSta_Event'
 */
//--------------------------------------------------------------------------------------------------
taf_wlanSta_EventHandlerRef_t taf_WlanSTASvcImpl::AddEventHandler(
    taf_wlanSta_WlanSTARef_t staRef,
    taf_wlanSta_HandlerFunc_t handlerPtr,
    void *contextPtr)
{
    StaCtx_t *staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, NULL, "Unable to find context");

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                "StaEventHandler",
                                                staCtxPtr->StaEvent,
                                                FirstLayerEventHandler,
                                                (void *)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_wlanSta_EventHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_wlanSta_Event'
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::RemoveEventHandler(taf_wlanSta_EventHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread to monitor WPA supplicant events
 */
//--------------------------------------------------------------------------------------------------
void *taf_WlanSTASvcImpl::StaWpaSuppMonitorThreadHdlr(void *context)
{
    struct wpa_ctrl *ctrl = nullptr;
    SuppThreadCtx_t *ThreadCtxPtr = (SuppThreadCtx_t *)context;
    std::string wpa_supplicant_path(WPA_SUPPLICANT_LOCATION_PATH);
    wpa_supplicant_path = wpa_supplicant_path + ThreadCtxPtr->IntfName;
    LE_DEBUG("Supplicant: %s", wpa_supplicant_path.c_str());

    StaWpaEvt_e resultEvent = EVT_WPA_ERROR;
    bool eventFound = false;

    ctrl = wpa_ctrl_open(wpa_supplicant_path.c_str());
    if (ctrl == nullptr)
    {
        LE_ERROR("Failed to open control interface");
        {
            std::lock_guard<std::mutex> lock(ThreadCtxPtr->mutex);
            ThreadCtxPtr->resultEvent = EVT_WPA_ERROR;
            ThreadCtxPtr->eventCompleted = true;
        }
        return nullptr;
    }

    int ret = wpa_ctrl_attach(ctrl);
    if (ret != 0)
    {
        LE_ERROR("Failed to attach to control interface");
        {
            std::lock_guard<std::mutex> lock(ThreadCtxPtr->mutex);
            ThreadCtxPtr->resultEvent = EVT_WPA_ERROR;
            ThreadCtxPtr->eventCompleted = true;
        }
        wpa_ctrl_close(ctrl);
        return nullptr;
    }

    while(!eventFound && !ThreadCtxPtr->shouldExit)
    {
        ret = wpa_ctrl_pending(ctrl);
        if (1 == ret)
        {
            char rsp[WPA_CTRL_RSP_BUF_LEN] = {0};
            size_t len = WPA_CTRL_RSP_BUF_LEN;
            // Event pending;
            ret = wpa_ctrl_recv(ctrl, rsp, &len);
            if (ret < 0)
            {
                LE_WARN("wpa_ctrl_recv failed");
                resultEvent = EVT_WPA_ERROR;
                eventFound = true;
                break;
            }
            LE_DEBUG("Received response Len: %zu", len);
            LE_DEBUG("Received response    : %s", rsp);
            // Split the received buffer using space as delimiter
            std::vector<std::string> ind = taf_WlanHelper::StrSplit(rsp, ' ');
            if (!ind.empty()) {
                LE_DEBUG("Received Event: %s", ind[0].c_str());
                // Check if the event that is received is the one we are waiting on
                for(const std::string& evt : ThreadCtxPtr->EventsToMonitor)
                {
                    LE_INFO("Wait for event: %s", evt.c_str());
                    std::string s1 = taf_WlanHelper::StrTrimEndSpace(evt);
                    std::string s2 = taf_WlanHelper::StrTrimEndSpace(ind[0]);
                    if (s2.find(s1) != std::string::npos)
                    {
                        if (s1 == taf_WlanHelper::StrTrimEndSpace(WPA_EVENT_SCAN_RESULTS))
                        {
                            // Scanning is done
                            resultEvent = EVT_WPA_AP_SCAN_DONE;
                        }
                        else if (s1 == taf_WlanHelper::StrTrimEndSpace(WPA_EVENT_CONNECTED))
                        {
                            // Network connection was successful
                            LE_DEBUG("Network connection successful");
                            resultEvent = EVT_WPA_AP_CONNECTED;
                        }
                        else if (s1 == taf_WlanHelper::StrTrimEndSpace(WPA_EVENT_TEMP_DISABLED))
                        {
                            LE_DEBUG("Network temporarily disabled");
                            resultEvent = EVT_WPA_AP_TEMP_DISABLED;
                        }
                        LE_DEBUG("Found event, Exiting thread");
                        eventFound = true;
                        break;
                    }
                }
            }
        }
        else if (ret < 0) {
            // Error occurred
            LE_ERROR("wpa_ctrl_pending failed");
            resultEvent = EVT_WPA_ERROR;
            eventFound = true;
            break;
        }
        else {
            // No events pending, sleep a bit to avoid busy waiting
            usleep(10000); // 10ms
        }
    }

    // Store the result in the thread context
    {
        std::lock_guard<std::mutex> lock(ThreadCtxPtr->mutex);
        ThreadCtxPtr->resultEvent = resultEvent;
        ThreadCtxPtr->eventCompleted = true;
    }

    LE_DEBUG("Exiting thread with result: %d", static_cast<int>(resultEvent));
    wpa_ctrl_close(ctrl);
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse "Flags" from SCAN_RESULTS response and populate taf_wlanSta_APInfo_t
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::ParseScanResultsFlags( StaCtx_t *CtxPtr, int Index, std::string FlagsStr)
{
    LE_DEBUG("AP Info Index: %d", Index);
    LE_DEBUG("Flags        : %s", FlagsStr.c_str());
    // Check for WPS
    if (FlagsStr.find(TAF_WLAN_WPS_STR) != std::string::npos)
    {
        LE_DEBUG("WPS: Enabled");
        CtxPtr->ApInfo[Index].WPSEnabled = true;
    }
    else
    {
        LE_DEBUG("WPS: Disabled");
        CtxPtr->ApInfo[Index].WPSEnabled = false;
    }

    // Check BSS/ESS
    if (FlagsStr.find(TAF_WLAN_ESS_STR) != std::string::npos)
    {
        LE_DEBUG("ESS");
        CtxPtr->ApInfo[Index].SS = TAF_WLAN_SS_EXTENDED;
    }
    else
    {
        LE_DEBUG("BSS");
        CtxPtr->ApInfo[Index].SS = TAF_WLAN_SS_BASIC;
    }

    // Check the security mode/protocol
    if (FlagsStr.find(TAF_WLAN_SEC_MODE_WEP_STR) != std::string::npos)
    {
        LE_DEBUG("SEC: WEP");
        CtxPtr->ApInfo[Index].secMode = TAF_WLAN_SEC_MODE_WEP;
    }
    else if (FlagsStr.find(TAF_WLAN_SEC_MODE_WPA3_STR) != std::string::npos)
    {
        LE_DEBUG("SEC: WPA3");
        CtxPtr->ApInfo[Index].secMode = TAF_WLAN_SEC_MODE_WPA3;
    }
    else if (FlagsStr.find(TAF_WLAN_SEC_MODE_WPA2_STR) != std::string::npos)
    {
        LE_DEBUG("SEC: WPA2");
        CtxPtr->ApInfo[Index].secMode = TAF_WLAN_SEC_MODE_WPA2;
    }
    else if (FlagsStr.find(TAF_WLAN_SEC_MODE_WPA_STR) != std::string::npos)
    {
        LE_DEBUG("SEC: WPA");
        CtxPtr->ApInfo[Index].secMode = TAF_WLAN_SEC_MODE_WPA;
    }
    else
    {
        LE_DEBUG("SEC: OPEN");
        CtxPtr->ApInfo[Index].secMode = TAF_WLAN_SEC_MODE_OPEN;
    }

    // For OPEN networks skip the authmode and encryption checks
    if (TAF_WLAN_SEC_MODE_OPEN == CtxPtr->ApInfo[Index].secMode)
    {
        LE_DEBUG("AUTH: NONE");
        CtxPtr->ApInfo[Index].secAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_NONE;
        LE_DEBUG("ENCRYPTION: UNKNOWN");
        CtxPtr->ApInfo[Index].secEncryptionMethod = TAF_WLAN_SEC_ENCRYPT_METHOD_UNKNOWN;
        return;
    }

    // Check Authentication Method
    if (FlagsStr.find(TAF_WLAN_SEC_AUTH_METHOD_EAP_STR) != std::string::npos)
    {
        LE_DEBUG("AUTH: EAP");
        if (FlagsStr.find(TAF_WLAN_SEC_AUTH_METHOD_EAP_SIM_STR) != std::string::npos)
        {
            LE_DEBUG("AUTH: EAP-SIM");
            CtxPtr->ApInfo[Index].secAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_EAP_SIM;
        }
        else if (FlagsStr.find(TAF_WLAN_SEC_AUTH_METHOD_EAP_AKA_STR) != std::string::npos)
        {
            LE_DEBUG("AUTH: EAP-AKA");
            CtxPtr->ApInfo[Index].secAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_EAP_AKA;
        }
        else if (FlagsStr.find(TAF_WLAN_SEC_AUTH_METHOD_EAP_LEAP_STR) != std::string::npos)
        {
            LE_DEBUG("AUTH: EAP-LEAP");
            CtxPtr->ApInfo[Index].secAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_EAP_LEAP;
        }
        else if (FlagsStr.find(TAF_WLAN_SEC_AUTH_METHOD_EAP_TLS_STR) != std::string::npos)
        {
            LE_DEBUG("AUTH: EAP-TLS");
            CtxPtr->ApInfo[Index].secAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_EAP_TLS;
        }
        else if (FlagsStr.find(TAF_WLAN_SEC_AUTH_METHOD_EAP_TTLS_STR) != std::string::npos)
        {
            LE_DEBUG("AUTH: EAP-TTLS");
            CtxPtr->ApInfo[Index].secAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_EAP_TTLS;
        }
        else if (FlagsStr.find(TAF_WLAN_SEC_AUTH_METHOD_EAP_PEAP_STR) != std::string::npos)
        {
            LE_DEBUG("AUTH: EAP-PEAP");
            CtxPtr->ApInfo[Index].secAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_EAP_PEAP;
        }
        else if (FlagsStr.find(TAF_WLAN_SEC_AUTH_METHOD_EAP_FAST_STR) != std::string::npos)
        {
            LE_DEBUG("AUTH: EAP-FAST");
            CtxPtr->ApInfo[Index].secAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_EAP_FAST;
        }
        else if (FlagsStr.find(TAF_WLAN_SEC_AUTH_METHOD_EAP_PSK_STR) != std::string::npos)
        {
            LE_DEBUG("AUTH: EAP-PSK");
            CtxPtr->ApInfo[Index].secAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_EAP_PSK;
        }
    }
    else if (FlagsStr.find(TAF_WLAN_SEC_AUTH_METHOD_PSK_STR) != std::string::npos)
    {
        LE_DEBUG("AUTH: PSK");
        CtxPtr->ApInfo[Index].secAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_PSK;
    }
    else if (FlagsStr.find(TAF_WLAN_SEC_AUTH_METHOD_SAE_STR) != std::string::npos)
    {
        LE_DEBUG("AUTH: SAE");
        CtxPtr->ApInfo[Index].secAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_SAE;
    }
    else
    {
        LE_DEBUG("AUTH: NONE");
        CtxPtr->ApInfo[Index].secAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_NONE;
        LE_DEBUG("ENCRYPTION: UNKNOWN");
        CtxPtr->ApInfo[Index].secEncryptionMethod = TAF_WLAN_SEC_ENCRYPT_METHOD_UNKNOWN;
        return;
    }

    // Encryption Methods
    if (FlagsStr.find(TAF_WLAN_SEC_ENCRYPT_METHOD_RC4_STR) != std::string::npos)
    {
        LE_DEBUG("ENCRYPTION: RC4");
        CtxPtr->ApInfo[Index].secEncryptionMethod = TAF_WLAN_SEC_ENCRYPT_METHOD_RC4;
    }
    else if (FlagsStr.find(TAF_WLAN_SEC_ENCRYPT_METHOD_TKIP_STR) != std::string::npos)
    {
        LE_DEBUG("ENCRYPTION: TKIP");
        CtxPtr->ApInfo[Index].secEncryptionMethod = TAF_WLAN_SEC_ENCRYPT_METHOD_TKIP;
    }
    else if (FlagsStr.find(TAF_WLAN_SEC_ENCRYPT_METHOD_AES_STR) != std::string::npos ||
             FlagsStr.find(TAF_WLAN_SEC_ENCRYPT_METHOD_CCMP_STR) != std::string::npos)
    {
        LE_DEBUG("ENCRYPTION: AES/CCMP");
        CtxPtr->ApInfo[Index].secEncryptionMethod = TAF_WLAN_SEC_ENCRYPT_METHOD_AES;
    }
    else if (FlagsStr.find(TAF_WLAN_SEC_ENCRYPT_METHOD_GCMP_STR) != std::string::npos)
    {
        LE_DEBUG("ENCRYPTION: GCMP");
        CtxPtr->ApInfo[Index].secEncryptionMethod = TAF_WLAN_SEC_ENCRYPT_METHOD_GCMP;
    }
    else
    {
        LE_DEBUG("ENCRYPTION: UNKNOWN");
        CtxPtr->ApInfo[Index].secEncryptionMethod = TAF_WLAN_SEC_ENCRYPT_METHOD_UNKNOWN;
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse SCAN_RESULTS response and populate taf_wlanSta_APInfo_t
 * Sample scan result to be parsed
 *
bssid / frequency / signal level / flags / ssid
00:1A:2B:3C:4D:5E       2447    -65     [WPA2-PSK-CCMP][WPS][ESS]       2GSSID
11:6F:7A:8B:9C:1D       5240    -48     [WPA2-PSK-CCMP][ESS]    5GSSDI1
22:2E:3F:4A:5B:6C       5785    -39     [WPA2-PSK-CCMP][ESS]    5GSSDI2
33:7D:8E:9F:1A:2B       5240    -84     [ESS]   Guest
44:3C:4D:5E:6F:7G       5240    -86     [WPA2-EAP-CCMP][ESS]    EAPSSID
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::PopulateScanResults(StaCtx_t *CtxPtr, const char *ScanResultsPtr)
{
    TAF_ERROR_IF_RET_NIL(CtxPtr == nullptr,
        "Null context pointer provided to PopulateScanResults");

    if (ScanResultsPtr == nullptr)
    {
        LE_ERROR("Null scan results pointer provided to PopulateScanResults");
        CtxPtr->numScannedAPs = 0;
        return;
    }

    std::string line;
    std::istringstream buf_stream(ScanResultsPtr);
    int iCount = 0;

    // Clear out earlier results
    CtxPtr->numScannedAPs = 0;
    memset(&CtxPtr->ApInfo[0], 0,
           (sizeof(taf_wlanSta_APInfo_t) * TAF_WLANSTA_MAX_APSCAN_RESULT_NUM));

    char delimiter = '\t';
    std::vector<std::string> tokens;
    std::string token;

    // Temporary vector to hold AP info before sorting
    std::vector<taf_wlanSta_APInfo_t> tempApInfo;

    while (std::getline(buf_stream, line, '\n'))
    {
        // Check if the line has "frequency". This is the heading line. skip it.
        if (line.find("frequency") != std::string::npos)
        {
            LE_DEBUG("Skip heading line");
            continue;
        }

        // Split line into separate fields
        {
            std::istringstream stream(line);
            tokens.clear();
            // The parameters are tab separated
            // Split the line into components and store in a vector
            while (std::getline(stream, token, delimiter))
            {
                tokens.push_back(token);
            }

            // Make sure we have enough tokens
            if (tokens.size() < 5) {
                LE_WARN("Invalid scan result line: %s", line.c_str());
                continue;
            }

            // Create a temporary AP info structure
            taf_wlanSta_APInfo_t apInfo;
            memset(&apInfo, 0, sizeof(taf_wlanSta_APInfo_t));

            // BSSID
            const char* bssidStr = tokens[0].c_str();
            if (bssidStr != nullptr && strlen(bssidStr) > 0) {
                if (strlen(bssidStr) >= TAF_WLAN_MAX_BSSID_LENGTH) {
                    LE_WARN("BSSID truncated (source length: %zu, max length: %d)",
                            strlen(bssidStr), TAF_WLAN_MAX_BSSID_LENGTH);
                }
                le_result_t result = le_utf8_Copy(apInfo.BSSID, bssidStr,
                                                 TAF_WLAN_MAX_BSSID_LENGTH + 1, NULL);
                if (result != LE_OK) {
                    LE_ERROR("Failed to copy BSSID: %s", LE_RESULT_TXT(result));
                    apInfo.BSSID[0] = '\0';
                }
            } else {
                apInfo.BSSID[0] = '\0';
            }

            // Frequency - with error checking
            try {
                apInfo.Frequency = std::stoi(tokens[1]);
            } catch (const std::exception& e) {
                LE_WARN("Failed to parse frequency: %s", e.what());
                apInfo.Frequency = 0;
            }

            // Signal Level - with error checking
            try {
                apInfo.SignalLevel = std::stoi(tokens[2]);
            } catch (const std::exception& e) {
                LE_WARN("Failed to parse signal level: %s", e.what());
                apInfo.SignalLevel = 0;
            }

            // Parse flags and populate relevant elements
            ParseScanResultsFlags(CtxPtr, iCount, tokens[3]);

            // Copy the flags parsing results to our temporary AP info
            apInfo.SS = CtxPtr->ApInfo[iCount].SS;
            apInfo.secMode = CtxPtr->ApInfo[iCount].secMode;
            apInfo.secAuthMethod = CtxPtr->ApInfo[iCount].secAuthMethod;
            apInfo.secEncryptionMethod = CtxPtr->ApInfo[iCount].secEncryptionMethod;
            apInfo.WPSEnabled = CtxPtr->ApInfo[iCount].WPSEnabled;

            // SSID
            const char* ssidStr = tokens[4].c_str();
            if (ssidStr != nullptr && strlen(ssidStr) > 0) {
                if (strlen(ssidStr) >= TAF_WLAN_MAX_SSID_LENGTH) {
                    LE_WARN("SSID truncated (source length: %zu, max length: %d)",
                            strlen(ssidStr), TAF_WLAN_MAX_SSID_LENGTH);
                }
                le_result_t result = le_utf8_Copy(apInfo.SSID, ssidStr,
                                                 TAF_WLAN_MAX_SSID_LENGTH + 1, NULL);
                if (result != LE_OK) {
                    LE_ERROR("Failed to copy SSID: %s", LE_RESULT_TXT(result));
                    apInfo.SSID[0] = '\0';
                }
            } else {
                apInfo.SSID[0] = '\0';
            }

            // Add to our temporary vector
            tempApInfo.push_back(apInfo);
            iCount++;
        }
    }

    // Sort the AP list by signal strength (strongest signal first)
    // Note: Signal strength is negative, so we sort in ascending order
    std::sort(tempApInfo.begin(), tempApInfo.end(),
        [](const taf_wlanSta_APInfo_t& a, const taf_wlanSta_APInfo_t& b) {
            return a.SignalLevel > b.SignalLevel;
        });

    // Copy the sorted results back to the context
    size_t numAPs = std::min(tempApInfo.size(),
        static_cast<size_t>(TAF_WLANSTA_MAX_APSCAN_RESULT_NUM));
    for (size_t i = 0; i < numAPs; ++i) {
        CtxPtr->ApInfo[i] = tempApInfo[i];

        // Debug output for sorted results
        LE_DEBUG("Sorted AP[%zu]: SSID=%s, Signal=%d dBm",
                 i, CtxPtr->ApInfo[i].SSID, CtxPtr->ApInfo[i].SignalLevel);
    }

    CtxPtr->numScannedAPs = static_cast<uint16_t>(numAPs);
    LE_INFO("Number of APs found and sorted: %d", CtxPtr->numScannedAPs);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends command to wpa_supplicant control interface to execute
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::runWPACommand(
    StaCtx_t *CtxPtr,
    const char *cmd,
    char *response,
    const size_t responseSize
)
{
    std::string wpa_supplicant_path(WPA_SUPPLICANT_LOCATION_PATH);
    wpa_supplicant_path = wpa_supplicant_path + CtxPtr->IntfName;

    LE_DEBUG("Supplicant: %s", wpa_supplicant_path.c_str());

    // Open wpa control interface
    struct wpa_ctrl *ctrl = wpa_ctrl_open(wpa_supplicant_path.c_str());
    if (ctrl == nullptr)
    {
        LE_ERROR("Error: %s", strerror(errno));
        LE_ERROR("Failed to open control interface");
        return LE_FAULT;
    }

    if(!CheckCommunication(CtxPtr, ctrl))
    {
        LE_ERROR("No communication exist");
        wpa_ctrl_close(ctrl);
        return LE_FAULT;
    }

    char buf[WPA_CTRL_RSP_BUF_LEN] = { 0 };
    size_t len = WPA_CTRL_RSP_BUF_LEN;
    LE_DEBUG("Running cmd: %s", cmd);
    int ret = wpa_ctrl_request(ctrl, cmd, strlen(cmd), buf, &len, nullptr);
    if (ret < 0)
    {
        if (ret == -2)
        {
            LE_ERROR("Timeout while waiting for GET_NETWORK response");
        }
        else
        {
            LE_ERROR("Failed to send GET_NETWORK command");
        }
        wpa_ctrl_close(ctrl);
        return LE_FAULT;
    }

    LE_DEBUG("Response Len : %zu", len);
    LE_DEBUG("Response     : %s", buf);

    if(response != nullptr)
    {
        le_utf8_Copy(response, buf, responseSize, nullptr);
    }

    wpa_ctrl_close(ctrl);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if network is already added
 * @return
 * - Network ID     -- If already added.
 * - "NOT_ADDED"    -- If not added.
 * - "FAIL"         -- On failure
 */
//--------------------------------------------------------------------------------------------------
std::string taf_WlanSTASvcImpl::CheckNetworkAdded(
    StaCtx_t *CtxPtr,
    const taf_wlanSta_APInfo_t* LE_NONNULL ApInfo
)
{
    std::string res(WPA_STA_NET_NOT_ADDED);
    std::string givenSSID(ApInfo->SSID);

    // check if a network corresponding to given SSID is already added
    for(int netID = 0; netID < WPA_STA_MAX_NETID; ++netID)
    {
        char buf[WPA_CTRL_RSP_BUF_LEN] = { 0 };

        std::string wpaReqCmd = "GET_NETWORK " + std::to_string(netID) + " ssid";
        le_result_t ret = runWPACommand(CtxPtr, wpaReqCmd.c_str(), buf, sizeof(buf));
        if (ret == LE_FAULT)
        {
            break;
        }

        std::string ssid(buf);
        if(ssid.substr(0, 4) == "FAIL")
        {
            LE_ERROR("GET_NETWORK for SSID %d failed", netID);
            break;
        }

        // we will get SSID with double quotes, so we need to remove them
        ssid = ssid.substr(1, ssid.length() - 2);
        LE_DEBUG("GET_NETWORK ssid: %s", ssid.c_str());

        if(ssid == givenSSID)
        {
            LE_INFO("%s found in added networks, netID: %d", givenSSID.c_str(), netID);
            res = std::to_string(netID);
            return res;
        }
    }

    LE_INFO("%s not found in added networks", givenSSID.c_str());
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check communication with a PING Request
 */
//--------------------------------------------------------------------------------------------------
bool taf_WlanSTASvcImpl::CheckCommunication
(
    StaCtx_t *CtxPtr,
    struct wpa_ctrl *ctrl
)
{
    LE_DEBUG("WPA CMD: PING");
    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};
    size_t rsp_len = WPA_CTRL_RSP_BUF_LEN;
    int ret = wpa_ctrl_request(ctrl, "PING", strlen("PING"), rsp_buf, &rsp_len, nullptr);
    if (ret < 0)
    {
        if (ret == -2)
        {
            LE_ERROR("Timeout while waiting for response");
        }
        LE_ERROR("Failed to send command");
        return false;
    }
    LE_DEBUG("PING RSP Len: %zu", rsp_len);
    LE_DEBUG("PING RSP: %s", rsp_buf);
    if (strncmp(rsp_buf, "PONG", strlen("PONG")) != 0)
    {
        LE_ERROR("PONG not received. %s", rsp_buf);
        return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Perform STA BSS scanning.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::PerformScan(StaCtx_t *CtxPtr)
{
    TAF_ERROR_IF_RET_NIL(CtxPtr == nullptr, "Null context pointer provided to PerformScan");

    // Check if interface name is valid
    if (CtxPtr->IntfName == nullptr || strlen(CtxPtr->IntfName) == 0)
    {
        LE_ERROR("Invalid interface name in context");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }

    // Start a thread of monitoring WPA control events
    le_thread_Ref_t supplicantThreadRef = nullptr;
    // Pass the interface to use and event that we are waiting for.
    SuppThreadCtx_t ThreadCtx;

    le_result_t result = le_utf8_Copy(ThreadCtx.IntfName, CtxPtr->IntfName,
                                     TAF_NET_INTERFACE_NAME_MAX_LEN + 1, nullptr);
    if (result != LE_OK) {
        LE_ERROR("Failed to copy interface name: %s", LE_RESULT_TXT(result));
        // Ensure null termination
        ThreadCtx.IntfName[0] = '\0';
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }

    ThreadCtx.eventCompleted = false;
    ThreadCtx.shouldExit = false;

    ThreadCtx.EventsToMonitor.push_back(WPA_EVENT_SCAN_RESULTS);
    supplicantThreadRef = le_thread_Create("suppThread", StaWpaSuppMonitorThreadHdlr,
                                                                          (void *)&ThreadCtx);

    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};

    // Start SCAN
    LE_DEBUG("WPA CMD: SCAN");
    memset(rsp_buf, 0, WPA_CTRL_RSP_BUF_LEN);
    std::string wpaReqCmd = "SCAN";
    le_result_t res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    if(res != LE_OK)
    {
        LE_ERROR("Failed to send SCAN command");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }
    if (0 != strncmp(rsp_buf, "OK", strlen("OK")))
    {
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }

    le_thread_Start(supplicantThreadRef);

    // SCAN is running. Wait for it to complete with timeout
    const int MAX_WAIT_TIME_SEC = 15;
    const int POLL_INTERVAL_MS = 100;
    int elapsed_ms = 0;
    bool eventCompleted = false;
    StaWpaEvt_e resultEvent;

    while (elapsed_ms < (MAX_WAIT_TIME_SEC * 1000))
    {
        {
            // Use mutex to safely check shared state
            std::lock_guard<std::mutex> lock(ThreadCtx.mutex);
            eventCompleted = ThreadCtx.eventCompleted;
            if (eventCompleted)
            {
                resultEvent = ThreadCtx.resultEvent;
                break;
            }
        }
        usleep(POLL_INTERVAL_MS * 1000);
        elapsed_ms += POLL_INTERVAL_MS;
    }

    if (!eventCompleted)
    {
        LE_ERROR("Scan operation timed out");
        ThreadCtx.shouldExit = true;
        le_thread_Join(supplicantThreadRef, nullptr);
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }

    // Wait for thread to finish
    le_thread_Join(supplicantThreadRef, nullptr);

    // Check the SCAN completion return
    if (resultEvent != EVT_WPA_AP_SCAN_DONE)
    {
        LE_ERROR("WPA AP scan failed");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }

    // Get the scan results
    memset(rsp_buf, 0, WPA_CTRL_RSP_BUF_LEN);
    wpaReqCmd = "SCAN_RESULTS";
    res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    if(res != LE_OK)
    {
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }

    // SCAN_RESULTS are in! Store them in the relevant context
    PopulateScanResults(CtxPtr, rsp_buf);

    // Report scan completed and clean up
    ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_COMPLETED);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets WPA2 PSK for an Access Point.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::SetWpa2Psk(
    taf_wlanSta_WlanSTARef_t staRef,
    const taf_wlanSta_APInfo_t* LE_NONNULL ApInfo,
    const char* LE_NONNULL psk
)
{
    if(ApInfo->secAuthMethod != TAF_WLAN_SEC_AUTH_METHOD_PSK)
    {
        LE_INFO("Other authentication methods are not supported yet");
        return LE_UNSUPPORTED;
    }

    TAF_ERROR_IF_RET_VAL(wlanSTAMgr == nullptr, LE_FAULT, "WLAN STA Manager not initialized");

    StaCtx_t *CtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(CtxPtr == nullptr, LE_FAULT, "Unable to find context");

    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};

    std::string wpaReqCmd;
    le_result_t res;

    std::string netID = CheckNetworkAdded(CtxPtr, ApInfo);
    bool isNetNotAdded = (netID == WPA_STA_NET_NOT_ADDED);
    if(isNetNotAdded)
    {
        // Start ADD_NETWORK
        wpaReqCmd = "ADD_NETWORK";
        res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
        if(res == LE_FAULT)
        {
            ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
            return LE_FAULT;
        }
        netID = rsp_buf;
    }

    if(netID.substr(0, 4) == "FAIL")
    {
        LE_ERROR("ADD_NETWORK failed");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }

    LE_INFO("Added Network ID: %s", netID.c_str());
    mNetID = netID;

    if(!isNetNotAdded)
    {
        // If we found a network ID for given SSID, we will
        // select network ID to make it current one and Force reassociation.
        memset(rsp_buf, 0, WPA_CTRL_RSP_BUF_LEN);

        wpaReqCmd = "SELECT_NETWORK " + netID;
        res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
        if (res == LE_FAULT)
        {
            ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
            return LE_FAULT;
        }
        if (strncmp(rsp_buf, "OK", strlen("OK")) != 0)
        {
            LE_ERROR("SELECT_NETWORK failed");
            ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
            return LE_FAULT;
        }
    }

    // Set network name(SSID)
    LE_DEBUG("WPA CMD: SET_NETWORK(SSID)");
    memset(rsp_buf, 0, WPA_CTRL_RSP_BUF_LEN);
    wpaReqCmd = "SET_NETWORK " + netID + " ssid \"" + ApInfo->SSID + "\"";
    res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    if (res == LE_FAULT)
    {
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }
    if (strncmp(rsp_buf, "OK", strlen("OK")) != 0)
    {
        LE_ERROR("SET_NETWORK(SSID) failed");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }

    if(ApInfo->secAuthMethod == TAF_WLAN_SEC_AUTH_METHOD_PSK)
    {
        // Set network(psk)
        LE_DEBUG("WPA CMD: SET_NETWORK(psk)");
        memset(rsp_buf, 0, WPA_CTRL_RSP_BUF_LEN);
        wpaReqCmd = "SET_NETWORK " + netID + " psk \"" + psk + "\"";
        res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
        if (res == LE_FAULT)
        {
            ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
            return LE_FAULT;
        }
        if (strncmp(rsp_buf, "OK", strlen("OK")) != 0)
        {
            LE_ERROR("SET_NETWORK(psk) failed");
            ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Connects to an AP
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::APConnect(
    taf_wlanSta_WlanSTARef_t staRef,
    const taf_wlanSta_APInfo_t* LE_NONNULL ApInfo
)
{
    if(ApInfo->secAuthMethod != TAF_WLAN_SEC_AUTH_METHOD_PSK)
    {
        LE_INFO("Other authentication methods are not supported yet");
        return LE_UNSUPPORTED;
    }

    TAF_ERROR_IF_RET_VAL(!wlanSTAMgr, LE_FAULT, "WLAN STA Manager not initialized");

    StaCtx_t *staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(!staCtxPtr, LE_FAULT, "Unable to find context");

    // Ensure STA is active
    std::vector<telux::wlan::StaStatus> status;
    telux::common::ErrorCode errCode = wlanSTAMgr->getStatus(status);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA getStatus failed with error : %d", static_cast<int>(errCode));
        return LE_FAULT;
    }
    for (auto &element : status)
    {
        LE_DEBUG("------------------------------------------");
        LE_DEBUG("STA Id: %d", static_cast<int>(element.id));
        if (taf_WlanHelper::TAFSTAidtoTeluxId(staCtxPtr->id) == element.id)
        {
            if (telux::wlan::StaInterfaceStatus::UNKNOWN == element.status)
            {
                LE_WARN("STA %d status known", staCtxPtr->id);
                return LE_FAULT;
            }
            else
            {
                // STA sate is good
                break;
            }
        }
    }

    staCtxPtr->ApInfoConnect = *ApInfo;

    // Send cmd event to connect to AP
    StaCmd_t cmd = {staCtxPtr, CMD_WPA_DO_AP_CONNECT};
    le_event_Report(staCommand_, &cmd, sizeof(StaCmd_t));
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect STA to any Access Point.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::Connect(
    StaCtx_t *CtxPtr,
    const taf_wlanSta_APInfo_t* LE_NONNULL ApInfo
)
{
    TAF_ERROR_IF_RET_VAL(CtxPtr == nullptr, LE_FAULT, "Received null context");
    TAF_ERROR_IF_RET_VAL(0 == strlen(ApInfo->SSID), LE_BAD_PARAMETER, "SSID is empty");

    // The network should have been added before connect is called.
    if (mNetID.empty())
    {
        LE_ERROR("Network not added");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }

    // Start separate thread for monitoring required WPA events
    le_thread_Ref_t networkConnectedThreadRef = nullptr;
    SuppThreadCtx_t ThreadCtx;

    if (CtxPtr->IntfName != nullptr && strlen(CtxPtr->IntfName) > 0)
    {
        le_result_t result = le_utf8_Copy(ThreadCtx.IntfName, CtxPtr->IntfName,
                                         TAF_NET_INTERFACE_NAME_MAX_LEN + 1, NULL);
        if (result != LE_OK)
        {
            LE_ERROR("Failed to copy interface name: %s", LE_RESULT_TXT(result));
            // Ensure null termination
            ThreadCtx.IntfName[0] = '\0';
            ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
            return LE_FAULT;
        }
    }
    else
    {
        LE_ERROR("Interface name is null or empty");
        ThreadCtx.IntfName[0] = '\0';
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }

    ThreadCtx.eventCompleted = false;
    ThreadCtx.shouldExit = false;

    ThreadCtx.EventsToMonitor.push_back(WPA_EVENT_CONNECTED);
    ThreadCtx.EventsToMonitor.push_back(WPA_EVENT_TEMP_DISABLED);

    networkConnectedThreadRef = le_thread_Create("connThread",
        StaWpaSuppMonitorThreadHdlr, (void *)&ThreadCtx);
    le_thread_Start(networkConnectedThreadRef);

    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};
    // enable network
    memset(rsp_buf, 0, WPA_CTRL_RSP_BUF_LEN);
    std::string wpaReqCmd = "ENABLE_NETWORK " + mNetID;
    le_result_t res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    if(res == LE_FAULT)
    {
        ThreadCtx.shouldExit = true;
        le_thread_Join(networkConnectedThreadRef, nullptr);
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }
    if (strncmp(rsp_buf, "OK", strlen("OK")) != 0)
    {
        LE_ERROR("ENABLE_NETWORK failed");
        ThreadCtx.shouldExit = true;
        le_thread_Join(networkConnectedThreadRef, nullptr);
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }

    // Force reassociation(for cases where Network ID was already added)
    memset(rsp_buf, 0, WPA_CTRL_RSP_BUF_LEN);
    wpaReqCmd = "REASSOCIATE";
    res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    if(res == LE_FAULT)
    {
        ThreadCtx.shouldExit = true;
        le_thread_Join(networkConnectedThreadRef, nullptr);
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }
    if (strncmp(rsp_buf, "OK", strlen("OK")) != 0)
    {
        LE_ERROR("REASSOCIATE failed");
        ThreadCtx.shouldExit = true;
        le_thread_Join(networkConnectedThreadRef, nullptr);
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }

    // Wait for connection result with timeout
    const int MAX_WAIT_TIME_SEC = 15;
    const int POLL_INTERVAL_MS = 100;
    int elapsed_ms = 0;
    bool eventCompleted = false;
    StaWpaEvt_e resultEvent;

    while (elapsed_ms < (MAX_WAIT_TIME_SEC * 1000))
    {
        {
            // Use mutex to safely check shared state
            std::lock_guard<std::mutex> lock(ThreadCtx.mutex);
            eventCompleted = ThreadCtx.eventCompleted;
            if (eventCompleted)
            {
                resultEvent = ThreadCtx.resultEvent;
                break;
            }
        }
        usleep(POLL_INTERVAL_MS * 1000);
        elapsed_ms += POLL_INTERVAL_MS;
    }

    if (!eventCompleted)
    {
        LE_TEST_INFO("Timeout waiting for connection event");
        ThreadCtx.shouldExit = true;
    }

    // Wait for thread to finish
    le_thread_Join(networkConnectedThreadRef, nullptr);

    if (eventCompleted)
    {
        if (resultEvent == EVT_WPA_AP_CONNECTED)
        {
            LE_INFO("Connection to AP %s was successful", ApInfo->SSID);
            // TAF_WLANSTA_STATE_CONNECTED event will be sent from onStationStatusChanged
            return LE_OK;
        }
        else if (resultEvent == EVT_WPA_AP_TEMP_DISABLED)
        {
            LE_ERROR("Authentication failed for AP %s", ApInfo->SSID);
            ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
            return LE_FAULT;
        }
    }

    // Default case - timeout or other error
    LE_WARN("Connection attempt to AP %s timed out or failed", ApInfo->SSID);
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disconnects from an AP
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::APDisconnect(
    taf_wlanSta_WlanSTARef_t staRef,
    const taf_wlanSta_APInfo_t* LE_NONNULL ApInfo
)
{
    TAF_ERROR_IF_RET_VAL(!wlanSTAMgr, LE_FAULT, "WLAN STA Manager not initialized");

    StaCtx_t *staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(!staCtxPtr, LE_FAULT, "Unable to find context");

    // Ensure STA is active
    std::vector<telux::wlan::StaStatus> status;
    telux::common::ErrorCode errCode = wlanSTAMgr->getStatus(status);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA getStatus failed with error : %d", static_cast<int>(errCode));
        return LE_FAULT;
    }
    for (auto &element : status)
    {
        LE_DEBUG("------------------------------------------");
        LE_DEBUG("STA Id: %d", static_cast<int>(element.id));
        if (taf_WlanHelper::TAFSTAidtoTeluxId(staCtxPtr->id) == element.id)
        {
            if (telux::wlan::StaInterfaceStatus::UNKNOWN == element.status)
            {
                LE_WARN("STA %d status known", staCtxPtr->id);
                return LE_FAULT;
            }
            else
            {
                // STA sate is good
                break;
            }
        }
    }

    staCtxPtr->ApInfoConnect = *ApInfo;

    // Send cmd event to disconnect from AP
    StaCmd_t cmd = {staCtxPtr, CMD_WPA_DO_AP_DISCONNECT};
    le_event_Report(staCommand_, &cmd, sizeof(StaCmd_t));
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disconnects from an Access Point.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::Disconnect(
    StaCtx_t *CtxPtr,
    const taf_wlanSta_APInfo_t* LE_NONNULL ApInfo
)
{
    TAF_ERROR_IF_RET_VAL(CtxPtr == nullptr, LE_FAULT, "Received null context");

    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};

    std::string netID = CheckNetworkAdded(CtxPtr, ApInfo);
    if((netID == WPA_STA_NET_NOT_ADDED) || (netID.substr(0, 4) == "FAIL"))
    {
        LE_ERROR("Network not found in connected networks");
        return LE_FAULT;
    }

    LE_INFO("Network ID: %s", netID.c_str());

    // Disconnect network
    LE_DEBUG("WPA CMD: DISCONNECT");
    memset(rsp_buf, 0, WPA_CTRL_RSP_BUF_LEN);
    std::string wpaReqCmd = "DISCONNECT";
    le_result_t res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    if (res == LE_FAULT)
    {
        LE_ERROR("DISCONNECT failed");
        return LE_FAULT;
    }
    if (strncmp(rsp_buf, "OK", strlen("OK")) != 0)
    {
        LE_ERROR("DISCONNECT failed");
        return LE_FAULT;
    }

    // disable network
    memset(rsp_buf, 0, WPA_CTRL_RSP_BUF_LEN);
    wpaReqCmd = "DISABLE_NETWORK " + netID;
    res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    if (res == LE_FAULT)
    {
        return LE_FAULT;
    }
    if (strncmp(rsp_buf, "OK", strlen("OK")) != 0)
    {
        LE_ERROR("DISABLE_NETWORK failed");
        return LE_FAULT;
    }

    // TAF_WLANSTA_STATE_DISCONNECTED event will be sent from onStationStatusChanged
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal STA events handler
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::StaCmdHandler(void *StaCmdPtr)
{
    StaCmd_t *cmdPtr = (StaCmd_t *)StaCmdPtr;
    LE_DEBUG("STA ID  : %d", cmdPtr->CtxPtr->id);
    LE_DEBUG("Command : %d", cmdPtr->cmd);
    switch (cmdPtr->cmd)
    {
        case CMD_WPA_DO_SCAN:
        {
            auto &myWlanSta = GetInstance();
            le_mutex_Lock(myWlanSta.STACtxMutex);
            myWlanSta.ReportStaState(cmdPtr->CtxPtr, TAF_WLANSTA_STATE_SCAN_STARTED);
            myWlanSta.PerformScan(cmdPtr->CtxPtr);
            le_mutex_Unlock(myWlanSta.STACtxMutex);
            break;
        }
        case CMD_WPA_DO_AP_CONNECT:
        {
            auto &myWlanSta = GetInstance();
            le_mutex_Lock(myWlanSta.STACtxMutex);
            StaCtx_t *ctxPtr = cmdPtr->CtxPtr;
            myWlanSta.ReportStaState(ctxPtr, TAF_WLANSTA_STATE_CONNECTING);
            myWlanSta.Connect(ctxPtr, &(ctxPtr->ApInfoConnect));
            le_mutex_Unlock(myWlanSta.STACtxMutex);
            break;
        }
        case CMD_WPA_DO_AP_DISCONNECT:
        {
            auto &myWlanSta = GetInstance();
            le_mutex_Lock(myWlanSta.STACtxMutex);
            StaCtx_t *ctxPtr = cmdPtr->CtxPtr;
            myWlanSta.Disconnect(ctxPtr, &(ctxPtr->ApInfoConnect));
            le_mutex_Unlock(myWlanSta.STACtxMutex);
            break;
        }
        case CMD_WPA_START_LINK_MONITORING:
        {
            auto &myWlanSta = GetInstance();
            if (myWlanSta.sigStrengthMonitorRefMap_[cmdPtr->CtxPtr->id]->Start())
            {
                LE_DEBUG("Signal monitoring started for STA: %d", cmdPtr->CtxPtr->id);
                // Schedule any stopped client event threads to be restarted.
                le_event_QueueFunctionToThread(myWlanSta.staClientEventsThreadRef_,
                            myWlanSta.queueStartApSigStrengthClientEventTimers, nullptr, nullptr);
            }
            break;
        }
        case CMD_WPA_STOP_LINK_MONITORING:
        {
            auto &myWlanSta = GetInstance();
            if (myWlanSta.sigStrengthMonitorRefMap_[cmdPtr->CtxPtr->id]->Stop())
            {
                LE_DEBUG("Signal monitoring stopped for STA: %d", cmdPtr->CtxPtr->id);
                // Schedule any running client event threads to be stopped.
                le_event_QueueFunctionToThread(myWlanSta.staClientEventsThreadRef_,
                            myWlanSta.queueStopApSigStrengthClientEventTimers, nullptr, nullptr);
            }
            break;
        }
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Starts the specified station.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::Start(taf_wlanSta_WlanSTARef_t staRef)
{
    StaCtx_t *staCtxPtr = NULL;
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;

    TAF_ERROR_IF_RET_VAL(nullptr == wlanSTAMgr, LE_FAULT, "WLAN STA Manager not initialized");

    staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    errCode = wlanSTAMgr->manageStaService( taf_WlanHelper::TAFSTAidtoTeluxId(staCtxPtr->id),
                                            telux::wlan::ServiceOperation::START);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA Start failed with error : %d", static_cast<int>(errCode));
        return LE_FAULT;
    }
    LE_INFO ("WLAN STA Start success");
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Stops the specified Station.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::Stop(taf_wlanSta_WlanSTARef_t staRef)
{
    StaCtx_t *staCtxPtr = NULL;
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;

    TAF_ERROR_IF_RET_VAL(nullptr == wlanSTAMgr, LE_FAULT, "WLAN STA Manager not initialized");

    staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    errCode = wlanSTAMgr->manageStaService( taf_WlanHelper::TAFSTAidtoTeluxId(staCtxPtr->id),
                                            telux::wlan::ServiceOperation::STOP);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA Stop failed with error : %d", static_cast<int>(errCode));
        return LE_FAULT;
    }
    LE_INFO ("WLAN STA Stop success");
    return LE_OK;

}
//--------------------------------------------------------------------------------------------------
/**
 * Restarts the specified Station.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::Restart(taf_wlanSta_WlanSTARef_t staRef)
{
    StaCtx_t *staCtxPtr = NULL;
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;

    TAF_ERROR_IF_RET_VAL(nullptr == wlanSTAMgr, LE_FAULT, "WLAN STA Manager not initialized");

    staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    errCode = wlanSTAMgr->manageStaService( taf_WlanHelper::TAFSTAidtoTeluxId(staCtxPtr->id),
                                            telux::wlan::ServiceOperation::RESTART);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA Restart failed with error : %d", static_cast<int>(errCode));
        return LE_FAULT;
    }
    LE_INFO ("WLAN STA Restart success");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the station to Bridged or Router mode.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::SetMode(
    taf_wlanSta_WlanSTARef_t staRef,
    ///< [IN] The WLAN STA reference.
    taf_wlanSta_Mode_t StaMode
    ///< [IN] The WLAN STA mode to set.
)
{
    StaCtx_t *staCtxPtr = NULL;
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;

    TAF_ERROR_IF_RET_VAL(nullptr == wlanSTAMgr, LE_FAULT, "WLAN STA Manager not initialized");

    staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    errCode = wlanSTAMgr->setBridgeMode( taf_WlanHelper::TAFSTAidtoTeluxId(staCtxPtr->id),
                                         taf_WlanHelper::StaModeToTelux(StaMode));
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA SetMode failed with error : %d", static_cast<int>(errCode));
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the station mode.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::GetMode
(
    taf_wlanSta_WlanSTARef_t staRef,
    ///< [IN] The WLAN STA reference.
    taf_wlanSta_Mode_t *StaModePtr
    ///< [OUT] The WLAN STA mode that is set.
)
{
    StaCtx_t *staCtxPtr = NULL;
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;

    TAF_ERROR_IF_RET_VAL(nullptr == wlanSTAMgr, LE_FAULT, "WLAN STA Manager not initialized");

    staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    std::vector<telux::wlan::StaConfig> config;
    errCode = wlanSTAMgr->getConfig(config);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA GetMode failed with error : %d", static_cast<int>(errCode));
        return LE_FAULT;
    }
    for (auto &cfg : config)
    {
        LE_DEBUG("------------------------------------------");
        LE_DEBUG("STA Id: %d", static_cast<int>(cfg.staId));
        if (taf_WlanHelper::TAFSTAidtoTeluxId(staCtxPtr->id) == cfg.staId)
        {
            *StaModePtr = taf_WlanHelper::StaModeToTAF(cfg.bridgeMode);
            break;
        }
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the IP configuration of the station.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::SetIPConfig
(
    taf_wlanSta_WlanSTARef_t staRef,
    ///< [IN] The WLAN STA reference.
    taf_wlanSta_IPType_t StaIPType,
    const taf_wlanSta_IPConfig_t *LE_NONNULL StaStaticIPConfigPtr)
{
    StaCtx_t *staCtxPtr = NULL;
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;

    TAF_ERROR_IF_RET_VAL(nullptr == wlanSTAMgr, LE_FAULT, "WLAN STA Manager not initialized");

    staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    telux::wlan::StaStaticIpConfig staticIpConfig;
    if (TAF_WLANSTA_IPTYPE_STATIC == StaIPType)
    {
        LE_INFO("Static IP configuration");
        // Static IP. Populate the static IP structure.
        staticIpConfig.ipAddr   = StaStaticIPConfigPtr->IPv4Addr;
        staticIpConfig.gwIpAddr = StaStaticIPConfigPtr->GWAddr;
        staticIpConfig.netMask  = StaStaticIPConfigPtr->NetMask;
        staticIpConfig.dnsAddr  = StaStaticIPConfigPtr->DNSAddr;
    }
    errCode = wlanSTAMgr->setIpConfig(taf_WlanHelper::TAFSTAidtoTeluxId(staCtxPtr->id),
                                      taf_WlanHelper::StaIPTypeToTelux(StaIPType),
                                      staticIpConfig);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA SetMode failed with error : %d", static_cast<int>(errCode));
        return LE_FAULT;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the IP configuration of the station.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::GetIPConfig
(
    taf_wlanSta_WlanSTARef_t staRef,
    ///< [IN] The WLAN STA reference.
    taf_wlanSta_IPType_t *StaIPTypePtr,
    ///< [OUT] Dynamic or Static IP address.
    taf_wlanSta_IPConfig_t *StaStaticIPConfigPtr
    ///< [OUT] Details of static IP configuration.
)
{
    StaCtx_t *staCtxPtr = NULL;
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;

    TAF_ERROR_IF_RET_VAL(nullptr == wlanSTAMgr, LE_FAULT, "WLAN STA Manager not initialized");

    staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    std::vector<telux::wlan::StaConfig> config;
    errCode = wlanSTAMgr->getConfig(config);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA GetMode failed with error : %d", static_cast<int>(errCode));
        return LE_FAULT;
    }
    for (auto &cfg : config)
    {
        LE_DEBUG("------------------------------------------");
        LE_DEBUG("STA Id: %d", static_cast<int>(cfg.staId));
        if (taf_WlanHelper::TAFSTAidtoTeluxId(staCtxPtr->id) == cfg.staId)
        {
            *StaIPTypePtr = taf_WlanHelper::StaIPTypeToTAF(cfg.ipConfig);
            if (StaStaticIPConfigPtr && telux::wlan::StaIpConfig::STATIC_IP==cfg.ipConfig)
            {
                le_result_t ret = LE_OK;
                LE_DEBUG ("IPv4Addr :%s",cfg.staticIpConfig.ipAddr.c_str());
                ret = le_utf8_Copy(StaStaticIPConfigPtr->IPv4Addr,cfg.staticIpConfig.ipAddr.c_str(),
                                               TAF_NET_IPV4_ADDR_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("IPv4Addr copy error: %d", ret);
                }
                LE_DEBUG ("GWAddr :%s",cfg.staticIpConfig.gwIpAddr.c_str());
                ret = le_utf8_Copy(StaStaticIPConfigPtr->GWAddr,cfg.staticIpConfig.gwIpAddr.c_str(),
                                               TAF_NET_IPV4_ADDR_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("GWAddr copy error: %d", ret);
                }
                LE_DEBUG ("DNSAddr :%s",cfg.staticIpConfig.dnsAddr.c_str());
                ret = le_utf8_Copy(StaStaticIPConfigPtr->DNSAddr,cfg.staticIpConfig.dnsAddr.c_str(),
                                               TAF_NET_IPV4_ADDR_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("DNSAddr copy error: %d", ret);
                }
                LE_DEBUG ("NetMask :%s",cfg.staticIpConfig.netMask.c_str());
                ret = le_utf8_Copy(StaStaticIPConfigPtr->NetMask,cfg.staticIpConfig.netMask.c_str(),
                                               TAF_NET_IPV4_ADDR_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("NetMask copy error: %d", ret);
                }
            }
            break;
        }
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the station state.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::GetStatus
(
    taf_wlanSta_WlanSTARef_t staRef,
    ///< [IN] The WLAN STA reference.
    taf_wlanSta_State_t *StaSatePtr,
    ///< [OUT] Station state.
    char *IntfName,
    ///< [OUT] Assocaited host interface name.
    size_t IntfNameSize,
    ///< [IN]
    char *IPv4Address,
    ///< [OUT] Assocaited IPv4 address.
    size_t IPv4AddressSize,
    ///< [IN]
    char *IPv6Address,
    ///< [OUT] Assocaited IPv6 address.
    size_t IPv6AddressSize,
    ///< [IN]
    char *MACAddress,
    ///< [OUT] Assocaited MAC address.
    size_t MACAddressSize
    ///< [IN]
)
{
    StaCtx_t *staCtxPtr = NULL;
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;

    TAF_ERROR_IF_RET_VAL(nullptr == wlanSTAMgr, LE_FAULT, "WLAN STA Manager not initialized");

    staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    std::vector<telux::wlan::StaStatus> status;
    errCode = wlanSTAMgr->getStatus(status);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA getStatus failed with error : %d", static_cast<int>(errCode));
        return LE_FAULT;
    }
    for (auto &element : status)
    {
        LE_DEBUG("------------------------------------------");
        LE_DEBUG("STA Id: %d", static_cast<int>(element.id));
        if (taf_WlanHelper::TAFSTAidtoTeluxId(staCtxPtr->id) == element.id)
        {
            le_result_t ret = LE_OK;
            *StaSatePtr = taf_WlanHelper::StaIntfStatusToTAF(element.status);
            if (IntfName && (IntfNameSize>0))
            {
                ret = le_utf8_Copy(IntfName, element.name.c_str(),
                                               TAF_NET_INTERFACE_NAME_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("IntfName copy error: %d", ret);
                }
            }
            if (IPv4Address && (IPv4AddressSize>0))
            {
                ret = le_utf8_Copy(IPv4Address, element.ipv4Address.c_str(),
                                               TAF_NET_IPV4_ADDR_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("IPv4Address copy error: %d", ret);
                }
            }
            if (IPv6Address && (IPv6AddressSize>0))
            {
                ret = le_utf8_Copy(IPv6Address, element.ipv6Address.c_str(),
                                               TAF_NET_IPV6_ADDR_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("IPv6Address copy error: %d", ret);
                }
            }
            if (MACAddress && (MACAddressSize>0))
            {
                ret = le_utf8_Copy(MACAddress, element.macAddress.c_str(),
                                               TAF_NET_IPV6_ADDR_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("MACAddress copy error: %d", ret);
                }
            }
            break;
        }
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get signal strength of the connected AP.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::SvcGetConnectedApSignalStrength
(
    taf_wlanSta_WlanSTARef_t wlanSTARef,
    int16_t *ssPtr
)
{
    TAF_ERROR_IF_RET_VAL(wlanSTARef == nullptr, LE_BAD_PARAMETER, "wlanSTARef is NULL!");
    TAF_ERROR_IF_RET_VAL(ssPtr      == nullptr, LE_BAD_PARAMETER, "signalStrengthPtr is NULL!");

    StaCtx_t *staCtxPtr = NULL;
    staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)wlanSTARef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    int16_t sigStrength = 0xFFFF;
    le_result_t result = sigStrengthMonitorRefMap_[staCtxPtr->id]->GetLastSignalStrength(
                                                                                    sigStrength);
    if (LE_OK == result)
    {
        *ssPtr = sigStrength;
    }
    else
    {
        LE_WARN("GetLastSignalStrength failed. Result:%d", result);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * First layer ConnectedApSignalStrength events handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::connectedApSignalStrengthEventFirstLayerHandler
(
    void *reportPtr,
    void *secondLayerHandlerFunc
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "reportPtr is NULL!");
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == NULL, "secondLayerHandlerFunc is NULL!");

    // The reportPtr is a StaConnectedApSignalStrengthEvt_t.
    StaConnectedApSignalStrengthEvt_t *eventPtr =
                                    static_cast<StaConnectedApSignalStrengthEvt_t *>(reportPtr);
    // The secondLayerHandlerFunc is taf_wlanSta_ConnectedApSignalStrengthHandlerFunc_t.
    // Call the application's handler.

    taf_wlanSta_ConnectedApSignalStrengthHandlerFunc_t clientHandlerFunc =
        (taf_wlanSta_ConnectedApSignalStrengthHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(eventPtr->staRef, eventPtr->signalStrength, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * The timer handler for connected AP signal strength notification to clients.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::apSigStrengthClientEventsTimerHandler(le_timer_Ref_t timerRef)
{
    TAF_ERROR_IF_RET_NIL(nullptr == timerRef, "timerRef is NULL!");

    // Get the context
    ConnectedApSigStrengthClientCtx_t *sigCtxPtr =
                        (ConnectedApSigStrengthClientCtx_t *)(le_timer_GetContextPtr(timerRef));
    TAF_ERROR_IF_RET_NIL(nullptr == sigCtxPtr, "sigCtxPtr is NULL!");
    LE_DEBUG("ConnectedApSigStrengthClientCtx_t: sigCtxPtr: %p", sigCtxPtr);

    ConnectedApSigStrengthEventIdKey_t key = {sigCtxPtr->key.threshold,
                                              sigCtxPtr->key.frequency,
                                              sigCtxPtr->key.bAverage};
    LE_DEBUG("Threshold: %d, Frequency: %d, Average: %s", key.threshold, key.frequency,
                                                            (key.bAverage) ? "true" : "false");

    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    le_result_t result = LE_FAULT;
    int16_t sigStrength = 0xFFFF;
    if (key.bAverage)
    {
        result = myWlanSta.sigStrengthMonitorRefMap_[sigCtxPtr->staId]->GetAverageSignalStrength (
                                                                        key.frequency, sigStrength);
    }
    else
    {
        result = myWlanSta.sigStrengthMonitorRefMap_[sigCtxPtr->staId]->GetLastSignalStrength(
                                                                                       sigStrength);
    }
    if (LE_OK != result)
    {
        LE_ERROR("GetSignalStrength failed: %d", result);
        return;
    }

    if (sigStrength <= key.threshold)
    {
        LE_DEBUG ("Measured %d <= threshold %d", sigStrength, key.threshold);
        StaConnectedApSignalStrengthEvt_t event = {sigCtxPtr->wlanSTARef, sigStrength};
        le_event_Report(sigCtxPtr->eventId, &event, sizeof(event));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to add signal thread handler in the context of staClientEventsThreadRef_.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::queueAddApSigStrengthClientEventTimer(void *param1Ptr, void *param2Ptr)
{

    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();

    ConnectedApSigStrengthClientCtx_t *sigCtxPtr = (ConnectedApSigStrengthClientCtx_t *)param1Ptr;
    if (nullptr == sigCtxPtr)
    {
        LE_ERROR("sigCtxPtr is NULL");
        // Complete promise with a nullptr
        myWlanSta.sigStrengthAddHandlerPromise_.set_value();
        return;
    }
    LE_INFO("ConnectedApSigStrengthClientCtx_t: %p", sigCtxPtr);

    ConnectedApSigStrengthEventIdKey_t key = {sigCtxPtr->key.threshold,
                                              sigCtxPtr->key.frequency,
                                              sigCtxPtr->key.bAverage};
    // Start a timer for this event.
    char nameStr[24] = {0};
    snprintf(nameStr, 23, "timer-%d-%d-%d", key.frequency, key.threshold, key.bAverage);
    le_timer_Ref_t timerRef = le_timer_Create(nameStr);
    if (timerRef)
    {
        le_timer_SetWakeup(timerRef, false);
        le_timer_SetRepeat(timerRef, 0);
        le_timer_SetMsInterval(timerRef, key.frequency * 1000);
        le_timer_SetHandler(timerRef, apSigStrengthClientEventsTimerHandler);
        le_timer_SetContextPtr(timerRef, sigCtxPtr);
        le_timer_Start(timerRef);
        LE_DEBUG("Started signal strength timer: %s", nameStr);
    }
    else
    {
        LE_ERROR("le_timer_Create failed. Clean up and exit.");
        sigCtxPtr->timerRef   = nullptr;
        // Complete the promise.
        myWlanSta.sigStrengthAddHandlerPromise_.set_value();
        return;
    }

    sigCtxPtr->timerRef   = timerRef;

    LE_DEBUG("timerRef   : %p", sigCtxPtr->timerRef);

    // Complete the promise.
    myWlanSta.sigStrengthAddHandlerPromise_.set_value();
}

//--------------------------------------------------------------------------------------------------
/**
 * Add signal strength notification handler for the connected AP.
 */
//--------------------------------------------------------------------------------------------------
taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t
taf_WlanSTASvcImpl::SvcAddConnectedApSignalStrengthHandler
(
    taf_wlanSta_WlanSTARef_t wlanSTARef,
    int16_t signalThreshold,
    uint16_t frequency,
    bool bAverage,
    taf_wlanSta_ConnectedApSignalStrengthHandlerFunc_t handlerFuncPtr,
    void *contextPtr,
    le_msg_SessionRef_t clientRef
)
{
    // Check error and return
    TAF_ERROR_IF_RET_VAL(nullptr == wlanSTARef, nullptr, "wlanSTARef is NULL!");
    TAF_ERROR_IF_RET_VAL(nullptr == handlerFuncPtr, nullptr, "handlerFuncPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(nullptr == clientRef, nullptr, "clientRef is NULL!");

    if (frequency < 5 || frequency > 300)
    {
        LE_WARN("frequency %d out of range. 5s will be used.", frequency);
        frequency = 5; // Default 5s as provided frequency is out of range
    }
    if (signalThreshold < -100 || signalThreshold > 0)
    {
        LE_WARN("signalThreshold %d out of range. -70 will be used.", signalThreshold);
        signalThreshold = -70; // Default -70 as provided signalThreshold is out of range
    }

    // Allocate signal strength context.
    ConnectedApSigStrengthClientCtx_t *addsigCtxPtr = nullptr;
    addsigCtxPtr = new (std::nothrow) ConnectedApSigStrengthClientCtx_t();
    // Check error and return
    TAF_ERROR_IF_RET_VAL(nullptr == addsigCtxPtr, nullptr, "Unable to allocate memory");

    StaCtx_t *staCtxPtr = nullptr;
    staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)wlanSTARef);
    // Check error and return
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, nullptr, "Unable to find context");
    TAF_ERROR_IF_RET_VAL( sigStrengthClientsAndHandlersRefMap_.size() >=
                            TAF_WLANSTA_MAX_CONNECTED_AP_SIGNAL_STRENGTH_CLIENTS,
                                    nullptr, "SignalStrength handler limit exceeded.");

    ConnectedApSigStrengthEventIdKey_t key = {signalThreshold, frequency,bAverage};
    LE_DEBUG("Threshold: %d, Frequency: %d, Average: %s", signalThreshold, frequency,
                                                                (bAverage) ? "true" : "false");

    le_event_Id_t eventId = sigStrengthMonitorRefMap_[staCtxPtr->id]->GetSignalStrengthEventId(key);

    char nameStr[24] = {0};
    // Use the client ref, frequency and threshold in the name.
    snprintf(nameStr, 23, "ApSigStrHdlr-%p-%d-%d", clientRef, signalThreshold, frequency);
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(nameStr, eventId,
                                                    connectedApSignalStrengthEventFirstLayerHandler,
                                                    (void *)handlerFuncPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);
    LE_DEBUG("Added new signal strength handler: %s", nameStr);

    LE_DEBUG("handlerRef : %p", handlerRef);
    LE_DEBUG("eventId    : %p", eventId);

    LE_INFO("Add ConnectedApSigStrengthClientCtx_t: %p", addsigCtxPtr);
    memset(addsigCtxPtr, 0, sizeof(ConnectedApSigStrengthClientCtx_t));
    addsigCtxPtr->staId            = staCtxPtr->id;
    addsigCtxPtr->key.threshold    = signalThreshold;
    addsigCtxPtr->key.frequency    = frequency;
    addsigCtxPtr->key.bAverage     = bAverage;
    addsigCtxPtr->wlanSTARef       = wlanSTARef;
    addsigCtxPtr->clientRef        = clientRef;
    addsigCtxPtr->eventId          = eventId;

    // Add the handler in the context of the client events thread.
    LE_DEBUG("Starting timer in client events thread context ...");
    sigStrengthAddHandlerPromise_ = std::promise<void>();
    std::future<void> fut = sigStrengthAddHandlerPromise_.get_future();
    // Queue to client events thread, staClientEventsThreadRef_
    le_event_QueueFunctionToThread(staClientEventsThreadRef_,
                                   queueAddApSigStrengthClientEventTimer, addsigCtxPtr, NULL);
    fut.get();

    LE_DEBUG("back in main thread.");
    // Check for errors
    if (nullptr == addsigCtxPtr->timerRef)
    {
        LE_ERROR("Adding timer handler failed.");
        LE_DEBUG("Release ConnectedApSigStrengthClientCtx_t: %p", addsigCtxPtr);
        delete addsigCtxPtr;
        return nullptr;
    }
    LE_DEBUG("timerRef   : %p", addsigCtxPtr->timerRef);

    le_timer_Ref_t timerRef = addsigCtxPtr->timerRef;

    // Add to sigStrengthClientsAndHandlersRefMap_
    addSigStrengthClientsAndHandlersRefMapEntry(clientRef,
                                (taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t)handlerRef);
    LE_INFO("Total handlerRefs per client: %zu", sigStrengthClientsAndHandlersRefMap_.size());

    // Add to sigStrengthHandlersRefAndStaIDMap_
    addSigStrengthHandlersRefAndStaIDMapEntry(
                    (taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t)handlerRef, staCtxPtr->id);
    LE_INFO("Num HandlerRef-STA ID: %zu", sigStrengthHandlersRefAndStaIDMap_.size());

    // Add to sigStrengthHandlerRefAndTimerRefMap_
    addSigStrengthHandlerRefAndTimerRefMapEntry(
                        (taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t)handlerRef, timerRef);
    LE_INFO("Num HandlerRef-Timer Ref: %zu", sigStrengthHandlerRefAndTimerRefMap_.size());

    // Set the signal monitor to active mode
    sigStrengthMonitorRefMap_[staCtxPtr->id]->SetActiveMode();

    return (taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t)handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to stop and delete the client signal strength timer from the context of
 * staClientEventsThreadRef_.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::queueCleanupApSigStrengthClientEventTimers
(
    void *param1Ptr,
    void *param2Ptr
)
{
    le_timer_Ref_t timerRef = (le_timer_Ref_t)param1Ptr;
    LE_UNUSED(param2Ptr);

    if (timerRef)
    {
        LE_DEBUG("timerRef  : %p", timerRef);
        if (le_timer_IsRunning(timerRef))
        {
            LE_DEBUG("Timer stopped.");
            le_timer_Stop(timerRef);
        }
        else
        {
            LE_DEBUG("Timer is not running.");
        }
        le_timer_Delete(timerRef);
        LE_DEBUG("Timer deleted.");
    }
    else
    {
        LE_WARN("timerRef is NULL");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to start client signal strength timers from the context of
 * staClientEventsThreadRef_. This will be done in scenarios where clients have registered for
 * signal strength handlers and the STA connects to an AP.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::queueStartApSigStrengthClientEventTimers
(
    void *param1Ptr,
    void *param2Ptr
)
{
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);
    // Restart any client regsitered timers
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    for (const auto &pair : myWlanSta.sigStrengthHandlerRefAndTimerRefMap_)
    {
        le_timer_Ref_t timerRef = pair.second;
        if (timerRef)
        {
            if (!le_timer_IsRunning(timerRef))
            {
                le_result_t result = le_timer_Start(timerRef);
                if (LE_OK == result)
                {
                    LE_DEBUG("Timer %p started.", timerRef);
                }
                else
                {
                    LE_WARN("Starting timer %p failed: %d", timerRef, result);
                }
            }
            else
            {
                LE_DEBUG("Timer already running: %p", timerRef);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to stop client signal strength timers from the context of
 * staClientEventsThreadRef_. This will be done in scenarios where clients have registered for
 * signal strength handlers and the STA disconnects from an AP.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::queueStopApSigStrengthClientEventTimers
(
    void *param1Ptr,
    void *param2Ptr
)
{
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);
    // Restart any client regsitered timers
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    for (const auto &pair : myWlanSta.sigStrengthHandlerRefAndTimerRefMap_)
    {
        le_timer_Ref_t timerRef = pair.second;
        if (timerRef)
        {
            if (le_timer_IsRunning(timerRef))
            {
                le_result_t result = le_timer_Stop(timerRef);
                if (LE_OK == result)
                {
                    LE_DEBUG("Timer %p stopped.", timerRef);
                }
                else
                {
                    LE_WARN("Stopping timer %p failed: %d", timerRef, result);
                }
            }
            else
            {
                LE_DEBUG("Timer already running: %p", timerRef);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to remove signal thread handler in the context of staClientEventsThreadRef_.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::queueRemoveApSigStrengthClientEventTimer(
    void *param1Ptr,
    void *param2Ptr)
{
    le_timer_Ref_t timerRef          = (le_timer_Ref_t)param1Ptr;
    LE_UNUSED(param2Ptr);

    if (timerRef)
    {
        LE_DEBUG("timerRef  : %p", timerRef);
        if (le_timer_IsRunning(timerRef))
        {
            LE_DEBUG("Timer stopped.");
            le_timer_Stop(timerRef);
        }
        else
        {
            LE_DEBUG("Timer is not running.");
        }
        le_timer_Delete(timerRef);
        LE_DEBUG("Timer deleted.");
    }
    else
    {
        LE_WARN("timerRef is NULL");
    }

    // Complete promise.
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    myWlanSta.sigStrengthRemoveHandlerPromise_.set_value();
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove signal strength notification handler for the connected AP.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::SvcRemoveConnectedApSignalStrengthHandler
(
    taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef,
    le_msg_SessionRef_t clientRef
)
{
    TAF_ERROR_IF_RET_NIL(nullptr == handlerRef, "handlerRef is NULL!");
    TAF_ERROR_IF_RET_NIL(nullptr == clientRef, "clientRef is NULL!");
    taf_wlan_STAid_t clientStaId = TAF_WLAN_STA_ID1;

    // Stop the timer for this event.
    le_timer_Ref_t timerRef = getTimerRefFromSigStrengthHandlerRefMap(handlerRef);
    TAF_ERROR_IF_RET_NIL(nullptr == timerRef, "timerRef is NULL!");
    // Get the timer signal context
    ConnectedApSigStrengthClientCtx_t *sigCtxPtr =
                        (ConnectedApSigStrengthClientCtx_t *)(le_timer_GetContextPtr(timerRef));
    TAF_ERROR_IF_RET_NIL(nullptr == sigCtxPtr, "sigCtxPtr is NULL!");
    LE_DEBUG("ConnectedApSigStrengthClientCtx_t: sigCtxPtr: %p", sigCtxPtr);

    // Stop the timer context of the client events thread.
    LE_DEBUG("Stop timer from client events thread context ...");
    sigStrengthRemoveHandlerPromise_ = std::promise<void>();
    std::future<void> fut = sigStrengthRemoveHandlerPromise_.get_future();
    // Queue to client events thread, staClientEventsThreadRef_
    le_event_QueueFunctionToThread(staClientEventsThreadRef_,
                                   queueRemoveApSigStrengthClientEventTimer, timerRef, handlerRef);
    fut.get();

    LE_DEBUG("Release ConnectedApSigStrengthClientCtx_t: %p", sigCtxPtr);
    delete sigCtxPtr;
    sigCtxPtr = nullptr;

    // Remove from sigStrengthHandlerRefAndTimerRefMap_
    removeSigStrengthHandlerRefAndTimerRefMapEntry(handlerRef);
    LE_INFO("Num HandlerRef-Timer Ref: %zu", sigStrengthHandlerRefAndTimerRefMap_.size());

    // Remove from sigStrengthClientsAndHandlersRefMap_
    removeSigStrengthClientsAndHandlersRefMapEntry(clientRef, handlerRef);
    LE_INFO("Total handlerRefs per client: %zu", sigStrengthClientsAndHandlersRefMap_.size());

    // Check if any more clients are monitoring signal strength for this STA
    // from sigStrengthHandlersRefAndStaIDMap_
    if (!getSigStrengthHandlersRefAndStaIDMapEntry(handlerRef, clientStaId))
    {
        LE_WARN("Unable to find STA ID.");
    }
    else
    {
        // Remove from sigStrengthHandlersRefAndStaIDMap_
        removeSigStrengthHandlersRefAndStaIDMapEntry(handlerRef);
        LE_INFO("Num HandlerRef-STA ID: %zu", sigStrengthHandlersRefAndStaIDMap_.size());

        // Get the number of clients remaining for that STA from sigStrengthHandlersRefAndStaIDMap_
        int numClients = GetNumSignalStrengthHandlersRefForSta(clientStaId);
        if (0 == numClients)
        {
            // No more clients for this event. Set to passive mode.
            LE_INFO("Set Sta[%d] signal strength monitoring to passive mode.", clientStaId);
            sigStrengthMonitorRefMap_[clientStaId]->SetPassiveMode();
        }
        else
        {
            LE_DEBUG("%d Clients are still monitoring signal strength for STA[%d].", numClients,
                                                                                    clientStaId);
        }
    }

    // Remove handler
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add an entry.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::addSigStrengthClientsAndHandlersRefMapEntry
(
    const le_msg_SessionRef_t sessionRef,
    const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef
)
{
    std::unique_lock<std::shared_mutex> lock(sigStrengthClientsAndHandlersRefMapMutex_);
    sigStrengthClientsAndHandlersRefMap_.insert({sessionRef, handlerRef});
    LE_DEBUG ("Added handler %p to session %p.", sessionRef, handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove an entry
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::removeSigStrengthClientsAndHandlersRefMapEntry
(
    const le_msg_SessionRef_t sessionRef,
    const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef
)
{
    std::unique_lock<std::shared_mutex> lock(sigStrengthClientsAndHandlersRefMapMutex_);
    auto range = sigStrengthClientsAndHandlersRefMap_.equal_range(sessionRef);
    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->second == handlerRef)
        {
            sigStrengthClientsAndHandlersRefMap_.erase(it);
            LE_DEBUG("Removed handler %p from session %p.", sessionRef, handlerRef);
            return;
        }
    }
    LE_WARN("Not found: handler %p for session %p.", sessionRef, handlerRef);
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear all entries for a sessionRef.
 */
//--------------------------------------------------------------------------------------------------
std::vector<taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t>
taf_WlanSTASvcImpl::getAllSigStrengthClientsAndHandlersRefMapEntries
(
    const le_msg_SessionRef_t sessionRef
)
{
    std::shared_lock<std::shared_mutex> lock(sigStrengthClientsAndHandlersRefMapMutex_);
    std::vector<taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t> handlerRefs;
    auto range = sigStrengthClientsAndHandlersRefMap_.equal_range(sessionRef);
    for (auto it = range.first; it != range.second; ++it)
    {
        handlerRefs.push_back(it->second);
    }
    LE_DEBUG("Number of handlerRefs %zu", handlerRefs.size());
    return handlerRefs;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add an entry
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::addSigStrengthHandlersRefAndStaIDMapEntry
(
    const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef,
     const taf_wlan_STAid_t &staId
)
{
    std::unique_lock<std::shared_mutex> lock(sigStrengthHandlersRefAndStaIDMapMutex_);
    sigStrengthHandlersRefAndStaIDMap_[handlerRef] = staId;
    LE_DEBUG("Added handler %p for STA %d.", handlerRef, staId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the STA ID for the given handler ref. Returns false if no entry found
 */
//--------------------------------------------------------------------------------------------------
bool taf_WlanSTASvcImpl::getSigStrengthHandlersRefAndStaIDMapEntry
(
    const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef,
    taf_wlan_STAid_t &staId
) const
{
    std::shared_lock<std::shared_mutex> lock(sigStrengthHandlersRefAndStaIDMapMutex_);
    auto it = sigStrengthHandlersRefAndStaIDMap_.find(handlerRef);
    if (it != sigStrengthHandlersRefAndStaIDMap_.end())
    {
        staId = it->second;
        LE_DEBUG("STA for handler %p: %d.", handlerRef, staId);
        return true;
    }
    LE_DEBUG("STA for handler %p not found.", handlerRef);
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the number of handler references for the specifed STA ID.
 */
//--------------------------------------------------------------------------------------------------
int taf_WlanSTASvcImpl::GetNumSignalStrengthHandlersRefForSta(const taf_wlan_STAid_t staId) const
{
    std::shared_lock<std::shared_mutex> lock(sigStrengthHandlersRefAndStaIDMapMutex_);

    int count = 0;
    for (const auto &entry : sigStrengthHandlersRefAndStaIDMap_)
    {
        if (entry.second == staId)
        {
            ++count;
        }
    }
    LE_DEBUG ("Number of handlers for STA %d = %d.", staId, count);
    return count;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear all entries for a sessionRef.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::removeSigStrengthHandlersRefAndStaIDMapEntry
(
    const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef
)
{
    std::unique_lock<std::shared_mutex> lock(sigStrengthHandlersRefAndStaIDMapMutex_);
    auto it = sigStrengthHandlersRefAndStaIDMap_.find(handlerRef);
    if (it != sigStrengthHandlersRefAndStaIDMap_.end())
    {
        sigStrengthHandlersRefAndStaIDMap_.erase(it);
        LE_DEBUG("Erased handler %p entry from map.", handlerRef);
        return;
    }
    LE_DEBUG("Entry for handler %p not found.", handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add an entry
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::addSigStrengthHandlerRefAndTimerRefMapEntry
(
    const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef,
    const le_timer_Ref_t timerRef
)
{
    std::unique_lock<std::shared_mutex> lock(sigStrengthHandlerRefAndTimerRefMapMutex_);
    sigStrengthHandlerRefAndTimerRefMap_[handlerRef] = timerRef;
    LE_DEBUG("Added handler %p for timer %p.", handlerRef, timerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the timerRef
 */
//--------------------------------------------------------------------------------------------------
le_timer_Ref_t taf_WlanSTASvcImpl::getTimerRefFromSigStrengthHandlerRefMap
(
    const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef
)
{
    std::shared_lock<std::shared_mutex> lock(sigStrengthHandlerRefAndTimerRefMapMutex_);
    auto it = sigStrengthHandlerRefAndTimerRefMap_.find(handlerRef);
    if (it != sigStrengthHandlerRefAndTimerRefMap_.end())
    {
        LE_DEBUG("Timer for handler %p: %p.", handlerRef, it->second);
        return it->second;
    }
    LE_DEBUG("Timer for handler %p not found.", handlerRef);
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove an entry
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::removeSigStrengthHandlerRefAndTimerRefMapEntry
(
    const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef
)
{
    std::unique_lock<std::shared_mutex> lock(sigStrengthHandlerRefAndTimerRefMapMutex_);
    auto it = sigStrengthHandlerRefAndTimerRefMap_.find(handlerRef);
    if (it != sigStrengthHandlerRefAndTimerRefMap_.end())
    {
        sigStrengthHandlerRefAndTimerRefMap_.erase(it);
        LE_DEBUG("Erased handler %p entry from map.", handlerRef);
        return;
    }
    LE_DEBUG("Entry for handler %p not found.", handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Performs AP scan.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::DoAPScan(
    taf_wlanSta_WlanSTARef_t staRef ///< [IN] The WLAN STA reference.
)
{
    StaCtx_t *staCtxPtr = NULL;
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;

    TAF_ERROR_IF_RET_VAL(nullptr == wlanSTAMgr, LE_FAULT, "WLAN STA Manager not initialized");

    staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    // Ensure STA is active
    std::vector<telux::wlan::StaStatus> status;
    errCode = wlanSTAMgr->getStatus(status);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA getStatus failed with error : %d", static_cast<int>(errCode));
        return LE_FAULT;
    }
    for (auto &element : status)
    {
        LE_DEBUG("------------------------------------------");
        LE_DEBUG("STA Id: %d", static_cast<int>(element.id));
        if (taf_WlanHelper::TAFSTAidtoTeluxId(staCtxPtr->id) == element.id)
        {
            if (telux::wlan::StaInterfaceStatus::UNKNOWN == element.status)
            {
                LE_WARN("STA %d status known", staCtxPtr->id);
                return LE_FAULT;
            }
            else
            {
                // STA sate is good. Scan can be performed.
                break;
            }
        }
    }

    // Send cmd event to perform scan
    StaCmd_t cmd = {staCtxPtr, CMD_WPA_DO_SCAN};
    le_event_Report(staCommand_, &cmd, sizeof(StaCmd_t));
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Performs AP scan.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::GetAPScanResults(
    taf_wlanSta_WlanSTARef_t staRef,
    ///< [IN] The WLAN STA reference.
    uint16_t *numAPPtr,
    ///< [OUT] Number of APs found in the scan.
    taf_wlanSta_APInfo_t *ApInfoPtr,
    ///< [OUT] Scanned available AP information.
    size_t *ApInfoSizePtr
    ///< [INOUT]
)
{
    StaCtx_t *staCtxPtr = NULL;

    // Check for null pointers
    TAF_ERROR_IF_RET_VAL(nullptr == wlanSTAMgr, LE_FAULT, "WLAN STA Manager not initialized");
    TAF_ERROR_IF_RET_VAL(numAPPtr == nullptr, LE_BAD_PARAMETER, "numAPPtr is NULL");
    TAF_ERROR_IF_RET_VAL(ApInfoPtr == nullptr, LE_BAD_PARAMETER, "ApInfoPtr is NULL");
    TAF_ERROR_IF_RET_VAL(ApInfoSizePtr == nullptr, LE_BAD_PARAMETER, "ApInfoSizePtr is NULL");
    TAF_ERROR_IF_RET_VAL(*ApInfoSizePtr == 0, LE_BAD_PARAMETER, "ApInfoSizePtr is 0");

    staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    // Update the number of APs available
    *numAPPtr = staCtxPtr->numScannedAPs;

    LE_DEBUG("numAPPtr     : %d", *numAPPtr);
    LE_DEBUG("ApInfoSizePtr: %zu", *ApInfoSizePtr);

    // Check if enough space is available to store all scanned APs.
    size_t copyCount = std::min(static_cast<size_t>(staCtxPtr->numScannedAPs), *ApInfoSizePtr);

    // Update the Info size pointer
    *ApInfoSizePtr = copyCount;

    if (copyCount < staCtxPtr->numScannedAPs) {
        LE_WARN("Number of elements(%zu) less than number of available APs(%d)",
                copyCount, staCtxPtr->numScannedAPs);
    }

    // Copy AP information
    for (size_t iCount = 0; iCount < copyCount; ++iCount)
    {
        // Initialize destination strings to empty
        ApInfoPtr[iCount].BSSID[0] = '\0';
        ApInfoPtr[iCount].SSID[0] = '\0';

        // BSSID
        if (staCtxPtr->ApInfo[iCount].BSSID != nullptr &&
            strlen(staCtxPtr->ApInfo[iCount].BSSID) > 0)
        {
            le_result_t result = le_utf8_Copy(ApInfoPtr[iCount].BSSID,
                staCtxPtr->ApInfo[iCount].BSSID, TAF_WLAN_MAX_BSSID_LENGTH + 1, NULL);
            if (result != LE_OK)
            {
                LE_ERROR("Failed to copy BSSID: %s", LE_RESULT_TXT(result));
                ApInfoPtr[iCount].BSSID[0] = '\0';
            }
        }

        // SSID
        if (staCtxPtr->ApInfo[iCount].SSID != nullptr &&
            strlen(staCtxPtr->ApInfo[iCount].SSID) > 0)
        {
            le_result_t result = le_utf8_Copy(ApInfoPtr[iCount].SSID,
                staCtxPtr->ApInfo[iCount].SSID,  TAF_WLAN_MAX_SSID_LENGTH + 1, NULL);
            if (result != LE_OK)
            {
                LE_ERROR("Failed to copy SSID: %s", LE_RESULT_TXT(result));
                ApInfoPtr[iCount].SSID[0] = '\0';
            }
        }

        // Signal Level
        ApInfoPtr[iCount].SignalLevel = staCtxPtr->ApInfo[iCount].SignalLevel;

        // Frequency
        ApInfoPtr[iCount].Frequency = staCtxPtr->ApInfo[iCount].Frequency;

        // Service Set
        ApInfoPtr[iCount].SS = staCtxPtr->ApInfo[iCount].SS;

        // Security Mode/Proto
        ApInfoPtr[iCount].secMode = staCtxPtr->ApInfo[iCount].secMode;

        // Auth Method
        ApInfoPtr[iCount].secAuthMethod = staCtxPtr->ApInfo[iCount].secAuthMethod;

        // Encryption Method
        ApInfoPtr[iCount].secEncryptionMethod = staCtxPtr->ApInfo[iCount].secEncryptionMethod;

        // WPS
        ApInfoPtr[iCount].WPSEnabled = staCtxPtr->ApInfo[iCount].WPSEnabled;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns the internal WLAN STA context based on taf_wlan_STAid_t
 *
 */
//--------------------------------------------------------------------------------------------------
StaCtx_t *taf_WlanSTASvcImpl::GetStaCtx(taf_wlan_STAid_t staId)
{
    le_dls_Link_t *linkPtr = NULL;

    le_mutex_Lock(STACtxMutex);
    linkPtr = le_dls_Peek(&STACtxList);
    while (linkPtr)
    {
        StaCtx_t *staCtxPtr = CONTAINER_OF(linkPtr, StaCtx_t, link);
        linkPtr = le_dls_PeekNext(&STACtxList, linkPtr);
        if (staCtxPtr->id == staId)
        {
            le_mutex_Unlock(STACtxMutex);
            return staCtxPtr;
        }
    }

    le_mutex_Unlock(STACtxMutex);
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns the WLAN STA reference.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
taf_wlanSta_WlanSTARef_t taf_WlanSTASvcImpl::GetWlanSTA (
    taf_wlan_STAid_t STAid,
        ///< [IN] STA identifier
    const char* LE_NONNULL STAIntfName
        ///< [IN] AP associated host interface name.
)
{
    StaCtx_t *staCtxPtr = GetStaCtx(STAid);

    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, NULL, "Unable to get context for STA ID: %d", STAid);

    // Check for valid interface name.
    TAF_ERROR_IF_RET_VAL(0 == strlen(STAIntfName), NULL, "Invalid STAIntfName");
    // TBD: Add more checks here.

    // Copy the Interface name to the context
    le_result_t ret = LE_OK;
    ret = le_utf8_Copy(staCtxPtr->IntfName, STAIntfName, TAF_NET_INTERFACE_NAME_MAX_LEN+1, NULL);
    if (LE_OK != ret)
    {
        LE_WARN("IntfName copy error: %d", ret);
    }
    LE_DEBUG("STA ID: %d, Intf: %s", staCtxPtr->id, staCtxPtr->IntfName);

    // Set the inteface name for signal strength monitoring object for this STA.
    sigStrengthMonitorRefMap_[STAid]->SetInterfaceName(STAIntfName);
    return staCtxPtr->staRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Return the instance of taf_WlanSTASvcImpl class.
 */
//--------------------------------------------------------------------------------------------------
taf_WlanSTASvcImpl &taf_WlanSTASvcImpl::GetInstance()
{
    static taf_WlanSTASvcImpl instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Return the internal command event ID "staCommand_".
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t taf_WlanSTASvcImpl::GetWlanStaInternalCmdEventId() const
{
    return staCommand_;
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread to monitor WPA supplicant events
 */
//--------------------------------------------------------------------------------------------------
void *taf_WlanSTASvcImpl::StaCmdThreadHdlr(void *context)
{
    // Add internal STA event handler
    auto &myWlanSta = GetInstance();
    le_event_AddHandler("STA Command Handler", myWlanSta.staCommand_, StaCmdHandler);
    // Start the event loop
    LE_INFO("Sta Cmd Handler Thread Started");
    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread to generate client events from the service.
 */
//--------------------------------------------------------------------------------------------------
void *taf_WlanSTASvcImpl::staClientEventsThreadHandler(void *context)
{
    // Start the event loop
    LE_INFO("Sta client events thread started");
    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Client connect function. Nothing is done.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::onWlanStaClientConnect(le_msg_SessionRef_t sessionRef, void *ctxPtr)
{
    LE_UNUSED(ctxPtr);
    LE_DEBUG("Client connected: %p", sessionRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Client disconnect function. Send an event to stop signal strength monitoring events for that
 * client.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::onWlanStaClientDisconnect(le_msg_SessionRef_t sessionRef, void *ctxPtr)
{
    LE_UNUSED(ctxPtr);
    TAF_ERROR_IF_RET_NIL(nullptr == sessionRef, "sessionRef is NULL!");
    LE_INFO("Client disconnected: %p", sessionRef);

    std::vector<taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t> retrievedHandlers;
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();

    retrievedHandlers = myWlanSta.getAllSigStrengthClientsAndHandlersRefMapEntries(sessionRef);
    size_t numHandlerRefs = retrievedHandlers.size();

    LE_DEBUG("Num handlers for client %p: %zu", sessionRef, numHandlerRefs);

    TAF_ERROR_IF_RET_NIL(0 == numHandlerRefs, "No handlers found for client %p", sessionRef);

    for (auto &clientHandlerRef : retrievedHandlers)
    {
        taf_wlan_STAid_t clientStaId = TAF_WLAN_STA_ID1;
        // Stop any running timers for client events
        le_timer_Ref_t timerRef = myWlanSta.getTimerRefFromSigStrengthHandlerRefMap(
                                                                                clientHandlerRef);
        if (timerRef)
        {
            LE_DEBUG("Queuing timer %p to be removed", timerRef);
            // Queue this timer to be removed in the conext of staClientEventsThreadRef_
            le_event_QueueFunctionToThread(myWlanSta.staClientEventsThreadRef_,
                                    queueCleanupApSigStrengthClientEventTimers, timerRef, nullptr);
        }
        else
        {
            LE_DEBUG("Timer Ref not found for client %p", sessionRef);
        }

        // Remove from sigStrengthClientsAndHandlersRefMap_
        myWlanSta.removeSigStrengthClientsAndHandlersRefMapEntry(sessionRef, clientHandlerRef);
        LE_INFO("Total handlerRefs per client: %zu",
                                            myWlanSta.sigStrengthClientsAndHandlersRefMap_.size());

        // Remove from sigStrengthHandlerRefAndTimerRefMap_
        myWlanSta.removeSigStrengthHandlerRefAndTimerRefMapEntry(clientHandlerRef);
        LE_INFO("Num HandlerRef-Timer Ref: %zu",
                                            myWlanSta.sigStrengthHandlerRefAndTimerRefMap_.size());

        // Check if any more clients are monitoring signal strength for this STA
        // from sigStrengthHandlersRefAndStaIDMap_
        if (!myWlanSta.getSigStrengthHandlersRefAndStaIDMapEntry(clientHandlerRef, clientStaId))
        {
            LE_WARN("Unable to get STA ID for handlerRef %p", clientHandlerRef);
        }
        else
        {
            // Remove from sigStrengthHandlersRefAndStaIDMap_
            myWlanSta.removeSigStrengthHandlersRefAndStaIDMapEntry(clientHandlerRef);
            LE_INFO("Num HandlerRef-STA ID: %zu",
                                            myWlanSta.sigStrengthHandlersRefAndStaIDMap_.size());

            // Get the num of clients remaining for that STA from sigStrengthHandlersRefAndStaIDMap_
            int numClients = myWlanSta.GetNumSignalStrengthHandlersRefForSta(clientStaId);
            if (0 == numClients)
            {
                // No more clients for this event. Set to passive mode.
                LE_INFO("Set Sta[%d] signal strength monitoring to passive mode.", clientStaId);
                myWlanSta.sigStrengthMonitorRefMap_[clientStaId]->SetPassiveMode();
            }
            else
            {
                LE_DEBUG("%d Clients are still monitoring signal strength for STA[%d].",
                                                                          numClients, clientStaId);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Register client connect/disconnect handlers
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::registerClientsConnectDisconnectHandlers()
{
    // Create client connect/disconnect handlers
    wlanStaClientConnectHandlerRef_ = le_msg_AddServiceOpenHandler(
        taf_wlanSta_GetServiceRef(),
        taf_WlanSTASvcImpl::onWlanStaClientConnect,
        NULL);

    wlanStaClientDisconnectHandlerRef_ = le_msg_AddServiceCloseHandler(
        taf_wlanSta_GetServiceRef(),
        taf_WlanSTASvcImpl::onWlanStaClientDisconnect,
        NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Unregister client connect/disconnect handlers
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::unregisterClientsConnectDisconnectHandlers()
{
    le_msg_RemoveServiceHandler(wlanStaClientConnectHandlerRef_);
    le_msg_RemoveServiceHandler(wlanStaClientDisconnectHandlerRef_);
}

//--------------------------------------------------------------------------------------------------
/**
 * taf_WlanAPSvcImpl Init function
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::Init()
{
    // Initialize to nullptr
    wlanSTAMgr = nullptr;

    auto &wlanFactory = telux::wlan::WlanFactory::getInstance();
    wlanSTAMgr = wlanFactory.getStaInterfaceManager();
    if (wlanSTAMgr == nullptr)
    {
        // Unable to initialize the WLAN STA subsystem. Stop the service.
        LE_FATAL(" *** Unable to initialize Wlan STA subsystem *** ");
    }

    // Register the Listener class shared object
    wlanSTAListener = std::make_shared<taf_WlanSTAListener>();
    telux::common::ErrorCode retCode = wlanSTAMgr->registerListener(wlanSTAListener);
    if (telux::common::ErrorCode::SUCCESS != retCode)
    {
        LE_WARN("WLAN STA registerListener failed: %d", static_cast<int>(retCode));
    }

    // Initiate the STA context pool.
    STACtxPoolRef = le_mem_InitStaticPool(tafWlanStaCtxPool,TAF_WLAN_MAX_NUM_STA,sizeof(StaCtx_t));

    // Create the STA context list mutex.
    STACtxMutex = le_mutex_CreateNonRecursive("STACtxMutex");

    // Create reference map for Station context(s)
    StaRefMap = le_ref_CreateMap("StaRefMap", TAF_WLAN_MAX_NUM_STA);

    // Create contexts for support Stations
    StaCtx_t *staCtxPtr = NULL;
    taf_wlanSta_WlanSTARef_t staRef = NULL;
    std::string eventName;
    for (int iCount = 1; iCount <= TAF_WLAN_MAX_NUM_STA; iCount++)
    {
        staCtxPtr = NULL;
        staRef = NULL;
        staCtxPtr = (StaCtx_t *)le_mem_ForceAlloc(STACtxPoolRef);
        if (NULL == staCtxPtr)
        {
            LE_FATAL("Unable to allocate staCtxPtr for STA ID: %d", iCount);
        }
        // Set STA ID.
        staCtxPtr->id = static_cast<taf_wlan_STAid_t>(iCount);
        // NULL terminate Interface name string.
        staCtxPtr->IntfName[0] = 0;
        // Create reference for this context
        staRef = (taf_wlanSta_WlanSTARef_t)le_ref_CreateRef(StaRefMap, (void *)staCtxPtr);
        if (NULL == staRef)
        {
            LE_FATAL("Unable to allocate reference for STA ID: %d", iCount);
        }
        staCtxPtr->staRef = staRef;

        // Create STA event for applications.
        eventName.clear();
        eventName = "StaStateEvent-" + staCtxPtr->id;
        staCtxPtr->StaEvent = le_event_CreateIdWithRefCounting(eventName.c_str());

        // Queue this STA context
        le_dls_Queue(&STACtxList, &staCtxPtr->link);

        // Create a connected AP signal monitoring class object.
        sigStrengthMonitorRefMap_[staCtxPtr->id] =
                    std::make_shared<taf_WlanStaConnectedApSignalStrengthMonitor>(staCtxPtr->id);

        LE_DEBUG("Context created for STA ID: %d", staCtxPtr->id);
    }

    // Memory for STA events
    StaEventsPoolRef = le_mem_InitStaticPool(StaEventsPool, TAF_WLAN_MAX_SESSION_REF,
                                                          sizeof(StaEvents_t));

    // Mutex for STA events
    STAEventsMutexRef = le_mutex_CreateNonRecursive("STAEventsMutex");

    // Create internal STA command event ref
    staCommand_ = le_event_CreateId("staCommand_", sizeof(StaCmd_t));
    // Start Sta events thread
    staCmdThreadRef_ = le_thread_Create("StaCmdThread", StaCmdThreadHdlr,NULL);
    le_thread_Start(staCmdThreadRef_);

    // Start client events thread
    staClientEventsThreadRef_ = le_thread_Create("StaEvtThread",
                                                            staClientEventsThreadHandler, NULL);
    le_thread_Start(staClientEventsThreadRef_);

    // Register handlers to track connected clients.
    registerClientsConnectDisconnectHandlers();

    LE_INFO(" *** Wlan STA Initialized *** ");

    return;
}
