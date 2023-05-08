/*
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
#include "taf_pa_sms.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Set the preferred SMS storage place
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_SetPrefStorage
(
    taf_sms_Storage_t prefStorage  ///< [IN] The preferred SMS storage area
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the preferred SMS storage place
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_GetPrefStorage
(
    taf_sms_Storage_t* prefStoragePtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a message in PDU mode
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_SendPduMsg
(
    uint32_t                 length,
    const uint8_t*           dataPtr,
    uint32_t                 timeout
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * List indexes of messages stored in the preferred storage
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_ListMsgFromStorage
(
    taf_sms_Storage_t       storage,
    taf_sms_ReadStatus_t    rxStatus,
    uint32_t                *numOfIdx,
    uint32_t                *idxArray
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read message from the preferred storage
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_ReadPDUMsgFromStorage
(
    taf_sms_Storage_t   storage,
    uint32_t            index,
    taf_pa_sms_Pdu_t*   msgPtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable/disable indication for new RX message
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_SetRxMsgInd
(
    bool enableRxInd
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Modify message RX status to read/unread
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_SetReadStatus
(
    taf_sms_Storage_t    storage,
    uint32_t                index,
    taf_sms_ReadStatus_t    rxStatus
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set message lock status to locked/unlocked
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_SetLockStatus
(
    taf_sms_Storage_t       storage,
    uint32_t                index,
    taf_sms_LockStatus_t    lkStatus
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get message read status
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_sms_ReadStatus_t taf_pa_sms_GetReadStatus
(
    taf_sms_Storage_t       storage,
    uint32_t                index
)
{
    return TAF_SMS_RXSTS_UNREAD;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get message lock status
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_sms_LockStatus_t taf_pa_sms_GetLockStatus
(
    taf_sms_Storage_t       storage,
    uint32_t                index
)
{
    return TAF_SMS_LKSTS_UNLOCKED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete message from preferred storage
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_DelMsgFromStorage
(
    taf_sms_Storage_t       storage,
    uint32_t                index
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete all messages from preferred storage
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_DelAllMsgFromStorage
(
    taf_sms_Storage_t       storage
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register memory full indication
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_RegisterMemFullInd
(
    bool enable
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for new RX message
 *
 * @return taf_pa_sms_RxMsgHandlerRef_t handler function reference
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_pa_sms_RxMsgHandlerRef_t taf_pa_sms_AddNewMsgHandler
(
    taf_pa_sms_RxMsgHandlerFunc_t  rxMsghandler,
    void*                          contextPtr
)
{
    return (taf_pa_sms_RxMsgHandlerRef_t)NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for new RX message
 *
 * @return void
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_sms_RemoveRxMsgHandler
(
    taf_pa_sms_RxMsgHandlerRef_t    handlerRef
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function to get status of storage
 *
 * @return taf_pa_sms_StorageHandlerRef_t handler function reference
 */
//--------------------------------------------------------------------------------------------------

LE_SHARED taf_pa_sms_StorageHandlerRef_t taf_pa_sms_AddStorageHandler
(
    taf_pa_sms_StorageHandlerFunc_t  storageHandler,
    void*                            contextPtr
)
{
    return (taf_pa_sms_StorageHandlerRef_t)NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function to get status of storage
 *
 * @return void
 */
//--------------------------------------------------------------------------------------------------

LE_SHARED void taf_pa_sms_RemoveStorageHandler
(
    taf_pa_sms_StorageHandlerRef_t    handlerRef
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Store new message to HLOS storage
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_sms_StoreNewMsgToHLOS
(
    void* newMsg
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Init this component
 */
//--------------------------------------------------------------------------------------------------

COMPONENT_INIT
{
    LE_INFO("taf_pa_sms stub");
}
