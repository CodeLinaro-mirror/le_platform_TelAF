/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanSvcImpl.cpp
 *
 * @brief      Implemenation of TelAF WLAN Device Management Service APIs.
 *
 */

#include "tafWlan.hpp"

using namespace tafsvc;

LE_MEM_DEFINE_STATIC_POOL(DeviceStatusPool, TAF_WLAN_MAX_SESSION_REF,
                          sizeof(taf_wlan_DeviceState_t));

// Boolean variables to track if we are waiting for a promise to be fulfilled
// These are declared as static global variables because the event handlers are static.
static std::atomic<bool> bWaitingForIntSetPromise = {false};
static std::atomic<bool> bWaitingForIntGetPromise = {false};

//--------------------------------------------------------------------------------------------------
/**
 * Return Device state change event ID
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t taf_WlanSvcImpl::GetStateChangeEventID(void){
    return wlanDevStateChangeEvID;
}

//--------------------------------------------------------------------------------------------------
/**
 * Turn WLAN device ON. This action bring up the respective WLAN host interface.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::SetON(void)
{
    if (nullptr == wlanDevMgr)
    {
        LE_WARN ("WLAN Device not initialized");
        return LE_NOT_PERMITTED;
    }

    // Check if device is already enabled
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;
    bool enabled = false;
    std::vector<telux::wlan::InterfaceStatus> ifStatus;

    errCode = wlanDevMgr->getStatus(enabled, ifStatus);
    if(telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN ("WLAN Get Status failed: %d", (int) errCode);
        return LE_FAULT;
    }
    if (enabled){
        LE_INFO ("WLAN Device already enabled");
        return LE_OK;
    }

    // Enable WLAN Device
    wlanListener->resetPromise();
    errCode = wlanDevMgr->enable(true);
    if((telux::common::ErrorCode::SUCCESS != errCode) ||
            (false == wlanListener->getEnableStatus())) {
            LE_WARN ("WLAN Enable failed. Error Code: %d", (int) errCode);
            return LE_FAULT;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Turn WLAN device OFF. This action will remove the respective WLAN host interface.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::SetOFF(void)
{
    if (nullptr == wlanDevMgr)
    {
        LE_WARN ("WLAN Device not initialized");
        return LE_NOT_PERMITTED;
    }

    // Check if device is already disabled
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;
    bool enabled = false;
    std::vector<telux::wlan::InterfaceStatus> ifStatus;

    errCode = wlanDevMgr->getStatus(enabled, ifStatus);
    if(telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN ("WLAN Get Status failed: %d", (int) errCode);
        return LE_FAULT;
    }
    if (!enabled){
        LE_INFO ("WLAN Device already disabled");
        return LE_OK;
    }

    // Disable WLAN Device
    wlanListener->resetPromise();
    errCode = wlanDevMgr->enable(false);
    if((telux::common::ErrorCode::SUCCESS != errCode) ||
            (true == wlanListener->getEnableStatus())) {
            LE_WARN ("WLAN Disable failed. Error Code: %d", (int) errCode);
            return LE_FAULT;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get WLAN Device state. This API also provide the name of the WLAN device (if available).
 *
 * @return
 * - LE_OK            Succeeded.
 * - Appropriate error is returned on failure.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::GetState
(
    taf_wlan_DeviceState_t* statePtr
        ///< [OUT] WLAN device state.
)
{
    TAF_ERROR_IF_RET_VAL(statePtr == NULL,  LE_BAD_PARAMETER, "statePtr is NULL!");
    if (nullptr == wlanDevMgr)
    {
        LE_WARN ("WLAN Device not initialized");
        return LE_NOT_PERMITTED;
    }

    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;
    bool enableStat = false;
    std::vector<telux::wlan::InterfaceStatus> ifStatus;

    errCode = wlanDevMgr->getStatus(enableStat, ifStatus);
    if(telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN ("WLAN Get Status failed: %d", (int) errCode);
        return LE_FAULT;
    }

    // Update state pointer
    if (enableStat) {
        *statePtr = TAF_WLAN_ON;
    } else {
        *statePtr = TAF_WLAN_OFF;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the WLAN operating mode by specifying the number of Access Points and/or Stations to enable.
 * Check the actual mode enabled by using the taf_wlan_GetMode API.
 *
 * @return
 * - LE_OK            Succeeded.
 * - Appropriate error is returned on failure.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::SetMode
(
    taf_wlan_DeviceMode_t wlanMode ///< [IN] The WLAN device mode.
)
{
    if (nullptr == wlanDevMgr)
    {
        LE_WARN ("WLAN Device not initialized");
        return LE_NOT_PERMITTED;
    }

    int numAP  = 0;
    int numSTA = 0;

    // Transform taf_wlan_DeviceMode_t to number of APs and STAs.
    switch (wlanMode)
    {
    case TAF_WLAN_MODE_AP:
        numAP  = 1;
        numSTA = 0;
        break;
    case TAF_WLAN_MODE_STA:
        numAP  = 0;
        numSTA = 1;
        break;
    case TAF_WLAN_MODE_STA_AP:
        numAP  = 1;
        numSTA = 1;
        break;
    case TAF_WLAN_MODE_AP_AP:
        numAP = 2;
        numSTA = 0;
        break;
    case TAF_WLAN_MODE_AP_AP_STA:
        numAP = 2;
        numSTA = 1;
        break;
    // Unsupported modes
    case TAF_WLAN_MODE_UNKNOWN:
    default:
        LE_WARN ("Invalid Device Mode: %d", wlanMode);
        return LE_BAD_PARAMETER;
    };

    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;
    errCode = wlanDevMgr->setMode(numAP,numSTA);
    if(telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN ("WLAN Set Mode failed: %d", (int)errCode);
        return LE_FAULT;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the WLAN operating mode
 *
 * @return
 * - LE_OK            Succeeded.
 * - Appropriate error is returned on failure.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::GetMode
(
    taf_wlan_DeviceMode_t* wlanModePtr
        ///< [OUT] The WLAN device mode.
)
{
    TAF_ERROR_IF_RET_VAL(wlanModePtr == NULL,  LE_BAD_PARAMETER, "wlanModePtr is NULL!");

    if (nullptr == wlanDevMgr)
    {
        LE_WARN ("WLAN Device not initialized");
        return LE_NOT_PERMITTED;
    }

    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;
    int numOfAP=0, numOfSTA=0;
    errCode = wlanDevMgr->getConfig(numOfAP,numOfSTA);
    if(telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN ("WLAN Get Mode failed: %d", (int)errCode);
        return LE_FAULT;
    }
    LE_INFO ("WLAN Mode - AP: %d, STA: %d", numOfAP, numOfSTA);

    // Transform to taf_wlan_DeviceMode_t
    if (1==numOfAP && 0 == numOfSTA)
    {
        // AP only
        *wlanModePtr = TAF_WLAN_MODE_AP;
    }
    else if (0==numOfAP && 1 == numOfSTA)
    {
        // STA only
        *wlanModePtr = TAF_WLAN_MODE_STA;
    }
    else if (1==numOfAP && 1 == numOfSTA)
    {
        // STA + AP
        *wlanModePtr = TAF_WLAN_MODE_STA_AP;
    }
    else if (2 == numOfAP && 0 == numOfSTA)
    {
        // AP + AP
        *wlanModePtr = TAF_WLAN_MODE_AP_AP;
    }
    else if (2 == numOfAP && 1 == numOfSTA)
    {
        // AP + AP
        *wlanModePtr = TAF_WLAN_MODE_AP_AP_STA;
    }
    else
    {
        // Unsupported mode
        LE_WARN ("Unsupported mode");
        *wlanModePtr = TAF_WLAN_MODE_UNKNOWN;
        return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Fill active WLAN interface(s) information for GetIntfInfo() in case it is possible to get the
 * information from TelSDK.
 * This API is used only within the service and not exposed to application. Applications should use
 * taf_wlan_GetIntfInfo()
 *
 * The implementaiton can be improved to read the interface names directly from the wpa_supplicant
 * or hostapd conf files.
 *
 * @return
 * - LE_OK            Succeeded.
 * - Appropriate error is returned on failure.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::FillIntfInfo(
    taf_wlan_APIntfInfo_t *APIntfinfoPtr,
    ///< [OUT] The WLAN AP interfaces information.
    size_t *APIntfinfoSizePtr,
    ///< [INOUT]
    taf_wlan_STAIntfInfo_t *STAIntfinfoPtr,
    ///< [OUT] The WLAN STA interfaces information.
    size_t *STAIntfinfoSizePtr
    ///< [INOUT]
)
{
    le_result_t ret = LE_OK;
    taf_wlan_DeviceMode_t wlanMode;
    // Initialize size to 0
    *APIntfinfoSizePtr  = 0;
    *STAIntfinfoSizePtr = 0;

    // Get current WLAN mode
    ret = GetMode(&wlanMode);
    if (LE_OK != ret)
    {
        LE_WARN("Failed to get WLAN mode");
        return ret;
    }
    LE_DEBUG("Wlan Mode: %d", wlanMode);
    switch (wlanMode)
    {
    // AP only
    case TAF_WLAN_MODE_AP:
        LE_INFO("Wlan Mode: TAF_WLAN_MODE_AP");

        *APIntfinfoSizePtr = 1;
        *STAIntfinfoSizePtr = 0;
        APIntfinfoPtr[0].id = TAF_WLAN_AP_ID1;
        ret = le_utf8_Copy(APIntfinfoPtr[0].IntfName, "wlan0",
                           TAF_NET_INTERFACE_NAME_MAX_LEN + 1, NULL);
        if (LE_OK != ret)
        {
            LE_WARN("IntfName copy error: %d", ret);
        }
        break;
    // STA only
    case TAF_WLAN_MODE_STA:
        LE_INFO("Wlan Mode: TAF_WLAN_MODE_STA");
        *APIntfinfoSizePtr  = 0;
        *STAIntfinfoSizePtr = 1;
        STAIntfinfoPtr[0].id = TAF_WLAN_STA_ID1;
        ret = le_utf8_Copy(STAIntfinfoPtr[0].IntfName, "wlan0",
                           TAF_NET_INTERFACE_NAME_MAX_LEN + 1, NULL);
        if (LE_OK != ret)
        {
            LE_WARN("IntfName copy error: %d", ret);
        }
        break;
    // STA + AP
    case TAF_WLAN_MODE_STA_AP:
        LE_INFO("Wlan Mode: TAF_WLAN_MODE_STA_AP");
        *APIntfinfoSizePtr  = 1;
        *STAIntfinfoSizePtr = 1;
        STAIntfinfoPtr[0].id = TAF_WLAN_STA_ID1;
        ret = le_utf8_Copy(STAIntfinfoPtr[0].IntfName, "wlan0",
                           TAF_NET_INTERFACE_NAME_MAX_LEN + 1, NULL);
        if (LE_OK != ret)
        {
            LE_WARN("IntfName copy error: %d", ret);
        }
        APIntfinfoPtr[0].id = TAF_WLAN_AP_ID1;
        ret = le_utf8_Copy(APIntfinfoPtr[0].IntfName, "wlan1",
                           TAF_NET_INTERFACE_NAME_MAX_LEN + 1, NULL);
        if (LE_OK != ret)
        {
            LE_WARN("IntfName copy error: %d", ret);
        }
        break;
    // AP + AP
    case TAF_WLAN_MODE_AP_AP:
        LE_INFO("Wlan Mode: TAF_WLAN_MODE_AP_AP");
        *APIntfinfoSizePtr = 2;
        *STAIntfinfoSizePtr = 0;
        APIntfinfoPtr[0].id = TAF_WLAN_AP_ID1;
        ret = le_utf8_Copy(APIntfinfoPtr[0].IntfName, "wlan0",
                           TAF_NET_INTERFACE_NAME_MAX_LEN + 1, NULL);
        if (LE_OK != ret)
        {
            LE_WARN("IntfName copy error: %d", ret);
        }
        APIntfinfoPtr[1].id = TAF_WLAN_AP_ID2;
        ret = le_utf8_Copy(APIntfinfoPtr[1].IntfName, "wlan1",
                           TAF_NET_INTERFACE_NAME_MAX_LEN + 1, NULL);
        if (LE_OK != ret)
        {
            LE_WARN("IntfName copy error: %d", ret);
        }
        break;
        // AP + AP + STA
    case TAF_WLAN_MODE_AP_AP_STA:
        LE_INFO("Wlan Mode: TAF_WLAN_MODE_AP_AP_STA");
        *APIntfinfoSizePtr = 2;
        *STAIntfinfoSizePtr = 1;

        STAIntfinfoPtr[0].id = TAF_WLAN_STA_ID1;
        ret = le_utf8_Copy(STAIntfinfoPtr[0].IntfName, "wlan0",
                           TAF_NET_INTERFACE_NAME_MAX_LEN + 1, NULL);
        if (LE_OK != ret)
        {
            LE_WARN("IntfName copy error: %d", ret);
        }

        APIntfinfoPtr[0].id = TAF_WLAN_AP_ID1;
        ret = le_utf8_Copy(APIntfinfoPtr[0].IntfName, "wlan1",
                           TAF_NET_INTERFACE_NAME_MAX_LEN + 1, NULL);
        if (LE_OK != ret)
        {
            LE_WARN("IntfName copy error: %d", ret);
        }
        APIntfinfoPtr[1].id = TAF_WLAN_AP_ID2;
        ret = le_utf8_Copy(APIntfinfoPtr[1].IntfName, "wlan2",
                           TAF_NET_INTERFACE_NAME_MAX_LEN + 1, NULL);
        if (LE_OK != ret)
        {
            LE_WARN("IntfName copy error: %d", ret);
        }

        break;

    default:
        LE_WARN("Unknown mode");
        return LE_FAULT;
        break;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets active WLAN interface(s) information.
 * The information returned should be used to get the AP and STA reference(s) respectively.
 *
 * @return
 * - LE_OK            Succeeded.
 * - Appropriate error is returned on failure.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::GetIntfInfo
(
    taf_wlan_APIntfInfo_t* APIntfinfoPtr,
        ///< [OUT] The WLAN AP interfaces information.
    size_t* APIntfinfoSizePtr,
        ///< [INOUT]
    taf_wlan_STAIntfInfo_t* STAIntfinfoPtr,
        ///< [OUT] The WLAN STA interfaces information.
    size_t* STAIntfinfoSizePtr
        ///< [INOUT]
)
{
    if (nullptr == wlanDevMgr)
    {
        LE_WARN("WLAN Device not initialized");
        return LE_NOT_PERMITTED;
    }
    // Fill in the interface names in TelAF as TelSDK will not provide the interface names in all
    // scenarios.
    return FillIntfInfo(APIntfinfoPtr, APIntfinfoSizePtr, STAIntfinfoPtr, STAIntfinfoSizePtr);
}

//--------------------------------------------------------------------------------------------------
// Reset the band interference config details to default values
//--------------------------------------------------------------------------------------------------
void taf_WlanSvcImpl::ResetBandIntCfg(WlanBandIntCfg_t &config)
{
    config.state               = TAF_WLAN_BAND_INT_DISABLED;
    config.prioBand            = TAF_WLAN_PRIO_BAND_N79;
    config.wlanUnavailableTime = 30;
    config.n79UnavailableTime  = 30;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the WLAN 5GHz and N79 5G band interference state.
 * The internal cached values are updated here if the interference state is enabled.
 * It will overwrite any values that clients had set, if the interference state is enabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::GetBandIntState(taf_wlan_BandIntState_t *statePtr)
{
    TAF_ERROR_IF_RET_VAL(nullptr == statePtr, LE_BAD_PARAMETER, "statePtr is null");
    TAF_ERROR_IF_RET_VAL(nullptr == dataSettingsManager || !bDataSettingManagerReady,
                                        LE_NOT_POSSIBLE, "Data settings manager is not ready!");
    le_result_t result = LE_OK;

    // Populate the GET command
    WlanDataSettingsCmd_t cmd;
    cmd.cmdType = WLAN_DSCMD_BAND_INT_CFG_GET;
    cmd.bandIntGetCmd.contextPtr = NULL;

    // Set the get band config promise and get a future to get the response
    promGetBandIntConfig = std::promise<WlanGetBandIntCmdRsp_t>();
    std::future<WlanGetBandIntCmdRsp_t> futGetBandIntConfig = promGetBandIntConfig.get_future();

    // Send the command to the data settings thread handler
    le_event_Report(dataSettingsCmd, &cmd, sizeof(WlanDataSettingsCmd_t));

    LE_DEBUG("Waiting for response");
    std::chrono::seconds span(TAF_WLAN_CMD_TIMEOUT);

    // Set command is waiting for promise
    bWaitingForIntGetPromise.store(true);

    std::future_status waitStatus = futGetBandIntConfig.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("Response timeout");
        result = LE_TIMEOUT;
    }
    else
    {
        LE_INFO("Waiting for response.");
        WlanGetBandIntCmdRsp_t rsp = futGetBandIntConfig.get();
        if (LE_OK == rsp.result)
        {
            *statePtr = rsp.config.state;
            // Update the current cached value if command is successful
            bandIntCfgCurrent.state               = rsp.config.state;
            // Update the cached values if state is enabled. Else everything will be 0.
            if (TAF_WLAN_BAND_INT_ENABLED == bandIntCfgCurrent.state)
            {
                // Current setting
                bandIntCfgCurrent.prioBand            = rsp.config.prioBand;
                bandIntCfgCurrent.wlanUnavailableTime = rsp.config.wlanUnavailableTime;
                bandIntCfgCurrent.n79UnavailableTime  = rsp.config.n79UnavailableTime;

                // To set settings
                bandIntCfgToSet.prioBand            = rsp.config.prioBand;
                bandIntCfgToSet.wlanUnavailableTime = rsp.config.wlanUnavailableTime;
                bandIntCfgToSet.n79UnavailableTime  = rsp.config.n79UnavailableTime;
            }
            LE_INFO("Band int: State         : %d", bandIntCfgCurrent.state);
            LE_INFO("Band int: Priority      : %d", bandIntCfgCurrent.prioBand);
            LE_INFO("Band int: N79 wait time : %d", bandIntCfgCurrent.n79UnavailableTime);
            LE_INFO("Band int: WLAN wait time: %d", bandIntCfgCurrent.wlanUnavailableTime);
        }
        result = rsp.result;
    }
    // Reset command is waiting for promise
    bWaitingForIntGetPromise.store(false);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the WLAN 5GHz and N79 5G band interference state.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::SetBandIntState(taf_wlan_BandIntState_t state)
{
    TAF_ERROR_IF_RET_VAL(TAF_WLAN_BAND_INT_DISABLED != state && TAF_WLAN_BAND_INT_ENABLED != state,
                         LE_BAD_PARAMETER, "Invalid state");
    // Check if the data settings manager is ready or not
    TAF_ERROR_IF_RET_VAL(nullptr == dataSettingsManager || !bDataSettingManagerReady,
                                        LE_NOT_POSSIBLE, "Data settings manager is not ready!");

    le_result_t result = LE_OK;
    // Populate the SET command
    WlanDataSettingsCmd_t cmd;
    cmd.cmdType = WLAN_DSCMD_BAND_INT_CFG_SET;
    cmd.bandIntSetCmd.config.state               = state;
    cmd.bandIntSetCmd.config.prioBand            = bandIntCfgToSet.prioBand;
    cmd.bandIntSetCmd.config.wlanUnavailableTime = bandIntCfgToSet.wlanUnavailableTime;
    cmd.bandIntSetCmd.config.n79UnavailableTime  = bandIntCfgToSet.n79UnavailableTime;
    cmd.bandIntSetCmd.contextPtr = NULL;

    // Set the set band config promise and get a future to get the response
    promSetBandIntConfig = std::promise<le_result_t>();
    // Set waiting for promise
    bWaitingForIntSetPromise.store(true);
    std::future<le_result_t> futSetBandIntConfig = promSetBandIntConfig.get_future();

    // Send the command to the data settings thread handler
    le_event_Report(dataSettingsCmd, &cmd, sizeof(WlanDataSettingsCmd_t));

    LE_DEBUG("Waiting for response");
    std::chrono::seconds span(TAF_WLAN_CMD_TIMEOUT);
    std::future_status waitStatus = futSetBandIntConfig.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("Response timeout");
        result = LE_TIMEOUT;
    }
    else
    {
        result = futSetBandIntConfig.get();
    }
    // Reset waiting for promise
    bWaitingForIntSetPromise.store(false);

    // Update the current cached value if command is successful
    if (LE_OK == result)
    {
        // Update the current band config if set is successful.
        LE_INFO("Update the current int config");
        bandIntCfgCurrent.state               = state;
        bandIntCfgCurrent.prioBand            = bandIntCfgToSet.prioBand;
        bandIntCfgCurrent.wlanUnavailableTime = bandIntCfgToSet.wlanUnavailableTime;
        bandIntCfgCurrent.n79UnavailableTime  = bandIntCfgToSet.n79UnavailableTime;
    }
    else
    {
        LE_WARN("Skip current int config update");
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the WLAN 5GHz and N79 5G band wait times
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::SetBandIntWaitTime(taf_wlan_BandIntPriority_t band, uint32_t waitTime)
{
    TAF_ERROR_IF_RET_VAL( TAF_WLAN_PRIO_BAND_N79 != band && TAF_WLAN_PRIO_BAND_WLAN_5_GHZ  != band,
                                                            LE_BAD_PARAMETER, "band is invalid");
    TAF_ERROR_IF_RET_VAL(waitTime > SECONDS_IN_A_DAY, LE_BAD_PARAMETER,
                                                    "waitTime of %ds is too long", waitTime);
    TAF_ERROR_IF_RET_VAL(nullptr == dataSettingsManager || !bDataSettingManagerReady,
                         LE_NOT_POSSIBLE, "Data settings manager is not ready!");

    if (TAF_WLAN_PRIO_BAND_N79 == band)
    {
        bandIntCfgToSet.n79UnavailableTime = waitTime;
    }
    else
    {
        bandIntCfgToSet.wlanUnavailableTime = waitTime;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the WLAN 5GHz and N79 5G band wait times
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::GetBandIntWaitTime
(
    taf_wlan_BandIntPriority_t band,
    uint32_t *waitTimePtr)
{
    TAF_ERROR_IF_RET_VAL( TAF_WLAN_PRIO_BAND_N79 != band && TAF_WLAN_PRIO_BAND_WLAN_5_GHZ  != band,
                                                            LE_BAD_PARAMETER, "band is invalid");
    TAF_ERROR_IF_RET_VAL(nullptr == waitTimePtr, LE_BAD_PARAMETER,"waitTimePtr is null");
    TAF_ERROR_IF_RET_VAL(nullptr == dataSettingsManager || !bDataSettingManagerReady,
                                        LE_NOT_POSSIBLE, "Data settings manager is not ready!");

    if (TAF_WLAN_PRIO_BAND_N79 == band)
    {
        // If enabled, return the current config, else return the ToSet config.
        if (TAF_WLAN_BAND_INT_ENABLED == bandIntCfgCurrent.state)
            *waitTimePtr = bandIntCfgCurrent.n79UnavailableTime;
        else
            *waitTimePtr = bandIntCfgToSet.n79UnavailableTime;
    }
    else
    {
        // If enabled, return the current config, else return the ToSet config.
        if (TAF_WLAN_BAND_INT_ENABLED == bandIntCfgCurrent.state)
            *waitTimePtr = bandIntCfgCurrent.wlanUnavailableTime;
        else
            *waitTimePtr = bandIntCfgToSet.wlanUnavailableTime;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the WLAN 5GHz and N79 5G band interference priority.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::SetBandIntPriority(taf_wlan_BandIntPriority_t bandPriority)
{
    TAF_ERROR_IF_RET_VAL( TAF_WLAN_PRIO_BAND_N79 != bandPriority &&
                          TAF_WLAN_PRIO_BAND_WLAN_5_GHZ  != bandPriority,
                          LE_BAD_PARAMETER, "band is invalid");
    TAF_ERROR_IF_RET_VAL(nullptr == dataSettingsManager || !bDataSettingManagerReady,
                                        LE_NOT_POSSIBLE, "Data settings manager is not ready!");

    bandIntCfgToSet.prioBand = bandPriority;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the WLAN 5GHz and N79 5G band interference priority.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::GetBandIntPriority(taf_wlan_BandIntPriority_t *bandPriorityPtr)
{
    TAF_ERROR_IF_RET_VAL(nullptr == bandPriorityPtr, LE_BAD_PARAMETER,"bandPriorityPtr is null");
    TAF_ERROR_IF_RET_VAL(nullptr == dataSettingsManager || !bDataSettingManagerReady,
                                        LE_NOT_POSSIBLE, "Data settings manager is not ready!");

    LE_DEBUG("Current prio band: %d", bandIntCfgCurrent.prioBand);
    // If enabled, return the current config, else return the ToSet config.
    if (TAF_WLAN_BAND_INT_ENABLED == bandIntCfgCurrent.state)
        *bandPriorityPtr = bandIntCfgCurrent.prioBand;
    else
        *bandPriorityPtr = bandIntCfgToSet.prioBand;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The WLAN_DSCMD_BAND_INT_CFG_GET command handler
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSvcImpl::HandleBandIntGet(WlanGetBandIntCmd_t bandIntGet)
{
    // Future-Promise for synchronization
    std::promise<le_result_t> pObj;
    std::future<le_result_t> fObj = pObj.get_future();
    WlanBandIntCfg_t intConfig    = {TAF_WLAN_BAND_INT_DISABLED,
                                     TAF_WLAN_PRIO_BAND_UNKNOWN,
                                     0, 0};
    WlanGetBandIntCmdRsp_t cmdRsp;

    // The get request callback lambda function
    auto respCb = [&pObj, &intConfig](bool isEnabled,
                                      std::shared_ptr<telux::data::BandInterferenceConfig> config,
                                      telux::common::ErrorCode error)
    {
        le_result_t res = LE_OK;
        if (telux::common::ErrorCode::SUCCESS != error)
        {
            LE_WARN("requestBandInterferenceConfig cbk failed: %d", static_cast<int>(error));
            res = LE_FAULT;
        }
        else
        {
            LE_INFO("requestBandInterferenceConfig cbk succeeded");
            // Check of band interference config is enabled and update values.
            if (isEnabled)
            {
                intConfig.state    = TAF_WLAN_BAND_INT_ENABLED;
                intConfig.prioBand = taf_WlanHelper::ConvertInterferenceBand(config->priority);
                intConfig.wlanUnavailableTime = config->wlanWaitTimeInSec;
                intConfig.n79UnavailableTime  = config->n79WaitTimeInSec;
            }
        }
        // Callback is complete
        pObj.set_value(res);
    };

    // Request the band interference config from the data settings manager.
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    // Check if the data settings manager is ready or not.
    if (nullptr == myWlan.dataSettingsManager || !myWlan.bDataSettingManagerReady)
    {
        LE_WARN("Data settings manager is not ready");
        cmdRsp.result = LE_NOT_POSSIBLE;
        // Complete promGetBandIntConfig so that the main thread can get the response, only if the
        // main thread is expecting it.
        if (bWaitingForIntGetPromise.load())
        {
            myWlan.promGetBandIntConfig.set_value(cmdRsp);
        }
        return;
    }

    // Data settings manager is ready. Proceed with the get request.
    telux::common::Status ret = myWlan.dataSettingsManager->requestBandInterferenceConfig(respCb);
    if (telux::common::Status::SUCCESS != ret)
    {
        // Command failed. Return error to the app's callback handler.
        LE_WARN("requestBandInterferenceConfig failed: %d", static_cast<int>(ret));
        cmdRsp.result = LE_FAULT;
    }
    else
    {
        // Command is running. Wait for callback to finish and return.
        LE_DEBUG("Wait for callback");
        le_result_t result = fObj.get();
        if (LE_OK == result)
        {
            cmdRsp.result = LE_OK;
            cmdRsp.config.state    = intConfig.state;
            cmdRsp.config.prioBand = intConfig.prioBand;
            cmdRsp.config.wlanUnavailableTime = intConfig.wlanUnavailableTime;
            cmdRsp.config.n79UnavailableTime  = intConfig.n79UnavailableTime;
        }
        else
        {
            cmdRsp.result = result;
        }
    }
    // Complete promGetBandIntConfig so that the main thread can get the response, only if the main
    // thread is expecting it.
    if (bWaitingForIntGetPromise.load())
    {
        myWlan.promGetBandIntConfig.set_value(cmdRsp);
    }
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * The WLAN_DSCMD_BAND_INT_CFG_SET command handler
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSvcImpl::HandleBandIntSet(WlanSetBandIntCmd_t bandIntSet)
{
    bool bEnable = false;
    le_result_t result = LE_OK;
    std::shared_ptr<telux::data::BandInterferenceConfig> config = nullptr;
    if (TAF_WLAN_BAND_INT_ENABLED == bandIntSet.config.state)
    {
        LE_DEBUG("Enable band int");
        bEnable = true;
    }
    if (bEnable)
    {
        config = std::make_shared<telux::data::BandInterferenceConfig>();
        config->priority = taf_WlanHelper::ConvertInterferenceBand(bandIntSet.config.prioBand);
        config->wlanWaitTimeInSec = bandIntSet.config.wlanUnavailableTime;
        config->n79WaitTimeInSec = bandIntSet.config.n79UnavailableTime;
        LE_DEBUG("priority: %d", static_cast<int>(config->priority));
        LE_DEBUG("wlanWaitTimeInSec: %d", static_cast<int>(config->wlanWaitTimeInSec));
        LE_DEBUG("n79WaitTimeInSec: %d", static_cast<int>(config->n79WaitTimeInSec));
    }

    // Future-Promise for synchronization
    std::promise<le_result_t> pObj;
    std::future<le_result_t> fObj = pObj.get_future();

    auto respCb = [&pObj](telux::common::ErrorCode error)
    {
        le_result_t result = LE_OK;
        if (telux::common::ErrorCode::SUCCESS == error)
        {
            LE_INFO("setBandInterferenceConfig cbk succeeded");
        }
        else
        {
            LE_WARN("setBandInterferenceConfig cbk failed: %d", static_cast<int>(error));
            result = LE_FAULT;
        }
        pObj.set_value(result); // Callback is complete
    };

    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    // Check if the data settings manager is ready or not.
    if (nullptr == myWlan.dataSettingsManager || !myWlan.bDataSettingManagerReady)
    {
        LE_WARN("Data settings manager is not ready");
        // Complete promGetBandIntConfig so that the main thread can get the response, only if the
        // main thread is expecting it.
        if (bWaitingForIntGetPromise.load())
        {
            myWlan.promSetBandIntConfig.set_value(LE_NOT_POSSIBLE);
        }
        return;
    }

    // Data settings manager is ready, proceed with the set operation.
    telux::common::Status ret = myWlan.dataSettingsManager->setBandInterferenceConfig(bEnable,
                                                                                    config, respCb);
    if (telux::common::Status::SUCCESS != ret)
    {
        // Command failed. Return error to the app's callback handler.
        LE_WARN("setBandInterferenceConfig failed: %d", static_cast<int>(ret));

        // Complete promSetBandIntConfig so that the main thread can get the response
        result = LE_FAULT;
    }
    else
    {
        // Command is running. Wait for callback to finish and return.
        LE_DEBUG("Wait for callback");
        result = fObj.get();
    }

    // Complete promSetBandIntConfig so that the main thread can get the response, only if the main
    // thread is expecting it.
    if (bWaitingForIntSetPromise.load())
    {
        myWlan.promSetBandIntConfig.set_value(result);
    }
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * The band interference config set/get command event handler
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSvcImpl::DataSettingsCmdHandler(void *PayloadPtrPtr)
{
    TAF_ERROR_IF_RET_NIL(NULL == PayloadPtrPtr, "PayloadPtrPtr is NULL!");
    auto &myWlan = taf_WlanSvcImpl::GetInstance();

    WlanDataSettingsCmd_t *WlanCmdPtr = static_cast<WlanDataSettingsCmd_t *>(PayloadPtrPtr);

    switch (WlanCmdPtr->cmdType)
    {
    case WLAN_DSCMD_BAND_INT_CFG_GET:
        LE_INFO("WLAN_DSCMD_BAND_INT_CFG_GET");
        myWlan.HandleBandIntGet(WlanCmdPtr->bandIntGetCmd);
        break;
    case WLAN_DSCMD_BAND_INT_CFG_SET:
        LE_INFO("WLAN_DSCMD_BAND_INT_CFG_SET");
        myWlan.HandleBandIntSet(WlanCmdPtr->bandIntSetCmd);
        break;

    default:
        LE_WARN("Unknown command type: %d", WlanCmdPtr->cmdType);
        break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The band interference config set/get command thread where the event loop is run to receive
 * commands.
 */
