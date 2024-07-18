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

#define EVENTS_POOL_SIZE   2
static le_sem_Ref_t TestSemaphoreRef;
static taf_simRsim_MessageHandlerRef_t  MsgHandlerRef;
static le_thread_Ref_t ThreadRef;
static le_mem_PoolRef_t RsimMsgsPool;
static uint8_t ExpectedMsg[TAF_SIMRSIM_MAX_MSG_SIZE] = {0};
static size_t ExpectedMsgSize = 0;
static uint8_t ConnectReqMsg[12] =
{ 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x14, 0x00, 0x00};
static uint8_t ConnectReqLength = 12;
static uint8_t ConnectOkRespMsg[12] =
{ 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};

static uint8_t PowerDownReq[4] = {0x09, 0x00, 0x00, 0x00};
static uint8_t PowerDownReqLength = 4;

static uint8_t PowerDownResp[12] =
{0x0A, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
static uint8_t PowerDownRespLength = 12;
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


static uint8_t APDUReq[16] = {0x05, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x07,
                            0x00, 0xA4, 0x00, 0x04, 0x02, 0x3F, 0x00, 0x00};
static uint8_t APDUReqLength = 16;
static uint8_t APDUResp[60] = { 0x06, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                              0x04, 0x00, 0x00, 0x29, 0x62, 0x25, 0x82, 0x02, 0x78, 0x21, 0x83, 0x02,
                              0x3F, 0x00, 0xA5, 0x0B, 0x80, 0x01, 0x71, 0x83, 0x03, 0x07, 0x93, 0x81,
                              0x87, 0x01, 0x01, 0x8A, 0x01, 0x05, 0x8B, 0x03, 0x2F, 0x06, 0x02, 0xC6,
                              0x06, 0x90, 0x01, 0x00, 0x83, 0x01, 0x01, 0x90, 0x00, 0x00, 0x00, 0x00 };
static uint8_t APDURespLength = 60;

static uint8_t DisconnectReq[4] = {0x02, 0x00, 0x00, 0x00};
static uint8_t DisconnectReqLength = 4;
static uint8_t DisconnectInd[] = {0x04, 0x01, 0x00, 0x00,0x03, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 };
static uint8_t DisconnectResp[] = {0x03, 0x00, 0x00, 0x00};
static uint8_t DisconnectRespLength = 4;

typedef struct
{
    uint8_t msg[TAF_SIMRSIM_MAX_MSG_SIZE];
    size_t  msgLength;
    taf_simRsim_CallbackHandlerFunc_t callback;
    void*   contextPtr;
}
RsimMsg_t;

static void SAPMessageHandler(const uint8_t* msgPtr,
                        size_t msgSize, void* contextPtr) {
    int i = 0;
    LE_INFO("Received SAP message");
    LE_DUMP(msgPtr, msgSize);
    LE_INFO("Expected SAP message");
    LE_DUMP(ExpectedMsg, ExpectedMsgSize);

    LE_ASSERT(msgSize == ExpectedMsgSize);

    for(i = 0; i < msgSize; i++) {
        LE_ASSERT(msgPtr[i] == ExpectedMsg[i]);
    }
    le_sem_Post(TestSemaphoreRef);
}

static void* Test_taf_simRsim_AddHandler(void* context) {

    taf_simRsim_ConnectService();

    MsgHandlerRef = taf_simRsim_AddMessageHandler(SAPMessageHandler, NULL);
    LE_ASSERT(MsgHandlerRef != NULL);
    LE_INFO("Message Handler Added successfully MsgHandlerRef = %p", MsgHandlerRef);

    le_event_RunLoop();
    return NULL;
}

static void Test_taf_simRsim_RemoveHandler(void* param1, void* param2) {

    LE_INFO("Remove Message Handler MsgHandlerRef = %p", MsgHandlerRef);
    taf_simRsim_RemoveMessageHandler(MsgHandlerRef);
    LE_ASSERT(MsgHandlerRef != NULL);

    le_sem_Post(TestSemaphoreRef);
}


static void CallbackHandler( uint8_t messageId, le_result_t result, void* contextPtr) {
    LE_DEBUG("Sending result: messageId=%d, result=%d, context=%p", messageId, result, contextPtr);
    LE_ASSERT(result == (le_result_t)contextPtr)
    le_sem_Post(TestSemaphoreRef);
}

static void SendMessage(void * paramPtr1,void * paramPtr2 ) {

    RsimMsg_t* messagePtr = (RsimMsg_t*) paramPtr1;
    le_result_t result = taf_simRsim_SendMessage(messagePtr->msg,
                                messagePtr->msgLength, messagePtr->callback,
                                messagePtr->contextPtr);
    LE_ASSERT(result == LE_OK);
    le_mem_Release(messagePtr);
}

static void Test_taf_simRsim_ConnectPowerOffON() {
    memset(ExpectedMsg, 0, sizeof(ExpectedMsg));
    memcpy(ExpectedMsg, PowerDownReq, PowerDownReqLength);
    ExpectedMsgSize = PowerDownReqLength;

    RsimMsg_t* rsimPtr = le_mem_ForceAlloc(RsimMsgsPool);
    memcpy(rsimPtr->msg ,ConnectOkRespMsg, 12);
    rsimPtr->msgLength = 12;
    rsimPtr->callback = CallbackHandler;
    rsimPtr->contextPtr = NULL;


    le_event_QueueFunctionToThread(ThreadRef, SendMessage, rsimPtr, NULL);

    le_sem_Wait(TestSemaphoreRef);
    le_sem_Wait(TestSemaphoreRef);


    memset(ExpectedMsg, 0, sizeof(ExpectedMsg));
    memcpy(ExpectedMsg, PowerOnReq, PowerOnReqLength);
    ExpectedMsgSize = PowerOnReqLength;
    RsimMsg_t* rsimPtr1 = le_mem_ForceAlloc(RsimMsgsPool);
    memcpy(rsimPtr1->msg ,PowerDownResp, PowerDownRespLength);
    rsimPtr1->msgLength = PowerDownRespLength;
    rsimPtr1->callback = CallbackHandler;
    rsimPtr1->contextPtr = NULL;
    le_event_QueueFunctionToThread(ThreadRef, SendMessage, rsimPtr1, NULL);
    le_sem_Wait(TestSemaphoreRef);
}

static void Test_taf_simRsim_ATR(void) {
    memset(ExpectedMsg, 0, sizeof(ExpectedMsg));
    memcpy(ExpectedMsg, ATRReqMsg, ATRReqLength);
    ExpectedMsgSize = ATRReqLength;
    RsimMsg_t* rsimPtr1 = le_mem_ForceAlloc(RsimMsgsPool);
    memcpy(rsimPtr1->msg ,PowerOnResp, PowerOnRespLength);
    rsimPtr1->msgLength = PowerOnRespLength;
    rsimPtr1->callback = CallbackHandler;
    rsimPtr1->contextPtr = NULL;
    le_event_QueueFunctionToThread(ThreadRef, SendMessage, rsimPtr1, NULL);

    le_sem_Wait(TestSemaphoreRef);
    le_sem_Wait(TestSemaphoreRef);

    memset(ExpectedMsg, 0, sizeof(ExpectedMsg));
    memcpy(ExpectedMsg, APDUReq, APDUReqLength);
    ExpectedMsgSize = APDUReqLength;

    RsimMsg_t* rsimPtr = le_mem_ForceAlloc(RsimMsgsPool);
    memcpy(rsimPtr->msg ,ATRRespMsg, ATRRespLength);
    rsimPtr->msgLength = ATRRespLength;
    rsimPtr->callback = CallbackHandler;
    rsimPtr->contextPtr = NULL;


    le_event_QueueFunctionToThread(ThreadRef, SendMessage, rsimPtr, NULL);
    le_sem_Wait(TestSemaphoreRef);
}

static void Test_taf_simRsim_APDU(void) {
    RsimMsg_t* rsimPtr1 = le_mem_ForceAlloc(RsimMsgsPool);
    memcpy(rsimPtr1->msg ,APDUResp, APDURespLength);
    rsimPtr1->msgLength = APDURespLength;
    rsimPtr1->callback = CallbackHandler;
    rsimPtr1->contextPtr = NULL;
    le_event_QueueFunctionToThread(ThreadRef, SendMessage, rsimPtr1, NULL);
    le_sem_Wait(TestSemaphoreRef);
}

static void Test_taf_simRsim_Disconnect(void) {
    memset(ExpectedMsg, 0, sizeof(ExpectedMsg));
    memcpy(ExpectedMsg, DisconnectReq, DisconnectReqLength);
    ExpectedMsgSize = DisconnectReqLength;
    RsimMsg_t* rsimPtr1 = le_mem_ForceAlloc(RsimMsgsPool);
    memcpy(rsimPtr1->msg ,DisconnectInd, 12);
    rsimPtr1->msgLength = 12;
    rsimPtr1->callback = CallbackHandler;
    rsimPtr1->contextPtr = NULL;
    le_event_QueueFunctionToThread(ThreadRef, SendMessage, rsimPtr1, NULL);
    le_sem_Wait(TestSemaphoreRef);
    le_sem_Wait(TestSemaphoreRef);

    RsimMsg_t* rsimPtr = le_mem_ForceAlloc(RsimMsgsPool);
    memcpy(rsimPtr->msg ,DisconnectResp, DisconnectRespLength);
    rsimPtr->msgLength = DisconnectRespLength;
    rsimPtr->callback = CallbackHandler;
    rsimPtr->contextPtr = NULL;
    le_event_QueueFunctionToThread(ThreadRef, SendMessage, rsimPtr, NULL);
}

static void StartUnitTestThread(void) {

    le_result_t result;
    LE_INFO("***Start RSIM Unit test***");
    TestSemaphoreRef = le_sem_Create("SAPSem", 0);
    memset(ExpectedMsg, 0, sizeof(ExpectedMsg));
    memcpy(ExpectedMsg, ConnectReqMsg, ConnectReqLength);
    ExpectedMsgSize = ConnectReqLength;

    ThreadRef = le_thread_Create("taf_simRsim_test_thread", Test_taf_simRsim_AddHandler, NULL);
    le_thread_Start(ThreadRef);

    le_sem_Wait(TestSemaphoreRef);

    Test_taf_simRsim_ConnectPowerOffON();
    le_sem_Wait(TestSemaphoreRef);

    Test_taf_simRsim_ATR();
    le_sem_Wait(TestSemaphoreRef);

    Test_taf_simRsim_APDU();

    Test_taf_simRsim_Disconnect();

    le_event_QueueFunctionToThread(ThreadRef, Test_taf_simRsim_RemoveHandler, NULL, NULL);
    LE_INFO("***RSIM Unit test done***");

    result = le_thread_Cancel(ThreadRef);
    LE_ASSERT(result == LE_OK);

    le_sem_Delete(TestSemaphoreRef);
}

COMPONENT_INIT
{
    RsimMsgsPool = le_mem_CreatePool("MessagePool", sizeof(RsimMsg_t));
    le_mem_ExpandPool(RsimMsgsPool, EVENTS_POOL_SIZE);

    StartUnitTestThread();

    exit(EXIT_SUCCESS);
}

