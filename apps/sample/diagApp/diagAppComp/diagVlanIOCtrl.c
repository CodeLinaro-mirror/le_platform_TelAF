/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "diagPrivate.h"

// Supported DIDs
#define DIAG_IOCTRL_DID_CNT 2
static const uint16_t diagIOCtrlDids[DIAG_IOCTRL_DID_CNT] = {0x9006, 0x9007};

// Diag IOCtrl: first dimension = DID index, second dimension = VLAN index
#define DIAG_IOCTRL_VLAN_CNT 2
static taf_diagIOCtrl_ServiceRef_t    svcRef[DIAG_IOCTRL_DID_CNT][DIAG_IOCTRL_VLAN_CNT];
static taf_diagIOCtrl_RxMsgHandlerRef_t diagIOCtrlMsgRef[DIAG_IOCTRL_DID_CNT][DIAG_IOCTRL_VLAN_CNT];

// Semaphores: one per thread (DID_COUNT * VLAN_COUNT threads)
static le_sem_Ref_t semRef[DIAG_IOCTRL_DID_CNT * DIAG_IOCTRL_VLAN_CNT];

// Thread context passed to each worker thread
typedef struct
{
    unsigned int didIdx;  ///< Index into diagIOCtrlDids[]
    unsigned int vlanIdx; ///< 0 = TEST_VLAN_ID_0, 1 = TEST_VLAN_ID_1
} IOCtrlThreadCtx_t;

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
    size_t dataSize = sizeof(data);
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
    IOCtrlThreadCtx_t* ctx = (IOCtrlThreadCtx_t*)ctxPtr;
    unsigned int didIdx  = ctx->didIdx;
    unsigned int vlanIdx = ctx->vlanIdx;
    unsigned int semIdx  = didIdx * DIAG_IOCTRL_VLAN_CNT + vlanIdx;
    uint16_t dataId      = diagIOCtrlDids[didIdx];
    uint16_t vlanId      = (vlanIdx == 0) ? TEST_VLAN_ID_0 : TEST_VLAN_ID_1;

    taf_diagIOCtrl_ConnectService();

    // Get diag IOCtrl svc reference for this DID
    svcRef[didIdx][vlanIdx] = taf_diagIOCtrl_GetService(dataId);
    if (svcRef[didIdx][vlanIdx] == NULL)
    {
        LE_ERROR("Get IO control service error for DID 0x%x", dataId);
        return (void*)LE_FAULT;
    }

    le_result_t result = taf_diagIOCtrl_SetVlanId(svcRef[didIdx][vlanIdx], vlanId);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to set vlan id for DID 0x%x, vlan 0x%x", dataId, vlanId);
        return (void*)LE_FAULT;
    }

    diagIOCtrlMsgRef[didIdx][vlanIdx] = taf_diagIOCtrl_AddRxMsgHandler(
            svcRef[didIdx][vlanIdx], IOCtrlMsgHandler, NULL);
    LE_TEST_OK(diagIOCtrlMsgRef[didIdx][vlanIdx] != NULL,
            "Registered successfully for IOCtrlMsgHandler DID 0x%x vlan 0x%x",
            dataId, vlanId);

    le_sem_Post(semRef[semIdx]);
    le_event_RunLoop();
    return NULL;
}

le_result_t diagVlanIOControl_Init(void)
{
    LE_TEST_INFO("Init");

    // Create semaphores and threads for each DID + VLAN combination
    for (unsigned int didIdx = 0; didIdx < DIAG_IOCTRL_DID_CNT; didIdx++)
    {
        for (unsigned int vlanIdx = 0; vlanIdx < DIAG_IOCTRL_VLAN_CNT; vlanIdx++)
        {
            unsigned int semIdx = didIdx * DIAG_IOCTRL_VLAN_CNT + vlanIdx;
            char semName[32];
            char threadName[32];

            snprintf(semName,    sizeof(semName),    "SemRef%u",        semIdx);
            snprintf(threadName, sizeof(threadName), "ioControlThread%u", semIdx);

            semRef[semIdx] = le_sem_Create(semName, 0);

            IOCtrlThreadCtx_t* ctx = malloc(sizeof(IOCtrlThreadCtx_t));
            LE_ASSERT(ctx != NULL);
            ctx->didIdx  = didIdx;
            ctx->vlanIdx = vlanIdx;

            le_thread_Ref_t threadRef = le_thread_Create(threadName,
                    diagIOCtrlMsgThread, ctx);
            le_thread_Start(threadRef);
            le_sem_Wait(semRef[semIdx]);
        }
    }

    return LE_OK;
}