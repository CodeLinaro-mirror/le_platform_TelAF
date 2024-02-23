/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanAPSvc.cpp
 *
 * @brief      Server side interface for TelAF WLAN Access Point Service APIs.
 *
 */


#include "tafWlan.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Returns the WLAN AP reference.
 *
 * @return
 * - WLAN AP reference on success, nullptr on failure
 */
//--------------------------------------------------------------------------------------------------
taf_wlanAp_WlanAPRef_t taf_wlanAp_GetWlanAP
(
    taf_wlan_APid_t apID,               ///< [IN] AP identifier
    const char* LE_NONNULL apIntfName   ///< [IN] AP assocaited host interface name.
)
{
    TAF_ERROR_IF_RET_VAL(apIntfName == nullptr, nullptr, "apIntfName is nullptr!");
    auto &wlan = taf_WlanAPSvcImpl::GetInstance();
    return wlan.GetWlanAPReference(apID, apIntfName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Starts the specified Access Point.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanAp_Start
(
    taf_wlanAp_WlanAPRef_t wlanAPRef    ///< [IN] The WLAN AP reference.
)
{
    auto &wlan = taf_WlanAPSvcImpl::GetInstance();
    return wlan.Start(wlanAPRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Stops the specified Access Point.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanAp_Stop
(
    taf_wlanAp_WlanAPRef_t wlanAPRef    ///< [IN] The WLAN AP reference.
)
{
    auto &wlan = taf_WlanAPSvcImpl::GetInstance();
    return wlan.Stop(wlanAPRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Restarts the specified Access Point.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanAp_Restart
(
    taf_wlanAp_WlanAPRef_t wlanAPRef    ///< [IN] The WLAN AP reference.
)
{
    auto &wlan = taf_WlanAPSvcImpl::GetInstance();
    return wlan.Restart(wlanAPRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the configuration for the specified Access Point.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanAp_SetConfig
(
    taf_wlanAp_WlanAPRef_t wlanAPRef,
        ///< [IN] The WLAN AP reference.
    const taf_wlanAp_WlanAPConfig_t * LE_NONNULL ConfigPtr
        ///< [IN] The WLAN AP configuration.
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == ConfigPtr, LE_BAD_PARAMETER, "ConfigPtr is nullptr!");
    auto &wlan = taf_WlanAPSvcImpl::GetInstance();
    return wlan.SetConfig(wlanAPRef, ConfigPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the configuration for the specified Access Point.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanAp_GetConfig
(
    taf_wlanAp_WlanAPRef_t wlanAPRef,
        ///< [IN] The WLAN AP reference.
    taf_wlanAp_WlanAPConfig_t * ConfigPtr
        ///< [OUT] The WLAN AP configuration.
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == ConfigPtr, LE_BAD_PARAMETER,
        "ConfigPtr is nullptr!");
    auto &wlan = taf_WlanAPSvcImpl::GetInstance();
    return wlan.GetConfig(wlanAPRef, ConfigPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the security configuration for the specified Access Point.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanAp_SetSecurityConfig
(
    taf_wlanAp_WlanAPRef_t wlanAPRef,
        ///< [IN] The WLAN AP reference.
    const taf_wlanAp_WlanAPSecurityConfig_t * LE_NONNULL SecurityConfigPtr
        ///< [IN] The WLAN AP security configuration.
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == SecurityConfigPtr, LE_BAD_PARAMETER,
        "SecurityConfigPtr is nullptr!");
    auto &wlan = taf_WlanAPSvcImpl::GetInstance();
    return wlan.SetSecurityConfig(wlanAPRef, SecurityConfigPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the security configuration for the specified Access Point.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanAp_GetSecurityConfig
(
    taf_wlanAp_WlanAPRef_t wlanAPRef,
        ///< [IN] The WLAN AP reference.
    taf_wlanAp_WlanAPSecurityConfig_t * SecurityConfigPtr
        ///< [OUT] The WLAN AP security configuration.
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == SecurityConfigPtr, LE_BAD_PARAMETER,
        "SecurityConfigPtr is nullptr!");
    auto &wlan = taf_WlanAPSvcImpl::GetInstance();
    return wlan.GetSecurityConfig(wlanAPRef, SecurityConfigPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the status of the specified Access Point.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanAp_GetStatus
(
    taf_wlanAp_WlanAPRef_t wlanAPRef,
        ///< [IN] The WLAN AP reference.
    taf_wlanAp_WlanAPStatus_t * StatusPtr
        ///< [OUT] The WLAN AP status.
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == StatusPtr, LE_BAD_PARAMETER,
        "StatusPtr is nullptr!");
    auto &wlan = taf_WlanAPSvcImpl::GetInstance();
    return wlan.GetStatus(wlanAPRef, StatusPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the list of devices connected to the AP
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanAp_GetConnectedDevices
(
    taf_wlanAp_WlanAPRef_t wlanAPRef,
        ///< [IN] The WLAN AP reference.
    uint16_t* numDevicesPtr,
        ///< [OUT] Number of devices connected to the AP.
    taf_wlanAp_WlanAPConnectedDeviceInfo_t* DevInfoPtr,
        ///< [OUT] Connected device information.
    size_t* DevInfoSizePtr
        ///< [INOUT]
)
{
    TAF_ERROR_IF_RET_VAL(nullptr == numDevicesPtr, LE_BAD_PARAMETER,
        "numDevicesPtr is nullptr!");
    TAF_ERROR_IF_RET_VAL(nullptr == DevInfoPtr, LE_BAD_PARAMETER,
        "DevInfoPtr is nullptr!");
    TAF_ERROR_IF_RET_VAL(nullptr == DevInfoSizePtr, LE_BAD_PARAMETER,
        "DevInfoSizePtr is nullptr!");
    auto &wlan = taf_WlanAPSvcImpl::GetInstance();
    return wlan.GetConnectedDevices(wlanAPRef, numDevicesPtr, DevInfoPtr,
        DevInfoSizePtr);
}
