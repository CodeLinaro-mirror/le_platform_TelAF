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

#ifndef TAF_SECBACKEND_SVR_HPP
#define TAF_SECBACKEND_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#define DEFAULT_MSG_REF_CNT 16

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic SessionControl service Req message.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagSecBackend_SesTypeCheckRef_t sesTypeRef; ///< reference.
    uint8_t svcId;                                   ///< Service Identifier.
    taf_diagSecBackend_SessionType_t sesType;        ///< Rer subFunction.
    le_dls_Link_t link;                              ///< Link to the Rx msg list.
}taf_SesCtrlReqMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic Session change notification message.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagSecBackend_SessionType_t prev_session;       ///< Previous session type.
    taf_diagSecBackend_SessionType_t current_session;    ///< Current active session type.
} taf_SesCtrlChangeMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * SessionCtrl service Resp message structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagSecBackend_SesTypeCheckRef_t sesTypeRef;   ///< Reference
    taf_diagSecBackend_SesControlErrorCode_t errCode;  ///< NRC error code
}taf_SesCtrlRespMsg_t;

namespace telux
{
    namespace tafsvc
    {
        class taf_SecBackend : public ITafSvc
        {
            public:
                taf_SecBackend(){};
                ~taf_SecBackend(){};

                static taf_SecBackend &GetInstance();
                void Init();

                // Session control
                static void SesCtrlEventHandler(void* reportPtr, void* subHandlerFunc);
                taf_diagSecBackend_SesTypeCheckHandlerRef_t AddSesTypeCheckHandler(
                        taf_diagSecBackend_SesTypeHandlerFunc_t handlerPtr,
                                void* contextPtr);
                void RemoveSesTypeCheckHandler(taf_diagSecBackend_SesTypeCheckHandlerRef_t
                        handlerRef);
                le_result_t SendSesTypeCheckResp(taf_diagSecBackend_SesTypeCheckRef_t sesTypeRef,
                        taf_diagSecBackend_SesControlErrorCode_t errCode);

                // Session change indication function
                static void SesTypeChangeHandler(void* reportPtr, void* subHandlerFunc);
                taf_diagSecBackend_SesChangeHandlerRef_t AddSesChangeHandler(
                        taf_diagSecBackend_SesChangeHandlerFunc_t handlerPtr,
                                void* contextPtr);
                void RemoveSesChangeHandler(taf_diagSecBackend_SesChangeHandlerRef_t handlerRef);

                // SessionCtrl internal request event-handler.
                le_event_Id_t SesCtrlReqEvtId;
                static void SesCtrlReqEvtHandler(void* sesCtrlReqPtr);

                // Session change notification internal event-handler.
                le_event_Id_t SesChangeEvtId;
                static void SesChangeEvtHandler(void* sesChangePtr);

            private:
                // SessionCtrl event-handler
                le_event_Id_t DSCEvent;
                le_event_HandlerRef_t DSCEventHandlerRef;

                // Session change notification event-handler
                le_event_Id_t DSCChangeEvent;
                le_event_HandlerRef_t DSCChangeEventHandlerRef;

                le_mem_PoolRef_t RespSesMsgPool;
        };
    }
}
#endif /* #ifndef TAF_SECBACKEND_SVR_HPP */