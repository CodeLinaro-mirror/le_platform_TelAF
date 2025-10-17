/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define VLAN_ID_10 10
#define VLAN_ID_110 110

void TestPauseDiagWithoutVlan()
{
    // Get diag svc reference
    taf_diag_ServiceRef_t svcRef = taf_diag_GetService();
    if(svcRef == NULL)
    {
        LE_ERROR("Get diag service reference error");
        return;
    }

    le_result_t result = taf_diag_Pause(svcRef);
    LE_INFO("Pause diag without VLAN result: %d", result);
}

void TestPauseDiagWithVlan()
{
    le_result_t result;
    // Get diag svc reference
    taf_diag_ServiceRef_t svcRef = taf_diag_GetService();
    if(svcRef == NULL)
    {
        LE_ERROR("Get diag service reference error");
        return;
    }

    // Associate all VLANs to svcRef, here set VLAN 10
    result = taf_diag_SetVlanId(svcRef, VLAN_ID_10);
    if(result != LE_OK)
    {
        LE_ERROR("Set VLAN Id 10 error");
        return;
    }

    // Set VLAN 110
    result = taf_diag_SetVlanId(svcRef, VLAN_ID_110);
    if(result != LE_OK)
    {
        LE_ERROR("Set VLAN Id 110 error");
        return;
    }

    // Select each VLAN and pause diag for each VLAN, here select VLAN 10
    result = taf_diag_SelectTargetVlanID(svcRef, VLAN_ID_10);
    if(result != LE_OK)
    {
        LE_ERROR("Select target VLAN Id 10 error");
        return;
    }

    // Pause diag with VLAN Id 10
    result = taf_diag_Pause(svcRef);
    LE_INFO("Pause diag with VLAN Id 10, result: %d", result);

    // Select VLAN 110
    result = taf_diag_SelectTargetVlanID(svcRef, VLAN_ID_110);
    if(result != LE_OK)
    {
        LE_ERROR("Select target VLAN Id 110 error");
        return;
    }

    // Pause diag with VLAN Id 110
    result = taf_diag_Pause(svcRef);
    LE_INFO("Pause diag with VLAN Id 110, result: %d", result);
}

void TestResumeDiagWithoutVlan()
{
    // Get diag svc reference
    taf_diag_ServiceRef_t svcRef = taf_diag_GetService();
    if(svcRef == NULL)
    {
        LE_ERROR("Get diag service reference error");
        return;
    }

    le_result_t result = taf_diag_Resume(svcRef);
    LE_INFO("Resume diag without VLAN, result: %d", result);
}

void TestResumeDiagWithVlan()
{
    le_result_t result;
    // Get diag svc reference
    taf_diag_ServiceRef_t svcRef = taf_diag_GetService();
    if(svcRef == NULL)
    {
        LE_ERROR("Get diag service reference error");
        return;
    }

    // Associate all VLANs to svcRef, here set VLAN 10
    result = taf_diag_SetVlanId(svcRef, VLAN_ID_10);
    if(result != LE_OK)
    {
        LE_ERROR("Set VLAN Id 10 error");
        return;
    }

    // Set VLAN 110
    result = taf_diag_SetVlanId(svcRef, VLAN_ID_110);
    if(result != LE_OK)
    {
        LE_ERROR("Set VLAN Id 110 error");
        return;
    }

    // Select each VLAN and resume diag for each VLAN, here select VLAN 10
    result = taf_diag_SelectTargetVlanID(svcRef, VLAN_ID_10);
    if(result != LE_OK)
    {
        LE_ERROR("Select target VLAN Id 10 error");
        return;
    }

    // Resume diag with VLAN Id 10
    result = taf_diag_Resume(svcRef);
    LE_INFO("Resume diag with VLAN Id 10, result: %d", result);

    // Select VLAN 110
    result = taf_diag_SelectTargetVlanID(svcRef, VLAN_ID_110);
    if(result != LE_OK)
    {
        LE_ERROR("Select target VLAN Id 110 error");
        return;
    }

    // Resume diag with VLAN Id 110
    result = taf_diag_Resume(svcRef);
    LE_INFO("Resume diag with VLAN Id 110, result: %d", result);
}

void TestShutdownDoIPConnection()
{
    le_result_t result;

    // Get diag svc reference
    taf_diag_ServiceRef_t svcRef = taf_diag_GetService();
    if(svcRef == NULL)
    {
        LE_ERROR("Get diag service reference error");
        return;
    }

    result = taf_diag_Shutdown(svcRef);
    LE_INFO(" Shutdown diag service, result is %d ", result);
}

COMPONENT_INIT
{
    LE_INFO("%s [Test start]", __FUNCTION__);

    int numberOfArgs = le_arg_NumArgs();
    LE_INFO("Total numberOfArgs count: %d", numberOfArgs);

    if (numberOfArgs == 1)
    {
        const char* actionPtr = le_arg_GetArg(0);

        if (actionPtr == NULL)
        {
            LE_ERROR("actionPtr is NULL");
            return;
        }

        if (strcmp(actionPtr,"shutdown") == 0)
        {
            TestShutdownDoIPConnection();
        }
        else
        {
            printf("\n === action type argument is not correct ===\n");
            LE_ERROR("action type argument is not correct");
            return;
        }
    }
    else if (numberOfArgs == 2)
    {

        const char* actionPtr = le_arg_GetArg(0);

        if (actionPtr == NULL)
        {
            LE_ERROR("actionPtr is NULL");
            return;
        }

        const char* vlanTypePtr = le_arg_GetArg(1);

        if (vlanTypePtr == NULL)
        {
            LE_ERROR("vlanTypePtr is NULL");
            return;
        }

        if (strcmp(actionPtr,"pause") == 0)
        {
            if (strcmp(vlanTypePtr,"vlan") == 0)
            {
                LE_INFO("pause with vlan");
                TestPauseDiagWithVlan();
            }
            else if (strcmp(vlanTypePtr,"nonVlan") == 0)
            {
                LE_INFO("pause without vlan");
                TestPauseDiagWithoutVlan();
            }
            else
            {
                printf("\n === vlan type argument is not correct ===\n");
                LE_ERROR("vlan type argument is not correct");
                return;
            }
        }
        else if (strcmp(actionPtr,"resume") == 0)
        {

            if (strcmp(vlanTypePtr,"vlan") == 0)
            {
                LE_INFO("resume with vlan");
                TestResumeDiagWithVlan();
            }
            else if (strcmp(vlanTypePtr,"nonVlan") == 0)
            {
                LE_INFO("resume without vlan");
                TestResumeDiagWithoutVlan();
            }
            else
            {
                printf("\n === vlan type argument is not correct ===\n");
                LE_ERROR("vlan type argument is not correct");
                return;
            }
        }
        else
        {
            printf("\n === action type argument is not correct ===\n");
            LE_ERROR("action type argument is not correct");
            return;
        }
    }
    else
    {
        printf("Please follow the instructions and passed argument as mentioned\n");
        printf("Set argument with pause/resume vlan/nonVlan or closeDoIPConn\n");
        printf("=== eg 1: app runProc tafDiagState tafDiagState -- pause vlan \n");
        printf("=== eg 2: app runProc tafDiagState tafDiagState -- shutdown \n");
        LE_TEST_EXIT;
    }

    LE_INFO("%s [Test done]", __FUNCTION__);

}