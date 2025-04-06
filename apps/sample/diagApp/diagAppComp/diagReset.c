/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "diagPrivate.h"

#define CAN_BE_RESET 1

//Diag Reset
static taf_diagReset_ServiceRef_t diagResetSvcRef = NULL;
static taf_diagReset_RxMsgHandlerRef_t diagResetMsgRef = NULL;

static le_sem_Ref_t semRef;

//Function to convert reset type to string
static char* tafResetTypeToString(uint8_t resetType)
{
    char* state;
    switch(resetType)
    {
        case 0x01 :
            state = "Hard reset";
            break;
        case 0x02 :
            state = "Key off on reset";
            break;
        case 0x03 :
            state = "Soft reset";
            break;
        case 0x04 :
            state = "Enable rapid power shoutdown reset";
            break;
        case 0x05 :
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
    uint8_t resetType,
    void* contextPtr
)
{
    LE_TEST_INFO("Received reset req msg %s", tafResetTypeToString(resetType));

    switch(resetType)
    {
        case 0x01:
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

