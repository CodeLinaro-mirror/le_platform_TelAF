/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "diagPrivate.h"

#define CAN_BE_RESET 1

//Diag Reset
static taf_diagReset_ServiceRef_t diagResetSvcRef = NULL;
static taf_diagReset_RxMsgHandlerRef_t diagResetMsgRef = NULL;

static le_sem_Ref_t semRef;

//Function to convert reset type to string
static char* tafResetTypeToString(taf_diagReset_Type_t resetType)
{
    char* state;
    switch(resetType)
    {
        case TAF_DIAGRESET_HARD_RESET :
            state = "Hard reset";
            break;
        case TAF_DIAGRESET_KEY_OFF_ON_RESET :
            state = "Key off on reset";
            break;
        case TAF_DIAGRESET_SOFT_RESET :
            state = "Soft reset";
            break;
        case TAF_DIAGRESET_ENABLE_RAPID_POWER_SHUTDOWN_RESET :
            state = "Enable rapid power shoutdown reset";
            break;
        case TAF_DIAGRESET_DISABLE_RAPID_POWER_SHUTDOWN_RESET :
            state = "Disable rapid power shoutdown reset";
            break;
        default :
            state = "Unknown";
            break;
    }
    return state;
}

// Callback function for reset request message
// Hard reset type is used to reboot to active after firmware is installed in this sample.
void resetMsgHandler
(
    taf_diagReset_RxMsgRef_t rxMsgRef,
    taf_diagReset_Type_t resetType,
    void* contextPtr
)
{
    LE_TEST_INFO("Received reset req msg %s", tafResetTypeToString(resetType));

    switch(resetType)
    {
        case TAF_DIAGRESET_HARD_RESET:
            //If installed firmware successfully, then can reboot to active
            if(diagRoutineCtrl_GetUpdateState() == TAF_UPDATE_INSTALL_SUCCESS)
            {
                taf_fwupdate_RebootToActive();
                if(taf_diagReset_SendResp(rxMsgRef, TAF_DIAGRESET_NO_ERROR ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
            }
            else
            {
                if(taf_diagReset_SendResp(rxMsgRef, TAF_DIAGRESET_CONDITIONS_NOT_CORRECT) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
            }
            break;
        default:
            //Check if the criteria for the ECUReset request is met
            if(CAN_BE_RESET)
            {
                //Do other reset
                if(taf_diagReset_SendResp(rxMsgRef, TAF_DIAGRESET_NO_ERROR ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
            }
            else
            {
                if(taf_diagReset_SendResp(rxMsgRef, TAF_DIAGRESET_CONDITIONS_NOT_CORRECT) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
            }
            break;
    }
}

static void* diagResetMsgThread(void* ctxPtr)
{
    taf_diagReset_ConnectService();
    taf_fwupdate_ConnectService();

    diagResetMsgRef = taf_diagReset_AddRxMsgHandler(diagResetSvcRef, resetMsgHandler, NULL);
    LE_TEST_OK(diagResetMsgRef != NULL, "Registered successfully for resetMsgHandler");

    le_sem_Post(semRef);
    le_event_RunLoop();
    return NULL;
}

le_result_t diagReset_Init(void)
{
    semRef = le_sem_Create("SemRef", 0);

    //get diag reset svc reference
    diagResetSvcRef = taf_diagReset_GetService(TAF_DIAGRESET_ALL_RESET);
    if(diagResetSvcRef == NULL)
    {
        LE_ERROR("Get diagReset service");
        return LE_FAULT;
    }

    // Create diag reset message handle thread to handle ECUReset request(0x11)
    le_thread_Ref_t resetThreadRef = le_thread_Create("resetThread",
            diagResetMsgThread, NULL);

    le_thread_Start(resetThreadRef);
    le_sem_Wait(semRef);

    return LE_OK;
}

