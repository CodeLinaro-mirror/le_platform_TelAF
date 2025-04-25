/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <errno.h>
#include <arpa/inet.h>

#include "legato.h"
#include "interfaces.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "le_socketLib.h"
#ifdef __cplusplus
}
#endif

#include "tafDoIPStack.h"
#include "tafDoIPCommon.hpp"
#include "tafDoIPConnection.hpp"
#include "tafDoIPConnectionMgr.hpp"
#include "tafDoIPCommunicationMgr.hpp"
#include "tafDoIPVehicleMgr.hpp"
#include "tafDoIPVehDiscoveryAndParser.hpp"

using namespace taf::doip;

Connection::Connection
(
    std::shared_ptr<ConnectionManager> mgr,
    le_socket_Ref_t         sockRef,
    taf_doip_ConnectType_t  type,
    std::string&            ip,
    uint16_t                port,
    std::string&            ifname
)
{
    cliSockRef  = sockRef;
    remoteIp    = ip;
    remotePort  = port;
    connType    = type;
    connectionMgr = mgr;
    localIface  = ifname;
    connState = TAF_DOIP_CONNECT_STATE_INITIALIZED;

    LE_INFO("Connection is created, socket ref is %p!!!\n", sockRef);
}

Connection::~Connection()
{
    LE_INFO("Connection delete!!!\n");
}

// Create timer and allocate memory for the connection.
taf_doip_Result_t Connection::Start()
{
    uint16_t sourceAddr;
    auto& vehicleMgr = VehicleManager::GetInstance();

    if (vehicleMgr.GetEntityLogicalAddr(&sourceAddr) != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to get entity source address.\n");
        return TAF_DOIP_RESULT_UNKNOWN_SA;
    }

    if (connType == TAF_DOIP_CONNECTION_TYPE_SVR)
    {
        entitySA = sourceAddr;
    }
    else if (connType == TAF_DOIP_CONNECTION_TYPE_CLI)
    {
        testerSA = sourceAddr;
    }

    inBuf   = (taf_doip_Buffer_t*)le_mem_ForceAlloc(connectionMgr->inMsgPool);
    memset(inBuf, 0, sizeof(taf_doip_Buffer_t));
    LE_DEBUG("Alloc a buffer(%p) for reception", inBuf);

    //outBuf  = (taf_doipBuffer_t*)le_mem_ForceAlloc(mgr->msgPool);
    //memset(outBuf, 0, sizeof(taf_doipBuffer_t));

    // Create connection timer.
    uint32_t    interval;
    if (vehicleMgr.GetInitialInactivityTime(&interval) != TAF_DOIP_RESULT_OK)
    {
        // Using default value.
        interval = TAF_DOIP_GENERAL_TIMEOUT_DEF;
    }
    initialTimerRef = le_timer_Create("Initial inactivity timer");
    le_timer_SetMsInterval(initialTimerRef, interval);
    le_timer_SetRepeat(initialTimerRef, 1);
    le_timer_SetContextPtr(initialTimerRef, cliSockRef);
    le_timer_SetHandler(initialTimerRef, InitialTimerHandler);
    le_timer_SetWakeup(initialTimerRef, false);

    generalTimerRef = le_timer_Create("General inactivity timer");
    if (vehicleMgr.GetGenInactivityTime(&interval) != TAF_DOIP_RESULT_OK)
    {
        interval = TAF_DOIP_INITIAL_TIMEOUT_DEF;
    }

    le_timer_SetMsInterval(generalTimerRef, interval);
    le_timer_SetRepeat(generalTimerRef, 1);
    le_timer_SetContextPtr(generalTimerRef, cliSockRef);
    le_timer_SetHandler(generalTimerRef, GeneralTimerHandler);
    le_timer_SetWakeup(generalTimerRef, false);

    aliveCheckTimerRef = le_timer_Create("Alive check timer");
    if (vehicleMgr.GetAliveCheckTime(&interval) != TAF_DOIP_RESULT_OK)
    {
        interval = TAF_DOIP_INITIAL_TIMEOUT_DEF;
    }

    le_timer_SetMsInterval(aliveCheckTimerRef, interval);
    le_timer_SetRepeat(aliveCheckTimerRef, 1);
    le_timer_SetContextPtr(aliveCheckTimerRef, cliSockRef);
    le_timer_SetHandler(aliveCheckTimerRef, AliveCheckTimerHandler);
    le_timer_SetWakeup(aliveCheckTimerRef, false);

    if (LE_OK != le_socket_SetTimeout(cliSockRef, SOCKET_TIMEOUT_MS))
    {
        LE_ERROR("Failed to set response timeout.");
        goto errOut;
    }

    // Set the socket event callback for fd monitor.
    if (LE_OK != le_socket_AddEventHandler(cliSockRef, SocketEventCallback, NULL))
    {
        LE_ERROR("Failed to add data socket event handler.");
        goto errOut;
    }

    // Enable async mode and start fd monitor.
    if (LE_OK != le_socket_SetMonitoring(cliSockRef, true))
    {
       LE_ERROR("Failed to enable data socket monitor.");
       goto errOut;
    }

    LE_DEBUG("Set connection state machine into initialized");
    ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_INITIALIZED, 0);

    LE_DEBUG("Connection start successfully");
    return TAF_DOIP_RESULT_OK;

errOut:
    le_timer_Delete(aliveCheckTimerRef);
    le_timer_Delete(generalTimerRef);
    le_timer_Delete(initialTimerRef);
    le_mem_Release(inBuf);
    inBuf = NULL;

    return TAF_DOIP_RESULT_ERROR;
}

// When the connection is deleting, it shall be called to release resources.
taf_doip_Result_t Connection::Stop()
{
    if (le_timer_IsRunning(aliveCheckTimerRef))
    {
        le_timer_Stop(aliveCheckTimerRef);
    }

    if (le_timer_IsRunning(generalTimerRef))
    {
        le_timer_Stop(generalTimerRef);
    }

    if (le_timer_IsRunning(initialTimerRef))
    {
        le_timer_Stop(initialTimerRef);
    }

    le_timer_Delete(aliveCheckTimerRef);
    le_timer_Delete(generalTimerRef);
    le_timer_Delete(initialTimerRef);

    if (inBuf != NULL)
    {
        le_mem_Release(inBuf);
        inBuf = NULL;
    }

    /*if (outBuf != NULL)
    {
        le_mem_Release(outBuf);
        outBuf = NULL;
    }*/

    if (udsBuf != NULL)
    {
        le_mem_Release(udsBuf);
        udsBuf = NULL;
    }
    le_socket_Delete(cliSockRef);

    return TAF_DOIP_RESULT_OK;
}

