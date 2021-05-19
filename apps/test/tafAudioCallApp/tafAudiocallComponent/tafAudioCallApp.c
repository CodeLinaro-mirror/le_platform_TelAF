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

static taf_voicecall_CallRef_t CallRef;

static taf_voicecall_StateHandlerRef_t VoiceCallHandlerRef;

static char DestNum[18];

static bool callStarted        = false;
static bool callIncoming    = false;
static bool callInProgress  = false;

static taf_audio_StreamRef_t mdmRxAudioRef = NULL;
static taf_audio_StreamRef_t mdmTxAudioRef = NULL;
static taf_audio_StreamRef_t speakerRef = NULL;
static taf_audio_StreamRef_t micRef = NULL;
static taf_audio_StreamRef_t inRef = NULL;
static taf_audio_StreamRef_t outRef = NULL;
static taf_audio_ConnectorRef_t   inputConnectorRef = NULL;
static taf_audio_ConnectorRef_t   outputConnectorRef = NULL;

static void DisconnectAudio
(
    taf_voicecall_CallRef_t reference
)
{
        LE_INFO("DisconnectAudio");
        if (inputConnectorRef)
        {
                if (micRef)
                {
                        taf_audio_Disconnect(inputConnectorRef, micRef);
                }
                if(inRef)
                {
                        taf_audio_Disconnect(inputConnectorRef, inRef);
                }
                if(mdmTxAudioRef)
                {
                        taf_audio_Disconnect(inputConnectorRef, mdmTxAudioRef);
                }
        }
        if(outputConnectorRef)
        {
                if(speakerRef)
                {
                        taf_audio_Disconnect(outputConnectorRef, speakerRef);
                }
                if(outRef)
                {
                        taf_audio_Disconnect(outputConnectorRef, outRef);
                }
                if(mdmRxAudioRef)
                {
                        taf_audio_Disconnect(outputConnectorRef, mdmRxAudioRef);
                }
        }

        if(inputConnectorRef)
        {
                taf_audio_DeleteConnector(inputConnectorRef);
                inputConnectorRef = NULL;
        }
        if(outputConnectorRef)
        {
                taf_audio_DeleteConnector(outputConnectorRef);
                outputConnectorRef = NULL;
        }
        if(mdmRxAudioRef)
        {
                taf_audio_Close(mdmRxAudioRef);
                outRef = NULL;
        }
        if(mdmTxAudioRef)
        {
                taf_audio_Close(mdmTxAudioRef);
                outRef = NULL;
        }
        if(speakerRef)
        {
                taf_audio_Close(speakerRef);
                speakerRef = NULL;
        }
        if(micRef)
        {
                taf_audio_Close(micRef);
                micRef = NULL;
        }
        if(inRef)
        {
                taf_audio_Close(inRef);
                inRef = NULL;
        }
        if(outRef)
        {
                taf_audio_Close(outRef);
                outRef = NULL;
        }
}

