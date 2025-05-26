/*
 *  Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/*
 * @file       tafKeyStoreUnitTest.c
 * @brief      This file includes test functions for tafKeyStore Service
 */

#include "legato.h"
#include "interfaces.h"
#include "linux/file.h"
#include <openssl/hmac.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/evp.h>

//--------------------------------------------------------------------------------------------------
/**
 * Paddings for RSA signing/verification.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    NO_PADDING,
    PKCS_V15_PADDING,
    PSS_PADDING,
}
RsaPadding_t;

//--------------------------------------------------------------------------------------------------
/**
 * Paddings for RSA signing/verification.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    DIGEST_MD5,
    DIGEST_SHA1,
    DIGEST_SHA2_224,
    DIGEST_SHA2_256,
    DIGEST_SHA2_384,
    DIGEST_SHA2_512,
}
Digest_t;

//--------------------------------------------------------------------------------------------------
/**
 * Tests for key management APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static le_result_t OpensslVerifySignature
(
    const uint8_t* messagePtr,
    size_t messageSize,
    const uint8_t* pubKeyDataPtr,
    size_t pubKeyDataSize,
    const uint8_t* signaturePtr,
    size_t signatureSize,
    Digest_t digest,
    RsaPadding_t padding
)
{
    // Check parameter.
    if ((messagePtr == NULL) || (pubKeyDataPtr == NULL) || (signaturePtr == NULL) ||
        (messageSize == 0) || (pubKeyDataSize == 0) || (signatureSize == 0))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    // Check digest.
    const EVP_MD* method = NULL;
    switch (digest)
    {
        case DIGEST_MD5:
            method = EVP_md5();
            break;
        case DIGEST_SHA1:
            method = EVP_sha1();
            break;
        case DIGEST_SHA2_224:
            method = EVP_sha224();
            break;
        case DIGEST_SHA2_256:
            method = EVP_sha256();
            break;
        case DIGEST_SHA2_384:
            method = EVP_sha384();
            break;
        case DIGEST_SHA2_512:
            method = EVP_sha512();
            break;
        default:
            LE_ERROR("unknown digest(%d).", digest);
            return LE_FAULT;
            break;
    }

    // Import a x.509 public key and get the Openssl pubKey object.
    const uint8_t* expDataPtr = pubKeyDataPtr;
    EVP_PKEY* evpPubKey = d2i_PUBKEY(NULL, &expDataPtr, pubKeyDataSize);
    if (evpPubKey == NULL)
    {
        LE_ERROR("Failed to import the RSA public key to OpenSSL key object.");
        return LE_FAULT;
    }

    // Create an openssl context for verification.
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if(ctx == NULL)
    {
        LE_ERROR("ctx is NULL");
        EVP_PKEY_free(evpPubKey);
        return LE_FAULT;
    }

    // Create a key context for setting the PSS padding.
    EVP_PKEY_CTX *pKeyCtx= NULL;
    EVP_DigestVerifyInit(ctx, &pKeyCtx, method, NULL, evpPubKey);

    // Set RSA-PSS padding if required.
    if (padding == PSS_PADDING)
    {
        if ((EVP_PKEY_CTX_set_rsa_padding(pKeyCtx, RSA_PKCS1_PSS_PADDING) <= 0) ||
            (EVP_PKEY_CTX_set_rsa_pss_saltlen(pKeyCtx, -1) <= 0))
        {
            LE_ERROR("Failed to set RSA-PSS padding.");
            EVP_PKEY_free(evpPubKey);
            EVP_MD_CTX_free(ctx);
            return LE_FAULT;
        }
    }

    int err = 0;
    if(EVP_DigestVerifyUpdate(ctx, messagePtr, messageSize) == 1)
    {
        err = EVP_DigestVerifyFinal(ctx, signaturePtr, signatureSize);
    }

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(evpPubKey);

    if (err != 1)
    {
        LE_ERROR("Verification failed.");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests for key management APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void KeyManagementTest(void)
{
    le_result_t result;
    const char keyId[] = "KeyManagementTest";
    taf_ks_KeyRef_t keyRef, keyRef1;
    taf_ks_KeyUsage_t keyUsage;
    taf_ks_CryptoSessionRef_t sessionRef;

    LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId, TAF_KS_RSA_ENCRYPT_DECRYPT, &keyRef),
                   "Test RSA encryption/decryption key creation.");

    LE_TEST_ASSERT(LE_OK == taf_ks_SetKeyMaxUsesPerBoot(keyRef, 10),
                   "Test taf_ks_SetKeyMaxUsesPerBoot().");

    LE_TEST_ASSERT(LE_OK == taf_ks_SetKeyMinSecondsBetweenOps(keyRef, 5),
                   "Test taf_ks_SetKeyMinSecondsBetweenOps().");

    result = taf_ks_GetKeyUsage(keyRef, &keyUsage);
    LE_TEST_ASSERT((LE_OK == result) && (keyUsage == TAF_KS_RSA_ENCRYPT_DECRYPT),
                    "Test checking the key usage before key provision.");

    LE_TEST_ASSERT(LE_NOT_FOUND == taf_ks_GetKey(keyId, &keyRef1),
                   "Test geting the key before key provision.");

    result = taf_ks_CreateKey(keyId, TAF_KS_RSA_ENCRYPT_DECRYPT, &keyRef1);
    LE_TEST_ASSERT(LE_NOT_PERMITTED == result,
                   "Test key creation again with the same keyUsage.");

    result = taf_ks_CreateKey(keyId, TAF_KS_RSA_ENCRYPT_ONLY, &keyRef1);
    LE_TEST_ASSERT(LE_NOT_PERMITTED == result,
                   "Test key creation again with a different keyUsage.");

    result = taf_ks_ProvisionAesKeyValue(keyRef,
                                         TAF_KS_AES_SIZE_256,
                                         TAF_KS_AES_MODE_ECB_PAD_PKCS7,
                                         NULL, 0);
    LE_TEST_ASSERT(LE_NOT_PERMITTED == result,
                   "Test wrong key value provision.");

    result = taf_ks_ProvisionRsaEncKeyValue(keyRef,
                                            TAF_KS_RSA_SIZE_4096,
                                            TAF_KS_RSA_ENC_PAD_PKCS1_V15,
                                            NULL, 0);
    LE_TEST_ASSERT(LE_OK == result,
                   "Test correct key value provision.");

    result = taf_ks_ProvisionRsaEncKeyValue(keyRef,
                                            TAF_KS_RSA_SIZE_4096,
                                            TAF_KS_RSA_ENC_PAD_PKCS1_V15,
                                            NULL, 0);
    LE_TEST_ASSERT(LE_NOT_PERMITTED == result,
                   "Test correct key value provision again.");

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_ENCRYPT),
                   "Test start session for data encryption.");

    LE_TEST_ASSERT(LE_NOT_PERMITTED == taf_ks_DeleteKey(keyRef),
                   "Test key deletion after session start.");

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionAbort(sessionRef), "Test abort session.");

    result = taf_ks_GetKey(keyId, &keyRef1);
    LE_TEST_ASSERT((LE_OK == result) && (keyRef1 == keyRef),
                   "Test Geting the key after key provision.");

    result = taf_ks_GetKeyUsage(keyRef1, &keyUsage);
    LE_TEST_ASSERT((LE_OK == result) && (keyUsage == TAF_KS_RSA_ENCRYPT_DECRYPT),
                   "Test checking the key usage after key provision.");

    LE_TEST_ASSERT(LE_OK == taf_ks_DeleteKey(keyRef1),
                   "Test key deletion.");

    LE_TEST_ASSERT(LE_NOT_FOUND == taf_ks_DeleteKey(keyRef1),
                   "Test key deletion again.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests for key sharing management APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void KeySharingTest(void)
{
    const char keyId[] = "MySharedKeyTest";
    const char invalidKeyId[] ="Wrong#&keyId";
    const char nonExistSharedKeyId[] = "NonExistApp::NonExistKeyId";
    const char* sharedAppNameList[] = {"Shared_App1", "Shared_App2", "Shared_App3", "Shared_App4",
        "Shared_App5", "shared_App6", "Shared_app7", "Shared_App8","Shared_app9", "shared_App10",
        "Shared_app11", "shared_app12"};
    char appName[TAF_KS_MAX_APP_NAME_SIZE+1] = { 0 };
    char keyName[TAF_KS_MAX_KEY_ID_SIZE+1] = { 0 };
    taf_ks_KeyRef_t keyRef;
    taf_ks_KeyRef_t keyRef1;
    taf_ks_KeyUsage_t keyCap;
    taf_ks_AppCapMask_t appCap;

    // Get/Create an invalid key.
    LE_TEST_ASSERT((LE_BAD_PARAMETER == taf_ks_GetKey(invalidKeyId, &keyRef)) &&
                   (LE_BAD_PARAMETER == taf_ks_CreateKey(invalidKeyId, TAF_KS_RSA_ENCRYPT_DECRYPT,
                                                          &keyRef)),
                   "Get/Create an invalid key (wrong keyId format).");

    // Get a non-exist shared key.
    LE_TEST_ASSERT(LE_NOT_FOUND == taf_ks_GetKey(nonExistSharedKeyId, &keyRef),
                   "Get a non-exist shared key.");

    if (LE_NOT_FOUND == taf_ks_GetKey(keyId, &keyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId, TAF_KS_RSA_ENCRYPT_DECRYPT, &keyRef),
                                                 "Test shared key creation.");
        LE_TEST_ASSERT(LE_NOT_PERMITTED == taf_ks_ShareKey(keyRef, "SharedApp_X",
                                                           TAF_KS_RSA_ENCRYPT_DECRYPT, 0),
                       "New created key is not allowed to share.");
        LE_TEST_ASSERT(LE_OK == taf_ks_ProvisionRsaEncKeyValue(keyRef,
                                                               TAF_KS_RSA_SIZE_1024,
                                                               TAF_KS_RSA_ENC_PAD_OAEP_SHA2_512,
                                                               NULL, 0),
                       "Shared key is provisioned.");

        // Share a wrong keyCap.
        for (int i = 0; i < NUM_ARRAY_MEMBERS(sharedAppNameList); i++)
        {
            LE_TEST_ASSERT(LE_NOT_PERMITTED == taf_ks_ShareKey(keyRef, sharedAppNameList[i],
                                                               TAF_KS_AES_ENCRYPT_DECRYPT, 0),
                           "Share a wrong keyCap to app: '%s'.", sharedAppNameList[i]);
        }

        // Share a correct keyCap.
        for (int i = 0; i < NUM_ARRAY_MEMBERS(sharedAppNameList); i++)
        {
            LE_TEST_ASSERT(LE_OK == taf_ks_ShareKey(keyRef, sharedAppNameList[i],
                                                    TAF_KS_RSA_ENCRYPT_DECRYPT,
                                                    TAF_KS_CAP_EXPORT_KEY),
                           "Share key to app: '%s'.", sharedAppNameList[i]);
        }

        // Share to one more app will fail since we only support to share at most 12 apps.
        LE_TEST_ASSERT(LE_NO_MEMORY == taf_ks_ShareKey(keyRef, "Shared_AppY",
                                                       TAF_KS_RSA_ENCRYPT_DECRYPT, 0),
                       "Share key to the 6th app: '%s'.", "Shared_AppY");

        // Share the same keyCap and appCap.
        LE_TEST_ASSERT(LE_DUPLICATE == taf_ks_ShareKey(keyRef, "Shared_App3",
                                                       TAF_KS_RSA_ENCRYPT_DECRYPT,
                                                       TAF_KS_CAP_EXPORT_KEY),
                       "Share the same keyCap/appCap to app: '%s'.", "Shared_App3");

        // Update the keyCap and appCap.
        LE_TEST_ASSERT(LE_OK == taf_ks_ShareKey(keyRef, "Shared_App3",
                                                         TAF_KS_RSA_ENCRYPT_DECRYPT,
                                                         TAF_KS_CAP_DELETE_KEY),
                       "Update the keyCap/appCap to app: '%s'.", "Shared_App3");
    }
    else
    {
        // Get the key by keyName="<appName>::<keyId>", Shall return the same key reference
        // If the appName is our own app.

        LE_TEST_ASSERT (LE_OK == taf_ks_GetCallingAppName(appName, sizeof(appName)),
                        " Get calling app name: '%s'", appName);

        snprintf(keyName, sizeof(keyName), "%s::%s", appName, keyId);
        LE_TEST_ASSERT((LE_OK == taf_ks_GetKey(keyName, &keyRef1)) && (keyRef == keyRef1),
                       "Get the same key again (%s).", keyName);

        // Get sharedApp list.
        LE_TEST_ASSERT(LE_OK == taf_ks_GetFirstSharedApp(keyRef, appName, sizeof(appName),
                                                         &keyCap, &appCap),
                       "Get shared app entry: name='%s', keyCap=%u, appCap=%u",
                       appName, keyCap, appCap);
        while (LE_OK == taf_ks_GetNextSharedApp(keyRef, appName, sizeof(appName), &keyCap, &appCap))
        {
            LE_TEST_INFO("Get shared app entry: name'%s', keyCap=%u, appCap=%u",
                         appName, keyCap, appCap);
        }

        // Get sharedApp list again.
        LE_TEST_ASSERT(LE_OK == taf_ks_GetFirstSharedApp(keyRef, appName, sizeof(appName),
                                                         &keyCap, &appCap),
                       "Get shared app entry: name='%s', keyCap=%u, appCap=%u",
                       appName, keyCap, appCap);
        while (LE_OK == taf_ks_GetNextSharedApp(keyRef, appName, sizeof(appName), &keyCap, &appCap))
        {
            LE_TEST_INFO("Get shared app entry: name'%s', keyCap=%u, appCap=%u",
                         appName, keyCap, appCap);
        }

        // Cancel the sharing for non-shared app will fail.
        LE_TEST_ASSERT(LE_NOT_PERMITTED == taf_ks_CancelKeySharing(keyRef, "Shared_AppZ"),
                       "Cancel keySharing for a non-shared app: '%s'.", "Shared_AppZ");

        // Cancel the sharing for all apps
        for (int i = 0; i < NUM_ARRAY_MEMBERS(sharedAppNameList); i++)
        {
            LE_TEST_ASSERT(LE_OK == taf_ks_CancelKeySharing(keyRef, sharedAppNameList[i]),
                           "Cancel the key sharing to app: '%s'.", sharedAppNameList[i]);
        }

        // Cancel the sharing again will fail.
        for (int i = 0; i < NUM_ARRAY_MEMBERS(sharedAppNameList); i++)
        {
            LE_TEST_ASSERT(LE_NOT_PERMITTED == taf_ks_CancelKeySharing(keyRef,
                                                                       sharedAppNameList[i]),
                           "Cancel the key sharing to app: '%s'.", sharedAppNameList[i]);
        }

        // Delete the key
        LE_TEST_ASSERT(LE_OK == taf_ks_DeleteKey(keyRef), "Delete the shared key.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests for AES GCM encryption/decryption
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void AesGcmTest(void)
{
    le_result_t result;
    int32_t errCode;
    size_t encSize1, encSize2, decSize, totalSize;
    const char keyId[] = "aesGcmTest";
    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;
    const uint8_t plainText[] = "Keystore service for AES GCM test messages.";
    uint8_t encryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t encryptedDataSize = sizeof(encryptedData);
    uint8_t decryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t decryptedDataSize = sizeof(decryptedData);
    const uint8_t nonce[12] = {"abcdefghijk"};
    const uint8_t aead[12] = {"123456789ab"};

    if (LE_NOT_FOUND == taf_ks_GetKey(keyId, &keyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId,TAF_KS_AES_ENCRYPT_DECRYPT, &keyRef),
                       "Test AES GCM key creation.");
        result = taf_ks_ProvisionAesKeyValue(keyRef,
                                             TAF_KS_AES_SIZE_256,
                                             TAF_KS_AES_MODE_GCM,
                                             NULL, 0);
        LE_TEST_ASSERT(LE_OK == result, "Test AES GCM key value provision.");
    }
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test crypto session creation for encryption.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionSetAesNonce(sessionRef, nonce, sizeof(nonce)),
                   "Test setting nonce.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_ENCRYPT),
                   "Test start session for data encryption.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionProcessAead(sessionRef,
        aead, sizeof(aead)/2), "Test setting AEAD for part 1.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionProcessAead(sessionRef,
        aead + sizeof(aead)/2, sizeof(aead)/2), "Test setting AEAD for part 2.");

    LE_TEST_INFO("plainText message (size = %"PRIuS"): %s", sizeof(plainText), plainText);
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         plainText,
                                         sizeof(plainText)/2,
                                         encryptedData,
                                         &encryptedDataSize);
    encSize1 = encryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test encrypted size = %"PRIuS" for part 1.", encSize1);
    encryptedDataSize = sizeof(encryptedData) - encSize1;
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                  plainText + sizeof(plainText)/2,
                                  sizeof(plainText)/2,
                                  encryptedData + encSize1,
                                  &encryptedDataSize);
    encSize2 = encryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test encrypted size = %"PRIuS" for part 2.", encSize2);
    encryptedDataSize = sizeof(encryptedData) - encSize1 -encSize2;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     encryptedData + encSize1 + encSize2,
                                     &encryptedDataSize);
    totalSize = encryptedDataSize + encSize1 + encSize2;
    LE_TEST_ASSERT(LE_OK == result, "Test encrypted total size = %"PRIuS".", totalSize);

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test crypto session creation for decryption.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionSetAesNonce(sessionRef, nonce, sizeof(nonce)),
                   "Test setting nonce.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_DECRYPT),
                   "Test start session for data decryption.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionProcessAead(sessionRef, aead, sizeof(aead)),
                   "Test setting AEAD.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                  encryptedData,
                                  totalSize,
                                  decryptedData,
                                  &decryptedDataSize);
    decSize = decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted size = %"PRIuS"", decSize);
    decryptedDataSize = sizeof(decryptedData) - decSize;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     decryptedData + decSize,
                                     &decryptedDataSize);
    totalSize = decSize + decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted total size = %"PRIuS".", totalSize);
    LE_TEST_ASSERT(totalSize == sizeof(plainText), "Test data size verification.");
    errCode = memcmp(plainText, decryptedData, totalSize);
    LE_TEST_ASSERT(errCode == 0, "Test decrypted data verification.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Tests for RSA encryption/decryption.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void RsaEncTest(void)
{
    le_result_t result;
    size_t encSize, decSize, totalSize;
    const char keyId1[] = "rsaEncShortMessageTest";
    const char keyId2[] = "rsaEncLongMessageTest";
    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;
    const uint8_t plainText[] = "Keystore service RSA Encryption Descryption test message.";
    const uint8_t longPlainText[480] = { 0 }; // must be less than key size.
    // message test buffer.
    uint8_t encryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    uint8_t decryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t encryptedDataSize = sizeof(encryptedData);
    size_t decryptedDataSize = sizeof(decryptedData);

    //---------------------------------------------------------------------------------------------
    //--------------Short message test.------------------------------------------------------------
    //---------------------------------------------------------------------------------------------

    // -------------Start encryption---------------------------------------------------------------
    if (LE_NOT_FOUND == taf_ks_GetKey(keyId1, &keyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId1, TAF_KS_RSA_ENCRYPT_DECRYPT, &keyRef),
                       "Test RSA encryption key creation");
        LE_TEST_ASSERT(LE_NOT_PERMITTED == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                       "Test session creation before key provision.");
        result = taf_ks_ProvisionRsaEncKeyValue(keyRef,
                                                TAF_KS_RSA_SIZE_4096,
                                                TAF_KS_RSA_ENC_PAD_OAEP_SHA2_512,
                                                NULL, 0);
        LE_TEST_ASSERT(LE_OK == result, "Test RSA encryption key value provision.");
    }

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         plainText,
                                         sizeof(plainText),
                                         encryptedData,
                                         &encryptedDataSize);
    LE_TEST_ASSERT(LE_NOT_PERMITTED == result, "Test process session before session start.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_ENCRYPT),
                   "Test start session for data encryption.");
    LE_TEST_ASSERT(LE_DUPLICATE == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_ENCRYPT),
                   "Test start session again.");
    LE_TEST_INFO("Short plainText message (size = %"PRIuS"): %s", sizeof(plainText), plainText);
    encryptedDataSize = sizeof(encryptedData);
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         plainText,
                                         sizeof(plainText),
                                         encryptedData,
                                         &encryptedDataSize);
    encSize = encryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test encrypted data size = %"PRIuS".", encSize);
    encryptedDataSize = sizeof(encryptedData) - encSize;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     encryptedData + encSize,
                                     &encryptedDataSize);
    totalSize = encryptedDataSize + encSize;
    LE_TEST_ASSERT(LE_OK == result,
                   "Test short message encrypted data total size = %"PRIuS".", totalSize);

    // ----------- Start decryption--------------------------------------------------------------
    LE_TEST_ASSERT(LE_OK == taf_ks_GetKey(keyId1, &keyRef), "Test Geting the key.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_DECRYPT),
                   "Test start session for data decryption.");
    decryptedDataSize = sizeof(decryptedData);
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                  encryptedData,
                                  totalSize,
                                  decryptedData,
                                  &decryptedDataSize);
    decSize = decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted size = %"PRIuS"", decSize);
    decryptedDataSize = sizeof(decryptedData) - decSize;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     decryptedData + decSize,
                                     &decryptedDataSize);
    totalSize = decSize + decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result,
                   "Test short message decrypted total size = %"PRIuS".", totalSize);
    LE_TEST_ASSERT(totalSize == sizeof(plainText), "Test short message data size verification.");
    LE_TEST_ASSERT(0 == memcmp(plainText, decryptedData, totalSize),
                   "Test short message decrypted data verification.");

    //----------------------------------------------------------------------------------------------
    //--------------Long message test.-------------------------------------------------------------
    //----------------------------------------------------------------------------------------------

    // -------------Start encryption---------------------------------------------------------------
    if (LE_NOT_FOUND == taf_ks_GetKey(keyId2, &keyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId2, TAF_KS_RSA_ENCRYPT_DECRYPT, &keyRef),
                       "Test RSA encryption key creation");
        result = taf_ks_ProvisionRsaEncKeyValue(keyRef,
                                                TAF_KS_RSA_SIZE_4096,
                                                TAF_KS_RSA_ENC_PAD_PKCS1_V15,
                                                NULL, 0);
        LE_TEST_ASSERT(LE_OK == result, "Test RSA encryption key value provision.");
    }

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_ENCRYPT),
                   "Test start session for data encryption.");
    LE_TEST_INFO("Long plainText message (size = %"PRIuS")", sizeof(longPlainText));
    encryptedDataSize = sizeof(encryptedData);
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         longPlainText,
                                         240,
                                         encryptedData,
                                         &encryptedDataSize);
    encSize = encryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test encrypted data size = %"PRIuS".", encSize);
    encryptedDataSize = sizeof(encryptedData) - encSize;
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         longPlainText+240,
                                         240,
                                         encryptedData + encSize,
                                         &encryptedDataSize);
    encSize += encryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test encrypted data size = %"PRIuS".", encSize);
    encryptedDataSize = sizeof(encryptedData) - encSize;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     encryptedData + encSize,
                                     &encryptedDataSize);
    totalSize = encryptedDataSize + encSize;
    LE_TEST_ASSERT(LE_OK == result,
                   "Test long message encrypted data total size = %"PRIuS".", totalSize);

    // ----------- Start decryption--------------------------------------------------------------
    LE_TEST_ASSERT(LE_OK == taf_ks_GetKey(keyId2, &keyRef), "Test Geting the key.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_DECRYPT),
                   "Test start session for data decryption.");
    decryptedDataSize = sizeof(decryptedData);
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                  encryptedData,
                                  totalSize/2,
                                  decryptedData,
                                  &decryptedDataSize);
    decSize = decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted size = %"PRIuS"", decSize);
    decryptedDataSize = sizeof(decryptedData) - decSize;
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                  encryptedData+totalSize/2,
                                  totalSize/2,
                                  decryptedData + decSize,
                                  &decryptedDataSize);
    decSize += decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted size = %"PRIuS"", decSize);
    decryptedDataSize = sizeof(decryptedData) - decSize;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     decryptedData + decSize,
                                     &decryptedDataSize);
    totalSize = decSize + decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result,
                   "Test long message decrypted total size = %"PRIuS".", totalSize);
    LE_TEST_ASSERT(totalSize == sizeof(longPlainText), "Test long message data size verification.");
    LE_TEST_ASSERT(0 == memcmp(longPlainText, decryptedData, totalSize),
                   "Test long message decrypted data verification.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests for RSA signing/verification.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void RsaSigTest(void)
{
    le_result_t result;
    const char keyId[] = "rsaSigTest";
    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;
    const uint8_t message[TAF_KS_MAX_PACKET_SIZE/2] =
        "Keystore service RSA Signing Verification test message.";
    uint8_t signature[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t signatureSize = sizeof(signature);

    // -------------Start signing------------------------------------------------------------------

    if (LE_NOT_FOUND == taf_ks_GetKey(keyId, &keyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId, TAF_KS_RSA_SIGN_VERIFY, &keyRef),
                       "Test RSA signing key creation");
        result = taf_ks_ProvisionRsaSigKeyValue(keyRef,
                                                TAF_KS_RSA_SIZE_1024,
                                                TAF_KS_RSA_SIG_PAD_PSS_SHA2_256,
                                                NULL, 0);
        LE_TEST_ASSERT(LE_OK == result, "Test RSA signing key value provision.");
    }

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_SIGN),
                   "Test start session for data signing.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         message,
                                         sizeof(message),
                                         NULL, 0);
    LE_TEST_ASSERT(LE_OK == result,
                   "Message to sign(size = %"PRIuS"): %s", sizeof(message), message);
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     signature,
                                     &signatureSize);
    LE_TEST_ASSERT(LE_OK == result, "Test signature size = %"PRIuS".", signatureSize);

    // -------------Start verification-------------------------------------------------------------

    // Test verification use the correct signature.
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_VERIFY),
                   "Test start session for data verification.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         message,
                                         sizeof(message),
                                         NULL, 0);
    LE_TEST_ASSERT(LE_OK == result,
                   "Message to verify(size = %"PRIuS"): %s", sizeof(message), message);
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     signature,
                                     signatureSize,
                                     NULL, 0);
    LE_TEST_ASSERT(LE_OK == result, "Test data verification using the correct signature.");

    // Test verifycation use a wrong signature.
    memset(signature, 0x1, sizeof(signature));
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_VERIFY),
                   "Test start session for data verification.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         message,
                                         sizeof(message),
                                         NULL, 0);
    LE_TEST_ASSERT(LE_OK == result,
                   "Message to verify(size = %"PRIuS"): %s", sizeof(message), message);
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     signature,
                                     signatureSize,
                                     NULL, 0);
    LE_TEST_ASSERT(LE_FAULT == result, "Test data verification using a wrong signature.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests for AES ECB encryption/decryption.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void AesEcbTest(void)
{
    le_result_t result;
    int32_t errCode;
    size_t encSize, decSize, totalSize, totalSize1;
    const char keyId[] = "aesEcbTest";
    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;
    const uint8_t plainText[TAF_KS_MAX_PACKET_SIZE/2] = { 0 };
    uint8_t encryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t encryptedDataSize = sizeof(encryptedData);
    uint8_t decryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t decryptedDataSize = sizeof(decryptedData);

    // -------------Start encryption---------------------------------------------------------------

    if (LE_NOT_FOUND == taf_ks_GetKey(keyId, &keyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId,TAF_KS_AES_ENCRYPT_DECRYPT, &keyRef),
                       "Test AES ECB key creation.");
        result = taf_ks_ProvisionAesKeyValue(keyRef,
                                             TAF_KS_AES_SIZE_256,
                                             TAF_KS_AES_MODE_ECB_PAD_PKCS7,
                                             NULL, 0);
        LE_TEST_ASSERT(LE_OK == result, "Test AES ECB key value provision.");
    }

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_ENCRYPT),
                   "Test start session for data encryption.");
    LE_TEST_INFO("plainText message (size = %"PRIuS")", sizeof(plainText));
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         plainText,
                                         sizeof(plainText),
                                         encryptedData,
                                         &encryptedDataSize);
    encSize = encryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test encrypted size = %"PRIuS".", encSize);
    encryptedDataSize = sizeof(encryptedData) - encSize;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     encryptedData + encSize,
                                     &encryptedDataSize);
    totalSize = encryptedDataSize + encSize;
    totalSize1 = totalSize;
    LE_TEST_ASSERT(LE_OK == result, "Test encrypted total size = %"PRIuS".", totalSize);

    // -------------Start decryption---------------------------------------------------------------

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_DECRYPT),
                   "Test start session for data decryption.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         encryptedData,
                                         totalSize,
                                         decryptedData,
                                         &decryptedDataSize);
    decSize = decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted size = %"PRIuS"", decSize);
    decryptedDataSize = sizeof(decryptedData) - decSize;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     decryptedData + decSize,
                                     &decryptedDataSize);
    totalSize = decSize + decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted total size = %"PRIuS".", totalSize);
    LE_TEST_ASSERT(totalSize == sizeof(plainText), "Test data size verification.");
    errCode = memcmp(plainText, decryptedData, totalSize);
    LE_TEST_ASSERT(errCode == 0, "Test decrypted data verification.");

    // Test data decryption with wrong encrpted data.
    memset(encryptedData, 1, sizeof(encryptedData));
    decryptedDataSize = sizeof(decryptedData);
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_DECRYPT),
                   "Test start session for wrong data decryption.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         encryptedData,
                                         totalSize1,
                                         decryptedData,
                                         &decryptedDataSize);
    decSize = decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted size = %"PRIuS"", decSize);
    decryptedDataSize = sizeof(decryptedData) - decSize;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     decryptedData + decSize,
                                     &decryptedDataSize);
    totalSize = decSize + decryptedDataSize;
    LE_TEST_ASSERT(LE_FAULT == result, "Test wrong data decryption");
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests for AES CBC encryption/decryption.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void AesCbcTest(void)
{
    le_result_t result;
    int32_t errCode;
    size_t encSize, decSize, totalSize, totalSize1;
    const char keyId[] = "aesCbcTest";
    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;
    const uint8_t plainText[TAF_KS_MAX_PACKET_SIZE/2] = { 0 };
    uint8_t encryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t encryptedDataSize = sizeof(encryptedData);
    uint8_t decryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t decryptedDataSize = sizeof(decryptedData);
    uint8_t iv[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
                      0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10};

    // -------------Start encryption---------------------------------------------------------------

    if (LE_NOT_FOUND == taf_ks_GetKey(keyId, &keyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId,TAF_KS_AES_ENCRYPT_DECRYPT, &keyRef),
                       "Test AES CBC key creation.");
        result = taf_ks_ProvisionAesKeyValue(keyRef,
                                             TAF_KS_AES_SIZE_128,
                                             TAF_KS_AES_MODE_CBC_PAD_NONE,
                                             NULL, 0);
        LE_TEST_ASSERT(LE_OK == result, "Test AES CBC key value provision.");
    }

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionSetAesNonce(sessionRef, iv, sizeof(iv)),
                   "Test seting nonce.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_ENCRYPT),
                   "Test start session for data encryption.");
    LE_TEST_INFO("plainText message (size = %"PRIuS")", sizeof(plainText));
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         plainText,
                                         sizeof(plainText),
                                         encryptedData,
                                         &encryptedDataSize);
    encSize = encryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test encrypted size = %"PRIuS".", encSize);
    encryptedDataSize = sizeof(encryptedData) - encSize;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     encryptedData + encSize,
                                     &encryptedDataSize);
    totalSize = encryptedDataSize + encSize;
    totalSize1 = totalSize;
    LE_TEST_ASSERT(LE_OK == result, "Test encrypted total size = %"PRIuS".", totalSize);

    // -------------Start decryption---------------------------------------------------------------

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionSetAesNonce(sessionRef, iv, sizeof(iv)),
                   "Test seting nonce.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_DECRYPT),
                   "Test start session for data decryption.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         encryptedData,
                                         totalSize,
                                         decryptedData,
                                         &decryptedDataSize);
    decSize = decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted size = %"PRIuS"", decSize);
    decryptedDataSize = sizeof(decryptedData) - decSize;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     decryptedData + decSize,
                                     &decryptedDataSize);
    totalSize = decSize + decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted total size = %"PRIuS".", totalSize);
    LE_TEST_ASSERT(totalSize == sizeof(plainText), "Test data size verification.");
    errCode = memcmp(plainText, decryptedData, totalSize);
    LE_TEST_ASSERT(errCode == 0, "Test decrypted data verification.");

    // Test data decryption with wrong iv.
    iv[0] = 0;
    decryptedDataSize = sizeof(decryptedData);
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionSetAesNonce(sessionRef, iv, sizeof(iv)),
                   "Test seting nonce.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_DECRYPT),
                   "Test start session for wrong iv data decryption.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         encryptedData,
                                         totalSize1,
                                         decryptedData,
                                         &decryptedDataSize);
    decSize = decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted size = %"PRIuS"", decSize);
    decryptedDataSize = sizeof(decryptedData) - decSize;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     decryptedData + decSize,
                                     &decryptedDataSize);
    totalSize = decSize + decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test wrong iv data decryption");
    LE_TEST_ASSERT(totalSize == sizeof(plainText), "Test data size verification.");
    errCode = memcmp(plainText, decryptedData, totalSize);
    LE_TEST_ASSERT(errCode != 0, "Test wrong iv decrypted data verification.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Import an AES CTR key and tests data encryption/decryption.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void AesCtrTest(void)
{
    le_result_t result;
    int32_t errCode;
    size_t encSize, decSize, totalSize, totalSize1;
    const char keyId[] = "aesCtrTest";
    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;
    //uint8_t  tmpBuf[CHUNK_SIZE] = { 0 };
    const uint8_t plainText[TAF_KS_MAX_PACKET_SIZE/2] = { 0 };
    uint8_t encryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t encryptedDataSize = sizeof(encryptedData);
    uint8_t decryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t decryptedDataSize = sizeof(decryptedData);
    uint8_t iv[16] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
                      0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x00};
    uint8_t AES_KEY[16] = {0xa3,0x3e,0x01,0x89,0xb6,0xbb,0x75,0x3c,
                           0xf1,0x20,0x72,0xc7,0x0e,0xe5,0xc5,0xb5};

    // -------------Start encryption---------------------------------------------------------------

    if (LE_NOT_FOUND == taf_ks_GetKey(keyId, &keyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId,TAF_KS_AES_ENCRYPT_DECRYPT, &keyRef),
                       "Test AES CTR key creation.");
        result = taf_ks_ProvisionAesKeyValue(keyRef,
                                             TAF_KS_AES_SIZE_128,
                                             TAF_KS_AES_MODE_CTR,
                                             AES_KEY, 16);
        LE_TEST_ASSERT(LE_OK == result, "Test AES CTR import key value.");
    }

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionSetAesNonce(sessionRef, iv, sizeof(iv)),
                   "Test seting nonce.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_ENCRYPT),
                   "Test start session for data encryption.");
    LE_TEST_INFO("plainText message (size = %"PRIuS")", sizeof(plainText));
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         plainText,
                                         sizeof(plainText),
                                         encryptedData,
                                         &encryptedDataSize);
    encSize = encryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test encrypted size = %"PRIuS".", encSize);
    encryptedDataSize = sizeof(encryptedData) - encSize;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     encryptedData + encSize,
                                     &encryptedDataSize);
    totalSize = encryptedDataSize + encSize;
    totalSize1 = totalSize;
    LE_TEST_ASSERT(LE_OK == result, "Test encrypted total size = %"PRIuS".", totalSize);

    // -------------Start decryption---------------------------------------------------------------

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionSetAesNonce(sessionRef, iv, sizeof(iv)),
                   "Test seting nonce.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_DECRYPT),
                   "Test start session for data decryption.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         encryptedData,
                                         totalSize,
                                         decryptedData,
                                         &decryptedDataSize);
    decSize = decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted size = %"PRIuS"", decSize);
    decryptedDataSize = sizeof(decryptedData) - decSize;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     decryptedData + decSize,
                                     &decryptedDataSize);
    totalSize = decSize + decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted total size = %"PRIuS".", totalSize);
    LE_TEST_ASSERT(totalSize == sizeof(plainText), "Test data size verification.");
    errCode = memcmp(plainText, decryptedData, totalSize);
    LE_TEST_ASSERT(errCode == 0, "Test decrypted data verification.");

    // Test data decryption with wrong iv.
    iv[0] = 0;
    decryptedDataSize = sizeof(decryptedData);
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionSetAesNonce(sessionRef, iv, sizeof(iv)),
                   "Test seting nonce.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_DECRYPT),
                   "Test start session for wrong iv data decryption.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         encryptedData,
                                         totalSize1,
                                         decryptedData,
                                         &decryptedDataSize);
    decSize = decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted size = %"PRIuS"", decSize);
    decryptedDataSize = sizeof(decryptedData) - decSize;
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     decryptedData + decSize,
                                     &decryptedDataSize);
    totalSize = decSize + decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test wrong iv data decryption");
    LE_TEST_ASSERT(totalSize == sizeof(plainText), "Test data size verification.");
    errCode = memcmp(plainText, decryptedData, totalSize);
    LE_TEST_ASSERT(errCode != 0, "Test wrong iv decrypted data verification.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Import an AES CBC key and tests data file encryption.
 */
