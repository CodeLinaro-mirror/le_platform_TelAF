/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafMngdPmIntTest.c
 * @brief      This file includes integration test functions of Managed Connectivity Service.
 */

#include "legato.h"
#include "interfaces.h"


taf_mngdPm_wsRef_t wsRef = NULL;
static le_sem_Ref_t tafMpmAppSem;
le_clk_Time_t Timeout = { 5 , 0 };
int status = EXIT_SUCCESS;
const char* vHalTag = "vehichle_on";

static le_sem_Ref_t semRef = NULL, queueSemRef = NULL;
static le_thread_Ref_t threadRef = NULL;

#define VEHICHLE_WAKEUP_REASON_DEFAULT 0

static void PrintUsage ()
{
    puts("\n"
        "app start tafMngdPMIntTest\n"
        "--------To know Usage--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- help \n"
        "--------To Restart the System with TAF_MNGDPM_RESTART_MODE_NAD_REBOOT--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- RebootSystem \n"
        "--------To Restart the System with TAF_MNGDPM_RESTART_SYSTEM_OFF_ON--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- RestartSystem \n"
        "--------To KeepAwakeThenRestartSystem the System --------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- KeepAwakeThenRestartSystem \n"
        "--------To trigger the Forced System Shutdown--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ForcedSystemShutdown \n"
        "--------To triggger the Graceful shutdown with the wake lock acquired--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysShutdownWakeLock <NODE_ID>\n"
        "--------To trigger the Graceful shutdown without wake lock acquired--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysShutdown <NODE_ID>\n"
        "--------To triggger the Graceful suspend with the wake lock acquired--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysSuspendWakeLock <NODE_ID>\n"
        "--------To trigger the Graceful suspend without wake lock acquired--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysSuspend <NODE_ID>\n"
        "------------To set the modem wakeuptypes-----------\n"
        "--------1 -> For SMS wakeuptype------------\n"
        "--------2 -> For VOICE_CALL wakeuptype------------\n"
        "--------3 -> For SMS and VOICE_CALL wakeuptype------\n"
        "--------4 -> For MCU_VHAL wakeuptype------------\n"
        "--------5 -> For SMS and MCU_VHAL wakeuptype------\n"
        "--------6 -> For VOICE_CALL and MCU_VHAL wakeuptype------\n"
        "--------7 -> For SMS, VOICE_CALL and MCU_VHAL wakeuptype------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- SetModemWakeupSource <wakeuptype>\n"
        "------------To suspend the system with wakeuptypes-----------\n"
        "--------0 -> For APP_STAYAWAKE wakeuptype------------\n"
        "--------1 -> For SMS wakeuptype------------\n"
        "--------2 -> For VOICE_CALL wakeuptype------------\n"
        "--------3 -> For MCU_VHAL wakeuptype------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- SuspendSystem <wakeuptype> <NODE_ID> \n"
        "------------To resume the system with wakeuptypes-----------\n"
        "--------0 -> For APP_STAYAWAKE wakeuptype------------\n"
        "--------1 -> For SMS wakeuptype------------\n"
        "--------2 -> For VOICE_CALL wakeuptype------------\n"
        "--------3 -> For MCU_VHAL wakeuptype------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ResumeSystem <wakeuptype> <NODE_ID>\n"
        "\n"
        "------------To restart the particular node with node ID-----------\n"
        "--------0 -> For NAD ------------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- RestartNode <NODE_ID>\n"
        "\n"
        "------------To Shutdown the particular node with node ID-----------\n"
        "--------0 -> For NAD ------------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ShutdownNode <NODE_ID>\n"
        "------------To WakeupVehicle-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- WakeupVehicle\n"
        "------------To GetInfoReport-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GetInfoReport\n"
        "------------To AddInfoReportHandler-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- AddInfoReportHandler\n"
        "------------To ForcedSystemShutdownAndSuspend-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ForcedSystemShutdownAndSuspend\n"
        "------------To Create a new TestWakeSourceSampleApp-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- TestWakeSourceSampleApp\n"
        "------------To create a TestWakeSourceIntApp-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- TestWakeSourceIntApp\n"
        "------------To Create a CreateMutlipleClients-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- CreateMutlipleClients\n");
}

