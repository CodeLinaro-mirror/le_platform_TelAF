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
 * @file       tafSmsIntTest.cpp
 * @brief      This file includes integration test functions of the SMS Service.
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Define test pattern
 */
//--------------------------------------------------------------------------------------------------
#define TEXT_PATTERN_EMPTY  ""
#define TEXT_PATTERN_TEST   "Integration test from telaf-1. Integration test from telaf-2. Integration test from telaf-3."
#define TEXT_PATTERN_NUM    "0123456789 0123456789 0123456789 0123456789 0123456789"
#define TEXT_PATTERN_SYMBOL "~!@#$^&*()_+{}:<>?"

#define DEST_PATTERN_EMPTY  ""
#define DEST_PATTERN_VALID  "0978172902"    // Use the same sim and device to send/receive message

#define PHONE_ID_PATTERN_1  1               // Phone ID to test

#define TIME_TX_SEND        2               // Time interval between sending message
#define TIMEOUT_TX_TEST     30              // Time interval for sending all messages and get callback
#define TIMEOUT_RX_TEST     60              // Wait for receicing message sent from remote party

#define AMOUNT_MSG_TX       3               // Total message amount to send from this test app
#define AMOUNT_MSG_RX       2               // Total message amount to send from this test app

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

static le_sem_Ref_t sem_TxTest;
static le_sem_Ref_t sem_RxTest;

static taf_sms_RxMsgHandlerRef_t RxHandlerRef;

static le_thread_Ref_t RxThreadRef;
static le_thread_Ref_t TxThreadRef;

static uint8_t RxCount = 0;
static uint8_t TxCount = 0;

static bool RxPassed = false;
static bool TxPassed = false;

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

 FUNCTION        TxCallback

 DESCRIPTION     handler to get status of sending message

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