//--------------------------------------------------------------------------------------------------
#define CHUNK_SIZE (TAF_KS_MAX_PACKET_SIZE/2)
__attribute__((unused)) static void EncDataFileTest(void)
{
    le_result_t result;
    const char keyId[] = "aesCbcEncFileTest";
    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;

    // Encrypt data from an file ,here is /tmp/plaintext.
    // And output the encrypted data into file /tmp/encrypted.
    char plaintextFilePath[64] = "/tmp/plaintext";
    char encryptedFilePath[64] = "/tmp/encrypted";
    uint8_t encryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t encryptedDataSize = sizeof(encryptedData);
    uint8_t tmpBuf[CHUNK_SIZE] = {0};
    struct stat fileStat = {0};
    uint32_t fileSize = 0;
    ssize_t readCnt = 0;
    int ifd = -1;
    int ofd = -1;
    // IV = "112233445566778899AABBCCDDEEFF00"
    uint8_t iv[16] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
                      0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x00};
    // KEY = "0123456789ABCDEFFEDCBA9876543210"
    uint8_t AES_KEY[16] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
                           0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10};

    if (LE_NOT_FOUND == taf_ks_GetKey(keyId, &keyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId,TAF_KS_AES_ENCRYPT_DECRYPT, &keyRef),
                       "Test AES CBC import key creation.");
        result = taf_ks_ProvisionAesKeyValue(keyRef,
                                             TAF_KS_AES_SIZE_128,
                                             TAF_KS_AES_MODE_CBC_PAD_PKCS7,
                                             AES_KEY, 16);
        LE_TEST_ASSERT(LE_OK == result, "Test AES CBC import key value .");
    }

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionSetAesNonce(sessionRef, iv, sizeof(iv)),
                   "Test seting nonce.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_ENCRYPT),
                   "Test start session for data encryption.");

    // Check if input data file exists.
    if (!file_Exists(plaintextFilePath))
    {
        LE_TEST_INFO("No data file '%s', skip the encrypt data file test!", plaintextFilePath);
        return;
    }

    // Open the input data file.
    ifd = le_fd_Open(plaintextFilePath, O_RDONLY);
    if (ifd < 0)
    {
        LE_ERROR("Failed to open data file '%s'.", plaintextFilePath);
        return;
    }

    // Check the input data file size.
    fstat(ifd, &fileStat);
    fileSize = fileStat.st_size;
    if (fileSize == 0)
    {
        le_fd_Close(ifd);
        return;
    }

    // Open the output data file.
    ofd = open(encryptedFilePath, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
    if (ofd < 0)
    {
        LE_ERROR("Failed to open data file '%s'.", encryptedFilePath);
        le_fd_Close(ifd);
        return;
    }

    // Encrypt the input data file, and write the encrypted data into output file.
    LE_TEST_INFO("Start to encrypt data file '%s' (size = %u).", plaintextFilePath, fileSize);

    if (fileSize <= CHUNK_SIZE)
    {
        readCnt = le_fd_Read(ifd, tmpBuf, fileSize);
        if (readCnt != fileSize)
        {
            LE_ERROR("Error (%s) reading data file '%s'.", LE_ERRNO_TXT(errno), plaintextFilePath);
            le_fd_Close(ifd);
            le_fd_Close(ofd);
            return;
        }

        // Encrypt the data chunk.
        encryptedDataSize = sizeof(encryptedData);
        result = taf_ks_CryptoSessionProcess(sessionRef,
                                             tmpBuf,
                                             readCnt,
                                             encryptedData,
                                             &encryptedDataSize);
        LE_TEST_ASSERT(LE_OK == result, "Get encrypted data size = %"PRIuS".", encryptedDataSize);
        if (encryptedDataSize > 0)
        {
            if (le_fd_Write(ofd, encryptedData, encryptedDataSize) != encryptedDataSize)
            {
                LE_ERROR("Failed to write encrypted data to file '%s'.", encryptedFilePath);
                le_fd_Close(ifd);
                le_fd_Close(ofd);
                return;
            }
            LE_INFO("Write encrypted data size = %"PRIuS".", encryptedDataSize);
        }

        encryptedDataSize = sizeof(encryptedData);
        result = taf_ks_CryptoSessionEnd(sessionRef,
                                         NULL, 0,
                                         encryptedData,
                                         &encryptedDataSize);
        LE_TEST_ASSERT(LE_OK == result, "Get encrypted data size = %"PRIuS".", encryptedDataSize);
        if (encryptedDataSize > 0)
        {
            if (le_fd_Write(ofd, encryptedData, encryptedDataSize) != encryptedDataSize)
            {
                LE_ERROR("Failed to write encrypted data to file '%s'.", encryptedFilePath);
                le_fd_Close(ifd);
                le_fd_Close(ofd);
                return;
            }
            LE_INFO("Write encrypted data size = %"PRIuS".", encryptedDataSize);
        }

    }
    else
    {
        int i = 0;
        do
        {
            size_t currentSize = fileSize - (i * CHUNK_SIZE);
            if (currentSize > CHUNK_SIZE)
            {
                currentSize = CHUNK_SIZE;
            }

            readCnt = le_fd_Read(ifd, tmpBuf, currentSize);
            if (readCnt != currentSize)
            {
                LE_ERROR("Error (%s) reading data file '%s'.",
                         LE_ERRNO_TXT(errno), plaintextFilePath);
                le_fd_Close(ifd);
                le_fd_Close(ofd);
                return;
            }

            // Encrypt the data chunk.
            encryptedDataSize = sizeof(encryptedData);
            result = taf_ks_CryptoSessionProcess(sessionRef,
                                                 tmpBuf,
                                                 readCnt,
                                                 encryptedData,
                                                 &encryptedDataSize);
            LE_TEST_ASSERT(LE_OK == result, "Get encrypted data size = %"PRIuS".",
                           encryptedDataSize);
            if (encryptedDataSize > 0)
            {
                if (le_fd_Write(ofd, encryptedData, encryptedDataSize) != encryptedDataSize)
                {
                    LE_ERROR("Failed to write encrypted data to file '%s'.", encryptedFilePath);
                    le_fd_Close(ifd);
                    le_fd_Close(ofd);
                    return;
                }
                LE_INFO("Write encrypted data size = %"PRIuS".", encryptedDataSize);
            }

            i++;
        }
        while ((i * CHUNK_SIZE) < fileSize);

        encryptedDataSize = sizeof(encryptedData);
        result = taf_ks_CryptoSessionEnd(sessionRef,
                                         NULL, 0,
                                         encryptedData,
                                         &encryptedDataSize);
        LE_TEST_ASSERT(LE_OK == result, "Get encrypted data size = %"PRIuS".",
                       encryptedDataSize);
        if (encryptedDataSize > 0)
        {
            if (le_fd_Write(ofd, encryptedData, encryptedDataSize) != encryptedDataSize)
            {
                LE_ERROR("Failed to write encrypted data to file '%s'.", encryptedFilePath);
                le_fd_Close(ifd);
                le_fd_Close(ofd);
                return;
            }
            LE_INFO("Write encrypted data size = %"PRIuS".", encryptedDataSize);
        }
    }
    le_fd_Close(ifd);
    le_fd_Close(ofd);

    LE_TEST_INFO("Data file '%s' encryption done.", plaintextFilePath);
}

