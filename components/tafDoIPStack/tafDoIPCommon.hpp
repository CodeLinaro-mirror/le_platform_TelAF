/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef TAFDOIP_COMMON_HPP
#define TAFDOIP_COMMON_HPP

#define TAF_DOIP_PROTOCOL_VERSION_2010      0x1     // ISO 13400-2010
#define TAF_DOIP_PROTOCOL_VERSION_2012      0x2     // ISO 13400-2012
#define TAF_DOIP_PROTOCOL_VERSION_2019      0x3     // ISO 13400-2019
#define TAF_DOIP_PROTOCOL_VERSION_DEFAULT   0xFF

//-------------------------------------------------------------------------------------------------
/**
 * The IPv4 address maximum length.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_IPV4_ADDR_MAX_LEN  16

//-------------------------------------------------------------------------------------------------
/**
 * The IPv6 address ("ffff:ffff:ffff:ffff:ffff:ffff:255:255:255:255") maximum length.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_IPV6_ADDR_MAX_LEN  46

//-------------------------------------------------------------------------------------------------
/**
 * The IP address maximum length.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_IP_ADDR_MAX_LEN  TAF_DOIP_IPV6_ADDR_MAX_LEN

//-------------------------------------------------------------------------------------------------
/**
 * Vehicle identification number size.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_VIN_SIZE  17

//-------------------------------------------------------------------------------------------------
/**
 * Entity iedntification size.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_EID_SIZE  6

//-------------------------------------------------------------------------------------------------
/**
 * Group identification size.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_GID_SIZE  6

//-------------------------------------------------------------------------------------------------
/**
 * maximum number of interfaces.
 */
//-------------------------------------------------------------------------------------------------
#define MAX_INF_NUM     8

//-------------------------------------------------------------------------------------------------
/**
 * DoIP Payload types.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_PAYLOAD_TYPE_HEADER_NEGATIVE_ACK           0x0000
#define TAF_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST      0x0001
#define TAF_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST_EID  0x0002
#define TAF_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST_VIN  0x0003
#define TAF_DOIP_PAYLOAD_TYPE_VEHICLE_ANNOUNCEMENT          0x0004
#define TAF_DOIP_PAYLOAD_TYPE_VEHICLE_ID_RESPONSE           0x0004
#define TAF_DOIP_PAYLOAD_TYPE_ROUTING_ACTIVE_REQUEST        0x0005
#define TAF_DOIP_PAYLOAD_TYPE_ROUTING_ACTIVE_RESPONSE       0x0006
#define TAF_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_REQUEST           0x0007
#define TAF_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_RESPONSE          0x0008
#define TAF_DOIP_PAYLOAD_TYPE_ENTITY_STATUS_REQUEST         0x4001
#define TAF_DOIP_PAYLOAD_TYPE_ENTITY_STATUS_RESPONSE        0x4002
#define TAF_DOIP_PAYLOAD_TYPE_POWER_MODE_INFO_REQUEST       0x4003
#define TAF_DOIP_PAYLOAD_TYPE_POWER_MODE_INFO_RESPONSE      0x4004
#define TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_MESSAGE            0x8001
#define TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_POSITIVE_ACK       0x8002
#define TAF_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_NEGATIVE_ACK       0x8003

// Generic DoIP Header structure length
#define TAF_DOIP_PROTOCOL_VERSION_LENGTH            1
#define TAF_DOIP_INVERSE_PROTOCOL_VERSION_LENGTH    1
#define TAF_DOIP_PAYLOAD_TYPE_LENGTH                2
#define TAF_DOIP_PAYLOAD_LENGTH                     4
#define TAF_DOIP_HEADER_GENERIC_LENGTH  (TAF_DOIP_PROTOCOL_VERSION_LENGTH + \
                                        TAF_DOIP_INVERSE_PROTOCOL_VERSION_LENGTH + \
                                        TAF_DOIP_PAYLOAD_TYPE_LENGTH + \
                                        TAF_DOIP_PAYLOAD_LENGTH)

#define TAF_DOIP_LOGICAL_ADDRESS_LENGTH     2
#define TAF_DOIP_FURTHER_ACTION_LENGTH      1
#define TAF_DOIP_RA_RES_CODE_LENGTH         1
#define TAF_DOIP_RA_ACTIVATION_TYPE_LENGTH  1
#define TAF_DOIP_RESERVED_LENGTH            4
#define TAF_DOIP_NACK_CODE_LENGTH           1
#define TAF_DOIP_ACK_CODE_LENGTH            1
#define TAF_DOIP_VIN_GID_SYNC_LENGTH        1
#define TAF_DOIP_NODE_TYPE_LENGTH           1
#define TAF_DOIP_MCTS_LENGTH                1
#define TAF_DOIP_NCTS_LENGTH                1
#define TAF_DOIP_MDS_LENGTH                 4

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic power mode status payload length.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_POWERMODE_LENGTH           1

//-------------------------------------------------------------------------------------------------
/**
 * DoIP Entity status response payload length.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_ENTITY_STATUS_RES_LENGTH   (TAF_DOIP_NODE_TYPE_LENGTH + \
                                            TAF_DOIP_MCTS_LENGTH + \
                                            TAF_DOIP_NCTS_LENGTH + \
                                            TAF_DOIP_MDS_LENGTH)

//-------------------------------------------------------------------------------------------------
/**
 * Vehicle information mandatory length.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_VEHICLE_INFO_MAND_LENGTH   (TAF_DOIP_EID_SIZE + \
                                            TAF_DOIP_GID_SIZE + \
                                            TAF_DOIP_VIN_SIZE + \
                                            TAF_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                            TAF_DOIP_FURTHER_ACTION_LENGTH)

//-------------------------------------------------------------------------------------------------
/**
 * Vehicle information all length.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_VEHICLE_INFO_ALL_LENGTH    (TAF_DOIP_EID_SIZE + \
                                            TAF_DOIP_GID_SIZE + \
                                            TAF_DOIP_VIN_SIZE + \
                                            TAF_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                            TAF_DOIP_FURTHER_ACTION_LENGTH + \
                                            TAF_DOIP_VIN_GID_SYNC_LENGTH)

//-------------------------------------------------------------------------------------------------
/**
 * Routing activation response payload length.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_PAYLOAD_RA_RES_LENGTH      (TAF_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                            TAF_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                            TAF_DOIP_RA_RES_CODE_LENGTH + \
                                            TAF_DOIP_RESERVED_LENGTH)

//-------------------------------------------------------------------------------------------------
/**
 * Routing activation request mandatory payload length.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_PAYLOAD_RA_REQ_MAND_LEN    (TAF_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                            TAF_DOIP_RA_ACTIVATION_TYPE_LENGTH + \
                                            TAF_DOIP_RESERVED_LENGTH)

//-------------------------------------------------------------------------------------------------
/**
 * Routing activation request all payload length.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_PAYLOAD_RA_REQ_ALL_LEN     (TAF_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                            TAF_DOIP_RA_ACTIVATION_TYPE_LENGTH +\
                                            TAF_DOIP_RESERVED_LENGTH +\
                                            TAF_DOIP_RESERVED_LENGTH)

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic negative acknowledgement length.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_DIAG_NEGATIVE_ACK_LENGTH   (TAF_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                            TAF_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                            TAF_DOIP_NACK_CODE_LENGTH)

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic positive acknowledgement length.
 */
