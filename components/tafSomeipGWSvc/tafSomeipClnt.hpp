/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFSOMEIPCLNT_HPP
#define TAFSOMEIPCLNT_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#include <chrono>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <vsomeip/vsomeip.hpp>

#define CLNT_SERVICE_REF_CNT 16
#define CLNT_TXMSG_REF_CNT 16
#define CLNT_STATE_HANDLER_REF_CNT 16
#define CLNT_EVENT_HANDLER_REF_CNT 16
#define CLNT_MAX_TXNID_CNT 512

//--------------------------------------------------------------------------------------------------
/**
 * TelAF VSOMEIP Generic event messages.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    VS_MSG_STATE,                                   ///< VSOMEIP State message.
    VS_MSG_RESPONSE,                                ///< VSOMEIP Response message.
    VS_MSG_EVENT,                                   ///< VSOMEIP Event message.
}VsClntMsgType_t;

typedef struct
{
    uint16_t serviceId;
    uint16_t instanceId;
    uint8_t majVer;
    uint32_t minVer;
    bool isAvailable;
}VsState_t;

typedef struct
{
    VsClntMsgType_t type;
    union
    {
        void* msgPtr;
        VsState_t state;
    };
}VsClntMsg_t;

//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP client-service-instance state.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    SERVICE_STATE_UNKNOWN,                          ///< Service state is unknown.
    SERVICE_STATE_UNAVAILABLE,                      ///< Service state is unavailable.
    SERVICE_STATE_AVAILABLE,                        ///< Service state is available.
}ServiceState_t;

//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP common header in a request/response message.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t serviceId;                             ///< Service Identifier.
    uint16_t instanceId;                            ///< Instance Identifier.
    uint16_t methodId;                              ///< Method Identifier.
    uint16_t sessionId;                             ///< Session Identifier.
    uint16_t clientId;                              ///< Client Identifier.
    uint8_t  interfaceVer;                          ///< Interface version of the service.
}CommonMsgHdr_t;

//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP payload data in a request/response message.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    size_t size;                                    ///< payload data size.
    uint8_t data[TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE];   ///< payload data buffer.
}Payload_t;

//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP response message struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    CommonMsgHdr_t msgHdr;                          ///< Common message header.
    bool isErrResp;                                 ///< If an error response.
    uint8_t returnCode;                             ///< Return code.
    Payload_t* payloadPtr;                          ///< Payload data.
}SomeipClnt_RespMsg_t;

//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP event message struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    CommonMsgHdr_t msgHdr;                          ///< Common message header.
    Payload_t* payloadPtr;                          ///< Payload data.
}SomeipClnt_EventMsg_t;

//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP request message struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;                             ///< Link to the list.
    taf_someipClnt_TxMsgRef_t ref;                  ///< Self reference.
    taf_someipClnt_ServiceRef_t serviceRef;         ///< Server reference.
    uint16_t methodId;                              ///< Method ID.
    bool isNonRet;                                  ///< If it's non-return request.
    bool isReliable;                                ///< Send message via TCP or UDP.
    uint32_t timeoutMs;                             ///< Timeout milliseconds for response.
    Payload_t* payloadPtr;                          ///< Payload data.
}SomeipClnt_TxMsg_t;

//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP Transaction ID struct. Used to track a request-response pair.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;                             ///< Link to the list.
    CommonMsgHdr_t msgHdr;                          ///< Common message header.
    taf_someipClnt_ServiceRef_t serviceRef;         ///< Service reference.
    le_timer_Ref_t timerRef;                        ///< Response waiting timer.
    taf_someipClnt_RespMsgHandlerFunc_t respHandleFunc; ///< Response handler function.
    void* contextPtr;                               ///< Function context.
}Txn_t;

//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP service state change handler struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;                             ///< Link to the list.
    taf_someipClnt_StateChangeHandlerRef_t ref;     ///< Self reference.
    taf_someipClnt_ServiceRef_t serviceRef;         ///< Service reference.
    taf_someipClnt_StateChangeHandlerFunc_t handlefunc; ///< State change handler function.
    void* context;                                  ///< Function context.
}StateHandler_t;

//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP client-service-instace struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;                             ///< Link to the list.
    uint16_t serviceId;                             ///< Service Identifier.
    uint16_t instanceId;                            ///< Instance Identifier.
    ServiceState_t state;                           ///< Service state.
    uint8_t majorVersion;                           ///< Major Version retrived from VSOMEIP stack.
    uint32_t minorVersion;                          ///< Minor version retrived from VSOMEIP stack.
    le_dls_List_t groupList;                        ///< Enabled Event group list.
    le_dls_List_t eventList;                        ///< Enabled Event list
}SomeipClnt_Service_t;

//--------------------------------------------------------------------------------------------------
/**
 * TelAF client-service-session struct. Presents a client which requesting a SOME/IP service.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_someipClnt_ServiceRef_t ref;                ///< Self reference.
    le_msg_SessionRef_t clientRef;                  ///< Client session reference.
    SomeipClnt_Service_t* servicePtr;               ///< Service object pointer.
    ServiceState_t state;                           ///< Service state.
    le_dls_List_t eventHandlerList;                 ///< Event group handler.
    le_dls_List_t stateHandlerList;                 ///< State handler list.
    le_dls_List_t txMsgList;                        ///< Tx message list(unsent messages).
    le_dls_List_t txnIdList;                        ///< Txn IDs list.
}SvcClient_t;

//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP event message handler struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;                             ///< Link to the list.
    taf_someipClnt_EventMsgHandlerRef_t ref;        ///< Self reference.
    taf_someipClnt_ServiceRef_t serviceRef;         ///< Service reference.
    uint16_t groupId;                               ///< Event group Identifier.
    taf_someipClnt_EventMsgHandlerFunc_t handlefunc;///< State change handler function.
    void* context;                                  ///< Function context.
}EventHandler_t;

//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP event group structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;                             ///< Link to the group list of the service.
    uint16_t groupId;                               ///< Event Group Identifier.
    bool setForSubscription;                        ///< If subscription is required.
    bool isSubscribed;                              ///< If subscription is done in VSOMEIP layer.
    le_msg_SessionRef_t clientRef;                  ///< Client session reference.
    SomeipClnt_Service_t* servicePtr;               ///< Service object pointer.
}SomeipClnt_Group_t;

//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP event structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;                             ///< Link to event list of the service.
    uint16_t eventId;                               ///< Event Identifier.
    uint16_t groupId;                               ///< Group Identifier the event is added to.
    taf_someipDef_EventType_t eventType;            ///< Event Type.
    le_msg_SessionRef_t clientRef;                  ///< Client session reference.
    SomeipClnt_Service_t* servicePtr;               ///< Service object pointer.
}SomeipClnt_Event_t;

//--------------------------------------------------------------------------------------------------
/**
 * SOME/IP Client Service Class
 */
