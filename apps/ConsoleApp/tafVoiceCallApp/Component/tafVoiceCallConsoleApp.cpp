/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <iostream>
#include <string.h>

#include "legato.h"
#include "interfaces.h"

#include "taf_voicecall_interface.h"
#include "taf_voicecall_service.h"

using namespace std;

#define MAX_DESTINATION_LEN_BYTE    51
#define MAX_DESTINATION_LEN         50

#define ESC "\033["
#define PURPLE_TXT "35"
#define RESET "\033[m"

//--------------------------
// Destination Phone number.
//--------------------------
static char DestinationNumber[TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES];

typedef struct
{
    le_sem_Ref_t                         semaphore;
    le_thread_Ref_t                      threadRef;
    uint8_t                              phoneId;
    char                                 destId[MAX_DESTINATION_LEN_BYTE];
    taf_voicecall_StateHandlerRef_t      stateHandlerRef;
    taf_voicecall_CallRef_t              requestRef;
    taf_voicecall_CallEndCause_t         appCause;
} UnitTestContext_t;

static UnitTestContext_t AppCtx;

static taf_voicecall_Event_t LocalExpectEvent;
static taf_voicecall_CallRef_t LocalExpectCallRef;

const char* return_val(taf_voicecall_CallEndCause_t result)
{
    const char* ret_val;
    switch (result)
    {
     case 0:
         ret_val = "END_NORMAL";
         break;
     case 1:
         ret_val = "END_NETWORK_FAIL";
         break;
     case 2:
         ret_val = "END_UNOBTAINABLE_NUMBER";
         break;
     case 3:
         ret_val = "END_BUSY";
         break;
     case 4:
         ret_val = "END_LOCAL";
         break;
     case 5:
         ret_val = "END_REMOTE";
         break;
     case 6:
         ret_val = "END_UNDEFINED";
         break;
     case 7:
         ret_val = "END_REJECTED";
         break;
     default:
         ret_val = "END_NORESPONSE";
         break;
    }
    return ret_val;
}

static void MyCallEventHandler(taf_voicecall_CallRef_t callRef, \
                               const char* id, \
                               taf_voicecall_Event_t callEvent, \
                               void* ctxPtr)
{
    LE_INFO("Client get event: callEvent %d, callRef: %p!\n", (uint32_t)callEvent, callRef);

    taf_voicecall_CallEndCause_t reason = (taf_voicecall_CallEndCause_t)TAF_VOICECALL_TERM_UNDEFINED;

    if (callEvent == TAF_VOICECALL_EVENT_ALERTING)
    {
        if (LocalExpectEvent != TAF_VOICECALL_EVENT_ALERTING)
            std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"Voice Call"<<RESET \
                     <<" - TAF_VOICECALL_EVENT_ALERTING"<<endl;
        LocalExpectEvent = callEvent;
    }
    else if (callEvent == TAF_VOICECALL_EVENT_ACTIVE)
    {
        if (LocalExpectEvent != TAF_VOICECALL_EVENT_ACTIVE)
            std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"Voice Call"<<RESET \
                     <<" - TAF_VOICECALL_EVENT_ACTIVE"<<endl;
        LocalExpectEvent = callEvent;
    }
    else if (callEvent == TAF_VOICECALL_EVENT_ENDED)
    {
        if (LocalExpectEvent != TAF_VOICECALL_EVENT_ENDED)
        {
            std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"Voice Call"<<RESET \
                     <<" - TAF_VOICECALL_EVENT_ENDED"<<endl;
            taf_voicecall_GetEndCause(callRef, &reason);
            std::cout<<endl<<ESC << PURPLE_TXT <<"m" \
                     <<"Termination reason is: "<<RESET<<return_val(reason)<<endl;
        }
        LocalExpectEvent = callEvent;
    }
    else if (callEvent == TAF_VOICECALL_EVENT_INCOMING)
    {
        if (LocalExpectEvent != TAF_VOICECALL_EVENT_INCOMING)
        {
            std::cout<<endl<<ESC << PURPLE_TXT <<"m" \
                     <<"Voice Call"<<RESET<<" - TAF_VOICECALL_EVENT_INCOMING"<<endl;
            std::cout<<"Enter 2 to Answer call"<<endl;
            std::cout<<"Enter 3 to Reject call"<<endl;
        }
        LocalExpectEvent = callEvent;
        LocalExpectCallRef = callRef;
    }
    else if (callEvent == TAF_VOICECALL_EVENT_ONHOLD)
    {
        if (LocalExpectEvent != TAF_VOICECALL_EVENT_ONHOLD)
            std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"Voice Call"<<RESET \
                     <<" - TAF_VOICECALL_EVENT_ONHOLD"<<endl;
        LocalExpectEvent = callEvent;
    }
    return;
}


