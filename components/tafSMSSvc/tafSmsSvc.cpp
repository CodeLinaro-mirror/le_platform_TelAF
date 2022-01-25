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

FUNCTION       taf_sms_CreateRxMsgList

DESCRIPTION    Create message list for accessing messages

DEPENDENCIES   Initialization of SMS service

PARAMETERS     None

RETURN VALUE   taf_sms_MsgListRef_t: new message list

SIDE EFFECTS

======================================================================*/

taf_sms_MsgListRef_t taf_sms_CreateRxMsgList
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
    taf_sms_MsgListRef_t        msgListRef
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

   msgPtr->type = TAF_SMS_TYPE_TX;
   msgPtr->sendStatus = TAF_SMS_TXSTS_UNSENT;

   msgPtr->userdataLen = 0;
   msgPtr->userCount = 1;
   msgPtr->inList = false;
   msgPtr->callBackPtr = NULL;
   msgPtr->ctxPtr = NULL;

   msgPtr->applyDel = false;
   msgPtr->pduReady = false;

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

   TAF_ERROR_IF_RET_VAL(msgPtr->type != TAF_SMS_TYPE_TX, LE_NOT_PERMITTED, "Not TX message");

   size_t length = strnlen(textPtr, TAF_SMS_TEXT_BYTES);

   TAF_KILL_CLIENT_IF_RET_VAL(length > (TAF_SMS_TEXT_BYTES-1), LE_FAULT, "strlen(text) > %d", (TAF_SMS_TEXT_BYTES-1));

   TAF_ERROR_IF_RET_VAL(length == 0, LE_BAD_PARAMETER, "Input string length = 0");

   msgPtr->format = TAF_SMS_FORMAT_TEXT;
   msgPtr->userdataLen = length;
   msgPtr->pduReady = false;

   LE_DEBUG("Copy text: %s, len: %zd for msgPtr.%p", textPtr, length, msgPtr);

   le_utf8_Copy(msgPtr->text, textPtr, sizeof(msgPtr->text), NULL);

   return LE_OK;
}

/*======================================================================

FUNCTION       taf_sms_SetBinary

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

le_result_t taf_sms_SetBinary
(
   taf_sms_MsgRef_t  msgRef,
   const uint8_t*    binPtr,
   size_t            len
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgRef provided");

   TAF_KILL_CLIENT_IF_RET_VAL(binPtr == NULL, LE_FAULT, "Invalid binPtr provided");

   TAF_ERROR_IF_RET_VAL(msgPtr->type != TAF_SMS_TYPE_TX, LE_NOT_PERMITTED, "Not TX message");

   TAF_KILL_CLIENT_IF_RET_VAL(len > TAF_SMS_BINARY_BYTES, LE_FAULT, "len > %d", TAF_SMS_BINARY_BYTES);

   TAF_ERROR_IF_RET_VAL(len == 0, LE_BAD_PARAMETER, "Input data length = 0");

   msgPtr->format = TAF_SMS_FORMAT_BINARY;
   msgPtr->userdataLen = len;
   msgPtr->pduReady = false;

   memcpy(msgPtr->binary, binPtr, len);

   LE_DEBUG("Copy bin len: %zd for msgPtr.%p", len, msgPtr);

   for(uint i = 0; i < len; i++)
   {
      LE_DEBUG("msgPtr->binary[%d] = 0x%.2X", i, msgPtr->binary[i]);
   }

   return LE_OK;
}

/*======================================================================

FUNCTION       taf_sms_SetUCS2

DESCRIPTION    Set UCS2 (16 bit format) for message

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

le_result_t taf_sms_SetUCS2
(
   taf_sms_MsgRef_t  msgRef,
   const uint16_t*   ucs2Ptr,
   size_t            numOfUcs2
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgRef provided");

   TAF_KILL_CLIENT_IF_RET_VAL(ucs2Ptr == NULL, LE_FAULT, "Invalid binPtr provided");

   TAF_ERROR_IF_RET_VAL(msgPtr->type != TAF_SMS_TYPE_TX, LE_NOT_PERMITTED, "Not TX message");

   TAF_KILL_CLIENT_IF_RET_VAL(numOfUcs2 > TAF_SMS_UCS2_CHARS, LE_FAULT, "num Of Ucs2 > %d", TAF_SMS_UCS2_CHARS);

   TAF_ERROR_IF_RET_VAL(numOfUcs2 == 0, LE_BAD_PARAMETER, "Input element number = 0");

   msgPtr->format = TAF_SMS_FORMAT_UCS2;
   msgPtr->userdataLen = numOfUcs2 * 2;
   msgPtr->pduReady = false;

   memcpy(msgPtr->binary, (uint8_t *) ucs2Ptr, msgPtr->userdataLen);

   LE_DEBUG("Copy ucs2 num: %zd for msgPtr.%p", numOfUcs2, msgPtr);

   for(uint i = 0; i < numOfUcs2; i +=2)
   {
      LE_DEBUG("msgPtr->binary[%d] = 0x%.2X%.2X", i, msgPtr->binary[i], msgPtr->binary[i + 1]);
   }

   return LE_OK;
}

/*======================================================================

FUNCTION       taf_sms_SetPDU

DESCRIPTION    Set PDU format for message

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

le_result_t taf_sms_SetPDU
(
   taf_sms_MsgRef_t  msgRef,
   const uint16_t*   pduPtr,
   size_t            len
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgRef provided");

   TAF_KILL_CLIENT_IF_RET_VAL(pduPtr == NULL, LE_FAULT, "Invalid binPtr provided");

   TAF_ERROR_IF_RET_VAL(msgPtr->type != TAF_SMS_TYPE_TX, LE_NOT_PERMITTED, "Not TX message");

   TAF_KILL_CLIENT_IF_RET_VAL(len > TAF_SMS_PDU_BYTES, LE_FAULT, "len > %d", TAF_SMS_PDU_BYTES);

   TAF_ERROR_IF_RET_VAL(len == 0, LE_BAD_PARAMETER, "Input PDU length = 0");

   msgPtr->format = TAF_SMS_FORMAT_PDU;
   msgPtr->pdu.length = len;
   msgPtr->pduReady = true;

   memcpy(msgPtr->pdu.data, pduPtr, len);

   LE_DEBUG("Copy pdu len: %zd for msgPtr.%p", len, msgPtr);

   for(uint i = 0; i < len; i++)
   {
      LE_DEBUG("msgPtr->pdu.data[%d] = 0x%.2X", i, msgPtr->pdu.data[i]);
   }

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

//--------------------------------------------------------------------------------------------------
/**
 * ReInitialize message list when deleting list, for internal usage
 */
