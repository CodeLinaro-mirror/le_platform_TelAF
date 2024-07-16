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
#include "tafSvcIF.hpp"

#include "dg_struc.h"
#include "callbacks.h"
#include "log.h"
#include "DiagNode.h"
#include <iostream>
#include <cstring>
#include <mutex>

namespace telux
{
    namespace tafsvc
    {
        // Response status
        enum En_ResponseStatus
        {
            READYFORNEWREQ,
            PROCESSING,
            FINISHED,
            FAILED
        };

        class taf_DiagStack : public ITafSvc , public Callbacks
        {
            public:
                taf_DiagStack()
                {
                    ResponseStatus = READYFORNEWREQ;
                };

                ~taf_DiagStack(){};

                static taf_DiagStack& GetInstance();
                void Init();

                // Read/Write DID
                virtual Tdd_DG_Status AP_ReadWriteDataByID(C_USHORT Lus_AP_RecordID,
                        C_UBYTE Luc_AP_ServiceId, C_UBYTE Position, C_USHORT Size) override;

                virtual Tdd_DG_Status AP_ValidSession_ReadWriteDataByID(
                        Tdd_DG_DiagSession Ldd_DG_DiagSession,C_USHORT Lus_AP_RecordID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                // Diagnostic Session Control
                virtual Tdd_DG_Status AP_Condition_For_DiagSess(
                        Tdd_DG_DiagSession Luc_DG_DiagSess) override;

                virtual void AP_SessionChange(Tdd_DG_DiagSession prevLuc_DG_DiagSess,
                        Tdd_DG_DiagSession Luc_DG_DiagSess) override;

                virtual void AP_SessionTimeout() override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                }

                // ECU Reset
                virtual Tdd_DG_Status AP_Condition_For_ECUReset() override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_DiagnosticKernel_Ready() override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_ECUReset(C_UBYTE param) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual void AP_EcuProgramSess() override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                }

                virtual Tdd_DG_Status AP_ForceShutDownMemWrtECUReset() override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                // Read DTC
                virtual C_UBYTE AP_GetDTCFormatId(void) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual DK_BOOL AP_GetDTCSupported_By_Index( C_USHORT) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual DK_BOOL AP_GetDTCSupported_By_Record(C_UINT32 ) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual C_UBYTE AP_GetExtendData_By_RecordNumber( C_USHORT ,  C_UBYTE) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual  C_UBYTE AP_GetPRODUCTCommonSSData_By_RecordNo( C_UBYTE, C_UBYTE) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual C_UBYTE AP_GetIndex_from_Record(C_UINT32 ) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual C_UBYTE AP_GetLength_LocalSS( C_UBYTE) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual C_UBYTE AP_GetLocalSSData_By_RecordNo( C_UBYTE, C_UBYTE) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual C_USHORT AP_GetNoofDTCsSet(void) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual C_UBYTE AP_Get_NoOfSnapshotByIndex(void) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                Tdd_DG_Status AP_GetValidSnapshotRecord(C_UBYTE* bytes) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual C_UINT32 AP_GetRecordByIndex( C_USHORT ) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual C_UBYTE AP_GetRecordNoOfIdentifiers( C_UBYTE) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual C_UBYTE AP_GetStatus_All_DTC(C_UINT32) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual C_UBYTE AP_Get_StatusAvailabilityMask(void) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual C_UBYTE AP_Get_DTC_StatusbyIndex( C_USHORT ) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual DK_BOOL AP_ValidIndex( C_USHORT, C_UBYTE) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual DK_BOOL AP_ValidRecordNumber ( C_UBYTE ) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual C_USHORT AP_GetIndex_From_SupportedDTC(C_UINT32) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual void vd_updateFIFO( C_USHORT) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                }

