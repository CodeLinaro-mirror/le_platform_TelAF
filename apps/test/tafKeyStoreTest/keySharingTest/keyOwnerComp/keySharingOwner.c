/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include "keySharing.h"

//--------------------------------------------------------------------------------------------------
/**
 * The key is never shared to any apps.
 */
//--------------------------------------------------------------------------------------------------
static taf_ks_KeyRef_t UnsharedKeyRef;

//--------------------------------------------------------------------------------------------------
/**
 * Shared key that only allows to use for 10 times, then cancels the sharing.
 */
//--------------------------------------------------------------------------------------------------
static taf_ks_KeyRef_t ShortSharedKeyRef;

//--------------------------------------------------------------------------------------------------
/**
 * Shared key that allows to use permanently, never cancels the sharing.
 */
//--------------------------------------------------------------------------------------------------
//static taf_ks_KeyRef_t PermanentSharedKeyRef;

//--------------------------------------------------------------------------------------------------
/**
 * Shared key that allows to be deleted by shared app.
 */
//--------------------------------------------------------------------------------------------------
static taf_ks_KeyRef_t DeletableSharedKeyRef;

//--------------------------------------------------------------------------------------------------
/**
 * Shared key that allows to be exported by shared app.
 */
//--------------------------------------------------------------------------------------------------
//static taf_ks_KeyRef_t ExportableSharedKeyRef;

//--------------------------------------------------------------------------------------------------
/**
 * Shared key that allows to list share app info by shared app.
 */
//--------------------------------------------------------------------------------------------------
//static taf_ks_KeyRef_t ListableSharedKeyRef;

//--------------------------------------------------------------------------------------------------
/**
 * Stub handler.
 */
//--------------------------------------------------------------------------------------------------
static void DummyHandler
(
    const char* keyId,
    const char* appName,
    taf_ks_SharingState_t state,
    void* contextPtr
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Queque function handler
 */
//--------------------------------------------------------------------------------------------------
static void TestDoneFunc
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);

    LE_TEST_ASSERT(LE_OK == taf_ks_DeleteKey(UnsharedKeyRef), "Delete %s key.", UNSHARED_KEY);
    LE_TEST_ASSERT(LE_OK == taf_ks_DeleteKey(ShortSharedKeyRef),
                                             "Delete %s key.", SHORT_SHARED_KEY);
    LE_TEST_ASSERT(LE_NOT_FOUND == taf_ks_DeleteKey(DeletableSharedKeyRef),
                                             "Delete %s key", DELE_SHARED_KEY);

    LE_TEST_INFO("=== telaf key sharing owner test END ===");
    LE_TEST_EXIT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process AES data encryption/decryption. The input data will be split into chunks due to the
 * maximum IPC packet size limitation. Here the chunk size is TAF_KS_MAX_PACKET_SIZE.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AesProcessData
