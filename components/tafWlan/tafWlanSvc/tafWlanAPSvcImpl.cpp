/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
le_result_t taf_WlanAPSvcImpl::Start(void)
{
    if (nullptr == wlanAPMgr)
    {
        LE_WARN ("WLAN AP Manager not initialized");
        return LE_FAULT;
    }
    telux::common::ErrorCode errCode =
            wlanAPMgr->manageApService(wlanAPID, telux::wlan::ServiceOperation::START);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN AP Start failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    LE_INFO ("WLAN AP Start success");
    return LE_OK;
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
le_result_t taf_WlanAPSvcImpl::Stop(void)
{
    if (nullptr == wlanAPMgr)
    {
        LE_WARN ("WLAN AP Manager not initialized");
        return LE_FAULT;
    }
    telux::common::ErrorCode errCode =
            wlanAPMgr->manageApService(wlanAPID, telux::wlan::ServiceOperation::STOP);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN AP Stop failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    LE_INFO ("WLAN AP Stop success");
    return LE_OK;
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
le_result_t taf_WlanAPSvcImpl::Restart ( void )
{
    if (nullptr == wlanAPMgr)
    {
        LE_WARN("WLAN AP Manager not initialized");
        return LE_FAULT;
    }
    telux::common::ErrorCode errCode =
        wlanAPMgr->manageApService(wlanAPID, telux::wlan::ServiceOperation::RESTART);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN AP Restart failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    LE_INFO("WLAN AP restart success");
    return LE_OK;
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
le_result_t taf_WlanAPSvcImpl::SetConfig ( const taf_wlanAp_WlanAPConfig_t* wlanAPConfigPtr )
{
    std::vector<telux::wlan::ApConfig> config;
    telux::wlan::ApConfig configSet;
    if (nullptr == wlanAPMgr)
    {
        LE_WARN("WLAN AP Manager not initialized");
        return LE_FAULT;
    }

    // Get the current configuration and set only the values that are required.
    telux::common::ErrorCode errCode = wlanAPMgr->getConfig(config);
    if (errCode == telux::common::ErrorCode::SUCCESS)
    {
        for (auto &cfg : config)
        {
            LE_DEBUG("cfg ------------------------------------------");
            if (wlanAPID == cfg.id)
            {
                configSet = cfg;
                for (auto &netCfg : configSet.network)
                {
                    LE_DEBUG("netcfg ------------------------------------------");
                    netCfg.isVisible = wlanAPConfigPtr->bSSIDVisible;
                    netCfg.ssid = wlanAPConfigPtr->SSID;
                }
            }
        }
    }
    else
    {
        LE_WARN("Unable to get configuration: %d", (int)errCode);
        return LE_FAULT;
    }

    //Set the AP configuration
    errCode = wlanAPMgr->setConfig(configSet);
    if (errCode != telux::common::ErrorCode::SUCCESS)
    {
        LE_WARN("Unable to get configuration: %d", (int)errCode);
        return LE_FAULT;
    }
    return LE_OK;
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
le_result_t taf_WlanAPSvcImpl::GetConfig ( taf_wlanAp_WlanAPConfig_t* wlanAPConfigPtr )
{
    if (nullptr == wlanAPMgr)
    {
        LE_WARN("WLAN AP Manager not initialized");
        return LE_FAULT;
    }
    std::vector<telux::wlan::ApConfig> config;
    telux::common::ErrorCode errCode = wlanAPMgr->getConfig(config);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN AP GetConfig failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    for (auto &cfg : config)
    {
        LE_DEBUG ("------------------------------------------");
        LE_DEBUG ("AP Id: %d", (int)cfg.id);
        if (wlanAPID == cfg.id)
        {
            LE_DEBUG("AP Venue Type : %d", (int)cfg.venue.type);
            LE_DEBUG("AP Venue Group: %d", (int)cfg.venue.group);
            int netCount = 1;
            le_result_t ret;
            for (auto &netCfg : cfg.network)
            {
                LE_DEBUG("ApNetConfig %d Details: ", netCount++);
                LE_DEBUG("AP Type: %d", (int)netCfg.info.apType);
                LE_DEBUG("AP Radio: %d", (int)netCfg.info.apRadio);
                LE_DEBUG("AP SSID: %s", netCfg.ssid.c_str());
                ret = le_utf8_Copy(wlanAPConfigPtr->SSID, netCfg.ssid.c_str(),
                                              TAF_WLAN_MAX_SSID_LENGTH+1,NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("SSID copy error: %d", ret);
                }
                LE_DEBUG("AP is Visible: %d", (int)netCfg.isVisible);
                wlanAPConfigPtr->bSSIDVisible = netCfg.isVisible;
                LE_DEBUG("netCfg.elementInfoConfig");
                LE_DEBUG("AP Interworking: %d", (int)netCfg.interworking);
                LE_DEBUG("AP Security: ");
                LE_DEBUG("    Mode: %d", (int)netCfg.apSecurity.mode);
                LE_DEBUG("    Authorization: %d", (int)netCfg.apSecurity.auth);
                LE_DEBUG("    Encryption: %d", (int)netCfg.apSecurity.encrypt);
                LE_DEBUG("AP Passphrase: %s", netCfg.passPhrase.c_str());
            }
        }
    }
    return LE_OK;
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
le_result_t
taf_WlanAPSvcImpl::SetSecurityConfig ( const taf_wlanAp_WlanAPSecurityConfig_t* wlanAPSecCfgPtr )
{
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;
    telux::wlan::ApSecurity secConfig = {};

    if (nullptr == wlanAPMgr)
    {
        LE_WARN("WLAN AP Manager not initialized");
        return LE_FAULT;
    }

    // Set value only if user wants to update the value.
    if (TAF_WLAN_SEC_MODE_UNKNOWN != wlanAPSecCfgPtr->SecMode)
    {
        secConfig.mode = taf_WlanHelper::SecModeToTelux(wlanAPSecCfgPtr->SecMode);
    }
    // Set value only if user wants to update the value.
    if (TAF_WLAN_SEC_AUTH_METHOD_UNKNOWN != wlanAPSecCfgPtr->SecAuthMethod)
    {
        secConfig.auth = taf_WlanHelper::SecAuthToTelux(wlanAPSecCfgPtr->SecAuthMethod);
    }
    // Set value only if user wants to update the value.
    if (TAF_WLAN_SEC_ENCRYPT_METHOD_UNKNOWN != wlanAPSecCfgPtr->SecEncryptMethod)
    {
        secConfig.encrypt = taf_WlanHelper::SecEncryptToTelux(wlanAPSecCfgPtr->SecEncryptMethod);
    }

    errCode = wlanAPMgr->setSecurityConfig(wlanAPID, secConfig);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN AP SetSecurityConfig failed with error : %d", (int)errCode);
        return LE_FAULT;
    }

    errCode = wlanAPMgr->setPassPhrase(wlanAPID, wlanAPSecCfgPtr->PassPhrase);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN AP SetSecurityConfig(passpharase) failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    return LE_OK;
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
le_result_t
taf_WlanAPSvcImpl::GetSecurityConfig  ( taf_wlanAp_WlanAPSecurityConfig_t* wlanAPSecCfgPtr )
{
    if (nullptr == wlanAPMgr)
    {
        LE_WARN("WLAN AP Manager not initialized");
        return LE_FAULT;
    }
    std::vector<telux::wlan::ApConfig> config;
    telux::common::ErrorCode errCode = wlanAPMgr->getConfig(config);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN AP GetConfig failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    for (auto &cfg : config)
    {
        LE_DEBUG("------------------------------------------");
        LE_DEBUG("AP Id: %d", (int)cfg.id);
        if (wlanAPID == cfg.id)
        {
            LE_DEBUG("AP Venue Type : %d", (int)cfg.venue.type);
            LE_DEBUG("AP Venue Group: %d", (int)cfg.venue.group);
            int netCount = 1;
            le_result_t ret;
            for (auto &netCfg : cfg.network)
            {
                LE_DEBUG("ApNetConfig %d Details: ", netCount++);
                LE_DEBUG("AP Type: %d", (int)netCfg.info.apType);
                LE_DEBUG("AP Radio: %d", (int)netCfg.info.apRadio);
                LE_DEBUG("AP SSID: %s", netCfg.ssid.c_str());

                LE_DEBUG("AP is Visible: %d", (int)netCfg.isVisible);
                LE_DEBUG("netCfg.elementInfoConfig");
                LE_DEBUG("AP Interworking: %d", (int)netCfg.interworking);
                LE_DEBUG("AP Security: ");
                LE_DEBUG("    Mode: %d", (int)netCfg.apSecurity.mode);
                wlanAPSecCfgPtr->SecMode = taf_WlanHelper::SecModeToTAF(netCfg.apSecurity.mode);
                LE_DEBUG("    Authorization: %d", (int)netCfg.apSecurity.auth);
                wlanAPSecCfgPtr->SecAuthMethod =
                                       taf_WlanHelper::SecAuthToTAF(netCfg.apSecurity.auth);
                LE_DEBUG("    Encryption: %d", (int)netCfg.apSecurity.encrypt);
                wlanAPSecCfgPtr->SecEncryptMethod =
                                       taf_WlanHelper::SecEncryptToTAF(netCfg.apSecurity.encrypt);
                LE_DEBUG("AP Passphrase: %s", netCfg.passPhrase.c_str());
                ret = le_utf8_Copy(wlanAPSecCfgPtr->PassPhrase, netCfg.passPhrase.c_str(),
                                               TAF_WLAN_MAX_PASSPHRASE_LENGTH+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("PassPhrase copy error: %d", ret);
                }
            }
        }
    }
    return LE_OK;
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
le_result_t
taf_WlanAPSvcImpl::GetStatus ( taf_wlanAp_WlanAPStatus_t* wlanAPStatusPtr )
{
    if (nullptr == wlanAPMgr)
    {
        LE_WARN("WLAN AP Manager not initialized");
        return LE_FAULT;
    }
    std::vector<telux::wlan::ApStatus> status;
    telux::common::ErrorCode errCode = wlanAPMgr->getStatus(status);

    // Initialize status to false
    wlanAPStatusPtr->bEnabled = false;
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN AP GetStatus failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    if (status.size()>0)
    {
        le_result_t ret;
        for (auto &ap : status)
        {
            LE_DEBUG("------------------------------------------");
            LE_DEBUG("AP Info");
            if (wlanAPID == ap.id)
            {
                LE_DEBUG ("Id                 : %d", (int)ap.id);
                LE_DEBUG ("Network Interface  : %s", ap.name.c_str());
                LE_DEBUG ("IPv4 Addr          : %s", ap.ipv4Address.c_str());
                LE_DEBUG ("MAC Addr           : %s", ap.macAddress.c_str());
                wlanAPStatusPtr->bEnabled = true;
                ret = le_utf8_Copy (wlanAPStatusPtr->IntfName, ap.name.c_str(),
                                                            TAF_NET_INTERFACE_NAME_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("IntfName copy error: %d", ret);
                }
                ret = le_utf8_Copy(wlanAPStatusPtr->IPv4Address, ap.ipv4Address.c_str(),
                                   TAF_NET_IPV4_ADDR_MAX_LEN + 1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("IPv4Address copy error: %d", ret);
                }
                ret = le_utf8_Copy(wlanAPStatusPtr->MACAddress, ap.macAddress.c_str(),
                                   TAF_NET_MAC_ADDR_MAX_LEN + 1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("MACAddress copy error: %d", ret);
                }
                for (auto &netInfo : ap.network)
                {
                    LE_DEBUG ("SSID       : %s", netInfo.ssid.c_str());
                    LE_DEBUG ("Radio Type : %d",(int)netInfo.info.apRadio);
                    LE_DEBUG ("AP Type    : %d",(int)netInfo.info.apType);
                }
            }
        }
    }
    return LE_OK;
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
le_result_t taf_WlanAPSvcImpl::GetConnectedDevices(
    uint16_t *numDevicesPtr,
    ///< [OUT] Nmber of devices connected to the AP.
    taf_wlanAp_WlanAPConnectedDeviceInfo_t *DevInfoPtr,
    ///< [OUT] Connected device information.
    size_t *DevInfoSizePtr
    ///< [INOUT]
)
{
    le_result_t ret;
    if (nullptr == wlanAPMgr)
    {
        LE_WARN("WLAN AP Manager not initialized");
        return LE_FAULT;
    }

    std::vector<telux::wlan::DeviceInfo> deviceList;
    std::vector<telux::wlan::DeviceInfo> APSepcificDevList;

    telux::common::ErrorCode errCode = wlanAPMgr->getConnectedDevices(deviceList);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN AP GetConnectedDevices failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    for (auto &dev : deviceList)
    {
        LE_DEBUG("------------------------------------------");
        LE_DEBUG("Associated AP          : %d", (int)dev.id);
        LE_DEBUG("Device Name            : %s", dev.name.c_str());
        LE_DEBUG("Device MAC Address     : %s", dev.macAddress.c_str());
        LE_DEBUG("Device IPv4 Address    : %s", dev.ipv4Address.c_str());
        if (wlanAPID == dev.id)
        {
            // Place this device in the AP specific vector
            APSepcificDevList.push_back(dev);
        }
    }

    // Set the number of devices connected to the AP.
    *numDevicesPtr = APSepcificDevList.size();
    LE_INFO ("Number of devices connected: %d", *numDevicesPtr);

    // Check if the device info array that was passed can hold all the devices' information.
    if (*DevInfoSizePtr < *numDevicesPtr)
    {
        // The size of the available members in the device info array is less
        // Keep the value same and warn user.
        LE_WARN("DevInfo array size is less than the number of connected devices");
    }
    else
    {
        // Update the DevInfoSizePtr
        *DevInfoSizePtr = *numDevicesPtr;
    }

    // Set the max limit for the number of connected devices TAF_WLAN_AP_MAX_CONNECTED_DEVICES
    if (*DevInfoSizePtr > TAF_WLANAP_MAX_CONNECTED_DEVICES)
    {
        LE_WARN("Number of connected devices is more thatn the number supported by TelAF");
        *DevInfoSizePtr = TAF_WLANAP_MAX_CONNECTED_DEVICES;
    }

    // Populate device information up to to DevInfoSize elements.
    for (int i = 0; i < (int)*DevInfoSizePtr; i++)
    {
        ret = le_utf8_Copy(DevInfoPtr[i].Name, APSepcificDevList[i].name.c_str(),
                        TAF_WLAN_MAX_DEVICE_NAME_LENGTH+1, NULL);
        if (LE_OK != ret) {
            LE_WARN("Device Name copy error: %d", ret);
        }
        ret = le_utf8_Copy(DevInfoPtr[i].MACAddress, APSepcificDevList[i].macAddress.c_str(),
                        TAF_NET_MAC_ADDR_MAX_LEN+1, NULL);
        if (LE_OK != ret)
        {
            LE_WARN("Device MAC address copy error: %d", ret);
        }
        ret = le_utf8_Copy(DevInfoPtr[i].IPv4Address, APSepcificDevList[i].ipv4Address.c_str(),
                        TAF_NET_IPV4_ADDR_MAX_LEN+1, NULL);
        if (LE_OK != ret)
        {
            LE_WARN("Device IPv4 address copy error: %d", ret);
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Return the instance of taf_wlanAp implementation class.
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
    // Initialize to nullptr
    wlanAPMgr = nullptr;

    auto &wlanFactory = telux::wlan::WlanFactory::getInstance();
    wlanAPMgr = wlanFactory.getApInterfaceManager ();
    if (wlanAPMgr==nullptr)
    {
        // Unable to initialize the WLAN AP subsystem. Stop the service.
        LE_FATAL (" *** Unable to initialize Wlan AP subsystem *** ");
    }

    // Register the Listener class shared object
    wlanAPListener = std::make_shared<taf_WlanAPListener>();
    telux::common::ErrorCode retCode = wlanAPMgr->registerListener(wlanAPListener);
    if (telux::common::ErrorCode::SUCCESS != retCode)
    {
        LE_WARN("WLAN AP registerListener failed: %d", (int)retCode);
    }

    LE_INFO(" *** Wlan AP Initialized *** ");

    return;
}