/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanSTASvcImpl.cpp
 *
 * @brief      Implemenation of TelAF WLAN Station Service APIs.
 *
 */


#include "tafWlan.hpp"
#include <wpa_ctrl.h>
#include <errno.h>

using namespace telux::tafsvc;

#define WPA_CTRL_RSP_BUF_LEN 2048

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


    taf_wlanSta_HandlerFunc_t clientHandlerFunc =
        (taf_wlanSta_HandlerFunc_t)secondLayerHandlerFunc;

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
    StaCtx_t *StaCtxPtr,      // Sta Context
    taf_wlanSta_State_t state // State to report
)
{
    StaEvents_t *StaEventPtr = (StaEvents_t *)le_mem_ForceAlloc(StaEventsPoolRef);
    TAF_ERROR_IF_RET_NIL(StaEventPtr == NULL, "StaEventPtr is NULL!");
    StaEventPtr->staRef = StaCtxPtr->staRef;
    StaEventPtr->state = state;
    le_event_ReportWithRefCounting(StaCtxPtr->StaEvent, (void *)StaEventPtr);
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

    struct wpa_ctrl *ctrl;

    char rsp[WPA_CTRL_RSP_BUF_LEN] = {0};
    size_t len = WPA_CTRL_RSP_BUF_LEN;
    SuppThreadCtx_t *ThreadCtxPtr = (SuppThreadCtx_t *)context;
    std::string wpa_supplicant_path(WPA_SUPPLICANT_LOCATION_PATH);
    wpa_supplicant_path = wpa_supplicant_path + ThreadCtxPtr->IntfName;
    LE_DEBUG("Supplicant: %s", wpa_supplicant_path.c_str());
    LE_INFO("Wait for event: %s", ThreadCtxPtr->EventToMonitor);

    ctrl = wpa_ctrl_open(wpa_supplicant_path.c_str());
    if (ctrl == NULL)
    {
        auto &myWlanSta = GetInstance();
        myWlanSta.PromiseWPA.set_value(EVT_WPA_ERROR);
        LE_ERROR("Failed to open control interface");
        return NULL;
    }

    int ret = wpa_ctrl_attach(ctrl);
    if (ret != 0)
    {
        LE_ERROR("Failed to attach to control interface");
        auto &myWlanSta = GetInstance();
        myWlanSta.PromiseWPA.set_value(EVT_WPA_ERROR);
        wpa_ctrl_close(ctrl);
        return NULL;
    }

    do
    {
        ret = wpa_ctrl_pending(ctrl);
        if (1 == ret)
        {
            memset(rsp, 0, WPA_CTRL_RSP_BUF_LEN);
            len = WPA_CTRL_RSP_BUF_LEN;
            // Event pending;
            ret = wpa_ctrl_recv(ctrl, rsp, &len);
            if (ret < 0)
            {
                auto &myWlanSta = GetInstance();
                myWlanSta.PromiseWPA.set_value(EVT_WPA_ERROR);
                LE_WARN("wpa_ctrl_recv failed");
                break;
            }
            LE_DEBUG ("Received response Len: %ld", len);
            LE_DEBUG("Received response    : %s", rsp);
            // Split te received buffer using space as delimiter
            std::vector<std::string> ind = taf_WlanHelper::StrSplit(rsp, ' ');
            LE_DEBUG("Received Event: %s", ind[0].c_str());
            // Check if the event that is received is the one we are waiting on
            if (ind[0].find(ThreadCtxPtr->EventToMonitor) != std::string::npos)
            {
                LE_INFO("Event: %s", ThreadCtxPtr->EventToMonitor);
                // Let the main thread know that scanning is done
                auto &myWlanSta = GetInstance();
                myWlanSta.PromiseWPA.set_value(EVT_WPA_AP_SCAN_DONE);
                // break out of the while loop
                break;
            }
        }
    } while (true);

    LE_DEBUG("Exiting thread");
    wpa_ctrl_close(ctrl);
    return NULL;
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
    std::string line;
    std::istringstream buf_stream(ScanResultsPtr);
    int iCount = 0;
    le_result_t ret = LE_OK;

    // Clear out earlier results
    CtxPtr->numScannedAPs = 0;
    memset(&CtxPtr->ApInfo[0], 0,
           (sizeof(taf_wlanSta_APInfo_t) * TAF_WLANSTA_MAX_APSCAN_RESULT_NUM));

    char delimiter = '\t';
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream;

    while (std::getline(buf_stream, line, '\n'))
    {
        if (CtxPtr->numScannedAPs >= TAF_WLANSTA_MAX_APSCAN_RESULT_NUM)
        {
            // Max number of scanned APs supported is reached. Break from loop.
            LE_WARN("Max number of scanned APs(%d) supported is reached",
                static_cast<int>(TAF_WLANSTA_MAX_APSCAN_RESULT_NUM));
            break;
        }

        // Check if the line has "frequency". This is the heading line. skip it.
        if (line.find("frequency") != std::string::npos)
        {
            LE_DEBUG ("Skip heading line");
            continue;
        }
        // Split line into separate fields
        {
            std::istringstream stream (line);
            tokens.clear();
            // The parameters are tab separated
            // Split the line into components and store in a vector
            while (std::getline(stream, token, delimiter))
            {
                tokens.push_back(token);
            }
            // BSSID
            ret = le_utf8_Copy(CtxPtr->ApInfo[iCount].BSSID, tokens[0].c_str(),
                               TAF_WLAN_MAX_BSSID_LENGTH+1, NULL);
            if (LE_OK != ret)
            {
                LE_WARN("BSSID copy error: %d", ret);
            }
            LE_DEBUG("BSSID[%d]: %s", iCount, CtxPtr->ApInfo[iCount].BSSID);

            // Frequency
            CtxPtr->ApInfo[iCount].Frequency = std::stoi(tokens[1]);
            LE_DEBUG("Frequency[%d]: %d MHz", iCount, CtxPtr->ApInfo[iCount].Frequency);

            // Signal Level
            CtxPtr->ApInfo[iCount].SignalLevel = std::stoi(tokens[2]);
            LE_DEBUG("SignalLevel[%d]: %d dBm", iCount, CtxPtr->ApInfo[iCount].SignalLevel);

            // Parse flags and populate relevant elements
            ParseScanResultsFlags(CtxPtr, iCount, tokens[3]);

            // SSID
            ret = le_utf8_Copy(CtxPtr->ApInfo[iCount].SSID, tokens[4].c_str(),
                               TAF_WLAN_MAX_SSID_LENGTH+1, NULL);
            if (LE_OK != ret)
            {
                LE_WARN("SSID copy error: %d", ret);
            }
            LE_DEBUG("SSID[%d]: %s", iCount, CtxPtr->ApInfo[iCount].SSID);

            // Increment the number of scanned APs
            CtxPtr->numScannedAPs++;
        }
        iCount++;
    }
    LE_DEBUG("Number of APs found: %d", CtxPtr->numScannedAPs);
}

