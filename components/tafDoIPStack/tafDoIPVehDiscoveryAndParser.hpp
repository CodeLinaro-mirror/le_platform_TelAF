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

#ifndef TAFDOIPVEHDISCOVERY_HPP
#define TAFDOIPVEHDISCOVERY_HPP

#include <time.h>
#include <stdint.h>
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

#define TAF_DOIP_NO_FURTHER_ACTION_REQUIRED  0x00
#define TAF_DOIP_FURTHER_ACTION_REQUIRED     0x10

#define MIN_ANNOUNCE_WAIT_TIME 0
#define MAX_ANNOUNCE_WAIT_TIME 500

typedef enum
{
    TAF_DOIP_SOCKET_TYPE_UNKNOWN   = 0x00,
    TAF_DOIP_SOCKET_TYPE_TCP       = 0x01,
    TAF_DOIP_SOCKET_TYPE_UDP_UNI   = 0x02,
    TAF_DOIP_SOCKET_TYPE_UDP_MULTI = 0x03
}taf_doipSockType_t;

typedef enum DOIP_ROUTING_ACTIVE_RES_CODE
{
    TAF_DOIP_RA_RES_UNKNOWN_SOURCE_ADDRESS              = 0x00,
    TAF_DOIP_RA_RES_ALL_SOCKET_REGISTED_AND_ACTIVE      = 0x01,
    TAF_DOIP_RA_RES_SA_DIFFERENT_ACTIVATED_SOCKET       = 0x02,
    TAF_DOIP_RA_RES_SA_REGISTED_DIFFERENT_SOCKET        = 0x03,
    TAF_DOIP_RA_RES_MISSING_AUTHENTICATION              = 0x04,
    TAF_DOIP_RA_RES_REJECT_CONFIRMATION                 = 0x05,
    TAF_DOIP_RA_RES_UNSUPPORTED_RA_TYPE                 = 0x06,
    TAF_DOIP_RA_RES_ROUTING_SUCCESSFULLY_ACTIVATED      = 0x10,
    TAF_DOIP_RA_RES_CONFIRMATION_REQUIRED               = 0x11
}taf_doipRaResCode_t;

typedef enum DOIP_HEADER_NACK_CODE
{
    TAF_DOIP_HEADER_NACK_INCORRECT_PATTERN_FORMAT = 0x00,
    TAF_DOIP_HEADER_NACK_UNKNOWN_PAYLOAD_TYPE     = 0x01,
    TAF_DOIP_HEADER_NACK_MESSAGE_TOO_LARGE        = 0x02,
    TAF_DOIP_HEADER_NACK_OUT_OF_MEMORY            = 0x03,
    TAF_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH   = 0x04
}taf_doipHeaderNACKCode_t;

typedef enum DOIP_PAYLOAD_TYPE
{
    TAF_DOIP_HEADER_NEGATIVE_ACK          = 0x00,
    TAF_DOIP_VEHICLE_ANNOUNCEMENT         = 0x01,
    TAF_DOIP_VEHICLE_IDENTIFY_RESPONSE    = 0x02,
    TAF_DOIP_ROUTING_ACTIVE_RESPONSE      = 0x03,
    TAF_DOIP_ALIVE_CHECK_REQUEST          = 0x04,
    TAF_DOIP_ENTITY_STATUS_RESPONSE       = 0x05,
    TAF_DOIP_POWER_MODE_INFO_RESPONSE     = 0x06,
    TAF_DOIP_DIAGNOSTIC_POSITIVE_ACK      = 0x07,
    TAF_DOIP_DIAGNOSTIC_NEGATIVE_ACK      = 0x08,
    TAF_DOIP_DIAGNOSTIC_FROM_ENTITY       = 0x09,
    TAF_DOIP_VEHICLE_IDENTIFY_REQUEST     = 0x10,
    TAF_DOIP_VEHICLE_IDENTIFY_REQUEST_EID = 0x11,
    TAF_DOIP_VEHICLE_IDENTIFY_REQUEST_VIN = 0x12,
    TAF_DOIP_ROUTING_ACTIVE_REQUEST       = 0x13,
    TAF_DOIP_ALIVE_CHECK_RESPONSE         = 0x14,
    TAF_DOIP_ENTITY_STATUS_REQUEST        = 0x15,
    TAF_DOIP_POWER_MODE_INFO_REQUEST      = 0x16,
    TAF_DOIP_DIAGNOSTIC_FROM_EQUIP        = 0x17
}taf_doipPayloadType_t;

