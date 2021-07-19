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

/*
 * @file       tafSMSSvc.cpp
 * @brief      This file provides the SMS service as interfaces described
 *             in tafSMSSvc.api, including sending message, receiving message from
 *             handler etc.
 */

#include "legato.h"

#include "interfaces.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <unistd.h>

#include "tafSms.hpp"

using namespace telux::tel;
using namespace telux::common;
using namespace telux::tafsvc;
using namespace std;

/*======================================================================

FUNCTION       taf_sms_AddRxMsgHandler

DESCRIPTION    Register handler to receive new SMS handler

DEPENDENCIES   Initialization of SMS service

PARAMETERS     [IN] taf_sms_RxMsgHandlerFunc_t handlerFuncPtr: Handler to process new message
               [IN] void* contextPtr: context to process new message

RETURN VALUE   taf_sms_RxMsgHandlerRef_t
                  Rx message handler reference

SIDE EFFECTS

======================================================================*/

taf_sms_RxMsgHandlerRef_t taf_sms_AddRxMsgHandler
(
    taf_sms_RxMsgHandlerFunc_t   handlerFuncPtr,
    void*                        contextPtr
)
{
   TAF_ERROR_IF_RET_VAL(handlerFuncPtr == NULL, NULL, "Input handlerPtr is NULL!");

   TAF_KILL_CLIENT_IF_RET_VAL(NULL == handlerFuncPtr, NULL, "handlerFuncPtr is NULL");

   auto &mySms = taf_Sms::GetInstance();
   taf_sms_RxMsgHandlerRef_t  handlerRef;

   // Session node doesn't exist, create a new one
   SessionNode_t* sessionNode = mySms.GetSessionNode(taf_sms_GetClientSessionRef());
   if (!sessionNode)
   {
      sessionNode = mySms.CreateSessionCtx();

      TAF_ERROR_IF_RET_VAL(sessionNode == NULL, NULL, "System unable to create the session node");
   }

   handlerRef = mySms.CreateRxHandlerCtx(sessionNode, handlerFuncPtr, contextPtr);

   return handlerRef;
}

/*======================================================================

FUNCTION       taf_sms_RemoveRxMsgHandler

DESCRIPTION    Deregister RX message handler

DEPENDENCIES   Register RX message handler

PARAMETERS     [IN] taf_sms_RxMsgHandlerRef_t handlerRef: Handler referenced

RETURN VALUE   None

SIDE EFFECTS

======================================================================*/

void taf_sms_RemoveRxMsgHandler
(
    taf_sms_RxMsgHandlerRef_t   handlerRef
)
{
    auto &mySms = taf_Sms::GetInstance();
    mySms.RemoveRxHandlerCtx(handlerRef);
}

/*======================================================================

FUNCTION       taf_sms_CreateNewRxMsgList

DESCRIPTION    Create message list for accessing messages

DEPENDENCIES   Initialization of SMS service

PARAMETERS     None

RETURN VALUE   taf_sms_MsgListRef_t: new message list

SIDE EFFECTS

======================================================================*/

taf_sms_MsgListRef_t taf_sms_CreateNewRxMsgList
(
    void
)
{
   auto &mySms = taf_Sms::GetInstance();
   return mySms.CreateNewMsgList();
}

/*======================================================================

FUNCTION       taf_sms_GetFirst

DESCRIPTION    Get first message of list

DEPENDENCIES   Create RX message list

PARAMETERS     [IN] taf_sms_MsgListRef_t msgListRef: message list reference

RETURN VALUE   taf_sms_MsgRef_t: the first message of message list

SIDE EFFECTS

======================================================================*/

taf_sms_MsgRef_t taf_sms_GetFirst
(
    taf_sms_MsgListRef_t        msgListRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_MsgNode_t* nodePtr = NULL;
   le_dls_Link_t* msgLinkPtr = NULL;
   taf_sms_List_t* listPtr = (taf_sms_List_t*)le_ref_Lookup(mySms.ListRefMap, msgListRef);

   TAF_KILL_CLIENT_IF_RET_VAL(listPtr == NULL, NULL, "Invalid listPtr provided");

   msgLinkPtr = le_dls_Peek(&(listPtr->list));

   TAF_ERROR_IF_RET_VAL(msgLinkPtr == NULL, NULL, "msgLinkPtr is NULL!");

   nodePtr = CONTAINER_OF(msgLinkPtr, taf_sms_MsgNode_t, listLink);
   listPtr->tmpLink = msgLinkPtr;
   return nodePtr->msgRef;
}

