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

#include "legato.h"
#include "interfaces.h"
#include "telux/tel/PhoneFactory.hpp"
#include "tafSms.hpp"
#include <unistd.h>

using namespace telux::tel;
using namespace telux::common;
using namespace telux::tafsvc;
using namespace std;

LE_MEM_DEFINE_STATIC_POOL(SmsMsg, MAX_OF_SMS_MSG, sizeof(taf_sms_Msg_t));
LE_MEM_DEFINE_STATIC_POOL(ListSms, MAX_OF_LIST, sizeof(taf_sms_List_t));
LE_MEM_DEFINE_STATIC_POOL(SmsReference, MAX_OF_SMS_MSG, sizeof(taf_sms_MsgNode_t));
LE_MEM_DEFINE_STATIC_POOL(Handler, MAX_SMS_SESSION, sizeof(HandlerNode_t));
LE_MEM_DEFINE_STATIC_POOL(SessionCtx, MAX_SMS_SESSION, sizeof(SessionNode_t));
LE_MEM_DEFINE_STATIC_POOL(MsgRef, MAX_SMS_SESSION*MAX_OF_SMS_MSG, sizeof(MsgNode_t));

taf_Sms* taf_Handler::TafSmsPtr = NULL;

//-----------------------------------------------------------------------------
// Class Handler Implementations
//
taf_Handler::taf_Handler()
{

}

taf_Handler::~taf_Handler()
{

}

void taf_Handler::Init()
{

}

void taf_Handler::ProcessNewMessage(void* incomingMsgPtr)
{
   LE_INFO("ProcessNewMessage\n");
   auto &sms = taf_Sms::GetInstance();

   newSms_t *newMsgPtr = (newSms_t*) incomingMsgPtr;
   taf_sms_Msg_t *tafNewMsg = (taf_sms_Msg_t*)le_mem_ForceAlloc(sms.MsgPool);

   le_utf8_Copy(tafNewMsg->tel, newMsgPtr->tel, DESTINATION_LEN, NULL);
   le_utf8_Copy(tafNewMsg->text, newMsgPtr->text, TAF_SMS_TEXT_BYTES, NULL);

   size_t length = strnlen(tafNewMsg->text, TAF_SMS_TEXT_BYTES);

   if(length > (TAF_SMS_TEXT_BYTES-1))
   {
      length = TAF_SMS_TEXT_BYTES - 1;
   }

   tafNewMsg->userdataLen = length;
   tafNewMsg->readStatus = TAF_SMS_UNREAD;
   tafNewMsg->type = TAF_SMS_RX;

   sms.NewSmsHandler(tafNewMsg);
}

void taf_Handler::ProcessSendMessage(void* context)
{
   auto &sms = taf_Sms::GetInstance();
   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(sms.MsgRefMap, sms.sendingMsgRef);

   auto smsManager = sms.smsManagers[msgPtr->phoneId - 1];
   smsManager->sendSms(std::string(msgPtr->text), std::string(msgPtr->tel), sms.smsSentCb, sms.smsDeliveryCb);
}

void taf_Handler::ProcessSendingStateEvent(void* context)
{
   auto &sms = taf_Sms::GetInstance();

   taf_sms_MsgRef_t *sendMsgRef = (taf_sms_MsgRef_t*) context;
   TAF_ERROR_IF_RET_NIL(sendMsgRef == nullptr, "sendMsgRef is nullptr!");

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(sms.MsgRefMap, *sendMsgRef);
   TAF_ERROR_IF_RET_NIL(msgPtr == nullptr, "msgPtr is nullptr!");

   taf_sms_CallbackResultFunc_t functionPtr = (taf_sms_CallbackResultFunc_t)(msgPtr->callBackPtr);

   if (functionPtr)
   {
      LE_DEBUG("Sending CallBack (%p), Status %d", functionPtr, msgPtr->sendStatus);

      functionPtr(*sendMsgRef, msgPtr->sendStatus, msgPtr->ctxPtr);
   }
   else
   {
      LE_WARN("No CallBackFunction Found for message, status %d!!", msgPtr->sendStatus);
   }
}

