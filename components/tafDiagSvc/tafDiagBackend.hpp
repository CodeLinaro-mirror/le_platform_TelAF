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
#ifndef TAF_DIAG_BACKEND_HPP
#define TAF_DIAG_BACKEND_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#ifndef LE_CONFIG_DIAG_VSTACK
#include "tafUDSStack.h"
#endif

#include <map>

#ifdef LE_CONFIG_DIAG_VSTACK
    // Enumeration of supported logical target address types.
    typedef enum
    {
        TAF_UDS_TA_TYPE_PHYSICAL   = 0x00,    ///< Physical addressing.
        TAF_UDS_TA_TYPE_FUNCTIONAL = 0x01     ///< Functional addressing.
    }taf_uds_TaType_t;

    // Logical address information structure in uds communication.
    typedef struct
    {
        uint16_t            sa;         ///< Source address of message senders.
        uint16_t            ta;         ///< Target address of message recipients.
        taf_uds_TaType_t   taType;      ///< Target address type of message recipients.
    }taf_uds_AddrInfo_t;
#endif

namespace telux {
namespace tafsvc {
    #define TAF_STACK_CONFIG_PATH   "./tafDoIP.json"

    typedef enum
    {
        TAF_DIAG_SUCCESS = 0,                             ///< Successful.
        TAF_DIAG_SERVICE_NOT_SUPPORTED = 0x11,            ///< Service is not supported
        TAF_DIAG_SUBFUNCTION_NOT_SUPPORTED = 0x12,        ///< SubFunction is not supported.
        TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT = 0x13, ///< Message length not correct.
        TAF_DIAG_CONDITION_NOT_CORRECT = 0x22,            ///< Prerequisite conditions not correct.
        TAF_DIAG_REQUEST_SEQUENCE_ERROR = 0x24,           ///< Request sequence error.
        TAF_DIAG_REQUEST_OUT_OF_RANGE = 0x31              ///< Parameter is out of range.
    }taf_diag_ErrorCode_t;

    // Define the interface for each service.
    class taf_UDSInterface
    {
        public:
            // UDS message handler
            virtual void UDSMsgHandler(const taf_uds_AddrInfo_t* addrPtr,
                uint8_t sid, uint8_t* msgPtr, size_t msgLen) = 0;
    };

    class taf_DiagBackend
    {
        public:
            taf_DiagBackend(){};
            ~taf_DiagBackend(){};

            static taf_DiagBackend& GetInstance();
            void Init();

#ifndef LE_CONFIG_DIAG_VSTACK
            // Registered to UDS stack for reception message.
            static void UdsIndicationHanler(const taf_uds_AddrInfo_t*         addrInfoPtr,
                const taf_uds_DiagMsg_t* diagMsgPtr, le_result_t result, void* userPtr);
#else
            //Registered to KPIT stack for reception messsage
            static void IntIndicationHandler( taf_diagBackend_DiagInfRef_t ref,
                const taf_diagBackend_AddrInfo_t *addrInfoPtr, uint8_t serviceID,
                    const uint8_t* msgPtr, size_t msgSize, void* contextPtr);
#endif

            // UDS layer service identifier.
            le_result_t RegisterUdsService(uint8_t sid, taf_UDSInterface* service);
            void UnregisterUdsService(uint8_t sid);

            // Can be used in each service to respond the request.
            le_result_t RespDiagPositive(uint8_t sid, const taf_uds_AddrInfo_t* addrInfoPtr,
                const uint8_t* dataPtr, size_t dataLen);
            le_result_t RespDiagPositive(uint8_t sid, const taf_uds_AddrInfo_t* addrInfoPtr);
            le_result_t RespDiagNegative(uint8_t sid, const taf_uds_AddrInfo_t* addrInfoPtr, uint8_t nrc);
        private:
            le_result_t InitUdsStack();
            void DeInitUdsStack();
#ifndef LE_CONFIG_DIAG_VSTACK
            taf_uds_DiagIndicationHandlerRef_t udsIndHandlerRef = NULL;
#else
            taf_diagBackend_DiagEventHandlerRef_t integrationIndHandlerRef = NULL;
#endif

            uint8_t regstFlag = 0;
            std::map<uint8_t, taf_UDSInterface *> svcMap;
#ifdef LE_CONFIG_DIAG_VSTACK
            std::map<uint8_t, taf_diagBackend_DiagInfRef_t> refMap;
#endif
    };
}
}
#endif
