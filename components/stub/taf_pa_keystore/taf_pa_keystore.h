/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef TAF_PA_KEYSTORAGE_INCLUDE_GUARD
#define TAF_PA_KEYSTORAGE_INCLUDE_GUARD

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Tag ID.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_KS_TAG_MAX_USES_PER_BOOT = 0,          ///< Number of times the key can be used per boot.
    TAF_PA_KS_TAG_MIN_SECONDS_BETWEEN_OPS  = 1,   ///< Minimum elapsed time between cryptographic
                                                  ///  operations with the key.
    TAF_PA_KS_TAG_APPLICATION_DATA = 2,           ///< Data identifying the authorized application.
    TAF_PA_KS_TAG_ACTIVE_DATETIME = 3,            ///< Start of validity.
    TAF_PA_KS_TAG_ORIGINATION_EXPIRE_DATETIME = 4,///< Date when new "messages" should no longer be
                                                  ///  created.
    TAF_PA_KS_TAG_USAGE_EXPIRE_DATETIME = 5,      ///< Date when existing "messages" should no
                                                  ///  longer be trusted.
    TAF_PA_KS_TAG_MAX_IDS = 6,
}
taf_pa_ks_TagId_t;

//--------------------------------------------------------------------------------------------------
/**
 * Parameter ID
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_KS_PARAM_NONCE = 0,                     ///< Nonce or Initialization Vector.
    TAF_PA_KS_PARAM_APPLICATION_DATA = 1,          ///< Data identifying the authorized application.
    TAF_PA_KS_PARAM_MAX_IDS = 2,
}
taf_pa_ks_ParamId_t;

//--------------------------------------------------------------------------------------------------
/**
 * AES nonce size
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_KS_AES_GCM_NONCE_SIZE = 12,             ///< AES GCM nonce, length is 12 byte.
    TAF_PA_KS_AES_CBC_NONCE_SIZE = 16,             ///< AES CBC nonce, length is 16 byte.
    TAF_PA_KS_AES_CTR_NONCE_SIZE = 16,             ///< AES CTR nonce, lenght is 16 byte.
}
taf_pa_ks_NonceSize_t;

//--------------------------------------------------------------------------------------------------
/**
 * Encryption key purpose
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_KS_ENCRYPT_DECRYPT = 0,
    TAF_PA_KS_ENCRYPT_ONLY = 1,
    TAF_PA_KS_DECRYPT_ONLY = 2,
    TAF_PA_KS_ENC_MAX,
}
taf_pa_ks_EncPurpose_t;

//--------------------------------------------------------------------------------------------------
/**
 * Signing key purpose
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_KS_SIGN_VERIFY = 0,
    TAF_PA_KS_SIGN_ONLY = 1,
    TAF_PA_KS_VERIFY_ONLY = 2,
    TAF_PA_KS_SIG_MAX,
}
taf_pa_ks_SigPurpose_t;

//--------------------------------------------------------------------------------------------------
/**
 * AES Nonce struct
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_pa_ks_NonceSize_t size;                    ///< AES nonce size.
    uint8_t data[TAF_KS_MAX_AES_NONCE_SIZE];       ///< AES nonce buffer.
}
taf_pa_ks_Nonce_t;

//--------------------------------------------------------------------------------------------------
/**
 * common data buffer struct
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    size_t   size;                                 ///< data size.
    uint8_t  data[TAF_KS_MAX_PACKET_SIZE];         ///< data buffer.
}
taf_pa_ks_Data_t;

//--------------------------------------------------------------------------------------------------
/**
 * Keystore Tag struct
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t   link;                          ///< Link to the tagList of the key
    taf_pa_ks_TagId_t id;                          ///< KS tag ID
    union
    {
        uint32_t maxUsesPerBoot;                   ///< TAF_PA_KS_TAG_MAX_USES_PER_BOOT
        uint32_t minSecondsBetweenOps;             ///< TAF_PA_KS_TAG_MIN_SECONDS_BETWEEN_OPS
        uint64_t activeDateTime;                   ///< TAF_PA_KS_TAG_ACTIVE_DATETIME
        uint64_t originationExpireDateTime;        ///< TAF_PA_KS_TAG_ORIGINATION_EXPIRE_DATETIME
        uint64_t usageExpireDateTime;              ///< TAF_PA_KS_TAG_USAGE_EXPIRE_DATETIME
        taf_pa_ks_Data_t* appDataPtr;              ///< TAF_PA_KS_TAG_APPLICATION_DATA
    };
}
taf_pa_ks_Tag_t;

//--------------------------------------------------------------------------------------------------
/**
 * Keystore Parameter struct
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t     link;                       ///< Link to the paramList of the crypto session
    taf_pa_ks_ParamId_t id;                       ///< KS parameter ID
    union
    {
        taf_pa_ks_Nonce_t* nonceDataPtr;          ///< TAF_PA_KS_PARAM_NONCE
        taf_pa_ks_Data_t* appDataPtr;             ///< TAF_PA_KS_PARAM_APPLICATION_DATA
    };
}
taf_pa_ks_Param_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference to a key file object
 */