static void* ut_tafVoiceCall_StateHandler(void* ctxPtr)
{
    taf_voicecall_StateHandlerRef_t callHandlerRef;
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;

    taf_voicecall_ConnectService();

    LE_INFO("ut_tafVoiceCall_StateHandler Called");

    callHandlerRef = taf_voicecall_AddStateHandler
                     (
                         (taf_voicecall_StateHandlerFunc_t)MyCallEventHandler,
                         NULL
                     );

    LE_ASSERT(callHandlerRef != NULL);

    appCtxPtr->stateHandlerRef = callHandlerRef;

    LE_INFO("ut_tafVoiceCall_StateHandler Success");

    le_event_RunLoop();
}

static le_result_t calling(void* ctxPtr, void* param)
{
    le_result_t  res = LE_OK;

    AppCtx.requestRef = taf_voicecall_Start(DestinationNumber, 1);

    std::cout<<ESC<<PURPLE_TXT<<"m"<<"Calling: "<<RESET<<DestinationNumber << endl;

    if (!AppCtx.requestRef)
    {
    std::cout<<endl<<"Voice Call Initialisation Failed"<<endl;
        return LE_FAULT;
    }
    return res;
}

static le_result_t answer_call(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_Answer(appCtxPtr->requestRef);

    if (leRet == LE_OK)
    {
        std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"taf_voicecall_Answer return value: "<<RESET \
                 <<" - LE_OK"<<endl;
    }
    else if (leRet == LE_NOT_FOUND)
    {
        std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"taf_voicecall_Answer return value: "<<RESET \
                 <<" - LE_NOT_FOUND"<<endl;
    }
    return leRet;
}

static le_result_t terminating(void* ctxPtr, void* param)
{
    le_result_t leRet;

    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;

    leRet = taf_voicecall_End(appCtxPtr->requestRef);

    if (leRet == LE_OK)
    {
        std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"taf_voicecall_End return value: "<<RESET \
                 <<" - LE_OK"<<endl;
    }
    else if (leRet == LE_NOT_FOUND)
    {
        std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"taf_voicecall_End return value: "<<RESET \
                 <<" - LE_NOT_FOUND"<<endl;
    }
    return leRet;
}

static le_result_t reject(void* ctxPtr, void* param)
{
    le_result_t leRet;

    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;

    leRet = taf_voicecall_End(appCtxPtr->requestRef);

    if (leRet == LE_OK)
    {
        std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"taf_voicecall_End return value: "<<RESET \
                 <<" - LE_OK"<<endl;
    }
    else if (leRet == LE_NOT_FOUND)
    {
        std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"taf_voicecall_End return value: "<<RESET \
                 <<" - LE_NOT_FOUND"<<endl;
    }
    return leRet;
}

static le_result_t call_on_hold(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_Hold(appCtxPtr->requestRef);

    if (leRet == LE_OK)
    {
        std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"taf_voicecall_Hold return value: "<<RESET \
                 <<" - LE_OK"<<endl;
    }
    else if (leRet == LE_NOT_FOUND)
    {
        std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"taf_voicecall_Hold return value: "<<RESET \
                 <<" - LE_NOT_FOUND"<<endl;
    }
    return leRet;
}

static le_result_t call_resume(void* ctxPtr, void* param)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;
    le_result_t leRet;

    leRet = taf_voicecall_Resume(appCtxPtr->requestRef);

    if (leRet == LE_OK)
    {
        std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"taf_voicecall_Resume return value: "<<RESET \
                 <<" - LE_OK"<<endl;
    }
    else if (leRet == LE_NOT_FOUND)
    {
        std::cout<<endl<<ESC << PURPLE_TXT <<"m"<<"taf_voicecall_Resume return value: "<<RESET \
                 <<" - LE_NOT_FOUND"<<endl;
    }
    return leRet;
}

static void register_call_handler(void)
{
    // handler test
    LE_INFO("===== Register state handler =====");

    if(AppCtx.threadRef == NULL)
    {
        AppCtx.threadRef = le_thread_Create(   "taf_voiceCall_handler",
                                               ut_tafVoiceCall_StateHandler,
                                               &AppCtx
                                           );
        le_thread_Start(AppCtx.threadRef);
    }
}

static void voicecall_start()
{
    std::cout<<"Voice Call Initialized"<<endl;

    le_event_QueueFunctionToThread(   AppCtx.threadRef,
                                      (le_event_DeferredFunc_t)calling,
                                      &AppCtx,
                                      NULL
                                  );
}

le_result_t IncomingCall()
{
    if(LocalExpectEvent != TAF_VOICECALL_EVENT_INCOMING)
    {
        std::cout<<"There is no incoming call"<<endl;
    }
    else
    {
        std::cout<<"Accepting IncomingCall"<<endl;
        AppCtx.requestRef = LocalExpectCallRef;
        le_event_QueueFunctionToThread(   AppCtx.threadRef,
                                          (le_event_DeferredFunc_t)answer_call,
                                          &AppCtx,
                                          NULL
                                      );
    }
    return LE_OK;
}