(
    taf_ks_CryptoSessionRef_t sessionRef,
    const uint8_t* inputDataPtr,
    size_t inputDataSize,
    uint8_t* outputDataPtr,
    size_t* outputDataSizePtr
)
{
    le_result_t result;
    const uint8_t* unconsumedInputDataPtr = inputDataPtr;
    uint8_t* uncopiedOutputDataPtr = outputDataPtr;
    size_t unconsumedInputDataSize = inputDataSize;
    size_t uncopiedOutputDataSize = *outputDataSizePtr;
    size_t inputBytes = 0;
    size_t totalCopiedOutputBytes = 0;
    size_t requestCopyBytes = uncopiedOutputDataSize;

    if ((inputDataPtr == NULL) || (inputDataSize == 0) ||
        (outputDataPtr == NULL) || (outputDataSizePtr == NULL))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    do
    {
        inputBytes = unconsumedInputDataSize > CHUNK_SIZE ? CHUNK_SIZE : unconsumedInputDataSize;
        result = taf_ks_CryptoSessionProcess(sessionRef,
                                             unconsumedInputDataPtr,
                                             inputBytes,
                                             uncopiedOutputDataPtr,
                                             &requestCopyBytes);
        if (result != LE_OK)
        {
            return result;
        }

        unconsumedInputDataPtr += inputBytes;
        unconsumedInputDataSize -= inputBytes;

        uncopiedOutputDataPtr += requestCopyBytes;
        uncopiedOutputDataSize -= requestCopyBytes;

        totalCopiedOutputBytes += requestCopyBytes;
        requestCopyBytes = uncopiedOutputDataSize;

        LE_DEBUG("unconsumedBytes(%"PRIuS"), copiedBytes(%"PRIuS").",
                unconsumedInputDataSize, totalCopiedOutputBytes);
    }
    while(unconsumedInputDataSize > 0);

    result = taf_ks_CryptoSessionEnd(sessionRef,
                                     NULL, 0,
                                     uncopiedOutputDataPtr,
                                     &requestCopyBytes);
    if (result != LE_OK)
    {
        return result;
    }

    totalCopiedOutputBytes += requestCopyBytes;
    *outputDataSizePtr = totalCopiedOutputBytes;

    LE_DEBUG("Total copiedBytes(%"PRIuS").", totalCopiedOutputBytes);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Crypto printer API function.
 */
//--------------------------------------------------------------------------------------------------
le_result_t cryptPrinter_Print
(
    const char* LE_NONNULL keyName,
        ///< [IN]
    const char* LE_NONNULL plainString,
        ///< [IN]
    uint8_t* cipherDataPtr,
        ///< [OUT]
    size_t* cipherDataSizePtr
        ///< [INOUT]
)
{
    taf_ks_KeyRef_t keyRef = NULL;
    taf_ks_CryptoSessionRef_t sessionRef = NULL;
    static int cnt = 0;

    if (keyName == NULL || plainString == NULL ||
       cipherDataPtr == NULL || cipherDataSizePtr == NULL)
    {
        LE_TEST_FATAL("Invalid parameter.");
    }

    if (strcmp (keyName, SHORT_SHARED_KEY) == 0)
    {
        keyRef = ShortSharedKeyRef;
    }
    else if (strcmp (keyName, DELE_SHARED_KEY) == 0)
    {
        keyRef = DeletableSharedKeyRef;
    }
    else if (strcmp(keyName, KEY_SHARING_TEST_DONE) == 0)
    {
        le_event_QueueFunction(TestDoneFunc, NULL, NULL);
        return LE_OK;
    }
    else
    {
        LE_TEST_FATAL("Unknown key (%s).", keyName);
    }

    // Encrypt the request-data from client app.
    if ((LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef)) &&
        (LE_OK == taf_ks_CryptoSessionSetAesNonce(sessionRef,
            (uint8_t*)(AES_NONCE), strlen(AES_NONCE))) &&
        (LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_ENCRYPT)) &&
        (LE_OK == taf_ks_CryptoSessionProcessAead(sessionRef,
            (uint8_t*)(AES_AEAD), strlen(AES_AEAD))) &&
        (LE_OK == AesProcessData(sessionRef, (uint8_t*)plainString, strlen(plainString),
            cipherDataPtr, cipherDataSizePtr)))
    {
        LE_TEST_INFO("Handle the crypto request(%d) data(%s) for %s key.",
                     cnt, plainString, keyName);

        if (keyRef == ShortSharedKeyRef)
        {
            if (cnt++ > 5)
            {
                LE_TEST_ASSERT(LE_OK == taf_ks_CancelKeySharing(ShortSharedKeyRef,
                    SHARED_APP), "Cancel key sharing for %s key.", keyName);

                // Means key is not shared any more.
                return LE_UNSUPPORTED;
            }
        }

        return LE_OK;
    }

    LE_TEST_FATAL("Unable to handle the crypto request data(%s) for %s key.", plainString, keyName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test unshared key.
 */
//--------------------------------------------------------------------------------------------------
static void OwnerTestUnsharedKey
(
    void
)
{
    char appName[TAF_KS_MAX_APP_NAME_SIZE + 1] = { 0 };
	taf_ks_KeyUsage_t keyCap = 0;
    taf_ks_AppCapMask_t appCap = 0;

    le_result_t result = taf_ks_GetKey(UNSHARED_KEY, &UnsharedKeyRef);

    if (LE_NOT_FOUND == result)
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(UNSHARED_KEY, TAF_KS_RSA_ENCRYPT_DECRYPT,
            &UnsharedKeyRef), "Created %s key(%p).", UNSHARED_KEY, UnsharedKeyRef);
        LE_TEST_ASSERT(LE_OK == taf_ks_ProvisionRsaEncKeyValue(UnsharedKeyRef,
            TAF_KS_RSA_SIZE_1024, TAF_KS_RSA_ENC_PAD_OAEP_SHA2_512, NULL, 0),
            "provisioned %s key.", UNSHARED_KEY);
    }
    else if (LE_OK == result)
    {
        LE_TEST_INFO("%s key already exists.", UNSHARED_KEY);
    }
    else
    {
        LE_TEST_FATAL("Failed(%s) to create %s key.", LE_ERRNO_TXT(result), UNSHARED_KEY);
    }

    // The key is not shared.
    LE_TEST_ASSERT(LE_NOT_FOUND == taf_ks_GetFirstSharedApp(UnsharedKeyRef, appName,
        sizeof(appName), &keyCap, &appCap), "Get empty app list.");

    // Unable to register the key sharing handler for my own key.
    LE_TEST_ASSERT(LE_OK == taf_ks_GetCallingAppName(appName, sizeof(appName)),
                   "Get call app name (%s).", appName);
    LE_TEST_ASSERT(NULL == taf_ks_AddKeySharingHandler(UNSHARED_KEY, appName, DummyHandler, NULL),
                   "Register handler for our own key.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test short time shared key.
 */
//--------------------------------------------------------------------------------------------------
static void OwnerTestShortSharedKey
(
    void
)
{
    char appName[TAF_KS_MAX_APP_NAME_SIZE + 1] = { 0 };
    taf_ks_KeyUsage_t keyCap = 0;
    taf_ks_AppCapMask_t appCap = 0;

    le_result_t result = taf_ks_GetKey(SHORT_SHARED_KEY, &ShortSharedKeyRef);
    if (LE_NOT_FOUND == result)
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(SHORT_SHARED_KEY, TAF_KS_AES_ENCRYPT_DECRYPT,
            &ShortSharedKeyRef), "Created %s key(%p).", SHORT_SHARED_KEY, ShortSharedKeyRef);
        LE_TEST_ASSERT(LE_OK == taf_ks_ProvisionAesKeyValue(ShortSharedKeyRef,
            TAF_KS_AES_SIZE_256, TAF_KS_AES_MODE_GCM, NULL, 0),
            "Provisioned %s key.", SHORT_SHARED_KEY);
        LE_TEST_ASSERT(LE_OK == taf_ks_ShareKey(ShortSharedKeyRef, SHARED_APP,
            TAF_KS_AES_ENCRYPT_DECRYPT, 0),  "Shared %s key.", SHORT_SHARED_KEY);
    }
    else if (LE_OK == result)
    {
        result = taf_ks_GetFirstSharedApp(ShortSharedKeyRef, appName, sizeof(appName),
                                          &keyCap, &appCap);
        LE_TEST_ASSERT((LE_OK == result) && (strcmp(appName, SHARED_APP) == 0),
                        "%s key already shared.", SHORT_SHARED_KEY);
    }
    else
    {
        LE_TEST_FATAL("Failed(%s) to create %s key.", LE_ERRNO_TXT(result), SHORT_SHARED_KEY);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test deletable shared key.
 */
//--------------------------------------------------------------------------------------------------
static void OwnerTestDeletableSharedKey
(
    void
)
{
    char appName[TAF_KS_MAX_APP_NAME_SIZE + 1] = { 0 };
    taf_ks_KeyUsage_t keyCap = 0;
    taf_ks_AppCapMask_t appCap = 0;

    le_result_t result = taf_ks_GetKey(DELE_SHARED_KEY, &DeletableSharedKeyRef);
    if (LE_NOT_FOUND == result)
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(DELE_SHARED_KEY, TAF_KS_AES_ENCRYPT_DECRYPT,
            &DeletableSharedKeyRef), "Created %s key(%p).", DELE_SHARED_KEY, DeletableSharedKeyRef);
        LE_TEST_ASSERT(LE_OK == taf_ks_ProvisionAesKeyValue(DeletableSharedKeyRef,
            TAF_KS_AES_SIZE_256, TAF_KS_AES_MODE_GCM, NULL, 0),
            "Provisioned %s key.", DELE_SHARED_KEY);
        LE_TEST_ASSERT(LE_OK == taf_ks_ShareKey(DeletableSharedKeyRef, SHARED_APP,
            TAF_KS_AES_ENCRYPT_DECRYPT, TAF_KS_CAP_DELETE_KEY),
            "Shared %s key.", DELE_SHARED_KEY);
    }
    else if (LE_OK == result)
    {
        result = taf_ks_GetFirstSharedApp(DeletableSharedKeyRef, appName, sizeof(appName),
                                          &keyCap, &appCap);
        LE_TEST_ASSERT((LE_OK == result) && (strcmp(appName, SHARED_APP) == 0),
                        "%s key already shared.", DELE_SHARED_KEY);
    }
    else
    {
        LE_TEST_FATAL("Failed(%s) to create %s key.", LE_ERRNO_TXT(result), DELE_SHARED_KEY);
    }
}

COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("=== telaf key sharing owner test BEGIN ===");

    OwnerTestUnsharedKey();
    OwnerTestShortSharedKey();
    OwnerTestDeletableSharedKey();
}
