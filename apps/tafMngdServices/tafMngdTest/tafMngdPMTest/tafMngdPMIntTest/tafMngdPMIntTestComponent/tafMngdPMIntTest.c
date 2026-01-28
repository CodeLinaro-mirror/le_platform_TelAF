/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafMngdPmIntTest.c
 * @brief      This file includes integration test functions of Managed Connectivity Service.
 */

#include "legato.h"
#include "interfaces.h"


taf_mngdPm_wsRef_t wsRef = NULL;
taf_mngdPm_wsRef_t wsRef1 = NULL;
taf_mngdPm_wsRef_t wsRef2 = NULL;
taf_mngdPm_NodePowerStateChangeBitMask_t stateMask = 0;
static le_sem_Ref_t tafMpmAppSem;
le_clk_Time_t Timeout = { 5 , 0 };
le_clk_Time_t AckTimeout = { 1 , 0 };

static le_sem_Ref_t tafMpmEcallSem;
le_clk_Time_t EcallTimeout = { 10 , 0 };
int status = EXIT_SUCCESS;
const char* wsTag = "testWsTag";

static le_sem_Ref_t semRef = NULL, queueSemRef = NULL;
static le_thread_Ref_t threadRef = NULL;
static le_sem_Ref_t semRef1 = NULL;
static le_thread_Ref_t threadRef1 = NULL;
static le_sem_Ref_t semRef2 = NULL;
static le_thread_Ref_t threadRef2 = NULL;
int stateChangeAck = 1;
#define VEHICHLE_WAKEUP_REASON_DEFAULT 0
#define AUTHORIZE_ALL_STAY_AWAKE_REASON 0xFFFFFFFF

static void PrintUsage ()
{
    puts("\n"
        "app start tafMngdPMIntTest\n"
        "--------To know Usage--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- help \n"
        "--------To Reboot the PVM System with given reasons--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- RebootSystemWithReason \n"
        "--------To Restart the PVM System with given reasons--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- RestartSystemWithReason \n"
        "--------To trigger the Forceful PVM System Shutdown with given reasons--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ForcedSystemShutdownWithReason \n"
        "--------To trigger the Graceful shutdown of particular node with NODE_ID with the wake lock acquired from this app--------\n"
        "--------0 -> For PVM NAD ------------\n"
        "--------1 -> For RPC NAD ------------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysShutdownWakeLock <NODE_ID>\n"
        "--------To trigger the Graceful shutdown of particular node with NODE_ID--------\n"
        "--------0 -> For PVM NAD ------------\n"
        "--------1 -> For RPC NAD ------------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysShutdown <NODE_ID>\n"
        "--------To trigger the Graceful suspend with the wake lock acquired from this app--------\n"
        "--------0 -> For PVM NAD ------------\n"
        "--------1 -> For RPC NAD ------------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysSuspendWakeLock <NODE_ID>\n"
        "--------To trigger the Graceful suspend without wake lock acquired--------\n"
        "--------0 -> For PVM NAD ------------\n"
        "--------1 -> For RPC NAD ------------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysSuspend <NODE_ID>\n"
        "------------To set the modem wakeuptypes-----------\n"
        "--------1 -> For SMS wakeuptype------------\n"
        "--------2 -> For VOICE_CALL wakeuptype------------\n"
        "--------3 -> For SMS and VOICE_CALL wakeuptype------\n"
        "--------4 -> For MCU_VHAL wakeuptype------------\n"
        "--------5 -> For SMS and MCU_VHAL wakeuptype------\n"
        "--------6 -> For VOICE_CALL and MCU_VHAL wakeuptype------\n"
        "--------7 -> For SMS, VOICE_CALL and MCU_VHAL wakeuptype------\n"
        "\n"
        "------------To Restart the particular node with node ID-----------\n"
        "--------0 -> For PVM NAD ------------\n"
        "--------1 -> For RPC NAD ------------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- RestartNode <NODE_ID>\n"
        "\n"
        "------------Forceful Shutdown of particular node with node ID-----------\n"
        "--------0 -> For PVM NAD ------------\n"
        "--------1 -> For RPC NAD ------------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ShutdownNode <NODE_ID>\n"
        "------------To test WakeupVehicle of VHAL MCU-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- WakeupVehicle\n"
        "------------To test GetInfoReport of BUB status-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GetInfoReport\n"
        "------------To test AddInfoReportHandler of BUB status-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- AddInfoReportHandler\n"
        "\n"
        "-------Fixed Issues Test Cases--------\n"
        "--------To KeepAwakeThenRestartSystem the System --------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- KeepAwakeThenRestartSystem \n"
        "------------To ForcedSystemShutdownAndSuspend-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ForcedSystemShutdownAndSuspend\n"
        "------------To Create a CreateMultipleClients-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- CreateMultipleClients\n"
        "------------To  test AllowWakingupDuringSuspending-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- AllowWakingupDuringSuspending\n"
        "------------To  test AllowAuthorizedWakingupDuringSuspending-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- AllowAuthorizedWakingupDuringSuspending\n"
        "------------To  test ShouldRejectUnauthorizedWakingupDuringSuspending-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ShouldRejectUnauthorizedWakingupDuringSuspending\n"
        "------------To Test System Resume and Suspend -----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- TestAuthorizedResumeandSuspend\n"
        "------------To Test Node Resume and Suspend -----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- TestNodeResumeandSuspend\n"
        "------------To Test Bub with ecall use cases-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- TestBubCases\n"
        "------------To Test Test NonAuthorized StayAwake wake source-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- TestNonAuthorizedStayAwake\n"
        "------------To Test clearing of unauthorized wake source after calling AuthorizeStayAwakeReason-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- TestClearUnAuthorizedWakeSource\n"
        "------------To Test ForcedSysShutdown with multiple clients-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- MultiClntForcedSysShutdown\n"
        "------------To Test System Restart with multiple clients-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- MultiClntRestartSystem\n"
        "------------To Test Wakeup Vehicle with multiple clients-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- MultiClntWakeupVehicle\n"
        "------------To set NodePowerStateChangeAck for state change acknowledgement-----------\n"
        "------  1   -> ACK ------------\n"
        "-----  -1   -> NACK ------------\n"
        "------  2   -> NO_RESP ------------\n"
        "------  3   -> ACK_AFTER_TIMEOUT ------------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysSuspendWithAckType <ACK_TYPE>\n"
        "------------To Test Refresh Authorized Wake Source Cases-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- TestRefreshAuthorizedWsCases\n"
        "------------To Test stayawake request during shutdown-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ForcedSystemShutdownAndResume\n"
        "------------To test waking up vehicle when releasing WS-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- WakeupVehicleWhenReleasingWsTest\n"
        "------------To test vehicle wakeup when system is waking up-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- WakeupVehicleWhenWakingUpTest\n"
        "------------To Test delete wakeup source if not acquired-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- DeleteWsIfNotAcquired\n"
        "------------To Test rejecting deletion of wakeup source if acquired-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ShouldNotDeleteWsIfAcquired\n"
        "------------To Test rejecting deletion of wakeup source if ignored-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ShouldNotDeleteWsIfIgnored\n"
        "------------To Test PMVHAL notification on client disconnection-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- NotifyVhalOnClientDisconnectionForReleaseWS\n"
        "------------To Test PMVHAL stayawake after while suspending through MPMS-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- TestPmvhalStayAwakeAfterMpmsSuspendTrigger\n");
}

void NodePowerStateChangeHandlerCB(
     uint8_t pmNodeId,
     taf_mngdPm_nodePowerStateRef_t nodePowerStateRef,
     taf_mngdPm_NodePowerState_t state,
	 void *contextPtr)
{
    LE_INFO("NodePowerStateChangeHandlerFunc callback");
    le_result_t res = LE_FAULT;
    if(stateChangeAck == 2)
        return;
    else if(stateChangeAck == 3)
    {
        tafMpmAppSem = le_sem_Create("tafMpmAppSem", 0);
        le_sem_WaitWithTimeOut(tafMpmAppSem, AckTimeout);
        LE_INFO("state change ack timer expired");
        le_sem_Delete(tafMpmAppSem);
        res = taf_mngdPm_SendNodePowerStateChangeAck(pmNodeId, nodePowerStateRef, stateChangeAck);
        if(res == LE_OK)
        {
            LE_INFO("SendNodePowerStateChangeAck is success");
            exit(EXIT_SUCCESS);
        }
    }
    res = taf_mngdPm_SendNodePowerStateChangeAck(pmNodeId, nodePowerStateRef, stateChangeAck);
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
        stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE");
        }
    }
    else if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE") == 0)
    {
        stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE");
        }
    }
    else if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE") == 0)
    {
        stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE");
        }
    }
    else if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME") == 0)
    {
        stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME");
        }
     }
}

void RestartCallback(taf_mngdPm_RestartMode_t mode, taf_mngdPm_ResponseMode_t rspmode ,
        le_result_t result, void* contextPtr)
{
    LE_INFO("RestartCallback response mode is %d and result %d", rspmode, result);
    if(rspmode == 0)
    {
        LE_INFO("----Restart System success----");
    }
    else
    {
        LE_INFO("----RestartSystem failed----");
        exit(EXIT_FAILURE);
    }
}

static void RebootSystemWithReason()
{
    LE_INFO("----RebootSystem test----" );
    int input;
    le_result_t res = LE_FAULT;
    char buffer[100];
    uint8_t pmNodeId = 0;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE", pmNodeId);

    printf("Choose the Reboot reason\n -1.Exit\n 0.TAF_MNGDPM_RESTART_REASON_NORMAL\n "
            "1.TAF_MNGDPM_RESTART_REASON_SW_UPDATE\n 2.TAF_MNGDPM_RESTART_REASON_ECALL_RECOVERY\n 16.TAF_MNGDPM_RESTART_REASON_VENDOR_1\n ");
    if(fgets(buffer, sizeof(buffer), stdin))
        LE_INFO("Value read successfully");
    buffer[strcspn(buffer, "\n")] = '\0';
    input = atoi(buffer);
    LE_INFO("input: %d", input);
    if(input == 0)
    {
        res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT,
                RestartCallback, NULL, TAF_MNGDPM_RESTART_REASON_NORMAL);
    }
    if(input == 1)
    {
        res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT,
                RestartCallback, NULL, TAF_MNGDPM_RESTART_REASON_SW_UPDATE);
    }
    if(input == 2)
    {
        res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT,
                RestartCallback, NULL, TAF_MNGDPM_RESTART_REASON_ECALL_RECOVERY);
    }
    if(input == 16)
    {
        res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT,
                RestartCallback, NULL, TAF_MNGDPM_RESTART_REASON_VENDOR_1);
    }
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
    LE_TEST_INFO("To test RestartSystem!" );
    uint8_t pmNodeId = 0;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE", pmNodeId);
    le_result_t res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON,
            RestartCallback, NULL, TAF_MNGDPM_RESTART_REASON_NORMAL);

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

