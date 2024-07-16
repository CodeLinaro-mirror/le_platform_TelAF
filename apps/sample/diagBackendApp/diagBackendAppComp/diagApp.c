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

static le_sem_Ref_t semRef;
static taf_diagBackend_DiagEventHandlerRef_t diagEventHandler = NULL;


// Callback function for read dataID request message
void diagEventMsgHandler
(
    taf_diagBackend_DiagInfRef_t rxMsgRef,
    const taf_diagBackend_AddrInfo_t* addrInfoPtr,
    uint8_t svcID,
    const uint8_t* dataPtr,
    size_t dataLen,
    void* contextPtr
)
{
    LE_INFO("diagEventMsgHandler start");

    uint8_t sendBuf[TAF_DIAGBACKEND_MAX_PAYLOAD_SIZE];
    size_t sendBufLen = 0;

    LE_INFO("diagEventMsgHandler receive ref: %p",rxMsgRef);
    LE_INFO("diagEventMsgHandler receive svcId: %x",svcID);
 
    if(svcID == 0x22){
        // Sending response for some DID, which are defined/hardcoded in config.h file.
        LE_INFO("diagEvent for ReadDID");
        LE_INFO("Received data length: %zu", dataLen);
        uint8_t dataPos = 1; // Skip Service id
        uint16_t dataId;
        dataId = (dataPtr[dataPos]  << 8) + dataPtr[dataPos + 1];
        dataPos += 2;
        LE_INFO("Received dataID: %x", dataId);
        if (dataId == 0x1122)
        {
            LE_INFO("Came for 0x1122 Identifier");
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
        if(taf_diagBackend_SendResp( rxMsgRef, addrInfoPtr, svcID, 0, sendBuf, sendBufLen ) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
    }
    else if(svcID == 0x2E)
    {
        LE_INFO("diagEvent for WriteDID");
        LE_INFO("Receive ref: %p", rxMsgRef);

        // Send response
        if(taf_diagBackend_SendResp(rxMsgRef, addrInfoPtr, svcID, 0, NULL, 0) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
    }
    else if(svcID == 0x10)
    {
        LE_INFO("diagEvent for session control request");
        LE_INFO("Receive ref: %p", rxMsgRef);
        LE_INFO("Requested sesType: %x", dataPtr[1]);

        // Send response
        if(taf_diagBackend_SendResp(rxMsgRef, addrInfoPtr, svcID, 0, NULL, 0) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
    }
    else
    {
        LE_INFO("diagEvent for callback notification for session control");
        LE_INFO("Previous sesType: %x",dataPtr[1]);
        LE_INFO("Current sesType: %x",dataPtr[2]);
    }

    LE_INFO("diagEventMsgHandler completed");
    memset(sendBuf, 0, TAF_DIAGBACKEND_MAX_PAYLOAD_SIZE);
    sendBufLen = 0;

    return;
}


static void* diagEventMsgThread(void* ctxPtr)
{
    taf_diagBackend_ConnectService();

    LE_INFO("diagEventMsgThread start");

    diagEventHandler = taf_diagBackend_AddDiagEventHandler(diagEventMsgHandler, NULL);
    LE_TEST_OK(diagEventHandler != NULL,
            "Registered successfully for readDIDMsgHandler");

    le_sem_Post(semRef);
    le_event_RunLoop();

    LE_INFO("diagEventMsgThread completed");
    return NULL;
}

COMPONENT_INIT
{
    LE_INFO("tafDiagBackendApp starting");

    semRef = le_sem_Create("SemRef", 0);

    // Create the diag ReadDID message handle thread to handle read/wriet DID request
    le_thread_Ref_t diagEventThreadRef = le_thread_Create("diagEventTd",
            diagEventMsgThread, NULL);

    le_thread_Start(diagEventThreadRef);
    le_sem_Wait(semRef);

    LE_INFO("tafDiagBackendApp completed");
}
