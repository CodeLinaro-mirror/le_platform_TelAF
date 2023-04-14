/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of [Organization] nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <string>

#include "legato.h"
#include "interfaces.h"

using namespace std;

#define ESC "\033["
#define PURPLE_TXT "35"
#define RED_TXT "31"
#define RESET "\033[m"

//------------------------------------------------------------------
/**
* Destination Phone number.
*/
//------------------------------------------------------------------
static char DestinationNumber[TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES];

//------------------------------------------------------------------
/**
* SMSC number.
*/
//------------------------------------------------------------------
static char SMSCNumber[TAF_SMS_SMSC_ADDR_BYTES - 1];

//------------------------------------------------------------------
/**
 * Define test pattern
 */
//------------------------------------------------------------------
#define TEXT_PATTERN_EMPTY  ""
#define TEXT_PATTERN_TEST   "Message from tafSMSSvc console app."
#define BINARY_PATTERN      {0, 255}
#define UCS2_PATTERN        {0x2C6E, 0x668A}

#define PHONE_ID_PATTERN_1  1               // Phone ID to test

static uint8_t binary_pattern[2] = BINARY_PATTERN;
static uint16_t ucs2_pattern[2]  = UCS2_PATTERN;

typedef union {
    char     text[TAF_SMS_TEXT_BYTES];
    uint8_t  binary[TAF_SMS_BINARY_BYTES];
    uint8_t  pdu[TAF_SMS_PDU_BYTES];
    uint16_t ucs2[TAF_SMS_UCS2_CHARS];
}
RxSmsContent_t;

static le_sem_Ref_t sem_RxTest;

static taf_sms_RxMsgHandlerRef_t RxHandlerRef;

static le_thread_Ref_t RxThreadRef;
static le_thread_Ref_t TxThreadRef;

static uint8_t RxCount = 0;
static uint8_t TxCount = 0;

/*======================================================================

 FUNCTION        Test_taf_sms_SetGetPreferredStorage

 DESCRIPTION     Test setting/getting preferred storage of SMS service.

======================================================================*/

static void Test_taf_sms_SetGetPreferredStorage
(
    int index
)
{
    if(index==1)
    {
        LE_TEST_OK(taf_sms_SetPreferredStorage(TAF_SMS_STORAGE_SIM)==LE_OK,
                   "taf_sms_SetPreferredStorage - LE_OK");
    }
    else if(index==2)
    {
        LE_TEST_OK(taf_sms_SetPreferredStorage(TAF_SMS_STORAGE_HLOS)==LE_OK,
                   "taf_sms_SetPreferredStorage - LE_OK");
    }
}

/*======================================================================

 FUNCTION        Callback_MsgSendStatus

 DESCRIPTION     handler to get status of sending message

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

    le_result_t result;
    int* index = (int*) contextPtr;
    taf_sms_MsgRef_t tmpMsg_alphabet;
    taf_sms_MsgRef_t tmpMsg_binary;
    taf_sms_MsgRef_t tmpMsg_ucs2;

    if(*index ==1)
    {
        tmpMsg_alphabet = taf_sms_Create();

        LE_ASSERT(tmpMsg_alphabet);

        LE_TEST_OK(taf_sms_SetDestination(tmpMsg_alphabet, DestinationNumber) == LE_OK,
                   "taf_sms_SetDestination - LE_OK");

        LE_TEST_OK(taf_sms_SetCallback(tmpMsg_alphabet, Callback_MsgSendStatus, NULL) == LE_OK,
                   "taf_sms_SetCallback - LE_OK");

        LE_TEST_OK(taf_sms_SetText(tmpMsg_alphabet, TEXT_PATTERN_TEST) == LE_OK,
                   "taf_sms_SetText - LE_OK");

        LE_TEST_OK((result=taf_sms_Send(tmpMsg_alphabet))==LE_OK, "taf_sms_Send - LE_OK");

        if(result == LE_OK)
        {
            std::cout<<endl<<endl<<ESC << PURPLE_TXT <<"m"<<"SMS sent successfully."<<RESET<<endl;
        }
        else
        {
            std::cout<<endl<<endl<<ESC << RED_TXT <<"m"<<"SMS sending failed."<<RESET<<endl;
        }
    }
    else if(*index ==2)
    {
        tmpMsg_binary = taf_sms_Create();

        LE_ASSERT(tmpMsg_binary);

        LE_TEST_OK(taf_sms_SetDestination(tmpMsg_binary, DestinationNumber) == LE_OK,
                   "taf_sms_SetDestination - LE_OK");

        LE_TEST_OK(taf_sms_SetCallback(tmpMsg_binary, Callback_MsgSendStatus, NULL) == LE_OK,
                   "taf_sms_SetCallback - LE_OK");

        LE_TEST_OK(taf_sms_SetBinary(tmpMsg_binary, binary_pattern,sizeof(binary_pattern)) == LE_OK,
                   "taf_sms_SetBinary - LE_OK");

        LE_TEST_OK(taf_sms_Send(tmpMsg_binary) == LE_OK, "taf_sms_Send - LE_OK");
    }
    else if(*index ==3)
    {
        tmpMsg_ucs2 = taf_sms_Create();

        LE_ASSERT(tmpMsg_ucs2);

        LE_TEST_OK(taf_sms_SetDestination(tmpMsg_ucs2, DestinationNumber) == LE_OK,
                   "taf_sms_SetDestination - LE_OK");

        LE_TEST_OK(taf_sms_SetCallback(tmpMsg_ucs2, Callback_MsgSendStatus, NULL) == LE_OK,
                   "taf_sms_SetCallback - LE_OK");

        LE_TEST_OK(taf_sms_SetUCS2(tmpMsg_ucs2, ucs2_pattern, sizeof(ucs2_pattern)/sizeof(ucs2_pattern[0])) == LE_OK,
                   "taf_sms_SetUCS2 - LE_OK");

        LE_TEST_OK(taf_sms_Send(tmpMsg_ucs2) == LE_OK, "taf_sms_Send - LE_OK");
    }
    le_event_RunLoop();
}

/*======================================================================

 FUNCTION        Test_taf_sms_Send

 DESCRIPTION     Test send message APIs of SMS service.

======================================================================*/

