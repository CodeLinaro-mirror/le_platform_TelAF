/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "diagPrivate.h"

//Diag IOCtrl
static taf_diagIOCtrl_ServiceRef_t svcRef[2] = {NULL};
static taf_diagIOCtrl_RxMsgHandlerRef_t diagIOCtrlMsgRef[2] = {NULL};

static le_sem_Ref_t semRef[2];

//Control state data response.
static const uint8_t data[] = {0x0C};

// Callback function for IOCtrl request message
static void IOCtrlMsgHandler
(
    taf_diagIOCtrl_RxMsgRef_t rxMsgRef,
    uint16_t dataId,
    uint8_t ioCtrlParameter,
    void* contextPtr
)
{
    le_result_t result;
    LE_TEST_INFO("IOCtrlMsgHandler!");
    LE_TEST_INFO("Received dataID: 0x%x", dataId);
    LE_TEST_INFO("Received inputOutputControlParameter: 0x%x", ioCtrlParameter);

    uint16_t vlanId = 0;
    if (taf_diagIOCtrl_GetVlanIdFromMsg(rxMsgRef, &vlanId) != LE_OK)
    {
        LE_ERROR("Failed to get vlan id");
        if (taf_diagIOCtrl_SendResp(rxMsgRef, TAF_DIAGIOCTRL_CONDITIONS_NOT_CORRECT, NULL, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    LE_TEST_INFO("Received a IO control request from vlan0x%x", vlanId);

    // Send IO control service positive response.
    size_t dataSize = 0;
    dataSize = sizeof(data);
    result = taf_diagIOCtrl_SendResp(rxMsgRef, TAF_DIAGIOCTRL_NO_ERROR, data, dataSize);
    if (result == LE_OK)
    {
        LE_TEST_INFO("IOControl response is sent");
    }
    else
    {
        LE_ERROR("Send response error");
    }

    return;
}

static void* diagIOCtrlMsgThread(void* ctxPtr)
{
    unsigned long idx = (unsigned long)(uintptr_t)ctxPtr;
    uint16_t vlanId;
    taf_diagIOCtrl_ConnectService();

    uint16_t dataId = 0x9006;
    //get diag IOCtrl svc reference
    svcRef[idx] = taf_diagIOCtrl_GetService(dataId);
    if(svcRef[idx] == NULL)
    {
        LE_ERROR("Get IO control service error");
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
    result = taf_diagIOCtrl_SetVlanId(svcRef[idx], vlanId);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to set vlan id for this service");
        return (void*)LE_FAULT;
    }

    diagIOCtrlMsgRef[idx] = taf_diagIOCtrl_AddRxMsgHandler(svcRef[idx], IOCtrlMsgHandler, NULL);
    LE_TEST_OK(diagIOCtrlMsgRef[idx] != NULL, "Registered successfully for IOCtrlMsgHandler");

    le_sem_Post(semRef[idx]);
    le_event_RunLoop();
    return NULL;
}

le_result_t diagVlanIOControl_Init(void)
{
    LE_TEST_INFO("Init");

    semRef[0] = le_sem_Create("SemRef0", 0);
    semRef[1] = le_sem_Create("SemRef1", 0);

    // Create diag IOCtrl message handle thread to handle IOCtrl request(0x2F)
    le_thread_Ref_t ioCtrlThreadRef = le_thread_Create("ioControlThread0",
            diagIOCtrlMsgThread, (void *)(uintptr_t)0);

    le_thread_Start(ioCtrlThreadRef);
    le_sem_Wait(semRef[0]);

    // Create diag IOCtrl message handle thread to handle IOCtrl request(0x2F)
    le_thread_Ref_t ioCtrlThreadRef1 = le_thread_Create("ioControlThread1",
            diagIOCtrlMsgThread, (void *)(uintptr_t)1);

    le_thread_Start(ioCtrlThreadRef1);
    le_sem_Wait(semRef[0]);

    return LE_OK;
}