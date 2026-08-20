/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
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
 */

/*
 *  Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafSms.hpp"

using namespace tafsvc;
using namespace std;

LE_MEM_DEFINE_STATIC_POOL(SmsMsg, MAX_OF_SMS_MSG, sizeof(taf_sms_Msg_t));
LE_MEM_DEFINE_STATIC_POOL(ListSms, MAX_OF_LIST, sizeof(taf_sms_List_t));
LE_MEM_DEFINE_STATIC_POOL(SmsReference, MAX_OF_SMS_MSG, sizeof(taf_sms_MsgNode_t));
LE_MEM_DEFINE_STATIC_POOL(Handler, MAX_SMS_SESSION, sizeof(HandlerNode_t));
LE_MEM_DEFINE_STATIC_POOL(SessionCtx, MAX_SMS_SESSION, sizeof(SessionNode_t));
LE_MEM_DEFINE_STATIC_POOL(MsgRef, MAX_SMS_SESSION*MAX_OF_SMS_MSG, sizeof(MsgNode_t));
LE_MEM_DEFINE_STATIC_POOL(SmsSendStatus, MAX_OF_SMS_MSG, sizeof(tafSmsSendStatus_t));

taf_Sms* taf_Handler::TafSmsPtr = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Encode PDU message
 */
//--------------------------------------------------------------------------------------------------

static le_result_t EncodeMsgToPdu
(
   taf_sms_Msg_t* msgPtr
)
{
   LE_DEBUG("EncodeMsgToPdu");

   if (msgPtr->pduReady)
   {
      LE_DEBUG("PDU format is ready");
      return LE_OK;
   }

   le_result_t result = LE_FAULT;
   smsPdu_EncodeMsg_t encodeData;

   memset(&encodeData, 0, sizeof(encodeData));

   encodeData.protocol = SMS_PROTOCOL_GSM;
   encodeData.addrData = msgPtr->tel;
   encodeData.statusReport = false;

   switch (msgPtr->format)
   {
      case TAF_SMS_FORMAT_TEXT:
         LE_DEBUG("encode TAF_SMS_FORMAT_TEXT");
         encodeData.msgData = (const uint8_t*)msgPtr->text;
         encodeData.msgDataLen = msgPtr->userdataLen;
         encodeData.encoding = PDU_ENCODING_7_BITS;
         encodeData.type = SMS_TYPE_SUBMIT;
         result = smsPdu_Encode(&encodeData, &(msgPtr->pdu));
         break;

      case TAF_SMS_FORMAT_BINARY:
         LE_DEBUG("encode TAF_SMS_FORMAT_BINARY");
         encodeData.msgData = msgPtr->binary;
         encodeData.msgDataLen = msgPtr->userdataLen;
         encodeData.encoding = PDU_ENCODING_8_BITS;
         encodeData.type = SMS_TYPE_SUBMIT;
         result = smsPdu_Encode(&encodeData, &(msgPtr->pdu));
         break;

      case TAF_SMS_FORMAT_UCS2:
         LE_DEBUG("encode TAF_SMS_FORMAT_UCS2");
         encodeData.msgData = msgPtr->binary;
         encodeData.msgDataLen = msgPtr->userdataLen;
         encodeData.encoding = PDU_ENCODING_16_BITS;
         encodeData.type = SMS_TYPE_SUBMIT;
         result = smsPdu_Encode(&encodeData, &(msgPtr->pdu));
         break;

      case TAF_SMS_FORMAT_PDU:
         LE_DEBUG("TAF_SMS_FORMAT_PDU no need to encode");
         result = LE_OK;
         break;

      case TAF_SMS_FORMAT_UNKNOWN:
      default:
         LE_WARN("TAF_SMS_FORMAT_UNKNOWN cannot be encoded");
         result = LE_FAULT;

         break;
   }

   if (result == LE_OK)
   {
      msgPtr->pduReady = true;
   }

   return result;
}

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
   LE_DEBUG("ProcessNewMessage");

   newSms_t *newMsgPtr = (newSms_t*) incomingMsgPtr;

   auto &sms = taf_Sms::GetInstance();

   taf_sms_Msg_t *tafNewMsg = sms.CreateRxMsgNode(nullptr);

   tafNewMsg->phoneId = newMsgPtr->phoneId;

   taf_sms_Pdu_t pduMsg = {0};

   size_t pduHexLen = strnlen(newMsgPtr->pdu, (TAF_SMS_PDU_BYTES * 2) + 1);
   if (pduHexLen % 2 != 0 || (pduHexLen / 2) > TAF_SMS_PDU_BYTES)
   {
      LE_ERROR("Invalid PDU length: hexLen=%zu", pduHexLen);
      le_mem_Release(tafNewMsg);
      return;
   }

   le_hex_StringToBinary(newMsgPtr->pdu, strlen(newMsgPtr->pdu), pduMsg.data, sizeof(pduMsg.data));

   pduMsg.length = strlen(newMsgPtr->pdu) / 2;
   LE_DEBUG("pduMsg.length = %d", pduMsg.length);

   tafNewMsg->pduReady = true;

   if(sms.sysPrefStorage == TAF_SMS_STORAGE_HLOS)
   {
      pduMsg.storage = TAF_SMS_STORAGE_HLOS;

      TAF_ERROR_IF_RET_NIL(pduMsg.length > sizeof(pduMsg.data), "Invalid msg length(%d)", pduMsg.length);

      le_result_t storeRes = taf_sms_hlos_StoreNewMsgToHLOS(&pduMsg);
      if (storeRes == LE_FAULT)
      {
          LE_ERROR("Failed to store new SMS to HLOS, res: %d", storeRes);
          le_mem_Release(tafNewMsg);
          return;
      }
      else if (storeRes == LE_NO_MEMORY)
      {
          LE_ERROR("No available slot in HLOS, notify upper layer without storage info");
          tafNewMsg->storage = TAF_SMS_STORAGE_NONE;
          tafNewMsg->storageIdx = 0;
      }
      else
      {
      tafNewMsg->storage = TAF_SMS_STORAGE_HLOS;
      tafNewMsg->storageIdx = pduMsg.index;
      }
   }

   if(sms.sysPrefStorage == TAF_SMS_STORAGE_SIM)
   {
      tafNewMsg->storage = TAF_SMS_STORAGE_SIM;
      tafNewMsg->storageIdx = newMsgPtr->storageIdx;
   }

   sms_PduMsg_t decodedPduMsg = {0};

   if(smsPdu_Decode(SMS_PROTOCOL_GSM, pduMsg.data, &decodedPduMsg) != LE_OK)
   {
      LE_INFO("smsPdu_Decode fail");
      le_mem_Release(tafNewMsg);
      return;
   }

   if(sms.constructSmsDeliver(tafNewMsg, &decodedPduMsg) != LE_OK)
   {
      LE_INFO("constructSmsDeliver fail");
      le_mem_Release(tafNewMsg);
      return;
   }

   tafNewMsg->pdu.length = pduMsg.length;
   memcpy(&(tafNewMsg->pdu.data), pduMsg.data, tafNewMsg->pdu.length);

   tafNewMsg->readStatus = TAF_SMS_RXSTS_UNREAD;
   tafNewMsg->lockStatus = TAF_SMS_LKSTS_UNLOCKED;
   tafNewMsg->type = TAF_SMS_TYPE_RX;

   sms.NewSmsHandler(tafNewMsg);
}