static le_result_t OpenAudioSpeaker
(
    taf_voicecall_CallRef_t reference
)
{
    le_result_t res;
    mdmRxAudioRef = taf_audio_OpenModemVoiceRx(1); // SlotId input param
    LE_ERROR_IF((mdmRxAudioRef==NULL), "taf_audio_OpenModemVoiceRx returns NULL!");
    LE_INFO("Connect Speaker");

    outRef = taf_audio_OpenSpeaker();
    LE_ERROR_IF((outRef==NULL), "taf_audio_OpenSpeaker returns NULL!");
    outputConnectorRef = taf_audio_CreateConnector();
    LE_ERROR_IF((outputConnectorRef==NULL), "outputConnectorRef is NULL!");

    if (mdmRxAudioRef && outRef && outputConnectorRef)
    {
        res = taf_audio_Connect(outputConnectorRef, outRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect RX on Output connector!");
        res = taf_audio_Connect(outputConnectorRef, mdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
    return LE_OK;
}

le_result_t OpenAudioMic
(
    taf_voicecall_CallRef_t reference
)
{
    le_result_t res;
    mdmTxAudioRef = taf_audio_OpenModemVoiceTx(1); //slotId 1 as input param
    LE_ERROR_IF((mdmTxAudioRef==NULL), "taf_audio_OpenModemVoiceTx returns NULL!");
    LE_INFO("Connect Mic");
    inRef = taf_audio_OpenMic();
    LE_ERROR_IF((inRef==NULL), "taf_audio_OpenMic returns NULL!");
    inputConnectorRef = taf_audio_CreateConnector();
    LE_ERROR_IF((inputConnectorRef==NULL), "inputConnectorRef is NULL!");
    if (mdmTxAudioRef && inRef && inputConnectorRef )
    {
        res = taf_audio_Connect(inputConnectorRef, inRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect TX on Input connector!");
        res = taf_audio_Connect(inputConnectorRef, mdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
    }
    return LE_OK;
}

static void CallEventHandler
(
    taf_voicecall_CallRef_t reference,
    const char* identifier,
    taf_voicecall_Event_t callEvent,
    void* contextPtr
    )
{
    taf_voicecall_CallEndCause_t term = TAF_VOICECALL_TERM_UNDEFINED;

    LE_INFO("New Call event: %d for Call %p, from %s", callEvent, reference, identifier);

    if (callEvent == TAF_VOICECALL_EVENT_ALERTING)
    {
        LE_INFO("TAF_VOICECALL_EVENT_ALERTING");
        OpenAudioSpeaker(reference);
        //OpenAudioMic(reference);
        LE_INFO("Destination phone is ringing...");
    }

    else if (callEvent == TAF_VOICECALL_EVENT_CONNECTED)
    {
        callIncoming = false;
        callInProgress = true;
        LE_INFO("TAF_VOICECALL_EVENT_CONNECTED");
        LE_INFO("You are now connected to %s", DestNum);

    }
    else if (callEvent == TAF_VOICECALL_EVENT_TERMINATED)
    {
        callStarted     = false;
        callIncoming    = false;
        callInProgress  = false;
        DisconnectAudio(reference);
        LE_INFO("TAF_VOICECALL_EVENT_TERMINATED");
        taf_voicecall_GetEndCause(reference, &term);
        switch(term)
        {
            case TAF_VOICECALL_TERM_NETWORK_FAIL:
            {
                LE_ERROR("TAF_VOICECALL_TERM_NETWORK_FAIL");
            }
            break;

            case TAF_VOICECALL_TERM_BAD_ADDRESS:
            {
                LE_ERROR("TAF_VOICECALL_TERM_BAD_ADDRESS");
            }
            break;

            case TAF_VOICECALL_TERM_BUSY:
            {
                LE_ERROR("TAF_VOICECALL_TERM_BUSY");
            }
            break;

                        case TAF_VOICECALL_TERM_LOCAL_ENDED:
            {
                LE_INFO("TAF_VOICECALL_TERM_LOCAL_ENDED");
            }
            break;

                        case TAF_VOICECALL_TERM_REMOTE_ENDED:
            {
                LE_INFO("TAF_VOICECALL_TERM_REMOTE_ENDED");
            }
            break;

            case TAF_VOICECALL_TERM_UNDEFINED:
            {
                LE_INFO("TAF_VOICECALL_TERM_UNDEFINED");
            }
            break;

            default:
            {
                LE_ERROR("Termination reason is %d", term);
            }
            break;
        }

        taf_voicecall_Delete(reference);
    }
    else if (callEvent == TAF_VOICECALL_EVENT_INCOMING)
    {
        LE_INFO("TAF_VOICECALL_EVENT_INCOMING");
        OpenAudioSpeaker(reference);
        //OpenAudioMic(reference);
        callIncoming = true;
        CallRef = reference;
    }
    else if (callEvent == TAF_VOICECALL_EVENT_CALL_END_FAILED)
    {
        LE_INFO("TAF_VOICECALL_EVENT_CALL_END_FAILED");
    }
    else if (callEvent == TAF_VOICECALL_EVENT_CALL_ANSWER_FAILED)
    {
        LE_INFO("TAF_VOICECALL_EVENT_CALL_ANSWER_FAILED");
    }
    else if (callEvent == TAF_VOICECALL_EVENT_OFFLINE)
    {
        LE_INFO("TAF_VOICECALL_EVENT_OFFLINE");
    }
    else if (callEvent == TAF_VOICECALL_EVENT_BUSY)
    {
        LE_INFO("TAF_VOICECALL_EVENT_BUSY");
    }
    else if (callEvent == TAF_VOICECALL_EVENT_RESOURCE_BUSY)
    {
        LE_INFO("TAF_VOICECALL_EVENT_RESOURCE_BUSY");
    }
    else
    {
        LE_ERROR("CallEventHandler failed, unknowm event %d.", callEvent);
    }
}

static le_result_t voicecall_start
(
    void
)
{
    le_result_t  res = LE_FAULT;
    taf_voicecall_CallEndCause_t reason = TAF_VOICECALL_TERM_UNDEFINED;

    CallRef = taf_voicecall_Start(DestNum,1);
    if (!CallRef)
    {
        res = taf_voicecall_GetEndCause(CallRef, &reason);
        LE_ASSERT(res == LE_OK);
        LE_INFO("Termination reason is: %d", reason);
        return LE_FAULT;
    }
    return LE_OK;
}

static le_result_t isValidNum
(
    const char* destNum
)
{
    if (NULL == destNum)
    {
        LE_ERROR("Dest number NULL");
        return LE_FAULT;
    }
    int i = 0;
    int numLength = strlen(destNum);
    if (numLength+1 > 18)
    {
        return LE_FAULT;
    }
    for (i = 0; i <= numLength-1; i++)
    {
        char dig = *destNum;
        if(!isdigit(dig))
        {
            LE_INFO("Should be Numeric %c", dig);
            return LE_FAULT;
        }
        destNum++;
    }
    return LE_OK;
}

le_result_t tafAudioCtrl_MakeCall
(
    const char * argPtr
)
{
    if (callStarted)
    {
        LE_INFO("Active voice call in progress. Please try again later.");
        return LE_NOT_POSSIBLE;
    }
    else
    {
        const char* destNum = argPtr;
        if(isValidNum(destNum) != LE_OK)
        {
            LE_INFO("Dest Num is not valid!");
            return LE_FAULT;
        }
        else
        {
            callStarted = true;
            le_utf8_Copy(DestNum, destNum, sizeof(DestNum), NULL);
            LE_INFO("Dest Num %s is valid.", DestNum);
            LE_ASSERT(voicecall_start() == LE_OK);
        }
    }
    return LE_OK;
}

le_result_t tafAudioCtrl_AnswerCall
(
    void
)
{
    le_result_t res = LE_OK;
    res = taf_voicecall_Answer(CallRef);
    if (res == LE_OK)
    {
        LE_INFO("Incoming call %s", DestNum);
    }
    else
    {
        LE_ERROR("No incoming call!");
    }
    return res;
}

le_result_t tafAudioCtrl_HangupCall
(
    void
    )
{
    le_result_t res = LE_OK;
    if (!callStarted && !callInProgress && !callIncoming)
    {
        LE_INFO("There is no voice call to end. ");
        return LE_FAULT;
    }
    else if (callIncoming)
    {
        callStarted = false;
        callIncoming = false;
        LE_INFO("Rejecting the incoming call!");
        taf_voicecall_Answer(CallRef);
        res = taf_voicecall_End(CallRef);
        if (res != LE_OK)
        {
            LE_INFO("Failed to end call.");
        }
    }
    else
    {
        callStarted = false;
        LE_INFO("Hanging up all calls!");
        res = taf_voicecall_End(CallRef);
        if (res != LE_OK)
        {
            LE_INFO("Failed to end call.");
        }
    }
    return res;
}

COMPONENT_INIT
{

    VoiceCallHandlerRef = taf_voicecall_AddStateHandler(CallEventHandler, NULL);

}
