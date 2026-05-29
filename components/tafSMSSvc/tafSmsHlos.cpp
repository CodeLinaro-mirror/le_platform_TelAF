/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string.h>

#include "interfaces.h"
#include "legato.h"
#include "tafSmsHlos.hpp"
#include "tafSms.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Set the preferred SMS storage place
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_sms_hlos_SetPrefStorage(
    taf_sms_Storage_t prefStorage ///< [IN] The preferred SMS storage area
)
{
    auto& sms = taf_sms_hlos::GetInstance();
    sms.sysPrefStorage = prefStorage;

    LE_INFO("taf_sms_hlos_SetPrefStorage successfully");

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
LE_SHARED le_result_t
taf_sms_hlos_SetLockStatus(taf_sms_Storage_t storage,
    uint32_t index,
    taf_sms_LockStatus_t lkStatus)
{
    if (storage == TAF_SMS_STORAGE_HLOS)
    {
        return taf_sms_hlos_setLockStatus(index, lkStatus);
    }
    else
    {
        LE_ERROR("Storage type %d is doesn't support lock status", storage);
    }
    return LE_NOT_PERMITTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get message lock status
 *
 * @return LE_FAULT         The function failed, please refer to QMI error code
 * @return LE_OK            The function succeeded
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_sms_LockStatus_t
taf_sms_hlos_GetLockStatus(taf_sms_Storage_t storage, uint32_t index)
{
    if (storage == TAF_SMS_STORAGE_HLOS)
    {
        return taf_sms_hlos_getLockStatus(index);
    }
    else
    {
        LE_ERROR("Storage type %d is doesn't support lock status", storage);
    }
    return TAF_SMS_LKSTS_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Store new message to HLOS storage
 *
 * If preferred storage is set as TAF_SMS_STORAGE_HLOS, telaf
 * needs to read PDU message info then delete it from SIM storage,
 * since message content is not included in new SMS indication.
 *
 * @return void
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_sms_hlos_StoreNewMsgToHLOS(void* newMsg)
{
    LE_DEBUG("taf_sms_hlos_StoreNewMsgToHLOS");

        taf_sms_Pdu_t* pduPtr = (taf_sms_Pdu_t*)newMsg;

    uint32_t occupiedSlot = 0;
    le_result_t storeRes = taf_sms_hlos_storeRxMsg(pduPtr->data, pduPtr->length,
        &pduPtr->index, &occupiedSlot);
    LE_DEBUG("storeRes: %d, index: %u, occupiedSlot: %u", storeRes, pduPtr->index, occupiedSlot);

    if (storeRes == LE_FAULT)
    {
        LE_ERROR("Failed to store new SMS to HLOS, res: %d", storeRes);
        return LE_FAULT;
    }

        taf_sms_hlos_StorageInd_t storageInd;
        storageInd.fullType = TAF_SMS_FULL_UNKNOWN;

    if (storeRes == LE_NO_MEMORY || occupiedSlot >= MAX_OF_SMS_MSG_IN_HLOS)
        {
            LE_INFO("TAF_SMS_FULL_HLOS");
            storageInd.fullType = TAF_SMS_FULL_HLOS;
        }
    else if (occupiedSlot >= HLOS_ALERT_THRESHOLD)
        {
            LE_INFO("TAF_SMS_FULL_HLOS_ALERT");
            storageInd.fullType = TAF_SMS_FULL_HLOS_ALERT;
        }

        if (storageInd.fullType != TAF_SMS_FULL_UNKNOWN)
        {
            auto &smsInstance = tafsvc::taf_Sms::GetInstance();
            le_event_Report(smsInstance.StorageEvent, (void*)&storageInd, sizeof(storageInd));
        }

    return storeRes;
}

//--------------------------------------------------------------------------------------------------
/**
 * Encrypt the message in HLOS storage
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_sms_hlos_EncryptFromStorage(taf_sms_Storage_t storage,
    uint32_t index)
{
    if (storage == TAF_SMS_STORAGE_HLOS)
    {
        return taf_sms_hlos_EncryptFromStorage(index);
    }
    return LE_NOT_PERMITTED;
}

taf_sms_hlos& taf_sms_hlos::GetInstance()
{
    static taf_sms_hlos instance;
    return instance;
}

le_result_t taf_sms_hlos_storeRxMsg(uint8_t pduData[TAF_SMS_PDU_BYTES],
    uint32_t dataLen,
    uint32_t* index,
    uint32_t* occupiedSlot)
{
    TAF_ERROR_IF_RET_VAL(dataLen > TAF_SMS_PDU_BYTES, LE_FAULT,
        "dataLen(%u) overflow", dataLen);
    TAF_ERROR_IF_RET_VAL(index == NULL || occupiedSlot == NULL, LE_FAULT,
        "invalid null parameter");

    for (uint i = 0; i < dataLen; i++)
    {
        LE_DEBUG("pduData: 0x%.2X", pduData[i]);
    }

    bool findSlot = false;
    *index = 0;
    *occupiedSlot = 0;
    le_result_t res = LE_OK;

    char smsFileStr[SMS_MAX_FILE_LEN * 2 + 1] = { 0 };
    char* payloadPtr = smsFileStr + (HLOS_SMS_HEADER_LEN * 2);
    size_t payloadLen = ((SMS_MAX_FILE_LEN - HLOS_SMS_HEADER_LEN) * 2) + 1;

    taf_sms_hlos_createNewHeader(smsFileStr, (HLOS_SMS_HEADER_LEN * 2) + 1,
        false);
    le_hex_BinaryToString(pduData, dataLen, payloadPtr, payloadLen);

    dataLen += HLOS_SMS_HEADER_LEN;

    LE_DEBUG("smsFileStr: %s", smsFileStr);

    le_fs_FileRef_t fileRef;

    for (uint32_t i = 0; i < MAX_OF_SMS_MSG_IN_HLOS; i++)
    {
        char smsPath[HLOS_PATH_MAX_BYTE];

        snprintf(smsPath, sizeof(smsPath), "%s/%d", SMS_STORAGE_PATH, i);

        if (le_fs_Exists(smsPath))
        {
            (*occupiedSlot)++;
        }
        else if (findSlot == false)
        {
            res = le_fs_Open(smsPath, LE_FS_CREAT | LE_FS_WRONLY, &fileRef);

            if (res != LE_OK)
            {
                LE_ERROR("Fail to open state file, slot[%u], res=%d, try next.", i, res);
                continue;
            }

            size_t size = ((size_t)dataLen) * sizeof(char) * 2;
            res = le_fs_Write(fileRef, (uint8_t*)smsFileStr, size);
            le_fs_Close(fileRef);

            if (res != LE_OK)
            {
                LE_ERROR("Fail to write state, slot[%u], res=%d, try next.", i, res);
                le_fs_Delete(smsPath);
                continue;
            }

                LE_DEBUG("Write to slot [%u]", i);

                *index = i;
                findSlot = true;
            (*occupiedSlot)++;

        }
    }

    LE_DEBUG("occupied slots = %d", *occupiedSlot);

    if (findSlot == false)
    {
        LE_ERROR("No available slot in HLOS storage");
        return LE_NO_MEMORY;
    }

    if (*occupiedSlot >= HLOS_RECYCLING_THRESHOLD)
    {
        LE_DEBUG("Recycling threshold = %d, enter mechanism",
            HLOS_RECYCLING_THRESHOLD);
        taf_sms_hlos_recycling();
    }

    return LE_OK;
}

uint32_t taf_sms_hlos_recycling()
{
    LE_DEBUG("taf_sms_hlos_recycling");

    uint32_t recycledSlot = 0;

    for (uint32_t i = 0; i < MAX_OF_SMS_MSG_IN_HLOS; i++)
    {
        char smsPath[HLOS_PATH_MAX_BYTE];

        snprintf(smsPath, sizeof(smsPath), "%s/%d", SMS_STORAGE_PATH, i);

        if (le_fs_Exists(smsPath))
        {
            if (taf_sms_hlos_getReadStatus(i) == TAF_SMS_RXSTS_READ &&
               taf_sms_hlos_getLockStatus(i) == TAF_SMS_LKSTS_UNLOCKED)
            {
                le_result_t res = taf_sms_hlos_DelMsgFromStorage(i);

                if (res == LE_OK)
                {
                    LE_DEBUG("slot[%d] is recycled", i);
                    recycledSlot++;
                }
            }
        }

        if (recycledSlot >= HLOS_RECYCLING_BUFFER)
        {
            return recycledSlot;
        }
    }

    LE_DEBUG("recycled slots = %d", recycledSlot);

    return recycledSlot;
}

static void KsServerDisconnectHandler(void* contextPtr)
{
    LE_ERROR("KeyStore server disconnected");

    auto& sms = taf_sms_hlos::GetInstance();
    sms.isConnectToKeyStoreSvc = false;
}

static le_result_t KsServerConnect()
{
    // Add connection check - this is the required modification
    le_result_t res = LE_OK;
    auto& sms = taf_sms_hlos::GetInstance();
    if(sms.isConnectToKeyStoreSvc == false)
    {
        res = taf_ks_TryConnectService();
        if (res != LE_OK) {
            LE_ERROR("Failed to connect to key service, res = %d", res);
            return res;
        }
        LE_INFO("Connected to KeyStore service");
        sms.isConnectToKeyStoreSvc = true;
        taf_ks_SetNonExitServerDisconnectHandler(KsServerDisconnectHandler, NULL);
    }
    return LE_OK;
}

le_result_t taf_sms_hlos_encryptMsg(uint8_t* data,
    size_t dataSize,
    uint8_t* encryptedData,
    size_t* encryptedDataSize)
{
    le_result_t res = KsServerConnect();
    if (res != LE_OK)
    {
        return LE_UNAVAILABLE;
    }

    const char keyId[] = SMS_HLOS_KEY_ID;

    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;

    size_t totalEncryptedSize = *encryptedDataSize;
    size_t encSize = 0;

    res = taf_ks_GetKey(keyId, &keyRef);

    TAF_ERROR_IF_RET_VAL(LE_FAULT == res, LE_FAULT, "Get key(%s) failed",
        SMS_HLOS_KEY_ID);

    LE_INFO("Get key(%s) res = %d", SMS_HLOS_KEY_ID, res);

    if (LE_NOT_FOUND == res)
    {
        res = taf_ks_CreateKey(keyId, TAF_KS_AES_ENCRYPT_DECRYPT, &keyRef);

        TAF_ERROR_IF_RET_VAL(LE_FAULT == res, LE_FAULT, "Create key(%s) failed",
            SMS_HLOS_KEY_ID);

        res = taf_ks_ProvisionAesKeyValue(keyRef, TAF_KS_AES_SIZE_256,
            TAF_KS_AES_MODE_ECB_PAD_PKCS7, NULL, 0);

        TAF_ERROR_IF_RET_VAL(LE_FAULT == res, LE_FAULT,
            "Key value provision failed");
    }
    else if (LE_OK != res)
    {
        return LE_FAULT;
    }

    res = taf_ks_CryptoSessionCreate(keyRef, &sessionRef);

    TAF_ERROR_IF_RET_VAL(LE_OK != res, LE_FAULT, "Create session failed");

    res = taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_ENCRYPT);

    TAF_ERROR_IF_RET_VAL(LE_OK != res, LE_FAULT, "Start session failed");

    res = taf_ks_CryptoSessionProcess(sessionRef, data, dataSize, encryptedData,
        encryptedDataSize);

    TAF_ERROR_IF_RET_VAL(LE_OK != res, LE_FAULT, "Process session failed");

    encSize = *encryptedDataSize;

    *encryptedDataSize = totalEncryptedSize - encSize;

    res = taf_ks_CryptoSessionEnd(sessionRef, NULL, 0, encryptedData + encSize,
        encryptedDataSize);

    TAF_ERROR_IF_RET_VAL(LE_OK != res, LE_FAULT, "End session failed");

    *encryptedDataSize += encSize;

    return LE_OK;
}

le_result_t taf_sms_hlos_decryptMsg(uint8_t* cypherData,
    size_t cypherDataLen,
    uint8_t* decryptedData,
    size_t* decryptedDataSize)
{
    le_result_t res = KsServerConnect();
    if (res != LE_OK)
    {
        return LE_UNAVAILABLE;
    }

    const char keyId[] = SMS_HLOS_KEY_ID;

    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;

    size_t totalDecryptedSize = *decryptedDataSize;
    size_t decSize = 0;

    for (uint i = 0; i < cypherDataLen; i++)
    {
        LE_DEBUG("cypherData: 0x%.2X", cypherData[i]);
    }

    res = taf_ks_GetKey(keyId, &keyRef);

    TAF_ERROR_IF_RET_VAL(LE_OK != res, LE_FAULT, "Get key(%s) failed",
        SMS_HLOS_KEY_ID);

    res = taf_ks_CryptoSessionCreate(keyRef, &sessionRef);

    TAF_ERROR_IF_RET_VAL(LE_OK != res, LE_FAULT, "Create session failed");

    res = taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_DECRYPT);

    TAF_ERROR_IF_RET_VAL(LE_OK != res, LE_FAULT, "Start session failed");

    res = taf_ks_CryptoSessionProcess(sessionRef, cypherData, cypherDataLen,
        decryptedData, decryptedDataSize);

    TAF_ERROR_IF_RET_VAL(LE_OK != res, LE_FAULT, "Process session failed");

    LE_DEBUG("totalDecryptedSize %" PRIuS, totalDecryptedSize);

    decSize = *decryptedDataSize;
    LE_DEBUG("decSize %" PRIuS, decSize);

    *decryptedDataSize = totalDecryptedSize - decSize;
    LE_DEBUG("decryptedDataSize %" PRIuS, *decryptedDataSize);

    res = taf_ks_CryptoSessionEnd(sessionRef, NULL, 0, decryptedData + decSize,
        decryptedDataSize);

    TAF_ERROR_IF_RET_VAL(LE_OK != res, LE_FAULT, "End session failed(%d)", res);

    *decryptedDataSize += decSize;

    return LE_OK;
}

le_result_t taf_sms_hlos_ListMsgFromStorage(taf_sms_ReadStatus_t rxStatus,
    uint32_t* numOfIdx,
    uint32_t* idxArray)
{
    for (uint32_t i = 0; i < MAX_OF_SMS_MSG_IN_HLOS; ++i)
    {
        char smsPath[HLOS_PATH_MAX_BYTE] = { 0 };
        snprintf(smsPath, sizeof(smsPath), "%s/%d", SMS_STORAGE_PATH, i);

        if (le_fs_Exists(smsPath) && (taf_sms_hlos_getReadStatus(i) == rxStatus))
        {
            LE_DEBUG("HLOS SMS exist: %s", smsPath);
            idxArray[*numOfIdx] = i;
            (*numOfIdx)++;
        }
    }

    return LE_OK;
}

le_result_t taf_sms_hlos_ReadPDUMsgFromStorage(uint32_t index,
    taf_sms_Pdu_t* pduMsg)
{
    char smsPath[HLOS_PATH_MAX_BYTE];

    snprintf(smsPath, sizeof(smsPath), "%s/%d", SMS_STORAGE_PATH, index);

    le_fs_FileRef_t fileRef;
    le_result_t res = le_fs_Open(smsPath, LE_FS_RDONLY, &fileRef);

    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Fail to open sms file, res=%d", res);

    char smsFileStr[SMS_MAX_FILE_LEN * 2 + 1] = { 0 };
    uint8_t cypherData[SMS_AES_PKCS7_ENCRYPTED_LEN] = { 0 };

    size_t bufSize = sizeof(smsFileStr);
    res = le_fs_Read(fileRef, (uint8_t*)smsFileStr, &bufSize);

    if (res != LE_OK)
    {
        LE_ERROR("Fail to read sms file, res=%d", res);
        le_fs_Close(fileRef);
        return res;
    }
    else
    {
        LE_DEBUG("smsFileStr: %s", (char*)smsFileStr);
        LE_DEBUG("bufSize: %" PRIuS, bufSize);
    }

    le_fs_Close(fileRef);

    char* payloadPtr = smsFileStr + (HLOS_SMS_HEADER_LEN * 2);
    bufSize -= (HLOS_SMS_HEADER_LEN * 2);
    le_hex_StringToBinary(payloadPtr, bufSize, cypherData, sizeof(cypherData));

    taf_sms_hlos_encryptStatus_t encrytSts = taf_sms_hlos_getEncryptSts(index);
    LE_DEBUG(
        "getEncryptSts(index: %d) = %s", encrytSts,
        (encrytSts == TAF_SMS_HLOS_ENCRYPT_STS_ENCRYPTED) ? "true" : "false");

    if (encrytSts == TAF_SMS_HLOS_ENCRYPT_STS_ENCRYPTED)
    {
        uint8_t decryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
        size_t decryptedDataSize = sizeof(decryptedData);
        size_t cypherLen = bufSize / 2;

        le_result_t decryptRes = taf_sms_hlos_decryptMsg(
            cypherData, cypherLen, decryptedData, &decryptedDataSize);
        LE_DEBUG("taf_sms_hlos_decryptMsg res: %d", decryptRes);
        if (LE_OK != decryptRes)
        {
            return LE_FAULT; // FIX: fail fast on decrypt error
        }

        for (uint i = 0; i < decryptedDataSize; i++)
        {
            LE_DEBUG("decryptedData[%d] = 0x%.2X", i, decryptedData[i]);
            pduMsg->data[i] = decryptedData[i];
        }
        pduMsg->length = decryptedDataSize;
    }
    else
    {
        le_hex_StringToBinary(payloadPtr, bufSize, pduMsg->data,
            sizeof(pduMsg->data));
        pduMsg->length = bufSize / 2;
    }

    pduMsg->rxStatus = taf_sms_hlos_getReadStatus(index);
    pduMsg->lkStatus = taf_sms_hlos_getLockStatus(index);
    pduMsg->storage = TAF_SMS_STORAGE_HLOS;
    pduMsg->index = index;

    LE_INFO("taf_sms_hlos_ReadPDUMsgFromStorage index: %d finished", index);

    return res;
}

le_result_t taf_sms_hlos_DelMsgFromStorage(uint32_t index)
{
    char smsPath[HLOS_PATH_MAX_BYTE];

    snprintf(smsPath, sizeof(smsPath), "%s/%d", SMS_STORAGE_PATH, index);

    LE_DEBUG("Try to delete file: %s", smsPath);

    if (le_fs_Exists(smsPath))
    {
        // If message is locked, not allow the deletion, return LE_BUSY
        if (taf_sms_hlos_getLockStatus(index) == TAF_SMS_LKSTS_LOCKED)
        {
            LE_DEBUG("File: %s is locked, cannot be deleted", smsPath);
            return LE_BUSY;
        }
        le_fs_Delete(smsPath);

        return LE_OK;
    }

    TAF_ERROR_IF_RET_VAL(true, LE_BAD_PARAMETER, "Cannot find msg with index %d",
        index);
}

le_result_t taf_sms_hlos_DelAllMsgFromStorage()
{
    char smsPath[HLOS_PATH_MAX_BYTE];

    for (uint32_t i = 0; i < MAX_OF_SMS_MSG_IN_HLOS; i++)
    {
        snprintf(smsPath, sizeof(smsPath), "%s/%d", SMS_STORAGE_PATH, i);

        LE_DEBUG("Try to delete file: %s", smsPath);

        if (le_fs_Exists(smsPath))
        {
            // If message is locked, not allow the deletion, return
            // LE_BUSY
            if (taf_sms_hlos_getLockStatus(i) == TAF_SMS_LKSTS_LOCKED)
            {
                LE_DEBUG("File: %s is locked, cannot be deleted", smsPath);
                return LE_BUSY;
            }
            le_fs_Delete(smsPath);
        }
    }
    return LE_OK;
}

uint32_t ComputeHeaderCRC32(uint8_t* data, uint8_t dataLen)
{
    uint32_t crc = LE_CRC_START_CRC32;

    crc = le_crc_Crc32(data, dataLen, crc);

    return crc;
}

le_result_t SetHeaderStatus(uint32_t index, uint8_t statusMask, bool enable)
{
    char smsPath[HLOS_PATH_MAX_BYTE] = {0};
    snprintf(smsPath, sizeof(smsPath), "%s/%d", SMS_STORAGE_PATH, index);

    if (!le_fs_Exists(smsPath))
    {
        return LE_NOT_FOUND;
    }

    le_fs_FileRef_t fileRef;
    le_result_t res = le_fs_Open(smsPath, LE_FS_RDONLY, &fileRef);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Fail to open sms file, res=%d", res);

    uint8_t header[HLOS_SMS_HEADER_LEN] = {0};
    size_t headerSize = sizeof(header);

    char fHeader[(HLOS_SMS_HEADER_LEN * 2) + 1] = {0};
    size_t fHeaderSize = sizeof(fHeader);

    res = le_fs_Read(fileRef, (uint8_t *)fHeader, &fHeaderSize);
    if (res != LE_OK)
    {
        LE_ERROR("Fail to read sms file, res=%d", res);
        le_fs_Close(fileRef);
        return LE_FAULT;
    }
    LE_DEBUG("header hex bytes read: %" PRIuS, fHeaderSize);

    // Compute the effective hex string length (max header length in hex)
    size_t headerHexLen = HLOS_SMS_HEADER_LEN * 2;
    size_t hexLen = (fHeaderSize < headerHexLen) ? fHeaderSize : headerHexLen;
    fHeader[hexLen] = '\0';

    // Decode hex header to binary
    le_hex_StringToBinary(fHeader, hexLen, header, headerSize);

    le_fs_Close(fileRef);

    // Modify status bit
    if (enable)
    {
        header[HLOS_SMS_HEADER_INDEX_STATUS] |= statusMask;
    }
    else
    {
        header[HLOS_SMS_HEADER_INDEX_STATUS] &= (~statusMask);
    }

    // Recompute CRC and update header
    uint32_t crcWrite = ComputeHeaderCRC32(header, HLOS_SMS_CONFIG_LEN);
    memcpy(&header[HLOS_SMS_HEADER_INDEX_CRC], &crcWrite, HLOS_SMS_CRC_LEN);

    // Encode header back to hex string
    // Note: fHeader buffer is large enough ((HLOS_SMS_HEADER_LEN * 2) + 1)
    le_hex_BinaryToString(header, headerSize, fHeader, sizeof(fHeader));

    // Write back only the header portion (hex without terminating NUL)
    res = le_fs_Open(smsPath, LE_FS_WRONLY, &fileRef);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Fail to open sms file for write");

    size_t toWrite = headerHexLen; // exact number of hex chars for header
    res = le_fs_Write(fileRef, (uint8_t *)fHeader, toWrite);
    if (res != LE_OK)
    {
        LE_ERROR("Fail to write sms file, res=%d", res);
        le_fs_Close(fileRef);
        return LE_FAULT;
    }

    le_fs_Close(fileRef);
    return LE_OK;
}

bool IsHeaderStatusEnable(uint32_t index, uint8_t statusMask)
{
    char smsPath[HLOS_PATH_MAX_BYTE] = {0};
    snprintf(smsPath, sizeof(smsPath), "%s/%d", SMS_STORAGE_PATH, index);

    le_fs_FileRef_t fileRef;
    le_result_t res = le_fs_Open(smsPath, LE_FS_RDONLY, &fileRef);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, false, "Fail to open sms file, res=%d", res);

    uint8_t header[HLOS_SMS_HEADER_LEN] = {0};
    size_t headerSize = sizeof(header);

    char fHeader[(HLOS_SMS_HEADER_LEN * 2) + 1] = {0};
    size_t fHeaderSize = sizeof(fHeader);

    res = le_fs_Read(fileRef, (uint8_t *)fHeader, &fHeaderSize);
    if (res != LE_OK)
    {
        LE_ERROR("Fail to read sms file, res=%d", res);
        le_fs_Close(fileRef);
        return false;
    }

    // Compute the effective hex string length (max header length in hex)
    size_t headerHexLen = HLOS_SMS_HEADER_LEN * 2;
    size_t hexLen = (fHeaderSize < headerHexLen) ? fHeaderSize : headerHexLen;
    fHeader[hexLen] = '\0';

    le_hex_StringToBinary(fHeader, hexLen, header, headerSize);
    le_fs_Close(fileRef);

    // CRC validation
    uint32_t crcRead = 0;
    memcpy(&crcRead, &header[HLOS_SMS_HEADER_INDEX_CRC], HLOS_SMS_CRC_LEN);

    uint32_t crcCheck = ComputeHeaderCRC32(header, HLOS_SMS_CONFIG_LEN);
    if (crcRead != crcCheck)
    {
        LE_DEBUG("CRC check error, crc read: 0x%.8X, crc check: 0x%.8X", crcRead, crcCheck);
        return false;
    }

    // Check status bit
    return (statusMask & header[HLOS_SMS_HEADER_INDEX_STATUS]) != 0;
}

le_result_t taf_sms_hlos_setReadStatus(uint32_t index,
    taf_sms_ReadStatus_t rxStatus)
{
    return SetHeaderStatus(index, HLOS_SMS_READSTS_MASK,
        (rxStatus == TAF_SMS_RXSTS_READ) ? true : false);
}

taf_sms_ReadStatus_t taf_sms_hlos_getReadStatus(uint32_t index)
{
    return ((IsHeaderStatusEnable(index, HLOS_SMS_READSTS_MASK) == true)
            ? TAF_SMS_RXSTS_READ
            : TAF_SMS_RXSTS_UNREAD);
}

le_result_t taf_sms_hlos_setLockStatus(uint32_t index,
    taf_sms_LockStatus_t lockStatus)
{
    if (lockStatus == TAF_SMS_LKSTS_LOCKED &&
      (IsHeaderStatusEnable(index, HLOS_SMS_LOCKSTS_MASK) == true))
    {
        LE_ERROR("msg[%d] is alread locked", index);
        return LE_NOT_PERMITTED;
    }

    if (lockStatus == TAF_SMS_LKSTS_UNLOCKED &&
      (IsHeaderStatusEnable(index, HLOS_SMS_LOCKSTS_MASK) == false))
    {
        LE_ERROR("msg[%d] is alread unlocked", index);
        return LE_NOT_PERMITTED;
    }

    return SetHeaderStatus(index, HLOS_SMS_LOCKSTS_MASK,
        (lockStatus == TAF_SMS_LKSTS_LOCKED) ? true : false);
}

taf_sms_LockStatus_t taf_sms_hlos_getLockStatus(uint32_t index)
{
    return ((IsHeaderStatusEnable(index, HLOS_SMS_LOCKSTS_MASK) == true)
            ? TAF_SMS_LKSTS_LOCKED
            : TAF_SMS_LKSTS_UNLOCKED);
}

le_result_t taf_sms_hlos_setEncryptSts(
    uint32_t index,
    taf_sms_hlos_encryptStatus_t encryptSts)
{
    return SetHeaderStatus(
        index, HLOS_SMS_ENCRYPTSTS_MASK,
        (encryptSts == TAF_SMS_HLOS_ENCRYPT_STS_ENCRYPTED) ? true : false);
}

taf_sms_hlos_encryptStatus_t taf_sms_hlos_getEncryptSts(uint32_t index)
{
    return ((IsHeaderStatusEnable(index, HLOS_SMS_ENCRYPTSTS_MASK) == true)
            ? TAF_SMS_HLOS_ENCRYPT_STS_ENCRYPTED
            : TAF_SMS_HLOS_ENCRYPT_STS_UNENCRYPTED);
}

le_result_t taf_sms_hlos_createNewHeader(char* dataPtr,
    size_t dataLen,
    bool encrypted)
{
    TAF_ERROR_IF_RET_VAL(dataLen < (HLOS_SMS_HEADER_LEN * 2) + 1, LE_OVERFLOW,
        "data length is not enough to create header");

    uint8_t header[HLOS_SMS_HEADER_LEN] = { 0 };

    header[HLOS_SMS_HEADER_INDEX_MAGIC] = HLOS_SMS_HEADER_MAGIC_NUM;
    header[HLOS_SMS_HEADER_INDEX_VERSION] = HLOS_SMS_HEADER_DEFAULT_VER;
    header[HLOS_SMS_HEADER_INDEX_STATUS] = HLOS_SMS_HEADER_DEFAULT_STS;

    if (encrypted)
    {
        header[HLOS_SMS_HEADER_INDEX_STATUS] |= HLOS_SMS_ENCRYPTSTS_MASK;
    }

    header[HLOS_SMS_HEADER_INDEX_RESERVE] = 0;

    uint32_t crcWrite = ComputeHeaderCRC32(header, HLOS_SMS_CONFIG_LEN);
    memcpy(&header[HLOS_SMS_HEADER_INDEX_CRC], &crcWrite, HLOS_SMS_CRC_LEN);

    le_hex_BinaryToString(header, sizeof(header), dataPtr, dataLen);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Encrypt the message in HLOS storage
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_sms_hlos_EncryptFromStorage(uint32_t index)
{
    char smsPath_src[HLOS_PATH_MAX_BYTE] = { 0 };
    snprintf(smsPath_src, sizeof(smsPath_src), "%s/%d", SMS_STORAGE_PATH, index);

    // Check whether it's an existed file
    TAF_ERROR_IF_RET_VAL(le_fs_Exists(smsPath_src) == false, LE_BAD_PARAMETER,
        "The message with index %d doesn't exist", index);

    // Check whether it's an encrypted file
    if (taf_sms_hlos_getEncryptSts(index) == TAF_SMS_HLOS_ENCRYPT_STS_ENCRYPTED)
    {
        return LE_OK;
    }

    // Read message content from HLOS
    taf_sms_Pdu_t pduMsg = {};
    taf_sms_hlos_ReadPDUMsgFromStorage(index, &pduMsg);

    uint8_t encryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t encryptedDataSize = sizeof(encryptedData);
    le_result_t encryptRes = LE_OK;

    // Encrypt message content from HLOS
    encryptRes = taf_sms_hlos_encryptMsg(pduMsg.data, pduMsg.length,
        encryptedData, &encryptedDataSize);
    LE_DEBUG("taf_sms_hlos_encryptMsg res: %d", encryptRes);
    LE_DEBUG("taf_sms_hlos_encryptMsg size: %" PRIuS, encryptedDataSize);

    TAF_ERROR_IF_RET_VAL(encryptRes != LE_OK, LE_FAULT,
        "Message encryption failed");

    // Read and copy message header from HLOS
    le_fs_FileRef_t fileRef_src, fileRef_dest;

    le_result_t res = le_fs_Open(smsPath_src, LE_FS_RDONLY, &fileRef_src);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Fail to open sms file %s, res=%d",
        smsPath_src, res);

    char smsFileStr[SMS_MAX_FILE_LEN * 2 + 1] = { 0 };
    size_t fHeaderSize = (HLOS_SMS_HEADER_LEN)*2 + 1;

    res = le_fs_Read(fileRef_src, (uint8_t*)smsFileStr, &fHeaderSize);

    if (res != LE_OK)
    {
        LE_ERROR("Fail to read sms file %s, res=%d", smsPath_src, res);
        le_fs_Close(fileRef_src);
        return LE_FAULT;
    }

    le_fs_Close(fileRef_src);

    // Copy encrypted message content
    char* payloadPtr = smsFileStr + (HLOS_SMS_HEADER_LEN * 2);
    size_t payloadLen = ((SMS_MAX_FILE_LEN - HLOS_SMS_HEADER_LEN) * 2) + 1;

    le_hex_BinaryToString(encryptedData, encryptedDataSize, payloadPtr,
        payloadLen);

    uint32_t dataLen = HLOS_SMS_HEADER_LEN + encryptedDataSize;

    // Write encrypted message to HLOS
    char smsPath_dest[HLOS_PATH_MAX_BYTE] = { 0 };
    snprintf(smsPath_dest, sizeof(smsPath_dest), "%s/%d%s", SMS_STORAGE_PATH,
        index, SMS_STORAGE_ENCRYPTED_SUFFIX);

    res = le_fs_Open(smsPath_dest, LE_FS_CREAT | LE_FS_WRONLY, &fileRef_dest);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Fail to open sms file %s, res=%d",
        smsPath_dest, res);

    size_t size = ((size_t)dataLen) * sizeof(char) * 2;
    res = le_fs_Write(fileRef_dest, (uint8_t*)smsFileStr, size);
    if (res != LE_OK)
    {
        LE_ERROR("Fail to write sms file %s, res %d", smsPath_dest, res);
        le_fs_Close(fileRef_dest);
        return LE_FAULT;
    }

    le_fs_Close(fileRef_dest);

    res = le_fs_Move(smsPath_dest, smsPath_src);
    TAF_ERROR_IF_RET_VAL(res != LE_OK, LE_FAULT, "Fail to overwrite sms file %s",
        smsPath_src);

    taf_sms_hlos_setEncryptSts(index, TAF_SMS_HLOS_ENCRYPT_STS_ENCRYPTED);

    return LE_OK;
}