/*======================================================================

FUNCTION       taf_sms_GetNext

DESCRIPTION    Get next message of list

DEPENDENCIES   Create RX message list

PARAMETERS     [IN] taf_sms_MsgListRef_t msgListRef: message list reference

RETURN VALUE   taf_sms_MsgRef_t: the next message from tmp link of the message list

SIDE EFFECTS

======================================================================*/

taf_sms_MsgRef_t taf_sms_GetNext
(
    taf_sms_MsgRef_t        msgListRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_MsgNode_t*   nodePtr = NULL;
   le_dls_Link_t*       msgLinkPtr = NULL;
   taf_sms_List_t*      listPtr = (taf_sms_List_t*)le_ref_Lookup(mySms.ListRefMap, msgListRef);

   TAF_KILL_CLIENT_IF_RET_VAL(listPtr == NULL, NULL, "Invalid listPtr provided");

   msgLinkPtr = le_dls_PeekNext(&(listPtr->list), listPtr->tmpLink);

   TAF_ERROR_IF_RET_VAL(msgLinkPtr == NULL, NULL, "msgLinkPtr is NULL!");

   nodePtr = CONTAINER_OF(msgLinkPtr, taf_sms_MsgNode_t, listLink);
   listPtr->tmpLink = msgLinkPtr;
   return nodePtr->msgRef;
}

/*======================================================================

FUNCTION       taf_sms_Create

DESCRIPTION    Create new message for sending

DEPENDENCIES   Initialization of SMS service

PARAMETERS     None

RETURN VALUE   taf_sms_MsgRef_t: the created message

SIDE EFFECTS

======================================================================*/

taf_sms_MsgRef_t taf_sms_Create
(
    void
)
{
   taf_sms_Msg_t  *msgPtr;

   auto &mySms = taf_Sms::GetInstance();

   SessionNode_t* sessionNode = mySms.GetSessionNode(taf_sms_GetClientSessionRef());

   // Session node doesn't exist, create a new one
   if (!sessionNode)
   {
      sessionNode = mySms.CreateSessionCtx();

      TAF_ERROR_IF_RET_VAL(sessionNode == NULL, NULL, "System unable to create the session node");
   }

   msgPtr = (taf_sms_Msg_t*)le_mem_ForceAlloc(mySms.MsgPool);

   msgPtr->tel[0] = '\0';
   msgPtr->text[0] = '\0';
   msgPtr->timestamp[0] = '\0';
   msgPtr->phoneId = DEFAULT_SLOT_ID;

   msgPtr->type = TAF_SMS_TX;
   msgPtr->sendStatus = TAF_SMS_UNSENT;

   msgPtr->userdataLen = 0;
   msgPtr->smsUserCount = 1;
   msgPtr->inList = false;
   msgPtr->callBackPtr = NULL;
   msgPtr->ctxPtr = NULL;

   // Return a message reference
   return mySms.SetMsgRefForSessionCtx(msgPtr, sessionNode);
}