static void Test_taf_sms_Send
(
    void* param
)
{
    int* index = (int*) param;
    TxThreadRef = le_thread_Create("SmsUnitTestTx", SmsTxThread, index);
    le_thread_Start(TxThreadRef);

    return;
}

/*======================================================================

 FUNCTION        RxHandler

 DESCRIPTION     Handle RX message

======================================================================*/

static void RxHandler
(
    taf_sms_MsgRef_t msgRef,
    void* context
)
{

    RxSmsContent_t rxContent;
    size_t len = 0;

    memset(rxContent.text, 0, TAF_SMS_TEXT_BYTES);

    LE_TEST_OK(taf_sms_GetType(msgRef) == TAF_SMS_TYPE_RX, "Received msg type is TAF_SMS_TYPE_RX");

    LE_TEST_OK(taf_sms_GetSenderTel(msgRef, rxContent.text, sizeof(rxContent.text)) == LE_OK,
                                    "taf_sms_GetSenderTel - LE_OK");

    LE_INFO("taf_sms_GetSenderTel = %s", rxContent.text);

    switch(taf_sms_GetFormat(msgRef))
    {
        case TAF_SMS_FORMAT_TEXT:
            LE_TEST_OK(taf_sms_GetText(msgRef, rxContent.text, sizeof(rxContent.text)) == LE_OK,
                       "taf_sms_GetText - LE_OK");
            LE_INFO("taf_sms_GetText = %s", rxContent.text);
            std::cout<<endl<<endl<<ESC << PURPLE_TXT <<"m"<<"Received message: "
                     <<RESET<<rxContent.text<<endl;
            break;

        case TAF_SMS_FORMAT_BINARY:
            len = sizeof(rxContent.binary);

            LE_TEST_OK(taf_sms_GetBinary(msgRef, rxContent.binary, &len) == LE_OK,
                       "taf_sms_GetBinary - LE_OK");

            LE_INFO("taf_sms_GetBinary, len:%d", len);
            for(size_t i = 0; i < len; i++)
            {
                LE_INFO("0x%.2X", rxContent.binary[i]);
            }
            break;

        case TAF_SMS_FORMAT_UCS2:

            len = sizeof(rxContent.ucs2);

            LE_TEST_OK(taf_sms_GetUCS2(msgRef, rxContent.ucs2, &len) == LE_OK,
                       "taf_sms_GetUCS2 - LE_OK");

            LE_INFO("taf_sms_GetUCS2, len:%d", len);
            for(size_t i = 0; i < len; i++)
            {
                LE_INFO("0x%.4X", rxContent.ucs2[i]);
            }
            break;

        default:
            break;
    }

    RxCount++;
}

static void* SmsRxHandlerThread
(
    void* contextPtr
)
{
    taf_sms_ConnectService();

    RxHandlerRef = taf_sms_AddRxMsgHandler(RxHandler, NULL);

    LE_TEST_OK(RxHandlerRef != NULL, "RxHandlerRef != NULL");

    le_event_RunLoop();
}

/*======================================================================

 FUNCTION        Test_taf_sms_RemoveRxHandler

 DESCRIPTION     Test removing RX handler API of SMS service.

======================================================================*/

