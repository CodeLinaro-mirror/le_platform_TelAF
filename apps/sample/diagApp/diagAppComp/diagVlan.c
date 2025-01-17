/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "diagPrivate.h"

//Diag
static taf_diag_ServiceRef_t svcRef[2] = {NULL};
static taf_diag_TesterStateHandlerRef_t diagTesterMsgRef[2] = {NULL};

static le_sem_Ref_t semRef[2];

/*
 * Callback function for tester state notification
 */
static void TesterStateHandler
(
    taf_diag_TesterStateRef_t stateRef,
    uint16_t vlanId,
    taf_diag_State_t state,
    void* contextPtr
)
{
    LE_TEST_INFO("TesterStateHandler");

    // Print the current tester state.
    LE_TEST_INFO("Tester state ref: %p", stateRef);
    LE_TEST_INFO("Received a tester state notification from vlan 0x%x", vlanId);
    LE_TEST_INFO("Current tester state: %d", state);

    return;
}

/*
 * Thread's main function
 */
static void* diagTesterStateMsgThread
(
    void* ctxPtr
)
{
    LE_TEST_INFO("diagTesterStateMsgThread");

    unsigned long idx = (unsigned long)(uintptr_t)ctxPtr;
    taf_diag_ConnectService();

    // Define vlan id
    uint16_t vlanId;

    // Get diag svc reference
    svcRef[idx] = taf_diag_GetService();
    if(svcRef[idx] == NULL)
    {
        LE_ERROR("Get Diag service error");
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

    le_result_t result;
    result = taf_diag_SetVlanId(svcRef[idx], vlanId);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to set vlan id for this service");
        return (void*)LE_FAULT;
    }

    // Register tester state notification handler for Vlan id = 10
    diagTesterMsgRef[idx] = taf_diag_AddTesterStateHandler(svcRef[idx],
            TesterStateHandler, NULL);
    LE_TEST_OK(diagTesterMsgRef[idx] != NULL,
            "Registered successfully for TesterStateHandler for vlanId1");

    le_sem_Post(semRef[idx]);
    le_event_RunLoop();

    return NULL;
}

/*
 * Init
 */
le_result_t diagVlanDiag_Init
(
    void
)
{
    LE_TEST_INFO("Init");

    semRef[0] = le_sem_Create("SemRef0", 0);
    semRef[1] = le_sem_Create("SemRef1", 0);

    // Create diag tester state message handler thread. Client session 1
    le_thread_Ref_t testerStateThreadRef1 = le_thread_Create("testerStateThread1",
            diagTesterStateMsgThread, (void *)(uintptr_t)0);

    // Create diag tester state message handler thread. Client session 2
    le_thread_Ref_t testerStateThreadRef2 = le_thread_Create("testerStateThread2",
            diagTesterStateMsgThread, (void *)(uintptr_t)1);

    le_thread_Start(testerStateThreadRef1);
    le_thread_Start(testerStateThreadRef2);
    le_sem_Wait(semRef[0]);
    le_sem_Wait(semRef[1]);

    return LE_OK;
}