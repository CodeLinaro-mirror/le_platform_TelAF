/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *  Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 */

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2022, 2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFSMS_HPP
#define TAFSMS_HPP

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <vector>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/SmsManager.hpp>
#include <telux/tel/CellBroadcastManager.hpp>
#include "telux/common/CommonDefines.hpp"
#include "tafSvcIF.hpp"
#include "tafSmsHlos.hpp"
#include "tafSmsPdu.hpp"

using namespace telux::tel;
using namespace telux::common;

#define MIN_SIM_SLOT_COUNT 1
#define MAX_SIM_SLOT_COUNT 2

#define MIN_PHONE_ID     1
#define MAX_PHONE_ID     2
#define DEFAULT_PHONE_ID 1

#define MAX_OF_SMS_MSG_IN_STORAGE   256
#define MAX_OF_SMS_MSG    (MAX_OF_SMS_MSG_IN_STORAGE*2)
#define MAX_OF_LIST    25

#define MAX_SMS_SESSION 5

#define TIMEOUT_GET_SMSC           2
#define TIMEOUT_SET_SMSC           2
#define TIMEOUT_ACTIVATE_CB        2
#define TIMEOUT_PREF_STORAGE       2
#define TIMEOUT_RQUEST_CB_FILTER   2
#define TIMEOUT_UPDATE_CB_FILTER   2

#define CFG_MODEMSERVICE_SMS_PATH "tafSMSSvc:/sms"
#define CFG_NODE_PREFERRED_STORAGE "prefStorage"

#define LENGTH_CFG_NODE 50

constexpr uint8_t kSetTagWaitTime = 5;
constexpr uint8_t kListRxMsgWaitTime = 5;
constexpr uint8_t kSendMessageWaitTime = 30;
constexpr uint8_t kDeleteMessageWaitTime = 5;
constexpr uint8_t kReadFromStorageWaitTime = 5;
constexpr uint8_t kPreferredStorageWaitTime = 5;

//--------------------------------------------------------------------------------------------------
/**
 * SMS message structure
 */
//--------------------------------------------------------------------------------------------------

typedef struct taf_sms_Msg
{
   char                 tel[TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES];
   taf_sms_Pdu_t        pdu;
   bool                 pduReady;

   union
   {
      char              text[TAF_SMS_TEXT_BYTES];
      uint8_t           binary[TAF_SMS_BINARY_BYTES];
   };
   size_t               userdataLen;

   char                 timestamp[TAF_SMS_TIMESTAMP_BYTES];
   int8_t               phoneId;

   taf_sms_Type_t       type;
   taf_sms_Format_t     format;
   taf_sms_SendStatus_t sendStatus;
   taf_sms_ReadStatus_t readStatus;
   taf_sms_LockStatus_t lockStatus;
   le_msg_SessionRef_t  sessionRef;

   taf_sms_Storage_t    storage;
   uint8_t              storageIdx;
   bool                 applyDel;
   bool                 isEncrypted;

   bool                 inList;
   uint8_t              userCount;
   void*                callBackPtr;
   void*                ctxPtr;
}
taf_sms_Msg_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure to references message node of the list which is created from client side
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_sms_MsgNode
{
    taf_sms_MsgRef_t     msgRef;
    le_dls_Link_t        listLink;
}
taf_sms_MsgNode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Message list created with 'CreateRxMsgList' function
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_sms_MsgList
{
    taf_sms_MsgListRef_t msgListRef;
    le_msg_SessionRef_t  sessionRef;
    le_dls_List_t        list;
    le_dls_Link_t*       tmpLink;
}
taf_sms_List_t;

