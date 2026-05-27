/*
 *  Copyright (c) 2021-2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef _TAF_SMS_PDU_H_
#define _TAF_SMS_PDU_H_

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafSmsHlos.hpp"

#define BITMASK_7BITS                       0x7F
#define BITMASK_8BITS                       0xFF
#define BITMASK_HIGH_4BITS                  0xF0
#define BITMASK_LOW_4BITS                   0x0F

#define TAF_SMS_ADDR_PLUS_CHARS             1   // Optional '+'
#define TAF_SMS_ADDR_NULL_CHARS             1   // '\0'
#define TAF_SMS_ADDR_PROTO_EXTRA_CHARS      2   // Extra bytes added to 3GPP TS 23.040 max SMS address length (18 -> 20)

#define TAF_SMS_ADDR_EXTRA_CHARS \
    (TAF_SMS_ADDR_PLUS_CHARS + \
     TAF_SMS_ADDR_NULL_CHARS + \
     TAF_SMS_ADDR_PROTO_EXTRA_CHARS)

typedef enum
{
    SMS_TYPE_DELIVER        = 0,
    SMS_TYPE_SUBMIT         = 1,
    SMS_TYPE_STATUS_REPORT  = 2,
    SMS_TYPE_PDU            = 3,
    SMS_TYPE_CELL_BROADCAST = 4,
    SMS_TYPE_UNSUPPORTED    = 5
}
sms_Type_t;

typedef enum
{
    SMS_PROTOCOL_UNKNOWN = 0,
    SMS_PROTOCOL_GSM     = 1,
    SMS_PROTOCOL_CDMA    = 2,
    SMS_PROTOCOL_GW_CB   = 3
}
sms_Protocol_t;

typedef enum
{
    PDU_ENCODING_7_BITS           = 0x0,
    PDU_ENCODING_8_BITS           = 0x1,
    PDU_ENCODING_16_BITS          = 0x2,
    PDU_ENCODING_UNKNOWN          = 0x3
}
pdu_Encoding_t;

typedef struct {
    char            addr[TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES + TAF_SMS_ADDR_EXTRA_CHARS];
    char            data[TAF_SMS_TEXT_BYTES];
    uint32_t        dataLen;
    sms_Type_t      type;
    pdu_Encoding_t  encoding;
}
sms_PduMsg_t;

typedef struct
{
    sms_Protocol_t      protocol;
    const uint8_t*      msgData;
    size_t              msgDataLen;
    const char*         addrData;
    pdu_Encoding_t      encoding;
    sms_Type_t          type;
    bool                statusReport;
}
smsPdu_EncodeMsg_t;

le_result_t smsPdu_Decode
(
    sms_Protocol_t    protocol,
    const uint8_t*    dataPtr,
    sms_PduMsg_t*     smsPduPtr
);

le_result_t smsPdu_Encode
(
    smsPdu_EncodeMsg_t*    data,
    taf_sms_Pdu_t*         smsPdu
);

#endif   //_TAF_SMS_PDU_H_
