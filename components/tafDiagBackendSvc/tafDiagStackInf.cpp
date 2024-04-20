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

#include "tafDiagStackInf.hpp"
#include "tafDIDBackendSvr.hpp"
#include "tafSecBackendSvr.hpp"

#include "DiagNode.h"
#include "ecu_types.h"
#include "callbacks_der.h"
#include "dg_struc.h"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance.
 */
//--------------------------------------------------------------------------------------------------
taf_DiagStack &taf_DiagStack::GetInstance()
{
    static taf_DiagStack instance;

    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_DiagStack::Init(void)
{
    LE_INFO("taf_DiagStack Init!");

    // Initialize DK module function
    auto &cb = taf_DiagStack::GetInstance();

    DiagNode<ECU1> ecu1;
    ecu1.Start(cb,"Gateway",false);
    ecu1.Run(true);

    // Create memory pools for request.
    ReadDIDMsgPool = le_mem_CreatePool("ReadDIDMsgPool", sizeof(taf_ReadDIDReqMsg_t));
    SesCtrlMsgPool = le_mem_CreatePool("SesCtrlMsgPool", sizeof(taf_SesCtrlReqMsg_t));
    SesChangeMsgPool = le_mem_CreatePool("SesChangeMsgPool", sizeof(taf_SesCtrlChangeMsg_t));

    // Create reference maps
    ReadDIDMsgRefMap = le_ref_CreateMap("ReadDIDMsgRefMap", DEFAULT_MSG_REF_CNT);
    DSCMsgRefMap = le_ref_CreateMap("DSCMsgRefMap", DEFAULT_MSG_REF_CNT);

    //Create the semaphore
    le_sem_Ref_t semRef = le_sem_Create("SmThreadSem", 0);

    // Create the event handle thread
    RespEvtThreadRef = le_thread_Create("RespEvtThread", RespEventThread, (void*)semRef);
    le_thread_Start(RespEvtThreadRef);

    le_sem_Wait(semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response event thraed.
 */
//--------------------------------------------------------------------------------------------------
void* taf_DiagStack::RespEventThread
(
    void* contextPtr
)
{
    LE_INFO("ReqEventThread!");
    auto &diagStack = taf_DiagStack::GetInstance();

    le_sem_Ref_t semRef = (le_sem_Ref_t)contextPtr;

    // Create Internal event for readDID to report request msg.
    diagStack.ReadDIDRespEvtId = le_event_CreateIdWithRefCounting("ReadDIDResponseEvent");
    le_event_AddHandler("ReadDID response Event Handler", diagStack.ReadDIDRespEvtId,
            ReadDIDRespEvtHandler);

    // Create Internal event for sessionCtrl to report request msg.
    diagStack.SesCtrlRespEvtId = le_event_CreateIdWithRefCounting("SesCtrlResponseEvent");
    le_event_AddHandler("Session ctrl response Event Handler", diagStack.SesCtrlRespEvtId,
            SesCtrlRespEvtHandler);

    le_sem_Post(semRef);

    LE_DEBUG("Create event loop for DID event");
    le_event_RunLoop();

    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * Read/Write Diagnostic Data Identifier callback
 */
//-------------------------------------------------------------------------------------------------
Tdd_DG_Status taf_DiagStack::AP_ReadWriteDataByID
(
    C_USHORT Lus_AP_RecordID,
    C_UBYTE Luc_AP_ServiceId,
    C_UBYTE Position,
    C_USHORT Size
)
{
    LE_INFO("AP_ReadWriteDataByID callback!");
    auto &didBackend = taf_DIDBackend::GetInstance();

    if (Luc_AP_ServiceId == 0x22)
    {
        std::lock_guard<std::mutex> lock(ResponseStatusMtx);
        if (ResponseStatus == READYFORNEWREQ)
        {
            taf_ReadDIDReqMsg_t *readDIDReqMsgPtr = NULL;
            readDIDReqMsgPtr = (taf_ReadDIDReqMsg_t*)le_mem_ForceAlloc(ReadDIDMsgPool);
            memset(readDIDReqMsgPtr, 0, sizeof(taf_ReadDIDReqMsg_t));
            LE_DEBUG("AP_ReadWriteDataByID readDIDReqMsgPtr address: %p", readDIDReqMsgPtr);

            // Fill the readDID data request msg and report to requestEventID
            readDIDReqMsgPtr->svcId = Luc_AP_ServiceId;
            readDIDReqMsgPtr->readDID = Lus_AP_RecordID;
            readDIDReqMsgPtr->link = LE_DLS_LINK_INIT;
            readDIDReqMsgPtr->readDIDRef = (taf_diagDIDBackend_ReadDIDRef_t)le_ref_CreateRef(
                    ReadDIDMsgRefMap, readDIDReqMsgPtr);

            le_dls_Queue(&readDIDList, &readDIDReqMsgPtr->link);

            // Report readDID request event.
            le_event_ReportWithRefCounting(didBackend.ReadDIDReqEvtId, readDIDReqMsgPtr);
            LE_INFO("AP_ReadWriteDataByID callback reported");

            //
            ResponseStatus = PROCESSING;
            return DG_RESP_PEND;
        }
        else if (ResponseStatus == PROCESSING)
        {
            return DG_RESP_PEND;
        }
        else if (ResponseStatus == FINISHED)
        {
            // write buffer
            Callbacks::api->DG_Write_Buffer(data, dataLen);
            ResponseStatus = READYFORNEWREQ;
            return DG_OK;
        }
        else
        {
            Callbacks::api->DG_Negative_Response_Code(nrcValue);
            ResponseStatus = READYFORNEWREQ;
            return DG_NOT_OK;
        }
    }
    else if (Luc_AP_ServiceId == 0x2E)
    {
        // Not implemented.
        return DG_NOT_OK;
    }

    return DG_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Session control request callback.
 */
//--------------------------------------------------------------------------------------------------
Tdd_DG_Status taf_DiagStack::AP_Condition_For_DiagSess
(
    Tdd_DG_DiagSession Luc_DG_DiagSess
)
{
    LE_INFO("AP_Condition_For_DiagSess callback!");
    auto &SecBackend = taf_SecBackend::GetInstance();

    std::lock_guard<std::mutex> lock(ResponseStatusMtx);
    if (ResponseStatus == READYFORNEWREQ)
    {
        taf_SesCtrlReqMsg_t *sesCtrlReqMsgPtr = NULL;
        sesCtrlReqMsgPtr = (taf_SesCtrlReqMsg_t *)le_mem_ForceAlloc(SesCtrlMsgPool);
        memset(sesCtrlReqMsgPtr, 0, sizeof(taf_SesCtrlReqMsg_t));

        sesCtrlReqMsgPtr->svcId = 0x10;
        sesCtrlReqMsgPtr->sesType = (taf_diagSecBackend_SessionType_t)Luc_DG_DiagSess;
        sesCtrlReqMsgPtr->link = LE_DLS_LINK_INIT;
        sesCtrlReqMsgPtr->sesTypeRef = (taf_diagSecBackend_SesTypeCheckRef_t)le_ref_CreateRef(
            DSCMsgRefMap, sesCtrlReqMsgPtr);

        le_dls_Queue(&DSCList, &sesCtrlReqMsgPtr->link);

        // Report readDID request event.
        le_event_ReportWithRefCounting(SecBackend.SesCtrlReqEvtId, sesCtrlReqMsgPtr);
        LE_DEBUG("AP_Condition_For_DiagSess callback reported");

        ResponseStatus = PROCESSING;
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == PROCESSING)
    {
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == FINISHED)
    {
        ResponseStatus = READYFORNEWREQ;
        return DG_OK;
    }
    else
    {
        Callbacks::api->DG_Negative_Response_Code(nrcValue);
        ResponseStatus = READYFORNEWREQ;
        return DG_NOT_OK;
    }
    return DG_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Session control change notification callback.
 */
//--------------------------------------------------------------------------------------------------
void taf_DiagStack::AP_SessionChange
(
    Tdd_DG_DiagSession prevLuc_DG_DiagSess,
    Tdd_DG_DiagSession Luc_DG_DiagSess
)
{
    LE_INFO("AP_SessionChange callback!");
    auto &SecBackend = taf_SecBackend::GetInstance();

    taf_SesCtrlChangeMsg_t *sesChanegeMsgPtr = NULL;
    sesChanegeMsgPtr = (taf_SesCtrlChangeMsg_t *)le_mem_ForceAlloc(SesChangeMsgPool);
    memset(sesChanegeMsgPtr, 0, sizeof(taf_SesCtrlChangeMsg_t));

    // Fill the previous and current seesion type
    sesChanegeMsgPtr->prev_session = (taf_diagSecBackend_SessionType_t)prevLuc_DG_DiagSess;
    sesChanegeMsgPtr->current_session = (taf_diagSecBackend_SessionType_t)Luc_DG_DiagSess;

    // Report session chnage notification event.
    le_event_ReportWithRefCounting(SecBackend.SesChangeEvtId, sesChanegeMsgPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read DID response handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_DiagStack::ReadDIDRespEvtHandler
(
    void* reqPtr
)
{
    LE_INFO("ReadDIDRespEvtHandler!");

    taf_ReadDIDRespMsg_t* readDIDRespMsgPtr = (taf_ReadDIDRespMsg_t *)reqPtr;
    auto &cb = taf_DiagStack::GetInstance();

    // Check the reference to find the previous request.
    taf_ReadDIDReqMsg_t *readDIDReqMsgPtr =
            (taf_ReadDIDReqMsg_t*)le_ref_Lookup(cb.ReadDIDMsgRefMap, readDIDRespMsgPtr->readDIDRef);
    if (readDIDReqMsgPtr == NULL)
    {
        std::lock_guard<std::mutex> lock(cb.ResponseStatusMtx);
        cb.ResponseStatus = FAILED;
        LE_ERROR("Cannot find the reqMsgRef");
        return ;
    }

    // Send the response data
    if (readDIDRespMsgPtr->errCode == 0)
    {
        LE_DEBUG("Send Positive response");
        // Get response data and send positive response.
        std::memcpy(cb.data, readDIDRespMsgPtr->dataRec, sizeof(readDIDRespMsgPtr->dataRec));
        cb.dataLen = readDIDRespMsgPtr->dataRecLen;
        std::lock_guard<std::mutex> lock(cb.ResponseStatusMtx);
        cb.ResponseStatus = FINISHED;
        LE_DEBUG("Positive response sent");
    }
    else
    {
        LE_DEBUG("Send NRC response");
        //Negative response.
        std::lock_guard<std::mutex> lock(cb.ResponseStatusMtx);
        cb.nrcValue = readDIDRespMsgPtr->errCode;
        cb.ResponseStatus = FAILED;
        LE_DEBUG("NRC response Sent");
    }

    // Remove the message from the list.
    le_dls_Remove(&(cb.readDIDList), &(readDIDReqMsgPtr->link));

    // Free the message
    le_ref_DeleteRef(cb.ReadDIDMsgRefMap, readDIDReqMsgPtr->readDIDRef);

    le_mem_Release(readDIDReqMsgPtr);

    readDIDRespMsgPtr->readDIDRef = NULL;
    le_mem_Release(readDIDRespMsgPtr);

    LE_DEBUG("sent and allocated memory freed!");
}

//--------------------------------------------------------------------------------------------------
/**
 * Session control response handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_DiagStack::SesCtrlRespEvtHandler
(
    void* reqPtr
)
{
    LE_INFO("SesCtrlRespEvtHandler!");

    taf_SesCtrlRespMsg_t *sesCrtlRespMsgPtr = (taf_SesCtrlRespMsg_t *)reqPtr;
    auto &cb = taf_DiagStack::GetInstance();

    // Check the reference to find the previous request.
    taf_SesCtrlReqMsg_t *sesCtrlReqMsgPtr =
        (taf_SesCtrlReqMsg_t *)le_ref_Lookup(cb.DSCMsgRefMap, sesCrtlRespMsgPtr->sesTypeRef);
    if (sesCtrlReqMsgPtr == NULL)
    {
        std::lock_guard<std::mutex> lock(cb.ResponseStatusMtx);
        cb.ResponseStatus = FAILED;
        LE_ERROR("Cannot find the reqMsgRef");
        return ;
    }

    if (sesCrtlRespMsgPtr->errCode == 0)
    {
        LE_DEBUG("Send Positive response");
        // Send positive response
        std::lock_guard<std::mutex> lock(cb.ResponseStatusMtx);
        cb.ResponseStatus = FINISHED;
        LE_DEBUG("Positive response sent");
    }
    else
    {
        LE_DEBUG("Send NRC response");
        std::lock_guard<std::mutex> lock(cb.ResponseStatusMtx);
        cb.nrcValue = sesCrtlRespMsgPtr->errCode;
        cb.ResponseStatus = FAILED;
    }

    // Remove the message from the list.
    le_dls_Remove(&(cb.DSCList), &(sesCtrlReqMsgPtr->link));

    // Free the message
    le_ref_DeleteRef(cb.DSCMsgRefMap, sesCtrlReqMsgPtr->sesTypeRef);

    le_mem_Release(sesCtrlReqMsgPtr);

    sesCrtlRespMsgPtr->sesTypeRef = NULL;
    le_mem_Release(sesCrtlRespMsgPtr);

    LE_DEBUG("sent and allocated memory freed!");
}