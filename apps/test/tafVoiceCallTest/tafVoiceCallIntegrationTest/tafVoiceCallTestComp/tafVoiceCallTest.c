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

//--------------------------------------------------------------------------------------------------
/**
 * Necessary Static Variables.
 */
//--------------------------------------------------------------------------------------------------
static taf_voicecall_CallRef_t TafCallRef;


static taf_voicecall_StateHandlerRef_t TafHandlerRef;


static le_timer_Ref_t TafHoldTimer;

//--------------------------------------------------------------------------------------------------
/**
 * Start Call Timer Reference.
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t StartCallTimer;


//--------------------------------------------------------------------------------------------------
/**
 * Destination Number(NOT SUPPORT DSDA).
 */
//--------------------------------------------------------------------------------------------------
static char  DestinationNum[TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES+1];

//--------------------------------------------------------------------------------------------------
/**
 * Initiate call or wait for incoming call
 */
//--------------------------------------------------------------------------------------------------
static bool initiateFlag = true;

//--------------------------------------------------------------------------------------------------
/**
 * Test count/counter
 */
//--------------------------------------------------------------------------------------------------
static int maxTestCount = 1;
static int testCounter = 0;


#ifdef TAF_AUDIO_ENABLE
//--------------------------------------------------------------------------------------------------
/**
 * Close Audio(NOT SUPPORT).
 *
 */
//--------------------------------------------------------------------------------------------------
static void DisconnectAudioStream
(
    taf_voicecall_CallRef_t reference
)
{
    LE_ERROR("NOT SUPPORT!");
}


//--------------------------------------------------------------------------------------------------
/**
 * Open Audio(NOT SUPPORT).
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ConnectAudioStream
(
    taf_voicecall_CallRef_t reference
)
{
    LE_ERROR("NOT SUPPORT!");

    return LE_OK;
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Test: start the voice call, using slot 1.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Test_taf_voicecall_start
(
    void
)
{
    le_result_t  res = LE_FAULT;
    taf_voicecall_CallEndCause_t reason;

    LE_INFO("Asking server to  call taf_voicecall_Start");
    TafCallRef = taf_voicecall_Start(DestinationNum, 1);
    testCounter++;

    if (!TafCallRef)
    {
       res = taf_voicecall_GetEndCause(TafCallRef, &reason);
       LE_ASSERT(res == LE_OK);
       LE_INFO("taf_voicecall_GetEndCause %d", reason);
       return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Hold Call Timer handler.
 */