void taf_Handler::ProcessSendMessage(void* context)
{
   auto &sms = taf_Sms::GetInstance();
   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(sms.MsgRefMap, sms.sendingMsgRef);
   TAF_ERROR_IF_RET_NIL(msgPtr == nullptr, "msgPtr is nullptr!");

   uint32_t timeout = kSendMessageWaitTime;
   uint8_t phoneId = msgPtr->phoneId;

   // Encode to PDU
   le_result_t result = EncodeMsgToPdu(msgPtr);
   if (result != LE_OK)
   {
      LE_ERROR("Cannot encode Message Object %p", msgPtr);
      msgPtr->sendStatus = TAF_SMS_TXSTS_SENDING_FAILED;
      le_event_Report(sms.MsgSendCallbackEvent, &sms.sendingMsgRef, sizeof(taf_sms_MsgRef_t));
      return;
   }

   pa_result_t paRes = taf_pa_sms_SendRawSms(
      msgPtr->pdu.data,
      msgPtr->pdu.length,
      timeout,
      phoneId
   );

   if (paRes != PA_OK)
   {
      LE_ERROR("taf_pa_sms_SendRawSms failed, errorCode: %d", (int)paRes);
      msgPtr->sendStatus = TAF_SMS_TXSTS_SENDING_FAILED;
   }
   else
   {
      LE_DEBUG("taf_pa_sms_SendRawSms was successful");
      msgPtr->sendStatus = TAF_SMS_TXSTS_SENT;
   }

   le_event_Report(sms.MsgSendCallbackEvent, &sms.sendingMsgRef, sizeof(taf_sms_MsgRef_t));
}

void taf_Handler::ProcessSendingStateEvent(void* reportPtr)
{
   auto &sms = taf_Sms::GetInstance();

   tafSmsSendStatus_t* sendStatusMsgPtr = (tafSmsSendStatus_t*) reportPtr;
   TAF_ERROR_IF_RET_NIL(sendStatusMsgPtr == nullptr, "sendMsgRef is nullptr!");

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(sms.MsgRefMap, sendStatusMsgPtr->msgRef);
   if(msgPtr == nullptr)
   {
      LE_ERROR("msgPtr is nullptr!");
      le_mem_Release(sendStatusMsgPtr);
      return;
   }

   taf_sms_CallbackResultFunc_t functionPtr = (taf_sms_CallbackResultFunc_t)(msgPtr->callBackPtr);

   if(sendStatusMsgPtr->result == PA_OK) {
      LE_INFO("SMS sent successfully");
      msgPtr->sendStatus = TAF_SMS_TXSTS_SENT;
   }
   else {
      LE_ERROR("SMS sent failed, err=%d", (int)sendStatusMsgPtr->result);
      msgPtr->sendStatus = TAF_SMS_TXSTS_SENDING_FAILED;
   }

   if (functionPtr)
   {
      LE_DEBUG("Sending CallBack (%p), Status %d", functionPtr, msgPtr->sendStatus);

      functionPtr(sendStatusMsgPtr->msgRef, msgPtr->sendStatus, msgPtr->ctxPtr);
   }
   else
   {
      LE_WARN("No CallBackFunction Found for message, status %d!!", msgPtr->sendStatus);
   }
   le_mem_Release(sendStatusMsgPtr);
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
                  msgPtr->userCount = 1;
                  newMessage = false;
               }
               else
               {
                  msgPtr->userCount++;
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

   smsRxMsgList->list = LE_DLS_LIST_INIT;

   if (ListAllRxMsg(smsRxMsgList) > 0)
   {
      smsRxMsgList->tmpLink = nullptr;
      smsRxMsgList->sessionRef = taf_sms_GetClientSessionRef();
      smsRxMsgList->msgListRef = (taf_sms_List_t*)le_ref_CreateRef(ListRefMap, smsRxMsgList);

      return smsRxMsgList->msgListRef;
   }
   else
   {
      le_mem_Release(smsRxMsgList);
      return nullptr;
   }
}

taf_sms_MsgRef_t taf_Sms::GetFirstMessage
(
   taf_sms_MsgListRef_t msgListRef
)
{
   auto &sms = taf_Sms::GetInstance();

   taf_sms_List_t* listPtr = (taf_sms_List_t*)le_ref_Lookup(sms.ListRefMap, msgListRef);
   TAF_KILL_CLIENT_IF_RET_VAL(listPtr == nullptr, nullptr, "Invalid listPtr provided");

   le_dls_Link_t* msgLinkPtr = le_dls_Peek(&(listPtr->list));

   TAF_ERROR_IF_RET_VAL(msgLinkPtr == nullptr, nullptr, "msgLinkPtr is NULL!");

   taf_sms_MsgNode_t* nodePtr = CONTAINER_OF(msgLinkPtr, taf_sms_MsgNode_t, listLink);
   listPtr->tmpLink = msgLinkPtr;
   return nodePtr->msgRef;
}

