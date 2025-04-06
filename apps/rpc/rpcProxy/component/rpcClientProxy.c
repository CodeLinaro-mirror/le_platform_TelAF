/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include "rpcClientProxy.h"
#include "rpcProxyMessaging.h"
#include "sdirToolProtocol.h"


//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP server-service instance common data struct.
 *
 * It's created by RPC client proxy, and TelAF service is one-to-one mapping to SOME/IP service.
 */
//--------------------------------------------------------------------------------------------------
typedef struct SomeipServer
{
    taf_someipSvr_ServiceRef_t serviceRef;          ///< Someip server service reference
    taf_someipSvr_RxMsgHandlerRef_t rxMsgHandlerRef;///< Someip server Rx Message handler reference
    uint16_t serviceId;                             ///< Service ID
    uint16_t instanceId;                            ///< Service Instance ID
    uint16_t port;                                  ///< port number
    bool isReliable;                                ///< Use TCP if true
    bool isOffered;                                 ///< True if the server instance is offered
}SomeipServer_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC client proxy instance data structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct RpcClientProxy
{
    ServiceLink_t serviceCfg;                      ///< Service base config information
    SomeipServer_t someipServer;                   ///< Someip server bind to this RPC client proxy
    size_t maxMessageSize;                         ///< Max payload message size
    char version[LIMIT_MAX_PROTOCOL_ID_BYTES];     ///< RPC service version
    le_timer_Ref_t timerRef;                       ///< Timer to check if the TelAF service is up
    le_dls_List_t rpcClientNodeList;               ///< rpcClientNode list of this RPC client proxy
    le_dls_Link_t link;                            ///< Link to the global list
}RpcClientProxy_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC client proxy node data struct.
 * Represent a remote client system for a RPC client proxy.
 */
//--------------------------------------------------------------------------------------------------
typedef struct RpcClientProxyNode
{
    ProxyNode_t baseClass;                ///< Proxy type
    RpcRemoteSystem_Ref_t systemRef;      ///< Remote client system ID
    taf_someipSvr_SubscriptionHandlerRef_t subsHandlerRef;///< Subscription handler ref
    RpcClientProxy_t* rpcClientPtr;       ///< RPC client proxy
    bool enabled;                         ///< If the remote client system is enabled
    char bindingInterface[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES]; ///< Interface name of this system
    uint16_t sysEventId;                  ///< SOME/IP event ID for this remote system
    RpcNodeState_t state;                 ///< RPC node state
    le_dls_List_t proxySessionList;       ///< Local IPC client proxy session list
    le_dls_Link_t link;                   ///< Link to rpcClientNode list of the RPC client proxy
}RpcClientProxyNode_t;


//--------------------------------------------------------------------------------------------------
/**
 * Local IPC client proxy session data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct ClientProxySession
{
    RpcClientProxyNode_t* rpcClientNodePtr;///< RPC client node reference
    le_msg_SessionRef_t sessionRef;     ///< Local IPC client session reference
    uint32_t rpcSessionId;              ///< RPC session ID assigned to this proxy session
    le_dls_List_t msgList;              ///< Proxy message list
    le_dls_Link_t link;                 ///< Link to the proxySessionList of the RPC client node
}ClientProxySession_t;


//--------------------------------------------------------------------------------------------------
/**
 * Local IPC proxy message data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct ClientProxyMsg
{
    ClientProxySession_t* clientProxySessionPtr;///< proxy session
    taf_someipSvr_RxMsgRef_t rpcMsgRef;         ///< IPC message reference
    le_dls_Link_t link;                         ///< Link to proxy session
}ClientProxyMsg_t;


//--------------------------------------------------------------------------------------------------
/**
 * Action in internal event
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    ACTION_REMOTE_UNSUBSCRIBED,             ///< Remote system is unsubsribed
    ACTION_REMOTE_SUBSCRIBED,               ///< Remote system is subscribed
    ACTION_REMOTE_CONNECTED                 ///< Remote system is connected
}Action_t;


//--------------------------------------------------------------------------------------------------
/**
 * Internal event data struct
 */
//--------------------------------------------------------------------------------------------------
typedef struct ClientNodeEvent
{
    Action_t action;                        ///< Action
    RpcClientProxyNode_t* rpcClientNodePtr; ///< RPC client proxy node pointer
}ClientNodeEvent_t;


//--------------------------------------------------------------------------------------------------
/**
 * TelAF service information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct ServiceInfo
{
    char version[LIMIT_MAX_PROTOCOL_ID_BYTES];      ///< Real version.
    size_t maxPayloadSize;                          ///< Max Message size.
}ServiceInfo_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC client proxy node pool.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(RpcClientProxyNodePool, (RPC_MAX_SYSTEMS*RPC_MAX_SERVICES),
                                  sizeof(RpcClientProxyNode_t));

static le_mem_PoolRef_t RpcClientProxyNodePoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * RPC client proxy instance pool.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(RpcClientProxyPool, RPC_MAX_SERVICES, sizeof(RpcClientProxy_t));

static le_mem_PoolRef_t RpcClientProxyPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Local IPC client proxy session pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t LocalClientProxySessionPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Local IPC message pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ClientProxyMsgPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * RPC client proxy node event and handler.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t RpcClientNodeEvent = NULL;
le_event_HandlerRef_t RpcClientNodeEventHandlerRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * RPC client proxy list.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t RpcClientProxyList = LE_DLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * RPC Rx message buffer.
 */
//--------------------------------------------------------------------------------------------------
static uint8_t RxMessageBuffer[TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE];


//--------------------------------------------------------------------------------------------------
/**
 * Get action name string.
 */
//--------------------------------------------------------------------------------------------------
static const char* GetActionString
(
    Action_t action  ///< [in] The action to be translated.
)
{
    switch (action)
    {
        case ACTION_REMOTE_UNSUBSCRIBED:
            return "ACTION_REMOTE_UNSUBSCRIBED";
        case ACTION_REMOTE_SUBSCRIBED:
            return "ACTION_REMOTE_SUBSCRIBED";
        case ACTION_REMOTE_CONNECTED:
            return "ACTION_REMOTE_CONNECTED";
    }

    LE_ERROR("action %d out of range.", action);
    return "(unknown)";
}


//--------------------------------------------------------------------------------------------------
/**
 * Allocate a RPC session ID. Possible IDs are from 1 to 0xFFFFFFFF.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t AllocateRpcSessionId
(
    void
)
{
    static uint32_t RpcSessionid = 0;

    RpcSessionid++;

    if (RpcSessionid == 0)
    {
        RpcSessionid = 1;
    }

    return RpcSessionid;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find the RPC client proxy node by remote system ID.
 */
