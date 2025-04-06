/*
 *  Copyright (c) 2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include "rpcProxyMessaging.h"


//--------------------------------------------------------------------------------------------------
/**
 * RPC commands.
 */
//--------------------------------------------------------------------------------------------------
typedef enum ReqCommand
{
    RPC_COMMAND_CONNECT_SERVICE,   ///< [IN] Connect Service Command.
    RPC_COMMAND_CREATE_SESSION,    ///< [IN] Create Session Command.
    RPC_COMMAND_DELETE_SESSION,    ///< [IN] Delete Session Command.
    RPC_COMMAND_MESSAGE            ///< [IN] Message-Request command.
}ReqCommand_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC common response handler data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct CommonHandlerData
{
    ReqCommand_t cmd;             ///< [IN] RPC command type.
    void* handlerPtr;             ///< [IN] Dedicate Response Handler.
    void* contextPtr;             ///< [IN] Context of the dedicate handler.
}CommonHandlerData_t;


//--------------------------------------------------------------------------------------------------
/**
 * RPC common response handler pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t CommonHandlerPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * RPC Tx message buffer.
 */
//--------------------------------------------------------------------------------------------------
static uint8_t TxMessageBuffer[TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE];


//--------------------------------------------------------------------------------------------------
/**
 * Get RPC node state name string.
 */
