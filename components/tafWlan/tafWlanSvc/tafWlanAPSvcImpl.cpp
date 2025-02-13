/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <wpa_ctrl.h>
#include "limit.h"

using namespace tafsvc;

// Memory pool for AP contexts
LE_MEM_DEFINE_STATIC_POOL(tafWlanAPCtxPool, TAF_WLAN_MAX_NUM_AP, sizeof(taf_wlan_AP_Ctx_t));

// Memory pool for client connection event for applications
LE_MEM_DEFINE_STATIC_POOL(DeviceCnxEventPool, TAF_WLAN_MAX_SESSION_REF, (sizeof(DeviceCnxEvent_t)));

//--------------------------------------------------------------------------------------------------
/**
 * Returns the context for a given WLAN AP ID.
 *
 * @return
 * - WLAN AP context on success, nullptr on failure
 */
//--------------------------------------------------------------------------------------------------
taf_wlan_AP_Ctx_t* taf_WlanAPSvcImpl::GetWlanAPCtx
(
    taf_wlan_APid_t apID                ///< [IN] AP identifier
)
{
    le_dls_Link_t* linkPtr = nullptr;
    le_mutex_Lock(APCtxMutex);
    linkPtr = le_dls_Peek(&APCtxList);
    while (linkPtr)
    {
        taf_wlan_AP_Ctx_t* ctxPtr = CONTAINER_OF(linkPtr, taf_wlan_AP_Ctx_t, link);
        linkPtr = le_dls_PeekNext(&APCtxList, linkPtr);
        if (ctxPtr->id == apID)
        {
            le_mutex_Unlock(APCtxMutex);
            return ctxPtr;
        }
    }
    le_mutex_Unlock(APCtxMutex);
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns the WLAN AP reference.
 *
 * @return
 * - WLAN AP reference on success, nullptr on failure
 */
//--------------------------------------------------------------------------------------------------
taf_wlanAp_WlanAPRef_t taf_WlanAPSvcImpl::GetWlanAPReference
(
    taf_wlan_APid_t apID,               ///< [IN] AP identifier
    const char *LE_NONNULL apIntfName   ///< [IN] AP assocaited host interface name.
)
{
    taf_wlan_AP_Ctx_t* ctxPtr = GetWlanAPCtx(apID);
    if (ctxPtr == nullptr)
    {
        LE_ERROR ("Unable to context for AP ID: %d", apID);
        return nullptr;
    }

    if (strlen(apIntfName) == 0)
    {
        LE_ERROR ("apIntfName is invalid");
        return nullptr;
    }

    le_result_t ret = le_utf8_Copy(ctxPtr->interfaceName, apIntfName,
        TAF_NET_INTERFACE_NAME_MAX_LEN, nullptr);
    if (ret != LE_OK)
    {
        LE_WARN("Interface name copy error: %d", ret);
    }

    // Create the wpa ctrl thread for this AP, but don't start it
    // This should be done only once, when the reference is provided for a client the first time.
    if (nullptr == ctxPtr->APWpaCtrlThreadRef)
    {
        LE_INFO("Create WPA CTRL thread");
        // wpa ctrl thread is not yet created, create it.
        ctxPtr->APWpaCtrlThreadRef = le_thread_Create("wpa_ctrl", APWpaCtrlThreadHdlr, ctxPtr);
        if (nullptr == ctxPtr->APWpaCtrlThreadRef)
        {
            LE_ERROR("Failed to create WPA Ctrl Thread");
            return nullptr;
        }
    }
    else
    {
        LE_INFO("WPA CTRL thread already created");
    }

    LE_INFO("AP ID: %d, Interface name: %s", ctxPtr->id, ctxPtr->interfaceName);
    return ctxPtr->wlanAPRef;
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
le_result_t taf_WlanAPSvcImpl::Start(taf_wlanAp_WlanAPRef_t apRef)
{
    if (nullptr == wlanAPMgr)
    {
        LE_WARN ("WLAN AP Manager not initialized");
        return LE_FAULT;
    }

    taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    telux::wlan::Id id = taf_WlanHelper::TAFAPidtoTeluxId(ctxPtr->id);
    telux::common::ErrorCode errCode =
            wlanAPMgr->manageApService(id, telux::wlan::ServiceOperation::START);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN AP Start failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    LE_INFO("WLAN AP Start success");
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
le_result_t taf_WlanAPSvcImpl::Stop(taf_wlanAp_WlanAPRef_t apRef)
{
    if (nullptr == wlanAPMgr)
    {
        LE_WARN ("WLAN AP Manager not initialized");
        return LE_FAULT;
    }

    taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    telux::wlan::Id id = taf_WlanHelper::TAFAPidtoTeluxId(ctxPtr->id);

    telux::common::ErrorCode errCode =
            wlanAPMgr->manageApService(id, telux::wlan::ServiceOperation::STOP);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN AP Stop failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    LE_INFO("WLAN AP Stop success");
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
le_result_t taf_WlanAPSvcImpl::Restart(taf_wlanAp_WlanAPRef_t apRef)
{
    if (nullptr == wlanAPMgr)
    {
        LE_WARN("WLAN AP Manager not initialized");
        return LE_FAULT;
    }

    taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    telux::wlan::Id id = taf_WlanHelper::TAFAPidtoTeluxId(ctxPtr->id);
    telux::common::ErrorCode errCode =
        wlanAPMgr->manageApService(id, telux::wlan::ServiceOperation::RESTART);
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
le_result_t taf_WlanAPSvcImpl::SetConfig
(
    taf_wlanAp_WlanAPRef_t apRef,
    const taf_wlanAp_WlanAPConfig_t* wlanAPConfigPtr
)
{
    std::vector<telux::wlan::ApConfig> config;
    telux::wlan::ApConfig configSet;
    if (nullptr == wlanAPMgr)
    {
        LE_WARN("WLAN AP Manager not initialized");
        return LE_FAULT;
    }

    taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    telux::wlan::Id id = taf_WlanHelper::TAFAPidtoTeluxId(ctxPtr->id);

    // Get the current configuration and set only the values that are required.
    telux::common::ErrorCode errCode = wlanAPMgr->getConfig(config);
    if (errCode == telux::common::ErrorCode::SUCCESS)
    {
        for (auto &cfg : config)
        {
            LE_DEBUG("cfg ------------------------------------------");
            if (id == cfg.id)
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
le_result_t taf_WlanAPSvcImpl::GetConfig
(
    taf_wlanAp_WlanAPRef_t apRef,
    taf_wlanAp_WlanAPConfig_t* wlanAPConfigPtr
)
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

    taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    telux::wlan::Id id = taf_WlanHelper::TAFAPidtoTeluxId(ctxPtr->id);

    for (auto &cfg : config)
    {
        LE_DEBUG("------------------------------------------");
        LE_DEBUG("AP Id: %d", (int)cfg.id);
        if (id == cfg.id)
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
                        TAF_WLAN_MAX_SSID_LENGTH + 1, nullptr);
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
le_result_t taf_WlanAPSvcImpl::SetSecurityConfig
(
    taf_wlanAp_WlanAPRef_t apRef,
    const taf_wlanAp_WlanAPSecurityConfig_t* wlanAPSecCfgPtr
)
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

    taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    telux::wlan::Id id = taf_WlanHelper::TAFAPidtoTeluxId(ctxPtr->id);

    errCode = wlanAPMgr->setSecurityConfig(id, secConfig);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN AP SetSecurityConfig failed with error : %d", (int)errCode);
        return LE_FAULT;
    }

    errCode = wlanAPMgr->setPassPhrase(id, wlanAPSecCfgPtr->PassPhrase);
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
le_result_t taf_WlanAPSvcImpl::GetSecurityConfig
(
    taf_wlanAp_WlanAPRef_t apRef,
    taf_wlanAp_WlanAPSecurityConfig_t* wlanAPSecCfgPtr
)
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

    taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    telux::wlan::Id id = taf_WlanHelper::TAFAPidtoTeluxId(ctxPtr->id);

    for (auto &cfg : config)
    {
        LE_DEBUG("------------------------------------------");
        LE_DEBUG("AP Id: %d", (int)cfg.id);
        if (id == cfg.id)
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
                                               TAF_WLAN_MAX_PASSPHRASE_LENGTH + 1, nullptr);
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
le_result_t taf_WlanAPSvcImpl::GetStatus
(
    taf_wlanAp_WlanAPRef_t apRef,
    taf_wlanAp_WlanAPStatus_t* wlanAPStatusPtr
)
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

    taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    telux::wlan::Id id = taf_WlanHelper::TAFAPidtoTeluxId(ctxPtr->id);

    if (status.size() > 0)
    {
        le_result_t ret;
        for (auto &ap : status)
        {
            LE_DEBUG("------------------------------------------");
            LE_DEBUG("AP Info");
            if (id == ap.id)
            {
                LE_DEBUG ("Id                 : %d", (int)ap.id);
                LE_DEBUG ("Network Interface  : %s", ap.name.c_str());
                LE_DEBUG ("IPv4 Addr          : %s", ap.ipv4Address.c_str());
                LE_DEBUG ("MAC Addr           : %s", ap.macAddress.c_str());
                wlanAPStatusPtr->bEnabled = true;
                ret = le_utf8_Copy (wlanAPStatusPtr->IntfName, ap.name.c_str(),
                        TAF_NET_INTERFACE_NAME_MAX_LEN + 1, nullptr);
                if (LE_OK != ret)
                {
                    LE_WARN("IntfName copy error: %d", ret);
                }
                ret = le_utf8_Copy(wlanAPStatusPtr->IPv4Address, ap.ipv4Address.c_str(),
                                   TAF_NET_IPV4_ADDR_MAX_LEN + 1, nullptr);
                if (LE_OK != ret)
                {
                    LE_WARN("IPv4Address copy error: %d", ret);
                }
                ret = le_utf8_Copy(wlanAPStatusPtr->MACAddress, ap.macAddress.c_str(),
                                   TAF_NET_MAC_ADDR_MAX_LEN + 1, nullptr);
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
le_result_t taf_WlanAPSvcImpl::GetConnectedDevices
(
    taf_wlanAp_WlanAPRef_t apRef,
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
        LE_WARN("wlanAPMgr->getConnectedDevices failed with error : %d", (int)errCode);
        return LE_FAULT;
    }

    taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ctxPtr == nullptr, LE_FAULT, "Unable to find context");

    telux::wlan::Id id = taf_WlanHelper::TAFAPidtoTeluxId(ctxPtr->id);

    for (auto &dev : deviceList)
    {
        LE_DEBUG("------------------------------------------");
        LE_DEBUG("Associated AP          : %d", (int)dev.id);
        LE_DEBUG("Device Name            : %s", dev.name.c_str());
        LE_DEBUG("Device MAC Address     : %s", dev.macAddress.c_str());
        LE_DEBUG("Device IPv4 Address    : %s", dev.ipv4Address.c_str());
        if (id == dev.id)
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

    // Set the max limit for the number of connected devices TAF_WLANAP_MAX_CONNECTED_DEVICES
    if (*DevInfoSizePtr > TAF_WLANAP_MAX_CONNECTED_DEVICES)
    {
        LE_WARN("Number of connected clients is more than the number supported by TelAF");
        *DevInfoSizePtr = TAF_WLANAP_MAX_CONNECTED_DEVICES;
    }

    // Populate device information up to to DevInfoSize elements.
    for (int i = 0; i < (int)*DevInfoSizePtr; ++i)
    {
        ret = le_utf8_Copy(DevInfoPtr[i].Name, APSepcificDevList[i].name.c_str(),
                        TAF_WLAN_MAX_DEVICE_NAME_LENGTH + 1, nullptr);
        if (LE_OK != ret)
        {
            LE_WARN("Device Name copy error: %d", ret);
        }
        ret = le_utf8_Copy(DevInfoPtr[i].MACAddress, APSepcificDevList[i].macAddress.c_str(),
                        TAF_NET_MAC_ADDR_MAX_LEN + 1, nullptr);
        if (LE_OK != ret)
        {
            LE_WARN("Device MAC address copy error: %d", ret);
        }
        ret = le_utf8_Copy(DevInfoPtr[i].IPv4Address, APSepcificDevList[i].ipv4Address.c_str(),
                        TAF_NET_IPV4_ADDR_MAX_LEN + 1, nullptr);
        if (LE_OK != ret)
        {
            LE_WARN("Device IPv4 address copy error: %d", ret);
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * First layer device connection event handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPSvcImpl::DeviceCnxFirstLayerEventHandler(void *reportPtr,
                                                        void *secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "reportPtr is NULL!");
    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == NULL,"secondLayerHandlerFunc is NULL!");

    DeviceCnxEvent_t *DeviceCnxEventPtr = (DeviceCnxEvent_t *)reportPtr;

    taf_wlanAp_DeviceConnectionEventHandlerFunc_t deviceHandlerFunc =
                  (taf_wlanAp_DeviceConnectionEventHandlerFunc_t)secondLayerHandlerFunc;

    deviceHandlerFunc(DeviceCnxEventPtr->apRef,
                      DeviceCnxEventPtr->event,
                      DeviceCnxEventPtr->MACAddress,
                      le_event_GetContextPtr());
    // Release the pointer that was allocated when the event was sent.
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Report device connection events for registered clients
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPSvcImpl::ReportDevCnxEvents(
    taf_wlan_AP_Ctx_t *ApctxPtr,              // AP Context
    taf_wlanAp_DeviceConnectionEvent_t event, // Event type.
    const char *MACAddressStr                 // MAC address of the device
)
{
    DeviceCnxEvent_t *devCnxEvtPtr = (DeviceCnxEvent_t *)le_mem_ForceAlloc(DeviceCnxEventPoolRef);
    TAF_ERROR_IF_RET_NIL(devCnxEvtPtr == NULL, "devCnxEvtPtr is NULL!");
    devCnxEvtPtr->apRef = ApctxPtr->wlanAPRef;
    devCnxEvtPtr->event = event;
    le_result_t result = le_utf8_Copy(devCnxEvtPtr->MACAddress, MACAddressStr,
                                      TAF_NET_MAC_ADDR_MAX_LEN + 1, nullptr);
    if (LE_OK != result)
    {
        LE_WARN("Device MAC address copy error: %d", result);
    }
    le_event_ReportWithRefCounting(ApctxPtr->DeviceConnectionEvent, (void *)devCnxEvtPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * AP wpa ctrl thread destructor. This is called before the thread terminates.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPSvcImpl::APWpaCtrlThreadDestructor(void *context)
{
    // Close the WPA ctrl handle
    wpa_ctrl_close((struct wpa_ctrl *)context);
    LE_INFO("WPA CTRL closed");
}

//--------------------------------------------------------------------------------------------------
/**
 * AP wpa ctrl thread handler. This thread is used to receive events.
 */
//--------------------------------------------------------------------------------------------------
void *taf_WlanAPSvcImpl::APWpaCtrlThreadHdlr(void *context)
{
    struct wpa_ctrl *ctrl;
    taf_wlan_AP_Ctx_t *ApctxPtr = (taf_wlan_AP_Ctx_t *)context;
    char rsp[AP_WPA_CTRL_RSP_BUF_LEN] = {0};
    size_t len = AP_WPA_CTRL_RSP_BUF_LEN;
    std::string hostapd_path(HOSTAPD_LOCATION_PATH);
    hostapd_path = hostapd_path + ApctxPtr->interfaceName;
    LE_INFO("hostapd path: %s", hostapd_path.c_str());

    ctrl = wpa_ctrl_open(hostapd_path.c_str());
    if (nullptr == ctrl)
    {
        LE_ERROR("Failed to open control interface");
        return nullptr;
    }

    int ret = wpa_ctrl_attach(ctrl);
    if (ret != 0)
    {
        LE_ERROR("Failed to attach to control interface");
        wpa_ctrl_close(ctrl);
        return nullptr;
    }

    // Add destructor and pass WPA CTRL handle to it.
    le_thread_AddDestructor(APWpaCtrlThreadDestructor, (void *)ctrl);

    while(true)
    {
        ret = wpa_ctrl_pending(ctrl);
        if (1 == ret)
        {
            memset(rsp, 0, AP_WPA_CTRL_RSP_BUF_LEN);
            len = AP_WPA_CTRL_RSP_BUF_LEN;
            // Event pending;
            ret = wpa_ctrl_recv(ctrl, rsp, &len);
            if (ret < 0)
            {
                LE_WARN("wpa_ctrl_recv failed");
                break;
            }
            LE_DEBUG("Received response Len: %zu", len);
            LE_DEBUG("Received response    : %s", rsp);
            // Split the received buffer using space as delimiter
            std::vector<std::string> ind = taf_WlanHelper::StrSplit(rsp, ' ');
            LE_DEBUG("Received Event: %s", ind[0].c_str());
            if (ind[0].find(taf_WlanHelper::StrTrimEndSpace(AP_STA_CONNECTED)) !=
                                                                                std::string::npos)
            {
                LE_INFO("AP_STA_CONNECTED. MAC: %s", ind[1].c_str());
                auto &wlan = taf_WlanAPSvcImpl::GetInstance();
                wlan.ReportDevCnxEvents(ApctxPtr, TAF_WLANAP_DEVICE_CONNECTED, ind[1].c_str());
            }
            if (ind[0].find(taf_WlanHelper::StrTrimEndSpace(AP_STA_DISCONNECTED)) !=
                                                                                std::string::npos)
            {
                LE_INFO("AP_STA_DISCONNECTED. MAC: %s", ind[1].c_str());
                auto &wlan = taf_WlanAPSvcImpl::GetInstance();
                wlan.ReportDevCnxEvents(ApctxPtr, TAF_WLANAP_DEVICE_DISCONNECTED, ind[1].c_str());
            }
        }
    }
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Implementation function for taf_wlanAp_AddClientConnectionEventHandler
 */
//--------------------------------------------------------------------------------------------------
taf_wlanAp_DeviceConnectionEventHandlerRef_t
taf_WlanAPSvcImpl::AddDeviceConnectionEventHandler(taf_wlanAp_WlanAPRef_t apRef,
                                        taf_wlanAp_DeviceConnectionEventHandlerFunc_t handlerPtr,
                                        void *contextPtr)
{
    if (nullptr == wlanAPMgr)
    {
        LE_WARN("WLAN AP Manager not initialized");
        return nullptr;
    }

    taf_wlan_AP_Ctx_t *ApctxPtr = (taf_wlan_AP_Ctx_t *)le_ref_Lookup(APRefMap, (void *)apRef);
    TAF_ERROR_IF_RET_VAL(ApctxPtr == nullptr, nullptr, "Unable to find context");

    // Check if the AP is on
    taf_wlanAp_WlanAPStatus_t wlanAPStatus;
    le_result_t result = GetStatus(apRef, &wlanAPStatus);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, nullptr, "Unable to get AP status");

    TAF_ERROR_IF_RET_VAL(!(wlanAPStatus.bEnabled), nullptr, "AP is not active");

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
        "DeviceConnectionEventHandler",
        ApctxPtr->DeviceConnectionEvent,
        DeviceCnxFirstLayerEventHandler,
        (void *)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    // Insert this client into the list of handlers that are interested in receiving
    // device connection events from this AP.
    le_msg_SessionRef_t sessionRef = taf_wlanAp_GetClientSessionRef();
    // Check if this client had requested the event earlier and add it to the clients list.
    if (ApctxPtr->devCnxEvtClients.find(sessionRef) == ApctxPtr->devCnxEvtClients.end())
    {
        // Insert this client to the clients list
        LE_INFO("Insert client: %p", sessionRef);
        ApctxPtr->devCnxEvtClients.insert(sessionRef);
    }
    else
    {
        // This client is already in the list. Skip adding from the clients list
        LE_INFO("Skip client: %p", sessionRef);
    }

    // Check if APWpaCtrlThreadRef is running, if not start it.
    // If the number  of clients that requetsed this event is 1 and if the wpa ctrl thread is not
    // running, start the thread.
    if ((1 == ApctxPtr->devCnxEvtClients.size()) && !(ApctxPtr->isAPWpaCtrlThreadRunning))
    {
        LE_INFO("First client. Start WPA ctrl thread");
        le_thread_Start(ApctxPtr->APWpaCtrlThreadRef);
        ApctxPtr->isAPWpaCtrlThreadRunning = true;
    }

    return (taf_wlanAp_DeviceConnectionEventHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Implementation function for taf_wlanAp_RemoveDeviceConnectionEventHandler
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPSvcImpl::RemoveDeviceConnectionEventHandler(
                                        taf_wlanAp_DeviceConnectionEventHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);

    // Pop this client from the list of handlers that are interested in receiving
    // device connection events from this AP.
    le_msg_SessionRef_t sessionRef = taf_wlanAp_GetClientSessionRef();

    // Find this AP context this client had registered the handler for
    for (int iCount = 1; iCount <= TAF_WLAN_MAX_NUM_AP; iCount++)
    {
        taf_wlan_AP_Ctx_t *ApctxPtr = GetWlanAPCtx(taf_wlan_APid_t(iCount));
        LE_INFO("Checking AP Ctx Id: %d", iCount);
        // Check if this client had requested the event and pop from the clients list.
        if (ApctxPtr->devCnxEvtClients.find(sessionRef) != ApctxPtr->devCnxEvtClients.end())
        {
            // Remove this client from the list
            LE_INFO("Remove client: %p", sessionRef);
            ApctxPtr->devCnxEvtClients.erase(sessionRef);
        }
        else
        {
            // This client is not in the list. Skip removing from the clients list
            LE_INFO("Skip client: %p", sessionRef);
        }

        // If there are no more clients requesting this event then stop the wpa ctrl thread.
        if (0 == ApctxPtr->devCnxEvtClients.size() && (ApctxPtr->isAPWpaCtrlThreadRunning))
        {
            LE_INFO("Last client. Stop WPA ctrl thread");
            le_thread_Cancel(ApctxPtr->APWpaCtrlThreadRef);
            ApctxPtr->isAPWpaCtrlThreadRunning = false;
            ApctxPtr->APWpaCtrlThreadRef = nullptr;
        }
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * WLAN Svc client connected handler.
 * TBD: Move to a common location in tafWlanSvcImpl
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPSvcImpl::OnWlanSvcClientConnect(le_msg_SessionRef_t sessionRef, void *context)
{
    LE_UNUSED(context);
    pid_t pid;
    char appName[LIMIT_MAX_PATH_BYTES] = {0};

    if (LE_OK != le_msg_GetClientProcessId(sessionRef, &pid))
    {
        LE_WARN("Error, Failed to get client pid.");
        return;
    }

    if (le_appInfo_GetName(pid, appName, sizeof(appName)) == LE_OK)
    {
        LE_INFO("Client Details: ref: %p, pid: %ld, name: %s", sessionRef,
                                                               static_cast<long int>(pid),
                                                               appName);
    }
    else
    {
        LE_WARN("Error, Failed to get client app name");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * WLAN Svc client disconnected handler.
 * TBD: Move to a common location in tafWlanSvcImpl
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanAPSvcImpl::OnWlanSvcClientDisconnect(le_msg_SessionRef_t sessionRef, void *context)
{
    LE_UNUSED(context);
    LE_INFO("Client ref: %p", sessionRef);

    // Find this AP context this client had registered the handler for
    for (int iCount = 1; iCount <= TAF_WLAN_MAX_NUM_AP; iCount++)
    {
        auto &wlanAp = taf_WlanAPSvcImpl::GetInstance();
        taf_wlan_AP_Ctx_t *ApctxPtr = wlanAp.GetWlanAPCtx(taf_wlan_APid_t(iCount));
        // Check if this client had requested the event and pop from the clients list.
        if (ApctxPtr->devCnxEvtClients.find(sessionRef) != ApctxPtr->devCnxEvtClients.end())
        {
            // Remove this client from the device connection events clients list
            LE_INFO("Remove DevCnxEvt client: %p", sessionRef);
            ApctxPtr->devCnxEvtClients.erase(sessionRef);
        }
        else
        {
            // This client is not in the list. Skip removing from connection events clients list.
            LE_INFO("Skip DevCnxEvt client: %p", sessionRef);
        }

        // If there are no more clients requesting this event then stop the wpa ctrl thread.
        if (0 == ApctxPtr->devCnxEvtClients.size() && (ApctxPtr->isAPWpaCtrlThreadRunning))
        {
            LE_INFO("Last client. Stop WPA ctrl thread");
            le_thread_Cancel(ApctxPtr->APWpaCtrlThreadRef);
            ApctxPtr->isAPWpaCtrlThreadRunning = false;
            ApctxPtr->APWpaCtrlThreadRef = nullptr;
        }
    }
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

    APCtxPoolRef = le_mem_InitStaticPool(tafWlanAPCtxPool, TAF_WLAN_MAX_NUM_AP,
        sizeof(taf_wlan_AP_Ctx_t));

    APCtxMutex = le_mutex_CreateNonRecursive("APCtxMutex");

    APRefMap = le_ref_CreateMap("APRefMap", TAF_WLAN_MAX_NUM_AP);

    for (int id = 1; id <= TAF_WLAN_MAX_NUM_AP; ++id)
    {
        taf_wlan_AP_Ctx_t *ctxPtr = (taf_wlan_AP_Ctx_t *)le_mem_ForceAlloc(APCtxPoolRef);
        if (ctxPtr == nullptr)
        {
            LE_FATAL("Unable to allocate ctxPtr for AP ID: %d", id);
        }
        ctxPtr->id = (taf_wlan_APid_t)id;
        ctxPtr->interfaceName[0] = 0;
        taf_wlanAp_WlanAPRef_t apRef =
            (taf_wlanAp_WlanAPRef_t)le_ref_CreateRef(APRefMap, (void *)ctxPtr);
        if (apRef == nullptr)
        {
            LE_FATAL("Unable to allocate reference for AP ID: %d", id);
        }
        ctxPtr->wlanAPRef = apRef;
        le_dls_Queue(&APCtxList, &ctxPtr->link);
        LE_DEBUG("Context created for STA ID: %d", ctxPtr->id);


        // Create client connection event for applications.
        std::string eventName = "ApCtx-" + ctxPtr->id + std::string(": ClientCnxEvt");
        ctxPtr->DeviceConnectionEvent = le_event_CreateIdWithRefCounting(eventName.c_str());

        // Initialize APWpaCtrlThreadRef to null
        ctxPtr->APWpaCtrlThreadRef = nullptr;

        // Clear the list of clients that have requested device connection events.
        ctxPtr->devCnxEvtClients.clear();
    }

    // Memory for client connection
    DeviceCnxEventPoolRef = le_mem_InitStaticPool(DeviceCnxEventPool, TAF_WLAN_MAX_SESSION_REF,
                                                  sizeof(DeviceCnxEvent_t));

    // Create clientdisconnect handler
    le_msg_AddServiceOpenHandler(taf_wlanAp_GetServiceRef(),
                                 taf_WlanAPSvcImpl::OnWlanSvcClientConnect, NULL);

    le_msg_AddServiceCloseHandler(taf_wlanAp_GetServiceRef(),
                                  taf_WlanAPSvcImpl::OnWlanSvcClientDisconnect, NULL);

    LE_INFO(" *** Wlan AP Initialized *** ");

    return;
}