//--------------------------------------------------------------------------------------------------
void *taf_WlanSvcImpl::DataSettingsThreadHdlr(void *context)
{
    auto &myWlan = taf_WlanSvcImpl::GetInstance();

    // Get the data settings manager
    if (nullptr == myWlan.dataSettingsManager)
    {
        auto &dataFactory = telux::data::DataFactory::getInstance();
        // Use getDataSettingsManager without callback to get ServiceStatus
        myWlan.dataSettingsManager = dataFactory.getDataSettingsManager(
                                                        telux::data::OperationType::DATA_LOCAL);
        if (nullptr == myWlan.dataSettingsManager)
        {
            LE_ERROR("Failed to get Data Settings manager instance");
            myWlan.promDataSettingThreadStart.set_value(LE_FAULT);
            return nullptr;
        }
        else
        {
            telux::common::ServiceStatus status = myWlan.dataSettingsManager->getServiceStatus();
            if (status != telux::common::ServiceStatus::SERVICE_AVAILABLE)
            {
                LE_INFO("Data settings manager is ready");
                myWlan.bDataSettingManagerReady = true;
            }
            else
            {
                LE_WARN("Data Settings manager service status is not available (%d)",
                                                                        static_cast<int>(status));
                // Use getDataSettingsManager with callback as ServiceStatus was not AVAILABLE
                std::promise<telux::common::ServiceStatus> promStatus;
                myWlan.dataSettingsManager = dataFactory.getDataSettingsManager(
                    telux::data::OperationType::DATA_LOCAL,
                    [&](telux::common::ServiceStatus svcStatus)
                    {
                        if (svcStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                        {
                            LE_INFO("getDataSettingsManager promStatus.set_value AVAILABLE...");
                            promStatus.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                        }
                        else
                        {
                            LE_INFO("getDataSettingsManager promStatus.set_value FAILED...");
                            promStatus.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
                        }
                    });
                LE_INFO("Waiting for data settings subsystem to be ready...");
                std::future<telux::common::ServiceStatus> futStatus = promStatus.get_future();
                std::future_status waitStatus = futStatus.wait_for(std::chrono::seconds(
                                                    TAF_WLAN_GET_DATA_SETTINGS_TIMEOUT));
                if (std::future_status::timeout == waitStatus)
                {
                    LE_ERROR("Timeout waiting for Data setting subsystem");
                    myWlan.promDataSettingThreadStart.set_value(LE_FAULT);
                    return nullptr;
                }
                else
                {
                    status = futStatus.get();
                }
                if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    myWlan.bDataSettingManagerReady = true;
                    LE_INFO("Data settings manager is ready");
                }
                else
                {
                    LE_ERROR("Failed to init data setting manager subsystem");
                    myWlan.promDataSettingThreadStart.set_value(LE_FAULT);
                    return nullptr;
                }
            }
        }
    }
    // Create the event ID for the BandIntCfg command
    myWlan.dataSettingsCmd = le_event_CreateId("dataSettingsCmd", sizeof(WlanDataSettingsCmd_t));
    // Add the handler for the BandIntCfg command
    le_event_AddHandler("dataSettingsCmd handler", myWlan.dataSettingsCmd, DataSettingsCmdHandler);
    // Start the event loop to receive the BandIntCfg commands
    LE_INFO("DataSettingsThread started");
    myWlan.promDataSettingThreadStart.set_value(LE_OK);
    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Timer from which the first band interference configuration is retrieved. This will run once.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSvcImpl::GetFirstBandIntConfigTimerHdlr(le_timer_Ref_t timerRef)
{
    taf_wlan_BandIntState_t state;
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    le_result_t result = myWlan.GetBandIntState(&state);
    if (LE_OK != result)
    {
        LE_WARN("Unable to get current band interference state");
    }
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Service initialization function
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSvcImpl::Init(void)
{
    // Initialize relevant variables
    wlanDevMgr = nullptr;
    // WLAN 5G and N79 5G band interference configuration current settings
    ResetBandIntCfg(bandIntCfgCurrent);
    // WLAN 5G and N79 5G band interference configuration current settings
    ResetBandIntCfg(bandIntCfgToSet);

    std::promise<telux::common::ServiceStatus> initPromise;
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;

    // [1] Instantiate subsystem initialization callback
    auto initCb = [&](telux::common::ServiceStatus status) {
         initPromise.set_value(status);
    };
    // [2] Get the WlanFactory and Device Manager instance
    auto &wlanFactory = telux::wlan::WlanFactory::getInstance();

    do {
        wlanDevMgr  = wlanFactory.getWlanDeviceManager(initCb);
        if (wlanDevMgr) {
            // [3] Check if Device manager is ready
            LE_INFO ("Initializing Wlan subsystem Please wait ...");
            subSystemStatus = initPromise.get_future().get();
            LE_INFO ("Subsystem Status = %d", (int)subSystemStatus);
        }
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LE_INFO (" *** Wlan SubSystem is Ready *** ");
        }
        else {
            wlanDevMgr = nullptr;
            // Unable to initialize the WLAN subsystem. Stop the service.
            LE_FATAL (" *** Unable to initialize Wlan subsystem *** ");
        }
    }while(0);

    wlanListener = std::make_shared<taf_WlanListener>();
    // Register the Listener class
    telux::common::ErrorCode retCode = wlanDevMgr->registerListener(wlanListener);
    if (telux::common::ErrorCode::SUCCESS != retCode)
    {
        LE_WARN("WLAN registerListener failed: %d", (int)retCode);
    }

    // Create WLAN state event ID
    wlanDevStateChangeEvID = le_event_CreateIdWithRefCounting("DeviceStateChangeEvent");

    // Create wlan mutex
    wlanMutexRef =  le_mutex_CreateRecursive("WlanMutex");
    // Create mem pool for state change event reporting.
    DeviceStatusPoolRef = le_mem_InitStaticPool(DeviceStatusPool, TAF_WLAN_MAX_SESSION_REF,
                                                sizeof(taf_wlan_DeviceState_t));

    // Create the band interference config thread
    dataSettingsThreadRef = le_thread_Create("DataSettingsThread", DataSettingsThreadHdlr, NULL);
    // Start the thread and wait for it to finish initializing.
    promDataSettingThreadStart = std::promise<le_result_t>();
    std::future<le_result_t> futDataSettingThreadStart = promDataSettingThreadStart.get_future();
    le_thread_Start(dataSettingsThreadRef);
    le_result_t result = futDataSettingThreadStart.get();
    if (LE_OK == result)
    {
        bDataSettingManagerReady = true;
        // Start a timer to get the current config after a small delay. This is a one shot timer.
        getFirstBandIntConfigTimerRef = le_timer_Create("GetFirstBandIntConfigTimer");
        le_timer_SetWakeup(getFirstBandIntConfigTimerRef, false);
        le_timer_SetHandler(getFirstBandIntConfigTimerRef, GetFirstBandIntConfigTimerHdlr);
        le_timer_SetMsInterval(getFirstBandIntConfigTimerRef, GetFirstBandIntConfigInterval);
        le_timer_Start(getFirstBandIntConfigTimerRef);
    }
    else
    {
        bDataSettingManagerReady = false;
    }
    LE_INFO("Init done");
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the service status and send a notificaion in case of failure.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSvcImpl::SetDeviceState (bool enable)
{
    taf_wlan_DeviceState_t *devStatePtr = NULL;
    // Send event to applications.
    le_mutex_Lock(wlanMutexRef);

    devStatePtr = (taf_wlan_DeviceState_t *)le_mem_ForceAlloc(DeviceStatusPoolRef);
    if (enable) {
        LE_INFO( "Send TAF_WLAN_ON Event" );
        *devStatePtr = TAF_WLAN_ON;
    } else {
        LE_INFO( "Send TAF_WLAN_OFF Event" );
        *devStatePtr = TAF_WLAN_OFF;
    }

    le_event_ReportWithRefCounting(wlanDevStateChangeEvID, (void *)devStatePtr);

    le_mutex_Unlock(wlanMutexRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the service status and send a notificaion in case of failure.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSvcImpl::SetSubsystemState (telux::common::ServiceStatus status)
{
    taf_wlan_DeviceState_t *devStatePtr = NULL;

    wlanSubSystemState = status;

    if ( telux::common::ServiceStatus::SERVICE_UNAVAILABLE == status||
         telux::common::ServiceStatus::SERVICE_FAILED == status)
    {
        // Send TAF_WLAN_UNAVAILABLE event to applications.
        le_mutex_Lock(wlanMutexRef);
        LE_INFO( "Send TAF_WLAN_UNAVAILABLE Event" );
        devStatePtr = (taf_wlan_DeviceState_t *)le_mem_ForceAlloc(DeviceStatusPoolRef);
        *devStatePtr = TAF_WLAN_UNAVAILABLE;
        le_event_ReportWithRefCounting(wlanDevStateChangeEvID, (void *)devStatePtr);
        le_mutex_Unlock(wlanMutexRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Return the instance of taf_Wlan class.
 */
//--------------------------------------------------------------------------------------------------
taf_WlanSvcImpl &taf_WlanSvcImpl::GetInstance()
{
    static taf_WlanSvcImpl instance;
    return instance;
}
