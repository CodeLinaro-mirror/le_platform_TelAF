/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "taf_pa_net_types.hpp"
#include "taf_pa_net.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Set device mode
 *
 * @return LE_FAULT                      Failed
 *         LE_BAD_PARAMETER              Invalid deviceMode
 *         LE_OK                         Succeeded
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_net_SetDeviceMode
(
    taf_net_DeviceMode_t deviceMode  ///< [IN] Device mode
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get device mode
 *
 * @return taf_net_DeviceMode_t          Device mode
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_net_DeviceMode_t taf_pa_net_GetDeviceMode
(
)
{
    return TAF_NET_DEVICE_NONE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set SOCKS authentication method
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_net_SetSocksAuthMethod
(
    taf_net_AuthMethod_t authMethod
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get SOCKS authentication method
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_net_AuthMethod_t taf_pa_net_GetSocksAuthMethod
(
)
{LE_INFO("-----stub get socks auth method---");
    return TAF_NET_SOCKS_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets SOCKS LAN interface
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_net_SetSocksLanInterface
(
    const char* ifName
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets SOCKS LAN interface
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_net_GetSocksLanInterface
(
    char* ifName,
    size_t ifNameSize
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds username/profile association
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_net_AddSocksAssociation
(
    const char* userName,
    uint32_t profileId
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes username/profile association
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_net_RemoveSocksAssociation
(
    const char* userName
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Init this component
 */
//--------------------------------------------------------------------------------------------------

COMPONENT_INIT
{
    LE_INFO("taf_pa_net stub");
}
