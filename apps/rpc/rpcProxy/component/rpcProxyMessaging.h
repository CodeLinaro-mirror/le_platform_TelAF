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

#ifndef TAF_RPC_PROXY_MESSAGING_H_INCLUDE_GUARD
#define TAF_RPC_PROXY_MESSAGING_H_INCLUDE_GUARD

#include "legato.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP Event and Method definitions for RPC communication.
 */
//--------------------------------------------------------------------------------------------------
#define RPC_EVENT_ID_BASE           0x8001

#define RPC_EVENT_OP_MESSAGE        0x0
#define RPC_EVENT_OP_DELETE_SESSION 0x1

#define RPC_METHOD_CONNECT_SERVICE 0x7001
#define RPC_METHOD_MESSAGE         0x7002
#define RPC_METHOD_CREATE_SESSION  0x7003
#define RPC_METHOD_DELETE_SESSION  0x7004


//--------------------------------------------------------------------------------------------------
/**
 * RPC node state.
 */
//--------------------------------------------------------------------------------------------------
typedef enum RpcNodeState
{
    RPC_NODE_NOT_CONNECTED,                ///< RPC service is unavailable.
    RPC_NODE_CONNECTING,                   ///< RPC service is in connecting.
    RPC_NODE_CONNECT_ABORTED,              ///< RPC service connecting is aborted.
    RPC_NODE_CONNECTED                     ///< RPC service is connected.
}
RpcNodeState_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC session response type.
 */
//--------------------------------------------------------------------------------------------------
typedef enum RpcSessionResp
{
    RPC_SESSION_CREATE_RESPONSE,          ///< RPC create session response.
    RPC_SESSION_DELETE_RESPONSE           ///< RPC delete session response.
}
RpcSessionResp_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC event common header.
 */
//--------------------------------------------------------------------------------------------------
typedef struct __attribute__((packed))
{
    uint32_t rpcSessionId;                ///< RPC session Id.
    uint8_t opCode;                       ///< Operation code.
}
RpcEventCommonHeader_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC message common header.
 */
//--------------------------------------------------------------------------------------------------
typedef struct __attribute__((packed))
{
    uint32_t rpcSessionId;                ///< RPC session Id.
}
RpcMessageCommonHeader_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC connect service request message.
 */
//--------------------------------------------------------------------------------------------------
typedef struct __attribute__((packed))
{
    char user[LIMIT_MAX_USER_NAME_BYTES];           ///< Service user.
    char name[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];  ///< Service name.
    char protocolId[LIMIT_MAX_PROTOCOL_ID_BYTES];   ///< Service protocol Id.
    uint16_t sysEventId;                            ///< Event/group ID assigned to this service.
}
RpcConnectServiceReq_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC connect service response message
 */
//--------------------------------------------------------------------------------------------------
typedef struct __attribute__((packed))
{
    uint32_t maxMsgSize;                            ///< Max payload size.
    char protocolId[LIMIT_MAX_PROTOCOL_ID_BYTES];   ///< Service protocol Id.
    uint16_t sysEventId;                            ///< Event/Group ID assigned to this service.
}
RpcConnectServiceResp_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC connect service response handler
 */
//--------------------------------------------------------------------------------------------------
typedef void (*RpcConnectServiceHandlerFunc_t)
(
    le_result_t result,                             ///< Result of the command.
    RpcConnectServiceResp_t* respPtr,               ///< Response data.
    void* contextPtr                                ///< Request context.
);


//--------------------------------------------------------------------------------------------------
/**
 * RPC session command response handler
 */
//--------------------------------------------------------------------------------------------------
typedef void (*RpcSessionHandlerFunc_t)
(
    le_result_t result,                            ///< Result of the command.
    RpcSessionResp_t respType,                     ///< Response data.
    uint32_t sessionId,                            ///< RPC session Id.
    void* contextPtr                               ///< Request context.
);


//--------------------------------------------------------------------------------------------------
/**
 * RPC message command response handler
 */