void taf_Handler::CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] Session reference of client application.
    void*               contextPtr   ///< [IN] Context pointer of CloseSessionEventHandler.
)
{
   TAF_ERROR_IF_RET_NIL(!sessionRef, "sessionRef is NULL");

   auto &sms = taf_Sms::GetInstance();
   sms.ReleaseSession(sessionRef, contextPtr);
}

//-----------------------------------------------------------------------------
// Class taf_Sms implementation
//

SessionNode_t* taf_Sms::CreateSessionCtx(void)
{
   SessionNode_t* sessionNodePtr = (SessionNode_t*)le_mem_ForceAlloc(SessionNodePool);
   TAF_ERROR_IF_RET_VAL(sessionNodePtr == NULL, NULL, "Cannot allocate sessionCtx");

   sessionNodePtr->sessionRef = taf_sms_GetClientSessionRef();
   sessionNodePtr->msgRefList = LE_DLS_LIST_INIT;
   sessionNodePtr->handlerList = LE_DLS_LIST_INIT;
   sessionNodePtr->link = LE_DLS_LINK_INIT;

   le_dls_Queue(&SessionList, &(sessionNodePtr->link));

   LE_DEBUG("SessionRef %p creates context at %p", sessionNodePtr->sessionRef, sessionNodePtr);

   return sessionNodePtr;
}


SessionNode_t* taf_Sms::GetSessionNode
(
    le_msg_SessionRef_t sessionRef
)
{
   TAF_ERROR_IF_RET_VAL(sessionRef == NULL, NULL, "Invalid sessionRef provided");

   SessionNode_t* sessionNodePtr = NULL;
   le_dls_Link_t* linkPtr = NULL;

   linkPtr = le_dls_Peek(&SessionList);

   while (linkPtr != NULL)
   {
      SessionNode_t* sessionTmpPtr = CONTAINER_OF(linkPtr, SessionNode_t, link);
      linkPtr = le_dls_PeekNext(&SessionList, linkPtr);

      if (sessionTmpPtr->sessionRef == sessionRef)
      {
         sessionNodePtr = sessionTmpPtr;

         LE_DEBUG("sessionCtx %p matched for the sessionRef %p", sessionNodePtr, sessionRef);
         return sessionNodePtr;
      }
   }

   return NULL;
}

SessionNode_t* taf_Sms::GetSessionNodeFromMsgRef
(
    taf_sms_MsgRef_t msgRef
)
{
   TAF_ERROR_IF_RET_VAL(msgRef == NULL, NULL, "Invalid msgRef provided");

   le_dls_Link_t* linkPtr = le_dls_Peek(&SessionList);

   while (linkPtr != NULL)
   {
      SessionNode_t* sessionNodePtr = NULL;
      le_dls_Link_t* SessionCtxPtr = NULL;

      sessionNodePtr = CONTAINER_OF(linkPtr, SessionNode_t, link);
      linkPtr = le_dls_PeekNext(&SessionList, linkPtr);
      SessionCtxPtr = le_dls_Peek(&(sessionNodePtr->msgRefList));

      while (SessionCtxPtr != NULL)
      {
         MsgNode_t* msgRefNode = NULL;

         msgRefNode = CONTAINER_OF(SessionCtxPtr, MsgNode_t, link);
         SessionCtxPtr = le_dls_PeekNext(&(sessionNodePtr->msgRefList), SessionCtxPtr);

         if (msgRefNode->msgRef == msgRef)
         {
            LE_DEBUG("For msgRef %p, get sessionCtx %p", msgRef, sessionNodePtr);

            return sessionNodePtr;
         }
      }
   }

   return NULL;
}