/*======================================================================

FUNCTION       taf_sms_SetDestination

DESCRIPTION    Set dentination phone number for message

DEPENDENCIES   Create new message

PARAMETERS     [IN] taf_sms_MsgListRef_t msgRef: specific message
               [IN] const char* destPtr: destination phone number

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_FAULT: Fail
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_SetDestination
(
    taf_sms_MsgRef_t    msgRef,
    const char*         destPtr
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgPtr provided");

   size_t length = strnlen(destPtr, TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES);

   TAF_KILL_CLIENT_IF_RET_VAL(length > (TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES-1), LE_FAULT, "strlen(dest) > %d", (TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES-1));

   TAF_ERROR_IF_RET_VAL(length == 0, LE_BAD_PARAMETER, "Input string length = 0");

   le_utf8_Copy(msgPtr->tel, destPtr, TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES, NULL);

   return LE_OK;
}

/*======================================================================

FUNCTION       taf_sms_SetText

DESCRIPTION    Set text for message

DEPENDENCIES   Create new message

PARAMETERS     [IN] taf_sms_MsgListRef_t msgRef: specific message
               [IN] const char* destPtr: message content

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_FAULT: Fail
                  LE_BAD_PARAMETER: Invalid parameters
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_SetText
(
    taf_sms_MsgRef_t    msgRef,
    const char*         textPtr
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgRef provided");

   size_t length = strnlen(textPtr, TAF_SMS_TEXT_BYTES);

   TAF_KILL_CLIENT_IF_RET_VAL(length > (TAF_SMS_TEXT_BYTES-1), LE_FAULT, "strlen(text) > %d", (TAF_SMS_TEXT_BYTES-1));

   TAF_ERROR_IF_RET_VAL(length == 0, LE_BAD_PARAMETER, "Input string length = 0");

   msgPtr->userdataLen = length;
   LE_DEBUG("Try to copy data %s, len.%zd @ msgPtr->text.%p for msgPtr.%p",
            textPtr, length, msgPtr->text, msgPtr);

   le_utf8_Copy(msgPtr->text, textPtr, sizeof(msgPtr->text), NULL);

   return LE_OK;
}

/*======================================================================

FUNCTION       taf_sms_SetCallback

DESCRIPTION    Set callback function for message

DEPENDENCIES   Create new message

PARAMETERS     [IN] taf_sms_MsgListRef_t msgRef: specific message
               [IN] taf_sms_CallbackResultFunc_t handlerPtr: handler function
               [IN] void* contextPtr: context pointer

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_BAD_PARAMETER: Invalid parameters
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_SetCallback
(
   taf_sms_MsgRef_t              msgRef,
   taf_sms_CallbackResultFunc_t  handlerPtr,
   void* contextPtr
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgPtr provided");

   TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, LE_BAD_PARAMETER, "Invalid handlerPtr");

   msgPtr->callBackPtr = (void*)handlerPtr;

   msgPtr->ctxPtr = (void*)contextPtr;

   LE_DEBUG("Assign handler %p", handlerPtr);

   return LE_OK;
}

/*======================================================================

FUNCTION       taf_sms_SetPhoneId

DESCRIPTION    Set phone ID function for message

DEPENDENCIES   Create new message

PARAMETERS     [IN] taf_sms_MsgListRef_t msgRef: specific message
               [IN] uint8 phoneId: phone ID

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_SetPhoneId
(
   taf_sms_MsgRef_t  msgRef,
   uint8_t           phoneId
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgPtr provided");

   if (phoneId < 1 || phoneId > 2)
   {
      msgPtr->phoneId = DEFAULT_SLOT_ID;
   }
   else
   {
      msgPtr->phoneId = phoneId;
   }

   return LE_OK;
}

static void ReInitializeList
(
    le_dls_List_t*  msgListPtr
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_MsgNode_t*  msgNode;
   le_dls_Link_t *currentListLink;

   currentListLink = le_dls_Pop(msgListPtr);
   if (currentListLink != NULL)
   {
      do
      {
         msgNode = CONTAINER_OF(currentListLink, taf_sms_MsgNode_t, listLink);

         taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgNode->msgRef);

         if (msgPtr != NULL)
         {
            (msgPtr->smsUserCount)--;

            if (msgPtr->smsUserCount == 0)
            {
               taf_sms_DeleteFromStorage(msgNode->msgRef);
            }

            le_mem_Release(msgPtr);
         }

         le_ref_DeleteRef(mySms.MsgRefMap, msgNode->msgRef);

         currentListLink = le_dls_Pop(msgListPtr);

         le_mem_Release(msgNode);

      } while (currentListLink != NULL);
   }
}

/*======================================================================

FUNCTION       taf_sms_DeleteList

DESCRIPTION    Delete message list

DEPENDENCIES   Create message list

PARAMETERS     [IN] taf_sms_MsgListRef_t msgListRef: message list reference

RETURN VALUE   None

SIDE EFFECTS

======================================================================*/