static void RestartSystemWithReason()
{
    LE_INFO("----Restart System test----" );
    int input;
    le_result_t res = LE_FAULT;
    char buffer[100];
    uint8_t pmNodeId = 0;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE", pmNodeId);

    printf("Choose the Restart reason\n -1.Exit\n 0.TAF_MNGDPM_RESTART_REASON_NORMAL\n "
            "1.TAF_MNGDPM_RESTART_REASON_SW_UPDATE\n 2.TAF_MNGDPM_RESTART_REASON_ECALL_RECOVERY\n 16.TAF_MNGDPM_RESTART_REASON_VENDOR_1\n ");
    if(fgets(buffer, sizeof(buffer), stdin))
        LE_INFO("Value read successfully");
    buffer[strcspn(buffer, "\n")] = '\0';
    input = atoi(buffer);
    LE_INFO("input: %d", input);
    if(input == 0)
    {
        res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON,
                RestartCallback, NULL, TAF_MNGDPM_RESTART_REASON_NORMAL);
    }
    if(input == 1)
    {
        res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON,
                RestartCallback, NULL, TAF_MNGDPM_RESTART_REASON_SW_UPDATE);
    }
    if(input == 2)
    {
        res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON,
                RestartCallback, NULL, TAF_MNGDPM_RESTART_REASON_ECALL_RECOVERY);
    }
    if(input == 16)
    {
        res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON,
                RestartCallback, NULL, TAF_MNGDPM_RESTART_REASON_VENDOR_1);
    }

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
     taf_mngdPm_ResponseMode_t ResponseMode, le_result_t result, void* contextPtr)
{
    LE_INFO("ForcedSystemShutdownCallBack response mode is %d and result %d", ResponseMode, result);
    if(ResponseMode == 0)
    {
        LE_INFO("----ForcedSystemShutdown success----");
    }
    else{
        exit(EXIT_FAILURE);
    }
}

void MultiClntRestartSystemCB(taf_mngdPm_RestartMode_t mode, taf_mngdPm_ResponseMode_t rspmode ,
        le_result_t result, void* contextPtr)
{
    LE_INFO("MultiClntRestartSystemCB response mode is %d and result %d", rspmode, result);
    if(rspmode == 0)
    {
        LE_INFO("----Restart System success----");
    }
}

void* MultiClntRestartSystemFunction(void* threadID) {

    taf_mngdPm_ConnectService();
    le_result_t res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT,
                MultiClntRestartSystemCB, NULL, TAF_MNGDPM_RESTART_REASON_NORMAL);
    if(res == LE_OK)
    {
            printf("MultiClntRestartSystem Requested\n");
    }
    le_sem_Post(semRef);
    le_event_RunLoop();
}

void MultiClntRestartSystem()
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
                                    MultiClntRestartSystemFunction, NULL);
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

void MultiClntForcedSysShutdownCB(taf_mngdPm_ShutdownMode_t mode,
     taf_mngdPm_ResponseMode_t ResponseMode, le_result_t result, void* contextPtr)
{
    LE_INFO("MultiClntForcedSysShutdownCB response mode is %d and result %d", ResponseMode, result);
    if(ResponseMode == 0)
    {
        LE_INFO("----MultiClntForcedSysShutdown success----");
    }
}

void* MultiClntForcedSysShutdownFunction(void* threadID) {

    taf_mngdPm_ConnectService();
    le_result_t res = taf_mngdPm_ShutdownReqAsync(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
            MultiClntForcedSysShutdownCB, NULL, TAF_MNGDPM_SHUTDOWN_REASON_NORMAL);
    if(res == LE_OK)
    {
            printf("ForcedSysShutdown Requested\n");
    }
    le_sem_Post(semRef);
    le_event_RunLoop();
}

void MultiClntForcedSysShutdown()
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
                                    MultiClntForcedSysShutdownFunction, NULL);
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

void MultiClntWakeupVehicleCB(int32_t reason, int32_t rspmode ,
        le_result_t result, void* contextPtr)
{
    LE_INFO("WakeupVehicleback response is %d and result %d", rspmode, result);
}

void* MultiClntWakeupVehicleFunction(void* threadID) {

    taf_mngdPm_ConnectService();
    le_result_t res = taf_mngdPm_WakeupVehicleReqAsync(VEHICHLE_WAKEUP_REASON_DEFAULT,
            MultiClntWakeupVehicleCB, NULL);
    if(res == LE_OK)
    {
            printf("WakeupVehicle Requested\n");
    }
    le_sem_Post(semRef);
    le_event_RunLoop();
}

void MultiClntWakeupVehicle()
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
                                    MultiClntWakeupVehicleFunction, NULL);
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

