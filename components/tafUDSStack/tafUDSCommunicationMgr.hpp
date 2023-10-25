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

namespace taf{
namespace uds{

    #define UDS_DATA_SIZE 4095
    #define UDS_P2_SERVER 2000
    #define UDS_P2_STAR_SERVER 3000
    #define TAF_UDS_HANDLER_REF_CNT 1

    // DID Config tree definition
    #define DID_NODE_LEN                 30
    #define DID_CONFIG_TREE_NODE         "diag/DID"
    #define DID_CONFIG_TREE_DATA_FORMAT  "diag/DID/%2x"
    #define DID_DATA_FORMAT              "data%d"

    // DTC Config tree definition
    #define DTC_CONFIG_TREE_NODE                   "diag/DTC"
    #define DTC_STATUS_AVAILABILITY_MASK           "DTCStatusAvailabilityMask"
    #define DTC_INFORMATION                        "info"
    #define DTC_STR_INFO_LEN                       20
    #define DTC_STATUS                             "status"
    #define DTC_SUB_FUNCTION_REPORT_DTC_BY_STATUS  0x2

    // Negative Response (0x7F)
    #define UDS_NEGATIVE_RESP_SID 0x7F
    #define UDS_NEG_RESP_LEN 3

    // Session control service (0x10)
    #define UDS_SESSION_CTRL_REQ_MIN_LEN 2
    #define UDS_SESSION_CTRL_RESP_LEN 6

    // ECUReset service (0x11)
    #define UDS_ECU_RESET_REQ_MIN_LEN 2
    #define UDS_ECU_RESET_RESP_BASE_LEN 2

    // ReadDTCInformation service (0x19)
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN 3
    #define UDS_READ_DTC_INFO_RESP_BASE_LEN 3

    // ReadDataByIdentifier service (0x22)
    #define UDS_READ_DID_REQ_MIN_LEN 3
    #define UDS_DID_LEN 2

    // WriteDataByIdentifier service (0x2E)
    #define UDS_WRITE_DID_REQ_MIN_LEN 4
    #define UDS_WRITE_DID_REQ_BASE_LEN 3  // Service ID(1) + DID (2)
    #define UDS_WRITE_DID_RESP_LEN 3

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
    #define UDS_REQ_FILE_XFER_BASE_LEN 4
    #define UDS_REQ_FILE_XFER_DATA_FORMAT_ID 1
    #define UDS_REQ_FILE_XFER_FILE_SIZE_PARAMETER_LEN 1
    #define UDS_REQ_FILE_XFER_DATA_FORMAT_ID_LEN 1
    #define UDS_RESP_FILE_XFER_BASE_LEN 2
    #define UDS_RESP_FILE_XFER_LEN_FORMAT_ID_LEN 1
    #define UDS_RESP_FILE_XFER_DATA_FORMAT_ID_LEN 1

    // RequestFileTranser service mode of operation type
    typedef enum
    {
        ADD_FILE = 0x01,
        DELETE_FILE = 0x02,
        REPLACE_FILE = 0x03,
        READ_FILE = 0x04,
        READ_DIR = 0x05,
        RESUME_FILE = 0x06
    }taf_UDSReqFileXferMOOPType_t;

    // Diagnostic Request service ID
    typedef enum
    {
        SESSION_CONTROL_REQUEST_ID = 0x10,
        ECU_RESET_REQUEST_ID = 0x11,
        READ_DTC_INFO_REQUEST_ID = 0x19,
        READ_DID_REQUEST_ID = 0x22,
        WRITE_DID_REQUEST_ID = 0x2E,
        ROUTINE_CONTROL_REQUEST_ID = 0x31,
        TRANSFER_DATA_REQUEST_ID = 0x36,
        REQUEST_TRANSFER_EXIT_REQUEST_ID = 0x37,
        REQUEST_FILE_TRANSFER_REQUEST_ID = 0x38
    }taf_UDSReqSvcID_t;