//--------------------------------------------------------------------------------------------------
namespace telux
{
    namespace tafsvc
    {
        class taf_SomeipClient: public ITafSvc
        {
            public:
                taf_SomeipClient() {};
                ~taf_SomeipClient() {};
                void Init();

                // Static member functions.
                static taf_SomeipClient &GetInstance();
                static void TafClientDisconnection(le_msg_SessionRef_t sessionRef, void *contextPtr);
                static void TafVsMsgHandler(void* reportPtr);
                static void TafServiceDestructor(void* objPtr);
                static void TafGroupDestructor(void* objPtr);
                static void TafTxnTimerExpiryHandler(le_timer_Ref_t timerRef);

                // Static hash functions.
                static size_t HashComputeTxnId(const void* keyPtr);
                static bool HashCompareTxnIds(const void* firstKeyPtr, const void* secondKeyPtr);

                // Public methods used in VSOMEIP handlers or context.
                void VSOMEIPInit(const std::shared_ptr<vsomeip::application>& app);
                void VSOMEIPRespHandler(const std::shared_ptr<vsomeip::message>& msg);
                void VSOMEIPEventHandler(const std::shared_ptr<vsomeip::message>& msg);
                void VSOMEIPStateHandler(uint16_t serviceId, uint16_t instanceId, bool isAvailable);

                // Public methods used in generic service layer.
                void ClientDisconnection(le_msg_SessionRef_t sessionRef, void *contextPtr);
                void ServiceDestructor(void* objPtr);
                void GroupDestructor(void* objPtr);
                void ProcessStateChangeMessage(VsState_t serviceState);
                void ProcessResponseMessage(void* msgPtr);
                void ProcessEventMessage(void* msgPtr);
                void TxnTimerExpiryHandler(le_timer_Ref_t timerRef);

                // Pubilc methods for API handler wrapper.
                uint16_t GetClientId(void);
                taf_someipClnt_ServiceRef_t RequestService(uint16_t serviceId, uint16_t instanceId);
                le_result_t ReleaseService(taf_someipClnt_ServiceRef_t serviceRef);
                le_result_t GetState(taf_someipClnt_ServiceRef_t serviceRef,
                                        taf_someipClnt_State_t* statePtr);
                le_result_t GetVersion(taf_someipClnt_ServiceRef_t serviceRef,
                                          uint8_t* majVerPtr,
                                          uint32_t* minVerPtr);
                taf_someipClnt_StateChangeHandlerRef_t AddStateChangeHandler(
                    taf_someipClnt_ServiceRef_t serviceRef,
                    taf_someipClnt_StateChangeHandlerFunc_t handlerPtr,
                    void* contextPtr);
                void RemoveStateChangeHandler(taf_someipClnt_StateChangeHandlerRef_t handlerRef);
                taf_someipClnt_TxMsgRef_t CreateMsg(taf_someipClnt_ServiceRef_t serviceRef,
                                                       uint16_t methodId);
                le_result_t SetNonRet(taf_someipClnt_TxMsgRef_t msgRef);