static void Test_taf_sms_RemoveRxHandler
(
    void
)
{
    taf_sms_RemoveRxMsgHandler(RxHandlerRef);

    le_sem_Post(sem_RxTest);
}

/*======================================================================

 FUNCTION        Test_taf_sms_Receive

 DESCRIPTION     Test receive message APIs of SMS service.

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

 FUNCTION        Test_taf_sms_get_Smsc

 DESCRIPTION     Test get sms center address

======================================================================*/

static void Test_taf_sms_get_Smsc
(
    void
)
{
    char addr[TAF_SMS_SMSC_ADDR_BYTES - 1];
    size_t len = TAF_SMS_SMSC_ADDR_BYTES - 1;

    LE_TEST_OK(taf_sms_GetSmsCenterAddress(PHONE_ID_PATTERN_1, addr, len) == LE_OK,
               "taf_sms_GetSmsCenterAddress - LE_OK");

    std::cout<<ESC << PURPLE_TXT <<"m"<<"SMSC Address: "<<RESET;
    for (int i = 0; addr[i] != '\0' && addr[i] != ','; i++)
    {
        std::cout<<addr[i];
    }
    std::cout<<endl;

    return;
}

/*======================================================================

 FUNCTION        Test_taf_sms_set_Smsc

 DESCRIPTION     Test set sms center address

======================================================================*/

static void Test_taf_sms_set_Smsc
(
    void
)
{
    le_result_t Result;

    LE_TEST_OK((Result = taf_sms_SetSmsCenterAddress(PHONE_ID_PATTERN_1, SMSCNumber)) == LE_OK,
               "taf_sms_SetSmsCenterAddress - LE_OK");

    if(Result == LE_OK)
    {
        std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"SMSC is set successfully"<<RESET<<endl;
    }
    else
    {
        std::cout<<endl<<ESC << RED_TXT <<"m"<<"Setting SMSC is failed"<<RESET<<endl;
    }

    return;
}

/*======================================================================

 FUNCTION        Test_taf_sms_SetPhoneId

 DESCRIPTION     Test Set phone ID for message

======================================================================*/

static void Test_taf_sms_SetPhoneId
(
    void
)
{
    le_result_t Result;

    taf_sms_MsgRef_t tmpMsg;

    tmpMsg = taf_sms_Create();

    LE_TEST_OK((Result=taf_sms_SetPhoneId(tmpMsg, PHONE_ID_PATTERN_1)) == LE_OK,
               "taf_sms_SetPhoneId - LE_OK");

    if(Result == LE_OK)
    {
        std::cout<<ESC << PURPLE_TXT <<"m"<<"Phone ID is set successfully"<<RESET<<endl;
    }
    else
    {
        std::cout<<ESC << RED_TXT <<"m"<<"Setting phone ID failed"<<RESET<<endl;
    }

    return;
}

