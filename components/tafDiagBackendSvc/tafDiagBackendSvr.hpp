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


#ifndef TAF_DIAGBACKEND_SVR_HPP
#define TAF_DIAGBACKEND_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#define DEFAULT_MSG_REF_CNT 16
#define DID_LEN 2
#define DATA_RECORD_REQ_MAX_SIZE 4092
#define READ_DID_REQ_BASE_LEN 3
#define WRITE_DID_REQ_BASE_LEN 3
#define SES_CONTROL_REQ_BASE_LEN 2
#define SES_CHANGE_REQ_BASE_LEN 3
#define SECURITY_ACCESS_REQ_BASE_LEN 2
#define REQUEST_FILE_TRANSFER_REQ_BASE_LEN 4
#define TRANSFER_DATA_REQ_BASE_LEN 2
#define REQUEST_TRANSFER_EXIT_REQ_BASE_LEN 1
#define ECU_RESET_REQ_BASE_LEN 2
#define ROUTINE_CONTROL_REQ_BASE_LEN 4
#define IO_CTRL_REQ_BASE_LEN 4

typedef enum
{
    SID_DIAGNOSTIC_SESSION_CONTROL = 0x10,
    SID_ECU_RESET = 0x11,
    SID_CLEAR_DIAGNOSTIC_INFO = 0x14,
    SID_READ_DTC_INFO = 0x19,
    SID_READ_DATA_BY_IDENTIFIER = 0x22,
    SID_SECURITY_ACCESS = 0x27,
    SID_WRITE_DATA_BY_IDENTIFIER = 0x2E,
    SID_ROUTINE_CONTROL = 0x31,
    SID_TRANSFER_DATA = 0x36,
    SID_REQUEST_TRANSFER_EXIT = 0x37,
    SID_REQUEST_FILE_TRANSFER = 0x38,
    SID_CONTROL_DTC_SETTING = 0x85,
    SID_IO_CTRL = 0x2F,
    SID_DIAGNOSTIC_SESSION_CHANGE = 0xff
}taf_diagBackend_ServiceId_t;

//-------------------------------------------------------------------------------------------------
/**
 * Service Rx message structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagBackend_DiagInfRef_t  ref;                      ///< Reference
    uint8_t data[TAF_DIAGBACKEND_MAX_PAYLOAD_SIZE];         ///< DataPtr
    size_t  dataLen;                                        ///< DataLen
    uint8_t  svcId;                                         ///< Service Identifier.
    le_dls_Link_t link;                                     ///< for list
}taf_DiagReqMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * Service Resp message structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagBackend_DiagInfRef_t  ref;                      ///< Reference
    uint8_t  svcId;                                         ///< Service Identifier.
    uint8_t errCode;                                        ///< Error Code
    uint8_t dataRec[TAF_DIAGBACKEND_MAX_PAYLOAD_SIZE];      ///< DataPtr
    size_t  dataLen;                                        ///< DataLen
}taf_DiagRespMsg_t;

namespace telux
{
    namespace tafsvc
    {
        class taf_DiagBackend : public ITafSvc
        {
            public:
                taf_DiagBackend(){};
                ~taf_DiagBackend(){};

                void Init();
                static taf_DiagBackend& GetInstance();

                static void DiagEventHandler(void* reportPtr, void* subHandlerFunc);
                // Add Event handler to get request message
                taf_diagBackend_DiagEventHandlerRef_t AddDiagEventHandler(
                        taf_diagBackend_DiagHandlerFunc_t handlerPtr, void* contextPtr);

                // Add Event handler to get request message
                void RemoveDiagEventHandler(taf_diagBackend_DiagEventHandlerRef_t handlerRef);

                // Add Event handler to get request message
                le_result_t SendResp( taf_diagBackend_DiagInfRef_t ref,
                        const taf_diagBackend_AddrInfo_t* addrInfoPtr, uint8_t serviceID,
                                uint8_t errCode, const uint8_t* dataPtr, size_t dataSize);

                le_event_Id_t DiagEventReqEvtId;
                static void DiagEventReqEvtHandler(void* didReqPtr);

            private:

                le_event_Id_t DiagEvent;
                le_event_HandlerRef_t DiagEventHandlerRef;
                le_mem_PoolRef_t RespMsgPool;
        };
    }
}
#endif