//--------------------------------------------------------------------------------------------------
static RpcClientProxyNode_t* FindRpcClientNodeBySystemId
(
    uint16_t systemId,              ///< [IN] Remote system Id
    RpcClientProxy_t* rpcClientPtr  ///< [IN] RPC client proxy pointer
)
{
    if (rpcClientPtr == NULL)
    {
        LE_ERROR("Bad parameter.");
        return NULL;
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(&rpcClientPtr->rpcClientNodeList);

    while (linkPtr != NULL)
    {
        RpcClientProxyNode_t* rpcClientNodePtr = CONTAINER_OF(linkPtr, RpcClientProxyNode_t, link);
        LE_ASSERT((rpcClientNodePtr != NULL) &&
                  (rpcClientNodePtr->rpcClientPtr == rpcClientPtr))

        if (rpcProxy_GetRemoteSystemId(rpcClientNodePtr->systemRef) == systemId)
        {
            return rpcClientNodePtr;
        }

        linkPtr = le_dls_PeekNext(&rpcClientPtr->rpcClientNodeList, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find the RPC client proxy session by RPC session ID.
 */
//--------------------------------------------------------------------------------------------------
static ClientProxySession_t* FindClientProxySessionBySessionId
(
    uint32_t rpcSessionId,                  ///< [IN] RPC session ID
    RpcClientProxyNode_t* rpcClientNodePtr  ///< [IN] RPC client proxy node pointer
)
{
    if ((rpcSessionId == 0) || (rpcClientNodePtr == NULL))
    {
        LE_ERROR("Bad parameter.");
        return NULL;
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(&rpcClientNodePtr->proxySessionList);

    while (linkPtr != NULL)
    {
        ClientProxySession_t* clientProxySessionPtr =
            CONTAINER_OF(linkPtr, ClientProxySession_t, link);

        LE_ASSERT((clientProxySessionPtr != NULL) &&
                  (clientProxySessionPtr->rpcClientNodePtr == rpcClientNodePtr))

        if (clientProxySessionPtr->rpcSessionId == rpcSessionId)
        {
            return clientProxySessionPtr;
        }

        linkPtr = le_dls_PeekNext(&rpcClientNodePtr->proxySessionList, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete all proxy sessions of a given client proxy node.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteAllProxySessions
(
    RpcClientProxyNode_t* rpcClientNodePtr  ///< [IN] RPC client proxy node pointer
)
{
    LE_ASSERT((rpcClientNodePtr != NULL) && (rpcClientNodePtr->rpcClientPtr != NULL));

    le_dls_Link_t* linkPtr = le_dls_Pop(&rpcClientNodePtr->proxySessionList);

    while (linkPtr != NULL)
    {
        ClientProxySession_t* clientProxySessionPtr =
            CONTAINER_OF(linkPtr, ClientProxySession_t, link);
        LE_ASSERT((clientProxySessionPtr != NULL) &&
                  (clientProxySessionPtr->rpcClientNodePtr == rpcClientNodePtr))
        uint32_t rpcSessionId = clientProxySessionPtr->rpcSessionId;

        // Delete the IPC session associated.
        le_msg_DeleteSession(clientProxySessionPtr->sessionRef);
        clientProxySessionPtr->sessionRef = NULL;

        // Free the proxy session if no ongoing proxy messages associated.
        if (le_dls_IsEmpty(&clientProxySessionPtr->msgList))
        {
            le_mem_Release(clientProxySessionPtr);
            LE_INFO("Deleted proxy session(ID=0x%x) for interface '%s'.",
                    rpcSessionId, rpcClientNodePtr->bindingInterface);
        }

        linkPtr = le_dls_Pop(&rpcClientNodePtr->proxySessionList);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC client proxy node internal event handler.
 */
//--------------------------------------------------------------------------------------------------
static void RpcClientNodeEventHandler
(
    void* reportPtr
)
{
    ClientNodeEvent_t* myEventPtr = reportPtr;
    LE_ASSERT((myEventPtr != NULL) && (myEventPtr->rpcClientNodePtr != NULL) &&
              (myEventPtr->rpcClientNodePtr->rpcClientPtr != NULL));

    RpcClientProxyNode_t* rpcClientNodePtr = myEventPtr->rpcClientNodePtr;
    RpcClientProxy_t* rpcClientPtr = rpcClientNodePtr->rpcClientPtr;
    uint16_t serviceId = rpcClientPtr->someipServer.serviceId;
    uint16_t instanceId = rpcClientPtr->someipServer.instanceId;
    Action_t action = myEventPtr->action;

    RpcNodeState_t curState = rpcClientNodePtr->state;

    LE_INFO("Received action event('%s') in state('%s') of serviceNode(0x%x/0x%x<-0x%x).",
            GetActionString(action), rpcProxyMessage_GetStateString(curState),
            serviceId, instanceId, rpcProxy_GetRemoteSystemId(rpcClientNodePtr->systemRef));

    switch(curState)
    {
        case RPC_NODE_NOT_CONNECTED:
        {
            if (action == ACTION_REMOTE_SUBSCRIBED)
            {
                rpcClientNodePtr->state = RPC_NODE_CONNECTING;
            }
        }
        break;

        case RPC_NODE_CONNECTING:
        {
            if (action == ACTION_REMOTE_UNSUBSCRIBED)
            {
                rpcClientNodePtr->state = RPC_NODE_NOT_CONNECTED;
            }
            else if (action == ACTION_REMOTE_CONNECTED)
            {
                rpcClientNodePtr->state = RPC_NODE_CONNECTED;
            }
        }
        break;

        case RPC_NODE_CONNECTED:
        {
            if (action == ACTION_REMOTE_UNSUBSCRIBED)
            {
                rpcClientNodePtr->state = RPC_NODE_NOT_CONNECTED;

                // Cleanup all local proxy sessions.
                DeleteAllProxySessions(rpcClientNodePtr);
            }
        }
        break;

        default:
            LE_FATAL("Unknown curState (%d).", curState);
        break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * App install/uninstall callback handler.
 */
//--------------------------------------------------------------------------------------------------
static void AppUpdateHandler
(
    const char* appNamePtr,  ///< Name of the new application.
    void* contextPtr         ///< Registered context for this callback.
)
{
    if (appNamePtr != NULL)
    {
        rpcProxy_CreateAllProxyBindings();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Data struct initialization.
 */
//--------------------------------------------------------------------------------------------------
static void DataInit
(
    void
)
{
    // Initialize RPC client node object pool.
    RpcClientProxyNodePoolRef = le_mem_InitStaticPool(RpcClientProxyNodePool,
                                                      (RPC_MAX_SYSTEMS*RPC_MAX_SERVICES),
                                                      sizeof(RpcClientProxyNode_t));

    // Initialize RPC client proxy pool.
    RpcClientProxyPoolRef = le_mem_InitStaticPool(RpcClientProxyPool,
                                                  RPC_MAX_SERVICES,
                                                  sizeof(RpcClientProxy_t));

    // Initialize local client proxy session pool.
    LocalClientProxySessionPoolRef = le_mem_CreatePool("LocalClientProxySessionPool",
                                                       sizeof(ClientProxySession_t));

    // Initialize local message pool.
    ClientProxyMsgPoolRef = le_mem_CreatePool("ClientProxyMessagePool",
                                              sizeof(ClientProxyMsg_t));

    // Create internal event and add event handler.
    RpcClientNodeEvent = le_event_CreateId("RpcClientNodeEvent", sizeof(ClientNodeEvent_t));
    RpcClientNodeEventHandlerRef = le_event_AddHandler("RpcClientNodeEvent Handler",
                                                       RpcClientNodeEvent,
                                                       RpcClientNodeEventHandler);

    // Register handlers for app installation/un-installiation events to
    // recreate the proxy node bindings.
    le_instStat_AddAppInstallEventHandler(AppUpdateHandler, NULL);
    le_instStat_AddAppUninstallEventHandler(AppUpdateHandler, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC client node initialization function.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RpcClientNodeInit
(
    RpcClientProxy_t* rpcClientProxyPtr,             ///< [IN] RPC client proxy ptr.
    const SystemId_t* clientSystemTablePtr,          ///< [IN] Remote client system table.
    const uint8_t number                             ///< [IN] Number of the table.
)
{
    // Parameter check.
    if (rpcClientProxyPtr == NULL)
    {
        LE_ERROR("rpcClientProxyPtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Parameter check.
    if ((number == 0) || (clientSystemTablePtr == NULL))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    uint8_t cnt;
    uint16_t systemIndex;
    SystemId_t systemId;
    RpcRemoteSystem_Ref_t systemRef;
    RpcClientProxyNode_t* rpcClientNodePtr;

    // Check all supported remote client systems for this RPC client proxy instance.
    for (cnt = 0; cnt < number; cnt++)
    {
        systemId = *clientSystemTablePtr;
        systemRef = rpcProxy_FindRemoteSystem(systemId);
        if (systemRef == NULL)
        {
            LE_ERROR("Can not find the remote system for ID 0x%x.", systemId);
            return LE_FAULT;
        }

        systemIndex = rpcProxy_GetRemoteSystemIndex(systemRef);

        // Create a RPC client proxy node for this remote system.
        rpcClientNodePtr = le_mem_TryAlloc(RpcClientProxyNodePoolRef);
        if (rpcClientNodePtr == NULL)
        {
            LE_ERROR("rpcClientNodePtr is NULL.");
            return LE_NO_MEMORY;
        }

        // Initialize the node.
        memset(rpcClientNodePtr, 0, sizeof(RpcClientProxyNode_t));

        rpcClientNodePtr->rpcClientPtr= rpcClientProxyPtr;
        rpcClientNodePtr->enabled = false;
        rpcClientNodePtr->sysEventId = RPC_EVENT_ID_BASE + systemIndex;
        rpcClientNodePtr->state = RPC_NODE_NOT_CONNECTED;
        rpcClientNodePtr->systemRef = systemRef;
        rpcClientNodePtr->subsHandlerRef = NULL;
        rpcClientNodePtr->proxySessionList = LE_DLS_LIST_INIT;
        rpcClientNodePtr->link = LE_DLS_LINK_INIT;

        // Set binding interface.
        char interfaceName[256]={ 0 };
        snprintf(interfaceName, sizeof(interfaceName), "rpcClient_%x_%s",
                 systemId, rpcClientProxyPtr->serviceCfg.name);
        le_utf8_Copy(rpcClientNodePtr->bindingInterface, interfaceName,
                     sizeof(rpcClientNodePtr->bindingInterface), NULL);

        rpcClientNodePtr->baseClass.type = RPC_CLIENT_PROXY;
        rpcClientNodePtr->baseClass.link = LE_DLS_LINK_INIT;

        // Save the node in the rpcClientNode list of this RPC client proxy.
        le_dls_Queue(&rpcClientProxyPtr->rpcClientNodeList, &rpcClientNodePtr->link);

        // Save the node in the rpcNodeList of the remote system.
        rpcProxy_AddNode(rpcClientNodePtr->systemRef, &rpcClientNodePtr->baseClass);

        LE_INFO("Created a RPC client node(%p) for system ID(0x%x) and sysEvent ID(0x%x)" \
                "of rpcClientProxy(%p).", rpcClientNodePtr, systemId,
                rpcClientNodePtr->sysEventId, rpcClientNodePtr->rpcClientPtr);

        // Move to the next entry in the remote client system table.
        clientSystemTablePtr++;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC client proxy initialization function.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcClientProxy_Init
(
    const OfferServiceConfigEntry_t* serviceConfigPtr, ///< [IN] Offer service configuration table.
    uint8_t number                                     ///< [IN] Number of the table.
)
{
    if (number == 0)
    {
        // Simply return LE_OK if no RPC clients configured.
        LE_INFO("No RPC clientProxy is configured.");
        return LE_NOT_FOUND;
    }

    if (serviceConfigPtr == NULL)
    {
        LE_ERROR("serviceConfigPtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Data struct init.
    DataInit();

    uint8_t cnt;
    RpcClientProxy_t* rpcClientProxyPtr;
    le_result_t result;
    SystemLink_t mySystemLink;

    // Get my system info.
    memset(&mySystemLink, 0, sizeof(mySystemLink));
    rpcProxy_GetMySystemLink(&mySystemLink);

    // Create RPC client proxies and nodes.
    for(cnt = 0; cnt < number; cnt++)
    {
        rpcClientProxyPtr = le_mem_TryAlloc(RpcClientProxyPoolRef);
        if (rpcClientProxyPtr == NULL)
        {
            LE_ERROR("rpcClientProxyPtr is NULL.");
            return LE_NO_MEMORY;
        }

        // Initialize data struct.
        memset(rpcClientProxyPtr, 0, sizeof(RpcClientProxy_t));

        // Set base service info.
        rpcClientProxyPtr->serviceCfg.id = serviceConfigPtr->service.id;
        le_utf8_Copy(rpcClientProxyPtr->serviceCfg.user, serviceConfigPtr->service.user,
                     sizeof(rpcClientProxyPtr->serviceCfg.user), NULL);
        le_utf8_Copy(rpcClientProxyPtr->serviceCfg.name, serviceConfigPtr->service.name,
                     sizeof(rpcClientProxyPtr->serviceCfg.name), NULL);
        le_utf8_Copy(rpcClientProxyPtr->serviceCfg.protocolId, serviceConfigPtr->service.protocolId,
                     sizeof(rpcClientProxyPtr->serviceCfg.protocolId), NULL);

        // Set maxMessageSize and version to 0, later they will be set once the service
        // info is retrieved from serviceDirectory.
        rpcClientProxyPtr->maxMessageSize = 0;
        rpcClientProxyPtr->version[0] = '\0';

        // Initialize timer ref.
        rpcClientProxyPtr->timerRef = NULL;

        // Set SOME/IP server parameters. The service instance ID is equal to the local
        // system ID.
        rpcClientProxyPtr->someipServer.instanceId = mySystemLink.id;
        rpcClientProxyPtr->someipServer.serviceId = serviceConfigPtr->service.id;
        rpcClientProxyPtr->someipServer.isReliable = serviceConfigPtr->port.isReliable;
        rpcClientProxyPtr->someipServer.port = serviceConfigPtr->port.number;
        rpcClientProxyPtr->someipServer.serviceRef = NULL;
        rpcClientProxyPtr->someipServer.rxMsgHandlerRef = NULL;
        rpcClientProxyPtr->someipServer.isOffered = false;

        rpcClientProxyPtr->rpcClientNodeList = LE_DLS_LIST_INIT;
        rpcClientProxyPtr->link = LE_DLS_LINK_INIT;

        // Add the proxy object into list.
        le_dls_Queue(&RpcClientProxyList, &rpcClientProxyPtr->link);

        LE_INFO("Created rpcClientNode for" \
                " svrId=0x%x/0x%x svrName='%s' user='%s' protoId='%s'",
                rpcClientProxyPtr->someipServer.serviceId,
                rpcClientProxyPtr->someipServer.instanceId,
                rpcClientProxyPtr->serviceCfg.name,
                rpcClientProxyPtr->serviceCfg.user,
                rpcClientProxyPtr->serviceCfg.protocolId);

        // Create and initialize all client nodes of this RPC client proxy.
        result = RpcClientNodeInit(rpcClientProxyPtr,
                                   serviceConfigPtr->clientSystems,
                                   serviceConfigPtr->systemCnt);

        if (result != LE_OK)
        {
            return result;
        }

        // Move to the next entry in the offer service configuration table.
        serviceConfigPtr++;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if the service is up and running and get the service info if possible.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SdirGetServiceInfo
(
    const uid_t serviceUid,        ///< [IN] UID of the TelAF service.
    const char* serviceNamePtr,    ///< [IN] Service name.
    const char* protocolIdPtr,     ///< [IN] Service protocol ID.
    ServiceInfo_t* serviceInfoPtr  ///< [OUT] Service information.
)
{
    // Parameter check.
    if ((serviceNamePtr == NULL) && (protocolIdPtr == NULL) && (serviceInfoPtr == NULL))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    le_msg_ProtocolRef_t protocolRef;
    le_msg_SessionRef_t sessionRef;
    le_msg_MessageRef_t msgRef;
    le_sdtp_Msg_t* reqPayloadPtr;
    le_sdtp_resp_t* resPayloadPtr;

    // Try to connect to service directory.
    protocolRef = le_msg_GetProtocolRef(LE_SDTP_PROTOCOL_ID, sizeof(le_sdtp_Msg_t));
    sessionRef = le_msg_CreateSession(protocolRef, LE_SDTP_INTERFACE_NAME);

    if (LE_OK != le_msg_TryOpenSessionSync(sessionRef))
    {
        LE_ERROR("Failed to connect serviceDirectory.");
        le_msg_DeleteSession(sessionRef);
        return LE_FAULT;
    }

    // Construct the get service info request message.
    msgRef = le_msg_CreateMsg(sessionRef);
    reqPayloadPtr = le_msg_GetPayloadPtr(msgRef);
    reqPayloadPtr->msgType = LE_SDTP_MSGID_FIND_SERVICE;
    reqPayloadPtr->server = serviceUid;
    le_utf8_Copy(reqPayloadPtr->serverInterfaceName, serviceNamePtr,
                 sizeof(reqPayloadPtr->serverInterfaceName), NULL);

    // Send the message and wait for a response.
    msgRef = le_msg_RequestSyncResponse(msgRef);

    // If a response message was not received, then the operation failed.
    if (msgRef == NULL)
    {
        LE_ERROR("Communication with Service Directory failed.");
        le_msg_DeleteSession(sessionRef);
        return LE_FAULT;
    }

    // Get the response.
    resPayloadPtr = le_msg_GetPayloadPtr(msgRef);
    if (resPayloadPtr->result != LE_OK)
    {
        LE_ERROR("Failed to get service info (%s).", LE_RESULT_TXT(resPayloadPtr->result));
        le_msg_ReleaseMsg(msgRef);
        le_msg_DeleteSession(sessionRef);
        return LE_FAULT;
    }

    // Check if service versions match.
    if ((strcmp(protocolIdPtr, "ANY_VERSION") == 0) ||
        (strcmp(protocolIdPtr, resPayloadPtr->id) == 0))
    {
        // Copy and return the service info.
        serviceInfoPtr->maxPayloadSize = resPayloadPtr->maxPayloadSize;
        le_utf8_Copy(serviceInfoPtr->version, resPayloadPtr->id,
                     sizeof(serviceInfoPtr->version), NULL);

        // Release the message ref and session ref.
        le_msg_ReleaseMsg(msgRef);
        le_msg_DeleteSession(sessionRef);
        return LE_OK;
    }

    LE_ERROR("Service versions mismatch(reqService version='%s', getService version='%s').",
             protocolIdPtr, resPayloadPtr->id);
    le_msg_ReleaseMsg(msgRef);
    le_msg_DeleteSession(sessionRef);

    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a serviceDirectory binding entry.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SdirCreateBindingEntry
(
    const uid_t clientUid,         ///< [IN] Client user ID.
    const char* clientInterfacePtr,///< [IN] Client interface name.
    const uid_t serverUid,         ///< [IN] Server user ID.
    const char* serverInterfacePtr ///< [IN] Server interface name.
)
{
    // Parameter check.
    if ((clientInterfacePtr == NULL) || (serverInterfacePtr == NULL))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    le_msg_ProtocolRef_t protocolRef;
    le_msg_SessionRef_t sessionRef;
    le_msg_MessageRef_t msgRef;
    le_sdtp_Msg_t* msgPtr;

    // Try to connect to service directory.
    protocolRef = le_msg_GetProtocolRef(LE_SDTP_PROTOCOL_ID, sizeof(le_sdtp_Msg_t));
    sessionRef = le_msg_CreateSession(protocolRef, LE_SDTP_INTERFACE_NAME);
    if (LE_OK != le_msg_TryOpenSessionSync(sessionRef))
    {
        LE_ERROR("Failed to connect serviceDirectory.");
        le_msg_DeleteSession(sessionRef);
        return LE_FAULT;
    }

    // Construct the request message.
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgPtr->msgType = LE_SDTP_MSGID_BIND;

    // Fill the client interface specifier.
    msgPtr->client = clientUid;
    le_utf8_Copy(msgPtr->clientInterfaceName, clientInterfacePtr,
                 sizeof(msgPtr->clientInterfaceName), NULL);

    // Fill the server interface specifier.
    msgPtr->server = serverUid;
    le_utf8_Copy(msgPtr->serverInterfaceName, serverInterfacePtr,
                 sizeof(msgPtr->serverInterfaceName), NULL);

    // Send the message and wait for a response.
    msgRef = le_msg_RequestSyncResponse(msgRef);

    // If a response message was not received, then the operation failed.
    if (msgRef == NULL)
    {
        LE_ERROR("Communication with Service Directory failed.");
        le_msg_DeleteSession(sessionRef);
        return LE_FAULT;
    }

    le_msg_ReleaseMsg(msgRef);
    le_msg_DeleteSession(sessionRef);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Validate the service information.
 */
//--------------------------------------------------------------------------------------------------
bool IsServiceInfoValid
(
    RpcConnectServiceReq_t* reqPtr,   ///< [IN] Request data of ConnectService.
    RpcClientProxyNode_t* rpcClientNodePtr    ///< [IN] RPC client proxy node.
)
{
    // Parameter check.
    if ((reqPtr == NULL) || (rpcClientNodePtr == NULL) ||
        (rpcClientNodePtr->rpcClientPtr == NULL))
    {
        LE_ERROR("Bad parameters.");
        return false;
    }

    RpcClientProxy_t* rpcClientPtr = rpcClientNodePtr->rpcClientPtr;

    // Validate service information.
    if ((strcmp(rpcClientPtr->serviceCfg.user, reqPtr->user) != 0) ||
        (strcmp(rpcClientPtr->serviceCfg.name, reqPtr->name) != 0) ||
        ((strcmp(rpcClientPtr->version, reqPtr->protocolId) != 0) &&
         (strcmp(reqPtr->protocolId, "ANY_VERSION") != 0)) ||
        (rpcClientNodePtr->sysEventId != reqPtr->sysEventId))
    {
        return false;
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Validate the Rx message received, and put the message payload to the global Rx Message buffer.
 */
//--------------------------------------------------------------------------------------------------
static bool IsRxMessageValid
(
    taf_someipSvr_RxMsgRef_t rpcMsgRef,        ///< [IN] RPC Rx message reference.
    RpcClientProxy_t* rpcClientPtr,            ///< [IN] RPC client proxy.
    RpcClientProxyNode_t** rpcClientNodePtrPtr,///< [OUT] RPC client proxy pointer.
    uint16_t* methodIdPtr,                     ///< [OUT] Method ID.
    uint8_t* payloadDataPtr,                   ///< [OUT] Payload data buffer.
    size_t* payloadSizePtr                     ///< [INOUT] Payload data size.
)
{
    le_result_t result;
    uint16_t remoteSystemId;
    uint16_t methodId;
    RpcClientProxyNode_t* rpcClientNodePtr;

    // Parameters check.
    if ((rpcMsgRef == NULL) || (rpcClientPtr == NULL) || (methodIdPtr == NULL) ||
        (payloadDataPtr == NULL) || (payloadSizePtr == NULL))
    {
        LE_ERROR("Bad parameters.");
        return false;
    }

    // Get the remote system ID from the RPC message.
    result = taf_someipSvr_GetClientId(rpcMsgRef, &remoteSystemId);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get the system ID (result='%s').", LE_RESULT_TXT(result));
        return false;
    }

    // Get the method ID from the RPC message.
    result = taf_someipSvr_GetMethodId(rpcMsgRef, &methodId);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get the method ID (result='%s').", LE_RESULT_TXT(result));
        return false;
    }

    // Validate remote system ID by finding corresponding client proxy node.
    rpcClientNodePtr = FindRpcClientNodeBySystemId(remoteSystemId, rpcClientPtr);
    if (rpcClientNodePtr == NULL)
    {
        LE_ERROR("Failed to find the client proxy node for system(ID=0x%x).", remoteSystemId);
        return false;
    }

    // Check if the remote system is subscribed. Not allowed to process any
    // RPC command from a remote system unless the remote system is subscribed.
    if (rpcClientNodePtr->state == RPC_NODE_NOT_CONNECTED)
    {
        LE_ERROR("System(ID=0x%x) is not subscribed yet.", remoteSystemId);
        return false;
    }

    // Only connectService command is allowed in RPC_NODE_CONNECTING state.
    if ((rpcClientNodePtr->state == RPC_NODE_CONNECTING) &&
        (methodId != RPC_METHOD_CONNECT_SERVICE))
    {
        LE_ERROR("System(ID=0x%x) is not connected yet.", remoteSystemId);
        return false;
    }

    // Get the payload data of the Rx Message.
    result = taf_someipSvr_GetPayloadData(rpcMsgRef, payloadDataPtr, payloadSizePtr);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get the payload data.");
        return false;
    }

    *rpcClientNodePtrPtr = rpcClientNodePtr;
    *methodIdPtr = methodId;

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * IPC session closed handler for IPC client proxy session.
 */
//--------------------------------------------------------------------------------------------------
static void ProxySessionCloseHandler
(
    le_msg_SessionRef_t sessionRef, ///< [IN] Session reference
    void*               contextPtr ///< [IN] Context pointer
)
{
    ClientProxySession_t* clientProxySessionPtr = contextPtr;
    LE_ASSERT((clientProxySessionPtr != NULL) &&
              (clientProxySessionPtr->rpcClientNodePtr != NULL) &&
              (clientProxySessionPtr->rpcClientNodePtr->rpcClientPtr != NULL) &&
              (clientProxySessionPtr->sessionRef == sessionRef));

    RpcClientProxyNode_t* rpcClientNodePtr = clientProxySessionPtr->rpcClientNodePtr;
    taf_someipSvr_ServiceRef_t serviceRef =
        clientProxySessionPtr->rpcClientNodePtr->rpcClientPtr->someipServer.serviceRef;
    uint16_t eventId = rpcClientNodePtr->sysEventId;
    uint32_t rpcSessionId = clientProxySessionPtr->rpcSessionId;

    // Remove the proxy session from the list.
    le_dls_Remove(&rpcClientNodePtr->proxySessionList, &clientProxySessionPtr->link);

    // Delete the IPC session associated.
    le_msg_DeleteSession(clientProxySessionPtr->sessionRef);
    clientProxySessionPtr->sessionRef = NULL;

    // Notify peer system that this client proxy session is closed.
    if (rpcClientNodePtr->state == RPC_NODE_CONNECTED)
    {
        rpcProxyMessage_NotifyDeleteSession(serviceRef, eventId, rpcSessionId);
    }
    else
    {
        LE_ERROR("Error state(%d) for events.", rpcClientNodePtr->state);
    }

    // Free the proxy session if no ongoing proxy messages associated.
    if (le_dls_IsEmpty(&clientProxySessionPtr->msgList))
    {
        le_mem_Release(clientProxySessionPtr);
        LE_INFO("Deleted proxy session(ID=0x%x) for interface '%s'.",
                rpcSessionId, rpcClientNodePtr->bindingInterface);
    }

    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * IPC event message handler for IPC client proxy session.
 */
//--------------------------------------------------------------------------------------------------
static void ProxySessionEventMsgHandler
(
    le_msg_MessageRef_t  msgRef,
    void*                contextPtr
)
{
    ClientProxySession_t* clientProxySessionPtr = contextPtr;
    LE_ASSERT((msgRef != NULL) && (clientProxySessionPtr != NULL) &&
              (clientProxySessionPtr->rpcClientNodePtr != NULL) &&
              (clientProxySessionPtr->rpcClientNodePtr->rpcClientPtr != NULL));

    RpcClientProxyNode_t* rpcClientNodePtr = clientProxySessionPtr->rpcClientNodePtr;
    taf_someipSvr_ServiceRef_t serviceRef =
        clientProxySessionPtr->rpcClientNodePtr->rpcClientPtr->someipServer.serviceRef;
    uint16_t eventId = rpcClientNodePtr->sysEventId;
    uint32_t rpcSessionId = clientProxySessionPtr->rpcSessionId;

    if (rpcClientNodePtr->state == RPC_NODE_CONNECTED)
    {
        // Send RPC event message.
        rpcProxyMessage_NotifyMessage(serviceRef, eventId, rpcSessionId, msgRef);
    }
    else
    {
        LE_ERROR("Error state(%d) for events.", rpcClientNodePtr->state);
    }

    // Release the IPC event message.
    le_msg_ReleaseMsg(msgRef);

    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * IPC response message handler for IPC client proxy session.
 */
//--------------------------------------------------------------------------------------------------
static void ProxySessionResponseMsgHandler
(
    le_msg_MessageRef_t  msgRef,
    void*                contextPtr
)
{
    ClientProxyMsg_t* clientProxyMsgPtr = contextPtr;
    LE_ASSERT((clientProxyMsgPtr != NULL) && (clientProxyMsgPtr->clientProxySessionPtr != NULL) &&
              (clientProxyMsgPtr->rpcMsgRef != NULL));

    ClientProxySession_t* clientProxySessionPtr = clientProxyMsgPtr->clientProxySessionPtr;
    uint32_t rpcSessionId = clientProxySessionPtr->rpcSessionId;
    taf_someipSvr_RxMsgRef_t rpcMsgRef = clientProxyMsgPtr->rpcMsgRef;

    // Remove the proxy message from the list and free it.
    le_dls_Remove(&clientProxySessionPtr->msgList, &clientProxyMsgPtr->link);
    le_mem_Release(clientProxyMsgPtr);

    if (msgRef != NULL)
    {
        // Send RPC message response.
        rpcProxyMessage_ResponseMessage(rpcMsgRef, rpcSessionId, msgRef);

        // Release the IPC response message.
        le_msg_ReleaseMsg(msgRef);
    }
    else
    {
        // Session already closed.
        LE_WARN("No session for response.");

        // Send RPC error response.
        rpcProxyMessage_ResponseError(rpcMsgRef);
    }

    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC receive message Handler on RPC client proxy side.
 */
//--------------------------------------------------------------------------------------------------
static void RpcRxMessageHandler
(
    taf_someipSvr_RxMsgRef_t rpcMsgRef,
    void* contextPtr
)
{
    uint16_t methodId;
    size_t rxMessageSize = TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE;
    RpcClientProxy_t* rpcClientPtr = (RpcClientProxy_t*)contextPtr;
    RpcClientProxyNode_t* rpcClientNodePtr;

    // Validate the Rx message and save it in the global message buffer if validation passed.
    if (!IsRxMessageValid(rpcMsgRef, rpcClientPtr, &rpcClientNodePtr, &methodId,
                           RxMessageBuffer, &rxMessageSize))
    {
        LE_ERROR("Invalid rpcMsgRef(%p).", rpcMsgRef);

        // Send a RPC generic error response if validation failed.
        rpcProxyMessage_ResponseError(rpcMsgRef);
        return;
    }

    // Process the Rx message.
    switch (methodId)
    {
        case RPC_METHOD_CONNECT_SERVICE:
        {
            // Validate the request message size.
            if (rxMessageSize != sizeof(RpcConnectServiceReq_t))
            {
                LE_ERROR("Invalid rxMessageSize(%"PRIuS") of RPC_METHOD_CONNECT_SERVICE",
                         rxMessageSize);

                // Send a RPC generic error response.
                rpcProxyMessage_ResponseError(rpcMsgRef);
                return;
            }

            // Get the Rx message buffer.
            RpcConnectServiceReq_t* reqPtr = (RpcConnectServiceReq_t*)RxMessageBuffer;
            reqPtr->sysEventId = be16toh(reqPtr->sysEventId);

            // Validate service information.
            if (!IsServiceInfoValid(reqPtr, rpcClientNodePtr))
            {
                LE_ERROR("Service(0x%x/0x%x) info mismatch(requestServicer='<%s>.%s',"\
                         "responseService='<%s>.%s').", rpcClientPtr->someipServer.serviceId,
                         rpcClientPtr->someipServer.instanceId, reqPtr->user, reqPtr->name,
                         rpcClientPtr->serviceCfg.user, rpcClientPtr->serviceCfg.name);

                // Send a RPC generic error response.
                rpcProxyMessage_ResponseError(rpcMsgRef);
                return;
            }

            // Create and construct the connectService response.
            RpcConnectServiceResp_t resp;
            memset(&resp, 0, sizeof(RpcConnectServiceResp_t));
            resp.maxMsgSize = rpcClientPtr->maxMessageSize;
            le_utf8_Copy(resp.protocolId, rpcClientPtr->version, sizeof(resp.protocolId), NULL);
            resp.sysEventId = rpcClientNodePtr->sysEventId;

            // Send the response.
            rpcProxyMessage_ResponseConnectService(rpcMsgRef, &resp);

            // Now service is connected.
            if (rpcClientNodePtr->state == RPC_NODE_CONNECTING)
            {
                // Create the internal event.
                ClientNodeEvent_t myEvent;
                myEvent.rpcClientNodePtr = rpcClientNodePtr;
                myEvent.action = ACTION_REMOTE_CONNECTED;
                le_event_Report(RpcClientNodeEvent, &myEvent, sizeof(ClientNodeEvent_t));
            }
        }
        break;


        case RPC_METHOD_CREATE_SESSION:
        {
            // Validate the request message size.
            if (rxMessageSize != 0)
            {
                LE_ERROR("Invalid rxMessageSize(%"PRIuS") of RPC_METHOD_CREATE_SESSION",
                         rxMessageSize);

                // Send a RPC generic error response.
                rpcProxyMessage_ResponseError(rpcMsgRef);
                return;
            }

            // Create an IPC session.
            le_msg_ProtocolRef_t protocolRef =
                le_msg_GetProtocolRef(rpcClientPtr->version, rpcClientPtr->maxMessageSize);
            le_msg_SessionRef_t sessionRef =
                le_msg_CreateSession(protocolRef, rpcClientNodePtr->bindingInterface);

            // Open the IPC session to connect destination service.
            if (LE_OK != le_msg_TryOpenSessionSync(sessionRef))
            {
                LE_ERROR("Failed to open IPC client proxy session.");
                le_msg_DeleteSession(sessionRef);

                // Send a RPC generic error response.
                rpcProxyMessage_ResponseError(rpcMsgRef);
                return;
            }

            // Create a corresponding proxy session object.
            ClientProxySession_t* clientProxySessionPtr =
                le_mem_ForceAlloc(LocalClientProxySessionPoolRef);
            memset(clientProxySessionPtr, 0, sizeof(ClientProxySession_t));

            clientProxySessionPtr->sessionRef = sessionRef;
            clientProxySessionPtr->rpcSessionId = AllocateRpcSessionId();
            clientProxySessionPtr->rpcClientNodePtr = rpcClientNodePtr;
            clientProxySessionPtr->msgList = LE_DLS_LIST_INIT;
            clientProxySessionPtr->link = LE_DLS_LINK_INIT;

            // Put the client proxy session into the list.
            le_dls_Queue(&rpcClientNodePtr->proxySessionList, &clientProxySessionPtr->link);

            // Register the IPC event handler for this proxy session.
            le_msg_SetSessionRecvHandler(sessionRef, ProxySessionEventMsgHandler,
                                         (void*)clientProxySessionPtr);

            // Register the IPC session close handler for this proxy session.
            le_msg_SetSessionCloseHandler(sessionRef, ProxySessionCloseHandler,
                                          (void*)clientProxySessionPtr);

            // Send RPC createSession response.
            rpcProxyMessage_ResponseSession(rpcMsgRef, clientProxySessionPtr->rpcSessionId);

            LE_INFO("Created proxy session(ID=0x%x) for interface '%s'" \
                    "(proto='%s', msgSize=%"PRIuS").", clientProxySessionPtr->rpcSessionId,
                    rpcClientNodePtr->bindingInterface, rpcClientPtr->version,
                    rpcClientPtr->maxMessageSize);
        }
        break;


        case RPC_METHOD_DELETE_SESSION:
        {
            // Validate the request message size.
            if (rxMessageSize != sizeof(RpcMessageCommonHeader_t))
            {
                LE_ERROR("Invalid rxMessageSize(%"PRIuS") of RPC_METHOD_DELETE_SESSION",
                         rxMessageSize);

                // Send a RPC generic error response.
                rpcProxyMessage_ResponseError(rpcMsgRef);
                return;
            }

            // Get the Rx message buffer.
            RpcMessageCommonHeader_t* reqPtr = (RpcMessageCommonHeader_t*)RxMessageBuffer;
            uint32_t rpcSessionId = be32toh(reqPtr->rpcSessionId);

            // Find the client proxy session by session ID.
            ClientProxySession_t* clientProxySessionPtr =
                FindClientProxySessionBySessionId(rpcSessionId, rpcClientNodePtr);
            if (clientProxySessionPtr == NULL)
            {
                LE_ERROR("Failed to find client proxy session for rpcSessionID(0x%x).",
                         rpcSessionId);

                // Send a RPC generic error response.
                rpcProxyMessage_ResponseError(rpcMsgRef);
                return;
            }

            // Remove the proxy session from the list.
            le_dls_Remove(&rpcClientNodePtr->proxySessionList, &clientProxySessionPtr->link);

            // Delete the IPC session associated.
            le_msg_DeleteSession(clientProxySessionPtr->sessionRef);
            clientProxySessionPtr->sessionRef = NULL;

            // Free the proxy session if no ongoing proxy messages associated.
            if (le_dls_IsEmpty(&clientProxySessionPtr->msgList))
            {
                le_mem_Release(clientProxySessionPtr);
                LE_INFO("Deleted proxy session(ID=0x%x) for interface '%s'.",
                        rpcSessionId, rpcClientNodePtr->bindingInterface);
            }

            // Send RPC DeleteSession response.
            rpcProxyMessage_ResponseSession(rpcMsgRef, rpcSessionId);
        }
        break;


        case RPC_METHOD_MESSAGE:
        {
            // Validate the request message size.
            if (rxMessageSize != sizeof(RpcMessageCommonHeader_t) + rpcClientPtr->maxMessageSize)
            {
                LE_ERROR("Invalid rxMessageSize(%"PRIuS").", rxMessageSize);

                // Send a RPC generic error response.
                rpcProxyMessage_ResponseError(rpcMsgRef);
                return;
            }

            // Get the Rx message buffer.
            RpcMessageCommonHeader_t* reqPtr = (RpcMessageCommonHeader_t*)RxMessageBuffer;
            uint32_t rpcSessionId = be32toh(reqPtr->rpcSessionId);

            // Find the client proxy session by session ID.
            ClientProxySession_t* clientProxySessionPtr =
                FindClientProxySessionBySessionId(rpcSessionId, rpcClientNodePtr);

            // Check if the proxy session exists.
            if (clientProxySessionPtr == NULL)
            {
                LE_ERROR("Invalid rpcSession ID(0x%x).", rpcSessionId);

                // Send a RPC generic error response.
                rpcProxyMessage_ResponseError(rpcMsgRef);
                return;
            }

            // Check if associated IPC session exists.
            if (clientProxySessionPtr->sessionRef == NULL)
            {
                LE_ERROR("IpcSession for rpcSession ID(0x%x) is already closed.", rpcSessionId);

                // Send a RPC generic error response.
                rpcProxyMessage_ResponseError(rpcMsgRef);
                return;
            }

            // Create an IPC request message for this RPC request message.
            le_msg_MessageRef_t msgRef = le_msg_CreateMsg(clientProxySessionPtr->sessionRef);

            // Sanity check of the message size.
            size_t dataSize = le_msg_GetMaxPayloadSize(msgRef);
            if (rpcClientPtr->maxMessageSize != dataSize)
            {
                LE_ERROR("Invalid IPC message size(%"PRIuS").", dataSize);
                le_msg_ReleaseMsg(msgRef);

                // Send a RPC generic error response.
                rpcProxyMessage_ResponseError(rpcMsgRef);
                return;
            }

            // Create a proxy message for this RPC message.
            ClientProxyMsg_t* clientProxyMsgPtr = le_mem_ForceAlloc(ClientProxyMsgPoolRef);
            memset(clientProxyMsgPtr, 0, sizeof(ClientProxyMsg_t));

            clientProxyMsgPtr->rpcMsgRef = rpcMsgRef;
            clientProxyMsgPtr->clientProxySessionPtr = clientProxySessionPtr;
            clientProxyMsgPtr->link = LE_DLS_LINK_INIT;

            // Save the proxy message into the list.
            le_dls_Queue(&clientProxySessionPtr->msgList, &clientProxyMsgPtr->link);

            // Copy the RPC message into IPC message buffer.
            uint8_t* dataPtr = le_msg_GetPayloadPtr(msgRef);
            memcpy(dataPtr, RxMessageBuffer + sizeof(RpcMessageCommonHeader_t), dataSize);

            // Send the IPC request, save the corresponding rpcMsgRef in contextPtr
            // So that we can send back the matched RPC response to remote once
            // the IPC response is received.
            le_msg_RequestResponse(msgRef, ProxySessionResponseMsgHandler,
                                   (void*)clientProxyMsgPtr);
        }
        break;

        default:
            LE_FATAL("Unknown methodId(%d).", methodId);
        break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Remote system subscription handler.
 */
//--------------------------------------------------------------------------------------------------
static void RpcSubscriptionHandler
(
    taf_someipSvr_ServiceRef_t serviceRef,
    uint16_t sysEventId,
    bool isSubscribed,
    void* contextPtr
)
{
    RpcClientProxyNode_t* rpcClientNodePtr = contextPtr;

    // Sanity check for parameters.
    LE_ASSERT((rpcClientNodePtr != NULL) && (rpcClientNodePtr->rpcClientPtr != NULL) &&
              (rpcClientNodePtr->rpcClientPtr->someipServer.serviceRef == serviceRef) &&
              (rpcClientNodePtr->sysEventId == sysEventId));

    // Create and send the internal event.
    ClientNodeEvent_t myEvent;
    myEvent.rpcClientNodePtr = rpcClientNodePtr;
    myEvent.action = isSubscribed ? ACTION_REMOTE_SUBSCRIBED : ACTION_REMOTE_UNSUBSCRIBED;
    le_event_Report(RpcClientNodeEvent, &myEvent, sizeof(ClientNodeEvent_t));
}


//--------------------------------------------------------------------------------------------------
/**
 * Enable the proxy node.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EnableProxyNode
(
    RpcClientProxyNode_t* nodePtr
)
{
    // Parameter check.
    if (nodePtr == NULL)
    {
        LE_ERROR("nodePtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    RpcClientProxy_t* rpcClientPtr = nodePtr->rpcClientPtr;
    if (rpcClientPtr == NULL)
    {
        LE_ERROR("rpcClientPtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    uint16_t systemId = rpcProxy_GetRemoteSystemId(nodePtr->systemRef);
    SomeipServer_t* someipServerPtr = &rpcClientPtr->someipServer;
    uint16_t serviceId = someipServerPtr->serviceId;
    uint16_t instanceId = someipServerPtr->instanceId;

    // Check if service is offered.
    if (!someipServerPtr->isOffered)
    {
        LE_ERROR("Service(0x%x/0x%x) is not offered.", serviceId, instanceId);
        return LE_NOT_PERMITTED;
    }

    // Check if associated system is registered.
    if (nodePtr->enabled)
    {
        LE_WARN("Remote system(id=0x%x) for service(0x%x/0x%x) is already registered.",
                systemId, serviceId, instanceId);
        return LE_DUPLICATE;
    }

    // Enable the RPC events and event group and then offer the events.
    taf_someipSvr_ServiceRef_t serviceRef = someipServerPtr->serviceRef;
    uint16_t groupId = nodePtr->sysEventId;
    uint16_t eventId = nodePtr->sysEventId;

    if ((LE_OK != taf_someipSvr_EnableEvent(serviceRef, eventId, groupId)) ||
        (LE_OK != taf_someipSvr_OfferEvent(serviceRef, eventId)))
    {
        LE_ERROR("Failed to enable RPC events of service(0x%x/0x%x) for remote system(id=%d).",
                 serviceId, instanceId, systemId);
        return LE_FAULT;
    }

    // Add a subscription Handler for the remote system.
    taf_someipSvr_SubscriptionHandlerRef_t handlerRef;
    handlerRef = taf_someipSvr_AddSubscriptionHandler(serviceRef, groupId,
                                                      RpcSubscriptionHandler,
                                                      (void*)nodePtr);
    if (handlerRef == NULL)
    {
        LE_ERROR("Failed to register remote system(id=0x%x) for service(0x%x/0x%x).",
                 systemId, serviceId, instanceId);
        return LE_FAULT;
    }

    // Finally enable this remote system.
    nodePtr->subsHandlerRef = handlerRef;
    nodePtr->enabled = true;

    rpcClientProxy_CreateBinding(nodePtr);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Offer a TelAF RPC service remotely.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t OfferRpcService
(
    RpcClientProxy_t* rpcClientProxyPtr               ///< [IN] RPC client proxy pointer.
)
{
    // Parameter check.
    if (rpcClientProxyPtr == NULL)
    {
        LE_ERROR("rpcClientProxyPtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    SomeipServer_t* someipServerPtr = &rpcClientProxyPtr->someipServer;
    uint16_t serviceId = someipServerPtr->serviceId;
    uint16_t instanceId = someipServerPtr->instanceId;
    taf_someipSvr_ServiceRef_t serviceRef;
    taf_someipSvr_RxMsgHandlerRef_t rxMsgHanderRef;

    // Check if TelAF RPC service info is retrieved.
    if ((rpcClientProxyPtr->version[0] == '\0') || (rpcClientProxyPtr->maxMessageSize == 0))
    {
        LE_ERROR("RPC service(0x%x/0x%x) info is not retrieved.", serviceId, instanceId);
        return LE_NOT_PERMITTED;
    }

    // Check if the service is already offered.
    if (someipServerPtr->isOffered)
    {
        LE_ERROR("RPC service (0x%x/0x%x) is already offered.", serviceId, instanceId);
        return LE_DUPLICATE;
    }

    // Get the SOME/IP server service reference.
    if (rpcProxyConfig_GetRoutingName() == NULL)
    {
        serviceRef = taf_someipSvr_GetService(serviceId, instanceId);
    }
    else
    {
        serviceRef = taf_someipSvr_GetServiceEx(serviceId, instanceId,
                                                rpcProxyConfig_GetRoutingName());
    }

    if (serviceRef == NULL)
    {
        LE_ERROR("Failed to get SOME/IP service (0x%x/0x%x).", serviceId, instanceId);
        return LE_FAULT;
    }

    // Set the port.
    if (someipServerPtr->isReliable)
    {
        if (LE_OK != taf_someipSvr_SetServicePort(serviceRef, 0, someipServerPtr->port, true))
        {
            LE_ERROR("Failed to set the TCP port (%d) for service (0x%x/0x%x).",
                     someipServerPtr->port, serviceId, instanceId);
            return LE_FAULT;
        }
    }
    else
    {
        if (LE_OK != taf_someipSvr_SetServicePort(serviceRef, someipServerPtr->port, 0, false))
        {
            LE_ERROR("Failed to set the UDP port (%d) for service (0x%x/0x%x).",
                     someipServerPtr->port, serviceId, instanceId);
            return LE_FAULT;
        }
    }

    // Set the service version.
    if (LE_OK != taf_someipSvr_SetServiceVersion(serviceRef, RPC_MAJOR_VERSION, RPC_MINOR_VERSION))
    {
        LE_ERROR("Failed to set version for service (0x%x/0x%x).", serviceId, instanceId);
        return LE_FAULT;
    }

    // Offer the service.
    if (LE_OK != taf_someipSvr_OfferService(serviceRef))
    {
        LE_ERROR("Failed to offer service(0x%x/0x%x).", serviceId, instanceId);
        return LE_FAULT;
    }

    // Register RPC Rx message Handler.
    rxMsgHanderRef = taf_someipSvr_AddRxMsgHandler(serviceRef, RpcRxMessageHandler,
                                                       (void*)rpcClientProxyPtr);
    if (rxMsgHanderRef == NULL)
    {
        LE_ERROR("Failed to add RxMsgHandler for service(0x%x/0x%x).", serviceId, instanceId);
        return LE_FAULT;
    }

    // Save the server service reference, rx handler and mark the service as "offered".
    someipServerPtr->serviceRef = serviceRef;
    someipServerPtr->rxMsgHandlerRef = rxMsgHanderRef;
    someipServerPtr->isOffered = true;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * The handler for service availability check timer.
 */
//--------------------------------------------------------------------------------------------------
static void ServiceAvailabilityCheckHandler
(
    le_timer_Ref_t timerRef
)
{
    RpcClientProxy_t* rpcClientPtr = le_timer_GetContextPtr(timerRef);
    LE_ASSERT(rpcClientPtr != NULL);

    const char* userPtr = rpcClientPtr->serviceCfg.user;
    const char* namePtr = rpcClientPtr->serviceCfg.name;
    const char* protoIdPtr = rpcClientPtr->serviceCfg.protocolId;
    ServiceInfo_t serviceInfo;
    uid_t uid;

    // Check if RPC client proxy is already enabled.
    if (rpcClientPtr->someipServer.isOffered)
    {
        LE_WARN("service(0x%x/0x%x) is already offered.",
                rpcClientPtr->someipServer.serviceId,
                rpcClientPtr->someipServer.instanceId);

        le_timer_Delete(timerRef);
        rpcClientPtr->timerRef = NULL;
        return;
    }

    // Get the user ID of the user name.
    if (user_GetUid(userPtr, &uid) != LE_OK)
    {
        LE_ERROR("Failed to get the uid of user '%s'.", userPtr);

        le_timer_Delete(timerRef);
        rpcClientPtr->timerRef = NULL;
        return;
    }

    // Check if the TelAF RPC service of this client proxy is up and running and
    // get the service info before offering the service.
    memset(&serviceInfo, 0, sizeof(serviceInfo));
    if (LE_OK != SdirGetServiceInfo(uid, namePtr, protoIdPtr, &serviceInfo))
    {
        LE_WARN("Service '<%s>.%s' not running, continue to monitor.", userPtr, namePtr);
        return;
    }

    // Validate ths MaxMessage size of the service. It shall be less than
    // TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE.
    if (serviceInfo.maxPayloadSize > TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE)
    {
       LE_WARN("Service '<%s>.%s' has too long payload message size(%"PRIxS").",
               userPtr, namePtr, serviceInfo.maxPayloadSize);
       return;
    }

    // Save the service version and maxMessage size.
    le_utf8_Copy(rpcClientPtr->version, serviceInfo.version,
                 sizeof(rpcClientPtr->version), NULL);
    rpcClientPtr->maxMessageSize = serviceInfo.maxPayloadSize;

    LE_INFO("Service '<%s>.%s' (version='%s' maxMsgSize=%"PRIxS") is now running.",
            userPtr, namePtr, rpcClientPtr->version, rpcClientPtr->maxMessageSize);
    if (LE_OK != OfferRpcService(rpcClientPtr))
    {
        LE_ERROR("Failed to offer service.");
        le_timer_Delete(timerRef);
        rpcClientPtr->timerRef = NULL;
        return;
    }

    // Enable all nodes under this RPC proxy.
    le_dls_Link_t* linkPtr = le_dls_Peek(&rpcClientPtr->rpcClientNodeList);
    while (linkPtr != NULL)
    {
        RpcClientProxyNode_t* nodePtr = CONTAINER_OF(linkPtr, RpcClientProxyNode_t, link);
        if (LE_OK != EnableProxyNode(nodePtr))
        {
            LE_ERROR("Failed to enabled Remote system (id=0x%x) for service(0x%x/0x%x).",
                      rpcProxy_GetRemoteSystemId(nodePtr->systemRef),
                      rpcClientPtr->someipServer.serviceId,
                      rpcClientPtr->someipServer.instanceId);
        }
        else
        {
            LE_INFO("Enabled Remote system (id=0x%x) for service(0x%x/0x%x).",
                     rpcProxy_GetRemoteSystemId(nodePtr->systemRef),
                     rpcClientPtr->someipServer.serviceId,
                     rpcClientPtr->someipServer.instanceId);
        }

        linkPtr = le_dls_PeekNext(&rpcClientPtr->rpcClientNodeList, linkPtr);
    }

    le_timer_Delete(timerRef);
    rpcClientPtr->timerRef = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Start a RPC client proxy.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcClientProxy_Start
(
    RpcClientProxyNode_Ref_t nodeRef                  ///< [IN] RPC client proxy node reference.
)
{
    // Parameter check.
    if (nodeRef == NULL)
    {
        LE_ERROR("nodeRef is NULL.");
        return LE_BAD_PARAMETER;
    }

    RpcClientProxyNode_t* nodePtr = nodeRef;
    SystemId_t systemId = rpcProxy_GetRemoteSystemId(nodePtr->systemRef);
    RpcClientProxy_t* rpcClientPtr = nodePtr->rpcClientPtr;
    if (rpcClientPtr == NULL)
    {
        LE_ERROR("rpcClientPtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    SomeipServer_t* someipServerPtr = &rpcClientPtr->someipServer;
    uint16_t serviceId = someipServerPtr->serviceId;
    uint16_t instanceId = someipServerPtr->instanceId;
    const char* userPtr = rpcClientPtr->serviceCfg.user;
    const char* namePtr = rpcClientPtr->serviceCfg.name;
    const char* protoIdPtr = rpcClientPtr->serviceCfg.protocolId;
    ServiceInfo_t serviceInfo;
    uid_t uid;

    // Try to get the service info before offering service. Start a timer to periodically
    // check the availability of the service if the service is not up and running yet.
    if (!someipServerPtr->isOffered)
    {
        if (rpcClientPtr->timerRef != NULL)
        {
            LE_WARN("ServiceAvailabilityCheck timer for Service '<%s>.%s' is running," \
                    " skip client proxy.", userPtr, namePtr);
            return LE_OK;
        }

        // Get the user ID of the user name.
        if (user_GetUid(userPtr, &uid) != LE_OK)
        {
            LE_ERROR("Failed to get the uid of user '%s'.", userPtr);
            return LE_FAULT;
        }

        // Check if the TelAF RPC service of this client proxy is up and running and
        // get the service info before offering the service.
        memset(&serviceInfo, 0, sizeof(serviceInfo));
        if ((LE_OK != SdirGetServiceInfo(uid, namePtr, protoIdPtr, &serviceInfo)) ||
            (serviceInfo.maxPayloadSize > TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE))
        {
            LE_WARN("Service '<%s>.%s' not running, start ServiceAvailabilityCheck timer.",
                    userPtr, namePtr);

            char timerName[LIMIT_MAX_USER_NAME_BYTES+LIMIT_MAX_IPC_INTERFACE_NAME_BYTES + 2]={0};
            snprintf(timerName, sizeof(timerName), "<%s>.%s", userPtr, namePtr);

            // Create service check timer.
            rpcClientPtr->timerRef = le_timer_Create(timerName);
            le_timer_SetMsInterval(rpcClientPtr->timerRef, 2000);
            le_timer_SetHandler(rpcClientPtr->timerRef, ServiceAvailabilityCheckHandler);
            le_timer_SetWakeup(rpcClientPtr->timerRef, false);
            le_timer_SetRepeat(rpcClientPtr->timerRef, 0);
            le_timer_SetContextPtr(rpcClientPtr->timerRef, rpcClientPtr);

            // Start the timer.
            le_timer_Start(rpcClientPtr->timerRef);

            return LE_OK;
        }

        // Save the service version and maxMessage size.
        le_utf8_Copy(rpcClientPtr->version, serviceInfo.version,
                     sizeof(rpcClientPtr->version), NULL);
        rpcClientPtr->maxMessageSize = serviceInfo.maxPayloadSize;

        LE_INFO("Service '<%s>.%s' (version='%s' maxMsgSize=%"PRIuS") is now running.",
                userPtr, namePtr, rpcClientPtr->version, rpcClientPtr->maxMessageSize);

        // Offer the SOME/IP service binding to this RPC service.
        if (LE_OK != OfferRpcService(rpcClientPtr))
        {
            return LE_FAULT;
        }
    }

    // Enable the remote system of this proxy node.
    if( LE_OK != EnableProxyNode(nodePtr))
    {
        LE_ERROR("Failed to enable remote system (id=0x%x) for service(0x%x/0x%x).",
                 systemId, serviceId, instanceId);
        return LE_FAULT;
    }

    LE_INFO("Enabled remote system (id=0x%x) for service(0x%x/0x%x).",
            systemId, serviceId, instanceId);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create the binding for a RPC client proxy node.
 */
//--------------------------------------------------------------------------------------------------
void rpcClientProxy_CreateBinding
(
    RpcClientProxyNode_Ref_t nodeRef                  ///< [IN] RPC client proxy node reference.
)
{
    if (nodeRef != NULL)
    {
        RpcClientProxyNode_t* nodePtr = nodeRef;
        RpcClientProxy_t* rpcClientPtr = nodePtr->rpcClientPtr;
        if (nodePtr->enabled && (rpcClientPtr != NULL))
        {
            // Get the user ID of the user name.
            uid_t serverUid;
            if (user_GetUid(rpcClientPtr->serviceCfg.user, &serverUid) != LE_OK)
            {
                LE_ERROR("Failed to get the uid of user '%s'.", rpcClientPtr->serviceCfg.user);
                return;
            }

            // Create a binding entry for this node.
            uid_t myUid = getuid();
            if (LE_OK == SdirCreateBindingEntry(myUid, nodePtr->bindingInterface,
                                        serverUid, rpcClientPtr->serviceCfg.name))
            {
                LE_INFO("Created binding entries for RPC proxy interface(%s) to service(<%s>.%s).",
                         nodePtr->bindingInterface, rpcClientPtr->serviceCfg.user,
                         rpcClientPtr->serviceCfg.name);
            }
        }
    }
}