void Connection::SetLogicalSourceAddr
(
    uint16_t    sa
)
{
    if (connType == TAF_DOIP_CONNECTION_TYPE_SVR)
    {
        entitySA = sa;
    }
    else
    {
        testerSA = sa;
    }
}

uint16_t Connection::GetLogicalSourceAddr
(
)
{
    if (connType == TAF_DOIP_CONNECTION_TYPE_SVR)
    {
        return entitySA;
    }
    else
    {
        return testerSA;
    }
}

void Connection::SetLogicalTargetAddr
(
    uint16_t    sa
)
{
    if (connType == TAF_DOIP_CONNECTION_TYPE_SVR)
    {
        testerSA = sa;
    }
    else
    {
        entitySA = sa;
    }
}

uint16_t Connection::GetLogicalTargetAddr
(
)
{
    if (connType == TAF_DOIP_CONNECTION_TYPE_SVR)
    {
        return testerSA;
    }
    else
    {
        return entitySA;
    }
}

bool Connection::IsLogicalAddressMatch
(
    uint16_t    logicalAddr
)
{
    if (connType == TAF_DOIP_CONNECTION_TYPE_CLI && logicalAddr == entitySA)
    {
        LE_DEBUG("The Connection is match.\n");
        return true;
    }
    else if (connType == TAF_DOIP_CONNECTION_TYPE_SVR && logicalAddr == testerSA)
    {
        LE_DEBUG("The Connection is match.\n");
        return true;
    }

    return false;
}

bool Connection::IsSocketRefMatch
(
    le_socket_Ref_t sockRef
)
{
    if (sockRef == cliSockRef)
    {
        return true;
    }

    return false;
}