taf_sms_Msg_t* taf_Sms::CreateRxMsgNode
(
   taf_sms_Pdu_t*   pduMsg,
   char*            phoneNum,
   taf_sms_Format_t format,
   char*            data,
   int16_t          dataLen
)
{
   taf_sms_Msg_t  *msgPtr;

   msgPtr = (taf_sms_Msg_t*)le_mem_ForceAlloc(MsgPool);

   memset(msgPtr, 0, sizeof(taf_sms_Msg_t));

   msgPtr->tel[0] = '\0';
   msgPtr->text[0] = '\0';
   msgPtr->timestamp[0] = '\0';
   msgPtr->phoneId = DEFAULT_PHONE_ID;

   memcpy(&(msgPtr->pdu), pduMsg, sizeof(taf_sms_Pdu_t));
   msgPtr->pduReady = true;

   msgPtr->type = TAF_SMS_TYPE_RX;
   msgPtr->format = format;
   msgPtr->readStatus = pduMsg->rxStatus;
   msgPtr->lockStatus = pduMsg->lkStatus;

   msgPtr->storage = pduMsg->storage;
   msgPtr->storageIdx = pduMsg->index;
   msgPtr->applyDel = false;

   le_utf8_Copy(msgPtr->tel, phoneNum, TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES, NULL);

   if(format == TAF_SMS_FORMAT_TEXT)
   {
      size_t copyLen = (dataLen > 0) ? (size_t)dataLen : 0;
      if (copyLen >= TAF_SMS_TEXT_BYTES) copyLen = TAF_SMS_TEXT_BYTES - 1;
      memcpy(msgPtr->text, data, copyLen);
      msgPtr->text[copyLen] = '\0';
   }
   else
   {
      size_t copyLen = (dataLen > 0) ? (size_t)dataLen : 0;
      if (copyLen > TAF_SMS_BINARY_BYTES) copyLen = TAF_SMS_BINARY_BYTES;
      memcpy(msgPtr->binary, data, copyLen);
   }

   msgPtr->userdataLen = (dataLen > 0) ? (size_t)dataLen : 0;

   return msgPtr;
}

taf_sms_Msg_t* taf_Sms::CreateRxMsgNode
(
   taf_sms_Pdu_t *pduMsg
)
{
   taf_sms_Msg_t  *msgPtr;

   msgPtr = (taf_sms_Msg_t*)le_mem_ForceAlloc(MsgPool);

   memset(msgPtr, 0, sizeof(taf_sms_Msg_t));

   msgPtr->type = TAF_SMS_TYPE_RX;
   msgPtr->applyDel = false;

   msgPtr->tel[0] = '\0';
   msgPtr->text[0] = '\0';
   msgPtr->timestamp[0] = '\0';

   if(pduMsg != nullptr)
   {
      memcpy(&(msgPtr->pdu), pduMsg, sizeof(taf_sms_Pdu_t));
      msgPtr->pduReady = true;

      msgPtr->readStatus = pduMsg->rxStatus;
      msgPtr->lockStatus = pduMsg->lkStatus;
      msgPtr->storage = pduMsg->storage;
      msgPtr->storageIdx = pduMsg->index;
      msgPtr->phoneId = pduMsg->phoneId;
   }

   return msgPtr;
}

le_result_t taf_Sms::constructSmsDeliver
(
   taf_sms_Msg_t*       msgPtr,
   sms_PduMsg_t*        decodedMsgPtr
)
{
   msgPtr->type = TAF_SMS_TYPE_RX;

   if(decodedMsgPtr->encoding == PDU_ENCODING_7_BITS)
   {
      msgPtr->format = TAF_SMS_FORMAT_TEXT;
   }
   else if(decodedMsgPtr->encoding == PDU_ENCODING_8_BITS)
   {
      msgPtr->format = TAF_SMS_FORMAT_BINARY;
   }
   else if(decodedMsgPtr->encoding == PDU_ENCODING_16_BITS)
   {
      msgPtr->format = TAF_SMS_FORMAT_UCS2;
   }
   else
   {
      msgPtr->format = TAF_SMS_FORMAT_PDU;
   }

   switch (msgPtr->format)
   {
      case TAF_SMS_FORMAT_BINARY:

         msgPtr->userdataLen = decodedMsgPtr->dataLen;
         memcpy(msgPtr->binary, decodedMsgPtr->data, msgPtr->userdataLen);
         break;

      case TAF_SMS_FORMAT_TEXT:

         msgPtr->userdataLen = decodedMsgPtr->dataLen;
         memcpy(msgPtr->text, decodedMsgPtr->data, msgPtr->userdataLen);
         break;

      case TAF_SMS_FORMAT_UCS2:

         msgPtr->userdataLen = decodedMsgPtr->dataLen;
         memcpy(msgPtr->binary, decodedMsgPtr->data, msgPtr->userdataLen);
         break;

      case TAF_SMS_FORMAT_PDU:
         break;

      default:
         LE_CRIT("Unknown format %d", msgPtr->format);
         return LE_FAULT;
   }

   if (msgPtr->format != TAF_SMS_FORMAT_PDU)
   {
      memcpy(msgPtr->tel, decodedMsgPtr->addr, TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES);
   }

   return LE_OK;
}

