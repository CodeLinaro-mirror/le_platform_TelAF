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
 */

#ifndef MYSMSLISTENER_HPP
#define MYSMSLISTENER_HPP

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <vector>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/SmsManager.hpp>
#include "telux/common/CommonDefines.hpp"
#include "tafSvcIF.hpp"

using namespace telux::tel;
using namespace telux::common;

#define MAX_OF_SMS_MSG_IN_STORAGE   256
#define MAX_OF_SMS_MSG    (MAX_OF_SMS_MSG_IN_STORAGE*4)
#define MAX_OF_LIST    128

#define MAX_SMS_SESSION 5

#define TIMEOUT_SEND_SEMAPHORE     2
#define TIMEOUT_GET_SMSC_SEMAPHORE 2
#define TIMEOUT_SET_SMSC_SEMAPHORE 2

//--------------------------------------------------------------------------------------------------
/**
 * SMS message structure
 */
//--------------------------------------------------------------------------------------------------

typedef struct taf_sms_Msg
{
   char                 tel[TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES];
   char                 text[TAF_SMS_TEXT_BYTES];
   char                 timestamp[TAF_SMS_TIMESTAMP_BYTES];
   int8_t               phoneId;

   taf_sms_Type_t       type;
   taf_sms_SendStatus_t sendStatus;
   taf_sms_ReadStatus_t readStatus;
   le_msg_SessionRef_t  sessionRef;

   uint32_t             storageId;
   bool                 inList;
   size_t               userdataLen;
   int32_t              smsUserCount;
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


namespace telux {
namespace tafsvc {

   class tafSmsListener : public telux::tel::ISmsListener {
   public:
      void onIncomingSms(int phoneId, std::shared_ptr<telux::tel::SmsMessage> message) override;
   };

   class tafSmsCallback : public ICommandResponseCallback {
   public:
      void commandResponse(ErrorCode error) override;
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

   typedef struct
   {
      char                 tel[TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES];
      char                 text[TAF_SMS_TEXT_BYTES];
      char                 timestamp[TAF_SMS_TIMESTAMP_BYTES];
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
      taf_sms_MsgRef_t SetMsgRefForSessionCtx(taf_sms_Msg_t* msgPtr, SessionNode_t* sessionCtxPtr);
      taf_sms_RxMsgHandlerRef_t CreateRxHandlerCtx(SessionNode_t* sessionCtxPtr, taf_sms_RxMsgHandlerFunc_t handlerFuncPtr, void* contextPtr);
      taf_sms_MsgListRef_t CreateNewMsgList(void);
      void RemoveMsgRefFromSessionCtx(SessionNode_t* sessionCtxPtr, taf_sms_MsgRef_t msgRef);
      void RemoveRxHandlerCtx(taf_sms_RxMsgHandlerRef_t handlerRef);
      void NewSmsHandler(taf_sms_Msg_t *newMsg);
      void MessageHandlers(taf_sms_Msg_t* msgPtr);
      void ReleaseSession(le_msg_SessionRef_t sessionRef, void* ctxPtr);

      le_result_t sendMessage(void);

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

      le_sem_Ref_t SmsSendSem = nullptr;
      le_sem_Ref_t SmscGetSem = nullptr;
      le_sem_Ref_t SmscSetSem = nullptr;

      taf_sms_MsgRef_t sendingMsgRef;

      // objects used by telSdk interfaces
      std::shared_ptr<telux::tel::IPhoneManager> phoneManager;
      std::shared_ptr<telux::tel::ISmsManager> smsMgr;
      std::shared_ptr<tafSmsCallback> smsSentCb;
      std::shared_ptr<tafSmsDeliveryCallback> smsDeliveryCb;
      std::shared_ptr<tafSmsListener> mySmsListener;
      std::shared_ptr<tafSmscAddressCallback> getSmscCb;

      std::vector<std::shared_ptr<telux::tel::ISmsManager>> smsManagers;

      char smscAddr[TAF_SMS_SMSC_ADDR_BYTES];
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
}

#endif