//--------------------------------------------------------------------------------------------------
/**
 * Perform STA BSS scanning.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::PerformScan(StaCtx_t *CtxPtr)
{
    struct wpa_ctrl *ctrl;
    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};
    size_t rsp_len = WPA_CTRL_RSP_BUF_LEN;
    int ret;

    std::string wpa_supplicant_path(WPA_SUPPLICANT_LOCATION_PATH);
    wpa_supplicant_path = wpa_supplicant_path + CtxPtr->IntfName;

    LE_DEBUG("Supplicant: %s", wpa_supplicant_path.c_str());

    // Open wpa control interface
    ctrl = wpa_ctrl_open(wpa_supplicant_path.c_str());
    if (ctrl == NULL)
    {
        LE_ERROR("Error: %s", strerror(errno));
        LE_ERROR("Failed to open control interface");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }

    // Check communication with a PING Request
    LE_DEBUG("WPA CMD: PING");
    ret = wpa_ctrl_request(ctrl, "PING", strlen("PING"), rsp_buf, &rsp_len, NULL);
    if (ret == -2)
    {
        LE_ERROR("Timeout while waiting for response");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        wpa_ctrl_close(ctrl);
        return;
    }
    else if (ret < 0)
    {
        LE_ERROR("Failed to send command");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        wpa_ctrl_close(ctrl);
        return;
    }
    LE_DEBUG("PING RSP Len: %ld", rsp_len);
    LE_DEBUG("PING RSP: %s", rsp_buf);
    if (0 != strncmp(rsp_buf, "PONG", strlen("PONG")))
    {
        LE_ERROR("PONG not received. %s", rsp_buf);
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        wpa_ctrl_close(ctrl);
        return;
    }

    // STA AP scanning can be started.

    // Start a thread of monitoring WPA control events
    le_thread_Ref_t supplicantThreadRef = NULL;
    // Pass the interface to use and event that we are waiting for.
    SuppThreadCtx_t ThreadCtx = {{0}, {0}};
    le_utf8_Copy( ThreadCtx.IntfName, CtxPtr->IntfName, TAF_NET_INTERFACE_NAME_MAX_LEN+1,NULL);
    le_utf8_Copy( ThreadCtx.EventToMonitor,
                  taf_WlanHelper::StrTrimEndSpace(WPA_EVENT_SCAN_RESULTS).c_str(),
                  WLANSTA_MAX_WPA_EVENT_LEN+1, NULL);
    supplicantThreadRef = le_thread_Create("supplicantThread", StaWpaSuppMonitorThreadHdlr,
                                                                              (void *)&ThreadCtx);

    // Start SCAN
    LE_DEBUG("WPA CMD: SCAN");
    memset(rsp_buf, 0, WPA_CTRL_RSP_BUF_LEN);
    rsp_len = WPA_CTRL_RSP_BUF_LEN;
    ret = wpa_ctrl_request(ctrl, "SCAN", strlen("SCAN"), rsp_buf, &rsp_len, NULL);
    if (ret == -2)
    {
        LE_ERROR("Timeout while waiting for SCAN response");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        wpa_ctrl_close(ctrl);
        return;
    }
    else if (ret < 0)
    {
        LE_ERROR("Failed to send SCAN command");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        wpa_ctrl_close(ctrl);
        return;
    }
    LE_DEBUG("SCAN RSP Len: %ld", rsp_len);
    LE_DEBUG("SCAN RSP: %s", rsp_buf);
    if (0 != strncmp(rsp_buf, "OK", strlen("OK")))
    {
        LE_ERROR("SCAN OK not received. %s", rsp_buf);
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        wpa_ctrl_close(ctrl);
        return;
    }
    else
    {
        le_thread_Start(supplicantThreadRef);

        // SCAN is running. Wait for it to complete.
        PromiseWPA = std::promise<StaWpaEvt_e>();
        StaWpaEvt_e staWpaEvt = PromiseWPA.get_future().get();
        // Check the SCAN completion return.
        if (EVT_WPA_AP_SCAN_DONE != staWpaEvt)
        {
            LE_ERROR("WPA AP scan failed");
            ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
            wpa_ctrl_close(ctrl);
            return;
        }
    }

    // Get the scan results
    LE_DEBUG("WPA CMD: SCAN_RESULTS");
    memset(rsp_buf, 0, WPA_CTRL_RSP_BUF_LEN);
    rsp_len = WPA_CTRL_RSP_BUF_LEN;
    ret = wpa_ctrl_request(ctrl, "SCAN_RESULTS", strlen("SCAN_RESULTS"), rsp_buf, &rsp_len, NULL);
    if (ret == -2)
    {
        LE_ERROR("Timeout while waiting for SCAN_RESULTS response");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        wpa_ctrl_close(ctrl);
        return;
    }
    else if (ret < 0)
    {
        LE_ERROR("Failed to send SCAN_RESULTS command");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        wpa_ctrl_close(ctrl);
        return;
    }
    LE_DEBUG("SCAN_RESULTS RSP Len: %ld", rsp_len);
    // SCAN_RESULTS are in! Store them in the relevant context
    PopulateScanResults(CtxPtr, rsp_buf);

    // Report scan completed and clean up
    ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_COMPLETED);
    wpa_ctrl_close(ctrl);
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

    if (EVT_WPA_DO_SCAN == cmdPtr->cmd)
    {
        auto &myWlanSta = GetInstance();
        le_mutex_Lock(myWlanSta.STACtxMutex);
        myWlanSta.ReportStaState(cmdPtr->CtxPtr, TAF_WLANSTA_STATE_SCAN_STARTED);
        myWlanSta.PerformScan(cmdPtr->CtxPtr);
        le_mutex_Unlock(myWlanSta.STACtxMutex);
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
le_result_t taf_WlanSTASvcImpl::SetStaticIPConfig
(
    taf_wlanSta_WlanSTARef_t staRef,
    ///< [IN] The WLAN STA reference.
    const taf_wlanSta_IPConfig_t *LE_NONNULL StaStaticIPConfigPtr)
{
    StaCtx_t *staCtxPtr = NULL;
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;

    TAF_ERROR_IF_RET_VAL(nullptr == wlanSTAMgr, LE_FAULT, "WLAN STA Manager not initialized");

    staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    telux::wlan::StaStaticIpConfig staticIpConfig;
    // Static IP. Populate the static IP structure.
    staticIpConfig.ipAddr   = StaStaticIPConfigPtr->IPv4Addr;
    staticIpConfig.gwIpAddr = StaStaticIPConfigPtr->GWAddr;
    staticIpConfig.netMask  = StaStaticIPConfigPtr->NetMask;
    staticIpConfig.dnsAddr  = StaStaticIPConfigPtr->DNSAddr;
    errCode = wlanSTAMgr->setIpConfig(taf_WlanHelper::TAFSTAidtoTeluxId(staCtxPtr->id),
                                      taf_WlanHelper::StaIPTypeToTelux(TAF_WLANSTA_IPTYPE_STATIC),
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
    StaCmd_t cmd = {staCtxPtr, EVT_WPA_DO_SCAN};
    le_event_Report(StaCommand, &cmd, sizeof(StaCmd_t));
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
    //telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;

    TAF_ERROR_IF_RET_VAL(nullptr == wlanSTAMgr, LE_FAULT, "WLAN STA Manager not initialized");

    staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    // Update the number of APs available
    *numAPPtr = staCtxPtr->numScannedAPs;

    LE_DEBUG("numAPPtr     : %d", *numAPPtr);
    LE_DEBUG("ApInfoSizePtr: %ld", *ApInfoSizePtr);

    // Check if enough space is available to store all scanned APs.
    if (*ApInfoSizePtr >= staCtxPtr->numScannedAPs)
    {
        // Update the Info size pointer
        *ApInfoSizePtr = staCtxPtr->numScannedAPs;
    }
    else
    {
        LE_WARN("Number of element(%ld) less than number of available APs(%d)", *ApInfoSizePtr,
                                                                          staCtxPtr->numScannedAPs);
    }
    le_result_t ret = LE_OK;
    for (int iCount = 0; iCount < static_cast<int>(*ApInfoSizePtr); iCount++)
    {
        // BSSID
        ret = le_utf8_Copy(ApInfoPtr[iCount].BSSID, staCtxPtr->ApInfo[iCount].BSSID,
                           TAF_WLAN_MAX_BSSID_LENGTH+1, NULL);
        if (LE_OK != ret)
        {
            LE_WARN("BSSID copy error: %d", ret);
        }

        // SSID
        ret = le_utf8_Copy(ApInfoPtr[iCount].SSID, staCtxPtr->ApInfo[iCount].SSID,
                           TAF_WLAN_MAX_SSID_LENGTH+1, NULL);
        if (LE_OK != ret)
        {
            LE_WARN("SSID copy error: %d", ret);
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
 * Thread to monitor WPA supplicant events
 */
