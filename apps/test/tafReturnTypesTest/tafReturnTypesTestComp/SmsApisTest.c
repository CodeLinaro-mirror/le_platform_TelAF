/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate SMS Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void smsRetTest_RunApis
(
    void
)
{
    le_result_t result;
    taf_sms_MsgRef_t tmpMsg_binary;
    #define BINARY_PATTERN {0, 255}
    #define UCS2_PATTERN {0x2C6E, 0x668A}
    static uint8_t binary_pattern[2] = BINARY_PATTERN;
    static uint16_t ucs2_pattern[2]  = UCS2_PATTERN;
    static uint8_t PDU_TEST_PATTERN_7BITS[]=
    {
    0x00,0x01,0x00,0x0C,0x91,0x88,0x96,0x87,0x71,0x92,0x20,0x00,0x11,0x08,0xD4,0xA0,0x11,0x44,0x2F,0xCF,0xE9
    };
    LE_TEST_INFO("smsRetTest_RunApis");

    //1.taf_sms_GetSendStatus -TAF_SMS_TXSTS_UNKNOWN scenario
    taf_sms_SendStatus_t ref = taf_sms_GetSendStatus(NULL);
    LE_TEST_OK(ref==TAF_SMS_TXSTS_UNKNOWN,"taf_sms_GetSendStatus-TAF_SMS_TXSTS_UNKNOWN");

    //2.taf_sms_GetReadStatus -TAF_SMS_RXSTS_UNKNOWN scenario
    taf_sms_ReadStatus_t status = taf_sms_GetReadStatus(NULL);
    LE_TEST_OK(status==TAF_SMS_RXSTS_UNKNOWN,"taf_sms_GetReadStatus-TAF_SMS_RXSTS_UNKNOWN");

    //3.taf_sms_GetLockStatus -TAF_SMS_LKSTS_UNKNOWN scenario
    taf_sms_LockStatus_t lock = taf_sms_GetLockStatus(NULL);
    LE_TEST_OK(lock==TAF_SMS_LKSTS_UNKNOWN,"taf_sms_GetLockStatus-TAF_SMS_LKSTS_UNKNOWN");

    //4.taf_sms_SetBinary -LE_BAD_PARAMETER scenario
    tmpMsg_binary = taf_sms_Create();
    result = taf_sms_SetBinary(tmpMsg_binary,binary_pattern,0);
    LE_TEST_OK(result==LE_BAD_PARAMETER,"taf_sms_SetBinary-LE_BAD_PARAMETER");

    //5.taf_sms_SetUCS2 -LE_BAD_PARAMETER scenario
    result = taf_sms_SetUCS2(tmpMsg_binary,ucs2_pattern,0);
    LE_TEST_OK(result==LE_BAD_PARAMETER,"taf_sms_SetUCS2-LE_BAD_PARAMETER");

    //6.taf_sms_SetPDU -LE_BAD_PARAMETER scenario
    result = taf_sms_SetPDU(tmpMsg_binary,PDU_TEST_PATTERN_7BITS,0);
    LE_TEST_OK(result==LE_BAD_PARAMETER,"taf_sms_SetPDU-LE_BAD_PARAMETER");

    //7.taf_sms_LockFromStorage -LE_NOT_FOUND scenario
    result = taf_sms_LockFromStorage(NULL);
    LE_TEST_OK(result==LE_NOT_FOUND,"taf_sms_LockFromStorage-LE_NOT_FOUND");

    //8.taf_sms_UnlockFromStorage -LE_NOT_FOUND scenario
    result = taf_sms_UnlockFromStorage(NULL);
    LE_TEST_OK(result==LE_NOT_FOUND,"taf_sms_UnlockFromStorage-LE_NOT_FOUND");

    //9.taf_sms_EncryptFromStorage -LE_NOT_FOUND scenario
    result = taf_sms_EncryptFromStorage(NULL);
    LE_TEST_OK(result==LE_NOT_FOUND,"taf_sms_EncryptFromStorage-LE_NOT_FOUND");

    //10.taf_sms_SendPduMsg -LE_BAD_PARAMETER scenario
    result = taf_sms_SendPduMsg(PDU_TEST_PATTERN_7BITS,0, 1000);
    LE_TEST_OK(result==LE_BAD_PARAMETER,"taf_sms_SendPduMsg-LE_BAD_PARAMETER");

    //11.taf_sms_SendPduMsgEx -LE_BAD_PARAMETER scenario
    result = taf_sms_SendPduMsgEx(1,PDU_TEST_PATTERN_7BITS,0, 1000);
    LE_TEST_OK(result==LE_BAD_PARAMETER,"taf_sms_SendPduMsgEx-LE_BAD_PARAMETER");

    //12.taf_sms_AddCellBroadcastIds -LE_BAD_PARAMETER scenario
    result = taf_sms_AddCellBroadcastIds(0,4352,4354);
    LE_TEST_OK(result==LE_BAD_PARAMETER,"taf_sms_AddCellBroadcastIds-LE_BAD_PARAMETER");

    //13.taf_sms_RemoveCellBroadcastIds -LE_BAD_PARAMETER scenario
    result = taf_sms_RemoveCellBroadcastIds(0,4352,4354);
    LE_TEST_OK(result==LE_BAD_PARAMETER,"taf_sms_RemoveCellBroadcastIds-LE_BAD_PARAMETER");
}
