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

#ifndef TAFUDS_COMMUNICATION_MGR_HPP
#define TAFUDS_COMMUNICATION_MGR_HPP

#include "tafDoIPStack.h"
#include "legato.h"
#include "interfaces.h"
#include "configuration.hpp"

using namespace telux::tafsvc;

namespace taf{
namespace uds{

    #define UDS_DATA_SIZE 4095
    #define UDS_P2_SERVER 50 // Default P2 server interval
    #define UDS_P2_SERVER_MAX 65535 //Maximal P2 server interval
    #define UDS_P2_STAR_SERVER 5000 // Default P2* server interval
    #define UDS_P2_STAR_SERVER_MAX 655350 //Maximal P2* server interval
    #define UDS_P2_STAR_SERVER_CNT 120
    #define UDS_S3_SERVER 5000
    #define TAF_UDS_HANDLER_REF_CNT 1

    // DID Config tree definition
    #define DID_NODE_LEN                 100
    #define DID_CONFIG_TREE_NODE         "diag/DID"
    #define DID_READ_PROPERTY_SUPPORTED_FUNCTION  "diag/DID/%2x/supported_functions/read_did"
    #define DID_WRITE_PROPERTY_SUPPORTED_FUNCTION  "diag/DID/%2x/supported_functions/write_did"
    #define DID_READ_SEC_PROPERTY_SUPPORTED_FUNCTION  "diag/DID/%2x/supported_functions/read_sec"
    #define DID_WRITE_SEC_PROPERTY_SUPPORTED_FUNCTION  "diag/DID/%2x/supported_functions/write_sec"
    #define DID_CONFIG_TREE_VALUE_FORMAT  "diag/DID/%2x/value"
    #define DID_DATA_FORMAT              "data%d"

    // DTC Config tree definition
    #define DTC_CONFIG_TREE_NODE                   "diag/DTC"
    #define DTC_STATUS_AVAILABILITY_MASK           "DTCStatusAvailabilityMask"
    #define DTC_INFORMATION                        "info"
    #define DTC_STR_INFO_LEN                       20
    #define DTC_STATUS                             "status"
    #define DTC_SUB_FUNCTION_REPORT_DTC_BY_STATUS  0x2

    // UDS minimal len
    #define UDS_REQ_MIN_LEN 1

    // Negative Response (0x7F)
    #define UDS_NEGATIVE_RESP_SID 0x7F
    #define UDS_NEG_RESP_LEN 3

    // Session control service (0x10)
    #define UDS_SESSION_CTRL_REQ_MIN_LEN 2
    #define UDS_SESSION_CTRL_RESP_LEN 6
    #define UDS_SESSION_CHANGE_DATA_SIZE 3

    // ECUReset service (0x11)
    #define UDS_ECU_RESET_REQ_MIN_LEN 2
    #define UDS_ECU_RESET_RESP_BASE_LEN 2

    // ReadDTCInformation service (0x19)
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN 2
    #define UDS_READ_DTC_INFO_RESP_BASE_LEN 2

    // ReadDataByIdentifier service (0x22)
    #define UDS_READ_DID_REQ_MIN_LEN 3
    #define UDS_READ_DID_RESP_BASE_LEN 1
    #define UDS_READ_DID_RESP_MIN_LEN 3
    #define UDS_DID_LEN 2

    // Security access service (0x27)
    #define UDS_SECURITY_ACCESS_REQ_MIN_LEN 2
    #define UDS_SECURITY_ACCESS_SEND_KEY_REQ_MIN_LEN 3 //sid(1)+subfunc(1)+securitykey(1)
    #define UDS_SECURITY_ACCESS_RESP_MIN_LEN 2
    #define UDS_SECURITY_ACCESS_RESP_SEED_ZERO_LEN 4

    // WriteDataByIdentifier service (0x2E)
    #define UDS_WRITE_DID_REQ_MIN_LEN 4
    #define UDS_WRITE_DID_REQ_BASE_LEN 3  // Service ID(1) + DID (2)
    #define UDS_WRITE_DID_RESP_LEN 3

    // InputOutputControlByIdentifier service (0x2F)
    #define UDS_IOCBID_REQ_MIN_LEN 4  // SI+DID+IOCP
    #define UDS_IOCBID_RESP_MIN_LEN 4

    // Routine control service (0x31)
    #define UDS_ROUTINE_CTRL_REQ_MIN_LEN 4
    #define UDS_ROUTINE_CTRL_RESP_MIN_LEN 4

