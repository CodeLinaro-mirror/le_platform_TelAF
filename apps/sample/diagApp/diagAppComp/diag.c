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

#include "legato.h"
#include "interfaces.h"

//tmp definition to test
#define TEST_ECUREST
#define TEST_UPDATE
#define TEST_ROUTINE_CONTROL
#define TEST_SECURITY_ACCESS

#define CAN_BE_RESET 1
#define UPDATE_PRE_DOWNLOAD_CHECK_IDENTIFIER 0x0246
#define UPDATE_POST_DOWNLOAD_CHECK_IDENTIFIER 0x0247
#define PRE_DOWNLOAD_CHECK_OK 1
#define ROUTEINE_CONTROL_RECORD_LENGTH 100
#define UPDATE_FILE_PATH_LENGTH 1024
#define SYSTEM_COMMAND_STR_LENGTH 1030
#define DELETE_SYSTEM_CMD_FORMAT "rm %s"

static le_sem_Ref_t semRef;

#ifdef TEST_ECUREST
//Diag Reset
static taf_diagReset_ServiceRef_t diagResetSvcRef = NULL;
static taf_diagReset_RxMsgHandlerRef_t diagResetMsgRef = NULL;
const uint8_t seedData[] = {0x36, 0x57};
#endif

#ifdef TEST_ROUTINE_CONTROL
//Diag Routine Control
static taf_diagRoutineCtrl_ServiceRef_t diagRCPreDlSvcRef = NULL;
static taf_diagRoutineCtrl_ServiceRef_t diagRCPostDlSvcRef = NULL;
static taf_diagRoutineCtrl_RxMsgHandlerRef_t diagRoutineCtrlMsgRef = NULL;
#endif

#ifdef TEST_UPDATE
//Diag Update
static taf_diagUpdate_ServiceRef_t diagUpdateSvcRef = NULL;
static taf_diagUpdate_RxFileXferMsgHandlerRef_t diagFileXferMsgRef = NULL;
static taf_diagUpdate_RxXferDataMsgHandlerRef_t diagXferDataMsgRef = NULL;
static taf_diagUpdate_RxXferExitMsgHandlerRef_t diagXferExitMsgRef = NULL;
#endif

#ifdef TEST_SECURITY_ACCESS
//Diag Security
static taf_diagSecurity_ServiceRef_t diagSecuritySvcRef = NULL;
static taf_diagSecurity_RxSecAccessMsgHandlerRef_t diagSecurityMsgRef = NULL;
#endif

//TelAF Update
static taf_update_StateHandlerRef_t UpdateStateHandlerRef = NULL;

taf_update_State_t updateState = TAF_UPDATE_IDLE;

FILE  *filePtr = NULL;
static char filePath[UPDATE_FILE_PATH_LENGTH];

#ifdef TEST_ECUREST
//Function to convert reset type to string
char* tafResetTypeToString(taf_diagReset_Type_t resetType)
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
#endif

#ifdef TEST_ROUTINE_CONTROL
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
#endif

#ifdef TEST_UPDATE
//Function to convert modeOfOperation type to string
char* tafModeOfOperationToString(taf_diagUpdate_ModeOfOpsType_t modeOfOps)
{
    char* operation;
    switch(modeOfOps)
    {
        case TAF_DIAGUPDATE_ADD_FILE :
            operation = "Add file";
            break;
        case TAF_DIAGUPDATE_DELETE_FILE :
            operation = "Delete file";
            break;
        default :
            operation = "Not supported";
            break;
    }
    return operation;
}

// Function to create a file
static FILE* createFile(const char * pathNamePtr)
{
    le_result_t result;
    mode_t f_attrib;
    f_attrib = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

    FILE* file = le_flock_TryCreateStream(pathNamePtr,
                                          LE_FLOCK_READ_AND_APPEND,
                                          LE_FLOCK_FAIL_IF_EXIST ,
                                          f_attrib,
                                          &result);
    if(result != LE_OK)
    {
        LE_ERROR("ERROR TO CREATE FILE");
        return NULL;
    }

    return file;

}

// Function to close a file
static void closeFile()
{

    if(filePtr != NULL)
    {
        le_flock_CloseStream(filePtr);
        filePtr = NULL;
    }
}

