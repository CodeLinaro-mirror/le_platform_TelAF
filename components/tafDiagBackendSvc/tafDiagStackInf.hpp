/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

// 3rd party UDS stack library
#include "dg_struc.h"
#include "callbacks.h"
#include "DiagNode.h"
#include <iostream>
#include <cstring>
#include <mutex>

///{
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

                // Security Access
                virtual Tdd_DG_Status AP_SuppSecAlg (C_UBYTE *SeedValue, C_USHORT SeedLength,
                            C_UBYTE *Key, C_USHORT KeyLength, C_UBYTE SecurityLevel) override;

                virtual Tdd_DG_Status AP_Get_SeedValue (C_UBYTE* SeedValue, C_USHORT SeedLength,
                        C_UBYTE SecurityLevel, C_UBYTE* SecurityAccessDataRec,
                                C_USHORT SecurityAccessDataRecLen) override;

                // Request File Transfer
                virtual Tdd_DG_Status AP_RequestFileTransfer(C_UBYTE modeOfOperation,
                        C_USHORT filePathAndNameLength, C_UBYTE * filePathAndName,
                                C_UBYTE fileSizeParameterLength, C_UINT32 fileSizeUncompressed,
                                        C_UINT32 fileSizeCompressed, C_UBYTE dataFormatId) override;

                virtual Tdd_DG_Status AP_Condition_For_DiagSess_ReqFileTrnsfr(
                        Tdd_DG_DiagSession Session)
                {
                    return DG_OK;
                }

                // Transfer Data
                virtual Tdd_DG_Status  AP_TransferData_Dwnld(C_UBYTE seqCounter,
                        C_UBYTE* dwnldDataPtr, C_UINT32 dataLen) override;

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

                // Transfer Exit
                virtual Tdd_DG_Status   AP_TransferExit(C_UBYTE * transferReqParamRecord,
                        C_UBYTE length) override;

                virtual Tdd_DG_Status AP_Condition_For_DiagSess_TransferExit(
                        Tdd_DG_DiagSession Sessions) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                // Routine Control
                virtual Tdd_DG_Status AP_Routine_Control(C_USHORT routineId,
                        Tdd_RoutineCtrl_SubFunc subFunction, C_UBYTE* ctrlOptionRecord,
                                C_UINT32 ctrlOptionRecordLen) override;

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

                // ECU Reset
                virtual Tdd_DG_Status AP_ECUReset(C_UBYTE param) override;

                virtual Tdd_DG_Status AP_Condition_For_ECUReset(C_UBYTE& powerDownTime) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_DiagnosticKernel_Ready() override
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

                // IO Communication Control
                 virtual Tdd_DG_Status AP_IO_Control_By_Id(
                        C_USHORT recordId, Tdd_IO_CtrlParam ioCtrlParam, C_UBYTE* ctrlState,
                                C_UINT32 ctrlStateLen, C_UBYTE* ctrlEnableMaskRecord,
                                        C_UINT32 ctrlEnableMaskRecordLen) override;

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

                virtual void vd_updateFIFO(C_USHORT Lus_FD_Index, C_UBYTE Luc_Mem_Selection)
                        override
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

#ifdef DG_REQUESTUPLOAD
                virtual Tdd_DG_Status  AP_DataTransferCheck_UPLD(void)
                {
                    return DG_OK;
                }

                virtual Tdd_DG_Status  AP_TransferData_Upld(C_UBYTE seqCounter,
                        Tst_DG_Upld* upldDataPtr) override;
