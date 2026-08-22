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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/*
 * @file       tafSMSSvc.cpp
 * @brief      This file provides the SMS service as interfaces described
 *             in tafSMSSvc.api, including sending message, receiving message from
 *             handler etc.
 */

#include "tafSms.hpp"

using namespace tafsvc;
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
   auto &sms = taf_Sms::GetInstance();
   return sms.CreateNewMsgList();
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
   auto &sms = taf_Sms::GetInstance();
   return sms.GetFirstMessage(msgListRef);
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
   msgPtr->phoneId = DEFAULT_PHONE_ID;

   msgPtr->type = TAF_SMS_TYPE_TX;
   msgPtr->sendStatus = TAF_SMS_TXSTS_UNSENT;

   msgPtr->userdataLen = 0;
   msgPtr->userCount = 1;
   msgPtr->inList = false;
   msgPtr->callBackPtr = NULL;
   msgPtr->ctxPtr = NULL;

   msgPtr->storage = TAF_SMS_STORAGE_NONE;
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

   TAF_ERROR_IF_RET_VAL(length > (TAF_SMS_TEXT_BYTES-1), LE_BAD_PARAMETER, "strlen(text) > %d", (TAF_SMS_TEXT_BYTES-1));

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

   for (size_t i = 0; i < numOfUcs2; i++)
   {
      msgPtr->binary[i * 2]     = (uint8_t)((ucs2Ptr[i] >> 8) & 0xFF);
      msgPtr->binary[i * 2 + 1] = (uint8_t)( ucs2Ptr[i]       & 0xFF);
      LE_DEBUG("msgPtr->binary[%zu] = 0x%.2X%.2X",
               i, msgPtr->binary[i * 2], msgPtr->binary[i * 2 + 1]);
   }

   LE_DEBUG("Copy ucs2 num: %zd for msgPtr.%p", numOfUcs2, msgPtr);

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
   taf_sms_MsgRef_t msgRef,
   const uint8_t*   pduPtr,
   size_t           len
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
      msgPtr->phoneId = DEFAULT_PHONE_ID;
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

FUNCTION       taf_sms_GetPhoneId

DESCRIPTION    Gets the message's phone ID
               (only for the messages stored in the SIM or created by the client).