taf_sms_Msg_t* taf_Sms::CreateAndConstructMsg
(
    taf_sms_Pdu_t*   pduMsgPtr,
    sms_PduMsg_t*    decodedMsgPtr
)
{
   taf_sms_Msg_t* newMsgPtr = CreateRxMsgNode(pduMsgPtr);

   switch (decodedMsgPtr->type)
   {
      case SMS_TYPE_DELIVER:
         if (constructSmsDeliver(newMsgPtr, decodedMsgPtr) != LE_OK)
         {
            LE_INFO("constructSmsDeliver failed");
            le_mem_Release(newMsgPtr);
            newMsgPtr = NULL;
         }
         break;
      case SMS_TYPE_PDU:
         LE_INFO("SMS type: SMS_TYPE_PDU, currently not supported");
         break;
      case SMS_TYPE_CELL_BROADCAST:
         LE_INFO("SMS type: SMS_TYPE_CELL_BROADCAST, currently not supported");
         break;
      case SMS_TYPE_STATUS_REPORT:
         LE_INFO("SMS type: SMS_TYPE_STATUS_REPORT, currently not supported");
         default:
      break;
   }

   return newMsgPtr;
}

uint32_t taf_Sms::GetMsgFromStorage
(
   taf_sms_List_t      *msgListPtr,
   taf_sms_Storage_t   storage,
   uint32_t            numOfMsg,
   uint32_t            *arrayPtr,
   uint8_t             phoneId
)
{
   TAF_ERROR_IF_RET_VAL(msgListPtr == nullptr, LE_FAULT, "msgListPtr is nullptr!");

   TAF_ERROR_IF_RET_VAL(arrayPtr == nullptr, LE_FAULT, "arrayPtr is nullptr!");

   uint32_t getMsgCount = 0;

   auto &sms = taf_Sms::GetInstance();

   for (uint32_t i = 0 ; i < numOfMsg ; ++i)
   {
      taf_sms_Pdu_t pduMsg = {0};

      le_result_t res = LE_OK;
      if(storage == TAF_SMS_STORAGE_HLOS)
      {
         res = taf_sms_hlos_ReadPDUMsgFromStorage(arrayPtr[i], &pduMsg);
      }
      else
      {
         res = sms.ReadFromStorage(&pduMsg, arrayPtr[i], storage, phoneId);
      }

      if (res != LE_OK)
      {
         LE_ERROR("readMessage failed for index[%d]", arrayPtr[i]);
         continue;
      }

      if (pduMsg.length > TAF_SMS_PDU_BYTES)
      {
         LE_ERROR("PDU length (%u) out of range for index[%d]", pduMsg.length, arrayPtr[i]);
         continue;
      }

      sms_Protocol_t msgType = SMS_PROTOCOL_GSM;
      sms_PduMsg_t decodedPduMsg = {0};

      if (smsPdu_Decode(msgType,
                        pduMsg.data,
                        &decodedPduMsg) == LE_OK)
      {
         LE_DEBUG("decodedPduMsg.type: %d", decodedPduMsg.type);

         if (decodedPduMsg.type != SMS_TYPE_SUBMIT)
         {
            taf_sms_Msg_t* newMsg = CreateAndConstructMsg(&pduMsg, &decodedPduMsg);

            if (newMsg == NULL)
            {
               LE_ERROR("create rx message node failed");
               continue;
            }

            taf_sms_MsgNode_t* msgNodePtr = (taf_sms_MsgNode_t*)le_mem_ForceAlloc(MsgRefNodePool);
            msgNodePtr->msgRef = (taf_sms_MsgRef_t)le_ref_CreateRef(MsgRefMap, newMsg);

            newMsg->userCount++;

            LE_DEBUG("create rx node[%p], obj[%p], ref[%p]", msgNodePtr, newMsg, msgNodePtr->msgRef);

            msgNodePtr->listLink = LE_DLS_LINK_INIT;
            le_dls_Queue(&(msgListPtr->list), &(msgNodePtr->listLink));

            getMsgCount++;
         }
      }
   }

   return getMsgCount;
}

uint32_t taf_Sms::ListRxMsg
(
   taf_sms_List_t          *msgListPtr,
   taf_sms_ReadStatus_t    rxStatus,
   taf_sms_Storage_t       storage,
   uint8_t                 phoneId
)
{
   TAF_ERROR_IF_RET_VAL(msgListPtr == nullptr, 0, "msgListPtr is nullptr!");
   uint32_t numOfIdx = 0;
   uint32_t idxArray[MAX_OF_SMS_MSG_IN_STORAGE] = {0};
   uint32_t msgCount = 0;

   if(storage == TAF_SMS_STORAGE_HLOS)
   {
      le_result_t result = taf_sms_hlos_ListMsgFromStorage(rxStatus, &numOfIdx, idxArray);
      TAF_ERROR_IF_RET_VAL(result != LE_OK, 0,
         "taf_sms_hlos_ListMsgFromStorage result: %d", result);
      TAF_ERROR_IF_RET_VAL(numOfIdx >= MAX_OF_SMS_MSG_IN_STORAGE, LE_FAULT,
         "Too much SMS to read %d", numOfIdx);
   }
   else
   {
      taf_pa_sms_Tag smsTagType;
      switch(rxStatus)
      {
         case TAF_SMS_RXSTS_READ:
            smsTagType = taf_pa_sms_Tag::TAF_PA_READ;
            break;
         case TAF_SMS_RXSTS_UNREAD:
            smsTagType = taf_pa_sms_Tag::TAF_PA_NOT_READ;
            break;
         default:
            smsTagType = taf_pa_sms_Tag::TAF_PA_UNKNOWN;
      }

      int32_t paListCount = 0;
      pa_result_t ret = taf_pa_sms_RequestSmsMessageList(idxArray,
         MAX_OF_SMS_MSG_IN_STORAGE, kListRxMsgWaitTime, smsTagType, phoneId, &paListCount);
      if (ret != PA_OK)
      {
          LE_ERROR("taf_pa_sms_RequestSmsMessageList failed, errorCode: %d", (int)ret);
          return -1;
      }
      else
      {
          LE_DEBUG("taf_pa_sms_RequestSmsMessageList was successful, count = %d", paListCount);
          numOfIdx = (uint32_t)paListCount;
      }
   }

   msgCount = GetMsgFromStorage(msgListPtr, storage, numOfIdx, idxArray, phoneId);
   if(msgCount == 0)
   {
      LE_WARN("No message retrieve for storage %d", storage);
   }
   return msgCount;
}