//--------------------------------------------------------------------------------------------------

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
            (msgPtr->userCount)--;

            if (msgPtr->applyDel)
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

   (msgPtr->userCount)--;

   LE_DEBUG("userCount: %d", msgPtr->userCount);

   if ((msgPtr->applyDel) && (msgPtr->userCount == 0))
   {
      taf_sms_DeleteFromStorage(msgRef);

      msgPtr->callBackPtr = NULL;

      le_mem_Release(msgPtr);

      LE_DEBUG("userCount reaches 0, release the msgPtr");
   }

   SessionNode_t* sessionNode = mySms.GetSessionNodeFromMsgRef(msgRef);

   TAF_ERROR_IF_RET_NIL(sessionNode == NULL, "No sessionCtx found for msgRef");

   mySms.RemoveMsgRefFromSessionCtx(sessionNode, msgRef);

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

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr->type != TAF_SMS_TYPE_RX,
                              LE_NOT_PERMITTED,
                              "Not RX message type for msgRef(%p), not permitted to get sender", msgRef);

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

   size_t len = 0;

   switch (msgPtr->format)
   {
      case TAF_SMS_FORMAT_TEXT:
      case TAF_SMS_FORMAT_BINARY:
         len = msgPtr->userdataLen;
         break;
      case TAF_SMS_FORMAT_UCS2:
         len = (msgPtr->userdataLen / 2);
         break;
      default:
         return 0;
   }

   return len;
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
    char*             textPtr,
    size_t            len
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

FUNCTION       taf_sms_GetBinary

DESCRIPTION    Get binary data of message

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

le_result_t taf_sms_GetBinary
(
   taf_sms_MsgRef_t  msgRef,
   uint8_t*          binPtr,
   size_t*           lenPtr
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgPtr provided");

   TAF_KILL_CLIENT_IF_RET_VAL(binPtr == NULL, LE_FAULT, "binPtr is NULL");

   TAF_KILL_CLIENT_IF_RET_VAL(lenPtr == NULL, LE_FAULT, "lenPtr is NULL");

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr->format != TAF_SMS_FORMAT_BINARY, LE_FAULT, "Invalid format");

   if(msgPtr->userdataLen > *lenPtr)
   {
      memcpy(binPtr, msgPtr->binary, *lenPtr);
      LE_ERROR("Input len is smaller than binary length");

      return LE_OVERFLOW;
   }

   memcpy (binPtr, msgPtr->binary, msgPtr->userdataLen);
   *lenPtr = msgPtr->userdataLen;
   return LE_OK;
}

