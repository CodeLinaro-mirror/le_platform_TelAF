/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ​​​​​Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "legato.h"
#include "interfaces.h"
#include "telux/tel/PhoneFactory.hpp"
#include "telux/common/DeviceConfig.hpp"
#include "tafSms.hpp"
#include <unistd.h>
#include <stdlib.h>

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
   LE_INFO("ProcessNewMessage");

   newSms_t *newMsgPtr = (newSms_t*) incomingMsgPtr;

   auto &sms = taf_Sms::GetInstance();

   if(sms.sysPrefStorage == TAF_SMS_STORAGE_HLOS)
   {
      taf_pa_sms_Pdu_t pduMsg = {0};

      pduMsg.storage = TAF_SMS_STORAGE_NONE;

      le_hex_StringToBinary(newMsgPtr->pdu, strlen(newMsgPtr->pdu), pduMsg.data, sizeof(pduMsg.data));

      pduMsg.length = strlen(newMsgPtr->pdu) / 2;
      LE_INFO("pduMsg.length = %d", pduMsg.length);

      TAF_ERROR_IF_RET_NIL(pduMsg.length > sizeof(pduMsg.data), "Invalid msg length(%d)", pduMsg.length);

      taf_pa_sms_StoreNewMsgToHLOS(&pduMsg);
   }

   taf_sms_Msg_t *tafNewMsg = (taf_sms_Msg_t*)le_mem_ForceAlloc(sms.MsgPool);
   memset(tafNewMsg, 0, sizeof(taf_sms_Msg_t));

   sms_PduMsg_t decodedPduMsg = {0};
   uint8_t pdu[TAF_SMS_PDU_BYTES] = {0};

   le_hex_StringToBinary(newMsgPtr->pdu, strlen(newMsgPtr->pdu), pdu, sizeof(pdu));

   if(smsPdu_Decode(SMS_PROTOCOL_GSM, pdu, &decodedPduMsg) != LE_OK)
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

   tafNewMsg->readStatus = TAF_SMS_RXSTS_UNREAD;
   tafNewMsg->lockStatus = TAF_SMS_LKSTS_UNLOCKED;
   tafNewMsg->type = TAF_SMS_TYPE_RX;

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
      smsRxMsgList->tmpLink = NULL;
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

taf_sms_Msg_t* taf_Sms::CreateRxMsgNode
(
   taf_pa_sms_Pdu_t *pduMsg,
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
   msgPtr->phoneId = DEFAULT_SLOT_ID;

   memcpy(&(msgPtr->pdu), pduMsg, sizeof(taf_pa_sms_Pdu_t));
   msgPtr->pduReady = true;

   msgPtr->type = TAF_SMS_TYPE_RX;
   msgPtr->format = format;
   msgPtr->readStatus = pduMsg->rxStatus;
   msgPtr->lockStatus = pduMsg->lkStatus;

   msgPtr->storage = pduMsg->storage;
   msgPtr->storageIdx = pduMsg->index;
   msgPtr->applyDel = false;

   memcpy(msgPtr->tel, phoneNum, TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES);

   if(format == TAF_SMS_FORMAT_TEXT)
   {
      memcpy(msgPtr->text, data, TAF_SMS_TEXT_BYTES);
   }
   else
   {
      memcpy(msgPtr->binary, data, TAF_SMS_BINARY_BYTES);
   }

   msgPtr->userdataLen = (size_t)dataLen;

   return msgPtr;
}

