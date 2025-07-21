/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "diagPrivate.h"

// Diag Authentication
static taf_diagAuth_ServiceRef_t svcRef[2] = {NULL};
static taf_diagAuth_RxMsgHandlerRef_t rxMsgHandlerRef[2] = {NULL};
static taf_diagAuth_AuthStateExpHandlerRef_t expHandlerRef[2] = {NULL};

static le_sem_Ref_t semRef[2];

static uint8_t challengeSvr[4096];
static uint64_t role = 0x03;

static void AuthTransCertHandle
(
    taf_diagAuth_RxMsgRef_t rxMsgRef
)
{
    LE_TEST_INFO("AuthTransCertHandle!");

    uint16_t evalId;
    uint16_t certLen;
    le_result_t ret = taf_diagAuth_GetCertEvalId(rxMsgRef, &evalId);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get certificate evaluation id!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    LE_TEST_INFO("certificate evaluation id is 0x%x", evalId);

    ret = taf_diagAuth_GetCertSize(rxMsgRef, &certLen);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get certificate size!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }
    else if (certLen == 0)
    {
        LE_ERROR("certificate size is 0!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_INCORRECT_MSG_LEN_OR_INVALID_FORMAT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    LE_TEST_INFO("certificate size is 0x%x", certLen);

    uint8_t *cert = malloc(certLen);
    if (cert == NULL)
    {
        LE_ERROR("Failed to allocate memory for certificate!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    size_t certBufSize = (size_t)certLen;
    ret = taf_diagAuth_GetCert(rxMsgRef, cert, &certBufSize);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get certificate!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        free(cert);
        return;
    }

    taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_NO_ERROR, TAF_DIAGAUTH_REQUEST_ACCEPTED);
    free(cert);

    return;
}

static void AuthVerifyCertUniHandle
(
    taf_diagAuth_RxMsgRef_t rxMsgRef
)
{
    LE_TEST_INFO("AuthVerifyCertUniHandle!");

    uint8_t commConf;
    uint16_t certLen = 0;
    le_result_t ret = taf_diagAuth_GetCommConf(rxMsgRef, &commConf);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get communication configuration!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    LE_TEST_INFO("communication configuration is 0x%x", commConf);

    if (commConf == 0x09)
    {
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONFIGURATION_DATA_USAGE_FAILED, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    ret = taf_diagAuth_GetCertSize(rxMsgRef, &certLen);
    if (ret != LE_OK || certLen == 0)
    {
        LE_ERROR("Failed to get certificate size!ret=%d", ret);
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }


    LE_TEST_INFO("certificate size is 0x%x", certLen);

    uint8_t *cert = malloc(certLen);
    if (cert == NULL)
    {
        LE_ERROR("Failed to allocate memory for certificate!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    size_t certBufSize = (size_t)certLen;
    ret = taf_diagAuth_GetCert(rxMsgRef, cert, &certBufSize);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get certificate!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        free(cert);
        return;
    }
    else if (certBufSize == 0)
    {
        LE_ERROR("certificate size is 0!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_INCORRECT_MSG_LEN_OR_INVALID_FORMAT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        free(cert);
        return;
    }

    free(cert);

    uint16_t challengeLen;
    ret = taf_diagAuth_GetChallengeSize(rxMsgRef, &challengeLen);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get challenge size!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }
    else if (challengeLen != 0)
    {
        uint8_t *challenge = malloc(challengeLen);
        if (challenge == NULL)
        {
            LE_ERROR("Failed to allocate memory for certificate!");
            if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                    != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

        size_t challengeBufSize = (size_t)challengeLen;
        ret = taf_diagAuth_GetChallenge(rxMsgRef, challenge, &challengeBufSize);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to get certificate!");
            if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                    != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            free(challenge);
            return;
        }
        free(challenge);
    }

    LE_TEST_INFO("challenge size is 0x%x", challengeLen);

    ret = taf_diagAuth_SetChallenge(rxMsgRef, challengeSvr, sizeof(challengeSvr));
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set challenge!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_NO_ERROR, TAF_DIAGAUTH_REQUEST_ACCEPTED);

    return;
}

static void AuthPOWNHandle
(
    taf_diagAuth_RxMsgRef_t rxMsgRef
)
{
    LE_TEST_INFO("AuthPOWNHandle!");

    uint16_t POWNLen;
    le_result_t ret = taf_diagAuth_GetPOWNSize(rxMsgRef, &POWNLen);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get the size of proof of ownership!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }
    else if (POWNLen == 0)
    {
        LE_ERROR("POWN size is 0!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_INCORRECT_MSG_LEN_OR_INVALID_FORMAT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    LE_TEST_INFO("the size of proof of ownership is 0x%x", POWNLen);

    uint8_t *pown = malloc(POWNLen);
    if (pown == NULL)
    {
        LE_ERROR("Failed to allocate memory for POWN!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    size_t pownBufSize = (size_t)POWNLen;
    ret = taf_diagAuth_GetPOWN(rxMsgRef, pown, &pownBufSize);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get certificate!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        free(pown);
        return;
    }

    free(pown);

    uint16_t keyLen;
    ret = taf_diagAuth_GetPublicKeySize(rxMsgRef, &keyLen);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get public key size!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }
    else if (keyLen != 0)
    {
        uint8_t *key = malloc(keyLen);
        if (key == NULL)
        {
            LE_ERROR("Failed to allocate memory for public key!");
            if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                    != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

        size_t keyBufSize = (size_t)keyLen;
        ret = taf_diagAuth_GetPublicKey(rxMsgRef, key, &keyBufSize);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to get public key!");
            if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                    != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            free(key);
            return;
        }
        free(key);
    }

    ret = taf_diagAuth_SetRole(rxMsgRef, role);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set role id!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    uint8_t sessKey[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA};
    ret = taf_diagAuth_SetSessKeyInfo(rxMsgRef, sessKey, sizeof(sessKey));
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set session key info!");
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_NO_ERROR, TAF_DIAGAUTH_REQUEST_ACCEPTED);

    return;
}

static void DeAuthHandle
(
    taf_diagAuth_RxMsgRef_t rxMsgRef
)
{
    LE_TEST_INFO("DeAuthHandle!");

    taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_NO_ERROR, TAF_DIAGAUTH_REQUEST_ACCEPTED);

    return;
}

static void AuthConfigHandle
(
    taf_diagAuth_RxMsgRef_t rxMsgRef
)
{
    LE_TEST_INFO("AuthConfigHandle!");

    taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_NO_ERROR, TAF_DIAGAUTH_REQUEST_ACCEPTED);

    return;
}