static void voicecall_end()
{
    std::cout<<"Voice Call Termination"<<endl;

    le_event_QueueFunctionToThread(   AppCtx.threadRef,
                                      (le_event_DeferredFunc_t)terminating,
                                      &AppCtx,
                                      NULL
                                  );
}

static void reject_voicecall()
{
    if(LocalExpectEvent != TAF_VOICECALL_EVENT_INCOMING)
    {
        std::cout<<"There is no incoming call"<<endl;
    }
    else
    {
        std::cout<<"Rejecting Voice Call"<<endl;
        AppCtx.requestRef = LocalExpectCallRef;
        le_event_QueueFunctionToThread(
                                          AppCtx.threadRef,
                                          (le_event_DeferredFunc_t)reject,
                                          &AppCtx,
                                          NULL
                                      );
    }
}

le_result_t on_Hold()
{
    std::cout<<"Putting Voice Call OnHold"<<endl;

    le_event_QueueFunctionToThread(   AppCtx.threadRef,
                                      (le_event_DeferredFunc_t)call_on_hold,
                                      &AppCtx,
                                      NULL
                                  );
    if(
         (TAF_VOICECALL_EVENT_ONHOLD == LocalExpectEvent) &&
         (LocalExpectCallRef == AppCtx.requestRef)
      )
    {
        std::cout<<"Voice Call Successfully put OnHold"<<endl;
    }
    return LE_OK;
}

le_result_t Resume()
{
    std::cout<<"Resuming Voice Call"<<endl;

    le_event_QueueFunctionToThread(   AppCtx.threadRef,
                                      (le_event_DeferredFunc_t)call_resume,
                                      &AppCtx,
                                      NULL
                                  );

    return LE_OK;
}

static void remove_call_handler(void* ctxPtr)
{
    UnitTestContext_t* appCtxPtr = (UnitTestContext_t*) ctxPtr;

    taf_voicecall_RemoveStateHandler(appCtxPtr->stateHandlerRef);
}

static void Remove_Voicecall_StateHandler()
{
    if(AppCtx.threadRef != NULL)
    {
        le_event_QueueFunctionToThread(   AppCtx.threadRef,
                                          (le_event_DeferredFunc_t)remove_call_handler,
                                          &AppCtx,
                                          NULL
                                      );
        AppCtx.threadRef = NULL;
    }
}

COMPONENT_INIT
{
    int index;
    char input_str[5];
    char *unused __attribute__((unused));

    register_call_handler();

    std::cout << endl;
    std::cout << "  1 - Dial" << endl;
    std::cout << "  2 - Accept_call" << endl;
    std::cout << "  3 - Reject_call" << endl;
    std::cout << "  4 - Hangup" << endl;
    std::cout << "  5 - Hold_call" << endl;
    std::cout << "  6 - Resume_call" << endl;
    std::cout << "  7 - Conference" << endl;
    std::cout << "  8 - Swap" << endl<<endl;
    std::cout << "  ? - help" << endl;
    std::cout << "  0 - exit" << endl << endl;
    std::cout << "TelAF > ";
    unused = fgets(input_str,sizeof(input_str),stdin);

    while(input_str[0]!='0')
    {
        if((input_str[0]=='1')&&(input_str[1]=='\n'))
        {
            string number;
            std::cout << endl;
            std::cout << "Dialer > ";
            std::cout << "Enter Number:";
            std::cin >> number;
            index=0;
            while(number.c_str()[index]!='\0')
            {
                DestinationNumber[index] = number.c_str()[index];
                index++;
            }
            DestinationNumber[index]='\0';
            voicecall_start();
            std::cout << endl;
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='4')
        {
            std::cout << endl;
            voicecall_end();
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='2')
        {
            std::cout << endl;
            IncomingCall();
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='3')
        {
            std::cout << endl;
            reject_voicecall();
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='5')
        {
            std::cout << endl;
            on_Hold();
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='6')
        {
            std::cout << endl;
            Resume();
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='7')
        {
            std::cout << endl;
            std::cout << "*********************************" << endl;
            std::cout << "Conference option is not supported !!!" << endl;
            std::cout << "*********************************" << endl;
            std::cout << endl;
            std::cout << "TelAF > ";
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]=='8')
        {
            std::cout << endl;
            std::cout << "********************************" << endl;
            std::cout << "Swap call feature is not supported !!!" << endl;
            std::cout << "********************************" << endl;
            std::cout << endl;
            std::cout << "TelAF > ";
            unused = fgets(input_str,sizeof(input_str),stdin);
        }
        else if(input_str[0]== '0')
        {
            Remove_Voicecall_StateHandler();
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
            std::cout << "  1 - Dial" << endl;
            std::cout << "  2 - Accept_call" << endl;
            std::cout << "  3 - Reject_call" << endl;
            std::cout << "  4 - Hangup" << endl;
            std::cout << "  5 - Hold_call" << endl;
            std::cout << "  6 - Resume_call" << endl;
            std::cout << "  7 - Conference" << endl;
            std::cout << "  8 - Swap" << endl<<endl;
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
