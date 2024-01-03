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

#include "legato.h"
#include "interfaces.h"
#include "rpcProxy.h"
#include "rpcProxyMessaging.h"
#include "rpcClientProxy.h"
#include "rpcServerProxy.h"


//--------------------------------------------------------------------------------------------------
/**
 * Remote system data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct RpcRemoteSystem
{
    SystemLink_t system;                  ///< Remote system info
    le_dls_List_t rpcNodeList;            ///< RPC proxy nodes list
    le_dls_Link_t link;                   ///< Link to the system list
}RpcRemoteSystem_t;


//--------------------------------------------------------------------------------------------------
/**
 * Remote system list.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t RemoteSystemList = LE_DLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * My system Link info.
 */
//--------------------------------------------------------------------------------------------------
static SystemLink_t MySystemLink = { 0, 0, "Unknown" };


//--------------------------------------------------------------------------------------------------
/**
 * RPC remote system pool.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(RpcRemoteSystemPool, RPC_MAX_SYSTEMS, sizeof(RpcRemoteSystem_t));

static le_mem_PoolRef_t RpcRemoteSystemPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Get my system info data struct.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_GetMySystemLink
(
    SystemLink_t* systemLinkPtr                      ///< [OUT] My system link info.
)
{
    LE_ASSERT(systemLinkPtr != NULL);

    systemLinkPtr->index = MySystemLink.index;
    systemLinkPtr->id = MySystemLink.id;
    le_utf8_Copy(systemLinkPtr->name, MySystemLink.name, sizeof(systemLinkPtr->name), NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Find a remote system by system ID.
 */