taf_sms_MsgRef_t taf_Sms::SetMsgRefForSessionCtx
(
   taf_sms_Msg_t* msgPtr,
   SessionNode_t* sessionCtxPtr
)
{
   TAF_ERROR_IF_RET_VAL(msgPtr == NULL, NULL, "Invalid msgPtr provided");
   TAF_ERROR_IF_RET_VAL(sessionCtxPtr == NULL, NULL, "Invalid sessionCtxPtr provided");

   MsgNode_t* msgNodePtr = (MsgNode_t*)le_mem_ForceAlloc(MsgRefPool);

   msgNodePtr->msgRef = (taf_sms_MsgRef_t)le_ref_CreateRef(MsgRefMap, msgPtr);
   msgNodePtr->link = LE_DLS_LINK_INIT;
   le_dls_Queue(&(sessionCtxPtr->msgRefList), &(msgNodePtr->link));

   LE_DEBUG("Set msgRef %p for message %p and session %p", msgNodePtr->msgRef, msgPtr, sessionCtxPtr);

   return msgNodePtr->msgRef;
}

void taf_Sms::RemoveMsgRefFromSessionCtx
(
    SessionNode_t*   sessionCtxPtr,
    taf_sms_MsgRef_t msgRef
)
{
   le_dls_Link_t* linkPtr = NULL;

   linkPtr = le_dls_Peek(&sessionCtxPtr->msgRefList);

   while (linkPtr != NULL)
   {
      MsgNode_t* msgRefNode = CONTAINER_OF(linkPtr, MsgNode_t, link);

      linkPtr = le_dls_PeekNext(&sessionCtxPtr->msgRefList, linkPtr);

      if (msgRefNode->msgRef == msgRef)
      {
         LE_DEBUG("For sessionCtxPtr %p, remove msgRef %p", sessionCtxPtr, msgRef);

         le_dls_Remove(&(sessionCtxPtr->msgRefList), &(msgRefNode->link));
         le_ref_DeleteRef(MsgRefMap, msgRefNode->msgRef);

         le_mem_Release(msgRefNode);

         return;
      }
   }
}

taf_sms_RxMsgHandlerRef_t taf_Sms::CreateRxHandlerCtx
(
   SessionNode_t*             sessionCtxPtr,
   taf_sms_RxMsgHandlerFunc_t handlerFuncPtr,
   void*                      contextPtr
)
{
   HandlerNode_t* handlerCtxPtr = (HandlerNode_t*)le_mem_ForceAlloc(HandlerNodePool);
   TAF_ERROR_IF_RET_VAL(handlerCtxPtr == NULL, NULL, "Cannot allocate handlerCtx");

   handlerCtxPtr->handlerFuncPtr = handlerFuncPtr;
   handlerCtxPtr->link = LE_DLS_LINK_INIT;
   handlerCtxPtr->sessionCtxPtr = sessionCtxPtr;
   handlerCtxPtr->handlerRef = (taf_sms_RxMsgHandlerRef_t)le_ref_CreateRef(HandlerRefMap, handlerCtxPtr);
   handlerCtxPtr->userContext = contextPtr;

   le_dls_Queue(&(sessionCtxPtr->handlerList), &(handlerCtxPtr->link));

   return handlerCtxPtr->handlerRef;
}

void taf_Sms::RemoveRxHandlerCtx
(
   taf_sms_RxMsgHandlerRef_t handlerRef
)
{
   HandlerNode_t* handlerCtxPtr = (HandlerNode_t*)le_ref_Lookup(HandlerRefMap, handlerRef);

   TAF_ERROR_IF_RET_NIL(handlerCtxPtr == NULL, "Invalid handlerRef provided");

   le_ref_DeleteRef(HandlerRefMap, handlerRef);

   SessionNode_t* sessionCtxPtr = handlerCtxPtr->sessionCtxPtr;

   TAF_ERROR_IF_RET_NIL(sessionCtxPtr == NULL, "Invalid sessionCtxPtr");

   le_dls_Remove(&(sessionCtxPtr->handlerList), &(handlerCtxPtr->link));
   le_mem_Release(handlerCtxPtr);
}