taf_doip_Result_t Connection::SendTCPData
(
    char*       buffer,
    uint32_t    length
)
{
    le_result_t ret;

    if (buffer == NULL || length == 0)
    {
        LE_ERROR("Bad parameter.\n");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    if (le_timer_IsRunning(generalTimerRef))
    {
        le_timer_Restart(generalTimerRef);
    }

    ret = le_socket_Send(cliSockRef, buffer, length);
    if (ret != LE_OK)
    {
        LE_ERROR("Socket send data failed, error code is %d\n", ret);
        return TAF_DOIP_RESULT_ERROR;
    }

    return TAF_DOIP_RESULT_OK;
}

taf_doip_Result_t Connection::SendDoipMessage
(
    taf_doip_Buffer_t*  buffer
)
{
    return SendTCPData(buffer->data, buffer->dataSize);
}

taf_doip_Result_t Connection::SendDiagMessage
(
    char*       buffer,
    uint32_t    length
)
{
    // [DoIP-131]
    if (connState != TAF_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE)
    {
        LE_ERROR("Routing is inactive. It's state is %d\n", connState);
        return TAF_DOIP_RESULT_ROUTING_INACTIVE;
    }

    return SendTCPData(buffer, length);
}

taf_doip_Result_t Connection::ReceiveTCPData
(
    taf_doip_Buffer_t*  buffer
)
{
    le_result_t ret;
    size_t    received;
    taf_doip_Result_t doipResult;

    if (buffer == NULL)
    {
        LE_ERROR("Bad parameter.\n");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    // [DoIP-080] Reset general inactivity timer.
    if (le_timer_IsRunning(generalTimerRef))
    {
        le_timer_Restart(generalTimerRef);
    }
    else
    {
        le_timer_Start(generalTimerRef);
    }

    // Receive DoIP messages.
    received = TAF_DOIP_MAX_BUFFER_SIZE - (buffer->dataPos + buffer->dataSize);
    if (received <= TAF_DOIP_MAX_MSG_LEN_EXCEPT_UDS)
    {
        // No enough buffer, discard the incomplete messages, reset buffer.
        buffer->dataSize    = 0;
        buffer->dataPos     = 0;
        received            = TAF_DOIP_MAX_BUFFER_SIZE;
        LE_WARN("Discard the incomplete messages and reset the reception buffer");
    }

    uint32_t    wrPos = buffer->dataPos + buffer->dataSize;
    ret = le_socket_Read(cliSockRef, buffer->data + wrPos, &received);
    if (LE_OK != ret)
    {
        LE_ERROR("Failed to receive data from socket. Error code is %d\n", ret);
        return TAF_DOIP_RESULT_NETWORK_ERROR;
    }

    buffer->dataSize    += received;

    auto& parser = ProtocolParser::GetInstance();
    taf_doipHeader_t hdr;

    // Loop to process messages if each message is enough.
    while (buffer->dataSize >= TAF_DOIP_HEADER_GENERIC_LENGTH)
    {
        parser.UnpackHeaderStruct(buffer->data + buffer->dataPos, buffer->dataSize, &hdr);
        if (hdr.payloadType != TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_MESSAGE
            && hdr.payloadType != TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_POSITIVE_ACK
                && hdr.payloadType != TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_NEGATIVE_ACK)
        {
            // Check DoIP protocol header.
            doipResult = CheckDoipHeader(hdr, buffer);
            if (doipResult != TAF_DOIP_RESULT_OK)
            {
                LE_ERROR("DoIP header error!, ret=0x%x\n", doipResult);
                // Reset the buffer.
                buffer->dataSize    = 0;
                buffer->dataPos     = 0;
                return doipResult;
            }

            ProcessDoipMessage(hdr.payloadType, buffer, hdr.payloadLen);
        }
        else
        {
            // Only Diagnostic messages can run here.
            doipResult = CheckDoipHeader(hdr, buffer);
            if (doipResult != TAF_DOIP_RESULT_OK)
            {
                LE_ERROR("DoIP header error!, ret=0x%x\n", doipResult);
                // Reset the buffer.
                buffer->dataSize    = 0;
                buffer->dataPos     = 0;
                return doipResult;
            }

            ProcessDoipMessage(hdr.payloadType, buffer, hdr.payloadLen);
            break;
        }
    }

    return TAF_DOIP_RESULT_OK;
}

taf_doip_Result_t Connection::ReceiveDiagMessage
(
)
{
    le_result_t ret;
    size_t    received;

    le_timer_Restart(generalTimerRef);

    if (udsTotalLen > udsReceived)
    {
        if (udsTotalLen > le_mem_GetObjectSize(connectionMgr->udsMsgPool))
        {
            received = le_mem_GetObjectSize(connectionMgr->udsMsgPool) - udsReceived;
        }
        else
        {
            received = udsTotalLen - udsReceived;
        }


        ret = le_socket_Read(cliSockRef, udsBuf + udsReceived, &received);
        if (LE_OK != ret)
        {
            LE_ERROR("Failed to receive data from socket. Error code is %d\n", ret);
            return TAF_DOIP_RESULT_NETWORK_ERROR;
        }

        udsReceived += received;
    }

    if (udsTotalLen <= udsReceived
        || udsReceived == le_mem_GetObjectSize(connectionMgr->udsMsgPool))
    {
        // Receive all diagnostic message.
        DiagnosticMsgSecondHandler();
    }

    return TAF_DOIP_RESULT_OK;
}

taf_doip_Result_t Connection::CheckDoipHeader
(
    taf_doipHeader_t& header,
    taf_doip_Buffer_t* buffer
)
{
    taf_doipHeaderNACKCode_t    nackCode;
    uint32_t    mds;
    auto& vehicleMgr = VehicleManager::GetInstance();

    if (header.protocolVer != TAF_DOIP_PROTOCOL_VERSION_2010 &&
        header.protocolVer != TAF_DOIP_PROTOCOL_VERSION_2012 &&
        header.protocolVer != TAF_DOIP_PROTOCOL_VERSION_2019 &&
        header.protocolVer != TAF_DOIP_PROTOCOL_VERSION_DEFAULT)
    {
        // [DoIP-041] NACK code set to 0x00 when protocol version does not match.
        LE_ERROR("Protocol version[0x%02x] doesn't support!\n", header.protocolVer);
        nackCode = TAF_DOIP_HEADER_NACK_INCORRECT_PATTERN_FORMAT;
        goto errOut;
    }

    if (header.protocolVer != (uint8_t)~header.invProtocolVer)
    {
        // [DoIP-041] NACK code set to 0x00 when inverse protocol version does not match.
        LE_ERROR("Incorrect pattern format!version is [0x%02x], inverse version is [0x%02x]\n",
            header.protocolVer, header.invProtocolVer);
        nackCode = TAF_DOIP_HEADER_NACK_INCORRECT_PATTERN_FORMAT;
        goto errOut;
    }

    // Payload type is supported on TCP_DATA.
    if (connType == TAF_DOIP_CONNECTION_TYPE_SVR)
    {
        // We are DoIP entity.
        if ((header.payloadType != TAF_DOIP_PAYLOAD_TYPE_ROUTING_ACTIVE_REQUEST)
            && (header.payloadType != TAF_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_REQUEST)
            && (header.payloadType != TAF_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_RESPONSE)
            && (header.payloadType != TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_MESSAGE))
        {
            // [DoIP-042] NACK code set to 0x01 if the payload type is not supported.
            LE_ERROR("Unknow payload type!payload type is [0x%x]\n", header.payloadType);
            nackCode = TAF_DOIP_HEADER_NACK_UNKNOWN_PAYLOAD_TYPE;
            goto errOut;
        }
    }
    else if (connType == TAF_DOIP_CONNECTION_TYPE_CLI)
    {
        // We are tester.
        if ((header.payloadType != TAF_DOIP_PAYLOAD_TYPE_HEADER_NEGATIVE_ACK)
            && (header.payloadType != TAF_DOIP_PAYLOAD_TYPE_ROUTING_ACTIVE_RESPONSE)
            && (header.payloadType != TAF_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_REQUEST)
            && (header.payloadType != TAF_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_RESPONSE)
            && (header.payloadType != TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_MESSAGE)
            && (header.payloadType != TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_POSITIVE_ACK)
            && (header.payloadType != TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_NEGATIVE_ACK))
        {
            // Not need to send DoIP header nack as a tester.
            LE_ERROR("Unknow payload type!payload type is [0x%x]\n", header.payloadType);
            return TAF_DOIP_RESULT_HDR_ERROR;
        }
    }

    if (vehicleMgr.GetMaxDataSize(&mds) != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to get max data size, using default!\n");
        mds = TAF_DOIP_MDS_DEFAULT;
    }

    if (header.payloadLen > (mds - TAF_DOIP_HEADER_GENERIC_LENGTH))
    {
        // [DoIP-043] NACK code set to 0x02 if the payload length exceeds
        // the maximum DoIP message size.
        LE_ERROR("DoIP message is too large! payload len is %d, mds is %d\n",
            header.payloadLen, mds);
        nackCode = TAF_DOIP_HEADER_NACK_MESSAGE_TOO_LARGE;
        goto errOut;
    }

    if (header.payloadType != TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_MESSAGE
        && header.payloadLen > (TAF_DOIP_MAX_BUFFER_SIZE - TAF_DOIP_HEADER_GENERIC_LENGTH))
    {
        // [DoIP-044] NACK code set to 0x03 if the payload length exceeds
        // the currently available DoIP protocol handler memory.
        LE_ERROR("DoIP message is out of memory! payload len is %d\n",
            header.payloadLen);
        nackCode = TAF_DOIP_HEADER_NACK_OUT_OF_MEMORY;
        goto errOut;
    }

    return TAF_DOIP_RESULT_OK;

errOut:
    if (connType == TAF_DOIP_CONNECTION_TYPE_CLI)
    {
        // [DoIP-040] Tester shall not send DoIP header negative ack.
        return TAF_DOIP_RESULT_HDR_ERROR;
    }

    // [DoIP-038],[DoIP-087]
    LE_DEBUG("payload len is %u, buffer data size is %u", header.payloadLen, buffer->dataSize);
    if ((header.payloadLen + TAF_DOIP_HEADER_GENERIC_LENGTH) > buffer->dataSize)
    {
        RespondHeaderNegativeACK(nackCode,
            header.payloadLen + TAF_DOIP_HEADER_GENERIC_LENGTH - buffer->dataSize);
    }
    else
    {
        RespondHeaderNegativeACK(nackCode, 0);
    }

    return TAF_DOIP_RESULT_HDR_ERROR;
}

void Connection::ProcessDoipMessage
(
    uint16_t            payloadType,
    taf_doip_Buffer_t*  buffer,
    uint32_t            payloadLen
)
{
    taf_doipLink_t link;
    uint32_t payloadPos;
    auto& parser = ProtocolParser::GetInstance();

    // Skip DoIP header length.
    payloadPos = buffer->dataPos + TAF_DOIP_HEADER_GENERIC_LENGTH;

    switch (payloadType)
    {
    case TAF_DOIP_PAYLOAD_TYPE_HEADER_NEGATIVE_ACK:
        // [DoIP-039]Ignored if entity recevie header nack.
        break;
    case TAF_DOIP_PAYLOAD_TYPE_ROUTING_ACTIVE_REQUEST:
        // [DoIP-057] Support routing activation request.
        LE_DEBUG("Routing activation request is receiving.\n");
        RoutingActiveReqHandler(buffer->data + payloadPos,
                                payloadLen);
        break;
    case TAF_DOIP_PAYLOAD_TYPE_ROUTING_ACTIVE_RESPONSE:
        LE_DEBUG("Routing activation response is receiving.\n");
        // We didn't support tester functionality yet.
        break;
    case TAF_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_REQUEST:
        LE_DEBUG("Alive check request is receiving.\n");
        link.commType = TAF_DOIP_SOCKET_TYPE_TCP;
        link.sockRef = cliSockRef;

        parser.AliveCheckRes(&link, testerSA);
        break;
    case TAF_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_RESPONSE:
        LE_DEBUG("Alive check response is receiving.\n");
        AliveCheckResHandler(buffer->data + payloadPos,
                             payloadLen);
        break;
    case TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_MESSAGE:
        if (buffer->dataSize < (TAF_DOIP_HEADER_GENERIC_LENGTH
            + 2 * TAF_DOIP_LOGICAL_ADDRESS_LENGTH))
        {
            // The diagnostic shall be received more than 12bytes before handling.
            return;
        }
        LE_DEBUG("Diagnostic message is receiving.\n");
        DiagnosticMsgFirstHandler(buffer->data + payloadPos, payloadLen,
                buffer->dataSize - TAF_DOIP_HEADER_GENERIC_LENGTH);
        break;
    case TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_POSITIVE_ACK:
        LE_DEBUG("Diagnostic positive ack is receiving.\n");
        // Only tester can receive.We didn't support tester functionality yet.
        break;
    case TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_NEGATIVE_ACK:
        LE_DEBUG("Diagnostic negative ack is receiving.\n");
        // Only tester can receive. We didn't support tester functionality yet.
        break;
    default:
        LE_DEBUG("Invalid payload type(0x%x) for Connection.\n", payloadType);
        break;
    }

    if ((payloadLen + TAF_DOIP_HEADER_GENERIC_LENGTH) > buffer->dataSize)
    {
        // Only diagnostic payload can enter this branch.
        // Diagnostic message received incompletely.
        // The diagnostic message was copied to uds buffer,
        // so here we reset the current buffer for next reception.
        buffer->dataSize    = 0;
        buffer->dataPos     = 0;
    }
    else
    {
        buffer->dataSize    -= payloadLen + TAF_DOIP_HEADER_GENERIC_LENGTH;
        buffer->dataPos     += payloadLen + TAF_DOIP_HEADER_GENERIC_LENGTH;
        if (buffer->dataSize == 0)
        {
            // If there is not data in buffer, Shall receive data at start position of the buffer.
            // Therefore, the buffer will be maximized.
            buffer->dataPos = 0;
        }
    }
}

void Connection::RoutingActiveReqHandler
(
    char*       payload,
    uint32_t    payloadLen
)
{
    auto& parser = ProtocolParser::GetInstance();
    taf_doipLink_t link;
    taf_doipRaResCode_t resCode = TAF_DOIP_RA_RES_ROUTING_SUCCESSFULLY_ACTIVATED;

    // [DoIP-100] process the routing activation request message.
    if (TAF_DOIP_PAYLOAD_RA_MAND_LEN != payloadLen &&
        TAF_DOIP_PAYLOAD_RA_ALL_LEN != payloadLen)
    {
        LE_ERROR("Invalid payload length.\n");
        RespondHeaderNegativeACK(TAF_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH, 0);
        return;
    }

    uint16_t    pos = 0;
    uint16_t    fromSa = 0x0000;
    uint16_t    ourSa = entitySA;
    uint8_t     type = 0;
    uint32_t    raReserved;
    uint32_t    oemReserved = 0;
    size_t      registeredCnt;

    memcpy(&fromSa, payload + pos, TAF_DOIP_LOGICAL_ADDRESS_LENGTH);
    fromSa = ntohs(fromSa);
    pos += TAF_DOIP_LOGICAL_ADDRESS_LENGTH;

    if (fromSa < 0x0E00 || fromSa > 0x0FFF)
    {
        // [DoIP-059] Unknown source address.
        LE_ERROR("Unknown source address-0x%02x in routing activation request.\n", fromSa);
        resCode = TAF_DOIP_RA_RES_UNKNOWN_SOURCE_ADDRESS;
        goto errOut;
    }

    raTesterSa = fromSa;
    memcpy(&type, payload + pos, TAF_DOIP_RA_ACTIVATION_TYPE_LEN);
    pos += TAF_DOIP_RA_ACTIVATION_TYPE_LEN;

    if (type >= 0x02 && type <= 0xDF)
    {
        // [DoIP-151] Routing activation type is not supported.
        LE_ERROR("Routing activation type is not supported in routing activation request.\n");
        resCode = TAF_DOIP_RA_RES_UNSUPPORTED_RA_TYPE;
        goto errOut;
    }

    memcpy(&raReserved, payload + pos, TAF_DOIP_RA_RESERVED_LEN);
    raReserved = ntohl(raReserved);
    pos += TAF_DOIP_RA_RESERVED_LEN;

    if (payloadLen == TAF_DOIP_PAYLOAD_RA_ALL_LEN)
    {
        memcpy(&oemReserved, payload + pos, TAF_DOIP_RA_RESERVED_LEN);
        oemReserved = ntohl(oemReserved);
        pos += TAF_DOIP_RA_RESERVED_LEN;
    }

    LE_DEBUG("Tester SA is 0x%x, activation type is 0x%x, "
        "iso13400 reserved is 0x%x, oem reserved is 0x%x\n",
        fromSa, type, raReserved, oemReserved);

    // [DoIP-128][DoIP-085] Stop initail inactivity timer.
    le_timer_Stop(initialTimerRef);

    authInfo = oemReserved;

    registeredCnt = connectionMgr->GetRegisteredConnectionNum();
    if (registeredCnt == 0)
    {
        // Assign SA to this connection and jump to next routing activation step.
        testerSA = fromSa;
        ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_AUTH,
                oemReserved);
        return;
    }

    if (IsConnectionRegitered())
    {
        // [DoIP-089] SA is already assign to this connection.
        if (fromSa == testerSA)
        {
            // Assign SA to this connection and jump to next routing activation step.
            testerSA = fromSa;
            ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_AUTH,
                    oemReserved);
            return;
        }
        else
        {
            // [DoIP-106] Reject routing activation if SA is defferent from this connection.
            // [DoIP-149] An different SA was on the already activated connection.
            LE_ERROR("Received sa is not assigned to this ativated connection."
                "testSA is 0x%x, but recevied sa is 0x%x.", testerSA, fromSa);
            resCode = TAF_DOIP_RA_RES_SA_DIFFERENT_ACTIVATED_SOCKET;
            goto errOut;
        }
    }
    else
    {
        std::shared_ptr<Connection> connectionTmp(connectionMgr->
                                                  FindConnectionByLogicalAddr(fromSa));
        if (connectionTmp != nullptr && connectionTmp != shared_from_this())
        {
            // If alive check was already requested on this connection,
            // return and wait for response.
            if (!connectionTmp->singleAliveCheckFlag) {
                // [DoIP-091] Send an alive check request to another connection
                // which was assigned the same SA.
                // If the connection has a source address. It means it is registered.
                auto& parser = ProtocolParser::GetInstance();
                connectionTmp->routingConnectionPtr = shared_from_this();
                taf_doipLink_t link;
                link.commType = TAF_DOIP_SOCKET_TYPE_TCP;
                link.sockRef = connectionTmp->cliSockRef;
                parser.AliveCheckReq(&link);

                // Start the timer that is in another connection.
                connectionTmp->singleAliveCheckFlag = true;
                le_timer_Start(connectionTmp->aliveCheckTimerRef);
            }
        }
        else
        {
            uint32_t mcts = 0;
            auto& vehicleMgr = VehicleManager::GetInstance();

            if (vehicleMgr.GetMaxConcurrentSockNum(&mcts) != TAF_DOIP_RESULT_OK)
            {
                LE_ERROR("Failed to get max concurrent socket number.\n");
                mcts = TAF_DOIP_MCTS_DEFAULT;
            }

            if (registeredCnt == mcts)
            {
                // [DoIP-094] Send an alive check request to all connection if the MCTS is reached.
                connectionMgr->PerformAllAliveCheck(shared_from_this());
            }
            else
            {
                testerSA = fromSa;
                ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_AUTH,
                                       oemReserved);
            }
        }
    }

    return;

