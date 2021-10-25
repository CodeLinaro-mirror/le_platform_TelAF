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

#include "tafSmsPdu.hpp"
#include <string.h>

#define TP_TYPE_MASK                 0x03
#define TP_TYPE_SMS_DELIVER          0x00
#define TP_TYPE_SMS_SUBMIT           0x01
#define TP_TYPE_SMS_STATUS_REPORT    0x02
#define TP_TYPE_RESERVED             0x03

uint8_t ByteAtPos
(
    const uint8_t* bufPtr,
    uint8_t        pos
)
{
    return bufPtr[pos];
}

static uint8_t pduDecodeAddr(const unsigned char* buffer, uint8_t addrLen, char* outputAddr)
{
    LE_DEBUG("pduDecodeAddr");

    for (uint8_t i = 0; i < addrLen; ++i)
    {
        if (i % 2 == 1)
        {
            outputAddr[i] = ((buffer[i / 2] & BITMASK_HIGH_4BITS) >> 4) + '0';
        }
        else
        {
            outputAddr[i] = (buffer[i / 2] & BITMASK_LOW_4BITS) + '0';
        }
    }
    outputAddr[addrLen] = '\0';

    LE_DEBUG("outputAddr: %s", outputAddr);

    return addrLen;
}

static int16_t pduDecodeUserData
(
    pdu_Encoding_t encoding,
    const unsigned char* buffer,
    char* userData,
    uint8_t userDataLen
)
{
    LE_DEBUG("pduDecodeUserData");

    uint8_t outputDataLen = 0;

    switch(encoding)
    {
        case PDU_ENCODING_7_BITS:
        {
            if(userDataLen == 0)
                break;

            userData[outputDataLen++] = buffer[0] & BITMASK_7BITS;

            uint8_t carryOnBits = 1;

            uint8_t i = 0;
            for (i = 1; i < userDataLen; ++i)
            {
                userData[outputDataLen++] = ((buffer[i - 1] >> (8 - carryOnBits) | (buffer[i] << carryOnBits))) & BITMASK_7BITS;

                if (outputDataLen == userDataLen)
                {
                    userData[outputDataLen + 1] = '\0';
                    break;
                }

                carryOnBits++;

                if (carryOnBits == 8)
                {
                    userData[outputDataLen++] = buffer[i] & BITMASK_7BITS;
                    carryOnBits = 1;

                    if (outputDataLen == userDataLen)
                    {
                        userData[outputDataLen + 1] = '\0';
                        break;
                    }
                }

            }
            if (outputDataLen < userDataLen)
            {
                userData[outputDataLen++] =	buffer[i - 1] >> (8 - carryOnBits);
            }
        }
        userData[outputDataLen + 1] = '\0';
        break;

        case PDU_ENCODING_8_BITS:
        {
            outputDataLen = userDataLen;
            memcpy(userData, buffer, userDataLen);

            break;
        }

        case PDU_ENCODING_UCS2_16_BITS:
        {
            outputDataLen = userDataLen;
            memcpy(userData, buffer, userDataLen);

            break;
        }

        default:
            return 0;

    }

    return outputDataLen;
}

static pdu_Encoding_t getDataCodingScheme
(
    uint8_t smsDcs
)
{
    LE_DEBUG("getDataCodingScheme");

    pdu_Encoding_t encoding = PDU_ENCODING_UNKNOWN;

    if ((smsDcs >> 4) == 0xF)
    {
        encoding = (pdu_Encoding_t)((smsDcs >> 2) & 1);
    }
    else if ((smsDcs >> 6) == 0)
    {
        encoding = (pdu_Encoding_t)((smsDcs >> 2) & 0x3);
    }
    else
    {
        LE_DEBUG("encoding is not supported");
        return PDU_ENCODING_UNKNOWN;
    }

    return encoding;
}

