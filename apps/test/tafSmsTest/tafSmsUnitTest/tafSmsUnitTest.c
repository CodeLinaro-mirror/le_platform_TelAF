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

#define DEST_PATTERN_EMPTY  ""
#define DEST_PATTERN_VALID  "0909070026"    // Use the same sim and device to send/receive message

#define SMSC_ADDR_PATTERN_VALID "\"+886935874443\"" //SMS center address for TWN Mobile

#define PHONE_ID_PATTERN_1  1               // Phone ID to test

#define TIMEOUT_TX_TEST     3               // Time interval between sending message
#define TIMEOUT_RX_TEST     25              // Wait for receicing message sent from this test app

#define TIME_SET_SMSC       5               // Wait for settingi sms center take effect

#define AMOUNT_MSG_TX       3               // Total message amount to send from this test app

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

le_result_t WaitForSem_Timeout
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

static void Test_taf_sms_CreateDeleteRxMsgList
(
    void
)
{
    RxMsgListRef = taf_sms_CreateRxMsgList();

    LE_ASSERT(RxMsgListRef != NULL);

    taf_sms_DeleteList(RxMsgListRef);
}

/*======================================================================

 FUNCTION        Test_taf_sms_SetGetParam

 DESCRIPTION     Test setting/getting parameter APIs of SMS service.

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

static void Test_taf_sms_SetGetParam
(
    void
)
{
    taf_sms_MsgRef_t    tmpMsg;
    char                text[TAF_SMS_TEXT_BYTES] = {0};

    tmpMsg = taf_sms_Create();

    LE_ASSERT(tmpMsg);

    LE_ASSERT(taf_sms_SetText(tmpMsg, TEXT_PATTERN_EMPTY) == LE_BAD_PARAMETER);

    LE_ASSERT(taf_sms_SetText(tmpMsg, TEXT_PATTERN_TEST) == LE_OK);

    LE_ASSERT(taf_sms_GetText(tmpMsg, text, sizeof(text)) == LE_OK);

    LE_ASSERT(taf_sms_GetUserdataLen(tmpMsg) == strlen(TEXT_PATTERN_TEST));

    LE_ASSERT(strncmp(text, TEXT_PATTERN_TEST, strlen(TEXT_PATTERN_TEST)) == 0);

    LE_ASSERT(taf_sms_SetDestination(tmpMsg, DEST_TEST) == LE_OK);

    LE_ASSERT(taf_sms_SetPhoneId(tmpMsg, PHONE_ID_PATTERN_1) == LE_OK);

    LE_ASSERT(taf_sms_GetType(tmpMsg) == TAF_SMS_TYPE_TX);

    taf_sms_Delete(tmpMsg);
}

/*======================================================================

 FUNCTION        Callback_MsgSendStatus

 DESCRIPTION     handler to get status of sending message

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

static void Callback_MsgSendStatus
(
    taf_sms_MsgRef_t msgRef,
    taf_sms_SendStatus_t status,
    void* contextPtr
)
{
    LE_INFO("msg: %p, Sendstatus: %d", msgRef, status);

    LE_ASSERT(status == TAF_SMS_TXSTS_SENT);

    TxCount++;

    taf_sms_Delete(msgRef);
}

static void* SmsTxThread
(
    void* contextPtr
)
{
    taf_sms_ConnectService();

    taf_sms_MsgRef_t tmpMsg_1;
    taf_sms_MsgRef_t tmpMsg_2;
    taf_sms_MsgRef_t tmpMsg_3;

    tmpMsg_1 = taf_sms_Create();

    LE_ASSERT(tmpMsg_1);

    LE_ASSERT(taf_sms_SetDestination(tmpMsg_1, DEST_TEST) == LE_OK);

    LE_ASSERT(taf_sms_SetCallback(tmpMsg_1, Callback_MsgSendStatus, NULL) == LE_OK);

    LE_ASSERT(taf_sms_SetText(tmpMsg_1, TEXT_PATTERN_TEST) == LE_OK);

    LE_ASSERT(taf_sms_Send(tmpMsg_1) == LE_OK);

    le_thread_Sleep(TIMEOUT_TX_TEST);

    tmpMsg_2 = taf_sms_Create();

    LE_ASSERT(tmpMsg_2);

    LE_ASSERT(taf_sms_SetDestination(tmpMsg_2, DEST_TEST) == LE_OK);

    LE_ASSERT(taf_sms_SetCallback(tmpMsg_2, Callback_MsgSendStatus, NULL) == LE_OK);

    LE_ASSERT(taf_sms_SetText(tmpMsg_2, TEXT_PATTERN_NUM) == LE_OK);

    LE_ASSERT(taf_sms_Send(tmpMsg_2) == LE_OK);

    le_thread_Sleep(TIMEOUT_TX_TEST);

    tmpMsg_3 = taf_sms_Create();

    LE_ASSERT(tmpMsg_3);

    LE_ASSERT(taf_sms_SetDestination(tmpMsg_3, DEST_TEST) == LE_OK);

    LE_ASSERT(taf_sms_SetCallback(tmpMsg_3, Callback_MsgSendStatus, NULL) == LE_OK);

    LE_ASSERT(taf_sms_SetText(tmpMsg_3, TEXT_PATTERN_SYMBOL) == LE_OK);

    LE_ASSERT(taf_sms_Send(tmpMsg_3) == LE_OK);

    le_thread_Sleep(TIMEOUT_TX_TEST);

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

static void Test_taf_sms_Send
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

static void RxHandler
(
    taf_sms_MsgRef_t msgRef,
    void* context
)
{

    RxSmsContent_t rxContent;

    memset(rxContent.text, 0, TAF_SMS_TEXT_BYTES);

    LE_ASSERT(taf_sms_GetType(msgRef) == TAF_SMS_TYPE_RX);

    LE_ASSERT(taf_sms_GetSenderTel(msgRef, rxContent.text, sizeof(rxContent.text)) == LE_OK);

    LE_INFO("taf_sms_GetSenderTel = %s", rxContent.text);

    LE_ASSERT(taf_sms_GetText(msgRef, rxContent.text, sizeof(rxContent.text)) == LE_OK);

    LE_INFO("taf_sms_GetText = %s", rxContent.text);

    RxCount++;

    switch(RxCount)
    {
        case 1:
            LE_ASSERT(strstr(rxContent.text, TEXT_PATTERN_TEST) != NULL);
            break;

        case 2:
            LE_ASSERT(strstr(rxContent.text, TEXT_PATTERN_NUM) != NULL);
            break;

        case 3:
            LE_ASSERT(strstr(rxContent.text, TEXT_PATTERN_SYMBOL) != NULL);
            break;

        default:
            break;
    }

    if(RxCount == AMOUNT_MSG_TX)
    {
        le_sem_Post(sem_RxTest);
    }
}

static void* SmsRxHandlerThread
(
    void* contextPtr
)
{
    taf_sms_ConnectService();

    RxHandlerRef = taf_sms_AddRxMsgHandler(RxHandler, NULL);

    LE_ASSERT(RxHandlerRef != NULL);

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

static void Test_taf_sms_AddRemoveRxHandler
(
    void
)
{
    RxHandlerRef = taf_sms_AddRxMsgHandler(RxHandler, NULL);

    LE_ASSERT(RxHandlerRef != NULL);

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

static void Test_taf_sms_Receive
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

static void Test_taf_sms_Smsc
(
    void
)
{
    char addr[TAF_SMS_SMSC_ADDR_BYTES - 1];
    size_t len = TAF_SMS_SMSC_ADDR_BYTES - 1;

    LE_ASSERT(taf_sms_GetSmsCenterAddress(PHONE_ID_PATTERN_1, addr, len) == LE_OK);

    LE_ASSERT(taf_sms_SetSmsCenterAddress(PHONE_ID_PATTERN_1, SMSC_ADDR_PATTERN_VALID) == LE_OK);

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

static void Test_taf_sms_SendPdu
(
    void
)
{
#ifdef TEST_SMS_PDU
    uint32_t dataSize = sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]);
    taf_sms_SendPduMsg(PDU_TEST_PATTERN_7BITS, dataSize, 1000);
#else
    LE_UNUSED(PDU_TEST_PATTERN_7BITS);
#endif
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
    LE_INFO("===== Test_taf_sms_SetGetParam =====");
    Test_taf_sms_SetGetParam();
    LE_INFO("##### Test_taf_sms_SetGetParam OK #####");

    LE_INFO("===== Test_taf_sms_AddRemoveRxHandler =====");
    Test_taf_sms_AddRemoveRxHandler();
    LE_INFO("##### Test_taf_sms_AddRemoveRxHandler OK #####");

    LE_INFO("===== Test_taf_sms_CreateDeleteRxMsgList =====");
    Test_taf_sms_CreateDeleteRxMsgList();
    LE_INFO("##### Test_taf_sms_CreateDeleteRxMsgList OK #####");

    LE_INFO("===== Test_taf_sms_Smsc =====");
    Test_taf_sms_Smsc();
    LE_INFO("##### Test_taf_sms_Smsc OK #####");

    LE_INFO("===== Test_taf_sms_SendPdu =====");
    Test_taf_sms_SendPdu();
    LE_INFO("##### Test_taf_sms_SendPdu OK #####");

    LE_INFO("===== Test_taf_sms_Receive =====");
    Test_taf_sms_Receive();

    LE_INFO("===== Test_taf_sms_Send =====");
    Test_taf_sms_Send();

    LE_ASSERT_OK(WaitForSem_Timeout(sem_RxTest, TIMEOUT_RX_TEST));

    LE_ASSERT(RxCount == AMOUNT_MSG_TX)

    taf_sms_RemoveRxMsgHandler(RxHandlerRef);

    le_thread_Cancel(RxThreadRef);

    LE_INFO("##### Test_taf_sms_Receive OK #####");

    le_thread_Cancel(TxThreadRef);

    LE_INFO("##### Test_taf_sms_Send OK #####");

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