//--------------------------------------------------------------------------------------------------
typedef void* taf_pa_ks_KeyFileRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * PA initialization.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Create or import a RSA encryption key and return a key file reference
 *
 * The impData must be a PKCS#8 der bytes if provided.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_GenerateRsaEncKey
(
    le_msg_SessionRef_t clientSessionRef, ///< [IN] Client session reference
    const char* keyName,                  ///< [IN] Key Name
    taf_ks_RsaKeySize_t keySize,          ///< [IN] Key Size, ignored if impData is provided
    taf_pa_ks_EncPurpose_t purpose,       ///< [IN] Encryption purpose
    taf_ks_RsaEncPadding_t padding,       ///< [IN] RSA encryption padding type
    le_dls_List_t* tagListPtr,            ///< [IN] List of taf_pa_ks_Tag_t
    const uint8_t* impDataPtr,            ///< [IN] Imported key data
    size_t impDataSize,                   ///< [IN] less than TAF_KS_MAX_PACKET_SIZE
    taf_pa_ks_KeyFileRef_t* keyFileRefPtr ///< [OUT] Key file reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Create or import a RSA signature key and return a key file reference.
 *
 * The impData must be a PKCS#8 der bytes if provided.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_GenerateRsaSigKey
(
    le_msg_SessionRef_t clientSessionRef, ///< [IN] Client session reference
    const char* keyName,                  ///< [IN] Key Name
    taf_ks_RsaKeySize_t keySize,          ///< [IN] Key Size, ignored if impData is provided
    taf_pa_ks_SigPurpose_t purpose,       ///< [IN] Signature purpose
    taf_ks_RsaSigPadding_t padding,       ///< [IN] RSA signature padding type
    le_dls_List_t* tagListPtr,            ///< [IN] List of taf_pa_ks_Tag_t
    const uint8_t* impDataPtr,            ///< [IN] Imported key data
    size_t impDataSize,                   ///< [IN] less than TAF_KS_MAX_PACKET_SIZE
    taf_pa_ks_KeyFileRef_t* keyFileRefPtr ///< [OUT] Key file reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Create or import an ECDSA key and return a key file reference.
 *
 * The impData must be PKCS#8 der bytes if provided.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_GenerateEcdsaKey
(
    le_msg_SessionRef_t clientSessionRef, ///< [IN] Client session reference
    const char* keyName,                  ///< [IN] Key Name
    taf_ks_EccKeySize_t keySize,          ///< [IN] ECC curve, ignored if impData is provided
    taf_pa_ks_SigPurpose_t purpose,       ///< [IN] Signature purpose
    taf_ks_Digest_t digest,               ///< [IN] Digest
    le_dls_List_t* tagListPtr,            ///< [IN] List of taf_pa_ks_Tag_t
    const uint8_t* impDataPtr,            ///< [IN] Imported key data
    size_t impDataSize,                   ///< [IN] less than TAF_KS_MAX_PACKET_SIZE
    taf_pa_ks_KeyFileRef_t* keyFileRefPtr ///< [OUT] Key file reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Create or import an AES key and return a key file reference.
 *
 * The impData must be raw key bytes if provided.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_GenerateAesKey
(
    le_msg_SessionRef_t clientSessionRef, ///< [IN] Client session reference
    const char* keyName,                  ///< [IN] Key Name
    taf_ks_AesKeySize_t keySize,          ///< [IN] AES key size, ignored if impData is provided
    taf_pa_ks_EncPurpose_t purpose,       ///< [IN] Encryption purpose
    taf_ks_AesBlockMode_t mode,           ///< [IN] AES block mode
    le_dls_List_t* tagListPtr,            ///< [IN] List of taf_pa_ks_Tag_t
    const uint8_t* impDataPtr,            ///< [IN] Imported key data
    size_t impDataSize,                   ///< [IN] less than TAF_KS_MAX_PACKET_SIZE
    taf_pa_ks_KeyFileRef_t* keyFileRefPtr ///< [OUT] Key file reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Create or import a HMAC key and return a key file reference.
 *
 * Currently only digest DIGEST_SHA2_256 is supported. The impData must be raw key bytes if provided
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_GenerateHmacKey
(
    le_msg_SessionRef_t clientSessionRef, ///< [IN] Client session reference
    const char* keyName,                  ///< [IN] Key Name
    uint32_t keySize,                     ///< [IN] HMAC Key Size, ignored if impData is provided
    taf_pa_ks_SigPurpose_t purpose,       ///< [IN] Signature purpose
    taf_ks_Digest_t digest,               ///< [IN] digest
    le_dls_List_t* tagListPtr,            ///< [IN] List of taf_pa_ks_Tag_t
    const uint8_t* impDataPtr,            ///< [IN] Imported key data
    size_t impDataSize,                   ///< [IN] less than TAF_KS_MAX_PACKET_SIZE
    taf_pa_ks_KeyFileRef_t* keyFileRefPtr ///< [OUT] Key file reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Export a key into specified key data format.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_ExportKey
(
    le_msg_SessionRef_t clientSessionRef, ///< [IN] Client session reference
    taf_pa_ks_KeyFileRef_t keyFileRef,    ///< [IN] Key file reference
    const uint8_t* appDataPtr,            ///< [IN] Application data
    size_t appDataSize,                   ///< [IN]
    uint8_t* expDataPtr,                  ///< [OUT] exported key data
    size_t* expDataSizePtr                ///< [INOUT]
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete a key file by key name.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_DeleteKey
(
    le_msg_SessionRef_t clientSessionRef, ///< [IN] Client session reference
    taf_pa_ks_KeyFileRef_t keyFileRef     ///< [IN] Key file reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Get a key file reference by key name.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_GetKey
(
    le_msg_SessionRef_t clientSessionRef, ///< [IN] Client session reference
    const char* keyName,                  ///< [IN] Key Name
    taf_pa_ks_KeyFileRef_t* keyFileRefPtr ///< [OUT] Key file reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get key usage
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_GetKeyUsage
(
    le_msg_SessionRef_t clientSessionRef, ///< [IN] Client session reference
    taf_pa_ks_KeyFileRef_t keyFileRef,    ///< [IN] Key file reference
    taf_ks_KeyUsage_t*    keyUsagePtr     ///< [OUT] Key usage
);

//--------------------------------------------------------------------------------------------------
/**
 * Start the session for the given crypto operation.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_CryptoSessionStart
(
    le_msg_SessionRef_t clientSessionRef, ///< [IN] Client session reference
    taf_pa_ks_KeyFileRef_t     keyFileRef,///< [IN] Key file reference
    taf_ks_CryptoPurpose_t  cryptoPurpose,///< [IN] Crypto purpose
    le_dls_List_t*           paramListPtr,///< [IN] List of taf_pa_ks_Param_t
    uint64_t*                 opHandlePtr ///< [OUT]Cyrpto operation handle
);

//--------------------------------------------------------------------------------------------------
/**
 * Provides AES AEAD to the running crypto session started with CryptoSessionStart API for AES GCM
 * mode.
 *
 * This API can be called for multiple times but must before CryptoSessionProcess API.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_CryptoSessionProcessAead
(
    uint64_t               opHandle,      ///< [IN] Cyrpto operation handle
    const uint8_t*     inputDataPtr,      ///< [IN] Data buffer to hold the AEAD data
    size_t            inputDataSize       ///< [IN]

);

//--------------------------------------------------------------------------------------------------
/**
 * Provides data to, and possibly receives output from, an runing crypto operation started with
 * CryptoStartSession API. It can be called for multiple times to support streaming mode until
 * CryptoEndSession API is called.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_CryptoSessionProcess
(
    uint64_t               opHandle,      ///< [IN] Cyrpto operation handle
    const uint8_t*     inputDataPtr,      ///< [IN] InputData can be one of below 4 cases:
                                          ///<      1: plain text for encryption session.
                                          ///<      2: cipher text for decryption session.
                                          ///<      3: message to sign for signing session.
                                          ///<      4: message to verify for verification session.
    size_t            inputDataSize,      ///< [IN]
    uint8_t*          outputDataPtr,      ///< [OUT] OutputData can be one of below 3 cases:
                                          ///<       1: encrypted data for encryption session.
                                          ///<       2: decrypted data for decryption session.
                                          ///<       3: ignore for signing and verification session.
    size_t*        outputDataSizePtr      ///< [INOUT]
);

//--------------------------------------------------------------------------------------------------
/**
 * Finalizes and stop a crypto operation session started with CryptoStartSession API.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_CryptoSessionEnd
(
    uint64_t               opHandle,      ///< [IN] Cyrpto operation handle
    const uint8_t*     inputDataPtr,      ///< [IN] Signature to verify for verification session
                                          ///<      and ignored for other sessions.
    size_t            inputDataSize,      ///< [IN]
    uint8_t*          outputDataPtr,      ///< [OUT] OutputData can be one of below 4 cases:
                                          ///<       1: encrypted data for encryption session.
                                          ///<       2: decrypted data for decryption session.
                                          ///<       3: signature for signing session.
                                          ///<       4: ignore for verfication session.
    size_t*        outputDataSizePtr      ///< [INOUT]
);

//--------------------------------------------------------------------------------------------------
/**
 * Abort crypto operation session started with CryptoStartSession API.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_ks_CryptoSessionAbort
(
    uint64_t                opHandle      ///< [IN] Cyrpto operation handle
);

#endif // TAF_PA_KEYSTORAGE_INCLUDE_GUARD