/*======================================================================

FUNCTION       taf_sms_GetUCS2

DESCRIPTION    Get UCS2 encoding of message

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

le_result_t taf_sms_GetUCS2
(
   taf_sms_MsgRef_t  msgRef,
   uint16_t*         ucs2Ptr,
   size_t*           ucs2LenPtr
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgPtr provided");

   TAF_KILL_CLIENT_IF_RET_VAL(ucs2Ptr == NULL, LE_FAULT, "ucs2Ptr is NULL");

   TAF_KILL_CLIENT_IF_RET_VAL(ucs2LenPtr == NULL, LE_FAULT, "ucs2LenPtr is NULL");

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr->format != TAF_SMS_FORMAT_UCS2, LE_FAULT, "Invalid format");

   if(msgPtr->userdataLen > (*ucs2LenPtr * 2))
   {
      memcpy(ucs2Ptr, msgPtr->binary, *ucs2LenPtr);
      LE_ERROR("Input len is smaller than binary length");

      return LE_OVERFLOW;
   }

   memcpy (ucs2Ptr, msgPtr->binary, msgPtr->userdataLen);
   *ucs2LenPtr = (msgPtr->userdataLen) / 2;
   return LE_OK;
}

/*======================================================================

FUNCTION       taf_sms_GetPDU

DESCRIPTION    Get PDU format of message

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

le_result_t taf_sms_GetPDU
(
   taf_sms_MsgRef_t  msgRef,
   uint8_t*          pduPtr,
   size_t*           lenPtr
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgPtr provided");

   TAF_KILL_CLIENT_IF_RET_VAL(pduPtr == NULL, LE_FAULT, "pduPtr is NULL");

   TAF_KILL_CLIENT_IF_RET_VAL(lenPtr == NULL, LE_FAULT, "lenPtr is NULL");

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr->type != TAF_SMS_TYPE_RX, LE_FAULT, "Invalid type (not RX msg)");

   if(msgPtr->pdu.length > *lenPtr)
   {
      memcpy(pduPtr, msgPtr->pdu.data, *lenPtr);
      LE_ERROR("Input len is smaller than binary length");

      return LE_OVERFLOW;
   }

   memcpy (pduPtr, msgPtr->pdu.data, msgPtr->pdu.length);
   *lenPtr = msgPtr->pdu.length;
   return LE_OK;
}

/*======================================================================

FUNCTION       le_sms_GetPDULen

DESCRIPTION    Get PDU length of message

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

size_t le_sms_GetPDULen
(
   taf_sms_MsgRef_t  msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgPtr provided");

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr->type != TAF_SMS_TYPE_RX, LE_FAULT, "Invalid type (not RX msg)");

   if (msgPtr->pduReady)
   {
      return (msgPtr->pdu.length);
   }

    return 0;
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

   TAF_ERROR_IF_RET_VAL(msgPtr == NULL, TAF_SMS_TXSTS_UNKNOWN, "msgPtr is NULL!");

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

   TAF_ERROR_IF_RET_VAL(msgPtr == NULL, TAF_SMS_RXSTS_UNKNOWN, "msgPtr is NULL!");

   return msgPtr->readStatus;
}

/*======================================================================

FUNCTION       taf_sms_GetType

DESCRIPTION    Get type of message

DEPENDENCIES   Access TX/RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   taf_sms_Type_t: message type

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

FUNCTION       taf_sms_GetFormat

DESCRIPTION    Get format of message

DEPENDENCIES   Access TX/RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   taf_sms_Format_t: message format

SIDE EFFECTS

======================================================================*/