void taf_Sms::NewSmsHandler
(
   taf_sms_Msg_t *newMsg
)
{
   le_dls_Link_t* linkPtr = le_dls_Peek(&SessionList);
   bool handlerPresent = false;

   while (linkPtr != NULL)
   {
      SessionNode_t* sessionNodePtr = CONTAINER_OF(linkPtr, SessionNode_t, link);
      linkPtr = le_dls_PeekNext(&SessionList, linkPtr);
      le_dls_Link_t* linkHandlerPtr = le_dls_Peek(&(sessionNodePtr->handlerList));

      if (linkHandlerPtr)
      {
         LE_DEBUG("Handler has been registered for the session (%p)", sessionNodePtr);
         handlerPresent = true;
         break;
      }
   }

   TAF_ERROR_IF_RET_NIL(newMsg == NULL, "Invalid newMsg");

   if (false == handlerPresent)
   {
      LE_DEBUG("No client sessions are subscribed for handler.");
      le_mem_Release(newMsg);
      return;
   }

   MessageHandlers(newMsg);
}

void taf_Sms::MessageHandlers
(
   taf_sms_Msg_t* msgPtr
)
{
   bool newMessage = true;

   le_dls_Link_t* linkPtr = le_dls_PeekTail(&SessionList);

   //Call all the handlers from sessions
   while (linkPtr != NULL)
   {
      SessionNode_t* sessionCtxPtr = CONTAINER_OF(linkPtr, SessionNode_t, link);

      linkPtr = le_dls_PeekPrev(&SessionList, linkPtr);

      le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(sessionCtxPtr->handlerList));

      if (linkHandlerPtr != NULL)
      {
         while (linkHandlerPtr != NULL)
         {
            taf_sms_MsgRef_t msgRef = SetMsgRefForSessionCtx(msgPtr, sessionCtxPtr);

            if (msgRef == NULL)
            {
               LE_ERROR("msgRef is NULL");
            }
            else
            {
               if (newMessage)
               {
                  msgPtr->smsUserCount = 1;
                  newMessage = false;
               }
               else
               {
                  msgPtr->smsUserCount++;
               }

               HandlerNode_t * handlerCtxPtr = NULL;

               handlerCtxPtr = CONTAINER_OF(linkHandlerPtr, HandlerNode_t, link);
               linkHandlerPtr = le_dls_PeekPrev(&(sessionCtxPtr->handlerList), linkHandlerPtr);
               handlerCtxPtr->handlerFuncPtr(msgRef, handlerCtxPtr->userContext);

               LE_DEBUG("Handler for sessionRef %p, msgRef %p is called", sessionCtxPtr->sessionRef, msgRef);
            }
         }
      }
      else
      {
         LE_DEBUG("No handler for sessionCtxPtr %p", sessionCtxPtr);
      }
   }
}

taf_sms_MsgListRef_t taf_Sms::CreateNewMsgList
(
   void
)
{
   taf_sms_List_t* smsRxMsgList = (taf_sms_List_t*)le_mem_ForceAlloc(MsgListPool);

   if (smsRxMsgList)
   {
      smsRxMsgList->tmpLink = NULL;
      smsRxMsgList->list = LE_DLS_LIST_INIT;
      smsRxMsgList->sessionRef = taf_sms_GetClientSessionRef();
      smsRxMsgList->msgListRef = (taf_sms_List_t*)le_ref_CreateRef(ListRefMap, smsRxMsgList);

      return smsRxMsgList->msgListRef;
   }
   else
   {
      le_mem_Release(smsRxMsgList);
      return NULL;
   }
}

