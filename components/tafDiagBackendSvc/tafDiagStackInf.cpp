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
#include "tafDiagBackendSvr.hpp"

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

    DiagMsgPool = le_mem_CreatePool("DiagMsgPool", sizeof(taf_DiagReqMsg_t));
    DiagMsgRefMap = le_ref_CreateMap("DiagMsgRefMap", DEFAULT_MSG_REF_CNT);

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

    // Create Internal event for given service to report request msg.
    diagStack.DiagEventRespEvtId = le_event_CreateIdWithRefCounting("DiagEventResponseEvent");
    le_event_AddHandler("DiagEvent response Event Handler", diagStack.DiagEventRespEvtId,
            DiagEventRespEvtHandler);

    le_sem_Post(semRef);

    LE_DEBUG("Create event loop for Diag event");
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
    auto &diagBackend = taf_DiagBackend::GetInstance();

    if (Luc_AP_ServiceId == SID_READ_DATA_BY_IDENTIFIER)
    {
        std::lock_guard<std::mutex> lock(ResponseStatusMtx);
        if (ResponseStatus == READYFORNEWREQ)
        {
            taf_DiagReqMsg_t *diagReqMsgPtr = NULL;
            diagReqMsgPtr = (taf_DiagReqMsg_t*)le_mem_ForceAlloc(DiagMsgPool);
            memset(diagReqMsgPtr, 0, sizeof(taf_DiagReqMsg_t));
            LE_DEBUG("AP_ReadWriteDataByID diagReqMsgPtr address: %p", diagReqMsgPtr);

            LE_DEBUG("Requested DID: %hu",Lus_AP_RecordID);
            LE_DEBUG("Parameter given size : %hu",Size);
            LE_DEBUG("Parameter Position: %x",Position);
            diagReqMsgPtr->svcId = Luc_AP_ServiceId;
            diagReqMsgPtr->data[0] = Luc_AP_ServiceId;
            diagReqMsgPtr->data[1] = (Lus_AP_RecordID & 0xff00) >> 8;
            diagReqMsgPtr->data[2] = (Lus_AP_RecordID & 0xff);
            diagReqMsgPtr->dataLen = READ_DID_REQ_BASE_LEN;
            diagReqMsgPtr->ref = (taf_diagBackend_DiagInfRef_t)le_ref_CreateRef(
                    DiagMsgRefMap, diagReqMsgPtr);
            diagReqMsgPtr->link = LE_DLS_LINK_INIT;

            LE_DEBUG("Pushed data:");
            LE_DEBUG("diagReqMsgPtr->dataLen: %ld", sizeof(data));
            LE_DEBUG("1:%x",diagReqMsgPtr->data[1]);
            LE_DEBUG("2:%x",diagReqMsgPtr->data[2]);

            le_dls_Queue(&diagMsgList, &diagReqMsgPtr->link);

            // Report readDID request event.
            le_event_ReportWithRefCounting(diagBackend.DiagEventReqEvtId, diagReqMsgPtr);
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
            memset(data, 0, dataLen);
            dataLen = 0;
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
    else if (Luc_AP_ServiceId == SID_WRITE_DATA_BY_IDENTIFIER)
    {
        // Fill writeDID data request msg and report to requestEventID
        std::lock_guard<std::mutex> lock(ResponseStatusMtx);
        if (ResponseStatus == READYFORNEWREQ)
        {
            taf_DiagReqMsg_t *diagReqMsgPtr = NULL;
            diagReqMsgPtr = (taf_DiagReqMsg_t*)le_mem_ForceAlloc(DiagMsgPool);
            memset(diagReqMsgPtr, 0, sizeof(taf_DiagReqMsg_t));
            LE_INFO("AP_ReadWriteDataByID diagReqMsgPtr address: %p", diagReqMsgPtr);
            
            LE_DEBUG("Received write request");
            LE_DEBUG("DID :0x%X, Size : %d", Lus_AP_RecordID, Size);
            LE_DEBUG("Requested DID: %hu",Lus_AP_RecordID);
            LE_DEBUG("Parameter given size : %hu",Size);
            LE_DEBUG("Parameter Position: %x",Position);

            C_UBYTE dataptr[DATA_RECORD_REQ_MAX_SIZE];

            Tdd_DG_Status Ldd_DG_Status =  Callbacks::api->DG_Read_Buffer(dataptr, 3, Size);
            
            if(Ldd_DG_Status == DG_OK) {
                // Fill the readDID data request msg and report to requestEventID
                diagReqMsgPtr->svcId = Luc_AP_ServiceId;
                diagReqMsgPtr->data[0] = Luc_AP_ServiceId;
                diagReqMsgPtr->data[1] = (Lus_AP_RecordID & 0xff00) >> 8;
                diagReqMsgPtr->data[2] = (Lus_AP_RecordID & 0xff);
                size_t dataRecLen = WRITE_DID_REQ_BASE_LEN;

                for (int i = 0; i < Size; i++)
                {
                    LE_DEBUG("received data");
                    diagReqMsgPtr->data[i + WRITE_DID_REQ_BASE_LEN] = dataptr[i];
                    dataRecLen++; 
                    LE_DEBUG("received write DID data:%x",dataptr[i]);
                    LE_DEBUG("received write DID data:%d",int(dataptr[i]));
                }
                LE_DEBUG("dataLen:%d", int(dataRecLen));
                diagReqMsgPtr->dataLen = dataRecLen;
                diagReqMsgPtr->link = LE_DLS_LINK_INIT;

                diagReqMsgPtr->ref = (taf_diagBackend_DiagInfRef_t)le_ref_CreateRef(
                    DiagMsgRefMap, diagReqMsgPtr);

                le_dls_Queue(&diagMsgList, &diagReqMsgPtr->link);

                // Report readDID request event.
                le_event_ReportWithRefCounting(diagBackend.DiagEventReqEvtId, diagReqMsgPtr);
                LE_INFO("AP_ReadWriteDataByID callback reported");

                ResponseStatus = PROCESSING;
                return DG_RESP_PEND;
            }else{
                LE_INFO("Read Failed");
                return DG_NOT_OK;
            }

        }
        else if (ResponseStatus == PROCESSING)
        {
            return DG_RESP_PEND;
        }
        else if (ResponseStatus == FINISHED)
        {
            memset(data, 0, dataLen);
            dataLen = 0;
            ResponseStatus = READYFORNEWREQ;
            return DG_OK;
        }
        else
        {
            ResponseStatus = READYFORNEWREQ;
            return DG_NOT_OK;
        }
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
    auto &diagBackend = taf_DiagBackend::GetInstance();

    std::lock_guard<std::mutex> lock(ResponseStatusMtx);
    if (ResponseStatus == READYFORNEWREQ)
    {
        taf_DiagReqMsg_t *diagReqMsgPtr = NULL;
        diagReqMsgPtr = (taf_DiagReqMsg_t *)le_mem_ForceAlloc(DiagMsgPool);
        memset(diagReqMsgPtr, 0, sizeof(taf_DiagReqMsg_t));

        diagReqMsgPtr->svcId = SID_DIAGNOSTIC_SESSION_CONTROL;
        diagReqMsgPtr->data[0] = SID_DIAGNOSTIC_SESSION_CONTROL;
        diagReqMsgPtr->data[1] = Luc_DG_DiagSess;
        diagReqMsgPtr->dataLen = SES_CONTROL_REQ_BASE_LEN;
        diagReqMsgPtr->ref = (taf_diagBackend_DiagInfRef_t)le_ref_CreateRef(
            DiagMsgRefMap, diagReqMsgPtr);
        diagReqMsgPtr->link = LE_DLS_LINK_INIT;

        le_dls_Queue(&diagMsgList, &diagReqMsgPtr->link);
        
        // Report session chnage notification event.
        le_event_ReportWithRefCounting(diagBackend.DiagEventReqEvtId, diagReqMsgPtr);
        
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
        memset(data, 0, dataLen);
	dataLen = 0;
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
    auto &diagBackend = taf_DiagBackend::GetInstance();

    taf_DiagReqMsg_t *diagReqMsgPtr = NULL;
    diagReqMsgPtr = (taf_DiagReqMsg_t *)le_mem_ForceAlloc(DiagMsgPool);
    memset(diagReqMsgPtr, 0, sizeof(taf_DiagReqMsg_t));

    diagReqMsgPtr->svcId = SID_DIAGNOSTIC_SESSION_CHANGE;
    diagReqMsgPtr->data[0] = SID_DIAGNOSTIC_SESSION_CHANGE;
    diagReqMsgPtr->data[1] = prevLuc_DG_DiagSess;
    diagReqMsgPtr->data[2] = Luc_DG_DiagSess;
    diagReqMsgPtr->dataLen = SES_CHANGE_REQ_BASE_LEN;
    diagReqMsgPtr->ref = (taf_diagBackend_DiagInfRef_t)le_ref_CreateRef(
        DiagMsgRefMap, diagReqMsgPtr);
    diagReqMsgPtr->link = LE_DLS_LINK_INIT;

    le_dls_Queue(&diagMsgList, &diagReqMsgPtr->link);
    
    // Report session chnage notification event.
    le_event_ReportWithRefCounting(diagBackend.DiagEventReqEvtId, diagReqMsgPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Diag Event response handler.
 */
//--------------------------------------------------------------------------------------------------
void taf_DiagStack::DiagEventRespEvtHandler
(
    void* reqPtr
)
{
    LE_INFO("DiagEventRespEvtHandler!");

    taf_DiagRespMsg_t* diagEventRespMsgPtr = (taf_DiagRespMsg_t *)reqPtr;
    auto &cb = taf_DiagStack::GetInstance();

    // Check the reference to find the previous request.
    taf_DiagReqMsg_t *diagEventReqMsgPtr =
            (taf_DiagReqMsg_t*)le_ref_Lookup(cb.DiagMsgRefMap, diagEventRespMsgPtr->ref);
    if (diagEventRespMsgPtr == NULL)
    {
        std::lock_guard<std::mutex> lock(cb.ResponseStatusMtx);
        cb.ResponseStatus = FAILED;
        LE_ERROR("Cannot find the reqMsgRef");
        return ;
    }

    // Send the response data
    if (diagEventRespMsgPtr->errCode == 0)
    {
        LE_DEBUG("Send Positive response");
        // Get response data and send positive response.
        if(diagEventRespMsgPtr->dataRec != nullptr && diagEventRespMsgPtr->dataLen != 0){
            std::memcpy(cb.data, diagEventRespMsgPtr->dataRec, sizeof(diagEventRespMsgPtr->dataRec));
            cb.dataLen = diagEventRespMsgPtr->dataLen;
            for(int k=0; k < int(cb.dataLen); k++){
                LE_DEBUG("cb.data[%d]: %x",k,cb.data[k]);
            }
        }
	std::lock_guard<std::mutex> lock(cb.ResponseStatusMtx);
        cb.ResponseStatus = FINISHED;
        LE_DEBUG("Positive response sent");
    }
    else
    {
        LE_DEBUG("Send NRC response");
        //Negative response.
        std::lock_guard<std::mutex> lock(cb.ResponseStatusMtx);
        cb.nrcValue = diagEventRespMsgPtr->errCode;
        cb.ResponseStatus = FAILED;
        LE_DEBUG("NRC response Sent");
    }

    // Remove the message from the list.
    le_dls_Remove(&(cb.diagMsgList), &(diagEventReqMsgPtr->link));

    // Free the message
    le_ref_DeleteRef(cb.DiagMsgRefMap, diagEventReqMsgPtr->ref);

    le_mem_Release(diagEventReqMsgPtr);

    diagEventRespMsgPtr->ref = NULL;
    le_mem_Release(diagEventRespMsgPtr);

    LE_DEBUG("sent and allocated memory freed!");
}
