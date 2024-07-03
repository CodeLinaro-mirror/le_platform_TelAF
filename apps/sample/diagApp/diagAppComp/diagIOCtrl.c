/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "diagPrivate.h"

//Diag IOCtrl
static taf_diagIOCtrl_ServiceRef_t svcRef = NULL;
static taf_diagIOCtrl_RxMsgHandlerRef_t diagIOCtrlMsgRef = NULL;

static le_sem_Ref_t semRef;

//Control state data response.
static const uint8_t data[] = {0x0C};

// Callback function for IOCtrl request message
void IOCtrlMsgHandler
(
    taf_diagIOCtrl_RxMsgRef_t rxMsgRef,
    uint16_t dataId,
    uint8_t ioCtrlParameter,
    void* contextPtr
)
{
    LE_TEST_INFO("IOCtrlMsgHandler!");
    LE_TEST_INFO("Received dataID: 0x%x", dataId);
    LE_TEST_INFO("Received inputOutputControlParameter: 0x%x", ioCtrlParameter);

    le_result_t result;

    // Get request inputOutputControlParameter.
    LE_TEST_INFO("inputOutputControlParameter: %x", ioCtrlParameter);

    // Get request controlStatte.
    uint8_t reqCtrlState[TAF_DIAGIOCTRL_MAX_CONTROL_RECORD_SIZE];
    size_t reqCtrlStateSize = 0;
    result = taf_diagIOCtrl_GetCtrlState(rxMsgRef, reqCtrlState, &reqCtrlStateSize);
    if (result != LE_OK)
    {
        LE_ERROR("Getting request control state error!");
        // Send NRC response;
        if (taf_diagIOCtrl_SendResp(rxMsgRef, TAF_DIAGIOCTRL_CONDITIONS_NOT_CORRECT, NULL, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }
    else
    {
        LE_TEST_INFO("Received controlState size: %"PRIuS"", reqCtrlStateSize);
        for (size_t i = 0; i<reqCtrlStateSize; i++)
        {
            LE_TEST_INFO("Received controlState: %x", reqCtrlState[i]);
        }
    }

    // Get request control enable mask record.
    uint8_t reqMaskRec[TAF_DIAGIOCTRL_MAX_CONTROL_RECORD_SIZE];
    size_t reqMaskRecSize = 0;
    result = taf_diagIOCtrl_GetCtrlEnableMaskRecd(rxMsgRef, reqMaskRec, &reqMaskRecSize);
    if (result != LE_OK)
    {
        LE_ERROR("Getting request control enable mask record error!");
        // Send NRC response;
        if (taf_diagIOCtrl_SendResp(rxMsgRef, TAF_DIAGIOCTRL_CONDITIONS_NOT_CORRECT, NULL, 0)
                != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }
    else
    {
        LE_TEST_INFO("Recevied controlEnableMaskRecord size: %"PRIuS"", reqMaskRecSize);
        for (size_t i = 0; i<reqMaskRecSize; i++)
        {
            LE_TEST_INFO("Received controlEnableMaskRecord : %x", reqMaskRec[i]);
        }
    }

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
    taf_diagIOCtrl_ConnectService();

    diagIOCtrlMsgRef = taf_diagIOCtrl_AddRxMsgHandler(svcRef, IOCtrlMsgHandler, NULL);
    LE_TEST_OK(diagIOCtrlMsgRef != NULL, "Registered successfully for IOCtrlMsgHandler");

    le_sem_Post(semRef);
    le_event_RunLoop();
    return NULL;
}

le_result_t diagIOControl_Init(void)
{
    LE_TEST_INFO("Init");

    semRef = le_sem_Create("SemRef", 0);

    uint16_t dataId = 0x9006;
    //get diag IOCtrl svc reference
    svcRef = taf_diagIOCtrl_GetService(dataId);
    if(svcRef == NULL)
    {
        LE_ERROR("Get IO control service error");
        return LE_FAULT;
    }

    // Create diag IOCtrl message handle thread to handle IOCtrl request(0x2F)
    le_thread_Ref_t ioCtrlThreadRef = le_thread_Create("ioControlThread",
            diagIOCtrlMsgThread, NULL);

    le_thread_Start(ioCtrlThreadRef);
    le_sem_Wait(semRef);

    return LE_OK;
}