void taf_Sms::ReleaseSession
(
   le_msg_SessionRef_t sessionRef,
   void*               ctxPtr
)
{
   TAF_ERROR_IF_RET_NIL(sessionRef == NULL, "sessionRef is NULL");

   LE_INFO("SessionRef %p is being closed", sessionRef);

   SessionNode_t* sessionNodePtr = NULL;

   sessionNodePtr = GetSessionNode(sessionRef);

   if (sessionNodePtr != NULL)
   {
      le_dls_Link_t* linkPtr = NULL;

      linkPtr = le_dls_Peek(&(sessionNodePtr->msgRefList));

      while (linkPtr != NULL)
      {
         MsgNode_t* msgRefPtr = NULL;

         msgRefPtr = CONTAINER_OF(linkPtr, MsgNode_t, link);

         linkPtr = le_dls_PeekNext(&(sessionNodePtr->msgRefList), linkPtr);

         taf_sms_Delete(msgRefPtr->msgRef);
      }
   }

   le_ref_IterRef_t iterListRef = le_ref_GetIterator(ListRefMap);
   le_result_t result = le_ref_NextNode(iterListRef);

   while (result == LE_OK)
   {
      taf_sms_List_t* smsListPtr = NULL;

      smsListPtr = (taf_sms_List_t*)le_ref_GetValue(iterListRef);

      if (smsListPtr->sessionRef == sessionRef)
      {
         taf_sms_MsgListRef_t msgListRef = NULL;

         msgListRef = (taf_sms_MsgListRef_t) le_ref_GetSafeRef(iterListRef);

         LE_INFO("Release msgListRef %p", msgListRef);

         taf_sms_DeleteList(msgListRef);
      }

      result = le_ref_NextNode(iterListRef);
   }
}

le_result_t taf_Sms::sendMessage()
{
   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(MsgRefMap, sendingMsgRef);

   smsSentCb = std::make_shared<tafSmsCallback>();
   smsSentCb->msgRef = sendingMsgRef;
   auto smsManager = smsManagers[msgPtr->phoneId - 1];
   smsManager->sendSms(std::string(msgPtr->text), std::string(msgPtr->tel), smsSentCb);

   return LE_OK;
}

void taf_Sms::Init(void)
{

   auto &phoneFactory = PhoneFactory::getInstance();
   phoneManager = phoneFactory.getPhoneManager();

   bool subSystemsStatus = phoneManager->isSubsystemReady();
   if(!subSystemsStatus) {
      LE_INFO("wait for system ready\n");
      std::future<bool> f = phoneManager->onSubsystemReady();
      subSystemsStatus = f.get();
   }

   MsgPool = le_mem_InitStaticPool(SmsMsg,
                                    MAX_OF_SMS_MSG,
                                    sizeof(taf_sms_Msg_t));


   MsgListPool = le_mem_InitStaticPool(ListSms,
                                     MAX_OF_LIST,
                                     sizeof(taf_sms_List_t));

   MsgRefNodePool = le_mem_InitStaticPool(SmsReference,
                                          MAX_OF_SMS_MSG,
                                          sizeof(taf_sms_MsgNode_t));

   MsgRefPool = le_mem_InitStaticPool(MsgRef,
                                       MAX_SMS_SESSION*MAX_OF_SMS_MSG,
                                       sizeof(MsgNode_t));

   HandlerNodePool = le_mem_InitStaticPool(Handler,
                                        MAX_SMS_SESSION,
                                        sizeof(HandlerNode_t));

   SessionNodePool = le_mem_InitStaticPool(SessionCtx,
                                           MAX_SMS_SESSION,
                                           sizeof(SessionNode_t));

   le_msg_AddServiceCloseHandler(taf_sms_GetServiceRef(), taf_Handler::CloseSessionEventHandler, NULL);

   MsgRefMap = le_ref_CreateMap("tafMsgRefMap", MAX_OF_SMS_MSG);

   ListRefMap = le_ref_CreateMap("tafListRefMap", MAX_OF_LIST);

   HandlerRefMap = le_ref_CreateMap("tafHandlerRefMap", MAX_SMS_SESSION);

   SessionList = LE_DLS_LIST_INIT;

   SmsSendSem = le_sem_Create("SmsSendSem", 1);

   // Handle telsdk call events
   NewMsgEvent = le_event_CreateId("tafSms Event", sizeof(newSms_t));
   MsgSendEvent = le_event_CreateId("tafSms send Event", 0);
   MsgSendCallbackEvent = le_event_CreateId("tafSms send callback Event", sizeof(taf_sms_MsgRef_t));

   // Add the state changed handler
   le_event_AddHandler("taf new message", NewMsgEvent, taf_Handler::ProcessNewMessage);
   le_event_AddHandler("taf send message", MsgSendEvent, taf_Handler::ProcessSendMessage);
   le_event_AddHandler("taf callback", MsgSendCallbackEvent, taf_Handler::ProcessSendingStateEvent);

   // Init the handler class static member
   taf_Handler::TafSmsPtr = this;

   smsMgr = phoneFactory.getSmsManager();
   mySmsListener = std::make_shared<tafSmsListener>();

   std::vector<int> phoneIds;
   telux::common::Status status = phoneManager->getPhoneIds(phoneIds);
   if (status == telux::common::Status::SUCCESS) {
      for (auto index = 1; index <= (int)phoneIds.size(); index++) {
         smsMgr = phoneFactory.getSmsManager(index);
         if (smsMgr) {
            // add listeners for incoming SMS notification
            telux::common::Status status = smsMgr->registerListener(mySmsListener);
            if(status != telux::common::Status::SUCCESS) {
               LE_ERROR("Unable to register Listener\n");
            }
            smsManagers.emplace_back(smsMgr);
         }
      }
   }

   smsSentCb = std::shared_ptr<tafSmsCallback>();
   smsDeliveryCb = std::shared_ptr<tafSmsDeliveryCallback>();

   LE_INFO("System ready, start tafSms service!\n");
}


