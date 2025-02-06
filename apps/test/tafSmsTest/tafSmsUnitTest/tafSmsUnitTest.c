
/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ​​​​​Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

/*
 * @file       tafSmsUnitTest.cpp
 * @brief      This file includes unit test functions of the SMS Service.
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Define test pattern
 */
//--------------------------------------------------------------------------------------------------
#define TEXT_PATTERN_EMPTY  ""
#define TEXT_PATTERN_TEST   "Unit test from telaf-1. Unit test from telaf-2. Unit test from telaf-3."
#define TEXT_PATTERN_NUM    "0123456789 0123456789 0123456789 0123456789 0123456789"
#define TEXT_PATTERN_SYMBOL "~!@#$^&*()_+{}:<>?"
#define BINARY_PATTERN      {0, 255}
#define UCS2_PATTERN        {0x2C6E, 0x668A}

#define DEST_PATTERN_EMPTY  ""
#define DEST_PATTERN_VALID  "0979334397"    // Use the same sim and device to send/receive message

#define SMSC_ADDR_PATTERN_VALID "\"+886935874443\"" //SMS center address for TWN Mobile

#define PHONE_ID_PATTERN_1  1               // Phone ID to test

#define TIMEOUT_TX_TEST     3               // Time interval between sending message
#define TIMEOUT_RX_TEST     45              // Wait for receicing message sent from this test app

#define TIME_SET_SMSC       5               // Wait for settingi sms center take effect

#define AMOUNT_MSG_TX       5               // Total message amount to send from this test app

//--------------------------------------------------------------------------------------------------
/**
 * Message in PDU format, generated from PDU converter
 * Message text: TAF test
 */
//--------------------------------------------------------------------------------------------------
static uint8_t PDU_TEST_PATTERN_7BITS[]=
{
0x00,0x01,0x00,0x0C,0x91,0x88,0x96,0x87,0x71,0x92,0x20,0x00,0x11,0x08,0xD4,0xA0,0x11,0x44,0x2F,0xCF,0xE9
};

static uint8_t binary_pattern[2] = BINARY_PATTERN;

static uint16_t ucs2_pattern[2]  = UCS2_PATTERN;

typedef union {
    char     text[TAF_SMS_TEXT_BYTES];
    uint8_t  binary[TAF_SMS_BINARY_BYTES];
    uint8_t  pdu[TAF_SMS_PDU_BYTES];
    uint16_t ucs2[TAF_SMS_UCS2_CHARS];
}
RxSmsContent_t;

//--------------------------------------------------------------------------------------------------
/**
 * Resources used for test
 */
//--------------------------------------------------------------------------------------------------

static char DEST_TEST[TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES];

static le_sem_Ref_t sem_RxTest;

static taf_sms_RxMsgHandlerRef_t RxHandlerRef;
static taf_sms_MsgListRef_t RxMsgListRef;

static le_thread_Ref_t RxThreadRef;
static le_thread_Ref_t TxThreadRef;

static uint8_t RxCount = 0;
static uint8_t TxCount = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Set semaphore timeout
 */
//--------------------------------------------------------------------------------------------------

__attribute__((unused)) le_result_t WaitForSem_Timeout
(
    le_sem_Ref_t semRef,
    uint32_t seconds
)
{
    le_clk_Time_t timeToWait = {seconds, 0};

    return le_sem_WaitWithTimeOut(semRef, timeToWait);
}

