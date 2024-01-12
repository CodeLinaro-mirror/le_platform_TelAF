/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#ifndef TAF_RPC_PROXY_CONFIG_H_INCLUDE_GUARD
#define TAF_RPC_PROXY_CONFIG_H_INCLUDE_GUARD

#include "rpcProxy.h"

//--------------------------------------------------------------------------------------------------
/**
 * RPC offer service entry configuration data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct OfferServiceConfigEntry
{
    ServiceLink_t service;                        // Offering service data struct.
    struct
    {
        bool isReliable;                          // True for TCP, otherwise UDP.
        uint16_t number;                          // Port number.
    }port;
    SystemId_t clientSystems[RPC_MAX_SYSTEMS];    // Remote client system ID array.
    uint8_t systemCnt;                            // Count in array.
}OfferServiceConfigEntry_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC request service entry configuration data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct RequestServiceConfigEntry
{
    ServiceLink_t service;                        // Requesting service data struct.
    bool isReliable;                              // If using reliable connection.
    uint16_t responseTimeout;                     // Response timeout seconds.
    SystemId_t serverSystems[RPC_MAX_SYSTEMS];    // Remote server system ID array.
    uint8_t systemCnt;                            // Count in array.
}RequestServiceConfigEntry_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC global configuration data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct RpcConfigData
{
    SystemLink_t mySystem;                                        // My system.
    SystemLink_t remoteSystems[RPC_MAX_SYSTEMS];                  // Remote system array.
    uint8_t remoteSystemCnt;                                      // Count in array.
    OfferServiceConfigEntry_t offerServices[RPC_MAX_SERVICES];    // Offer service array.
    uint8_t offerServiceCnt;                                      // Count in array.
    RequestServiceConfigEntry_t requestServices[RPC_MAX_SERVICES];// Request service array.s
    uint8_t requestServiceCnt;                                    // Count in array.
}RpcConfigData_t;

//--------------------------------------------------------------------------------------------------
/**
 * RPC configuration callback.
 * The "result" parameter contains the return code of the rpcProxy_LoadConfiguration().
 * The possible return code can be one of below:
 *     - LE_OK -- JSON file loading Succeeded.
 *     - LE_NOT_FOUND -- JSON file is not found, or configuration for the local system is not found.
 *     - LE_IN_PROGRESS -- Previous rpcProxy_LoadConfiguration() is still in progress.
 *     - LE_FAULT -- Error detected during parsing the JSON file.
 *
 * A pointer to the configuration data struct will be passed in once the return code is LE_OK.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*RpcConfigCallbackFunc_t)
(
    le_result_t result,
    const RpcConfigData_t* configurationPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Load the RPC configuration from the JSON file. The configuration callback function will be called
 * once JSON file parsing is done.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyConfig_LoadConfiguration
(
    const char* filePathPtr,            // The full path of the JSON file.
    RpcConfigCallbackFunc_t funcPtr     // The callback function pointer.
);

//--------------------------------------------------------------------------------------------------
/**
 * Dump all RPC configurations.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyConfig_ShowConfiguration
(
    void
);

#endif /* TAF_RPC_PROXY_CONFIG_H_INCLUDE_GUARD */
