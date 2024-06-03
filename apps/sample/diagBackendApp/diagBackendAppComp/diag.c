/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

// DID length definition
#define DID_LEN  2

static le_sem_Ref_t semRef;
static taf_diagDIDBackend_ReadDIDHandlerRef_t diagReadDIDMsgRef = NULL;

static taf_diagSecBackend_SesTypeCheckHandlerRef_t diagSesReqMsgRef = NULL;
static taf_diagSecBackend_SesChangeHandlerRef_t diagSesChangeMsgRef = NULL;

// Callback function for read dataID request message
void readDIDMsgHandler
(
    taf_diagDIDBackend_ReadDIDRef_t rxMsgRef,
    uint16_t dataId,
    void* contextPtr
)
{
    LE_INFO("readDIDMsgHandler start");

    uint8_t sendBuf[TAF_DIAGDIDBACKEND_READ_DID_PAYLOAD_SIZE];
    size_t sendBufLen = 0;
    //uint8_t result;

    LE_INFO("readDIDMsgHandler receive ref: %p",rxMsgRef);
    LE_INFO("readDIDMsgHandler receive dataId: %x",dataId);

    // Sending response for some DID, which are defined/hardcoded in config.h file.
    if (dataId == 0x1122)
    {
        sendBuf[sendBufLen] = 0x12;
        sendBufLen++;
    }
    else if (dataId == 0x1133)
    {
        sendBuf[sendBufLen] = 0x12;
        sendBufLen++;
        sendBuf[sendBufLen] = 0x34;
        sendBufLen++;
    }
    else
    {
        sendBuf[sendBufLen] = 0x11;
        sendBufLen++;
    }

    // Send response
    if(taf_diagDIDBackend_SendReadDIDResp( rxMsgRef, TAF_DIAGDIDBACKEND_READ_DID_NO_ERROR,
            sendBuf, sendBufLen ) != LE_OK)
    {
        LE_ERROR("Send response error");
    }

    LE_INFO("readDIDMsgHandler completed");
    memset(sendBuf, 0, TAF_DIAGDIDBACKEND_READ_DID_PAYLOAD_SIZE);
    sendBufLen = 0;

    return;
}

// Callback function for SessionCtrl request message
void sesReqMsgHandler
(
    taf_diagSecBackend_SesTypeCheckRef_t sesTypeRef,
    taf_diagSecBackend_SessionType_t sesType,
    void* contextPtr
)
{
    LE_INFO("sesReqMsgHandler start");
    LE_INFO("Receive ref: %p", sesTypeRef);
    LE_INFO("Requested sesType: %x", sesType);

    // Send response
    if(taf_diagSecBackend_SendSesTypeCheckResp(sesTypeRef, TAF_DIAGSECBACKEND_SES_CONTROL_NO_ERROR)
            != LE_OK)
    {
        LE_ERROR("Send response error");
    }
}

// Callback function for session change notification message
void sesChangeMsgHandler
(
    taf_diagSecBackend_SessionType_t prev_session,
    taf_diagSecBackend_SessionType_t current_session,
    void* contextPtr
)
{
    LE_INFO("sesChangeMsgHandler start");

    LE_INFO("Previous sesType: %x",prev_session);
    LE_INFO("Current sesType: %x",current_session);
}

static void* diagReadDIDMsgThread(void* ctxPtr)
{
    taf_diagDIDBackend_ConnectService();

    LE_INFO("diagReadDIDMsgThread start");

    diagReadDIDMsgRef = taf_diagDIDBackend_AddReadDIDHandler(readDIDMsgHandler, NULL);
    LE_TEST_OK(diagReadDIDMsgRef != NULL,
            "Registered successfully for readDIDMsgHandler");

    le_sem_Post(semRef);
    le_event_RunLoop();

    LE_INFO("diagReadDIDMsgThread completed");
    return NULL;
}

static void* diagSecurityMsgThread(void* ctxPtr)
{
    taf_diagSecBackend_ConnectService();

    LE_INFO("diagSecurityMsgThread start");

    diagSesReqMsgRef = taf_diagSecBackend_AddSesTypeCheckHandler(sesReqMsgHandler, NULL);
    LE_TEST_OK(diagSesReqMsgRef != NULL,
            "Registered successfully for sesReqMsgHandler");

    diagSesChangeMsgRef = taf_diagSecBackend_AddSesChangeHandler(sesChangeMsgHandler, NULL);
    LE_TEST_OK(diagSesChangeMsgRef != NULL,
            "Registered successfully for sesChangeMsgHandler");

    le_sem_Post(semRef);
    le_event_RunLoop();

    LE_INFO("diagReadDIDMsgThread completed");
    return NULL;
}

COMPONENT_INIT
{
    LE_INFO("tafDiagBackendApp starting");

    semRef = le_sem_Create("SemRef", 0);

    // Create the diag ReadDID message handle thread to handle read/wriet DID request
    le_thread_Ref_t readDIDThreadRef = le_thread_Create("readDataIdTd",
            diagReadDIDMsgThread, NULL);

    le_thread_Start(readDIDThreadRef);
    le_sem_Wait(semRef);

    // Create the diag security message handle thread to handle sesCtrl request
    le_thread_Ref_t securityThreadRef = le_thread_Create("securityTd",
            diagSecurityMsgThread, NULL);

    le_thread_Start(securityThreadRef);
    le_sem_Wait(semRef);

    LE_INFO("tafDiagBackendApp completed");
}