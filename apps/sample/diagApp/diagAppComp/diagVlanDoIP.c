/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "diagPrivate.h"

// Diag DoIP service reference
static taf_diagDoIP_ServiceRef_t diagDoIPSvcRef[2] = {NULL};
static taf_diagDoIP_EventHandlerRef_t diagDoIPEventRef[2] = {NULL};
static le_sem_Ref_t semRef[2];

// Callback function for DoIP event
static void DoIPEventHandler
(
    taf_diagDoIP_ServiceRef_t svrRef,
    taf_diagDoIP_EventType_t eventType,
    uint16_t remoteLogicAddr,
    uint16_t vlanId,
    void* contextPtr
)
{
    LE_TEST_INFO("Received a DoIP event notification");
    LE_TEST_INFO("Event type is 0x%x, client logical address is 0x%x, vlan0x%x",
        (uint32_t)eventType, remoteLogicAddr, vlanId);
}

static void* diagDoIPEventThread
(
    void* ctxPtr
)
{
    unsigned long idx = (unsigned long)(uintptr_t)ctxPtr;
    uint16_t vlanId;

    taf_diagDoIP_ConnectService();

    diagDoIPSvcRef[idx] = taf_diagDoIP_GetService(0x0201);
    LE_TEST_ASSERT(diagDoIPSvcRef[idx] != NULL, "taf_diagDoIP_GetService");

    if (idx == 0ul)
    {
        vlanId = TEST_VLAN_ID_0;
    }
    else
    {
        vlanId = TEST_VLAN_ID_1;
    }

    diagDoIPEventRef[idx] = taf_diagDoIP_AddEventHandler(diagDoIPSvcRef[idx], vlanId, DoIPEventHandler, NULL);
    LE_TEST_ASSERT(diagDoIPEventRef[idx] != NULL, "Registered DoIPEventHandler");

    le_sem_Post(semRef[idx]);
    le_event_RunLoop();
    return NULL;
}

le_result_t diagVlanDoIP_Init
(
    void
)
{
    semRef[0] = le_sem_Create("SemRef0", 0);
    semRef[1] = le_sem_Create("SemRef1", 0);

    // Create diag DoIP event handle thread to handle related events
    le_thread_Ref_t diagDoIPThrRef = le_thread_Create("diagDoIPThr0",
            diagDoIPEventThread, (void *)(uintptr_t)0);

    le_thread_Start(diagDoIPThrRef);
    le_sem_Wait(semRef[0]);

    le_thread_Ref_t diagDoIPThrRef1 = le_thread_Create("diagDoIPThr1",
            diagDoIPEventThread, (void *)(uintptr_t)1);

    le_thread_Start(diagDoIPThrRef1);
    le_sem_Wait(semRef[1]);

    return LE_OK;
}