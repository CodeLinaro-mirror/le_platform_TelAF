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

#include "tafDoIPVehDiscoveryAndParser.hpp"
#include "tafDoIPVehicleMgr.hpp"
#include "tafDoIPCommunicationMgr.hpp"

using namespace taf::doip;

/**
 * Returns ProtocolParser instance
 */
ProtocolParser &ProtocolParser::GetInstance()
{
    static ProtocolParser instance;
    return instance;
}

// Pack DoIP generic header
void ProtocolParser::PackGenericHeader
(
    char* headerPtr,
    uint16_t payloadType,
    uint32_t payloadLen
)
{
    LE_DEBUG("PackGenericHeader!");

    if (headerPtr == NULL)
    {
        LE_ERROR("headerPtr is null!");
        return;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    uint8_t protocolVer, invProtocolVer;
    uint8_t headerPos = 0;

    protocolVer = vehicleMgr.GetProtocolVersion();
    memcpy(headerPtr, &protocolVer, TAF_DOIP_PROTOCOL_VERSION_LENGTH);
    headerPos += TAF_DOIP_PROTOCOL_VERSION_LENGTH;

    invProtocolVer = ~(protocolVer);
    memcpy(headerPtr + headerPos, &invProtocolVer, TAF_DOIP_INVERSE_PROTOCOL_VERSION_LENGTH);
    headerPos += TAF_DOIP_INVERSE_PROTOCOL_VERSION_LENGTH;

    payloadType = htons(payloadType);
    memcpy(headerPtr + headerPos, &payloadType,TAF_DOIP_PAYLOAD_TYPE_LENGTH);
    headerPos += TAF_DOIP_PAYLOAD_TYPE_LENGTH;

    payloadLen = htonl(payloadLen);
    memcpy(headerPtr + headerPos, &payloadLen, TAF_DOIP_PAYLOAD_LENGTH);
    headerPos += TAF_DOIP_PAYLOAD_LENGTH;
}

// Pack and announce vehcile discovery msg or response vehicle identification
void ProtocolParser::VehicleAnnounceOrIdRes
(
    taf_doipLink_t *linkPtr,
    uint16_t entityLA,
    uint8_t furtherAction
)
{
    LE_DEBUG("VehicleAnnounceOrIdRes!");

    if (linkPtr == NULL)
    {
        LE_ERROR("linkPtr is null!");
        return;
    }

    auto &parser = ProtocolParser::GetInstance();
    auto &vehicleMgr = VehicleManager::GetInstance();

    uint16_t payloadType = TAF_DOIP_PAYLOAD_TYPE_VEHICLE_ID_RESPONSE;
    uint32_t payloadLen, dataPackSize;

    payloadLen = TAF_DOIP_VEHICLE_INFO_MAND_LENGTH;
    dataPackSize = payloadLen + TAF_DOIP_HEADER_GENERIC_LENGTH;

    char vehIdResData[dataPackSize];
    parser.PackGenericHeader(vehIdResData, payloadType, payloadLen);

    uint8_t headerPos = TAF_DOIP_HEADER_GENERIC_LENGTH;

    char vin[TAF_DOIP_VIN_SIZE];
    vehicleMgr.GetVin(vin);

    char eid[TAF_DOIP_EID_SIZE];
    vehicleMgr.GetEid(eid);

    char gid[TAF_DOIP_GID_SIZE];
    vehicleMgr.GetGid(gid);

    memcpy(vehIdResData + headerPos, vin, TAF_DOIP_VIN_SIZE);
    headerPos += TAF_DOIP_VIN_SIZE;

    entityLA = htons(entityLA);
    memcpy(vehIdResData + headerPos, &entityLA, TAF_DOIP_LOGICAL_ADDRESS_LENGTH);
    headerPos += TAF_DOIP_LOGICAL_ADDRESS_LENGTH;

    memcpy(vehIdResData + headerPos, eid, TAF_DOIP_EID_SIZE);
    headerPos += TAF_DOIP_EID_SIZE;

    memcpy(vehIdResData + headerPos, gid, TAF_DOIP_GID_SIZE);
    headerPos += TAF_DOIP_GID_SIZE;

    memcpy(vehIdResData + headerPos, &furtherAction, TAF_DOIP_FURTHER_ACTION_LENGTH);
    headerPos += TAF_DOIP_FURTHER_ACTION_LENGTH;

    if(linkPtr->commType == TAF_DOIP_SOCKET_TYPE_UDP_UNI)
    {
        parser.SendDoIPMsg(linkPtr, TAF_DOIP_VEHICLE_IDENTIFY_RESPONSE,
                vehIdResData, dataPackSize);
    }
    else if (linkPtr->commType == TAF_DOIP_SOCKET_TYPE_UDP_MULTI)
    {
        parser.SendDoIPMsg(linkPtr, TAF_DOIP_VEHICLE_ANNOUNCEMENT,
                vehIdResData, dataPackSize);
    }
}

// Routing activation response
void ProtocolParser::RoutingActivationRes
(
    taf_doipLink_t *linkPtr,
    uint16_t equipLA,
    uint16_t entityLA,
    taf_doipRaResCode_t raResCode,
    uint32_t reserved
)
{
    LE_DEBUG("RoutingActivationRes!");

    if (linkPtr == NULL)
    {
        LE_ERROR("linkPtr is null!");
    }

    auto &parser = ProtocolParser::GetInstance();

    uint32_t payloadLen = TAF_DOIP_PAYLOAD_RA_RES_LENGTH;
    uint32_t dataPackSize = payloadLen + TAF_DOIP_HEADER_GENERIC_LENGTH;
    char raResData[dataPackSize];

    uint16_t payloadType = TAF_DOIP_PAYLOAD_TYPE_ROUTING_ACTIVE_RESPONSE;
    parser.PackGenericHeader(raResData, payloadType, payloadLen);

    equipLA = htons(equipLA);
    uint8_t headerPos = TAF_DOIP_HEADER_GENERIC_LENGTH;
    memcpy(raResData + headerPos, &equipLA, TAF_DOIP_LOGICAL_ADDRESS_LENGTH);
    headerPos += TAF_DOIP_LOGICAL_ADDRESS_LENGTH;

    entityLA = htons(entityLA);
    memcpy(raResData + headerPos, &entityLA, TAF_DOIP_LOGICAL_ADDRESS_LENGTH);
    headerPos += TAF_DOIP_LOGICAL_ADDRESS_LENGTH;

    memcpy(raResData + headerPos, &raResCode, TAF_DOIP_RA_RES_CODE_LENGTH);
    headerPos += TAF_DOIP_RA_RES_CODE_LENGTH;

    memcpy(raResData + headerPos, &reserved, TAF_DOIP_RESERVED_LENGTH);
    headerPos += TAF_DOIP_RESERVED_LENGTH;

    parser.SendDoIPMsg(linkPtr, TAF_DOIP_ROUTING_ACTIVE_RESPONSE, raResData,
            dataPackSize);
}

// Alive check request msg
void ProtocolParser::AliveCheckReq
(
    taf_doipLink_t *linkPtr
)
{
    LE_DEBUG("AliveCheckReq!");

    if (linkPtr == NULL)
    {
        LE_ERROR("linkPtr is null!");
    }

    auto &parser = ProtocolParser::GetInstance();

    uint16_t payloadType = TAF_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_REQUEST;
    uint32_t payloadLen = 0x00;
    uint32_t dataPackSize = payloadLen + TAF_DOIP_HEADER_GENERIC_LENGTH;
    char aliveCheckReqHeader[dataPackSize];

    parser.PackGenericHeader(aliveCheckReqHeader, payloadType, payloadLen);

    parser.SendDoIPMsg(linkPtr, TAF_DOIP_ALIVE_CHECK_REQUEST, aliveCheckReqHeader, dataPackSize);
}

// Alive check response
void ProtocolParser::AliveCheckRes
(
    taf_doipLink_t *linkPtr,
    uint16_t equipLA
)
{
    LE_DEBUG("AliveCheckRes!");

    if (linkPtr == NULL)
    {
        LE_ERROR("linkPtr is null!");
    }

    auto &parser = ProtocolParser::GetInstance();

    uint32_t payloadLen = TAF_DOIP_LOGICAL_ADDRESS_LENGTH;
    uint32_t dataPackSize = payloadLen + TAF_DOIP_HEADER_GENERIC_LENGTH;
    char aliveCheckResData[dataPackSize];

    uint16_t payloadType = TAF_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_RESPONSE;
    parser.PackGenericHeader(aliveCheckResData, payloadType, payloadLen);

    uint8_t headerPos = TAF_DOIP_HEADER_GENERIC_LENGTH;
    equipLA = htons(equipLA);
    memcpy(aliveCheckResData + headerPos, &equipLA, TAF_DOIP_LOGICAL_ADDRESS_LENGTH);
    headerPos += TAF_DOIP_LOGICAL_ADDRESS_LENGTH;

    parser.SendDoIPMsg(linkPtr, TAF_DOIP_ALIVE_CHECK_RESPONSE, aliveCheckResData, dataPackSize);
}

// Diagnostic Power mode response
void ProtocolParser::DiagPowerModeRes
(
    taf_doipLink_t *linkPtr,
    uint8_t powerMode
)
{
    LE_DEBUG("DiagPowerModeRes!");

    if (linkPtr == NULL)
    {
        LE_ERROR("linkPtr is null!");
    }

    auto &parser = ProtocolParser::GetInstance();

    uint32_t payloadLen = TAF_DOIP_POWERMODE_LENGTH;
    uint32_t dataPackSize = payloadLen + TAF_DOIP_HEADER_GENERIC_LENGTH;
    char pwrModeData[dataPackSize];

    uint16_t payloadType = TAF_DOIP_PAYLOAD_TYPE_POWER_MODE_INFO_RESPONSE;
    parser.PackGenericHeader(pwrModeData, payloadType, payloadLen);

    uint8_t headerPos = TAF_DOIP_HEADER_GENERIC_LENGTH;

    memcpy(pwrModeData + headerPos, &powerMode, TAF_DOIP_POWERMODE_LENGTH);
    headerPos += TAF_DOIP_POWERMODE_LENGTH;

    parser.SendDoIPMsg(linkPtr, TAF_DOIP_POWER_MODE_INFO_RESPONSE, pwrModeData, dataPackSize);
}

// DoIP Entity Status response
void ProtocolParser::DoIPEntityStatusRes
(
    taf_doipLink_t *linkPtr,
    taf_doip_NodeType_t nodeType,
    uint8_t maxCTS,
    uint8_t numCTS,
    uint32_t maxDataSize
)
{
    LE_DEBUG("DoIPEntityStatusRes!");

    if (linkPtr == NULL)
    {
        LE_ERROR("linkPtr is null!");
    }

    auto &parser = ProtocolParser::GetInstance();

    uint32_t payloadLen = TAF_DOIP_ENTITY_STATUS_RES_LENGTH;
    uint32_t dataPackSize = payloadLen + TAF_DOIP_HEADER_GENERIC_LENGTH;
    char entityStatusData[dataPackSize];

    uint16_t payloadType = TAF_DOIP_PAYLOAD_TYPE_ENTITY_STATUS_RESPONSE;
    parser.PackGenericHeader(entityStatusData, payloadType, payloadLen);

    uint8_t headerPos = TAF_DOIP_HEADER_GENERIC_LENGTH;
    memcpy(entityStatusData + headerPos, &nodeType, TAF_DOIP_NODE_TYPE_LENGTH);
    headerPos += TAF_DOIP_NODE_TYPE_LENGTH;

    memcpy(entityStatusData + headerPos, &maxCTS, TAF_DOIP_MCTS_LENGTH);
    headerPos += TAF_DOIP_MCTS_LENGTH;

    memcpy(entityStatusData + headerPos, &numCTS, TAF_DOIP_NCTS_LENGTH);
    headerPos += TAF_DOIP_NCTS_LENGTH;

    memcpy(entityStatusData + headerPos, &maxDataSize, TAF_DOIP_MDS_LENGTH);
    headerPos += TAF_DOIP_MDS_LENGTH;

    parser.SendDoIPMsg(linkPtr, TAF_DOIP_ENTITY_STATUS_RESPONSE, entityStatusData,
            dataPackSize);
}

// Generic DoIP header negative acknowledgement
void ProtocolParser::HeaderNegativeACK
(
    taf_doipLink_t *linkPtr,
    taf_doipHeaderNACKCode_t headerNACKCode
)
{
    LE_DEBUG("HeaderNegativeACK!");

    if (linkPtr == NULL)
    {
        LE_ERROR("linkPtr is null!");
    }

    auto &parser = ProtocolParser::GetInstance();

    if(headerNACKCode < TAF_DOIP_HEADER_NACK_INCORRECT_PATTERN_FORMAT
        || headerNACKCode > TAF_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH)
    {
        LE_ERROR("Invalid negative acknowledge header code!");
        return;
    }

    uint32_t payloadLen = TAF_DOIP_NACK_CODE_LENGTH;
    uint32_t dataPackSize = payloadLen + TAF_DOIP_HEADER_GENERIC_LENGTH;
    char nACKData[dataPackSize];

    uint16_t payloadType = TAF_DOIP_PAYLOAD_TYPE_HEADER_NEGATIVE_ACK;
    parser.PackGenericHeader(nACKData, payloadType, payloadLen);

    uint8_t headerPos = TAF_DOIP_HEADER_GENERIC_LENGTH;
    memcpy(nACKData + headerPos, &headerNACKCode, TAF_DOIP_NACK_CODE_LENGTH);
    headerPos += TAF_DOIP_NACK_CODE_LENGTH;

    parser.SendDoIPMsg(linkPtr, TAF_DOIP_HEADER_NEGATIVE_ACK, nACKData, dataPackSize);
}

// Diagnostic negative acknowledgement
void ProtocolParser::DiagNegativeACK
(
    taf_doipLink_t *linkPtr,
    uint16_t entityLA,
    uint16_t equipLA,
    taf_doipDiagNACKCode_t diagNACKCode
)
{
    LE_DEBUG("DiagNegativeACK!");

    if (linkPtr == NULL)
    {
        LE_ERROR("linkPtr is null!");
    }

    auto &parser = ProtocolParser::GetInstance();

    if(diagNACKCode < TAF_DOIP_DIAGNOSTIC_NACK_INVALID_SA
        || diagNACKCode > TAF_DOIP_DIAGNOSTIC_NACK_TRANS_PROTO_ERROR)
    {
        LE_ERROR("Invalid negative acknowledge diagnostic code!");
        return;
    }

    uint32_t payloadLen = TAF_DOIP_DIAG_NEGATIVE_ACK_LENGTH;
    uint32_t dataPackSize = payloadLen + TAF_DOIP_HEADER_GENERIC_LENGTH;
    char diagNACKData[dataPackSize];

    uint16_t payloadType = TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_NEGATIVE_ACK;
    parser.PackGenericHeader(diagNACKData, payloadType, payloadLen);

    uint8_t headerPos = TAF_DOIP_HEADER_GENERIC_LENGTH;

    entityLA = htons(entityLA);
    memcpy(diagNACKData + headerPos, &entityLA, TAF_DOIP_LOGICAL_ADDRESS_LENGTH);
    headerPos += TAF_DOIP_LOGICAL_ADDRESS_LENGTH;

    equipLA = htons(equipLA);
    memcpy(diagNACKData + headerPos, &equipLA, TAF_DOIP_LOGICAL_ADDRESS_LENGTH);
    headerPos += TAF_DOIP_LOGICAL_ADDRESS_LENGTH;

    memcpy(diagNACKData + headerPos, &diagNACKCode, TAF_DOIP_NACK_CODE_LENGTH);
    headerPos += TAF_DOIP_NACK_CODE_LENGTH;

    parser.SendDoIPMsg(linkPtr, TAF_DOIP_DIAGNOSTIC_NEGATIVE_ACK,
            diagNACKData, dataPackSize);
}

// Diagnostic positive acknowledgement
void ProtocolParser::DiagPositiveACK
(
    taf_doipLink_t *linkPtr,
    uint16_t entityLA,
    uint16_t equipLA
)
{
    LE_DEBUG("DiagPositiveACK!");

    if (linkPtr == NULL)
    {
        LE_ERROR("linkPtr is null!");
    }

    auto &parser = ProtocolParser::GetInstance();

    uint32_t payloadLen = TAF_DOIP_DIAG_POSITIVE_ACK_LENGTH;
    uint32_t dataPackSize = payloadLen + TAF_DOIP_HEADER_GENERIC_LENGTH;
    char diagACKData[dataPackSize];

    uint16_t payloadType = TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_POSITIVE_ACK;
    parser.PackGenericHeader(diagACKData, payloadType, payloadLen);

    uint8_t headerPos = TAF_DOIP_HEADER_GENERIC_LENGTH;

    entityLA = htons(entityLA);
    memcpy(diagACKData + headerPos, &entityLA, TAF_DOIP_LOGICAL_ADDRESS_LENGTH);
    headerPos += TAF_DOIP_LOGICAL_ADDRESS_LENGTH;

    equipLA = htons(equipLA);
    memcpy(diagACKData + headerPos, &equipLA, TAF_DOIP_LOGICAL_ADDRESS_LENGTH);
    headerPos += TAF_DOIP_LOGICAL_ADDRESS_LENGTH;

    //memcpy(diagACKData + headerPos, &TAF_DOIP_DIAGNOSTIC_ACK, TAF_DOIP_ACK_CODE_LENGTH);
    *(diagACKData + headerPos) = TAF_DOIP_DIAGNOSTIC_ACK;
    headerPos += TAF_DOIP_ACK_CODE_LENGTH;

    parser.SendDoIPMsg(linkPtr, TAF_DOIP_DIAGNOSTIC_POSITIVE_ACK, diagACKData,
            dataPackSize);
}

//Send DoIP message
taf_doip_Result_t ProtocolParser::SendDoIPMsg
(
    taf_doipLink_t *linkPtr,
    uint8_t payloadType,
    char* dataPtr,
    size_t dataLen
)
{
    LE_DEBUG("SendDoIPMsg!");

    if (linkPtr == NULL)
    {
        LE_ERROR("linkPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }
    else if (dataPtr == NULL)
    {
        LE_ERROR("dataPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &commMgr = CommunicationMgr::GetInstance();

    switch(payloadType)
    {
        case TAF_DOIP_HEADER_NEGATIVE_ACK:
        {
            if(linkPtr->commType == TAF_DOIP_SOCKET_TYPE_TCP)
            {
                commMgr.SendTCPData(linkPtr->sockRef, dataPtr, dataLen);
            }
            else if(linkPtr->commType == TAF_DOIP_SOCKET_TYPE_UDP_MULTI)
            {
                commMgr.SendUDPData(linkPtr->sockRef, dataPtr, dataLen);
            }
            else if(linkPtr->commType == TAF_DOIP_SOCKET_TYPE_UDP_UNI)
            {
                commMgr.SendUDPData(linkPtr->sockRef, linkPtr->ip, linkPtr->port,
                        dataPtr, dataLen);
            }
            else
            {
                LE_ERROR("Invalid socket communication type!");
                return TAF_DOIP_RESULT_ERROR;
            }
            break;
        }
        case TAF_DOIP_VEHICLE_ANNOUNCEMENT:
        {
            if(linkPtr->commType == TAF_DOIP_SOCKET_TYPE_UDP_MULTI)
            {
                commMgr.SendUDPData(linkPtr->sockRef, dataPtr, dataLen);
            }
            else
            {
                LE_ERROR("Wrong socket communication type!");
                return TAF_DOIP_RESULT_ERROR;
            }
            break;
        }
        case TAF_DOIP_VEHICLE_IDENTIFY_RESPONSE:
        {
            if(linkPtr->commType == TAF_DOIP_SOCKET_TYPE_UDP_UNI)
            {
                commMgr.SendUDPData(linkPtr->sockRef, linkPtr->ip, linkPtr->port,
                        dataPtr, dataLen);
            }
            else
            {
                LE_ERROR("Wrong socket communication type!");
                return TAF_DOIP_RESULT_ERROR;
            }
            break;
        }
        case TAF_DOIP_ROUTING_ACTIVE_RESPONSE:
        {
            if(linkPtr->commType == TAF_DOIP_SOCKET_TYPE_TCP)
            {
                commMgr.SendTCPData(linkPtr->sockRef, dataPtr, dataLen);
            }
            else
            {
                LE_ERROR("Wrong socket communication type!");
                return TAF_DOIP_RESULT_ERROR;
            }
            break;
        }
        case TAF_DOIP_ALIVE_CHECK_REQUEST:
        {
            if(linkPtr->commType == TAF_DOIP_SOCKET_TYPE_TCP)
            {
                commMgr.SendTCPData(linkPtr->sockRef, dataPtr, dataLen);
            }
            else
            {
                LE_ERROR("Wrong socket communication type!");
                return TAF_DOIP_RESULT_ERROR;
            }
            break;
        }
        case TAF_DOIP_ALIVE_CHECK_RESPONSE:
        {
            if(linkPtr->commType == TAF_DOIP_SOCKET_TYPE_TCP)
            {
                commMgr.SendTCPData(linkPtr->sockRef, dataPtr, dataLen);
            }
            else
            {
                LE_ERROR("Wrong socket communication type!");
                return TAF_DOIP_RESULT_ERROR;
            }
            break;
        }
        case TAF_DOIP_POWER_MODE_INFO_RESPONSE:
        {
            if(linkPtr->commType == TAF_DOIP_SOCKET_TYPE_UDP_UNI)
            {
                commMgr.SendUDPData(linkPtr->sockRef, linkPtr->ip, linkPtr->port,
                        dataPtr, dataLen);
            }
            else
            {
                LE_ERROR("Wrong socket communication type!");
                return TAF_DOIP_RESULT_ERROR;
            }
            break;
        }
        case TAF_DOIP_ENTITY_STATUS_RESPONSE:
        {
            if(linkPtr->commType == TAF_DOIP_SOCKET_TYPE_UDP_UNI)
            {
                commMgr.SendUDPData(linkPtr->sockRef, linkPtr->ip, linkPtr->port,
                        dataPtr, dataLen);
            }
            else
            {
                LE_ERROR("Wrong socket communication type!");
                return TAF_DOIP_RESULT_ERROR;
            }
            break;
        }
        case TAF_DOIP_DIAGNOSTIC_POSITIVE_ACK:
        {
            if(linkPtr->commType == TAF_DOIP_SOCKET_TYPE_TCP)
            {
                commMgr.SendTCPData(linkPtr->sockRef, dataPtr, dataLen);
            }
            else
            {
                LE_ERROR("Wrong socket communication type!");
                return TAF_DOIP_RESULT_ERROR;
            }
            break;
        }
        case TAF_DOIP_DIAGNOSTIC_NEGATIVE_ACK:
        {
            if(linkPtr->commType == TAF_DOIP_SOCKET_TYPE_TCP)
            {
                commMgr.SendTCPData(linkPtr->sockRef, dataPtr, dataLen);
            }
            else
            {
                LE_ERROR("Wrong socket communication type!");
                return TAF_DOIP_RESULT_ERROR;
            }
            break;
        }
        default:
            LE_ERROR("Invalid payloadType error: %d", payloadType);
            break;
    }

    return TAF_DOIP_RESULT_OK;
}

//unpack doip received header structure
void ProtocolParser::UnpackHeaderStruct
(
    char *dataPtr,
    uint32_t datalen,
    taf_doipHeader_t *headerPtr
)
{
    LE_DEBUG("UnpackHeaderStruct!");

    if (dataPtr == NULL)
    {
        LE_ERROR("dataPtr is null!");
        return;
    }

    if (headerPtr == NULL)
    {
        LE_ERROR("headerPtr is null!");
        return;
    }

    uint8_t protocolVer, invProtocolVer;
    uint16_t payloadType;
    uint32_t dataLen;

    uint8_t headerPos = 0;
    memcpy(&protocolVer, dataPtr + headerPos, TAF_DOIP_PROTOCOL_VERSION_LENGTH);
    headerPos += TAF_DOIP_PROTOCOL_VERSION_LENGTH;

    memcpy(&invProtocolVer, dataPtr + headerPos, TAF_DOIP_INVERSE_PROTOCOL_VERSION_LENGTH);
    headerPos += TAF_DOIP_INVERSE_PROTOCOL_VERSION_LENGTH;

    memcpy(&payloadType, dataPtr + headerPos, TAF_DOIP_PAYLOAD_TYPE_LENGTH);
    payloadType = ntohs(payloadType);
    headerPos += TAF_DOIP_PAYLOAD_TYPE_LENGTH;

    memcpy(&dataLen, dataPtr + headerPos, TAF_DOIP_PAYLOAD_LENGTH);
    dataLen = ntohl(dataLen);
    headerPos += TAF_DOIP_PAYLOAD_LENGTH;

    headerPtr->protocolVer = protocolVer;
    headerPtr->invProtocolVer = invProtocolVer;
    headerPtr->payloadType = payloadType;
    headerPtr->payloadLen = dataLen;

    return;
}

/**
 * Returns VehicleDiscovery instance
 */
VehicleDiscovery &VehicleDiscovery::GetInstance()
{
    static VehicleDiscovery instance;
    return instance;
}

void VehicleDiscovery::Init
(
)
{
    LE_INFO("vehicle discovery announcment started");
    waitTimerRef = le_timer_Create("announce wait timer");
}

taf_doip_Result_t VehicleDiscovery::VehicleDiscoveryAnnounce
(
)
{
    LE_DEBUG("VehicleDiscoveryAnnounce!");

    auto &vehicleMgr = VehicleManager::GetInstance();
    auto &vehicleDis = VehicleDiscovery::GetInstance();

    vehicleMgr.GetMaxAnnounceCount(&vehicleDis.maxAnnounceCount);

    if (waitTimerRef != NULL)
    {
        LE_DEBUG("call vehicle Announce Handler!");
        uint32_t waitTime = le_rand_GetNumBetween(MIN_ANNOUNCE_WAIT_TIME, MAX_ANNOUNCE_WAIT_TIME);
        le_timer_SetMsInterval(waitTimerRef , waitTime);
        le_timer_SetRepeat(waitTimerRef , 1);
        le_timer_SetHandler(waitTimerRef , VehicleAnnounceTimerHandler);
        le_timer_Start(waitTimerRef);
        LE_DEBUG("Announcement remaining time: %d", le_timer_GetMsTimeRemaining(waitTimerRef));
        LE_DEBUG("called VehicleAnnounceTimerHandler!");
    }
    else
    {
        LE_INFO("Vehicle Discovery message already announced");
    }

    return TAF_DOIP_RESULT_OK;
}

void VehicleDiscovery::VehicleAnnounceTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("VehicleAnnounceTimerHandler!");

    auto &vehicleMgr = VehicleManager::GetInstance();
    auto &parser = ProtocolParser::GetInstance();
    auto &commMgr = CommunicationMgr::GetInstance();
    auto &vehicleDis = VehicleDiscovery::GetInstance();

    taf_doipLink_t link;
    char netType[TAF_DOIP_IPTYPE_MAX_LEN];

    vehicleMgr.GetNetType(netType);
    if (!strncasecmp(netType, "IPv4", 4))
    {
        link.sockRef = commMgr.udpEquipSockRef;
    }
    else
    {
        link.sockRef = commMgr.udpDiscoverSockRef;
    }

    vehicleMgr.GetMulticast(link.ip);
    vehicleMgr.GetUdpPort(&(link.port));
    link.commType = TAF_DOIP_SOCKET_TYPE_UDP_MULTI;

    uint16_t sa;
    vehicleMgr.GetEntityLogicalAddr(&sa);

    if (vehicleMgr.GetAuthEnableStatus() == true)
    {
        parser.VehicleAnnounceOrIdRes(&link, sa, TAF_DOIP_FURTHER_ACTION_REQUIRED);
    }
    else
    {
        parser.VehicleAnnounceOrIdRes(&link, sa, TAF_DOIP_NO_FURTHER_ACTION_REQUIRED);
    }

    vehicleDis.announceCount++;
    LE_INFO("%d Announcement completed", vehicleDis.announceCount);

    if (vehicleDis.announceCount < vehicleDis.maxAnnounceCount)
    {
        uint32_t announceIntTime;
        vehicleMgr.GetAnnounceIntervalTime(&announceIntTime);
        le_timer_SetMsInterval(timerRef , announceIntTime);
        le_timer_SetHandler(timerRef, VehicleAnnounceTimerHandler);
        le_timer_Start(timerRef);
        LE_INFO("Announcement remaining time: %d", le_timer_GetMsTimeRemaining(timerRef));
    }
    else if (vehicleDis.announceCount == vehicleDis.maxAnnounceCount)
    {
        LE_INFO("Vehicle discovery message announced!");
        vehicleDis.announceCount = 0;
        le_timer_Stop(timerRef);
        le_timer_Delete(timerRef);
    }
}

taf_doip_Result_t VehicleDiscovery::VehicleIdentityRes
(
    taf_doipLink_t *linkPtr
)
{
    LE_DEBUG("VehicleIdentityRes!");

    if (linkPtr == NULL)
    {
        LE_ERROR("linkPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    resTimerRef = le_timer_Create("announce wait timer");
    uint32_t waitTime = le_rand_GetNumBetween(MIN_ANNOUNCE_WAIT_TIME, MAX_ANNOUNCE_WAIT_TIME);
    le_timer_SetMsInterval(resTimerRef, waitTime);
    le_timer_SetRepeat(resTimerRef, 1);
    le_timer_SetHandler(resTimerRef, VehicleIdentityTimerHandler);
    le_timer_SetContextPtr(resTimerRef, (taf_doipLink_t *)linkPtr);
    le_timer_Start(resTimerRef);

    return TAF_DOIP_RESULT_OK;
}

void VehicleDiscovery::VehicleIdentityTimerHandler
(
    le_timer_Ref_t resTimerRef
)
{
    LE_DEBUG("VehicleIdentityTimerHandler!");

    auto &vehicleMgr = VehicleManager::GetInstance();
    auto &parser = ProtocolParser::GetInstance();

    taf_doipLink_t *linkContextPtr = (taf_doipLink_t *)le_timer_GetContextPtr(resTimerRef);

    uint16_t sa;
    vehicleMgr.GetEntityLogicalAddr(&sa);

    if (vehicleMgr.GetAuthEnableStatus() == true)
    {
        parser.VehicleAnnounceOrIdRes(linkContextPtr, sa, TAF_DOIP_FURTHER_ACTION_REQUIRED);
    }
    else
    {
        parser.VehicleAnnounceOrIdRes(linkContextPtr, sa, TAF_DOIP_NO_FURTHER_ACTION_REQUIRED);
    }

    if(le_timer_IsRunning(resTimerRef) == true)
    {
        le_timer_Stop(resTimerRef);
        le_timer_Delete(resTimerRef);
    }
}