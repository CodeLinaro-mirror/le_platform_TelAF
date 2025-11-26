/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanAPSvcImpl.cpp
 *
 * @brief      Server side implementation of TelAF WLAN Access Point Service APIs.
 *
 */

#include "tafWlan.hpp"
#include <wpa_ctrl.h>
#include "limit.h"
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string>
#include <vector>

using namespace tafsvc;

// Memory pool for AP contexts
LE_MEM_DEFINE_STATIC_POOL(tafWlanAPCtxPool, TAF_WLAN_MAX_NUM_AP, sizeof(taf_wlan_AP_Ctx_t));

// Memory pool for client connection event for applications
LE_MEM_DEFINE_STATIC_POOL(DeviceCnxEventPool, TAF_WLAN_MAX_SESSION_REF,
    (sizeof(DeviceCnxEvent_t)));

le_mutex_Ref_t taf_WlanAPSvcImpl::APCtxMutex = nullptr;

static std::vector<std::string> Split(const std::string &str, char delim)
{
    std::vector<std::string> out;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        out.push_back(item);
    }
    return out;
}

//--------------------------------------------------------------------------------------------------
/**
 * Trims a std::string in-place, removing leading and trailing ASCII whitespace.
 *
 * This helper ensures strings parsed from hostapd or system files are normalized
 * before validation or comparison.
 *
 * @param
 *  - s: Reference to the string to be trimmed.
 *
 */
//--------------------------------------------------------------------------------------------------
static inline void TrimInPlace
(
    std::string &s
)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    [](unsigned char ch)
                                    { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch)
                         { return !std::isspace(ch); })
                .base(),
            s.end());
}

//--------------------------------------------------------------------------------------------------
/**
 * Queries IPv4 and IPv6 addresses for a given network interface.
 *
 * @param
 *  - iface: Interface name (e.g., "wlan0").
 *  - ipv4Out: Output string to receive the IPv4 address (empty if none).
 *  - ipv6Out: Output string to receive the IPv6 address (empty if none).
 *
 * @return
 *  - void (results are returned through output parameters).
 *
 * Notes:
 *  - Uses getifaddrs() to enumerate interface addresses.
 *  - Only captures primary address per family; multiple addresses are not aggregated.
 */
//--------------------------------------------------------------------------------------------------
static void GetInterfaceAddrs
(
    const std::string &iface,
    std::string &ipv4Out,
    std::string &ipv6Out
)
{
    ipv4Out.clear();
    ipv6Out.clear();

    struct ifaddrs *ifa = nullptr;
    if (getifaddrs(&ifa) != 0)
        return;

    for (struct ifaddrs *p = ifa; p != nullptr; p = p->ifa_next)
    {
        if (!p->ifa_name || iface != p->ifa_name || !p->ifa_addr)
            continue;

        char addrBuf[INET6_ADDRSTRLEN] = {0};
        if (p->ifa_addr->sa_family == AF_INET)
        {
            auto *sin = reinterpret_cast<struct sockaddr_in *>(p->ifa_addr);
            if (inet_ntop(AF_INET, &sin->sin_addr, addrBuf, sizeof(addrBuf)))
                ipv4Out = addrBuf;
        }
        else if (p->ifa_addr->sa_family == AF_INET6)
        {
            auto *sin6 = reinterpret_cast<struct sockaddr_in6 *>(p->ifa_addr);
            if (inet_ntop(AF_INET6, &sin6->sin6_addr, addrBuf, sizeof(addrBuf)))
                ipv6Out = addrBuf;
        }
    }
    freeifaddrs(ifa);
}

//--------------------------------------------------------------------------------------------------
/**
 * Reads the MAC address of a network interface from sysfs.
 *
 * @param
 *  - iface: Interface name (e.g., "wlan0").
 *
 * @return
 *  - MAC address in the usual colon-separated format (e.g., "aa:bb:cc:dd:ee:ff"),
 *    or an empty string on failure.
 *
 * Notes:
 *  - Reads /sys/class/net/<iface>/address and trims trailing newlines.
 */
//--------------------------------------------------------------------------------------------------
static std::string GetInterfaceMac
(
    const std::string &iface
)
{
    std::string path = "/sys/class/net/" + iface + "/address";
    int fd = open(path.c_str(), O_RDONLY);
    if (fd >= 0)
    {
        char buf[64] = {0};
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        if (n > 0)
        {
            while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
                --n;
            return std::string(buf, n);
        }
    }
    return "";
}

//--------------------------------------------------------------------------------------------------
/**
 * Looks up the IPv4 address corresponding to a MAC address on a given interface via ARP.
 *
 * @param
 *  - iface: Interface name to match against the ARP table (e.g., "wlan0").
 *  - mac: MAC address to search for (case-insensitive).
 *
 * @return
 *  - IPv4 address as a string if found; otherwise an empty string.
 *
 * Notes:
 *  - Reads /proc/net/arp and matches both device name and hardware address (MAC).
 *  - This is a best-effort method; entries may be absent or stale.
 */