#endif

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

                Tdd_FD_Status AP_NVRAM_Clear_SetAdditionalData (C_UBYTE Luc_Mem_Selection)
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return FD_OK;
                }

                Tdd_FD_NVRAM_State AP_NVRAM_Write (C_USHORT Luc_AP_Handle, C_UBYTE *Luc_AP_SrcPtr,
                        C_USHORT Luc_AP_Length, C_USHORT Luc_AP_Offset, C_UBYTE Luc_Mem_Selection)
                {
                    return DK_NVRAM_ERROR;
                }

                Tdd_FD_NVRAM_State AP_NVRAM_Read (C_USHORT Luc_AP_Handle, C_UBYTE *Luc_AP_DestPtr,
                        C_USHORT Luc_AP_Length, C_USHORT Luc_AP_Offset, C_UBYTE Luc_Mem_Selection)
                {
                    return DK_NVRAM_ERROR;
                }

                Tdd_FD_NVRAM_State AP_NVRAM_Clear (C_USHORT Luc_AP_Handle, C_USHORT Luc_AP_Length,
                        C_USHORT Luc_AP_offset, C_UBYTE Luc_Mem_Selection)
                {
                    return DK_NVRAM_ERROR;
                }

                DK_BOOL AP_Allow_No_Setting_DTCs (void)
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return 1;
                }

                void AP_Store_Local_Snapshot (C_UINT32 Lul_FD_DTC, C_UBYTE *Luc_FD_LocalSnapshot,
                        C_UBYTE Luc_Mem_Selection)
                {
                    //std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                }

                Tdd_FD_Status AP_Additional_Cntrl_DTC_Setting_Info (C_UBYTE Luc_FD_SettingType,
                        C_UBYTE Luc_Mem_Selection)
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return FD_OK;
                }

                /*Authentication*/
                virtual Tdd_DG_Status AP_GetAuthenticationConfig() override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual void AP_Notify_Deauthentication()override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                }

                virtual Tdd_DG_Status AP_VerifyCertUni(C_UBYTE commConfig, 
                        C_UINT16 lenOfCertClient, C_UBYTE *certClient,
                                C_UINT16 lenOfChallClient, C_UBYTE *challClient) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_VerifyClientPOWN(C_UINT16 lenOfPOWNClient,
                        C_UBYTE *POWNClient, C_UINT16 lenOfPubKey, C_UBYTE *pubKeyClient)override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_TransmitCertificate(C_UINT16 certEvaluationID,
                        C_UINT16 lenOfCertData, C_UBYTE *certData) override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }
				
                virtual Tdd_DG_Status AP_RequestChallenge(C_UINT8 commCtrl, C_UINT8 algoindicatorLen, C_UINT8* algoindicator) override
				{
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }
				
				virtual Tdd_DG_Status AP_VerifyPOWNUni(C_UINT8 algoindicatorLen, C_UINT8* algoindicator, C_UINT16 pownClientLen, C_UINT8* pownClient,
			                               C_UINT16 challengeClientLen, C_UINT8* challengeClient, C_UINT16 additionalParamLen,
										   C_UINT8* additionalParam) override
				{
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }
				
	            virtual Tdd_DG_Status AP_VerifyPOWNBi(C_UINT8 algoindicatorLen, C_UINT8* algoindicator, C_UINT16 pownClientLen, C_UINT8* pownClient,
			                               C_UINT16 challengeClientLen, C_UINT8* challengeClient, C_UINT16 additionalParamLen,
										   C_UINT8* additionalParam) override
				{
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }
				
	            virtual Tdd_DG_Status AP_VerifyCertiBi(C_UINT8 commCtrl, C_UINT16 certiClientLen, C_UINT8* certiClient,
			                               C_UINT16 challengeClientLen, C_UINT8* challengeClient) override
				{
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                /* Response On Event */
                virtual Tdd_DG_Status AP_Condition_For_ResponseOnEvent( C_UBYTE Luc_AP_EventType)
                        override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
                }

                virtual Tdd_DG_Status AP_Update_VMResoponeOnEventTrigger(C_UBYTE Luc_AP_EventId,
                        C_UBYTE Luc_AP_EventType, C_UBYTE *Luc_AP_EventTypeRecord,
                                C_UBYTE Luc_AP_EventTypeRecordLen, C_UBYTE Luc_AP_EventStatus)
                                        override
                {
                    std::cout << ">>>>>>>>>>>>>>>>>>>>" << __func__ << "\n";
                    return DG_OK;
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