taf_sms_Format_t taf_sms_GetFormat
(
   taf_sms_MsgRef_t msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_ERROR_IF_RET_VAL(msgPtr == NULL, TAF_SMS_FORMAT_UNKNOWN, "msgPtr is NULL!");

   return (msgPtr->format);
}

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
         encodeData.encoding = PDU_ENCODING_8_BITS;
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

   result = EncodeMsgToPdu(msgPtr);

   if (result == LE_OK)
   {
#ifdef TAF_SMS_SEND_FROM_TELSDK
      if (msgPtr->phoneId < 1 || msgPtr->phoneId > 2)
      {
         msgPtr->phoneId = DEFAULT_SLOT_ID;
      }

      le_clk_Time_t timeToWait = {TIMEOUT_SEND_SEMAPHORE, 0};
      result = le_sem_WaitWithTimeOut(mySms.SmsSendSem, timeToWait);
#endif
      msgPtr->sendStatus = TAF_SMS_TXSTS_SENDING;

#ifdef TAF_SMS_SEND_FROM_TELSDK
      mySms.sendingMsgRef = msgRef;
      mySms.sendMessage();
#else
      result = taf_pa_sms_SendPduMsg(msgPtr->pdu.length, msgPtr->pdu.data, TIMEOUT_SENDING_PDU);
#endif

   }
   else
   {
      LE_ERROR("Cannot encode Message Object %p", msgPtr);
      result = LE_FORMAT_ERROR;
   }

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

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr->storage == TAF_SMS_STORAGE_UNKNOWN, LE_NO_MEMORY, "Invalid storage");

   le_result_t   res = LE_OK;

   if (msgPtr->userCount == 1)
   {
      res = taf_pa_sms_DelMsgFromStorage(msgPtr->storage, msgPtr->storageIdx);
   }
   msgPtr->applyDel = true;

   return res;
}

/*======================================================================

FUNCTION       taf_sms_GetSmsCenterAddress

DESCRIPTION    Get SMS center address

DEPENDENCIES   Initialization of SMS service

PARAMETERS     [IN]  int8_t   phoneId: phone ID
               [OUT] char*    addr: to store SMS center address
               [IN]  size_t   len: expected max address length

RETURN VALUE   le_result_t
                  LE_OVERFLOW: expected len is not enough
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_GetSmsCenterAddress
(
   int8_t   phoneId,
   char*    addr,
   size_t   len
)
{
   auto &mySms = taf_Sms::GetInstance();
   auto smsManager = mySms.smsManagers[phoneId - 1];

   auto ret = smsManager->requestSmscAddress(mySms.getSmscCb);

   TAF_ERROR_IF_RET_VAL(ret != telux::common::Status::SUCCESS
                        , LE_FAULT, "Set SmscAddress request failed");

   le_clk_Time_t timeToWait = {TIMEOUT_GET_SMSC_SEMAPHORE, 0};
   le_result_t res = le_sem_WaitWithTimeOut(mySms.SmscGetSem, timeToWait);

   TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "SmscGetSem semaphore timeout");

   TAF_KILL_CLIENT_IF_RET_VAL(strlen(addr) > (len - 1), LE_OVERFLOW, "address length overflow");

   TAF_KILL_CLIENT_IF_RET_VAL(len > TAF_SMS_SMSC_ADDR_BYTES - 1, LE_OVERFLOW, "len is greater than TAF_SMS_SMSC_ADDR_LEN");

   le_utf8_Copy(addr, mySms.smscAddr, len, NULL);

   LE_DEBUG("returned smsc address: %s", addr);

   return LE_OK;
}

/*======================================================================

FUNCTION       taf_sms_SetSmsCenterAddress

DESCRIPTION    Set SMS center address

DEPENDENCIES   Initialization of SMS service

PARAMETERS     [IN] int8_t       phoneId: phone ID
               [IN] const char*  addr: to store SMS center address

RETURN VALUE   le_result_t
                  LE_FAULT: wait callback timeout or get error
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_SetSmsCenterAddress
(
   int8_t      phoneId,
   const char* addr
)
{
   auto &mySms = taf_Sms::GetInstance();
   auto smsManager = mySms.smsManagers[phoneId - 1];

   TAF_KILL_CLIENT_IF_RET_VAL(addr == NULL, LE_FAULT, "Invalid address provided");

   TAF_KILL_CLIENT_IF_RET_VAL(strlen(addr) > TAF_SMS_SMSC_ADDR_BYTES,
                              LE_FAULT,
                              "Invalid address provided");

   LE_DEBUG("set smsc address as %s", addr);

   auto ret = smsManager->setSmscAddress(addr, tafSetSmscAddressResponseCallback::setSmscResponse);

   le_clk_Time_t timeToWait = {TIMEOUT_SET_SMSC_SEMAPHORE, 0};
   le_result_t res = le_sem_WaitWithTimeOut(mySms.SmscSetSem, timeToWait);

   TAF_ERROR_IF_RET_VAL(res != LE_OK, res, "SmscSetSem semaphore timeout");

   if(ret == telux::common::Status::SUCCESS) {
      LE_INFO("Set SmscAddress request success\n");
      return LE_OK;
   }
   else {
      LE_INFO("Set SmscAddress request failed\n");
      return LE_FAULT;
   }
}

/*======================================================================

FUNCTION       taf_sms_Markread

DESCRIPTION    Mark message status as 'read'

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   None

SIDE EFFECTS   None

======================================================================*/