//--------------------------------------------------------------------------------------------------
static std::string GetIpForMac
(
    const std::string &iface,
    const std::string &mac
)
{
    FILE *fp = fopen("/proc/net/arp", "r");
    if (!fp)
        return "";

    char line[256];
    // Skip header - explicitly handle return value
    if (!fgets(line, sizeof(line), fp))
    {
        fclose(fp);
        return "";
    }

    std::string macLower(mac);
    std::transform(macLower.begin(), macLower.end(), macLower.begin(), ::tolower);

    while (fgets(line, sizeof(line), fp))
    {
        char ip[64], hw[64], dev[64];
        if (sscanf(line, "%63s %*s %*s %63s %*s %63s", ip, hw, dev) == 3)
        {
            std::string hwLower(hw);
            std::transform(hwLower.begin(), hwLower.end(), hwLower.begin(), ::tolower);

            if (hwLower == macLower && iface == dev)
            {
                fclose(fp);
                return std::string(ip);
            }
        }
    }
    fclose(fp);
    return "";
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a command to hostapd over its control socket and optionally retrieves the response.
 *
 * @param
 *  - iface: Interface name (e.g., "wlan0") used to locate the hostapd control socket.
 *  - cmd: Command string (e.g., "STATUS", "ENABLE", "SET ssid MyAP").
 *  - respOut: Optional pointer to std::string to receive the response (if non-null).
 *
 * @return
 *  - true on successful request/response exchange; false on failure.
 *
 * Behavior:
 *  - Serializes access via a static mutex to avoid concurrent requests on shared socket paths.
 *  - Tries multiple control socket base directories (HOSTAPD_LOCATION_PATH, /var/run/hostapd/,
 *    /run/hostapd/).
 *  - Validates control path length to avoid abstract UNIX socket overflow issues.
 *  - Ensures returned buffer is within bounds and contains printable characters before assigning
 *    to respOut.
 *
 * Notes:
 *  - This function only handles transport-layer success; callers should inspect respOut content
 *    (e.g., "OK", "FAIL", etc.) to determine semantic success.
 */
//--------------------------------------------------------------------------------------------------
static bool HostapdCommand
(
    const char *iface,
    const std::string &cmd,
    std::string *respOut = nullptr
)
{
    if (!iface || cmd.empty())
    {
        LE_WARN("Invalid @param iface=%p, cmd.empty()=%d", iface, cmd.empty());
        return false;
    }

    // Thread safety: serialize hostapd commands
    static le_mutex_Ref_t HostapdCmdMutex = le_mutex_CreateNonRecursive("HostapdCmdMutex");
    le_mutex_Lock(HostapdCmdMutex);

    const char *bases[] = {HOSTAPD_LOCATION_PATH, "/var/run/hostapd/", "/run/hostapd/"};
    struct wpa_ctrl *ctrl = nullptr;

    // Try to open control socket
    for (const char *base : bases)
    {
        if (!base || !*base)
            continue;
        std::string ctrlPath(base);
        if (ctrlPath.back() != '/')
            ctrlPath.push_back('/');
        ctrlPath += iface;

        // Validate path length to prevent buffer overflows in wpa_ctrl
        if (ctrlPath.length() >= 108) // UNIX_PATH_MAX
        {
            LE_WARN("Control path too long: %zu chars", ctrlPath.length());
            continue;
        }

        ctrl = wpa_ctrl_open(ctrlPath.c_str());
        if (ctrl)
        {
            LE_INFO("Opened ctrl: %s", ctrlPath.c_str());
            break;
        }
    }

    if (!ctrl)
    {
        LE_WARN("Failed to open hostapd ctrl for %s", iface);
        le_mutex_Unlock(HostapdCmdMutex);
        return false;
    }

    // Execute command with proper bounds checking
    char buf[4096] = {0};
    size_t len = sizeof(buf) - 1; // Reserve space for null terminator

    int rc = wpa_ctrl_request(ctrl, cmd.c_str(), cmd.size(), buf, &len, nullptr);

    // Always close control handle before checking results
    wpa_ctrl_close(ctrl);

    le_mutex_Unlock(HostapdCmdMutex);

    if (rc != 0)
    {
        LE_WARN("Command '%s' failed: rc=%d", cmd.c_str(), rc);
        return false;
    }

    // Critical: Validate len before using it
    if (len >= sizeof(buf))
    {
        LE_ERROR("Response buffer overflow: len=%zu, buf_size=%zu", len, sizeof(buf));
        return false;
    }

    buf[len] = '\0';

    if (respOut)
    {
        // Validate response is valid UTF-8 or printable ASCII
        bool valid = true;
        for (size_t i = 0; i < len; ++i)
        {
            if (buf[i] != '\0' && !std::isprint(static_cast<unsigned char>(buf[i])) &&
                !std::isspace(static_cast<unsigned char>(buf[i])))
            {
                LE_WARN("Non-printable character at position %zu: 0x%02x", i,
                        static_cast<unsigned char>(buf[i]));
                valid = false;
                break;
            }
        }

        if (valid)
        {
            respOut->assign(buf, len);
        }
        else
        {
            LE_ERROR("Invalid response data from hostapd");
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Extracts and validates a MAC address from an arbitrary response token.
 *
 * @param
 *  - resp: Input string token (may contain additional text).
 *
 * @return
 *  - A validated MAC address (17 chars, colon-separated) or an empty string if invalid.
 *
 * Behavior:
 *  - Considers the first token (before space/newline) as MAC candidate.
 *  - Verifies exactly 5 colons and correct hex digit positions.
 */
//--------------------------------------------------------------------------------------------------
static std::string ExtractMacFromResponse
(
    const std::string &resp
)
{
    if (resp.empty())
    {
        LE_INFO("Empty response");
        return "";
    }

    // MAC is always first token before space or newline
    auto spacePos = resp.find(' ');
    auto newlinePos = resp.find('\n');
    auto endPos = std::min(spacePos, newlinePos);

    std::string mac = (endPos == std::string::npos) ? resp : resp.substr(0, endPos);
    TrimInPlace(mac);

    // Validate MAC format (17 chars with 5 colons)
    if (mac.length() != 17)
    {
        LE_INFO("Invalid MAC length: %zu", mac.length());
        return "";
    }

    if (std::count(mac.begin(), mac.end(), ':') != 5)
    {
        LE_INFO("Invalid colon count in MAC");
        return "";
    }

    // Validate each character is hex or colon
    for (size_t i = 0; i < mac.length(); ++i)
    {
        if (i % 3 == 2) // Colon positions: 2, 5, 8, 11, 14
        {
            if (mac[i] != ':')
            {
                LE_INFO("Missing colon at position %zu", i);
                return "";
            }
        }
        else
        {
            if (!std::isxdigit(static_cast<unsigned char>(mac[i])))
            {
                LE_INFO("Non-hex character at position %zu: '%c'", i, mac[i]);
                return "";
            }
        }
    }

    return mac;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves a list of currently connected station MAC addresses on an AP interface using
 * hostapd's STA-FIRST/STA-NEXT iteration.
 *
 * @param
 *  - iface: AP interface name (e.g., "wlan0").
 *
 * @return
 *  - Vector of unique, lower-case MAC addresses.
 *
 * Notes:
 *  - Deduplicates results via std::set before returning.
 *  - Applies a safety cap on iterations to avoid infinite loops on unexpected hostapd behavior.
 */
//--------------------------------------------------------------------------------------------------
static std::vector<std::string> GetConnectedMacs
(
    const char *iface
)
{
    std::vector<std::string> macs;

    if (!iface || !*iface)
    {
        LE_WARN("Invalid interface name");
        return macs;
    }

    std::string resp;

    if (HostapdCommand(iface, "STA-FIRST", &resp))
    {
        TrimInPlace(resp);
        if (!resp.empty() && resp.find("FAIL") == std::string::npos)
        {
            std::string mac = ExtractMacFromResponse(resp);
            if (!mac.empty())
            {
                macs.push_back(mac);

                // Iterate through remaining stations
                int maxIterations = 256; // Safety limit
                int iterations = 0;
                while (iterations++ < maxIterations)
                {
                    std::string cmd = "STA-NEXT " + mac;
                    if (!HostapdCommand(iface, cmd, &resp))
                        break;

                    TrimInPlace(resp);
                    if (resp.empty() || resp.find("FAIL") != std::string::npos)
                        break;

                    std::string nextMac = ExtractMacFromResponse(resp);
                    if (nextMac.empty() || nextMac == mac) // Prevent infinite loop
                        break;

                    macs.push_back(nextMac);
                    mac = nextMac;
                }

                if (iterations >= maxIterations)
                {
                    LE_WARN("Hit max iterations in STA-NEXT loop");
                }
            }
        }
    }

    // Deduplicate
    std::set<std::string> uniq(macs.begin(), macs.end());
    std::vector<std::string> result(uniq.begin(), uniq.end());

    LE_INFO("Found %zu unique MACs for interface %s", result.size(), iface);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Parses the output of hostapd GET_CONFIG into friendly fields.
 *
 * @param
 *  - resp: Raw GET_CONFIG response string (key=value lines).
 *  - ssidOut: Output SSID string (truncated if beyond TAF_WLAN_MAX_SSID_LENGTH).
 *  - visibleOut: Output boolean indicating if SSID is broadcast (true => visible).
 *  - secOut: Optional security config output (may remain partially UNKNOWN if hostapd
 *            does not include all keys or runtime changes are not reflected).
 *
 * Behavior:
 *  - Reads key=value lines, validates keys (alnum or underscore), trims values.
 *  - Extracts WPA mode, key management, and pairwise cipher to approximate security config.
 *
 * Notes:
 *  - Some hostapd builds omit certain keys; secOut may be supplemented by cached values elsewhere.
 */
//--------------------------------------------------------------------------------------------------
static void ParseHostapdConfig
(
    const std::string &resp,
    std::string &ssidOut,
    bool &visibleOut,
    taf_wlanAp_WlanAPSecurityConfig_t *secOut = nullptr
)
{
    ssidOut.clear();
    visibleOut = true;

    if (resp.empty())
    {
        LE_WARN("Empty config response");
        return;
    }

    std::map<std::string, std::string> config;
    size_t pos = 0;

    while (pos < resp.size())
    {
        size_t end = resp.find('\n', pos);
        std::string line = resp.substr(pos, (end == std::string::npos ? resp.size() : end) - pos);
        TrimInPlace(line);

        auto eqPos = line.find('=');
        if (eqPos != std::string::npos && eqPos > 0 && eqPos < line.length() - 1)
        {
            std::string key = line.substr(0, eqPos);
            std::string val = line.substr(eqPos + 1);
            TrimInPlace(key);
            TrimInPlace(val);

            // Validate key contains only alphanumeric and underscore
            bool validKey = !key.empty() && std::all_of(key.begin(), key.end(),
                            [](char c)
                            {
                                return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
                            });
            if (validKey)
            {
                config[key] = val;
            }
            else
            {
                LE_INFO("Skipping invalid config key: '%s'", key.c_str());
            }
        }

        if (end == std::string::npos)
            break;
        pos = end + 1;
    }

    // Extract basic config with bounds checking
    if (config.count("ssid"))
    {
        ssidOut = config["ssid"];
        if (ssidOut.length() > TAF_WLAN_MAX_SSID_LENGTH)
        {
            LE_WARN("SSID too long (%zu), truncating", ssidOut.length());
            ssidOut.resize(TAF_WLAN_MAX_SSID_LENGTH);
        }
    }

    visibleOut = (config["ignore_broadcast_ssid"] != "1");

    // Extract security config if requested
    if (secOut)
    {
        // Initialize to safe defaults
        secOut->SecMode = TAF_WLAN_SEC_MODE_UNKNOWN;
        secOut->SecAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_UNKNOWN;
        secOut->SecEncryptMethod = TAF_WLAN_SEC_ENCRYPT_METHOD_UNKNOWN;

        int wpa = 0;
        if (config.count("wpa"))
        {
            try
            {
                wpa = std::stoi(config["wpa"]);
                if (wpa < 0 || wpa > 3)
                {
                    LE_WARN("Invalid wpa value: %d, defaulting to 0", wpa);
                    wpa = 0;
                }
            }
            catch (const std::exception &e)
            {
                LE_WARN("Failed to parse wpa value '%s': %s", config["wpa"].c_str(), e.what());
                wpa = 0;
            }
        }

        std::string keyMgmt = config["wpa_key_mgmt"];
        if (keyMgmt.empty())
            keyMgmt = config["key_mgmt"];

        // Determine security mode
        if (wpa == 0)
            secOut->SecMode = TAF_WLAN_SEC_MODE_OPEN;
        else if (wpa == 1)
            secOut->SecMode = TAF_WLAN_SEC_MODE_WPA;
        else if (keyMgmt.find("SAE") != std::string::npos)
            secOut->SecMode = TAF_WLAN_SEC_MODE_WPA3;
        else if (wpa == 2)
            secOut->SecMode = TAF_WLAN_SEC_MODE_WPA2;

        // Determine auth method
        if (keyMgmt.find("SAE") != std::string::npos)
            secOut->SecAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_SAE;
        else if (keyMgmt.find("WPA-PSK") != std::string::npos)
            secOut->SecAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_PSK;
        else if (keyMgmt.find("WPA-EAP") != std::string::npos)
            secOut->SecAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_EAP_TLS;

        // Determine encryption
        std::string enc = config["rsn_pairwise"];
        if (enc.empty())
            enc = config["wpa_pairwise"];
        if (enc.empty())
            enc = config["pairwise"];

        if (enc.find("GCMP") != std::string::npos)
            secOut->SecEncryptMethod = TAF_WLAN_SEC_ENCRYPT_METHOD_GCMP;
        else if (enc.find("CCMP") != std::string::npos)
            secOut->SecEncryptMethod = TAF_WLAN_SEC_ENCRYPT_METHOD_AES;
        else if (enc.find("TKIP") != std::string::npos)
            secOut->SecEncryptMethod = TAF_WLAN_SEC_ENCRYPT_METHOD_TKIP;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Finds and returns the AP context for a given AP ID.
 *
 * @param
 *  - apID: Identifier of the AP (1..TAF_WLAN_MAX_NUM_AP).
 *
 * @return
 *  - Pointer to taf_wlan_AP_Ctx_t on success; nullptr if not found.
 *
 */
//--------------------------------------------------------------------------------------------------
taf_wlan_AP_Ctx_t* taf_WlanAPSvcImpl::GetWlanAPCtx
(
    taf_wlan_APid_t apID                ///< [IN] AP identifier
)
{
    le_dls_Link_t* linkPtr = nullptr;
    le_mutex_Lock(APCtxMutex);
    linkPtr = le_dls_Peek(&APCtxList);
    while (linkPtr)
    {
        taf_wlan_AP_Ctx_t *ctxPtr = CONTAINER_OF(linkPtr, taf_wlan_AP_Ctx_t, link);
        linkPtr = le_dls_PeekNext(&APCtxList, linkPtr);
        if (ctxPtr->id == apID)
        {
            le_mutex_Unlock(APCtxMutex);
            return ctxPtr;
        }
    }
    le_mutex_Unlock(APCtxMutex);
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Associates a host interface name with an AP context and returns its reference.
 *
 * @param
 *  - apID: AP identifier (1..TAF_WLAN_MAX_NUM_AP).
 *  - apIntfName: Interface name to associate (e.g., "wlan0").
 *
 * @return
 *  - AP reference handle on success; nullptr on failure.
 *
 * Behavior:
 *  - Validates interface name length and content.
 *  - Copies the interface name into the context under APCtxMutex.
 *  - Creates (but does not start) the WPA control thread for event reception.
 */
//--------------------------------------------------------------------------------------------------
taf_wlanAp_WlanAPRef_t taf_WlanAPSvcImpl::GetWlanAPReference
(
    taf_wlan_APid_t apID,             ///< [IN] AP identifier
    const char *LE_NONNULL apIntfName ///< [IN] AP assocaited host interface name.
)
{
    if (!apIntfName || strlen(apIntfName) == 0)
    {
        LE_ERROR("apIntfName is invalid");
        return nullptr;
    }

    if (strlen(apIntfName) >= TAF_NET_INTERFACE_NAME_MAX_LEN)
    {
        LE_ERROR("apIntfName too long: %zu", strlen(apIntfName));
        return nullptr;
    }

    taf_wlan_AP_Ctx_t *ctxPtr = GetWlanAPCtx(apID);
    if (ctxPtr == nullptr)
    {
        LE_ERROR("Unable to get context for AP ID: %d", apID);
        return nullptr;
    }

    le_mutex_Lock(APCtxMutex);
    le_result_t ret = le_utf8_Copy(ctxPtr->interfaceName, apIntfName,
                                   TAF_NET_INTERFACE_NAME_MAX_LEN, nullptr);
    le_mutex_Unlock(APCtxMutex);

    if (ret != LE_OK)
    {
        LE_ERROR("Interface name copy error: %d", ret);
        return nullptr;
    }

    // Create the wpa ctrl thread for this AP, but don't start it
    if (nullptr == ctxPtr->APWpaCtrlThreadRef)
    {
        LE_INFO("Create WPA CTRL thread");
        ctxPtr->APWpaCtrlThreadRef = le_thread_Create("wpa_ctrl", APWpaCtrlThreadHdlr, ctxPtr);
        if (nullptr == ctxPtr->APWpaCtrlThreadRef)
        {
            LE_ERROR("Failed to create WPA Ctrl Thread");
            return nullptr;
        }
    }
    else
    {
        LE_INFO("WPA CTRL thread already created");
    }

    LE_INFO("AP ID: %d, Interface name: %s", ctxPtr->id, ctxPtr->interfaceName);
    return ctxPtr->wlanAPRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends ENABLE to hostapd for the given AP and verifies success.
 *
 * @param
 *  - apRef: Reference to the AP context.
 *
 * @return
 *  - LE_OK on success; LE_FAULT if hostapd rejects or cannot be reached.
 *
 * Notes:
 *  - Treats only an explicit "OK" response as success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanAPSvcImpl::Start
(
    taf_wlanAp_WlanAPRef_t apRef
)
{
    taf_wlan_AP_Ctx_t *ctxPtr =
        static_cast<taf_wlan_AP_Ctx_t *>(le_ref_Lookup(APRefMap, (void *)apRef));
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    std::string resp;
    if (HostapdCommand(ctxPtr->interfaceName, "ENABLE", &resp))
    {
        TrimInPlace(resp);
        if (resp == "OK")
        {
            LE_INFO("AP started: %s", ctxPtr->interfaceName);
            return LE_OK;
        }

        // ENABLE returned non-OK; verify actual status and treat already-enabled as success
        LE_WARN("ENABLE returned: %s on %s; verifying status", resp.c_str(),
            ctxPtr->interfaceName);

        taf_wlanAp_WlanAPStatus_t status = {};
        le_result_t st = GetStatus(apRef, &status);
        if ((st == LE_OK) && status.bEnabled)
        {
            LE_INFO("AP %s already enabled; no effect", ctxPtr->interfaceName);
            return LE_OK;
        }

        // Not enabled -> propagate failure
        LE_WARN("AP %s not enabled after ENABLE", ctxPtr->interfaceName);
        return LE_FAULT;
    }
    else
    {
        // Transport-layer failure; verify status and treat already-enabled as success
        LE_WARN("ENABLE failed on %s (hostapd may not be running); verifying status",
                ctxPtr->interfaceName);

        taf_wlanAp_WlanAPStatus_t status = {};
        le_result_t st = GetStatus(apRef, &status);
        if ((st == LE_OK) && status.bEnabled)
        {
            LE_INFO("AP %s already enabled; no effect", ctxPtr->interfaceName);
            return LE_OK;
        }

        LE_ERROR("Failed to enable AP %s", ctxPtr->interfaceName);
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends DISABLE to hostapd for the given AP and verifies success.
 *
 * @param
 *  - apRef: Reference to the AP context.
 *
 * @return
 *  - LE_OK on success; LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanAPSvcImpl::Stop
(
    taf_wlanAp_WlanAPRef_t apRef
)
{
    taf_wlan_AP_Ctx_t *ctxPtr =
        static_cast<taf_wlan_AP_Ctx_t *>(le_ref_Lookup(APRefMap, (void *)apRef));
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    std::string resp;
    if (HostapdCommand(ctxPtr->interfaceName, "DISABLE", &resp))
    {
        TrimInPlace(resp);
        if (resp == "OK")
        {
            LE_INFO("AP stopped: %s", ctxPtr->interfaceName);
            return LE_OK;
        }

        // DISABLE returned non-OK; verify actual status and treat already-disabled as success
        LE_WARN("DISABLE returned: %s on %s; verifying status", resp.c_str(),
            ctxPtr->interfaceName);

        taf_wlanAp_WlanAPStatus_t status = {};
        le_result_t st = GetStatus(apRef, &status);
        if ((st == LE_OK) && !status.bEnabled)
        {
            LE_INFO("AP %s already disabled; no effect", ctxPtr->interfaceName);
            return LE_OK;
        }

        // Still enabled -> failure
        LE_WARN("AP %s still enabled after DISABLE", ctxPtr->interfaceName);
        return LE_FAULT;
    }
    else
    {
        // Transport-layer failure; verify status and treat already-disabled as success
        LE_WARN("DISABLE failed on %s; verifying status", ctxPtr->interfaceName);

        taf_wlanAp_WlanAPStatus_t status = {};
        le_result_t st = GetStatus(apRef, &status);
        if ((st == LE_OK) && !status.bEnabled)
        {
            LE_INFO("AP %s already disabled; no effect", ctxPtr->interfaceName);
            return LE_OK;
        }

        LE_ERROR("Failed to disable AP %s", ctxPtr->interfaceName);
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Restarts the AP by sending RELOAD or DISABLE/ENABLE sequence to hostapd.
 *
 * @param
 *  - apRef: Reference to the AP context.
 *
 * @return
 *  - LE_OK on success; LE_FAULT on failure.
 *
 * Behavior:
 *  - Prefers RELOAD for faster config application.
 *  - Falls back to DISABLE then ENABLE if RELOAD isn't successful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanAPSvcImpl::Restart
(
    taf_wlanAp_WlanAPRef_t apRef
)
{
    taf_wlan_AP_Ctx_t *ctxPtr =
        static_cast<taf_wlan_AP_Ctx_t *>(le_ref_Lookup(APRefMap, (void *)apRef));
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    std::string resp;

    // Try RELOAD first
    if (HostapdCommand(ctxPtr->interfaceName, "RELOAD", &resp))
    {
        TrimInPlace(resp);
        if (resp == "OK")
        {
            LE_INFO("AP reloaded: %s", ctxPtr->interfaceName);
            return LE_OK;
        }
        LE_WARN("RELOAD returned: %s on %s", resp.c_str(), ctxPtr->interfaceName);
    }

    // Fallback to DISABLE/ENABLE
    if (HostapdCommand(ctxPtr->interfaceName, "DISABLE", &resp))
    {
        TrimInPlace(resp);
        if (resp != "OK")
        {
            LE_WARN("DISABLE returned: %s on %s", resp.c_str(), ctxPtr->interfaceName);
            return LE_FAULT;
        }
    }
    else
    {
        LE_WARN("DISABLE failed on %s", ctxPtr->interfaceName);
        return LE_FAULT;
    }

    if (HostapdCommand(ctxPtr->interfaceName, "ENABLE", &resp))
    {
        TrimInPlace(resp);
        if (resp == "OK")
        {
            LE_INFO("AP restarted via disable/enable: %s", ctxPtr->interfaceName);
            return LE_OK;
        }
        LE_WARN("ENABLE returned: %s on %s", resp.c_str(), ctxPtr->interfaceName);
    }
    else
    {
        LE_WARN("ENABLE failed on %s", ctxPtr->interfaceName);
    }

    LE_ERROR("Failed to restart AP %s", ctxPtr->interfaceName);
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Applies basic AP configuration (SSID and broadcast visibility) via hostapd runtime SET.
 *
 * @param
 *  - apRef: AP reference handle.
 *  - wlanAPConfigPtr: Pointer to configuration (SSID and visibility fields used).
 *
 * @return
 *  - LE_OK on success; LE_BAD_PARAMETER on validation errors; LE_FAULT on hostapd errors.
 *
 * Behavior:
 *  - Validates SSID length and printable characters.
 *  - Sends SET ssid and SET ignore_broadcast_ssid (0=visible, 1=hidden).
 *  - Triggers RELOAD to apply changes.
 *
 * Notes:
 *  - Some hostapd builds may require a full restart for certain changes; this function
 *    returns LE_FAULT if runtime SET isn't supported (non-"OK" responses).
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanAPSvcImpl::SetConfig
(
    taf_wlanAp_WlanAPRef_t apRef,
    const taf_wlanAp_WlanAPConfig_t *wlanAPConfigPtr
)
{
    TAF_ERROR_IF_RET_VAL(!wlanAPConfigPtr, LE_BAD_PARAMETER, "nullptr config pointer");

    taf_wlan_AP_Ctx_t *ctxPtr =
        static_cast<taf_wlan_AP_Ctx_t *>(le_ref_Lookup(APRefMap, (void *)apRef));
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    // Validate SSID
    size_t ssidLen = strnlen(wlanAPConfigPtr->SSID, TAF_WLAN_MAX_SSID_LENGTH + 1);
    if (ssidLen == 0 || ssidLen > TAF_WLAN_MAX_SSID_LENGTH)
    {
        LE_ERROR("Invalid SSID length: %zu", ssidLen);
        return LE_BAD_PARAMETER;
    }

    // Validate SSID contains only printable characters
    for (size_t i = 0; i < ssidLen; ++i)
    {
        if (!std::isprint(static_cast<unsigned char>(wlanAPConfigPtr->SSID[i])))
        {
            LE_ERROR("SSID contains non-printable character at position %zu", i);
            return LE_BAD_PARAMETER;
        }
    }

    std::string resp;

    // SET ssid
    {
        std::string ssidCmd = "SET ssid " + std::string(wlanAPConfigPtr->SSID);
        if (!HostapdCommand(ctxPtr->interfaceName, ssidCmd, &resp))
        {
            LE_WARN("SET ssid command failed");
            return LE_FAULT;
        }
        TrimInPlace(resp);
        if (resp != "OK")
        {
            LE_WARN("SET ssid returned: %s", resp.c_str());
            return LE_FAULT;
        }
    }

    // SET ignore_broadcast_ssid
    {
        std::string visCmd = "SET ignore_broadcast_ssid " +
                             std::string(wlanAPConfigPtr->bSSIDVisible ? "0" : "1");
        if (!HostapdCommand(ctxPtr->interfaceName, visCmd, &resp))
        {
            LE_WARN("SET ignore_broadcast_ssid command failed");
            return LE_FAULT;
        }
        TrimInPlace(resp);
        if (resp != "OK")
        {
            LE_WARN("SET ignore_broadcast_ssid returned: %s", resp.c_str());
            return LE_FAULT;
        }
    }

    // Apply changes
    HostapdCommand(ctxPtr->interfaceName, "RELOAD", nullptr);
    LE_INFO("Config updated for AP %s: SSID=%s, visible=%d",
            ctxPtr->interfaceName, wlanAPConfigPtr->SSID, wlanAPConfigPtr->bSSIDVisible);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the current AP configuration via hostapd GET_CONFIG and fills SSID and visibility.
 *
 * @param
 *  - apRef: AP reference handle.
 *  - wlanAPConfigPtr: Output structure to receive SSID and broadcast visibility.
 *
 * @return
 *  - LE_OK on success; LE_FAULT if hostapd GET_CONFIG fails.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanAPSvcImpl::GetConfig
(
    taf_wlanAp_WlanAPRef_t apRef,
    taf_wlanAp_WlanAPConfig_t *wlanAPConfigPtr
)
{
    TAF_ERROR_IF_RET_VAL(!wlanAPConfigPtr, LE_BAD_PARAMETER, "nullptr config pointer");

    taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    std::string resp;
    TAF_ERROR_IF_RET_VAL(!HostapdCommand(ctxPtr->interfaceName, "GET_CONFIG", &resp),
        LE_FAULT, "GET_CONFIG failed");

    std::string ssid;
    bool visible;
    ParseHostapdConfig(resp, ssid, visible, nullptr);

    le_utf8_Copy(wlanAPConfigPtr->SSID, ssid.c_str(), TAF_WLAN_MAX_SSID_LENGTH + 1, nullptr);
    wlanAPConfigPtr->bSSIDVisible = visible;

    LE_INFO("Retrieved config for AP %s: SSID=%s, visible=%d",
            ctxPtr->interfaceName, wlanAPConfigPtr->SSID, wlanAPConfigPtr->bSSIDVisible);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Applies AP security configuration via hostapd runtime SET commands.
 *
 * @param
 *  - apRef: AP reference handle.
 *  - wlanAPSecCfgPtr: Desired security configuration (mode, auth, encryption, passphrase).
 *
 * @return
 *  - LE_OK on success; LE_BAD_PARAMETER for validation errors; LE_FAULT for hostapd errors.
 *
 * Behavior:
 *  - Validates passphrase length and characters when provided.
 *  - Sets WPA mode (wpa=0/1/2), key management (PSK/SAE/EAP), pairwise ciphers (CCMP/GCMP/TKIP).
 *  - For WPA3 SAE, requires PMF (ieee80211w=2) and optionally attempts sae_require_mfp=1.
 *  - Triggers RELOAD to apply changes.
 *  - Caches last applied security configuration (without passphrase).
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanAPSvcImpl::SetSecurityConfig
(
    taf_wlanAp_WlanAPRef_t apRef,
    const taf_wlanAp_WlanAPSecurityConfig_t *wlanAPSecCfgPtr
)
{
    TAF_ERROR_IF_RET_VAL(!wlanAPSecCfgPtr, LE_BAD_PARAMETER, "nullptr config pointer");

    taf_wlan_AP_Ctx_t *ctxPtr =
        static_cast<taf_wlan_AP_Ctx_t *>(le_ref_Lookup(APRefMap, (void *)apRef));
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    // Validate passphrase if provided
    if (wlanAPSecCfgPtr->PassPhrase[0] != '\0')
    {
        size_t ppLen = strnlen(wlanAPSecCfgPtr->PassPhrase, TAF_WLAN_MAX_PASSPHRASE_LENGTH + 1);
        if (ppLen < 8 || ppLen > TAF_WLAN_MAX_PASSPHRASE_LENGTH)
        {
            LE_ERROR("Invalid passphrase length: %zu (must be 8-%d)",
                     ppLen, TAF_WLAN_MAX_PASSPHRASE_LENGTH);
            return LE_BAD_PARAMETER;
        }

        // Validate passphrase contains only printable ASCII
        for (size_t i = 0; i < ppLen; ++i)
        {
            if (!std::isprint(static_cast<unsigned char>(wlanAPSecCfgPtr->PassPhrase[i])))
            {
                LE_ERROR("Passphrase contains non-printable character at position %zu", i);
                return LE_BAD_PARAMETER;
            }
        }
    }

    std::string resp;

    auto sendSet = [&](const std::string &cmd) -> bool
    {
        if (!HostapdCommand(ctxPtr->interfaceName, cmd, &resp))
        {
            LE_WARN("Command failed: %s", cmd.c_str());
            return false;
        }
        TrimInPlace(resp);
        if (resp != "OK")
        {
            LE_WARN("Command returned error: %s -> %s", cmd.c_str(), resp.c_str());
            return false;
        }
        return true;
    };

    // WPA mode
    switch (wlanAPSecCfgPtr->SecMode)
    {
    case TAF_WLAN_SEC_MODE_OPEN:
        if (!sendSet("SET wpa 0"))
        {
            LE_ERROR("Failed to set open security mode for AP %s", ctxPtr->interfaceName);
            return LE_FAULT;
        }
        break;
    case TAF_WLAN_SEC_MODE_WPA:
        if (!sendSet("SET wpa 1"))
        {
            LE_ERROR("Failed to set WPA security mode for AP %s", ctxPtr->interfaceName);
            return LE_FAULT;
        }
        break;
    case TAF_WLAN_SEC_MODE_WPA2:
    case TAF_WLAN_SEC_MODE_WPA3:
        if (!sendSet("SET wpa 2"))
        {
            LE_ERROR("Failed to set WPA2/WPA3 security mode for AP %s", ctxPtr->interfaceName);
            return LE_FAULT;
        }
        break;
    default:
        LE_ERROR("Invalid security mode: %d", wlanAPSecCfgPtr->SecMode);
        return LE_BAD_PARAMETER;
    }

    // Auth method + passphrase
    if (wlanAPSecCfgPtr->SecAuthMethod == TAF_WLAN_SEC_AUTH_METHOD_SAE)
    {
        if (!sendSet("SET wpa_key_mgmt SAE"))
        {
            LE_ERROR("Failed to set SAE auth method for AP %s", ctxPtr->interfaceName);
            return LE_FAULT;
        }

        if (wlanAPSecCfgPtr->PassPhrase[0])
        {
            std::string saePwd = "SET sae_password " + std::string(wlanAPSecCfgPtr->PassPhrase);
            if (!sendSet(saePwd))
            {
                LE_ERROR("Failed to set SAE password for AP %s", ctxPtr->interfaceName);
                return LE_FAULT;
            }
        }

        // WPA3/SAE typically requires PMF/MFP
        if (!sendSet("SET ieee80211w 2"))
        {
            LE_ERROR("Failed to set PMF for AP %s", ctxPtr->interfaceName);
            return LE_FAULT;
        }

        // Try enabling SAE MFP requirement (optional: do not fail hard if unsupported)
        std::string optResp;
        if (HostapdCommand(ctxPtr->interfaceName, "SET sae_require_mfp 1", &optResp))
        {
            TrimInPlace(optResp);
            if (optResp != "OK")
            {
                LE_WARN("Optional 'sae_require_mfp' returned: %s", optResp.c_str());
            }
        }
        else
        {
            LE_WARN("Optional 'sae_require_mfp' command failed");
        }
    }
    else if (wlanAPSecCfgPtr->SecAuthMethod == TAF_WLAN_SEC_AUTH_METHOD_PSK)
    {
        if (!sendSet("SET wpa_key_mgmt WPA-PSK"))
        {
            LE_ERROR("Failed to set PSK auth method for AP %s", ctxPtr->interfaceName);
            return LE_FAULT;
        }

        if (wlanAPSecCfgPtr->PassPhrase[0])
        {
            std::string wpaPwd = "SET wpa_passphrase " + std::string(wlanAPSecCfgPtr->PassPhrase);
            if (!sendSet(wpaPwd))
            {
                LE_ERROR("Failed to set WPA passphrase for AP %s", ctxPtr->interfaceName);
                return LE_FAULT;
            }
        }
    }
    else if (wlanAPSecCfgPtr->SecAuthMethod == TAF_WLAN_SEC_AUTH_METHOD_EAP_TLS)
    {
        if (!sendSet("SET wpa_key_mgmt WPA-EAP"))
        {
            LE_ERROR("Failed to set EAP-TLS auth method for AP %s", ctxPtr->interfaceName);
            return LE_FAULT;
        }
        // Certificates and EAP specifics are expected to be configured out-of-band
    }
    else if (wlanAPSecCfgPtr->SecMode != TAF_WLAN_SEC_MODE_OPEN)
    {
        LE_ERROR("Invalid auth method: %d", wlanAPSecCfgPtr->SecAuthMethod);
        return LE_BAD_PARAMETER;
    }

    // Encryption
    if (wlanAPSecCfgPtr->SecEncryptMethod == TAF_WLAN_SEC_ENCRYPT_METHOD_AES)
    {
        if (!sendSet("SET rsn_pairwise CCMP"))
        {
            LE_ERROR("Failed to set AES encryption for AP %s", ctxPtr->interfaceName);
            return LE_FAULT;
        }
    }
    else if (wlanAPSecCfgPtr->SecEncryptMethod == TAF_WLAN_SEC_ENCRYPT_METHOD_GCMP)
    {
        if (!sendSet("SET rsn_pairwise GCMP"))
        {
            LE_ERROR("Failed to set GCMP encryption for AP %s", ctxPtr->interfaceName);
            return LE_FAULT;
        }
    }
    else if (wlanAPSecCfgPtr->SecEncryptMethod == TAF_WLAN_SEC_ENCRYPT_METHOD_TKIP)
    {
        if (!sendSet("SET wpa_pairwise TKIP"))
        {
            LE_ERROR("Failed to set TKIP encryption for AP %s", ctxPtr->interfaceName);
            return LE_FAULT;
        }
    }
    else if (wlanAPSecCfgPtr->SecMode != TAF_WLAN_SEC_MODE_OPEN)
    {
        LE_ERROR("Invalid encryption method: %d", wlanAPSecCfgPtr->SecEncryptMethod);
        return LE_BAD_PARAMETER;
    }

    // Apply changes
    HostapdCommand(ctxPtr->interfaceName, "RELOAD", nullptr);

    // Cache config (without passphrase)
    le_mutex_Lock(APCtxMutex);
    ctxPtr->lastSecCfg = *wlanAPSecCfgPtr;
    ctxPtr->lastSecCfg.PassPhrase[0] = '\0';
    ctxPtr->hasLastSecCfg = true;
    le_mutex_Unlock(APCtxMutex);

    LE_INFO("Security config updated for AP %s: mode=%d, auth=%d, encrypt=%d",
            ctxPtr->interfaceName, wlanAPSecCfgPtr->SecMode,
            wlanAPSecCfgPtr->SecAuthMethod, wlanAPSecCfgPtr->SecEncryptMethod);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the current AP security configuration via hostapd GET_CONFIG.
 *
 * @param
 *  - apRef: AP reference handle.
 *  - wlanAPSecCfgPtr: Output structure to receive security settings.
 *
 * @return
 *  - LE_OK on success; LE_BAD_PARAMETER if output pointer is null; LE_FAULT if hostapd fails.
 *
 * Behavior:
 *  - Initializes output to UNKNOWN defaults.
 *  - Parses hostapd GET_CONFIG and fills mode, auth method, encryption.
 *  - If parsing leaves fields UNKNOWN, consults cached settings to augment results.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanAPSvcImpl::GetSecurityConfig
(
    taf_wlanAp_WlanAPRef_t apRef,
    taf_wlanAp_WlanAPSecurityConfig_t *wlanAPSecCfgPtr
)
{
    if (!wlanAPSecCfgPtr)
    {
        LE_WARN("GetSecurityConfig: wlanAPSecCfgPtr is nullptr");
        return LE_BAD_PARAMETER;
    }

    taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    // Initialize output to safe defaults
    memset(wlanAPSecCfgPtr, 0, sizeof(*wlanAPSecCfgPtr));
    wlanAPSecCfgPtr->SecMode = TAF_WLAN_SEC_MODE_UNKNOWN;
    wlanAPSecCfgPtr->SecAuthMethod = TAF_WLAN_SEC_AUTH_METHOD_UNKNOWN;
    wlanAPSecCfgPtr->SecEncryptMethod = TAF_WLAN_SEC_ENCRYPT_METHOD_UNKNOWN;

    std::string resp;
    if (!HostapdCommand(ctxPtr->interfaceName, "GET_CONFIG", &resp))
    {
        LE_WARN("GET_CONFIG failed on %s", ctxPtr->interfaceName);

        // Try to use cached config
        le_mutex_Lock(APCtxMutex);
        if (ctxPtr->hasLastSecCfg)
        {
            *wlanAPSecCfgPtr = ctxPtr->lastSecCfg;
            le_mutex_Unlock(APCtxMutex);
            LE_INFO("Using cached security config for AP %s", ctxPtr->interfaceName);
            return LE_OK;
        }
        le_mutex_Unlock(APCtxMutex);

        LE_ERROR("Failed to get security config for AP %s", ctxPtr->interfaceName);
        return LE_FAULT;
    }

    std::string dummySsid;
    bool dummyVisible = true;
    ParseHostapdConfig(resp, dummySsid, dummyVisible, wlanAPSecCfgPtr);

    // Fill in any unknowns from cached config
    le_mutex_Lock(APCtxMutex);
    if (ctxPtr->hasLastSecCfg)
    {
        if (wlanAPSecCfgPtr->SecAuthMethod == TAF_WLAN_SEC_AUTH_METHOD_UNKNOWN)
        {
            wlanAPSecCfgPtr->SecAuthMethod = ctxPtr->lastSecCfg.SecAuthMethod;
        }
        if (wlanAPSecCfgPtr->SecEncryptMethod == TAF_WLAN_SEC_ENCRYPT_METHOD_UNKNOWN)
        {
            wlanAPSecCfgPtr->SecEncryptMethod = ctxPtr->lastSecCfg.SecEncryptMethod;
        }
        if (wlanAPSecCfgPtr->SecMode == TAF_WLAN_SEC_MODE_UNKNOWN)
        {
            wlanAPSecCfgPtr->SecMode = ctxPtr->lastSecCfg.SecMode;
        }
    }
    le_mutex_Unlock(APCtxMutex);

    LE_INFO("Retrieved security config for AP %s: mode=%d, auth=%d, encrypt=%d",
            ctxPtr->interfaceName, wlanAPSecCfgPtr->SecMode,
            wlanAPSecCfgPtr->SecAuthMethod, wlanAPSecCfgPtr->SecEncryptMethod);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Queries hostapd for AP status and populates interface identifiers and addresses.
 *
 * @param
 *  - apRef: AP reference handle.
 *  - wlanAPStatusPtr: Output structure to receive status fields.
 *
 * @return
 *  - LE_OK on success; LE_BAD_PARAMETER for null output; LE_FAULT for lookup failure.
 *
 * Behavior:
 *  - Uses hostapd STATUS to determine whether AP is enabled.
 *  - Copies interface name, retrieves IPv4/IPv6 addresses, and reads MAC from sysfs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanAPSvcImpl::GetStatus
(
    taf_wlanAp_WlanAPRef_t apRef,
    taf_wlanAp_WlanAPStatus_t* wlanAPStatusPtr
)
{
    TAF_ERROR_IF_RET_VAL(!wlanAPStatusPtr, LE_BAD_PARAMETER, "nullptr status pointer");

    taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    memset(wlanAPStatusPtr, 0, sizeof(*wlanAPStatusPtr));

    std::string resp;
    if (HostapdCommand(ctxPtr->interfaceName, "STATUS", &resp))
    {
        wlanAPStatusPtr->bEnabled = (resp.find("state=ENABLED") != std::string::npos);
    }

    le_utf8_Copy(wlanAPStatusPtr->IntfName, ctxPtr->interfaceName,
        TAF_NET_INTERFACE_NAME_MAX_LEN + 1, nullptr);

    std::string ipv4, ipv6;
    GetInterfaceAddrs(ctxPtr->interfaceName, ipv4, ipv6);
    le_utf8_Copy(wlanAPStatusPtr->IPv4Address, ipv4.c_str(),
        TAF_NET_IPV4_ADDR_MAX_LEN + 1, nullptr);
    le_utf8_Copy(wlanAPStatusPtr->IPv6Address, ipv6.c_str(),
        TAF_NET_IPV6_ADDR_MAX_LEN + 1, nullptr);

    std::string mac = GetInterfaceMac(ctxPtr->interfaceName);
    le_utf8_Copy(wlanAPStatusPtr->MACAddress, mac.c_str(), TAF_NET_MAC_ADDR_MAX_LEN + 1, nullptr);

    LE_INFO("AP %s status: enabled=%d, IPv4=%s, IPv6=%s, MAC=%s",
            ctxPtr->interfaceName, wlanAPStatusPtr->bEnabled,
            wlanAPStatusPtr->IPv4Address, wlanAPStatusPtr->IPv6Address,
            wlanAPStatusPtr->MACAddress);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Enumerates devices connected to the AP and returns their MAC and IPv4 addresses.
 *
 * @param
 *  - apRef: AP reference handle.
 *  - numDevicesPtr: Output count of connected devices (may exceed capacity; see DevInfoSizePtr).
 *  - DevInfoPtr: Output array to receive device info (MAC, IPv4).
 *  - DevInfoSizePtr: In/out capacity of DevInfoPtr; set to number of entries written.
 *
 * @return
 *  - LE_OK on success; LE_BAD_PARAMETER for invalid pointers; LE_FAULT on context lookup failure.
 *
 * Behavior:
 *  - Fetches connected MACs via GetConnectedMacs().
 *  - Caps returned entries by DevInfoSizePtr and TAF_WLANAP_MAX_CONNECTED_DEVICES.
 *  - Attempts to map MAC to IPv4 via ARP; hostname is left empty.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanAPSvcImpl::GetConnectedDevices
(
    taf_wlanAp_WlanAPRef_t apRef,
    uint16_t *numDevicesPtr,
    ///< [OUT] Nmber of devices connected to the AP.
    taf_wlanAp_WlanAPConnectedDeviceInfo_t *DevInfoPtr,
    ///< [OUT] Connected device information.
    size_t *DevInfoSizePtr
    ///< [INOUT]
)
{
    TAF_ERROR_IF_RET_VAL(!numDevicesPtr || !DevInfoPtr || !DevInfoSizePtr,
                         LE_BAD_PARAMETER, "nullptr pointer");

    taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    if (*DevInfoSizePtr == 0)
    {
        LE_WARN("DevInfoSize is 0");
        *numDevicesPtr = 0;
        return LE_OK;
    }

    std::vector<std::string> macs = GetConnectedMacs(ctxPtr->interfaceName);

    *numDevicesPtr = static_cast<uint16_t>(std::min(macs.size(), (size_t)UINT16_MAX));
    size_t copyCount = std::min(*DevInfoSizePtr, macs.size());
    copyCount = std::min(copyCount, (size_t)TAF_WLANAP_MAX_CONNECTED_DEVICES);
    *DevInfoSizePtr = copyCount;

    LE_INFO("Found %u connected devices on %s (returning %zu)",
            *numDevicesPtr, ctxPtr->interfaceName, copyCount);

    for (size_t i = 0; i < copyCount; ++i)
    {
        // Initialize structure
        memset(&DevInfoPtr[i], 0, sizeof(DevInfoPtr[i]));

        // Copy MAC address
        le_result_t ret = le_utf8_Copy(DevInfoPtr[i].MACAddress, macs[i].c_str(),
                                       TAF_NET_MAC_ADDR_MAX_LEN + 1, nullptr);
        if (ret != LE_OK)
        {
            LE_WARN("Failed to copy MAC address at index %zu: %d", i, ret);
            continue;
        }

        // Try to get IP address
        std::string ipv4 = GetIpForMac(ctxPtr->interfaceName, macs[i]);
        if (!ipv4.empty())
        {
            ret = le_utf8_Copy(DevInfoPtr[i].IPv4Address, ipv4.c_str(),
                               TAF_NET_IPV4_ADDR_MAX_LEN + 1, nullptr);
            if (ret != LE_OK)
            {
                LE_WARN("Failed to copy IP address at index %zu: %d", i, ret);
            }
        }

        // Name is left empty as we don't have hostname information
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * First-layer event handler adaptor that invokes client-provided callbacks.
 *
 * @param
 *  - reportPtr: Pointer to DeviceCnxEvent_t allocated when reporting the event.
 *  - secondLayerHandlerFunc: Client callback function to be invoked.
 *
 * Behavior:
 *  - Extracts event data and calls the client handler with the original context.
 *  - Releases the ref-counted event memory after dispatch.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPSvcImpl::DeviceCnxFirstLayerEventHandler
(
    void *reportPtr,
    void *secondLayerHandlerFunc
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "reportPtr is nullptr!");
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == nullptr, "secondLayerHandlerFunc is nullptr!");

    DeviceCnxEvent_t *DeviceCnxEventPtr = (DeviceCnxEvent_t *)reportPtr;

    taf_wlanAp_DeviceConnectionEventHandlerFunc_t deviceHandlerFunc =
        (taf_wlanAp_DeviceConnectionEventHandlerFunc_t)secondLayerHandlerFunc;

    deviceHandlerFunc(DeviceCnxEventPtr->apRef,
                      DeviceCnxEventPtr->event,
                      DeviceCnxEventPtr->MACAddress,
                      le_event_GetContextPtr());
    // Release the pointer that was allocated when the event was sent.
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Reports device connection/disconnection events to all registered clients for an AP.
 *
 * @param
 *  - ApctxPtr: AP context pointer.
 *  - event: Connection event type (connected/disconnected).
 *  - MACAddressStr: MAC address string of the station (validated: length 17).
 *
 * Behavior:
 *  - Allocates a ref-counted event payload, copies metadata, and posts the event via Legato.
 *  - Validates inputs to guard against malformed addresses.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPSvcImpl::ReportDevCnxEvents
(
    taf_wlan_AP_Ctx_t *ApctxPtr,              // AP Context
    taf_wlanAp_DeviceConnectionEvent_t event, // Event type.
    const char *MACAddressStr                 // MAC address of the device
)
{
    if (!ApctxPtr)
    {
        LE_ERROR("nullptr AP context pointer");
        return;
    }

    if (!MACAddressStr)
    {
        LE_ERROR("nullptr MAC address string");
        return;
    }

    // Validate MAC address format
    size_t macLen = strnlen(MACAddressStr, TAF_NET_MAC_ADDR_MAX_LEN + 1);
    if (macLen != 17)
    {
        LE_ERROR("Invalid MAC address length: %zu", macLen);
        return;
    }

    DeviceCnxEvent_t *devCnxEvtPtr = (DeviceCnxEvent_t *)le_mem_ForceAlloc(DeviceCnxEventPoolRef);
    if (!devCnxEvtPtr)
    {
        LE_ERROR("Failed to allocate DeviceCnxEvent");
        return;
    }

    devCnxEvtPtr->apRef = ApctxPtr->wlanAPRef;
    devCnxEvtPtr->event = event;

    le_result_t result = le_utf8_Copy(devCnxEvtPtr->MACAddress, MACAddressStr,
                                      TAF_NET_MAC_ADDR_MAX_LEN + 1, nullptr);
    if (LE_OK != result)
    {
        LE_WARN("Device MAC address copy error: %d", result);
        le_mem_Release(devCnxEvtPtr);
        return;
    }

    le_event_ReportWithRefCounting(ApctxPtr->DeviceConnectionEvent, (void *)devCnxEvtPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Destructor callback for the WPA control thread.
 *
 * @param
 *  - context: The wpa_ctrl* handle associated with the thread.
 *
 * Behavior:
 *  - Detaches from the hostapd control interface and then closes the socket.
 *  - Ensures hostapd does not retain stale monitor attachments.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPSvcImpl::APWpaCtrlThreadDestructor
(
    void *context
)
{
    // Ensure we detach before closing to avoid stale monitor attachment inside hostapd
    struct wpa_ctrl *ctrl = static_cast<struct wpa_ctrl *>(context);
    if (ctrl)
    {
        wpa_ctrl_detach(ctrl);
        wpa_ctrl_close(ctrl);
    }
    LE_INFO("WPA CTRL detached and closed");
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread handler that attaches to hostapd and listens for AP station events.
 *
 * @param
 *  - context: AP context pointer for which to receive events.
 *
 * Behavior:
 *  - Resolves and opens hostapd control socket (tries multiple base paths).
 *  - Attaches to control interface to receive asynchronous events.
 *  - Uses select() with a timeout to process incoming messages and allow cancellation.
 *  - Recognizes AP-STA-CONNECTED and AP-STA-DISCONNECTED and extracts MAC addresses.
 *  - Reports device connection events via ReportDevCnxEvents().
 *
 * Thread Cleanup:
 *  - Registers a destructor to detach and close the control interface upon termination.
 */
//--------------------------------------------------------------------------------------------------
void *taf_WlanAPSvcImpl::APWpaCtrlThreadHdlr
(
    void *context
)
{
    struct wpa_ctrl *ctrl = nullptr;
    taf_wlan_AP_Ctx_t *ApctxPtr = static_cast<taf_wlan_AP_Ctx_t *>(context);

    if (!ApctxPtr)
    {
        LE_ERROR("nullptr context pointer");
        return nullptr;
    }

    // Get singleton instance to access non-static members
    auto &wlanAp = taf_WlanAPSvcImpl::GetInstance();

    // Thread-safe access to interface name
    char ifaceName[TAF_NET_INTERFACE_NAME_MAX_LEN + 1] = {0};
    le_mutex_Lock(wlanAp.APCtxMutex);
    le_result_t copyResult = le_utf8_Copy(ifaceName, ApctxPtr->interfaceName,
                                          sizeof(ifaceName), nullptr);
    le_mutex_Unlock(wlanAp.APCtxMutex);

    if (copyResult != LE_OK || ifaceName[0] == '\0')
    {
        LE_ERROR("Failed to copy interface name or empty interface name");
        return nullptr;
    }

    // Try to open hostapd control interface using multiple base paths, normalize trailing slash
    const char *bases[] = {HOSTAPD_LOCATION_PATH, "/var/run/hostapd/", "/run/hostapd/"};
    std::string chosenPath;

    for (const char *base : bases)
    {
        if (!base || !*base)
            continue;

        std::string path(base);
        if (path.back() != '/')
        {
            path.push_back('/');
        }
        path += ifaceName;

        // Validate path length (UNIX abstract socket path limit)
        if (path.length() >= 108) // UNIX_PATH_MAX for sockaddr_un.sun_path
        {
            LE_WARN("Hostapd ctrl path too long (%zu): %s", path.length(), path.c_str());
            continue;
        }

        ctrl = wpa_ctrl_open(path.c_str());
        if (ctrl)
        {
            chosenPath = path;
            LE_INFO("Opened hostapd ctrl: %s", chosenPath.c_str());
            break;
        }
    }

    if (!ctrl)
    {
        LE_ERROR("Failed to open hostapd control interface for %s", ifaceName);
        return nullptr;
    }

    int ret = wpa_ctrl_attach(ctrl);
    if (ret != 0)
    {
        LE_ERROR("Failed to attach to control interface '%s': %d", chosenPath.c_str(), ret);
        wpa_ctrl_close(ctrl);
        return nullptr;
    }

    int ctrl_fd = wpa_ctrl_get_fd(ctrl);
    if (ctrl_fd < 0)
    {
        LE_ERROR("Failed to get control interface file descriptor for '%s'", chosenPath.c_str());
        wpa_ctrl_detach(ctrl);
        wpa_ctrl_close(ctrl);
        return nullptr;
    }

    // Add destructor and pass WPA CTRL handle to it.
    le_thread_AddDestructor(APWpaCtrlThreadDestructor, static_cast<void *>(ctrl));

    LE_INFO("WPA control thread started for %s (fd=%d)", ifaceName, ctrl_fd);

    // Event-driven loop using select()
    char rsp[AP_WPA_CTRL_RSP_BUF_LEN] = {0};

    while (true)
    {
        fd_set rfds;
        struct timeval tv;

        FD_ZERO(&rfds);
        FD_SET(ctrl_fd, &rfds);

        // Timeout for select - allows periodic checking of thread cancellation
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(ctrl_fd + 1, &rfds, nullptr, nullptr, &tv);

        if (ret < 0)
        {
            if (errno == EINTR)
            {
                // Interrupted by signal, check if thread is being cancelled
                LE_DEBUG("select() interrupted by signal");
                continue;
            }
            LE_ERROR("select() failed: %s", strerror(errno));
            break;
        }
        else if (ret == 0)
        {
            // Timeout - no events, continue waiting
            continue;
        }

        // Data is available on the file descriptor
        if (FD_ISSET(ctrl_fd, &rfds))
        {
            // Process all pending messages
            while (wpa_ctrl_pending(ctrl) > 0)
            {
                size_t len = AP_WPA_CTRL_RSP_BUF_LEN - 1; // Reserve space for null terminator
                memset(rsp, 0, sizeof(rsp));

                ret = wpa_ctrl_recv(ctrl, rsp, &len);
                if (ret < 0)
                {
                    LE_WARN("wpa_ctrl_recv failed: %d", ret);
                    break;
                }

                // Validate length
                if (len >= AP_WPA_CTRL_RSP_BUF_LEN)
                {
                    LE_ERROR("Response buffer overflow: len=%zu", len);
                    break;
                }

                rsp[len] = '\0';

                // Validate response contains printable characters
                bool valid = true;
                for (size_t i = 0; i < len; ++i)
                {
                    unsigned char c = static_cast<unsigned char>(rsp[i]);
                    if (c != '\0' && !std::isprint(c) && !std::isspace(c))
                    {
                        LE_WARN("Invalid character in response at position %zu: 0x%02x", i, c);
                        valid = false;
                        break;
                    }
                }

                if (!valid)
                {
                    LE_WARN("Skipping invalid response");
                    continue;
                }

                std::string msg(rsp);
                LE_INFO("Received event: %s", msg.c_str());

                // Detect station connect/disconnect event by substring (robust to priority prefix)
                bool isConnected = (msg.find("AP-STA-CONNECTED") != std::string::npos);
                bool isDisconnected = (msg.find("AP-STA-DISCONNECTED") != std::string::npos);
                if (!isConnected && !isDisconnected)
                {
                    continue;
                }

                // Split the received buffer and find the MAC address token.
                // Skip optional priority token like "<3>".
                std::vector<std::string> tokens = Split(msg, ' ');
                std::string mac;
                for (const auto &tok : tokens)
                {
                    if (!tok.empty() && tok.front() == '<' && tok.back() == '>')
                    {
                        continue; // priority prefix
                    }

                    std::string candidate = ExtractMacFromResponse(tok);
                    if (!candidate.empty())
                    {
                        mac = candidate;
                        break;
                    }
                }

                if (mac.empty())
                {
                    LE_WARN("Missing MAC address in event: %s", msg.c_str());
                    continue;
                }

                if (isConnected)
                {
                    LE_INFO("AP-STA-CONNECTED. MAC: %s", mac.c_str());
                    wlanAp.ReportDevCnxEvents(ApctxPtr, TAF_WLANAP_DEVICE_CONNECTED, mac.c_str());
                }
                else // isDisconnected
                {
                    LE_INFO("AP-STA-DISCONNECTED. MAC: %s", mac.c_str());
                    wlanAp.ReportDevCnxEvents(ApctxPtr, TAF_WLANAP_DEVICE_DISCONNECTED, mac.c_str());
                }
            }
        }
    }

    LE_INFO("WPA control thread exiting for %s", ifaceName);
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Registers a client handler for AP device connection events and starts the WPA ctrl thread
 * if needed.
 *
 * @param
 *  - apRef: AP reference handle.
 *  - handlerPtr: Client callback function to receive connection events.
 *  - contextPtr: Client-provided context passed back during callbacks.
 *
 * @return
 *  - Handler reference on success; nullptr on failure.
 *
 * Behavior:
 *  - Validates AP is enabled before allowing handler registration.
 *  - Adds layered handler to Legato event system, maintains per-session client set.
 *  - Starts the WPA control thread when the first client registers.
 */
//--------------------------------------------------------------------------------------------------
taf_wlanAp_DeviceConnectionEventHandlerRef_t taf_WlanAPSvcImpl::AddDeviceConnectionEventHandler
(
    taf_wlanAp_WlanAPRef_t apRef,
    taf_wlanAp_DeviceConnectionEventHandlerFunc_t handlerPtr,
    void *contextPtr
)
{
    if (!handlerPtr)
    {
        LE_ERROR("nullptr handler pointer");
        return nullptr;
    }

    taf_wlan_AP_Ctx_t *ApctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ApctxPtr == nullptr, nullptr, "Unable to find context");

    // Check if the AP is on
    taf_wlanAp_WlanAPStatus_t wlanAPStatus;
    le_result_t result = GetStatus(apRef, &wlanAPStatus);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, nullptr, "Unable to get AP status");

    TAF_ERROR_IF_RET_VAL(!(wlanAPStatus.bEnabled), nullptr, "AP is not active");

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
        "DeviceConnectionEventHandler",
        ApctxPtr->DeviceConnectionEvent,
        DeviceCnxFirstLayerEventHandler,
        (void *)handlerPtr);

    if (!handlerRef)
    {
        LE_ERROR("Failed to add event handler");
        return nullptr;
    }

    le_event_SetContextPtr(handlerRef, contextPtr);

    le_msg_SessionRef_t sessionRef = taf_wlanAp_GetClientSessionRef();

    le_mutex_Lock(APCtxMutex);
    if (ApctxPtr->devCnxEvtClients.find(sessionRef) == ApctxPtr->devCnxEvtClients.end())
    {
        LE_INFO("Insert client: %p", sessionRef);
        ApctxPtr->devCnxEvtClients.insert(sessionRef);
    }
    else
    {
        LE_INFO("Client already registered: %p", sessionRef);
    }

    bool shouldStartThread = (ApctxPtr->devCnxEvtClients.size() == 1) &&
                             !(ApctxPtr->isAPWpaCtrlThreadRunning);
    le_mutex_Unlock(APCtxMutex);

    if (shouldStartThread)
    {
        LE_INFO("First client. Start WPA ctrl thread");
        le_thread_Start(ApctxPtr->APWpaCtrlThreadRef);

        le_mutex_Lock(APCtxMutex);
        ApctxPtr->isAPWpaCtrlThreadRunning = true;
        le_mutex_Unlock(APCtxMutex);
    }

    return (taf_wlanAp_DeviceConnectionEventHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Unregisters a client event handler and stops the WPA ctrl thread if no clients remain.
 *
 * @param
 *  - handlerRef: Handler reference to remove.
 *
 * Behavior:
 *  - Removes the handler from the Legato event system.
 *  - For all APs, removes the session from their client sets.
 *  - Cancels the WPA control thread for an AP when the last client is removed.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPSvcImpl::RemoveDeviceConnectionEventHandler
(
    taf_wlanAp_DeviceConnectionEventHandlerRef_t handlerRef
)
{
    if (!handlerRef)
    {
        LE_WARN("nullptr handler reference");
        return;
    }

    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);

    le_msg_SessionRef_t sessionRef = taf_wlanAp_GetClientSessionRef();

    for (int iCount = 1; iCount <= TAF_WLAN_MAX_NUM_AP; iCount++)
    {
        taf_wlan_AP_Ctx_t *ApctxPtr = GetWlanAPCtx(taf_wlan_APid_t(iCount));
        if (!ApctxPtr)
        {
            LE_WARN("nullptr context for AP ID: %d", iCount);
            continue;
        }

        LE_DEBUG("Checking AP Ctx Id: %d", iCount);

        le_mutex_Lock(APCtxMutex);

        auto it = ApctxPtr->devCnxEvtClients.find(sessionRef);
        if (it != ApctxPtr->devCnxEvtClients.end())
        {
            LE_INFO("Remove client: %p from AP %d", sessionRef, iCount);
            ApctxPtr->devCnxEvtClients.erase(it);
        }
        else
        {
            LE_DEBUG("Client %p not found in AP %d", sessionRef, iCount);
        }

        bool shouldStopThread = (ApctxPtr->devCnxEvtClients.size() == 0) &&
                                (ApctxPtr->isAPWpaCtrlThreadRunning);

        le_mutex_Unlock(APCtxMutex);

        if (shouldStopThread)
        {
            LE_INFO("Last client removed. Stop WPA ctrl thread for AP %d", iCount);
            if (ApctxPtr->APWpaCtrlThreadRef)
            {
                le_thread_Cancel(ApctxPtr->APWpaCtrlThreadRef);
                le_mutex_Lock(APCtxMutex);
                ApctxPtr->isAPWpaCtrlThreadRunning = false;
                ApctxPtr->APWpaCtrlThreadRef = nullptr;
                le_mutex_Unlock(APCtxMutex);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Service client connection handler (called when a client connects to the WLAN service).
 *
 * @param
 *  - sessionRef: Client session reference.
 *  - context: Unused pointer for Legato handler signature compliance.
 *
 * Behavior:
 *  - Retrieves client PID and app name for logging.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPSvcImpl::OnWlanSvcClientConnect
(
    le_msg_SessionRef_t sessionRef,
    void *context
)
{
    LE_UNUSED(context);
    pid_t pid;
    char appName[LIMIT_MAX_PATH_BYTES] = {0};

    if (LE_OK != le_msg_GetClientProcessId(sessionRef, &pid))
    {
        LE_WARN("Error, Failed to get client pid.");
        return;
    }

    if (le_appInfo_GetName(pid, appName, sizeof(appName)) == LE_OK)
    {
        LE_INFO("Client Details: ref: %p, pid: %ld, name: %s", sessionRef,
                static_cast<long int>(pid),
                appName);
    }
    else
    {
        LE_WARN("Error, Failed to get client app name");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Service client disconnection handler.
 *
 * @param
 *  - sessionRef: Client session reference being disconnected.
 *  - context: Unused pointer for Legato handler signature compliance.
 *
 * Behavior:
 *  - Removes the session from each AP's client set.
 *  - If an AP has no remaining clients, cancels and clears its WPA control thread.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPSvcImpl::OnWlanSvcClientDisconnect
(
    le_msg_SessionRef_t sessionRef,
    void *context
)
{
    LE_UNUSED(context);
    LE_INFO("Client disconnected: %p", sessionRef);

    for (int iCount = 1; iCount <= TAF_WLAN_MAX_NUM_AP; iCount++)
    {
        auto &wlanAp = taf_WlanAPSvcImpl::GetInstance();
        taf_wlan_AP_Ctx_t *ApctxPtr = wlanAp.GetWlanAPCtx(taf_wlan_APid_t(iCount));

        if (!ApctxPtr)
        {
            LE_WARN("nullptr context for AP ID: %d", iCount);
            continue;
        }

        le_mutex_Lock(APCtxMutex);

        auto it = ApctxPtr->devCnxEvtClients.find(sessionRef);
        if (it != ApctxPtr->devCnxEvtClients.end())
        {
            LE_INFO("Remove DevCnxEvt client: %p from AP %d", sessionRef, iCount);
            ApctxPtr->devCnxEvtClients.erase(it);
        }
        else
        {
            LE_DEBUG("Skip DevCnxEvt client: %p (not in AP %d)", sessionRef, iCount);
        }

        bool shouldStopThread = (ApctxPtr->devCnxEvtClients.size() == 0) &&
                                (ApctxPtr->isAPWpaCtrlThreadRunning);

        le_mutex_Unlock(APCtxMutex);

        if (shouldStopThread)
        {
            LE_INFO("Last client disconnected. Stop WPA ctrl thread for AP %d", iCount);
            if (ApctxPtr->APWpaCtrlThreadRef)
            {
                le_thread_Cancel(ApctxPtr->APWpaCtrlThreadRef);
                le_mutex_Lock(APCtxMutex);
                ApctxPtr->isAPWpaCtrlThreadRunning = false;
                ApctxPtr->APWpaCtrlThreadRef = nullptr;
                le_mutex_Unlock(APCtxMutex);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns the singleton instance of the AP service implementation.
 *
 * @return
 *  - Reference to a static taf_WlanAPSvcImpl instance.
 *
 * Notes:
 *  - Ensures a single global instance is used throughout the service.
 */
//--------------------------------------------------------------------------------------------------
taf_WlanAPSvcImpl &taf_WlanAPSvcImpl::GetInstance()
{
    static taf_WlanAPSvcImpl instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * taf_WlanAPSvcImpl Init function
*/
//--------------------------------------------------------------------------------------------------
void taf_WlanAPSvcImpl::Init()
{
    // Initialize AP contexts and events.
    APCtxPoolRef = le_mem_InitStaticPool(tafWlanAPCtxPool, TAF_WLAN_MAX_NUM_AP,
                                         sizeof(taf_wlan_AP_Ctx_t));

    APCtxMutex = le_mutex_CreateNonRecursive("APCtxMutex");

    APRefMap = le_ref_CreateMap("APRefMap", TAF_WLAN_MAX_NUM_AP);

    for (int id = 1; id <= TAF_WLAN_MAX_NUM_AP; ++id)
    {
        taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_mem_ForceAlloc(APCtxPoolRef);
        if (ctxPtr == nullptr)
        {
            LE_FATAL("Unable to allocate ctxPtr for AP ID: %d", id);
        }

        ctxPtr->id = (taf_wlan_APid_t)id;
        ctxPtr->interfaceName[0] = 0;

        taf_wlanAp_WlanAPRef_t apRef =
            (taf_wlanAp_WlanAPRef_t)le_ref_CreateRef(APRefMap, (void *)ctxPtr);
        if (apRef == nullptr)
        {
            LE_FATAL("Unable to allocate reference for AP ID: %d", id);
        }

        ctxPtr->wlanAPRef = apRef;
        le_dls_Queue(&APCtxList, &ctxPtr->link);
        LE_DEBUG("Context created for STA ID: %d", ctxPtr->id);

        // Create client connection event for applications.
        std::string eventName =
            "ApCtx-" + std::to_string(ctxPtr->id) + std::string(": ClientCnxEvt");
        ctxPtr->DeviceConnectionEvent = le_event_CreateIdWithRefCounting(eventName.c_str());

        // Threads and clients
        ctxPtr->APWpaCtrlThreadRef = nullptr;
        ctxPtr->devCnxEvtClients.clear();
        ctxPtr->isAPWpaCtrlThreadRunning = false;

        // Per-AP security cache
        memset(&ctxPtr->lastSecCfg, 0, sizeof(ctxPtr->lastSecCfg));
        ctxPtr->hasLastSecCfg = false;
    }

    // Memory for client connection
    DeviceCnxEventPoolRef = le_mem_InitStaticPool(DeviceCnxEventPool, TAF_WLAN_MAX_SESSION_REF,
                                                  sizeof(DeviceCnxEvent_t));

    // Client connect/disconnect handlers
    le_msg_AddServiceOpenHandler(taf_wlanAp_GetServiceRef(),
        taf_WlanAPSvcImpl::OnWlanSvcClientConnect, nullptr);
    le_msg_AddServiceCloseHandler(taf_wlanAp_GetServiceRef(),
        taf_WlanAPSvcImpl::OnWlanSvcClientDisconnect, nullptr);

    LE_INFO(" *** Wlan AP Initialized *** ");
}
