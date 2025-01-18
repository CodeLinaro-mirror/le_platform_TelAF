/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef TAF_AUTH_SERVER_HPP
#define TAF_AUTH_SERVER_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafDiagBackend.hpp"

#define DEFAULT_SVC_REF_CNT 16
#define DEFAULT_RX_MSG_REF_CNT 16
#define DEFAULT_RX_HANDLER_REF_CNT 16

#define MAX_AUTH_RESP_LEN   (1+2+TAF_DIAGAUTH_MAX_CHALLENGE_SIZE+2+TAF_DIAGAUTH_MAX_PUBIC_KEY_SIZE)

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic Authentication service structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagAuth_ServiceRef_t svcRef;                    ///< Own reference.
    le_dls_List_t rxMsgList;                             ///< Rx message list of the service.
    taf_diagAuth_RxMsgHandlerRef_t rxHandlerRef;         ///< Rx Message handler ref of the service.
    taf_diagAuth_AuthStateExpHandlerRef_t expHandlerRef; ///< State expiry handler ref.
    le_msg_SessionRef_t sessionRef;                      ///< Reference to a client-server session.
    le_dls_List_t supportedVlanList;                     ///< VLAN ID list.
}taf_AuthSvc_t;

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic Authentication service Rx message.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t commConf;
    uint16_t certLen;
    uint8_t certData[TAF_DIAGAUTH_MAX_CERT_SIZE];
    uint16_t challengeLen;
    uint8_t challengeData[TAF_DIAGAUTH_MAX_CHALLENGE_SIZE];
}taf_AuthVerifyCertUniMsg_t;

typedef struct
{
    uint16_t POWNLen;
    uint8_t POWNData[TAF_DIAGAUTH_MAX_POWN_SIZE];
    uint16_t publicKeyLen;
    uint8_t publicKey[TAF_DIAGAUTH_MAX_PUBIC_KEY_SIZE];
}taf_AuthPOWNMsg_t;

typedef struct
{
    uint16_t certEvalId;
    uint16_t certLen;
    uint8_t certData[TAF_DIAGAUTH_MAX_CERT_SIZE];
}taf_AuthTransCertMsg_t;

typedef struct
{
    taf_diagAuth_RxMsgRef_t rxMsgRef;   ///< Own reference.
    taf_uds_AddrInfo_t addrInfo;        ///< Rx logical address information structure.
    uint8_t subFunc;                    ///< Rx subFunction.
    le_dls_Link_t link;                 ///< Link to the Rx message list.
    union
    {
        taf_AuthVerifyCertUniMsg_t verifyCertUniMsg;
        taf_AuthPOWNMsg_t POWNMsg;
        taf_AuthTransCertMsg_t transCertMsg;
    }playload;

    uint8_t resp[MAX_AUTH_RESP_LEN];
    size_t respLen;
}taf_AuthRxMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic Authentication notification message.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_uds_AddrInfo_t addrInfo;        ///< Rx logical address information structure.
    uint64_t roleId;
}taf_AuthNotifyMsg_t;

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic Authentication service RxMsg handler structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagAuth_RxMsgHandlerRef_t handlerRef;  ///< Own reference.
    taf_diagAuth_ServiceRef_t svcRef;           ///< Service reference.
    taf_diagAuth_RxMsgHandlerFunc_t func;       ///< Handler function.
    void* ctxPtr;                               ///< Handler context.
}taf_AuthReqHandler_t;

//-------------------------------------------------------------------------------------------------
/**
 * Diagnostic Authentication service state expiry handler structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagAuth_AuthStateExpHandlerRef_t handlerRef;   ///< Own reference.
    taf_diagAuth_ServiceRef_t svcRef;                   ///< Service reference.
    taf_diagAuth_AuthStateExpHandlerFunc_t func;        ///< Handler function.
    void* ctxPtr;                                       ///< Handler context.
}taf_AuthExpiryHandler_t;

//-------------------------------------------------------------------------------------------------
/**
 * VLAN ID structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t   link;
    uint16_t        vlanId;
}taf_AuthVlanIdNode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag Auth Server Service Class
 */
//--------------------------------------------------------------------------------------------------
namespace telux
{
    namespace tafsvc
    {
        class taf_AuthSvr : public ITafSvc, public taf_UDSInterface
        {
            public:
                taf_AuthSvr() {};
                ~taf_AuthSvr() {};
                void Init();
                static taf_AuthSvr &GetInstance();
                taf_diagAuth_ServiceRef_t GetService();
                static void OnClientDisconnection(le_msg_SessionRef_t sessionRef,
                        void *contextPtr);

