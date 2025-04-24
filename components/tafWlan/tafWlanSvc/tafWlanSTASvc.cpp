/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_wlanSta_Event'
 */
//--------------------------------------------------------------------------------------------------
taf_wlanSta_EventHandlerRef_t taf_wlanSta_AddEventHandler(
    taf_wlanSta_WlanSTARef_t wlanSTARef,
    ///< [IN] The WLAN STA reference.
    taf_wlanSta_HandlerFunc_t handlerPtr,
    ///< [IN]
    void *contextPtr
    ///< [IN]
)
{
    TAF_ERROR_IF_RET_VAL(NULL == wlanSTARef, NULL, "wlanSTARef is NULL!");
    TAF_ERROR_IF_RET_VAL(NULL == handlerPtr, NULL, "handlerPtr is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.AddEventHandler(wlanSTARef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_wlanSta_Event'
 */
//--------------------------------------------------------------------------------------------------
void taf_wlanSta_RemoveEventHandler(
    taf_wlanSta_EventHandlerRef_t handlerRef
    ///< [IN]
)
{
    TAF_ERROR_IF_RET_NIL(NULL == handlerRef, "handlerRef is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.RemoveEventHandler(handlerRef);
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
le_result_t taf_wlanSta_SetIPConfig
(
    taf_wlanSta_WlanSTARef_t wlanSTARef,
        ///< [IN] The WLAN STA reference.
    taf_wlanSta_IPType_t StaIPType,
        ///< [IN] Dynamic or Static IP address.
    const taf_wlanSta_IPConfig_t * LE_NONNULL StaIPConfigPtr
        ///< [IN] IP address to set for static IP address mode.
)
{
    TAF_ERROR_IF_RET_VAL(NULL == StaIPConfigPtr, LE_BAD_PARAMETER,
                                                     "StaIPConfigPtr is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.SetIPConfig(wlanSTARef, StaIPType, StaIPConfigPtr);
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
    TAF_ERROR_IF_RET_VAL(NULL == wlanSTARef, LE_BAD_PARAMETER, "wlanSTARef is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.GetStatus(wlanSTARef, StaSatePtr, IntfName, IntfNameSize,
                               IPv4Address, IPv4AddressSize,
                               IPv6Address, IPv6AddressSize,
                               MACAddress, MACAddressSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Scan for available access points. This is an asynchronous operation and will trigger event
 * STATE_SCAN_COMPLETE on completion.
 * Use GetAPScanResults to get the results of the scan.
 * When a scan is started, it will clear the results from earlier scans.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanSta_DoAPScan(
    taf_wlanSta_WlanSTARef_t wlanSTARef
    ///< [IN] The WLAN STA reference.
)
{
    TAF_ERROR_IF_RET_VAL(NULL == wlanSTARef, LE_BAD_PARAMETER, "wlanSTARef is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.DoAPScan(wlanSTARef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the list of scanned access points. This list is updated after every call to DoScan.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanSta_GetAPScanResults(
    taf_wlanSta_WlanSTARef_t wlanSTARef,
    ///< [IN] The WLAN STA reference.
    uint16_t *numAPPtr,
    ///< [OUT] Number of APs found in the scan.
    taf_wlanSta_APInfo_t *ApInfoPtr,
    ///< [OUT] Scanned available AP information.
    size_t *ApInfoSizePtr
    ///< [INOUT]
)
{
    TAF_ERROR_IF_RET_VAL(NULL == wlanSTARef, LE_BAD_PARAMETER, "wlanSTARef is NULL!");
    TAF_ERROR_IF_RET_VAL(NULL == numAPPtr, LE_BAD_PARAMETER, "numAPPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(NULL == ApInfoPtr, LE_BAD_PARAMETER, "ApInfoPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(NULL == ApInfoSizePtr, LE_BAD_PARAMETER, "ApInfoSizePtr is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.GetAPScanResults(wlanSTARef, numAPPtr, ApInfoPtr, ApInfoSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets WPA2 PSK for an Access Point.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanSta_SetWpa2Psk(
    taf_wlanSta_WlanSTARef_t wlanSTARef,
    ///< [IN] The WLAN STA reference.
    const taf_wlanSta_APInfo_t* LE_NONNULL ApInfoPtr,
    ///< [IN] AP information.
    const char* LE_NONNULL psk
    ///< [IN] AP information.
)
{
    TAF_ERROR_IF_RET_VAL(wlanSTARef == nullptr, LE_BAD_PARAMETER, "wlanSTARef is NULL!");
    TAF_ERROR_IF_RET_VAL(ApInfoPtr == nullptr, LE_BAD_PARAMETER, "ApInfoPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(std::string(ApInfoPtr->SSID).empty(),
    LE_BAD_PARAMETER, "Empty SSID passed");
    TAF_ERROR_IF_RET_VAL(std::string(psk).empty(),
        LE_BAD_PARAMETER, "Empty psk passed");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.SetWpa2Psk(wlanSTARef, ApInfoPtr, psk);
}

//--------------------------------------------------------------------------------------------------
/**
 * Connects to an Access Point.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanSta_Connect(
    taf_wlanSta_WlanSTARef_t wlanSTARef,
    ///< [IN] The WLAN STA reference.
    const taf_wlanSta_APInfo_t* LE_NONNULL ApInfoPtr
    ///< [IN] AP information.
)
{
    TAF_ERROR_IF_RET_VAL(wlanSTARef == nullptr, LE_BAD_PARAMETER, "wlanSTARef is NULL!");
    TAF_ERROR_IF_RET_VAL(ApInfoPtr == nullptr, LE_BAD_PARAMETER, "ApInfoPtr is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.APConnect(wlanSTARef, ApInfoPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disconnects from an Access Point.
 *
 * @return
 * - LE_OK -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlanSta_Disconnect(
    taf_wlanSta_WlanSTARef_t wlanSTARef,
    ///< [IN] The WLAN STA reference.
    const taf_wlanSta_APInfo_t* LE_NONNULL ApInfoPtr
    ///< [IN] AP information.
)
{
    TAF_ERROR_IF_RET_VAL(wlanSTARef == nullptr, LE_BAD_PARAMETER, "wlanSTARef is NULL!");
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    return myWlanSta.APDisconnect(wlanSTARef, ApInfoPtr);
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
