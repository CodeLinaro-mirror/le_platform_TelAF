/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanSTASvc.cpp
 *
 * @brief      Server side interface for TelAF WLAN Station Service APIs.
 *
 */


#include "tafWlan.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Starts the specified station.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanSta_Start
(
    taf_wlanSta_WlanSTARef_t wlanSTARef
        ///< [IN] The WLAN STA reference.
)
{
    TAF_ERROR_IF_RET_VAL(NULL == wlanSTARef, LE_BAD_PARAMETER, "wlanSTARef is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.Start(wlanSTARef);
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
le_result_t taf_wlanSta_Stop
(
    taf_wlanSta_WlanSTARef_t wlanSTARef
        ///< [IN] The WLAN STA reference.
)
{
    TAF_ERROR_IF_RET_VAL(NULL == wlanSTARef, LE_BAD_PARAMETER, "wlanSTARef is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.Stop(wlanSTARef);
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
le_result_t taf_wlanSta_Restart
(
    taf_wlanSta_WlanSTARef_t wlanSTARef
        ///< [IN] The WLAN STA reference.
)
{
    TAF_ERROR_IF_RET_VAL(NULL == wlanSTARef, LE_BAD_PARAMETER, "wlanSTARef is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.Restart(wlanSTARef);
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
le_result_t taf_wlanSta_SetMode
(
    taf_wlanSta_WlanSTARef_t wlanSTARef,
        ///< [IN] The WLAN STA reference.
    taf_wlanSta_Mode_t StaMode
        ///< [IN] The WLAN STA mode to set.
)
{
    LE_UNUSED (wlanSTARef);
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.SetMode(wlanSTARef, StaMode);
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
le_result_t taf_wlanSta_GetMode
(
    taf_wlanSta_WlanSTARef_t wlanSTARef,
        ///< [IN] The WLAN STA reference.
    taf_wlanSta_Mode_t* StaModePtr
        ///< [OUT] The WLAN STA mode that is set.
)
{
    LE_UNUSED (wlanSTARef);
    TAF_ERROR_IF_RET_VAL(NULL == StaModePtr, LE_BAD_PARAMETER, "StaModePtr is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.GetMode(wlanSTARef, StaModePtr);
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
le_result_t taf_wlanSta_SetStaticIPConfig
(
    taf_wlanSta_WlanSTARef_t wlanSTARef,
        ///< [IN] The WLAN STA reference.
    const taf_wlanSta_IPConfig_t * LE_NONNULL StaIPConfigPtr
        ///< [IN] IP address to set for static IP address mode.
)
{
    LE_UNUSED (wlanSTARef);
    TAF_ERROR_IF_RET_VAL(NULL == StaIPConfigPtr, LE_BAD_PARAMETER,
                                                     "StaIPConfigPtr is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.SetStaticIPConfig(wlanSTARef, StaIPConfigPtr);
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
le_result_t taf_wlanSta_GetIPConfig
(
    taf_wlanSta_WlanSTARef_t wlanSTARef,
        ///< [IN] The WLAN STA reference.
    taf_wlanSta_IPType_t* StaIPTypePtr,
        ///< [OUT] Dynamic or Static IP address.
    taf_wlanSta_IPConfig_t * StaStaticIPConfigPtr
        ///< [OUT] Details of static IP configuration.
)
{
    LE_UNUSED (wlanSTARef);
    TAF_ERROR_IF_RET_VAL(NULL == StaIPTypePtr, LE_BAD_PARAMETER,"StaIPTypePtr is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.GetIPConfig(wlanSTARef, StaIPTypePtr, StaStaticIPConfigPtr);
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
le_result_t taf_wlanSta_GetStatus
(
    taf_wlanSta_WlanSTARef_t wlanSTARef,
        ///< [IN] The WLAN STA reference.
    taf_wlanSta_State_t* StaSatePtr,
        ///< [OUT] Station state.
    char* IntfName,
        ///< [OUT] Assocaited host interface name.
    size_t IntfNameSize,
        ///< [IN]
    char* IPv4Address,
        ///< [OUT] Assocaited IPv4 address.
    size_t IPv4AddressSize,
        ///< [IN]
    char* IPv6Address,
        ///< [OUT] Assocaited IPv6 address.
    size_t IPv6AddressSize,
        ///< [IN]
    char* MACAddress,
        ///< [OUT] Assocaited MAC address.
    size_t MACAddressSize
        ///< [IN]
)
{
    LE_UNUSED (wlanSTARef);
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.GetStatus(wlanSTARef, StaSatePtr, IntfName, IntfNameSize,
                               IPv4Address, IPv4AddressSize,
                               IPv6Address, IPv6AddressSize,
                               MACAddress, MACAddressSize);
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
taf_wlanSta_WlanSTARef_t taf_wlanSta_GetWlanSTA
(
    taf_wlan_STAid_t STAid,
        ///< [IN] STA identifier
    const char* LE_NONNULL STAIntfName
        ///< [IN] AP assocaited host interface name.
)
{
    TAF_ERROR_IF_RET_VAL(NULL == STAIntfName, NULL,"STAIntfName is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.GetWlanSTA (STAid,STAIntfName);
}