//--------------------------------------------------------------------------------------------------
const char* rpcProxyMessage_GetStateString
(
    RpcNodeState_t state  ///< [IN] The state to be translated.
)
{
    switch (state)
    {
        case RPC_NODE_NOT_CONNECTED:
            return "RPC_NODE_NOT_CONNECTED";
        case RPC_NODE_CONNECTING:
            return "RPC_NODE_CONNECTING";
        case RPC_NODE_CONNECT_ABORTED:
            return "RPC_NODE_CONNECT_ABORTED";
        case RPC_NODE_CONNECTED:
            return "RPC_NODE_CONNECTED";
    }
    LE_ERROR("state %d out of range.", state);
    return "(unknown)";
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC common response handler.
 */
//--------------------------------------------------------------------------------------------------
static void CommonRespHandlerFunc
(
    le_result_t result,
    bool isErrRsp,
    uint8_t returnCode,
    const uint8_t* dataPtr,
    size_t dataSize,
    void* contextPtr
)
{
    CommonHandlerData_t* handlerDataPtr = contextPtr;
    LE_ASSERT((handlerDataPtr != NULL) && (handlerDataPtr->handlerPtr != NULL));

    le_result_t ret = result;

    if ((ret == LE_OK) && (isErrRsp || (returnCode != TAF_SOMEIPDEF_E_OK)))
    {
        LE_ERROR("Invalid RPC response.");
        ret = LE_FAULT;
    }

    ReqCommand_t cmd = handlerDataPtr->cmd;

    switch(cmd)
    {
        case RPC_COMMAND_CONNECT_SERVICE:
        {
            RpcConnectServiceHandlerFunc_t respHandlerPtr = handlerDataPtr->handlerPtr;

            // Check the response data size.
            if ((ret == LE_OK) && (dataSize != sizeof(RpcConnectServiceResp_t)))
            {
                LE_ERROR("Invalid connect service response size(%"PRIuS")", dataSize);
                ret = LE_FAULT;
            }

            if (ret == LE_OK)
            {
                // Convert to Host-Order in response data.
                RpcConnectServiceResp_t* respDataPtr = (RpcConnectServiceResp_t*)dataPtr;
                respDataPtr->maxMsgSize = be32toh(respDataPtr->maxMsgSize);
                respDataPtr->sysEventId = be16toh(respDataPtr->sysEventId);

                // Call dedicate response handler.
                respHandlerPtr(LE_OK, respDataPtr, handlerDataPtr->contextPtr);
            }
            else
            {
                respHandlerPtr(ret, NULL, handlerDataPtr->contextPtr);
            }
        }
        break;

        case RPC_COMMAND_CREATE_SESSION:
        case RPC_COMMAND_DELETE_SESSION:
        {
            RpcSessionHandlerFunc_t respHandlerPtr = handlerDataPtr->handlerPtr;

            // Set the response type.
            RpcSessionResp_t respType = (cmd == RPC_COMMAND_CREATE_SESSION) ?
                RPC_SESSION_CREATE_RESPONSE : RPC_SESSION_DELETE_RESPONSE;

            // Check the response data size.
            if ((ret == LE_OK) && (dataSize != sizeof(RpcMessageCommonHeader_t)))
            {
                LE_ERROR("Invalid RPC session response size(%"PRIuS")", dataSize);
                ret = LE_FAULT;
            }

            if (ret == LE_OK)
            {
                // Convert it to Host-Order in response data.
                RpcMessageCommonHeader_t* respDataPtr = (RpcMessageCommonHeader_t*)dataPtr;
                respDataPtr->rpcSessionId = be32toh(respDataPtr->rpcSessionId);

                // Call dedicate response handler.
                respHandlerPtr(LE_OK, respType, respDataPtr->rpcSessionId,
                               handlerDataPtr->contextPtr);
            }
            else
            {
                respHandlerPtr(ret, respType, 0, handlerDataPtr->contextPtr);
            }
        }
        break;

        case RPC_COMMAND_MESSAGE:
        {
            RpcMessageHandlerFunc_t respHandlerPtr = handlerDataPtr->handlerPtr;

            // Check the response data size.
            if ((ret == LE_OK) && (dataSize <= sizeof(RpcMessageCommonHeader_t)))
            {
                LE_ERROR("Invalid RPC session response size(%"PRIuS")", dataSize);
                ret = LE_FAULT;
            }

            if (ret == LE_OK)
            {
                // Convert it to Host-Order in response data.
                RpcMessageCommonHeader_t* msgHdrPtr = (RpcMessageCommonHeader_t*)dataPtr;
                msgHdrPtr->rpcSessionId = be32toh(msgHdrPtr->rpcSessionId);

                // Call dedicate response handler.
                respHandlerPtr(LE_OK, msgHdrPtr->rpcSessionId,
                               dataPtr+sizeof(RpcMessageCommonHeader_t),
                               dataSize-sizeof(RpcMessageCommonHeader_t),
                               handlerDataPtr->contextPtr);
            }
            else
            {
                respHandlerPtr(ret, 0, NULL, 0, handlerDataPtr->contextPtr);
            }
        }
        break;

        default:
        {
            LE_FATAL("Unknown cmd(%d).", cmd);
        }
        break;
    }

    // Release the handlerData.
    le_mem_Release(handlerDataPtr);
}


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
)
{
    // Sanity check for the parameters.
    LE_ASSERT((serviceRef != NULL) && (reqPtr != NULL) &&
              (respHandler != NULL) && (contextPtr != NULL));

    // Create a RPC request message.
    taf_someipClnt_TxMsgRef_t rpcMsgRef =
        taf_someipClnt_CreateMsg(serviceRef, RPC_METHOD_CONNECT_SERVICE);
    LE_ASSERT(rpcMsgRef != NULL);

    // Connect with TCP mode.
    if (isReliable)
    {
        LE_ASSERT(LE_OK == taf_someipClnt_SetReliable(rpcMsgRef));
    }

    // Set payload data.
    reqPtr->sysEventId = htobe16(reqPtr->sysEventId);
    LE_ASSERT(LE_OK == taf_someipClnt_SetPayload(rpcMsgRef, (uint8_t*)reqPtr,
                                                 sizeof(RpcConnectServiceReq_t)));

    // Create a response handler.
    CommonHandlerData_t* handlerDataPtr = le_mem_ForceAlloc(CommonHandlerPoolRef);
    memset(handlerDataPtr, 0, sizeof(CommonHandlerData_t));
    handlerDataPtr->cmd = RPC_COMMAND_CONNECT_SERVICE;
    handlerDataPtr->handlerPtr = (void*)respHandler;
    handlerDataPtr->contextPtr = contextPtr;

    // Send the RPC command.
    taf_someipClnt_RequestResponse(rpcMsgRef, CommonRespHandlerFunc, (void*)handlerDataPtr);
}


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
)
{
    // Sanity check for the parameters.
    LE_ASSERT((serviceRef != NULL) && (respHandler != NULL) && (contextPtr != NULL));

    // Create a RPC request message.
    taf_someipClnt_TxMsgRef_t rpcMsgRef =
        taf_someipClnt_CreateMsg(serviceRef, RPC_METHOD_CREATE_SESSION);
    LE_ASSERT(rpcMsgRef != NULL);

    // Connect with TCP mode.
    if (isReliable)
    {
        LE_ASSERT(LE_OK == taf_someipClnt_SetReliable(rpcMsgRef));
    }

    // Create a response handler.
    CommonHandlerData_t* handlerDataPtr = le_mem_ForceAlloc(CommonHandlerPoolRef);
    memset(handlerDataPtr, 0, sizeof(CommonHandlerData_t));
    handlerDataPtr->cmd = RPC_COMMAND_CREATE_SESSION;
    handlerDataPtr->handlerPtr = (void*)respHandler;
    handlerDataPtr->contextPtr = contextPtr;

    // Send the RPC command.
    taf_someipClnt_RequestResponse(rpcMsgRef, CommonRespHandlerFunc, (void*)handlerDataPtr);
}


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
)
{
    // Sanity check for the parameters.
    LE_ASSERT((serviceRef != NULL) && (respHandler != NULL) &&
              (sessionId != 0) && (contextPtr != NULL));

    // Create a RPC request message.
    taf_someipClnt_TxMsgRef_t rpcMsgRef =
        taf_someipClnt_CreateMsg(serviceRef, RPC_METHOD_DELETE_SESSION);
    LE_ASSERT(rpcMsgRef != NULL);

    // Connect with TCP mode.
    if (isReliable)
    {
        LE_ASSERT(LE_OK == taf_someipClnt_SetReliable(rpcMsgRef));
    }

    // Set the RPC session ID and convert it to Network-Order in the payload message.
    RpcMessageCommonHeader_t reqData;
    memset(&reqData, 0, sizeof(RpcMessageCommonHeader_t));
    reqData.rpcSessionId = sessionId;
    reqData.rpcSessionId = htobe32(reqData.rpcSessionId);
    LE_ASSERT(LE_OK == taf_someipClnt_SetPayload(rpcMsgRef, (uint8_t*)&reqData, sizeof(reqData)));

    // Create a response handler.
    CommonHandlerData_t* handlerDataPtr = le_mem_ForceAlloc(CommonHandlerPoolRef);
    memset(handlerDataPtr, 0, sizeof(CommonHandlerData_t));
    handlerDataPtr->cmd = RPC_COMMAND_DELETE_SESSION;
    handlerDataPtr->handlerPtr = (void*)respHandler;
    handlerDataPtr->contextPtr = contextPtr;

    // Send the RPC command.
    taf_someipClnt_RequestResponse(rpcMsgRef, CommonRespHandlerFunc, (void*)handlerDataPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * RPC message request response function
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_MessageRequestResponse
(
    taf_someipClnt_ServiceRef_t serviceRef,    ///< [IN] SOME/IP service reference.
    bool isReliable,                           ///< [IN] True if using TCP.
    uint32_t timeoutSecs,                      ///< [IN] Response timeout secs.
    uint32_t sessionId,                        ///< [IN] RPC session Id.
    le_msg_MessageRef_t msgRef,                ///< [IN] IPC message reference.
    RpcMessageHandlerFunc_t respHandler,       ///< [IN] Response handler.
    void* contextPtr                           ///< [IN] Request context.
)
{
    // Sanity check for the parameters.
    LE_ASSERT((serviceRef != NULL) && (respHandler != NULL) &&
              (sessionId != 0) && (msgRef != NULL) && (contextPtr != NULL));

    // Create a RPC request message.
    taf_someipClnt_TxMsgRef_t rpcMsgRef =
        taf_someipClnt_CreateMsg(serviceRef, RPC_METHOD_MESSAGE);
    LE_ASSERT(rpcMsgRef != NULL);

    // Connect with TCP mode.
    if (isReliable)
    {
        LE_ASSERT(LE_OK == taf_someipClnt_SetReliable(rpcMsgRef));
    }

    // Set timeout secs.
    LE_ASSERT(LE_OK == taf_someipClnt_SetTimeout(rpcMsgRef, timeoutSecs*1000));

    // Set the RPC session ID and convert it to Network-Order.
    RpcMessageCommonHeader_t msgHeader;
    memset(&msgHeader, 0, sizeof(RpcMessageCommonHeader_t));
    msgHeader.rpcSessionId = sessionId;
    msgHeader.rpcSessionId = htobe32(msgHeader.rpcSessionId);
    memcpy(TxMessageBuffer, &msgHeader, sizeof(RpcMessageCommonHeader_t));

    // Create the payload message.
    size_t msgSize = le_msg_GetMaxPayloadSize(msgRef);
    uint8_t* msgPtr = le_msg_GetPayloadPtr(msgRef);
    memcpy(TxMessageBuffer + sizeof(RpcMessageCommonHeader_t), msgPtr, msgSize);

    // Set the payload message.
    LE_ASSERT(LE_OK == taf_someipClnt_SetPayload(rpcMsgRef, TxMessageBuffer,
                                                 sizeof(RpcMessageCommonHeader_t) + msgSize));

    // Create a response handler.
    CommonHandlerData_t* handlerDataPtr = le_mem_ForceAlloc(CommonHandlerPoolRef);
    memset(handlerDataPtr, 0, sizeof(CommonHandlerData_t));
    handlerDataPtr->cmd = RPC_COMMAND_MESSAGE;
    handlerDataPtr->handlerPtr = (void*)respHandler;
    handlerDataPtr->contextPtr = contextPtr;

    // Send the RPC command.
    taf_someipClnt_RequestResponse(rpcMsgRef, CommonRespHandlerFunc, (void*)handlerDataPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Send RPC generic error response.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_ResponseError
(
    taf_someipSvr_RxMsgRef_t rpcMsgRef        ///< [IN] SOME/IP Rx message reference.
)
{
    // Sanity check for the parameter.
    LE_ASSERT(rpcMsgRef != NULL);

    // Send the RPC response.
    taf_someipSvr_SendResponse(rpcMsgRef, true, TAF_SOMEIPDEF_E_NOT_OK, NULL, 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Send RPC connect service response.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_ResponseConnectService
(
    taf_someipSvr_RxMsgRef_t rpcMsgRef,        ///< [IN] SOME/IP Rx message reference.
    RpcConnectServiceResp_t* respPtr           ///< [IN] Response data for ConnectService request.
)
{
    // Sanity check for the parameter.
    LE_ASSERT((rpcMsgRef != NULL) && (respPtr != NULL));

    // Construct and copy response data.
    RpcConnectServiceResp_t respData;
    memset(&respData, 0, sizeof(RpcConnectServiceResp_t));
    memcpy(&respData, respPtr, sizeof(RpcConnectServiceResp_t));

    // Convert to Network-Order.
    respData.maxMsgSize = htobe32(respData.maxMsgSize);
    respData.sysEventId = htobe16(respData.sysEventId);

    // Send the RPC response.
    taf_someipSvr_SendResponse(rpcMsgRef, false, TAF_SOMEIPDEF_E_OK,
                               (uint8_t*)&respData, sizeof(RpcConnectServiceResp_t));
}


//--------------------------------------------------------------------------------------------------
/**
 * Send RPC session response.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_ResponseSession
(
    taf_someipSvr_RxMsgRef_t rpcMsgRef,        ///< [IN] SOME/IP Rx message reference.
    uint32_t rpcSessionId                      ///< [IN] RPC session ID.
)
{
    // Sanity check for the parameter.
    LE_ASSERT((rpcMsgRef != NULL) && (rpcSessionId != 0));

    // Construct and copy the response data.
    RpcMessageCommonHeader_t respData;
    memset(&respData, 0, sizeof(RpcMessageCommonHeader_t));
    respData.rpcSessionId = rpcSessionId;

    // Convert to Network-Order.
    respData.rpcSessionId = htobe32(respData.rpcSessionId);

    // Send the RPC response.
    taf_someipSvr_SendResponse(rpcMsgRef, false, TAF_SOMEIPDEF_E_OK,
                               (uint8_t*)&respData, sizeof(RpcMessageCommonHeader_t));
}


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
)
{
    // Sanity check for the parameter.
    LE_ASSERT((rpcMsgRef != NULL) && (msgRef != NULL) && (rpcSessionId != 0));

    // Set the RPC session ID and convert it to Network-Order.
    RpcMessageCommonHeader_t msgHeader;
    memset(&msgHeader, 0, sizeof(RpcMessageCommonHeader_t));
    msgHeader.rpcSessionId = rpcSessionId;
    msgHeader.rpcSessionId = htobe32(msgHeader.rpcSessionId);
    memcpy(TxMessageBuffer, &msgHeader, sizeof(RpcMessageCommonHeader_t));

    // Get the payload data of the message.
    size_t msgSize = le_msg_GetMaxPayloadSize(msgRef);
    uint8_t* msgPtr = le_msg_GetPayloadPtr(msgRef);
    memcpy(TxMessageBuffer + sizeof(RpcMessageCommonHeader_t), msgPtr, msgSize);

    // Send the RPC response.
    taf_someipSvr_SendResponse(rpcMsgRef, false, TAF_SOMEIPDEF_E_OK, TxMessageBuffer,
                               sizeof(RpcMessageCommonHeader_t) + msgSize);
}


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

)
{
    // Sanity check for the parameter.
    LE_ASSERT((serviceRef != NULL) && (rpcSessionId != 0));

    // Set the RPC session ID.
    RpcEventCommonHeader_t evtHeader;
    memset(&evtHeader, 0, sizeof(RpcEventCommonHeader_t));
    evtHeader.rpcSessionId = rpcSessionId;
    evtHeader.opCode = RPC_EVENT_OP_DELETE_SESSION;

    // Convert the session ID to Network-Order.
    evtHeader.rpcSessionId = htobe32(evtHeader.rpcSessionId);

    // Send the IPC event message through the RPC event message.
    taf_someipSvr_Notify(serviceRef, eventId,
                         (uint8_t*)&evtHeader, sizeof(RpcEventCommonHeader_t));
}


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
)
{
    // Sanity check for the parameter.
    LE_ASSERT((serviceRef != NULL) && (msgRef != NULL) && (rpcSessionId != 0));

    // Set the RPC session ID.
    RpcEventCommonHeader_t evtHeader;
    memset(&evtHeader, 0, sizeof(RpcEventCommonHeader_t));
    evtHeader.rpcSessionId = rpcSessionId;
    evtHeader.opCode = RPC_EVENT_OP_MESSAGE;

    // Convert the session ID to Network-Order.
    evtHeader.rpcSessionId = htobe32(evtHeader.rpcSessionId);

    // Copy the header to Tx buffer.
    memcpy(TxMessageBuffer, &evtHeader, sizeof(RpcEventCommonHeader_t));

    // Get the IPC event message.
    size_t evtSize = le_msg_GetMaxPayloadSize(msgRef);
    uint8_t* evtPtr = le_msg_GetPayloadPtr(msgRef);

    // Copy the IPC event data to Tx buffer.
    memcpy(TxMessageBuffer+sizeof(RpcEventCommonHeader_t), evtPtr, evtSize);

    // Send the RPC event message.
    taf_someipSvr_Notify(serviceRef, eventId, TxMessageBuffer,
                         sizeof(RpcEventCommonHeader_t) + evtSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialization function.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyMessage_Init
(
    void
)
{
    CommonHandlerPoolRef = le_mem_CreatePool("CommonHandlerPoolRef", sizeof(CommonHandlerData_t));
}