/*======================================================================

 FUNCTION        Test_taf_sms_CreateDeleteRxMsgList

 DESCRIPTION     Test adding/removing RX handler API of SMS service.

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_sms_CreateDeleteRxMsgList
(
    void
)
{
    RxMsgListRef = taf_sms_CreateRxMsgList();

    LE_TEST_ASSERT(RxMsgListRef != NULL, "Test taf_sms_CreateRxMsgList");

    taf_sms_DeleteList(RxMsgListRef);
}

/*======================================================================

 FUNCTION        Test_taf_sms_SetGetReadStatus

 DESCRIPTION     Test setting/getting read status of msg

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_sms_SetGetReadStatus
(
    void
)
{
    RxMsgListRef = taf_sms_CreateRxMsgList();

    taf_sms_MsgRef_t msgRef = NULL;
    taf_sms_MsgRef_t lastMsgRef = NULL;

    msgRef = taf_sms_GetFirst(RxMsgListRef);

    do
    {
        if (msgRef == NULL)
        {
            break;
        }
        lastMsgRef = msgRef;
        msgRef = taf_sms_GetNext(RxMsgListRef);
    }

    while ( msgRef != NULL);

    LE_TEST_ASSERT(taf_sms_GetReadStatus(lastMsgRef) == TAF_SMS_RXSTS_UNREAD, "Test taf_sms_GetReadStatus");

    taf_sms_MarkRead(lastMsgRef);

    LE_TEST_ASSERT(taf_sms_GetReadStatus(lastMsgRef) == TAF_SMS_RXSTS_READ, "Test taf_sms_GetReadStatus");

    taf_sms_MarkUnread(lastMsgRef);

    LE_TEST_ASSERT(taf_sms_GetReadStatus(lastMsgRef) == TAF_SMS_RXSTS_UNREAD, "Test taf_sms_GetReadStatus");

    taf_sms_DeleteList(RxMsgListRef);
}

/*======================================================================

 FUNCTION        Test_taf_sms_SetGetLockStatus

 DESCRIPTION     Test setting/getting lock status of msg

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_sms_SetGetLockStatus
(
    void
)
{
    RxMsgListRef = taf_sms_CreateRxMsgList();

    taf_sms_MsgRef_t msgRef = NULL;
    taf_sms_MsgRef_t lastMsgRef = NULL;

    msgRef = taf_sms_GetFirst(RxMsgListRef);

    do {
        if (msgRef == NULL)
        {
            break;
        }
        lastMsgRef = msgRef;
        msgRef = taf_sms_GetNext(RxMsgListRef);
    }
    while ( msgRef != NULL);

    LE_TEST_ASSERT(taf_sms_GetLockStatus(lastMsgRef) == TAF_SMS_LKSTS_UNLOCKED, "Test taf_sms_GetLockStatus");

    LE_TEST_ASSERT(taf_sms_LockFromStorage(lastMsgRef) == LE_OK, "Test taf_sms_LockFromStorage");

    LE_TEST_ASSERT(taf_sms_GetLockStatus(lastMsgRef) == TAF_SMS_LKSTS_LOCKED, "Test taf_sms_GetLockStatus");

    LE_TEST_ASSERT(taf_sms_DeleteFromStorage(lastMsgRef) == LE_BUSY, "Test taf_sms_DeleteFromStorage");

    LE_TEST_ASSERT(taf_sms_UnlockFromStorage(lastMsgRef) == LE_OK, "Test taf_sms_UnlockFromStorage");

    LE_TEST_ASSERT(taf_sms_GetLockStatus(lastMsgRef) == TAF_SMS_LKSTS_UNLOCKED, "Test taf_sms_GetLockStatus");

    LE_TEST_ASSERT(taf_sms_DeleteFromStorage(lastMsgRef) == LE_OK, "Test taf_sms_DeleteFromStorage");

    taf_sms_DeleteList(RxMsgListRef);
}

/*======================================================================

 FUNCTION        Test_taf_sms_EncryptFromStorage

 DESCRIPTION     Test encrypting msg from HLOS

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_sms_EncryptFromStorage
(
    void
)
{
    RxMsgListRef = taf_sms_CreateRxMsgList();

    taf_sms_MsgRef_t msgRef = NULL;
    taf_sms_MsgRef_t lastMsgRef = NULL;

    msgRef = taf_sms_GetFirst(RxMsgListRef);

    do {
        if (msgRef == NULL)
        {
            break;
        }
        lastMsgRef = msgRef;
        msgRef = taf_sms_GetNext(RxMsgListRef);
    }
    while ( msgRef != NULL);

    LE_TEST_ASSERT(taf_sms_EncryptFromStorage(lastMsgRef) == LE_OK, "Test EncryptFromStorage");

    taf_sms_DeleteList(RxMsgListRef);
}

/*======================================================================

 FUNCTION        Test_taf_sms_DeleteMsgFromStorage

 DESCRIPTION     Test deleting the last msg of list from storage

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_sms_DeleteMsgFromStorage
(
    void
)
{
    RxMsgListRef = taf_sms_CreateRxMsgList();

    taf_sms_MsgRef_t msgRef = NULL;
    taf_sms_MsgRef_t lastMsgRef = NULL;

    msgRef = taf_sms_GetFirst(RxMsgListRef);

    do {

        if (msgRef == NULL)
        {
            break;
        }
        lastMsgRef = msgRef;
        msgRef = taf_sms_GetNext(RxMsgListRef);
    }
    while ( msgRef != NULL);

    LE_TEST_ASSERT(taf_sms_DeleteFromStorage(lastMsgRef) == LE_OK, "Test taf_sms_DeleteFromStorage");

    taf_sms_DeleteList(RxMsgListRef);
}

/*======================================================================

 FUNCTION        Test_taf_sms_DeleteAllMsgFromStorage

 DESCRIPTION     Test deleting all the msgs from preferred storage

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_sms_DeleteAllMsgFromStorage
(
    void
)
{
    taf_sms_Storage_t prefStorage = TAF_SMS_STORAGE_UNKNOWN;

    LE_TEST_ASSERT(taf_sms_GetPreferredStorage(&prefStorage) == LE_OK, "Test taf_sms_GetPreferredStorage");

    LE_TEST_ASSERT(taf_sms_DeleteAllFromStorage(prefStorage) == LE_OK, "Test taf_sms_DeleteAllFromStorage");
}

/*======================================================================

 FUNCTION        Test_taf_sms_SetGetPreferredStorage

 DESCRIPTION     Test setting/getting preferred storage of SMS service.

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_sms_SetGetPreferredStorage
(
    void
)
{
    taf_sms_Storage_t prefStorage = TAF_SMS_STORAGE_UNKNOWN;

    LE_TEST_ASSERT(taf_sms_SetPreferredStorage(TAF_SMS_STORAGE_NONE) == LE_OK, "Test taf_sms_SetPreferredStorage");

    LE_TEST_ASSERT(taf_sms_GetPreferredStorage(&prefStorage) == LE_OK, "Test taf_sms_GetPreferredStorage");

    LE_TEST_ASSERT(prefStorage == TAF_SMS_STORAGE_NONE, "Test taf_sms_GetPreferredStorage");

    LE_TEST_ASSERT(taf_sms_SetPreferredStorage(TAF_SMS_STORAGE_HLOS) == LE_OK, "Test taf_sms_SetPreferredStorage");

    LE_TEST_ASSERT(taf_sms_GetPreferredStorage(&prefStorage) == LE_OK, "Test taf_sms_GetPreferredStorage");

    LE_TEST_ASSERT(prefStorage == TAF_SMS_STORAGE_HLOS, "Test taf_sms_GetPreferredStorage");
}

/*======================================================================

 FUNCTION        Test_taf_sms_AddRemoveFullStorageHandler

 DESCRIPTION     Test adding/removing full storage handler API of SMS service.

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void FullStorageHandler
(
    taf_sms_StorageFullType_t type,
    void* contextPtr
)
{
    LE_INFO("Storage is %d", type);
}

__attribute__((unused)) static void Test_taf_sms_AddRemoveFullStorageHandler
(
    void
)
{
    taf_sms_FullStorageEventHandlerRef_t handlerRef;

    handlerRef = taf_sms_AddFullStorageEventHandler(FullStorageHandler, NULL);

    LE_TEST_ASSERT(handlerRef != NULL, "Test taf_sms_AddFullStorageEventHandler");

    taf_sms_RemoveFullStorageEventHandler(handlerRef);
}

/*======================================================================

 FUNCTION        Test_taf_sms_SetGetParam

 DESCRIPTION     Test setting/getting parameter APIs of SMS service.

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_sms_SetGetParam
(
    void
)
{
    taf_sms_MsgRef_t    tmpMsg;
    char                text[TAF_SMS_TEXT_BYTES] = {0};
    uint8_t             phoneId;

    tmpMsg = taf_sms_Create();

    LE_TEST_ASSERT(tmpMsg, "Test taf_sms_Create");

    LE_TEST_ASSERT(taf_sms_SetText(tmpMsg, TEXT_PATTERN_EMPTY) == LE_BAD_PARAMETER, "Test taf_sms_SetText");

    LE_TEST_ASSERT(taf_sms_SetText(tmpMsg, TEXT_PATTERN_TEST) == LE_OK, "Test taf_sms_SetText");

    LE_TEST_ASSERT(taf_sms_GetText(tmpMsg, text, sizeof(text)) == LE_OK, "Test taf_sms_GetText");

    LE_TEST_ASSERT(taf_sms_GetUserdataLen(tmpMsg) == strlen(TEXT_PATTERN_TEST), "Test taf_sms_GetUserdataLen");

    LE_TEST_ASSERT(strncmp(text, TEXT_PATTERN_TEST, strlen(TEXT_PATTERN_TEST)) == 0, "Test taf_sms_GetText");

    LE_TEST_ASSERT(taf_sms_SetDestination(tmpMsg, DEST_TEST) == LE_OK, "Test taf_sms_SetDestination");

    LE_TEST_ASSERT(taf_sms_SetPhoneId(tmpMsg, PHONE_ID_PATTERN_1) == LE_OK, "Test taf_sms_SetPhoneId");

    LE_TEST_ASSERT(taf_sms_GetPhoneId(tmpMsg, &phoneId) == LE_OK, "Test taf_sms_GetPhoneId");

    LE_TEST_ASSERT(phoneId == PHONE_ID_PATTERN_1, "Test taf_sms_GetPhoneId");

    LE_TEST_ASSERT(taf_sms_GetType(tmpMsg) == TAF_SMS_TYPE_TX, "Test taf_sms_GetType");

    taf_sms_Delete(tmpMsg);

    tmpMsg = taf_sms_Create();

    size_t dataSize = sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]);

    LE_TEST_ASSERT(taf_sms_SetPDU(tmpMsg, NULL, 0) != LE_OK, "Test taf_sms_SetPDU");

    LE_TEST_ASSERT(taf_sms_SetPDU(tmpMsg, PDU_TEST_PATTERN_7BITS, dataSize) == LE_OK,
                    "Test taf_sms_SetPDU");
}

/*======================================================================

 FUNCTION        Callback_MsgSendStatus

 DESCRIPTION     handler to get status of sending message

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Callback_MsgSendStatus
(
    taf_sms_MsgRef_t msgRef,
    taf_sms_SendStatus_t status,
    void* contextPtr
)
{
    LE_INFO("msg: %p, Sendstatus: %d", msgRef, status);

    LE_TEST_ASSERT(status == TAF_SMS_TXSTS_SENT, "Test TAF_SMS_TXSTS");

    TxCount++;

    taf_sms_Delete(msgRef);
}

__attribute__((unused)) static void* SmsTxThread
(
    void* contextPtr
)
{
    taf_sms_ConnectService();

    taf_sms_MsgRef_t tmpMsg_alphabet;
    taf_sms_MsgRef_t tmpMsg_num;
    taf_sms_MsgRef_t tmpMsg_symbol;
    taf_sms_MsgRef_t tmpMsg_binary;
    taf_sms_MsgRef_t tmpMsg_ucs2;

    tmpMsg_alphabet = taf_sms_Create();

    LE_TEST_ASSERT(tmpMsg_alphabet, "Test taf_sms_Create");

    LE_TEST_ASSERT(taf_sms_SetDestination(tmpMsg_alphabet, DEST_TEST) == LE_OK, "Test taf_sms_SetDestination");

    LE_TEST_ASSERT(taf_sms_SetCallback(tmpMsg_alphabet, Callback_MsgSendStatus, NULL) == LE_OK, "Test taf_sms_SetCallback");

    LE_TEST_ASSERT(taf_sms_SetText(tmpMsg_alphabet, TEXT_PATTERN_TEST) == LE_OK, "Test taf_sms_SetText");

    LE_TEST_ASSERT(taf_sms_Send(tmpMsg_alphabet) == LE_OK, "Test taf_sms_Send %s", "#s# + send msg with type [alphabet]");

    le_thread_Sleep(TIMEOUT_TX_TEST);

    tmpMsg_num = taf_sms_Create();

    LE_TEST_ASSERT(tmpMsg_num, "Test taf_sms_Create");

    LE_TEST_ASSERT(taf_sms_SetDestination(tmpMsg_num, DEST_TEST) == LE_OK, "Test taf_sms_SetDestination");

    LE_TEST_ASSERT(taf_sms_SetCallback(tmpMsg_num, Callback_MsgSendStatus, NULL) == LE_OK, "Test taf_sms_SetCallback");

    LE_TEST_ASSERT(taf_sms_SetText(tmpMsg_num, TEXT_PATTERN_NUM) == LE_OK, "Test taf_sms_SetText");

    LE_TEST_ASSERT(taf_sms_Send(tmpMsg_num) == LE_OK, "Test taf_sms_Send %s", "#s# + send msg with type [number]");

    le_thread_Sleep(TIMEOUT_TX_TEST);

    tmpMsg_symbol = taf_sms_Create();

    LE_TEST_ASSERT(tmpMsg_symbol, "Test taf_sms_Create");

    LE_TEST_ASSERT(taf_sms_SetDestination(tmpMsg_symbol, DEST_TEST) == LE_OK, "Test taf_sms_SetDestination");

    LE_TEST_ASSERT(taf_sms_SetCallback(tmpMsg_symbol, Callback_MsgSendStatus, NULL) == LE_OK, "Test taf_sms_SetCallback");

    LE_TEST_ASSERT(taf_sms_SetText(tmpMsg_symbol, TEXT_PATTERN_SYMBOL) == LE_OK, "Test taf_sms_SetText");

    LE_TEST_ASSERT(taf_sms_Send(tmpMsg_symbol) == LE_OK, "Test taf_sms_Send %s", "#s# + send msg with type [symbol]");

    le_thread_Sleep(TIMEOUT_TX_TEST);

    tmpMsg_binary = taf_sms_Create();

    LE_TEST_ASSERT(tmpMsg_binary, "Test taf_sms_Send");

    LE_TEST_ASSERT(taf_sms_SetDestination(tmpMsg_binary, DEST_TEST) == LE_OK, "Test taf_sms_SetDestination");

    LE_TEST_ASSERT(taf_sms_SetCallback(tmpMsg_binary, Callback_MsgSendStatus, NULL) == LE_OK, "Test taf_sms_SetCallback");

    LE_TEST_ASSERT(taf_sms_SetBinary(tmpMsg_binary, binary_pattern, sizeof(binary_pattern)) == LE_OK, "Test taf_sms_SetBinary");

    LE_TEST_ASSERT(taf_sms_Send(tmpMsg_binary) == LE_OK, "Test taf_sms_Send %s", "#s# + send msg with type [binary]");

    le_thread_Sleep(TIMEOUT_TX_TEST);

    tmpMsg_ucs2 = taf_sms_Create();

    LE_TEST_ASSERT(tmpMsg_ucs2, "Test taf_sms_Send");

    LE_TEST_ASSERT(taf_sms_SetDestination(tmpMsg_ucs2, DEST_TEST) == LE_OK, "Test taf_sms_SetDestination");

    LE_TEST_ASSERT(taf_sms_SetCallback(tmpMsg_ucs2, Callback_MsgSendStatus, NULL) == LE_OK, "Test taf_sms_SetCallback");

    LE_TEST_ASSERT(taf_sms_SetUCS2(tmpMsg_ucs2, ucs2_pattern, sizeof(ucs2_pattern)/sizeof(ucs2_pattern[0])) == LE_OK, "Test taf_sms_SetUCS2");

    LE_TEST_ASSERT(taf_sms_Send(tmpMsg_ucs2) == LE_OK, "Test taf_sms_Send %s", "#s# + send msg with type [ucs2]");

    le_thread_Sleep(TIMEOUT_TX_TEST);

    LE_TEST_ASSERT(taf_sms_GetSendStatus(tmpMsg_alphabet) == TAF_SMS_TXSTS_SENT, "Test taf_sms_GetSendStatus");

    LE_TEST_ASSERT(taf_sms_GetSendStatus(tmpMsg_num) == TAF_SMS_TXSTS_SENT, "Test taf_sms_GetSendStatus");

    LE_TEST_ASSERT(taf_sms_GetSendStatus(tmpMsg_symbol) == TAF_SMS_TXSTS_SENT, "Test taf_sms_GetSendStatus");

    LE_TEST_ASSERT(taf_sms_GetSendStatus(tmpMsg_binary) == TAF_SMS_TXSTS_SENT, "Test taf_sms_GetSendStatus");

    LE_TEST_ASSERT(taf_sms_GetSendStatus(tmpMsg_ucs2) == TAF_SMS_TXSTS_SENT, "Test taf_sms_GetSendStatus");

    le_event_RunLoop();
}

/*======================================================================

 FUNCTION        Test_taf_sms_Send

 DESCRIPTION     Test send message APIs of SMS service.

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_sms_Send
(
    void
)
{
    TxThreadRef = le_thread_Create("SmsUnitTestTx", SmsTxThread, NULL);
    le_thread_Start(TxThreadRef);

    return;
}

/*======================================================================

 FUNCTION        RxHandler

 DESCRIPTION     Handle RX message

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void RxHandler
(
    taf_sms_MsgRef_t msgRef,
    void* context
)
{

    RxSmsContent_t rxContent;
    size_t len = 0;
    size_t checkLen = 0;

    memset(rxContent.text, 0, TAF_SMS_TEXT_BYTES);

    LE_TEST_ASSERT(taf_sms_GetType(msgRef) == TAF_SMS_TYPE_RX, "Test taf_sms_GetType");

    LE_TEST_ASSERT(taf_sms_GetSenderTel(msgRef, rxContent.text, sizeof(rxContent.text)) == LE_OK, "Test taf_sms_GetSenderTel");

    LE_INFO("taf_sms_GetSenderTel = %s", rxContent.text);

    len = sizeof(rxContent.pdu);

    LE_TEST_ASSERT(taf_sms_GetPDU(msgRef, rxContent.pdu, &len) == LE_OK, "Test taf_sms_GetPDU");

    LE_INFO("PDU len = %" PRIuS, len);

    LE_TEST_ASSERT(len > 0, "Test taf_sms_GetPDU length");

    checkLen = taf_sms_GetPDULen(msgRef);

    LE_TEST_ASSERT(checkLen == len, "Test taf_sms_GetPDULen");

    bool PDU_IsEmpty = true;

    for(int i = 0; i < len; i++)
    {
        LE_INFO("PDU data[%d] = 0x%x" PRIuS, i, rxContent.pdu[i]);
        if(rxContent.pdu[i] != 0)
        {
            PDU_IsEmpty = false;
            break;
        }
    }
    LE_TEST_ASSERT(PDU_IsEmpty == false, "Test taf_sms_GetPDU content");

    switch(taf_sms_GetFormat(msgRef))
    {
        case TAF_SMS_FORMAT_TEXT:
            LE_TEST_ASSERT(taf_sms_GetText(msgRef, rxContent.text, sizeof(rxContent.text)) == LE_OK, "Test taf_sms_GetText");
            LE_INFO("taf_sms_GetText = %s", rxContent.text);
            break;

        case TAF_SMS_FORMAT_BINARY:

            len = sizeof(rxContent.binary);

            LE_TEST_ASSERT(taf_sms_GetBinary(msgRef, rxContent.binary, &len) == LE_OK, "Test taf_sms_GetBinary");

            LE_INFO("taf_sms_GetBinary, len:%" PRIuS, len);
            for(int i = 0; i < len; i++)
            {
                LE_INFO("0x%.2X", rxContent.binary[i]);
            }
            break;

        case TAF_SMS_FORMAT_UCS2:

            len = sizeof(rxContent.ucs2);

            LE_TEST_ASSERT(taf_sms_GetUCS2(msgRef, rxContent.ucs2, &len) == LE_OK, "Test taf_sms_GetUCS2");

            LE_INFO("taf_sms_GetUCS2, len:%" PRIuS, len);
            for(int i = 0; i < len; i++)
            {
                LE_INFO("0x%.4X", rxContent.ucs2[i]);
            }
            break;

        default:
            break;
    }

    RxCount++;

    switch(RxCount)
    {
        case 1:
            LE_TEST_ASSERT(strstr(rxContent.text, TEXT_PATTERN_TEST) != NULL, "Test RX message content");
            break;

        case 2:
            LE_TEST_ASSERT(strstr(rxContent.text, TEXT_PATTERN_NUM) != NULL, "Test RX message content");
            break;

        case 3:
            LE_TEST_ASSERT(strstr(rxContent.text, TEXT_PATTERN_SYMBOL) != NULL, "Test RX message content");
            break;

        case 4:
            LE_TEST_ASSERT(memcmp(rxContent.binary, binary_pattern, sizeof(binary_pattern)) == 0, "Test RX message content");
            break;

        case 5:
            LE_TEST_ASSERT(memcmp(rxContent.binary, ucs2_pattern, sizeof(ucs2_pattern)) == 0, "Test RX message content");
            break;

        default:
            break;
    }

    if(RxCount == AMOUNT_MSG_TX)
    {
        le_sem_Post(sem_RxTest);
    }
}

__attribute__((unused)) static void* SmsRxHandlerThread
(
    void* contextPtr
)
{
    taf_sms_ConnectService();

    RxHandlerRef = taf_sms_AddRxMsgHandler(RxHandler, NULL);

    LE_TEST_ASSERT(RxHandlerRef != NULL, "Test taf_sms_AddRxMsgHandler");

    le_event_RunLoop();
}

/*======================================================================

 FUNCTION        Test_taf_sms_AddRemoveRxHandler

 DESCRIPTION     Test adding/removing RX handler API of SMS service.

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_sms_AddRemoveRxHandler
(
    void
)
{
    RxHandlerRef = taf_sms_AddRxMsgHandler(RxHandler, NULL);

    LE_TEST_ASSERT(RxHandlerRef != NULL, "Test taf_sms_AddRxMsgHandler");

    taf_sms_RemoveRxMsgHandler(RxHandlerRef);
}

/*======================================================================

 FUNCTION        Test_taf_sms_Receive

 DESCRIPTION     Test receive message APIs of SMS service.

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_sms_Receive
(
    void
)
{
    sem_RxTest = le_sem_Create("MsgRxSem", 0);

    RxThreadRef = le_thread_Create("SmsUnitTestRx", SmsRxHandlerThread, NULL);
    le_thread_Start(RxThreadRef);
}

/*======================================================================

 FUNCTION        Test_taf_sms_Smsc

 DESCRIPTION     Test get/set sms center address

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_sms_Smsc
(
    void
)
{
    char addr[TAF_SMS_SMSC_ADDR_BYTES - 1] = {};
    size_t len = TAF_SMS_SMSC_ADDR_BYTES - 1;

    LE_TEST_ASSERT(taf_sms_GetSmsCenterAddress(PHONE_ID_PATTERN_1, addr, len) == LE_OK,
        "Test taf_sms_GetSmsCenterAddress");

    LE_TEST_ASSERT(taf_sms_SetSmsCenterAddress(PHONE_ID_PATTERN_1, addr) == LE_OK,
        "Test taf_sms_SetSmsCenterAddress");

    le_thread_Sleep(TIME_SET_SMSC);

    return;
}

/*======================================================================

 FUNCTION        Test_taf_sms_SendPdu

 DESCRIPTION     Test get/set sms center address

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_sms_SendPdu
(
    void
)
{
#ifdef TEST_SMS_PDU
    uint32_t dataSize = sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]);
    LE_TEST_ASSERT(taf_sms_SendPduMsg(PDU_TEST_PATTERN_7BITS, dataSize, 1000) == LE_OK,
                    "Test taf_sms_SendPduMsg");

    le_thread_Sleep(TIMEOUT_TX_TEST);

    LE_TEST_ASSERT(taf_sms_SendPduMsgEx(PHONE_ID_PATTERN_1,
                                        PDU_TEST_PATTERN_7BITS,
                                        dataSize,
                                        1000) == LE_OK,
                                        "Test taf_sms_SendPduMsgEx");
#else
    LE_UNUSED(PDU_TEST_PATTERN_7BITS);
#endif
    return;
}

/*======================================================================

 FUNCTION        Test_taf_sms_CellBroadcast

 DESCRIPTION     Test all APIs about cell broadcast messages.

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

__attribute__((unused)) static void Test_taf_sms_CellBroadcast
(
    void
)
{
    LE_TEST_ASSERT(taf_sms_ActivateCellBroadcast(PHONE_ID_PATTERN_1) == LE_OK, "Test taf_sms_ActivateCellBroadcast");

    LE_TEST_ASSERT(taf_sms_AddCellBroadcastIds(PHONE_ID_PATTERN_1, 4352, 4354) == LE_OK, "Test taf_sms_AddCellBroadcastIds");
    LE_TEST_ASSERT(taf_sms_RemoveCellBroadcastIds(PHONE_ID_PATTERN_1, 4352, 4354) == LE_OK, "Test taf_sms_RemoveCellBroadcastIds");

    LE_TEST_ASSERT(taf_sms_DeactivateCellBroadcast(PHONE_ID_PATTERN_1) == LE_OK, "Test taf_sms_DeactivateCellBroadcast");

    return;
}

/*======================================================================

 FUNCTION        Test_main

 DESCRIPTION     Test main function, call each test sub-function

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

void Test_main
(
    void
)
{
    LE_INFO("===== Test_taf_sms_CellBroadcast =====");
    Test_taf_sms_CellBroadcast();
    LE_INFO("##### Test_taf_sms_CellBroadcast OK #####");

    LE_INFO("===== Test_taf_sms_SetGetPreferredStorage =====");
    Test_taf_sms_SetGetPreferredStorage();
    LE_INFO("##### Test_taf_sms_SetGetPreferredStorage OK #####");

    LE_INFO("===== Test_taf_sms_AddRemoveFullStorageHandler =====");
    Test_taf_sms_AddRemoveFullStorageHandler();
    LE_INFO("##### Test_taf_sms_AddRemoveFullStorageHandler OK #####");

    LE_INFO("===== Test_taf_sms_SetGetParam =====");
    Test_taf_sms_SetGetParam();
    LE_INFO("##### Test_taf_sms_SetGetParam OK #####");

    LE_INFO("===== Test_taf_sms_AddRemoveRxHandler =====");
    Test_taf_sms_AddRemoveRxHandler();
    LE_INFO("##### Test_taf_sms_AddRemoveRxHandler OK #####");

    LE_INFO("===== Test_taf_sms_Smsc =====");
    Test_taf_sms_Smsc();
    LE_INFO("##### Test_taf_sms_Smsc OK #####");

    LE_INFO("===== Test_taf_sms_Receive =====");
    Test_taf_sms_Receive();

    LE_INFO("===== Test_taf_sms_Send =====");
    Test_taf_sms_Send();

    LE_ASSERT_OK(WaitForSem_Timeout(sem_RxTest, TIMEOUT_RX_TEST));

    LE_TEST_ASSERT(RxCount == AMOUNT_MSG_TX, "Test RX messages");

    taf_sms_RemoveRxMsgHandler(RxHandlerRef);

    le_thread_Cancel(RxThreadRef);

    LE_INFO("##### Test_taf_sms_Receive OK #####");

    le_thread_Cancel(TxThreadRef);

    LE_INFO("##### Test_taf_sms_Send OK #####");

    LE_INFO("===== Test_taf_sms_SendPdu =====");
    Test_taf_sms_SendPdu();
    LE_INFO("##### Test_taf_sms_SendPdu OK #####");

    LE_INFO("===== Test_taf_sms_CreateDeleteRxMsgList =====");
    Test_taf_sms_CreateDeleteRxMsgList();
    LE_INFO("##### Test_taf_sms_CreateDeleteRxMsgList OK #####");

    LE_INFO("===== Test_taf_sms_SetGetReadStatus =====");
    Test_taf_sms_SetGetReadStatus();
    LE_INFO("##### Test_taf_sms_SetGetReadStatus OK #####");

    LE_INFO("===== Test_taf_sms_SetGetLockStatus =====");
    Test_taf_sms_SetGetLockStatus();
    LE_INFO("##### Test_taf_sms_SetGetLockStatus OK #####");

#ifndef LE_CONFIG_TARGET_SIMULATION
    LE_INFO("===== Test_taf_sms_EncryptFromStorage =====");
    Test_taf_sms_EncryptFromStorage();
    LE_INFO("##### Test_taf_sms_EncryptFromStorage OK #####");
#endif

    LE_INFO("===== Test_taf_sms_DeleteMsgFromStorage =====");
    Test_taf_sms_DeleteMsgFromStorage();
    LE_INFO("##### Test_taf_sms_DeleteMsgFromStorage OK #####");

    LE_INFO("===== Test_taf_sms_DeleteAllMsgFromStorage =====");
    Test_taf_sms_DeleteAllMsgFromStorage();
    LE_INFO("##### Test_taf_sms_DeleteAllMsgFromStorage OK #####");

    LE_INFO("##### taf SMS unit test PASS #####");

    exit(EXIT_SUCCESS);
}

/*======================================================================

 FUNCTION        COMPONENT_INIT

 DESCRIPTION     Start SMS unit test

 DEPENDENCIES    None

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS    None

======================================================================*/
/**
 * Start app : app start tafSmsUnitTest
 * Execute app : app runProc tafSmsUnitTest --exe=tafSmsUnitTest -- <Phone number>
 * Phone number could be set as the same SIM phone number with revceiver, it will
 * test to send message to the test target itself
 */
COMPONENT_INIT
{
    if (le_arg_NumArgs() == 1)
    {
        const char* phoneNumber = le_arg_GetArg(0);
        if (NULL == phoneNumber)
        {
            LE_ERROR("phoneNumber is NULL");
            exit(EXIT_FAILURE);
        }
        le_utf8_Copy((char*)DEST_TEST, phoneNumber, TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES, NULL);
        LE_INFO("Phone number %s", DEST_TEST);
    }
    else if(le_arg_NumArgs() == 0)
    {
        le_utf8_Copy((char*)DEST_TEST, DEST_PATTERN_VALID, TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES, NULL);
        LE_INFO("Phone number %s", DEST_TEST);
    }
    else
    {
        LE_ERROR("Please start with => app runProc tafSmsUnitTest --exe=tafSmsUnitTest -- <SIM Phone Number>");
    }

    Test_main();
}
