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

#ifndef TAF_DTC_SVR_HPP
#define TAF_DTC_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafDiagBackend.hpp"


// Response data size.
#define RESPONSE_DATA_SIZE 4095

// ReadDTCInformation service (0x19)
#define NO_OF_DTC_BY_STATUS_MASK_REQ_LEN 3
#define NO_OF_DTC_BY_STATUS_MASK_RESP_BASE_LEN 4
#define DTC_BY_STATUS_MASK_REQ_LEN 3
#define DTC_BY_STATUS_MASK_RESP_BASE_LEN 1
#define DTC_EXT_DATA_REC_BY_DTC_NO_REQ_LEN 6
#define DTC_EXT_DATA_REC_BY_DTC_NO_RESP_BASE_LEN 4
#define SUPPORTED_DTC_REQ_LEN 2
#define SUPPORTED_DTC_RESP_BASE_LEN 1
#define DTC_FAULT_DETECTION_COUNTER_REQ_LEN 2
#define DTC_FAULT_DETECTION_COUNTER_RESP_BASE_LEN 0

// ClearDiagnosticInformation service (0x14)
#define CLEAR_DTC_INFO_REQ_MIN_LEN 4
#define CLEAR_DTC_INFO_RESP_LEN 0

//-------------------------------------------------------------------------------------------------
/**
 * DTC subfunction.
 */
//-------------------------------------------------------------------------------------------------
typedef enum
{
    REPORT_NO_OF_DTC_BY_STATUS_MASK = 0x01,
    REPORT_DTC_BY_STATUS_MASK = 0x02,
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

namespace telux
{
    namespace tafsvc
    {
        class taf_DTCInf : public ITafSvc, public taf_UDSInterface
        {
            public:
                taf_DTCInf(){};
                ~taf_DTCInf(){};

                static taf_DTCInf& GetInstance();
                void Init();

                // UDS message handler.
                void UDSMsgHandler(const taf_uds_AddrInfo_t* addrPtr, uint8_t sid, uint8_t* msgPtr,
                        size_t msgLen) override;

                le_result_t SendDTCResp(uint8_t sid, const taf_uds_AddrInfo_t* addrInfoPtr,
                        const uint8_t* dataPtr, size_t dataLen);

                // Subfunction reportNumberOfDTCByStatusMask (0x01)
                le_result_t GetNumOfDtcByStatusMask(uint8_t statusMask);

                // Subfunction reportDTCByStatusMask (0x02)
                le_result_t GetDtcByStatusMask(uint8_t statusMask);

                // Subfunction reportDTCExtDataRecordByDTCNumber (0x06)
                le_result_t GetExtDataRecordByDTCNum(uint32_t dtcMaskRec, uint8_t dtcExtDataRec);

                // Subfunction reportSupportedDTC (0x0A)
                le_result_t GetSupportedDtc();

                // Subfunction reportDTCFaultDetectionCounter (0x14)
                le_result_t GetFaultDetCounter();

                // ClearDiagnosticInformation service (0x14)
                le_result_t GetClearDTCResp(uint32_t grpOfDTC);

            private:

                // Send NRC response msg.
                le_result_t SendNRCResp(uint8_t sid, const taf_uds_AddrInfo_t*  addrInfoPtr,
                        uint8_t errCode);

                uint8_t reqReadDTCSvcId = 0x19;    // ReadDTC request service ID.
                uint8_t respReadDTCSvcId = 0x59;   // ReadDTC response service ID.
                uint8_t reqClearDTCSvcId = 0x14;   // ClearDTC request service ID.
                uint8_t respClearDTCSvcId = 0x54;  // ClearDTC response service ID.
                uint16_t logAddr;                  // Service logic address.

                uint8_t respBuf[RESPONSE_DATA_SIZE];
                uint16_t respBufLen = 0;
        };
    }
}
#endif /* TAF_DTC_SVR_HPP */