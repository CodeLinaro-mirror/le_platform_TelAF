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

static const char* ComponentName;

static void PrintUsage
(
)
{
    puts(
        "\n"
        "NAME:\n"
        "    audiocall - Used to perform audiocall operations.\n"
        "\n"
        "PREREQUISITES:\n"
        "    SIM is inserted, registered on the network, and is in ready state.\n"
        "\n"
        "    tafAudioCallService and tafVoicecallService is running.\n"
        "\n"
        "DESCRIPTION:\n"
        "    audiocall call <Destination Number>\n"
        "       Initiates a audiocall call to <Destination Number>.  <Destination Number> is assumed to be valid\n"
        "\n"
        "    audiocall answer\n"
        "       Answers an incoming audiocall call. TAF_VOICECALL_EVENT_INCOMING indicates that there is an incoming call.\n"
        "\n"
        "    audiocall hangup\n"
        "       Ends an active voice call. If there is an incoming call, it rejects the call. If a number is being\n"
        "       dialed, it ends the outgoing call.\n"
        "\n"
        );

    exit(EXIT_SUCCESS);
}

static void Start
(
    const char* callPtr
)
{
    tafAudioCtrl_MakeCall(callPtr);
}

static void CallHandler
(
    const char* callPtr
)
{
    if (strcmp(callPtr, "call") == 0 && le_arg_NumArgs() == 2)
    {
        le_arg_AddPositionalCallback(Start);
    }
    else if (strcmp(callPtr, "answer") == 0 && le_arg_NumArgs() == 1)
    {
        tafAudioCtrl_AnswerCall();
    }
    else if (strcmp(callPtr, "hangup") == 0 && le_arg_NumArgs() == 1)
    {
        tafAudioCtrl_HangupCall();
    }
    else
    {
        fprintf(stderr, "Unknown command.\n");
        fprintf(stderr, "Try '%s --help'.\n", ComponentName);
        exit(EXIT_FAILURE);
    }
}

COMPONENT_INIT
{
    ComponentName = le_arg_GetProgramName();
    if (ComponentName == NULL)
    {
        ComponentName = "audiocall";
    }

    le_arg_SetFlagCallback(PrintUsage, "h", "help");
    le_arg_AddPositionalCallback(CallHandler);

    le_arg_Scan();

    exit(EXIT_SUCCESS);
}
