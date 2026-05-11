/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_IDPS_SERVER_HPP
#define TAF_IDPS_SERVER_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafUDSStack.h"

#define DEFAULT_IDPS_SVC_REF_CNT        16
#define DEFAULT_IDPS_STATUS_MSG_REF_CNT 16
#define DEFAULT_IDPS_HANDLER_REF_CNT    16

// Service IDs used for HIGH_LEVEL filtering.
#define IDPS_SID_SECURITY_ACCESS  0x27   ///< Security access service ID.
#define IDPS_SID_AUTHENTICATION   0x29   ///< Authentication service ID.

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic IDPS service structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagIDPS_ServiceRef_t svcRef;               ///< Own reference.
    taf_diagIDPS_SecurityLevel_t securityLevel;     ///< Security level for this service.
    le_dls_List_t statusMsgList;                    ///< Status message list of the service.
    taf_diagIDPS_StatusHandlerRef_t statusHandlerRef; ///< Status handler ref of the service.
    le_msg_SessionRef_t sessionRef;                 ///< Reference to a client-server session.
    le_dls_List_t supportedVlanList;                ///< VLAN ID list.
} taf_IdpsSvc_t;

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic IDPS status message payload.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagIDPS_SecurityLevel_t securityLevel; ///< Security level.
    uint8_t serviceId;                          ///< Service ID.
    uint8_t status;                             ///< 0 = successful, or NRC.
    uint16_t dataLen;                           ///< Length of data field.
    uint8_t data[TAF_DIAGIDPS_MAX_DATA_SIZE];   ///< Data payload.
    uint16_t extraDataLen;                      ///< Length of extra data field.
    uint8_t extraData[TAF_DIAGIDPS_MAX_DATA_SIZE]; ///< Extra data payload.
} taf_IdpsStatusPayload_t;

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic IDPS status message structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagIDPS_StatusMsgRef_t statusMsgRef;   ///< Own reference.
    taf_uds_IdpsAddrInfo_t addrInfo;            ///< Idps logical address information structure.
    le_dls_Link_t link;                         ///< Link to the status message list.
    taf_IdpsStatusPayload_t payload;            ///< Status message payload.
    taf_diagIDPS_ServiceRef_t ownerSvcRef;      ///< Ref of the service that owns this message.
} taf_IdpsStatusMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic IDPS status handler structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagIDPS_StatusHandlerRef_t handlerRef; ///< Own reference.
    taf_diagIDPS_ServiceRef_t svcRef;           ///< Service reference.
    taf_diagIDPS_StatusHandlerFunc_t func;      ///< Handler function.
    void* ctxPtr;                               ///< Handler context.
} taf_IdpsStatusHandler_t;

//-------------------------------------------------------------------------------------------------
/**
 * VLAN ID structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;
    uint16_t vlanId;
} taf_IdpsVlanIdNode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag IDPS Server Service Class
 */
//--------------------------------------------------------------------------------------------------
namespace tafsvc
{
    class taf_IdpsSvr : public ITafSvc
    {
        public:
            taf_IdpsSvr() {};
            ~taf_IdpsSvr() {};
            void Init();
            static taf_IdpsSvr& GetInstance();

            static void OnClientDisconnection(le_msg_SessionRef_t sessionRef,
                    void* contextPtr);

            // UDS IDPS indication handler.
            static void IdpsIndicationHandler(const taf_uds_IdpsAddrInfo_t* addrInfoPtr,
                    const taf_uds_IdpsStatusInfo_t* diagMsgPtr, void* userPtr);

            static void StatusEventHandler(void* reportPtr);


            taf_diagIDPS_ServiceRef_t GetService(taf_diagIDPS_SecurityLevel_t securityLevel);
            le_result_t RemoveSvc(taf_diagIDPS_ServiceRef_t svcRef);

            le_result_t SetVlanId(taf_diagIDPS_ServiceRef_t svcRef, uint16_t vlanId);

            taf_diagIDPS_StatusHandlerRef_t AddStatusHandler(
                    taf_diagIDPS_ServiceRef_t svcRef,
                    taf_diagIDPS_StatusHandlerFunc_t handlerPtr,
                    void* contextPtr);
            void RemoveStatusHandler(taf_diagIDPS_StatusHandlerRef_t handlerRef);

            le_result_t GetVlanIdFromMsg(taf_diagIDPS_StatusMsgRef_t statusMsgRef,
                    uint16_t* vlanIdPtr);
            le_result_t GetLogicalAddr(taf_diagIDPS_StatusMsgRef_t statusMsgRef,
                    uint16_t* sourceAddrPtr, uint16_t* targetAddrPtr);
            le_result_t GetDataSize(taf_diagIDPS_StatusMsgRef_t statusMsgRef,
                    uint16_t* sizePtr);
            le_result_t GetData(taf_diagIDPS_StatusMsgRef_t statusMsgRef,
                    uint8_t* dataPtr, size_t* dataSizePtr);
            le_result_t GetExtraDataSize(taf_diagIDPS_StatusMsgRef_t statusMsgRef,
                    uint16_t* sizePtr);
            le_result_t GetExtraData(taf_diagIDPS_StatusMsgRef_t statusMsgRef,
                    uint8_t* dataPtr, size_t* dataSizePtr);
            le_result_t ReleaseStatusMsg(taf_diagIDPS_StatusMsgRef_t statusMsgRef);

        private:
            void ClearMsgList(taf_IdpsSvc_t* servicePtr);
            void ClearVlanList(taf_IdpsSvc_t* servicePtr);
            taf_IdpsSvc_t* GetServiceObj(le_msg_SessionRef_t sessionRef);
            taf_IdpsSvc_t* GetServiceObj(uint16_t vlanId,
                    taf_diagIDPS_SecurityLevel_t securityLevel);

            // Service and event object
            le_mem_PoolRef_t SvcPool;
            le_ref_MapRef_t SvcRefMap;

            // Status message resource
            le_mem_PoolRef_t StatusMsgPool;
            le_ref_MapRef_t StatusMsgRefMap;

            // Status handler object
            le_mem_PoolRef_t StatusHandlerPool;
            le_ref_MapRef_t StatusHandlerRefMap;

            le_mem_PoolRef_t VlanPool;

            // IDPS indication handler reference.
            taf_uds_IdpsIndicationHandlerRef_t idpsIndHandlerRef;

            // Event for service.
            le_event_Id_t StatusEvent;
            le_event_HandlerRef_t StatusEventHandlerRef;
    };
}

#endif /* TAF_IDPS_SERVER_HPP */
