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

static void TestNewSimStateHandler
(
    taf_sim_Id_t     simId,
    taf_sim_States_t simState,
    void*           contextPtr
)
{
    LE_INFO("New SIM event for SIM card: %d", simId);
    LE_INFO("SIM state: %s", SimStateToString(simState));
}

//Function to convert sim state to string
char* SimStateToString(taf_sim_States_t state) {
    char *cardState;
    switch(state) {
        case TAF_SIM_INSERTED:
            cardState = "TAF_SIM_INSERTED";
            break;
        case TAF_SIM_ABSENT:
            cardState = "TAF_SIM_ABSENT";
            break;
        case TAF_SIM_READY:
            cardState = "TAF_SIM_READY";
            break;
        case TAF_SIM_ERROR:
            cardState = "TAF_SIM_ERROR";
            break;
        default:
            cardState = "TAF_SIM_STATE_UNKNOWN";
            break;
    }
    return cardState;
}

//Function to test sim state
void tafSimTest_state
(
    taf_sim_Id_t simId
)
{
    taf_sim_States_t             state;
    taf_sim_NewStateHandlerRef_t testNewStateHandlerRef;

    testNewStateHandlerRef = taf_sim_AddNewStateHandler(TestNewSimStateHandler, NULL);
    LE_ASSERT(NULL != testNewStateHandlerRef);

    state = taf_sim_GetState(simId);

    LE_INFO("test: state %d", state);

    LE_ASSERT((state >= TAF_SIM_INSERTED) && (state <= TAF_SIM_ERROR));

    printf("\n Sim Card.%d state = %s\n" , simId ,SimStateToString(state));
    printf("\n Is SIM card Ready = %s\n", taf_sim_IsReady(simId) ? "true" : "false");
    printf("\n Is SIM card Present = %s \n", taf_sim_IsPresent(simId) ? "true" : "false");
}
