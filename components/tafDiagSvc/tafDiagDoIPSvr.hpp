/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAF_DIAG_DOIP_SVR_HPP
#define TAF_DIAG_DOIP_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafDiagBackend.hpp"
#include "tafDoIPStack.h"

#define DEFAULT_SVC_REF_CNT 16
#define DEFAULT_VLAN_REF_CNT 8

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
    le_dls_List_t vlanInfoList;
    taf_DoIPSVC_t *svrPtr;
    le_dls_Link_t link;
}taf_DoIPSession_t;

typedef struct
{
    uint16_t vlanId;
    taf_diagDoIP_EventHandlerFunc_t func;
    void *ctxPtr;
    void *safeRef;
    taf_DoIPSession_t *sessPtr;
    le_dls_Link_t link;
}taf_VlanCallback_t;

namespace tafsvc {
    class taf_DiagDoIPSvr : public ITafSvc
    {
        public:
            taf_DiagDoIPSvr() {};
            ~taf_DiagDoIPSvr() {};
            static taf_DiagDoIPSvr& GetInstance();
            void Init();

            static void DoIPEventHandler(taf_doip_Ref_t doipRef, taf_doip_Event_t event,
                    uint16_t remoteAddr, uint16_t fromVlanId, void* userPtr);
            static void OnClientDisconnection(le_msg_SessionRef_t sessionRef,
                    void *contextPtr);

            taf_diagDoIP_ServiceRef_t FindOrCreateService(uint16_t identifier);
            le_result_t RemoveService(taf_diagDoIP_ServiceRef_t svcRef);
            taf_diagDoIP_EventHandlerRef_t AddEventHandler(
                    taf_diagDoIP_ServiceRef_t svcRef,
                    uint16_t vlanId,
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
            le_mem_PoolRef_t vlanPool;
            le_ref_MapRef_t vlanRefMap;

            taf_DoIPSVC_t* GetServiceObj(uint16_t identifier);
            taf_DoIPSession_t *AddSessionToService(taf_DoIPSVC_t* servicePtr,
                    le_msg_SessionRef_t sessionRef);
            le_result_t RemoveSessionFromService(taf_DoIPSVC_t* servicePtr,
                    le_msg_SessionRef_t sessionRef);
            le_result_t SetSessionEventHandler(taf_DoIPSession_t *sessionPtr, uint16_t vlanId,
                    taf_diagDoIP_EventHandlerFunc_t handlerPtr, void* contextPtr);
            void RemoveAllVlanFromSession(taf_DoIPSession_t* sessionPtr);
            taf_VlanCallback_t* GetVlanInfoWithVlanId(taf_DoIPSession_t* sessionPtr,
                    uint16_t vlanId);
    };
}
#endif