uint32_t taf_Sms::ListAllRxMsg
(
   taf_sms_List_t *msgListPtr
)
{
   int32_t res;
   int32_t msgCount = 0;

   TAF_ERROR_IF_RET_VAL(msgListPtr == nullptr, 0, "msgListPtr is nullptr!");

   for(uint8_t phoneId = 1; phoneId <= NumOfSlot; phoneId++)
   {
      res = ListRxMsg(msgListPtr, TAF_SMS_RXSTS_READ, TAF_SMS_STORAGE_SIM, phoneId);
      if (res < 0)
      {
         LE_ERROR("Read SIM storage was unsuccessful, return %d", res);
         return LE_FAULT;
      }
      msgCount += res;

      res = ListRxMsg(msgListPtr, TAF_SMS_RXSTS_UNREAD, TAF_SMS_STORAGE_SIM, phoneId);
      if (res < 0)
      {
         LE_ERROR("Read SIM storage was unsuccessful, return %d", res);
         return LE_FAULT;
      }
      msgCount += res;
   }

   res = ListRxMsg(msgListPtr, TAF_SMS_RXSTS_READ, TAF_SMS_STORAGE_HLOS, 0);
   if (res < 0)
   {
      LE_ERROR("Read HLOS storage was unsuccessful, return %d",res);
      return LE_FAULT;
   }
   msgCount += res;

   res = ListRxMsg(msgListPtr, TAF_SMS_RXSTS_UNREAD, TAF_SMS_STORAGE_HLOS, 0);
   if (res < 0)
   {
      LE_ERROR("Read HLOS storage was unsuccessful, return %d",res);
      return LE_FAULT;
   }
   msgCount += res;

   return msgCount;
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

   std::vector<taf_sms_MsgListRef_t> toDelete;
   le_ref_IterRef_t iterListRef = le_ref_GetIterator(ListRefMap);
   le_result_t result = le_ref_NextNode(iterListRef);

   while (result == LE_OK)
   {
      taf_sms_List_t* smsListPtr = NULL;

      smsListPtr = (taf_sms_List_t*)le_ref_GetValue(iterListRef);

      if(smsListPtr != NULL)
      {
         if (smsListPtr->sessionRef == sessionRef)
         {
            taf_sms_MsgListRef_t msgListRef =

               (taf_sms_MsgListRef_t) le_ref_GetSafeRef(iterListRef);


            toDelete.push_back(msgListRef);
         }
      }

      result = le_ref_NextNode(iterListRef);
   }
   for (taf_sms_MsgListRef_t msgListRef : toDelete)
   {
      LE_INFO("Release msgListRef %p", msgListRef);
      taf_sms_DeleteList(msgListRef);
   }
}

le_result_t taf_Sms::ReadFromStorage(taf_sms_Pdu_t* pduMsg,
   uint32_t idx, taf_sms_Storage_t storage, uint8_t phoneId)
{
   taf_pa_sms_Tag pduRxStatus;
   std::vector<uint8_t> pduBuffer;
   uint32_t pduMsgIndex;

   pa_result_t paRes = taf_pa_sms_ReadMessage(idx, kReadFromStorageWaitTime,
      phoneId, &pduRxStatus, pduBuffer, &pduMsgIndex);
   if (paRes != PA_OK)
   {
      LE_ERROR("taf_pa_sms_ReadMessage failed, errorCode: %d", (int)paRes);
      return LE_FAULT;
   }
   LE_DEBUG("taf_pa_sms_ReadMessage was successful");

   pduMsg->storage = storage;
   pduMsg->length = pduBuffer.size();
   if (pduMsg->length > TAF_SMS_PDU_BYTES)
   {
      LE_ERROR("PDU length (%u) out of range for index[%d]", pduMsg->length, idx);
      return LE_FAULT;
   }

   for (unsigned int i = 0; i < pduMsg->length; ++i)
   {
      pduMsg->data[i] = pduBuffer[i];
   }

   pduMsg->index = pduMsgIndex;
   switch (pduRxStatus)
   {
      case taf_pa_sms_Tag::TAF_PA_READ:
         pduMsg->rxStatus = TAF_SMS_RXSTS_READ;
         break;
      case taf_pa_sms_Tag::TAF_PA_NOT_READ:
         pduMsg->rxStatus = TAF_SMS_RXSTS_UNREAD;
         break;
      default:
         pduMsg->rxStatus = TAF_SMS_RXSTS_UNKNOWN;
   }

   return LE_OK;
}

le_result_t taf_Sms::SendMessage(taf_sms_Msg_t* msgPtr)
{
   TAF_ERROR_IF_RET_VAL(msgPtr == nullptr, LE_FAULT, "msgPtr is nullptr!");

   le_result_t result = EncodeMsgToPdu(msgPtr);
   if (result != LE_OK)
   {
      LE_ERROR("Cannot encode Message Object %p", msgPtr);
      return LE_FORMAT_ERROR;
   }

   if (msgPtr->phoneId < 1 || msgPtr->phoneId > 2)
   {
      return LE_BAD_PARAMETER;
   }

   return SendPDUMessageSync(msgPtr->pdu.data, msgPtr->pdu.length,
      kSendMessageWaitTime, msgPtr->phoneId);
}

le_result_t taf_Sms::SendPDUMessageSync
(
   uint8_t     *pduData,
   uint32_t    pduLength,
   uint32_t    timeout,
   uint8_t     phoneId
)
{
   pa_result_t paRes = taf_pa_sms_SendRawSms(pduData, pduLength, timeout, phoneId);
   if (paRes != PA_OK)
   {
      LE_ERROR("taf_pa_sms_SendRawSms failed, errorCode: %d", (int)paRes);
      return LE_FAULT;

   }
   LE_DEBUG("taf_pa_sms_SendRawSms was successful");
   return LE_OK;
}

