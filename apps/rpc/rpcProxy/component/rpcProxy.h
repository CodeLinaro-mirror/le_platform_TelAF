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

#ifndef TAF_RPC_PROXY_H_INCLUDE_GUARD
#define TAF_RPC_PROXY_H_INCLUDE_GUARD

#include "limit.h"
#include "user.h"
#include "jansson.h"


//--------------------------------------------------------------------------------------------------
/**
 * RPC version number. Used to fill the service version value in SOME/IP-SD message.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_MAJOR_VERSION 0x01            ///< 1 byte
#define RPC_MINOR_VERSION 0x00000000      ///< 4 bytes


//--------------------------------------------------------------------------------------------------
/**
 * RPC configuration MACROs.
 */
//---------------------------------------------------------------------------------
#define RPC_MAX_SYSTEMS  8           // Maximum RPC systems can be confingured for a service.
#define RPC_MAX_SERVICES 64          // Maximum RPC services can be configured in JSON file.


//--------------------------------------------------------------------------------------------------
/**
 * MACRO definition for the default value of configuration items.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_RPC_DEFAULT_CONFIG_FILE "taf_rpc.json"
#define TAF_RPC_DEFAULT_SERVICE_VER "ANY_VERSION"
#define TAF_RPC_DEFAULT_RESP_TIMEOUT 30
#define TAF_RPC_DEFAULT_CONNECTION_TYPE "TCP"
#define TAF_RPC_DEFAULT_BASE_SERVICE_ID 0xED00
#define TAF_RPC_DEFAULT_BASE_PORT_NUMBER 40000


//--------------------------------------------------------------------------------------------------
/**
 * RPC system Id type.
 */
//--------------------------------------------------------------------------------------------------
typedef uint16_t SystemId_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC service Id type.
 */
//--------------------------------------------------------------------------------------------------
typedef uint16_t ServiceId_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC system base information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct SystemLink
{
    uint16_t index;                                 ///< Index in the system table of JSON file
    SystemId_t id;                                  ///< System ID
    char name[LIMIT_MAX_SYSTEM_NAME_BYTES];         ///< System name
}SystemLink_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC service base information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct ServiceLink
{
    ServiceId_t id;                                 ///< SOME/IP service id
    char user[LIMIT_MAX_USER_NAME_BYTES];           ///< User name
    char name[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];  ///< Interface name
    char protocolId[LIMIT_MAX_PROTOCOL_ID_BYTES];   ///< Protocol ID string
}ServiceLink_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC proxy type.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    RPC_CLIENT_PROXY,             ///< RPC client proxy
    RPC_SERVER_PROXY,             ///< RPC server proxy
}ProxyType_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC proxy node
 */
//--------------------------------------------------------------------------------------------------
typedef struct ProxyNode
{
    ProxyType_t type;             ///< RPC proxy type
    le_dls_Link_t link;           ///< Link to the list
}ProxyNode_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference type for a RPC remote system.
 */
//--------------------------------------------------------------------------------------------------
typedef struct RpcRemoteSystem* RpcRemoteSystem_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference type for RPC client node.
 */
//--------------------------------------------------------------------------------------------------
typedef struct RpcClientProxyNode* RpcClientProxyNode_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference type for RPC server node.
 */
//--------------------------------------------------------------------------------------------------
typedef struct RpcServerProxy* RpcServerProxyNode_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Get my system link info data struct.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_GetMySystemLink
(
    SystemLink_t* systemLinkPtr               ///< [OUT] My system info.
);


//--------------------------------------------------------------------------------------------------
/**
 * Find a remote system by system ID.
 */
//--------------------------------------------------------------------------------------------------
RpcRemoteSystem_Ref_t rpcProxy_FindRemoteSystem
(
    const SystemId_t systemId                ///< [IN] Remote system ID.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get system ID by remote system reference.
 */
//--------------------------------------------------------------------------------------------------
SystemId_t rpcProxy_GetRemoteSystemId
(
    const RpcRemoteSystem_Ref_t systemRef    ///< [IN] System refence.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get system index by remote system reference.
 */
//--------------------------------------------------------------------------------------------------
uint16_t rpcProxy_GetRemoteSystemIndex
(
    const RpcRemoteSystem_Ref_t systemRef    ///< [IN] System refence.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a RPC proxy node into the list of a given system.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_AddNode
(
    RpcRemoteSystem_Ref_t systemRef,         ///< [IN] Reference to a remote system.
    ProxyNode_t* nodePtr                     ///< [IN] RPC proxy node pointer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Recreate proxy binding entries for RPC client proxy nodes.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_CreateAllProxyBindings
(
    void
);

#endif /* TAF_RPC_PROXY_H_INCLUDE_GUARD */
