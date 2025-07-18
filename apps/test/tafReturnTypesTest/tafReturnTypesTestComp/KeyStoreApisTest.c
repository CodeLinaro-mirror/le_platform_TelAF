/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"
#include "KeyStoreApisTest.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate KeyStore Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void keystoreRetTest_RunApis
(
   void
)
{
    le_result_t result;
    const char keyId[] = "KeyStoreTest";
    const char keyId2[] = "KeyStoreTest2";
    const char keyId_ActiveDate[] = "KeyStoreActiveDate";
    const char keyId_AppData[] = "KeyStoreAppData";
    const char keyId_ExpDate[] = "KeyStoreExpDate";
    const char keyId_OrgExpDate[] = "KeyStoreOrgExpDate";
    const char keyId_Create[] = "KeyStoreCreateKey";
    taf_ks_KeyUsage_t keyUsage;
    taf_ks_KeyRef_t keyRef,keyRef1=NULL,keyRef2,keyRef_AppData,keyRef_ActiveDate;
    taf_ks_KeyRef_t keyRef_ExpDate,keyRef_OrgExpDate;
    uint8_t data[MAX_PACKET_SIZE];
    uint8_t expKeyData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t expKeySize = sizeof(expKeyData);
    static taf_ks_KeyRef_t ShortSharedKeyRef;
    taf_ks_CryptoSessionRef_t sessionRef;
    char appName[TAF_KS_MAX_APP_NAME_SIZE+1] = { 0 };
    taf_ks_KeyUsage_t keyCap;
    taf_ks_AppCapMask_t appCap;
    const uint8_t nonce[12] = {"abcdefghijk"};
    uint8_t appData[MAX_PACKET_SIZE];
    uint64_t value = 2025;
    LE_TEST_INFO("keystoreRetTest_RunApis");

    //1.taf_ks_GetKeyUsage- LE_BAD_PARAMETER scenario
    taf_ks_CreateKey(keyId, TAF_KS_RSA_ENCRYPT_DECRYPT, &keyRef);
    result = taf_ks_GetKeyUsage(keyRef, NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_ks_GetKeyUsage***-LE_BAD_PARAMETER ");

    //2.taf_ks_GetKeyUsage- LE_NOT_FOUND scenario
    result = taf_ks_GetKeyUsage(keyRef1, &keyUsage);
    LE_TEST_OK(result == LE_NOT_FOUND , "***taf_ks_GetKeyUsage***-LE_NOT_FOUND ");

    //3.taf_ks_SetKeyMaxUsesPerBoot - LE_NOT_FOUND scenario
    result = taf_ks_SetKeyMaxUsesPerBoot(NULL, 10);
    LE_TEST_OK(result == LE_NOT_FOUND , "***taf_ks_SetKeyMaxUsesPerBoot***-LE_NOT_FOUND ");

    //4.taf_ks_SetKeyMaxUsesPerBoot - LE_NOT_FOUND scenario
    result = taf_ks_SetKeyMinSecondsBetweenOps(NULL, 5);
    LE_TEST_OK(result == LE_NOT_FOUND , "***taf_ks_SetKeyMaxUsesPerBoot***-LE_NOT_FOUND ");

    //5.taf_ks_SetKeyAppData - LE_BAD_PARAMETER; scenario
    result = taf_ks_SetKeyAppData(keyRef1,NULL, 0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_SetKeyAppData***-LE_BAD_PARAMETER; ");

    //6.taf_ks_SetKeyAppData - LE_NOT_FOUND scenario
    result = taf_ks_SetKeyAppData(NULL,data,MAX_PACKET_SIZE);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_SetKeyAppData***-LE_NOT_FOUND; ");

    //7.taf_ks_SetKeyAppData - LE_OK scenario
    taf_ks_CreateKey(keyId_AppData, TAF_KS_RSA_ENCRYPT_DECRYPT, &keyRef_AppData);
    result = taf_ks_SetKeyAppData(keyRef_AppData,data,MAX_PACKET_SIZE);
    LE_TEST_OK(result == LE_OK, "***taf_ks_SetKeyAppData***-LE_OK ");

    //8.taf_ks_SetKeyActiveDateTime - LE_NOT_FOUND scenario
    result = taf_ks_SetKeyActiveDateTime(NULL,0);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_SetKeyActiveDateTime***-LE_NOT_FOUND; ");

    //9.taf_ks_SetKeyActiveDateTime - LE_OK scenario
    taf_ks_CreateKey(keyId_ActiveDate, TAF_KS_RSA_ENCRYPT_DECRYPT, &keyRef_ActiveDate);
    result = taf_ks_SetKeyActiveDateTime(keyRef_ActiveDate,value);
    LE_TEST_OK(result == LE_OK, "***taf_ks_SetKeyActiveDateTime***-LE_OK; ");

    //10.taf_ks_SetKeyOriginationExpireDateTime - LE_NOT_FOUND scenario
    result = taf_ks_SetKeyOriginationExpireDateTime(NULL,0);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_SetKeyOriginationExpireDateTime***-LE_NOT_FOUND; ");

    //11.taf_ks_SetKeyOriginationExpireDateTime - LE_OK scenario
    taf_ks_CreateKey(keyId_ExpDate, TAF_KS_RSA_ENCRYPT_DECRYPT, &keyRef_ExpDate);
    result = taf_ks_SetKeyOriginationExpireDateTime(keyRef_ExpDate,value);
    LE_TEST_OK(result == LE_OK, "***taf_ks_SetKeyOriginationExpireDateTime***-LE_OK; ");

    //12.taf_ks_SetKeyUsageExpireDateTime - LE_NOT_FOUND scenario
    result = taf_ks_SetKeyUsageExpireDateTime(NULL,0);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_SetKeyUsageExpireDateTime***-LE_NOT_FOUND; ");

    //13.taf_ks_SetKeyUsageExpireDateTime - LE_OK scenario
    taf_ks_CreateKey(keyId_OrgExpDate, TAF_KS_RSA_ENCRYPT_DECRYPT, &keyRef_OrgExpDate);
    result = taf_ks_SetKeyUsageExpireDateTime(keyRef_OrgExpDate,value);
    LE_TEST_OK(result == LE_OK, "***taf_ks_SetKeyUsageExpireDateTime***-LE_OK; ");

    //14.taf_ks_ProvisionRsaEncKeyValue - LE_BAD_PARAMETER scenario
    taf_ks_CreateKey(keyId2, TAF_KS_RSA_ENCRYPT_DECRYPT, &keyRef2);
    result = taf_ks_ProvisionRsaEncKeyValue(keyRef2,
                                            TAF_KS_RSA_SIZE_MAX,
                                            TAF_KS_RSA_ENC_PAD_PKCS1_V15,
                                            NULL, 0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_ProvisionRsaEncKeyValue***-LE_BAD_PARAMETER; ");

    //15.taf_ks_ProvisionRsaEncKeyValue - LE_NOT_FOUND scenario
    result = taf_ks_ProvisionRsaEncKeyValue(NULL,
                                            TAF_KS_RSA_SIZE_4096,
                                            TAF_KS_RSA_ENC_PAD_PKCS1_V15,
                                            NULL, 0);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_ProvisionRsaEncKeyValue***-LE_NOT_FOUND; ");



    //16.taf_ks_ProvisionRsaSigKeyValue - LE_BAD_PARAMETER scenario
    result = taf_ks_ProvisionRsaSigKeyValue(keyRef2,
                                            TAF_KS_RSA_SIZE_MAX,
                                            TAF_KS_RSA_ENC_PAD_PKCS1_V15,
                                            NULL, 0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_ProvisionRsaSigKeyValue***-LE_BAD_PARAMETER; ");

    //17.taf_ks_ProvisionRsaSigKeyValue - LE_NOT_FOUND scenario
    result = taf_ks_ProvisionRsaSigKeyValue(NULL,
                                            TAF_KS_RSA_SIZE_4096,
                                            TAF_KS_RSA_ENC_PAD_PKCS1_V15,
                                            NULL, 0);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_ProvisionRsaSigKeyValue***-LE_NOT_FOUND; ");


    //18.taf_ks_ProvisionEcdsaKeyValue - LE_BAD_PARAMETER scenario
    taf_ks_CreateKey(keyId2, TAF_KS_RSA_ENCRYPT_DECRYPT, &keyRef2);
    result = taf_ks_ProvisionEcdsaKeyValue(keyRef2,
                                            TAF_KS_RSA_SIZE_MAX,
                                            TAF_KS_RSA_ENC_PAD_PKCS1_V15,
                                            NULL, 0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_ProvisionEcdsaKeyValue***-LE_BAD_PARAMETER; ");

    //19.taf_ks_ProvisionEcdsaKeyValue - LE_NOT_FOUND scenario
    result = taf_ks_ProvisionEcdsaKeyValue(NULL,
                                            TAF_KS_RSA_SIZE_4096,
                                            TAF_KS_RSA_ENC_PAD_PKCS1_V15,
                                            NULL, 0);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_ProvisionEcdsaKeyValue***-LE_NOT_FOUND; ");

    //20.taf_ks_ProvisionAesKeyValue  - LE_BAD_PARAMETER scenario
    result = taf_ks_ProvisionAesKeyValue(keyRef2,
                                         TAF_KS_RSA_SIZE_MAX,
                                         TAF_KS_RSA_ENC_PAD_PKCS1_V15,
                                         NULL, 0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_ProvisionAesKeyValue***-LE_BAD_PARAMETER; ");

    //21.taf_ks_ProvisionAesKeyValue - LE_NOT_FOUND scenario
    result = taf_ks_ProvisionAesKeyValue(NULL,
                                         TAF_KS_RSA_SIZE_3072,
                                         TAF_KS_RSA_ENC_PAD_PKCS1_V15,
                                         NULL, 0);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_ProvisionAesKeyValue***-LE_NOT_FOUND; ");

    //22.taf_ks_ProvisionHmacKeyValue  - LE_BAD_PARAMETER scenario
    result = taf_ks_ProvisionHmacKeyValue(keyRef2,
                                          TAF_KS_MAX_HMAC_KEY_SIZE+1,
                                          TAF_KS_DIGEST_SHA2_256,
                                          NULL, 0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_ProvisionHmacKeyValue***-LE_BAD_PARAMETER; ");

    //23.taf_ks_ProvisionHmacKeyValue - LE_NOT_FOUND scenario
    result = taf_ks_ProvisionHmacKeyValue(NULL,
                                         TAF_KS_MAX_HMAC_KEY_SIZE,
                                         TAF_KS_DIGEST_SHA2_256,
                                         NULL, 0);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_ProvisionHmacKeyValue***-LE_NOT_FOUND; ");


    //24.taf_ks_ExportKey  - LE_BAD_PARAMETER scenario
    result = taf_ks_ExportKey(keyRef2,NULL,0,NULL, 0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_ExportKey***-LE_BAD_PARAMETER; ");

    //25.taf_ks_ExportKey - LE_NOT_FOUND scenario
    result = taf_ks_ExportKey(NULL, NULL, 0, expKeyData, &expKeySize);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_ExportKey***-LE_NOT_FOUND; ");

    //26.taf_ks_ShareKey  - LE_NOT_FOUND scenario
    result = taf_ks_ShareKey(NULL, SHARED_APP_ID,
                             TAF_KS_AES_ENCRYPT_DECRYPT, 0);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_ShareKey***-LE_NOT_FOUND");

    //27.taf_ks_ShareKey  - LE_FAULT scenario
    taf_ks_CreateKey(SHORT_SHARED_KEY, TAF_KS_AES_ENCRYPT_DECRYPT,
            &ShortSharedKeyRef);
    taf_ks_ProvisionAesKeyValue(ShortSharedKeyRef,
            TAF_KS_AES_SIZE_256, TAF_KS_AES_MODE_GCM, NULL, 0);
    result = taf_ks_ShareKey(ShortSharedKeyRef, SHARED_APP,
            TAF_KS_AES_ENCRYPT_DECRYPT, 0);
    LE_TEST_OK(result == LE_FAULT, "***taf_ks_ShareKey***-LE_FAULT ");

    //28.taf_ks_CancelKeySharing  - LE_NOT_FOUND scenario
    result = taf_ks_CancelKeySharing(NULL, SHARED_APP_ID);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_CancelKeySharing***-LE_NOT_FOUND");

    //29.taf_ks_GetCallingAppName LE_BAD_PARAMETER scenario
    result = taf_ks_GetCallingAppName(NULL,0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_GetCallingAppName***-LE_BAD_PARAMETER");

    //30.taf_ks_GetFirstSharedApp LE_BAD_PARAMETER scenario
    result = taf_ks_GetFirstSharedApp(keyRef2, NULL,0,NULL,NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_GetFirstSharedApp***-LE_BAD_PARAMETER");

    //31.taf_ks_GetNextSharedApp LE_BAD_PARAMETER scenario
    result = taf_ks_GetNextSharedApp(keyRef2, NULL,0,NULL,NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_GetNextSharedApp***-LE_BAD_PARAMETER");

    //32.taf_ks_GetNextSharedApp LE_NOT_FOUND scenario
    result = taf_ks_GetNextSharedApp(NULL,appName,sizeof(appName),&keyCap,&appCap);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_GetNextSharedApp***-LE_NOT_FOUND");

    //33.taf_ks_CryptoSessionCreate LE_BAD_PARAMETER scenario
    result = taf_ks_CryptoSessionCreate(keyRef2, NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_CryptoSessionCreate***-LE_BAD_PARAMETER");

    //34.taf_ks_CryptoSessionCreate LE_NOT_FOUND scenario
    result = taf_ks_CryptoSessionCreate(NULL, &sessionRef);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_CryptoSessionCreate***-LE_NOT_FOUND");

    //35.taf_ks_CryptoSessionSetAesNonce -LE_BAD_PARAMETER scenario
    result = taf_ks_CryptoSessionSetAesNonce(sessionRef,NULL,0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_CryptoSessionSetAesNonce***-LE_BAD_PARAMETER");

    //36.taf_ks_CryptoSessionSetAesNonce -LE_NOT_FOUND scenario
    result = taf_ks_CryptoSessionSetAesNonce(NULL,nonce,sizeof(nonce));
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_CryptoSessionSetAesNonce***-LE_NOT_FOUND");

    //37.taf_ks_CryptoSessionSetRsaPadding -LE_BAD_PARAMETER scenario
    result = taf_ks_CryptoSessionSetRsaPadding(sessionRef,2);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_CryptoSessionSetRsaPadding***-LE_BAD_PARAMETER");

    //38.taf_ks_CryptoSessionSetRsaPadding -LE_NOT_FOUND scenario
    result = taf_ks_CryptoSessionSetRsaPadding(NULL,TAF_KS_RSA_PSS);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_CryptoSessionSetRsaPadding***-LE_NOT_FOUND");

    //39.taf_ks_CryptoSessionSetAppData -LE_BAD_PARAMETER scenario
    result = taf_ks_CryptoSessionSetAppData(sessionRef,NULL,0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_CryptoSessionSetAppData***-LE_BAD_PARAMETER");

    //40.taf_ks_CryptoSessionSetAppData -LE_NOT_FOUND scenario
    result = taf_ks_CryptoSessionSetAppData(NULL,appData,sizeof(appData));
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_CryptoSessionSetAppData***-LE_NOT_FOUND");

    //41.taf_ks_CryptoSessionStart -LE_BAD_PARAMETER scenario
    result = taf_ks_CryptoSessionStart(sessionRef,TAF_KS_CRYPTO_MAX);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_CryptoSessionStart***-LE_BAD_PARAMETER");

    //42.taf_ks_CryptoSessionStart -LE_NOT_FOUND  scenario
    result = taf_ks_CryptoSessionStart(NULL,TAF_KS_CRYPTO_MAX-1);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_CryptoSessionStart***-LE_NOT_FOUND");

    //43.taf_ks_CryptoSessionProcessAead -LE_BAD_PARAMETER scenario
    result = taf_ks_CryptoSessionProcessAead(sessionRef,NULL,0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_CryptoSessionProcessAead***-LE_BAD_PARAMETER");

    //44.taf_ks_CryptoSessionProcessAead -LE_NOT_FOUND scenario
    result = taf_ks_CryptoSessionProcessAead(NULL,appData,sizeof(appData));
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_CryptoSessionProcessAead***-LE_NOT_FOUND");

    //45.taf_ks_CryptoSessionProcess -LE_BAD_PARAMETER scenario
    const uint8_t plainText[] = "Keystore service RSA Encryption Descryption test message.";
    uint8_t encryptedData[TAF_KS_MAX_PACKET_SIZE] = { 0 };
    size_t encryptedDataSize = sizeof(encryptedData);
    result = taf_ks_CryptoSessionProcess(sessionRef,
                                         plainText,
                                         0,
                                         encryptedData,
                                         &encryptedDataSize);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_CryptoSessionProcess***-LE_BAD_PARAMETER");

    //46.taf_ks_CryptoSessionProcess -LE_NOT_FOUND scenario
    result = taf_ks_CryptoSessionProcess(NULL,
                                         plainText,
                                         sizeof(plainText),
                                         encryptedData,
                                         &encryptedDataSize);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_CryptoSessionProcess***-LE_NOT_FOUND");

    //47.taf_ks_CryptoSessionAbort -LE_NOT_FOUND scenario
    result = taf_ks_CryptoSessionAbort(NULL);
    LE_TEST_OK(result == LE_NOT_FOUND, "***taf_ks_CryptoSessionAbort***-LE_NOT_FOUND");

    //48.taf_ks_CreateKey -LE_BAD_PARAMETER scenario
    result = taf_ks_CreateKey(keyId_Create,TAF_KS_KEYUSAGE_MAX,NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_ks_CreateKey***-LE_BAD_PARAMETER");
}