// Function to write data into file
static le_result_t writeFile(const uint8_t *data, const uint16_t len)
{

    int bytes;
    if(filePtr == NULL)
    {
        LE_ERROR("write data error");
        return LE_FAULT;
    }
    if ((bytes = write(fileno(filePtr), data, len) < 0))
    {
        LE_ERROR("write data error");
        return LE_FAULT;
    }
    return LE_OK;
}

#endif
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

#ifdef TEST_ECUREST
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
            if(updateState == TAF_UPDATE_INSTALL_SUCCESS)
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

#endif

#ifdef TEST_ROUTINE_CONTROL
// Callback function for routine control request message
void routineCtrl_0246_MsgHandler
(
    taf_diagRoutineCtrl_RxMsgRef_t rxMsgRef,
    taf_diagRoutineCtrl_Type_t routineCtrlType,
    uint16_t identifier,
    void* contextPtr
)
{
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
                if(taf_diagRoutineCtrl_SendResp( rxMsgRef,
                                                 TAF_DIAGROUTINECTRL_NO_ERROR,
                                                 NULL, 0
                                               ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
            }
            else
            {
                LE_ERROR("Pre download check is failed");
                if(taf_diagRoutineCtrl_SendResp( rxMsgRef,
                                                 TAF_DIAGROUTINECTRL_GENERAL_PROGRAMMING_FAILURE,
                                                 NULL, 0
                                               ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
            }
            break;
        default:
            //stop routine, request routine result,etc.
            if(taf_diagRoutineCtrl_SendResp( rxMsgRef,
                                             TAF_DIAGROUTINECTRL_NO_ERROR,
                                             NULL, 0
                                           ) != LE_OK)
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
    uint8_t recordData[ROUTEINE_CONTROL_RECORD_LENGTH];
    size_t dataLen = 0;
    LE_TEST_INFO("Received routine control req id 0x%x, type = %s",
                 identifier, tafRoutineCtrlTypeToString(routineCtrlType));

    switch( routineCtrlType)
    {
        //start routine to install firmware
        case TAF_DIAGROUTINECTRL_START_ROUTINE:
            LE_INFO("Start routine for identifier 0x%x",identifier);
            if(updateState == TAF_UPDATE_IDLE || filePath[0] != '\0')
            {
                LE_INFO("Post download check is OK");
                //Install the firmware
                result = taf_update_Install(TAF_UPDATE_FOTA, filePath);
                LE_INFO("update firmware, result=%d, filepath=%s",result, filePath);
                if(result == LE_OK)
                {
                    if(taf_diagRoutineCtrl_SendResp( rxMsgRef,
                                                     TAF_DIAGROUTINECTRL_NO_ERROR,
                                                     NULL, 0
                                                   ) != LE_OK)
                    {
                        LE_ERROR("Send response error");
                    }
                }
                else
                {
                    if(taf_diagRoutineCtrl_SendResp( rxMsgRef,
                                                    TAF_DIAGROUTINECTRL_GENERAL_PROGRAMMING_FAILURE,
                                                    NULL, 0
                                                   ) != LE_OK)
                    {
                        LE_ERROR("Send response error");
                    }
                }
            }
            else
            {
                LE_ERROR("Post download check is failed");
                if(taf_diagRoutineCtrl_SendResp( rxMsgRef,
                                                 TAF_DIAGROUTINECTRL_GENERAL_PROGRAMMING_FAILURE,
                                                 NULL, 0
                                               ) != LE_OK)
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

            if(taf_diagRoutineCtrl_SendResp( rxMsgRef,
                                             TAF_DIAGROUTINECTRL_NO_ERROR,
                                             recordData, dataLen
                                           ) != LE_OK)
            {
                LE_ERROR("Send response error");
            }

            break;
        default:
            //stop routine, request routine result,etc.
            if(taf_diagRoutineCtrl_SendResp( rxMsgRef,
                                             TAF_DIAGROUTINECTRL_NO_ERROR,
                                             NULL, 0
                                           ) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            break;
    }

}
#endif

#ifdef TEST_UPDATE
// Callback function for file transfer request message
void fileXferMsgHandler
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
    taf_diagUpdate_ModeOfOpsType_t modeOfOps,
    void* contextPtr
)
{
    le_result_t result;
    int delRet = 0;
    char filePathAndName[UPDATE_FILE_PATH_LENGTH];
    size_t fileLen = UPDATE_FILE_PATH_LENGTH;

    LE_TEST_INFO("Received file transfer req msg %s", tafModeOfOperationToString(modeOfOps));

    switch(modeOfOps)
    {
        case TAF_DIAGUPDATE_ADD_FILE:
            //Add file

            result = taf_diagUpdate_GetFilePathAndName( rxMsgRef,
                                                        (uint8_t *)filePathAndName,
                                                        &fileLen
                                                      );


            if(result != LE_OK)
            {
                LE_ERROR("Getting file name");
                if(taf_diagUpdate_SendFileXferResp( rxMsgRef,
                                                    TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT
                                                  ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
                return;
            }

            LE_INFO("Filename = %s", filePathAndName);
            le_utf8_Copy(filePath, filePathAndName, UPDATE_FILE_PATH_LENGTH, NULL);
            //Create the file
            filePtr = createFile( filePathAndName);
            if(filePtr == NULL)
            {
                LE_ERROR("!!!! File creation failed");
                if(taf_diagUpdate_SendFileXferResp( rxMsgRef,
                                                   TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT
                                                  ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }

            }
            else
            {
                LE_INFO("File creation ");
                if(taf_diagUpdate_SendFileXferResp(rxMsgRef, TAF_DIAGUPDATE_FILE_XFER_NO_ERROR
                                                  ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }

            }
            break;
        case TAF_DIAGUPDATE_DELETE_FILE:

            result = taf_diagUpdate_GetFilePathAndName( rxMsgRef,
                                                        (uint8_t*)filePathAndName,
                                                        &fileLen
                                                      );

            if(result != LE_OK)
            {
                LE_ERROR("Getting file name");
                if(taf_diagUpdate_SendFileXferResp( rxMsgRef,
                                                   TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT
                                                  ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
                return;
            }

            LE_INFO("Filename = %s", filePathAndName);
            //Delete the file
            delRet = unlink(filePathAndName);
            if ( delRet == -1 )
            {
                LE_ERROR("failed to delete %s", filePathAndName);
                if(taf_diagUpdate_SendFileXferResp( rxMsgRef,
                                                    TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT
                                                  ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }

            }
            else
            {
                if(taf_diagUpdate_SendFileXferResp( rxMsgRef,
                                                    TAF_DIAGUPDATE_FILE_XFER_NO_ERROR
                                                  ) != LE_OK)
                {
                    LE_ERROR("Send response error");
                }
            }

            break;
        default:
            if(taf_diagUpdate_SendFileXferResp( rxMsgRef, TAF_DIAGUPDATE_FILE_XFER_NO_ERROR
                                              ) != LE_OK)
            {
                LE_ERROR("Send response error");
            }

            break;
    }

}

// Callback function for data transfer request message
// Write data into file
void xferDataMsgHandler
(
    taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef,
    void* contextPtr
)
{
    le_result_t result;
    uint8_t xferData[TAF_DIAGUPDATE_MAX_XFER_PARAM_REC_SIZE];
    size_t xferDataLen = 0;

    LE_DEBUG("Received transfer data req msg");

    result = taf_diagUpdate_GetXferDataParamRecLen( rxMsgRef, (uint16_t *)&xferDataLen);
    if(result != LE_OK || xferDataLen == 0 || xferDataLen > TAF_DIAGUPDATE_MAX_XFER_PARAM_REC_SIZE)
    {
        LE_ERROR("Getting file len");
        if(taf_diagUpdate_SendXferDataResp( rxMsgRef,
                                            TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE,
                                            NULL,0
                                            ) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    result = taf_diagUpdate_GetXferDataParamRec( rxMsgRef, xferData, &xferDataLen);

    if(result != LE_OK)
    {
        LE_ERROR("Getting file name");
        if(taf_diagUpdate_SendXferDataResp( rxMsgRef,
                                            TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE,
                                            NULL, 0
                                          ) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    if(xferDataLen <=0)
    {
        LE_ERROR("Getting data");
        if(taf_diagUpdate_SendXferDataResp( rxMsgRef,
                                            TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE,
                                            NULL, 0
                                          ) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    if(filePtr != NULL)
    {
        //write data into file
        result = writeFile(xferData, xferDataLen);
        if(result != LE_OK)
        {
            LE_ERROR("Failed to write data");
            if(taf_diagUpdate_SendXferDataResp( rxMsgRef,
                                               TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE,
                                               NULL, 0
                                              ) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
        }
        else
        {
            if(taf_diagUpdate_SendXferDataResp( rxMsgRef,
                                                TAF_DIAGUPDATE_XFER_DATA_NO_ERROR , NULL, 0
                                              ) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
        }
    }
    else
    {

        LE_ERROR("File pointer is NULL");
        if(taf_diagUpdate_SendXferDataResp( rxMsgRef,
                                            TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE,
                                            NULL, 0
                                          ) != LE_OK)
        {
            LE_ERROR("Send response error");
        }

    }

}

// Callback function for transfer exit request message
// Close the file
void xferExitMsgHandler
(
    taf_diagUpdate_RxXferExitMsgRef_t rxMsgRef,
    void* contextPtr
)
{

    LE_TEST_INFO("Received transfer exit req msg");

    if(filePtr != NULL)
    {
        closeFile();
        if(taf_diagUpdate_SendXferExitResp( rxMsgRef, TAF_DIAGUPDATE_XFER_EXIT_NO_ERROR,
                                            NULL, 0
                                          ) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
    }
    else
    {
        if(taf_diagUpdate_SendXferExitResp( rxMsgRef, TAF_DIAGUPDATE_XFER_EXIT_NO_ERROR,
                                            NULL, 0
                                          ) != LE_OK)
        {
            LE_ERROR("Send response error");
        }

    }


}
#endif

#ifdef TEST_SECURITY_ACCESS
// Callback function for security request message
void securityMsgHandler
(
    taf_diagSecurity_RxSecAccessMsgRef_t rxMsgRef,
    uint8_t accessType,
    void* contextPtr
)
{
    size_t seedDataLen = 0, keyDataLen = 0;
    uint8_t keyData[TAF_DIAGSECURITY_MAX_SEC_ACCESS_PAYLOAD_SIZE];

    le_result_t result;

    LE_TEST_INFO("Received security req msg access type: %d", accessType);

    //Send seed response
    if(accessType %2 != 0)
    {
        seedDataLen = sizeof(seedData);

        if(taf_diagSecurity_SendSecAccessResp( rxMsgRef,
                TAF_DIAGSECURITY_SEC_ACCESS_NO_ERROR, seedData, seedDataLen ) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
    }
    //Validate the key
    else
    {
        result = taf_diagSecurity_GetSecAccessPayloadLen( rxMsgRef, (uint16_t *)&keyDataLen);
        if(result != LE_OK || keyDataLen != sizeof(seedData) ||
                keyDataLen > TAF_DIAGSECURITY_MAX_SEC_ACCESS_PAYLOAD_SIZE )
        {
            LE_ERROR("Getting key len");
            if(taf_diagSecurity_SendSecAccessResp( rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_INVALID_KEY, NULL,0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

        result = taf_diagSecurity_GetSecAccessPayload( rxMsgRef, keyData, &keyDataLen);

        if(result != LE_OK)
        {
            LE_ERROR("Getting key data");
            if(taf_diagSecurity_SendSecAccessResp( rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_CONDITIONS_NOT_CORRECT, NULL, 0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

        //Validate the key, the algorithum is same as the python cliet tool
        if(keyData[0] == seedData[0]+1 && keyData[1] == seedData[1]+2)
        {
            if(taf_diagSecurity_SendSecAccessResp( rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_NO_ERROR, NULL,0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }

        }
        // Key is invalid
        else
        {
            if(taf_diagSecurity_SendSecAccessResp( rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_INVALID_KEY, NULL,0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
        }

    }

    return;

}

#endif

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

#ifdef TEST_ECUREST
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
#endif

#ifdef TEST_ROUTINE_CONTROL
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
#endif

#ifdef TEST_UPDATE
static void* diagUpdateMsgThread(void* ctxPtr)
{
    taf_diagUpdate_ConnectService();

    diagFileXferMsgRef = taf_diagUpdate_AddRxFileXferMsgHandler( diagUpdateSvcRef,
                                                                 fileXferMsgHandler, NULL
                                                               );
    LE_TEST_OK(diagFileXferMsgRef != NULL, "Registered successfully for fileXferMsgHandler");

    diagXferDataMsgRef = taf_diagUpdate_AddRxXferDataMsgHandler( diagUpdateSvcRef,
                                                                 xferDataMsgHandler, NULL
                                                               );
    LE_TEST_OK(diagXferDataMsgRef != NULL, "Registered successfully for xferDataMsgHandler");

    diagXferExitMsgRef = taf_diagUpdate_AddRxXferExitMsgHandler( diagUpdateSvcRef,
                                                                 xferExitMsgHandler, NULL
                                                               );
    LE_TEST_OK(diagXferExitMsgRef != NULL, "Registered successfully for xferExitMsgHandler");

    le_sem_Post(semRef);
    le_event_RunLoop();
    return NULL;
}
#endif

#ifdef TEST_SECURITY_ACCESS
static void* diagSecurityMsgThread(void* ctxPtr)
{
    taf_diagSecurity_ConnectService();

    diagSecurityMsgRef = taf_diagSecurity_AddRxSecAccessMsgHandler( diagSecuritySvcRef,
                                                                    securityMsgHandler, NULL
                                                                  );
    LE_TEST_OK(diagSecurityMsgRef != NULL, "Registered successfully for securityMsgHandler");

    le_sem_Post(semRef);
    le_event_RunLoop();
    return NULL;
}
#endif

COMPONENT_INIT
{
    LE_INFO("tafDiagApp starting");

    memset(filePath, 0, UPDATE_FILE_PATH_LENGTH);

    semRef = le_sem_Create("SemRef", 0);

#ifdef TEST_ECUREST
    //get diag reset svc reference
    diagResetSvcRef = taf_diagReset_GetService(TAF_DIAGRESET_ALL_RESET);
    if(diagResetSvcRef == NULL)
    {
        LE_ERROR("Get diagReset service");
        return;
    }
#endif
#ifdef TEST_ROUTINE_CONTROL
    //get diag routinectrl svc reference for pre-download check
    diagRCPreDlSvcRef = taf_diagRoutineCtrl_GetService(UPDATE_PRE_DOWNLOAD_CHECK_IDENTIFIER);
    if(diagRCPreDlSvcRef == NULL)
    {
        LE_ERROR("Get diagRoutineCtrl service for pre-download");
        return;
    }

    //get diag routinectrl svc reference for post-download check
    diagRCPostDlSvcRef = taf_diagRoutineCtrl_GetService(UPDATE_POST_DOWNLOAD_CHECK_IDENTIFIER);
    if(diagRCPostDlSvcRef == NULL)
    {
        LE_ERROR("Get diagRoutineCtrl service for post-download");
        return;
    }

#endif

#ifdef TEST_UPDATE
    //get diag update reference
    diagUpdateSvcRef = taf_diagUpdate_GetService();
    if(diagUpdateSvcRef == NULL)
    {
        LE_ERROR("Get diagUpdate service");
        return;
    }
#endif

#ifdef TEST_SECURITY_ACCESS
    //get diag security reference
    diagSecuritySvcRef = taf_diagSecurity_GetService();
    if(diagSecuritySvcRef == NULL)
    {
        LE_ERROR("Get diagSecurity service");
        return;
    }
#endif

    // Create Update State Handler thread to get the update status
    le_thread_Ref_t updateStateThreadRef = le_thread_Create("updateStateTd",
                                                             updateStateThread, NULL
                                                           );

    le_thread_Start(updateStateThreadRef);
    le_sem_Wait(semRef);

#ifdef TEST_ECUREST
    // Create diag reset message handle thread to handle ECUReset request(0x11)
    le_thread_Ref_t resetThreadRef = le_thread_Create("resetThread",
                                                      diagResetMsgThread, NULL
                                                     );

    le_thread_Start(resetThreadRef);
    le_sem_Wait(semRef);
#endif
#ifdef TEST_ROUTINE_CONTROL
    // Create diag routine control message handle thread to handle routine control request(0x31)
    le_thread_Ref_t routineCtrlThreadRef = le_thread_Create("routineCtrlTd",
                                                            diagRoutingCtrlMsgThread, NULL
                                                           );

    le_thread_Start(routineCtrlThreadRef);
    le_sem_Wait(semRef);
#endif
#ifdef TEST_UPDATE
    // Create diag upate message handle thread to handle filetransfer(0x38), transferdata(0x36)
    // and transfer exit(0x37)
    le_thread_Ref_t diagUpdateThreadRef = le_thread_Create("diagUpdateTd",
                                                            diagUpdateMsgThread, NULL
                                                          );

    le_thread_Start(diagUpdateThreadRef);
    le_sem_Wait(semRef);

#endif

#ifdef TEST_SECURITY_ACCESS
    // Create diag security message handle thread to handle security access(0x27)
    le_thread_Ref_t diagSecurityThreadRef = le_thread_Create("diagSecurityTd",
                                                            diagSecurityMsgThread, NULL
                                                          );

    le_thread_Start(diagSecurityThreadRef);
    le_sem_Wait(semRef);

#endif

}