                le_result_t SetReliable(taf_someipClnt_TxMsgRef_t msgRef);
                le_result_t SetTimeout(taf_someipClnt_TxMsgRef_t msgRef, uint32_t timeOut);
                le_result_t SetPayload(taf_someipClnt_TxMsgRef_t msgRef, const uint8_t* dataPtr,
                                          size_t dataSize);
                le_result_t DeleteMsg(taf_someipClnt_TxMsgRef_t msgRef);
                void RequestResponse(taf_someipClnt_TxMsgRef_t msgRef,
                                          taf_someipClnt_RespMsgHandlerFunc_t handlerPtr,
                                          void* contextPtr);
                le_result_t EnableEventGroup(taf_someipClnt_ServiceRef_t serviceRef,
                                                  uint16_t groupId,
                                                  uint16_t eventId,
                                                  taf_someipDef_EventType_t eventType);
                le_result_t DisableEventGroup(taf_someipClnt_ServiceRef_t serviceRef,
                                                    uint16_t groupId);
                le_result_t SubscribeEventGroup(taf_someipClnt_ServiceRef_t serviceRef,
                                                      uint16_t groupId);
                le_result_t UnsubscribeEventGroup(taf_someipClnt_ServiceRef_t serviceRef,
                                                         uint16_t groupId);
                taf_someipClnt_EventMsgHandlerRef_t AddEventMsgHandler(
                    taf_someipClnt_ServiceRef_t serviceRef,
                    uint16_t groupId,
                    taf_someipClnt_EventMsgHandlerFunc_t handlerPtr,
                    void* contextPtr);
                void RemoveEventMsgHandler(taf_someipClnt_EventMsgHandlerRef_t handlerRef);

                // Public varibles.
                le_sem_Ref_t InitSem;
            private:
                // Internal functions.
                SomeipClnt_Service_t* FindService(uint16_t serviceId, uint16_t instanceId);
                SvcClient_t* FindSvcClient(SomeipClnt_Service_t* serverPtr,
                                               le_msg_SessionRef_t clientSessionRef);
                bool IsSvcClientReleasable(SvcClient_t* svcClientPtr);
                void CleanupClientResources(SvcClient_t* svcClientPtr);
                void NotifyServiceStateChange(SvcClient_t* svcClientPtr,
                                                      taf_someipClnt_State_t state);
                void NotifyEventMessage(SvcClient_t* svcClientPtr,
                                              uint16_t groupId,
                                              uint16_t eventId,
                                              Payload_t* payloadPtr);
                SomeipClnt_Group_t* FindGroup(le_dls_List_t* groupListPtr, uint16_t groupId);
                SomeipClnt_Event_t* FindEvent(le_dls_List_t* eventListPtr, uint16_t eventId);
                void ClearEventsInGroup(le_dls_List_t* eventListPtr, uint16_t groupId,
                                        le_msg_SessionRef_t clientSessionRef);
                void ClearGroups(le_dls_List_t* groupListPtr, le_msg_SessionRef_t clientSessionRef);

                // Internal VSOMEIP interface functions.
                void VSOMEIPRequestService(SomeipClnt_Service_t* servicePtr);
                void VSOMEIPReleaseService(SomeipClnt_Service_t* servicePtr);
                void VSOMEIPSendRequest(CommonMsgHdr_t* msgHdrPtr,bool isNonRetMsg,
                                              bool isReliable, Payload_t* payloadPtr);
                bool VSOMEIPGetServiceInfo(uint16_t serviceId, uint16_t instanceId,
                                            uint8_t* majVerPtr, uint32_t* minVerPtr);
                void VSOMEIPSubscribeEventGroup(SomeipClnt_Group_t* groupPtr);
                void VSOMEIPUnsubscribeEventGroup(SomeipClnt_Group_t* groupPtr);

                // VSOME/IP client ID.
                uint16_t VsClientId;

                // Global Txn hash map and Txn object.
                le_hashmap_Ref_t TxnHashMapRef;
                le_mem_PoolRef_t TxnPool;

                // Global service list and service object.
                le_dls_List_t ServiceList;
                le_mem_PoolRef_t ServicePool;

                // Client session object that requests a service.
                le_ref_MapRef_t SvcClientRefMap;
                le_mem_PoolRef_t SvcClientPool;

                // Tx message object.
                le_ref_MapRef_t TxMsgRefMap;
                le_mem_PoolRef_t TxMsgPool;

                // State handler object.
                le_ref_MapRef_t StateHandlerRefMap;
                le_mem_PoolRef_t StateHandlerPool;

                // Payload in Tx/Resp message.
                le_mem_PoolRef_t PayloadPool;

                // Response message object.
                le_mem_PoolRef_t RespMsgPool;

                // Event message object.
                le_mem_PoolRef_t EventMsgPool;

                // Event and Group object.
                le_mem_PoolRef_t GroupPool;
                le_mem_PoolRef_t EventPool;

                // Event handler object.
                le_ref_MapRef_t EventHandlerRefMap;
                le_mem_PoolRef_t EventHandlerPool;

                // VSOMEIP interface.
                le_event_Id_t VsMsgEvent;
                le_event_HandlerRef_t VsMsgEventHandlerRef;

                std::shared_ptr<vsomeip::application> VsomeipApp;
        };
    }
}
#endif /* #ifndef TAFCAN_HPP */