    // TransferData service (0x36)
    #define UDS_REQ_XFER_DATA_BASE_LEN 2
    #define UDS_RESP_XFER_DATA_BASE_LEN 2

    // RequestTransferExit service (0x37)
    #define UDS_REQ_XFER_EXIT_BASE_LEN 1
    #define UDS_RESP_XFER_EXIT_BASE_LEN 1

    // RequestFileTranser service (0x38)
    #define SIZE_OF_SID    1 /* RequestFileTransfer Request SID */
    #define SIZE_OF_MOOP   1 /* modeOfOperation */
    #define SIZE_OF_FPL    2 /* filePathAndNameLength */
    #define SIZE_OF_FP_B1  1 /* first byte of filePathAndName */
    #define SIZE_OF_DFI_   1 /* dataFormatIdentifier */
    #define SIZE_OF_FSL    1 /*fileSizeParameterLength */

    #define RFT_MIN_LEN    (SIZE_OF_SID + SIZE_OF_MOOP + SIZE_OF_FPL + SIZE_OF_FP_B1)
    #define RFT_BASE_LEN   (RFT_MIN_LEN - SIZE_OF_FP_B1)
    #define INDEX_FP_B1    (SIZE_OF_SID + SIZE_OF_MOOP + SIZE_OF_FPL)

    #define SIZE_OF_R_SID  SIZE_OF_SID /* RequestFileTransfer Response SID  */
    #define SIZE_OF_LFID   1 /* lengthFormatIdentifier */
    #define RRFT_BASE_LEN  (SIZE_OF_R_SID + SIZE_OF_MOOP)

    // Tester present service (0x3E)
    #define UDS_TESTER_PRESENT_REQ_LEN 2
    #define UDS_TESTER_PRESENT_RESP_LEN 2

    // ClearDiagnosticInformation service (0x14)
    #define UDS_CLEAR_DIAG_INFO_REQ_MIN_LEN 4
    #define UDS_CLEAR_DIAG_INFO_RESP_LEN 1

    // ControlDTCSetting service (0x85)
    #define UDS_CTRL_DTC_SETTING_REQ_MIN_LEN 2
    #define UDS_CTRL_DTC_SETTING_RESP_LEN 2

    // S3 timer action
    typedef enum
    {
        TAF_UDS_S3_TIMER_STOP          = 0,
        TAF_UDS_S3_TIMER_START         = 0x01,
        TAF_UDS_S3_TIMER_RESTART       = 0x02,
        TAF_UDS_P2STAR_TIMER_STOP      = 0x03,
        TAF_UDS_P2STAR_TIMER_START     = 0x04,
        TAF_UDS_P2STAR_TIMER_RESTART   = 0x05
    }taf_UDSTimer_EventType_t;

    // RequestFileTranser service mode of operation type
    typedef enum
    {
        MOOP_ADD_FILE     = 0x01,
        MOOP_DELETE_FILE  = 0x02,
        MOOP_REPLACE_FILE = 0x03,
        MOOP_READ_FILE    = 0x04,
        MOOP_READ_DIR     = 0x05,
        MOOP_RESUME_FILE  = 0x06
    }taf_UDSReqFileXferMOOPType_t;

    // Diagnostic Request service ID
    typedef enum
    {
        SESSION_CONTROL_REQUEST_ID = 0x10,
        ECU_RESET_REQUEST_ID = 0x11,
        CLEAR_DIAG_INFO_REQUEST_ID = 0x14,
        READ_DTC_INFO_REQUEST_ID = 0x19,
        READ_DID_REQUEST_ID = 0x22,
        SECURITY_ACCESS_REQUEST_ID = 0x27,
        WRITE_DID_REQUEST_ID = 0x2E,
        INPUT_OUTPUT_CONTROL_REQUEST_ID = 0x2F,
        ROUTINE_CONTROL_REQUEST_ID = 0x31,
        TRANSFER_DATA_REQUEST_ID = 0x36,
        REQUEST_TRANSFER_EXIT_REQUEST_ID = 0x37,
        REQUEST_FILE_TRANSFER_REQUEST_ID = 0x38,
        TESTER_PRESENT_REQUEST_ID = 0x3E,
        CONTROL_DTC_SETTING_REQUEST_ID = 0x85
    }taf_UDSReqSvcID_t;