errOut:
    // Send nack
    link.commType = TAF_DOIP_SOCKET_TYPE_TCP;
    link.sockRef = cliSockRef;
    parser.RoutingActivationRes(&link, fromSa, ourSa, resCode, 0);

    // Disconnect
    //connectionMgr.DeleteConnection(shared_from_this());
    ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_FINALIZE, 0);

    return;
}

bool Connection::IsConnectionRegitered()
{
    if (connState == TAF_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE ||
        connState == TAF_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_CONF ||
        connState == TAF_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void Connection::StartAliveCheckTimer()
{
    le_timer_Start(aliveCheckTimerRef);
}

// Connection state machine.
void Connection::ConnectionStateMachine
(
    taf_doip_ConnectState_t state,
    uint32_t                authenInfo
)
{
    taf_doip_ConnectState_t preConnState = connState;
    connState = state;

    switch (state) {
    case TAF_DOIP_CONNECT_STATE_INITIALIZED:
        // Start initial timer and wait for routing activation request.
        le_timer_Start(initialTimerRef);
        break;
    case TAF_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_AUTH:
        CheckRoutingActivationAuthen(authenInfo);
        break;
    case TAF_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_CONF:
        CheckRoutingActivationConfirm();
        break;
    case TAF_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE:
        connectionMgr->ReportConnectionEvent(testerSA, entitySA,
                TAF_DOIP_RESULT_SA_REGISTERED, localIface);
        break;
    case TAF_DOIP_CONNECT_STATE_FINALIZE:
        // [DoIP-133] Tcp socket shall be closed and resources shall be freed.
        if (preConnState == TAF_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE)
        {
            connectionMgr->ReportConnectionEvent(testerSA, entitySA,
                TAF_DOIP_RESULT_SA_DEREGISTERED, localIface);
        }
        connectionMgr->DeleteConnection(shared_from_this());
        break;
    default:  // TAF_DOIP_CONNECT_STATE_LISTEN
        // This state is for Connection manager. Connection does not do anything.
        break;
    }
}

void Connection::CheckRoutingActivationAuthen
(
    uint32_t authenInfo
)
{
    auto& vehicleMgr = VehicleManager::GetInstance();

    if (vehicleMgr.GetAuthEnableStatus() == true)
    {
        uint32_t    vehicleAuthInfo = 0;

        vehicleMgr.GetAuthInfo(&vehicleAuthInfo);

        if (authenInfo == vehicleAuthInfo || 0 == vehicleAuthInfo)
        {
            // [DoIP-129] Jump to 'Registered[Pending for Confirmation]'.
            ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_CONF, 0);
        }
        else
        {
            // [DoIP-061] Authentication failed.
            auto& parser = ProtocolParser::GetInstance();
            taf_doipLink_t link;

            // Send nack
            link.commType = TAF_DOIP_SOCKET_TYPE_TCP;
            link.sockRef = cliSockRef;
            parser.RoutingActivationRes(&link, testerSA, entitySA,
                    TAF_DOIP_RA_RES_MISSING_AUTHENTICATION, 0);

            // [DoIP-132] Connection shall be set to state 'Finalize'
            ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_FINALIZE, 0);
        }
    }
    else
    {
        // [DoIP-129]
        ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_CONF, 0);
    }
}

