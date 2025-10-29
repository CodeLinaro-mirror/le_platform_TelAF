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
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace tafsvc;

// Memory pool for STA events for applications
LE_MEM_DEFINE_STATIC_POOL(StaEventsPool,
                          TAF_WLAN_MAX_SESSION_REF,
                          (sizeof(StaEvents_t)));

// Memory pool for STA contexts
LE_MEM_DEFINE_STATIC_POOL ( tafWlanStaCtxPool,
                            TAF_WLAN_MAX_NUM_STA,
                            (sizeof(StaCtx_t)) );

namespace {
// Single active context (commands are serialized in this service)
static SuppFdCtx_t *gActiveWpaMonCtx = nullptr;

// Operation args for queued start/stop
struct MonOpArgs
{
    enum class Op { Start, Stop } op;
    SuppFdCtx_t *ctx;
    std::string intfName;                 // used for Start
    std::promise<le_result_t> *startProm; // used for Start
    std::promise<void> *stopProm;         // used for Stop
};

// FD monitor handler (runs on staClientEventsThreadRef_)
static void WpaFdHandlerSimple(int fd, short events)
{
    LE_UNUSED(events);
    SuppFdCtx_t *ctx = gActiveWpaMonCtx;
    if (!ctx || !ctx->ctrl || ctx->fd != fd)
        return;

    if (ctx->eventCompleted || ctx->EventsToMonitor.empty())
        return;

    char rsp[WPA_CTRL_RSP_BUF_LEN] = {0};
    size_t len = sizeof(rsp);
    if (wpa_ctrl_recv(ctx->ctrl, rsp, &len) < 0)
    {
        std::lock_guard<std::mutex> lock(ctx->mutex);
        ctx->resultEvent = EVT_WPA_ERROR;
        ctx->eventCompleted = true;
        ctx->eventCv.notify_all();
        return;
    }

    std::string evt(rsp, len);
    for (const auto &evtToMonitor : ctx->EventsToMonitor)
    {
        if (evt.find(evtToMonitor) != std::string::npos)
        {
            StaWpaEvt_e matched = EVT_WPA_ERROR;
            if (evtToMonitor == WPA_EVENT_SCAN_RESULTS)
                matched = EVT_WPA_AP_SCAN_DONE;
            else if (evtToMonitor == WPA_EVENT_CONNECTED)
                matched = EVT_WPA_AP_CONNECTED;
            else if (evtToMonitor == WPA_EVENT_TEMP_DISABLED)
                matched = EVT_WPA_AP_TEMP_DISABLED;
            else if (evtToMonitor == WPA_EVENT_DISCONNECTED)
                matched = EVT_WPA_AP_DISCONNECTED;

            {
                std::lock_guard<std::mutex> lock(ctx->mutex);
                ctx->resultEvent = matched;
                ctx->eventCompleted = true;
            }
            ctx->eventCv.notify_all();
            return;
        }
    }
}

// Runs on staClientEventsThreadRef_: create or delete monitor
static void QueueWpaMonOp(void *param1Ptr, void *param2Ptr)
{
    LE_UNUSED(param2Ptr);
    auto *args = static_cast<MonOpArgs *>(param1Ptr);
    if (!args || !args->ctx)
        return;

    if (args->op == MonOpArgs::Op::Start)
    {
        le_result_t res = LE_FAULT;

        do
        {
            // Open/attach ctrl
            std::string path = std::string(WPA_SUPPLICANT_LOCATION_PATH) + args->intfName;
            struct wpa_ctrl *ctrl = wpa_ctrl_open(path.c_str());
            if (!ctrl)
                break;

            if (wpa_ctrl_attach(ctrl) != 0)
            {
                wpa_ctrl_close(ctrl); break;
            }

            int fd = wpa_ctrl_get_fd(ctrl);
            if (fd < 0)
            {
                wpa_ctrl_detach(ctrl);
                wpa_ctrl_close(ctrl);
                break;
            }

            auto monRef = le_fdMonitor_Create("WpaCtrlMon", fd, WpaFdHandlerSimple, POLLIN);
            if (!monRef)
            {
                wpa_ctrl_detach(ctrl);
                wpa_ctrl_close(ctrl);
                break;
            }

            args->ctx->ctrl = ctrl;
            args->ctx->fd = fd;
            args->ctx->fdMonitorRef = monRef;
            args->ctx->resultEvent = EVT_WPA_ERROR;
            args->ctx->eventCompleted = false;

            gActiveWpaMonCtx = args->ctx;
            res = LE_OK;
        } while (0);

        if (args->startProm)
            args->startProm->set_value(res);
    }
    else
    {
        // Stop
        auto *ctx = args->ctx;

        // Wake any waiter
        {
            std::lock_guard<std::mutex> lock(ctx->mutex);
            if (!ctx->eventCompleted)
            {
                ctx->resultEvent = EVT_WPA_ERROR;
                ctx->eventCompleted = true;
            }
        }
        ctx->eventCv.notify_all();

        if (ctx->fdMonitorRef)
        {
            le_fdMonitor_Delete(ctx->fdMonitorRef);
            ctx->fdMonitorRef = nullptr;
        }
        if (ctx->ctrl)
        {
            wpa_ctrl_detach(ctx->ctrl);
            wpa_ctrl_close(ctx->ctrl);
            ctx->ctrl = nullptr;
        }
        if (gActiveWpaMonCtx == ctx)
            gActiveWpaMonCtx = nullptr;

        if (args->stopProm)
            args->stopProm->set_value();
    }

    delete args;
}

} // anonymous namespace