le_result_t taf_Sms::SendPDUMessageAsync(taf_sms_MsgRef_t msgRef)
{
   auto &sms = taf_Sms::GetInstance();
   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(sms.MsgRefMap, msgRef);
   TAF_ERROR_IF_RET_VAL(msgPtr == nullptr, LE_FAULT, "msgPtr is nullptr!");

   le_result_t result = EncodeMsgToPdu(msgPtr);
   if (result != LE_OK)
   {
      LE_ERROR("Cannot encode Message Object %p", msgPtr);
      msgPtr->sendStatus = TAF_SMS_TXSTS_SENDING_FAILED;
      le_event_Report(sms.MsgSendCallbackEvent, &msgRef, sizeof(taf_sms_MsgRef_t));
      return LE_FORMAT_ERROR;
   }

   uint8_t *pduData = msgPtr->pdu.data;
   uint32_t pduLength = msgPtr->pdu.length;
   uint8_t phoneId = msgPtr->phoneId;

   if (phoneId < 1 || phoneId > 2)
   {
      msgPtr->sendStatus = TAF_SMS_TXSTS_SENDING_FAILED;
      le_event_Report(sms.MsgSendCallbackEvent, &msgRef, sizeof(taf_sms_MsgRef_t));
      return LE_BAD_PARAMETER;
   }

   if (pduLength == 0)
   {
      LE_INFO("pduLength is 0");
      msgPtr->sendStatus = TAF_SMS_TXSTS_SENDING_FAILED;
      le_event_Report(sms.MsgSendCallbackEvent, &msgRef, sizeof(taf_sms_MsgRef_t));
      return LE_BAD_PARAMETER;
   }

   if (pduLength > TAF_SMS_PDU_BYTES)
   {
      LE_INFO("pduLength [%u] is greater than TAF_SMS_PDU_BYTES\n",
        pduLength);
      msgPtr->sendStatus = TAF_SMS_TXSTS_SENDING_FAILED;
      le_event_Report(sms.MsgSendCallbackEvent, &msgRef, sizeof(taf_sms_MsgRef_t));
      return LE_OUT_OF_RANGE;
   }

   auto cb = [msgRef](pa_result_t result)
   {
      auto &sms = taf_Sms::GetInstance();

      tafSmsSendStatus_t* msgSendStatusPtr = (tafSmsSendStatus_t*)le_mem_ForceAlloc(sms.SmsSendStatusPool);
      if (msgSendStatusPtr == nullptr) {
          LE_ERROR("Failed to allocate memory for send status, cannot report callback");
          return;
      }

      msgSendStatusPtr->msgRef = msgRef;
      msgSendStatusPtr->result = result;

      le_event_ReportWithRefCounting(sms.MsgSendCallbackEvent, msgSendStatusPtr);
   };

   pa_result_t paRes = taf_pa_sms_SendPDUMessageAsync(phoneId, pduData, pduLength, cb);
   if (paRes != PA_OK)
   {
       LE_WARN("taf_pa_sms_SendPDUMessageAsync failed, errorCode: %d", (int)paRes);
       return LE_FAULT;
   }
   return LE_OK;
}


le_result_t taf_Sms::SetTag(taf_sms_Msg_t* msgPtr, taf_pa_sms_Tag tagType)
{
   pa_result_t paRes = taf_pa_sms_SetTag(msgPtr->storageIdx, tagType,
      kSetTagWaitTime, DEFAULT_PHONE_ID);
   if (paRes != PA_OK)
   {
      LE_ERROR("taf_pa_sms_SetTag failed, errorCode: %d", (int)paRes);
      return LE_FAULT;
   }

   LE_DEBUG("taf_pa_sms_SetTag was successful");
   return LE_OK;
}

le_result_t taf_Sms::DeleteMessage(uint32_t messageIndex)
{
   pa_result_t paRes = taf_pa_sms_DeleteMessage(messageIndex, kDeleteMessageWaitTime,
      DEFAULT_PHONE_ID);
   if (paRes != PA_OK)
   {
      LE_ERROR("taf_pa_sms_DeleteMessage failed, errorCode: %d", (int)paRes);
      return LE_FAULT;
   }

   LE_DEBUG("taf_pa_sms_DeleteMessage was successful");
   return LE_OK;
}

le_result_t taf_Sms::DeleteAllMessages(taf_sms_Storage_t storage)
{
   auto &sms = taf_Sms::GetInstance();

   taf_sms_MsgListRef_t listRef = CreateNewMsgList();
   if (listRef == nullptr)
   {
      LE_INFO("listRef is NULL");
      return LE_UNSUPPORTED;
   }

   taf_sms_MsgRef_t msgRef = GetFirstMessage(listRef);
   do
   {
      if(msgRef == nullptr)
      {
         break;
      }

      taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(sms.MsgRefMap, msgRef);
      TAF_ERROR_IF_RET_VAL(msgPtr == nullptr, LE_FAULT, "msgPtr is nullptr!");

      if((msgPtr->storage == storage) && (msgPtr->userCount == 1))
      {
         le_result_t res = DeleteMessage(msgPtr->storageIdx);
         LE_DEBUG("Delete result: %d, storage: %d, index: %d", res,
            msgPtr->storage, msgPtr->storageIdx);

         msgPtr->applyDel = true;
         LE_DEBUG("applyDel for storage: %d, index: %d", msgPtr->storage, msgPtr->storageIdx);
      }
   }
   while ((msgRef = taf_sms_GetNext(listRef)) != nullptr);

   taf_sms_DeleteList(listRef);

   return LE_OK;
}