//-------------------------------------------------------------------------------------------------
#define TAF_DOIP_DIAG_POSITIVE_ACK_LENGTH  (TAF_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                              TAF_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                              TAF_DOIP_ACK_CODE_LENGTH)

#define TAF_DOIP_MAX_BUFFER_SIZE            (1500)
#define TAF_DOIP_MAX_MSG_LEN_EXCEPT_UDS     41
#define SOCKET_TIMEOUT_MS                   1000


#define TAF_DOIP_THREAD_STACK_SIZE          0x20000

#define TAF_DOIP_IPv4_DEFAULT               "127.0.0.1"
#define TAF_DOIP_IPv6_DEFAULT               "::1"

#define TAF_DOIP_IPTYPE_DEFAULT             "IPv4"
#define TAF_DOIP_IPTYPE_MAX_LEN             8

#define TAF_DOIP_INTERFACE_DEFAULT          "bridge0"

#define TAF_DOIP_UDP_BROADCAST_IP           "255.255.255.255"
#define TAF_DOIP_UDP6_BROADCAST_IP          "FF02::1"

#define TAF_DOIP_IPv4_ADDRESS_ANY           "0.0.0.0"

#define TAF_DOIP_UDP_DISCOVERY_DEFAULT      13400
#define TAF_DOIP_TCP_DATA_DEFAULT           13400

#define TAF_DOIP_VIN_REQ_WITH_EID_OPTION    0
#define TAF_DOIP_ENTITY_STATUS_REQ_OPTION   0

typedef enum
{
    TAF_DOIP_IP_NOT_CONFIGURED_NOT_ANNOUNCED    = 0x00,
    TAF_DOIP_IP_CONFIGURED_NOT_ANNOUNCED        = 0x01,
    TAF_DOIP_IP_CONFIGURED_ANNOUNCED            = 0x02
}taf_doip_AnnounceState_t;

typedef enum
{
    TAF_DOIP_NT_GATEWAY = 0x00,     ///< Node type is gateway
    TAF_DOIP_NT_NODE    = 0x01,     ///< Node type is node
    TAF_DOIP_NT_UNKOWN  = 0xFF      ///< Node type is unkown
}taf_doip_NodeType_t;

// DoIP header structure
typedef struct
{
    uint8_t protocolVer;
    uint8_t invProtocolVer;
    uint16_t payloadType;
    uint32_t payloadLen;
}taf_doipHeader_t;

#endif  //TAFDOIP_COMMON_HPP
