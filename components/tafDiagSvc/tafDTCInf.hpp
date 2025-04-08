/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAF_DTC_SVR_HPP
#define TAF_DTC_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafDiagBackend.hpp"
#include "configuration.hpp"

// Response data size.
#define RESPONSE_DATA_SIZE 4095

// ReadDTCInformation service (0x19)
// Subfunction reportNumberOfDTCByStatusMask (0x01)
#define NO_OF_DTC_BY_STATUS_MASK_REQ_LEN 3
#define NO_OF_DTC_BY_STATUS_MASK_RESP_BASE_LEN 4

// Subfunction reportDTCByStatusMask (0x02)
#define DTC_BY_STATUS_MASK_REQ_LEN 3
#define DTC_BY_STATUS_MASK_RESP_BASE_LEN 1

// Subfunction reportDTCSnapshotIdentification (0x03)
#define DTC_SNAPSHOT_ID_REQ_LEN 2
#define DTC_SNAPSHOT_ID_RESP_BASE_LEN 0

// Subfunction reportDTCSnapshotRecordByDTCNumber (0x04)
#define DTC_SNAPSHOT_REC_BY_DTC_NUM_REQ_LEN 6
#define DTC_SNAPSHOT_REC_BY_DTC_NUM_RESP_BASE_LEN 4

// Subfunction reportDTCExtDataRecordByDTCNumber (0x06)
#define DTC_EXT_DATA_REC_BY_DTC_NO_REQ_LEN 6
#define DTC_EXT_DATA_REC_BY_DTC_NO_RESP_BASE_LEN 4

// Subfunction reportSupportedDTC (0x0A)
#define SUPPORTED_DTC_REQ_LEN 2
#define SUPPORTED_DTC_RESP_BASE_LEN 1

// Subfunction reportDTCFaultDetectionCounter (0x14)
#define DTC_FAULT_DETECTION_COUNTER_REQ_LEN 2
#define DTC_FAULT_DETECTION_COUNTER_RESP_BASE_LEN 0

// ClearDiagnosticInformation service (0x14)
#define CLEAR_DTC_INFO_REQ_MIN_LEN 4
#define CLEAR_DTC_INFO_RESP_LEN 0

#define FEATURE_A_PROGRAMMING_SESSION 0x2
#define FEATURE_A_FOTA_SESSION 0x42
#define FEATURE_A_APPLICATION_DTC_AVAILABILITY_MASK 0x9
#define FEATURE_A_REPROGRAMMING_DTC_AVAILABILITY_MASK 0x11

//-------------------------------------------------------------------------------------------------
/**
 * DTC subfunction.
 */
//-------------------------------------------------------------------------------------------------
typedef enum
{
    REPORT_NO_OF_DTC_BY_STATUS_MASK = 0x01,
    REPORT_DTC_BY_STATUS_MASK = 0x02,
    REPORT_DTC_SNAPSHOT_ID = 0x03,
    REPORT_DTC_SNAPSHOT_REC_BY_DTC_NO = 0x04,
    REPORT_DTC_EXT_DATA_REC_BY_DTC_NO = 0x06,
    REPORT_SUPPORTED_DTC = 0x0A,
    REPORT_DTC_FAULT_DETECTION_COUNTER = 0x14
}taf_diagReadDTC_SubFunc_t;

//-------------------------------------------------------------------------------------------------
/**
 * DTC error code.
 */
//-------------------------------------------------------------------------------------------------
typedef enum
{
    POSITIVE_RESPONSE = 0,
    SERVICE_NOT_SUPPORTED = 0x11,
    SUBFUNCTION_NOT_SUPPORTED = 0x12,
    INCORRECT_MSG_LEN_OR_INVALID_FORMAT = 0x13,
    CONDITIONS_NOT_CORRECT = 0x22,
    REQ_SEQUENCE_ERROR = 0x24,
    REQ_OUT_OF_RANGE = 0x31,
    GENERAL_PROGRAMMING_FAILURE = 0x72,
    REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING = 0x78
}taf_DTCErrorCode_t;

    namespace tafsvc
    {
        class taf_DTCInf : public ITafSvc, public taf_UDSInterface
        {
            public:
                taf_DTCInf(){};
                ~taf_DTCInf(){};

                static taf_DTCInf& GetInstance();
                void Init();

                bool IsDTCCurrentSesTypeConfig(uint32_t dtc, uint8_t currentSesType);

                // UDS message handler.
                void UDSMsgHandler(const taf_uds_AddrInfo_t* addrPtr, uint8_t sid, uint8_t* msgPtr,
                        size_t msgLen) override;

                le_result_t SendDTCResp(uint8_t sid, const taf_uds_AddrInfo_t* addrInfoPtr,
                        const uint8_t* dataPtr, size_t dataLen);

                // Subfunction reportNumberOfDTCByStatusMask (0x01)
                le_result_t GetNumOfDtcByStatusMask(uint8_t statusMask, uint16_t vlanId);

                // Subfunction reportDTCByStatusMask (0x02)
                le_result_t GetDtcByStatusMask(uint8_t statusMask, uint16_t vlanId);

                // Subfunction reportDTCSnapshotIdentification (0x03)
                le_result_t GetDtcSnapshotID(uint16_t vlanId);

                // Subfunction reportDTCSnapshotRecordByDTCNumber (0x04)
                le_result_t GetDtcSnapshotRecordByDTCNum(uint32_t dtcMaskRec,
                        uint8_t dtcRecNum, uint16_t vlanId);

                // Subfunction reportDTCExtDataRecordByDTCNumber (0x06)
                le_result_t GetExtDataRecordByDTCNum(uint32_t dtcMaskRec, uint8_t dtcExtDataRec,
                        uint16_t vlanId);

                // Subfunction reportSupportedDTC (0x0A)
                le_result_t GetSupportedDtc(uint16_t vlanId);

                // Subfunction reportDTCFaultDetectionCounter (0x14)
                le_result_t GetFaultDetCounter(uint16_t vlanId);

                // ClearDiagnosticInformation service (0x14)
                le_result_t GetClearDTCResp(uint32_t grpOfDTC);

            private:

                // Send NRC response msg.
                le_result_t SendNRCResp(uint8_t sid, const taf_uds_AddrInfo_t*  addrInfoPtr,
                        uint8_t errCode);
#ifdef LE_CONFIG_DIAG_FEATURE_A
                uint8_t GetAvailableStatusMaskByCurrentSession(uint8_t currentSesType);
#endif
                uint8_t reqReadDTCSvcId = 0x19;    // ReadDTC request service ID.
                uint8_t respReadDTCSvcId = 0x59;   // ReadDTC response service ID.
                uint8_t reqClearDTCSvcId = 0x14;   // ClearDTC request service ID.
                uint8_t respClearDTCSvcId = 0x54;  // ClearDTC response service ID.
                uint16_t logAddr;                  // Service logic address.

                uint8_t respBuf[RESPONSE_DATA_SIZE];
                uint16_t respBufLen = 0;
        };
    }
#endif /* TAF_DTC_SVR_HPP */