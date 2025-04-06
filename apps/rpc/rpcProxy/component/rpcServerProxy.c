/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include "rpcServerProxy.h"
#include "rpcProxyMessaging.h"
#include "sdirToolProtocol.h"


//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP client-service instance common data struct.
 *
 * It's created by RPC server proxy, and TelAF service is one-to-one mapping to SOME/IP service.
 */
//--------------------------------------------------------------------------------------------------
typedef struct SomeipClient
{
    uint16_t serviceId;                            ///< Service ID
    uint16_t instanceId;                           ///< Service instance ID
    taf_someipClnt_ServiceRef_t serviceRef;        ///< Someip client service reference
    taf_someipClnt_StateChangeHandlerRef_t stateHandlerRef;///< Someip client state handler ref
    taf_someipClnt_EventMsgHandlerRef_t eventHandlerRef;///< Someip client event handler ref
    taf_someipClnt_State_t serviceState;           ///< Someip service state
    bool isReliable;                               ///< Use TCP if true
    uint16_t responseTimeout;                      ///< Response timeout seconds
}SomeipClient_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC server proxy instance data structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct RpcServerProxy
{
    ProxyNode_t baseClass;                      ///< Proxy type
    ServiceLink_t serviceCfg;                   ///< Service base information
    SomeipClient_t someipClient;                ///< Someip client bind to this RPC server proxy
    size_t maxMessageSize;                      ///< Max payload message size
    char version[LIMIT_MAX_PROTOCOL_ID_BYTES];  ///< RPC service version
    char bindingInterface[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES]; ///< Interface name for this service
    uint16_t sysEventId;                        ///< SOME/IP event ID assigned to my system
    bool isRequested;                           ///< If SOME/IP service is requested
    RpcNodeState_t state;                       ///< RPC node state
    le_timer_Ref_t timerRef;                    ///< Generic timer
    RpcRemoteSystem_Ref_t systemRef;            ///< Remote system bind to this RPC server proxy
    le_msg_ServiceRef_t proxyServiceRef;        ///< Local IPC proxy service reference
    le_dls_List_t proxySessionList;             ///< Local IPC server proxy session list
    le_dls_Link_t link;                         ///< Link to the global list
}RpcServerProxy_t;


//--------------------------------------------------------------------------------------------------
/**
 * Local IPC client-server proxy session data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct ProxySession
{
    RpcServerProxy_t* rpcServerPtr;       ///< RPC server proxy of this proxy session
    le_msg_SessionRef_t sessionRef;       ///< Local IPC server session reference
    uint32_t rpcSessionId;                ///< RPC session ID assigned to this proxy session
    bool isRemoteClosed;                  ///< Remote session is closed.
    le_dls_List_t pendingMsgList;         ///< Pending IPC message list
    le_dls_List_t msgList;                ///< Ongoing IPC message list
    le_dls_Link_t link;                   ///< Link to RPC server proxy
}ProxySession_t;


//--------------------------------------------------------------------------------------------------
/**
 * Local IPC message data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct LocalMsg
{
    ProxySession_t* proxySessionPtr;      ///< proxy session
    le_msg_MessageRef_t msgRef;           ///< IPC message reference
    le_dls_Link_t link;                   ///< Link to proxy session
}LocalMsg_t;


//--------------------------------------------------------------------------------------------------
/**
 * Action in internal event
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    ACTION_CONNECT_SERVICE,               ///< Connect service
    ACTION_RECONNECT_SERVICE,             ///< Reconnect service
    ACTION_DISCONNECT_SERVICE             ///< Disconnect service

}Action_t;


//--------------------------------------------------------------------------------------------------
/**
 * Internal event data struct
 */
//--------------------------------------------------------------------------------------------------
typedef struct ServerProxyEvent
{
    Action_t action;                      ///< Action
    RpcServerProxy_t* rpcServerPtr;       ///< RPC server proxy pointer
}ServerProxyEvent_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC server proxy instance pool.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(RpcServerProxyPool, (RPC_MAX_SYSTEMS*RPC_MAX_SERVICES),
                                  sizeof(RpcServerProxy_t));

static le_mem_PoolRef_t RpcServerProxyPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Local IPC server proxy session pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ProxySessionPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Local IPC message pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t MsgPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * RPC server proxy event and handler.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t RpcServerEvent = NULL;
le_event_HandlerRef_t RpcServerEventHandlerRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * RPC server proxy list.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t RpcServerProxyList = LE_DLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * Find the proxy session by IPC session Reference.
 */