taf_sms_Msg_t* taf_Sms::CreateRxMsgNode
(
   taf_pa_sms_Pdu_t *pduMsg
)
{
   taf_sms_Msg_t  *msgPtr;

   msgPtr = (taf_sms_Msg_t*)le_mem_ForceAlloc(MsgPool);

   memset(msgPtr, 0, sizeof(taf_sms_Msg_t));

   memcpy(&(msgPtr->pdu), pduMsg, sizeof(taf_pa_sms_Pdu_t));
   msgPtr->pduReady = true;

   msgPtr->readStatus = pduMsg->rxStatus;
   msgPtr->lockStatus = pduMsg->lkStatus;
   msgPtr->storage = pduMsg->storage;
   msgPtr->storageIdx = pduMsg->index;

   msgPtr->type = TAF_SMS_TYPE_RX;
   msgPtr->applyDel = false;

   msgPtr->tel[0] = '\0';
   msgPtr->text[0] = '\0';
   msgPtr->timestamp[0] = '\0';

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
    taf_pa_sms_Pdu_t*   pduMsgPtr,
    sms_PduMsg_t*       decodedMsgPtr
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
   uint32_t            *arrayPtr
)
{
   TAF_ERROR_IF_RET_VAL(msgListPtr == nullptr, LE_FAULT, "msgListPtr is nullptr!");

   TAF_ERROR_IF_RET_VAL(arrayPtr == nullptr, LE_FAULT, "arrayPtr is nullptr!");

   uint32_t getMsgCount = 0;

   for (uint32_t i = 0 ; i < numOfMsg ; i++)
   {
      taf_pa_sms_Pdu_t pduMsg = {0};

      le_result_t res = taf_pa_sms_ReadPDUMsgFromStorage(storage, arrayPtr[i], &pduMsg);

      if (res != LE_OK)
      {
         LE_ERROR("taf_pa_sms_ReadPDUMsgFromStorage failed, index[%d]", arrayPtr[i]);
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
   taf_sms_Storage_t       storage
)
{
   le_result_t  result = LE_OK;

   uint32_t numOfIdx = 0;
   uint32_t idxArray[MAX_OF_SMS_MSG_IN_STORAGE]={0};

   uint32_t msgCount = 0;

   TAF_ERROR_IF_RET_VAL(msgListPtr == nullptr, 0, "msgListPtr is nullptr!");

   result = taf_pa_sms_ListMsgFromStorage(storage, rxStatus, &numOfIdx, idxArray);

   TAF_ERROR_IF_RET_VAL(result != LE_OK, 0, "taf_pa_sms_ListMsgFromStorage result: %d", result);

   TAF_ERROR_IF_RET_VAL(numOfIdx >= MAX_OF_SMS_MSG_IN_STORAGE, LE_FAULT, "Too much SMS to read %d", numOfIdx);

   if (numOfIdx == 0)
   {
      return 0;
   }
   else
   {
      int32_t res;
      res = GetMsgFromStorage(msgListPtr, storage, numOfIdx, idxArray);

      if(res == LE_FAULT)
      {
         LE_WARN("No message retrieve for storage %d", storage);
      }
      else
      {
         msgCount = res;
      }
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

   res = ListRxMsg(msgListPtr, TAF_SMS_RXSTS_READ, TAF_SMS_STORAGE_SIM);
   if (res < 0)
   {
         LE_ERROR("Read SIM storage unsuccessfully, return %d",res);
         return LE_FAULT;
   }
   msgCount += res;

   res = ListRxMsg(msgListPtr, TAF_SMS_RXSTS_UNREAD, TAF_SMS_STORAGE_SIM);
   if (res < 0)
   {
         LE_ERROR("Read SIM storage unsuccessfully, return %d",res);
         return LE_FAULT;
   }
   msgCount += res;

   res = ListRxMsg(msgListPtr, TAF_SMS_RXSTS_READ, TAF_SMS_STORAGE_HLOS);
   if (res < 0)
   {
      LE_ERROR("Read NV storage unsuccessfully, return %d",res);
      return LE_FAULT;
   }
   msgCount += res;

   res = ListRxMsg(msgListPtr, TAF_SMS_RXSTS_UNREAD, TAF_SMS_STORAGE_HLOS);
   if (res < 0)
   {
      LE_ERROR("Read NV storage unsuccessfully, return %d",res);
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

   smsSentCb->msgRef = sendingMsgRef;
   auto smsManager = smsManagers[msgPtr->phoneId - 1];
   smsManager->sendSms(std::string(msgPtr->text), std::string(msgPtr->tel), smsSentCb);

   return LE_OK;
}

void taf_Sms::Init(void)
{
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
   SmscGetSem = le_sem_Create("SmscGetSem", 0);
   SmscSetSem = le_sem_Create("SmscSetSem", 0);

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

   int noOfSlots = MIN_SIM_SLOT_COUNT;
   if(telux::common::DeviceConfig::isMultiSimSupported()) {
      noOfSlots = MAX_SIM_SLOT_COUNT;
      LE_INFO("MultiSim supported");
   }

   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   mySmsListener = std::make_shared<tafSmsListener>();

   for(auto index = 1; index <= noOfSlots; index++) {
      std::promise<telux::common::ServiceStatus> prom;
      auto smsMgr = phoneFactory.getSmsManager(index, [&](telux::common::ServiceStatus status) {
         prom.set_value(status);
      });

      if (!smsMgr) {
         LE_ERROR("Failed to get SMS Manager instance ");
      }

      telux::common::ServiceStatus smsMgrStatus = prom.get_future().get();
      if (smsMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
      {
         auto status = smsMgr->registerListener(mySmsListener);
         {
            if(status != telux::common::Status::SUCCESS)
            {
               LE_ERROR("Unable to register Listener");
            }
            smsManagers.emplace_back(smsMgr);
         }
      }
      else
      {
         LE_ERROR("Unable to initialize SMS Manager");
      }

      auto CbMgr = phoneFactory.getCellBroadcastManager(static_cast<SlotId>(index));
      if (CbMgr) {
         //  Check if cellbroadcast subsystem is ready
         bool subSystemStatus = CbMgr->isSubsystemReady();
         //  If cellbroadcast subsystem is not ready, wait for it to be ready
         if (!subSystemStatus) {
            LE_INFO("Cellbroadcast subsystem is not ready, Please wait");
            std::future<bool> f = CbMgr->onSubsystemReady();
            // If we want to wait unconditionally for cellbroadcast subsystem to be ready
            subSystemStatus = f.get();

            //  Exit the application, if SDK is unable to initialize cell broadcast subsystem
            //  for any of the slot
            if (!subSystemStatus) {
               LE_ERROR("Unable to initialize Cellbroadcast SMS Manager");
            }
         }
         CbManagers.emplace_back(CbMgr);
      }
   }

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

   smsSentCb = std::make_shared<tafSmsCallback>();
   getSmscCb = std::make_shared<tafSmscAddressCallback>();

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

   LE_INFO("Received SMS from phone ID %d from: %s", phoneId, smsMsg->getSender().c_str());
   LE_INFO("message: %s", smsMsg->getText().c_str());

   LE_INFO("sysPrefStorage: %d", sms.sysPrefStorage);

   newSms_t newMsg = {0};

   le_utf8_Copy(newMsg.pdu, smsMsg->getPdu().c_str(), (TAF_SMS_PDU_BYTES * 2) + 1, NULL);

   LE_INFO("PDU: %s", smsMsg->getPdu().c_str());

   le_event_Report(sms.NewMsgEvent, &newMsg, sizeof(newSms_t));
}

void tafSmsCallback::commandResponse(telux::common::ErrorCode error) {

   auto &sms = taf_Sms::GetInstance();

   taf_sms_MsgRef_t tmpMsgRef = msgRef;
   le_sem_Post(sms.SmsSendSem);

   LE_INFO("onSmsSent error = %d\n", (int)error);

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(sms.MsgRefMap, tmpMsgRef);

   TAF_ERROR_IF_RET_NIL(msgPtr == nullptr, "msgPtr is nullptr!");

   if(error == telux::common::ErrorCode::SUCCESS) {
      LE_INFO("onSmsSent successfully\n");
      msgPtr->sendStatus = TAF_SMS_TXSTS_SENT;
   }
   else {
      LE_INFO("onSmsSent failed\n");
      msgPtr->sendStatus = TAF_SMS_TXSTS_SENDING_FAILED;
   }

   le_event_Report(sms.MsgSendCallbackEvent, &tmpMsgRef, sizeof(taf_sms_MsgRef_t));
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
                                                 telux::common::ErrorCode error)
{
   auto &sms = taf_Sms::GetInstance();

   if(error == telux::common::ErrorCode::SUCCESS)
   {
      LE_INFO("requestSmscAddress smscAddressResponse:%s\n", address.c_str());
      le_utf8_Copy(sms.smscAddr, address.c_str(), TAF_SMS_SMSC_ADDR_BYTES - 1, NULL);
      sms.SmsCenterSyncPromise.set_value(LE_OK);
   }
   else
   {
      LE_INFO("requestSmscAddress failed, errorCode: %d\n", static_cast<int>(error));
      sms.SmsCenterSyncPromise.set_value(LE_FAULT);
   }
}

// Implementation of set SMSC Address callback
void tafSetSmscAddressResponseCallback::setSmscResponse(telux::common::ErrorCode error)
{
   auto &sms = taf_Sms::GetInstance();

   if(error == telux::common::ErrorCode::SUCCESS)
   {
      LE_INFO("setSmscAddress sent successfully\n");
      sms.SmsCenterSyncPromise.set_value(LE_OK);
   }
   else
   {
      LE_INFO("setSmscAddress failed with errorCode: %d\n", static_cast<int>(error));
      sms.SmsCenterSyncPromise.set_value(LE_FAULT);
   }
}

// Implementation of set SMS cellbroadcast activate status callback
void tafSetSmsCBResponseCallback::setSmsCBResponse(telux::common::ErrorCode error)
{
   auto &sms = taf_Sms::GetInstance();
   if(error == telux::common::ErrorCode::SUCCESS)
   {
      LE_INFO("Set Activation status request sent successfully\n");
      sms.CBActivateSyncPromise.set_value(LE_OK);
   }
   else
   {
      LE_INFO("Set Activation status request failed with errorCode: %d\n", static_cast<int>(error));
      sms.CBActivateSyncPromise.set_value(LE_FAULT);
   }
}

void tafSetSmsCBResponseCallback::requestFilterResponse(
    std::vector<telux::tel::CellBroadcastFilter> filters,
    telux::common::ErrorCode errorCode)
{
   auto &sms = taf_Sms::GetInstance();

   if (errorCode == telux::common::ErrorCode::SUCCESS)
   {
      LE_INFO("Request for get msg filters successfully");
      for (uint index = 0; index < filters.size(); index++)
      {
         LE_INFO("Filter[%d]:", index);
         LE_INFO("Start msg id: %d", filters[index].startMessageId);
         LE_INFO("End msg id: %d", filters[index].endMessageId);
      }
      sms.CBFilterList = filters;
      sms.CBRequestIdsSyncPromise.set_value(LE_OK);
   }
   else
   {
      LE_INFO("Request for msg filters failed with errorCode: %d", static_cast<int>(errorCode));
      sms.CBRequestIdsSyncPromise.set_value(LE_FAULT);
   }
}

void tafSetSmsCBResponseCallback::updateFilterResponse(telux::common::ErrorCode error)
{
   auto &sms = taf_Sms::GetInstance();

   if(error == telux::common::ErrorCode::SUCCESS)
   {
      LE_INFO("Set Activation status request sent successfully");
      sms.CBAddIdsSyncPromise.set_value(LE_OK);
   }
   else
   {
      LE_INFO("Set Activation status request failed with errorCode: %d", static_cast<int>(error));
      sms.CBAddIdsSyncPromise.set_value(LE_FAULT);
   }
}

le_result_t taf_Sms::ActivateCellBroadcast(uint8_t phoneId, bool activate)
{
   // initialize the synchronous promise
   CBActivateSyncPromise = std::promise<le_result_t>();
   std::chrono::seconds span(TIMEOUT_ACTIVATE_CB);
   auto &sms = taf_Sms::GetInstance();
   auto CbMgr = sms.CbManagers[phoneId - 1];

   if (CbMgr)
   {
      telux::common::Status reqStatus = CbMgr->setActivationStatus(activate, tafSetSmsCBResponseCallback::setSmsCBResponse);

      if (reqStatus != telux::common::Status::SUCCESS)
      {
         LE_INFO("Set Activation status request failed");
         return LE_FAULT;
      }

      // blocking here to get call event response
      std::future<le_result_t> futResult = CBActivateSyncPromise.get_future();
      std::future_status waitStatus = futResult.wait_for(span);
      if (std::future_status::timeout == waitStatus)
      {
        LE_ERROR("waiting promise timeout for %d seconds", TIMEOUT_ACTIVATE_CB);
        return LE_TIMEOUT;
      }
      else
      {
        return futResult.get();
      }
   }
   else
   {
      LE_ERROR("Cell broadcast service error");
      return LE_FAULT;
   }
}

le_result_t taf_Sms::RequestBroadcastIds(uint8_t phoneId)
{
   if (phoneId < MIN_PHONE_ID || phoneId > MAX_PHONE_ID)
   {
      return LE_BAD_PARAMETER;
   }

   CBRequestIdsSyncPromise = std::promise<le_result_t>();
   std::chrono::seconds span(TIMEOUT_RQUEST_CB_FILTER);
   auto &sms = taf_Sms::GetInstance();
   auto CbMgr = sms.CbManagers[phoneId - 1];

   if (CbMgr)
   {
      telux::common::Status reqStatus = CbMgr->requestMessageFilters(tafSetSmsCBResponseCallback::requestFilterResponse);

      if (reqStatus != telux::common::Status::SUCCESS)
      {
         LE_INFO("Set Activation status request failed");
         return LE_FAULT;
      }

      // blocking here to get call event response
      std::future<le_result_t> futResult = CBRequestIdsSyncPromise.get_future();
      std::future_status waitStatus = futResult.wait_for(span);
      if (std::future_status::timeout == waitStatus)
      {
        LE_ERROR("Waiting promise timeout for %d seconds", TIMEOUT_RQUEST_CB_FILTER);
        return LE_TIMEOUT;
      }
      else
      {
        return futResult.get();
      }
   }
   else
   {
      LE_ERROR("Cell broadcast service error");
      return LE_FAULT;
   }


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

   // Add new filter to current filter list
   telux::tel::CellBroadcastFilter filter = {};
   filter.startMessageId = fromId;
   filter.endMessageId = toId;
   CBFilterList.emplace_back(filter);

   // initialize the synchronous promise
   CBAddIdsSyncPromise = std::promise<le_result_t>();
   std::chrono::seconds span(TIMEOUT_ACTIVATE_CB);
   auto &sms = taf_Sms::GetInstance();
   auto CbMgr = sms.CbManagers[phoneId - 1];

   if (CbMgr)
   {
      telux::common::Status reqStatus = CbMgr->updateMessageFilters(CBFilterList, tafSetSmsCBResponseCallback::updateFilterResponse);

      if (reqStatus != telux::common::Status::SUCCESS)
      {
         LE_INFO("Update message filters failed");
         return LE_FAULT;
      }

      // blocking here to get call event response
      std::future<le_result_t> futResult = CBAddIdsSyncPromise.get_future();
      std::future_status waitStatus = futResult.wait_for(span);
      if (std::future_status::timeout == waitStatus)
      {
        LE_ERROR("Waiting promise timeout for %d seconds", TIMEOUT_UPDATE_CB_FILTER);
        return LE_TIMEOUT;
      }
      else
      {
        return futResult.get();
      }
   }
   else
   {
      LE_ERROR("Cell broadcast service error");
      return LE_FAULT;
   }
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

   bool overlapped = false;

   // Detect whether the input filter is overlapped with current filter list
   for (uint index = 0; index < CBFilterList.size(); index++)
   {
      // If the filter is totally covered by the input filter, remove the current filter
      if(fromId <= CBFilterList[index].startMessageId && toId >= CBFilterList[index].endMessageId)
      {
         CBFilterList.erase(CBFilterList.begin() + index);

         overlapped = true;
         continue;
      }

      // If the input filter is totally covered by the current filter, seperate filter and remove the input range
      if(CBFilterList[index].startMessageId <= fromId && CBFilterList[index].endMessageId >= toId)
      {
         // Shrink the front part to eliminate input range
         CBFilterList[index].endMessageId = fromId - 1;

         // Add a new filter to cover back part and eliminate the input range
         telux::tel::CellBroadcastFilter filter = {};
         filter.startMessageId = toId + 1;
         filter.endMessageId = CBFilterList[index].endMessageId;
         CBFilterList.emplace_back(filter);

         overlapped = true;
         continue;
      }

      // If fromId is within the filter, reset the filter coverage
      if(fromId >= CBFilterList[index].startMessageId && fromId <= CBFilterList[index].endMessageId)
      {
         CBFilterList[index].endMessageId = fromId - 1;
         overlapped = true;
      }

      // If toId is within the filter, reset the filter coverage
      if(toId >= CBFilterList[index].startMessageId && toId <= CBFilterList[index].endMessageId)
      {
         CBFilterList[index].startMessageId = toId + 1;
         overlapped = true;
      }
   }

   if(overlapped == false)
   {
      return LE_OK;
   }

   // initialize the synchronous promise
   CBAddIdsSyncPromise = std::promise<le_result_t>();
   std::chrono::seconds span(TIMEOUT_ACTIVATE_CB);
   auto &sms = taf_Sms::GetInstance();
   auto CbMgr = sms.CbManagers[phoneId - 1];

   if (CbMgr)
   {
      telux::common::Status reqStatus = CbMgr->updateMessageFilters(CBFilterList, tafSetSmsCBResponseCallback::updateFilterResponse);

      if (reqStatus != telux::common::Status::SUCCESS)
      {
         LE_INFO("Update message filters failed");
         return LE_FAULT;
      }

      // blocking here to get call event response
      std::future<le_result_t> futResult = CBAddIdsSyncPromise.get_future();
      std::future_status waitStatus = futResult.wait_for(span);
      if (std::future_status::timeout == waitStatus)
      {
        LE_ERROR("Waiting promise timeout for %d seconds", TIMEOUT_UPDATE_CB_FILTER);
        return LE_TIMEOUT;
      }
      else
      {
        return futResult.get();
      }
   }
   else
   {
      LE_ERROR("Cell broadcast service error");
      return LE_FAULT;
   }
}

// Implementation of get preferred storage callback
void tafSetSmsStorageCallback::getPreferredStorageResponse(telux::tel::StorageType type,
   telux::common::ErrorCode errorCode)
{
    auto &sms = taf_Sms::GetInstance();

    if(errorCode == telux::common::ErrorCode::SUCCESS)
    {
        LE_INFO("Request for get preferred storage sent successfully");

        switch(type)
        {
            case telux::tel::StorageType::NONE:
                sms.sysPrefStorage = TAF_SMS_STORAGE_NONE;
                break;
            case telux::tel::StorageType::SIM:
                sms.sysPrefStorage = TAF_SMS_STORAGE_SIM;
                break;
            default:
                sms.sysPrefStorage = TAF_SMS_STORAGE_UNKNOWN;
                break;
        }
        sms.PreferredStorageSyncPromise.set_value(LE_OK);
    }
    else
    {
        LE_INFO("Request for get preferred storage failed with errorCode: %d", static_cast<int>(errorCode));
        sms.PreferredStorageSyncPromise.set_value(LE_FAULT);
    }
}

// Implementation of set preferred storage callback
void tafSetSmsStorageCallback::setPreferredStorageResponse(telux::
   common::ErrorCode errorCode)
{
    auto &sms = taf_Sms::GetInstance();

    if(errorCode == telux::common::ErrorCode::SUCCESS)
    {
        LE_INFO("Request for set preferred storage sent successfully");
        sms.PreferredStorageSyncPromise.set_value(LE_OK);
    }
    else
    {
        LE_INFO("Request for set preferred storage failed with errorCode: %d", static_cast<int>(errorCode));
        sms.PreferredStorageSyncPromise.set_value(LE_FAULT);
    }
}

le_result_t taf_Sms::GetPreferredStorage(taf_sms_Storage_t* storage)
{
   if(sysPrefStorage == TAF_SMS_STORAGE_HLOS)
   {
      *storage = TAF_SMS_STORAGE_HLOS;
      return LE_OK;
   }

   // initialize the synchronous promise
   PreferredStorageSyncPromise = std::promise<le_result_t>();
   std::chrono::seconds span(TIMEOUT_PREF_STORAGE);
   auto smsManager = smsManagers[DEFAULT_SLOT_ID - 1];

   if (smsManager)
   {
      telux::common::Status reqStatus = smsManager->requestPreferredStorage(
         tafSetSmsStorageCallback::getPreferredStorageResponse);

      if (reqStatus != telux::common::Status::SUCCESS)
      {
         LE_INFO("Get preferred storage failed");
         return LE_FAULT;
      }

      // blocking here to get preferred storage
      std::future<le_result_t> futResult = PreferredStorageSyncPromise.get_future();
      std::future_status waitStatus = futResult.wait_for(span);
      if (std::future_status::timeout == waitStatus)
      {
        LE_ERROR("waiting promise timeout for %d seconds", TIMEOUT_PREF_STORAGE);
        return LE_TIMEOUT;
      }
      else
      {
         le_result_t res = futResult.get();
         if(res == LE_OK)
         {
            *storage = sysPrefStorage;
            LE_INFO("Get preferred storage = %d", sysPrefStorage);
         }
         return res;
      }
   }
   else
   {
      return LE_FAULT;
   }
}

le_result_t taf_Sms::SetPreferredStorage(taf_sms_Storage_t storage)
{
   // initialize the synchronous promise
   PreferredStorageSyncPromise = std::promise<le_result_t>();
   std::chrono::seconds span(TIMEOUT_PREF_STORAGE);
   auto smsManager = smsManagers[DEFAULT_SLOT_ID - 1];

   telux::tel::StorageType type;

   switch(storage)
   {
      case TAF_SMS_STORAGE_NONE:
      case TAF_SMS_STORAGE_HLOS:
            type = telux::tel::StorageType::NONE;
            break;
      case TAF_SMS_STORAGE_SIM:
            type = telux::tel::StorageType::SIM;
            break;
      default:
            return LE_UNSUPPORTED;
   }

   if (smsManager)
   {
      telux::common::Status reqStatus = smsManager->setPreferredStorage(static_cast<telux::tel::StorageType>(type),
         tafSetSmsStorageCallback::setPreferredStorageResponse);

      if (reqStatus != telux::common::Status::SUCCESS)
      {
         LE_INFO("Set preferred storage failed");
         return LE_FAULT;
      }

      // blocking here to set preferred storage
      std::future<le_result_t> futResult = PreferredStorageSyncPromise.get_future();
      std::future_status waitStatus = futResult.wait_for(span);
      if (std::future_status::timeout == waitStatus)
      {
        LE_ERROR("waiting promise timeout for %d seconds", TIMEOUT_PREF_STORAGE);
        return LE_TIMEOUT;
      }
      else
      {
         le_result_t res = futResult.get();
         if(res == LE_OK)
         {
            sysPrefStorage = storage;
            LE_INFO("Set preferred storage as %d", sysPrefStorage);

            SetConfig_PreferredStorage(storage);
            taf_pa_sms_SetPrefStorage(storage);
         }
         return res;
      }
   }
   else
   {
      return LE_FAULT;
   }
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