void taf_sms_Markread
(
    taf_sms_MsgRef_t msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_NIL(msgPtr == NULL, "Invalid msgPtr provided");

   msgPtr->readStatus = TAF_SMS_RXSTS_READ;

   taf_pa_sms_ModifyTag(msgPtr->storage, msgPtr->storageIdx, TAF_SMS_RXSTS_READ);
}

/*======================================================================

FUNCTION       taf_sms_MarkUnRead

DESCRIPTION    Mark message status as 'unread'

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   None

SIDE EFFECTS   None

======================================================================*/

void taf_sms_MarkUnRead
(
    taf_sms_MsgRef_t msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_NIL(msgPtr == NULL, "Invalid msgPtr provided");

   msgPtr->readStatus = TAF_SMS_RXSTS_UNREAD;

   taf_pa_sms_ModifyTag(msgPtr->storage, msgPtr->storageIdx, TAF_SMS_RXSTS_UNREAD);
}

/*======================================================================

FUNCTION       taf_sms_SendPduMsg

DESCRIPTION    Send PDU message

DEPENDENCIES   Initialization of SMS service

PARAMETERS     uint32_t             length: message length
               [IN] const uint8_t   dataPtr: data pointer
               uint32_t             timeout: timeout value in milli-second

RETURN VALUE   le_result_t

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_SendPduMsg
(
   const uint8_t*   dataPtr,
   size_t           dataSize,
   uint32_t         timeout
)
{
   le_result_t res = taf_pa_sms_SendPduMsg(dataSize, dataPtr, timeout);

   return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer storage handler, for internal usage
 */
//--------------------------------------------------------------------------------------------------