    // Diagnostic Response service ID
    typedef enum
    {
        SESSION_CONTROL_RESPONSE_ID = 0x50,
        ECU_RESET_RESPONSE_ID = 0x51,
        READ_DTC_INFO_RESPONSE_ID = 0x59,
        READ_DID_RESPONSE_ID = 0x62,
        WRITE_DID_RESPONSE_ID = 0x6E,
        ROUTINE_CONTROL_RESPONSE_ID = 0x71,
        TRANSFER_DATA_RESPONSE_ID = 0x76,
        REQUEST_TRANSFER_EXIT_RESPONSE_ID = 0x77,
        REQUEST_FILE_TRANSFER_RESPONSE_ID = 0x78
    }taf_UDSRespSvcID_t;

    // UDS error code.
    typedef enum
    {
        POSITIVE_RESPONSE = 0,
        SERVICE_NOT_SUPPORTED = 0x11,
        SUBFUNCTION_NOT_SUPPORTED = 0x12,
        INCORRECT_MSG_LEN_OR_INVALID_FORMAT = 0x13,
        RESP_TOO_LONG = 0x14,
        CONDITIONS_NOT_CORRECT = 0x22,
        REQ_OUT_OF_RANGE = 0x31,
        UPLOAD_DOWNLOAD_NOT_ACCEPTED = 0x70,
        GENERAL_PROGRAMMING_FAILURE = 0x72
    }taf_UDSErrorCode_t;

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
        EXTENDED_DIAGNOSTIC_SESSION = 0x03
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

            le_result_t SendUDSResp( uint16_t sa, uint16_t ta, uint8_t addrType, uint8_t serviceId,
                    uint8_t err, const uint8_t* dataPtr, uint16_t dataSize);

            le_result_t SetNRC(uint8_t sid, uint8_t errorCode);
            le_result_t CheckAndSendInd(uint8_t sid, taf_doip_AddrInfo_t* addrInfoPtr,
                    taf_doip_DiagMsg_t* diagMsgPtr);

            static void P2TimeoutHandler(le_timer_Ref_t timerRef);

            le_ref_MapRef_t udsHandlerRefMap = NULL;
            taf_UDSIndicationHandler_t udsIndicationHandler;

        private:
            // Indicate recevied service message to Diag service.
            le_result_t IndicateECUResetReq();       // ECUReset service (0x11).
            le_result_t IndicateRoutinrCtrlReq();    // RoutineControl service (0x31).
            le_result_t IndicateRxFileXferReq();     // RequestFileTransfer service (0x38).
            le_result_t IndicateRxXferDataReq();     // TransferData service (0x36).
            le_result_t IndicateRxXferExitReq();     // RequestTransferExit service (0x37).

            // Internally check and Respond UDS message to uds client (through DoIP stack).
            le_result_t SessionCtrlResp();    // SessionControl service (0x10).
            le_result_t ReadDTCInfoResp();    // ReadDTCInformation service (0x19).
            le_result_t ReadDIDResp();        // ReadDataByIdentifier service (0x22).
            le_result_t WriteDIDResp();       // WriteDataByIdentifier service (0x2E).

            // To read and write from ConfigTree.
            uint8_t readDIDFromConfigTree(const uint16_t dataId);
            uint8_t writeDIDToConfigTree(const uint16_t dataId,
                    const uint8_t* dataPtr, uint16_t dataSize);
            uint8_t readDTCByStatusMask(uint8_t statusMask);

            // Send UDS response message from Diag service.
            le_result_t ECUResetResp(uint8_t serviceId, uint8_t err);
            le_result_t RoutineCtrlResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);
            le_result_t XferDataResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);
            le_result_t ReqXferExitResp(uint8_t serviceId, uint8_t err);
            le_result_t ReqFileXferResp(uint8_t serviceId, uint8_t err);

            // update status parameter.
            bool isXferActive = false;

            taf_doip_Ref_t  DoipEntityRef = NULL;
            taf_doip_DiagIndicationHandlerRef_t IndicationRef = NULL;
            taf_doip_PowerModeQueryHandlerRef_t PmQueryRef = NULL;
            taf_SessionType_t SessionType = DEFAULT_SESSION;
            uint8_t recvBuf[UDS_DATA_SIZE];
            uint8_t sendBuf[UDS_DATA_SIZE];
            uint16_t recvDataLen = 0;
            uint16_t sendDataLen = 0;
            le_timer_Ref_t timerRef;
    };
}
}
#endif  // TAFUDS_COMMUNICATION_MGR_HPP
