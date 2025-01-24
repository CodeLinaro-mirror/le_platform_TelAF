/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "diagPrivate.h"

//Diag Session/Security
static taf_diagSecurity_ServiceRef_t diagVlanSecSvcRef[2] = {NULL};
static taf_diagSecurity_RxSesTypeCheckHandlerRef_t diagVlanSesTypeMsgRef[2] = {NULL};
static taf_diagSecurity_SesChangeHandlerRef_t diagVlanSesChangeRef[2] = {NULL};
static taf_diagSecurity_RxSecAccessMsgHandlerRef_t diagVlanSecMsgRef[2] = {NULL};

static le_sem_Ref_t semRef[2];

static const uint8_t seedData[] = {0x36, 0x57};

// Callback function for sessionCtrl request message
static void sesTypeMsgHandler
(
    taf_diagSecurity_RxSesTypeCheckRef_t rxMsgRef,
    uint8_t SesCtrlType,
    void* contextPtr
)
{
    LE_TEST_INFO("Received session control type req msg");
    LE_TEST_INFO("Received session control type is %x", SesCtrlType);

    uint16_t vlanId = 0;
    if (taf_diagSecurity_GetVlanIdFromMsg((taf_diagSecurity_RxMsgRef_t)rxMsgRef, &vlanId) != LE_OK)
    {
        LE_ERROR("Failed to get vlan id");
        if(taf_diagSecurity_SendSesTypeCheckResp( rxMsgRef,
            TAF_DIAGSECURITY_SES_CONTROL_CONDITIONS_NOT_CORRECT) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    LE_TEST_INFO("Received a session type request from vlan0x%x", vlanId);

    le_result_t result;

    result = taf_diagSecurity_SendSesTypeCheckResp(rxMsgRef,
        TAF_DIAGSECURITY_SES_CONTROL_NO_ERROR);
    if (result == LE_OK)
    {
        LE_TEST_INFO("Session response is sent");
    }
}

// Callback function for session change message
static void sesChangeHandler
(
    taf_diagSecurity_SesChangeRef_t sesChangeRef,
    uint8_t PreviousType,
    uint8_t CurrentType,
    void* contextPtr
)
{
    LE_TEST_INFO("Previous session type: %x, Current session type: %x", PreviousType, CurrentType);

    unsigned long idx = (unsigned long)(uintptr_t)contextPtr;
    uint8_t currentSesType;

    uint16_t vlanId = 0;
    le_result_t result = LE_OK;

    result = taf_diagSecurity_GetVlanIdFromMsg((taf_diagSecurity_RxMsgRef_t)sesChangeRef, &vlanId);
    LE_ASSERT(result == LE_OK);

    if (PreviousType == 0x02 && CurrentType != 0x02)
    {
        LE_INFO("Deactivate programming --> release all resources");
        diagRFT_DeactivateProgrammingByVlanId((uint32_t)vlanId);
    }

    result = taf_diagSecurity_SelectTargetVlanID(diagVlanSecSvcRef[idx], vlanId);
    if (result == LE_OK)
    {
        LE_TEST_INFO("VlanId selected for session type %x", vlanId);
    }

    result = taf_diagSecurity_GetCurrentSesType(diagVlanSecSvcRef[idx], &currentSesType);
    if (result == LE_OK)
    {
        LE_TEST_INFO("Current active session type is %x", currentSesType);
    }

    // Release the session change msg.
    result = taf_diagSecurity_ReleaseSesChangeMsg(sesChangeRef);
    LE_TEST_OK(result == LE_OK, "Session change msg released successfully");

    return;
}

// Callback function for security request message
static void securityMsgHandler
(
    taf_diagSecurity_RxSecAccessMsgRef_t rxMsgRef,
    uint8_t accessType,
    void* contextPtr
)
{
    size_t seedDataLen = 0, keyDataLen = 0;
    uint8_t keyData[TAF_DIAGSECURITY_MAX_SEC_ACCESS_PAYLOAD_SIZE];
    uint8_t securityAccessDataRecord[TAF_DIAGSECURITY_MAX_SEC_ACCESS_PAYLOAD_SIZE];
    size_t securityAccessDataRecordSize = 0;

    le_result_t result;

    LE_TEST_INFO("Received security req msg access type: %d", accessType);

    uint16_t vlanId = 0;
    if (taf_diagSecurity_GetVlanIdFromMsg((taf_diagSecurity_RxMsgRef_t)rxMsgRef, &vlanId) != LE_OK)
    {
        LE_ERROR("Failed to get vlan id");
        if(taf_diagSecurity_SendSecAccessResp( rxMsgRef,
            TAF_DIAGSECURITY_SEC_ACCESS_CONDITIONS_NOT_CORRECT,
                NULL, 0 ) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    LE_TEST_INFO("Received a security request from vlan0x%x", vlanId);

    //Send seed response
    if(accessType %2 != 0)
    {
        result = taf_diagSecurity_GetSecAccessPayloadLen(
                                        rxMsgRef,
                                        (uint16_t *)&securityAccessDataRecordSize);
        if(result != LE_OK)
        {
            LE_ERROR("API GetSecAccessPayloadLen");
            // UDS_0x27_NRC_22: API GetSecAccessPayloadLen
            if(taf_diagSecurity_SendSecAccessResp(
                    rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_CONDITIONS_NOT_CORRECT,
                    NULL, 0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

        result = taf_diagSecurity_GetSecAccessPayload(rxMsgRef,
                                                      securityAccessDataRecord,
                                                      &securityAccessDataRecordSize);
        if(result != LE_OK)
        {
            LE_ERROR("API GetSecAccessPayload");
            // UDS_0x27_NRC_22: API GetSecAccessPayload
            if(taf_diagSecurity_SendSecAccessResp(
                    rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_CONDITIONS_NOT_CORRECT,
                    NULL, 0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

        LE_INFO("Request-Seed: securityAccessDataRecord size is [%" PRIuS "]",
                securityAccessDataRecordSize);

        #define isValidSecurityAccessDataRecord(record, size) (1)
        if (! isValidSecurityAccessDataRecord(securityAccessDataRecord,
                                              securityAccessDataRecordSize))
        {
            LE_ERROR("Invalid securityAccessDataRecord");
            // UDS_0x27_NRC_31: Invalid securityAccessDataRecord
            if(taf_diagSecurity_SendSecAccessResp(
                    rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_REQUEST_OUT_OF_RANGE,
                    NULL, 0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

        seedDataLen = sizeof(seedData);

        if(taf_diagSecurity_SendSecAccessResp( rxMsgRef,
                TAF_DIAGSECURITY_SEC_ACCESS_NO_ERROR, seedData, seedDataLen ) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
    }
    //Validate the key
    else
    {
        result = taf_diagSecurity_GetSecAccessPayloadLen( rxMsgRef, (uint16_t *)&keyDataLen);
        if(result != LE_OK || keyDataLen != sizeof(seedData) ||
                keyDataLen > TAF_DIAGSECURITY_MAX_SEC_ACCESS_PAYLOAD_SIZE )
        {
            LE_ERROR("Getting key len");
            // UDS_0x27_NRC_35: Send-Key size is bad
            if(taf_diagSecurity_SendSecAccessResp( rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_INVALID_KEY, NULL,0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

        result = taf_diagSecurity_GetSecAccessPayload( rxMsgRef, keyData, &keyDataLen);

        if(result != LE_OK)
        {
            LE_ERROR("Getting key data");
            // UDS_0x27_NRC_22: API GetSecAccessPayload (send-key)
            if(taf_diagSecurity_SendSecAccessResp( rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_CONDITIONS_NOT_CORRECT, NULL, 0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

        //Validate the key, the algorithum is same as the python cliet tool
        if(keyData[0] == seedData[0]+1 && keyData[1] == seedData[1]+2)
        {
            if(taf_diagSecurity_SendSecAccessResp( rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_NO_ERROR, NULL,0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }

        }
        // Key is invalid
        else
        {
            // UDS_0x27_NRC_35: Got invalid key by API
            if(taf_diagSecurity_SendSecAccessResp( rxMsgRef,
                    TAF_DIAGSECURITY_SEC_ACCESS_INVALID_KEY, NULL,0) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
        }
    }

    return;
}



static void* diagSecurityMsgThread(void* ctxPtr)
{
    unsigned long idx = (unsigned long)(uintptr_t)ctxPtr;
    uint16_t vlanId;
    le_result_t result;
    uint8_t currentSesType;

    taf_diagSecurity_ConnectService();

    if (idx == 0ul)
    {
        vlanId = TEST_VLAN_ID_0;
    }
    else
    {
        vlanId = TEST_VLAN_ID_1;
    }

    //get diag security reference
    diagVlanSecSvcRef[idx] = taf_diagSecurity_GetService();
    if(diagVlanSecSvcRef[idx] == NULL)
    {
        LE_ERROR("Get diagSecurity service");
        return (void*)LE_FAULT;
    }

    result = taf_diagSecurity_SetVlanId(diagVlanSecSvcRef[idx], vlanId);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to set vlan id for this service");
        return (void*)LE_FAULT;
    }

    result = taf_diagSecurity_SelectTargetVlanID(diagVlanSecSvcRef[idx], vlanId);
    if (result == LE_OK)
    {
        LE_TEST_INFO("VlanId selected for session type %x", vlanId);
    }

    result = taf_diagSecurity_GetCurrentSesType(diagVlanSecSvcRef[idx], &currentSesType);
    if (result == LE_OK)
    {
        LE_TEST_INFO("Current active session type is %x", currentSesType);
    }

    diagVlanSesTypeMsgRef[idx] = taf_diagSecurity_AddRxSesTypeCheckHandler( diagVlanSecSvcRef[idx],
            sesTypeMsgHandler, NULL);
    LE_TEST_OK(diagVlanSesTypeMsgRef[idx] != NULL, "Registered successfully for sesTypeMsgHandler");

    diagVlanSesChangeRef[idx] = taf_diagSecurity_AddSesChangeHandler( diagVlanSecSvcRef[idx],
            sesChangeHandler, ctxPtr);
    LE_TEST_OK(diagVlanSesChangeRef[idx] != NULL, "Registered successfully for sesChangeHandler");

    diagVlanSecMsgRef[idx] = taf_diagSecurity_AddRxSecAccessMsgHandler( diagVlanSecSvcRef[idx],
            securityMsgHandler, NULL);
    LE_TEST_OK(diagVlanSecMsgRef[idx] != NULL, "Registered successfully for securityMsgHandler");

    le_sem_Post(semRef[idx]);
    le_event_RunLoop();
    return NULL;
}

le_result_t diagVlanSecurityAccess_Init(void)
{
    semRef[0] = le_sem_Create("SemRef0", 0);
    semRef[1] = le_sem_Create("SemRef1", 0);

    // Create diag security message handle thread to handle security access(0x27)
    le_thread_Ref_t diagSecurityThreadRef = le_thread_Create("diagSecurityTd0",
            diagSecurityMsgThread, (void *)(uintptr_t)0);

    // Create diag security message handle thread to handle security access(0x27)
    le_thread_Ref_t diagSecurityThreadRef1 = le_thread_Create("diagSecurityTd1",
            diagSecurityMsgThread, (void *)(uintptr_t)1);

    le_thread_Start(diagSecurityThreadRef);
    le_thread_Start(diagSecurityThreadRef1);
    le_sem_Wait(semRef[0]);
    le_sem_Wait(semRef[1]);

    return LE_OK;
}
