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

#ifndef TAF_DIDBACKEND_SVR_HPP
#define TAF_DIDBACKEND_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#define DEFAULT_MSG_REF_CNT 16

//-------------------------------------------------------------------------------------------------
/**
 * ReadDID service Rx message structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagDIDBackend_ReadDIDRef_t  readDIDRef;            ///< Reference
    uint8_t  svcId;                                         ///< Service Identifier.
    uint16_t   readDID;                                     ///< Read DID.
    le_dls_Link_t      link;                                ///< for list
}taf_ReadDIDReqMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * ReadDID service Resp message structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagDIDBackend_ReadDIDRef_t  readDIDRef;                  ///< Reference
	taf_diagDIDBackend_ReadDIDErrorCode_t errCode;                ///< NRC error code
	uint8_t  dataRec[TAF_DIAGDIDBACKEND_READ_DID_PAYLOAD_SIZE];   ///< Data record.
	uint16_t  dataRecLen;                                         ///< Data record length.
}taf_ReadDIDRespMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * WriteDID service Rx message structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagDIDBackend_WriteDIDRef_t  writeDIDRef;                        ///< Reference
    uint8_t  svcId;                                                       ///< Service Identifier.
    uint16_t  writeDID;                                                   ///< Write DID.
    uint8_t   dataRecord[TAF_DIAGDIDBACKEND_WRITE_DID_DATA_RECORD_SIZE];  ///< Data record.
    uint16_t  dataRecLen;                                                 ///< Write data record length.
    le_dls_Link_t      link;                                              ///< for list
}taf_WriteDIDReqMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * WriteDID service Resp message structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagDIDBackend_WriteDIDRef_t  writeDIDRef;  ///< Reference
	taf_diagDIDBackend_WriteDIDErrorCode_t errCode; ///< NRC error code
	uint16_t  writeDID;                             ///< Write DID.
}taf_WriteDIDRespMsg_t;


namespace telux
{
    namespace tafsvc
    {
        class taf_DIDBackend : public ITafSvc
        {
            public:
                taf_DIDBackend(){};
                ~taf_DIDBackend(){};

                static taf_DIDBackend& GetInstance();
                void Init();

                // ReadDID
                static void ReadDIDHandler(void* reportPtr, void* subHandlerFunc);
                taf_diagDIDBackend_ReadDIDHandlerRef_t AddReadDIDHandler(
                        taf_diagDIDBackend_ReadDIDHandlerFunc_t handlerPtr,
                                void* contextPtr);
                void RemoveReadDIDHandler(taf_diagDIDBackend_ReadDIDHandlerRef_t handlerRef);
                le_result_t SendReadDIDResp(taf_diagDIDBackend_ReadDIDRef_t readDIDRef,
                        taf_diagDIDBackend_ReadDIDErrorCode_t errCode, const uint8_t* dataPtr,
                                size_t dataSize);

                // WriteDID
                static void WriteDIDHandler(void* reportPtr, void* subHandlerFunc);
                taf_diagDIDBackend_WriteDIDHandlerRef_t AddWriteDIDHandler(
                        taf_diagDIDBackend_WriteDIDHandlerFunc_t handlerPtr,
                                void* contextPtr);
                void RemoveWriteDIDHandler(
                        taf_diagDIDBackend_WriteDIDHandlerRef_t handlerRef);
                le_result_t SendWriteDIDResp(taf_diagDIDBackend_WriteDIDRef_t writeDIDRef,
                        taf_diagDIDBackend_WriteDIDErrorCode_t errCode, uint16_t dataId);

                // ReadDID request event-handler.
                le_event_Id_t ReadDIDReqEvtId;
                static void ReadDIDReqEvtHandler(void* didReqPtr);

            private:
                // ReadDID event-handler
                le_event_Id_t ReadDIDEvent;
                le_event_HandlerRef_t ReadDIDEventHandlerRef;

                // WriteDID event-handler
                le_event_Id_t WriteDIDEvent;
                le_event_HandlerRef_t WriteDIDEventHandlerRef;

                le_mem_PoolRef_t RespDIDMsgPool;
        };
    }
}
#endif /* #ifndef TAF_DIDBACKEND_SVR_HPP */