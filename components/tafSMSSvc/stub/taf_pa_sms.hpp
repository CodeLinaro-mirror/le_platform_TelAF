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

#ifndef TAF_PA_SMS_HPP
#define TAF_PA_SMS_HPP

#include "tafSvcIF.hpp"

#define QMI_TIMEOUT_MS 1000
#define MAXIMUM_RETRY 10


typedef enum
{
    TAF_PA_SMS_PROTOCOL_UNKNOWN = 0,
    TAF_PA_SMS_PROTOCOL_GSM     = 1,
    TAF_PA_SMS_PROTOCOL_CDMA    = 2,
    TAF_PA_SMS_PROTOCOL_GW_CB   = 3
}
taf_pa_sms_Protocol_t;

//--------------------------------------------------------------------------------------------------
/**
 * PDU msg structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint32_t                index;
    taf_sms_Storage_t       storage;
    taf_sms_ReadStatus_t    rxStatus;
    taf_sms_LockStatus_t    lkStatus;
    uint8_t                 data[TAF_SMS_PDU_BYTES];
    uint32_t                length;
}
taf_pa_sms_Pdu_t;

//--------------------------------------------------------------------------------------------------
/**
 * RX msg indication structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint32_t                index;
    taf_pa_sms_Protocol_t   protocol;
    taf_sms_Storage_t       storage;
    uint8_t                 pduLen;
}
taf_pa_sms_RxMsgInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Set the preferred SMS storage place
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_SetPrefStorage
(
    taf_sms_Storage_t prefStorage
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the preferred SMS storage place
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_GetPrefStorage
(
    taf_sms_Storage_t* prefStoragePtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Send a message in PDU mode
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_SendPduMsg
(
    uint32_t                 length,
    const uint8_t*           dataPtr,
    uint32_t                 timeout
);

//--------------------------------------------------------------------------------------------------
/**
 * List indexes of messages stored in the preferred storage
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_ListMsgFromStorage
(
    taf_sms_Storage_t    storage,
    taf_sms_ReadStatus_t    rxStatus,
    uint32_t                *numOfIdx,
    uint32_t                *idxArray
);

//--------------------------------------------------------------------------------------------------
/**
 * Read message from the preferred storage
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_ReadPDUMsgFromStorage
(
    taf_sms_Storage_t    storage,
    uint32_t             index,
    taf_pa_sms_Pdu_t*    msgPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Enable/disable indication for new RX message
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_SetRxMsgInd
(
    bool enableRxInd
);

//--------------------------------------------------------------------------------------------------
/**
 * Write a new message given in its raw format
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_WriteRawMsg
(
    taf_sms_Storage_t    storage,
    uint32_t             length,
    const uint8_t*       dataPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Set message RX status to read/unread
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_SetReadStatus
(
    taf_sms_Storage_t       storage,
    uint32_t                index,
    taf_sms_ReadStatus_t    rxStatus
);

//--------------------------------------------------------------------------------------------------
/**
 * Set message lock status to locked/unlocked
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_SetLockStatus
(
    taf_sms_Storage_t       storage,
    uint32_t                index,
    taf_sms_LockStatus_t    lkStatus
);

//--------------------------------------------------------------------------------------------------
/**
 * Get message RX status
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_sms_ReadStatus_t taf_pa_sms_GetReadStatus
(
    taf_sms_Storage_t       storage,
    uint32_t                index
);

//--------------------------------------------------------------------------------------------------
/**
 * Get message Lock status
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_sms_LockStatus_t taf_pa_sms_GetLockStatus
(
    taf_sms_Storage_t       storage,
    uint32_t                index
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete message from preferred storage
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_DelMsgFromStorage
(
    taf_sms_Storage_t       storage,
    uint32_t                index
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete all messages from preferred storage
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_DelAllMsgFromStorage
(
    taf_sms_Storage_t       storage
);

//--------------------------------------------------------------------------------------------------
/**
 * Register memory full indication
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_sms_RegisterMemFullInd
(
    bool enable
);

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler function to report new message has arrived
 */
//--------------------------------------------------------------------------------------------------
typedef void (*taf_pa_sms_RxMsgHandlerFunc_t)
(
    taf_pa_sms_RxMsgInd_t* pduMsgRef, void* contextPtr
);

typedef struct taf_pa_sms_RxMsgHandler* taf_pa_sms_RxMsgHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for new RX message
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_pa_sms_RxMsgHandlerRef_t taf_pa_sms_AddNewMsgHandler
(
    taf_pa_sms_RxMsgHandlerFunc_t  rxMsghandler,
    void*                          contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for new RX message
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_sms_RemoveRxMsgHandler
(
    taf_pa_sms_RxMsgHandlerRef_t  handlerRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler function to report storage memory status
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    taf_sms_StorageFullType_t  fullType;
}
taf_pa_sms_StorageInd_t;

typedef void (*taf_pa_sms_StorageHandlerFunc_t)
(
    taf_pa_sms_StorageInd_t* storageMsgInd, void* contextPtr
);

typedef struct taf_pa_sms_StorageHandler* taf_pa_sms_StorageHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function to get status of storage
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_pa_sms_StorageHandlerRef_t taf_pa_sms_AddStorageHandler
(
    taf_pa_sms_StorageHandlerFunc_t storageHandler,
    void*                          contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function to get status of storage
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_sms_RemoveStorageHandler
(
    taf_pa_sms_StorageHandlerRef_t  handlerRef
);

class taf_pa_sms{
    public:
    static taf_pa_sms &GetInstance();
    taf_pa_sms() {};
    ~taf_pa_sms() {};
};

#endif /* TAF_PA_SMS_H_ */
