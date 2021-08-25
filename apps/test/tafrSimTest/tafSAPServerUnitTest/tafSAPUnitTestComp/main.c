/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "legato.h"
#include "interfaces.h"

static le_sem_Ref_t TestSemaphoreRef;
static taf_sap_MessageHandlerRef_t  MsgHandlerRef;
static uint8_t ExpectedMsg[TAF_SAP_MAX_MSG_SIZE] = {0};
static size_t ExpectedMsgSize = 0;
static uint8_t ConnectReqMsg[12] =
{ 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x14, 0x00, 0x00};
static uint8_t ConnectReqLength = 12;
static uint8_t ConnectOkRespMsg[12] =
{ 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};

static uint8_t ATRReqMsg[4] =
{0x07, 0x00, 0x00, 0x00};

static uint8_t ATRReqLength = 4;

static uint8_t ATRRespMsg[40] =
{0x08, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x06,
 0x00, 0x00, 0x17, 0x3B, 0x9F, 0x97, 0x80, 0x3F, 0xC7, 0x82, 0x80, 0x31, 0xE0,
 0x73, 0xFE, 0x21, 0x1F, 0x64, 0x09, 0x06, 0x01, 0x00, 0x82, 0x90, 0x00, 0x68, 0x00 };

static uint8_t ATRRespLength = 40;

static uint8_t PowerOnReq[4] = {0x0B, 0x00, 0x00, 0x00};
static uint8_t PowerOnReqLength = 4;

