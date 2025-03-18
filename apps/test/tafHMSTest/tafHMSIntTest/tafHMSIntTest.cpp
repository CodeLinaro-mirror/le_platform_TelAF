/*
* Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "legato.h"
#include "interfaces.h"

taf_hms_ModemEvtHandlerRef_t modemStatusHandlerRef = NULL;

void HMPrintHelpMenu()
{
    puts(
        "NAME:\n"
        "app runProc tafHMSIntTest tafHMSIntTest - Health monitor Service Integration Test.\n"
        "\n"
        "SYNOPSIS:\n"
        "    app runProc tafHMSIntTest tafHMSIntTest -- help\n"
        "    app runProc tafHMSIntTest tafHMSIntTest -- ModemEventHandler 100\n"
        "\n"
        "DESCRIPTION:\n"
        "    app runProc tafHMSIntTest tafHMSIntTest -- help\n"
        "       Display this help and exit.\n"
        "\n"
        "    app runProc tafHMSIntTest tafHMSIntTest -- handler WaitSecs\n"
        "       Monitor modem reboot for 'WaitSecs' time."
        "       Will receive a notification when the modem reboots."
        "\n"
    );

    exit(EXIT_SUCCESS);
}

void CheckArgs(uint8_t argNum)
{
    if (le_arg_NumArgs() < argNum)
    {
        HMPrintHelpMenu();
    }
}

const char* ModemEventTypeToStr(taf_hms_ModemEvtType_t eventType)
{
    switch (eventType)
    {
        case TAF_HMS_MODEM_EVENT_TYPE_CONTINUE_REBOOT:
            return "CONTINUE_REBOOT";
    }
    return "UNKNOWN";
}

const char* ModemEventLevelToStr(taf_hms_ModemEvtSeverity_t eventLevel)
{
    switch (eventLevel)
    {
        case TAF_HMS_MODEM_EVENT_SEVERITY_LOW:
            return "LOW";
        case TAF_HMS_MODEM_EVENT_SEVERITY_MEDIUM:
            return "MEDIUM";
        case TAF_HMS_MODEM_EVENT_SEVERITY_HIGH:
            return "HIGH";
    }
    return "UNKNOWN";
}

void ModemStatusHandler(
    taf_hms_ModemEvtType_t eventType,
    taf_hms_ModemEvtSeverity_t eventLevel,
    taf_hms_ModemEventRef_t eventRef,
    void* contextPtr
)
{
    LE_INFO("Event Type: %s. Event Level: %s Event Reference: %p\n",
        ModemEventTypeToStr(eventType), ModemEventLevelToStr(eventLevel), eventRef);
    le_result_t result = taf_hms_ReleaseModemEvt(eventRef);
    LE_TEST_OK(result == LE_OK, "taf_hms_ReleaseModemEvt - LE_OK.");
}

void* ModemEventHandlerTestThread(void* contextPtr)
{
    taf_hms_ConnectService();
    modemStatusHandlerRef = 
        taf_hms_AddModemEvtHandler((taf_hms_ModemEvtHandlerFunc_t)ModemStatusHandler,NULL);
    LE_TEST_OK(modemStatusHandlerRef != NULL, "taf_hms_AddModemEvtHandler - OK");

    le_sem_Post((le_sem_Ref_t)contextPtr);
    le_event_RunLoop();
    return NULL;
}

void RemoveRefModemEvtHandler(void)
{
    LE_INFO("Removing modem event handler");
    taf_hms_RemoveModemEvtHandler(modemStatusHandlerRef);
    LE_TEST_OK(true, "taf_hms_RemoveModemEvtHandler - OK");
}

void ModemEventHandlerTest(void)
{
    CheckArgs(2);
    LE_TEST_INFO("======== Modem Event Handler Test ========\n");

    const char* arg2 = le_arg_GetArg(1);
    if (arg2 != NULL)
    {
        long time = strtol(arg2, NULL, 10);
        le_sem_Ref_t semaphore = le_sem_Create("ModemEventHandlerSemaphore", 0);
        le_thread_Ref_t threadRef = le_thread_Create("ModemEventHandlerTestThread",
        ModemEventHandlerTestThread, (void*)semaphore);
        le_thread_Start(threadRef);

        le_thread_Sleep(time);
        le_sem_Wait(semaphore);
        le_sem_Delete(semaphore);

        RemoveRefModemEvtHandler();
    }
}

COMPONENT_INIT
{
    LE_INFO("*** Checking for Console args ***");
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    CheckArgs(1);
    const char* cmd = le_arg_GetArg(0);
    if (cmd == NULL)
    {
        LE_ERROR("cmd is NULL");
        LE_TEST_EXIT;
    }

    LE_TEST_INFO("======== Health Monitor Service Integration Test %s ========", cmd);

    if (strncmp(cmd, "ModemEventHandler", strlen(cmd)) == 0)
    {
        ModemEventHandlerTest();
    }
    else
    {
        HMPrintHelpMenu();
    }
    LE_TEST_EXIT;
}
