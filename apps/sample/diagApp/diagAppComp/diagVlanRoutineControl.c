/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "diagPrivate.h"

#define UPDATE_PRE_DOWNLOAD_CHECK_IDENTIFIER 0x0246
#define UPDATE_POST_DOWNLOAD_CHECK_IDENTIFIER 0x0247

static taf_diagRoutineCtrl_ServiceRef_t diagRCPreDlSvcRef[2] = {NULL};
static taf_diagRoutineCtrl_ServiceRef_t diagRCPostDlSvcRef[2] = {NULL};
static taf_diagRoutineCtrl_RxMsgHandlerRef_t diagRoutineCtrlMsgRef[2] = {NULL};

static le_sem_Ref_t semRef[2];

// Callback function for routine control request message
static void routineCtrl_0246_MsgHandler
(
    taf_diagRoutineCtrl_RxMsgRef_t rxMsgRef,
    taf_diagRoutineCtrl_Type_t routineCtrlType,
    uint16_t identifier,
    void* contextPtr
)
{
    uint16_t vlanId = 0;
    if (taf_diagRoutineCtrl_GetVlanIdFromMsg(rxMsgRef, &vlanId) != LE_OK)
    {
        LE_ERROR("Failed to get vlan id");
        if(taf_diagRoutineCtrl_SendResp(rxMsgRef,
            TAF_DIAGROUTINECTRL_GENERAL_PROGRAMMING_FAILURE, NULL, 0)
            != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    LE_TEST_INFO("Received a routine control request from vlan0x%x", vlanId);

    if(taf_diagRoutineCtrl_SendResp( rxMsgRef, TAF_DIAGROUTINECTRL_NO_ERROR,
            NULL, 0 ) != LE_OK)
    {
        LE_ERROR("Send response error");
    }
}

// Callback function for routine control request message
// Identifier 0x0247 is used to install the firmware(start routine) and get the result( request
// routine results) in this sample.
static void routineCtrl_0247_MsgHandler
(
    taf_diagRoutineCtrl_RxMsgRef_t rxMsgRef,
    taf_diagRoutineCtrl_Type_t routineCtrlType,
    uint16_t identifier,
    void* contextPtr
)
{
    uint16_t vlanId = 0;
    if (taf_diagRoutineCtrl_GetVlanIdFromMsg(rxMsgRef, &vlanId) != LE_OK)
    {
        LE_ERROR("Failed to get vlan id");
        if(taf_diagRoutineCtrl_SendResp(rxMsgRef,
            TAF_DIAGROUTINECTRL_GENERAL_PROGRAMMING_FAILURE, NULL, 0)
            != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    LE_TEST_INFO("Received a routine control request from vlan0x%x", vlanId);

    if(taf_diagRoutineCtrl_SendResp( rxMsgRef, TAF_DIAGROUTINECTRL_NO_ERROR,
            NULL, 0 ) != LE_OK)
    {
        LE_ERROR("Send response error");
    }
}

static void* diagRoutingCtrlMsgThread(void* ctxPtr)
{
    le_result_t result;
    unsigned long idx = (unsigned long)(uintptr_t)ctxPtr;
    uint16_t vlanId;
    taf_diagRoutineCtrl_ConnectService();

    //get diag routinectrl svc reference for pre-download check
    diagRCPreDlSvcRef[idx] = taf_diagRoutineCtrl_GetService(UPDATE_PRE_DOWNLOAD_CHECK_IDENTIFIER);
    if(diagRCPreDlSvcRef[idx] == NULL)
    {
        LE_ERROR("Get diagRoutineCtrl service for pre-download");
        return (void*)LE_FAULT;
    }

    //get diag routinectrl svc reference for post-download check
    diagRCPostDlSvcRef[idx] = taf_diagRoutineCtrl_GetService(UPDATE_POST_DOWNLOAD_CHECK_IDENTIFIER);
    if(diagRCPostDlSvcRef[idx] == NULL)
    {
        LE_ERROR("Get diagRoutineCtrl service for post-download");
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

    result = taf_diagRoutineCtrl_SetVlanId(diagRCPreDlSvcRef[idx], vlanId);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to set vlan id for this service");
        return (void*)LE_FAULT;
    }

    result = taf_diagRoutineCtrl_SetVlanId(diagRCPostDlSvcRef[idx], vlanId);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to set vlan id for this service");
        return (void*)LE_FAULT;
    }

    diagRoutineCtrlMsgRef[idx] = taf_diagRoutineCtrl_AddRxMsgHandler( diagRCPreDlSvcRef[idx],
            routineCtrl_0246_MsgHandler, NULL);
    LE_TEST_OK(diagRoutineCtrlMsgRef[idx] != NULL,
              "Registered successfully for routineCtrl_0246_MsgHandler");

    diagRoutineCtrlMsgRef[idx] = taf_diagRoutineCtrl_AddRxMsgHandler( diagRCPostDlSvcRef[idx],
            routineCtrl_0247_MsgHandler, NULL);
    LE_TEST_OK(diagRoutineCtrlMsgRef[idx] != NULL,
            "Registered successfully for routineCtrl_0247_MsgHandler");

    le_sem_Post(semRef[idx]);
    le_event_RunLoop();
    return NULL;
}

le_result_t diagVlanRoutineControl_Init(void)
{
    semRef[0] = le_sem_Create("SemRef0", 0);
    semRef[1] = le_sem_Create("SemRef1", 0);

    // Create diag routine control message handle thread to handle routine control request(0x31)
    le_thread_Ref_t routineCtrlThreadRef = le_thread_Create("routineCtrlTd0",
            diagRoutingCtrlMsgThread, (void *)(uintptr_t)0);

    le_thread_Start(routineCtrlThreadRef);
    le_sem_Wait(semRef[0]);

    // Create diag routine control message handle thread to handle routine control request(0x31)
    le_thread_Ref_t routineCtrlThreadRef1 = le_thread_Create("routineCtrlTd1",
            diagRoutingCtrlMsgThread, (void *)(uintptr_t)1);

    le_thread_Start(routineCtrlThreadRef1);
    le_sem_Wait(semRef[1]);

    return LE_OK;
}