//--------------------------------------------------------------------------------------------------
/**
 * msgRef node structure used for the msgRefList list
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
   taf_sms_MsgRef_t  msgRef;
   le_dls_Link_t     link;
}
MsgNode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Session node used for the SessionList list
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
   le_msg_SessionRef_t sessionRef;
   le_dls_Link_t       link;
   le_dls_List_t       msgRefList;
   le_dls_List_t       handlerList;
}
SessionNode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Handler node structure used for the handlerList list
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
   taf_sms_RxMsgHandlerRef_t    handlerRef;
   le_dls_Link_t                link;
   taf_sms_RxMsgHandlerFunc_t   handlerFuncPtr;
   SessionNode_t*               sessionCtxPtr;
   void*                        userContext;
}
HandlerNode_t;


namespace tafsvc {

   class tafSmsListener : public telux::tel::ISmsListener {
   public:
      void onMemoryFull(int phoneId, telux::tel::StorageType type) override;
      void onIncomingSms(int phoneId, std::shared_ptr<telux::tel::SmsMessage> message) override;
   };

   class tafSmsCallback : public ICommandResponseCallback {
   public:
      void commandResponse(ErrorCode error) override;
      static void sendSmsResponse(std::vector<int> msgRefs,
         telux::common::ErrorCode errorCode);
      taf_sms_MsgRef_t msgRef;
   };

   class tafSmscAddressCallback : public telux::tel::ISmscAddressCallback {
   public:
      void smscAddressResponse(const std::string &address, telux::common::ErrorCode error) override;
   };

   class tafSetSmscAddressResponseCallback {
   public:
      static void setSmscResponse(telux::common::ErrorCode error);
   };

   class tafSmsDeliveryCallback : public telux::common::ICommandResponseCallback {
   public:
      void commandResponse(telux::common::ErrorCode error) override;
   };

   class tafSetSmsCBResponseCallback {
   public:
      static void setSmsCBResponse(telux::common::ErrorCode error);
      static void requestFilterResponse(std::vector<telux::tel::CellBroadcastFilter> filters,
                                        telux::common::ErrorCode errorCode);
      static void updateFilterResponse(telux::common::ErrorCode error);
   };

   class tafSetSmsStorageCallback {
   public:
      static void getPreferredStorageResponse(telux::tel::StorageType type,
         telux::common::ErrorCode errorCode);
      static void setPreferredStorageResponse(telux::common::ErrorCode errorCode);
      static void setTagResponse(telux::common::ErrorCode errorCode);
      static void deleteResponse(telux::common::ErrorCode errorCode);
      static void readMsgResponse(telux::tel::SmsMessage smsMsg,
         telux::common::ErrorCode errorCode);
      static void reqMessageListResponse(std::vector<telux::tel::SmsMetaInfo> infos,
         telux::common::ErrorCode errorCode);
   };

   typedef struct
   {
      char     timestamp[TAF_SMS_TIMESTAMP_BYTES];
      char     pdu[(TAF_SMS_PDU_BYTES * 2) + 1];
      uint8_t  phoneId;
      uint32_t storageIdx;
   } newSms_t;

   typedef struct
   {
      telux::common::ErrorCode errcode;
   } tafSmsErrorCode_t;

   class taf_Sms : public ITafSvc {
   public:
      void Init(void);
      static taf_Sms &GetInstance();

      taf_Sms() {};
      ~taf_Sms() {};

      SessionNode_t* CreateSessionCtx(void);
      SessionNode_t* GetSessionNode(le_msg_SessionRef_t sessionRef);
      SessionNode_t* GetSessionNodeFromMsgRef(taf_sms_MsgRef_t msgRef);
      taf_sms_MsgRef_t SetMsgRefForSessionCtx(taf_sms_Msg_t* msgPtr,
         SessionNode_t* sessionCtxPtr);
      taf_sms_RxMsgHandlerRef_t CreateRxHandlerCtx(SessionNode_t* sessionCtxPtr,
         taf_sms_RxMsgHandlerFunc_t handlerFuncPtr, void* contextPtr);
      taf_sms_MsgListRef_t CreateNewMsgList(void);
      taf_sms_MsgRef_t GetFirstMessage(taf_sms_MsgListRef_t msgListRef);
      void RemoveMsgRefFromSessionCtx(SessionNode_t* sessionCtxPtr, taf_sms_MsgRef_t msgRef);
      void RemoveRxHandlerCtx(taf_sms_RxMsgHandlerRef_t handlerRef);
      void NewSmsHandler(taf_sms_Msg_t *newMsg);
      void MessageHandlers(taf_sms_Msg_t* msgPtr);
      void ReleaseSession(le_msg_SessionRef_t sessionRef, void* ctxPtr);

      taf_sms_Msg_t* CreateRxMsgNode(taf_sms_Pdu_t *pduMsg, char* phoneNum,
         taf_sms_Format_t format, char* data, int16_t dataLen);
      taf_sms_Msg_t* CreateRxMsgNode(taf_sms_Pdu_t *pduMsg);
      taf_sms_Msg_t* CreateAndConstructMsg(taf_sms_Pdu_t* pduMsgPtr, sms_PduMsg_t* decodedMsgPtr);
      le_result_t constructSmsDeliver(taf_sms_Msg_t* msgObjPtr, sms_PduMsg_t* decodedMsgPtr);
      uint32_t GetMsgFromStorage(taf_sms_List_t *msgListPtr, taf_sms_Storage_t storage,
         uint32_t numOfMsg, uint32_t *arrayPtr, uint8_t phoneId);
      uint32_t ListRxMsg(taf_sms_List_t *msgListPtr,taf_sms_ReadStatus_t rxStatus,
         taf_sms_Storage_t storage, uint8_t phoneId);
      uint32_t ListAllRxMsg(taf_sms_List_t *msgListPtr);
      le_result_t GetPreferredStorage(taf_sms_Storage_t* storage);
      le_result_t SetPreferredStorage(taf_sms_Storage_t storage);
      le_result_t SetConfig_PreferredStorage(const taf_sms_Storage_t storage);
      taf_sms_Storage_t GetConfig_PreferredStorage();
      le_result_t ActivateCellBroadcast(uint8_t phoneId, bool activate);
      le_result_t RequestBroadcastIds(uint8_t phoneId);
      le_result_t AddCellBroadcastIds(uint8_t phoneId, uint16_t fromId, uint16_t toId);
      le_result_t RemoveCellBroadcastIds(uint8_t phoneId, uint16_t fromId, uint16_t toId);

      le_result_t SendPDUMessage(uint8_t *pduData, uint32_t pduLength, uint32_t timeout,
         uint8_t phoneId);
      le_result_t SendMessage(taf_sms_Msg_t* msgPtr);
      le_result_t ReadFromStorage(taf_sms_Pdu_t* pduMsg,
         uint32_t idx, taf_sms_Storage_t storage);
      le_result_t SetTag(taf_sms_Msg_t* msgPtr, telux::tel::SmsTagType tagType);
      le_result_t DeleteMessage(uint32_t messageIndex);
      le_result_t DeleteAllMessages(taf_sms_Storage_t storage);

      le_ref_MapRef_t MsgRefMap = NULL;
      le_ref_MapRef_t ListRefMap = NULL;
      le_ref_MapRef_t HandlerRefMap = NULL;

      le_mem_PoolRef_t   MsgRefPool = NULL;        // Memory Pool for msgRef context (for client)
      le_mem_PoolRef_t   MsgPool = NULL;           // Memory Pool for stored SMS messages
      le_mem_PoolRef_t   MsgListPool = NULL;          // Memory Pool for Listed SMS messages
      le_mem_PoolRef_t   MsgRefNodePool = NULL;    // Memory Pool for message references
      le_mem_PoolRef_t   HandlerNodePool = NULL;   // Memory Pool for sessions context
      le_mem_PoolRef_t   SessionNodePool = NULL;   // Memory Pool for sessions context

      le_dls_List_t  SessionList;

      le_event_Id_t NewMsgEvent;
      le_event_Id_t MsgSendEvent;
      le_event_Id_t MsgSendCallbackEvent;
      le_event_Id_t StorageEvent;

      le_sem_Ref_t SmsSendSem = nullptr;
      le_sem_Ref_t SmscGetSem = nullptr;
      le_sem_Ref_t SmscSetSem = nullptr;

      taf_sms_MsgRef_t sendingMsgRef = nullptr;

      taf_sms_Storage_t sysPrefStorage = TAF_SMS_STORAGE_NONE;

      uint8_t NumOfSlot = MIN_SIM_SLOT_COUNT;

      // objects used by telSdk interfaces
      std::shared_ptr<tafSmsCallback> smsSentCb;
      std::shared_ptr<tafSmsDeliveryCallback> smsDeliveryCb;
      std::shared_ptr<tafSmsListener> mySmsListener;
      std::shared_ptr<tafSmscAddressCallback> getSmscCb;

      std::vector<std::shared_ptr<telux::tel::ISmsManager>> smsManagers;
      std::vector<std::shared_ptr<telux::tel::ICellBroadcastManager>> CbManagers;

      //for SMS center address
      char smscAddr[TAF_SMS_SMSC_ADDR_BYTES];
      std::promise<le_result_t> CmdSynchronousPromise;

      // for cell broadcast
      std::vector<telux::tel::CellBroadcastFilter> CBFilterList;
      std::promise<le_result_t> CBActivateSyncPromise;
      std::promise<le_result_t> CBRequestIdsSyncPromise;
      std::promise<le_result_t> CBAddIdsSyncPromise;
      std::promise<le_result_t> CBRemoveIdsSyncPromise;

      std::promise<telux::common::ErrorCode> SendMessageSyncPromise;

      std::promise<le_result_t> PreferredStorageSyncPromise;

      std::promise<le_result_t> SmsCenterSyncPromise;

      std::promise<le_result_t> SetTagSyncPromise;

      std::promise<le_result_t> DeleteMessageSyncPromise;

      std::promise<telux::tel::SmsMessage> ReadMessageSyncPromise;

      std::promise<std::vector<telux::tel::SmsMetaInfo>> MessageListSyncPromise;
   };


   // the interface between client and service
   class taf_Handler: public ITafSvc {
      public:
         void Init(void);

         taf_Handler();
         ~taf_Handler();

         // handler for the call request
         static void ProcessNewMessage(void* incomingMsgPtr);

         // handler for the call request
         static void ProcessSendMessage(void* context);

         // handler for the sending status
         static void ProcessSendingStateEvent(void* context);

         static void CloseSessionEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);

         // tafVoiceCall obj needs to be realized first
         static taf_Sms *TafSmsPtr;
   };

}

#endif