static void ForcedSystemShutdownWithReason()
{
    LE_INFO("----ForcedSystemShutdown test----");
    int input;
    le_result_t res = LE_FAULT;
    char buffer[100];
    uint8_t pmNodeId = 0;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE", pmNodeId);

    printf("Choose the shutdown reason\n -1.Exit\n 0.TAF_MNGDPM_SHUTDOWN_REASON_NORMAL\n "
            "1.TAF_MNGDPM_SHUTDOWN_REASON_BUB_ACTIVE\n 16.TAF_MNGDPM_SHUTDOWN_REASON_VENDOR_1\n ");
    if(fgets(buffer, sizeof(buffer), stdin))
        LE_INFO("Value read successfully");
    buffer[strcspn(buffer, "\n")] = '\0';
    input = atoi(buffer);
    LE_INFO("input: %d", input);
    if(input == 0)
    {
        res = taf_mngdPm_ShutdownReqAsync(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
                ForcedSystemShutdownCallBack, NULL, TAF_MNGDPM_SHUTDOWN_REASON_NORMAL);
    }
    if(input == 1)
    {
        res = taf_mngdPm_ShutdownReqAsync(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
                ForcedSystemShutdownCallBack, NULL, TAF_MNGDPM_SHUTDOWN_REASON_BUB_ACTIVE);
    }
    if(input == 16)
    {
        res = taf_mngdPm_ShutdownReqAsync(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
                ForcedSystemShutdownCallBack, NULL, TAF_MNGDPM_SHUTDOWN_REASON_VENDOR_1);
    }

    if(res == LE_OK)
    {
        LE_INFO("----ForcedSystemShutdown requested----");
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
         wsRef = taf_mngdPm_NewNodeWakeupSource(pmNodeId, TAF_MNGDPM_APP_STAYAWAKE, wsTag);
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
         wsRef = taf_mngdPm_NewNodeWakeupSource(pmNodeId, TAF_MNGDPM_APP_STAYAWAKE, wsTag);
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


void GracefulSysSuspendWithAckType(const char* status)
{
   LE_INFO("GracefulSysSuspendWithAckType");
    int Result = (int)atoi(status);
   stateChangeAck = Result;
   GracefulSysSuspend(0);
   LE_INFO("stateChangeAck is %d", stateChangeAck);
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
        le_result_t result, void* contextPtr)
{
    LE_INFO("WakeupVehicleback response is %d and result %d", rspmode, result);
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

void BubCallBack1( int32_t status, void *contextptr)
{
    LE_INFO("BubCallBack is:%d", status);
    if(status == TAF_MNGDPM_BUB_STATUS_IN_USE)
    {
        printf("Bub is TAF_MNGDPM_BUB_STATUS_IN_USE \n");
        le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(42);
        if(res == LE_OK) {
            LE_INFO("taf_mngdPm_AuthorizeStayAwakeReason");
        }
       //UC1 ( BUB active + ecall = OFF => shutdown)
        wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_1,
                TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRef != NULL) {
            LE_INFO("CreateWakeupSource ref is created for STAY_AWAKE_REASON_VENDOR_1");
            res = taf_mngdPm_StayAwake(wsRef);
            if(res == LE_OK) {
                LE_INFO("Wake up sysytem with StayAwakeReason STAY_AWAKE_REASON_VENDOR_1");
            }
        }
        tafMpmEcallSem = le_sem_Create("tafMpmEcallSem", 0);
        le_sem_WaitWithTimeOut(tafMpmEcallSem, EcallTimeout);
        LE_INFO("Timer expired");
        res = taf_mngdPm_SetNodeTargetedPowerMode(0, TAF_MNGDPM_SHUTDOWN);
        if(res == LE_OK) {
            LE_INFO("SetNodeTargetedPowerMode TAF_MNGDPM_SHUTDOWN");
        }
        le_sem_WaitWithTimeOut(tafMpmEcallSem, EcallTimeout); //stay awake till timer expires
        LE_INFO("Timer expired for TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_1");
        res = taf_mngdPm_Relax(wsRef);
        if(res == LE_OK) {
            LE_INFO("Relax sysytem with TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_1");
        }
        le_sem_Delete(tafMpmEcallSem);
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

void BubCallBack2( int32_t status, void *contextptr)
{
    LE_INFO("BubCallBack is:%d", status);
    if(status == TAF_MNGDPM_BUB_STATUS_IN_USE)
    {
        printf("Bub is TAF_MNGDPM_BUB_STATUS_IN_USE \n");
        le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(42);
        if(res == LE_OK) {
            LE_INFO("taf_mngdPm_AuthorizeStayAwakeReason");
        }
        //UC_3: BUB active + ecall = CALLBACK => suspend
        wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_1,
                TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRef != NULL) {
            LE_INFO("CreateWakeupSource ref is created for STAY_AWAKE_REASON_VENDOR_1");
            res = taf_mngdPm_StayAwake(wsRef);
            if(res == LE_OK) {
                LE_INFO("Wake up sysytem with StayAwakeReason STAY_AWAKE_REASON_VENDOR_1");
            }
        }
        taf_mngdPm_wsRef_t wsRef0 = taf_mngdPm_CreateWakeupSource(
                TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if (wsRef0 != NULL)
        {
            LE_INFO("CreateWakeupSource ref is created for STAY_AWAKE_REASON_ECALL_ACTIVE");
            res = taf_mngdPm_StayAwake(wsRef0);
            if(res == LE_OK) {
                LE_INFO("Wake up sysytem with TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE");
            }
        }
        tafMpmEcallSem = le_sem_Create("tafMpmEcallSem", 0);
        le_sem_WaitWithTimeOut(tafMpmEcallSem, EcallTimeout);
        LE_INFO("ECall timer expired");
        res = taf_mngdPm_SetNodeTargetedPowerMode(0, TAF_MNGDPM_SUSPEND);
        if(res == LE_OK) {
            LE_INFO("SetNodeTargetedPowerMode TAF_MNGDPM_SUSPEND");
        }
        //Releasing PM_REN acquired wakelocks
        res = taf_mngdPm_Relax(wsRef);
        if(res == LE_OK) {
            LE_INFO("Relax sysytem with TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_1");
        }
        le_sem_WaitWithTimeOut(tafMpmEcallSem, EcallTimeout);
        LE_INFO("Timer expired");
        //Releasing IVC acquired wakelocks
        res = taf_mngdPm_Relax(wsRef0);
        if(res == LE_OK) {
            LE_INFO("Relax sysytem with TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE");
        }
        le_sem_Delete(tafMpmEcallSem);
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

void BubCallBack3( int32_t status, void *contextptr)
{
    LE_INFO("BubCallBack is:%d", status);
    if(status == TAF_MNGDPM_BUB_STATUS_IN_USE)
    {
        printf("Bub is TAF_MNGDPM_BUB_STATUS_IN_USE \n");
        le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(42);
        if(res == LE_OK) {
            LE_INFO("taf_mngdPm_AuthorizeStayAwakeReason");
        }
        //UC4 SW update use case
        wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_1,
                TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRef != NULL) {
            LE_INFO("CreateWakeupSource ref is created for STAY_AWAKE_REASON_VENDOR_1");
            res = taf_mngdPm_StayAwake(wsRef);
            if(res == LE_OK) {
                LE_INFO("Wake up sysytem with StayAwakeReason STAY_AWAKE_REASON_VENDOR_1");
            }
        }
        taf_mngdPm_wsRef_t wsRef1 = taf_mngdPm_CreateWakeupSource(
                TAF_MNGDPM_STAY_AWAKE_REASON_SW_UPDATE, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if (wsRef1 != NULL)
        {
            LE_INFO("CreateWakeupSource ref is created for TAF_MNGDPM_STAY_AWAKE_REASON_SW_UPDATE");
            res = taf_mngdPm_StayAwake(wsRef1);
            if(res == LE_OK) {
                LE_INFO("Wake up sysytem with wakeuptype TAF_MNGDPM_STAY_AWAKE_REASON_SW_UPDATE");
            }
        }
        tafMpmEcallSem = le_sem_Create("tafMpmEcallSem", 0);
        le_sem_WaitWithTimeOut(tafMpmEcallSem, EcallTimeout);
        LE_INFO("Timer expired");
        res = taf_mngdPm_SetNodeTargetedPowerMode(0, TAF_MNGDPM_SHUTDOWN);
        if(res == LE_OK) {
            LE_INFO("SetNodeTargetedPowerMode TAF_MNGDPM_SHUTDOWN");
        }
        res = taf_mngdPm_Relax(wsRef);
        if(res == LE_OK) {
            LE_INFO("Relax sysytem with TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_1");
        }
        le_sem_WaitWithTimeOut(tafMpmEcallSem, EcallTimeout); //stay awake till timer expires
        LE_INFO("Timer expired for TAF_MNGDPM_STAY_AWAKE_REASON_SW_UPDATE");
        res = taf_mngdPm_Relax(wsRef1);
        if(res == LE_OK) {
            LE_INFO("Relax sysytem with TAF_MNGDPM_STAY_AWAKE_REASON_SW_UPDATE");
        }
        le_sem_Delete(tafMpmEcallSem);
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

void BubCallBack4( int32_t status, void *contextptr)
{
    LE_INFO("BubCallBack is:%d", status);
    if(status == TAF_MNGDPM_BUB_STATUS_IN_USE)
    {
        printf("Bub is TAF_MNGDPM_BUB_STATUS_IN_USE \n");
        le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(42);
        if(res == LE_OK) {
            LE_INFO("taf_mngdPm_AuthorizeStayAwakeReason");
        }
        //UC5 Shutdown use case ( ECall state transition from CALLBACK to OFF & BUB active)
        wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_1,
                TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRef != NULL) {
            LE_INFO("CreateWakeupSource ref is created for STAY_AWAKE_REASON_VENDOR_1");
            res = taf_mngdPm_StayAwake(wsRef);
            if(res == LE_OK) {
                LE_INFO("Wake up sysytem with StayAwakeReason STAY_AWAKE_REASON_VENDOR_1");
            }
        }
        tafMpmEcallSem = le_sem_Create("tafMpmEcallSem", 0);
        le_sem_WaitWithTimeOut(tafMpmEcallSem, EcallTimeout);
        LE_INFO("Timer expired");
        res = taf_mngdPm_SetNodeTargetedPowerMode(0, TAF_MNGDPM_SHUTDOWN);
        if(res == LE_OK) {
            LE_INFO("SetNodeTargetedPowerMode TAF_MNGDPM_SHUTDOWN");
        }
        le_sem_WaitWithTimeOut(tafMpmEcallSem, EcallTimeout); //stay awake till timer expires
        LE_INFO("Timer expired for TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_1");
        res = taf_mngdPm_Relax(wsRef);
        if(res == LE_OK) {
            LE_INFO("Relax sysytem with TAF_MNGDPM_STAY_AWAKE_REASON_VENDOR_1");
        }
        le_sem_Delete(tafMpmEcallSem);
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

void BubCallBack5( int32_t status, void *contextptr)
{
    LE_INFO("BubCallBack is:%d", status);
    if(status == TAF_MNGDPM_BUB_STATUS_IN_USE)
    {
        printf("Bub is TAF_MNGDPM_BUB_STATUS_IN_USE \n");
    }
    else if(status == TAF_MNGDPM_BUB_STATUS_NOT_IN_USE)
    {
        printf("Bub is TAF_MNGDPM_BUB_STATUS_NOT_IN_USE\n");
        //UC9 suspend use case
        tafMpmEcallSem = le_sem_Create("tafMpmEcallSem", 0);
        le_sem_WaitWithTimeOut(tafMpmEcallSem, EcallTimeout);
        LE_INFO("Timer expired");
        le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(AUTHORIZE_ALL_STAY_AWAKE_REASON);
        if(res == LE_OK) {
            LE_INFO("taf_mngdPm_AuthorizeStayAwakeReason");
        }
        res = taf_mngdPm_SetNodeTargetedPowerMode(0, TAF_MNGDPM_SUSPEND);
        if(res == LE_OK) {
            LE_INFO("SetNodeTargetedPowerMode TAF_MNGDPM_SUSPEND");
        }
        le_sem_Delete(tafMpmEcallSem);
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

void AddInfoReportHandler(void * bubCallBack)
{
    LE_INFO("AddInfoReportHandler");
    taf_mngdPm_InfoReportHandlerRef_t handlerRef;
    handlerRef = taf_mngdPm_AddInfoReportHandler((taf_mngdPm_InfoReportBitMask_t)1, bubCallBack, NULL);
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
    wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_APP_STAYAWAKE, wsTag);
    if(wsRef != NULL) {
        LE_INFO("NewNodeWakeupSource ref is created for APP_STAYAWAKE");
        res = taf_mngdPm_StayAwakeNode(wsRef);
        if(res == LE_OK) {
            LE_INFO("Wake up sysytem with wakeuptype APP_STAYAWAKE");
        }
    }

    taf_mngdPm_wsRef_t wsRefSms0 = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_SMS, wsTag);
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
    taf_mngdPm_wsRef_t wsRefSms1 = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_SMS, wsTag);
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

static void ForcedSystemShutdownAndSuspend()
{
    LE_INFO("----ForcedSystemShutdown test----");
    le_result_t result;
    uint8_t pmNodeId = 0;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE", pmNodeId);
     if(wsRef == NULL)
         wsRef = taf_mngdPm_NewNodeWakeupSource(pmNodeId, TAF_MNGDPM_APP_STAYAWAKE, wsTag);
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
            ForcedSystemShutdownCallBack, NULL, TAF_MNGDPM_SHUTDOWN_REASON_NORMAL);

    if(result == LE_OK)
    {
        result = taf_mngdPm_RelaxNode(wsRef);
        if(result == LE_OK) {
            LE_INFO("Triggered suspend sysytem");
        }
        LE_INFO("----ForcedSystemShutdown success----");
    }
    else
    {
        LE_ERROR("ForcedSystemShutdown request failed");
        exit(EXIT_FAILURE);
    }
}

static void AllowWakingupDuringSuspending()
{
    LE_INFO("----ForcedSystemShutdown test----");
    le_result_t result;
    uint8_t pmNodeId = 0;
     if(wsRef == NULL)
         wsRef = taf_mngdPm_NewNodeWakeupSource(pmNodeId, TAF_MNGDPM_APP_STAYAWAKE, wsTag);
     if(wsRef != NULL) {
         LE_INFO("NewNodeWakeupSource ref is created for APP_STAYAWAKE");
         if (wsRef != NULL) {
             result = taf_mngdPm_StayAwakeNode(wsRef);
             if(result == LE_OK) {
                 LE_INFO("Resumed sysytem with wakeuptype APP_STAYAWAKE");
             }
             else {
                 LE_INFO("Failed to acquire Wake source");
                 exit(EXIT_FAILURE);
             }
             result = taf_mngdPm_RelaxNode(wsRef);
             if(result == LE_OK) {
                 LE_INFO("suspended sysytem with wakeuptype APP_STAYAWAKE");
             result = taf_mngdPm_StayAwakeNode(wsRef);
             if(result == LE_OK) {
                 LE_INFO("Resumed sysytem with wakeuptype APP_STAYAWAKE");
                 exit(EXIT_SUCCESS);
             }
             }
         }
     }
     else {
         LE_ERROR("Failed to create wakeup source!");
        exit(EXIT_FAILURE);
     }
}

static void AllowAuthorizedWakingupDuringSuspending()
{
    LE_INFO("----AllowAuthorizedWakingupDuringSuspending test----");
    le_result_t result;
    if(wsRef == NULL)
        wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if(wsRef != NULL) {
        LE_INFO("WakeupSource ref is created for TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL");
        if (wsRef != NULL) {
            result = taf_mngdPm_StayAwake(wsRef);
            if(result == LE_OK) {
                LE_INFO("Resumed system with wakeuptype TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL");
                result = taf_mngdPm_Relax(wsRef);
                if(result == LE_OK) {
                    LE_INFO("suspended system with wakeuptype TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL");
                    result = taf_mngdPm_StayAwake(wsRef);
                    if(result == LE_OK) {
                        LE_INFO("Resumed system with wakeuptype TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL");
                    }
                }
            }
            else {
                LE_INFO("Failed to acquire Wake source");
                exit(EXIT_FAILURE);
            }
        }
    }
    else {
        LE_ERROR("Failed to create wakeup source!");
        exit(EXIT_FAILURE);
    }
}

static void ShouldRejectUnauthorizedWakingupDuringSuspending()
{
    LE_INFO("----ShouldRejectUnauthorizedWakingupDuringSuspending test----");
    int reason = TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL;
    int unauthorizedReason = TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE;
    // Authorized the reason for bit0
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
    if(res == LE_OK) {
        printf("'AuthorizeStayAwakeReason for bitmask %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
        taf_mngdPm_wsRef_t wsRefAuthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized) {
            printf("Created WakeupSource ref for authorized stay-awake reason %d\n", reason);
            if(wsRefAuthorized != NULL) {
                res = taf_mngdPm_StayAwake(wsRefAuthorized);
                if(res == LE_OK) {
                    printf("'Resumed system with wsRefAuthorized'\n");
                    res = taf_mngdPm_Relax(wsRefAuthorized);
                    if(res == LE_OK) {
                        printf("'Supended system with wsRefAuthorized'\n");
                        taf_mngdPm_wsRef_t wsRefUnauthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
                        if(wsRefUnauthorized) {
                            printf("Created WakeupSource ref for unauthorized stay-awake reason %d\n", unauthorizedReason);
                            taf_mngdPm_StayAwake(wsRefUnauthorized);
                            if(res == LE_OK) {
                                printf("'Resumed system with wsRefUnauthorized'\n");
                            }
                            else if(res == LE_NOT_PERMITTED){
                                LE_INFO("Resume system with unauthorized ws is not permitted");
                                exit(EXIT_SUCCESS);
                            }
                        }
                    }
                    else {
                        LE_INFO("Failed to release authorized wake source");
                        exit(EXIT_FAILURE);
                    }
                }
                else {
                    LE_INFO("Failed to acquire authorized wake source");
                    exit(EXIT_FAILURE);
                }
            }
        }
        else {
            printf("Failed to create wakeupsource ref for authorized reason %d\n", reason);
            exit(EXIT_FAILURE);
        }
    }
}

void TestBubCases()
{
    LE_INFO("testBubCases");
    int input = 0;
    char buffer[100];

    printf("Choose the BUB test case \n 8.Exit\n 1. UC_1: BUB active + ecall inactive => shutdown\n"
    " 2. UC_3: BUB active + ecall callback => suspend\n"
    " 3. UC_4: BUB active + SoftWare Update  => shutdown\n"
    " 4. UC_5: BUB active + ecall callback end => wakeup + enter resume state + shutdown\n"
    " 5. UC_9: BUB inactive + vehichle ON power mode => suspend\n ");
    if(fgets(buffer, sizeof(buffer), stdin))
        LE_INFO("Value read successfully");
    buffer[strcspn(buffer, "\n")] = '\0';
    input = atoi(buffer);
    LE_INFO("input: %d", input);
    if(input == 1)
    {
            LE_INFO("BUB active + ecall = OFF => shutdown");
            printf("Sets the targeted power mode as SHUTDOWN assuming there's no eCall happens.\n");
            taf_mngdPm_InfoReportHandlerRef_t handlerRef;
            handlerRef = taf_mngdPm_AddInfoReportHandler((taf_mngdPm_InfoReportBitMask_t)1,
                    BubCallBack1, NULL);
            if(handlerRef)
            {
                 LE_INFO("AddInfoReportHandler is success");
            }
    }
    if(input == 2)
    {
            LE_INFO("UC_3: BUB active + ecall = CALLBACK=> suspend");
            printf("Sets the targeted power mode to SUSPEND after eCall happens.\n");
            taf_mngdPm_InfoReportHandlerRef_t handlerRef;
            handlerRef = taf_mngdPm_AddInfoReportHandler((taf_mngdPm_InfoReportBitMask_t)1,
                    BubCallBack2, NULL);
            if(handlerRef)
            {
                 LE_INFO("AddInfoReportHandler is success");
            }
    }
    if(input == 3)
    {
            LE_INFO("UC_4: BUB active + SWL in critical phase\n");
            printf("stay awake until SWL exits critical phase, then shutdown.\n");
            taf_mngdPm_InfoReportHandlerRef_t handlerRef;
            handlerRef = taf_mngdPm_AddInfoReportHandler((taf_mngdPm_InfoReportBitMask_t)1,
                    BubCallBack3, NULL);
            if(handlerRef)
            {
                 LE_INFO("AddInfoReportHandler is success");
            }
    }
    if(input == 4)
    {
            LE_INFO("UC_5: ECall from CALLBACK to OFF & BUB active\n");
            printf("stay awake until Wake Lock exits then shutdown"
                    "as none of the previously authorized wakeup sources is active.\n");
            taf_mngdPm_InfoReportHandlerRef_t handlerRef;
            handlerRef = taf_mngdPm_AddInfoReportHandler((taf_mngdPm_InfoReportBitMask_t)1,
                    BubCallBack4, NULL);
            if(handlerRef)
            {
                 LE_INFO("AddInfoReportHandler is success");
            }
    }
    if(input == 5)
    {
            LE_INFO("UC_9: BUB inactive while ON power mode -> authorize all stay awakes");
            printf("Sets targeted power mode to SUSPEND.\n");
            taf_mngdPm_InfoReportHandlerRef_t handlerRef;
            handlerRef = taf_mngdPm_AddInfoReportHandler((taf_mngdPm_InfoReportBitMask_t)1,
                    BubCallBack5, NULL);
            if(handlerRef)
            {
                 LE_INFO("AddInfoReportHandler is success");
            }
    }
    if(input == 8)
    {
        exit(EXIT_SUCCESS);
    }

}

static void* TestNodeWakeSource(void* ctxPtr)
{
    LE_INFO("TestNodeWakeSource");
    taf_mngdPm_ConnectService();
    int input = 1;
    le_result_t res = LE_FAULT;
    taf_mngdPm_wsRef_t wsRef = NULL;
    char buffer[100];

    while(input != -1)
    {
        printf("Choose the TestNodeWakeSource Test Case\n -1.Exit\n 1.NewNodeWakeupSource\n "
                "2.ResumeSystem\n 3.SuspendSystem\n ");
        if(fgets(buffer, sizeof(buffer), stdin))
            LE_INFO("Value read successfully");
        buffer[strcspn(buffer, "\n")] = '\0';
        input = atoi(buffer);
        LE_INFO("input: %d", input);
        if(input == 1)
        {
            char NodeId[100];
            printf("Enter NODE_ID\n -1.Exit\n 0.PVM\n 1.RPC\n");
            if(fgets(NodeId, sizeof(NodeId), stdin))
                LE_INFO("Value read successfully");
            NodeId[strcspn(NodeId, "\n")] = '\0';
            uint8_t NODE_ID = atoi(NodeId);
            if(NODE_ID == -1)
                continue;
            printf("Enter WakeupType for NewNodeWakeupSource\n -1.Exit\n 0.APP_STAYAWAKE\n 1.SMS \n 2.VOICE_CALL \n 3.MCU_VHAL \n");
            char NewNodeWakeupSource[100];
            int wakeuptype;
            if(fgets(NewNodeWakeupSource, sizeof(NewNodeWakeupSource), stdin))
                LE_INFO("Value read successfully");
            NewNodeWakeupSource[strcspn(NewNodeWakeupSource, "\n")] = '\0';
            wakeuptype = atoi(NewNodeWakeupSource);
            if(wakeuptype == -1)
                continue;
            if(NODE_ID == 0) {
                wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, wakeuptype, wsTag);
                if(wsRef)
            printf("NewNodeWakeupSource wakeuptype is %d for NODE_ID %d\n", wakeuptype, NODE_ID);
           }
           else if(NODE_ID == 1) {
                wsRef1 = taf_mngdPm_NewNodeWakeupSource(NODE_ID, wakeuptype, wsTag);
                if(wsRef1)
            printf("NewNodeWakeupSource wakeuptype is %d for NODE_ID %d\n", wakeuptype, NODE_ID);
           }
        }
        if(input == 2)
        {
            char StayAwakeNode[100];
            printf("Enter NODE_ID\n -1.Exit\n 0.PVM\n 1.RPC\n");
            if(fgets(StayAwakeNode, sizeof(StayAwakeNode), stdin))
                LE_INFO("Value read successfully");
            StayAwakeNode[strcspn(StayAwakeNode, "\n")] = '\0';
            uint8_t NODE_ID = atoi(StayAwakeNode);
            if(NODE_ID == -1)
                continue;
            if(NODE_ID == 0) {
                printf("Resume PVM System\n");
                if(wsRef != NULL) {
                    res = taf_mngdPm_StayAwakeNode(wsRef);
                    if(res == LE_OK) {
                        printf("'Resumed sysytem'\n");
                     }
                }
                else
                    printf("'wsRef is null, Call NewNodeWakeupSource'\n");
            }
            else if(NODE_ID == 1) {
                printf("Resume RPC System\n");
                 if(wsRef1 != NULL) {
                     res = taf_mngdPm_StayAwakeNode(wsRef1);
                     if(res == LE_OK) {
                         printf("'Resumed sysytem'\n");
                      }
                 }
                else
                    printf("'wsRef is null for Rpc, Call NewNodeWakeupSource'\n");
            }
        }
        if(input == 3)
        {
            char RelaxNode[100];
            printf("Enter NODE_ID\n -1.Exit\n 0.PVM\n 1.RPC\n");
            if(fgets(RelaxNode, sizeof(RelaxNode), stdin))
                LE_INFO("Value read successfully");
            RelaxNode[strcspn(RelaxNode, "\n")] = '\0';
            uint8_t NODE_ID = atoi(RelaxNode);
            if(NODE_ID == -1)
                continue;
            if(NODE_ID == 0) {
                printf("Suspend PVM System\n");
                if(wsRef != NULL) {
                    res = taf_mngdPm_RelaxNode(wsRef);
                    if(res == LE_OK) {
                        printf("'Suspended PVM system'\n");
                     }
                }
                else
                    printf("'wsRef is null, Call NewNodeWakeupSource'\n");
            }
            else if(NODE_ID == 1) {
                printf("Suspend RPC System\n");
                 if(wsRef1 != NULL) {
                     res = taf_mngdPm_RelaxNode(wsRef1);
                     if(res == LE_OK) {
                         printf("'Suspended RPC system'\n");
                      }
                 }
                else
                    printf("'wsRef is null for Rpc, Call NewNodeWakeupSource'\n");
            }
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

void TestNodeWakeSourceCases()
{
    semRef = le_sem_Create("MngdIntTestApp", 0);
    queueSemRef = le_sem_Create("MngdPMIntQueueSem", 0);
    LE_INFO("createapp1 start");
        threadRef = le_thread_Create("inttestapp",
                                    TestNodeWakeSource, NULL);
        le_thread_Start(threadRef);
        le_sem_Wait(semRef);
}

static void* connect_service(void* ctxPtr)
{
    LE_INFO("TestWakeSourceSampleApp");
    taf_mngdPm_ConnectService();
    int input = 1;
    le_result_t res = LE_FAULT;
    taf_mngdPm_wsRef_t wsRef = NULL;
    char buffer[100];

    while(input != -1)
    {
        printf("Choose the TestResumeandSuspend Test Case\n -1.Exit\n 1.AuthorizeStayAwakeReason\n "
                "2.CreateWakeupSource\n 3.ResumeSystem\n 4.SuspendSystem\n ");
        if(fgets(buffer, sizeof(buffer), stdin))
            LE_INFO("Value read successfully");
        buffer[strcspn(buffer, "\n")] = '\0';
        input = atoi(buffer);
        LE_INFO("input: %d", input);
        if(input == 1)
        {
            printf("Enter bitmask for AuthorizeStayAwakeReason\n -1.Exit\n"
                    "\n"
                    " 1.TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL\n"
                    "\n"
                    " 2.TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_ECALL_ACTIVE\n"
                    "\n"
                    " 4.STAY_AWAKE_REASON_BIT_MASK_ECALL_CALLBACK\n"
                    "\n"
                    " 8.TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_SW_UPDATE\n"
                    "\n"
                    " 15.TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL,"
                            " TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_ECALL_ACTIVE,"
                                    " STAY_AWAKE_REASON_BIT_MASK_ECALL_CALLBACK,"
                                            " TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_SW_UPDATE\n"
                    "\n"
                    " 16.TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VEH_NETWORK\n"
                    "\n"
                    " 31.TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VEH_NETWORK,"
                            " STAY_AWAKE_REASON_BIT_MASK_SW_UPDATE,"
                                    " STAY_AWAKE_REASON_BIT_MASK_ECALL_CALLBACK,"
                                            " STAY_AWAKE_REASON_BIT_MASK_ECALL_ACTIVE,"
                                                    " STAY_AWAKE_REASON_BIT_MASK_NORMAL\n"
                    "\n"
                    " 65536.STAY_AWAKE_REASON_BIT_MASK_VENDOR_1\n"
                    "\n"
                    " Z. ALL\n");
            char StayAwakeReason[100];
            if(fgets(StayAwakeReason, sizeof(StayAwakeReason), stdin))
                LE_INFO("Value read successfully");
            StayAwakeReason[strcspn(StayAwakeReason, "\n")] = '\0';
            int entry = -1;
            if(strcmp(StayAwakeReason, "Z")==0)
            {
                le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(AUTHORIZE_ALL_STAY_AWAKE_REASON);
                if(res == LE_OK) {
                    printf("'AuthorizeStayAwakeReason for ALL bitmask is set'\n");
                    LE_INFO("AUTHORIZE_ALL_STAY_AWAKE_REASON %u", AUTHORIZE_ALL_STAY_AWAKE_REASON);
               }
            }
            else
            {
                entry = atoi(StayAwakeReason);
                le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(entry);
                if(res == LE_OK)
                    printf("'AuthorizeStayAwakeReason for bitmask %s is set'\n", StayAwakeReason);
            }
            if(entry == -1)
                continue;
        }
        if(input == 2)
        {
            printf("Enter stayawake reason for CreateWakeupSource\n -1.Exit\n"
            " 0.TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL\n"
            " 1.TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE\n"
            " 2.TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_CALLBACK\n"
            " 3.TAF_MNGDPM_STAY_AWAKE_REASON_SW_UPDATE\n"
            " 4.TAF_MNGDPM_STAY_AWAKE_REASON_VEH_NETWORK\n"
            " 16.STAY_AWAKE_REASON_BIT_MASK_VENDOR_1\n");
            char CreateWakeupSource[100];
            int reason;
            if(fgets(CreateWakeupSource, sizeof(CreateWakeupSource), stdin))
                LE_INFO("Value read successfully");
            CreateWakeupSource[strcspn(CreateWakeupSource, "\n")] = '\0';
            reason = atoi(CreateWakeupSource);
            if(reason == -1)
                continue;
            wsRef = taf_mngdPm_CreateWakeupSource(reason, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
            if(wsRef)
                printf("Created WakeupSource ref for reason %d\n", reason);
            else
                printf("Failed to Create WakeupSource ref for reason %d\n", reason);

        }
        if(input == 3)
        {
            printf("Resume PVM System\n");
            if(wsRef != NULL) {
                res = taf_mngdPm_StayAwake(wsRef);
                if(res == LE_OK) {
                    printf("'Resumed sysytem'\n");
                 }
            }
            else
                printf("'wsRef is null, Call NewNodeWakeupSource'\n");
        }
        if(input == 4)
        {
            printf("Suspend PVM System\n");
            if(wsRef != NULL) {
                res = taf_mngdPm_Relax(wsRef);
                if(res == LE_OK) {
                    printf("'Suspended PVM system'\n");
                 }
            }
            else
                printf("'wsRef is null, Call NewNodeWakeupSource'\n");
        }
    }
    le_sem_Post(semRef);
    exit(EXIT_FAILURE);
    le_event_RunLoop();
}
void* ThreadFunction(void* threadID) {

    taf_mngdPm_ConnectService();
    le_result_t res;

    wsRef = taf_mngdPm_NewNodeWakeupSource(0, 1, wsTag);
    if(wsRef)
        printf("NewNodeWakeupSource ref is created for\n");
    if(wsRef != NULL) {
        res = taf_mngdPm_StayAwakeNode(wsRef);
        if(res == LE_OK) {
            printf("Resumed sysytem\n");
         }
    }

    le_sem_Post(semRef);
    le_event_RunLoop();
}

void ReAuthorizeStayAwakeReason() {

    taf_mngdPm_ConnectService();
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(4);
    if(res == LE_OK) {
        printf("AuthorizeStayAwakeReason is set to ecall callback");
        exit(EXIT_SUCCESS);
    }
}

void TestClearUnAuthorizedWakeSource()
{
    taf_mngdPm_ConnectService();
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(1);
    if(res == LE_OK)
        printf("AuthorizeStayAwakeReason is set to normal");
    wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
            TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if(wsRef)
        printf("CreateWakeupSource for TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL\n");
    if(wsRef != NULL) {
        res = taf_mngdPm_StayAwake(wsRef);
        if(res == LE_OK) {
            printf("'Resumed sysytem'\n");
         }
    }
    ReAuthorizeStayAwakeReason();
}

void CreateMultipleClients()
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

void TestWakeSourceCases()
{
    semRef = le_sem_Create("MngdIntTestApp", 0);
    queueSemRef = le_sem_Create("MngdPMIntQueueSem", 0);
    LE_INFO("createapp1 start");
        threadRef = le_thread_Create("inttestapp",
                                    connect_service, NULL);
        le_thread_Start(threadRef);
        le_sem_Wait(semRef);
}

static void* acquireWakeLock1(void* ctxPtr)
{
    LE_INFO("acquireWakeLock1");
    taf_mngdPm_ConnectService();
    int reason = 0;
    le_result_t res = LE_FAULT;
    int *entry = (int*)(ctxPtr);
    LE_INFO("entry %u", *entry);
    if(*entry == 1)
        reason = 0;
    else if(*entry == 5)
        reason = 2;

    wsRef1 = taf_mngdPm_CreateWakeupSource(reason, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if(wsRef1) {
        printf("Created WakeupSource ref for reason %d\n", reason);
        if(wsRef1 != NULL) {
            res = taf_mngdPm_StayAwake(wsRef1);
            if(res == LE_OK) {
                printf("'Resumed sysytem with wsRef1'\n");
             }
        }
    }
    else
        printf("Failed to Create WakeupSource ref for reason %d\n", reason);

    le_sem_Post(semRef1);
    le_event_RunLoop();
}

static void* acquireWakeLock2(void* ctxPtr)
{
    LE_INFO("acquireWakeLock2");
    taf_mngdPm_ConnectService();
    int reason = 0;
    le_result_t res = LE_FAULT;
    int *entry = (int*)(ctxPtr);
    res = taf_mngdPm_AuthorizeStayAwakeReason(*entry);
    LE_INFO("entry %u", *entry);
    if(*entry == 1)
        reason = 0;
    else if(*entry == 5)
        reason = 2;

    wsRef2 = taf_mngdPm_CreateWakeupSource(reason, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if(wsRef2) {
        printf("Created WakeupSource ref for reason %d\n", reason);
        if(wsRef2 != NULL) {
            res = taf_mngdPm_StayAwake(wsRef2);
            if(res == LE_OK) {
                printf("'Resumed sysytem with wsRef2'\n");
             }
        }
    }
    else
        printf("Failed to Create WakeupSource ref for reason %d\n", reason);

    le_sem_Post(semRef2);
    le_event_RunLoop();
}

void authorizeAndAcquireLocks(int entry, bool isLocksRequired)
{
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(entry);
    if(res == LE_OK) {
        printf("'AuthorizeStayAwakeReason for bitmask %d is set'\n", entry);
        LE_INFO("entry %u", entry);
        if(isLocksRequired) {
            void *ptr = &entry;
            semRef1 = le_sem_Create("MngdIntTestApp", 0);
            threadRef1 = le_thread_Create("inttestapp",
                                        acquireWakeLock1, ptr);
            le_thread_Start(threadRef1);
            le_sem_Wait(semRef1);

            semRef2 = le_sem_Create("MngdIntTestApp", 0);
            threadRef2 = le_thread_Create("inttestapp",
                                        acquireWakeLock2, ptr);
            le_thread_Start(threadRef2);
            le_sem_Wait(semRef2);
        }
    }
}

//-------- UC-2 -----------//
void StayAwakeWithUnauthorizedWsShouldNotChangeSystemState()
{
    int reason = TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL;
    //Acquire an unauthorized reason ws
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_ECALL_ACTIVE);
    if(res == LE_OK){
        printf("'AuthorizeStayAwakeReason for bitmask %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE);
        taf_mngdPm_wsRef_t wsRefUnauthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefUnauthorized) {
            printf("Created WakeupSource ref for unauthorized reason %d\n", reason);
            if(wsRefUnauthorized != NULL) {
                res = taf_mngdPm_StayAwake(wsRefUnauthorized);
                if(res == LE_OK) {
                    printf("'Successfully acquired wsRefUnauthorized'\n");
                }
            }
        }
        else
            printf("Failed to Create WakeupSource ref for authorized reason %d\n", reason);
    }
}

//-------- UC-4 -----------//
void StayAwakeWithAuthorizedWsShouldResultInSuspend()
{
    int reason = TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL;
    taf_mngdPm_wsRef_t wsRefAuthorized = NULL;
    // Authorized the reason for bit0
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
    if(res == LE_OK) {
        printf("'AuthorizeStayAwakeReason for bitmask %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
        wsRefAuthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized) {
            printf("Created WakeupSource ref for reason %d\n", reason);
            if(wsRefAuthorized != NULL) {
                res = taf_mngdPm_StayAwake(wsRefAuthorized);
                if(res == LE_OK) {
                    printf("'Resumed system with wsRefAuthorized'\n");
                }
            }
        }
        else
            printf("Failed to Create WakeupSource ref for authorized reason %d\n", reason);
    }

    reason = TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE;
    //Acquire an unauthorized reason ws
    taf_mngdPm_wsRef_t wsRefUnauthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if(wsRefUnauthorized) {
        printf("Created WakeupSource ref for reason %d\n", reason);
        if(wsRefUnauthorized != NULL) {
            res = taf_mngdPm_StayAwake(wsRefUnauthorized);
            if(res == LE_OK) {
                printf("'Successfully acquired wsRefUnauthorized'\n");
                printf("'Relax system with wsRefAuthorized'\n");
                taf_mngdPm_Relax(wsRefAuthorized);
            }
        }
    }
    else
        printf("Failed to Create WakeupSource ref for unauthorized reason %d\n", reason);
}

//-------- UC-5 -----------//
void UnauthorizedWsShouldNotResultInSuspend()
{
    int reason = TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL;
    // Authorized the reason for bit0
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
    if(res == LE_OK) {
        printf("'AuthorizeStayAwakeReason for bitmask %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
        taf_mngdPm_wsRef_t wsRefAuthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized) {
            printf("Created WakeupSource ref for reason %d\n", reason);
            if(wsRefAuthorized != NULL) {
                res = taf_mngdPm_StayAwake(wsRefAuthorized);
                if(res == LE_OK) {
                    printf("'Resumed system with wsRefAuthorized'\n");
                }
            }
        }
        else
            printf("Failed to Create WakeupSource ref for authorized reason %d\n", reason);
    }

    reason = TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE;
    //Acquire an unauthorized reason ws
    taf_mngdPm_wsRef_t wsRefUnauthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if(wsRefUnauthorized) {
        printf("Created WakeupSource ref for reason %d\n", reason);
        if(wsRefUnauthorized != NULL) {
            res = taf_mngdPm_StayAwake(wsRefUnauthorized);
            if(res == LE_OK) {
                printf("'Successfully acquired wsRefUnauthorized'\n");
                printf("'Relax system with wsRefUnauthorized'\n");
                taf_mngdPm_Relax(wsRefUnauthorized);
            }
        }
    }
    else
        printf("Failed to Create WakeupSource ref for unauthorized reason %d\n", reason);
}

//-------- UC-6 -----------//
void StayAwakeWithUnauthorizedWsShouldNotResultInSuspend()
{
    int reason = TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL;
    // Authorized the reason for bit0
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
    if(res == LE_OK) {
        printf("'AuthorizeStayAwakeReason for bitmask %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
        taf_mngdPm_wsRef_t wsRefAuthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized) {
            printf("Created WakeupSource ref for reason %d\n", reason);
            if(wsRefAuthorized != NULL) {
                res = taf_mngdPm_StayAwake(wsRefAuthorized);
                if(res == LE_OK) {
                    printf("'Resumed system with wsRefAuthorized'\n");
                }
            }
        }
        else
            printf("Failed to Create WakeupSource ref for authorized reason %d\n", reason);
    }

    reason = TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE;
    //Acquire an unauthorized reason ws
    taf_mngdPm_wsRef_t wsRefUnauthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if(wsRefUnauthorized) {
        printf("Created WakeupSource ref for reason %d\n", reason);
        if(wsRefUnauthorized != NULL) {
            res = taf_mngdPm_StayAwake(wsRefUnauthorized);
            if(res == LE_OK) {
                printf("'Successfully acquired wsRefUnauthorized'\n");
            }
        }
    }
    else
        printf("Failed to Create WakeupSource ref for unauthorized reason %d\n", reason);
}

//-------- UC-7 -----------//
void NotifyPmVhalForUnauthorizedSarAndShouldResultInSuspend()
{
    int reason = TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL;
    // Authorized the reason for bit0
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
    if(res == LE_OK) {
        printf("'AuthorizeStayAwakeReason for bitmask %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
        taf_mngdPm_wsRef_t wsRefAuthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized) {
            printf("Created WakeupSource ref for reason %d\n", reason);
            if(wsRefAuthorized != NULL) {
                res = taf_mngdPm_StayAwake(wsRefAuthorized);
                if(res == LE_OK) {
                    printf("'Resumed system with wsRefAuthorized'\n");
                    // Unauthorize the reason for bit0
                    res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_ECALL_ACTIVE);
                    if(res == LE_OK)
                        printf("'Released Unauthorized wakesource'\n");
                }
            }
        }
        else
            printf("Failed to Create WakeupSource ref for authorized reason %d\n", reason);
    }
}

//-------- UC-8 -----------//
void NotifyPmVhalForUnauthorizedSarAndShouldNotResultInSuspend()
{
    int reason = TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL;
    // Authorized the reason for bit0
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
    if(res == LE_OK) {
        printf("'AuthorizeStayAwakeReason for bitmask %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
        taf_mngdPm_wsRef_t wsRefAuthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized) {
            printf("Created WakeupSource ref for reason %d\n", reason);
            if(wsRefAuthorized != NULL) {
                res = taf_mngdPm_StayAwake(wsRefAuthorized);
                if(res == LE_OK) {
                    printf("'Resumed system with wsRefAuthorized'\n");
                }
            }
        }
        else
            printf("Failed to Create WakeupSource ref for authorized reason %d\n", reason);
    }

    reason = TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE;
    // Authorized the reason for bit 0 & bit 1
    res = taf_mngdPm_AuthorizeStayAwakeReason(3);
    if(res == LE_OK) {
        printf("'AuthorizeStayAwakeReason for bitmask %d, %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL, TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_ECALL_ACTIVE);
        taf_mngdPm_wsRef_t wsRefAuthorized_1 = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized_1) {
            printf("Created WakeupSource ref for reason %d\n", reason);
            if(wsRefAuthorized_1 != NULL) {
                res = taf_mngdPm_StayAwake(wsRefAuthorized_1);
                if(res == LE_OK) {
                    printf("'Successfully acquired wsRefAuthorized_1'\n");
                    // Unauthorize the reason for bit1
                    res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
                    if(res == LE_OK)
                        printf("'Released Unauthorized wakesource'\n");
                }
            }
        }
        else
            printf("Failed to Create WakeupSource ref for unauthorized reason %d\n", reason);
    }
}

//-------- UC-9 -----------//
void NotifyPmVhalForAuthorizedSarAndShouldNotResultInSuspend()
{
    int reason = TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL;
    // Authorized the reason for bit0
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
    if(res == LE_OK) {
        printf("'AuthorizeStayAwakeReason for bitmask %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
        taf_mngdPm_wsRef_t wsRefAuthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized) {
            printf("Created WakeupSource ref for reason %d\n", reason);
            if(wsRefAuthorized != NULL) {
                res = taf_mngdPm_StayAwake(wsRefAuthorized);
                if(res == LE_OK) {
                    printf("'Resumed system with wsRefAuthorized'\n");
                }
            }
        }
        else
            printf("Failed to Create WakeupSource ref for authorized reason %d\n", reason);
    }

    reason = TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE;
    //Acquire an unauthorized reason ws
    taf_mngdPm_wsRef_t wsRefUnauthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if(wsRefUnauthorized) {
        printf("Created WakeupSource ref for reason %d\n", reason);
        if(wsRefUnauthorized != NULL) {
            res = taf_mngdPm_StayAwake(wsRefUnauthorized);
            if(res == LE_OK) {
                printf("'Successfully acquired wsRefUnauthorized'\n");
                // Authorize the reason for bit1
                res = taf_mngdPm_AuthorizeStayAwakeReason(3);
                if(res == LE_OK){
                    printf("'AuthorizeStayAwakeReason for bitmask %d, %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL, TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_ECALL_ACTIVE);
                }
            }
        else
            printf("Failed to Create WakeupSource ref for unauthorized reason %d\n", reason);
        }
    }
}

void* connect_service1(void* ctxPtr)
{
    LE_INFO("TestWakeSourceSampleApp");
    taf_mngdPm_ConnectService();
    int input = 1;
    char buffer[100];

    while(input != -1)
    {
        printf("Choose the TestRefreshAuthorizedWakeSources Test Case\n -1.Exit\n "
        "1:       Authorize 1       -> acquire multiple locks\n "
        "         Authorize 4 and 1 -> acquire locks with 4\n "
        "         Authorize only 4  -> mark 1 locks as ignored.\n "
        "         Authorize 2       -> Should release locks and mark ws as not acquired.\n "
        "\n"
        "2:       Authorize 1       -> acquire multiple locks\n "
        "        Authorize 4 and 1 -> acquire locks with 4\n "
        "        Authorize only 4  -> mark 1 locks as ignored.\n "
        "        Authorize 1 and 4 -> Locks of 1 should be unignored.\n "
        "\n"
        "3:       Authorize 1       -> acquire multiple locks\n "
        "        Authorize 4 and 1 -> acquire locks with 4\n "
        "        Authorize only 4  -> mark 1 locks as ignored.\n "
        "        Authorize 1 and 4 -> Locks of 1 should be unignored.\n "
        "        Authorize 2       -> Should release all locks and suspend and mark the locks as not acquired.\n "
        "        Authorize 1 and 4 -> Should display the old locks as not acquired.\n "
        "\n"
        "4:       Authorize 1       -> acquire multiple locks\n "
        "        Authorize 4 and 1 -> acquire locks with 4\n "
        "        Authorize 1 and 4 -> Locks of 1 should be unignored.\n "
        "        Authorize 2       -> Should release all locks and suspend and mark the locks as not acquired.\n "
        "        Authorize 1 and 4 -> Should display the old locks as not acquired.\n "
        "        Kill the clients  -> Should clear all the wake sources cache data and should release lock if any active.\n "
        "\n"
        "5 :      Authorize 1       -> acquire multiple locks\n "
        "        Authorize 2       -> Should release locks and mark ws as not acquired.\n "
        "\n"
        "6 :      Non authorized stay awake wakelock should not affect the state changes\n "
        "        Authorize 1       -> acquire multiple locks\n "
        "        Call from different client, CreateWakeSource()  -> returns wakesource reference of non authorized.\n "
        "        Acquire wakelock from other client -> Not increase the wsCount, since it is non authorized ws and marked as not acquired.\n "
        "        Authorize 2       -> Should release locks and mark ws as not acquired.\n "
        "\n"
        "7 :     Unauthorized stay awake\n " //UC2
        "        Stayawake system with unauthorize wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL\n"
        "        Should return LE_NOT_PERMITTED & keep the system in suspend if in suspend.\n "
        "        Unauthorized wakeup source: TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL state should be not acquired\n"
        "        PMVHAL notification: none\n"
        "\n"
        "8 :     Relax authorized wakeup source \n " //UC4
        "        Authorize & acquire wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL\n"
        "        Acquire unauthorized wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE \n "
        "        Relax system with the authorized wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL\n"
        "        Should set the authorized wakeup source: TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL state from acquired -> not acquired\n"
        "        Should send PMVHAL notification for authorized wakeup source: TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL as LOCK_RELEASED\n"
        "        Should mark the unauthorized wakeup source state from ignored -> not acquired and keep the system in suspend if in resume.\n "
        "\n"
        "9 :     Relax Unauthorized wakeup source \n "  //UC5
        "        Authorize & acquire wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL\n"
        "        Acquire unauthorized wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE \n "
        "        Relax system with the unauthorized wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE\n"
        "        PMVHAL notification: none\n"
        "        Should set the unauthorized wakeup source: TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE state from ignored -> not acquired and keep the system in resume if already in resume.\n "
        "\n"
        "10 :    Stay awake with unauthorized wakeup source\n "  //UC6
        "        Authorize & acquire wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL\n"
        "        Acquire unauthorized wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE \n "
        "        Stay awake system with the unauthorized wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE\n"
        "        PMVHAL notification: none\n"
        "        Should set the unauthorized wakeup source: TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE state from not acquired -> ignored and keep the system in resume if already in resume.\n"
        "\n"
        "11 :    Stay awake with unauthorized wakeup source\n " //UC7
        "        Authorize & acquire wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL\n"
        "        Unauthorize wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL\n"
        "        System state: Resume -> Suspend\n"
        "        WS state: ACQUIRED -> NOT_ACQUIRED\n"
        "        PMVHAL notification: LOCK_RELEASED\n"
        "\n"
        "12 :    Stay awake with unauthorized wakeup source\n "  //UC8
        "        Authorize & acquire wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL & TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE\n"
        "        Unauthorize wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE\n"
        "        System state: Resume -> Resume\n"
        "        WS state: ACQUIRED -> IGNORED\n"
        "        PMVHAL notification: LOCK_RELEASED\n"
        "\n"
        "13 :    Stay awake with unauthorized wakeup source\n"  //UC9
        "        Authorize & acquire wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL\n"
        "        Unauthorize & acquire wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE\n"
        "        Authorize wakeup source TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE\n"
        "        System state: Resume -> Resume\n"
        "        WS state: IGNORED -> ACQUIRED\n"
        "        PMVHAL notification: LOCK_ACQUIRED\n");

        if(fgets(buffer, sizeof(buffer), stdin))
            LE_INFO("Value read successfully");
        buffer[strcspn(buffer, "\n")] = '\0';
        input = atoi(buffer);
        LE_INFO("input: %d", input);
        if(input == -1)
        {
            exit(EXIT_SUCCESS);
        }
        if(input == 1)
        {
            authorizeAndAcquireLocks(1, true);
            authorizeAndAcquireLocks(5, true);
            authorizeAndAcquireLocks(4, false);
            authorizeAndAcquireLocks(2, false);
        }
        if(input == 2)
        {
            authorizeAndAcquireLocks(1, true);
            authorizeAndAcquireLocks(5, true);
            authorizeAndAcquireLocks(4, false);
            authorizeAndAcquireLocks(5, false);
        }
        if(input == 3)
        {
            authorizeAndAcquireLocks(1, true);
            authorizeAndAcquireLocks(5, true);
            authorizeAndAcquireLocks(4, false);
            authorizeAndAcquireLocks(5, false);
            authorizeAndAcquireLocks(2, false);
            authorizeAndAcquireLocks(5, false);
        }
        if(input == 4)
        {
            authorizeAndAcquireLocks(1, true);
            authorizeAndAcquireLocks(5, true);
            authorizeAndAcquireLocks(4, false);
            authorizeAndAcquireLocks(5, false);
            authorizeAndAcquireLocks(2, false);
            authorizeAndAcquireLocks(5, false);
            exit(EXIT_SUCCESS);
        }
        if(input == 5)
        {
            authorizeAndAcquireLocks(1, true);
            authorizeAndAcquireLocks(2, false);
        }
        if(input == 6)
        {
            authorizeAndAcquireLocks(1, true);
            int entry = 4;
            void *ptr = &entry;
            semRef1 = le_sem_Create("MngdIntTestApp", 0);
            threadRef1 = le_thread_Create("inttestapp",
                                        acquireWakeLock1, ptr);
            authorizeAndAcquireLocks(2, false);
        }
        if(input == 7)
        {
            //Do not change system state when Stayawake with unauthorized ws
            StayAwakeWithUnauthorizedWsShouldNotChangeSystemState();

        }
        if(input == 8)
        {
            //Relax authorized ws
            StayAwakeWithAuthorizedWsShouldResultInSuspend();

        }
        if(input == 9)
        {
            // Relax unauthorized ws
            UnauthorizedWsShouldNotResultInSuspend();

        }
        if(input == 10)
        {
            //Stayawake unauthorized ws
            StayAwakeWithUnauthorizedWsShouldNotResultInSuspend();

        }
        if(input == 11)
        {
            //Notify PM VHAL, when SAR changed from authorized to unauthorized - In case of only one SAR is authorized
            NotifyPmVhalForUnauthorizedSarAndShouldResultInSuspend();
        }
        if(input == 12)
        {
            //Notify PM VHAL, when SAR changed from authorized to unauthorized - In case of more than one SARs are authorized
           NotifyPmVhalForUnauthorizedSarAndShouldNotResultInSuspend();
        }
        if(input == 13)
        {
            //Notify PM VHAL, when SAR changed from unauthorized to authorized
            NotifyPmVhalForAuthorizedSarAndShouldNotResultInSuspend();
        }
    }
    le_sem_Post(semRef);
    le_event_RunLoop();
}

void TestAuthorizeWakeSourceCases()
{
    semRef = le_sem_Create("tafMngdIntTestApp", 0);
    LE_INFO("createapp1 start");
        threadRef = le_thread_Create("inttestapp",
                                    connect_service1, NULL);
        le_thread_Start(threadRef);
        le_sem_Wait(semRef);
}

void TestNonAuthorizedStayAwake()
{
    uint8_t pmNodeId = 0;
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(2);
    if(res == LE_OK)
        printf("taf_mngdPm_AuthorizeStayAwakeReason for STAY_AWAKE_REASON_ECALL_ACTIVE\n");
    wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
            TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if(wsRef)
        printf("CreateWakeupSource for TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL\n");
    if(wsRef != NULL) {
        res = taf_mngdPm_StayAwake(wsRef);
        if(res == LE_OK) {
            printf("'Resumed sysytem'\n");
         }
    }
    le_result_t result =  LE_FAULT;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE", pmNodeId);
    LE_INFO("GracefulSysSuspend without wake source");
    result = taf_mngdPm_SetNodeTargetedPowerMode(pmNodeId,
            TAF_MNGDPM_SUSPEND);
    if(result == LE_OK)
    {
        LE_INFO("----GracefulSysSuspend success----");
        exit(EXIT_SUCCESS);
    }
    else
    {
        LE_ERROR("GracefulSysSuspend request failed");
        exit(EXIT_FAILURE);
    }
}

static void ForcedSystemShutdownAndResume() //TELAF-3169 [Conti] 07743357 MPMS State Transition Issue: Stay Awake Request During Shutdown
{
    LE_INFO("----ForcedSystemShutdown test----");
    le_result_t result;
    uint8_t pmNodeId = 0;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE", pmNodeId);

    result = taf_mngdPm_ShutdownReqAsync(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
            ForcedSystemShutdownCallBack, NULL, TAF_MNGDPM_SHUTDOWN_REASON_NORMAL);

    if(result == LE_OK)
    {
         LE_INFO("----ForcedSystemShutdown success----");
         if(wsRef == NULL)
             wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
                     TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
         if(wsRef)
             LE_INFO("CreateWakeupSource for TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL\n");

         if(wsRef != NULL) {
             result = taf_mngdPm_StayAwake(wsRef);
             if(result == LE_OK) {
                 printf("'Resumed sysytem'\n");
              }
              else {
                  LE_INFO("Failed to acquire Wake source");
              }
         }
         else {
             LE_ERROR("Failed to create wakeup source!");
         }
    }
    else
    {
        LE_ERROR("ForcedSystemShutdown request failed");
        exit(EXIT_FAILURE);
    }
}

void DeleteWsIfNotAcquired()
{
    LE_INFO("DeleteWsIfNotAcquired");
    int reason = TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL;
    taf_mngdPm_wsRef_t wsRefAuthorized = NULL;
    // Authorized the reason for bit0
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
    if(res == LE_OK) {
        printf("'AuthorizeStayAwakeReason for bitmask %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
        wsRefAuthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized) {
            printf("Created wakeupsource ref for reason %d\n", reason);
            if(wsRefAuthorized != NULL) {
                res = taf_mngdPm_StayAwake(wsRefAuthorized);
                if(res == LE_OK) {
                    printf("'Resumed system with wsRefAuthorized'\n");
                    res = taf_mngdPm_Relax(wsRefAuthorized);
                    if(res == LE_OK) {
                        printf("'Suspended system with wsRefAuthorized'\n");
                        res = taf_mngdPm_DeleteWakeupSource(wsRefAuthorized);
                        if(res == LE_OK) {
                            printf("'Deleted not acquired wakesource'\n");
                            exit(EXIT_SUCCESS);
                        }
                    }
                }
            }
        }
        else{
            printf("Failed to create wakeupsource ref for authorized reason %d\n", reason);
            exit(EXIT_FAILURE);
        }
    }

}

void ShouldNotDeleteWsIfAcquired()
{
    LE_INFO("ShouldNotDeleteWsIfAcquired");
    int reason = TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL;
    taf_mngdPm_wsRef_t wsRefAuthorized = NULL;
    // Authorized the reason for bit0
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
    if(res == LE_OK) {
        printf("'AuthorizeStayAwakeReason for bitmask %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
        wsRefAuthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized) {
            printf("Created wakeupsource ref for reason %d\n", reason);
            if(wsRefAuthorized != NULL) {
                res = taf_mngdPm_StayAwake(wsRefAuthorized);
                if(res == LE_OK) {
                    printf("'Resumed system with wsRefAuthorized'\n");
                    res = taf_mngdPm_DeleteWakeupSource(wsRefAuthorized);
                    if(res == LE_OK) {
                        printf("'Error: deleted acquired wakesource'\n");
                        exit(EXIT_FAILURE);
                    }
                    else if(res == LE_NOT_PERMITTED)
                    {
                        printf("'Deletion of ws before releasing it, is not permitted '\n");
                        exit(EXIT_SUCCESS);
                    }
                }
            }
        }
        else{
            printf("Failed to create wakeupsource ref for authorized reason %d\n", reason);
            exit(EXIT_FAILURE);
        }
    }
}

void WakeupVehicleWhenReleasingWsTest()
{
    LE_INFO("----WakeupVehicleWhenReleasingWsTest----");
    // CreateWS test 0
    int reason = TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL;
    taf_mngdPm_wsRef_t wsRefAuthorized = NULL;
    taf_mngdPm_wsRef_t wsRefAuthorized1 = NULL;
    // Authorized the reason for bit0
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
    if(res == LE_OK) {
        printf("'AuthorizeStayAwakeReason for bitmask %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
        wsRefAuthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized) {
            printf("Created wakeupsource ref for reason %d\n", reason);
            if(wsRefAuthorized != NULL) {
                // StayAwake test
                res = taf_mngdPm_StayAwake(wsRefAuthorized);
                if(res == LE_OK) {
                    printf("'Resumed system with wsRefAuthorized'\n");
                }
            }
        }
    }
    else{
        printf("Failed to create wakeupsource ref for authorized reason %d\n", reason);
        exit(EXIT_FAILURE);
    }

    reason = TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE;
    res = taf_mngdPm_AuthorizeStayAwakeReason(3);
    if(res == LE_OK) {
        printf("'AuthorizeStayAwakeReason for bitmask %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_ECALL_ACTIVE);
        // CreateWS test1 0
        wsRefAuthorized1 = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized1) {
            printf("Created wakeupsource ref for reason %d\n", reason);
            if(wsRefAuthorized1 != NULL) {
                // StayAwake test1
                res = taf_mngdPm_StayAwake(wsRefAuthorized1);
                if(res == LE_OK) {
                    printf("'Resumed system with wsRefAuthorized1'\n");
                    res = taf_mngdPm_Relax(wsRefAuthorized1);
                    if(res == LE_OK) {
                        printf("'Suspended system with wsRefAuthorized1'\n");
                        // VehicleWakeup 0
                        int status = WakeupVehicle();
                        LE_INFO("'WakeupVehicle status:%d'",status);
                        exit(status);
                    }
                    else{
                        LE_INFO("Failed to resume system with wsRefAuthorized1");
                        exit(EXIT_FAILURE);
                    }
                }
                else{
                    LE_INFO("Failed to resume system with wsRefAuthorized1");
                    exit(EXIT_FAILURE);
                }
            }
        }
        else{
            LE_INFO("Failed to create wakeupsource ref for authorized reason %d", reason);
            exit(EXIT_FAILURE);
        }
    }
}

void WakeupVehicleWhenWakingUpTest()
{
    LE_INFO("----WakeupVehicleWhenWakingUpTest----");
    // CreateWS test 0
    int reason = TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL;
    taf_mngdPm_wsRef_t wsRefAuthorized = NULL;
    // Authorized the reason for bit0
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
    if(res == LE_OK) {
        printf("'AuthorizeStayAwakeReason for bitmask %d is set'\n", TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
        wsRefAuthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized) {
            printf("Created wakeupsource ref for reason %d\n", reason);
        }

        if(wsRefAuthorized != NULL) {
            res = taf_mngdPm_StayAwake(wsRefAuthorized);
            if(res == LE_OK) {
                printf("'Resumed system with wsRefAuthorized'\n");
                int status = WakeupVehicle();
                LE_INFO("'WakeupVehicle status:%d'",status);
                exit(status);
            }
        }
    }
    else{
        printf("Failed to create wakeupsource ref for authorized reason %d\n", reason);
        exit(EXIT_FAILURE);
    }
}

void ShouldNotDeleteWsIfIgnored()
{
    LE_INFO("ShouldNotDeleteWsIfIgnored");
    int reason = TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL;
    taf_mngdPm_wsRef_t wsRefUnauthorized = NULL;
    le_result_t res = LE_FAULT;
    // Authorized the reason for bit0
    res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);
    if(res == LE_OK) {
        taf_mngdPm_wsRef_t wsRefAuthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized) {
            printf("Created wakeupsource ref for wsRefUnauthorized reason %d\n", reason);
            if(wsRefAuthorized != NULL) {
                res = taf_mngdPm_StayAwake(wsRefAuthorized);
                if(res == LE_OK) {
                    printf("'Resumed system with wsRefAuthorized'\n");
                }
            }
        }
        else{
            printf("Failed to create wakeupsource ref for authorized reason %d\n", reason);
            exit(EXIT_FAILURE);
        }
    }

    reason = TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE;
    wsRefUnauthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if(wsRefUnauthorized) {
        printf("Created wakeupsource ref for wsRefUnauthorized reason %d\n", reason);
        if(wsRefUnauthorized != NULL) {
            res = taf_mngdPm_StayAwake(wsRefUnauthorized);
            if(res == LE_OK) {
                res = taf_mngdPm_DeleteWakeupSource(wsRefUnauthorized);
                if(res == LE_OK) {
                    printf("'Error: deleted ignored wakesource'\n");
                    exit(EXIT_FAILURE);
                }
                else if(res == LE_NOT_PERMITTED)
                {
                    printf("'Deletion of ws before releasing it, is not permitted '\n");
                    exit(EXIT_SUCCESS);
                }
            }
        }
    }
    else{
        printf("Failed to create wakeupsource ref for unauthorized reason %d\n", reason);
        exit(EXIT_FAILURE);
    }


}

void NodePowerStateChangeHandler(
    uint8_t pmNodeId,
    taf_mngdPm_nodePowerStateRef_t nodePowerStateRef,
    taf_mngdPm_NodePowerState_t state,
    void *contextPtr)
{
    if (state == TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE)
    {
        printf("Received SUSPEND state (%d). "
               "Intentionally NOT sending ACK to test timeout.\\n",
               state);
        // IMPORTANT: do NOT call taf_mngdPm_SendNodePowerStateChangeAck here
        // for this test, so that the MPMS timer expires.
    }
}

void TestPmvhalStayAwakeAfterMpmsSuspendTrigger()
{
    LE_INFO("TestPmvhalStayAwakeAfterMpmsSuspendTrigger");
    int reason = TAF_MNGDPM_STAY_AWAKE_REASON_VEH_NETWORK;
    le_result_t res = LE_FAULT;
    taf_mngdPm_NodePowerStateChangeHandlerRef_t ref =
        taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandler,
        NULL, 0, TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE);
    if(ref)
    {
        LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE");
    }

    // Authorized the reason for bit0
    res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_VEH_NETWORK);
    if(res == LE_OK) {
        taf_mngdPm_wsRef_t wsRefAuthorized = taf_mngdPm_CreateWakeupSource(reason, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
        if(wsRefAuthorized) {
            printf("Created wakeupsource ref for wsRefUnauthorized reason %d\n", reason);
            if(wsRefAuthorized != NULL) {
                res = taf_mngdPm_StayAwake(wsRefAuthorized);
                if(res == LE_OK) {
                    printf("'Resumed system with wsRefAuthorized'\n");
                    res = taf_mngdPm_Relax(wsRefAuthorized);
                    if(res == LE_OK) {
                        printf("'Suspended system with wsRefAuthorized'\n");
                    }
                }
            }
        }
        else{
            printf("Failed to create wakeupsource ref for authorized reason %d\n", reason);
            exit(EXIT_FAILURE);
        }
    }
}

void NotifyVhalOnClientDisconnectionForReleaseWS()
{
    LE_INFO("--NotifyVhalOnClientDisconnectionForReleaseWS--");
    le_result_t res = LE_FAULT;

    res = taf_mngdPm_AuthorizeStayAwakeReason(3);
    if(res != LE_OK) {
     LE_INFO("Failed to AuthorizeStayAwakeReason for bitmask %d \n", 3);
    }

    taf_mngdPm_wsRef_t wsRefAuthorized1 = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if(wsRefAuthorized1) {
        printf("Created wakeupsource ref for wsRefAuthorized1 reason %d\n", TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE);
        if(wsRefAuthorized1 != NULL) {
            res = taf_mngdPm_StayAwake(wsRefAuthorized1);
            if(res == LE_OK) {
                printf("'Resumed system with wsRefAuthorized1'\n");
            }
        }
    }
    else{
        printf("Failed to create wakeupsource ref for authorized reason %d\n", TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE);
        exit(EXIT_FAILURE);
    }

    taf_mngdPm_wsRef_t wsRefAuthorized = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if(wsRefAuthorized) {
        printf("Created wakeupsource ref for wsRefAuthorized reason %d\n", TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL);
        if(wsRefAuthorized != NULL) {
            res = taf_mngdPm_StayAwake(wsRefAuthorized);
            if(res != LE_OK) {
                printf("'Failed to resumed system with wsRefAuthorized'\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    else{
        printf("Failed to create wakeupsource ref for authorized reason %d\n", TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL);
        exit(EXIT_FAILURE);
    }

    res = taf_mngdPm_Relax(wsRefAuthorized);
    if(res == LE_OK) {
        printf("'Suspended system with wsRefAuthorized'\n");
        exit(EXIT_SUCCESS);
    }
}

static void TestGetCurrentPowerStateFromPrimaryNad()
{
    taf_mngdPm_NodePowerState_t pwrState = TAF_MNGDPM_NODE_STATE_RESUME;

    le_result_t result =
        taf_mngdPm_GetNodePowerState(
            0, &pwrState);

    if (result != LE_OK)
    {
        printf("/get.current.state -> error: %s\n",
               LE_RESULT_TXT(result));
        exit(EXIT_FAILURE);
    }

    switch (pwrState)
    {
        case TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE:
        printf("/get.current.state -> SHUTDOWN\n");
        break;

        case TAF_MNGDPM_NODE_STATE_RESTART_PREPARE:
        printf("/get.current.state -> RESTART\n");
        break;

        case TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE:
        printf("/get.current.state -> SUSPEND\n");
        break;

        case TAF_MNGDPM_NODE_STATE_RESUME:
        printf("/get.current.state -> RESUME\n");
        break;
    }

    exit(EXIT_SUCCESS);
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
        else if(strcmp(testType, "RestartSystemWithReason") == 0)
        {
            RestartSystemWithReason();
        }
        else if(strcmp(testType, "RebootSystemWithReason") == 0)
        {
            RebootSystemWithReason();
        }
        else if(strcmp(testType, "KeepAwakeThenRestartSystem") == 0)
        {
            status = KeepAwakeThenRestartSystem();
            exit(status);
        }
        else if(strcmp(testType, "ForcedSystemShutdownWithReason") == 0)
        {
            ForcedSystemShutdownWithReason();
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
            AddInfoReportHandler(&BubCallBack);
        }
        else if(strcmp(testType, "ForcedSystemShutdownAndSuspend") == 0)
        {
            ForcedSystemShutdownAndSuspend();
        }
        else if(strcmp(testType, "TestAuthorizedResumeandSuspend") == 0)
        {
            TestWakeSourceCases();
        }
        else if(strcmp(testType, "TestNodeResumeandSuspend") == 0)
        {
            TestNodeWakeSourceCases();
        }
        else if(strcmp(testType, "TestBubCases") == 0)
        {
            TestBubCases();
        }
        else if(strcmp(testType, "CreateMultipleClients") == 0)
        {
            CreateMultipleClients();
        }
        else if(strcmp(testType, "AllowWakingupDuringSuspending") == 0)
        {
            AllowWakingupDuringSuspending();
        }
        else if(strcmp(testType, "AllowAuthorizedWakingupDuringSuspending") == 0)
        {
            AllowAuthorizedWakingupDuringSuspending();
        }
        else if(strcmp(testType, "ShouldRejectUnauthorizedWakingupDuringSuspending") == 0)
        {
            ShouldRejectUnauthorizedWakingupDuringSuspending();
        }
        else if(strcmp(testType, "TestNonAuthorizedStayAwake") == 0)
        {
            TestNonAuthorizedStayAwake();
        }
        else if(strcmp(testType, "TestClearUnAuthorizedWakeSource") == 0)
        {
            TestClearUnAuthorizedWakeSource();
        }
        else if(strcmp(testType, "MultiClntForcedSysShutdown") == 0)
        {
            MultiClntForcedSysShutdown();
        }
        else if(strcmp(testType, "MultiClntRestartSystem") == 0)
        {
            MultiClntRestartSystem();
        }
        else if(strcmp(testType, "MultiClntWakeupVehicle") == 0)
        {
            MultiClntWakeupVehicle();
        }
        else if(strcmp(testType, "GracefulSysSuspendWithAckType") == 0)
        {
            if(testPar) {
                GracefulSysSuspendWithAckType(testPar);
            }
            else {
                printf("Enter PowerStateChangeAck");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "TestRefreshAuthorizedWsCases") == 0)
        {
            TestAuthorizeWakeSourceCases();
        }
        else if(strcmp(testType, "ForcedSystemShutdownAndResume") == 0)
        {
            ForcedSystemShutdownAndResume();
        }
        else if(strcmp(testType, "DeleteWsIfNotAcquired") == 0)
        {
            DeleteWsIfNotAcquired();
        }
        else if(strcmp(testType, "ShouldNotDeleteWsIfAcquired") == 0)
        {
            ShouldNotDeleteWsIfAcquired();
        }
        else if(strcmp(testType, "WakeupVehicleWhenReleasingWsTest") == 0)
        {
            WakeupVehicleWhenReleasingWsTest();
        }
        else if(strcmp(testType, "WakeupVehicleWhenWakingUpTest") == 0)
        {
            WakeupVehicleWhenWakingUpTest();
        }
        else if(strcmp(testType, "ShouldNotDeleteWsIfIgnored") == 0)
        {
            ShouldNotDeleteWsIfIgnored();
        }
        else if(strcmp(testType, "NotifyVhalOnClientDisconnectionForReleaseWS") == 0)
        {
            NotifyVhalOnClientDisconnectionForReleaseWS();
        }
        else if(strcmp(testType, "TestPmvhalStayAwakeAfterMpmsSuspendTrigger") == 0)
        {
            TestPmvhalStayAwakeAfterMpmsSuspendTrigger();
        }
        else if(strcmp(testType, "get.current.state") == 0)
        {
            TestGetCurrentPowerStateFromPrimaryNad();
        }
        else
        {
            LE_ERROR("Error command");
            exit(EXIT_FAILURE);
        }
    }
}
