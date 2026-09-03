/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafKeyStore.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * The keyStore daemon's initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    tafsvc::taf_KeyStore& myKeyStore = tafsvc::taf_KeyStore::GetInstance();
    myKeyStore.Init();
}

//--------------------------------------------------------------------------------------------------
/**
 * Creates a new key by key ID.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CreateKey
(
    const char* keyId,
    taf_ks_KeyUsage_t keyUsage,
    taf_ks_KeyRef_t* keyRefPtr
)
{
    return tafsvc::taf_KeyStore::GetInstance().CreateKey(keyId, keyUsage, keyRefPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get key reference to a provisioned key by key name.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_GetKey
(
    const char* keyName,
    taf_ks_KeyRef_t* keyRefPtr
)
{
    return tafsvc::taf_KeyStore::GetInstance().GetKey(keyName, keyRefPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete a key by key reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_DeleteKey
(
    taf_ks_KeyRef_t keyRef
)
{
    return tafsvc::taf_KeyStore::GetInstance().DeleteKey(keyRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get key usage of a key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_GetKeyUsage
(
    taf_ks_KeyRef_t keyRef,
    taf_ks_KeyUsage_t* keyUsagePtr
)
{
    return tafsvc::taf_KeyStore::GetInstance().GetKeyUsage(keyRef, keyUsagePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of times that a key may be used between system reboots.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_SetKeyMaxUsesPerBoot
(
    taf_ks_KeyRef_t keyRef,
    uint32_t value
)
{
    return tafsvc::taf_KeyStore::GetInstance().SetKeyMaxUsesPerBoot(keyRef, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the minimum amount of time that elapses between allowed operations using a key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_SetKeyMinSecondsBetweenOps
(
    taf_ks_KeyRef_t keyRef,
    uint32_t value
)
{
    return tafsvc::taf_KeyStore::GetInstance().SetKeyMinSecondsBetweenOps(keyRef, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set application data to the key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_SetKeyAppData
(
    taf_ks_KeyRef_t keyRef,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    return tafsvc::taf_KeyStore::GetInstance().SetKeyAppData(keyRef, dataPtr, dataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the date and time at which the key becomes active.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_SetKeyActiveDateTime
(
    taf_ks_KeyRef_t keyRef,
    uint64_t value
)
{
    return tafsvc::taf_KeyStore::GetInstance().SetKeyActiveDateTime(keyRef, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the date and time at which the key expires for signing and encryption.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_SetKeyOriginationExpireDateTime
(
    taf_ks_KeyRef_t keyRef,
    uint64_t value
)
{
    return tafsvc::taf_KeyStore::GetInstance().SetKeyOriginationExpireDateTime(keyRef, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the date and time at which the key expires for verification and decryption.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_SetKeyUsageExpireDateTime
(
    taf_ks_KeyRef_t keyRef,
    uint64_t value
)
{
    return tafsvc::taf_KeyStore::GetInstance().SetKeyUsageExpireDateTime(keyRef, value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Provision a RSA encryption key value to the new created key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_ProvisionRsaEncKeyValue
(
    taf_ks_KeyRef_t keyRef,
    taf_ks_RsaKeySize_t keySize,
    taf_ks_RsaEncPadding_t padding,
    const uint8_t* impDataPtr,
    size_t impDataSize
)
{
    return tafsvc::taf_KeyStore::GetInstance().ProvisionRsaEncKeyValue(keyRef, keySize, padding,
                                                                        impDataPtr, impDataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Provision a RSA signing key value to the new created key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_ProvisionRsaSigKeyValue
(
    taf_ks_KeyRef_t keyRef,
    taf_ks_RsaKeySize_t keySize,
    taf_ks_RsaSigPadding_t padding,
    const uint8_t* impDataPtr,
    size_t impDataSize
)
{
    return tafsvc::taf_KeyStore::GetInstance().ProvisionRsaSigKeyValue(keyRef, keySize, padding,
                                                                        impDataPtr, impDataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Provision an ECDSA key value to the new created key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_ProvisionEcdsaKeyValue
(
    taf_ks_KeyRef_t keyRef,
    taf_ks_EccKeySize_t keySize,
    taf_ks_Digest_t digest,
    const uint8_t* impDataPtr,
    size_t impDataSize
)
{
    return tafsvc::taf_KeyStore::GetInstance().ProvisionEcdsaKeyValue(keyRef, keySize, digest,
                                                                       impDataPtr, impDataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Provision an AES key value to the new created key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_ProvisionAesKeyValue
(
    taf_ks_KeyRef_t keyRef,
    taf_ks_AesKeySize_t keySize,
    taf_ks_AesBlockMode_t mode,
    const uint8_t* impDataPtr,
    size_t impDataSize
)
{
    return tafsvc::taf_KeyStore::GetInstance().ProvisionAesKeyValue(keyRef, keySize, mode,
                                                                     impDataPtr, impDataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Provision a HMAC key value to the new created key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_ProvisionHmacKeyValue
(
    taf_ks_KeyRef_t keyRef,
    uint32_t keySize,
    taf_ks_Digest_t digest,
    const uint8_t* impDataPtr,
    size_t impDataSize
)
{
    return tafsvc::taf_KeyStore::GetInstance().ProvisionHmacKeyValue(keyRef, keySize, digest,
                                                                      impDataPtr, impDataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Export a key into specified key data format.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_ExportKey
(
    taf_ks_KeyRef_t keyRef,
    const uint8_t* appDataPtr,
    size_t appDataSize,
    uint8_t* expDataPtr,
    size_t* expDataSizePtr
)
{
    return tafsvc::taf_KeyStore::GetInstance().ExportKey(keyRef, appDataPtr, appDataSize,
                                                          expDataPtr, expDataSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Share a key to another application to use.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_ShareKey
(
    taf_ks_KeyRef_t keyRef,
    const char* LE_NONNULL appName,
    taf_ks_KeyUsage_t keyCap,
    taf_ks_AppCapMask_t appCap
)
{
    return tafsvc::taf_KeyStore::GetInstance().ShareKey(keyRef, appName, keyCap, appCap);
}

//--------------------------------------------------------------------------------------------------
/**
 * Cancel the key sharing.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CancelKeySharing
(
    taf_ks_KeyRef_t keyRef,
    const char* LE_NONNULL appName
)
{
    return tafsvc::taf_KeyStore::GetInstance().CancelKeySharing(keyRef, appName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the application name of the calling client.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_GetCallingAppName
(
    char* appName,
    size_t appNameSize
)
{
    return tafsvc::taf_KeyStore::GetInstance().GetCallingAppName(appName, appNameSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the first application that the given key is shared to.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_GetFirstSharedApp
(
    taf_ks_KeyRef_t keyRef,
    char* appName,
    size_t appNameSize,
    taf_ks_KeyUsage_t* keyCapPtr,
    taf_ks_AppCapMask_t* appCapPtr
)
{
    return tafsvc::taf_KeyStore::GetInstance().GetFirstSharedApp(keyRef, appName, appNameSize,
                                                                  keyCapPtr, appCapPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next application that the given key is shared to.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_GetNextSharedApp
(
    taf_ks_KeyRef_t keyRef,
    char* appName,
    size_t appNameSize,
    taf_ks_KeyUsage_t* keyCapPtr,
    taf_ks_AppCapMask_t* appCapPtr
)
{
    return tafsvc::taf_KeyStore::GetInstance().GetNextSharedApp(keyRef, appName, appNameSize,
                                                                 keyCapPtr, appCapPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_ks_KeySharing'.
 */