static void StorageHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
   taf_sms_FullStorageHandlerFunc_t clientHandlerFunc =
      (taf_sms_FullStorageHandlerFunc_t)secondLayerHandlerFunc;

   taf_sms_Storage_t storage = *(taf_sms_Storage_t *)reportPtr;

   clientHandlerFunc(storage, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * Storage memory full indication from PA layer, for internal usage
 */
//--------------------------------------------------------------------------------------------------

static void taf_pa_sms_storageFullInd
(
   taf_pa_sms_StorageInd_t* ind,
   void* contextPtr
)
{
   LE_INFO("ind->storage = %d memory is full", ind->storage);

   taf_sms_Storage_t storage = ind->storage;

   auto &mySms = taf_Sms::GetInstance();

   le_event_Report(mySms.StorageEvent, (void*)&storage, sizeof(taf_sms_Storage_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Get messaga content from new message indication
 */
//--------------------------------------------------------------------------------------------------

static void taf_pa_sms_getNewRxMsgInd
(
   taf_pa_sms_RxMsgInd_t* pduMsgRef,
   void* contextPtr
)
{
   LE_DEBUG("pduMsgRef->index = %u", pduMsgRef->index);
   LE_DEBUG("pduMsgRef->storage = %d", pduMsgRef->storage);

   auto &mySms = taf_Sms::GetInstance();

   taf_pa_sms_Pdu_t pduMsg = {0};

   le_result_t res = taf_pa_sms_ReadPDUMsgFromStorage(pduMsgRef->storage, pduMsgRef->index, &pduMsg);

   TAF_ERROR_IF_RET_NIL(res != LE_OK, "taf_pa_sms_ReadPDUMsgFromStorage failed, index[%d]", pduMsgRef->index);

   TAF_ERROR_IF_RET_NIL(pduMsg.length > TAF_SMS_PDU_BYTES, "PDU length (%u) out of range for index[%d]", pduMsg.length, pduMsgRef->index);

   sms_PduMsg_t decodedPduMsg;

   if (smsPdu_Decode(SMS_PROTOCOL_GSM,
                        pduMsg.data,
                        &decodedPduMsg) == LE_OK)
   {
      LE_DEBUG("decodedPduMsg.type: %d", decodedPduMsg.type);

      if (decodedPduMsg.type != SMS_TYPE_SUBMIT)
      {
         taf_sms_Msg_t* newMsg = mySms.CreateAndConstructMsg(&pduMsg, &decodedPduMsg);

         newMsg->readStatus = TAF_SMS_RXSTS_UNREAD;
         newMsg->type = TAF_SMS_TYPE_RX;

         mySms.NewSmsHandler(newMsg);
      }
   }
}

/*======================================================================

FUNCTION       taf_sms_SetPreferredStorage

DESCRIPTION    Set preferred SMS storage place

DEPENDENCIES   Initialization of SMS service

PARAMETERS     [IN] taf_sms_Storage_t: preferred storage place

RETURN VALUE   le_result_t

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_SetPreferredStorage
(
   taf_sms_Storage_t prefStorage
)
{
   auto &mySms = taf_Sms::GetInstance();

   if(prefStorage == TAF_SMS_STORAGE_NV)
   {
      LE_INFO("NV storage is not supported");
      return LE_UNSUPPORTED;
   }

   le_result_t res = taf_pa_sms_SetPrefStorage(prefStorage);

   if(res == LE_OK)
   {
      if(prefStorage == TAF_SMS_STORAGE_HLOS || prefStorage == TAF_SMS_STORAGE_SIM)
      {
         taf_pa_sms_SetRxMsgInd(true);

         mySms.qmiRxMsgHandler = taf_pa_sms_AddNewMsgHandler((taf_pa_sms_RxMsgHandlerFunc_t)&taf_pa_sms_getNewRxMsgInd, NULL);
      }
      else
      {
         taf_pa_sms_SetRxMsgInd(false);

         taf_pa_sms_RemoveRxMsgHandler(mySms.qmiRxMsgHandler);
      }
   }

   return res;
}

/*======================================================================

FUNCTION       taf_sms_GetPreferredStorage

DESCRIPTION    Get preferred SMS storage place

DEPENDENCIES   Initialization of SMS service

PARAMETERS     [OUT] taf_sms_Storage_t: present preferred storage place

RETURN VALUE   le_result_t

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_GetPreferredStorage
(
   taf_sms_Storage_t* prefStorage
)
{
   le_result_t res = taf_pa_sms_GetPrefStorage(prefStorage);

   return res;
}

/*======================================================================

FUNCTION       taf_sms_AddFullStorageEventHandler

DESCRIPTION    Register event handler for storage

DEPENDENCIES   Initialization of SMS service

PARAMETERS     [IN] taf_sms_FullStorageHandlerFunc_t handlerFuncPtr: handler function
               [IN] void*                            contextPtr:     context pointer

RETURN VALUE   taf_sms_FullStorageEventHandlerRef_t

SIDE EFFECTS

======================================================================*/

taf_sms_FullStorageEventHandlerRef_t taf_sms_AddFullStorageEventHandler
(
   taf_sms_FullStorageHandlerFunc_t handlerFuncPtr,
   void*                            contextPtr
)
{
   auto &mySms = taf_Sms::GetInstance();

   le_event_HandlerRef_t handlerRef;

   TAF_KILL_CLIENT_IF_RET_VAL(handlerFuncPtr == NULL, NULL, "handlerFuncPtr is NULL !");

   handlerRef = le_event_AddLayeredHandler("StorageInd",
                                           mySms.StorageEvent,
                                           StorageHandler,
                                           (void*)handlerFuncPtr);

   le_event_SetContextPtr(handlerRef, contextPtr);

   return (taf_sms_FullStorageEventHandlerRef_t)(handlerRef);
}

/*======================================================================

FUNCTION       taf_sms_RemoveFullStorageEventHandler

DESCRIPTION    Deregister event handler for storage

DEPENDENCIES   Register storage event handler

PARAMETERS     [IN] taf_sms_RxMsgHandlerRef_t handlerRef: Handler referenced

RETURN VALUE   None

SIDE EFFECTS

======================================================================*/

void taf_sms_RemoveFullStorageEventHandler
(
    taf_sms_FullStorageEventHandlerRef_t   handlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
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

   mySms.StorageEvent = le_event_CreateId("StorageEventId", sizeof(taf_sms_Storage_t));
   taf_pa_sms_AddStorageHandler((taf_pa_sms_StorageHandlerFunc_t)&taf_pa_sms_storageFullInd, NULL);

   // defualt set HLOS as preferred storage
   taf_sms_SetPreferredStorage(TAF_SMS_STORAGE_HLOS);

   LE_INFO("tafSms service Ready...\n");
}
