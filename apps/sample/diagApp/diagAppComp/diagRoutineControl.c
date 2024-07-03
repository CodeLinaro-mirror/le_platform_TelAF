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

#define ROUTINE_CONTROL_RECORD_LENGTH 100
#define UPDATE_PRE_DOWNLOAD_CHECK_IDENTIFIER 0x0246
#define UPDATE_POST_DOWNLOAD_CHECK_IDENTIFIER 0x0247
#define PRE_DOWNLOAD_CHECK_OK 1

//TelAF Update
static taf_update_StateHandlerRef_t UpdateStateHandlerRef = NULL;

taf_update_State_t updateState = TAF_UPDATE_IDLE;

//Diag Routine Control
static taf_diagRoutineCtrl_ServiceRef_t diagRCPreDlSvcRef = NULL;
static taf_diagRoutineCtrl_ServiceRef_t diagRCPostDlSvcRef = NULL;
static taf_diagRoutineCtrl_RxMsgHandlerRef_t diagRoutineCtrlMsgRef = NULL;

static le_sem_Ref_t semRef;

// Callback function for TelAF update service to get the status
/*

ENUM State
{
    IDLE,              ///< Idle state; user can download OTA packages.
    DOWNLOADING,       ///< Downloading state, user can query download progress.
    DOWNLOAD_PAUSED,   ///< Download paused state, not supported.
    DOWNLOAD_SUCCESS,  ///< Download success state, OTA package downloaded successfully.
    DOWNLOAD_FAIL,     ///< Download fail state, user can retry the download.
    INSTALLING,        ///< Installing state, user can query installation progress.
    INSTALL_SUCCESS,   ///< Install success state, user can reboot to active if firmware installed
                       ///  or start probation if app installed.
    INSTALL_FAIL,      ///< Install fail state, update service drives to idle state later.
    PROBATION,         ///< Probation state, during probation, update service checks if newly
                       ///   installed firmware or app is stable and drives to idle state later.
    PROBATION_SUCCESS, ///< Probation success state, application or firmware is working properly
                       ///  without errors.
    PROBATION_FAIL,    ///< Probation fail state, update service drives to idle state later.
    ROLLBACK,          ///< Rollback state, rollback to the original system.
    ROLLBACK_SUCCESS,  ///< Rollback success state, rollback to the original system succeeded.
    ROLLBACK_FAIL,     ///< Rollback fail state, rollback to the original system failed.
    REPORTING          ///< Reporting state, update service is reporting state to server.
};

*/
void updateStateHandler(taf_update_StateInd_t* indication, void* contextPtr)
{
    updateState = indication->state;
    LE_INFO("-----Update state=%d",updateState);
}

/**
 * Get update state for other parts
 */
taf_update_State_t diagRoutineCtrl_GetUpdateState()
{
    return updateState;
}

//Function to convert routine control type to string
char* tafRoutineCtrlTypeToString(taf_diagRoutineCtrl_Type_t routineCtrlType)
{
    char* state;
    switch(routineCtrlType)
    {
        case TAF_DIAGROUTINECTRL_START_ROUTINE :
            state = "Start routine";
            break;
        case TAF_DIAGROUTINECTRL_STOP_ROUTINE :
            state = "Stop routine";
            break;
        case TAF_DIAGROUTINECTRL_REQUEST_ROUTINE_RESULTS :
            state = "Request routine result";
            break;
        default :
            state = "Unknown";
            break;
    }
    return state;
}


