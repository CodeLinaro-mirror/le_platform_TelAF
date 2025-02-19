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
 * Timer reference.
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t TimerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Convert shared-state enum to string.
 *
 * @return state string.
 */
//--------------------------------------------------------------------------------------------------
const char *GetState
(
    taf_ks_SharingState_t state
)
{
    switch (state)
    {
        case TAF_KS_SHARING_ENABLED:
            return "TAF_KS_SHARING_ENABLED";
            break;

        case TAF_KS_SHARING_UPDATED:
            return "TAF_KS_SHARING_UPDATED";
            break;

        case TAF_KS_SHARING_DISABLED:
            return "TAF_KS_SHARING_DISABLED";
            break;
        default:
            LE_TEST_FATAL("Invalid shared state");
            break;
    }
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
 * Crypto data request-response test.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CryptoDataRequestReponse
(
    taf_ks_KeyRef_t keyRef,
    const char* keyIdPtr
)
{
    le_result_t result;
    uint8_t encryptedText[TEXT_MAX_SIZE] = { 0 };
    size_t encryptedTextSize = TEXT_MAX_SIZE;
    uint8_t decryptedData[TEXT_MAX_SIZE] = { 0 };
    size_t decryptedDataSize = TEXT_MAX_SIZE;
    taf_ks_CryptoSessionRef_t sessionRef = NULL;

    LE_TEST_INFO("Crypto-PlainText is: '%s'.", keyIdPtr);

    // Get encrypted data from owner app. Also pass the keyId as the plainText.
    result = cryptPrinter_Print(keyIdPtr, keyIdPtr, encryptedText, &encryptedTextSize);

    if (LE_OK == result)
    {
        // Use shared key to decrypt the cipher data.
        if ((LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef)) &&
            (LE_OK == taf_ks_CryptoSessionSetAesNonce(sessionRef,
                (uint8_t*)(AES_NONCE), strlen(AES_NONCE))) &&
            (LE_OK == taf_ks_CryptoSessionStart(sessionRef, TAF_KS_CRYPTO_DECRYPT)) &&
            (LE_OK == taf_ks_CryptoSessionProcessAead(sessionRef,
                (uint8_t*)(AES_AEAD), strlen(AES_AEAD))) &&
            (LE_OK == AesProcessData(sessionRef, encryptedText, encryptedTextSize,
                decryptedData, &decryptedDataSize)) &&
            (0 == memcmp((uint8_t*)keyIdPtr, decryptedData, decryptedDataSize)))
        {
            LE_TEST_INFO("Crypto-DecryptedText is: '%s'.", decryptedData);
            return LE_OK;
        }
    }
    else if (LE_UNSUPPORTED == result)
    {
        LE_TEST_INFO("Skipped encryption since key sharing is already cancelled.");
        return LE_OK;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Timer handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TimerHandler
(
    le_timer_Ref_t timerRef
)
{
    static uint32_t cnt = 0;

    if (ShortSharedKeyRef != NULL)
    {
        LE_TEST_ASSERT(LE_OK == CryptoDataRequestReponse(ShortSharedKeyRef, SHORT_SHARED_KEY),
            "Crypto request-response(cnt %d) with %s key.", cnt++, SHORT_SHARED_KEY);
    }

    if (DeletableSharedKeyRef != NULL)
    {
        LE_TEST_ASSERT(LE_OK == CryptoDataRequestReponse(DeletableSharedKeyRef, DELE_SHARED_KEY),
            "Crypto request-response with %s key.", DELE_SHARED_KEY);

        // Delete the shared key.
        LE_TEST_ASSERT(LE_OK == taf_ks_DeleteKey(DeletableSharedKeyRef),
            "Delete %s key.", DELE_SHARED_KEY);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for short-time shared key sharing.
 *
 */
//--------------------------------------------------------------------------------------------------
void ShortSharedKeyHandler
(
    const char* keyId,
    const char* appName,
    taf_ks_SharingState_t state,
    void* contextPtr
)
{
    char keyName[TAF_KS_MAX_KEY_ID_SIZE + 1] = { 0 };

    // Sanity check for owner app name.
    LE_TEST_ASSERT((strcmp(appName, OWNER_APP) == 0) && (strcmp(keyId, SHORT_SHARED_KEY) == 0),
                   "State event: %s.", GetState(state));
    // Build the keyName.
    LE_ASSERT(snprintf(keyName, sizeof(keyName), "%s::%s", OWNER_APP, SHORT_SHARED_KEY)
              < sizeof(keyName));

    if ((state == TAF_KS_SHARING_ENABLED) && (ShortSharedKeyRef == NULL))
    {
        // Get the shared key after sharing.
        LE_TEST_ASSERT(LE_OK == taf_ks_GetKey(keyName, &ShortSharedKeyRef),
                       "Get %s key after sharing.", SHORT_SHARED_KEY);

        // Unable to delete the key since the app capability for deleting key is not shared.
        LE_TEST_ASSERT(LE_OK != taf_ks_DeleteKey(ShortSharedKeyRef),
                       "Unable to delete %s key.", SHORT_SHARED_KEY);

        // Start a timer to cyclically send crypto-request to owner app.
        if (TimerRef == NULL)
        {
            TimerRef = le_timer_Create("CryptoRequestResponse");
            le_timer_SetMsInterval(TimerRef, 2000);
            le_timer_SetHandler(TimerRef, TimerHandler);
            le_timer_SetRepeat(TimerRef, 0);
            le_timer_SetWakeup(TimerRef, false);
            le_timer_Start(TimerRef);
            LE_TEST_INFO("Create crypto-request-response timer.");
        }
    }
    else if ((state == TAF_KS_SHARING_DISABLED) && (ShortSharedKeyRef != NULL))
    {
        LE_TEST_ASSERT(LE_OK != taf_ks_GetKey(keyName, &ShortSharedKeyRef),
                       "Get %s key after cancel-sharing.", SHORT_SHARED_KEY);

        ShortSharedKeyRef = NULL;
    }

    if ((ShortSharedKeyRef == NULL) && (DeletableSharedKeyRef == NULL))
    {
        uint8_t dummyData[1] = { 0 };
        size_t dummyDataSize = 1;

        LE_TEST_ASSERT(LE_OK == cryptPrinter_Print(KEY_SHARING_TEST_DONE, KEY_SHARING_TEST_DONE,
                                                   dummyData, &dummyDataSize),
            "Key sharing crypto-request test done.");

        LE_TEST_INFO("=== telaf key sharing client test END ===");
        LE_TEST_EXIT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for short-time shared key sharing.
 *
 */
//--------------------------------------------------------------------------------------------------
void DeletableSharedKeyHandler
(
    const char* keyId,
    const char* appName,
    taf_ks_SharingState_t state,
    void* contextPtr
)
{
    char keyName[TAF_KS_MAX_KEY_ID_SIZE + 1] = { 0 };

    // Sanity check for owner app name.
    LE_TEST_ASSERT((strcmp(appName, OWNER_APP) == 0) && (strcmp(keyId, DELE_SHARED_KEY) == 0),
                    "State event: %s.", GetState(state));

    LE_ASSERT(snprintf(keyName, sizeof(keyName), "%s::%s", OWNER_APP, DELE_SHARED_KEY)
              < sizeof(keyName));

    if ((state == TAF_KS_SHARING_ENABLED) && (DeletableSharedKeyRef == NULL))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_GetKey(keyName, &DeletableSharedKeyRef),
                       "Get %s key after sharing.", DELE_SHARED_KEY);

        // Start a timer to cyclically send crypto-request to owner app.
        if (TimerRef == NULL)
        {
            TimerRef = le_timer_Create("CryptoRequestResponse");
            le_timer_SetMsInterval(TimerRef, 2000);
            le_timer_SetHandler(TimerRef, TimerHandler);
            le_timer_SetRepeat(TimerRef, 0);
            le_timer_SetWakeup(TimerRef, false);
            le_timer_Start(TimerRef);
            LE_TEST_INFO("Create crypto-request-response timer.");
        }
    }
    else if ((state == TAF_KS_SHARING_DISABLED) && (DeletableSharedKeyRef != NULL))
    {
        LE_TEST_ASSERT(LE_NOT_FOUND == taf_ks_GetKey(keyName, &DeletableSharedKeyRef),
            "Get %s key after deletion.", DELE_SHARED_KEY);

        DeletableSharedKeyRef = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test unshared key.
 */
//--------------------------------------------------------------------------------------------------
static void ClientTestUnsharedKey
(
    void
)
{
    char keyName[TAF_KS_MAX_KEY_ID_SIZE + 1] = { 0 };

    LE_ASSERT(snprintf(keyName, sizeof(keyName), "%s::%s", OWNER_APP, UNSHARED_KEY)
              < sizeof(keyName));

    // Will fail to get the un-shared key from owner app.
    LE_TEST_ASSERT(LE_OK != taf_ks_GetKey(keyName, &UnsharedKeyRef),
                   "Client app failed to get %s key.", keyName);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test short-time shared key.
 */
//--------------------------------------------------------------------------------------------------
static void ClientTestShortSharedKey
(
    void
)
{
    // Register handler for short-time shared key.
    LE_TEST_ASSERT(NULL != taf_ks_AddKeySharingHandler(SHORT_SHARED_KEY, OWNER_APP,
                                                       ShortSharedKeyHandler, NULL),
                   "Register keySharingHandler for %s key.", SHORT_SHARED_KEY);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test deletable shared key.
 */
//--------------------------------------------------------------------------------------------------
static void ClientTestDeletableKey
(
    void
)
{
    // Register handler for deletable shared key.
    LE_TEST_ASSERT(NULL != taf_ks_AddKeySharingHandler(DELE_SHARED_KEY, OWNER_APP,
                                                       DeletableSharedKeyHandler, NULL),
                   "Register keySharingHandler for %s key.", DELE_SHARED_KEY);
}

COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("=== telaf key sharing client test BEGIN ===");

    ClientTestUnsharedKey();
    ClientTestShortSharedKey();
    ClientTestDeletableKey();
}