//--------------------------------------------------------------------------------------------------
taf_ks_KeySharingHandlerRef_t taf_ks_AddKeySharingHandler
(
    const char* LE_NONNULL keyId,
    const char* LE_NONNULL appName,
    taf_ks_KeySharingHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    return tafsvc::taf_KeyStore::GetInstance().AddKeySharingHandler(keyId, appName, handlerPtr,
                                                                     contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_ks_KeySharing'.
 */
//--------------------------------------------------------------------------------------------------
void taf_ks_RemoveKeySharingHandler
(
    taf_ks_KeySharingHandlerRef_t handlerRef
)
{
    tafsvc::taf_KeyStore::GetInstance().RemoveKeySharingHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a crypto operation session for the key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionCreate
(
    taf_ks_KeyRef_t keyRef,
    taf_ks_CryptoSessionRef_t* sessionRefPtr
)
{
    return tafsvc::taf_KeyStore::GetInstance().CryptoSessionCreate(keyRef, sessionRefPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the nonce or IVs for AES GCM, CBC, CTR for a crypto session.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionSetAesNonce
(
    taf_ks_CryptoSessionRef_t sessionRef,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    return tafsvc::taf_KeyStore::GetInstance().CryptoSessionSetAesNonce(sessionRef, dataPtr,
                                                                         dataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the general padding type used for a RSA signing/verification cryptographic session.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionSetRsaPadding
(
    taf_ks_CryptoSessionRef_t sessionRef,
    taf_ks_RsaPaddingType_t paddingType
)
{
    return tafsvc::taf_KeyStore::GetInstance().CryptoSessionSetRsaPadding(sessionRef, paddingType);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Application data for a crypto session.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionSetAppData
(
    taf_ks_CryptoSessionRef_t sessionRef,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    return tafsvc::taf_KeyStore::GetInstance().CryptoSessionSetAppData(sessionRef, dataPtr,
                                                                        dataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Start the crypto session for the given crypto operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionStart
(
    taf_ks_CryptoSessionRef_t sessionRef,
    taf_ks_CryptoPurpose_t cryptoPurpose
)
{
    return tafsvc::taf_KeyStore::GetInstance().CryptoSessionStart(sessionRef, cryptoPurpose);
}

//--------------------------------------------------------------------------------------------------
/**
 * Provides AES AEAD to the running crypto session started with CryptoSessionStart API.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionProcessAead
(
    taf_ks_CryptoSessionRef_t sessionRef,
    const uint8_t* inputDataPtr,
    size_t inputDataSize
)
{
    return tafsvc::taf_KeyStore::GetInstance().CryptoSessionProcessAead(sessionRef, inputDataPtr,
                                                                         inputDataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Provides data to, and possibly receives output from, a running crypto session.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionProcess
(
    taf_ks_CryptoSessionRef_t sessionRef,
    const uint8_t* inputDataPtr,
    size_t inputDataSize,
    uint8_t* outputDataPtr,
    size_t* outputDataSizePtr
)
{
    return tafsvc::taf_KeyStore::GetInstance().CryptoSessionProcess(sessionRef, inputDataPtr,
                                                                      inputDataSize, outputDataPtr,
                                                                      outputDataSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Finalizes the crypto session started with CryptoSessionStart API.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionEnd
(
    taf_ks_CryptoSessionRef_t sessionRef,
    const uint8_t* inputDataPtr,
    size_t inputDataSize,
    uint8_t* outputDataPtr,
    size_t* outputDataSizePtr
)
{
    return tafsvc::taf_KeyStore::GetInstance().CryptoSessionEnd(sessionRef, inputDataPtr,
                                                                 inputDataSize, outputDataPtr,
                                                                 outputDataSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Abort the crypto session started with CryptoSessionStart API.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionAbort
(
    taf_ks_CryptoSessionRef_t sessionRef
)
{
    return tafsvc::taf_KeyStore::GetInstance().CryptoSessionAbort(sessionRef);
}