typedef enum DOIP_DIAGNOSTIC_NACK
{
    TAF_DOIP_DIAGNOSTIC_NACK_INVALID_SA         = 0x02,
    TAF_DOIP_DIAGNOSTIC_NACK_UNKNOWN_TA         = 0x03,
    TAF_DOIP_DIAGNOSTIC_NACK_DIAG_MSG_TOO_LARGE = 0x04,
    TAF_DOIP_DIAGNOSTIC_NACK_OUT_OF_MEMORY      = 0x05,
    TAF_DOIP_DIAGNOSTIC_NACK_TARGET_UNREACHABLE = 0x06,
    TAF_DOIP_DIAGNOSTIC_NACK_UNKNOWN_NETWORK    = 0x07,
    TAF_DOIP_DIAGNOSTIC_NACK_TRANS_PROTO_ERROR  = 0x08
}taf_doipDiagNACKCode_t;

typedef enum DOIP_DIAGNOSTIC_ACK
{
    TAF_DOIP_DIAGNOSTIC_ACK  = 0x00
}taf_doipDiagACKCode_t;

typedef struct
{
    le_socket_Ref_t sockRef;
    char ip[TAF_DOIP_IP_ADDR_MAX_LEN];
    uint16_t port;
    taf_doipSockType_t commType;
}taf_doipLink_t;

namespace taf{
namespace doip{
    class ProtocolParser{
        public:
            ProtocolParser() {};
            ~ProtocolParser() {};

            static ProtocolParser &GetInstance();

            // pack DOIP generic header
            void PackGenericHeader(char* headerPtr, uint16_t payloadType, uint32_t payloadLen);

            // Pack and announce vehcile discovery msg or response vehicle identification
            void VehicleAnnounceOrIdRes(taf_doipLink_t *linkPtr, uint16_t entityLA,
                    uint8_t furtherAction);

            // Routing activation response
            void RoutingActivationRes(taf_doipLink_t *linkPtr, uint16_t equipLA,
                    uint16_t entityLA, taf_doipRaResCode_t raResCode, uint32_t reserved);

            // Alive check request msg
            void AliveCheckReq(taf_doipLink_t *linkPtr);

            // Alive check response
            void AliveCheckRes(taf_doipLink_t *linkPtr, uint16_t equipLA);

            // Diagnostic Power mode response
            void DiagPowerModeRes(taf_doipLink_t *linkPtr, uint8_t powerMode);

            // DoIP Entity Status response
            void DoIPEntityStatusRes(taf_doipLink_t *linkPtr, taf_doip_NodeType_t nodeType,
                    uint8_t maxCTS, uint8_t numCTS, uint32_t maxDataSize);

            // Generic DoIP header negative acknowledgement
            void HeaderNegativeACK(taf_doipLink_t *linkPtr,
                    taf_doipHeaderNACKCode_t headerNackCode);

            // Diagnostic negative acknowledgement
            void DiagNegativeACK(taf_doipLink_t *linkPtr, uint16_t entityLA,
                    uint16_t equipLA, taf_doipDiagNACKCode_t diagNackCode);

            // Diagnostic positive acknowledgement
            void DiagPositiveACK(taf_doipLink_t *linkPtr, uint16_t entityLA,
                    uint16_t equipLA);

            //Send DoIP message
            taf_doip_Result_t SendDoIPMsg(taf_doipLink_t *linkPtr, uint8_t payloadType,
                    char* dataPtr, size_t dataLen);

            //unpack doip received header structure
            void UnpackHeaderStruct(char *dataPtr, uint32_t dataLen,
                    taf_doipHeader_t *headerPtr);

        private:
    };

    class VehicleDiscovery{
        public:

            static VehicleDiscovery &GetInstance();

            void Init();

            // Vehicle discovery and announcement function
            taf_doip_Result_t VehicleDiscoveryAnnounce();

            // Vehicle announcement wait timer handler function
            static void VehicleAnnounceTimerHandler(le_timer_Ref_t timerRef);

            // Vehicle identity response function
            taf_doip_Result_t VehicleIdentityRes(taf_doipLink_t *linkPtr);

            // Vehicle identity response handler function
            static void VehicleIdentityTimerHandler(le_timer_Ref_t resTimerRef);

            // Timer reference
            le_timer_Ref_t waitTimerRef = NULL;
            le_timer_Ref_t resTimerRef = NULL;

        private:
            uint32_t announceCount = 0;
            uint32_t maxAnnounceCount = 0;
    };
}
}
#endif // TAFDOIPVEHDISCOVERY_HPP