void taf_sms_DeleteList
(
    taf_sms_MsgListRef_t     msgListRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_List_t* msgListPtr = (taf_sms_List_t*)le_ref_Lookup(mySms.ListRefMap, msgListRef);

   TAF_KILL_CLIENT_IF_RET_NIL(msgListPtr == NULL, "Invalid msgListPtr provided");

   le_ref_DeleteRef(mySms.ListRefMap, msgListRef);

   msgListPtr->tmpLink = NULL;
   ReInitializeList((le_dls_List_t*) &(msgListPtr->list));
   le_mem_Release(msgListPtr);
}

/*======================================================================

FUNCTION       taf_sms_Delete

DESCRIPTION    Delete message from message list, if no message list reference

DEPENDENCIES   Create message list

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   None

SIDE EFFECTS

======================================================================*/

void taf_sms_Delete
(
    taf_sms_MsgRef_t  msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_NIL(msgPtr == NULL, "Invalid msgPtr provided");

   TAF_KILL_CLIENT_IF_RET_NIL(msgPtr->inList, "Since the message is in a RX list, please call 'DeleteList' as alternative");

   (msgPtr->smsUserCount)--;

   SessionNode_t* sessionNode = mySms.GetSessionNodeFromMsgRef(msgRef);

   TAF_ERROR_IF_RET_NIL(sessionNode == NULL, "No sessionCtx found for msgRef");

   mySms.RemoveMsgRefFromSessionCtx(sessionNode, msgRef);

   if (msgPtr->smsUserCount == 0)
   {
      msgPtr->callBackPtr = NULL;

      le_mem_Release(msgPtr);

      LE_DEBUG("smsUserCount reaches 0, release the msgPtr");
   }

   // delete the session node, if it is not used anymore
   if ((le_dls_NumLinks(&(sessionNode->handlerList)) == 0) &&
            (le_dls_NumLinks(&(sessionNode->msgRefList)) == 0))
   {
      le_dls_Remove(&mySms.SessionList, &(sessionNode->link));
      le_mem_Release(sessionNode);
   }
}

/*======================================================================

FUNCTION       taf_sms_GetSenderTel

DESCRIPTION    Get sender phone number of message

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message
               [OUT] char* telPtr: string to copy phone number
               [IN] size_t len: input string length

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_NOT_PERMITTED: Invalid message type
                  LE_FAULT: Invalid input string
                  LE_OVERFLOW: String overflow
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_GetSenderTel
(
    taf_sms_MsgRef_t msgRef,
    char*            telPtr,
    size_t           len
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgPtr provided");

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr->type != TAF_SMS_RX, LE_NOT_PERMITTED, "Not RX message type, not permitted to get sender");

   TAF_KILL_CLIENT_IF_RET_VAL(telPtr == NULL, LE_FAULT, "telPtr is NULL");

   TAF_ERROR_IF_RET_VAL(strlen(msgPtr->tel) > (len - 1), LE_OVERFLOW, "Input len is smaller than sender tel length");

   le_utf8_Copy(telPtr, msgPtr->tel, len, NULL);

   return LE_OK;
}

/*======================================================================

FUNCTION       taf_sms_GetUserdataLen

DESCRIPTION    Get text length of message

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   size_t: data length

SIDE EFFECTS

======================================================================*/

size_t taf_sms_GetUserdataLen
(
    taf_sms_MsgRef_t msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, 0, "Invalid msgPtr provided");

   msgPtr->userdataLen = strnlen(msgPtr->text, TAF_SMS_TEXT_BYTES);

   return msgPtr->userdataLen;
}

/*======================================================================

FUNCTION       taf_sms_GetText

DESCRIPTION    Get text of message

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message
               [OUT] char* textPtr: string to copy text
               [IN] size_t len: input string length

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_FAULT: Invalid input string
                  LE_OVERFLOW: String overflow
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_GetText
(
    taf_sms_MsgRef_t  msgRef,
    char*            textPtr,
    size_t           len
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgPtr provided");

   TAF_KILL_CLIENT_IF_RET_VAL(textPtr == NULL, LE_FAULT, "textPtr is NULL");

   TAF_KILL_CLIENT_IF_RET_VAL(strlen(msgPtr->text) > (len - 1), LE_OVERFLOW, "Input len is smaller than text length");

   le_utf8_Copy(textPtr, msgPtr->text, len, NULL);

   return LE_OK;
}

/*======================================================================

FUNCTION       taf_sms_GetSendStatus

DESCRIPTION    Get send status of message

DEPENDENCIES   Create and send message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   taf_sms_SendStatus_t: send status

SIDE EFFECTS

======================================================================*/

