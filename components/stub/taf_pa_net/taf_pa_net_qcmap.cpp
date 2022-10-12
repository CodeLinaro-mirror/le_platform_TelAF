/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "legato.h"
#include "interfaces.h"
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
