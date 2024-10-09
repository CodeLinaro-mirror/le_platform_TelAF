/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "diagPrivate.h"

//Diag Reset
static taf_diagReset_ServiceRef_t diagResetSvcRef[2] = {NULL};
static taf_diagReset_RxMsgHandlerRef_t diagResetMsgRef[2] = {NULL};

static le_sem_Ref_t semRef[2];

// Callback function for reset request message
// Hard reset type is used to reboot to active after firmware is installed in this sample.
static void resetMsgHandler
(
    taf_diagReset_RxMsgRef_t rxMsgRef,
    uint8_t resetType,
    void* contextPtr
)
{
    uint16_t vlanId = 0;
    if (taf_diagReset_GetVlanIdFromMsg(rxMsgRef, &vlanId) != LE_OK)
    {
        LE_ERROR("Failed to get vlan id");
        if(taf_diagReset_SendResp(rxMsgRef, TAF_DIAGRESET_CONDITIONS_NOT_CORRECT) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    LE_TEST_INFO("Received a reset request from vlan0x%x", vlanId);
    if(taf_diagReset_SendResp(rxMsgRef, TAF_DIAGRESET_NO_ERROR ) != LE_OK)
    {
        LE_ERROR("Send response error");
    }
}

static void* diagResetMsgThread(void* ctxPtr)
{
    le_result_t result;
    unsigned long idx = (unsigned long)(uintptr_t)ctxPtr;
    uint16_t vlanId;
    taf_diagReset_ConnectService();

    //get diag reset svc reference
    diagResetSvcRef[idx] = taf_diagReset_GetService(TAF_DIAGRESET_ALL_RESET);
    if(diagResetSvcRef[idx] == NULL)
    {
        LE_ERROR("Get diagReset service");
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

    result = taf_diagReset_SetVlanId(diagResetSvcRef[idx], vlanId);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to set vlan id for this service");
        return (void*)LE_FAULT;
    }

    diagResetMsgRef[idx] = taf_diagReset_AddRxMsgHandler(diagResetSvcRef[idx], resetMsgHandler, NULL);
    LE_TEST_OK(diagResetMsgRef[idx] != NULL, "Registered successfully for resetMsgHandler");

    le_sem_Post(semRef[idx]);
    le_event_RunLoop();
    return NULL;
}

le_result_t diagVlanReset_Init(void)
{
    semRef[0] = le_sem_Create("SemRef0", 0);
    semRef[1] = le_sem_Create("SemRef1", 0);

    // Create diag reset message handle thread to handle ECUReset request(0x11)
    le_thread_Ref_t resetThreadRef = le_thread_Create("resetThread",
            diagResetMsgThread, (void *)(uintptr_t)0);

    le_thread_Ref_t resetThreadRef1 = le_thread_Create("resetThread",
            diagResetMsgThread, (void *)(uintptr_t)1);

    le_thread_Start(resetThreadRef);
    le_thread_Start(resetThreadRef1);
    le_sem_Wait(semRef[0]);
    le_sem_Wait(semRef[1]);

    return LE_OK;
}