// Callback function for routine control request message
void routineCtrl_0246_MsgHandler
(
    taf_diagRoutineCtrl_RxMsgRef_t rxMsgRef,
    taf_diagRoutineCtrl_Type_t routineCtrlType,
    uint16_t identifier,
    void* contextPtr
)
{
    static bool active = false;
    LE_TEST_INFO("Received routine control req id 0x%x, type = %s",
                 identifier, tafRoutineCtrlTypeToString(routineCtrlType));

    switch( routineCtrlType)
    {
        //start routine
        case TAF_DIAGROUTINECTRL_START_ROUTINE:
            LE_INFO("Start routine for identifier 0x%x", identifier);
            if(PRE_DOWNLOAD_CHECK_OK)
            {
                LE_INFO("Pre download check is OK");
                if(taf_diagRoutineCtrl_SendResp( rxMsgRef, TAF_DIAGROUTINECTRL_NO_ERROR,
                        NULL, 0 ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
            }
            else
            {
                LE_ERROR("Pre download check is failed");
                if(taf_diagRoutineCtrl_SendResp( rxMsgRef,
                        TAF_DIAGROUTINECTRL_CONDITIONS_NOT_CORRECT, NULL, 0 ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
            }
            active = true;
            break;
        case TAF_DIAGROUTINECTRL_STOP_ROUTINE:
            if (active)
            {
                if(taf_diagRoutineCtrl_SendResp( rxMsgRef, TAF_DIAGROUTINECTRL_NO_ERROR,
                        NULL, 0 ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
            }
            else
            {
                if(taf_diagRoutineCtrl_SendResp( rxMsgRef, 
                    TAF_DIAGROUTINECTRL_REQUEST_SEQUENCE_ERROR, NULL, 0 ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
            }
            break;
        default:
            //stop routine, request routine result,etc.
            if(taf_diagRoutineCtrl_SendResp( rxMsgRef, TAF_DIAGROUTINECTRL_NO_ERROR,
                    NULL, 0 ) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            break;
    }

}

// Callback function for routine control request message
// Identifier 0x0247 is used to install the firmware(start routine) and get the result( request
// routine results) in this sample.
void routineCtrl_0247_MsgHandler
(
    taf_diagRoutineCtrl_RxMsgRef_t rxMsgRef,
    taf_diagRoutineCtrl_Type_t routineCtrlType,
    uint16_t identifier,
    void* contextPtr
)
{
    le_result_t result;
    uint8_t recordData[ROUTINE_CONTROL_RECORD_LENGTH];
    size_t dataLen = 0;
    LE_TEST_INFO("Received routine control req id 0x%x, type = %s",
                 identifier, tafRoutineCtrlTypeToString(routineCtrlType));

    switch( routineCtrlType)
    {
        //start routine to install firmware
        case TAF_DIAGROUTINECTRL_START_ROUTINE:
            LE_INFO("Start routine for identifier 0x%x",identifier);
            const char * filePath = diagRFT_GetCompleteFileName();
            if(updateState == TAF_UPDATE_IDLE || filePath[0] != '\0')
            {
                LE_INFO("Post download check is OK");
                //Install the firmware
                result = taf_update_Install(TAF_UPDATE_FOTA, filePath);
                LE_INFO("update firmware, result=%d, filepath=%s",result, filePath);
                if(result == LE_OK)
                {
                    if(taf_diagRoutineCtrl_SendResp( rxMsgRef, TAF_DIAGROUTINECTRL_NO_ERROR,
                            NULL, 0 ) != LE_OK)
                    {
                        LE_ERROR("Send response error");
                    }
                }
                else
                {
                    if(taf_diagRoutineCtrl_SendResp( rxMsgRef,
                            TAF_DIAGROUTINECTRL_GENERAL_PROGRAMMING_FAILURE, NULL, 0 ) != LE_OK)
                    {
                        LE_ERROR("Send response error");
                    }
                }
            }
            else
            {
                LE_ERROR("Post download check is failed");
                if(taf_diagRoutineCtrl_SendResp( rxMsgRef,
                        TAF_DIAGROUTINECTRL_BUSY_REPEAT_REQ, NULL, 0 ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
            }
            break;
        // request routine result to get the status of installation
        case TAF_DIAGROUTINECTRL_REQUEST_ROUTINE_RESULTS:
            LE_INFO("Request routine results for identifier 0x%x",identifier);

            recordData[0]= (uint8_t)updateState;
            dataLen = 1;

            if(taf_diagRoutineCtrl_SendResp( rxMsgRef, TAF_DIAGROUTINECTRL_NO_ERROR, recordData,
                    dataLen ) != LE_OK)
            {
                LE_ERROR("Send response error");
            }

            break;
        default:
            //stop routine, request routine result,etc.
            if(taf_diagRoutineCtrl_SendResp( rxMsgRef, TAF_DIAGROUTINECTRL_NO_ERROR,
                    NULL, 0 ) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            break;
    }
}

static void* diagRoutingCtrlMsgThread(void* ctxPtr)
{
    taf_diagRoutineCtrl_ConnectService();
    taf_appMgmt_ConnectService();
    taf_update_ConnectService();

    diagRoutineCtrlMsgRef = taf_diagRoutineCtrl_AddRxMsgHandler( diagRCPreDlSvcRef,
            routineCtrl_0246_MsgHandler, NULL);
    LE_TEST_OK(diagRoutineCtrlMsgRef != NULL,
              "Registered successfully for routineCtrl_0246_MsgHandler");

    diagRoutineCtrlMsgRef = taf_diagRoutineCtrl_AddRxMsgHandler( diagRCPostDlSvcRef,
            routineCtrl_0247_MsgHandler, NULL);
    LE_TEST_OK(diagRoutineCtrlMsgRef != NULL,
            "Registered successfully for routineCtrl_0247_MsgHandler");

    le_sem_Post(semRef);
    le_event_RunLoop();
    return NULL;
}

static void* updateStateThread(void* contextPtr)
{
    taf_update_ConnectService();

    LE_INFO("======== State Handler Thread  ========");
    UpdateStateHandlerRef = taf_update_AddStateHandler(
                                          (taf_update_StateHandlerFunc_t)updateStateHandler, NULL);
    LE_TEST_OK(UpdateStateHandlerRef != NULL, "Registered successfully for update state handler");

    le_sem_Post(semRef);
    le_event_RunLoop();
    return NULL;
}


le_result_t diagRoutineControl_Init(void)
{
    semRef = le_sem_Create("SemRef", 0);

    //get diag routinectrl svc reference for pre-download check
    diagRCPreDlSvcRef = taf_diagRoutineCtrl_GetService(UPDATE_PRE_DOWNLOAD_CHECK_IDENTIFIER);
    if(diagRCPreDlSvcRef == NULL)
    {
        LE_ERROR("Get diagRoutineCtrl service for pre-download");
        return LE_FAULT;
    }

    //get diag routinectrl svc reference for post-download check
    diagRCPostDlSvcRef = taf_diagRoutineCtrl_GetService(UPDATE_POST_DOWNLOAD_CHECK_IDENTIFIER);
    if(diagRCPostDlSvcRef == NULL)
    {
        LE_ERROR("Get diagRoutineCtrl service for post-download");
        return LE_FAULT;
    }

    // Create Update State Handler thread to get the update status
    le_thread_Ref_t updateStateThreadRef = le_thread_Create("updateStateTd",
            updateStateThread, NULL);

    le_thread_Start(updateStateThreadRef);
    le_sem_Wait(semRef);

    // Create diag routine control message handle thread to handle routine control request(0x31)
    le_thread_Ref_t routineCtrlThreadRef = le_thread_Create("routineCtrlTd",
            diagRoutingCtrlMsgThread, NULL);

    le_thread_Start(routineCtrlThreadRef);
    le_sem_Wait(semRef);

    return LE_OK;
}