//--------------------------------------------------------------------------------------------------
static void TafHoldTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("End the call after holding for a while!");
    LE_ERROR_IF(taf_voicecall_End(TafCallRef) != LE_OK, "Could not end the call.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Start Call Timer handler
 */
//--------------------------------------------------------------------------------------------------
static void StartCallTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("Start the call");
    LE_ASSERT(Test_taf_voicecall_start() == LE_OK);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Call Events Handler
 */
//--------------------------------------------------------------------------------------------------
static void AppCallHandler
(
    taf_voicecall_CallRef_t callRef, \
    const char* id,
    taf_voicecall_Event_t callEvent,\
    void* ctxPtr
)
{
    le_result_t res;

    LE_INFO("Voice Call TEST: New Call event: %d for Call %p, from %s", callEvent, callRef, id);
    if (callEvent == TAF_VOICECALL_EVENT_ACTIVE)
    {
        LE_INFO("TEST_VOICECALL_CLIENT: TAF_VOICECALL_EVENT_ACTIVE.");
        LE_INFO("Start TafHoldTimer to hold the call");
        LE_ASSERT(le_timer_Start(TafHoldTimer) == LE_OK);
    }
    else if (callEvent == TAF_VOICECALL_EVENT_ONHOLD)
    {
        LE_INFO("TEST_VOICECALL_CLIENT: TAF_VOICECALL_EVENT_ONHOLD.");
    }
    else if (callEvent == TAF_VOICECALL_EVENT_DIALING)
    {
        LE_INFO("TEST_VOICECALL_CLIENT: TAF_VOICECALL_EVENT_DIALING.");
    }
    else if (callEvent == TAF_VOICECALL_EVENT_INCOMING)
    {
        LE_INFO("TEST_VOICECALL_CLIENT: TAF_VOICECALL_EVENT_INCOMING.");
        res = taf_voicecall_Answer(callRef);
        if (res == LE_OK)
        {
            TafCallRef = callRef;
            LE_INFO("TEST_VOICECALL_CLIENT: answer the call ok");
        }
        else
        {
            LE_ERROR("TEST_VOICECALL_CLIENT: answer the call failed.");
        }
    }
    else if (callEvent == TAF_VOICECALL_EVENT_WAITING)
    {
        LE_INFO("TEST_VOICECALL_CLIENT: TAF_VOICECALL_EVENT_WAITING.");
    }
    else if (callEvent == TAF_VOICECALL_EVENT_ALERTING)
    {
        LE_INFO("TEST_VOICECALL_CLIENT: TAF_VOICECALL_EVENT_ALERTING. PICK UP THE CALL!!!");
    }
    else if (callEvent == TAF_VOICECALL_EVENT_ENDED)
    {
        LE_INFO("TEST_VOICECALL_CLIENT: TAF_VOICECALL_EVENT_ENDED.");
        res = taf_voicecall_Delete(callRef);

        LE_INFO("testCounter: %d", testCounter);
        if(initiateFlag)
        {
            if(testCounter < maxTestCount)
            {
                LE_INFO("Keep testing new call");
                LE_INFO("Start Call after 10 seconds");
                LE_ASSERT(le_timer_Start(StartCallTimer) == LE_OK);
            }
            else
            {
                taf_voicecall_RemoveStateHandler(TafHandlerRef);
            }
        }
    }
    else
    {
        LE_ERROR("Unknown call event %d.", callEvent);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/*
 * Start the app : app start tafVoiceCallTest
 * Execute the app : app runProc tafVoiceCallTest --exe=tafVoiceCallTest -- <Destination phone number>
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    int nbArgument = le_arg_NumArgs();

    LE_INFO("Starting tafVoiceCallTest, nbArgument: %d", nbArgument);

    if (nbArgument >= 1)
    {
        // Check whether to initiate a call or wait for an incoming call
        const char* initiateCallPtr = le_arg_GetArg(0);
        if (NULL == initiateCallPtr)
        {
            LE_ERROR("Options missing");
            exit(EXIT_FAILURE);
        }
        initiateFlag = strtol(initiateCallPtr, NULL, 0);

        if (nbArgument >= 2)
        {
            // Get the telephone number from the User.
            const char* phoneNumber = le_arg_GetArg(1);
            if (NULL == phoneNumber)
            {
                LE_ERROR("phoneNumber is NULL");
                exit(EXIT_FAILURE);
            }
            le_utf8_Copy(DestinationNum, phoneNumber, TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES, NULL);
            LE_INFO("Phone number %s", DestinationNum);
        }

        if (nbArgument >= 3)
        {
            const char* testCountPtr = le_arg_GetArg(2);
            if (NULL == testCountPtr)
            {
                LE_ERROR("Options missing");
                exit(EXIT_FAILURE);
            }
            maxTestCount = strtol(testCountPtr, NULL, 0);
            LE_INFO("maxTestCount %d", maxTestCount);
        }

        TafHoldTimer = le_timer_Create("holdthecall");
        LE_ASSERT(TafHoldTimer);
        static le_clk_Time_t HangUpInterval = {5, 0};
        LE_ASSERT(le_timer_SetInterval(TafHoldTimer, HangUpInterval) == LE_OK);
        LE_ASSERT(le_timer_SetHandler(TafHoldTimer, TafHoldTimerHandler) == LE_OK);

        StartCallTimer = le_timer_Create("startthecall");
        LE_ASSERT(StartCallTimer);
        static le_clk_Time_t StartCallInterval = {10, 0};
        LE_ASSERT(le_timer_SetInterval(StartCallTimer, StartCallInterval) == LE_OK);
        LE_ASSERT(le_timer_SetHandler(StartCallTimer, StartCallTimerHandler) == LE_OK);

        TafHandlerRef = taf_voicecall_AddStateHandler((taf_voicecall_StateHandlerFunc_t)AppCallHandler, NULL);
        if (initiateFlag)
        {
            LE_ASSERT(Test_taf_voicecall_start() == LE_OK);
        }
    }
    else
    {
        LE_ERROR("cmd: app runProc tafVoiceCallTest --exe=tafVoiceCallTest -- <initiate a call> <phone number> <test count>");
        exit(EXIT_SUCCESS);
    }
}