void Connection::CheckRoutingActivationConfirm()
{
    taf_doip_UserConfirmResult_t result;
    taf_doipLink_t link;
    auto& parser = ProtocolParser::GetInstance();
    auto& cm = CommunicationMgr::GetInstance();

    result = cm.ConfirmRoutingActivation(testerSA, entitySA);
    if (result == TAF_DOIP_USER_CONFIRM_RESULT_PASS)
    {
        // Send ack
        link.commType = TAF_DOIP_SOCKET_TYPE_TCP;
        link.sockRef = cliSockRef;

        // Some testers did not support TAF_DOIP_RA_RES_CONFIRMATION_REQUIRED
        parser.RoutingActivationRes(&link, testerSA, entitySA,
            TAF_DOIP_RA_RES_ROUTING_SUCCESSFULLY_ACTIVATED, 0); // [DoIP-062] RA Success.
        // [DoIP-130]
        ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE, 0);
    }
    else if (result == TAF_DOIP_USER_CONFIRM_RESULT_NO_NEED)
    {
        // Send ack
        link.commType = TAF_DOIP_SOCKET_TYPE_TCP;
        link.sockRef = cliSockRef;
        parser.RoutingActivationRes(&link, testerSA, entitySA,
            TAF_DOIP_RA_RES_ROUTING_SUCCESSFULLY_ACTIVATED, 0); // [DoIP-062] RA Success.
        ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE, 0);
    }
    else
    {
        // Send nack
        link.commType = TAF_DOIP_SOCKET_TYPE_TCP;
        link.sockRef = cliSockRef;
        parser.RoutingActivationRes(&link, testerSA, entitySA,
            TAF_DOIP_RA_RES_REJECT_CONFIRMATION, 0);
        // [DoIP-132] Connection shall be set to state 'Finalize'
        ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_FINALIZE, 0);
    }
}

