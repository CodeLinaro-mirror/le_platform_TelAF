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

#ifndef TAF_DIAG_DOIP_SVR_HPP
#define TAF_DIAG_DOIP_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafDiagBackend.hpp"
#include "tafDoIPStack.h"

#define DEFAULT_SVC_REF_CNT 16
#define DEFAULT_SESSION_REF_CNT 16

typedef struct
{
    taf_diagDoIP_ServiceRef_t ref;              ///< Own reference.
    uint16_t id;
    taf_doip_Ref_t doipRef;                     ///< DoIP reference.
    taf_doip_EventHandlerRef_t doiphandlerRef;  ///< DoIP event hander reference.
    le_dls_List_t sessionList;                  ///< Client-server session ref.
}taf_DoIPSVC_t;

typedef struct
{
    le_msg_SessionRef_t sessionRef;
    taf_diagDoIP_EventHandlerFunc_t func;
    void *ctxPtr;
    taf_DoIPSVC_t *svrPtr;
    void *safeRef;
    le_dls_Link_t link;
}taf_DoIPSession_t;

namespace telux {
namespace tafsvc {
    class taf_DiagDoIPSvr : public ITafSvc
    {
        public:
            taf_DiagDoIPSvr() {};
            ~taf_DiagDoIPSvr() {};
            static taf_DiagDoIPSvr& GetInstance();
            void Init();

            static void DoIPEventHandler(taf_doip_Ref_t doipRef, taf_doip_Event_t event,
                    uint16_t remoteAddr, void* userPtr);
            static void OnClientDisconnection(le_msg_SessionRef_t sessionRef,
                    void *contextPtr);

            taf_diagDoIP_ServiceRef_t FindOrCreateService(uint16_t identifier);
            le_result_t RemoveService(taf_diagDoIP_ServiceRef_t svcRef);
            taf_diagDoIP_EventHandlerRef_t AddEventHandler(
                    taf_diagDoIP_ServiceRef_t svcRef,
                    taf_diagDoIP_EventHandlerFunc_t handlerPtr,
                    void* contextPtr);
            void RemoveEventHandler(taf_diagDoIP_EventHandlerRef_t handlerRef);
            le_result_t SetVIN(const char* vin);
            le_result_t GetVIN(char* vin, size_t vinSize);
            le_result_t SetEID(const char* eid);
            le_result_t GetEID(char* eid, size_t eidSize);
            le_result_t SetGID(const char* gid);
            le_result_t GetGID(char* gid, size_t gidSize);
        private:
            // For Diag DoIP service object
            le_mem_PoolRef_t svcPool;
            le_ref_MapRef_t svcRefMap;

            le_mem_PoolRef_t sessPool;
            le_ref_MapRef_t sessRefMap;

            taf_DoIPSVC_t* GetServiceObj(uint16_t identifier);
            taf_DoIPSession_t *AddSessionToService(taf_DoIPSVC_t* servicePtr,
                    le_msg_SessionRef_t sessionRef);
            le_result_t RemoveSessionFromService(taf_DoIPSVC_t* servicePtr,
                    le_msg_SessionRef_t sessionRef);
            le_result_t SetSessionEventHandler(taf_DoIPSession_t *sessionPtr,
                    taf_diagDoIP_EventHandlerFunc_t handlerPtr, void* contextPtr);
    };
}
}
#endif