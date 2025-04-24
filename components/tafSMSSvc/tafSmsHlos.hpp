/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_SMS_HLOS_HPP
#define TAF_SMS_HLOS_HPP

#include "interfaces.h"
#include "legato.h"
#include "tafSvcIF.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * tafSMSSvc allocates HLOS (high-level OS) storage to store incoming SMS
 * messages to file system. The maximum size of HLOS is defined as
 * MAX_OF_SMS_MSG_IN_HLOS. To avoid running out storage block new message
 * delivery, service provides recyling mechanism to keep free slots. However, if
 * user skips reading messages or locks too many messages, some messages cannot
 * be recycled or deleted. In this case, it still possible that service runs out
 * of the HLOS storage and cannot storage more message to file system, then it
 * will try to store message in SIM.
 *
 * The recycling mechnism is triggered when storage reaches 90% usage, it follow
 * rules
 * - detect if there is "recyclable message", which means the message is
 * unlocked and marked as 'read' status
 * - delete quantity of HLOS_RECYCLING_BUFFER "recyclable messages" to save
 * slots
 * - if HLOS storage is full, which means there is no recyclable message, new
 * delivered message will be stored to SIM storage
 */
//--------------------------------------------------------------------------------------------------

#define MAX_OF_SMS_MSG_IN_HLOS 255
#define HLOS_ALERT_PERCENTAGE 80
#define HLOS_ALERT_THRESHOLD \
    (MAX_OF_SMS_MSG_IN_HLOS * HLOS_ALERT_PERCENTAGE / 100)
#define HLOS_RECYCLING_PERCENTAGE 90
#define HLOS_RECYCLING_THRESHOLD \
    (MAX_OF_SMS_MSG_IN_HLOS * HLOS_RECYCLING_PERCENTAGE / 100)
#define HLOS_RECYCLING_BUFFER 10

//--------------------------------------------------------------------------------------------------
/**
 * tafSMSSvc defines HLOS SMS file as the following format:
 * Byte 0: magic number, forced to define as 0xF1 here
 * Byte 1: SMS format version
 * Byte 2: message configuration
 * Byte 2 - bit 0: read status, 0 - unread, 1 - read
 * Byte 2 - bit 1: lock status, 0 - unlocked, 1 - locked
 * Byte 2 - bit 2: encrypt status, 0 - not encrypted, 1 - encrypted
 * Byte 2 - bit 3 to bit 7: reserved
 * Byte 3: reserved
 * Byte 4 to byte 7: CRC of byte 0 to byte 3
 * After byte 7: payload, which is ecrypted PDU-format data
 * Following AES-256, the payload length is multiple of 16 bytes.
 *
 * The following macros are defined to indicate the indexes and lenght for each
 * part of message file
 */
//--------------------------------------------------------------------------------------------------

#define HLOS_SMS_HEADER_INDEX_MAGIC 0
#define HLOS_SMS_HEADER_INDEX_VERSION 1
#define HLOS_SMS_HEADER_INDEX_STATUS 2
#define HLOS_SMS_HEADER_INDEX_RESERVE 3
#define HLOS_SMS_HEADER_INDEX_CRC \
    (HLOS_SMS_HEADER_INDEX_MAGIC + HLOS_SMS_CONFIG_LEN)

#define HLOS_SMS_CONFIG_LEN 4
#define HLOS_SMS_CRC_LEN 4
#define HLOS_SMS_HEADER_LEN (HLOS_SMS_CONFIG_LEN + HLOS_SMS_CRC_LEN)
#define HLOS_SMS_PAYLOAD_INDEX \
    (HLOS_SMS_HEADER_INDEX_MAGIC + HLOS_SMS_HEADER_LEN)

#define HLOS_SMS_HEADER_MAGIC_NUM 0xF1
#define HLOS_SMS_HEADER_DEFAULT_VER 0x00
#define HLOS_SMS_HEADER_DEFAULT_STS 0x00

#define HLOS_SMS_READSTS_MASK 0x01
#define HLOS_SMS_LOCKSTS_MASK 0x02
#define HLOS_SMS_ENCRYPTSTS_MASK 0x04

#define HLOS_PATH_MAX_BYTE 100
#define SMS_STORAGE_PATH "/taf_sms"
#define SMS_STORAGE_ENCRYPTED_SUFFIX "_encrypted"
#define SMS_HLOS_KEY_ID "SmsHlosStorageKey"

//--------------------------------------------------------------------------------------------------
/**
 * To follow AES-256 the HLOS file payload length is multiple of 16
 */
//--------------------------------------------------------------------------------------------------
#define SMS_AES_PKCS7_ENCRYPTED_LEN ((TAF_SMS_PDU_BYTES / 16 + 1) * 16)

#define SMS_MAX_PAYLOAD_LEN SMS_AES_PKCS7_ENCRYPTED_LEN
#define SMS_MAX_FILE_LEN (HLOS_SMS_HEADER_LEN + SMS_MAX_PAYLOAD_LEN)

class taf_sms_hlos
{
public:
    static taf_sms_hlos& GetInstance();
    taf_sms_Storage_t sysPrefStorage;
};