                virtual DK_BOOL AP_ValidSnapshotRecordNumber ( C_UBYTE ) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual DK_BOOL AP_GetCustSympStatus_By_Index ( C_USHORT) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                // Communication Control
                virtual Tdd_DG_Status AP_CommnControl(C_UBYTE controlType,
                        C_UBYTE commType) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                // Input Output Control by Identifier
                virtual Tdd_DG_Status AP_ValidSession_IOControlIdentifier(
                        Tdd_DG_DiagSession session, C_USHORT id) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_IO_CONTRL_ID_9805 (void) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                // Routine Control
                virtual Tdd_DG_Status AP_ValidSession_RoutineCntrl(
                        Tdd_DG_DiagSession Luc_DG_Session,
                                C_USHORT Lus_AP_RoutineControlID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_SecurityAccess_RoutineCntrl(
                        C_UINT32 routineId, C_UBYTE level) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Routine_Results_Available(
                        C_USHORT Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_2AA(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_2BB(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_2CC(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_2DD(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_2EE(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_3AA(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_3BB(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_3CC(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_3DD(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_3EE(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_4AA(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_4BB(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_4CC(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_4DD(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_4EE(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_5AA(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_5BB(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_5CC(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_5DD(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Dummy_RoutineOnDemandSelfTest_5EE(
                        C_UBYTE Lus_DG_RoutineID) override
                {
                        std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                        return DG_OK;
                }

                // Request Download
                virtual Tdd_DG_Status AP_Condition_For_DiagSess_ReqDWLD (
                        Tdd_DG_DiagSession Luc_DG_DiagSess) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_RequestDownload(C_UINT32 memAddr,
                        C_UINT32 memSize, C_UBYTE dataFormatId) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

#ifdef DG_REQUESTUPLOAD
                // Request Upload
                virtual Tdd_DG_Status AP_Condition_For_DiagSess_ReqUPLD(
                        Tdd_DG_DiagSession Luc_DG_Session) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_RequestUpload(C_UINT32 memAddress,
                        C_UINT32 memSize, C_UBYTE dataFormatId) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }
#endif
                // Request Transfer Data
                virtual Tdd_DG_Status AP_Condition_For_DiagSess_TransferData(
                        Tdd_DG_DiagSession Luc_DG_Session) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_DataTransferCheck_DWLD(void) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_DataTransferCheck_FILETRNSFR(void) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                // Read Memory By Address
                virtual Tdd_DG_Status AP_ReadMemoryByAddr(C_UINT32 address, C_USHORT size) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_FD_Customer_Symptom AP_GetCustSympExtendData_By_Index(
                        C_USHORT) override
                {
                    Tdd_FD_Customer_Symptom retVal;
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return retVal;
                }

                virtual Tdd_DG_Status AP_SuppSecAlg (C_UBYTE *SeedValue, C_USHORT SeedLength,
                            C_UBYTE *Key, C_USHORT KeyLength, C_UBYTE SecurityLevel)
                {
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Get_SeedValue (C_UBYTE* SeedValue, C_USHORT SeedLength,
                        C_UBYTE SecurityLevel)
                {
                    return DG_OK;
                }

                virtual Tdd_DG_Status  AP_TransferData_Dwnld(C_UBYTE seqCounter,
                        C_UBYTE* dwnldDataPtr, C_USHORT dataLen)
                {
                    return DG_OK;
                }

                virtual Tdd_DG_Status  AP_DataTransferCheck_UPLD(void)
                {
                    return DG_OK;
                }

                virtual Tdd_DG_Status  AP_TransferData_Upld(C_UBYTE seqCounter,
                        Tst_DG_Upld* upldDataPtr)
                {
                    return DG_OK;
                }

                // Request Transfer Exit
                virtual Tdd_DG_Status AP_Condition_For_DiagSess_TransferExit(
                        Tdd_DG_DiagSession Sessions) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status   AP_TransferExit(C_UBYTE * transferReqParamRecord,
                        C_UBYTE length)
                {
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Condition_For_DiagSess_ReqFileTrnsfr(
                        Tdd_DG_DiagSession Session)
                {
                    return DG_OK;
                }

                // Clear DTC
                virtual Tdd_DG_Status AP_ClrDTCInformation() override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                // Control DTC
                virtual Tdd_DG_Status AP_CntlDTCSetting (C_UBYTE status) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                // fault handler
                Tdd_FD_NVRAM_State AP_NVRAM_ClearDTC (C_USHORT Luc_AP_Handle,
                        C_USHORT Luc_AP_Length, C_USHORT Luc_AP_offset)
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ <<"\n";
                    return DK_NVRAM_OK;
                }

                Tdd_FD_Status AP_NVRAM_Clear_SetAdditionalData (void)
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return FD_OK;
                }

                Tdd_FD_NVRAM_State AP_NVRAM_Write (C_USHORT Luc_AP_Handle, C_UBYTE *Luc_AP_SrcPtr,
                        C_USHORT Luc_AP_Length, C_USHORT Luc_AP_Offset)
                {
                    uds_server::Log::info(">>>>>>>>%s, offset:%d, length:%d",__func__,
                            Luc_AP_Offset, Luc_AP_Length);
                    using namespace std;
                    return DK_NVRAM_OK;
                }

                Tdd_FD_NVRAM_State AP_NVRAM_Read (C_USHORT Luc_AP_Handle, C_UBYTE *Luc_AP_DestPtr,
                        C_USHORT Luc_AP_Length, C_USHORT Luc_AP_Offset)
                {
                    uds_server::Log::info(">>>>>>>>%s, offset:%d, length:%d",__func__,
                            Luc_AP_Offset, Luc_AP_Length);
                    return DK_NVRAM_OK;
                }

                DK_BOOL AP_Allow_No_Setting_DTCs (void)
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return 1;
                }

                void AP_Store_Local_Snapshot (C_USHORT Lus_FD_Index, C_UBYTE *Luc_FD_LocalSnapshot)
                {
                    //std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                }

                virtual Tdd_DG_Status AP_RequestFileTransfer(C_UBYTE modeOfOperation,
                        C_USHORT filePathAndNameLength, C_UBYTE * filePathAndName,
                                C_UBYTE fileSizeParameterLength, C_UBYTE fileSizeUncompressed,
                                        C_UBYTE fileSizeCompressed, C_UBYTE dataFormatId) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                Tdd_FD_Status AP_Additional_Cntrl_DTC_Setting_Info (C_UBYTE Luc_FD_SettingType)
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return FD_OK;
                }

                // Response event thread
                static void* RespEventThread(void* contextPtr);

                // Diag Event response event-handler
                le_event_Id_t DiagEventRespEvtId;
                static void DiagEventRespEvtHandler(void* RespPtr);

            private:
                le_thread_Ref_t RespEvtThreadRef = NULL;

                le_mem_PoolRef_t DiagMsgPool;
                le_ref_MapRef_t DiagMsgRefMap;
                le_dls_List_t diagMsgList;

                std::mutex ResponseStatusMtx;
                En_ResponseStatus ResponseStatus;

                uint8_t data[TAF_DIAGBACKEND_MAX_PAYLOAD_SIZE];
                uint16_t dataLen;

                unsigned char nrcValue;  // NRC handling.
                Tdd_DG_Status CallbackStatus;
        };
    }
}