    // Diagnostic Response service ID
    typedef enum
    {
        SESSION_CONTROL_RESPONSE_ID = 0x50,
        ECU_RESET_RESPONSE_ID = 0x51,
        CLEAR_DIAG_INFO_RESPONSE_ID = 0x54,
        READ_DTC_INFO_RESPONSE_ID = 0x59,
        READ_DID_RESPONSE_ID = 0x62,
        SECURITY_ACCESS_RESPONSE_ID = 0x67,
        WRITE_DID_RESPONSE_ID = 0x6E,
        IOCBID_RESPONSE_ID = 0x6F,
        ROUTINE_CONTROL_RESPONSE_ID = 0x71,
        TRANSFER_DATA_RESPONSE_ID = 0x76,
        REQUEST_TRANSFER_EXIT_RESPONSE_ID = 0x77,
        REQUEST_FILE_TRANSFER_RESPONSE_ID = 0x78,
        TESTER_PRESENT_RESPONSE_ID = 0x7E,
        CONTROL_DTC_SETTING_RESPONSE_ID = 0xC5
    }taf_UDSRespSvcID_t;

    // UDS error code.
    typedef enum
    {
        POSITIVE_RESPONSE = 0,
        SERVICE_NOT_SUPPORTED = 0x11,
        SUBFUNCTION_NOT_SUPPORTED = 0x12,
        INCORRECT_MSG_LEN_OR_INVALID_FORMAT = 0x13,
        RESP_TOO_LONG = 0x14,
        BUSY_REPEAT_REQ = 0x21,
        CONDITIONS_NOT_CORRECT = 0x22,
        REQ_SEQUENCE_ERROR = 0x24,
        REQ_OUT_OF_RANGE = 0x31,
        SECURITY_ACCESS_DENY = 0x33,
        INVALID_KEY = 0x35,
        UPLOAD_DOWNLOAD_NOT_ACCEPTED = 0x70,
        GENERAL_PROGRAMMING_FAILURE = 0x72,
        REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING = 0x78
    }taf_UDSErrorCode_t;

    typedef struct
    {
        taf_UDSTimer_EventType_t             event;
        uint32_t                             interval;
    } udsTimerEvent_t;

    // UDS stack indication handler structure.
    typedef struct
    {
        taf_doip_DiagIndicationHandlerFunc_t funcPtr;
        void*                                ctxPtr;
        void*                                safeRef;
    }taf_UDSIndicationHandler_t;

    // ENUM for session typr.
    typedef enum
    {
        DEFAULT_SESSION = 0x01,
        PROGRAMMING_SESSION = 0x02,
        EXTENDED_DIAGNOSTIC_SESSION = 0x03,
        VEHICLE_MANUFACTURER_SPECIFIC_SESSION = 0x40,
        FOTA_SESSION = 0x42,
        DOWNLOADED_ENUMLATION_SESSION = 0x52,
        SYSTEM_SUPPLIER_SPECIFIC_SESSION = 0x60
    }taf_SessionType_t;

    class UdsCommunicationMgr{
        public:
            UdsCommunicationMgr();
            ~UdsCommunicationMgr();
            static UdsCommunicationMgr &GetInstance();

            void Init();
            le_result_t UdsStart(const char* configPathPtr);

            le_result_t UdsAddDiagIndicationHandler();
            static void DiagIndicationHandler( taf_doip_AddrInfo_t* addrInfoPtr,
                    taf_doip_DiagMsg_t* diagMsgPtr, taf_doip_Result_t result, void* userPtr);
            static void DiagConfirmHandler(const taf_doip_AddrInfo_t* addrInfoPtr,
                taf_doip_Result_t result, void* userPtr);

            le_result_t SendUDSResp( uint16_t sa, uint16_t ta, uint8_t addrType, uint8_t serviceId,
                    uint8_t err, const uint8_t* dataPtr, uint16_t dataSize);

            le_result_t SetNRC(uint8_t sid, uint8_t errorCode);
            le_result_t SendNRC(uint8_t sid, uint8_t errorCode, taf_doip_AddrInfo_t*  addrInfoPtr);
            void SendData(taf_doip_AddrInfo_t*  addrInfoPtr);
            le_result_t CheckAndSendInd(uint8_t sid, taf_doip_AddrInfo_t* addrInfoPtr,
                    taf_doip_DiagMsg_t* diagMsgPtr);

            static void P2StarTimeoutHandler(le_timer_Ref_t timerRef);
            static void S3TimeoutHandler(le_timer_Ref_t timerRef);

            le_ref_MapRef_t udsHandlerRefMap = NULL;
            taf_UDSIndicationHandler_t udsIndicationHandler;

