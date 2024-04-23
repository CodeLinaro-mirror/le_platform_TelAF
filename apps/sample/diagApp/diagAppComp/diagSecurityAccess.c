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

//Diag Session/Security
static taf_diagSecurity_ServiceRef_t diagSecuritySvcRef = NULL;
static taf_diagSecurity_RxSesTypeCheckHandlerRef_t diagSesTypeMsgRef = NULL;
static taf_diagSecurity_SesChangeHandlerRef_t diagSesChangeRef = NULL;
static taf_diagSecurity_RxSecAccessMsgHandlerRef_t diagSecurityMsgRef = NULL;

static le_sem_Ref_t semRef;

static const uint8_t seedData[] = {0x36, 0x57};

// Callback function for sessionCtrl request message
static void sesTypeMsgHandler
(
    taf_diagSecurity_RxSesTypeCheckRef_t rxMsgRef,
    uint8_t SesCtrlType,
    void* contextPtr
)
{
    LE_TEST_INFO("Received session control type req msg");
    LE_TEST_INFO("Received session control type is %x", SesCtrlType);

    le_result_t result;

    result = taf_diagSecurity_SendSesTypeCheckResp(rxMsgRef, TAF_DIAGSECURITY_SES_CONTROL_NO_ERROR);
    if (result == LE_OK)
    {
        LE_TEST_INFO("Session response is sent");
    }
}

// Callback function for session change message
static void sesChangeHandler
(
    taf_diagSecurity_SesChangeRef_t sesChangeRef,
    uint8_t PreviousType,
    uint8_t CurrentType,
    void* contextPtr
)
{
    LE_TEST_INFO("sesChangeHandler");
    LE_TEST_INFO("Previous session type: %x, Current session type: %x", PreviousType, CurrentType);

    if (PreviousType == 0x02 && CurrentType != 0x02)
    {
        LE_INFO("Deactivate programming --> release all resources");
        diagRFT_DeactivateProgramming();
    }

    return;
}

// Callback function for security request message
static void securityMsgHandler
(
    taf_diagSecurity_RxSecAccessMsgRef_t rxMsgRef,
    uint8_t accessType,
    void* contextPtr
)
{
    size_t seedDataLen = 0, keyDataLen = 0;
    uint8_t keyData[TAF_DIAGSECURITY_MAX_SEC_ACCESS_PAYLOAD_SIZE];
    uint8_t securityAccessDataRecord[TAF_DIAGSECURITY_MAX_SEC_ACCESS_PAYLOAD_SIZE];
    size_t securityAccessDataRecordSize = 0;

    le_result_t result;

    LE_TEST_INFO("Received security req msg access type: %d", accessType);

    //Send seed response
    if(accessType %2 != 0)
    {
        result = taf_diagSecurity_GetSecAccessPayloadLen(
                                        rxMsgRef,
                                        (uint16_t *)&securityAccessDataRecordSize);
        if(result != LE_OK)
        {
            LE_ERROR("API GetSecAccessPayloadLen");
            // UDS_0x27_NRC_22: API GetSecAccessPayloadLen
            if(taf_diagSecurity_SendSecAccessResp(
                    rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_CONDITIONS_NOT_CORRECT,
                    NULL, 0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

        result = taf_diagSecurity_GetSecAccessPayload(rxMsgRef,
                                                      securityAccessDataRecord,
                                                      &securityAccessDataRecordSize);
        if(result != LE_OK)
        {
            LE_ERROR("API GetSecAccessPayload");
            // UDS_0x27_NRC_22: API GetSecAccessPayload
            if(taf_diagSecurity_SendSecAccessResp(
                    rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_CONDITIONS_NOT_CORRECT,
                    NULL, 0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

        LE_INFO("Request-Seed: securityAccessDataRecord size is [%" PRIuS "]",
                securityAccessDataRecordSize);

        #define isValidSecurityAccessDataRecord(record, size) (1)
        if (! isValidSecurityAccessDataRecord(securityAccessDataRecord,
                                              securityAccessDataRecordSize))
        {
            LE_ERROR("Invalid securityAccessDataRecord");
            // UDS_0x27_NRC_31: Invalid securityAccessDataRecord
            if(taf_diagSecurity_SendSecAccessResp(
                    rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_REQUEST_OUT_OF_RANGE,
                    NULL, 0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

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
            // UDS_0x27_NRC_35: Send-Key size is bad
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
            // UDS_0x27_NRC_22: API GetSecAccessPayload (send-key)
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
            // UDS_0x27_NRC_35: Got invalid key by API
            if(taf_diagSecurity_SendSecAccessResp( rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_INVALID_KEY, NULL,0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
        }
    }

    return;
}

static void* diagSecurityMsgThread(void* ctxPtr)
{
    taf_diagSecurity_ConnectService();

    diagSesTypeMsgRef = taf_diagSecurity_AddRxSesTypeCheckHandler( diagSecuritySvcRef,
            sesTypeMsgHandler, NULL);
    LE_TEST_OK(diagSesTypeMsgRef != NULL, "Registered successfully for sesTypeMsgHandler");

    diagSesChangeRef = taf_diagSecurity_AddSesChangeHandler( diagSecuritySvcRef,
            sesChangeHandler, NULL);
    LE_TEST_OK(diagSesChangeRef != NULL, "Registered successfully for sesChangeHandler");

    diagSecurityMsgRef = taf_diagSecurity_AddRxSecAccessMsgHandler( diagSecuritySvcRef,
            securityMsgHandler, NULL);
    LE_TEST_OK(diagSecurityMsgRef != NULL, "Registered successfully for securityMsgHandler");

    le_sem_Post(semRef);
    le_event_RunLoop();
    return NULL;
}

le_result_t diagSecurityAccess_Init(void)
{
    semRef = le_sem_Create("SemRef", 0);

    //get diag security reference
    diagSecuritySvcRef = taf_diagSecurity_GetService();
    if(diagSecuritySvcRef == NULL)
    {
        LE_ERROR("Get diagSecurity service");
        return LE_FAULT;
    }
    // Get the current session type
    le_result_t result;
    uint8_t currentSesType;
    result = taf_diagSecurity_GetCurrentSesType(diagSecuritySvcRef, &currentSesType);
    if (result == LE_OK)
    {
        LE_TEST_INFO("Current active session type is %x", currentSesType);
    }

    // Create diag security message handle thread to handle security access(0x27)
    le_thread_Ref_t diagSecurityThreadRef = le_thread_Create("diagSecurityTd",
            diagSecurityMsgThread, NULL);

    le_thread_Start(diagSecurityThreadRef);
    le_sem_Wait(semRef);

    return LE_OK;
}
