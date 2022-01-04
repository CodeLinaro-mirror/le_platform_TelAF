/*
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef _TAF_SMS_PDU_H_
#define _TAF_SMS_PDU_H_

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "taf_pa_sms.hpp"

#define BITMASK_7BITS                       0x7F
#define BITMASK_8BITS                       0xFF
#define BITMASK_HIGH_4BITS                  0xF0
#define BITMASK_LOW_4BITS                   0x0F


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
    PDU_ENCODING_UCS2_16_BITS     = 0x2,
    PDU_ENCODING_UNKNOWN          = 0x3
}
pdu_Encoding_t;

typedef struct {
    char            addr[TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES];
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
    smsPdu_EncodeMsg_t*   data,
    taf_pa_sms_Pdu_t*     smsPdu
);

#endif   //_TAF_SMS_PDU_H_