bool taf_WlanSTASvcImpl::IsConnectedToSSID(StaCtx_t *CtxPtr, const char *targetSsid)
{
    if (CtxPtr == nullptr || targetSsid == nullptr || strlen(targetSsid) == 0)
    {
        LE_ERROR("IsConnectedToSSID bad params");
        return false;
    }

    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};
    le_result_t res = runWPACommand(CtxPtr, "STATUS", rsp_buf, sizeof(rsp_buf));
    if (res != LE_OK)
    {
        LE_WARN("STATUS command failed");
        return false;
    }

    std::string status(rsp_buf);

    // Must be in COMPLETED state
    if (status.find("wpa_state=COMPLETED") == std::string::npos)
    {
        LE_INFO("IsConnectedToSSID: wpa_state not COMPLETED");
        return false;
    }

    // Helper to trim CR/LF and spaces at both ends and strip quotes
    auto normalize = [](std::string s) -> std::string
    {
        // trim end
        while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' '
            || s.back() == '\t'))
        {
            s.pop_back();
        }

        // trim start
        while (!s.empty() && (s.front() == '\r' || s.front() == '\n' || s.front() == ' '
            || s.front() == '\t'))
        {
            s.erase(s.begin());
        }

        // strip surrounding quotes
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
            s = s.substr(1, s.size() - 2);
        {
            return s;
        }
    };

    const std::string target = normalize(std::string(targetSsid));

    // 1) Try ssid= line
    {
        const std::string key = "ssid=";
        size_t pos = status.find(key);
        if (pos != std::string::npos)
        {
            size_t end = status.find('\n', pos);
            std::string ssidVal = status.substr(pos + key.length(),
                    (end == std::string::npos) ? std::string::npos : (end - (pos + key.length())));
            ssidVal = normalize(ssidVal);

            LE_INFO("IsConnectedToSSID: STATUS ssid='%s', target='%s'",
                     ssidVal.c_str(), target.c_str());
            if (!ssidVal.empty() && ssidVal == target)
            {
                return true;
            }
        }
    }

    // 2) Try id=<n> + GET_NETWORK <n> ssid
    int currentId = -1;
    {
        const std::string idKey = "id=";
        size_t pos = status.find(idKey);
        if (pos != std::string::npos)
        {
            pos += idKey.length();
            size_t end = status.find('\n', pos);
            std::string idStr = status.substr(pos, (end == std::string::npos) ?
                std::string::npos : (end - pos));
            // Trim any trailing junk
            idStr = normalize(idStr);
            try
            {
                currentId = std::stoi(idStr);
            }
            catch (...)
            {
                currentId = -1;
            }
        }
    }
    if (currentId >= 0)
    {
        char getBuf[WPA_CTRL_RSP_BUF_LEN] = {0};
        std::string getCmd = "GET_NETWORK " + std::to_string(currentId) + " ssid";
        if (runWPACommand(CtxPtr, getCmd.c_str(), getBuf, sizeof(getBuf)) == LE_OK)
        {
            std::string netSsid = normalize(std::string(getBuf));
            LE_INFO("IsConnectedToSSID: GET_NETWORK id=%d ssid='%s', target='%s'",
                     currentId, netSsid.c_str(), target.c_str());
            if (!netSsid.empty() && netSsid == target)
            {
                return true;
            }
        }
    }

    // 3) Fallback: LIST_NETWORKS, find [CURRENT] row and compare SSID
    {
        char listBuf[WPA_CTRL_RSP_BUF_LEN] = {0};
        if (runWPACommand(CtxPtr, "LIST_NETWORKS", listBuf, sizeof(listBuf)) == LE_OK)
        {
            // Rows: "network id\tssid\tbssid\tflags"
            std::istringstream ss(listBuf);
            std::string line;
            while (std::getline(ss, line))
            {
                if (line.find("[CURRENT]") == std::string::npos)
                {
                    continue;
                }

                // Split by tab; SSID is column 2
                std::vector<std::string> cols;
                std::istringstream ls(line);
                std::string tok;

                while (std::getline(ls, tok, '\t'))
                {
                    cols.push_back(tok);
                }

                if (cols.size() >= 2)
                {
                    std::string curSsid = normalize(cols[1]);
                    LE_INFO("IsConnectedToSSID: LIST_NETWORKS CURRENT ssid='%s', target='%s'",
                             curSsid.c_str(), target.c_str());
                    if (!curSsid.empty() && curSsid == target)
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool taf_WlanSTASvcImpl::WaitForSupplicantEvent(SuppFdCtx_t *ctx, int timeoutMs,
    StaWpaEvt_e &outEvt)
{
    if (!ctx)
    {
        return false;
    }
    std::unique_lock<std::mutex> lock(ctx->mutex);
    bool signaled = ctx->eventCv.wait_for(
        lock, std::chrono::milliseconds(timeoutMs),
        [&]{ return ctx->eventCompleted; });

    if (!signaled)
    {
        return false;
    }
    outEvt = ctx->resultEvent;
    return true;
}

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

    if (CtxPtr->IntfName == nullptr || strlen(CtxPtr->IntfName) == 0)
    {
        LE_ERROR("Invalid interface name in context");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }

    // Prepare monitor context
    auto *fdCtxPtr = new (std::nothrow) SuppFdCtx_t();
    if (!fdCtxPtr)
    {
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }
    fdCtxPtr->ctrl = nullptr; fdCtxPtr->fd = -1; fdCtxPtr->fdMonitorRef = nullptr;
    fdCtxPtr->EventsToMonitor = { WPA_EVENT_SCAN_RESULTS };
    fdCtxPtr->resultEvent = EVT_WPA_ERROR; fdCtxPtr->eventCompleted = false;

    // Start monitor in client-events thread
    std::promise<le_result_t> startProm;
    auto startFut = startProm.get_future();
    auto *startArgs = new (std::nothrow) MonOpArgs{ MonOpArgs::Op::Start, fdCtxPtr,
                                                    std::string(CtxPtr->IntfName),
                                                    &startProm, nullptr };
    if (!startArgs)
    {
        delete fdCtxPtr;
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }

    le_event_QueueFunctionToThread(staClientEventsThreadRef_, QueueWpaMonOp, startArgs, nullptr);

    le_result_t result = LE_FAULT;
    try
    {
        result = startFut.get();
    }
    catch (...)
    {
        LE_ERROR("Exception while waiting for monitor start");
    }

    if (result != LE_OK)
    {
        delete fdCtxPtr;
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }

    // Issue SCAN
    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};
    if (runWPACommand(CtxPtr, "SCAN", rsp_buf, sizeof(rsp_buf)) != LE_OK ||
        strncmp(rsp_buf, "OK", 2) != 0)
    {
        // Stop monitor and cleanup
        std::promise<void> stopProm; auto stopFut = stopProm.get_future();
        auto *stopArgs = new MonOpArgs{ MonOpArgs::Op::Stop, fdCtxPtr, {}, nullptr, &stopProm };
        le_event_QueueFunctionToThread(staClientEventsThreadRef_, QueueWpaMonOp,
            stopArgs, nullptr);
        try
        {
            stopFut.get();
        }
        catch (...)
        {
            LE_ERROR("Exception while waiting for monitor stop");
        }
        delete fdCtxPtr;
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }

    // Wait for event
    StaWpaEvt_e evt = EVT_WPA_ERROR;
    bool ok = WaitForSupplicantEvent(fdCtxPtr, 15000, evt);

    // Stop monitor
    std::promise<void> stopProm; auto stopFut = stopProm.get_future();
    auto *stopArgs = new MonOpArgs{ MonOpArgs::Op::Stop, fdCtxPtr, {}, nullptr, &stopProm };
    le_event_QueueFunctionToThread(staClientEventsThreadRef_, QueueWpaMonOp, stopArgs, nullptr);
    try
    {
        stopFut.get();
    }
    catch (...)
    {
        LE_ERROR("Exception while waiting for monitor stop");
    }
    delete fdCtxPtr;

    if (!ok || evt != EVT_WPA_AP_SCAN_DONE)
    {
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }

    // Fetch results
    memset(rsp_buf, 0, sizeof(rsp_buf));
    if (runWPACommand(CtxPtr, "SCAN_RESULTS", rsp_buf, sizeof(rsp_buf)) != LE_OK)
    {
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_FAILED);
        return;
    }
    PopulateScanResults(CtxPtr, rsp_buf);
    ReportStaState(CtxPtr, TAF_WLANSTA_STATE_SCAN_COMPLETED);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets WPA2 PSK for an Access Point.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::SetWpa2Psk(
    taf_wlanSta_WlanSTARef_t staRef,
    const taf_wlanSta_APInfo_t *LE_NONNULL ApInfo,
    const char *LE_NONNULL psk)
{
    if (ApInfo->secAuthMethod != TAF_WLAN_SEC_AUTH_METHOD_PSK)
    {
        LE_INFO("Other authentication methods are not supported yet");
        return LE_UNSUPPORTED;
    }

    StaCtx_t *CtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(CtxPtr == nullptr, LE_FAULT, "Unable to find context");

    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};
    std::string wpaReqCmd;
    le_result_t res;

    std::string netID = CheckNetworkAdded(CtxPtr, ApInfo);
    bool isNetNotAdded = (netID == WPA_STA_NET_NOT_ADDED);
    if (isNetNotAdded)
    {
        // ADD_NETWORK
        wpaReqCmd = "ADD_NETWORK";
        res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
        if (res == LE_FAULT)
        {
            ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
            return LE_FAULT;
        }
        netID = rsp_buf;
    }

    if (netID.substr(0, 4) == "FAIL")
    {
        LE_ERROR("ADD_NETWORK failed");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }

    LE_INFO("Network ID: %s", netID.c_str());
    mNetID = netID;

    // Set SSID
    memset(rsp_buf, 0, sizeof(rsp_buf));
    wpaReqCmd = "SET_NETWORK " + netID + " ssid \"" + std::string(ApInfo->SSID) + "\"";
    res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    if (res == LE_FAULT || strncmp(rsp_buf, "OK", 2) != 0)
    {
        LE_ERROR("SET_NETWORK(ssid) failed");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }

    // Set PSK
    memset(rsp_buf, 0, sizeof(rsp_buf));
    wpaReqCmd = "SET_NETWORK " + netID + " psk \"" + std::string(psk) + "\"";
    res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    if (res == LE_FAULT || strncmp(rsp_buf, "OK", 2) != 0)
    {
        LE_ERROR("SET_NETWORK(psk) failed");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
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
    if (ApInfo->secAuthMethod != TAF_WLAN_SEC_AUTH_METHOD_PSK)
    {
        LE_INFO("Other authentication methods are not supported yet");
        return LE_UNSUPPORTED;
    }

    StaCtx_t *staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(!staCtxPtr, LE_FAULT, "Unable to find context");

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
    TAF_ERROR_IF_RET_VAL(!CtxPtr, LE_FAULT, "Null context");
    TAF_ERROR_IF_RET_VAL(strlen(ApInfo->SSID) == 0, LE_BAD_PARAMETER, "SSID is empty");

    if (mNetID.empty())
    {
        LE_ERROR("Network not added");
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }

    if (IsConnectedToSSID(CtxPtr, ApInfo->SSID))
    {
        LE_INFO("Already connected to AP %s; reporting CONNECTED", ApInfo->SSID);
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_CONNECTED);
        return LE_OK;
    }

    // Prepare monitor context
    SuppFdCtx_t *fdCtxPtr = new (std::nothrow) SuppFdCtx_t();
    if (!fdCtxPtr)
    {
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }
    fdCtxPtr->ctrl = nullptr;
    fdCtxPtr->fd = -1;
    fdCtxPtr->fdMonitorRef = nullptr;
    fdCtxPtr->EventsToMonitor = { WPA_EVENT_CONNECTED, WPA_EVENT_TEMP_DISABLED };
    fdCtxPtr->resultEvent = EVT_WPA_ERROR;
    fdCtxPtr->eventCompleted = false;

    // Local cleanup helper to stop monitor (queued) and free ctx
    auto stopMonitorAndDelete = [&](SuppFdCtx_t *ctx)
    {
        if (!ctx) return;
        std::promise<void> stopProm; auto stopFut = stopProm.get_future();
        auto *stopArgs = new MonOpArgs{ MonOpArgs::Op::Stop, ctx, {}, nullptr, &stopProm };
        le_event_QueueFunctionToThread(staClientEventsThreadRef_, QueueWpaMonOp,
            stopArgs, nullptr);
        try
        {
            stopFut.get();
        }
        catch (...)
        {
            LE_ERROR("Exception while waiting for monitor stop");
        }
        delete ctx;
    };

    // Start monitor in client-events thread
    std::promise<le_result_t> startProm; auto startFut = startProm.get_future();
    auto *startArgs = new (std::nothrow) MonOpArgs{
        MonOpArgs::Op::Start, fdCtxPtr, std::string(CtxPtr->IntfName), &startProm, nullptr
    };
    if (!startArgs)
    {
        delete fdCtxPtr;
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }
    le_event_QueueFunctionToThread(staClientEventsThreadRef_, QueueWpaMonOp, startArgs, nullptr);

    le_result_t res = LE_FAULT;
    try
    {
        res = startFut.get();
    }
    catch (...)
    {
        LE_ERROR("Exception while waiting for monitor start");
    }

    if (res != LE_OK)
    {
        delete fdCtxPtr;
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }

    // Issue WPA commands
    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};

    std::string wpaReqCmd = "SELECT_NETWORK " + mNetID;
    res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    if (res == LE_FAULT || strncmp(rsp_buf, "OK", 2) != 0)
    {
        stopMonitorAndDelete(fdCtxPtr);
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }

    memset(rsp_buf, 0, sizeof(rsp_buf));
    wpaReqCmd = "ENABLE_NETWORK " + mNetID;
    res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    if (res == LE_FAULT || strncmp(rsp_buf, "OK", 2) != 0)
    {
        stopMonitorAndDelete(fdCtxPtr);
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }

    memset(rsp_buf, 0, sizeof(rsp_buf));
    wpaReqCmd = "RECONNECT";
    res = runWPACommand(CtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    if (res == LE_FAULT || strncmp(rsp_buf, "OK", 2) != 0)
    {
        stopMonitorAndDelete(fdCtxPtr);
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
        return LE_FAULT;
    }

    // Wait for connection result with timeout
    StaWpaEvt_e resultEvent = EVT_WPA_ERROR;
    bool signaled = WaitForSupplicantEvent(fdCtxPtr, 15000 /* ms */, resultEvent);

    // Cleanup monitor
    stopMonitorAndDelete(fdCtxPtr);

    if (signaled)
    {
        if (resultEvent == EVT_WPA_AP_CONNECTED)
        {
            LE_INFO("Connection to AP %s was successful", ApInfo->SSID);
            ReportStaState(CtxPtr, TAF_WLANSTA_STATE_CONNECTED);
            return LE_OK;
        }
        else if (resultEvent == EVT_WPA_AP_TEMP_DISABLED)
        {
            LE_ERROR("Authentication failed for AP %s", ApInfo->SSID);
            ReportStaState(CtxPtr, TAF_WLANSTA_STATE_ASSOCIATION_FAILED);
            return LE_FAULT;
        }
    }

    // Final fallback on timeout: verify STATUS
    if (IsConnectedToSSID(CtxPtr, ApInfo->SSID))
    {
        LE_INFO("Connected to AP %s (detected via STATUS) after timeout", ApInfo->SSID);
        ReportStaState(CtxPtr, TAF_WLANSTA_STATE_CONNECTED);
        return LE_OK;
    }

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
    const taf_wlanSta_APInfo_t *LE_NONNULL ApInfo)
{
    StaCtx_t *staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(!staCtxPtr, LE_FAULT, "Unable to find context");

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

    // Disable network
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

    LE_INFO("Reporting DISCONNECTED state");
    ReportStaState(CtxPtr, TAF_WLANSTA_STATE_DISCONNECTED);
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
    StaCtx_t *staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    std::string ctrlPath(WPA_SUPPLICANT_LOCATION_PATH);
    ctrlPath += staCtxPtr->IntfName;

    struct wpa_ctrl *ctrl = wpa_ctrl_open(ctrlPath.c_str());
    if (!ctrl)
    {
        LE_WARN("wpa_supplicant control socket not available at %s", ctrlPath.c_str());
        return LE_FAULT;
    }
    wpa_ctrl_close(ctrl);

    LE_INFO("wpa_supplicant control socket OK for %s", staCtxPtr->IntfName);
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
    StaCtx_t *staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};
    le_result_t res = runWPACommand(staCtxPtr, "DISCONNECT", rsp_buf, sizeof(rsp_buf));
    if (res != LE_OK || strncmp(rsp_buf, "OK", 2) != 0)
    {
        LE_WARN("DISCONNECT failed or returned: %s", rsp_buf);
        return LE_FAULT;
    }

    LE_INFO("STA disconnected: %s", staCtxPtr->IntfName);
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
    StaCtx_t *staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};

    runWPACommand(staCtxPtr, "DISCONNECT", rsp_buf, sizeof(rsp_buf));
    memset(rsp_buf, 0, sizeof(rsp_buf));
    if (runWPACommand(staCtxPtr, "RECONNECT", rsp_buf, sizeof(rsp_buf)) != LE_OK ||
        strncmp(rsp_buf, "OK", 2) != 0)
    {
        LE_WARN("RECONNECT failed or returned: %s", rsp_buf);
        return LE_FAULT;
    }

    LE_INFO("STA restart (disconnect/reconnect) OK: %s", staCtxPtr->IntfName);
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
    TAF_ERROR_IF_RET_VAL(!StaSatePtr, LE_BAD_PARAMETER, "StaSatePtr is NULL");

    StaCtx_t *staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    *StaSatePtr = TAF_WLANSTA_STATE_DISCONNECTED;
    if (IntfName && IntfNameSize)
        le_utf8_Copy(IntfName, staCtxPtr->IntfName, TAF_NET_INTERFACE_NAME_MAX_LEN + 1, NULL);
    if (IPv4Address && IPv4AddressSize)
        IPv4Address[0] = '\0';
    if (IPv6Address && IPv6AddressSize)
        IPv6Address[0] = '\0';
    if (MACAddress && MACAddressSize)
        MACAddress[0] = '\0';

    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};
    if (runWPACommand(staCtxPtr, "STATUS", rsp_buf, sizeof(rsp_buf)) == LE_OK)
    {
        std::string status(rsp_buf);

        auto findVal = [&](const std::string &key) -> std::string
        {
            size_t pos = status.find(key);
            if (pos == std::string::npos)
                return "";
            pos += key.size();
            size_t end = status.find('\n', pos);
            return status.substr(pos, (end == std::string::npos) ? std::string::npos : (end - pos));
        };

        std::string wpaState = findVal("wpa_state=");
        if (wpaState == "COMPLETED")
            *StaSatePtr = TAF_WLANSTA_STATE_CONNECTED;
        else if (wpaState == "SCANNING" || wpaState == "ASSOCIATING" ||
                 wpaState == "ASSOCIATED" || wpaState == "4WAY_HANDSHAKE")
            *StaSatePtr = TAF_WLANSTA_STATE_CONNECTING;
        else
            *StaSatePtr = TAF_WLANSTA_STATE_DISCONNECTED;

        std::string ip4 = findVal("ip_address=");
        std::string ip6 = findVal("ipv6_address=");
        if (IPv4Address && IPv4AddressSize && !ip4.empty())
            le_utf8_Copy(IPv4Address, ip4.c_str(), TAF_NET_IPV4_ADDR_MAX_LEN + 1, NULL);
        if (IPv6Address && IPv6AddressSize && !ip6.empty())
            le_utf8_Copy(IPv6Address, ip6.c_str(), TAF_NET_IPV6_ADDR_MAX_LEN + 1, NULL);
    }

    // Fallback to getifaddrs if needed
    if ((IPv4Address && IPv4AddressSize && IPv4Address[0] == '\0') ||
        (IPv6Address && IPv6AddressSize && IPv6Address[0] == '\0'))
    {
        struct ifaddrs *ifa = nullptr;
        if (getifaddrs(&ifa) == 0)
        {
            for (struct ifaddrs *p = ifa; p; p = p->ifa_next)
            {
                if (!p->ifa_name || strcmp(p->ifa_name, staCtxPtr->IntfName) != 0 || !p->ifa_addr)
                    continue;

                char abuf[INET6_ADDRSTRLEN] = {0};
                if (p->ifa_addr->sa_family == AF_INET && IPv4Address && IPv4AddressSize)
                {
                    auto *sin = (struct sockaddr_in *)p->ifa_addr;
                    if (inet_ntop(AF_INET, &sin->sin_addr, abuf, sizeof(abuf)))
                        le_utf8_Copy(IPv4Address, abuf, TAF_NET_IPV4_ADDR_MAX_LEN + 1, NULL);
                }
                else if (p->ifa_addr->sa_family == AF_INET6 && IPv6Address && IPv6AddressSize)
                {
                    auto *sin6 = (struct sockaddr_in6 *)p->ifa_addr;
                    if (inet_ntop(AF_INET6, &sin6->sin6_addr, abuf, sizeof(abuf)))
                        le_utf8_Copy(IPv6Address, abuf, TAF_NET_IPV6_ADDR_MAX_LEN + 1, NULL);
                }
            }
            freeifaddrs(ifa);
        }
    }

    // MAC from sysfs
    if (MACAddress && MACAddressSize)
    {
        char mac[32] = {0};
        char path[128] = {0};
        snprintf(path, sizeof(path), "/sys/class/net/%s/address", staCtxPtr->IntfName);
        int fd = open(path, O_RDONLY);
        if (fd >= 0)
        {
            ssize_t n = read(fd, mac, sizeof(mac) - 1);
            close(fd);
            if (n > 0)
            {
                while (n > 0 && (mac[n - 1] == '\n' || mac[n - 1] == '\r'))
                    --n;
                mac[n] = '\0';
                le_utf8_Copy(MACAddress, mac, TAF_NET_MAC_ADDR_MAX_LEN + 1, NULL);
            }
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
    StaCtx_t *staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

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
    TAF_ERROR_IF_RET_VAL(numAPPtr == nullptr, LE_BAD_PARAMETER, "numAPPtr is NULL");
    TAF_ERROR_IF_RET_VAL(ApInfoPtr == nullptr, LE_BAD_PARAMETER, "ApInfoPtr is NULL");
    TAF_ERROR_IF_RET_VAL(ApInfoSizePtr == nullptr, LE_BAD_PARAMETER, "ApInfoSizePtr is NULL");
    TAF_ERROR_IF_RET_VAL(*ApInfoSizePtr == 0, LE_BAD_PARAMETER, "ApInfoSizePtr is 0");

    StaCtx_t *staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(NULL == staCtxPtr, LE_FAULT, "Unable to find context");

    // Lazy refresh scan results if empty
    if (staCtxPtr->numScannedAPs == 0)
    {
        char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};
        if (runWPACommand(staCtxPtr, "SCAN_RESULTS", rsp_buf, sizeof(rsp_buf)) == LE_OK)
        {
            PopulateScanResults(staCtxPtr, rsp_buf);
        }
    }

    *numAPPtr = staCtxPtr->numScannedAPs;
    size_t copyCount = std::min(static_cast<size_t>(staCtxPtr->numScannedAPs), *ApInfoSizePtr);
    *ApInfoSizePtr = copyCount;

    for (size_t i = 0; i < copyCount; ++i)
    {
        // Copy fields
        le_utf8_Copy(ApInfoPtr[i].BSSID, staCtxPtr->ApInfo[i].BSSID,
                     TAF_WLAN_MAX_BSSID_LENGTH + 1, NULL);
        le_utf8_Copy(ApInfoPtr[i].SSID, staCtxPtr->ApInfo[i].SSID,
                     TAF_WLAN_MAX_SSID_LENGTH + 1, NULL);

        ApInfoPtr[i].SignalLevel = staCtxPtr->ApInfo[i].SignalLevel;
        ApInfoPtr[i].Frequency = staCtxPtr->ApInfo[i].Frequency;
        ApInfoPtr[i].SS = staCtxPtr->ApInfo[i].SS;
        ApInfoPtr[i].secMode = staCtxPtr->ApInfo[i].secMode;
        ApInfoPtr[i].secAuthMethod = staCtxPtr->ApInfo[i].secAuthMethod;
        ApInfoPtr[i].secEncryptionMethod = staCtxPtr->ApInfo[i].secEncryptionMethod;
        ApInfoPtr[i].WPSEnabled = staCtxPtr->ApInfo[i].WPSEnabled;
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
 * Extract estimated throughput value from BSS output string
 *
 * @return
 * - LE_OK if throughput extracted successfully
 * - LE_FAULT if key not found
 * - LE_BAD_PARAMETER if value is empty or invalid
 */
//--------------------------------------------------------------------------------------------------
inline le_result_t taf_WlanSTASvcImpl::BSSParser::extractEstThroughput
(
    const std::string& bssOutput,
    int& estTput
)
{
    const std::string key = "est_throughput=";
    size_t pos = bssOutput.find(key);
    TAF_ERROR_IF_RET_VAL(pos == std::string::npos, LE_FAULT,
        "est_throughput key not found in BSS output");

    pos += key.length();
    size_t end = bssOutput.find_first_not_of("0123456789", pos);
    std::string numberStr = bssOutput.substr(pos, end - pos);
    TAF_ERROR_IF_RET_VAL(numberStr.empty(), LE_BAD_PARAMETER,
        "Empty est_throughput value in BSS output");

    try
    {
        estTput = std::stoi(numberStr);
        return LE_OK;
    }
    catch (const std::exception& e)
    {
        LE_ERROR("Failed to parse est_throughput value '%s': %s", numberStr.c_str(), e.what());
        return LE_BAD_PARAMETER;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Extract age of measurement from BSS output string
 *
 * @return
 * - LE_OK if age extracted successfully
 * - LE_FAULT if key not found
 * - LE_BAD_PARAMETER if value is empty or invalid
 */
//--------------------------------------------------------------------------------------------------
inline le_result_t taf_WlanSTASvcImpl::BSSParser::extractAge
(
    const std::string& bssOutput,
    int& age
)
{
    const std::string key = "age=";
    size_t pos = bssOutput.find(key);
    TAF_ERROR_IF_RET_VAL(pos == std::string::npos, LE_FAULT,
        "age key not found in BSS output");

    pos += key.length();
    size_t end = bssOutput.find_first_not_of("0123456789", pos);
    std::string numberStr = bssOutput.substr(pos, end - pos);
    TAF_ERROR_IF_RET_VAL(numberStr.empty(), LE_BAD_PARAMETER,
        "Empty age value in BSS output");

    try
    {
        age = std::stoi(numberStr);
        return LE_OK;
    }
    catch (const std::exception& e)
    {
        LE_ERROR("Failed to parse age value '%s': %s", numberStr.c_str(), e.what());
        return LE_BAD_PARAMETER;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the AP estimated throughput.
 *
 * @return
 * - LE_OK           -- Succeeded.
 * - LE_NOT_FOUND    -- BSSID not found in scan results.
 * - LE_UNAVAILABLE  -- Estimated throughput information not available.
 * - Others          -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::GetAPEstimatedThroughput
(
    taf_wlanSta_WlanSTARef_t staRef,
    const char* BSSID,
    uint32_t* estimatedThroughputPtr,
    int32_t* agePtr
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == BSSID, LE_BAD_PARAMETER, "BSSID is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == estimatedThroughputPtr, LE_BAD_PARAMETER,
        "estimatedThroughputPtr is NULL");
    TAF_ERROR_IF_RET_VAL(nullptr == agePtr, LE_BAD_PARAMETER, "agePtr is NULL");

    // Initialize age to default negative value
    *agePtr = -1;

    StaCtx_t *staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(nullptr == staCtxPtr, LE_FAULT, "Unable to find context");

    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};
    std::string wpaReqCmd = "BSS " + std::string(BSSID);
    le_result_t res = runWPACommand(staCtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Failed to execute BSS command");

    // Check if BSSID was found
    TAF_ERROR_IF_RET_VAL(strncmp(rsp_buf, "FAIL", 4) == 0, LE_NOT_FOUND,
        "BSSID %s not found in scan results", BSSID);

    int throughput = 0;
    le_result_t tRes = BSSParser::extractEstThroughput(rsp_buf, throughput);
    if (tRes == LE_FAULT) {
        // est_throughput key not present -> information unavailable
        return LE_UNAVAILABLE;
    }
    if (tRes != LE_OK) {
        // malformed or empty value -> propagate LE_BAD_PARAMETER
        return tRes;
    }

    // Extract age (best-effort)
    int age = -1;
    le_result_t aRes = BSSParser::extractAge(rsp_buf, age);
    if (aRes == LE_OK) {
        *agePtr = static_cast<int32_t>(age);
    } else {
        LE_WARN("Failed to extract age for BSSID %s: %d (using default -1)", BSSID, aRes);
    }

    *estimatedThroughputPtr = static_cast<uint32_t>(throughput);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes a network configuration from the station.
 *
 * @return
 * - LE_OK          -- Succeeded.
 * - LE_NOT_FOUND   -- Network not found.
 * - Others         -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::RemoveNetwork(
    taf_wlanSta_WlanSTARef_t staRef,
    const taf_wlanSta_APInfo_t *LE_NONNULL ApInfo)
{
    TAF_ERROR_IF_RET_VAL(0 == strlen(ApInfo->SSID), LE_BAD_PARAMETER, "SSID is empty");

    StaCtx_t *staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(!staCtxPtr, LE_FAULT, "Unable to find context");

    // Check if network is added
    std::string netID = CheckNetworkAdded(staCtxPtr, ApInfo);
    if (netID == WPA_STA_NET_NOT_ADDED || netID.substr(0, 4) == "FAIL")
    {
        LE_INFO("Network %s not found in configured networks", ApInfo->SSID);
        return LE_NOT_FOUND;
    }

    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};

    // If currently connected to this SSID, disconnect
    memset(rsp_buf, 0, WPA_CTRL_RSP_BUF_LEN);
    if (runWPACommand(staCtxPtr, "STATUS", rsp_buf, sizeof(rsp_buf)) == LE_OK)
    {
        std::string st(rsp_buf);
        size_t pos = st.find("ssid=");
        if (pos != std::string::npos)
        {
            pos += 5;
            size_t end = st.find('\n', pos);
            std::string cur = st.substr(pos, (end == std::string::npos) ? std::string::npos : (end - pos));
            if (cur == std::string(ApInfo->SSID))
            {
                // Best-effort DISCONNECT
                char tmp[WPA_CTRL_RSP_BUF_LEN] = {0};
                runWPACommand(staCtxPtr, "DISCONNECT", tmp, sizeof(tmp));
            }
        }
    }

    // Disable the network
    memset(rsp_buf, 0, WPA_CTRL_RSP_BUF_LEN);
    std::string wpaReqCmd = "DISABLE_NETWORK " + netID;
    le_result_t res = runWPACommand(staCtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    if (res != LE_OK)
    {
        LE_WARN("DISABLE_NETWORK %s failed", netID.c_str());
        // continue to remove anyway
    }

    // Remove the network
    memset(rsp_buf, 0, WPA_CTRL_RSP_BUF_LEN);
    wpaReqCmd = "REMOVE_NETWORK " + netID;
    res = runWPACommand(staCtxPtr, wpaReqCmd.c_str(), rsp_buf, sizeof(rsp_buf));
    if (res != LE_OK || strncmp(rsp_buf, "OK", 2) != 0)
    {
        LE_ERROR("REMOVE_NETWORK failed: %s", rsp_buf);
        return LE_FAULT;
    }

    LE_INFO("Removed network %s (ID: %s)", ApInfo->SSID, netID.c_str());
    ReportStaState(staCtxPtr, TAF_WLANSTA_STATE_NETWORK_REMOVED);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Saves the current network configuration persistently.
 *
 * @return
 * - LE_OK      -- Succeeded.
 * - LE_FAULT   -- Failed to save configuration.
 * - Others     -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSTASvcImpl::SaveNetworkConfig(
    taf_wlanSta_WlanSTARef_t staRef
)
{
    StaCtx_t *staCtxPtr = (StaCtx_t *)le_ref_Lookup(StaRefMap, (void *)staRef);
    TAF_ERROR_IF_RET_VAL(!staCtxPtr, LE_FAULT, "Unable to find context");

    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};
    if (runWPACommand(staCtxPtr, "SAVE_CONFIG", rsp_buf, sizeof(rsp_buf)) != LE_OK ||
        strncmp(rsp_buf, "OK", 2) != 0)
    {
        LE_ERROR("SAVE_CONFIG failed or returned: %s", rsp_buf);
        return LE_FAULT;
    }

    LE_INFO("Network configuration saved successfully");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * taf_WlanAPSvcImpl Init function
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::Init()
{
    // STA contexts pool and mutex
    STACtxPoolRef = le_mem_InitStaticPool(tafWlanStaCtxPool, TAF_WLAN_MAX_NUM_STA, sizeof(StaCtx_t));
    STACtxMutex = le_mutex_CreateNonRecursive("STACtxMutex");
    StaRefMap = le_ref_CreateMap("StaRefMap", TAF_WLAN_MAX_NUM_STA);

    // Create contexts and per-STA resources
    for (int i = 1; i <= TAF_WLAN_MAX_NUM_STA; ++i)
    {
        StaCtx_t *staCtxPtr = (StaCtx_t *)le_mem_ForceAlloc(STACtxPoolRef);
        if (!staCtxPtr)
            LE_FATAL("Unable to allocate StaCtx for STA ID: %d", i);

        staCtxPtr->id = static_cast<taf_wlan_STAid_t>(i);
        staCtxPtr->IntfName[0] = '\0';
        staCtxPtr->numScannedAPs = 0;
        memset(staCtxPtr->ApInfo, 0, sizeof(staCtxPtr->ApInfo));
        memset(&staCtxPtr->ApInfoConnect, 0, sizeof(staCtxPtr->ApInfoConnect));

        taf_wlanSta_WlanSTARef_t staRef = (taf_wlanSta_WlanSTARef_t)le_ref_CreateRef(StaRefMap, (void *)staCtxPtr);
        if (!staRef)
            LE_FATAL("Unable to create ref for STA ID: %d", i);
        staCtxPtr->staRef = staRef;

        // Per-STA state change event
        std::string evtName = "StaStateEvent-" + std::to_string(staCtxPtr->id);
        staCtxPtr->StaEvent = le_event_CreateIdWithRefCounting(evtName.c_str());

        le_dls_Queue(&STACtxList, &staCtxPtr->link);

        // Create signal-strength monitor object for this STA
        sigStrengthMonitorRefMap_[staCtxPtr->id] =
            std::make_shared<taf_WlanStaConnectedApSignalStrengthMonitor>(staCtxPtr->id);
    }

    // STA events pool and mutex
    StaEventsPoolRef = le_mem_InitStaticPool(StaEventsPool, TAF_WLAN_MAX_SESSION_REF, sizeof(StaEvents_t));
    STAEventsMutexRef = le_mutex_CreateNonRecursive("STAEventsMutex");

    // Internal STA command event and thread
    staCommand_ = le_event_CreateId("staCommand_", sizeof(StaCmd_t));
    staCmdThreadRef_ = le_thread_Create("StaCmdThread", StaCmdThreadHdlr, NULL);
    le_thread_Start(staCmdThreadRef_);

    // Client events thread (timers and WPA monitors run here)
    staClientEventsThreadRef_ = le_thread_Create("StaEvtThread", staClientEventsThreadHandler, NULL);
    le_thread_Start(staClientEventsThreadRef_);

    // Register client connect/disconnect handlers
    registerClientsConnectDisconnectHandlers();

    LE_INFO(" *** Wlan STA Initialized *** ");
}
