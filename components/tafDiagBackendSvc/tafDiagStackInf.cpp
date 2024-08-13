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
#include "StackConfig.h"
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

    try
    {
        config::StackConfig configInstance;
        configInstance.Initialize("/data/kpit/uds_server_main.json");
    }
    catch (const std::exception& e)
    {
        LE_FATAL("json file is not present");
    }

    // Initialize DK module function
    auto &cb = taf_DiagStack::GetInstance();
    DiagNode<DynamicType> ecu1;
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
            C_UBYTE dataRec[DATA_RECORD_REQ_MAX_SIZE] = {0};

            Tdd_DG_Status Ldd_DG_Status =  Callbacks::api->DG_Read_Buffer(dataRec, WRITE_DID_REQ_BASE_LEN, Size);

            if(Ldd_DG_Status == DG_OK)
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

                // Fill the writeDID data request msg and report to requestEventID
                diagReqMsgPtr->svcId = Luc_AP_ServiceId;
                diagReqMsgPtr->data[0] = Luc_AP_ServiceId;
                diagReqMsgPtr->data[1] = (Lus_AP_RecordID & 0xff00) >> 8;
                diagReqMsgPtr->data[2] = (Lus_AP_RecordID & 0xff);
                size_t dataRecLen = WRITE_DID_REQ_BASE_LEN;

                for (int i = 0; i < Size; i++)
                {
                    LE_DEBUG("received data");
                    diagReqMsgPtr->data[i + WRITE_DID_REQ_BASE_LEN] = dataRec[i];
                    dataRecLen++;
                    LE_DEBUG("received write DID data:%x",dataRec[i]);
                    LE_DEBUG("received write DID data:%d",int(dataRec[i]));
                }
                LE_DEBUG("dataLen:%d", int(dataRecLen));
                diagReqMsgPtr->dataLen = dataRecLen;
                diagReqMsgPtr->link = LE_DLS_LINK_INIT;

                diagReqMsgPtr->ref = (taf_diagBackend_DiagInfRef_t)le_ref_CreateRef(
                    DiagMsgRefMap, diagReqMsgPtr);

                le_dls_Queue(&diagMsgList, &diagReqMsgPtr->link);

                // Report writeDID request event.
                le_event_ReportWithRefCounting(diagBackend.DiagEventReqEvtId, diagReqMsgPtr);
                LE_INFO("AP_ReadWriteDataByID callback reported");

                ResponseStatus = PROCESSING;
                return DG_RESP_PEND;
            }else{
                LE_ERROR("Read Data Record Failed");
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
            Callbacks::api->DG_Negative_Response_Code(nrcValue);
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
        LE_DEBUG("AP_Condition_For_DiagSess callback ----  READYFORNEWREQ");
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
        LE_DEBUG("AP_Condition_For_DiagSess callback ----  PROCESSING");
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == FINISHED)
    {
        LE_DEBUG("AP_Condition_For_DiagSess callback ----  FINISHED");
        memset(data, 0, dataLen);
        dataLen = 0;
        ResponseStatus = READYFORNEWREQ;
        return DG_OK;
    }
    else
    {
        LE_DEBUG("AP_Condition_For_DiagSess callback ----  Negative Response Sent");
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
 * Get Seed Value callback.
 */
//---------------//-----------------------------------------------------------------------------------
Tdd_DG_Status taf_DiagStack::AP_Get_SeedValue
(
    C_UBYTE *SeedValue,
    C_USHORT SeedLength,
    C_UBYTE SecurityLevel,
    C_UBYTE* SecurityAccessDataRec,
    C_USHORT SecurityAccessDataRecLen
)
{

    LE_INFO("AP_Get_SeedValue callback!");
    auto &diagBackend = taf_DiagBackend::GetInstance();

    std::lock_guard<std::mutex> lock(ResponseStatusMtx);
    if (ResponseStatus == READYFORNEWREQ)
    {
        LE_DEBUG("AP_Get_SeedValue callback ---  READYFORNEWREQ");
        taf_DiagReqMsg_t *diagReqMsgPtr = NULL;
        diagReqMsgPtr = (taf_DiagReqMsg_t *)le_mem_ForceAlloc(DiagMsgPool);
        memset(diagReqMsgPtr, 0, sizeof(taf_DiagReqMsg_t));

        diagReqMsgPtr->svcId = SID_SECURITY_ACCESS;
        diagReqMsgPtr->data[0] = SID_SECURITY_ACCESS;
        diagReqMsgPtr->data[1] = SecurityLevel;
        diagReqMsgPtr->dataLen = SECURITY_ACCESS_REQ_BASE_LEN;
        diagReqMsgPtr->link = LE_DLS_LINK_INIT;
        diagReqMsgPtr->ref = (taf_diagBackend_DiagInfRef_t)le_ref_CreateRef(
            DiagMsgRefMap, diagReqMsgPtr);

        le_dls_Queue(&diagMsgList, &diagReqMsgPtr->link);

        // Report getSeedValue request event.
        le_event_ReportWithRefCounting(diagBackend.DiagEventReqEvtId, diagReqMsgPtr);
        LE_DEBUG("AP_Get_SeedValue callback reported");

        ResponseStatus = PROCESSING;
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == PROCESSING)
    {
        LE_DEBUG("AP_Get_SeedValue callback ---  PROCESSING");
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == FINISHED)
    {
        LE_DEBUG("AP_Get_SeedValue callback ---  FINISHED");
        if(SeedValue != nullptr && SeedLength != DK_ZERO){
            memcpy(SeedValue, data, dataLen);
            SeedLength = dataLen;
            LE_INFO("AP_Get_SeedValue ------ Response Status is FINISHED");
            LE_INFO("SeedLen: %d",int(dataLen));
            for(int k=0; k < int(SeedLength); k++){
                LE_INFO("SeedValue[%d]: %x",k,SeedValue[k]);
            }
            ResponseStatus = READYFORNEWREQ;
            return DG_OK;
        }else{
            LE_DEBUG("The seedValue pointer is null");
            ResponseStatus = READYFORNEWREQ;
            return DG_NOT_OK;
        }
        memset(data, 0, dataLen);
        dataLen = 0;
    }
    else
    {
        LE_DEBUG("AP_Get_SeedValue callback ----  Negative Response Sent");
        Callbacks::api->DG_Negative_Response_Code(nrcValue);
        ResponseStatus = READYFORNEWREQ;
        return DG_NOT_OK;
    }
    return DG_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Supplier Security Algorithm callback.
 */
//--------------------------------------------------------------------------------------------------
Tdd_DG_Status taf_DiagStack::AP_SuppSecAlg
(
    C_UBYTE *SeedValue,
    C_USHORT SeedLength,
    C_UBYTE *Key,
    C_USHORT KeyLength,
    C_UBYTE SecurityLevel
)
{

    LE_INFO("AP_SuppSecAlg callback!");
    auto &diagBackend = taf_DiagBackend::GetInstance();

    std::lock_guard<std::mutex> lock(ResponseStatusMtx);
    if (ResponseStatus == READYFORNEWREQ)
    {
        LE_DEBUG("AP_SuppSecAlg callback ---  READYFORNEWREQ");
        taf_DiagReqMsg_t *diagReqMsgPtr = NULL;
        diagReqMsgPtr = (taf_DiagReqMsg_t *)le_mem_ForceAlloc(DiagMsgPool);
        memset(diagReqMsgPtr, 0, sizeof(taf_DiagReqMsg_t));

        diagReqMsgPtr->svcId = SID_SECURITY_ACCESS;
        diagReqMsgPtr->data[0] = SID_SECURITY_ACCESS;
        diagReqMsgPtr->data[1] = SecurityLevel;

        size_t dataRecLen = SECURITY_ACCESS_REQ_BASE_LEN;

        if(Key != nullptr && KeyLength != DK_ZERO)
        {
            memcpy(diagReqMsgPtr->data + SECURITY_ACCESS_REQ_BASE_LEN, Key, KeyLength);
        }
        else
        {
            LE_DEBUG("The key pointer is null");
            ResponseStatus = READYFORNEWREQ;
            return DG_NOT_OK;
        }

        diagReqMsgPtr->dataLen = dataRecLen + KeyLength;
        diagReqMsgPtr->link = LE_DLS_LINK_INIT;
        diagReqMsgPtr->ref = (taf_diagBackend_DiagInfRef_t)le_ref_CreateRef(
            DiagMsgRefMap, diagReqMsgPtr);

        le_dls_Queue(&diagMsgList, &diagReqMsgPtr->link);

        // Report AP_SuppSecAlg request event.
        le_event_ReportWithRefCounting(diagBackend.DiagEventReqEvtId, diagReqMsgPtr);
        LE_DEBUG("AP_SuppSecAlg callback reported");

        ResponseStatus = PROCESSING;
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == PROCESSING)
    {
        LE_DEBUG("AP_SuppSecAlg callback ---  PROCESSING");
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == FINISHED)
    {
        LE_DEBUG("AP_SuppSecAlg callback ---  FINISHED");
        memset(data, 0, dataLen);
        dataLen = 0;
        ResponseStatus = READYFORNEWREQ;
        return DG_OK;
    }
    else
    {
        LE_DEBUG("AP_SuppSecAlg callback ---  Negative Response");
        Callbacks::api->DG_Negative_Response_Code(nrcValue);
        ResponseStatus = READYFORNEWREQ;
        return DG_NOT_OK;
    }
    return DG_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Request File Transfer callback.
 */
//--------------------------------------------------------------------------------------------------
Tdd_DG_Status taf_DiagStack::AP_RequestFileTransfer
(
    C_UBYTE modeOfOperation,
    C_USHORT filePathAndNameLength,
    C_UBYTE * filePathAndName,
    C_UBYTE fileSizeParameterLength,
    C_UINT32 fileSizeUncompressed,
    C_UINT32 fileSizeCompressed,
    C_UBYTE dataFormatId
)
{

    LE_INFO("AP_RequestFileTransfer callback!");
    auto &diagBackend = taf_DiagBackend::GetInstance();

    std::lock_guard<std::mutex> lock(ResponseStatusMtx);
    LE_INFO("modeOfOperation: %x",modeOfOperation);
    LE_INFO("filePathAndNameLength: %hu", filePathAndNameLength);
    LE_INFO("filePathAndName: %s",filePathAndName);
    LE_INFO("fileSizeParameterLength: %x",fileSizeParameterLength);
    LE_INFO("fileSizeUncompressed: %d",int(fileSizeUncompressed));
    LE_INFO("fileSizeCompressed:%d",int(fileSizeCompressed));
    LE_INFO("dataFormatId:%x",dataFormatId);

    if (ResponseStatus == READYFORNEWREQ)
    {
        LE_DEBUG("AP_RequestFileTransfer callback ---  READYFORNEWREQ");
        taf_DiagReqMsg_t *diagReqMsgPtr = NULL;
        diagReqMsgPtr = (taf_DiagReqMsg_t *)le_mem_ForceAlloc(DiagMsgPool);
        memset(diagReqMsgPtr, 0, sizeof(taf_DiagReqMsg_t));

        size_t dataRecLen = REQUEST_FILE_TRANSFER_REQ_BASE_LEN;

        diagReqMsgPtr->svcId = SID_REQUEST_FILE_TRANSFER;
        diagReqMsgPtr->data[0] = SID_REQUEST_FILE_TRANSFER;
        diagReqMsgPtr->data[1] = modeOfOperation;
        diagReqMsgPtr->data[2] = (filePathAndNameLength & 0xff00) >> 8;
        diagReqMsgPtr->data[3] = (filePathAndNameLength & 0xff);

        memcpy(diagReqMsgPtr->data + dataRecLen, filePathAndName, filePathAndNameLength);
        dataRecLen += filePathAndNameLength;

        if(modeOfOperation == 0x01 || modeOfOperation == 0x03 || modeOfOperation == 0x06)
        {
            diagReqMsgPtr->data[dataRecLen] = dataFormatId;
            dataRecLen++;

            diagReqMsgPtr->data[dataRecLen] = fileSizeParameterLength;
            dataRecLen++;

            for (int i = 0; i < fileSizeParameterLength; i++)
            {
                diagReqMsgPtr->data[dataRecLen] =
                      (fileSizeUncompressed>> (8 * (fileSizeParameterLength - 1 - i))) & 0xFF;
                dataRecLen++;
            }

            for (int i = 0; i < fileSizeParameterLength; i++)
            {
                diagReqMsgPtr->data[dataRecLen] =
                      (fileSizeCompressed>> (8 * (fileSizeParameterLength - 1 - i))) & 0xFF;
                dataRecLen++;
            }
        }
        else if(modeOfOperation == 0x04)
        {
            diagReqMsgPtr->data[dataRecLen] = dataFormatId;
            dataRecLen++;
        }

        LE_INFO("dataLen:%d", int(dataRecLen));
        diagReqMsgPtr->dataLen = dataRecLen;
        diagReqMsgPtr->link = LE_DLS_LINK_INIT;
        diagReqMsgPtr->ref = (taf_diagBackend_DiagInfRef_t)le_ref_CreateRef(
                DiagMsgRefMap, diagReqMsgPtr);

        le_dls_Queue(&diagMsgList, &diagReqMsgPtr->link);

        // Report Request File Transfer event.
        le_event_ReportWithRefCounting(diagBackend.DiagEventReqEvtId, diagReqMsgPtr);
        LE_DEBUG("AP_RequestFileTransfer callback reported");

        ResponseStatus = PROCESSING;
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == PROCESSING)
    {
        LE_DEBUG("AP_RequestFileTransfer callback ---  PROCESSING");
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == FINISHED)
    {
        LE_DEBUG("AP_RequestFileTransfer callback ----  FINISHED");
        if(modeOfOperation == 0x04 || modeOfOperation == 0x05)
        {
            Callbacks::api->DG_Write_Buffer(data, dataLen);
        }
        else if(modeOfOperation == 0x06)
        {
            uint64_t filePos = 0;
            for (int i = 0; i < 8; ++i)
            {
                filePos |= static_cast<uint64_t>(data[i]) << (i * 8);
            }
            Callbacks::api->DG_Write_File_Position_Resume_File_Trfr(filePos);
        }
        memset(data, 0, dataLen);
        dataLen = 0;
        ResponseStatus = READYFORNEWREQ;
        return DG_OK;
    }
    else
    {
        LE_DEBUG("AP_RequestFileTransfer callback ----  Negative Response Sent");
        Callbacks::api->DG_Negative_Response_Code(nrcValue);
        ResponseStatus = READYFORNEWREQ;
        return DG_NOT_OK;
    }
    return DG_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Transfer Data Download request callback.
 */
//--------------------------------------------------------------------------------------------------
Tdd_DG_Status  taf_DiagStack::AP_TransferData_Dwnld
(
    C_UBYTE seqCounter,
    C_UBYTE* dwnldDataPtr,
    C_UINT32 dataLen
)
{

    LE_INFO("AP_TransferData_Dwnld callback!");
    auto &diagBackend = taf_DiagBackend::GetInstance();

    std::lock_guard<std::mutex> lock(ResponseStatusMtx);
    if (ResponseStatus == READYFORNEWREQ)
    {
        LE_DEBUG("AP_TransferData_Dwnld callback ---  READYFORNEWREQ");
        taf_DiagReqMsg_t *diagReqMsgPtr = NULL;
        diagReqMsgPtr = (taf_DiagReqMsg_t *)le_mem_ForceAlloc(DiagMsgPool);
        memset(diagReqMsgPtr, 0, sizeof(taf_DiagReqMsg_t));

        LE_INFO("seq counter: %x",seqCounter);
        LE_INFO("dataLen: %d",int(dataLen));

        size_t dataRecLen = TRANSFER_DATA_REQ_BASE_LEN;

        diagReqMsgPtr->svcId = SID_TRANSFER_DATA;
        diagReqMsgPtr->data[0] = SID_TRANSFER_DATA;
        diagReqMsgPtr->data[1] = seqCounter;

        memcpy(diagReqMsgPtr->data + dataRecLen, dwnldDataPtr, dataLen);
        dataRecLen += dataLen;

        diagReqMsgPtr->dataLen = dataRecLen;
        diagReqMsgPtr->link = LE_DLS_LINK_INIT;
        diagReqMsgPtr->ref = (taf_diagBackend_DiagInfRef_t)le_ref_CreateRef(
            DiagMsgRefMap, diagReqMsgPtr);

        le_dls_Queue(&diagMsgList, &diagReqMsgPtr->link);

        // Report getSeedValue request event.
        le_event_ReportWithRefCounting(diagBackend.DiagEventReqEvtId, diagReqMsgPtr);
        LE_DEBUG("AP_TransferData_Dwnld callback reported");

        ResponseStatus = PROCESSING;
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == PROCESSING)
    {
        LE_DEBUG("AP_TransferData_Dwnld callback ---  PROCESSING");
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == FINISHED)
    {
        LE_DEBUG("AP_TransferData_Dwnld callback ----  FINISHED");
        memset(data, 0, dataLen);
        dataLen = 0;
        ResponseStatus = READYFORNEWREQ;
        return DG_OK;
    }
    else
    {
        LE_DEBUG("AP_TransferData_Dwnld callback ----  Negative Response Sent");
        Callbacks::api->DG_Negative_Response_Code(nrcValue);
        ResponseStatus = READYFORNEWREQ;
        return DG_NOT_OK;
    }
    return DG_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Transfer Data Upload request callback.
 */
//--------------------------------------------------------------------------------------------------
Tdd_DG_Status taf_DiagStack::AP_TransferData_Upld
(
    C_UBYTE seqCounter,
    Tst_DG_Upld* upldDataPtr
)
{

    LE_INFO("AP_TransferData_Upld callback!");
    auto &diagBackend = taf_DiagBackend::GetInstance();

    std::lock_guard<std::mutex> lock(ResponseStatusMtx);
    if (ResponseStatus == READYFORNEWREQ)
    {
        LE_DEBUG("AP_TransferData_Upld callback ---  READYFORNEWREQ");
        taf_DiagReqMsg_t *diagReqMsgPtr = NULL;
        diagReqMsgPtr = (taf_DiagReqMsg_t *)le_mem_ForceAlloc(DiagMsgPool);
        memset(diagReqMsgPtr, 0, sizeof(taf_DiagReqMsg_t));

        LE_INFO("seq counter: %x",seqCounter);
        LE_INFO("dataLen: %d",int(dataLen));

        size_t dataRecLen = TRANSFER_DATA_REQ_BASE_LEN;

        diagReqMsgPtr->svcId = SID_TRANSFER_DATA;
        diagReqMsgPtr->data[0] = SID_TRANSFER_DATA;
        diagReqMsgPtr->data[1] = seqCounter;

        diagReqMsgPtr->dataLen = dataRecLen;
        diagReqMsgPtr->link = LE_DLS_LINK_INIT;
        diagReqMsgPtr->ref = (taf_diagBackend_DiagInfRef_t)le_ref_CreateRef(
            DiagMsgRefMap, diagReqMsgPtr);

        le_dls_Queue(&diagMsgList, &diagReqMsgPtr->link);

        // Report AP_TransferData_Upld request event.
        le_event_ReportWithRefCounting(diagBackend.DiagEventReqEvtId, diagReqMsgPtr);
        LE_DEBUG("AP_TransferData_Upld callback reported");

        ResponseStatus = PROCESSING;
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == PROCESSING)
    {
        LE_DEBUG("AP_TransferData_Upld callback ---  PROCESSING");
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == FINISHED)
    {
        LE_DEBUG("AP_TransferData_Upld callback ----  FINISHED");
        if(data != NULL && dataLen != 0)
        {
            for (int i=0; i < dataLen; i++)
            {
                upldDataPtr->uc_UpldDataPtr[i] = data[i];
            }
            upldDataPtr->uc_UpldDataLen = dataLen;
        }
        memset(data, 0, dataLen);
        dataLen = 0;
        ResponseStatus = READYFORNEWREQ;
        return DG_OK;
    }
    else
    {
        LE_DEBUG("AP_TransferData_Upld callback ----  Negative Response Sent");
        Callbacks::api->DG_Negative_Response_Code(nrcValue);
        ResponseStatus = READYFORNEWREQ;
        return DG_NOT_OK;
    }
    return DG_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * File Transfer Exit request callback.
 */
//--------------------------------------------------------------------------------------------------
Tdd_DG_Status   taf_DiagStack::AP_TransferExit
(
    C_UBYTE * transferReqParamRecord,
    C_UBYTE length
)
{

    LE_INFO("AP_TransferExit callback!");
    auto &diagBackend = taf_DiagBackend::GetInstance();

    std::lock_guard<std::mutex> lock(ResponseStatusMtx);
    if (ResponseStatus == READYFORNEWREQ)
    {
        LE_DEBUG("AP_TransferExit callback ---  READYFORNEWREQ");
        taf_DiagReqMsg_t *diagReqMsgPtr = NULL;
        diagReqMsgPtr = (taf_DiagReqMsg_t *)le_mem_ForceAlloc(DiagMsgPool);
        memset(diagReqMsgPtr, 0, sizeof(taf_DiagReqMsg_t));

        LE_INFO("length: %x",length);

        size_t dataRecLen = REQUEST_TRANSFER_EXIT_REQ_BASE_LEN;

        diagReqMsgPtr->svcId = SID_REQUEST_TRANSFER_EXIT;
        diagReqMsgPtr->data[0] = SID_REQUEST_TRANSFER_EXIT;

        memcpy(diagReqMsgPtr->data + dataRecLen, transferReqParamRecord, length);
        dataRecLen += length;

        LE_INFO("dataLen:%d", int(dataRecLen));
        diagReqMsgPtr->dataLen = dataRecLen;
        diagReqMsgPtr->link = LE_DLS_LINK_INIT;
        diagReqMsgPtr->ref = (taf_diagBackend_DiagInfRef_t)le_ref_CreateRef(
            DiagMsgRefMap, diagReqMsgPtr);

        le_dls_Queue(&diagMsgList, &diagReqMsgPtr->link);

        // Report AP_TransferExit request event.
        le_event_ReportWithRefCounting(diagBackend.DiagEventReqEvtId, diagReqMsgPtr);
        LE_DEBUG("AP_TransferExit callback reported");

        ResponseStatus = PROCESSING;
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == PROCESSING)
    {
        LE_DEBUG("AP_TransferExit callback ---  PROCESSING");
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == FINISHED)
    {
        LE_DEBUG("AP_TransferExit callback ----  FINISHED");
        memset(data, 0, dataLen);
        dataLen = 0;
        ResponseStatus = READYFORNEWREQ;
        return DG_OK;
    }
    else
    {
        LE_DEBUG("AP_TransferExit callback ----  Negative Response Sent");
        Callbacks::api->DG_Negative_Response_Code(nrcValue);
        ResponseStatus = READYFORNEWREQ;
        return DG_NOT_OK;
    }
    return DG_OK;
}

Tdd_DG_Status taf_DiagStack::AP_Routine_Control
(
    C_USHORT routineId,
    Tdd_RoutineCtrl_SubFunc subFunction,
    C_UBYTE* ctrlOptionRecord,
    C_UINT32 ctrlOptionRecordLen
)
{

    LE_INFO("AP_Routine_Control callback!");
    auto &diagBackend = taf_DiagBackend::GetInstance();

    std::lock_guard<std::mutex> lock(ResponseStatusMtx);
    if (ResponseStatus == READYFORNEWREQ)
    {
        LE_DEBUG("AP_Routine_Control callback ---  READYFORNEWREQ");
        taf_DiagReqMsg_t *diagReqMsgPtr = NULL;
        diagReqMsgPtr = (taf_DiagReqMsg_t *)le_mem_ForceAlloc(DiagMsgPool);
        memset(diagReqMsgPtr, 0, sizeof(taf_DiagReqMsg_t));

        size_t dataRecLen = ROUTINE_CONTROL_REQ_BASE_LEN;

        diagReqMsgPtr->svcId = SID_ROUTINE_CONTROL;
        diagReqMsgPtr->data[0] = SID_ROUTINE_CONTROL;
        diagReqMsgPtr->data[1] = subFunction;
        diagReqMsgPtr->data[2] = (routineId & 0xff00) >> 8;
        diagReqMsgPtr->data[3] = (routineId & 0xff);

        memcpy(diagReqMsgPtr->data + dataRecLen, ctrlOptionRecord, ctrlOptionRecordLen);
        dataRecLen += ctrlOptionRecordLen;

        LE_INFO("dataLen:%d", int(dataRecLen));
        diagReqMsgPtr->dataLen = dataRecLen;
        diagReqMsgPtr->link = LE_DLS_LINK_INIT;
        diagReqMsgPtr->ref = (taf_diagBackend_DiagInfRef_t)le_ref_CreateRef(
            DiagMsgRefMap, diagReqMsgPtr);

        le_dls_Queue(&diagMsgList, &diagReqMsgPtr->link);

        // Report AP_Routine_Control request event.
        le_event_ReportWithRefCounting(diagBackend.DiagEventReqEvtId, diagReqMsgPtr);
        LE_DEBUG("AP_Routine_Control callback reported");

        ResponseStatus = PROCESSING;
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == PROCESSING)
    {
        LE_DEBUG("AP_Routine_Control callback ---  PROCESSING");
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == FINISHED)
    {
        LE_DEBUG("AP_Routine_Control callback ----  FINISHED");
        Callbacks::api->DG_Write_Buffer(data, dataLen);
        memset(data, 0, dataLen);
        dataLen = 0;
        ResponseStatus = READYFORNEWREQ;
        return DG_OK;
    }
    else
    {
        LE_DEBUG("AP_Routine_Control callback ----  Negative Response Sent");
        Callbacks::api->DG_Negative_Response_Code(nrcValue);
        ResponseStatus = READYFORNEWREQ;
        return DG_NOT_OK;
    }
    return DG_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * ECU Reset callback.
 */
//--------------------------------------------------------------------------------------------------
Tdd_DG_Status taf_DiagStack::AP_ECUReset
(
    C_UBYTE param
)
{
    LE_INFO("AP_ECUReset callback!");
    auto &diagBackend = taf_DiagBackend::GetInstance();

    std::lock_guard<std::mutex> lock(ResponseStatusMtx);
    if (ResponseStatus == READYFORNEWREQ)
    {
        LE_DEBUG("AP_ECUReset callback ----  READYFORNEWREQ");
        taf_DiagReqMsg_t *diagReqMsgPtr = NULL;
        diagReqMsgPtr = (taf_DiagReqMsg_t *)le_mem_ForceAlloc(DiagMsgPool);
        memset(diagReqMsgPtr, 0, sizeof(taf_DiagReqMsg_t));

        diagReqMsgPtr->svcId = SID_ECU_RESET;
        diagReqMsgPtr->data[0] = SID_ECU_RESET;
        diagReqMsgPtr->data[1] = param;
        diagReqMsgPtr->dataLen = ECU_RESET_REQ_BASE_LEN;
        diagReqMsgPtr->ref = (taf_diagBackend_DiagInfRef_t)le_ref_CreateRef(
            DiagMsgRefMap, diagReqMsgPtr);
        diagReqMsgPtr->link = LE_DLS_LINK_INIT;

        le_dls_Queue(&diagMsgList, &diagReqMsgPtr->link);

        // Report AP_ECUReset notification event.
        le_event_ReportWithRefCounting(diagBackend.DiagEventReqEvtId, diagReqMsgPtr);

        LE_DEBUG("AP_ECUReset callback reported");

        ResponseStatus = PROCESSING;
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == PROCESSING)
    {
        LE_DEBUG("AP_ECUReset callback ----  PROCESSING");
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == FINISHED)
    {
        LE_DEBUG("AP_ECUReset callback ----  FINISHED");
        memset(data, 0, dataLen);
        dataLen = 0;
        ResponseStatus = READYFORNEWREQ;
        return DG_OK;
    }
    else
    {
        LE_DEBUG("AP_ECUReset callback ----  Negative Response Sent");
        Callbacks::api->DG_Negative_Response_Code(nrcValue);
        ResponseStatus = READYFORNEWREQ;
        return DG_NOT_OK;
    }
    return DG_OK;
}

//--------------------------------------------------------------------------------------------------
/**
* IOControl callback.
*/
//--------------------------------------------------------------------------------------------------
Tdd_DG_Status taf_DiagStack::AP_IO_Control_By_Id
(
    C_USHORT recordId,
    Tdd_IO_CtrlParam ioCtrlParam,
    C_UBYTE* ctrlState,
    C_UINT32 ctrlStateLen,
    C_UBYTE* ctrlEnableMaskRecord,
    C_UINT32 ctrlEnableMaskRecordLen
)
{
    LE_DEBUG("AP_IO_Control_By_Id");

    auto &diagBackend = taf_DiagBackend::GetInstance();
 
    std::lock_guard<std::mutex> lock(ResponseStatusMtx);
    if (ResponseStatus == READYFORNEWREQ)
    {
        LE_DEBUG("AP_IO_Control_By_Id callback ----  READYFORNEWREQ");
        taf_DiagReqMsg_t *diagReqMsgPtr = NULL;
        diagReqMsgPtr = (taf_DiagReqMsg_t *)le_mem_ForceAlloc(DiagMsgPool);
        memset(diagReqMsgPtr, 0, sizeof(taf_DiagReqMsg_t));

        LE_DEBUG("Requested recordId: %hu",recordId);
        LE_DEBUG("ioCtrlParam: %d",int(ioCtrlParam));
        LE_DEBUG("ctrlStateLen: %d",int(ctrlStateLen));
        for(int i=0;i<int(ctrlStateLen);i++){
            LE_DEBUG("ctrlState[%d]: %d",i,int(ctrlState[i]));
        }
        LE_DEBUG("ctrlEnableMaskRecordLen: %d",int(ctrlEnableMaskRecordLen));
        for(int i=0;i<int(ctrlEnableMaskRecordLen);i++){
            LE_DEBUG("ctrlEnableMaskRecord[%d]: %d",i,int(ctrlEnableMaskRecord[i]));
        }

        size_t dataRecLen = IO_CTRL_REQ_BASE_LEN;
        diagReqMsgPtr->svcId = SID_IO_CTRL;
        diagReqMsgPtr->data[0] = SID_IO_CTRL;
        diagReqMsgPtr->data[1] = (recordId & 0xff00) >> 8;
        diagReqMsgPtr->data[2] = (recordId & 0xff);
        diagReqMsgPtr->data[3] = ioCtrlParam;
        memcpy(diagReqMsgPtr->data + dataRecLen, ctrlState, ctrlStateLen);
        dataRecLen = dataRecLen + ctrlStateLen;
        memcpy(diagReqMsgPtr->data + dataRecLen, ctrlEnableMaskRecord, ctrlEnableMaskRecordLen);
        dataRecLen = dataRecLen + ctrlEnableMaskRecordLen;
        diagReqMsgPtr->dataLen = dataRecLen;
        LE_DEBUG("dataLen:%d", int(dataRecLen));

        diagReqMsgPtr->ref = (taf_diagBackend_DiagInfRef_t)le_ref_CreateRef(
            DiagMsgRefMap, diagReqMsgPtr);
        diagReqMsgPtr->link = LE_DLS_LINK_INIT;

        le_dls_Queue(&diagMsgList, &diagReqMsgPtr->link);

        // Report IOControl notification event.
        le_event_ReportWithRefCounting(diagBackend.DiagEventReqEvtId, diagReqMsgPtr);
        LE_DEBUG("AP_IO_Control_By_Id callback reported");

        ResponseStatus = PROCESSING;
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == PROCESSING)
    {
        LE_DEBUG("AP_IO_Control_By_Id callback ---  PROCESSING");
        return DG_RESP_PEND;
    }
    else if (ResponseStatus == FINISHED)
    {
        LE_DEBUG("AP_IO_Control_By_Id callback ----  FINISHED");
        Callbacks::api->DG_Write_Buffer(data, dataLen);
        memset(data, 0, dataLen);
        dataLen = 0;
        ResponseStatus = READYFORNEWREQ;
        return DG_OK;
    }
    else
    {
        LE_DEBUG("AP_IO_Control_By_Id callback ----  Negative Response Sent");
        Callbacks::api->DG_Negative_Response_Code(nrcValue);
        ResponseStatus = READYFORNEWREQ;
        return DG_NOT_OK;
    }
    return DG_OK;
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
            std::memcpy(cb.data, diagEventRespMsgPtr->dataRec,
                    sizeof(diagEventRespMsgPtr->dataRec));
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