static uint8_t PowerOnResp[12] =
{0x0C, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
static uint8_t PowerOnRespLength = 12;

static uint8_t PowerDownReq[4] = {0x09, 0x00, 0x00, 0x00};
static uint8_t PowerDownReqLength = 4;

static uint8_t PowerDownResp[12] =
{0x0A, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
static uint8_t PowerDownRespLength = 12;

static uint8_t APDUReq[16] = {0x05, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x07,
                            0x00, 0xA4, 0x00, 0x04, 0x02, 0x3F, 0x00, 0x00};
static uint8_t APDUReqLength = 16;
static uint8_t APDUResp[60] = { 0x06, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                              0x04, 0x00, 0x00, 0x29, 0x62, 0x25, 0x82, 0x02, 0x78, 0x21, 0x83, 0x02,
                              0x3F, 0x00, 0xA5, 0x0B, 0x80, 0x01, 0x71, 0x83, 0x03, 0x07, 0x74, 0x76,
                              0x87, 0x01, 0x01, 0x8A, 0x01, 0x05, 0x8B, 0x03, 0x2F, 0x06, 0x02, 0xC6,
                              0x06, 0x90, 0x01, 0x00, 0x83, 0x01, 0x01, 0x90, 0x00, 0x00, 0x00, 0x00 };
static uint8_t APDURespLength = 60;

static uint8_t CardResetReq[4] = {0x0D, 0x00, 0x00, 0x00};
static uint8_t CardResetReqLength = 4;
static uint8_t CardResetResp[12] = { 0x0E, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
static uint8_t CardResetRespLength = 12;


static uint8_t CardReaderReq[4] = {0x0F, 0x00, 0x00, 0x00};
static uint8_t CardReaderReqLength = 4;
static uint8_t CardReaderResp[20] = {0x10, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                     0x07, 0x00, 0x00, 0x01, 0x0F, 0x00, 0x00, 0x00 };
static uint8_t CardReaderRespLength = 20;

static uint8_t DisconnectReq[4] = {0x02, 0x00, 0x00, 0x00};
static uint8_t DisconnectReqLength = 4;
static uint8_t DisconnectResp[] = {0x03, 0x00, 0x00, 0x00};
static uint8_t DisconnectRespLength = 4;

static void SAPMessageHandler(const uint8_t* msgPtr,
                        size_t msgSize, void* contextPtr) {
    LE_INFO("Received SAP message");
    LE_DUMP(msgPtr, msgSize);
    int i = 0;
    LE_INFO("Expected SAP message");
    LE_DUMP(ExpectedMsg, ExpectedMsgSize);

    LE_ASSERT(msgSize == ExpectedMsgSize);

    for(i = 0; i < msgSize; i++) {
        LE_ASSERT(msgPtr[i] == ExpectedMsg[i]);
    }
    le_sem_Post(TestSemaphoreRef);
}

static void* Test_taf_sap_AddHandler(void* context) {

    taf_sap_ConnectService();

    MsgHandlerRef = taf_sap_AddMessageHandler(SAPMessageHandler, NULL);
    LE_ASSERT(MsgHandlerRef != NULL);
    LE_INFO("Message Handler Added successfully MsgHandlerRef = %p", MsgHandlerRef);

    le_event_RunLoop();
    return NULL;
}

static void Test_taf_sap_Conection(void) {

    //Initialize expected connect response message
    memset(ExpectedMsg, 0, sizeof(ExpectedMsg));
    memcpy(ExpectedMsg, ConnectOkRespMsg, 12);
    ExpectedMsgSize = 12;

    taf_sap_SendMessage(ConnectReqMsg, ConnectReqLength);

    le_sem_Wait(TestSemaphoreRef);

    LE_INFO("Connect test pass");
}

static void Test_taf_sap_ATR(void) {

    //Initialize expected ATR response message
    memset(ExpectedMsg, 0, sizeof(ExpectedMsg));
    memcpy(ExpectedMsg, ATRRespMsg, ATRRespLength);
    ExpectedMsgSize = ATRRespLength;

    taf_sap_SendMessage(ATRReqMsg, ATRReqLength);

    le_sem_Wait(TestSemaphoreRef);

    LE_INFO("ATR test pass");
}

static void Test_taf_sap_PowerOn(void) {

    //Initialize expected power ON response message
    memset(ExpectedMsg, 0, sizeof(ExpectedMsg));
    memcpy(ExpectedMsg, PowerOnResp, PowerOnRespLength);
    ExpectedMsgSize = PowerOnRespLength;

    taf_sap_SendMessage(PowerOnReq, PowerOnReqLength);

    le_sem_Wait(TestSemaphoreRef);

    LE_INFO("Power ON done");
}

static void Test_taf_sap_PowerDown(void) {

    //Initialize expected Power down response message
    memset(ExpectedMsg, 0, sizeof(ExpectedMsg));
    memcpy(ExpectedMsg, PowerDownResp, PowerDownRespLength);
    ExpectedMsgSize = PowerDownRespLength;

    taf_sap_SendMessage(PowerDownReq, PowerDownReqLength);

    le_sem_Wait(TestSemaphoreRef);

    LE_INFO("Power Down done");
}

static void Test_taf_sap_TransferAPDU(void) {

    //Initialize expected Power down response message
    memset(ExpectedMsg, 0, sizeof(ExpectedMsg));
    memcpy(ExpectedMsg, APDUResp, APDURespLength);
    ExpectedMsgSize = APDURespLength;

    taf_sap_SendMessage(APDUReq, APDUReqLength);

    le_sem_Wait(TestSemaphoreRef);

    LE_INFO("Tranfer APDU test completed");
}

static void Test_taf_sap_Reset(void) {

    //Initialize expected card reset response message
    memset(ExpectedMsg, 0, sizeof(ExpectedMsg));
    memcpy(ExpectedMsg, CardResetResp, CardResetRespLength);
    ExpectedMsgSize = CardResetRespLength;

    taf_sap_SendMessage(CardResetReq, CardResetReqLength);

    le_sem_Wait(TestSemaphoreRef);

    LE_INFO("Card reset done");
}

static void Test_taf_sap_TransferCardReader(void) {

    //Initialize expected card reader status response message
    memset(ExpectedMsg, 0, sizeof(ExpectedMsg));
    memcpy(ExpectedMsg, CardReaderResp, CardReaderRespLength);
    ExpectedMsgSize = CardReaderRespLength;

    taf_sap_SendMessage(CardReaderReq, CardReaderReqLength);

    le_sem_Wait(TestSemaphoreRef);

    LE_INFO("Transfer card reader status test done");
}

static void Test_taf_sap_Disconnect(void) {

    //Initialize expected disconnect response message
    memset(ExpectedMsg, 0, sizeof(ExpectedMsg));
    memcpy(ExpectedMsg, DisconnectResp, DisconnectRespLength);
    ExpectedMsgSize = DisconnectRespLength;

    taf_sap_SendMessage(DisconnectReq, DisconnectReqLength);

    le_sem_Wait(TestSemaphoreRef);

    LE_INFO("Disconnection done");
}

static void StartUnitTestThread() {

    le_result_t result;
    LE_INFO("***Start SAP Unit test***");
    TestSemaphoreRef = le_sem_Create("SAPSem", 0);

    //Test handler
    le_thread_Ref_t threadRef = le_thread_Create("taf_sap_test_thread", Test_taf_sap_AddHandler, NULL);
    le_thread_Start(threadRef);

    //Test connection
    Test_taf_sap_Conection();

    //Test ATR
    Test_taf_sap_ATR();

    //Test power down
    Test_taf_sap_PowerDown();

    //Test power ON
    Test_taf_sap_PowerOn();

    //Test APDU
    Test_taf_sap_TransferAPDU();

    //Test Card Reset
    Test_taf_sap_Reset();

    //Test Card Reader status
    Test_taf_sap_TransferCardReader();

    //Test status IND

    //Test disconnection
    Test_taf_sap_Disconnect();

    result = le_thread_Cancel(threadRef);
    LE_ASSERT(result == LE_OK);
    LE_INFO("***SAP Unit test done***");
}

COMPONENT_INIT
{
    // Start unit tests
    StartUnitTestThread();
}