void copy_DestinationNumber(char const *ptr)
{
    int index=0;
    while(ptr[index]!='\0')
    {
        DestinationNumber[index] = ptr[index];
        index++;
    }
    DestinationNumber[index]='\0';
}
COMPONENT_INIT
{
    char input_str[5];
    char *unused __attribute__((unused));
    string number,storage_type;
    int index;

    LE_INFO("===== Test_taf_sms_Receive =====");
    Test_taf_sms_Receive();

    std::cout << endl;
    std::cout << "  1 - Send_TEXT_SMS" << endl;
    std::cout << "  2 - Send_Binary_SMS" << endl;
    std::cout << "  3 - Send_UCS2_SMS" << endl;
    std::cout << "  4 - Set_Phone_ID" << endl;
    std::cout << "  5 - Get_Preferred_storage" << endl;
    std::cout << "  6 - Set_Preferred_storage" << endl;
    std::cout << "  7 - Get_SMSC_address" << endl;
    std::cout << "  8 - Set_SMSC_address" << endl<<endl;
    std::cout << "  ? - help" << endl;
    std::cout << "  0 - exit" << endl << endl;
    std::cout << "TelAF > ";
    unused = fgets(input_str,sizeof(input_str),stdin);

    while(input_str[0]!='0')
    {
        if((input_str[0]=='1')&&(input_str[1]=='\n'))
        {
            std::cout << endl;
            std::cout << "SMS > ";
            std::cout << "Enter Number:";
            std::cin >> number;
            copy_DestinationNumber(&number.c_str()[0]);
            index = 1;
            Test_taf_sms_Send(&index);
            std::cout << endl;
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='2')
        {
            std::cout << endl;
            std::cout << "SMS > ";
            std::cout << "Enter Number:";
            std::cin >> number;
            copy_DestinationNumber(&number.c_str()[0]);
            index = 2;
            Test_taf_sms_Send(&index);
            std::cout << endl;
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='3')
        {
            std::cout << endl;
            std::cout << "SMS > ";
            std::cout << "Enter Number:";
            std::cin >> number;
            copy_DestinationNumber(&number.c_str()[0]);
            index = 3;
            Test_taf_sms_Send(&index);
            std::cout << endl;
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='4')
        {
            std::cout << endl;
            Test_taf_sms_SetPhoneId();
            std::cout << endl;
            std::cout << "TelAF > ";
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='5')
        {
            taf_sms_Storage_t prefStorage = TAF_SMS_STORAGE_UNKNOWN;
            LE_TEST_OK(taf_sms_GetPreferredStorage(&prefStorage)==LE_OK,
                       "taf_sms_GetPreferredStorage - LE_OK");
            if(prefStorage==TAF_SMS_STORAGE_SIM)
            {
                std::cout<<endl<<ESC << PURPLE_TXT <<"m"
                         <<"Storage type: "<<RESET<<"STORAGE_SIM"<<endl;
            }
            else if(prefStorage==TAF_SMS_STORAGE_HLOS)
            {
                std::cout<<endl<<ESC << PURPLE_TXT <<"m"
                         <<"Storage type: "<<RESET<<"STORAGE_HLOS"<<endl;
            }
            else if(prefStorage==TAF_SMS_STORAGE_NV)
            {
                std::cout<<endl<<ESC << PURPLE_TXT <<"m"
                         <<"Storage type: "<<RESET<<"STORAGE_NV"<<endl;
            }
            else
            {
                std::cout<<endl<<ESC << PURPLE_TXT <<"m"
                         <<"Storage type: "<<RESET<<"STORAGE_UNKNOWN"<<endl;
            }
            std::cout << endl;
            std::cout << "TelAF > ";
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='6')
        {
            std::cout << endl;
            std::cout <<ESC << PURPLE_TXT <<"m"<< "Select 1 for STORAGE_SIM";
            std::cout <<endl<< "Select 2 for STORAGE_HLOS"<<endl<<endl;
            std::cout << "Enter Number: "<<RESET;
            std::cin >> storage_type;
            std::cout << endl;
            if(storage_type[0]=='1')
            {
                std::cout <<"Selected 1 - STORAGE_SIM"<<endl<<endl;
                Test_taf_sms_SetGetPreferredStorage(1);
            }
            else if(storage_type[0]=='2')
            {
                std::cout <<"Selected 2 - STORAGE_HLOS"<<endl<<endl;
                Test_taf_sms_SetGetPreferredStorage(2);
            }
            else
            {
                std::cout <<"Invalid Input"<<endl<<endl;
            }
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='7')
        {
            std::cout << endl;
            Test_taf_sms_get_Smsc();
            std::cout << endl;
            std::cout << "TelAF > ";
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='8')
        {
            std::cout << endl;
            std::cout << "SMSC > ";
            std::cout << "Enter Number:";
            std::cin >> number;
            SMSCNumber[0]='"';
            SMSCNumber[1]='+';
            index=1;
            while(number.c_str()[index-1]!='\0')
            {
                SMSCNumber[index+1]=number.c_str()[index-1];
                index++;
            }
            SMSCNumber[index+1]='"';
            SMSCNumber[index+2]='\0';
            Test_taf_sms_set_Smsc();
            std::cout << endl;
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]== '0')
        {
            le_event_QueueFunctionToThread( RxThreadRef,
                                            (le_event_DeferredFunc_t)Test_taf_sms_RemoveRxHandler,
                                            NULL, NULL
                                          );
            le_sem_Wait(sem_RxTest);
            exit(0);
        }
        else if(input_str[0]=='\n')
        {
            std::cout << "TelAF > ";
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='?')
        {
            std::cout << endl;
            std::cout << "  1 - Send_TEXT_SMS" << endl;
            std::cout << "  2 - Send_Binary_SMS" << endl;
            std::cout << "  3 - Send_UCS2_SMS" << endl;
            std::cout << "  4 - Set_Phone_ID" << endl;
            std::cout << "  5 - Get_Preferred_storage" << endl;
            std::cout << "  6 - Set_Preferred_storage" << endl;
            std::cout << "  7 - Get_SMSC_address" << endl;
            std::cout << "  8 - Set_SMSC_address" << endl<<endl;
            std::cout << "  ? - help" << endl;
            std::cout << "  0 - exit" << endl << endl;
            std::cout << "TelAF > ";
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else{
            std::cout << endl;
            std::cout << "  Invalid Input" << endl << endl;
            std::cout << "TelAF > ";
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
    }
    exit(0);
}
