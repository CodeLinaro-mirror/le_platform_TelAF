/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanSvc.cpp
 *
 * @brief      Server side interface for TelAF WLAN Device Management Service APIs.
 *
 */


#include "tafWlan.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Add Device State Handler
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerDevStateHandler(void *reportPtr, void *secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == nullptr, "Null ptr(secondLayerHandlerFunc)");

    taf_wlan_DeviceStateHandlerFunc_t clientHandlerFunc =
        (taf_wlan_DeviceStateHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(NULL, *(taf_wlan_DeviceState_t *)reportPtr, le_event_GetContextPtr());
    // Release the pointer that was allocated when the wlanDevStateChangeEvID event was sent.
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_wlan_DeviceState'
 */
//--------------------------------------------------------------------------------------------------
taf_wlan_DeviceStateHandlerRef_t taf_wlan_AddDeviceStateHandler
(
    taf_wlan_WlanRef_t WlanRef,
        ///< The WLAN reference. Reserved for future use.
    taf_wlan_DeviceStateHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    if (handlerPtr == nullptr ) {
        LE_ERROR("Null ptr(handlerPtr)");
        return NULL;
    }

    LE_UNUSED (WlanRef);

    le_event_HandlerRef_t handlerRef;
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    handlerRef = le_event_AddLayeredHandler("DeviceStateHandler", myWlan.GetStateChangeEventID(),
                                            FirstLayerDevStateHandler, (void *)handlerPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);
    return (taf_wlan_DeviceStateHandlerRef_t)(handlerRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_wlan_DeviceState'
 */
//--------------------------------------------------------------------------------------------------
void taf_wlan_RemoveDeviceStateHandler
(
    taf_wlan_DeviceStateHandlerRef_t handlerRef
        ///< [IN]
)
{
    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "handlerRef is NULL!");
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    return;
}
//--------------------------------------------------------------------------------------------------
/**
 * Turn WLAN device ON. This action bring up the respective WLAN host interface.
 *
 * @return
 * - LE_OK            Succeeded.
 * - Appropriate error is returned on failure.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlan_TurnOn
(
    taf_wlan_WlanRef_t WlanRef
        ///< The WLAN reference. Reserved for future use.
)
{
    LE_UNUSED (WlanRef);
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    return myWlan.SetON();
}
//--------------------------------------------------------------------------------------------------
/**
 * Turn WLAN device OFF. This action will remove the respective WLAN host interface.
 *
 * @return
 * - LE_OK            Succeeded.
 * - Appropriate error is returned on failure.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlan_TurnOff
(
    taf_wlan_WlanRef_t WlanRef
        ///< The WLAN reference. Reserved for future use.
)
{
    LE_UNUSED (WlanRef);
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    return myWlan.SetOFF();
}
//--------------------------------------------------------------------------------------------------
/**
 * Get WLAN Device state.
 *
 * @return
 * - LE_OK            Succeeded.
 * - Appropriate error is returned on failure.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlan_GetState
(
    taf_wlan_WlanRef_t WlanRef,
        ///< The WLAN reference. Reserved for future use.
    taf_wlan_DeviceState_t* statePtr
        ///< [OUT] WLAN device state.
)
{
    LE_UNUSED (WlanRef);
    TAF_ERROR_IF_RET_VAL(NULL==statePtr, LE_BAD_PARAMETER, "statePtr is NULL!");
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    return myWlan.GetState(statePtr);
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
le_result_t taf_wlan_SetMode
(
    taf_wlan_WlanRef_t WlanRef,
        ///< The WLAN reference. Reserved for future use.
    taf_wlan_DeviceMode_t wlanMode
        ///< [IN] The WLAN device mode.
)
{
    LE_UNUSED (WlanRef);
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    return myWlan.SetMode(wlanMode);
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
le_result_t taf_wlan_GetMode
(
    taf_wlan_WlanRef_t WlanRef,
        ///< The WLAN reference. Reserved for future use.
    taf_wlan_DeviceMode_t* wlanModePtr
        ///< [OUT] The WLAN device mode.
)
{
    LE_UNUSED (WlanRef);
    TAF_ERROR_IF_RET_VAL(NULL == wlanModePtr,  LE_BAD_PARAMETER, "wlanModePtr is NULL!");
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    return myWlan.GetMode(wlanModePtr);
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
le_result_t taf_wlan_GetIntfInfo
(
    taf_wlan_WlanRef_t WlanRef,
        ///< [IN] The WLAN reference. Reserved for future use.
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
    LE_UNUSED (WlanRef);
    TAF_ERROR_IF_RET_VAL(NULL == APIntfinfoPtr,
                                      LE_BAD_PARAMETER, "APIntfinfoPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(NULL == APIntfinfoSizePtr,
                                      LE_BAD_PARAMETER, "APIntfinfoSizePtr is NULL!");
    TAF_ERROR_IF_RET_VAL(0 == *APIntfinfoSizePtr,
                                      LE_BAD_PARAMETER, "APIntfinfoSizePtr is 0!");
    TAF_ERROR_IF_RET_VAL(NULL == STAIntfinfoPtr,
                                      LE_BAD_PARAMETER, "STAIntfinfoPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(NULL == STAIntfinfoSizePtr,
                                      LE_BAD_PARAMETER, "STAIntfinfoSizePtr is NULL!");
    TAF_ERROR_IF_RET_VAL(0 == *STAIntfinfoSizePtr,
                                      LE_BAD_PARAMETER, "STAIntfinfoSizePtr is 0!");

    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    return myWlan.GetIntfInfo(APIntfinfoPtr,APIntfinfoSizePtr,STAIntfinfoPtr,STAIntfinfoSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the wait time for WLAN 5GHz and 5G N79 bands if enabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlan_SetBandIntWaitTime
(
    taf_wlan_WlanRef_t WlanRef,
        ///< [IN] The WLAN reference. Reserved for future use.
    taf_wlan_BandIntPriority_t band,
        ///< [IN] The band interference configuration.
    uint32_t waitTime
        ///< [IN] The wait time in seconds.
)
{
    LE_UNUSED(WlanRef);
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    return myWlan.SetBandIntWaitTime(band, waitTime);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the wait time for WLAN 5GHz and 5G N79 bands.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlan_GetBandIntWaitTime
(
    taf_wlan_WlanRef_t WlanRef,
        ///< [IN] The WLAN reference. Reserved for future use.
    taf_wlan_BandIntPriority_t band,
        ///< [IN] The band interference configuration.
    uint32_t* waitTimePtr
        ///< [OUT] The wait time in seconds.
)
{
    LE_UNUSED(WlanRef);
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    return myWlan.GetBandIntWaitTime(band, waitTimePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the WLAN 5GHz and N79 5G band interference priority.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlan_SetBandIntPriority
(
    taf_wlan_WlanRef_t WlanRef,
        ///< [IN] The WLAN reference. Reserved for future use.
    taf_wlan_BandIntPriority_t bandPriority
        ///< [IN] The band interference priority.
)
{
    LE_UNUSED(WlanRef);
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    return myWlan.SetBandIntPriority(bandPriority);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the WLAN 5GHz and N79 5G band interference priority.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlan_GetBandIntPriority
(
    taf_wlan_WlanRef_t WlanRef,
        ///< [IN] The WLAN reference. Reserved for future use.
    taf_wlan_BandIntPriority_t* bandPriorityPtr
        ///< [OUT] The band interference priority.
)
{
    LE_UNUSED(WlanRef);
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    return myWlan.GetBandIntPriority(bandPriorityPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the WLAN 5GHz and N79 5G band interference state.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlan_SetBandIntState
(
    taf_wlan_WlanRef_t WlanRef,
        ///< [IN] The WLAN reference. Reserved for future use.
    taf_wlan_BandIntState_t state
        ///< [IN] The band interference configuration state.
)
{
    LE_UNUSED(WlanRef);
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    return myWlan.SetBandIntState(state);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the WLAN 5GHz and N79 5G band interference state.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_wlan_GetBandIntState
(
    taf_wlan_WlanRef_t WlanRef,
        ///< [IN] The WLAN reference. Reserved for future use.
    taf_wlan_BandIntState_t* statePtr
        ///< [OUT] The band interference configuration state.
)
{
    LE_UNUSED (WlanRef);
    auto &myWlan = taf_WlanSvcImpl::GetInstance();
    return myWlan.GetBandIntState(statePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * COMPONENT_INIT
 *
 * @return
 * - LE_OK            Succeeded.
 * - Appropriate error is returned on failure.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("tafWlan COMPONENT_INIT\n");
    auto &myWlan   = taf_WlanSvcImpl::GetInstance();
    myWlan.Init();
    LE_INFO("tafWlan Service init completed.\n");
    auto &myWlanAP = taf_WlanAPSvcImpl::GetInstance();
    myWlanAP.Init();
    LE_INFO("tafWlan AP Service init completed.\n");
    auto &myWlanSTA = taf_WlanSTASvcImpl::GetInstance();
    myWlanSTA.Init();
    LE_INFO("tafWlan STA Service init completed.\n");
}
