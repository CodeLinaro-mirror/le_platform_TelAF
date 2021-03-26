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
#include "main.h"

static taf_sim_NewStateHandlerRef_t NewSimStateHandlerRef = NULL;

static void DisplayAppUsage(void) {
     printf("Usage of the 'tafsimTest' application is:\n");
     printf("Test SIM state: app runProc tafSimTest --exe=tafSimTest -- state <slot1/slot2/unknown>\n");
     printf("Test SIM state change: app runProc tafSimTest --exe=tafSimTest -- events\n");
}

static taf_sim_Id_t GetSimId(const char* simIdPtr) {
    if(strcmp(simIdPtr, "slot1") == 0) {
        return TAF_SIM_EXTERNAL_SLOT_1;
    } else if(strcmp(simIdPtr, "slot2") == 0) {
        return TAF_SIM_EXTERNAL_SLOT_2;
    } else if(strcmp(simIdPtr, "unknown") == 0) {
        return TAF_SIM_UNSPECIFIED;
    }
    LE_ERROR("Unable to convert '%s' to a taf_sim_Id_t", simIdPtr);
    DisplayAppUsage();
    exit(EXIT_FAILURE);
}

static void TestNewSimStateHandler(taf_sim_Id_t simId, taf_sim_States_t simState,
        void* contextPtr){
    LE_INFO("New SIM event for SIM card: %d", simId);
    LE_INFO("SIM state: %s", SimStateToString(simState));
}

COMPONENT_INIT
{
    taf_sim_Id_t simId = 0;
    bool exitApplication = true;
    const char* testType = "";

    LE_INFO("Start tafSimTest app.");
    int NumberOfArgs = le_arg_NumArgs();

    if (NumberOfArgs >= 1) {
        testType = le_arg_GetArg(0);
        if (NULL == testType) {
            LE_ERROR("testType is NULL");
            exit(EXIT_FAILURE);
        }
    }
    if (NumberOfArgs > 1) {
        const char* simIdPtr = le_arg_GetArg(1);
        if (NULL == simIdPtr)
        {
            LE_ERROR("cardIdPtr is NULL");
            exit(EXIT_FAILURE);
        }
        simId = GetSimId(simIdPtr);
    }

    if (strcmp(testType, "state") == 0) {
        tafSimTest_state(simId);
    } else if (strcmp(testType, "events") == 0) {
        NewSimStateHandlerRef = taf_sim_AddNewStateHandler(TestNewSimStateHandler, NULL);
        LE_ASSERT(NewSimStateHandlerRef!=NULL);
        exitApplication = false;
    } else {
        DisplayAppUsage();
        exit(EXIT_FAILURE);
    }

    if (exitApplication)
    {
        LE_INFO("Exit tafSimTest App");
        exit(EXIT_SUCCESS);
    }
}