//--------------------------------------------------------------------------------------------------
typedef void (*RpcMessageHandlerFunc_t)
(
    le_result_t result,                            ///< Result of the command.
    uint32_t rpcSessionId,                         ///< RPC session Id.
    const uint8_t* respPtr,                        ///< Response data buffer.
    size_t respSize,                               ///< Response data size.
    void* contextPtr                               ///< Request context.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get RPC node state name string.
 */
//--------------------------------------------------------------------------------------------------
const char* rpcProxyMessage_GetStateString
(
    RpcNodeState_t state  ///< [IN] The state to be translated.
);


//--------------------------------------------------------------------------------------------------
/**
 * RPC connect service request response function
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_ConnectServiceRequestResponse
(
    taf_someipClnt_ServiceRef_t serviceRef,    ///< [IN] SOME/IP service reference.
    bool isReliable,                           ///< [IN] True if using TCP.
    RpcConnectServiceReq_t* reqPtr,            ///< [IN] Request data.
    RpcConnectServiceHandlerFunc_t respHandler,///< [IN] Response handler.
    void* contextPtr                           ///< [IN] Request context.
);


//--------------------------------------------------------------------------------------------------
/**
 * RPC create session request response function
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_CreateSessionRequestResponse
(
    taf_someipClnt_ServiceRef_t serviceRef,    ///< [IN] SOME/IP service reference.
    bool isReliable,                           ///< [IN] True if using TCP.
    RpcSessionHandlerFunc_t respHandler,       ///< [IN] Response handler.
    void* contextPtr                           ///< [IN] Request context.
);


//--------------------------------------------------------------------------------------------------
/**
 * RPC delete session request response function
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_DeleteSessionRequestResponse
(
    taf_someipClnt_ServiceRef_t serviceRef,    ///< [IN] SOME/IP service reference.
    bool isReliable,                           ///< [IN] True if using TCP.
    uint32_t sessionId,                        ///< [IN] RPC session Id.
    RpcSessionHandlerFunc_t respHandler,       ///< [IN] Response handler.
    void* contextPtr                           ///< [IN] Request context.
);


//--------------------------------------------------------------------------------------------------
/**
 * RPC message request response function
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_MessageRequestResponse
(
    taf_someipClnt_ServiceRef_t serviceRef,    ///< [IN] SOME/IP service reference.
    bool isReliable,                           ///< [IN] True if using TCP.
    uint32_t sessionId,                        ///< [IN] RPC session Id.
    le_msg_MessageRef_t msgRef,                ///< [IN] IPC message reference.
    RpcMessageHandlerFunc_t respHandler,       ///< [IN] Response handler.
    void* contextPtr                           ///< [IN] Request context.
);


//--------------------------------------------------------------------------------------------------
/**
 * Send RPC generic error response.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_ResponseError
(
    taf_someipSvr_RxMsgRef_t rpcMsgRef        ///< [IN] SOME/IP Rx message reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Send RPC connect service response.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_ResponseConnectService
(
    taf_someipSvr_RxMsgRef_t rpcMsgRef,        ///< [IN] SOME/IP Rx message reference.
    RpcConnectServiceResp_t* respPtr           ///< [IN] Response data for ConnectService request.
);


//--------------------------------------------------------------------------------------------------
/**
 * Send RPC session response.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_ResponseSession
(
    taf_someipSvr_RxMsgRef_t rpcMsgRef,        ///< [IN] SOME/IP Rx message reference.
    uint32_t rpcSessionId                      ///< [IN] RPC session ID.
);


//--------------------------------------------------------------------------------------------------
/**
 * Send RPC message response.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_ResponseMessage
(
    taf_someipSvr_RxMsgRef_t rpcMsgRef,        ///< [IN] SOME/IP Rx message reference.
    uint32_t rpcSessionId,                     ///< [IN] RPC session ID.
    le_msg_MessageRef_t msgRef                 ///< [IN] IPC response message.
);


//--------------------------------------------------------------------------------------------------
/**
 * Send RPC notification to remote to delete remote proxy session.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_NotifyDeleteSession
(
    taf_someipSvr_ServiceRef_t serviceRef,     ///< [IN] SOME/IP service reference.
    uint16_t eventId,                          ///< [IN] SOME/IP event ID.
    uint32_t rpcSessionId                      ///< [IN] RPC session ID.

);


//--------------------------------------------------------------------------------------------------
/**
 * Send RPC notification with IPC event message to remote.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_NotifyMessage
(
    taf_someipSvr_ServiceRef_t serviceRef,     ///< [IN] SOME/IP service reference.
    uint16_t eventId,                          ///< [IN] SOME/IP event ID.
    uint32_t rpcSessionId,                     ///< [IN] RPC session ID.
    le_msg_MessageRef_t msgRef                 ///< [IN] IPC event message.
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialization function.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_Init
(
    void
);
#endif /* TAF_RPC_PROXY_MESSAGING_H_INCLUDE_GUARD */