// Callback function for Authentication request message
static void AuthMsgHandler
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    taf_diagAuth_Type_t authTaskType,
    void* contextPtr
)
{
    LE_TEST_INFO("AuthMsgHandler!");
    LE_TEST_INFO("Received auth type: %d", authTaskType);

    uint16_t vlanId = 0;
    if (taf_diagAuth_GetVlanIdFromMsg(rxMsgRef, &vlanId) != LE_OK)
    {
        LE_ERROR("Failed to get vlan id");
        if(taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    LE_TEST_INFO("Received a auth request from vlan0x%x", vlanId);

    if (authTaskType == TAF_DIAGAUTH_TRANSMIT_CERT)  // 0x04
    {
        AuthTransCertHandle(rxMsgRef);
    }
    else if (authTaskType == TAF_DIAGAUTH_VERIFY_CERT_UNI)  // 0x01
    {
        AuthVerifyCertUniHandle(rxMsgRef);
    }
    else if (authTaskType == TAF_DIAGAUTH_PROOF_OF_OWNERSHIP)  // 0x03
    {
        AuthPOWNHandle(rxMsgRef);
    }
    else if (authTaskType == TAF_DIAGAUTH_DEAUTH)  // 0x00
    {
        DeAuthHandle(rxMsgRef);
    }
    else if (authTaskType == TAF_DIAGAUTH_CONFIG_AUTH)  // 0x08
    {
        AuthConfigHandle(rxMsgRef);
    }
    else
    {
        LE_ERROR("authTaskType%d is unknown!", authTaskType);
        if (taf_diagAuth_SendResp(rxMsgRef, TAF_DIAGAUTH_CONDITIONS_NOT_CORRECT, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }
}

static void AuthExpNotifyHandler
(
    taf_diagAuth_StateExpMsgRef_t expMsg,
    uint16_t vlanId,
    uint64_t roleId,
    void* contextPtr
)
{
    LE_TEST_INFO("AuthExpNotifyHandler!");
    LE_TEST_INFO("vlan id: %d, role id: %" PRIu64, vlanId, roleId);
}

static void *diagAuthMsgThread
(
    void* ctxPtr
)
{
    le_result_t result;
    unsigned long idx = (unsigned long)(uintptr_t)ctxPtr;
    uint16_t vlanId;
    taf_diagAuth_ConnectService();

    //get diag Auth svc reference
    svcRef[idx] = taf_diagAuth_GetService();
    if(svcRef[idx] == NULL)
    {
        LE_ERROR("Get authentication service error");
        return (void*)LE_FAULT;
    }

    if (idx == 0ul)
    {
        vlanId = TEST_VLAN_ID_0;
    }
    else
    {
        vlanId = TEST_VLAN_ID_1;
    }

    result = taf_diagAuth_SetVlanId(svcRef[idx], vlanId);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to set vlan id for this service");
        return (void*)LE_FAULT;
    }

    rxMsgHandlerRef[idx] = taf_diagAuth_AddRxMsgHandler(svcRef[idx], AuthMsgHandler, NULL);
    LE_TEST_OK(rxMsgHandlerRef[idx] != NULL, "Registered successfully for AuthMsgHandler");

    expHandlerRef[idx] = taf_diagAuth_AddAuthStateExpHandler(svcRef[idx],
        AuthExpNotifyHandler, NULL);
    LE_TEST_OK(expHandlerRef[idx] != NULL, "Registered successfully for AuthExpNotifyHandler");

    le_sem_Post(semRef[idx]);
    le_event_RunLoop();
    return NULL;
}

le_result_t diagVlanAuth_Init
(
    uint64_t roleValue
)
{
    LE_TEST_INFO("diagVlanAuth_Init with role value: %" PRIuS, roleValue);
    role = roleValue;

    semRef[0] = le_sem_Create("SemRef0", 0);
    semRef[1] = le_sem_Create("SemRef1", 0);

    // Create diag auth message handle thread to handle auth request(0x29)
    le_thread_Ref_t authThreadRef = le_thread_Create("AuthThread",
            diagAuthMsgThread, (void *)(uintptr_t)0);
    le_thread_Ref_t authThreadRef1 = le_thread_Create("AuthThread",
            diagAuthMsgThread, (void *)(uintptr_t)1);

    le_thread_Start(authThreadRef);
    le_thread_Start(authThreadRef1);
    le_sem_Wait(semRef[0]);
    le_sem_Wait(semRef[1]);

    return LE_OK;
}