static void TxCallback
(
    taf_sms_MsgRef_t msgRef,
    taf_sms_SendStatus_t status,
    void* contextPtr
)
{
    LE_INFO("msg: %p, Sendstatus: %d", msgRef, status);

    LE_ASSERT(status == TAF_SMS_TXSTS_SENT);

    TxCount++;

    LE_INFO("taf_sms_Send, send msg[%d] OK", TxCount);

    taf_sms_Delete(msgRef);

    if(TxCount == AMOUNT_MSG_TX)
    {
         le_sem_Post(sem_TxTest);
    }
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

    LE_INFO("taf_sms_Create, create msg OK");

    LE_ASSERT(taf_sms_SetText(tmpMsg_1, TEXT_PATTERN_TEST) == LE_OK);

    LE_INFO("taf_sms_SetText, set text OK");

    LE_ASSERT(taf_sms_SetDestination(tmpMsg_1, DEST_TEST) == LE_OK);

    LE_INFO("taf_sms_SetDestination, set destination OK");

    LE_ASSERT(taf_sms_SetCallback(tmpMsg_1, TxCallback, NULL) == LE_OK);

    LE_INFO("taf_sms_SetCallback, set callback OK");

    LE_ASSERT(taf_sms_Send(tmpMsg_1) == LE_OK);

    le_thread_Sleep(TIME_TX_SEND);

    tmpMsg_2 = taf_sms_Create();

    LE_ASSERT(tmpMsg_2);

    LE_INFO("taf_sms_Create, create msg OK");

    LE_ASSERT(taf_sms_SetText(tmpMsg_2, TEXT_PATTERN_NUM) == LE_OK);

    LE_INFO("taf_sms_SetText, set text OK");

    LE_ASSERT(taf_sms_SetDestination(tmpMsg_2, DEST_TEST) == LE_OK);

    LE_INFO("taf_sms_SetDestination, set destination OK");

    LE_ASSERT(taf_sms_SetCallback(tmpMsg_2, TxCallback, NULL) == LE_OK);

    LE_INFO("taf_sms_SetCallback, set callback OK");

    LE_ASSERT(taf_sms_Send(tmpMsg_2) == LE_OK);

    le_thread_Sleep(TIME_TX_SEND);

    tmpMsg_3 = taf_sms_Create();

    LE_ASSERT(tmpMsg_3);

    LE_INFO("taf_sms_Create, create msg OK");

    LE_ASSERT(taf_sms_SetText(tmpMsg_3, TEXT_PATTERN_SYMBOL) == LE_OK);

    LE_INFO("taf_sms_SetText, set text OK");

    LE_ASSERT(taf_sms_SetDestination(tmpMsg_3, DEST_TEST) == LE_OK);

    LE_INFO("taf_sms_SetDestination, set destination OK");

    LE_ASSERT(taf_sms_SetCallback(tmpMsg_3, TxCallback, NULL) == LE_OK);

    LE_INFO("taf_sms_SetCallback, set callback OK");

    LE_ASSERT(taf_sms_Send(tmpMsg_3) == LE_OK);

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

static void* Test_taf_sms_Send
(
    void* contextPtr
)
{
    LE_INFO("===== Test_taf_sms_Send =====");

    sem_TxTest = le_sem_Create("sem_TxTest", 0);

    TxThreadRef = le_thread_Create("SmsIntegrationTestTx", SmsTxThread, NULL);
    le_thread_Start(TxThreadRef);

    LE_ASSERT_OK(WaitForSem_Timeout(sem_TxTest, TIMEOUT_TX_TEST));

    LE_INFO("##### Test_taf_sms_Send PASS #####");

    TxPassed = true;

    le_thread_Cancel(TxThreadRef);

    le_thread_Exit(NULL);

    return NULL;
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
    char           tel[TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES];
    size_t         userdatalen = 0;

    LE_INFO("SMS received");

    LE_ASSERT(taf_sms_GetType(msgRef) == TAF_SMS_TYPE_RX);

    LE_INFO("taf_sms_GetType, msg type OK");

    LE_ASSERT(taf_sms_GetSenderTel(msgRef, tel, 1) == LE_OVERFLOW);

    LE_INFO("taf_sms_GetSenderTel, test expected overflow OK");

    LE_ASSERT(taf_sms_GetSenderTel(msgRef, tel, sizeof(tel)) == LE_OK);

    LE_ASSERT(strncmp(tel, DEST_TEST, sizeof(tel)) == 0);

    LE_INFO("taf_sms_GetSenderTel, sender phone number OK");

    userdatalen = taf_sms_GetUserdataLen(msgRef);

    LE_ASSERT(userdatalen > 0);

    LE_INFO("taf_sms_GetUserdataLen, data length OK");

    LE_ASSERT(taf_sms_GetText(msgRef, rxContent.text, sizeof(rxContent.text)) == LE_OK);

    LE_INFO("taf_sms_GetText = %s", rxContent.text);

    LE_INFO("taf_sms_GetText, text OK");

    LE_ASSERT(taf_sms_GetReadStatus(msgRef) == TAF_SMS_RXSTS_UNREAD);

    LE_INFO("taf_sms_GetReadStatus, read status OK");

    LE_ASSERT(taf_sms_DeleteFromStorage(msgRef) == LE_OK);

    LE_INFO("taf_sms_DeleteFromStorage, delete msg OK");

    RxCount++;

    if(RxCount == AMOUNT_MSG_RX)
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

    LE_INFO("***** Please send %d message to me in %ds *****", AMOUNT_MSG_RX, TIMEOUT_RX_TEST);

    le_event_RunLoop();
}

/*======================================================================

 FUNCTION        Test_taf_sms_Receive

 DESCRIPTION     Test receive message APIs of SMS service.

 DEPENDENCIES    None

 PARAMETERS      void

 RETURN VALUE    void

 SIDE EFFECTS

======================================================================*/

static void* Test_taf_sms_Receive
(
    void* contextPtr
)
{
    LE_INFO("===== Test_taf_sms_Receive =====");

    taf_sms_ConnectService();

    sem_RxTest = le_sem_Create("sem_RxTest", 0);

    RxThreadRef = le_thread_Create("SmsUnitTestRx", SmsRxHandlerThread, NULL);
    le_thread_Start(RxThreadRef);

    LE_ASSERT_OK(WaitForSem_Timeout(sem_RxTest, TIMEOUT_RX_TEST));

    RxPassed = true;

    taf_sms_RemoveRxMsgHandler(RxHandlerRef);

    le_thread_Cancel(RxThreadRef);

    LE_INFO("##### Test_taf_sms_Receive PASS #####");

    le_thread_Exit(NULL);

    return NULL;
}

/*======================================================================

 FUNCTION        Test_main

 DESCRIPTION     Start send/receive message test

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

    le_thread_Start(le_thread_Create("SmsIntTestRx", Test_taf_sms_Receive, NULL));

    le_thread_Start(le_thread_Create("SmsIntTestTx", Test_taf_sms_Send, NULL));

    while(RxPassed == false || TxPassed == false)
    {
        le_thread_Sleep(1);
    }

    LE_INFO("##### taf SMS integration test COMPLETE #####");

    exit(EXIT_SUCCESS);
}

/*======================================================================

 FUNCTION        COMPONENT_INIT

 DESCRIPTION     Start SMS integration test

 DEPENDENCIES    None

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS    None

======================================================================*/
/**
 * Start app : app start tafSmsIntTest
 * Execute app : app runProc tafSmsIntTest --exe=tafSmsIntTest -- <Phone number>
 * Phone number should be a remote party to receive messages sent from this test
 * and please send 2 arbitrary messages to the test target  in 60 seconds after
 * start this test app
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
        LE_ERROR("Please start with => app runProc tafSmsIntTest --exe=tafSmsIntTest -- <SIM Phone Number>");
    }

    Test_main();
}