void NodePowerStateChangeHandlerCB(
     uint8_t pmNodeId,
     taf_mngdPm_nodePowerStateRef_t nodePowerStateRef,
     taf_mngdPm_NodePowerState_t state,
	 void *contextPtr)
{
    LE_INFO("NodePowerStateChangeHandlerFunc callback");
    le_result_t res = LE_FAULT;
    res = taf_mngdPm_SendNodePowerStateChangeAck(pmNodeId, nodePowerStateRef, TAF_MNGDPM_CLIENT_READY);
    if(res == LE_OK)
    {
        LE_INFO("SendNodePowerStateChangeAck is success");
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}

void AddNodePowerStateChangeHandler
(
    const char* NodePowerStateChangeBitMask,
    uint8_t pmNodeId
)
{
    LE_INFO("taf_mngdPm_AddNodePowerStateChangeHandler");
    if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE") == 0)
    {
        taf_mngdPm_NodePowerStateChangeBitMask_t stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE");
        }
    }
    else if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE") == 0)
    {
        taf_mngdPm_NodePowerStateChangeBitMask_t stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE");
        }
    }
    else if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE") == 0)
    {
        taf_mngdPm_NodePowerStateChangeBitMask_t stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE");
        }
    }
    else if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME") == 0)
    {
        taf_mngdPm_NodePowerStateChangeBitMask_t stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME");
        }
     }
}

void RestartCallback(taf_mngdPm_RestartMode_t mode, taf_mngdPm_ResponseMode_t rspmode ,
        void* contextPtr)
{
    LE_INFO("RestartCallback response mode is %d", rspmode);
    if(rspmode == 0)
    {
        LE_INFO("----RestartSystem success----");
    }
    else
    {
        LE_INFO("----RestartSystem failed----");
        exit(EXIT_FAILURE);
    }
}

static void RebootSystem()
{
    LE_INFO("----RebootSystem test----" );
    uint8_t pmNodeId = 0;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE", pmNodeId);
    le_result_t res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT,
            RestartCallback, NULL);

    if(res == LE_OK)
    {
        LE_INFO("----RebootSystem requested----");
    }
    else
    {
        LE_ERROR("RebootSystem request failed");
        exit(EXIT_FAILURE);
    }
}

static void RestartSystem()
{
    LE_INFO("----Restart System test----" );
    uint8_t pmNodeId = 0;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE", pmNodeId);
    le_result_t res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON,
            RestartCallback, NULL);

    if(res == LE_OK)
    {
        LE_INFO("----RestartSystem requested----");
    }
    else
    {
        LE_ERROR("RestartSystem request failed");
        exit(EXIT_FAILURE);
    }
}

