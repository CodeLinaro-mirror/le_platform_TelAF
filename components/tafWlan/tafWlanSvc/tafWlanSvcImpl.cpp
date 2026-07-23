/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanSvcImpl.cpp
 *
 * @brief      Implementation of TelAF WLAN Device Management Service APIs.
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
le_event_Id_t taf_WlanSvcImpl::GetStateChangeEventID(void)
{
    LE_INFO("returning eventId=%p", wlanDevStateChangeEvID);
    return wlanDevStateChangeEvID;
}

//--------------------------------------------------------------------------------------------------
/**
 * Turn WLAN device ON. This action bring up the respective WLAN host interface.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::SetON(void)
{
    // If device is already ON, return success with a clear log.
    taf_wlan_DeviceState_t curState = TAF_WLAN_UNAVAILABLE;
    le_result_t st = GetState(&curState);
    if (st == LE_OK && curState == TAF_WLAN_ON)
    {
        LE_INFO("WLAN device already ON; no effect");
        return LE_OK;
    }

    taf_pa_result_t res = taf::pa::wlan::EnableDevice(true);
    if (res != TAF_PA_OK)
    {
        LE_ERROR("EnableDevice(true) failed, rc=%d; returning LE_FAULT", (int)res);
        return LE_FAULT;
    }
    LE_INFO("EnableDevice(true) succeeded; returning LE_OK");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Turn WLAN device OFF. This action will remove the respective WLAN host interface.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::SetOFF(void)
{
    // If device is already OFF, return success with a clear log.
    taf_wlan_DeviceState_t curState = TAF_WLAN_UNAVAILABLE;
    le_result_t st = GetState(&curState);
    if (st == LE_OK && curState == TAF_WLAN_OFF)
    {
        LE_INFO("WLAN device already OFF; no effect");
        return LE_OK;
    }

    taf_pa_result_t res = taf::pa::wlan::EnableDevice(false);
    if (res != TAF_PA_OK)
    {
        LE_ERROR("EnableDevice(false) failed, rc=%d; returning LE_FAULT", (int)res);
        return LE_FAULT;
    }
    LE_INFO("EnableDevice(false) succeeded; returning LE_OK");
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
    bool enabled = false;
    taf_pa_result_t res = taf::pa::wlan::GetStatus(enabled);
    if (res != TAF_PA_OK)
    {
        LE_ERROR("PA GetStatus failed rc=%d; returning LE_FAULT", (int)res);
        return LE_FAULT;
    }
    *statePtr = enabled ? TAF_WLAN_ON : TAF_WLAN_OFF;
    LE_INFO("enabled=%d -> state=%d; returning LE_OK", (int)enabled, (int)(*statePtr));
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
    taf_wlan_DeviceMode_t wlanMode
)
{
    int numAP = 0, numSTA = 0;
    switch (wlanMode)
    {
        case TAF_WLAN_MODE_AP:         numAP=1; numSTA=0; break;
        case TAF_WLAN_MODE_STA:        numAP=0; numSTA=1; break;
        case TAF_WLAN_MODE_STA_AP:     numAP=1; numSTA=1; break;
        case TAF_WLAN_MODE_AP_AP:      numAP=2; numSTA=0; break;
        case TAF_WLAN_MODE_AP_AP_STA:  numAP=2; numSTA=1; break;
        default:
            LE_WARN("Invalid wlanMode=%d; returning LE_BAD_PARAMETER", (int)wlanMode);
            return LE_BAD_PARAMETER;
    }
    taf_pa_result_t paRes = taf::pa::wlan::SetDeviceMode(numAP, numSTA);
    if (paRes == TAF_PA_OK)
    {
        LE_INFO("wlanMode=%d -> (AP=%d, STA=%d) succeeded; returning LE_OK",
                (int)wlanMode, numAP, numSTA);
        return LE_OK;
    }
    LE_ERROR("(AP=%d, STA=%d) failed rc=%d; returning LE_FAULT", numAP, numSTA, (int)paRes);
    return LE_FAULT;
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
    TAF_ERROR_IF_RET_VAL(!wlanModePtr, LE_BAD_PARAMETER, "wlanModePtr is NULL!");
    int numAPOut = 0, numSTAOut = 0;
    if (taf::pa::wlan::GetDeviceMode(numAPOut, numSTAOut) != TAF_PA_OK)
    {
        return LE_FAULT;
    }
    if (numAPOut == 1 && numSTAOut == 0)
        *wlanModePtr = TAF_WLAN_MODE_AP;
    else if (numAPOut == 0 && numSTAOut == 1)
        *wlanModePtr = TAF_WLAN_MODE_STA;
    else if (numAPOut == 1 && numSTAOut == 1)
        *wlanModePtr = TAF_WLAN_MODE_STA_AP;
    else if (numAPOut == 2 && numSTAOut == 0)
        *wlanModePtr = TAF_WLAN_MODE_AP_AP;
    else if (numAPOut == 2 && numSTAOut == 1)
        *wlanModePtr = TAF_WLAN_MODE_AP_AP_STA;
    else {
        *wlanModePtr = TAF_WLAN_MODE_UNKNOWN;
        LE_WARN("Unsupported WLAN mode configuration: AP=%d, STA=%d", numAPOut, numSTAOut);
        return LE_UNSUPPORTED;
    }
    LE_INFO("Current WLAN mode: (AP=%d, STA=%d)", numAPOut, numSTAOut);
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

    LE_INFO("Filled interface info: AP interfaces=%zu, STA interfaces=%zu",
            *APIntfinfoSizePtr, *STAIntfinfoSizePtr);
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
    // Fill in the interface names in TelAF as TelSDK will not provide the interface names in all
    // scenarios.
    le_result_t result = FillIntfInfo(APIntfinfoPtr, APIntfinfoSizePtr, STAIntfinfoPtr, STAIntfinfoSizePtr);
    if (result == LE_OK)
    {
        LE_INFO("Retrieved interface info: AP interfaces=%zu, STA interfaces=%zu",
                *APIntfinfoSizePtr, *STAIntfinfoSizePtr);
    }
    else
    {
        LE_ERROR("Failed to get interface information");
    }
    return result;
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

    bool enabledOut = false;
    taf::pa::wlan::BandInterferenceConfig_t paCfgOut = {};
    if (taf::pa::wlan::GetBandInterferenceConfig(enabledOut, paCfgOut) != TAF_PA_OK)
    {
        LE_ERROR("Failed to get band interference configuration");
        return LE_FAULT;
    }

    *statePtr = enabledOut ? TAF_WLAN_BAND_INT_ENABLED : TAF_WLAN_BAND_INT_DISABLED;

    // Update caches similar to legacy behavior
    bandIntCfgCurrent.state = *statePtr;
    if (bandIntCfgCurrent.state == TAF_WLAN_BAND_INT_ENABLED)
    {
        if (paCfgOut.prioBand == taf::pa::wlan::BandIntPriority_e::N79)
        {
            bandIntCfgCurrent.prioBand = TAF_WLAN_PRIO_BAND_N79;
        }
        else
        {
            bandIntCfgCurrent.prioBand = TAF_WLAN_PRIO_BAND_WLAN_5_GHZ;
        }
        bandIntCfgCurrent.wlanUnavailableTime = paCfgOut.wlanWaitTimeInSec;
        bandIntCfgCurrent.n79UnavailableTime = paCfgOut.n79WaitTimeInSec;

        bandIntCfgToSet = bandIntCfgCurrent;
    }

    LE_INFO("Band interference state: %s",
            (*statePtr == TAF_WLAN_BAND_INT_ENABLED) ? "ENABLED" : "DISABLED");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the WLAN 5GHz and N79 5G band interference state.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::SetBandIntState(taf_wlan_BandIntState_t state)
{
    TAF_ERROR_IF_RET_VAL(state != TAF_WLAN_BAND_INT_DISABLED &&
                         state != TAF_WLAN_BAND_INT_ENABLED,
                         LE_BAD_PARAMETER, "Invalid state");

    bool enable = (state == TAF_WLAN_BAND_INT_ENABLED);
    taf::pa::wlan::BandInterferenceConfig_t cfg = {};
    if (enable)
    {
        if(bandIntCfgToSet.prioBand == TAF_WLAN_PRIO_BAND_N79)
        {
            cfg.prioBand =
                taf::pa::wlan::BandIntPriority_e::N79;
        }
        else if(bandIntCfgToSet.prioBand == TAF_WLAN_PRIO_BAND_WLAN_5_GHZ)
        {
            cfg.prioBand =
                taf::pa::wlan::BandIntPriority_e::WLAN_5_GHZ;
        }
        cfg.wlanWaitTimeInSec  = bandIntCfgToSet.wlanUnavailableTime;
        cfg.n79WaitTimeInSec   = bandIntCfgToSet.n79UnavailableTime;
    }

    taf_pa_result_t paRes = taf::pa::wlan::SetBandInterferenceConfig(enable, cfg);
    if (paRes != TAF_PA_OK)
    {
        LE_ERROR("SetBandInterferenceConfig failed, errorcode: %d", (int)paRes);
        return LE_FAULT;
    }

    // Update current cached value on success
    bandIntCfgCurrent.state               = state;
    bandIntCfgCurrent.prioBand            = bandIntCfgToSet.prioBand;
    bandIntCfgCurrent.wlanUnavailableTime = bandIntCfgToSet.wlanUnavailableTime;
    bandIntCfgCurrent.n79UnavailableTime  = bandIntCfgToSet.n79UnavailableTime;

    LE_INFO("Band interference state set to %s",
            (state == TAF_WLAN_BAND_INT_ENABLED) ? "ENABLED" : "DISABLED");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the WLAN 5GHz and N79 5G band wait times
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::SetBandIntWaitTime(taf_wlan_BandIntPriority_t band, uint32_t waitTime)
{
    TAF_ERROR_IF_RET_VAL(band != TAF_WLAN_PRIO_BAND_N79 && band != TAF_WLAN_PRIO_BAND_WLAN_5_GHZ,
                         LE_BAD_PARAMETER, "band is invalid");
    TAF_ERROR_IF_RET_VAL(waitTime > SECONDS_IN_A_DAY, LE_BAD_PARAMETER,
                         "waitTime of %ds is too long", waitTime);

    if (band == TAF_WLAN_PRIO_BAND_N79)
    {
        bandIntCfgToSet.n79UnavailableTime = waitTime;
        LE_INFO("N79 band wait time set to %u seconds", waitTime);
    }
    else
    {
        bandIntCfgToSet.wlanUnavailableTime = waitTime;
        LE_INFO("WLAN 5GHz band wait time set to %u seconds", waitTime);
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
    uint32_t *waitTimePtr
)
{
    TAF_ERROR_IF_RET_VAL(band != TAF_WLAN_PRIO_BAND_N79 && band != TAF_WLAN_PRIO_BAND_WLAN_5_GHZ,
                         LE_BAD_PARAMETER, "band is invalid");
    TAF_ERROR_IF_RET_VAL(nullptr == waitTimePtr, LE_BAD_PARAMETER, "waitTimePtr is null");

    if (band == TAF_WLAN_PRIO_BAND_N79)
    {
        *waitTimePtr = (bandIntCfgCurrent.state == TAF_WLAN_BAND_INT_ENABLED)
                           ? bandIntCfgCurrent.n79UnavailableTime
                           : bandIntCfgToSet.n79UnavailableTime;
        LE_INFO("N79 band wait time: %u seconds", *waitTimePtr);
    }
    else
    {
        *waitTimePtr = (bandIntCfgCurrent.state == TAF_WLAN_BAND_INT_ENABLED)
                           ? bandIntCfgCurrent.wlanUnavailableTime
                           : bandIntCfgToSet.wlanUnavailableTime;
        LE_INFO("WLAN 5GHz band wait time: %u seconds", *waitTimePtr);
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
    TAF_ERROR_IF_RET_VAL(bandPriority != TAF_WLAN_PRIO_BAND_N79 &&
                         bandPriority != TAF_WLAN_PRIO_BAND_WLAN_5_GHZ,
                         LE_BAD_PARAMETER, "band is invalid");
    bandIntCfgToSet.prioBand = bandPriority;
    LE_INFO("Band interference priority set to %s",
            (bandPriority == TAF_WLAN_PRIO_BAND_N79) ? "N79" : "WLAN_5_GHZ");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the WLAN 5GHz and N79 5G band interference priority.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanSvcImpl::GetBandIntPriority(taf_wlan_BandIntPriority_t *bandPriorityPtr)
{
    TAF_ERROR_IF_RET_VAL(nullptr == bandPriorityPtr, LE_BAD_PARAMETER, "bandPriorityPtr is null");

    *bandPriorityPtr = (bandIntCfgCurrent.state == TAF_WLAN_BAND_INT_ENABLED)
        ? bandIntCfgCurrent.prioBand
        : bandIntCfgToSet.prioBand;
    LE_INFO("Band interference priority: %s",
            (*bandPriorityPtr == TAF_WLAN_PRIO_BAND_N79) ? "N79" : "WLAN_5_GHZ");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The WLAN_DSCMD_BAND_INT_CFG_GET command handler
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSvcImpl::HandleBandIntGet(WlanGetBandIntCmd_t bandIntGet)
{
    WlanGetBandIntCmdRsp_t cmdRsp;
    cmdRsp.result     = LE_FAULT;
    cmdRsp.contextPtr = bandIntGet.contextPtr;

    bool enabledOut = false;
    taf::pa::wlan::BandInterferenceConfig_t paCfgOut = {};

    if (taf::pa::wlan::GetBandInterferenceConfig(enabledOut, paCfgOut) == TAF_PA_OK)
    {
        cmdRsp.result = LE_OK;
        cmdRsp.config.state = enabledOut ? TAF_WLAN_BAND_INT_ENABLED : TAF_WLAN_BAND_INT_DISABLED;
        cmdRsp.config.prioBand = static_cast<taf_wlan_BandIntPriority_t>(paCfgOut.prioBand);
        cmdRsp.config.wlanUnavailableTime = paCfgOut.wlanWaitTimeInSec;
        cmdRsp.config.n79UnavailableTime = paCfgOut.n79WaitTimeInSec;

        bandIntCfgCurrent.state = cmdRsp.config.state;
        if (bandIntCfgCurrent.state == TAF_WLAN_BAND_INT_ENABLED)
        {
            bandIntCfgCurrent.prioBand = cmdRsp.config.prioBand;
            bandIntCfgCurrent.wlanUnavailableTime = cmdRsp.config.wlanUnavailableTime;
            bandIntCfgCurrent.n79UnavailableTime = cmdRsp.config.n79UnavailableTime;

            bandIntCfgToSet = bandIntCfgCurrent;
        }
    }

    if (bWaitingForIntGetPromise.load())
    {
        promGetBandIntConfig.set_value(cmdRsp);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The WLAN_DSCMD_BAND_INT_CFG_SET command handler
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSvcImpl::HandleBandIntSet(WlanSetBandIntCmd_t bandIntSet)
{
    bool enable = (bandIntSet.config.state == TAF_WLAN_BAND_INT_ENABLED);
    taf::pa::wlan::BandInterferenceConfig_t cfg = {};
    if (enable)
    {
        if(bandIntSet.config.prioBand == TAF_WLAN_PRIO_BAND_N79)
        {
            cfg.prioBand =
                taf::pa::wlan::BandIntPriority_e::N79;
        }
        else
        {
            cfg.prioBand =
                taf::pa::wlan::BandIntPriority_e::WLAN_5_GHZ;
        }
        cfg.wlanWaitTimeInSec  = bandIntSet.config.wlanUnavailableTime;
        cfg.n79WaitTimeInSec   = bandIntSet.config.n79UnavailableTime;
    }

    taf_pa_result_t paRes = taf::pa::wlan::SetBandInterferenceConfig(enable, cfg);
    le_result_t result = (paRes == TAF_PA_OK) ? LE_OK : LE_FAULT;

    if (bWaitingForIntSetPromise.load()) {
        promSetBandIntConfig.set_value(result);
    }
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
            myWlan.HandleBandIntGet(WlanCmdPtr->bandIntGetCmd);
            break;
        case WLAN_DSCMD_BAND_INT_CFG_SET:
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

    // This thread only creates the internal event and runs the loop.
    // PA owns any Telux interactions.

    // Create the event ID for the BandInterference config commands
    myWlan.dataSettingsCmd = le_event_CreateId("dataSettingsCmd", sizeof(WlanDataSettingsCmd_t));

    // Register the command handler
    le_event_AddHandler("dataSettingsCmd handler", myWlan.dataSettingsCmd, DataSettingsCmdHandler);

    // Signal the main thread that the internal event loop is ready
    myWlan.promDataSettingThreadStart.set_value(LE_OK);

    // Start the event loop to receive internal band interference commands
    LE_INFO("DataSettingsThread started");
    le_event_RunLoop();

    return nullptr;
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
    // Initialize cached configs
    ResetBandIntCfg(bandIntCfgCurrent);
    ResetBandIntCfg(bandIntCfgToSet);

    // Create events/mutex/mem pools
    wlanDevStateChangeEvID = le_event_CreateIdWithRefCounting("DeviceStateChangeEvent");
    wlanMutexRef =  le_mutex_CreateRecursive("WlanMutex");
    DeviceStatusPoolRef = le_mem_InitStaticPool(DeviceStatusPool, TAF_WLAN_MAX_SESSION_REF,
                                                sizeof(taf_wlan_DeviceState_t));

    // Initialize PA (OSS or default)
    taf_pa_result_t paRes = taf::pa::wlan::Init();
    if (paRes != TAF_PA_OK) {
        LE_FATAL("*** Unable to initialize WLAN PA, ret=%d", (int)paRes);
    }

    // Register PA device listener
    taf::pa::wlan::RegisterDeviceListener(
        [](bool enabled, taf::pa::wlan::ServiceState_e serviceStatus, std::any){
            auto &svc = taf_WlanSvcImpl::GetInstance();
            svc.SetDeviceState(enabled);
            svc.SetSubsystemState(serviceStatus);
        },
        nullptr
    );

    // Create the event ID for the BandIntCfg command and add handler
    dataSettingsCmd = le_event_CreateId("dataSettingsCmd", sizeof(WlanDataSettingsCmd_t));
    le_event_AddHandler("dataSettingsCmd handler", dataSettingsCmd, DataSettingsCmdHandler);

    // Optionally kick off a single GET to populate caches (or via timer if you prefer)
    WlanDataSettingsCmd_t cmd;
    cmd.cmdType = WLAN_DSCMD_BAND_INT_CFG_GET;
    cmd.bandIntGetCmd.contextPtr = nullptr;
    le_event_Report(dataSettingsCmd, &cmd, sizeof(cmd));

    LE_INFO("Init done");
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the service status and send a notificaion in case of failure.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSvcImpl::SetDeviceState (bool enable)
{
    le_mutex_Lock(wlanMutexRef);

    auto *devStatePtr = (taf_wlan_DeviceState_t *)le_mem_ForceAlloc(DeviceStatusPoolRef);
    *devStatePtr = enable ? TAF_WLAN_ON : TAF_WLAN_OFF;
    le_event_ReportWithRefCounting(wlanDevStateChangeEvID, (void *)devStatePtr);

    le_mutex_Unlock(wlanMutexRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the service status and send a notificaion in case of failure.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSvcImpl::SetSubsystemState(taf::pa::wlan::ServiceState_e serviceStatus)
{
    // Only signal UNAVAILABLE on explicit failure/unavailability.
    bool unavailable = (serviceStatus == taf::pa::wlan::ServiceState_e::SERVICE_UNAVAILABLE) ||
                       (serviceStatus == taf::pa::wlan::ServiceState_e::SERVICE_FAILED);

    if (!unavailable)
    {
        return; // available or ready
    }

    // Send TAF_WLAN_UNAVAILABLE event to applications.
    le_mutex_Lock(wlanMutexRef);
    LE_INFO("Send TAF_WLAN_UNAVAILABLE Event");
    auto *devStatePtr = (taf_wlan_DeviceState_t *)le_mem_ForceAlloc(DeviceStatusPoolRef);
    *devStatePtr = TAF_WLAN_UNAVAILABLE;
    le_event_ReportWithRefCounting(wlanDevStateChangeEvID, (void *)devStatePtr);
    le_mutex_Unlock(wlanMutexRef);
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