void Connection::AliveCheckResHandler
(
    char*       payload,
    uint32_t    payloadLen
)
{
    taf_doipLink_t link;
    auto& parser = ProtocolParser::GetInstance();

    if (payloadLen != TAF_DOIP_LOGICAL_ADDRESS_LENGTH)
    {
        LE_ERROR("Invalid payload length(0x%x)!", payloadLen);
        RespondHeaderNegativeACK(TAF_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH, 0);
        return;
    }

    uint16_t logicalAddr = 0;
    memcpy(&logicalAddr, payload, TAF_DOIP_LOGICAL_ADDRESS_LENGTH);
    logicalAddr = ntohs(logicalAddr);

    if (logicalAddr != testerSA)
    {
        LE_ERROR("Unexpected SA(0x%x), shall 0x%x\n", logicalAddr, testerSA);
        // Discard this message.
        return;
    }

    if (le_timer_IsRunning(aliveCheckTimerRef))
    {
        le_timer_Stop(aliveCheckTimerRef);
    }

    if (!allAliveCheckFlag && !singleAliveCheckFlag)
    {
        // The external test equipment use it to keep currently idle connection alive.
        LE_DEBUG("One-way acceptance of the alive check response from tester.\n");
        return;
    }

    uint32_t    mcts = 0;
    auto& vehicleMgr = VehicleManager::GetInstance();

    if (vehicleMgr.GetMaxConcurrentSockNum(&mcts) != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to get max concurrent socket number.\n");
        mcts = TAF_DOIP_MCTS_DEFAULT;
    }

    if (allAliveCheckFlag)
    {
        // alive check（all re/gistered sockets） response
        allAliveCheckFlag = false;
        connectionMgr->aliveCheckNcts++;
        if (connectionMgr->aliveCheckNcts == mcts)
        {
            LE_ERROR("All currently supported TCP_DATA sockets are registered and active\n");

            // [DoIP-096] All sockets active, reject routing activation request.
            if ((routingConnectionPtr != nullptr)
                    && (!routingConnectionPtr->IsConnectionRegitered()))
            {
                link.commType = TAF_DOIP_SOCKET_TYPE_TCP;
                link.sockRef = routingConnectionPtr->cliSockRef;
                // [DoIP-60]
                parser.RoutingActivationRes(&link, routingConnectionPtr->raTesterSa,
                    routingConnectionPtr->entitySA,
                    TAF_DOIP_RA_RES_ALL_SOCKET_REGISTED_AND_ACTIVE, 0);
                routingConnectionPtr->ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_FINALIZE, 0);
            }

            // Set back to the initial value for next alive check.
            connectionMgr->aliveCheckNcts = 0;
        }
    }

    if (singleAliveCheckFlag)
    {
        // alive check（single SA） response
        singleAliveCheckFlag = false;
        LE_ERROR("SA is already registered and active on this socket[%s].\n", remoteIp.c_str());

        // [DoIP-093] The socket which has same SA is activing. reject routing activation request.
        if ((routingConnectionPtr != nullptr) && (!routingConnectionPtr->IsConnectionRegitered()))
        {
            link.commType = TAF_DOIP_SOCKET_TYPE_TCP;
            link.sockRef = routingConnectionPtr->cliSockRef;
            // [DoIP-150]
            parser.RoutingActivationRes(&link, routingConnectionPtr->raTesterSa,
                    routingConnectionPtr->entitySA, TAF_DOIP_RA_RES_SA_REGISTED_DIFFERENT_SOCKET,
                            0);
            routingConnectionPtr->ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_FINALIZE, 0);
        }
    }
}