//--------------------------------------------------------------------------------------------------
static ProxySession_t* FindProxySessionByRef
(
    le_msg_SessionRef_t sessionRef, ///< [IN] IPC session reference
    RpcServerProxy_t* rpcServerPtr  ///< [IN] RPC server proxy pointer
)
{
    if ((sessionRef == NULL) || (rpcServerPtr == NULL))
    {
        LE_ERROR("Bad parameter.");
        return NULL;
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(&rpcServerPtr->proxySessionList);

    while (linkPtr != NULL)
    {
        ProxySession_t* proxySessionPtr = CONTAINER_OF(linkPtr, ProxySession_t, link);
        LE_ASSERT((proxySessionPtr != NULL) && (proxySessionPtr->rpcServerPtr == rpcServerPtr));

        if (proxySessionPtr->sessionRef == sessionRef)
        {
            return proxySessionPtr;
        }

        linkPtr = le_dls_PeekNext(&rpcServerPtr->proxySessionList, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find the proxy session by RPC session ID.
 */
//--------------------------------------------------------------------------------------------------
static ProxySession_t* FindProxySessionById
(
    uint32_t rpcSessionId,          ///< [IN] RPC session Id
    RpcServerProxy_t* rpcServerPtr  ///< [IN] RPC server proxy pointer
)
{
    if ((rpcSessionId == 0) || (rpcServerPtr == NULL))
    {
        LE_ERROR("Bad parameter.");
        return NULL;
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(&rpcServerPtr->proxySessionList);

    while (linkPtr != NULL)
    {
        ProxySession_t* proxySessionPtr = CONTAINER_OF(linkPtr, ProxySession_t, link);
        LE_ASSERT((proxySessionPtr != NULL) && (proxySessionPtr->rpcServerPtr == rpcServerPtr));

        if (proxySessionPtr->rpcSessionId == rpcSessionId)
        {
            return proxySessionPtr;
        }

        linkPtr = le_dls_PeekNext(&rpcServerPtr->proxySessionList, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC delete session response handler.
 */
//--------------------------------------------------------------------------------------------------
static void RpcDeleteSessionRespHandler
(
    le_result_t result,
    RpcSessionResp_t respType,
    uint32_t sessionId,
    void* contextPtr
)
{
    ProxySession_t* proxySessionPtr = contextPtr;
    LE_ASSERT(proxySessionPtr != NULL);

    switch (respType)
    {
        case RPC_SESSION_DELETE_RESPONSE:
        {
            LE_INFO("Deleted RPC session(ID=0x%x rID=0x%x, result='%s')",
                    proxySessionPtr->rpcSessionId, sessionId, LE_RESULT_TXT(result));

            if (result == LE_OK)
            {
                // Sanity check that the local proxy session must be closed before sending
                // RPC DeleteSession command.
                LE_ASSERT((proxySessionPtr->sessionRef == NULL) &&
                          (proxySessionPtr->rpcSessionId == sessionId) &&
                          (le_dls_IsEmpty(&proxySessionPtr->pendingMsgList)) &&
                          (le_dls_IsEmpty(&proxySessionPtr->msgList)))
            }

            // Release the RPC proxy session.
            le_mem_Release(proxySessionPtr);
        }
        break;

        default:
            LE_ERROR("Unknown respType(%d).", respType);
        break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC message command response handler.
 */
//--------------------------------------------------------------------------------------------------
static void RpcMessageRespHandler
(
    le_result_t result,
    uint32_t rpcSessionId,
    const uint8_t* respPtr,
    size_t respSize,
    void* contextPtr
)
{
    LE_ASSERT(contextPtr != NULL);

    LocalMsg_t* localMsgPtr = contextPtr;
    ProxySession_t* proxySessionPtr = localMsgPtr->proxySessionPtr;
    le_msg_MessageRef_t msgRef = localMsgPtr->msgRef;
    LE_ASSERT((proxySessionPtr != NULL) && (msgRef != NULL))

    RpcServerProxy_t* rpcServerPtr = proxySessionPtr->rpcServerPtr;
    LE_ASSERT(rpcServerPtr != NULL);

    if (result == LE_OK)
    {
        LE_ASSERT(proxySessionPtr->rpcSessionId == rpcSessionId);
    }

    taf_someipClnt_ServiceRef_t serviceRef = rpcServerPtr->someipClient.serviceRef;
    bool isReliable = rpcServerPtr->someipClient.isReliable;

    // Remove the local message from the list and release it.
    le_dls_Remove(&proxySessionPtr->msgList, &localMsgPtr->link);
    le_mem_Release(localMsgPtr);

    if (result == LE_OK)
    {
        if (proxySessionPtr->sessionRef != NULL)
        {
            // Copy back the repsonse into the message buffer of original msgRef
            // and send back to real client.
            uint8_t* respDataPtr = le_msg_GetPayloadPtr(msgRef);
            memcpy(respDataPtr, respPtr, respSize);
            le_msg_Respond(msgRef);
        }
        else
        {
            // Local proxy session is already closed, release the local message
            // and send the RPC DeleteSession command if needed.
            le_msg_ReleaseMsg(msgRef);
            if (le_dls_IsEmpty(&proxySessionPtr->msgList))
            {
                if (proxySessionPtr->isRemoteClosed)
                {
                   le_mem_Release(proxySessionPtr);
                }
                else
                {
                    // Send a RPC DeleteSession command to remote RPC proxy since local proxy
                    // session is closed and all ongoing messages are handled.
                    rpcProxyMessage_DeleteSessionRequestResponse(serviceRef,
                                                                 isReliable,
                                                                 proxySessionPtr->rpcSessionId,
                                                                 RpcDeleteSessionRespHandler,
                                                                 (void*)proxySessionPtr);
                }
            }
        }
    }
    else
    {
        LE_ERROR("Failed to receieve the IPC message response (result='%s') for proxy session(%p)",
                 LE_RESULT_TXT(result), proxySessionPtr->sessionRef);

        if (proxySessionPtr->sessionRef != NULL)
        {
            // Assume remote proxy session is closed,
            // Close the local proxy session.
            proxySessionPtr->isRemoteClosed = true;
            le_msg_CloseSession(proxySessionPtr->sessionRef);
            le_msg_ReleaseMsg(msgRef);
        }
        else
        {
            le_msg_ReleaseMsg(msgRef);
            if (le_dls_IsEmpty(&proxySessionPtr->msgList))
            {
                if (proxySessionPtr->isRemoteClosed)
                {
                    le_mem_Release(proxySessionPtr);
                }
                else
                {
                    // Send a RPC DeleteSession command to remote RPC proxy since local proxy
                    // session is closed and all ongoing messages are handled.
                    rpcProxyMessage_DeleteSessionRequestResponse(serviceRef,
                                                                 isReliable,
                                                                 proxySessionPtr->rpcSessionId,
                                                                 RpcDeleteSessionRespHandler,
                                                                 (void*)proxySessionPtr);
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC create session response handler.
 */
//--------------------------------------------------------------------------------------------------
static void RpcCreateSessionRespHandler
(
    le_result_t result,
    RpcSessionResp_t respType,
    uint32_t sessionId,
    void* contextPtr
)
{
    ProxySession_t* proxySessionPtr = contextPtr;
    LE_ASSERT(proxySessionPtr != NULL);

    RpcServerProxy_t* rpcServerPtr = proxySessionPtr->rpcServerPtr;
    LE_ASSERT(rpcServerPtr != NULL);

    le_dls_Link_t* linkPtr;
    LocalMsg_t* localMsgPtr;
    taf_someipClnt_ServiceRef_t serviceRef = rpcServerPtr->someipClient.serviceRef;
    bool isReliable = rpcServerPtr->someipClient.isReliable;
    uint32_t timeoutSecs = (uint32_t)rpcServerPtr->someipClient.responseTimeout;

    switch (respType)
    {
        case RPC_SESSION_CREATE_RESPONSE:
        {
            if (result != LE_OK)
            {
                LE_ERROR("Failed to create RPC session for proxy session(%p).",
                         proxySessionPtr->sessionRef);

                if (proxySessionPtr->sessionRef != NULL)
                {
                    // Assume remote proxy session is closed,
                    // close and release the local proxy session.
                    proxySessionPtr->isRemoteClosed = true;
                    le_msg_CloseSession(proxySessionPtr->sessionRef);
                }
                else
                {
                    // Local proxy session is already closed, only release it.
                    le_mem_Release(proxySessionPtr);
                }
            }
            else
            {
                LE_INFO("Created RPC session(ID=0x%x) for proxy session(%p).",
                        sessionId, proxySessionPtr->sessionRef);

                if (proxySessionPtr->sessionRef != NULL)
                {
                    if (sessionId == 0)
                    {
                        LE_ERROR("Invaild rpcSessionId received.");

                        // Assume remote proxy session is closed,
                        // close and release the local proxy session.
                        proxySessionPtr->isRemoteClosed = true;
                        le_msg_CloseSession(proxySessionPtr->sessionRef);
                    }
                    else
                    {
                        // Save the rpcSession ID.
                        proxySessionPtr->rpcSessionId = sessionId;

                        // Pulish all pending messages in pendingMsg list if have.
                        linkPtr = le_dls_Pop(&proxySessionPtr->pendingMsgList);
                        while (linkPtr != NULL)
                        {
                            localMsgPtr = CONTAINER_OF(linkPtr, LocalMsg_t, link);
                            LE_ASSERT((localMsgPtr != NULL) &&
                                      (localMsgPtr->proxySessionPtr == proxySessionPtr));

                            // Create and send a RPC Message command for each pending message
                            // to remote RPC proxy.
                            rpcProxyMessage_MessageRequestResponse(serviceRef,
                                                                   isReliable,
                                                                   timeoutSecs,
                                                                   sessionId,
                                                                   localMsgPtr->msgRef,
                                                                   RpcMessageRespHandler,
                                                                   (void*)localMsgPtr);

                            // Move the pending message to ongoingMsg list to track the response.
                            le_dls_Queue(&proxySessionPtr->msgList, &localMsgPtr->link);

                            linkPtr = le_dls_Pop(&proxySessionPtr->pendingMsgList);
                        }
                    }
                }
                else
                {
                    // Save the RPC sessionId anyway since we still need to validate the RPC
                    // sessionId in RpcDeleteSessionRespHandler().
                    proxySessionPtr->rpcSessionId = sessionId;

                    // Send a RPC DeleteSession command to remote RPC proxy to also delete
                    // remote proxy session since local proxy session is already closed.
                    rpcProxyMessage_DeleteSessionRequestResponse(serviceRef,
                                                                 isReliable,
                                                                 sessionId,
                                                                 RpcDeleteSessionRespHandler,
                                                                 (void*)proxySessionPtr);
                }
            }
        }
        break;

        default:
            LE_ERROR("Unknown respType(%d).", respType);
        break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Client-server proxy session open handler on RPC server proxy side.
 */
//--------------------------------------------------------------------------------------------------
static void ProxySessionOpenHandler
(
    le_msg_SessionRef_t  sessionRef,
    void*                contextPtr
)
{
    RpcServerProxy_t* rpcServerPtr = contextPtr;
    LE_ASSERT(sessionRef != NULL && rpcServerPtr != NULL);

    LE_INFO("client-server session(%p) to proxy service(%s) is opened.",
            sessionRef, rpcServerPtr->bindingInterface);

    // Create and save the proxy session.
    ProxySession_t* proxySessionPtr = le_mem_ForceAlloc(ProxySessionPoolRef);
    memset(proxySessionPtr, 0, sizeof(ProxySession_t));

    // Initialize members.
    proxySessionPtr->sessionRef = sessionRef;
    proxySessionPtr->rpcSessionId = 0;
    proxySessionPtr->isRemoteClosed = false;
    proxySessionPtr->rpcServerPtr = rpcServerPtr;
    proxySessionPtr->pendingMsgList = LE_DLS_LIST_INIT;
    proxySessionPtr->msgList = LE_DLS_LIST_INIT;
    proxySessionPtr->link = LE_DLS_LINK_INIT;

    // Add the proxy session into the proxySession list.
    le_dls_Queue(&rpcServerPtr->proxySessionList, &proxySessionPtr->link);

    // Send RPC createSession command to remote RPC proxy.
    taf_someipClnt_ServiceRef_t serviceRef = rpcServerPtr->someipClient.serviceRef;
    bool isReliable = rpcServerPtr->someipClient.isReliable;
    rpcProxyMessage_CreateSessionRequestResponse(serviceRef,
                                                 isReliable,
                                                 RpcCreateSessionRespHandler,
                                                 (void*)proxySessionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Client-server proxy session close handler on RPC server proxy side.
 */
//--------------------------------------------------------------------------------------------------
static void ProxySessionCloseHandler
(
    le_msg_SessionRef_t  sessionRef,
    void*                contextPtr
)
{
    le_dls_Link_t* linkPtr;
    LocalMsg_t* localMsgPtr;
    RpcServerProxy_t* rpcServerPtr = contextPtr;
    LE_ASSERT(sessionRef != NULL && rpcServerPtr != NULL);

    taf_someipClnt_ServiceRef_t serviceRef = rpcServerPtr->someipClient.serviceRef;
    bool isReliable = rpcServerPtr->someipClient.isReliable;
    ProxySession_t* proxySessionPtr = FindProxySessionByRef(sessionRef, rpcServerPtr);

    if (proxySessionPtr == NULL)
    {
        LE_ERROR("Can not find corresponding proxy session.");
        return;
    }

    LE_INFO("client-server session(ID=0x%x) of proxy service(%s) is closed(remoteClosed=%s).",
            proxySessionPtr->rpcSessionId, rpcServerPtr->bindingInterface,
            proxySessionPtr->isRemoteClosed ? "TRUE" : "FALSE");

    // Set sessionRef to NULL, which indicates this proxy session is closed.
    proxySessionPtr->sessionRef = NULL;

    // Delete the proxy session object from the list.
    le_dls_Remove(&rpcServerPtr->proxySessionList, &proxySessionPtr->link);

    if (proxySessionPtr->rpcSessionId == 0)
    {
        // The proxy session is busy processing CreateSession command.
        // Only cleanup all pending messages if have.
        linkPtr = le_dls_Pop(&proxySessionPtr->pendingMsgList);
        while (linkPtr != NULL)
        {
            localMsgPtr = CONTAINER_OF(linkPtr, LocalMsg_t, link);
            LE_ASSERT((localMsgPtr != NULL) && (localMsgPtr->proxySessionPtr == proxySessionPtr));

            le_msg_ReleaseMsg(localMsgPtr->msgRef);
            le_mem_Release(localMsgPtr);

            linkPtr = le_dls_Pop(&proxySessionPtr->pendingMsgList);
        }

        // The session close is triggered by the response handler of RPC CreateSession command.
        // Otherwise the session close is triggered by local client apps, we will release the
        // proxy session once related response handler of CreateSession command is called.
        if (proxySessionPtr->isRemoteClosed)
        {
            le_mem_Release(proxySessionPtr);
        }
    }
    else
    {
        // The RPC session for this proxy session is activated, check if there are
        // ongoing messages in this session.
        if (le_dls_IsEmpty(&proxySessionPtr->msgList))
        {
            if (!proxySessionPtr->isRemoteClosed)
            {
                // Send RPC DeleteSession command to remote RPC proxy if no ongoing messages.
                rpcProxyMessage_DeleteSessionRequestResponse(serviceRef,
                                                             isReliable,
                                                             proxySessionPtr->rpcSessionId,
                                                             RpcDeleteSessionRespHandler,
                                                             (void*)proxySessionPtr);
            }
            else
            {
                le_mem_Release(proxySessionPtr);
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Client-server proxy session message-receive handler on RPC server proxy side.
 */
//--------------------------------------------------------------------------------------------------
static void ProxySessionMsgRecvHandler
(
    le_msg_MessageRef_t msgRef,
    void* contextPtr
)
{
    RpcServerProxy_t* rpcServerPtr = contextPtr;
    LE_ASSERT(msgRef != NULL && rpcServerPtr != NULL);

    taf_someipClnt_ServiceRef_t serviceRef = rpcServerPtr->someipClient.serviceRef;
    bool isReliable = rpcServerPtr->someipClient.isReliable;
    uint32_t timeoutSecs = (uint32_t)rpcServerPtr->someipClient.responseTimeout;

    le_msg_SessionRef_t sessionRef = le_msg_GetSession(msgRef);
    ProxySession_t* proxySessionPtr = FindProxySessionByRef(sessionRef, rpcServerPtr);

    if (proxySessionPtr == NULL)
    {
        // Should never happen.
        LE_ERROR("Unknown msgRef (%p).", msgRef);

        le_msg_CloseSession(sessionRef);
        le_msg_ReleaseMsg(msgRef);

        return;
    }

    // Create a local message object for this IPC message.
    LocalMsg_t* localMsgPtr = le_mem_ForceAlloc(MsgPoolRef);
    memset(localMsgPtr, 0, sizeof(LocalMsg_t));

    localMsgPtr->msgRef = msgRef;
    localMsgPtr->proxySessionPtr = proxySessionPtr;
    localMsgPtr->link = LE_DLS_LINK_INIT;

    // Check if the RPC session for this proxy session is established. Possible RPC sessionID is
    // from 0x00000001 to 0xFFFFFFFF. 0 means the RPC session is not established.
    // If the RPC session is not established simply put the msgRef into the pendingMsg list,
    // otherwise put it into ongoingMsg list and call RPC Message command to send this msgRef via
    // its RPC session.
    if (proxySessionPtr->rpcSessionId == 0)
    {
        le_dls_Queue(&proxySessionPtr->pendingMsgList, &localMsgPtr->link);
    }
    else
    {
        le_dls_Queue(&proxySessionPtr->msgList, &localMsgPtr->link);

        rpcProxyMessage_MessageRequestResponse(serviceRef,
                                               isReliable,
                                               timeoutSecs,
                                               proxySessionPtr->rpcSessionId,
                                               localMsgPtr->msgRef,
                                               RpcMessageRespHandler,
                                               (void*)localMsgPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Advertise a IPC proxy service to represent a Remote RPC service.
 */
//--------------------------------------------------------------------------------------------------
static void AdvertiseService
(
    RpcServerProxy_t* rpcServerPtr                   ///< [IN] RPC server proxy pointer.
)
{
    LE_ASSERT(rpcServerPtr != NULL);

    if (rpcServerPtr->proxyServiceRef != NULL)
    {
        LE_WARN("RPC service (0x%x/0x%x) is already advertised locally.",
                 rpcServerPtr->someipClient.serviceId, rpcServerPtr->someipClient.instanceId);
        return;
    }

    if (rpcServerPtr->state != RPC_NODE_CONNECTED)
    {
        LE_ERROR("RPC service (0x%x/0x%x) is not in RPC_NODE_CONNECTED state.",
                 rpcServerPtr->someipClient.serviceId, rpcServerPtr->someipClient.instanceId);
        return;
    }

    // Create IPC proxy sevice for remote RPC service.
    le_msg_ProtocolRef_t protocolRef =
        le_msg_GetProtocolRef(rpcServerPtr->version, rpcServerPtr->maxMessageSize);
    le_msg_ServiceRef_t serviceRef =
        le_msg_CreateService(protocolRef, rpcServerPtr->bindingInterface);
    le_msg_AdvertiseService(serviceRef);

    // Add handlers for IPC proxy service.
    le_msg_AddServiceOpenHandler(serviceRef, ProxySessionOpenHandler, (void*)rpcServerPtr);
    le_msg_AddServiceCloseHandler(serviceRef, ProxySessionCloseHandler, (void*)rpcServerPtr);
    le_msg_SetServiceRecvHandler(serviceRef, ProxySessionMsgRecvHandler, (void*)rpcServerPtr);

    rpcServerPtr->proxyServiceRef = serviceRef;
    rpcServerPtr->proxySessionList = LE_DLS_LIST_INIT;

    LE_INFO("======= Advertise TelAF RPC Service '%s' (proto='%s', msgSize=%zu) =========",
            rpcServerPtr->bindingInterface, rpcServerPtr->version, rpcServerPtr->maxMessageSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC connect service response handler.
 */
//--------------------------------------------------------------------------------------------------
static void RpcConnectServiceRespHandler
(
    le_result_t result,
    RpcConnectServiceResp_t* respDataPtr,
    void* contextPtr
)
{
    RpcServerProxy_t* rpcServerPtr = contextPtr;
    LE_ASSERT(rpcServerPtr != NULL);

    // Check if this RPC node is still in RPC_NODE_CONNECTING.
    if (rpcServerPtr->state != RPC_NODE_CONNECTING)
    {
        LE_WARN("RPC service (0x%x/0x%x) node state(%s) is not RPC_NODE_CONNECTING.",
                rpcServerPtr->someipClient.serviceId, rpcServerPtr->someipClient.instanceId,
                rpcProxyMessage_GetStateString(rpcServerPtr->state));

        if (rpcServerPtr->state == RPC_NODE_CONNECT_ABORTED)
        {
            rpcServerPtr->state = RPC_NODE_NOT_CONNECTED;
        }

        return;
    }

    ServerProxyEvent_t myEvent;
    memset(&myEvent, 0, sizeof(ServerProxyEvent_t));
    myEvent.rpcServerPtr = (void*)rpcServerPtr;

    // Check the result code of the connect response.
    if (result != LE_OK)
    {
        LE_ERROR("RpcConnectRespHandler for service (0x%x/0x%x) gets failed (result=%s).",
                 rpcServerPtr->someipClient.serviceId,rpcServerPtr->someipClient.instanceId,
                 LE_RESULT_TXT(result));

        // Send internal event to re-connect the service.
        myEvent.action = ACTION_RECONNECT_SERVICE;
        le_event_Report(RpcServerEvent, &myEvent, sizeof(ServerProxyEvent_t));

        return;
    }

    LE_ASSERT(respDataPtr != NULL);

    // Validate the service Info.
    if (((strcmp(rpcServerPtr->serviceCfg.protocolId, "ANY_VERSION") != 0) &&
        (strcmp(rpcServerPtr->serviceCfg.protocolId, respDataPtr->protocolId) != 0)) ||
        (rpcServerPtr->sysEventId != respDataPtr->sysEventId))
    {
        LE_ERROR("Service(0x%x/0x%x) mismatch(reqVer='%s' evt=0x%x, respVer='%s' evt=0x%x).",
                 rpcServerPtr->someipClient.serviceId, rpcServerPtr->someipClient.instanceId,
                 rpcServerPtr->serviceCfg.protocolId, rpcServerPtr->sysEventId,
                 respDataPtr->protocolId, respDataPtr->sysEventId);

        // Send internal event to re-connect the service.
        myEvent.action = ACTION_RECONNECT_SERVICE;
        le_event_Report(RpcServerEvent, &myEvent, sizeof(ServerProxyEvent_t));

        return;
    }

    // Save the real service version and maxMessageSize.
    memset(rpcServerPtr->version, 0, sizeof(rpcServerPtr->version));
    le_utf8_Copy(rpcServerPtr->version, respDataPtr->protocolId,
                 sizeof(rpcServerPtr->version), NULL);
    rpcServerPtr->maxMessageSize = respDataPtr->maxMsgSize;

    // Service is connected.
    rpcServerPtr->state = RPC_NODE_CONNECTED;

    // Advertise service locally.
    AdvertiseService(rpcServerPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Connect a remote RPC service by sending the getServiceInfo command and validating the service
 * information in the response message.
 */
//--------------------------------------------------------------------------------------------------
void ConnectService
(
    RpcServerProxy_t* rpcServerPtr                     ///< [IN] RPC server proxy pointer.
)
{
    LE_ASSERT(rpcServerPtr != NULL);
    LE_ASSERT(rpcServerPtr->isRequested);

    RpcConnectServiceReq_t serviceInfo;
    memset(&serviceInfo, 0, sizeof(RpcConnectServiceReq_t));

    // Set the service Info request data.
    le_utf8_Copy(serviceInfo.user, rpcServerPtr->serviceCfg.user,
                 sizeof(serviceInfo.user), NULL);
    le_utf8_Copy(serviceInfo.name, rpcServerPtr->serviceCfg.name,
                 sizeof(serviceInfo.name), NULL);
    le_utf8_Copy(serviceInfo.protocolId, rpcServerPtr->serviceCfg.protocolId,
                 sizeof(serviceInfo.protocolId), NULL);

    serviceInfo.sysEventId = rpcServerPtr->sysEventId;

    // Call RPC get service info command.
    rpcProxyMessage_ConnectServiceRequestResponse(rpcServerPtr->someipClient.serviceRef,
        rpcServerPtr->someipClient.isReliable, &serviceInfo,
        RpcConnectServiceRespHandler, (void*)rpcServerPtr);

    LE_INFO("Connecting RPC service(0x%x/0x%x).",
            rpcServerPtr->someipClient.serviceId,rpcServerPtr->someipClient.instanceId);
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC re-connect timer handler. This will trigger the connectService() once the timer gets expired.
 */
//--------------------------------------------------------------------------------------------------
static void ReConnectTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    RpcServerProxy_t* rpcServerPtr = le_timer_GetContextPtr(timerRef);
    LE_ASSERT(rpcServerPtr != NULL);

    ConnectService(rpcServerPtr);

    // Delete the timer
    le_timer_Delete(timerRef);
    rpcServerPtr->timerRef = NULL;

    return;
}


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
        case ACTION_CONNECT_SERVICE:
            return "ACTION_CONNECT_SERVICE";
        case ACTION_RECONNECT_SERVICE:
            return "ACTION_RECONNECT_SERVICE";
        case ACTION_DISCONNECT_SERVICE:
            return "ACTION_DISCONNECT_SERVICE";
    }
    LE_ERROR("action %d out of range.", action);
    return "(unknown)";
}


//--------------------------------------------------------------------------------------------------
/**
 * Close all local proxy sessions of a service.
 */
//--------------------------------------------------------------------------------------------------
static void CloseAllProxySessions
(
    RpcServerProxy_t* rpcServerPtr
)
{
    LE_ASSERT(rpcServerPtr != NULL);
    ProxySession_t* proxySessionPtr;

    le_dls_Link_t* linkPtr = le_dls_Peek(&rpcServerPtr->proxySessionList);
    while (linkPtr != NULL)
    {
        proxySessionPtr = CONTAINER_OF(linkPtr, ProxySession_t, link);
        LE_ASSERT((proxySessionPtr != NULL) && (proxySessionPtr->sessionRef != NULL));

        linkPtr = le_dls_PeekNext(&rpcServerPtr->proxySessionList, linkPtr);

        proxySessionPtr->isRemoteClosed = true;
        le_msg_CloseSession(proxySessionPtr->sessionRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC server proxy internal event handler.
 */
//--------------------------------------------------------------------------------------------------
static void RpcServerEventHandler
(
    void* reportPtr
)
{
    ServerProxyEvent_t* myEventPtr = reportPtr;
    LE_ASSERT((myEventPtr != NULL) && (myEventPtr->rpcServerPtr != NULL));

    RpcServerProxy_t* rpcServerPtr = myEventPtr->rpcServerPtr;
    Action_t action = myEventPtr->action;

    RpcNodeState_t curState = rpcServerPtr->state;

    LE_INFO("Received action event('%s') in state('%s') of service(0x%x/0x%x).",
            GetActionString(action), rpcProxyMessage_GetStateString(curState),
            rpcServerPtr->someipClient.serviceId, rpcServerPtr->someipClient.instanceId);

    // Main state-machine.
    switch(curState)
    {
        case RPC_NODE_NOT_CONNECTED:
        {
            if (action == ACTION_CONNECT_SERVICE)
            {
                ConnectService(rpcServerPtr);
                rpcServerPtr->state = RPC_NODE_CONNECTING;
            }
        }
        break;

        case RPC_NODE_CONNECTING:
        {
            if (action == ACTION_RECONNECT_SERVICE)
            {
                // Create or start re-connect timer.
                if (rpcServerPtr->timerRef == NULL)
                {
                    char timerName[128]={0};
                    snprintf(timerName, sizeof(timerName), "Reconnect_service(0x%x/0x%x)",
                             rpcServerPtr->someipClient.serviceId,
                             rpcServerPtr->someipClient.instanceId);

                    rpcServerPtr->timerRef = le_timer_Create(timerName);
                    le_timer_SetMsInterval(rpcServerPtr->timerRef, 2000);
                    le_timer_SetHandler(rpcServerPtr->timerRef, ReConnectTimerHandler);
                    le_timer_SetWakeup(rpcServerPtr->timerRef, false);
                    le_timer_SetRepeat(rpcServerPtr->timerRef, 1);
                    le_timer_SetContextPtr(rpcServerPtr->timerRef, rpcServerPtr);

                    le_timer_Start(rpcServerPtr->timerRef);
                }
                else
                {
                    le_timer_Restart(rpcServerPtr->timerRef);
                }
            }
            else if (action == ACTION_DISCONNECT_SERVICE)
            {
                if (rpcServerPtr->timerRef == NULL)
                {
                    // Re-connect timer is not started, which means the connectService command
                    // is in progress. So set state to RPC_NODE_CONNECT_ABORTED.
                    rpcServerPtr->state = RPC_NODE_CONNECT_ABORTED;
                }
                else
                {
                    // Re-connect timer is started. Just stop and delete the timer and set state
                    // to RPC_NODE_NOT_CONNECTED.
                    le_timer_Delete(rpcServerPtr->timerRef);
                    rpcServerPtr->timerRef = NULL;
                    rpcServerPtr->state = RPC_NODE_NOT_CONNECTED;
                }
            }
        }
        break;

        case RPC_NODE_CONNECT_ABORTED:
        {
            if (action == ACTION_CONNECT_SERVICE)
            {
                // Simply set the state back to RPC_NODE_CONNECTING, which will trigger
                // re-connect timer later.
                rpcServerPtr->state = RPC_NODE_CONNECTING;
            }
            else if (action == ACTION_RECONNECT_SERVICE)
            {
                rpcServerPtr->state = RPC_NODE_NOT_CONNECTED;
            }
        }
        break;

        case RPC_NODE_CONNECTED:
        {
            if (action == ACTION_DISCONNECT_SERVICE)
            {
                LE_INFO("======= Delete TelAF RPC Service '%s' =========",
                        rpcServerPtr->bindingInterface);

                // Close all proxy sessions of this proxy service.
                CloseAllProxySessions(rpcServerPtr);

                // Delete the proxy service.
                le_msg_DeleteService(rpcServerPtr->proxyServiceRef);

                rpcServerPtr->proxyServiceRef = NULL;
                rpcServerPtr->state = RPC_NODE_NOT_CONNECTED;
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
 * Data struct initialization.
 */
//--------------------------------------------------------------------------------------------------
static void DataInit
(
    void
)
{
    // Initialize RPC server proxy pool.
    RpcServerProxyPoolRef = le_mem_InitStaticPool(RpcServerProxyPool,
                                                  (RPC_MAX_SYSTEMS*RPC_MAX_SERVICES),
                                                  sizeof(RpcServerProxy_t));

    // Initialize local server proxy session pool.
    ProxySessionPoolRef = le_mem_CreatePool("LocalServerProxySessionPool", sizeof(ProxySession_t));

    // Initialize local message pool.
    MsgPoolRef = le_mem_CreatePool("LocalMessagePool", sizeof(LocalMsg_t));

    // Create internal event and add event handler.
    RpcServerEvent = le_event_CreateId("RpcServerEvent", sizeof(ServerProxyEvent_t));
    RpcServerEventHandlerRef = le_event_AddHandler("RpcServerEvent Handler",
                                                   RpcServerEvent,
                                                   RpcServerEventHandler);
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC server proxy initialization function.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcServerProxy_Init
(
    const RequestServiceConfigEntry_t* serviceConfigPtr,///< [IN] Request service config table.
    uint8_t number                                      ///< [IN] Element number.
)
{
    if (number == 0)
    {
        // Simply return LE_OK if no RPC servers configured.
        LE_INFO("No RPC serverProxy is configured.");
        return LE_NOT_FOUND;
    }

    if (serviceConfigPtr == NULL)
    {
        LE_ERROR("serviceConfigPtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    // Data struct init.
    DataInit();

    uint8_t i, j, sysIdCnt;
    const SystemId_t* sysIdPtr;
    SystemId_t systemId;
    RpcRemoteSystem_Ref_t systemRef;
    RpcServerProxy_t* rpcServerProxyPtr;
    SystemLink_t mySystemLink;

    // Get my systemLink.
    rpcProxy_GetMySystemLink(&mySystemLink);

    for(i = 0; i < number; i++)
    {
        // Get the client system list.
        sysIdPtr = serviceConfigPtr->serverSystems;
        sysIdCnt = serviceConfigPtr->systemCnt;
        if ((sysIdPtr == NULL) || (sysIdCnt == 0))
        {
            LE_ERROR("sysIdPtr is NULL or sysCnt is 0.");
            return LE_BAD_PARAMETER;
        }

        for (j = 0; j < sysIdCnt; j++)
        {
            systemId = *sysIdPtr;
            systemRef = rpcProxy_FindRemoteSystem(systemId);
            if (systemRef == NULL)
            {
                LE_ERROR("Can not find the remote system for ID 0x%x.", systemId);
                return LE_FAULT;
            }

            rpcServerProxyPtr = le_mem_TryAlloc(RpcServerProxyPoolRef);
            if (rpcServerProxyPtr == NULL)
            {
                LE_ERROR("rpcServerProxyPtr is NULL.");
                return LE_NO_MEMORY;
            }

            // Initialize data struct.
            memset(rpcServerProxyPtr, 0, sizeof(RpcServerProxy_t));

            // Set base service info.
            rpcServerProxyPtr->serviceCfg.id = serviceConfigPtr->service.id;
            le_utf8_Copy(rpcServerProxyPtr->serviceCfg.user, serviceConfigPtr->service.user,
                         sizeof(rpcServerProxyPtr->serviceCfg.user), NULL);
            le_utf8_Copy(rpcServerProxyPtr->serviceCfg.name, serviceConfigPtr->service.name,
                         sizeof(rpcServerProxyPtr->serviceCfg.name), NULL);
            le_utf8_Copy(rpcServerProxyPtr->serviceCfg.protocolId,
                         serviceConfigPtr->service.protocolId,
                         sizeof(rpcServerProxyPtr->serviceCfg.protocolId), NULL);

            // Set maxMessageSize and version to 0, later they will be set once the service
            // info is retrieved from remote RPC server.
            rpcServerProxyPtr->maxMessageSize = 0;
            rpcServerProxyPtr->version[0] = '\0';

            // Use my system index as the event group and event id. This can be kept consistent
            // with remote RPC client proxy side since they're retrieved from the same JSON file.
            rpcServerProxyPtr->sysEventId = RPC_EVENT_ID_BASE + mySystemLink.index;

            // SOME/IP client parameters.
            rpcServerProxyPtr->someipClient.isReliable = serviceConfigPtr->isReliable;
            rpcServerProxyPtr->someipClient.responseTimeout = serviceConfigPtr->responseTimeout;
            rpcServerProxyPtr->someipClient.serviceId = serviceConfigPtr->service.id;
            rpcServerProxyPtr->someipClient.instanceId = systemId;
            rpcServerProxyPtr->someipClient.serviceRef = NULL;
            rpcServerProxyPtr->someipClient.stateHandlerRef = NULL;
            rpcServerProxyPtr->someipClient.eventHandlerRef = NULL;
            rpcServerProxyPtr->someipClient.serviceState = TAF_SOMEIPCLNT_UNAVAILABLE;

            // ServerProxySession list.
            rpcServerProxyPtr->proxySessionList = LE_DLS_LIST_INIT;

            // Server proxy parameters.
            rpcServerProxyPtr->timerRef = NULL;
            rpcServerProxyPtr->systemRef = systemRef;
            rpcServerProxyPtr->isRequested = false;
            rpcServerProxyPtr->state = RPC_NODE_NOT_CONNECTED;
            rpcServerProxyPtr->proxyServiceRef = NULL;
            rpcServerProxyPtr->link = LE_DLS_LINK_INIT;

            // Set binding interface.
            char interfaceName[256]={0};
            snprintf(interfaceName, sizeof(interfaceName), "rpcServer_%x_%s_%s",
                     systemId, rpcServerProxyPtr->serviceCfg.user,
                     rpcServerProxyPtr->serviceCfg.name);
            le_utf8_Copy(rpcServerProxyPtr->bindingInterface, interfaceName,
                         sizeof(rpcServerProxyPtr->bindingInterface), NULL);

            rpcServerProxyPtr->baseClass.type = RPC_SERVER_PROXY;
            rpcServerProxyPtr->baseClass.link = LE_DLS_LINK_INIT;

            // Add the proxy object into list.
            le_dls_Queue(&RpcServerProxyList, &rpcServerProxyPtr->link);

            // Save the node in the rpcNodeList of the remote system.
            rpcProxy_AddNode(rpcServerProxyPtr->systemRef, &rpcServerProxyPtr->baseClass);

            LE_INFO("Created rpcServerNode for" \
                    " svrId=0x%x/0x%x evtId=0x%x svrName='%s' user='%s' protoId='%s'",
                    rpcServerProxyPtr->someipClient.serviceId,
                    rpcServerProxyPtr->someipClient.instanceId,
                    rpcServerProxyPtr->sysEventId,
                    rpcServerProxyPtr->serviceCfg.name,
                    rpcServerProxyPtr->serviceCfg.user,
                    rpcServerProxyPtr->serviceCfg.protocolId);

            // Move to the next entry in the client system table.
            sysIdPtr++;
        }

        // Move to the next entry in the request service configuration table.
        serviceConfigPtr++;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Validate the RPC version.
 */
//--------------------------------------------------------------------------------------------------
le_result_t ValidateRpcVersion
(
    RpcServerProxy_t* rpcServerPtr                    ///< [IN] RPC server proxy pointer.
)
{
    if (rpcServerPtr == NULL)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    uint8_t majVer;
    uint32_t minVer;
    uint16_t serviceId = rpcServerPtr->someipClient.serviceId;
    uint16_t instanceId = rpcServerPtr->someipClient.instanceId;
    taf_someipClnt_ServiceRef_t serviceRef = rpcServerPtr->someipClient.serviceRef;

    if (LE_OK != taf_someipClnt_GetVersion(serviceRef, &majVer, &minVer))
    {
        LE_ERROR("Failed to get RPC service version of service(0x%x/0x%x).", serviceId,instanceId);
        return LE_FAULT;
    }
    else if ((majVer != RPC_MAJOR_VERSION) || (minVer != RPC_MINOR_VERSION))
    {
        LE_ERROR("RPC versions mismatch(myVersion=0x%x.0x%x, remoteVersion=0x%x.0x%x) for" \
                 "service(0x%x/0x%x).", (uint32_t)RPC_MAJOR_VERSION, RPC_MINOR_VERSION,
                 (uint32_t)majVer, minVer, serviceId, instanceId);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC state change Handler on RPC server proxy side.
 */
//--------------------------------------------------------------------------------------------------
static void RpcStateChangeHandler
(
    taf_someipClnt_ServiceRef_t serviceRef,
    taf_someipClnt_State_t state,
    void* contextPtr
)
{
    RpcServerProxy_t* rpcServerPtr = contextPtr;

    // Sanity check for parameters.
    LE_ASSERT((rpcServerPtr != NULL) && (rpcServerPtr->someipClient.serviceRef == serviceRef));

    // Validate the service interface version.
    if ((state == TAF_SOMEIPCLNT_AVAILABLE) &&
        (LE_OK != ValidateRpcVersion(rpcServerPtr)))
    {
        // Validation failed.
        return;
    }

    ServerProxyEvent_t myEvent;
    myEvent.rpcServerPtr = rpcServerPtr;
    myEvent.action =
        (state == TAF_SOMEIPCLNT_AVAILABLE) ? ACTION_CONNECT_SERVICE : ACTION_DISCONNECT_SERVICE;
    le_event_Report(RpcServerEvent, &myEvent, sizeof(ServerProxyEvent_t));

    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC event message Handler on RPC server proxy side.
 */
//--------------------------------------------------------------------------------------------------
static void RpcEventMsgHandler
(
    taf_someipClnt_ServiceRef_t serviceRef,
    uint16_t eventId,
    const uint8_t* dataPtr,
    size_t dataSize,
    void* contextPtr
)
{
    RpcServerProxy_t* rpcServerPtr = contextPtr;

    // Sanity check for parameters.
    LE_ASSERT((rpcServerPtr != NULL) && (rpcServerPtr->sysEventId == eventId) &&
              (rpcServerPtr->someipClient.serviceRef == serviceRef) &&
              (dataPtr != NULL) && (dataSize >= sizeof(RpcEventCommonHeader_t)));

    uint16_t serviceId = rpcServerPtr->someipClient.serviceId;
    uint16_t instanceId = rpcServerPtr->someipClient.instanceId;
    ProxySession_t* proxySessionPtr;
    const uint8_t* msgPtr;
    size_t msgSize;
    le_msg_MessageRef_t msgRef;

    // Set a pointer to the common event header.
    RpcEventCommonHeader_t* commonHeaderPtr = (RpcEventCommonHeader_t*)dataPtr;

    // Convert rpcSession ID into Host-Order before processing.
    commonHeaderPtr->rpcSessionId = be32toh(commonHeaderPtr->rpcSessionId);

    switch(commonHeaderPtr->opCode)
    {
        case RPC_EVENT_OP_MESSAGE:
        {
            msgPtr = dataPtr + sizeof(RpcEventCommonHeader_t);
            msgSize = dataSize - sizeof(RpcEventCommonHeader_t);
            if (msgSize <= 0)
            {
                LE_ERROR("Invalid event size for Message Event.");
                return;
            }

            // Try to find local proxy session.
            proxySessionPtr = FindProxySessionById(commonHeaderPtr->rpcSessionId, rpcServerPtr);
            if ((proxySessionPtr == NULL) || (proxySessionPtr->sessionRef == NULL))
            {
                LE_ERROR("No proxySession found for rpcSessionId(0x%x) of service(0x%x/0x%x).",
                         commonHeaderPtr->rpcSessionId, serviceId, instanceId);

                // <TBD> Do we need to notify remote side ?
                return;
            }

            // Create an IPC event message and send to the real client.
            msgRef = le_msg_CreateMsg(proxySessionPtr->sessionRef);

            // Sanity check of the message length.
            LE_ASSERT(msgSize == le_msg_GetMaxPayloadSize(msgRef));

            // Copy the event data into message buffer of the IPC message.
            uint8_t* ipcDataPtr = le_msg_GetPayloadPtr(msgRef);
            memcpy(ipcDataPtr, msgPtr, msgSize);

            // Send the IPC event.
            le_msg_Send(msgRef);
        }
        break;

        case RPC_EVENT_OP_DELETE_SESSION:
        {
            msgSize = dataSize - sizeof(RpcEventCommonHeader_t);
            if (msgSize != 0)
            {
                LE_ERROR("Invalid event size for DeleteSession Event.");
                return;
            }

            // Try to find local proxy session.
            proxySessionPtr = FindProxySessionById(commonHeaderPtr->rpcSessionId, rpcServerPtr);
            if (proxySessionPtr == NULL)
            {
                LE_ERROR("No proxySession found for rpcSessionId(0x%x) of service(0x%x/0x%x).",
                         commonHeaderPtr->rpcSessionId, serviceId, instanceId);
                return;
            }

            proxySessionPtr->isRemoteClosed = true;
            // Close local proxy session.
            if (proxySessionPtr->sessionRef != NULL)
            {
                le_msg_CloseSession(proxySessionPtr->sessionRef);
            }
        }
        break;

        default:
            LE_ERROR("Unknown opCode(%d).", commonHeaderPtr->opCode);
        break;
    }
    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * Request a remote RPC service by calling SOME/IP client APIs.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RequestService
(
    RpcServerProxy_t* rpcServerPtr                    ///< [IN] RPC server proxy pointer.
)
{
    // Parameter check.
    if (rpcServerPtr == NULL)
    {
        LE_ERROR("rpcServerPtr is NULL.");
        return LE_BAD_PARAMETER;
    }

    if (rpcServerPtr->isRequested)
    {
        LE_ERROR("RPC service (0x%x/0x%x) is already requested.",
                 rpcServerPtr->someipClient.serviceId, rpcServerPtr->someipClient.instanceId);
        return LE_DUPLICATE;
    }

    // Create the SOME/IP client service and request the service.
    SystemId_t systemId = rpcProxy_GetRemoteSystemId(rpcServerPtr->systemRef);
    SomeipClient_t* someipClientPtr = &rpcServerPtr->someipClient;
    uint16_t serviceId = someipClientPtr->serviceId;
    uint16_t instanceId = someipClientPtr->instanceId;
    taf_someipClnt_ServiceRef_t serviceRef;
    taf_someipClnt_StateChangeHandlerRef_t stateHandlerRef;
    taf_someipClnt_EventMsgHandlerRef_t eventHandlerRef;
    taf_someipClnt_State_t state;

    if (rpcProxyConfig_GetRoutingName() == NULL)
    {
        serviceRef = taf_someipClnt_RequestService(serviceId, instanceId);
    }
    else
    {
        serviceRef = taf_someipClnt_RequestServiceEx(serviceId, instanceId,
                                                     rpcProxyConfig_GetRoutingName());
    }

    if (serviceRef == NULL)
    {
        LE_ERROR("Failed to request SOME/IP service (0x%x/0x%x).", serviceId, instanceId);
        return LE_FAULT;
    }

    // Save the SOME/IP client service reference.
    someipClientPtr->serviceRef = serviceRef;

    // Register state handler for the service.
    stateHandlerRef = taf_someipClnt_AddStateChangeHandler(someipClientPtr->serviceRef,
                                                           RpcStateChangeHandler,
                                                           (void*)rpcServerPtr);
    if (stateHandlerRef == NULL)
    {
        LE_ERROR("Failed to add state handler for service(0x%x/0x%x) of remote system(id=0x%x).",
                 serviceId, instanceId, systemId);
        return LE_FAULT;
    }

    // Save the state change handler.
    someipClientPtr->stateHandlerRef = stateHandlerRef;

    // Enable and subscribe eventGroup to receive events.
    if (LE_OK != taf_someipClnt_EnableEventGroup(someipClientPtr->serviceRef,
                                                 rpcServerPtr->sysEventId,
                                                 rpcServerPtr->sysEventId,
                                                 TAF_SOMEIPDEF_ET_EVENT))
    {
        LE_ERROR("Failed to enable eventGroup(0x%x) for service(0x%x/0x%x).",
                 rpcServerPtr->sysEventId, serviceId, instanceId);
        return LE_FAULT;
    }

    if (LE_OK != taf_someipClnt_SubscribeEventGroup(serviceRef, rpcServerPtr->sysEventId))
    {
        LE_ERROR("Failed to subscribe eventGroup(0x%x) for service(0x%x/0x%x).",
                 rpcServerPtr->sysEventId, serviceId, instanceId);
        return LE_FAULT;
    }

    // Add event handler.
    eventHandlerRef = taf_someipClnt_AddEventMsgHandler(someipClientPtr->serviceRef,
                                                        rpcServerPtr->sysEventId,
                                                        RpcEventMsgHandler,
                                                        (void*)rpcServerPtr);
    if (eventHandlerRef == NULL)
    {
        LE_ERROR("Failed to add RPC event(0x%x) for service(0x%x/0x%x).",
                 rpcServerPtr->sysEventId, serviceId, instanceId);
        return LE_FAULT;
    }

    someipClientPtr->eventHandlerRef = eventHandlerRef;

    // Check if the service is available.
    if (LE_OK != taf_someipClnt_GetState(someipClientPtr->serviceRef, &state))
    {
        LE_ERROR("Failed to get RPC service state for service(0x%x/0x%x).",
                 serviceId, instanceId);
        return LE_FAULT;
    }

    if (state == TAF_SOMEIPCLNT_AVAILABLE)
    {
        if (LE_OK != ValidateRpcVersion(rpcServerPtr))
        {
            // Validation failed.
            return LE_FAULT;
        }

        // Try to connect service if the service is available and RPC version validation passed.
        ServerProxyEvent_t myEvent;
        myEvent.action = ACTION_CONNECT_SERVICE;
        myEvent.rpcServerPtr = rpcServerPtr;
        le_event_Report(RpcServerEvent, &myEvent, sizeof(ServerProxyEvent_t));
    }

    // Set the flag to indicate that service is requested.
    rpcServerPtr->isRequested = true;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Start a RPC server proxy.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcServerProxy_Start
(
    RpcServerProxyNode_Ref_t nodeRef                  ///< [IN] RPC server proxy node reference.
)
{
   RpcServerProxy_t* rpcServerPtr = nodeRef;

   return RequestService(rpcServerPtr);
}
