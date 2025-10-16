/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file tafWlanStaWpaSupplicantIntfImpl.cpp
 *
 * @brief Implementation of the WPA supplicant interface APIs.
 *
 */

#include "tafWlanSTA.hpp" // Include the header where taf_WlanStaWpaSupplicantIntf is declared
#include <wpa_ctrl.h>     // Required for struct wpa_ctrl and wpa_ctrl_close

namespace tafsvc
{
//--------------------------------------------------------------------------------------------------
/**
 * Constructor for taf_WlanStaWpaSupplicantIntf.
 */
//--------------------------------------------------------------------------------------------------
taf_WlanStaWpaSupplicantIntf::taf_WlanStaWpaSupplicantIntf(const taf_wlan_STAid_t staId)
    : staId_(staId), wpaCtrlPtr_(nullptr), wpaSupplicantPathStr_("")
{
    LE_DEBUG("taf_WlanStaWpaSupplicantIntf created for STA ID: %d", staId_);
}

//--------------------------------------------------------------------------------------------------
/**
 * Destructor for taf_WlanStaWpaSupplicantIntf.
 */
//--------------------------------------------------------------------------------------------------
taf_WlanStaWpaSupplicantIntf::~taf_WlanStaWpaSupplicantIntf()
{
    LE_DEBUG("taf_WlanStaWpaSupplicantIntf destructor for STA ID: %d", staId_);
    if (wpaCtrlPtr_)
    {
        wpa_ctrl_close(wpaCtrlPtr_);
        wpaCtrlPtr_ = nullptr;
        LE_DEBUG("WPA control socket closed for STA ID: %d", staId_);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the WPA supplicant path.
 *
 * @return
 *      The WPA supplicant path string.
 */
//--------------------------------------------------------------------------------------------------
const std::string &taf_WlanStaWpaSupplicantIntf::GetWpaSupplicantPath() const
{
    return wpaSupplicantPathStr_;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the WPA supplicant path.
 *
 * @param supplicantPath [IN] The path to the WPA supplicant.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
bool taf_WlanStaWpaSupplicantIntf::SetWpaSupplicantPath(const std::string &supplicantPath)
{
    if (wpaSupplicantPathStr_.empty())
    {
        // Path is empty. Update
        wpaSupplicantPathStr_ = supplicantPath;
        LE_DEBUG("WPA supplicant path updated to: %s. STA ID: %d",
                                                    wpaSupplicantPathStr_.c_str(), staId_);
    }
    else
    {
        // Path is already set. Don't update.
        LE_DEBUG("WPA supplicant path : %s. STA ID: %d", wpaSupplicantPathStr_.c_str(), staId_);
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the WPA supplicant control socket.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - LE_FAULT -- Failed to open socket.
 */
//--------------------------------------------------------------------------------------------------
bool taf_WlanStaWpaSupplicantIntf::OpenControlSocket(void)
{
    // If the socket is already open, close it first.
    if (wpaCtrlPtr_)
    {
        LE_DEBUG("WPA control socket already open for STA ID: %d.", staId_);
        return true;
    }

    wpaCtrlPtr_ = wpa_ctrl_open(wpaSupplicantPathStr_.c_str());
    if (!wpaCtrlPtr_)
    {
        LE_ERROR("Failed to open control interface: %d(%s)", errno, strerror(errno));
        LE_ERROR("Failed to open WPA control socket for STA ID: %d, Path: %s", staId_,
                                                                    wpaSupplicantPathStr_.c_str());
        return false;
    }

    LE_DEBUG("WPA control socket opened successfully for STA ID: %d, Path: %s", staId_,
                                                                    wpaSupplicantPathStr_.c_str());

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close the WPA supplicant control socket.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanStaWpaSupplicantIntf::CloseControlSocket(void)
{
    if (wpaCtrlPtr_)
    {
        wpa_ctrl_close(wpaCtrlPtr_);
        wpaCtrlPtr_ = nullptr;
        LE_DEBUG("WPA control socket closed for STA ID: %d", staId_);
    }
    else
    {
        LE_INFO("WPA control socket already closed or not opened for STA ID: %d", staId_);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the socket is active using a PING<-->PONG command and response.
 */
//--------------------------------------------------------------------------------------------------
bool taf_WlanStaWpaSupplicantIntf::IsControlSocketActive(void)
{
    TAF_ERROR_IF_RET_VAL(nullptr == wpaCtrlPtr_, false,
                                        "WPA control socket is not open for STA ID: %d", staId_);

    LE_DEBUG("Checking control socket for STA ID: %d", staId_);

    // Check for pending messages
    {
        LE_DEBUG("Check and flush pending messages");
        char pend_buf[WPA_CTRL_RSP_BUF_LEN] = {0};
        size_t pend_rsp_len = WPA_CTRL_RSP_BUF_LEN;
        int bPending = 0;
        bPending = wpa_ctrl_pending(wpaCtrlPtr_);
        while (bPending > 0)
        {
            pend_rsp_len = WPA_CTRL_RSP_BUF_LEN - 1;
            if (wpa_ctrl_recv(wpaCtrlPtr_, pend_buf, &pend_rsp_len) == 0)
            {
                pend_buf[pend_rsp_len] = '\0';
                LE_DEBUG("Pending rsp len: %zu", pend_rsp_len);
                LE_DEBUG("Pending rsp    : %s", pend_buf);
            }
            else
            {
                LE_WARN ("Failed to receive pending message");
            }
            bPending = wpa_ctrl_pending(wpaCtrlPtr_);
        }
    }

    LE_DEBUG("PING");
    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};
    size_t rsp_len = WPA_CTRL_RSP_BUF_LEN-1;
    constexpr size_t pingCmdLen = 4; // strlen("PING")
    constexpr size_t pongRspLen = 4; // strlen("PONG")
    int ret = wpa_ctrl_request(wpaCtrlPtr_, "PING", pingCmdLen, rsp_buf, &rsp_len, nullptr);
    if (ret < 0)
    {
        if (ret == -2)
        {
            LE_WARN("Timeout while waiting for response");
        }
        LE_ERROR("Failed to send command: %d", ret);
        controlSocketFailureCount_++;
        // Don't return from here. Retry, track error count and then declare error appropriately.
    }
    else
    {
        if (strncmp(rsp_buf, "PONG", pongRspLen) != 0)
        {
            LE_WARN("Err len: %zu", rsp_len);
            LE_WARN("Err str: %s", rsp_buf);
            controlSocketFailureCount_++;
        }
        else
        {
            LE_DEBUG("PONG");
            // Reset on successful communication.
            controlSocketFailureCount_ = 0;
        }
    }

    if (0 == controlSocketFailureCount_)
    {
        return true;
    }

    // Check the number of failures.
    if (controlSocketFailureCount_ >= MAX_CONTROL_SOCKET_FAILURE_COUNT)
    {
        LE_ERROR("Control socket comm err for STA ID %d has exceeded limit: %d", staId_,
                                                                        controlSocketFailureCount_);
        LE_INFO("Close control socket.");
        CloseControlSocket();
    }

    return false;
}

/**
 * Gets the RSSI of the AP the STA is connected to.
 *
 * LE_TIMEOUT is a non-fatal error.
 * LE_FAULT is a non-fatal error.
 * LE_COMM_ERROR is a fatal error.
 */
le_result_t taf_WlanStaWpaSupplicantIntf::GetConnectedApRSSI(int16_t &signalStrength)
{
    TAF_ERROR_IF_RET_VAL(nullptr == wpaCtrlPtr_, LE_COMM_ERROR,
                                        "WPA communication failure for STA ID: %d", staId_);

    char rsp_buf[WPA_CTRL_RSP_BUF_LEN] = {0};
    size_t rsp_len = WPA_CTRL_RSP_BUF_LEN;
    int16_t rssi = 0;
    bool bRssiFound = false;
    constexpr size_t pollCmdLen = 11; // strlen("SIGNAL_POLL")

    int ret = wpa_ctrl_request(wpaCtrlPtr_, "SIGNAL_POLL", pollCmdLen, rsp_buf, &rsp_len, nullptr);
    if (ret < 0)
    {
        if (ret == -2)
        {
            LE_ERROR("Timeout while waiting for response");
            return LE_TIMEOUT;
        }
        LE_ERROR("Failed to send command: %d", ret);
        // Don't return from here. Check communication using IsControlSocketActive() and error wil
        // be returned from there.
    }
    else
    {
        // Parse the response to extract RSSI
        std::istringstream iss(rsp_buf);
        std::string line;
        while (std::getline(iss, line))
        {
            try
            {
                rssi = std::stoi(line.substr(5));
                bRssiFound = true;
                break;
            }
            catch (const std::invalid_argument &e)
            {
                LE_ERROR("Invalid RSSI value format: %s", e.what());
            }
            catch (const std::out_of_range &e)
            {
                LE_ERROR("RSSI value out of range: %s", e.what());
            }
        }
    }

    // Handle RSSI not found case.
    if (!bRssiFound)
    {
        LE_WARN ("RSSI not found in response");
        // Check if the connection is active for debugging.
        if (ret < 0)
        {
            if (!IsControlSocketActive())
            {
                LE_WARN("Control socket is not active for STA ID: %d", staId_);
            }
        }
        return LE_FAULT;
    }

    // RSSI is found
    signalStrength = rssi;
    LE_DEBUG ("SIG strength: %d", signalStrength);
    return LE_OK;
}

} // namespace tafsvc
