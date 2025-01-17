/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "diag.h"

//Diag
static taf_diag_ServiceRef_t svcRef = NULL;
static taf_diag_TesterStateHandlerRef_t diagTesterStateRef = NULL;

static le_sem_Ref_t semRef;

#define MAX_ENABLE_CONDITION_ID 14

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
 * Diag tetsre state main thread function
 */
static void* diagTesterStateThread
(
    void* ctxPtr
)
{
    LE_TEST_INFO("diagTesterStateThread");

    taf_diag_ConnectService();

    // Register tester handler for tester state notification
    diagTesterStateRef = taf_diag_AddTesterStateHandler(svcRef, TesterStateHandler, NULL);
    LE_TEST_OK(diagTesterStateRef != NULL, "Registered successfully for TesterStateHandler");

    le_sem_Post(semRef);
    le_event_RunLoop();

    return NULL;
}

/*
 * Diag common init
 */
le_result_t diagCommon_Init
(
    void
)
{
    LE_TEST_INFO("diagCommon_Init");

    semRef = le_sem_Create("SemRef", 0);

    // Get diag svc reference
    svcRef = taf_diag_GetService();
    if(svcRef == NULL)
    {
        LE_ERROR("Get diag service reference error");
        return LE_FAULT;
    }

    // Create diag message handler thread to handle tester state.
    le_thread_Ref_t testerStateThreadRef = le_thread_Create("testerStateThread",
            diagTesterStateThread, NULL);

    le_thread_Start(testerStateThreadRef);
    le_sem_Wait(semRef);

    return LE_OK;
}

/*
 * Main function
 */
COMPONENT_INIT
{
    LE_INFO("%s [start]", __FUNCTION__);

    int numberOfArgs = le_arg_NumArgs();
    LE_INFO("Total numberOfArgs count: %d", numberOfArgs);

    if (numberOfArgs == 2)
    {
        LE_INFO("set enable status and multiLkan from cmd line argument");
        const char* enableStatusPtr = le_arg_GetArg(0);
        const char* vlanTypePtr = le_arg_GetArg(1);
        if (enableStatusPtr == NULL || vlanTypePtr == NULL)
        {
            LE_ERROR("enableStatusPtr or vlanTypePtr is NULL");
            return;
        }

        le_result_t result;
        if (strcmp(enableStatusPtr,"enableStatusTrue") == 0)
        {
            LE_INFO("set enable status as True");

            for (int enableID = 1; enableID <= MAX_ENABLE_CONDITION_ID; enableID++)
            {
                result = taf_diag_SetEnableCondition(enableID, true);
                if (result != LE_OK)
                {
                    LE_ERROR("Failed to set enable condition for %d", enableID);
                    return;
                }
            }
        }
        else if (strcmp(enableStatusPtr,"enableStatusFalse") == 0)
        {
            LE_INFO("set enable status as False");

            for (int enableID = 1; enableID <= MAX_ENABLE_CONDITION_ID; enableID++)
            {
                result = taf_diag_SetEnableCondition(enableID, false);
                if (result != LE_OK)
                {
                    LE_ERROR("Failed to set enable condition for %d", enableID);
                    return;;
                }
            }
        }
        else
        {
            printf("\n === Enable condition argument is not correct ===\n");
            LE_ERROR("Enable condion argument is not correct");
            return;
        }

        if (strcmp(vlanTypePtr,"multiVlan") == 0)
        {
            LE_INFO("multi vlan");
            // Multi-VLAN test
            LE_INFO("=====> Enter multi-VLAN sample.");
            LE_FATAL_IF(diagVlanDiag_Init() != LE_OK,
                    "diagVlan_Init -> init failed");
            LE_FATAL_IF(diagVlanDoIP_Init() != LE_OK,
                    "diagVlanDoIP_Init -> init failed");
            LE_ERROR_IF(diagVlanReadWriteDid_Init() != LE_OK,
                    "diagVlanReadWriteDid_Init -> init failed");
            LE_FATAL_IF(diagVlanSecurityAccess_Init() != LE_OK,
                    "diagVlanSecurityAccess_Init -> init failed");
            LE_FATAL_IF(diagVlanRequestFileTransfer_Init() != LE_OK,
                    "diagVlanRequestFileTransfer_Init -> init failed");
            LE_FATAL_IF(diagVlanReset_Init() != LE_OK,
                    "diagVlanReset_Init -> init failed");
            LE_FATAL_IF(diagVlanRoutineControl_Init() != LE_OK,
                    "diagVlanRoutineControl_Init -> init failed");
            LE_FATAL_IF(diagVlanIOControl_Init() != LE_OK,
                    "diagVlanIOControl_Init -> init failed");
            LE_FATAL_IF(diagVlanAuth_Init() != LE_OK,
                    "diagVlanAuth_Init -> init failed");
        }
        else if (strcmp(vlanTypePtr,"nonMultiVlan") == 0)
        {
            LE_INFO("non multi vlan");
            LE_ERROR_IF(diagCommon_Init() != LE_OK,
                    "diagCommon_Init -> init failed");
            LE_ERROR_IF(diagReadWriteDid_Init() != LE_OK,
                    "diagReadWriteDid_Init -> init failed");
            LE_FATAL_IF(diagSecurityAccess_Init() != LE_OK,
                    "diagSecurityAccess_Init -> init failed");
            LE_FATAL_IF(diagRequestFileTransfer_Init() != LE_OK,
                    "diagRequestFileTransfer_Init -> init failed");
            LE_FATAL_IF(diagReset_Init() != LE_OK,
                    "diagReset_Init -> init failed");
            LE_FATAL_IF(diagRoutineControl_Init() != LE_OK,
                    "diagRoutineControl_Init -> init failed");
            LE_FATAL_IF(diagIOControl_Init() != LE_OK,
                    "diagIOControl_Init -> init failed");
            LE_FATAL_IF(diagAuth_Init() != LE_OK,
                    "diagAuth_Init -> init failed");
#ifndef LE_CONFIG_DIAG_VSTACK
            LE_FATAL_IF(diagDoIP_Init() != LE_OK,
                    "diagDoIP_Init -> init failed");
#endif
        }
        else
        {
            printf("\n === multi vlan type argument is not correct ===\n");
            LE_ERROR("multi vlan type argument is not correct");
            return;
        }
    }
    else
    {
        printf("Please follow the instructions and passed argument as mentioned\n");
        printf("Set argument with enableStatusTrue/enableStatusFalse multiVlan/nonMultiVlan\n");
        printf("=== eg: app runProc tafDiagApp --exe=tafDiagApp -- enableStatusTrue multiVlan \n");
        LE_TEST_EXIT;
    }

    LE_INFO("%s [done]", __FUNCTION__);

}