//--------------------------------------------------------------------------------------------------
RpcRemoteSystem_Ref_t rpcProxy_FindRemoteSystem
(
    const SystemId_t systemId                          ///< [IN] Remote system ID.
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&RemoteSystemList);

    while (linkPtr != NULL)
    {
        RpcRemoteSystem_t* systemPtr = CONTAINER_OF(linkPtr, RpcRemoteSystem_t, link);
        if ((systemPtr != NULL) && (systemPtr->system.id == systemId))
        {
            return systemPtr;
        }

        linkPtr = le_dls_PeekNext(&RemoteSystemList, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get system ID by remote system reference.
 */
//--------------------------------------------------------------------------------------------------
SystemId_t rpcProxy_GetRemoteSystemId
(
    const RpcRemoteSystem_Ref_t systemRef    ///< [IN] System refence.
)
{
    RpcRemoteSystem_t* systemPtr = systemRef;
    LE_ASSERT(systemPtr != NULL);

    return systemPtr->system.id;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get system index by remote system reference.
 */
//--------------------------------------------------------------------------------------------------
uint16_t rpcProxy_GetRemoteSystemIndex
(
    const RpcRemoteSystem_Ref_t systemRef    ///< [IN] System refence.
)
{
    RpcRemoteSystem_t* systemPtr = systemRef;
    LE_ASSERT(systemPtr != NULL);

    return systemPtr->system.index;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add the proxy node into the list of a given remote system.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_AddNode
(
    RpcRemoteSystem_Ref_t systemRef,        ///< [IN] Reference to a remote system.
    ProxyNode_t*          nodePtr           ///< [IN] RPC proxy node ptr.
)
{
    RpcRemoteSystem_t* systemPtr = systemRef;
    LE_ASSERT((systemPtr != NULL) && (nodePtr != NULL));

    // Save the node in the rpcNode list of this remote system.
    le_dls_Queue(&systemPtr->rpcNodeList, &nodePtr->link);
}


//--------------------------------------------------------------------------------------------------
/**
 * Recreate proxy binding entries for RPC client proxy nodes. This is triggered after application
 * installation and uninstallation.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_CreateAllProxyBindings
(
    void
)
{
    le_dls_Link_t* sysLinkPtr = le_dls_Peek(&RemoteSystemList);
    while (sysLinkPtr != NULL)
    {
        RpcRemoteSystem_t* systemPtr = CONTAINER_OF(sysLinkPtr, RpcRemoteSystem_t, link);

        le_dls_Link_t* nodeLinkPtr = le_dls_Peek(&systemPtr->rpcNodeList);
        while (nodeLinkPtr != NULL)
        {
            ProxyNode_t* nodePtr = CONTAINER_OF(nodeLinkPtr, ProxyNode_t, link);
            if ((nodePtr != NULL) && (nodePtr->type == RPC_CLIENT_PROXY))
            {
                RpcClientProxyNode_Ref_t nodeRef = (RpcClientProxyNode_Ref_t)nodePtr;
                rpcClientProxy_CreateBinding(nodeRef);
            }

            nodeLinkPtr = le_dls_PeekNext(&systemPtr->rpcNodeList, nodeLinkPtr);
        }

        sysLinkPtr = le_dls_PeekNext(&RemoteSystemList, sysLinkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Remote systems initialization function.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LocalSystemInit
(
    const SystemLink_t* systemLinkPtr          ///< [IN] Local system info.
)
{
    // Parameters check.
    if (systemLinkPtr == NULL)
    {
        LE_ERROR("systemPtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Initialize local system info.
    memset(&MySystemLink, 0, sizeof(MySystemLink));

    MySystemLink.index = systemLinkPtr->index;
    MySystemLink.id = systemLinkPtr->id;
    le_utf8_Copy(MySystemLink.name, systemLinkPtr->name, sizeof(MySystemLink.name), NULL);

    LE_INFO("Local System(index=0x%x, id=0x%x, name='%s')",
            MySystemLink.index, MySystemLink.id, MySystemLink.name);

    // Sanity check for system id, which shall be identical to SOME/IP client ID.
    uint16_t mySomeipClientId = taf_someipClnt_GetClientId();
    if (mySomeipClientId != MySystemLink.id)
    {
        LE_ERROR("Local SystemId(0x%x) and SOME/IP ClientId(0x%x) do not match.",
                 MySystemLink.id, mySomeipClientId);
        return LE_NOT_PERMITTED;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remote systems initialization function.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RemoteSystemsInit
(
    const SystemLink_t* systemLinkTablePtr,  ///< [IN] Remote system info table.
    const uint8_t number                     ///< [IN] Number of the table.
)
{
    uint8_t cnt;
    RpcRemoteSystem_t* systemPtr;

    // Parameters check.
    if ((systemLinkTablePtr == NULL) || (number == 0))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    // Create remote system objects.
    for(cnt = 0; cnt < number; cnt++)
    {
        systemPtr = le_mem_TryAlloc(RpcRemoteSystemPoolRef);
        if (systemPtr == NULL)
        {
            LE_ERROR("systemPtr is NULL.");
            return LE_NO_MEMORY;
        }

        // Initialize the data struct members.
        memset(systemPtr, 0, sizeof(RpcRemoteSystem_t));

        systemPtr->system.index = systemLinkTablePtr->index;
        systemPtr->system.id = systemLinkTablePtr->id;
        le_utf8_Copy(systemPtr->system.name, systemLinkTablePtr->name,
                     sizeof(systemPtr->system.name), NULL);

        systemPtr->rpcNodeList = LE_DLS_LIST_INIT;
        systemPtr->link = LE_DLS_LINK_INIT;

        // Save the system object in the system list.
        le_dls_Queue(&RemoteSystemList, &systemPtr->link);
        LE_INFO("Created a remote system(index=0x%x, id=0x%x, name='%s')",
                systemPtr->system.index, systemPtr->system.id, systemPtr->system.name);

        // Move to the next entry in system info table.
        systemLinkTablePtr++;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC system initialization function.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RpcSystemInit
(
    const RpcConfigData_t* configPtr  ///< [IN] Configuration data struct pointer.
)
{
    if(configPtr == NULL)
    {
        LE_ERROR("configPtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != LocalSystemInit(&(configPtr->mySystem)))
    {
        LE_ERROR("Failed to initialize local system.");
        return LE_FAULT;
    }

    // Initialize remote systems.
    if (LE_OK != RemoteSystemsInit(configPtr->remoteSystems, configPtr->remoteSystemCnt))
    {
        LE_ERROR("Failed to initialize remote systems.");
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Do sanity check for the configuration.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RpcConfigCheck
(
    const RpcConfigData_t* configPtr  ///< [IN] Configuration data struct pointer.
)
{
    // <TBD>
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC start a system.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RpcStartSystem
(
    RpcRemoteSystem_t* systemPtr              ///< [IN] System ptr.
)
{
    if (systemPtr == NULL)
    {
        LE_ERROR("systemPtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Check the size of the rpcNodeList for a given system. And simply return if
    // the list is empty.
    if (le_dls_NumLinks(&systemPtr->rpcNodeList) == 0)
    {
        LE_WARN("No proxyNodes configured for system(id=0x%x).", systemPtr->system.id);
        return LE_OK;
    }

    // Go through the list and start all proxy nodes.
    le_dls_Link_t* linkPtr = le_dls_Peek(&systemPtr->rpcNodeList);
    while (linkPtr != NULL)
    {
        ProxyNode_t* nodePtr = CONTAINER_OF(linkPtr, ProxyNode_t, link);

        if (nodePtr->type == RPC_CLIENT_PROXY)
        {
            RpcClientProxyNode_Ref_t nodeRef = (RpcClientProxyNode_Ref_t)nodePtr;
            if (LE_OK != rpcClientProxy_Start(nodeRef))
            {
                return LE_FAULT;
            }
        }
        else if (nodePtr->type == RPC_SERVER_PROXY)
        {
            RpcServerProxyNode_Ref_t nodeRef = (RpcServerProxyNode_Ref_t)nodePtr;
            if (LE_OK != rpcServerProxy_Start(nodeRef))
            {
                return LE_FAULT;
            }
        }
        else
        {
            LE_ERROR("unknown node type (%d).", nodePtr->type);
            return LE_FAULT;
        }

        linkPtr = le_dls_PeekNext(&systemPtr->rpcNodeList, linkPtr);
    }

    LE_INFO("Started RPC remote system(id=0x%x).", systemPtr->system.id);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC start function.
 */
//--------------------------------------------------------------------------------------------------
static void RpcStart
(
    void
)
{
    // Walk through the global system list, start the RPC client proxies and server proxies
    // for each remote system.
    le_dls_Link_t* linkPtr = le_dls_Peek(&RemoteSystemList);
    while (linkPtr != NULL)
    {
        RpcRemoteSystem_t* systemPtr = CONTAINER_OF(linkPtr, RpcRemoteSystem_t, link);
        LE_ASSERT(systemPtr != NULL);

        if (LE_OK != RpcStartSystem(systemPtr))
        {
            LE_ERROR("Failed to start system (0x%x).", systemPtr->system.id);
        }

        linkPtr = le_dls_PeekNext(&RemoteSystemList, linkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC configuration handler function. The handler is triggered once loading configuration is done.
 * The configuration data is then passed as an input parameter if the result is LE_OK.
 */
//--------------------------------------------------------------------------------------------------
static void RpcConfigHandler
(
    le_result_t result,               ///< [IN] Result of loading the configuration.
    const RpcConfigData_t* configPtr  ///< [IN] Configuration data struct pointer.
)
{
    bool isRpcProxyDetected = false;

    if (result == LE_NOT_FOUND)
    {
        LE_FATAL("No JSON file or invalid JSON file is detected, Stop RPC proxy.");
    }

    if (result != LE_OK)
    {
        LE_FATAL("Failed to load RPC configuration(result = '%s'), Stop RPC proxy.",
                 LE_RESULT_TXT(result));
    }

    // Check the configuration.
    if (LE_OK != RpcConfigCheck(configPtr))
    {
        LE_ERROR("RpcConfigCheck() failed, RPC proxy is disabled.");
        return;
    }

    // Show RPC configuration.
    rpcProxyConfig_ShowConfiguration();

    // Initialize RPC proxy.
    if (LE_OK != RpcSystemInit(configPtr))
    {
        LE_ERROR("RpcSystemInit() failed, RPC proxy is disable.");
        return;
    }

    le_result_t ret;
    // Initialize RPC client proxies.
    ret = rpcClientProxy_Init(configPtr->offerServices, configPtr->offerServiceCnt);
    if ((ret != LE_OK) && (ret != LE_NOT_FOUND))
    {
        LE_ERROR("rpcClientProxy_Init() failed, RPC proxy is disabled.");
        return;
    }

    if (ret == LE_OK)
    {
        isRpcProxyDetected = true;
    }

    // Initialize RPC server proxies.
    ret = rpcServerProxy_Init(configPtr->requestServices, configPtr->requestServiceCnt);
    if ((ret != LE_OK) && (ret != LE_NOT_FOUND))
    {
        LE_ERROR("rpcServerProxy_Init() failed, RPC proxy is disabled.");
        return;
    }

    if (ret == LE_OK)
    {
        isRpcProxyDetected = true;
    }

    if (!isRpcProxyDetected)
    {
        LE_ERROR("No RPC proxies are configured, RPC proxy is disabled.");
        return;
    }

    // Start RPC
    RpcStart();
}


//--------------------------------------------------------------------------------------------------
/**
 * Data struct initialization function.
 */
//--------------------------------------------------------------------------------------------------
static void DataInit
(
    void
)
{
    // Initialize system users.
    user_Init();

    // RPC messaging initialization.
    rpcProxyMessage_Init();

    // Initialize RPC remote system pool.
    RpcRemoteSystemPoolRef = le_mem_InitStaticPool(RpcRemoteSystemPool,
                                                   RPC_MAX_SYSTEMS,
                                                   sizeof(RpcRemoteSystem_t));
}


//--------------------------------------------------------------------------------------------------
/**
 * Component initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    DataInit();
    LE_INFO("Loading configuration starts ...");
    rpcProxyConfig_LoadConfiguration(TAF_RPC_DEFAULT_CONFIG_FILE,
        (RpcConfigCallbackFunc_t)RpcConfigHandler);
}