taf_Sms &taf_Sms::GetInstance()
{
   static taf_Sms instance;
   return instance;
}

void tafSmsListener::onIncomingSms(int phoneId, std::shared_ptr<SmsMessage> smsMsg) {

   TAF_ERROR_IF_RET_NIL(smsMsg == nullptr, "smsMsg is nullptr!");

   auto &sms = taf_Sms::GetInstance();

   LE_INFO("Received SMS from phone ID %d from: %s\n", phoneId, smsMsg->getSender().c_str());
   LE_INFO("message: %s\n", smsMsg->toString().c_str());

   newSms_t newMsg = {0};
   le_utf8_Copy(newMsg.tel, smsMsg->getSender().c_str(), DESTINATION_LEN, NULL);
   le_utf8_Copy(newMsg.text, smsMsg->toString().c_str(), TAF_SMS_TEXT_BYTES, NULL);

   le_event_Report(sms.NewMsgEvent, &newMsg, sizeof(newSms_t));
}

void tafSmsCallback::commandResponse(telux::common::ErrorCode error) {

   auto &sms = taf_Sms::GetInstance();

   LE_INFO("onSmsSent error = %d\n", (int)error);

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(sms.MsgRefMap, msgRef);

   TAF_ERROR_IF_RET_NIL(msgPtr == nullptr, "msgPtr is nullptr!");

   if(error == telux::common::ErrorCode::SUCCESS) {
      LE_INFO("onSmsSent successfully\n");
      msgPtr->sendStatus = TAF_SMS_SENT;
   }
   else {
      LE_INFO("onSmsSent failed\n");
      msgPtr->sendStatus = TAF_SMS_SENDING_FAILED;
   }

   le_event_Report(sms.MsgSendCallbackEvent, &msgRef, sizeof(taf_sms_MsgRef_t));
}

void tafSmsDeliveryCallback::commandResponse(telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      LE_INFO("SMS Delivery successfully\n");
   }
   else {
      LE_INFO("SMS Delivery failed\n");
   }
   LE_INFO("SMS Delivery error = %d\n", (int)error);
}

// Implementation of SMSC Address callback
void tafSmscAddressCallback::smscAddressResponse(const std::string &address,
                                                telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      LE_INFO("requestSmscAddress smscAddressResponse:%s\n", address.c_str());
   }
   else {
      LE_INFO("requestSmscAddress failed, errorCode: %d\n", static_cast<int>(error));
   }
}

// Implementation of set SMSC Address callback
void tafSetSmscAddressResponseCallback::setSmscResponse(telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      LE_INFO("setSmscAddress sent successfully\n");
   }
   else {
      LE_INFO("setSmscAddress failed with errorCode: %d\n", static_cast<int>(error));
   }
}