//--------------------------------------------------------------------------------------------------
void *taf_WlanSTASvcImpl::StaCmdThreadHdlr(void *context)
{
    // Add internal STA event handler
    auto &myWlanSta = GetInstance();
    le_event_AddHandler("STA Command Handler", myWlanSta.StaCommand, StaCmdHandler);
    // Start the event loop
    LE_INFO("Sta Cmd Handler Thread Started");
    le_event_RunLoop();
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
    for (int iCount = 0; iCount < TAF_WLAN_MAX_NUM_STA; iCount++)
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
        eventName.empty();
        eventName = "StaCtx-" + staCtxPtr->id;
        staCtxPtr->StaEvent = le_event_CreateIdWithRefCounting(eventName.c_str());

        // Queue this STA context
        le_dls_Queue(&STACtxList, &staCtxPtr->link);

        LE_DEBUG("Context created for STA ID: %d", staCtxPtr->id);
    }

    // Memory for STA events
    StaEventsPoolRef = le_mem_InitStaticPool(StaEventsPool, TAF_WLAN_MAX_SESSION_REF,
                                                          sizeof(StaEvents_t));

    // Mutex for STA events
    STAEventsMutexRef = le_mutex_CreateNonRecursive("STAEventsMutex");

    // Create internal STA command event ref
    StaCommand = le_event_CreateId("StaCommand", sizeof(StaCmd_t));
    // Start Sta events thread
    StaCmdThreadRef = le_thread_Create("StaCmdThread", StaCmdThreadHdlr,NULL);
    le_thread_Start(StaCmdThreadRef);

    LE_INFO(" *** Wlan STA Initialized *** ");

    return;
}