                // UDS message handler.
                void UDSMsgHandler(const taf_uds_AddrInfo_t* addrPtr, uint8_t sid, uint8_t* msgPtr,
                        size_t msgLen) override;
                static void RxAuthEventHandler(void* reportPtr);
                static void AuthNotifyEventHandler(void* reportPtr);
                le_result_t RemoveSvc(taf_diagAuth_ServiceRef_t svcRef);
                taf_diagAuth_AuthStateExpHandlerRef_t AddAuthExpHandler(
                        taf_diagAuth_ServiceRef_t svcRef,
                        taf_diagAuth_AuthStateExpHandlerFunc_t handlerPtr,
                        void* contextPtr);
                void RemoveAuthExpHandler(taf_diagAuth_AuthStateExpHandlerRef_t handlerRef);
                taf_diagAuth_RxMsgHandlerRef_t AddRxMsgHandler(
                        taf_diagAuth_ServiceRef_t svcRef,
                        taf_diagAuth_RxMsgHandlerFunc_t handlerPtr,
                        void* contextPtr);
                void RemoveRxMsgHandler(taf_diagAuth_RxMsgHandlerRef_t handlerRef);
                le_result_t SetVlanId(taf_diagAuth_ServiceRef_t svcRef, uint16_t vlanId);
                le_result_t GetVlanIdFromMsg(taf_diagAuth_RxMsgRef_t rxMsgRef, uint16_t* vlanIdPtr);
                le_result_t GetCommConfVal(taf_diagAuth_RxMsgRef_t rxMsgRef, uint8_t* commConfPtr);
                le_result_t GetCertSize(taf_diagAuth_RxMsgRef_t rxMsgRef, uint16_t* sizePtr);
                le_result_t GetCertData(taf_diagAuth_RxMsgRef_t rxMsgRef,
                        uint8_t* certPtr,size_t* certSizePtr);
                le_result_t GetChallengeSize(taf_diagAuth_RxMsgRef_t rxMsgRef, uint16_t* sizePtr);
                le_result_t GetChallengeData(taf_diagAuth_RxMsgRef_t rxMsgRef,
                        uint8_t* challengePtr, size_t* challengeSizePtr);
                le_result_t GetPOWNSize(taf_diagAuth_RxMsgRef_t rxMsgRef, uint16_t* sizePtr);
                le_result_t GetPOWNData(taf_diagAuth_RxMsgRef_t rxMsgRef,
                        uint8_t* proofPtr, size_t* proofSizePtr);
                le_result_t GetPublicKeySize(taf_diagAuth_RxMsgRef_t rxMsgRef, uint16_t* sizePtr);
                le_result_t GetPublicKeyData(taf_diagAuth_RxMsgRef_t rxMsgRef,
                        uint8_t* publicKeyPtr, size_t* publicKeySizePtr);
                le_result_t GetCertEvalId(taf_diagAuth_RxMsgRef_t rxMsgRef, uint16_t* idPtr);
                le_result_t SetRole(taf_diagAuth_RxMsgRef_t rxMsgRef, uint64_t role);
                le_result_t SetChallenge(taf_diagAuth_RxMsgRef_t rxMsgRef,
                        const uint8_t* challengePtr, size_t challengeSize);
                le_result_t SetPublicKey(taf_diagAuth_RxMsgRef_t rxMsgRef,
                        const uint8_t* publicKeyPtr, size_t publicKeySize);
                le_result_t SetSessKeyInfo(taf_diagAuth_RxMsgRef_t rxMsgRef,
                        const uint8_t* sessKeyInfoPtr, size_t sessKeyInfoSize);
                le_result_t SendResp(taf_diagAuth_RxMsgRef_t rxMsgRef,
                        uint8_t errCode, uint8_t retVal);
            private:
                void ClearMsgList(taf_AuthSvc_t* servicePtr);
                void ClearVlanList(taf_AuthSvc_t* servicePtr);
                taf_AuthSvc_t* GetServiceObj(le_msg_SessionRef_t sessionRef);
                taf_AuthSvc_t* GetServiceObj(uint16_t vlanId);
                le_result_t SendNRCResp(uint8_t sid, const taf_uds_AddrInfo_t* addrInfoPtr,
                        uint8_t errCode);
                void AuthSvcMsgHandler(const taf_uds_AddrInfo_t* addrPtr,
                        uint8_t* msgPtr, size_t msgLen);
                void AuthNotifyMsgHandler(const taf_uds_AddrInfo_t* addrPtr,
                        uint8_t* msgPtr, size_t msgLen);

                uint8_t reqAuthSvcId = 0x29;   // Authentication request service ID.
                uint8_t respAuthSvcId = 0x69;  // Authentication response service ID.
                uint8_t authNotifyId = 0xFE;

                // Service and event object
                le_mem_PoolRef_t SvcPool;
                le_ref_MapRef_t SvcRefMap;

                // Rx message resource
                le_mem_PoolRef_t RxMsgPool;
                le_ref_MapRef_t RxMsgRefMap;

                // Rx request handler object
                le_mem_PoolRef_t ReqHandlerPool;
                le_ref_MapRef_t ReqHandlerRefMap;
                le_mem_PoolRef_t vlanPool;

                // Authentication expiry handler object
                le_mem_PoolRef_t ExpHandlerPool;
                le_ref_MapRef_t ExpHandlerRefMap;

                // Event for service.
                le_event_Id_t ReqEvent;
                le_event_HandlerRef_t ReqEventHandlerRef;
                le_event_Id_t AuthNotifyEvent;
                le_event_HandlerRef_t AuthNotifyEventHandlerRef;
        };
    }
}
#endif /* TAF_AUTH_SERVER_HPP */