DEPENDENCIES   Get RX message or create TX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_NOT_PERMITTED: Invalid message type
                  LE_BAD_PARAMETER: Invalid phone ID pointer
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_GetPhoneId
(
    taf_sms_MsgRef_t msgRef,
    uint8_t* phoneId
)
{
    auto &mySms = taf_Sms::GetInstance();

    taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

    TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgPtr provided");

    TAF_KILL_CLIENT_IF_RET_VAL(phoneId == NULL, LE_BAD_PARAMETER, "Invalid phoneId pointer");

    TAF_KILL_CLIENT_IF_RET_VAL(
                     (msgPtr->storage == TAF_SMS_STORAGE_HLOS),
                     LE_NOT_PERMITTED,
                     "Not permitted to get phone ID");

   *phoneId = msgPtr->phoneId;

   return LE_OK;
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

   size_t numOfUcs2 = msgPtr->userdataLen / 2;
   size_t copyCount = numOfUcs2;

   if (numOfUcs2 > *ucs2LenPtr)
   {
      LE_ERROR("Input buffer (%zu elements) is smaller than message length (%zu elements)",
                *ucs2LenPtr, numOfUcs2);
      copyCount = *ucs2LenPtr;
   }

   for (size_t i = 0; i < copyCount; i++)
   {
      ucs2Ptr[i] = ((uint16_t)msgPtr->binary[i * 2] << 8)
                 |  (uint16_t)msgPtr->binary[i * 2 + 1];
   }
   *ucs2LenPtr = copyCount;
   return (numOfUcs2 > copyCount) ? LE_OVERFLOW : LE_OK;
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

FUNCTION       taf_sms_GetPDULen

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

size_t taf_sms_GetPDULen
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

FUNCTION       taf_sms_GetLockStatus

DESCRIPTION    Get lock status of message

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   taf_sms_LockStatus_t: lock status

SIDE EFFECTS

======================================================================*/

taf_sms_LockStatus_t taf_sms_GetLockStatus
(
    taf_sms_MsgRef_t      msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_ERROR_IF_RET_VAL(msgPtr == nullptr, TAF_SMS_LKSTS_UNKNOWN, "msgPtr is NULL!");

   TAF_ERROR_IF_RET_VAL(msgPtr->storage == TAF_SMS_STORAGE_SIM,
      TAF_SMS_LKSTS_UNKNOWN, "Lock status not supported for SIM storage");

   return msgPtr->lockStatus;
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

/*======================================================================

FUNCTION       taf_sms_Send

DESCRIPTION    Send message

DEPENDENCIES   Create new message and set sending paremeters

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_BAD_PARAMETER: Invalid phone ID
                  LE_FORMAT_ERROR: Fail to encode message
                  LE_NO_MEMORY: Fail to allocate memory
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_Send
(
    taf_sms_MsgRef_t    msgRef
)
{
   auto &sms = taf_Sms::GetInstance();

   le_result_t result = LE_OK;
   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(sms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == nullptr, LE_NOT_FOUND, "msgPtr is nullptr!");

   msgPtr->sendStatus = TAF_SMS_TXSTS_SENDING;
   sms.sendingMsgRef = msgRef;

   result = sms.SendMessage(msgPtr);

   tafSmsSendStatus_t* msgSendStatusPtr = (tafSmsSendStatus_t*)le_mem_ForceAlloc(sms.SmsSendStatusPool);
   if (msgSendStatusPtr == nullptr) {
      LE_ERROR("Failed to allocate memory for send status");
      msgPtr->sendStatus = TAF_SMS_TXSTS_SENDING_FAILED;

      taf_sms_CallbackResultFunc_t functionPtr = (taf_sms_CallbackResultFunc_t)(msgPtr->callBackPtr);
      if (functionPtr) {
         functionPtr(msgRef, TAF_SMS_TXSTS_SENDING_FAILED, msgPtr->ctxPtr);
      }

      return LE_NO_MEMORY;
   }

   msgSendStatusPtr->msgRef = msgRef;

   msgSendStatusPtr->result =
        (result == LE_OK)
            ? PA_OK
            : PA_FAULT;

   le_event_ReportWithRefCounting(sms.MsgSendCallbackEvent, msgSendStatusPtr);

   return result;
}

/*======================================================================

FUNCTION       taf_sms_SendAsync

DESCRIPTION    Sends an asynchronous SMS message.

DEPENDENCIES   Create new message and set sending paremeters

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_BAD_PARAMETER: Invalid phone ID
                  LE_FORMAT_ERROR: Fail to encode message
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/
le_result_t taf_sms_SendAsync
(
   taf_sms_MsgRef_t              msgRef,
   taf_sms_CallbackResultFunc_t  handlerPtr,
   void* contextPtr
)
{
   auto &sms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(sms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgPtr provided");

   TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, LE_BAD_PARAMETER, "Invalid handlerPtr");

   msgPtr->callBackPtr = (void*)handlerPtr;

   msgPtr->ctxPtr = (void*)contextPtr;

   LE_DEBUG("Assign handler %p", handlerPtr);

   le_result_t result = sms.SendPDUMessageAsync(msgRef);
   return result;
}

/*======================================================================

FUNCTION       taf_sms_DeleteFromStorage

DESCRIPTION    Delete message from storage

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_BAD_PARAMETER: Invalid storage
                  LE_FAULT: Fail
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_DeleteFromStorage
(
    taf_sms_MsgRef_t msgRef   ///< [IN] The message to delete.
)
{
   auto &sms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(sms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr == nullptr, LE_NOT_FOUND, "Invalid msgPtr provided");

   TAF_KILL_CLIENT_IF_RET_VAL(msgPtr->storage == TAF_SMS_STORAGE_UNKNOWN, LE_BAD_PARAMETER,
                              "Invalid storage");

   le_result_t res = LE_OK;

   if (msgPtr->storage != TAF_SMS_STORAGE_NONE && msgPtr->userCount == 1)
   {
      if(msgPtr->storage == TAF_SMS_STORAGE_HLOS)
      {
         res = taf_sms_hlos_DelMsgFromStorage(msgPtr->storageIdx);
      }
      else
      {
         res = sms.DeleteMessage(msgPtr->storageIdx);
      }
   }
   msgPtr->applyDel = true;

   return res;
}

/*======================================================================

FUNCTION       taf_sms_DeleteAllFromStorage

DESCRIPTION    Delete all messages from storage

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_Storage_t storage: specific storage

RETURN VALUE   le_result_t
                  LE_BAD_PARAMETER: Invalid storage
                  LE_FAULT: Fail
                  LE_OK: Success

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_DeleteAllFromStorage
(
   taf_sms_Storage_t storage
)
{
   TAF_KILL_CLIENT_IF_RET_VAL(storage == TAF_SMS_STORAGE_UNKNOWN, LE_BAD_PARAMETER,
       "Invalid storage");

   auto &sms = taf_Sms::GetInstance();
   le_result_t res = LE_OK;
   if (storage != TAF_SMS_STORAGE_NONE)
   {
      if(storage == TAF_SMS_STORAGE_HLOS)
      {
         res = taf_sms_hlos_DelAllMsgFromStorage();
      }
      else
      {
         res = sms.DeleteAllMessages(storage);
      }
   }

   return res;
}

/*======================================================================

FUNCTION       taf_sms_GetSmsCenterAddress

DESCRIPTION    Get SMS center address

DEPENDENCIES   Initialization of SMS service

PARAMETERS     [IN]  uint8_t  phoneId: phone ID
               [OUT] char*    addr: to store SMS center address
               [IN]  size_t   len: expected max address length

RETURN VALUE   le_result_t
                  LE_OVERFLOW: Input buffer len is not enough
                  LE_FAULT: Internal error
                  LE_TIMEOUT: Timeout occurred
                  LE_BAD_PARAMETER: Invalid address pointer
                  LE_OK: Succeeded

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_GetSmsCenterAddress
(
   uint8_t  phoneId,
   char*    addr,
   size_t   len
)
{
   TAF_KILL_CLIENT_IF_RET_VAL(addr == NULL, LE_BAD_PARAMETER, "Invalid address pointer");

   pa_result_t paRes = taf_pa_sms_GetSmsCenterAddress(phoneId,
      addr, len, TIMEOUT_GET_SMSC);
   if (paRes != PA_OK)
   {
      LE_ERROR("taf_pa_sms_GetSmsCenterAddress failed, paRes: %d", (int)paRes);
      return LE_FAULT;
   }
   return LE_OK;
}

/*======================================================================

FUNCTION       taf_sms_SetSmsCenterAddress

DESCRIPTION    Set SMS center address

DEPENDENCIES   Initialization of SMS service

PARAMETERS     [IN] uint8_t      phoneId: phone ID
               [IN] const char*  addr: to store SMS center address

RETURN VALUE   le_result_t
                  LE_OVERFLOW: Input buffer len is not enough
                  LE_FAULT: Internal error
                  LE_TIMEOUT: Timeout occurred
                  LE_OK: Succeeded

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_SetSmsCenterAddress
(
   uint8_t     phoneId,
   const char* addr
)
{
   if (addr == nullptr)
   {
      return LE_BAD_PARAMETER;
   }

   pa_result_t paRes = taf_pa_sms_SetSmsCenterAddress(phoneId,
      addr, TIMEOUT_SET_SMSC);
   if (paRes != PA_OK)
   {
      LE_ERROR("taf_pa_sms_SetSmsCenterAddress failed, paRes: %d", (int)paRes);
      return LE_FAULT;
   }
   return LE_OK;
}

/*======================================================================

FUNCTION       taf_sms_MarkRead

DESCRIPTION    Mark message status as 'read'

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   None

SIDE EFFECTS   None

======================================================================*/

void taf_sms_MarkRead
(
    taf_sms_MsgRef_t msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_NIL(msgPtr == NULL, "Invalid msgPtr provided");

   le_result_t res = LE_OK;
   if(msgPtr->storage == TAF_SMS_STORAGE_HLOS)
   {
      res = taf_sms_hlos_setReadStatus(msgPtr->storageIdx, TAF_SMS_RXSTS_READ);
   }
   else
   {
      res = mySms.SetTag(msgPtr, taf_pa_sms_Tag::TAF_PA_READ);
   }
   if(res == LE_OK)
   {
      msgPtr->readStatus = TAF_SMS_RXSTS_READ;
   }
}

/*======================================================================

FUNCTION       taf_sms_MarkUnread

DESCRIPTION    Mark message status as 'unread'

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   None

SIDE EFFECTS   None

======================================================================*/

void taf_sms_MarkUnread
(
    taf_sms_MsgRef_t msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_KILL_CLIENT_IF_RET_NIL(msgPtr == NULL, "Invalid msgPtr provided");

   le_result_t res = LE_OK;
   if(msgPtr->storage == TAF_SMS_STORAGE_HLOS)
   {
      res = taf_sms_hlos_setReadStatus(msgPtr->storageIdx, TAF_SMS_RXSTS_UNREAD);
   }
   else
   {
      res = mySms.SetTag(msgPtr, taf_pa_sms_Tag::TAF_PA_NOT_READ);
   }
   if(res == LE_OK)
   {
      msgPtr->readStatus = TAF_SMS_RXSTS_UNREAD;
   }
}

/*======================================================================

FUNCTION       taf_sms_LockFromStorage

DESCRIPTION    Mark message lock status as 'locked', cannot be deleted from Storage

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_FAULT: Function failed
                  LE_OK: Function successful
                  LE_NOT_PERMITTED: Message have been locked or is not stored in HLOS

SIDE EFFECTS   None

======================================================================*/

le_result_t taf_sms_LockFromStorage
(
    taf_sms_MsgRef_t msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_ERROR_IF_RET_VAL(msgPtr == nullptr, LE_NOT_FOUND, "Invalid msgPtr provided");

   TAF_ERROR_IF_RET_VAL(msgPtr->storage == TAF_SMS_STORAGE_SIM,
      LE_NOT_PERMITTED, "Lock status not supported for SIM storage");

   le_result_t res = taf_sms_hlos_SetLockStatus(msgPtr->storage, msgPtr->storageIdx, TAF_SMS_LKSTS_LOCKED);

   if(res == LE_OK)
   {
      msgPtr->lockStatus = TAF_SMS_LKSTS_LOCKED;
   }

   return res;
}

/*======================================================================

FUNCTION       taf_sms_UnlockFromStorage

DESCRIPTION    Mark message lock status as 'unlocked', can be deleted from Storage

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_FAULT: Function failed
                  LE_OK: Function successful
                  LE_NOT_PERMITTED: Message is not locked or is not stored in HLOS

SIDE EFFECTS   None

======================================================================*/

le_result_t taf_sms_UnlockFromStorage
(
    taf_sms_MsgRef_t msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_ERROR_IF_RET_VAL(msgPtr == nullptr, LE_NOT_FOUND, "Invalid msgPtr provided");

   TAF_ERROR_IF_RET_VAL(msgPtr->storage == TAF_SMS_STORAGE_SIM,
      LE_NOT_PERMITTED, "Lock status not supported for SIM storage");

   le_result_t res = taf_sms_hlos_SetLockStatus(msgPtr->storage, msgPtr->storageIdx, TAF_SMS_LKSTS_UNLOCKED);

   if(res == LE_OK)
   {
      msgPtr->lockStatus = TAF_SMS_LKSTS_UNLOCKED;
   }

   return res;
}

/*======================================================================

FUNCTION       taf_sms_EncryptFromStorage

DESCRIPTION    Encrypt a message which is stored in HLOS

DEPENDENCIES   Get RX message

PARAMETERS     [IN] taf_sms_MsgRef_t msgRef: specific message

RETURN VALUE   le_result_t
                  LE_NOT_FOUND: Invalid message
                  LE_FAULT: Function failed
                  LE_OK: Function successful
                  LE_NOT_PERMITTED: Message is not stored in HLOS

SIDE EFFECTS   None

======================================================================*/

le_result_t taf_sms_EncryptFromStorage
(
    taf_sms_MsgRef_t msgRef
)
{
   auto &mySms = taf_Sms::GetInstance();

   taf_sms_Msg_t* msgPtr = (taf_sms_Msg_t*)le_ref_Lookup(mySms.MsgRefMap, msgRef);

   TAF_ERROR_IF_RET_VAL(msgPtr == NULL, LE_NOT_FOUND, "Invalid msgPtr provided");

   TAF_ERROR_IF_RET_VAL(msgPtr->storage != TAF_SMS_STORAGE_HLOS, LE_NOT_PERMITTED,
                        "Storage type %d is not supported", msgPtr->storage);

   le_result_t res = taf_sms_hlos_EncryptFromStorage(msgPtr->storage, msgPtr->storageIdx);

   if(res == LE_OK)
   {
      msgPtr->isEncrypted = true;
   }

   return res;
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
   auto &sms = taf_Sms::GetInstance();
   return sms.SendPDUMessageSync(const_cast<uint8_t*>(dataPtr), dataSize,
      timeout, DEFAULT_PHONE_ID);
}

/*======================================================================

FUNCTION       taf_sms_SendPduMsgEx

DESCRIPTION    Send PDU message

DEPENDENCIES   Initialization of SMS service

PARAMETERS     uint8_t              phoneId: phone ID from which message is going to send
               uint32_t             length: message length
               [IN] const uint8_t   dataPtr: data pointer
               uint32_t             timeout: timeout value in milli-second

RETURN VALUE   le_result_t

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_SendPduMsgEx
(
   uint8_t          phoneId,
   const uint8_t*   dataPtr,
   size_t           dataSize,
   uint32_t         timeout
)
{
   auto &sms = taf_Sms::GetInstance();
   return sms.SendPDUMessageSync(const_cast<uint8_t*>(dataPtr), dataSize,
      timeout, phoneId);
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

   taf_sms_StorageFullType_t fullType = *(taf_sms_StorageFullType_t *)reportPtr;

   clientHandlerFunc(fullType, le_event_GetContextPtr());
}

/*======================================================================

FUNCTION       taf_sms_SetPreferredStorage

DESCRIPTION    Set preferred SMS storage place

DEPENDENCIES   Initialization of SMS service

PARAMETERS     [IN] taf_sms_Storage_t: preferred storage place

RETURN VALUE   le_result_t
                  LE_FAULT: Internal error
                  LE_TIMEOUT: Timeout occurred
                  LE_UNSUPPORTED: Input storage is not suppported
                  LE_OK: Succeeded

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_SetPreferredStorage
(
   taf_sms_Storage_t prefStorage
)
{
   auto &mySms = taf_Sms::GetInstance();

   le_result_t res = mySms.SetPreferredStorage(prefStorage);

   return res;
}

/*======================================================================

FUNCTION       taf_sms_GetPreferredStorage

DESCRIPTION    Get preferred SMS storage place

DEPENDENCIES   Initialization of SMS service

PARAMETERS     [OUT] taf_sms_Storage_t: present preferred storage place

RETURN VALUE   le_result_t
                  LE_FAULT: Internal error
                  LE_TIMEOUT: Timeout occurred
                  LE_BAD_PARAMETER: Invalid prefStorage pointer
                  LE_OK: Succeeded

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_GetPreferredStorage
(
   taf_sms_Storage_t* prefStorage
)
{
   TAF_KILL_CLIENT_IF_RET_VAL(prefStorage == NULL, LE_BAD_PARAMETER,
                              "Invalid prefStorage pointer");

   auto &mySms = taf_Sms::GetInstance();

   le_result_t res = mySms.GetPreferredStorage(prefStorage);

   *prefStorage = mySms.sysPrefStorage;

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

FUNCTION       taf_sms_ActivateCellBroadcast

DESCRIPTION    Activation of configured broadcast messages for specified phone ID.

DEPENDENCIES   Cellbroadcast subsystem is ready

PARAMETERS     [IN] uint8_t       phoneId: phone ID

RETURN VALUE   le_result_t
                  LE_FAULT: Internal error
                  LE_TIMEOUT: Timeout occurred
                  LE_OK: Succceeded

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_ActivateCellBroadcast
(
   uint8_t phoneId
)
{
   auto &mySms = taf_Sms::GetInstance();
   return mySms.ActivateCellBroadcast(phoneId, true);
}

/*======================================================================

FUNCTION       taf_sms_DeactivateCellBroadcast

DESCRIPTION    Deactivation of configured broadcast messages for specified phone ID.

DEPENDENCIES   Cellbroadcast subsystem is ready

PARAMETERS     [IN] uint8_t       phoneId: phone ID

RETURN VALUE   le_result_t
                  LE_FAULT: Internal error
                  LE_TIMEOUT: Timeout occurred
                  LE_OK: Succceeded

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_DeactivateCellBroadcast
(
   uint8_t phoneId
)
{
   auto &mySms = taf_Sms::GetInstance();
   return mySms.ActivateCellBroadcast(phoneId, false);
}

/*======================================================================

FUNCTION       taf_sms_AddCellBroadcastIds

DESCRIPTION    Add cell broadcast message filter of identifier for specified phone ID.

DEPENDENCIES   Cellbroadcast subsystem is ready

PARAMETERS     [IN] uint8_t       phoneId: phone ID.
               [IN] uint16_t      fromId: Starting point of the filter.
               [IN] uint16_t      toId: Ending point of the filter.

RETURN VALUE   le_result_t
                  LE_BAD_PARAMETER: Invalid input
                  LE_TIMEOUT: Timeout occurred
                  LE_FAULT: Internal error
                  LE_OK: Succceeded

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_AddCellBroadcastIds
(
   uint8_t  phoneId,
   uint16_t fromId,
   uint16_t toId
)
{
   auto &mySms = taf_Sms::GetInstance();
   return mySms.AddCellBroadcastIds(phoneId, fromId, toId);
}

/*======================================================================

FUNCTION       taf_sms_RemoveCellBroadcastIds

DESCRIPTION    Add cell broadcast message filter of identifier for specified phone ID.

DEPENDENCIES   Cellbroadcast subsystem is ready

PARAMETERS     [IN] uint8_t       phoneId: phone ID.
               [IN] uint16_t      fromId: Starting point of the filter.
               [IN] uint16_t      toId: Ending point of the filter.

RETURN VALUE   le_result_t
                  LE_BAD_PARAMETER: Invalid input
                  LE_TIMEOUT: Timeout occurred
                  LE_FAULT: Internal error
                  LE_OK: Succceeded

SIDE EFFECTS

======================================================================*/

le_result_t taf_sms_RemoveCellBroadcastIds
(
   uint8_t  phoneId,
   uint16_t fromId,
   uint16_t toId
)
{
   auto &mySms = taf_Sms::GetInstance();
   return mySms.RemoveCellBroadcastIds(phoneId, fromId, toId);
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
   auto &mySms = taf_Sms::GetInstance();
   mySms.Init();

   // install the handler
   taf_Handler myHandler;

   mySms.StorageEvent = le_event_CreateId("StorageEvent", sizeof(taf_sms_StorageFullType_t));

   LE_INFO("tafSms service Ready...\n");
}
