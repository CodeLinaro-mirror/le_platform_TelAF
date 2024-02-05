/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanSTASvcImpl.cpp
 *
 * @brief      Implemenation of TelAF WLAN Station Service APIs.
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
le_result_t taf_WlanSTASvcImpl::Start()
{
    if (nullptr == wlanSTAMgr)
    {
        LE_WARN ("WLAN STA Manager not initialized");
        return LE_FAULT;
    }
    telux::common::ErrorCode errCode =
            wlanSTAMgr->manageStaService(wlanSTAid, telux::wlan::ServiceOperation::START);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA Start failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    LE_INFO ("WLAN STA Start success");
    return LE_OK;
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
le_result_t taf_WlanSTASvcImpl::Stop()
{
    if (nullptr == wlanSTAMgr)
    {
        LE_WARN ("WLAN STA Manager not initialized");
        return LE_FAULT;
    }
    telux::common::ErrorCode errCode =
            wlanSTAMgr->manageStaService(wlanSTAid, telux::wlan::ServiceOperation::STOP);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA Stop failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    LE_INFO ("WLAN STA Stop success");
    return LE_OK;

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
le_result_t taf_WlanSTASvcImpl::Restart()
{
    if (nullptr == wlanSTAMgr)
    {
        LE_WARN ("WLAN STA Manager not initialized");
        return LE_FAULT;
    }
    telux::common::ErrorCode errCode =
            wlanSTAMgr->manageStaService(wlanSTAid, telux::wlan::ServiceOperation::RESTART);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA Restart failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    LE_INFO ("WLAN STA Restart success");
    return LE_OK;
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
le_result_t taf_WlanSTASvcImpl::SetMode
(
    taf_wlanSta_Mode_t StaMode
        ///< [IN] The WLAN STA mode to set.
)
{
    if (nullptr == wlanSTAMgr)
    {
        LE_WARN ("WLAN STA Manager not initialized");
        return LE_FAULT;
    }
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;
    errCode = wlanSTAMgr->setBridgeMode ( wlanSTAid, taf_WlanHelper::StaModeToTelux(StaMode));
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA SetMode failed with error : %d", (int)errCode);
        return LE_FAULT;
    }

    return LE_OK;
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
le_result_t taf_WlanSTASvcImpl::GetMode
(
    taf_wlanSta_Mode_t* StaModePtr
        ///< [OUT] The WLAN STA mode that is set.
)
{
    TAF_ERROR_IF_RET_VAL(NULL == StaModePtr, LE_BAD_PARAMETER, "StaModePtr is NULL!");

    if (nullptr == wlanSTAMgr)
    {
        LE_WARN ("WLAN STA Manager not initialized");
        return LE_FAULT;
    }
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;
    std::vector<telux::wlan::StaConfig> config;
    errCode = wlanSTAMgr->getConfig(config);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA GetMode failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    for (auto &cfg : config)
    {
        LE_DEBUG("------------------------------------------");
        LE_DEBUG("STA Id: %d", (int)cfg.staId);
        if (wlanSTAid == cfg.staId)
        {
            *StaModePtr = taf_WlanHelper::StaModeToTAF(cfg.bridgeMode);
            break;
        }
    }
    return LE_OK;
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
le_result_t taf_WlanSTASvcImpl::SetStaticIPConfig  (
                        const taf_wlanSta_IPConfig_t * LE_NONNULL StaStaticIPConfigPtr
)
{
    TAF_ERROR_IF_RET_VAL(NULL == StaStaticIPConfigPtr, LE_BAD_PARAMETER,
                                                     "StaStaticIPConfigPtr is NULL!");
    if (nullptr == wlanSTAMgr)
    {
        LE_WARN ("WLAN STA Manager not initialized");
        return LE_FAULT;
    }
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;
    telux::wlan::StaStaticIpConfig staticIpConfig;

    // Static IP. Populate the static IP structure.
    staticIpConfig.ipAddr   = StaStaticIPConfigPtr->IPv4Addr;
    staticIpConfig.gwIpAddr = StaStaticIPConfigPtr->GWAddr;
    staticIpConfig.netMask  = StaStaticIPConfigPtr->NetMask;
    staticIpConfig.dnsAddr  = StaStaticIPConfigPtr->DNSAddr;
    errCode = wlanSTAMgr->setIpConfig ( wlanSTAid,
                                       taf_WlanHelper::StaIPTypeToTelux (TAF_WLANSTA_IPTYPE_STATIC),
                                       staticIpConfig);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA SetMode failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    return LE_OK;
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
le_result_t taf_WlanSTASvcImpl::GetIPConfig
(
    taf_wlanSta_IPType_t* StaIPTypePtr,
        ///< [OUT] Dynamic or Static IP address.
    taf_wlanSta_IPConfig_t * StaStaticIPConfigPtr
        ///< [OUT] Details of static IP configuration.
)
{
    TAF_ERROR_IF_RET_VAL(NULL == StaIPTypePtr, LE_BAD_PARAMETER,"StaIPTypePtr is NULL!");

    if (nullptr == wlanSTAMgr)
    {
        LE_WARN ("WLAN STA Manager not initialized");
        return LE_FAULT;
    }
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;
    std::vector<telux::wlan::StaConfig> config;
    errCode = wlanSTAMgr->getConfig(config);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA GetMode failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    for (auto &cfg : config)
    {
        LE_DEBUG("------------------------------------------");
        LE_DEBUG("STA Id: %d", (int)cfg.staId);
        if (wlanSTAid == cfg.staId)
        {
            *StaIPTypePtr = taf_WlanHelper::StaIPTypeToTAF(cfg.ipConfig);
            if (StaStaticIPConfigPtr && telux::wlan::StaIpConfig::STATIC_IP==cfg.ipConfig)
            {
                le_result_t ret = LE_OK;
                LE_DEBUG ("IPv4Addr :%s",cfg.staticIpConfig.ipAddr.c_str());
                ret = le_utf8_Copy(StaStaticIPConfigPtr->IPv4Addr,cfg.staticIpConfig.ipAddr.c_str(),
                                               TAF_NET_IPV4_ADDR_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("IPv4Addr copy error: %d", ret);
                }
                LE_DEBUG ("GWAddr :%s",cfg.staticIpConfig.gwIpAddr.c_str());
                ret = le_utf8_Copy(StaStaticIPConfigPtr->GWAddr,cfg.staticIpConfig.gwIpAddr.c_str(),
                                               TAF_NET_IPV4_ADDR_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("GWAddr copy error: %d", ret);
                }
                LE_DEBUG ("DNSAddr :%s",cfg.staticIpConfig.dnsAddr.c_str());
                ret = le_utf8_Copy(StaStaticIPConfigPtr->DNSAddr,cfg.staticIpConfig.dnsAddr.c_str(),
                                               TAF_NET_IPV4_ADDR_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("DNSAddr copy error: %d", ret);
                }
                LE_DEBUG ("NetMask :%s",cfg.staticIpConfig.netMask.c_str());
                ret = le_utf8_Copy(StaStaticIPConfigPtr->NetMask,cfg.staticIpConfig.netMask.c_str(),
                                               TAF_NET_IPV4_ADDR_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("NetMask copy error: %d", ret);
                }
            }
            break;
        }
    }
    return LE_OK;
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
le_result_t taf_WlanSTASvcImpl::GetStatus
(
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
    TAF_ERROR_IF_RET_VAL(NULL == StaSatePtr, LE_BAD_PARAMETER,"StaSatePtr is NULL!");
    if (nullptr == wlanSTAMgr)
    {
        LE_WARN ("WLAN STA Manager not initialized");
        return LE_FAULT;
    }
    telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;
    std::vector<telux::wlan::StaStatus> status;
    errCode = wlanSTAMgr->getStatus(status);
    if (telux::common::ErrorCode::SUCCESS != errCode)
    {
        LE_WARN("WLAN STA getStatus failed with error : %d", (int)errCode);
        return LE_FAULT;
    }
    for (auto &element : status)
    {
        LE_DEBUG("------------------------------------------");
        LE_DEBUG("STA Id: %d", (int)element.id);
        if (wlanSTAid == element.id)
        {
            le_result_t ret = LE_OK;
            *StaSatePtr = taf_WlanHelper::StaIntfStatusToTAF(element.status);
            if (IntfName && (IntfNameSize>0))
            {
                ret = le_utf8_Copy(IntfName, element.name.c_str(),
                                               TAF_NET_INTERFACE_NAME_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("IntfName copy error: %d", ret);
                }
            }
            if (IPv4Address && (IPv4AddressSize>0))
            {
                ret = le_utf8_Copy(IPv4Address, element.ipv4Address.c_str(),
                                               TAF_NET_IPV4_ADDR_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("IPv4Address copy error: %d", ret);
                }
            }
            if (IPv6Address && (IPv6AddressSize>0))
            {
                ret = le_utf8_Copy(IPv6Address, element.ipv6Address.c_str(),
                                               TAF_NET_IPV6_ADDR_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("IPv6Address copy error: %d", ret);
                }
            }
            if (MACAddress && (MACAddressSize>0))
            {
                ret = le_utf8_Copy(MACAddress, element.macAddress.c_str(),
                                               TAF_NET_IPV6_ADDR_MAX_LEN+1, NULL);
                if (LE_OK != ret)
                {
                    LE_WARN("MACAddress copy error: %d", ret);
                }
            }
            break;
        }
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Return the instance of taf_WlanSTASvcImpl class.
 */
//--------------------------------------------------------------------------------------------------
taf_WlanSTASvcImpl &taf_WlanSTASvcImpl::GetInstance()
{
    static taf_WlanSTASvcImpl instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * taf_WlanAPSvcImpl Init function
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanSTASvcImpl::Init()
{
    // Initialize to nullptr
    wlanSTAMgr = nullptr;

    auto &wlanFactory = telux::wlan::WlanFactory::getInstance();
    wlanSTAMgr = wlanFactory.getStaInterfaceManager ();
    if (wlanSTAMgr==nullptr)
    {
        // Unable to initialize the WLAN STA subsystem. Stop the service.
        LE_FATAL (" *** Unable to initialize Wlan STA subsystem *** ");
    }

    // Register the Listener class shared object
    wlanSTAListener = std::make_shared<taf_WlanSTAListener>();
    telux::common::ErrorCode retCode = wlanSTAMgr->registerListener(wlanSTAListener);
    if (telux::common::ErrorCode::SUCCESS != retCode)
    {
        LE_WARN("WLAN STA registerListener failed: %d", (int)retCode);
    }

    LE_INFO(" *** Wlan STA Initialized *** ");

    return;
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
taf_wlanSta_WlanSTARef_t taf_WlanSTASvcImpl::GetWlanSTA (
    taf_wlan_STAid_t STAid,
        ///< [IN] STA identifier
    const char* LE_NONNULL STAIntfName
        ///< [IN] AP assocaited host interface name.
)
{
    LE_UNUSED(STAid);
    LE_UNUSED(STAIntfName);
    return NULL;
}