void Connection::DiagnosticMsgFirstHandler
(
    char*       payload,
    uint32_t    payloadLen,
    uint32_t    receivedLen
)
{
    if (udsBuf == NULL)
    {
        udsBuf = (char*)le_mem_ForceAlloc(connectionMgr->udsMsgPool);
        udsTotalLen = 0;
        udsReceived = 0;
    }

    memcpy(udsBuf, payload, receivedLen);

    // If during diagnostic reception, set the uds variable value
    udsTotalLen = payloadLen;
    udsReceived = receivedLen;

    if (udsTotalLen <= udsReceived
            || udsReceived == le_mem_GetObjectSize(connectionMgr->udsMsgPool))
    {
        DiagnosticMsgSecondHandler();
        return;
    }
}

void Connection::DiagnosticMsgSecondHandler
(
)
{
    if (connType == TAF_DOIP_CONNECTION_TYPE_SVR)
    {
        // Diagnostic from tester equipment request.
        return DiagnosticMsgSvrSecondHandler();
    }
    else
    {
        return DiagnosticMsgCliSecondHandler();
    }
}

void Connection::DiagnosticMsgSvrSecondHandler
(
)
{
    uint16_t                logicalSourceAddr = 0;
    uint16_t                logicalTargetAddr = 0;
    taf_doipDiagNACKCode_t  nackCode;
    taf_doipLink_t          link;
    uint32_t                pos = 0UL;

    auto& parser = ProtocolParser::GetInstance();
    auto&       vehicleMgr = VehicleManager::GetInstance();
    uint32_t    mds;

    link.commType = TAF_DOIP_SOCKET_TYPE_TCP;
    link.sockRef = cliSockRef;

    if (udsTotalLen <= (TAF_DOIP_LOGICAL_ADDRESS_LENGTH * 2))
    {
        // Diagnostic payload length is at least 5 bytes.
        RespondHeaderNegativeACK(TAF_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH, 0);
        goto errOut2;
    }

    memcpy(&logicalSourceAddr, udsBuf + pos, TAF_DOIP_LOGICAL_ADDRESS_LENGTH);
    logicalSourceAddr = ntohs(logicalSourceAddr);
    pos += TAF_DOIP_LOGICAL_ADDRESS_LENGTH;

    memcpy(&logicalTargetAddr, udsBuf + pos, TAF_DOIP_LOGICAL_ADDRESS_LENGTH);
    logicalTargetAddr = ntohs(logicalTargetAddr);
    pos += TAF_DOIP_LOGICAL_ADDRESS_LENGTH;

    // [DoIP-131]
    if (connState != TAF_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE)
    {
        LE_ERROR("Connection don't active.Discard message\n");
        nackCode = TAF_DOIP_DIAGNOSTIC_NACK_TARGET_UNREACHABLE;
        goto errOut1;
    }

    if (logicalSourceAddr != testerSA)
    {
        LE_ERROR("Invalid SA(0x%x). Expect 0x%x\n", logicalSourceAddr, testerSA);
        nackCode =  TAF_DOIP_DIAGNOSTIC_NACK_INVALID_SA;
        goto errOut1;
    }

    if (vehicleMgr.GetMaxDataSize(&mds) != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to get max data size, using default!\n");
        mds = TAF_DOIP_MDS_DEFAULT;
    }

    if (udsTotalLen > mds)
    {
        // The payload length is longer than MDS.
        LE_ERROR("Diagnostic length(0x%x) is longer than MDS(0x%x)\n",
                 udsTotalLen, mds);
        nackCode =  TAF_DOIP_DIAGNOSTIC_NACK_DIAG_MSG_TOO_LARGE;
        goto errOut1;
    }
    else if (udsTotalLen > le_mem_GetObjectSize(connectionMgr->udsMsgPool))
    {
        LE_ERROR("Diagnostic length(0x%x) is longer than buffer size(0x%x)\n",
            udsTotalLen, (uint32_t)le_mem_GetObjectSize(connectionMgr->udsMsgPool));
        nackCode =  TAF_DOIP_DIAGNOSTIC_NACK_OUT_OF_MEMORY;
        goto errOut1;
    }
    else
    {
        taf_doip_Result_t ret = connectionMgr->InformUdsMessage(logicalSourceAddr,
                logicalTargetAddr, udsBuf, udsTotalLen, localIface);
        if (ret == TAF_DOIP_RESULT_UNKNOWN_TA)
        {
            LE_ERROR("Invalid TA(0x%x).", logicalTargetAddr);
            nackCode =  TAF_DOIP_DIAGNOSTIC_NACK_UNKNOWN_TA;
            goto errOut1;
        }
        else if (ret != TAF_DOIP_RESULT_OK)
        {
            LE_ERROR("Transfer error\n");
            nackCode =  TAF_DOIP_DIAGNOSTIC_NACK_TRANS_PROTO_ERROR;
            goto errOut1;
        }
    }

    // Set to zero. So can receive uds again.
    udsTotalLen = 0;
    udsReceived = 0;

    // The uds buffer is sending to uds thread. Shall alloc another buffer for uds.
    udsBuf = NULL;
    parser.DiagPositiveACK(&link, logicalTargetAddr, logicalSourceAddr);
    return;

errOut1:
    parser.DiagNegativeACK(&link, logicalTargetAddr, logicalSourceAddr, nackCode);
    if (nackCode == TAF_DOIP_DIAGNOSTIC_NACK_INVALID_SA)
    {
        ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_FINALIZE, 0);
    }
errOut2:
    // Discard diagnostic message.
    udsTotalLen = 0;
    udsReceived = 0;
    if (udsBuf)
    {
        le_mem_Release(udsBuf);
        udsBuf = NULL;
    }
    return;
}

void Connection::DiagnosticMsgCliSecondHandler
(
)
{
    return;
}