//--------------------------------------------------------------------------------------------------
/**
 * Types of encrypt status
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_SMS_HLOS_ENCRYPT_STS_ENCRYPTED = 0,
    TAF_SMS_HLOS_ENCRYPT_STS_UNENCRYPTED = 1,
    TAF_SMS_HLOS_ENCRYPT_STS_UNKNOWN = 2
} taf_sms_hlos_encryptStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * PDU msg structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t index;
    uint8_t phoneId;
    taf_sms_Storage_t storage;
    taf_sms_ReadStatus_t rxStatus;
    taf_sms_LockStatus_t lkStatus;
    uint8_t data[TAF_SMS_PDU_BYTES];
    uint32_t length;
} taf_sms_Pdu_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler function to report storage memory status
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_sms_StorageFullType_t fullType;
} taf_sms_hlos_StorageInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Set the preferred SMS storage place
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t
taf_sms_hlos_SetPrefStorage(taf_sms_Storage_t prefStorage);

//--------------------------------------------------------------------------------------------------
/**
 * Set message lock status to locked/unlocked
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_sms_hlos_SetLockStatus(taf_sms_Storage_t storage,
    uint32_t index,
    taf_sms_LockStatus_t lkStatus);

//--------------------------------------------------------------------------------------------------
/**
 * Get message Lock status
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_sms_LockStatus_t taf_sms_hlos_GetLockStatus(
    taf_sms_Storage_t storage, uint32_t index, uint8_t phoneId);

//--------------------------------------------------------------------------------------------------
/**
 * Store new message to HLOS storage
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_sms_hlos_StoreNewMsgToHLOS(void* newMsg);

//--------------------------------------------------------------------------------------------------
/**
 * Encrypt the message in HLOS storage
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_sms_hlos_EncryptFromStorage(taf_sms_Storage_t storage,
    uint32_t index);

//--------------------------------------------------------------------------------------------------
/**
 * Store delivered message to HLOS storage
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_sms_hlos_storeRxMsg(uint8_t pduData[TAF_SMS_PDU_BYTES],
    uint32_t dataLen, uint32_t* index);

//--------------------------------------------------------------------------------------------------
/**
 * Enter HLOS recyling mechanism
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_sms_hlos_recycling();

//--------------------------------------------------------------------------------------------------
/**
 * Encrypt message
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_sms_hlos_encryptMsg(uint8_t* msgData, size_t dataLen,
    uint8_t* encryptedData,
    size_t* encryptedDataSize);

//--------------------------------------------------------------------------------------------------
/**
 * Decrypt message
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_sms_hlos_decryptMsg(uint8_t* cypherData, size_t cypherDataLen,
    uint8_t* decryptedData,
    size_t* decryptedDataSize);

//--------------------------------------------------------------------------------------------------
/**
 * List HLOS message indexes
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_sms_hlos_ListMsgFromStorage(taf_sms_ReadStatus_t rxStatus,
    uint32_t* numOfIdx,
    uint32_t* idxArray);

//--------------------------------------------------------------------------------------------------
/**
 * Read HLOS message with specified index
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_sms_hlos_ReadPDUMsgFromStorage(uint32_t index,
    taf_sms_Pdu_t* pduMsg);

//--------------------------------------------------------------------------------------------------
/**
 * Delete HLOS message with specified index
 * If message is locked, cannot be deleted, will return LE_BUSY
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_sms_hlos_DelMsgFromStorage(uint32_t index);

//--------------------------------------------------------------------------------------------------
/**
 * Delete all unlocked HLOS messages
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_sms_hlos_DelAllMsgFromStorage();

//--------------------------------------------------------------------------------------------------
/**
 * Set read status of HLOS message with specified index
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_sms_hlos_setReadStatus(uint32_t index,
    taf_sms_ReadStatus_t rxStatus);

//--------------------------------------------------------------------------------------------------
/**
 * Get read status of HLOS message with specified index
 */
//--------------------------------------------------------------------------------------------------
taf_sms_ReadStatus_t taf_sms_hlos_getReadStatus(uint32_t index);

//--------------------------------------------------------------------------------------------------
/**
 * Set lock status of HLOS message with specified index
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_sms_hlos_setLockStatus(uint32_t index,
    taf_sms_LockStatus_t lockStatus);

//--------------------------------------------------------------------------------------------------
/**
 * Get lock status of HLOS message with specified index
 */
//--------------------------------------------------------------------------------------------------
taf_sms_LockStatus_t taf_sms_hlos_getLockStatus(uint32_t index);

//--------------------------------------------------------------------------------------------------
/**
 * Set encrypt status of HLOS message with specified index
 */
//--------------------------------------------------------------------------------------------------
le_result_t
taf_sms_hlos_setEncryptSts(uint32_t index,
    taf_sms_hlos_encryptStatus_t encryptStatus);

//--------------------------------------------------------------------------------------------------
/**
 * Get encrypt status of HLOS message with specified index
 */
//--------------------------------------------------------------------------------------------------
taf_sms_hlos_encryptStatus_t taf_sms_hlos_getEncryptSts(uint32_t index);

//--------------------------------------------------------------------------------------------------
/**
 * Add header to new deliverd message
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_sms_hlos_createNewHeader(char* dataPtr, size_t dataLen,
    bool encrypted);

//--------------------------------------------------------------------------------------------------
/**
 * Encrypt the message in HLOS storage
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_sms_hlos_EncryptFromStorage(uint32_t index);

#endif // TAF_SMS_HLOS_HPP