taf_sms_SendStatus_t taf_sms_GetSendStatus
(
    taf_sms_MsgRef_t      msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_ERROR_IF_RET_VAL(msgPtr == NULL, TAF_SMS_SEND_STATUS_UNKNOWN, "msgPtr is NULL!");

   return msgPtr->sendStatus;
}

/*======================================================================

FUNCTION       taf_sms_GetReadStatus

DESCRIPTION    Get read status of message

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   taf_sms_ReadStatus_t: read status

SIDE EFFECTS

======================================================================*/

taf_sms_ReadStatus_t taf_sms_GetReadStatus
(
    taf_sms_MsgRef_t      msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_ERROR_IF_RET_VAL(msgPtr == NULL, TAF_SMS_READ_STATUS_UNKNOWN, "msgPtr is NULL!");

   return msgPtr->readStatus;
}

/*======================================================================

FUNCTION       taf_sms_GetType

DESCRIPTION    Get type of message

DEPENDENCIES   Access TX/RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   taf_sms_Type_t

SIDE EFFECTS

======================================================================*/

taf_sms_Type_t taf_sms_GetType
(
   taf_sms_MsgRef_t msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_ERROR_IF_RET_VAL(msgPtr == NULL, TAF_SMS_TYPE_UNKNOWN, "msgPtr is NULL!");

   return (msgPtr->type);
}

/*======================================================================

FUNCTION       taf_sms_Send

DESCRIPTION    Send message

DEPENDENCIES   Create new message and set sending paremeters

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_Send
(
    taf_sms_MsgRef_t    msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   le_result_t   result = LE_OK;
   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgPtr provided");

   if (msgPtr->phoneId < 1 || msgPtr->phoneId > 2)
   {
      msgPtr->phoneId = DEFAULT_SLOT_ID;
   }

   le_clk_Time_t timeToWait = {TIMEOUT_SEND_SEMAPHORE, 0};
   result = le_sem_WaitWithTimeOut(mySms.SmsSendSem, timeToWait);

   msgPtr->sendStatus = TAF_SMS_SENDING;
   mySms.sendingMsgRef = msgRef;
   mySms.sendMessage();

   return result;
}

/*======================================================================

FUNCTION       taf_sms_DeleteFromStorage

DESCRIPTION    Delete message from storage

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_DeleteFromStorage
(
    taf_sms_MsgRef_t msgRef   ///< [IN] The message to delete.
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgPtr provided");

   LE_INFO("taf_sms_DeleteFromStorage is not implemented\n");

   return LE_OK;
}

/*======================================================================

FUNCTION       MarkReadStatus_Read

DESCRIPTION    Mark message status as 'read'

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   None

SIDE EFFECTS   None

======================================================================*/

void MarkReadStatus_Read
(
    taf_sms_MsgRef_t msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_NIL(msgPtr == NULL, "Invalid msgPtr provided");

   msgPtr->readStatus = TAF_SMS_READ;
}

/*======================================================================

FUNCTION       MarkReadStatus_Unread

DESCRIPTION    Mark message status as 'unread'

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   None

SIDE EFFECTS   None

======================================================================*/

void MarkReadStatus_Unread
(
    taf_sms_MsgRef_t msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_NIL(msgPtr == NULL, "Invalid msgPtr provided");

   msgPtr->readStatus = TAF_SMS_UNREAD;
}

/*======================================================================

FUNCTION       COMPONENT_INIT

DESCRIPTION    The initialization of SMS component

DEPENDENCIES   None

PARAMETERS     None

RETURN VALUE   None

SIDE EFFECTS

======================================================================*/

COMPONENT_INIT
{
   LE_INFO("tafSms service Init...\n");
   auto &mySms = taf_Sms::GetInstance();
   mySms.Init();

   // install the handler
   taf_Handler myHandler;

   LE_INFO("tafSms service Ready...\n");
}