        private:
            // Indicate recevied service message to Diag service if necessary.
            le_result_t IndicateSessionCtrlReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // SessionCtrl service (0x10).
            le_result_t IndicateECUResetReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // ECUReset service (0x11).
            le_result_t IndicateReadDIDReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // ReadDID service (0x22).
            le_result_t IndicateWriteDIDReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // WriteDID service (0x2E).
            le_result_t IndicateSecAccessReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // SecurrityAccess service (0x27).
            le_result_t IndicateIOCBIDReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // InputOutputControlByIdentifier service (0x2F).
            le_result_t IndicateRoutinrCtrlReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // RoutineControl service (0x31).
            le_result_t IndicateRxFileXferReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // RequestFileTransfer service (0x38).
            le_result_t IndicateRxXferDataReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // TransferData service (0x36).
            le_result_t IndicateRxXferExitReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // RequestTransferExit service (0x37).
            le_result_t IndicateClearDiagInfoReq(taf_doip_AddrInfo_t*  addrInfoPtr,
                    bool* isInternalHandle);    // ClearDiagnosticInformation service (0x14)
            le_result_t IndicateCtrlDTCSettingReq(taf_doip_AddrInfo_t*  addrInfoPtr,
                    bool* isInternalHandle);    // ControlDTCSetting service (0x85)
            le_result_t IndicateReadDTCInfoReq(taf_doip_AddrInfo_t*  addrInfoPtr,
                    bool* isInternalHandle);    // ReadDTCInfo service (0x19)

            // Internally check and Respond UDS message to uds client (through DoIP stack).
            le_result_t ReadDTCInfoResp(taf_doip_AddrInfo_t* addrInfoPtr);    // (0x19).
            le_result_t TesterPresentResp(taf_doip_AddrInfo_t*  addrInfoPtr);    // (0x3E)

            // To read DTC from ConfigTree.
            uint8_t readDTCByStatusMask(uint8_t statusMask);

            // Send UDS response message from Diag service.
            le_result_t SessionCtrlResp(uint8_t serviceId, uint8_t err);
            le_result_t ECUResetResp(uint8_t serviceId, uint8_t err);
            le_result_t ReadDIDResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);
            le_result_t WriteDIDResp(uint8_t serviceId, uint8_t err);
            le_result_t SecurityAccessResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);
            le_result_t IOCBIDResp(uint8_t serviceId, const uint8_t* dataPtr, uint16_t dataSize,
                    uint8_t err);
            le_result_t RoutineCtrlResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);
            le_result_t XferDataResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);
            le_result_t ReqXferExitResp(uint8_t serviceId, uint8_t err);
            le_result_t ReqFileXferResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);

            static void IndicateWhenChangingToDefault();
            le_result_t ReadDTCInfoResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);
            le_result_t ClearDiagInfoResp(uint8_t serviceId, uint8_t err);
            le_result_t CtrlDTCSettingResp(uint8_t serviceId, uint8_t err);

            static void* UdsTimerThread(void* ctxPtr);
            static void UdsTimerHandler(void* reqPtr);
            void UdsTimerEventReport(taf_UDSTimer_EventType_t timerEvent, uint32_t interval);
            bool IsSessTypeMatched(cfg::Node& node);
            bool IsSecurityAccessMatched(cfg::Node& node);
            bool IsRequestSubFuncSupported(cfg::Node& node, uint8_t subFunc);

            // update status parameter.
            bool isXferActive = false;

            // Security access request seed parameter.
            uint8_t reqSeedLevel = 0;
            // Security access level.
            uint8_t securityLevel = 0;

            //session change parameter.
            uint8_t sesChangeId = 0xFF;
            taf_doip_AddrInfo_t addrInfo;
            uint8_t sesChangeBuf[UDS_SESSION_CHANGE_DATA_SIZE];

            taf_doip_Ref_t  DoipEntityRef = NULL;
            taf_doip_DiagIndicationHandlerRef_t IndicationRef = NULL;
            taf_doip_PowerModeQueryHandlerRef_t PmQueryRef = NULL;
            taf_doip_DiagConfirmHandlerRef_t ConfirmRef = NULL;
            taf_SessionType_t SessionType = DEFAULT_SESSION;
            uint8_t recvBuf[UDS_DATA_SIZE];
            uint8_t sendBuf[UDS_DATA_SIZE];
            uint16_t recvDataLen = 0;
            uint16_t sendDataLen = 0;
            bool readyToRecvData = true;
            le_timer_Ref_t p2StarTimerRef;
            le_timer_Ref_t s3TimerRef;
            le_event_Id_t udsTimerEventId;
            le_sem_Ref_t semRef;
    };
}
}
#endif  // TAFUDS_COMMUNICATION_MGR_HPP