void taf_Sms::Init(void)
{
   pa_result_t paInitRes = taf_pa_sms_Init();
   if (paInitRes != PA_OK)
   {
       LE_FATAL("Cannot initialize SMS platform adaptor, errorcode: %d", (int)paInitRes);
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

   SmsSendStatusPool = le_mem_InitStaticPool(SmsSendStatus,
                                    MAX_OF_SMS_MSG,
                                    sizeof(tafSmsSendStatus_t));

   le_msg_AddServiceCloseHandler(taf_sms_GetServiceRef(), taf_Handler::CloseSessionEventHandler, NULL);

   MsgRefMap = le_ref_CreateMap("tafMsgRefMap", MAX_OF_SMS_MSG);

   ListRefMap = le_ref_CreateMap("tafListRefMap", MAX_OF_LIST);

   HandlerRefMap = le_ref_CreateMap("tafHandlerRefMap", MAX_SMS_SESSION);

   SessionList = LE_DLS_LIST_INIT;

   SmsSendSem = le_sem_Create("SmsSendSem", 1);

   // Handle telsdk call events
   NewMsgEvent = le_event_CreateId("tafSms Event", sizeof(newSms_t));
   MsgSendEvent = le_event_CreateId("tafSms send Event", 0);
   MsgSendCallbackEvent = le_event_CreateIdWithRefCounting("tafSms send callback Event");

   // Add the state changed handler
   le_event_AddHandler("taf new message", NewMsgEvent, taf_Handler::ProcessNewMessage);
   le_event_AddHandler("taf send message", MsgSendEvent, taf_Handler::ProcessSendMessage);
   le_event_AddHandler("taf callback", MsgSendCallbackEvent, taf_Handler::ProcessSendingStateEvent);

   // Init the handler class static member
   taf_Handler::TafSmsPtr = this;

   // Initialize preferred storage from persistent config
   taf_sms_Storage_t prefStorage = GetConfig_PreferredStorage();
   if(prefStorage != TAF_SMS_STORAGE_UNKNOWN)
   {
      SetPreferredStorage(prefStorage);
   }
   else
   {
      SetPreferredStorage(TAF_SMS_STORAGE_HLOS);
   }

   pa_result_t regIncomingRes = taf_pa_sms_RegisterIncomingSmsCallback
   (
      [](int phoneId, const std::string &pdu, const std::string &sender, int storageIdx)
      {
         LE_INFO("Incoming SMS detected");
         newSms_t newMsg = {0};
         newMsg.phoneId = static_cast<uint8_t>(phoneId);
         le_utf8_Copy(newMsg.pdu, pdu.c_str(), (TAF_SMS_PDU_BYTES * 2) + 1, NULL);
         newMsg.storageIdx = storageIdx;
         le_event_Report(taf_Sms::GetInstance().NewMsgEvent, &newMsg, sizeof(newSms_t));
      }
   );
   if (regIncomingRes != PA_OK)
   {
       LE_ERROR("taf_pa_sms_RegisterIncomingSmsCallback failed, errorCode: %d", (int)regIncomingRes);
   }

   pa_result_t regMemFullRes = taf_pa_sms_RegisterMemoryFullCallback
   (
      [](int phoneId, taf_pa_sms_StorageFullType fullType)
      {
         LE_INFO("Memory full detected");
         taf_sms_hlos_StorageInd_t storageInd;
         storageInd.fullType = TAF_SMS_FULL_UNKNOWN;
         switch(fullType)
         {
            case taf_pa_sms_StorageFullType::TAF_PA_FULL_SIM:
               if(phoneId == 1)
               {
                  storageInd.fullType = TAF_SMS_FULL_SIM;
               }
               else
               {
                  storageInd.fullType = TAF_SMS_FULL_SIM2;
               }
               le_event_Report(taf_Sms::GetInstance().StorageEvent,
                  (void*)&storageInd, sizeof(storageInd));
               break;
            default:
               break;
         }
      }
   );
   if (regMemFullRes != PA_OK)
   {
       LE_ERROR("taf_pa_sms_RegisterMemoryFullCallback failed, errorCode: %d", (int)regMemFullRes);
   }

   LE_INFO("System ready, start tafSms service!\n");
}

taf_Sms &taf_Sms::GetInstance()
{
   static taf_Sms instance;
   return instance;
}

le_result_t taf_Sms::ActivateCellBroadcast(uint8_t phoneId, bool activate)
{
   pa_result_t paRes = taf_pa_sms_SetActivationStatus(phoneId, activate,
      TIMEOUT_ACTIVATE_CB);
   if (paRes != PA_OK)
   {
      LE_ERROR("taf_pa_sms_SetActivationStatus failed, errorCode: %d", (int)paRes);
      return LE_FAULT;
   }

   LE_DEBUG("Set Activation status request sent successfully");
   return LE_OK;
}

le_result_t taf_Sms::RequestBroadcastIds(uint8_t phoneId)
{
   if (phoneId < MIN_PHONE_ID || phoneId > MAX_PHONE_ID)
   {
      return LE_BAD_PARAMETER;
   }
   pa_result_t paRes = taf_pa_sms_RequestMessageFilters(phoneId,
      TIMEOUT_RQUEST_CB_FILTER);
   if (paRes != PA_OK)
   {
      LE_ERROR("taf_pa_sms_RequestMessageFilters failed, errorCode: %d", (int)paRes);
      return LE_FAULT;
   }

   LE_DEBUG("taf_pa_sms_RequestMessageFilters was successful");
   return LE_OK;
}

le_result_t taf_Sms::AddCellBroadcastIds(uint8_t phoneId, uint16_t fromId, uint16_t toId)
{
   if (phoneId < MIN_PHONE_ID || phoneId > MAX_PHONE_ID)
   {
      return LE_BAD_PARAMETER;
   }

   if (fromId > toId)
   {
      return LE_BAD_PARAMETER;
   }

   // Retrieve current filter list
   TAF_ERROR_IF_RET_VAL(RequestBroadcastIds(phoneId) != LE_OK,
                        LE_FAULT, "Request message filter failed");

   pa_result_t paRes = taf_pa_sms_AddCellBroadcastIds(phoneId,
      fromId, toId, TIMEOUT_RQUEST_CB_FILTER);
   if (paRes != PA_OK)
   {
      LE_ERROR("taf_pa_sms_AddCellBroadcastIds failed, errorCode: %d", (int)paRes);
      return LE_FAULT;
   }

   LE_DEBUG("taf_pa_sms_AddCellBroadcastIds was successful");
   return LE_OK;
}

le_result_t taf_Sms::RemoveCellBroadcastIds(uint8_t phoneId, uint16_t fromId, uint16_t toId)
{
   if (phoneId < MIN_PHONE_ID || phoneId > MAX_PHONE_ID)
   {
      return LE_BAD_PARAMETER;
   }

   if (fromId > toId)
   {
      return LE_BAD_PARAMETER;
   }

   // Retrieve current filter list
   TAF_ERROR_IF_RET_VAL(RequestBroadcastIds(phoneId) != LE_OK,
                        LE_FAULT, "Request broadcast filter failed");

   pa_result_t paRes = taf_pa_sms_RemoveCellBroadcastIds(phoneId,
      fromId, toId, TIMEOUT_RQUEST_CB_FILTER);
   if (paRes != PA_OK)
   {
      LE_ERROR("taf_pa_sms_RemoveCellBroadcastIds failed, errorCode: %d", (int)paRes);
      return LE_FAULT;
   }

   LE_DEBUG("taf_pa_sms_RemoveCellBroadcastIds was successful");
   return LE_OK;
}

le_result_t taf_Sms::GetPreferredStorage(taf_sms_Storage_t* storage)
{
   if (sysPrefStorage == TAF_SMS_STORAGE_HLOS)
   {
      *storage = TAF_SMS_STORAGE_HLOS;
      return LE_OK;
   }

   taf_pa_sms_Storage type;
   pa_result_t paRes = taf_pa_sms_GetPreferredStorage(&type,
      kPreferredStorageWaitTime, DEFAULT_PHONE_ID);
   if (paRes != PA_OK)
   {
      LE_ERROR("taf_pa_sms_GetPreferredStorage failed, errorCode: %d", (int)paRes);
      return LE_FAULT;
   }
   else
   {
      LE_DEBUG("taf_pa_sms_GetPreferredStorage was successful");
   }

   switch(type)
   {
       case taf_pa_sms_Storage::TAF_PA_STORAGE_NONE:
           sysPrefStorage = TAF_SMS_STORAGE_NONE;
           break;
       case taf_pa_sms_Storage::TAF_PA_STORAGE_SIM:
           sysPrefStorage = TAF_SMS_STORAGE_SIM;
           break;
       default:
           sysPrefStorage = TAF_SMS_STORAGE_UNKNOWN;
           break;
   }

   LE_INFO("Get preferred storage = %d", sysPrefStorage);
   *storage = sysPrefStorage;

   return LE_OK;
}

le_result_t taf_Sms::SetPreferredStorage(taf_sms_Storage_t storage)
{
   taf_pa_sms_Storage type;
   switch(storage)
   {
      case TAF_SMS_STORAGE_NONE:
      case TAF_SMS_STORAGE_HLOS:
            type = taf_pa_sms_Storage::TAF_PA_STORAGE_NONE;
            break;
      case TAF_SMS_STORAGE_SIM:
            type = taf_pa_sms_Storage::TAF_PA_STORAGE_SIM;
            break;
      default:
            return LE_UNSUPPORTED;
   }

   le_result_t res = LE_FAULT;
   for(uint8_t phoneId = 1; phoneId <= NumOfSlot; phoneId++)
   {
      pa_result_t paRes = taf_pa_sms_SetPreferredStorage(type, kPreferredStorageWaitTime, phoneId);
      res = (paRes == PA_OK) ? LE_OK : LE_FAULT;
      if(res == LE_OK)
      {
         sysPrefStorage = storage;
         LE_INFO("Set preferred storage as %d", sysPrefStorage);

         SetConfig_PreferredStorage(storage);
         taf_sms_hlos_SetPrefStorage(storage);
      }
      else
      {
         LE_ERROR("taf_pa_sms_SetPreferredStorage failed, errorcode: %d", (int)paRes);
         return res;
      }
   }

   return res;
}

le_result_t taf_Sms::SetConfig_PreferredStorage(const taf_sms_Storage_t storage)
{
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_MODEMSERVICE_SMS_PATH );

    char config_node_storage[LENGTH_CFG_NODE] = {};
    snprintf(config_node_storage, sizeof(config_node_storage), "%s", CFG_NODE_PREFERRED_STORAGE);

    le_cfg_SetInt(iteratorRef, config_node_storage, storage);
    le_cfg_CommitTxn(iteratorRef);

    LE_INFO("Set config node %s as %d", config_node_storage, storage);

    return LE_OK;
}

taf_sms_Storage_t taf_Sms::GetConfig_PreferredStorage()
{
    taf_sms_Storage_t storage = TAF_SMS_STORAGE_UNKNOWN;
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn( CFG_MODEMSERVICE_SMS_PATH );

    char config_node_storage[LENGTH_CFG_NODE] = {};
    snprintf(config_node_storage, sizeof(config_node_storage), "%s", CFG_NODE_PREFERRED_STORAGE);

    if (le_cfg_NodeExists(iteratorRef, config_node_storage))
    {
        int32_t configStorage = le_cfg_GetInt(iteratorRef,
                                       config_node_storage, TAF_SMS_STORAGE_UNKNOWN);

        storage = (taf_sms_Storage_t)configStorage;

        LE_INFO("Get config node %s = %d", config_node_storage, storage);
        le_cfg_CancelTxn(iteratorRef);
        return storage;
    }

    LE_WARN("config node %s doesn't exist", config_node_storage);

    le_cfg_CancelTxn(iteratorRef);
    return TAF_SMS_STORAGE_UNKNOWN;
}