le_result_t sms_DecodeDeliver
(
    const uint8_t*    dataPtr,
    sms_PduMsg_t*     smsPduPtr
)
{
    LE_DEBUG("sms_DecodeDeliver");

    const uint8_t pos_smsDeliver = 1 + ByteAtPos(dataPtr, 0);

    const uint8_t smsAddrLen =  ByteAtPos(dataPtr, pos_smsDeliver + 1);
    const uint8_t pos_smsAddr = pos_smsDeliver + 3;

    TAF_ERROR_IF_RET_VAL((uint8_t)(smsAddrLen + 1) > sizeof(smsPduPtr->addr), LE_FAULT, "addr size overflow");

    pduDecodeAddr(dataPtr + pos_smsAddr, smsAddrLen, smsPduPtr->addr);

    const uint8_t pos_smsPid = pos_smsDeliver + 3 + (dataPtr[pos_smsDeliver + 1] + 1) / 2;

    const uint8_t pos_smsDcs = pos_smsPid + 1;

    pdu_Encoding_t encoding = getDataCodingScheme((uint8_t)dataPtr[pos_smsDcs]);

    TAF_ERROR_IF_RET_VAL(encoding == PDU_ENCODING_UNKNOWN, LE_UNSUPPORTED, "unsupported encoding");

    smsPduPtr->encoding = encoding;

    const uint8_t pos_dataLen = pos_smsDcs + 7 + 1;

    const uint8_t smsUdl = ByteAtPos(dataPtr, pos_dataLen);

    LE_DEBUG("smsUdl: %d", smsUdl);

    TAF_ERROR_IF_RET_VAL(sizeof(smsPduPtr->data) < smsUdl, LE_FAULT, "data size is not enough for decoding");

    const uint8_t pos_Data = pos_dataLen + 1;

    const int16_t decodedContentSize = pduDecodeUserData(encoding,
                                                         dataPtr + pos_Data,
                                                         smsPduPtr->data,
                                                         smsUdl);

    LE_DEBUG("decodedContentSize: %d", decodedContentSize);
    LE_DEBUG("smsPduPtr->data: %s", smsPduPtr->data);

    TAF_ERROR_IF_RET_VAL(decodedContentSize != smsUdl, LE_FAULT, "decoded content length doesn't match smsUdl");

    smsPduPtr->dataLen = decodedContentSize;

    return LE_OK;
}

le_result_t sms_DecodeGsmMsg
(
    const uint8_t*    dataPtr,
    sms_PduMsg_t*     smsPduPtr
)
{
    le_result_t result;

    memset(smsPduPtr, 0, sizeof(sms_PduMsg_t));

    const uint8_t pos_smsDeliver = 1 + ByteAtPos(dataPtr, 0);
    const uint8_t smsType = ByteAtPos(dataPtr, pos_smsDeliver);

    LE_DEBUG("smsType: 0x%.2x", smsType);

    if((smsType & TP_TYPE_MASK) == TP_TYPE_SMS_DELIVER)
    {
        LE_DEBUG("SMS_TYPE_DELIVER");
        smsPduPtr->type = SMS_TYPE_DELIVER;
        result = sms_DecodeDeliver(dataPtr, smsPduPtr);
    }
    else
    {
        result = LE_UNSUPPORTED;
    }

    return result;
}

le_result_t smsPdu_Decode
(
    sms_Protocol_t    protocol,
    const uint8_t*    dataPtr,
    sms_PduMsg_t*     smsPduPtr
)
{
    le_result_t result = LE_OK;

    LE_DEBUG("protocol: %d", protocol);

    switch(protocol)
    {
        case SMS_PROTOCOL_GSM:
            LE_INFO("SMS_PROTOCOL_GSM");
            result = sms_DecodeGsmMsg(dataPtr, smsPduPtr);
            break;

        case SMS_PROTOCOL_CDMA:
            LE_INFO("SMS_PROTOCOL_CDMA");
            result = LE_UNSUPPORTED;
            break;

        case SMS_PROTOCOL_GW_CB:
            LE_INFO("SMS_PROTOCOL_GW_CB");
            result = LE_UNSUPPORTED;
            break;

        default:
            result = LE_UNSUPPORTED;
            break;
    }

    return result;
}

