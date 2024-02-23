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
 * Starts the specified Access Point.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanAp_Start
(
    taf_wlanAp_WlanAPRef_t wlanAPRef
        ///< [IN] The WLAN AP reference. Reserved for future use.
)
{
    LE_UNUSED (wlanAPRef);
    auto &myWlan = taf_WlanAPSvcImpl::GetInstance();
    return myWlan.Start();
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
    taf_wlanAp_WlanAPRef_t wlanAPRef
        ///< [IN] The WLAN AP reference. Reserved for future use.
)
{
    LE_UNUSED (wlanAPRef);
    auto &myWlan = taf_WlanAPSvcImpl::GetInstance();
    return myWlan.Stop();
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
    taf_wlanAp_WlanAPRef_t wlanAPRef
        ///< [IN] The WLAN AP reference. Reserved for future use.
)
{
    LE_UNUSED (wlanAPRef);
    auto &myWlan = taf_WlanAPSvcImpl::GetInstance();
    return myWlan.Restart();
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
        ///< [IN] The WLAN AP reference. Reserved for future use.
    const taf_wlanAp_WlanAPConfig_t * LE_NONNULL ConfigPtr
        ///< [IN] The WLAN AP configuration.
)
{
    LE_UNUSED (wlanAPRef);
    TAF_ERROR_IF_RET_VAL(NULL == ConfigPtr, LE_BAD_PARAMETER, "ConfigPtr is NULL!");
    auto &myWlan = taf_WlanAPSvcImpl::GetInstance();
    return myWlan.SetConfig(ConfigPtr);
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
        ///< [IN] The WLAN AP reference. Reserved for future use.
    taf_wlanAp_WlanAPConfig_t * ConfigPtr
        ///< [OUT] The WLAN AP configuration.
)
{
    LE_UNUSED (wlanAPRef);
    TAF_ERROR_IF_RET_VAL(NULL == ConfigPtr, LE_BAD_PARAMETER, "ConfigPtr is NULL!");
    auto &myWlan = taf_WlanAPSvcImpl::GetInstance();
    return myWlan.GetConfig(ConfigPtr);
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
        ///< [IN] The WLAN AP reference. Reserved for future use.
    const taf_wlanAp_WlanAPSecurityConfig_t * LE_NONNULL SecurityConfigPtr
        ///< [IN] The WLAN AP security configuration.
)
{
    LE_UNUSED (wlanAPRef);
    TAF_ERROR_IF_RET_VAL(NULL == SecurityConfigPtr, LE_BAD_PARAMETER, "SecurityConfigPtr is NULL!");
    auto &myWlan = taf_WlanAPSvcImpl::GetInstance();
    return myWlan.SetSecurityConfig(SecurityConfigPtr);
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
        ///< [IN] The WLAN AP reference. Reserved for future use.
    taf_wlanAp_WlanAPSecurityConfig_t * SecurityConfigPtr
        ///< [OUT] The WLAN AP security configuration.
)
{
    LE_UNUSED (wlanAPRef);
    TAF_ERROR_IF_RET_VAL(NULL == SecurityConfigPtr, LE_BAD_PARAMETER, "SecurityConfigPtr is NULL!");
    auto &myWlan = taf_WlanAPSvcImpl::GetInstance();
    return myWlan.GetSecurityConfig(SecurityConfigPtr);
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
        ///< [IN] The WLAN AP reference. Reserved for future use.
    taf_wlanAp_WlanAPStatus_t * StatusPtr
        ///< [OUT] The WLAN AP status.
)
{
    LE_UNUSED (wlanAPRef);
    TAF_ERROR_IF_RET_VAL(NULL == StatusPtr, LE_BAD_PARAMETER, "StatusPtr is NULL!");
    auto &myWlan = taf_WlanAPSvcImpl::GetInstance();
    return myWlan.GetStatus(StatusPtr);
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
        ///< [IN] The WLAN AP reference. Reserved for future use.
    uint16_t* numDevicesPtr,
        ///< [OUT] Number of devices connected to the AP.
    taf_wlanAp_WlanAPConnectedDeviceInfo_t* DevInfoPtr,
        ///< [OUT] Connected device information.
    size_t* DevInfoSizePtr
        ///< [INOUT]
)
{
    LE_UNUSED (wlanAPRef);
    TAF_ERROR_IF_RET_VAL(NULL == numDevicesPtr, LE_BAD_PARAMETER, "numDevicesPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(NULL == DevInfoPtr, LE_BAD_PARAMETER, "DevInfoPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(NULL == DevInfoSizePtr, LE_BAD_PARAMETER, "DevInfoSizePtr is NULL!");
    auto &myWlan = taf_WlanAPSvcImpl::GetInstance();
    return myWlan.GetConnectedDevices(numDevicesPtr,DevInfoPtr,DevInfoSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns the WLAN AP reference.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
taf_wlanAp_WlanAPRef_t taf_wlanAp_GetWlanAP
(
    taf_wlan_APid_t APid,
        ///< [IN] AP identifier
    const char* LE_NONNULL APIntfName
        ///< [IN] AP assocaited host interface name.
)
{
    TAF_ERROR_IF_RET_VAL(NULL == APIntfName, NULL, "APIntfName is NULL!");
    auto &myWlan = taf_WlanAPSvcImpl::GetInstance();
    return myWlan.GetWlanAP(APid,APIntfName);
}
