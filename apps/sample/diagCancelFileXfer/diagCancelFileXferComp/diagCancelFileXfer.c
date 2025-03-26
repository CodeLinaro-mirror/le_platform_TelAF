/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define VLAN_ID_10 10
#define VLAN_ID_110 110
static void CancelFileXferHandlerFunc
(
    taf_diag_ServiceRef_t svcRef,
    uint16_t vlanId,
    le_result_t result,
    void* contextPtr
)
{
    LE_INFO("CancelFileXferHandlerFunc with vlanId:%d, result: %d", vlanId, result);
}

void TestCancelFileXferWithoutVlan()
{
    // Get diag svc reference
    taf_diag_ServiceRef_t svcRef = taf_diag_GetService();
    if(svcRef == NULL)
    {
        LE_ERROR("Get diag service reference error");
        return;
    }

     taf_diag_CancelFileXferAsync(svcRef, CancelFileXferHandlerFunc, NULL);
}

void TestCancelFileXferWithVlan()
{
    le_result_t result;
    // Get diag svc reference
    taf_diag_ServiceRef_t svcRef = taf_diag_GetService();
    if(svcRef == NULL)
    {
        LE_ERROR("Get diag service reference error");
        return;
    }

    result = taf_diag_SetVlanId(svcRef, VLAN_ID_10);
    if(result != LE_OK)
    {
        LE_ERROR("Set VLAN Id error");
        return;
    }

    //Only VLAN Id 10 in the list, target VLAN Id is 10
    LE_INFO("Cancel FileXfer for VLAN Id 10");
    taf_diag_CancelFileXferAsync(svcRef, CancelFileXferHandlerFunc, NULL);

    result = taf_diag_SetVlanId(svcRef, VLAN_ID_110);
    if(result != LE_OK)
    {
        LE_ERROR("Set VLAN Id error");
        return;
    }

    //VLAN Id 10 and VLANId 110 are in the list, target VLAN Id is 110 since last set VLAN Id is 110
    LE_INFO("Cancel FileXfer for VLAN Id 110");
    taf_diag_CancelFileXferAsync(svcRef, CancelFileXferHandlerFunc, NULL);

    result = taf_diag_SelectTargetVlanID(svcRef, VLAN_ID_10);
    if(result != LE_OK)
    {
        LE_ERROR("Select target VLAN Id 10 error");
        return;
    }

    //VLAN Id 10 and VLANId 110 are in the list, target VLAN Id is set to VLAN Id 10
    LE_INFO("Cancel FileXfer for VLAN Id 10");
    taf_diag_CancelFileXferAsync(svcRef, CancelFileXferHandlerFunc, NULL);
}


COMPONENT_INIT
{
    LE_INFO("%s [Test start]", __FUNCTION__);

    int numberOfArgs = le_arg_NumArgs();
    LE_INFO("Total numberOfArgs count: %d", numberOfArgs);

    if (numberOfArgs == 1)
    {
        const char* vlanTypePtr = le_arg_GetArg(0);

        if (vlanTypePtr == NULL)
        {
            LE_ERROR("vlanTypePtr is NULL");
            return;
        }

        if (strcmp(vlanTypePtr,"vlan") == 0)
        {
            LE_INFO("vlan");

            TestCancelFileXferWithVlan();
        }
        else if (strcmp(vlanTypePtr,"nonVlan") == 0)
        {
            LE_INFO("without vlan");
            TestCancelFileXferWithoutVlan();
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
        printf("Please follow the instructions and passed argument as mentioned\n");
        printf("Set argument with vlan/nonVlan\n");
        printf("=== eg: app runProc tafDiagCancelFileXfer --exe=tafDiagCancelFileXfer -- vlan \n");
        LE_TEST_EXIT;
    }

    LE_INFO("%s [Test done]", __FUNCTION__);

}