void ForcedSystemShutdownCallBack(taf_mngdPm_ShutdownMode_t mode,
    taf_mngdPm_ResponseMode_t ResponseMode, void* contextPtr)
{
    LE_INFO("ForcedSystemShutdownCallBack response mode is %d", ResponseMode);
    if(ResponseMode == 0)
    {
        LE_INFO("----ForcedSystemShutdown success----");
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}

static void ForcedSystemShutdown()
{
    LE_INFO("----ForcedSystemShutdown test----");
    le_result_t result;
    uint8_t pmNodeId = 0;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE", pmNodeId);
    result = taf_mngdPm_ShutdownReqAsync(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
            ForcedSystemShutdownCallBack, NULL);

    if(result == LE_OK)
    {
        LE_INFO("----ForcedSystemShutdown requested----");
    }
    else
    {
        LE_ERROR("ForcedSystemShutdown request failed");
        exit(EXIT_FAILURE);
    }
}

static void ForcedSystemShutdownAndSuspend()
{
    LE_INFO("----ForcedSystemShutdown test----");
    le_result_t result;
    uint8_t pmNodeId = 0;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE", pmNodeId);
     if(wsRef == NULL)
         wsRef = taf_mngdPm_NewNodeWakeupSource(pmNodeId, TAF_MNGDPM_APP_STAYAWAKE, vHalTag);
     if(wsRef != NULL) {
         LE_INFO("NewNodeWakeupSource ref is created for APP_STAYAWAKE");
         if (wsRef != NULL) {
             result = taf_mngdPm_StayAwakeNode(wsRef);
             if(result == LE_OK) {
                 LE_INFO("Resumed sysytem with wakeuptype APP_STAYAWAKE");
             }
             else {
                 LE_INFO("Failed to acquire Wake source");
             }
         }
     }
     else {
         LE_ERROR("Failed to create wakeup source!");
     }
    result = taf_mngdPm_ShutdownReqAsync(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
            ForcedSystemShutdownCallBack, NULL);

    if(result == LE_OK)
    {
        result = taf_mngdPm_RelaxNode(wsRef);
        if(result == LE_OK) {
            LE_INFO("suspended sysytem with wakeuptype MCU_VHAL");
        }
        LE_INFO("----ForcedSystemShutdown success----");
        exit(EXIT_SUCCESS);
    }
    else
    {
        LE_ERROR("ForcedSystemShutdown request failed");
        exit(EXIT_FAILURE);
    }
}

void GracefulSysShutdownWakeLock(uint8_t pmNodeId)
{
    LE_INFO("----GracefulSysShutdownWakeLock test----");
    le_result_t result =  LE_FAULT;
        AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE", pmNodeId);
     // Create and acquire a wakelock to get notified on last wakeup source release.
     if(wsRef == NULL)
         wsRef = taf_mngdPm_NewNodeWakeupSource(pmNodeId, TAF_MNGDPM_APP_STAYAWAKE, vHalTag);
     if(wsRef != NULL) {
         LE_INFO("NewNodeWakeupSource ref is created for APP_STAYAWAKE");
         if (wsRef != NULL) {
             result = taf_mngdPm_StayAwakeNode(wsRef);
             if(result == LE_OK) {
                 LE_INFO("Acquired wake lock successfully");
             }
             result = taf_mngdPm_SetNodeTargetedPowerMode(pmNodeId,
                     TAF_MNGDPM_SHUTDOWN);
             if(result == LE_OK)
                 LE_INFO("GracefulSysShutdownWakeLock triggered successfully");
             tafMpmAppSem = le_sem_Create("tafMpmAppSem", 0);
             le_sem_WaitWithTimeOut(tafMpmAppSem, Timeout);
             LE_INFO("wake lock timer expired");
             le_sem_Delete(tafMpmAppSem);
             result = taf_mngdPm_RelaxNode(wsRef);
             if(result == LE_OK)
                 LE_INFO("Wakesource releases successfully");
         }
         else {
             LE_INFO("Failed to acquire Wake source");
         }
     }
     else {
         LE_ERROR("Failed to create wakeup source!");
     }
    if(result == LE_OK)
    {
        LE_INFO("----GracefulSysShutdownWakeLock success----");
    }
    else
    {
        LE_ERROR("GracefulSysShutdownWakeLock request failed");
        exit(EXIT_FAILURE);
    }
}

void GracefulSysSuspendWakeLock(uint8_t pmNodeId)
{
    LE_INFO("----GracefulSysSuspendWakeLock test----");
    le_result_t result =  LE_FAULT;
        AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE", pmNodeId);
     // Create and acquire a wakelock to get notified on last wakeup source release.
     if(wsRef == NULL)
         wsRef = taf_mngdPm_NewNodeWakeupSource(pmNodeId, TAF_MNGDPM_APP_STAYAWAKE, vHalTag);
     if(wsRef != NULL) {
         LE_INFO("NewNodeWakeupSource ref is created for APP_STAYAWAKE");
         if (wsRef != NULL) {
             result = taf_mngdPm_StayAwakeNode(wsRef);
             if(result == LE_OK) {
                 LE_INFO("Resumed sysytem with wakeuptype APP_STAYAWAKE");
             }
             result = taf_mngdPm_SetNodeTargetedPowerMode(pmNodeId,
                     TAF_MNGDPM_SUSPEND);
             if(result == LE_OK)
                 LE_INFO("GracefulSysSuspendWakeLock triggered successfully");

             tafMpmAppSem = le_sem_Create("tafMpmAppSem", 0);
             le_sem_WaitWithTimeOut(tafMpmAppSem, Timeout);
             LE_INFO("wake lock timer expired");
             le_sem_Delete(tafMpmAppSem);

             result = taf_mngdPm_RelaxNode(wsRef);
             if(result == LE_OK)
                 LE_INFO("suspended sysytem with wakeuptype TAF_MNGDPM_APP_STAYAWAKE");
         }
         else {
             LE_INFO("Failed to acquire Wake source");
         }
     }
     else {
         LE_ERROR("Failed to create wakeup source!");
     }

    if(result == LE_OK)
    {
        LE_INFO("----GracefulSysSuspendWakeLock success----");
    }
    else
    {
        LE_ERROR("GracefulSysSuspendWakeLock request failed");
        exit(EXIT_FAILURE);
    }
}

void GracefulSysShutdown(uint8_t pmNodeId)
{
    LE_INFO("----GracefulSysShutdown test " );
    le_result_t result =  LE_FAULT;
        AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE", pmNodeId);
    LE_INFO("GracefulSysShutdown without wake source");
    result = taf_mngdPm_SetNodeTargetedPowerMode(pmNodeId,
            TAF_MNGDPM_SHUTDOWN);

    if(result == LE_OK)
    {
        LE_INFO("----GracefulSysShutdown success----");
    }
    else
    {
        LE_ERROR("GracefulSysShutdown request failed");
        exit(EXIT_FAILURE);
    }

}

void GracefulSysSuspend(uint8_t pmNodeId)
{
    LE_INFO("----GracefulSysSuspend test " );
    le_result_t result =  LE_FAULT;
        AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE", pmNodeId);
    LE_INFO("GracefulSysSuspend without wake source");
    result = taf_mngdPm_SetNodeTargetedPowerMode(pmNodeId,
            TAF_MNGDPM_SUSPEND);
    if(result == LE_OK)
    {
        LE_INFO("----GracefulSysSuspend success----");
    }
    else
    {
        LE_ERROR("GracefulSysSuspend request failed");
        exit(EXIT_FAILURE);
    }

}

static int SetModemWakeupSource(const char* wakeupSource)
{

    // Convert string to uint32_t
    uint32_t uintResult = (uint32_t)strtoul(wakeupSource, NULL, 10);
    // Check for conversion errors
    if (uintResult > UINT32_MAX) {
        fprintf(stderr, "Value out of range.\n");
        exit(EXIT_FAILURE);
    }
    // Print the result
    LE_INFO("String: %s\nConverted to uint32_t: %u\n", wakeupSource, uintResult);

    le_result_t res = LE_FAULT;
    res = taf_mngdPm_SetModemWakeupSource(uintResult);
    if(res == LE_OK) {
        return EXIT_SUCCESS;
    }
    else
    {
        LE_ERROR("SetModemWakeupSource request failed");
        return EXIT_FAILURE;
    }
}

static void TestWakeSourceIntApp()
{
    LE_INFO("TestWakeSourceIntApp");
//test cases
    int input = 0;
    le_result_t res = LE_FAULT;
    taf_mngdPm_wsRef_t wsRef = NULL;
    char buffer[100];

    while(input >= 0)
    {
        printf("Choose the TestWakeSourceSampleApp Test Case\n 8.Exit\n 1.SetModemWakeupSource\n "
                "2.NewNodeWakeupSource\n 3.ResumeSystem\n 4.SuspendSystem\n ");
        if(fgets(buffer, sizeof(buffer), stdin))
            LE_INFO("Value read successfully");
        buffer[strcspn(buffer, "\n")] = '\0';
        input = atoi(buffer);
        LE_INFO("input: %d", input);
        if(input == 1)
        {
            printf("Enter WakeupType for SetModemWakeupSource\n 1.SMS \n 2.VOICE_CALL \n 3.SMS,VOICE_CALL \n "
                    "4.MCU_VHAL \n 5.SMS,MCU_VHAL \n 6.VOICE_CALL,MCU_VHAL \n 7.SMS,VOICE_CALL,MCU_VHAL \n");
            char wakeuptype[100];
            if(fgets(wakeuptype, sizeof(wakeuptype), stdin))
                LE_INFO("Value read successfully");
            wakeuptype[strcspn(wakeuptype, "\n")] = '\0';
            int res = SetModemWakeupSource(wakeuptype);
            if(res == LE_OK)
                printf("'SetModemWakeupSource for wakeuptype %s is set'\n", wakeuptype);
        }
        if(input == 2)
        {
            printf("Enter WakeupType for NewNodeWakeupSource\n 0.APP_STAYAWAKE\n 1.SMS \n 2.VOICE_CALL \n 3.MCU_VHAL \n");
            int wakeuptype;
            char NewNodeWakeupSource[100];
            if(fgets(NewNodeWakeupSource, sizeof(NewNodeWakeupSource), stdin))
                LE_INFO("Value read successfully");
            NewNodeWakeupSource[strcspn(NewNodeWakeupSource, "\n")] = '\0';
            wakeuptype = atoi(NewNodeWakeupSource);
            LE_INFO("NewNodeWakeupSource wakeuptype is %d", wakeuptype);
            wsRef = taf_mngdPm_NewNodeWakeupSource(0, wakeuptype, vHalTag);
            if(wsRef)
                printf("'NewNodeWakeupSource ref is created for %d'\n", wakeuptype);
        }
        if(input == 3)
        {
             LE_INFO("ResumeSystem");
             if(wsRef != NULL) {
                 res = taf_mngdPm_StayAwakeNode(wsRef);
                 if(res == LE_OK) {
                     printf("'Resumed sysytem'\n");
                  }
             }
             else
                printf("'wsRef is null, Call NewNodeWakeupSource'\n");
        }
        if(input == 4)
        {
            if(wsRef != NULL) {
                LE_INFO("SuspendSystem");
                res = taf_mngdPm_RelaxNode(wsRef);
                if(res == LE_OK) {
                    printf("'suspended system'\n");
                    wsRef = NULL;
                }
            }
             else
                printf("'wsRef is null, Call NewNodeWakeupSource'\n'");
        }
        if(input == 8)
        {
            exit(EXIT_SUCCESS);
        }
    }
    exit(EXIT_FAILURE);
}

static int RestartNode(const char* node_id)
{
    LE_INFO("RestartNode");
    le_result_t res = LE_FAULT;
    uint8_t Node = atoi(node_id);
    LE_INFO("RestartNode for %d", Node);
    res = taf_mngdPm_RestartNode(Node);
    if(res == LE_OK)
    {
        LE_INFO("restarted the node");
        return EXIT_SUCCESS;
    }
    else {
        LE_ERROR("RestartNode failed");
        return EXIT_FAILURE;
    }
}

static int ShutdownNode(const char* node_id)
{
    LE_INFO("ShutdownNode");
    le_result_t res = LE_FAULT;
    uint8_t Node = atoi(node_id);
    LE_INFO("ShutdownNode for NAD %d", Node);
    res = taf_mngdPm_ShutdownNode(Node);
    if(res == LE_OK)
    {
        LE_INFO("ShutdownNode is successfull");
        return EXIT_SUCCESS;
    }
    else {
        LE_ERROR("ShutdownNode failed");
        return EXIT_FAILURE;
    }
}

void WakeupVehicleback(int32_t reason, int32_t rspmode ,
        void* contextPtr)
{
    LE_INFO("WakeupVehicleback response is %d", rspmode);
    exit(status);
}

static int WakeupVehicle()
{
    LE_INFO("WakeupVehicle");
    le_result_t res = taf_mngdPm_WakeupVehicleReqAsync(VEHICHLE_WAKEUP_REASON_DEFAULT,
            WakeupVehicleback, NULL);

    if(res == LE_OK)
    {
        LE_INFO("----WakeupVehicle success----");
        status = EXIT_SUCCESS;
    }
    else
    {
        LE_ERROR("WakeupVehicle request failed, Ensure device is in resume state");
        status = EXIT_FAILURE;
    }
    return status;
}

static int GetInfoReport()
{
    LE_INFO("GetInfoReport");
    int32_t status;
    le_result_t res = taf_mngdPm_GetInfoReport(TAF_MNGDPM_INFO_REPORT_BUB, &status);
    if(res == LE_OK)
    {
        if(status == TAF_MNGDPM_BUB_STATUS_IN_USE)
        {
            printf("Bub Status is TAF_MNGDPM_BUB_STATUS_IN_USE");
        }
        else if(status == TAF_MNGDPM_BUB_STATUS_NOT_IN_USE)
        {
            printf("Bub Status is TAF_MNGDPM_BUB_STATUS_NOT_IN_USE");
        }
        else if(status == TAF_MNGDPM_BUB_STATUS_UNKNOWN)
        {
            printf("Bub Status is TAF_MNGDPM_BUB_STATUS_UNKNOWN");
        }
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

void BubCallBack( int32_t status, void *contextptr)
{
    LE_INFO("BubCallBack is:%d", status);
    if(status == TAF_MNGDPM_BUB_STATUS_IN_USE)
    {
        printf("Bub is TAF_MNGDPM_BUB_STATUS_IN_USE \n");
    }
    else if(status == TAF_MNGDPM_BUB_STATUS_NOT_IN_USE)
    {
        printf("Bub is TAF_MNGDPM_BUB_STATUS_NOT_IN_USE\n");
    }
    else if(status == TAF_MNGDPM_BUB_STATUS_UNKNOWN)
    {
        printf("Bub is TAF_MNGDPM_BUB_STATUS_UNKNOWN\n");
    }
    else
    {
        printf("Error status returned");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

void AddInfoReportHandler()
{
    LE_INFO("AddInfoReportHandler");
    taf_mngdPm_InfoReportHandlerRef_t handlerRef;
    handlerRef = taf_mngdPm_AddInfoReportHandler((taf_mngdPm_InfoReportBitMask_t)1, BubCallBack, NULL);
    if(handlerRef)
    {
         LE_INFO("AddInfoReportHandler is success");
    }
}

static int KeepAwakeThenRestartSystem()
{
    LE_INFO("KeepAwakeThenRestartSystem");
    le_result_t res = LE_FAULT;
    uint8_t NODE_ID = 0;
    LE_INFO("NewNodeWakeupSource wakeuptype is APP_STAYAWAKE");
    wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_APP_STAYAWAKE, vHalTag);
    if(wsRef != NULL) {
        LE_INFO("NewNodeWakeupSource ref is created for APP_STAYAWAKE");
        res = taf_mngdPm_StayAwakeNode(wsRef);
        if(res == LE_OK) {
            LE_INFO("Wake up sysytem with wakeuptype APP_STAYAWAKE");
        }
    }

    taf_mngdPm_wsRef_t wsRefSms0 = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_SMS, vHalTag);
    if (wsRefSms0 != NULL)
    {
        LE_INFO("NewNodeWakeupSource ref is created for SMS 0");
        res = taf_mngdPm_StayAwakeNode(wsRefSms0);
        if(res == LE_OK) {
            LE_INFO("Wake up sysytem with wakeuptype SMS 0");
        }
        res = taf_mngdPm_RelaxNode(wsRefSms0);
        if(res == LE_OK) {
            LE_INFO("Relax sysytem with wakeuptype SMS 0");
        }
    }
    taf_mngdPm_wsRef_t wsRefSms1 = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_SMS, vHalTag);
    if (wsRefSms1 != NULL)
    {
        LE_INFO("NewNodeWakeupSource ref is created for SMS 1");
        res = taf_mngdPm_StayAwakeNode(wsRefSms1);
        if(res == LE_OK) {
            LE_INFO("Wake up sysytem with wakeuptype SMS 1");
        }
    }

    RestartSystem();
    return EXIT_SUCCESS;
}

static void* connect_service(void* ctxPtr)
{
    LE_INFO("TestWakeSourceSampleApp");
    taf_mngdPm_ConnectService();
    int input = 0;
    le_result_t res = LE_FAULT;
    taf_mngdPm_wsRef_t wsRef = NULL;
    char buffer[100];

    while(input >= 0)
    {
        printf("Choose the TestWakeSourceSampleApp Test Case\n 8.Exit\n 1.SetModemWakeupSource\n "
                "2.NewNodeWakeupSource\n 3.ResumeSystem\n 4.SuspendSystem\n ");
        if(fgets(buffer, sizeof(buffer), stdin))
            LE_INFO("Value read successfully");
        buffer[strcspn(buffer, "\n")] = '\0';
        input = atoi(buffer);
        LE_INFO("input: %d", input);
        if(input == 1)
        {
            printf("Enter WakeupType for SetModemWakeupSource\n 1.SMS \n 2.VOICE_CALL \n 3.SMS,VOICE_CALL \n "
                    "4.MCU_VHAL \n 5.SMS,MCU_VHAL \n 6.VOICE_CALL,MCU_VHAL \n 7.SMS,VOICE_CALL,MCU_VHAL \n");
            char wakeuptype[100];
            if(fgets(wakeuptype, sizeof(wakeuptype), stdin))
                LE_INFO("Value read successfully");
            wakeuptype[strcspn(wakeuptype, "\n")] = '\0';
            int res = SetModemWakeupSource(wakeuptype);
            if(res == LE_OK)
                printf("'SetModemWakeupSource for wakeuptype %s is set'\n", wakeuptype);
        }
        if(input == 2)
        {
            printf("Enter WakeupType for NewNodeWakeupSource\n 0.APP_STAYAWAKE\n 1.SMS \n 2.VOICE_CALL \n 3.MCU_VHAL \n");
            int wakeuptype;
            char NewNodeWakeupSource[100];
            if(fgets(NewNodeWakeupSource, sizeof(NewNodeWakeupSource), stdin))
                LE_INFO("Value read successfully");
            NewNodeWakeupSource[strcspn(NewNodeWakeupSource, "\n")] = '\0';
            wakeuptype = atoi(NewNodeWakeupSource);
            LE_INFO("NewNodeWakeupSource wakeuptype is %d", wakeuptype);
            wsRef = taf_mngdPm_NewNodeWakeupSource(0, wakeuptype, vHalTag);
            if(wsRef)
                printf("'NewNodeWakeupSource ref is created for %d'\n", wakeuptype);
        }
        if(input == 3)
        {
             LE_INFO("ResumeSystem");
             if(wsRef != NULL) {
                 res = taf_mngdPm_StayAwakeNode(wsRef);
                 if(res == LE_OK) {
                     printf("'Resumed sysytem'\n");
                  }
             }
             else
                printf("'wsRef is null, Call NewNodeWakeupSource'\n");
        }
        if(input == 4)
        {
            if(wsRef != NULL) {
                LE_INFO("SuspendSystem");
                res = taf_mngdPm_RelaxNode(wsRef);
                if(res == LE_OK) {
                    printf("'suspended system'\n");
                    wsRef = NULL;
                }
            }
             else
                printf("'wsRef is null, Call NewNodeWakeupSource'\n'");
        }
        if(input == 8)
        {
            exit(EXIT_SUCCESS);
        }
    }
    le_sem_Post(semRef);
    exit(EXIT_FAILURE);
    le_event_RunLoop();
}

void* ThreadFunction(void* threadID) {

    taf_mngdPm_ConnectService();
    // You can add any additional processing here
    le_result_t res = taf_mngdPm_SetModemWakeupSource(1);
    if(res == LE_OK)
        printf("SetModemWakeupSource for wakeuptype SMS is set\n");
    wsRef = taf_mngdPm_NewNodeWakeupSource(0, 1, vHalTag);
    if(wsRef)
        printf("NewNodeWakeupSource ref is created for\n");
    printf("ResumeSystem");
    if(wsRef != NULL) {
        res = taf_mngdPm_StayAwakeNode(wsRef);
        if(res == LE_OK) {
            printf("Resumed sysytem\n");
         }
    }
    le_sem_Post(semRef);
    le_event_RunLoop();
}

void CreateMutlipleClients()
{
    long t;
    semRef = le_sem_Create("MngdIntTestApp", 0);
    int NUM_THREADS = 0;
    char buffer[100];
    while(NUM_THREADS >= 0)
    {
    printf("Enter the number of clients\nEnter'-1' to exit\n");
    if(fgets(buffer, sizeof(buffer), stdin))
        LE_INFO("Value read successfully");
    buffer[strcspn(buffer, "\n")] = '\0';
    NUM_THREADS = atoi(buffer);
    for (t = 0; t < NUM_THREADS; t++) {
        threadRef = le_thread_Create("inttestapp",
                                    ThreadFunction, NULL);
        if (threadRef) {
            fprintf(stderr, "create thread :%ld \n", t);
        }
        le_thread_Start(threadRef);
        le_sem_Wait(semRef);
    }
    printf("All threads completed successfully.\n");
    if(NUM_THREADS == -1)
        exit(EXIT_SUCCESS);
    }
}

void CreatSampleApp()
{
    semRef = le_sem_Create("MngdIntTestApp", 0);
    queueSemRef = le_sem_Create("MngdPMIntQueueSem", 0);
    LE_INFO("createapp1 start");
        threadRef = le_thread_Create("inttestapp",
                                    connect_service, NULL);
        le_thread_Start(threadRef);
        le_sem_Wait(semRef);
}

COMPONENT_INIT
{
    const char* testType = "";
    const char* testPar = "";
    const char* testPar1 = "";
    if (le_arg_NumArgs() == 0 )
    {
        PrintUsage();
    }
    if (le_arg_NumArgs() >= 1)
    {
        testType = le_arg_GetArg(0);
        LE_INFO("arg0=%s ", testType);
        if (NULL == testType) {
            LE_ERROR("testType is NULL");
            exit(EXIT_FAILURE);
        }
        testPar = le_arg_GetArg(1);
        LE_INFO("arg1=%s ",testPar);
        if (NULL == testPar) {
            LE_ERROR("testPar is 0");
        }
        testPar1 = le_arg_GetArg(2);
        LE_INFO("arg1=%s ",testPar1);
        if (NULL == testPar1) {
            LE_ERROR("testPar1 is 0");
        }
        if (testType!= NULL && strncmp(testType,"help", 4) == 0)
        {
            PrintUsage();
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(testType, "RestartSystem") == 0)
        {
            RestartSystem();
        }
        else if(strcmp(testType, "RebootSystem") == 0)
        {
            RebootSystem();
        }
        else if(strcmp(testType, "KeepAwakeThenRestartSystem") == 0)
        {
            status = SetModemWakeupSource("1"); // whitelist SMS wakeup type
            status = KeepAwakeThenRestartSystem();
            exit(status);
        }
        else if(strcmp(testType, "ForcedSystemShutdown") == 0)
        {
            ForcedSystemShutdown();
        }
        else if(strcmp(testType, "GracefulSysShutdownWakeLock") == 0)
        {
            if(testPar)
                GracefulSysShutdownWakeLock(atoi(testPar));
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "GracefulSysShutdown") == 0)
        {
            if(testPar)
                GracefulSysShutdown(atoi(testPar));
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "GracefulSysSuspendWakeLock") == 0)
        {
            if(testPar)
                GracefulSysSuspendWakeLock(atoi(testPar));
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "GracefulSysSuspend") == 0)
        {
            if(testPar)
                GracefulSysSuspend(atoi(testPar));
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "SetModemWakeupSource") == 0)
        {
            if(testPar) {
                status = SetModemWakeupSource(testPar);
                exit(status);
            }
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "RestartNode") == 0)
        {
            if(testPar) {
                status = RestartNode(testPar);
                exit(status);
            }
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "ShutdownNode") == 0)
        {
            if(testPar) {
                status = ShutdownNode(testPar);
                exit(status);
            }
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "WakeupVehicle") == 0)
        {
            status = WakeupVehicle();
            exit(status);
        }
        else if(strcmp(testType, "GetInfoReport") == 0)
        {
            status = GetInfoReport();
            exit(status);
        }
        else if(strcmp(testType, "AddInfoReportHandler") == 0)
        {
            AddInfoReportHandler();
        }
        else if(strcmp(testType, "ForcedSystemShutdownAndSuspend") == 0)
        {
            ForcedSystemShutdownAndSuspend();
        }
        else if(strcmp(testType, "TestWakeSourceSampleApp") == 0)
        {
            CreatSampleApp();
        }
        else if(strcmp(testType, "TestWakeSourceIntApp") == 0)
        {
            TestWakeSourceIntApp();
        }
        else if(strcmp(testType, "CreateMutlipleClients") == 0)
        {
            CreateMutlipleClients();
        }
        else
        {
            LE_ERROR("Error command");
            exit(EXIT_FAILURE);
        }
    }
}
