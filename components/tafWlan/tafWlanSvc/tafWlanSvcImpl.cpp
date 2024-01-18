/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

using namespace telux::tafsvc;

LE_MEM_DEFINE_STATIC_POOL(DeviceStatusPool, TAF_WLAN_MAX_SESSION_REF,
                          sizeof(taf_wlan_DeviceState_t));

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
    // Unsupported modes
    case TAF_WLAN_MODE_UNSUPPORTED:
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
    else
    {
        // Unsupported mode
        LE_WARN ("Unsupported mode");
        *wlanModePtr = TAF_WLAN_MODE_UNSUPPORTED;
        return LE_UNSUPPORTED;
    }

    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Service initialization function
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSvcImpl::Init(void)
{
    // Initializa relevant variables
    wlanDevMgr = nullptr;

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
    wlanDevMgr->registerListener(wlanListener);

    // Create WLAN state event ID
    wlanDevStateChangeEvID = le_event_CreateIdWithRefCounting("DeviceStateChangeEvent");

    // Create wlan mutex
    wlanMutexRef =  le_mutex_CreateRecursive("WlanMutex");
    // Create mem pool for state change event reporting.
    DeviceStatusPool = le_mem_InitStaticPool( DeviceStatusPool, TAF_WLAN_MAX_SESSION_REF,
                                              sizeof(taf_wlan_DeviceState_t));
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

    devStatePtr = (taf_wlan_DeviceState_t *)le_mem_ForceAlloc(DeviceStatusPool);
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
        devStatePtr = (taf_wlan_DeviceState_t *)le_mem_ForceAlloc(DeviceStatusPool);
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