//--------------------------------------------------------------------------------------------------
/**
 * Import an AES CBC key and tests data file decryption.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void DecDataFileTest(void)
{
    le_result_t result;
    const char keyId[] = "aesCbcDecFileTest";
    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;

    // Decrypt data from an file, here is /tmp/ciphertext.
    // And output the decrypted data into file /tmp/decrypted.
    char ciphertextFilePath[64] = "/tmp/ciphertext";
    char decryptedFilePath[64] = "/tmp/decrypted";
    uint8_t decryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t decryptedDataSize = sizeof(decryptedData);
    uint8_t tmpBuf[CHUNK_SIZE] = {0};
    struct stat fileStat = {0};
    uint32_t fileSize = 0;
    ssize_t readCnt = 0;
    int ifd = -1;
    int ofd = -1;
    // IV = "00FFEEDDCCBBAA998877665544332211"
    uint8_t iv[16] = {0x00,0xFF,0xEE,0xDD,0xCC,0xBB,0xAA,0x99,
                      0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11};

    // KEY = "1032547698BADCFEEFCDAB8967452301"
    uint8_t AES_KEY[16] = {0x10,0x32,0x54,0x76,0x98,0xBA,0xDC,0xFE,
                           0xEF,0xCD,0xAB,0x89,0x67,0x45,0x23,0x01};

    if (LE_NOT_FOUND == taf_ks_GetKey(keyId, &keyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId,TAF_KS_AES_ENCRYPT_DECRYPT, &keyRef),
                       "Test AES CBC import key creation.");
        result = taf_ks_ProvisionAesKeyValue(keyRef,
                                             TAF_KS_AES_SIZE_128,
                                             TAF_KS_AES_MODE_CBC_PAD_PKCS7,
                                             AES_KEY, 16);
        LE_TEST_ASSERT(LE_OK == result, "Test AES CBC import key value .");
    }

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionSetAesNonce(sessionRef, iv, sizeof(iv)),
                   "Test seting nonce.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_DECRYPT),
                   "Test start session for data decryption.");

    // Check if input data file exists.
    if (!file_Exists(ciphertextFilePath))
    {
        LE_TEST_INFO("No data file '%s', skip the decrypt data file test!", ciphertextFilePath);
        return;
    }

    // Open the input data file.
    ifd = le_fd_Open(ciphertextFilePath, O_RDONLY);
    if (ifd < 0)
    {
        LE_ERROR("Failed to open data file '%s'.", ciphertextFilePath);
        return;
    }

    // Check the input data file size.
    fstat(ifd, &fileStat);
    fileSize = fileStat.st_size;
    if (fileSize == 0)
    {
        le_fd_Close(ifd);
        return;
    }

    // Open the output data file.
    ofd = open(decryptedFilePath, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
    if (ofd < 0)
    {
        LE_ERROR("Failed to open data file '%s'.", decryptedFilePath);
        le_fd_Close(ifd);
        return;
    }

    // Decrypt the input data file, and write the decrypted data into output file.
    LE_TEST_INFO("Start to decrypt data file '%s' (size = %u).", ciphertextFilePath, fileSize);

    if (fileSize <= CHUNK_SIZE)
    {
        readCnt = le_fd_Read(ifd, tmpBuf, fileSize);
        if (readCnt != fileSize)
        {
            LE_ERROR("Error (%s) reading data file '%s'.", LE_ERRNO_TXT(errno), ciphertextFilePath);
            le_fd_Close(ifd);
            le_fd_Close(ofd);
            return;
        }

        // Decrypt the data chunk.
        decryptedDataSize = sizeof(decryptedData);
        result = taf_ks_CryptoSessionProcess(sessionRef,
                                             tmpBuf,
                                             readCnt,
                                             decryptedData,
                                             &decryptedDataSize);
        LE_TEST_ASSERT(LE_OK == result, "Get decrypted data size = %"PRIuS".", decryptedDataSize);
        if (decryptedDataSize > 0)
        {
            if (le_fd_Write(ofd, decryptedData, decryptedDataSize) != decryptedDataSize)
            {
                LE_ERROR("Failed to write decrypted data to file '%s'.", decryptedFilePath);
                le_fd_Close(ifd);
                le_fd_Close(ofd);
                return;
            }
            LE_INFO("Write decrypted data size = %"PRIuS".", decryptedDataSize);
        }

        decryptedDataSize = sizeof(decryptedData);
        result = taf_ks_CryptoSessionEnd(sessionRef,
                                         NULL, 0,
                                         decryptedData,
                                         &decryptedDataSize);
        LE_TEST_ASSERT(LE_OK == result, "Get decrypted data size = %"PRIuS".", decryptedDataSize);
        if (decryptedDataSize > 0)
        {
            if (le_fd_Write(ofd, decryptedData, decryptedDataSize) != decryptedDataSize)
            {
                LE_ERROR("Failed to write decrypted data to file '%s'.", decryptedFilePath);
                le_fd_Close(ifd);
                le_fd_Close(ofd);
                return;
            }
            LE_INFO("Write decrypted data size = %"PRIuS".", decryptedDataSize);
        }

    }
    else
    {
        int i = 0;
        do
        {
            size_t currentSize = fileSize - (i * CHUNK_SIZE);
            if (currentSize > CHUNK_SIZE)
            {
                currentSize = CHUNK_SIZE;
            }

            readCnt = le_fd_Read(ifd, tmpBuf, currentSize);
            if (readCnt != currentSize)
            {
                LE_ERROR("Error (%s) reading data file '%s'.",
                         LE_ERRNO_TXT(errno), ciphertextFilePath);
                le_fd_Close(ifd);
                le_fd_Close(ofd);
                return;
            }

            // Encrypt the data chunk.
            decryptedDataSize = sizeof(decryptedData);
            result = taf_ks_CryptoSessionProcess(sessionRef,
                                                 tmpBuf,
                                                 readCnt,
                                                 decryptedData,
                                                 &decryptedDataSize);
            LE_TEST_ASSERT(LE_OK == result, "Get decrypted data size = %"PRIuS".",
                           decryptedDataSize);
            if (decryptedDataSize > 0)
            {
                if (le_fd_Write(ofd, decryptedData, decryptedDataSize) != decryptedDataSize)
                {
                    LE_ERROR("Failed to write decrypted data to file '%s'.", decryptedFilePath);
                    le_fd_Close(ifd);
                    le_fd_Close(ofd);
                    return;
                }
                LE_INFO("Write decrypted data size = %"PRIuS".", decryptedDataSize);
            }

            i++;
        }
        while ((i * CHUNK_SIZE) < fileSize);

        decryptedDataSize = sizeof(decryptedData);
        result = taf_ks_CryptoSessionEnd(sessionRef,
                                         NULL, 0,
                                         decryptedData,
                                         &decryptedDataSize);
        LE_TEST_ASSERT(LE_OK == result, "Get decrypted data size = %"PRIuS".",
                       decryptedDataSize);
        if (decryptedDataSize > 0)
        {
            if (le_fd_Write(ofd, decryptedData, decryptedDataSize) != decryptedDataSize)
            {
                LE_ERROR("Failed to write decrypted data to file '%s'.", decryptedFilePath);
                le_fd_Close(ifd);
                le_fd_Close(ofd);
                return;
            }
            LE_INFO("Write decrypted data size = %"PRIuS".", decryptedDataSize);
        }
    }
    le_fd_Close(ifd);
    le_fd_Close(ofd);

    LE_TEST_INFO("Data file '%s' decryption done.", ciphertextFilePath);
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests for ECDSA signing/verification.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void EcdsaSigTest(void)
{
    le_result_t result;
    const char keyId[] = "ecdsaTest";
    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;
    const uint8_t message[TAF_KS_MAX_PACKET_SIZE/2] =
        "Keystore service ECDSA Signing Verification test message.";
    uint8_t signature[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t signatureSize = sizeof(signature);

    // -------------Start signing------------------------------------------------------------------
    if (LE_NOT_FOUND == taf_ks_GetKey(keyId, &keyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId, TAF_KS_ECDSA_SIGN_VERIFY, &keyRef),
                       "Test ECDSA key creation");
        result = taf_ks_ProvisionEcdsaKeyValue(keyRef,
                                               TAF_KS_ECC_SIZE_521,
                                               TAF_KS_DIGEST_SHA2_512,
                                               NULL, 0);
        LE_TEST_ASSERT(LE_OK == result, "Test ECDSA key value provision.");
    }

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_SIGN),
                   "Test start session for data signing.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         message,
                                         sizeof(message),
                                         NULL, 0);
    LE_TEST_ASSERT(LE_OK == result,
                   "Message to sign(size = %"PRIuS"): %s", sizeof(message), message);
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     signature,
                                     &signatureSize);
    LE_TEST_ASSERT(LE_OK == result, "Test signature size = %"PRIuS".", signatureSize);

    // -------------Start verification-------------------------------------------------------------

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_VERIFY),
                   "Test start session for data verification.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         message,
                                         sizeof(message),
                                         NULL, 0);
    LE_TEST_ASSERT(LE_OK == result,
                   "Message to verify(size = %"PRIuS"): %s", sizeof(message), message);
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     signature,
                                     signatureSize,
                                     NULL, 0);
    LE_TEST_ASSERT(LE_OK == result, "Test data verification using correct signature.");

    // Test verifycation use a wrong signature.
    memset(signature, 0x1, sizeof(signature));
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_VERIFY),
                   "Test start session for data verification.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         message,
                                         sizeof(message),
                                         NULL, 0);
    LE_TEST_ASSERT(LE_OK == result,
                   "Message to verify(size = %"PRIuS"): %s", sizeof(message), message);
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     signature,
                                     signatureSize,
                                     NULL, 0);
    LE_TEST_ASSERT(LE_FAULT == result, "Test data verification using a wrong signature.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests for HMAC signing/verification.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void HmacSigTest(void)
{
    le_result_t result;
    const char keyId[] = "hmacTest";
    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;
    const uint8_t message[TAF_KS_MAX_PACKET_SIZE/2] =
        "Keystore service HMAC Signing Verification test message.";
    uint8_t signature[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t signatureSize = sizeof(signature);

    // -------------Start signing------------------------------------------------------------------
    if (LE_NOT_FOUND == taf_ks_GetKey(keyId, &keyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId, TAF_KS_HMAC_SIGN_VERIFY, &keyRef),
                       "Test HMAC key creation");
        result = taf_ks_ProvisionHmacKeyValue(keyRef,
                                              TAF_KS_MAX_HMAC_KEY_SIZE,
                                              TAF_KS_DIGEST_SHA2_256,
                                              NULL, 0);
        LE_TEST_ASSERT(LE_OK == result, "Test HMAC key value provision.");
    }

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_SIGN),
                   "Test start session for data signing.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         message,
                                         sizeof(message),
                                         NULL, 0);
    LE_TEST_ASSERT(LE_OK == result,
                   "Message to sign(size = %"PRIuS"): %s", sizeof(message), message);
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     signature,
                                     &signatureSize);
    LE_TEST_ASSERT(LE_OK == result, "Test signature size = %"PRIuS".", signatureSize);

    // -------------Start verification-------------------------------------------------------------

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_VERIFY),
                   "Test start session for data verification.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         message,
                                         sizeof(message),
                                         NULL, 0);
    LE_TEST_ASSERT(LE_OK == result,
                 "Message to verify(size = %"PRIuS"): %s", sizeof(message), message);
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     signature,
                                     signatureSize,
                                     NULL, 0);
    LE_TEST_ASSERT(LE_OK == result, "Test data verification using correct signature.");

    // Test verifycation use a wrong signature.
    memset(signature, 0x1, sizeof(signature));
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_VERIFY),
                   "Test start session for data verification.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         message,
                                         sizeof(message),
                                         NULL, 0);
    LE_TEST_ASSERT(LE_OK == result,
                   "Message to verify(size = %"PRIuS"): %s", sizeof(message), message);
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     signature,
                                     signatureSize,
                                     NULL, 0);
    LE_TEST_ASSERT(LE_FAULT == result, "Test data verification using a wrong signature.");
}

//--------------------------------------------------------------------------------------------------
/**
 * RSA key export test
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void RsaKeyExportTest(void)
{
    le_result_t result;
    const char keyId[] = "rsaExportKeySigningTest";
    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;
    uint8_t message[] = "Keystore service export RSA key Signing Verification test message.";
    uint8_t signature[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t signatureSize = sizeof(signature);
    uint8_t expKeyData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t expKeySize = sizeof(expKeyData);

    // Get a RSA signing key.
    if (LE_NOT_FOUND == taf_ks_GetKey(keyId, &keyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId, TAF_KS_RSA_SIGN_VERIFY, &keyRef),
                       "Test RSA export signing key creation");
        result = taf_ks_ProvisionRsaSigKeyValue(keyRef,
                                                TAF_KS_RSA_SIZE_1024,
                                                TAF_KS_RSA_SIG_PAD_PKCS1_V15_MD5,
                                                NULL, 0);
        LE_TEST_ASSERT(LE_OK == result, "Test RSA export signing key value provision.");
    }

    // Export the RSA pub key.
    result = taf_ks_ExportKey(keyRef, NULL, 0, expKeyData, &expKeySize);
    LE_TEST_ASSERT(LE_OK == result, "Test export RSA public key with X.509 format.");
    LE_INFO("x.509 public key size = %"PRIuS"", expKeySize);

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_SIGN),
                   "Test start session for data signing.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         message,
                                         sizeof(message),
                                         NULL, 0);
    LE_TEST_ASSERT(LE_OK == result,
                   "Message to sign(size = %"PRIuS"): %s", sizeof(message), message);
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     signature,
                                     &signatureSize);
    LE_TEST_ASSERT(LE_OK == result, "Test signature size = %"PRIuS".", signatureSize);

    // Verify with correct pub key.
    LE_TEST_ASSERT(LE_OK == OpensslVerifySignature(message, sizeof(message), expKeyData, expKeySize,
                   signature, signatureSize, DIGEST_MD5, PKCS_V15_PADDING),
                   "Verify with correct pub key.");

    // Verify with wrong pub key.
    memset(expKeyData, 0, sizeof(expKeyData));
    LE_TEST_ASSERT(LE_OK != OpensslVerifySignature(message, sizeof(message), expKeyData, expKeySize,
                   signature, signatureSize, DIGEST_MD5, PKCS_V15_PADDING),
                   "Verify with wrong pub key.");
}

//--------------------------------------------------------------------------------------------------
/**
 * RSA multiple padding test
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void RsaMultiPaddingTest(void)
{
    le_result_t result;
    const char keyId[] = "rsaMultiPaddingTest";
    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;
    uint8_t message[] = "Keystore service RSA multiple padding test message.";
    uint8_t signature[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t signatureSize = sizeof(signature);
    uint8_t expKeyData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t expKeySize = sizeof(expKeyData);

    // Get RSA multiple padding test key.
    if (LE_NOT_FOUND == taf_ks_GetKey(keyId, &keyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(keyId, TAF_KS_RSA_SIGN_VERIFY, &keyRef),
                       "Test RSA multiple padding key creation");
        result = taf_ks_ProvisionRsaSigKeyValue(keyRef,
                                                TAF_KS_RSA_SIZE_4096,
                                                TAF_KS_RSA_SIG_PAD_PKCS1_V15_AND_PSS_SHA2_512,
                                                NULL, 0);
        LE_TEST_ASSERT(LE_OK == result, "Test RSA multiple padding  key value provision.");
    }

    // Export the RSA pub key.
    result = taf_ks_ExportKey(keyRef, NULL, 0, expKeyData, &expKeySize);
    LE_TEST_ASSERT(LE_OK == result, "Test export RSA public key with X.509 format.");
    LE_INFO("x.509 public key size = %"PRIuS"", expKeySize);

    // Create a RSA-PSS signing session.
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test RSA-PSS session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionSetRsaPadding(sessionRef, TAF_KS_RSA_PSS),
                   "Test RSA-PSS padding.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_SIGN),
                   "Test start RSA-PSS session for data signing.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         message,
                                         sizeof(message),
                                         NULL, 0);
    LE_TEST_ASSERT(LE_OK == result,
                   "Message to RSA-PSS sign(size = %"PRIuS"): %s", sizeof(message), message);
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     signature,
                                     &signatureSize);
    LE_TEST_ASSERT(LE_OK == result, "Test RSA-PSS signature size = %"PRIuS".", signatureSize);

    // Verify with wrong padding.
    LE_TEST_ASSERT(LE_OK != OpensslVerifySignature(message, sizeof(message), expKeyData, expKeySize,
                   signature, signatureSize, DIGEST_SHA2_512, PKCS_V15_PADDING),
                   "Verify with wrong padding.");
    // Verify with wrong digest.
    LE_TEST_ASSERT(LE_OK != OpensslVerifySignature(message, sizeof(message), expKeyData, expKeySize,
                   signature, signatureSize, DIGEST_SHA2_256, PSS_PADDING),
                   "Verify with wrong digest.");
    // Verify with correct padding and digest.
    LE_TEST_ASSERT(LE_OK == OpensslVerifySignature(message, sizeof(message), expKeyData, expKeySize,
                   signature, signatureSize, DIGEST_SHA2_512, PSS_PADDING),
                   "Verify with correct padding and digest.");


    // Create a RSA-PCKS1-V1.5 signing session.
    memset(signature, 0, sizeof(signature));
    signatureSize = sizeof(signature);
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef),
                   "Test RSA-PKCS1_V15 session creation.");

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionSetRsaPadding(sessionRef, TAF_KS_RSA_PKCS1_V15),
                   "Test RSA-PKCS1_V15 padding.");

    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_SIGN),
                   "Test start RSA-PKCS1_V15 session for data signing.");
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         message,
                                         sizeof(message),
                                         NULL, 0);
    LE_TEST_ASSERT(LE_OK == result,
                   "Message to RSA-PKCS1_V15 sign(size = %"PRIuS"): %s", sizeof(message), message);
    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     signature,
                                     &signatureSize);
    LE_TEST_ASSERT(LE_OK == result, "Test RSA-PKCS1_V15 signature size = %"PRIuS".", signatureSize);

    // Verify with wrong padding.
    LE_TEST_ASSERT(LE_OK != OpensslVerifySignature(message, sizeof(message), expKeyData, expKeySize,
                   signature, signatureSize, DIGEST_SHA2_512, PSS_PADDING),
                   "Verify with wrong padding.");
    // Verify with wrong digest.
    LE_TEST_ASSERT(LE_OK != OpensslVerifySignature(message, sizeof(message), expKeyData, expKeySize,
                   signature, signatureSize, DIGEST_SHA2_224, PKCS_V15_PADDING),
                   "Verify with wrong digest.");
    // Verify with correct padding and digest.
    LE_TEST_ASSERT(LE_OK == OpensslVerifySignature(message, sizeof(message), expKeyData, expKeySize,
                   signature, signatureSize, DIGEST_SHA2_512, PKCS_V15_PADDING),
                   "Verify with correct padding and digest.");
}

COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("=== telaf keyStore test BEGIN ===");

    KeyManagementTest();       // Basic key management API test
    RsaEncTest();              // RSA Encryption/Decryption test
    RsaSigTest();              // RSA signing/verfication test
    RsaMultiPaddingTest();     // RSA multiple padding test
    EcdsaSigTest();            // ECDSA signing/verfication test
    HmacSigTest();             // HMAC signing/verification test
    AesEcbTest();              // AES ECB test
    AesCbcTest();              // AES CBC test
    AesCtrTest();              // AES CTR test
    AesGcmTest();              // AES GCM test
    EncDataFileTest();         // Encrypt file test
    DecDataFileTest();         // Decrypt file test
    RsaKeyExportTest();        // Export RSA key test
    KeySharingTest();          // Basic key sharing API test

    LE_TEST_INFO("=== telaf Keystore test END ===");

    LE_TEST_EXIT;
}