void Connection::SocketEventCallback
(
    le_socket_Ref_t sockRef,
    short           events,
    void*           userPtr
)
{
    std::shared_ptr<Connection> connectionPtr;

    LE_DEBUG("Enter tcp SocketEventCallback, event%x...", events);

    auto& commMgr = CommunicationMgr::GetInstance();
    connectionPtr = commMgr.GetConnectionMgr()->FindConnectionBySocket(sockRef);
    if (connectionPtr == nullptr)
    {
        LE_ERROR("Unknow socket reference.");
        return;
    }

    LE_DEBUG("socket ref is %p, connection is %p", sockRef, connectionPtr.get());

    // Check if Connection is disconnected.
    if (events & POLLRDHUP)
    {
        connectionPtr->ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_FINALIZE, 0);
        return;
    }

    if (events & POLLIN)
    {
        LE_DEBUG("uds total size is %d, received size is %d",
            connectionPtr->udsTotalLen, connectionPtr->udsReceived);
        if (connectionPtr->udsTotalLen > connectionPtr->udsReceived)
        {
            connectionPtr->ReceiveDiagMessage();
        }
        else
        {
            LE_DEBUG("protocol buffer address: %p", connectionPtr->inBuf);
            connectionPtr->ReceiveTCPData(connectionPtr->inBuf);
        }
    }

    if (events & POLLOUT)
    {
        return;
    }
}

void Connection::InitialTimerHandler
(
    le_timer_Ref_t  timerRef
)
{
    std::shared_ptr<Connection> connectionPtr;
    le_socket_Ref_t sockRef = (le_socket_Ref_t)le_timer_GetContextPtr(timerRef);

    LE_DEBUG("Initial inactive timer timeout.\n");

    auto& commMgr = CommunicationMgr::GetInstance();
    connectionPtr = commMgr.GetConnectionMgr()->FindConnectionBySocket(sockRef);
    if (connectionPtr == nullptr)
    {
        LE_ERROR("Unknow socket reference.");
        return;
    }

    le_timer_Stop(timerRef);

    connectionPtr->ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_FINALIZE, 0);
}

void Connection::GeneralTimerHandler
(
    le_timer_Ref_t  timerRef
)
{
    std::shared_ptr<Connection> connectionPtr;
    le_socket_Ref_t sockRef = (le_socket_Ref_t)le_timer_GetContextPtr(timerRef);

    LE_DEBUG("General inactive timer timeout.\n");
    auto& commMgr = CommunicationMgr::GetInstance();
    connectionPtr = commMgr.GetConnectionMgr()->FindConnectionBySocket(sockRef);
    if (connectionPtr == nullptr)
    {
        LE_ERROR("Unknow socket reference.");
        return;
    }

    le_timer_Stop(timerRef);

    connectionPtr->ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_FINALIZE, 0);
}

void Connection::AliveCheckTimerHandler
(
    le_timer_Ref_t  timerRef
)
{
    std::shared_ptr<Connection>  connectionPtr;
    std::shared_ptr<Connection>  routingPtr;

    LE_DEBUG("Alive check timer timeout.\n");
    le_socket_Ref_t sockRef = (le_socket_Ref_t)le_timer_GetContextPtr(timerRef);

    auto& commMgr = CommunicationMgr::GetInstance();
    connectionPtr = commMgr.GetConnectionMgr()->FindConnectionBySocket(sockRef);
    if (connectionPtr == nullptr)
    {
        LE_ERROR("Unknow socket reference.");
        return;
    }

    le_timer_Stop(timerRef);

    if (!connectionPtr->allAliveCheckFlag
        && !connectionPtr->singleAliveCheckFlag)
    {
        LE_DEBUG("Alive check timeout but we do not perform alive check!\n");
        return;
    }

    if (connectionPtr->allAliveCheckFlag)
    {
        uint32_t    mcts;
        auto& vehicleMgr = VehicleManager::GetInstance();

        if (vehicleMgr.GetMaxConcurrentSockNum(&mcts) != TAF_DOIP_RESULT_OK)
        {
            LE_ERROR("Failed to get max concurrent socket number.\n");
            mcts = TAF_DOIP_MCTS_DEFAULT;
        }

        connectionPtr->connectionMgr->aliveCheckNcts++;
        if (connectionPtr->connectionMgr->aliveCheckNcts == mcts)
        {
            connectionPtr->connectionMgr->aliveCheckNcts = 0;
        }
    }

    routingPtr = connectionPtr->routingConnectionPtr;
    if (routingPtr != nullptr)
    {
        // The registered connection does not receive an alive check response.
        // Assigning the SA to the newly requested connection.
        if (!routingPtr->IsConnectionRegitered())
        {
            routingPtr->testerSA = routingPtr->raTesterSa;
            routingPtr->ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_AUTH,
                    routingPtr->authInfo);
        }
    }

    // Close the connection and remove it.
    connectionPtr->ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_FINALIZE, 0);
}

void Connection::RespondHeaderNegativeACK
(
    taf_doipHeaderNACKCode_t nackCode,
    uint32_t left
)
{
    auto& parser = ProtocolParser::GetInstance();
    taf_doipLink_t link;

    link.commType = TAF_DOIP_SOCKET_TYPE_TCP;
    link.sockRef = cliSockRef;
    parser.HeaderNegativeACK(&link, nackCode);

    // [DoIP-087] Close socket if 'Incorrect pattern format' and Invalid payload length.
    if (nackCode == TAF_DOIP_HEADER_NACK_INCORRECT_PATTERN_FORMAT
        || nackCode == TAF_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH)
    {
        ConnectionStateMachine(TAF_DOIP_CONNECT_STATE_FINALIZE, 0);
    }
    else
    {
        ReadAndDiscardMsg((size_t)left);
    }
}

void Connection::ReadAndDiscardMsg
(
    size_t len
)
{
    LE_DEBUG("ReadAndDiscardMsg");

    size_t receivedLen;
    size_t bufPos = 0;

    if (len == 0)
    {
        return;
    }

    // Local buffer.
    char buff[len] = {0};

    do
    {
        receivedLen = len;
        le_result_t ret = le_socket_Read(cliSockRef, buff + bufPos, &receivedLen);
        if (LE_OK != ret)
        {
            LE_ERROR("Failed to receive data from socket. Error code is %d\n", ret);
            break;
        }

        bufPos += receivedLen;
        len -= receivedLen;
    } while (len > 0);

}
