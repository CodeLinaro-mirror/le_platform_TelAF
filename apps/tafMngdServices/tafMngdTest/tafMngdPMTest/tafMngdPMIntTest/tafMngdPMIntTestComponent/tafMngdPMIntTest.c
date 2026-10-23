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


taf_mngdPm_wsNodeRef_t wsNodeRef = NULL;
taf_mngdPm_wsNodeRef_t wsNodeRef1 = NULL;

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
int stateChangeAck = 0;
#define VEHICHLE_WAKEUP_REASON_DEFAULT 0
#define AUTHORIZE_ALL_STAY_AWAKE_REASON 0xFFFFFFFF

// -------- Timeouts --------
static const le_clk_Time_t REG_WAIT  = { .sec = 1, .usec = 0 };
static const le_clk_Time_t SNAP_WAIT = { .sec = 5, .usec = 0 };

// --- Shared state ---
static le_thread_Ref_t nodePwStateLoopThread = NULL;
static le_sem_Ref_t    nodePwStateRegSem     = NULL;  // signaled after registration
static le_sem_Ref_t    nodePwStateSnapSem    = NULL;  // signaled on immediate snapshot
static le_sem_Ref_t    nodePwStatePrepSem    = NULL;  // signaled on prepare notification

// Handler reference for cleanup
static taf_mngdPm_NodePowerStateChangeHandlerRef_t nodePwStateHandlerRef = NULL;

static volatile uint8_t  pwNodeId    = 0;
static volatile taf_mngdPm_NodePowerStateChangeBitMask_t nodePwStateMask = 0;

static volatile bool     nodePwStateGotSnapshot = false;
static volatile uint8_t  nodePwStateSnapNode    = 0;
static volatile taf_mngdPm_NodePowerState_t nodePwSnapState = 0;

static volatile bool     nodePwStateGotPrepare = false;
static volatile taf_mngdPm_NodePowerState_t nodePwPrepState = 0;

// -------- Bitmask helpers --------
static const taf_mngdPm_NodePowerStateChangeBitMask_t MASK_ALL =
      TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME
    | TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE
    | TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE
    | TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE;

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
        "------------To Test immediate current node power state notification to the registered client-----------\n"
        "-------- Provide ACK type: 1=ACK, -1=NACK, 2=NO_RESP, 3=ACK_AFTER_TIMEOUT when prompted--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ImmediateNotifyClientOnCurrNodePwStateOnRegister <NODE_ID>\n"
        "------------To Test GracefulSysShutdown for node pw state change notification-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- TestGracefulSysShutdownForNodePwStateChange <NODE_ID>\n"
        "------------To test WsDump: primary process-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- WsDumpClient01\n"
        "------------To test WsDump: helper process (different PID)-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTestHelper -- WsDumpClient02\n"
        "------------To Test PMVHAL stayawake after while suspending through MPMS-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- TestPmvhalStayAwakeAfterMpmsSuspendTrigger\n"
        "------------To register a client for power state notifications which does not acknowledges-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- RegisterClientForPowerStateNotificationWithoutAcknowledgement <NODE_POWER_STATE_CHANGE_NOTIFICATION_BITMASK>\n"
        "------------To Test node power state handler registration with state mask 0-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- TestAddNodePowerStateChangeHandlerStateMask\n"
        "------------Shutdown/restart VHAL prepare pending window tests-----------\n"
        "-------- The VHAL prepare response is selected on the test driver, not by these tests:--------\n"
        "--------   pmVHalDrv.so set prepare_reason timeout    (no callback -> 10s prepare timeout)--------\n"
        "--------   pmVHalDrv.so set prepare_reason not_ready  (synchronous NACK)--------\n"
        "--------   pmDriver v1 equivalent: echo 2 / 1 > /data/le_fs/ack --------\n"
        "-------- StayAwake in window, closed by timeout (needs prepare_reason=timeout)--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- PendingWinStayAwake <NODE_ID>\n"
        "-------- Relax in window driving wsCount to 0 (needs prepare_reason=timeout)--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- PendingWinRelax <NODE_ID>\n"
        "-------- AuthorizeStayAwakeReason churn in window (needs prepare_reason=timeout)--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- PendingWinAuthorize <NODE_ID>\n"
        "-------- StayAwake in the RESTARTING window (needs prepare_reason=timeout)--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- PendingWinRestart <NODE_ID>\n"
        "-------- Shutdown window closed by a VHAL NACK (needs prepare_reason=not_ready)--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- PendingWinNackShutdown <NODE_ID>\n"
        "-------- Restart window closed by a VHAL NACK (needs prepare_reason=not_ready)--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- PendingWinNackRestart <NODE_ID>\n");
}


static char* StateToString(taf_mngdPm_NodePowerStateChangeBitMask_t state)
{
    switch(state)
    {
        case TAF_MNGDPM_NODE_STATE_RESUME:           return "TAF_MNGDPM_NODE_STATE_RESUME";
        case TAF_MNGDPM_NODE_STATE_RESTART_PREPARE:  return "TAF_MNGDPM_NODE_STATE_RESTART_PREPARE";
        case TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE: return "TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE";
        case TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE:  return "TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE";
        default:                                     return "Unknown";
    }
}
static uint32_t StateToBit(taf_mngdPm_NodePowerState_t st)
{
    switch (st)
    {
        case TAF_MNGDPM_NODE_STATE_RESUME:           return TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME;
        case TAF_MNGDPM_NODE_STATE_RESTART_PREPARE:  return TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE;
        case TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE: return TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE;
        case TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE:  return TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE;
        default:                                     return 0;
    }
}

static bool WaitSemT(le_sem_Ref_t sem, le_clk_Time_t timeout, const char* what)
{
    if (le_sem_WaitWithTimeOut(sem, timeout) == LE_TIMEOUT)
    {
        LE_ERROR("Timeout waiting for %s (sec=%d usec=%d)", what, (int)timeout.sec, (int)timeout.usec);
        return false;
    }
    return true;
}

static void CleanupPwStateClient(void)
{
    if (nodePwStateHandlerRef)
    {
        taf_mngdPm_RemoveNodePowerStateChangeHandler(nodePwStateHandlerRef);
        nodePwStateHandlerRef = NULL;
    }
    if (nodePwStateRegSem)  { le_sem_Delete(nodePwStateRegSem);  nodePwStateRegSem  = NULL; }
    if (nodePwStateSnapSem) { le_sem_Delete(nodePwStateSnapSem); nodePwStateSnapSem = NULL; }
    if (nodePwStatePrepSem) { le_sem_Delete(nodePwStatePrepSem); nodePwStatePrepSem = NULL; }
}

static inline bool IsPrepareState(taf_mngdPm_NodePowerState_t s)
{
    return (s == TAF_MNGDPM_NODE_STATE_RESTART_PREPARE)
        || (s == TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE)
        || (s == TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE);
}

static void SendAckPolicy(uint8_t pmNodeId,
                          taf_mngdPm_nodePowerStateRef_t ref)
{
    if (stateChangeAck == 2) {
        LE_INFO("NO_RESP: not sending ACK for node=%u", pmNodeId);
        return;
    }

    if (stateChangeAck == 3) {
        le_sem_Ref_t delaySem = le_sem_Create("pmAckDelaySem", 0);
        (void)le_sem_WaitWithTimeOut(delaySem, AckTimeout);
        le_sem_Delete(delaySem);
        (void)taf_mngdPm_SendNodePowerStateChangeAck(pmNodeId, ref, TAF_MNGDPM_CLIENT_READY);
        LE_INFO("ACK_AFTER_TIMEOUT: READY sent for node=%u", pmNodeId);
        return;
    }

    int ackVal = (stateChangeAck == -1) ? TAF_MNGDPM_CLIENT_NOT_READY : TAF_MNGDPM_CLIENT_READY;
    (void)taf_mngdPm_SendNodePowerStateChangeAck(pmNodeId, ref, ackVal);
    LE_INFO("ACK sent: %d for node=%u", ackVal, pmNodeId);
}

static void NodePwStateCb(uint8_t pmNodeId,
                      taf_mngdPm_nodePowerStateRef_t ref,
                      taf_mngdPm_NodePowerState_t state,
                      void* ctx)
{
    LE_INFO("--- NodePwStateCb ---");
    (void)ctx;

    uint32_t bit = StateToBit(state);
    if ((nodePwStateMask & bit) == 0) return;

    if (IsPrepareState(state))
    {
        nodePwStateGotPrepare = true;
        nodePwPrepState = state;
        if (nodePwStatePrepSem) le_sem_Post(nodePwStatePrepSem);
        LE_INFO("Prepare node power state observed: node=%u state=%d", pmNodeId, state);

        SendAckPolicy(pmNodeId, ref);

        if (nodePwStateSnapSem) le_sem_Post(nodePwStateSnapSem);

        return;
    }

    if (state == TAF_MNGDPM_NODE_STATE_RESUME)
    {
        nodePwStateGotSnapshot = true;
        nodePwSnapState = state;
        nodePwStateSnapNode = pmNodeId;
        if (nodePwStateSnapSem) le_sem_Post(nodePwStateSnapSem);
        LE_INFO("Immediate snapshot observed: node=%u state=%d", pmNodeId, state);
        return;
    }

    LE_INFO("Unhandled state=%d for mask=0x%x (no ACK sent)", state, nodePwStateMask);
}

static void* NodePwStateClientThread(void* cbPtr)
{
    void (*cb)(uint8_t, taf_mngdPm_nodePowerStateRef_t, taf_mngdPm_NodePowerState_t, void*)
        = (void(*)(uint8_t, taf_mngdPm_nodePowerStateRef_t, taf_mngdPm_NodePowerState_t, void*))cbPtr;

    taf_mngdPm_ConnectService();

    nodePwStateHandlerRef = taf_mngdPm_AddNodePowerStateChangeHandler(cb, NULL, pwNodeId, nodePwStateMask);
    if (!nodePwStateHandlerRef)
    {
        LE_ERROR("AddNodePowerStateChangeHandler failed (mask=0x%x)", nodePwStateMask);
    }
    else
    {
        LE_INFO("AddNodePowerStateChangeHandler OK: node=%u mask=0x%x", pwNodeId, nodePwStateMask);
    }
    if (nodePwStateRegSem) le_sem_Post(nodePwStateRegSem);

    le_event_RunLoop();
    return NULL;
}

void ImmediateNotifyClientOnCurrNodePwStateOnRegister(uint8_t pmNodeId)
{
    LE_INFO("ImmediateNotifyClientOnCurrNodePwStateOnRegister: node=%u", pmNodeId);

    char ackBuf[32] = {0};
    printf("Enter ACK type (1/0=ACK, -1=NACK, 2=NO_RESP, 3=ACK_AFTER_TIMEOUT): ");
    if (fgets(ackBuf, sizeof(ackBuf), stdin)) {
        stateChangeAck = atoi(ackBuf);
        LE_INFO("User-selected stateChangeAck=%d", stateChangeAck);
    }

    pwNodeId        = pmNodeId;
    nodePwStateMask = MASK_ALL;

    nodePwStateGotSnapshot = false;
    nodePwStateGotPrepare  = false;
    nodePwSnapState        = 0;
    nodePwPrepState        = 0;

    nodePwStateRegSem   = le_sem_Create("pmRegSem", 0);
    nodePwStateSnapSem  = le_sem_Create("pmSnapSem", 0);
    nodePwStatePrepSem  = le_sem_Create("pmPrepSem", 0);

    nodePwStateLoopThread = le_thread_Create("PmClientThread", NodePwStateClientThread,
        (void*)NodePwStateCb);
    le_thread_Start(nodePwStateLoopThread);

    if (!WaitSemT(nodePwStateRegSem, REG_WAIT, "handler registration")) {
        CleanupPwStateClient();
        exit(EXIT_FAILURE);
    }

    if (!WaitSemT(nodePwStateSnapSem, SNAP_WAIT, "immediate snapshot"))
    {
        LE_ERROR("No immediate node pw state observed(node=%u mask=0x%x)",
            pmNodeId, nodePwStateMask);
        CleanupPwStateClient();
        exit(EXIT_FAILURE);
    }

    LE_INFO("Immediate node pw state change delivered (node=%u state=%d)", nodePwStateSnapNode, nodePwSnapState);
    CleanupPwStateClient();
    exit(EXIT_SUCCESS);
}

void TestGracefulSysShutdownForNodePwStateChange(uint8_t pmNodeId)
{
    LE_INFO("----TestGracefulSysShutdownForNodePwStateChange----");

    char ackBuf[32] = {0};
    printf("Enter ACK type (1=ACK, -1=NACK, 2=NO_RESP, 3=ACK_AFTER_TIMEOUT): ");
    if (fgets(ackBuf, sizeof(ackBuf), stdin)) {
        stateChangeAck = atoi(ackBuf);
        LE_INFO("User-selected stateChangeAck=%d", stateChangeAck);
    }

    pwNodeId = pmNodeId;
    nodePwStateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME |
                      TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE;

    nodePwStateGotPrepare = false;
    nodePwPrepState = 0;

    nodePwStateRegSem  = le_sem_Create("pmRegSem", 0);
    nodePwStatePrepSem = le_sem_Create("pmPrepSem", 0);
    nodePwStateSnapSem = NULL;

    nodePwStateLoopThread = le_thread_Create("PmClientThread", NodePwStateClientThread, (void*)NodePwStateCb);
    le_thread_Start(nodePwStateLoopThread);

    if (!WaitSemT(nodePwStateRegSem, REG_WAIT, "handler registration")) {
        CleanupPwStateClient();
        exit(EXIT_FAILURE);
    }

    LE_INFO("GracefulSysShutdown without wake source");
    le_result_t res = taf_mngdPm_SetNodeTargetedPowerMode(pmNodeId, TAF_MNGDPM_SHUTDOWN);
    if (res != LE_OK) {
        LE_ERROR("GracefulSysShutdown request failed");
        CleanupPwStateClient();
        exit(EXIT_FAILURE);
    }
    LE_INFO("GracefulSysShutdown requested, waiting for prepare...");

    if (!WaitSemT(nodePwStatePrepSem, SNAP_WAIT, "shutdown prepare")) {
        LE_ERROR("No shutdown prepare observed (node=%u mask=0x%x)", pmNodeId, nodePwStateMask);
        CleanupPwStateClient();
        exit(EXIT_FAILURE);
    }

    LE_INFO("Observed prepare state (node=%u state=%d)", pmNodeId, nodePwPrepState);
    CleanupPwStateClient();
    exit(EXIT_SUCCESS);
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

void NodePowerStateChangeHandlerWithoutAckCB(
     uint8_t pmNodeId,
     taf_mngdPm_nodePowerStateRef_t nodePowerStateRef,
     taf_mngdPm_NodePowerState_t state,
	 void *contextPtr)
{
    LE_INFO("NodePowerStateChangeHandlerWithoutAckCB called, no acknowledment sent for state %s", StateToString(state));
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
        LE_ERROR("----ForcedSystemShutdown failed----");
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
        wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);

     if(wsRef != NULL) {
         LE_INFO("NewNodeWakeupSource ref is created for APP_STAYAWAKE");
         if (wsRef != NULL) {
             result = taf_mngdPm_StayAwake(wsRef);
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
             result = taf_mngdPm_Relax(wsRef);
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
        wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
     if(wsRef != NULL) {
         LE_INFO("NewNodeWakeupSource ref is created for APP_STAYAWAKE");
         if (wsRef != NULL) {
             result = taf_mngdPm_StayAwake(wsRef);
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

             result = taf_mngdPm_Relax(wsRef);
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

    LE_INFO("NewNodeWakeupSource");
    wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if(wsRef != NULL) {
        LE_INFO("NewNodeWakeupSource ref is created");
        res = taf_mngdPm_StayAwake(wsRef);
        if(res == LE_OK) {
            LE_INFO("Wake up sysytem");
        }
    }

    taf_mngdPm_wsRef_t wsRefSms0 = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if (wsRefSms0 != NULL)
    {
        LE_INFO("NewNodeWakeupSource ref is created for SMS 0");
        res = taf_mngdPm_StayAwake(wsRefSms0);
        if(res == LE_OK) {
            LE_INFO("Wake up sysytem with wakeuptype SMS 0");
        }
        res = taf_mngdPm_Relax(wsRefSms0);
        if(res == LE_OK) {
            LE_INFO("Relax sysytem with wakeuptype SMS 0");
        }
    }
    taf_mngdPm_wsRef_t wsRefSms1 = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if (wsRefSms1 != NULL)
    {
        LE_INFO("NewNodeWakeupSource ref is created for SMS 1");
        res = taf_mngdPm_StayAwake(wsRefSms1);
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
         wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
     if(wsRef != NULL) {
         LE_INFO("NewNodeWakeupSource ref is created");
         if (wsRef != NULL) {
             result = taf_mngdPm_StayAwake(wsRef);
             if(result == LE_OK) {
                 LE_INFO("Resumed sysytem");
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
        result = taf_mngdPm_Relax(wsRef);
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

     if(wsRef == NULL)
         wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
     if(wsRef != NULL) {
         LE_INFO("NewNodeWakeupSource ref is created for APP_STAYAWAKE");
         if (wsRef != NULL) {
             result = taf_mngdPm_StayAwake(wsRef);
             if(result == LE_OK) {
                 LE_INFO("Resumed sysytem with wakeuptype APP_STAYAWAKE");
             }
             else {
                 LE_INFO("Failed to acquire Wake source");
                 exit(EXIT_FAILURE);
             }
             result = taf_mngdPm_Relax(wsRef);
             if(result == LE_OK) {
                 LE_INFO("suspended sysytem with wakeuptype APP_STAYAWAKE");
             result = taf_mngdPm_StayAwake(wsRef);
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
            int NODE_ID = atoi(NodeId);
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
                wsNodeRef = taf_mngdPm_CreateNodeWakeupSource(NODE_ID, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
                if(wsNodeRef)
            printf("NewNodeWakeupSource wakeuptype is %d for NODE_ID %d\n", wakeuptype, NODE_ID);
            }
            else if(NODE_ID == 1) {
                    wsNodeRef1 = taf_mngdPm_CreateNodeWakeupSource(NODE_ID, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
                    if(wsNodeRef1)
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
            int NODE_ID = atoi(StayAwakeNode);
            if(NODE_ID == -1)
                continue;
            if(NODE_ID == 0) {
                printf("Resume PVM System\n");
                if(wsNodeRef != NULL)
                {
                    res = taf_mngdPm_StayAwakeNode(wsNodeRef);
                    if(res == LE_OK)
                    {
                        printf("'Resumed sysytem'\n");
                    }
                }
                else
                {
                    printf("'wsNodeRef is null, Call NewNodeWakeupSource'\n");
                }
            }
            else if(NODE_ID == 1) {
                printf("Resume RPC System\n");
                if(wsNodeRef1 != NULL) {
                    res = taf_mngdPm_StayAwakeNode(wsNodeRef1);
                    if(res == LE_OK) {
                        printf("'Resumed sysytem'\n");
                    }
                }
                else
                {
                    printf("'wsNodeRef1 is null for Rpc, Call NewNodeWakeupSource'\n");
                }
            }
        }
        if(input == 3)
        {
            char RelaxNode[100];
            printf("Enter NODE_ID\n -1.Exit\n 0.PVM\n 1.RPC\n");
            if(fgets(RelaxNode, sizeof(RelaxNode), stdin))
                LE_INFO("Value read successfully");
            RelaxNode[strcspn(RelaxNode, "\n")] = '\0';
            int NODE_ID = atoi(RelaxNode);
            if(NODE_ID == -1)
                continue;
            if(NODE_ID == 0) {
                printf("Suspend PVM System\n");
                if(wsNodeRef != NULL) {
                    res = taf_mngdPm_RelaxNode(wsNodeRef);
                    if(res == LE_OK)
                    {
                        printf("'Suspended PVM system'\n");
                    }
                }
                else
                {
                    printf("'wsNodeRef is null, Call NewNodeWakeupSource'\n");
                }
            }
            else if(NODE_ID == 1) {
                printf("Suspend RPC System\n");
                if(wsNodeRef1 != NULL) {
                    res = taf_mngdPm_RelaxNode(wsNodeRef1);
                    if(res == LE_OK) {
                        printf("'Suspended RPC system'\n");
                    }
                }
                else
                {
                    printf("'wsNodeRef1 is null for Rpc, Call NewNodeWakeupSource'\n");
                }
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

    wsNodeRef = taf_mngdPm_CreateNodeWakeupSource(0, TAF_MNGDPM_WS_OPT_DEFAULT, wsTag);
    if(wsNodeRef)
        printf("NewNodeWakeupSource ref is created for\n");
    if(wsNodeRef != NULL) {
        res = taf_mngdPm_StayAwakeNode(wsNodeRef);
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

void StateMaskNodePowerStateChangeHandler
(
    uint8_t pmNodeId,
    taf_mngdPm_nodePowerStateRef_t nodePowerStateRef,
    taf_mngdPm_NodePowerState_t state,
    void *contextPtr
)
{
    LE_UNUSED(pmNodeId);
    LE_UNUSED(nodePowerStateRef);
    LE_UNUSED(state);
    LE_UNUSED(contextPtr);

    LE_INFO("Callback for NodePowerStateChange handler");
    exit(EXIT_FAILURE);
}

void TestAddNodePowerStateChangeHandlerStateMask(taf_mngdPm_NodePowerStateChangeBitMask_t inputStateMask)
{
    uint8_t pmNodeId = 0;

    LE_INFO("Registering NodePowerStateChangeHandler with state mask %d", inputStateMask);
    taf_mngdPm_NodePowerStateChangeHandlerRef_t ref =
        taf_mngdPm_AddNodePowerStateChangeHandler(
        StateMaskNodePowerStateChangeHandler, NULL, pmNodeId, inputStateMask);

    if (ref == NULL)
    {
        LE_ERROR("AddNodePowerStateChangeHandler failed for state mask %d", inputStateMask);
        exit(EXIT_FAILURE);
    }

    // Wait briefly to ensure no immediate callback is triggered for empty mask.
    le_thread_Sleep(1);
    taf_mngdPm_RemoveNodePowerStateChangeHandler(ref);
    LE_INFO("Add/Remove NodePowerStateChangeHandler succeeded for state mask 0");
    exit(EXIT_SUCCESS);
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

void *RegisterClientForPowerStateNotificationsWithoutAcknowledgementFunction(void *contextPtr)
{
    taf_mngdPm_ConnectService();
    taf_mngdPm_NodePowerStateChangeBitMask_t state = (taf_mngdPm_NodePowerStateChangeBitMask_t)(uintptr_t) contextPtr;
    taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
    ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerWithoutAckCB, NULL, 0, state);

    if(ref)
    {
        LE_INFO("AddNodePowerStateChangeHandler is success with bitmask %i", state);
    }
    le_sem_Post(semRef);
    le_event_RunLoop();
}

void RegisterClientForPowerStateNotificationWithoutAcknowledgement(taf_mngdPm_NodePowerStateChangeBitMask_t customBitmask)
{
    taf_mngdPm_NodePowerStateChangeBitMask_t bitmask = customBitmask;

    semRef = le_sem_Create("MngdIntTestApp", 0);
    threadRef = le_thread_Create("inttestapp",
                            RegisterClientForPowerStateNotificationsWithoutAcknowledgementFunction,
                            (void*)(uintptr_t) bitmask);
    if (threadRef) {
        fprintf(stderr, "Thread created with success\n");
    }
    le_thread_Start(threadRef);
    le_sem_Wait(semRef);
}

// WsDump multi-process test: primary client.
// Run alongside WsDumpClient02 (tafMngdPMIntTestHelper ELF) for a different-PID entry.
//
// Timer fires at ~t=15, 30, 42, 54s showing auth/ACQ state transitions.
void WsDumpClient01(void)
{
    taf_mngdPm_wsRef_t wsNormal = taf_mngdPm_CreateWakeupSource(
        TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL, TAF_MNGDPM_WS_OPT_DEFAULT, "ws-c01-normal");
    taf_mngdPm_wsRef_t wsSwUpd = taf_mngdPm_CreateWakeupSource(
        TAF_MNGDPM_STAY_AWAKE_REASON_SW_UPDATE, TAF_MNGDPM_WS_OPT_DEFAULT, "ws-c01-swupd");

    taf_mngdPm_StayAwake(wsNormal);
    // Only NORMAL is authorized: wsSwUpd → Unauthorized, ws-c02-ecall → Unauthorized
    taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_NORMAL);

    le_thread_Sleep(15);  // dump: normal(ACQ=yes,Auth) swupd(ACQ=no,Unauth) ecall(ACQ=yes,Unauth)

    taf_mngdPm_Relax(wsNormal);
    le_thread_Sleep(15);  // dump: normal(ACQ=no,Auth) swupd(ACQ=no,Unauth) ecall(ACQ=yes,Unauth)

    taf_mngdPm_AuthorizeStayAwakeReason(AUTHORIZE_ALL_STAY_AWAKE_REASON);
    le_thread_Sleep(12);  // dump: normal(ACQ=no,Auth) swupd(ACQ=no,Auth) ecall(ACQ=yes,Auth)

    taf_mngdPm_DeleteWakeupSource(wsNormal);
    taf_mngdPm_DeleteWakeupSource(wsSwUpd);
    le_thread_Sleep(12);  // dump: only pmVHAL + ws-c02-ecall

    LE_INFO("WsDumpClient01 done");
    exit(EXIT_SUCCESS);
}

// WsDump multi-process test: helper client (run as tafMngdPMIntTestHelper ELF → different PID).
// Stays alive for the full ~56s to appear in all 4 timer dumps.
void WsDumpClient02(void)
{
    taf_mngdPm_wsRef_t wsEcall = taf_mngdPm_CreateWakeupSource(
        TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE, TAF_MNGDPM_WS_OPT_DEFAULT, "ws-c02-ecall");

    taf_mngdPm_StayAwake(wsEcall);
    le_thread_Sleep(56);  // covers all 4 timer fires; auth state reflects Client01's changes

    taf_mngdPm_Relax(wsEcall);
    taf_mngdPm_DeleteWakeupSource(wsEcall);
    LE_INFO("WsDumpClient02 done");
    exit(EXIT_SUCCESS);
}

//==================================================================================================
// Shutdown/restart VHAL prepare pending window tests
//
// Window = interval from nodeStateChangePrepareAsync(SHUTDOWN/RESTART) dispatch until the VHAL
// prepare response arrives or the 10s hal_state_prepare_timeout fires. Inside it MPMS currentState
// is SHUTTING_DOWN or RESTARTING, StayAwake/Relax/AuthorizeStayAwakeReason only mutate wsCount
// while the PMS wake-source reference is frozen, and RevertPendingWindow() reconciles the two when
// the window closes on NOT_READY/INVALID_REQUEST/timeout.
//
// The VHAL prepare response is chosen by the test driver, not by this test process:
//     pmVHalDrv.so set prepare_reason timeout     // no callback -> 10s vhalAckTimer fires
//     pmVHalDrv.so set prepare_reason not_ready   // synchronous NACK
//     pmVHalDrv.so set prepare_reason ready       // proceeds to power off (not used here)
// pmDriver v1 equivalent: echo 2 / 1 / 0 > /data/le_fs/ack
//
// "timeout" is what gives the window real width: with a synchronous response the window is only one
// event-loop iteration wide and no client call can be squeezed into it. So the in-window action
// tests (PendingWinStayAwake/Relax/Authorize/Restart) all require
// prepare_reason=timeout, and the NACK tests only verify the close path.
//
// Note there is no SHUTDOWN_PREPARE/RESTART_PREPARE notification to synchronise on when the window
// opens: those are only emitted once PMS reports TAF_PM_STATE_SHUTDOWN/RESTART, which happens after
// ShutdownNAD()/RestartNAD() on a READY response. Under timeout/not_ready injection they never
// arrive. The window is instead known to be open as soon as the ReqAsync IPC returns LE_OK, because
// the server enters SHUTTING_DOWN/RESTARTING and starts the ack timer before replying.
//==================================================================================================

#define PENDING_WIN_WS_TAG      "ws-pendwin"

// hal_state_prepare_timeout is 10000 ms, so the watchdog guarding an async response must be wider
// than that. A NACK arrives much sooner, but using the same bound for it means a mis-set injection is
// reported as "expected NOT_READY, got TIMEOUT" instead of the far vaguer "no callback".
#define PENDING_WIN_CB_WAIT_MS  15000

// Bound for the post-revert SUSPEND_PREPARE notification in PendingWinRelax.
static const le_clk_Time_t PENDING_WIN_NTF_WAIT = { .sec = 15, .usec = 0 };

// How many notifications PendingWinWaitPrepare inspects before giving up. Intermediate prepare states
// are expected on the way to the one under test, but the count must stay bounded so a node
// oscillating between states fails the test instead of hanging it.
#define PENDING_WIN_NTF_ATTEMPTS 5

// All pending-window test state lives in one struct. Independent from the shared NodePwState*
// globals (registered client, prepSem, snapSem, prepState, gotPrepare) so these tests neither
// see nor collide with those and each test starts from a known-zero context.
typedef void (*PendingWinStepFunc_t)(void);

typedef struct
{
    // ---- Step machine ----
    PendingWinStepFunc_t continuation;   // next step, invoked from event loop after IPC response
    le_timer_Ref_t       watchdog;       // guards against a callback never arriving
    const char*          waitWhat;       // log tag for the currently-awaited response
    uint8_t              nodeId;         // node under test

    // ---- Async response captured from the shutdown/restart callback ----
    volatile taf_mngdPm_ResponseMode_t rspMode;

    // ---- Independent client + notification observation ----
    // Own thread + registration so these tests do not depend on the shared NodePwState* scaffolding.
    // gotPrepare is the only reliable "prepare was observed" signal - prepState alone cannot be
    // used because SHUTDOWN_PREPARE == 0 collides with the initial zero value.
    le_thread_Ref_t                                    clientThread;
    taf_mngdPm_NodePowerStateChangeHandlerRef_t        handlerRef;
    le_sem_Ref_t                                       regSem;      // handler registered
    le_sem_Ref_t                                       snapSem;     // immediate snapshot arrived
    le_sem_Ref_t                                       prepSem;     // a prepare notification arrived
    volatile bool                                      gotPrepare;
    volatile taf_mngdPm_NodePowerState_t               prepState;
} PendingWinCtx_t;

static PendingWinCtx_t g_pw;

static const char* RspModeToStr(taf_mngdPm_ResponseMode_t m)
{
    switch (m)
    {
        case TAF_MNGDPM_READY:           return "READY";
        case TAF_MNGDPM_NOT_READY:       return "NOT_READY";
        case TAF_MNGDPM_TIMEOUT:         return "TIMEOUT";
        case TAF_MNGDPM_INVALID_REQUEST: return "INVALID_REQUEST";
        default:                         return "UNKNOWN";
    }
}

static const char* NodeStateToStr(taf_mngdPm_NodePowerState_t s)
{
    switch (s)
    {
        case TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE: return "SHUTDOWN_PREPARE";
        case TAF_MNGDPM_NODE_STATE_RESTART_PREPARE:  return "RESTART_PREPARE";
        case TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE:  return "SUSPEND_PREPARE";
        case TAF_MNGDPM_NODE_STATE_RESUME:           return "RESUME";
        default:                                     return "UNKNOWN";
    }
}

static void PendingWinCleanup(void)
{
    if (g_pw.watchdog)
    {
        le_timer_Stop(g_pw.watchdog);
        le_timer_Delete(g_pw.watchdog);
        g_pw.watchdog = NULL;
    }
    if (g_pw.handlerRef)
    {
        taf_mngdPm_RemoveNodePowerStateChangeHandler(g_pw.handlerRef);
        g_pw.handlerRef = NULL;
    }
    if (g_pw.regSem)  { le_sem_Delete(g_pw.regSem);  g_pw.regSem  = NULL; }
    if (g_pw.prepSem) { le_sem_Delete(g_pw.prepSem); g_pw.prepSem = NULL; }
    if (g_pw.snapSem) { le_sem_Delete(g_pw.snapSem); g_pw.snapSem = NULL; }
}

static void PendingWinFail(const char* why)
{
    LE_ERROR("[PMWIN] ---- test FAILED: %s ----", why);
    PendingWinCleanup();
    exit(EXIT_FAILURE);
}

static void PendingWinPass(const char* what)
{
    LE_INFO("[PMWIN] ---- test PASSED: %s ----", what);
    PendingWinCleanup();
    exit(EXIT_SUCCESS);
}

static void PendingWinExpect(taf_mngdPm_ResponseMode_t want)
{
    if (g_pw.rspMode != want)
    {
        LE_ERROR("Expected %s, got %s", RspModeToStr(want), RspModeToStr(g_pw.rspMode));
        PendingWinFail("unexpected response mode");
    }
}

static void PendingWinWatchdogHandler(le_timer_Ref_t timerRef)
{
    (void)timerRef;
    LE_ERROR("Timeout after %d ms waiting for %s", PENDING_WIN_CB_WAIT_MS, g_pw.waitWhat);
    PendingWinFail("async response never arrived");
}

static void PendingWinRunContinuation(void* param1Ptr, void* param2Ptr)
{
    (void)param1Ptr; (void)param2Ptr;

    PendingWinStepFunc_t step = g_pw.continuation;
    g_pw.continuation = NULL;
    if (step) step();
}

// Register the continuation to run once the async response arrives, arm the watchdog, then RETURN so
// the event loop can run. The caller must not block after calling this.
//
// This is the whole reason the tests are written as step machines: a ShutdownReqAsync/RestartReqAsync
// callback is dispatched on the event loop of the thread that issued the request. Blocking that
// thread on a semaphore to wait for it therefore deadlocks - the response message sits in the queue
// with nobody to dispatch it, and the wait always ends in a spurious timeout even though the service
// answered on time.
static void PendingWinAwaitResponse(const char* what, PendingWinStepFunc_t next)
{
    LE_INFO("Awaiting %s, returning to the event loop", what);
    g_pw.waitWhat     = what;
    g_pw.continuation = next;
    le_timer_Start(g_pw.watchdog);
}

static void PendingWinOnResponse(taf_mngdPm_ResponseMode_t rspMode)
{
    g_pw.rspMode = rspMode;

    if (g_pw.watchdog) le_timer_Stop(g_pw.watchdog);

    // Continue from the event loop rather than inline, so the rest of the test does not issue new IPC
    // from inside a callback dispatch.
    le_event_QueueFunction(PendingWinRunContinuation, NULL, NULL);
}

// Unlike ForcedSystemShutdownCallBack/RestartCallback these do not treat non-READY as a failure:
// for the pending window tests NOT_READY/TIMEOUT is the expected outcome.
static void PendingWinShutdownCB(taf_mngdPm_ShutdownMode_t mode,
    taf_mngdPm_ResponseMode_t rspMode, le_result_t result, void* ctxPtr)
{
    (void)mode; (void)ctxPtr;
    LE_INFO("PendingWinShutdownCB: rspMode=%s(%d) result=%d", RspModeToStr(rspMode), rspMode, result);
    PendingWinOnResponse(rspMode);
}

static void PendingWinRestartCB(taf_mngdPm_RestartMode_t mode,
    taf_mngdPm_ResponseMode_t rspMode, le_result_t result, void* ctxPtr)
{
    (void)mode; (void)ctxPtr;
    LE_INFO("PendingWinRestartCB: rspMode=%s(%d) result=%d", RspModeToStr(rspMode), rspMode, result);
    PendingWinOnResponse(rspMode);
}

// Discard tokens already sitting on a counting semaphore, so a subsequent wait can only be satisfied
// by a notification that arrives after this point. g_pw.prepSem accumulates one token per prepare
// notification and several may be expected before the one under test.
static void PendingWinDrainSem(le_sem_Ref_t sem)
{
    static const le_clk_Time_t noWait = { .sec = 0, .usec = 0 };
    if (!sem) return;
    while (le_sem_WaitWithTimeOut(sem, noWait) == LE_OK)
    {
        LE_INFO("Drained a stale notification token");
    }
}

// Bring up the watchdog timer and an independent client thread holding a SHUTDOWN_PREPARE|
// RESTART_PREPARE|SUSPEND_PREPARE|RESUME handler. The handler posts g_pw.prepSem/snapSem and only
// this test's continuations wait on them, so notifications from the wider system cannot satisfy
// a wait meant for something else. stateChangeAck decides the client ACK, which is orthogonal to
// the VHAL prepare response driving the window.
static void PendingWinNotifyCb(uint8_t pmNodeId,
                               taf_mngdPm_nodePowerStateRef_t ref,
                               taf_mngdPm_NodePowerState_t state,
                               void* ctx)
{
    (void)ctx;
    LE_INFO("PendingWinNotifyCb: node=%u state=%s(%d)", pmNodeId, NodeStateToStr(state), state);

    if (IsPrepareState(state))
    {
        g_pw.gotPrepare = true;
        g_pw.prepState  = state;
        if (g_pw.prepSem) le_sem_Post(g_pw.prepSem);

        SendAckPolicy(pmNodeId, ref);
        return;
    }

    if (state == TAF_MNGDPM_NODE_STATE_RESUME)
    {
        if (g_pw.snapSem) le_sem_Post(g_pw.snapSem);
    }
}

static void* PendingWinClientThread(void* unused)
{
    (void)unused;
    taf_mngdPm_ConnectService();

    g_pw.handlerRef = taf_mngdPm_AddNodePowerStateChangeHandler(
        PendingWinNotifyCb, NULL, g_pw.nodeId, MASK_ALL);
    if (!g_pw.handlerRef)
    {
        LE_ERROR("PendingWin AddNodePowerStateChangeHandler failed");
    }
    if (g_pw.regSem) le_sem_Post(g_pw.regSem);

    le_event_RunLoop();
    return NULL;
}

static bool PendingWinStartClient(uint8_t pmNodeId)
{
    g_pw.nodeId     = pmNodeId;
    g_pw.gotPrepare = false;
    g_pw.prepState  = 0;
    g_pw.rspMode    = TAF_MNGDPM_READY;
    g_pw.continuation = NULL;

    g_pw.watchdog = le_timer_Create("pendWinWatchdog");
    le_timer_SetMsInterval(g_pw.watchdog, PENDING_WIN_CB_WAIT_MS);
    le_timer_SetHandler(g_pw.watchdog, PendingWinWatchdogHandler);

    g_pw.regSem  = le_sem_Create("pwRegSem",  0);
    g_pw.prepSem = le_sem_Create("pwPrepSem", 0);
    g_pw.snapSem = le_sem_Create("pwSnapSem", 0);

    g_pw.clientThread = le_thread_Create("PendingWinClient", PendingWinClientThread, NULL);
    le_thread_Start(g_pw.clientThread);

    if (!WaitSemT(g_pw.regSem, REG_WAIT, "handler registration"))
    {
        return false;
    }

    // Registration triggers an immediate snapshot. If the node is already headed for suspend the
    // snapshot is itself a prepare state, so both sems may hold a stale token; drain both before
    // any test-specific wait so only fresh notifications can satisfy it.
    (void)le_sem_WaitWithTimeOut(g_pw.snapSem, REG_WAIT);
    PendingWinDrainSem(g_pw.prepSem);
    PendingWinDrainSem(g_pw.snapSem);
    return true;
}

// Wait until a prepare notification for `want` arrives, tolerating other prepare states on the way.
// The post-revert path is RESUME -> Suspending -> SUSPEND_PREPARE, so a single wait can be consumed
// by an intermediate transition; loop instead of treating the first token as the answer.
//
// Blocking here is safe, unlike blocking on the async response: these notifications are delivered on
// PendingWinClientThread, which runs its own event loop, so they can be posted while this thread
// waits.
static bool PendingWinWaitPrepare(taf_mngdPm_NodePowerState_t want, const char* what)
{
    // Only tokens posted from here on may satisfy the wait.
    PendingWinDrainSem(g_pw.prepSem);

    for (int attempt = 0; attempt < PENDING_WIN_NTF_ATTEMPTS; ++attempt)
    {
        g_pw.gotPrepare = false;

        if (!WaitSemT(g_pw.prepSem, PENDING_WIN_NTF_WAIT, what))
        {
            return false;
        }
        if (!g_pw.gotPrepare)
        {
            LE_WARN("Woken with no prepare recorded, keep waiting for %s", what);
            continue;
        }
        if (g_pw.prepState == want)
        {
            LE_INFO("Observed %s as expected", NodeStateToStr(want));
            return true;
        }
        LE_INFO("Ignoring intermediate %s while waiting for %s",
                NodeStateToStr(g_pw.prepState), NodeStateToStr(want));
    }

    LE_ERROR("Gave up after %d notifications, none was %s", PENDING_WIN_NTF_ATTEMPTS,
             NodeStateToStr(want));
    return false;
}

// Log the MPMS view of the node power state. Note this reflects the PMS state (RESUME/SUSPEND/
// SHUTDOWN/RESTART), not the MPMS internal state machine, so it cannot observe SHUTTING_DOWN.
static taf_mngdPm_NodePowerState_t PendingWinDumpState(const char* when)
{
    taf_mngdPm_NodePowerState_t st = TAF_MNGDPM_NODE_STATE_RESUME;
    le_result_t res = taf_mngdPm_GetNodePowerState(g_pw.nodeId, &st);
    if (res != LE_OK)
    {
        LE_ERROR("[%s] GetNodePowerState failed: %d", when, res);
    }
    else
    {
        LE_INFO("[%s] node=%u power state = %s(%d)", when, g_pw.nodeId, NodeStateToStr(st), st);
    }
    return st;
}

static void PendingWinRequireResume(const char* why)
{
    if (PendingWinDumpState("after-revert") != TAF_MNGDPM_NODE_STATE_RESUME)
    {
        PendingWinFail(why);
    }
}

// Open the window. The server completes RequestStateChange -> ProcessStateChange(SHUTTING_DOWN/
// RESTARTING) -> le_timer_Start(vhalAckTimer) -> nodeStateChangePrepareAsync before replying, so once
// the IPC returns LE_OK the window is already open and in-window calls can be issued right away.
//
// Do NOT wait for a SHUTDOWN_PREPARE/RESTART_PREPARE notification here: those are only emitted once
// PMS reports TAF_PM_STATE_SHUTDOWN/RESTART, which happens after ShutdownNAD()/RestartNAD() on a
// READY response. Under timeout or not_ready injection they never arrive at all.
//
// LE_BUSY handling: the MPMS internal FSM (Suspend/Waking up/RELEASING_WAKE_SOURCE) transiently
// rejects Shutting-down/Restarting even when GetNodePowerState reports RESUME. Retry after a fresh
// RESUME NTF, up to PENDING_WIN_OPEN_RETRIES, rather than trusting the snapshot state.
#define PENDING_WIN_OPEN_RETRIES 3
static const le_clk_Time_t PENDING_WIN_RESUME_WAIT = { .sec = 5, .usec = 0 };

// Wait for a fresh RESUME notification. snapSem is posted by PendingWinNotifyCb only for RESUME, so
// draining first guarantees the token we consume was posted after this call started.
static bool PendingWinAwaitResumeNtf(const char* what)
{
    PendingWinDrainSem(g_pw.snapSem);
    if (!WaitSemT(g_pw.snapSem, PENDING_WIN_RESUME_WAIT, what))
    {
        LE_WARN("No RESUME NTF within timeout while waiting for %s", what);
        return false;
    }
    LE_INFO("Fresh RESUME NTF observed before %s", what);
    return true;
}

// Every pending-window test starts from a clean SUSPEND baseline: MPMS in SUSPEND, no client
// holds a wake source. Verifying this via a NodePowerState snapshot before anything else keeps
// each test's setup in a known context and aborts loudly on violation instead of drifting into a
// false pass or LE_BUSY. Hard fail — the operator must allow settle time between cases so SUSPEND
// is always reached.
static void PendingWinAssertSuspendPrepare(const char* what)
{
    taf_mngdPm_NodePowerState_t st = TAF_MNGDPM_NODE_STATE_RESUME;
    le_result_t res = taf_mngdPm_GetNodePowerState(g_pw.nodeId, &st);
    if (res != LE_OK)
    {
        LE_ERROR("[%s] GetNodePowerState failed: %d", what, res);
        PendingWinFail("GetNodePowerState failed while asserting SUSPEND_PREPARE");
    }
    if (st != TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE)
    {
        LE_ERROR("[%s] precondition NOT met: expected SUSPEND_PREPARE, got %s",
                 what, NodeStateToStr(st));
        PendingWinFail("test precondition: node not in SUSPEND_PREPARE");
    }
    LE_INFO("[%s] precondition OK: node in SUSPEND_PREPARE", what);
}

// Wait for the node to fall back into SUSPEND after the window closes and all wake sources are
// released. This is the post-condition every test asserts before exiting, so the next case in the
// runner can rely on the SUSPEND_PREPARE precondition. Uses the existing prepSem wait, which
// tolerates intermediate prepare transitions on the way down.
static void PendingWinAwaitSuspendFinal(void)
{
    if (!PendingWinWaitPrepare(TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE,
                               "final SUSPEND_PREPARE after cleanup"))
    {
        PendingWinFail("no SUSPEND_PREPARE after final cleanup: something still holds the node awake");
    }
    PendingWinAssertSuspendPrepare("final");
}

static bool PendingWinOpenShutdown(void)
{
    le_result_t res = LE_FAULT;
    for (int attempt = 0; attempt < PENDING_WIN_OPEN_RETRIES; ++attempt)
    {
        res = taf_mngdPm_ShutdownReqAsync(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
            PendingWinShutdownCB, NULL, TAF_MNGDPM_SHUTDOWN_REASON_NORMAL);
        if (res == LE_OK)
        {
            LE_INFO("ShutdownReqAsync accepted, MPMS in SHUTTING_DOWN, pending window is open");
            return true;
        }
        if (res != LE_BUSY)
        {
            LE_ERROR("ShutdownReqAsync failed: %d", res);
            return false;
        }
        LE_WARN("ShutdownReqAsync attempt %d returned LE_BUSY: MPMS FSM not at RESUME, wait for NTF",
                attempt + 1);
        (void)PendingWinAwaitResumeNtf("MPMS RESUME before retrying ShutdownReqAsync");
    }
    LE_ERROR("ShutdownReqAsync stayed LE_BUSY across %d retries", PENDING_WIN_OPEN_RETRIES);
    return false;
}

static bool PendingWinOpenRestart(void)
{
    le_result_t res = LE_FAULT;
    for (int attempt = 0; attempt < PENDING_WIN_OPEN_RETRIES; ++attempt)
    {
        res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT,
            PendingWinRestartCB, NULL, TAF_MNGDPM_RESTART_REASON_NORMAL);
        if (res == LE_OK)
        {
            LE_INFO("RestartReqAsync accepted, MPMS in RESTARTING, pending window is open");
            return true;
        }
        if (res != LE_BUSY)
        {
            LE_ERROR("RestartReqAsync failed: %d", res);
            return false;
        }
        LE_WARN("RestartReqAsync attempt %d returned LE_BUSY: MPMS FSM not at RESUME, wait for NTF",
                attempt + 1);
        (void)PendingWinAwaitResumeNtf("MPMS RESUME before retrying RestartReqAsync");
    }
    LE_ERROR("RestartReqAsync stayed LE_BUSY across %d retries", PENDING_WIN_OPEN_RETRIES);
    return false;
}

//==================================================================================================
// T1: StayAwake inside the window, closed by the 10s VHAL prepare timeout.
// Requires: pmVHalDrv.so set prepare_reason timeout   (pmDriver v1: echo 2 > /data/le_fs/ack)
//
// Expect in tafMngdPMSvc log:
//   AcquireWakeLock wsCount = N in shutdown/restart pending window   <- PMS StayAwake suppressed
//   VhalAckTimer Expired after 10000 msec for state 0
//   RevertPendingWindow: Shutting down -> Resume
// and the client callback receives TIMEOUT.
//
// "Post-revert: PMS StayAwake done" only appears when the PMS reference actually has to be taken,
// i.e. when nothing else held the node awake at window-open time. Running this on an already-awake
// system is still a valid pass, it just does not exercise that reconcile branch.
//==================================================================================================
// Pre-window ws1 (drives FSM WAKING_UP -> RESUME) and in-window ws2 (bumps wsCount) live in file
// scope so PendingWinStayAwakeDone can release both after the async response.
static taf_mngdPm_wsRef_t g_pwT1Ws1 = NULL;
static taf_mngdPm_wsRef_t g_pwT1Ws2 = NULL;

static void PendingWinStayAwakeDone(void);

static void PendingWinStayAwake(uint8_t pmNodeId)
{
    LE_INFO("---- PendingWinStayAwake (needs prepare_reason=timeout) ----");

    if (!PendingWinStartClient(pmNodeId)) PendingWinFail("client bring-up");

    // Precondition: idle system, MPMS in SUSPEND.
    PendingWinAssertSuspendPrepare("T1 start");

    // Pre-window ws1: create + StayAwake, then wait for the VHAL RESUME NTF so FSM currentState is
    // guaranteed to be RESUME before the shutdown request. ws1 stays held until Done to keep the
    // system from sinking back into SUSPEND while the window is open.
    g_pwT1Ws1 = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
        TAF_MNGDPM_WS_OPT_DEFAULT, PENDING_WIN_WS_TAG);
    if (g_pwT1Ws1 == NULL) PendingWinFail("T1 pre-window CreateWakeupSource(ws1)");

    le_result_t res = taf_mngdPm_StayAwake(g_pwT1Ws1);
    if (res != LE_OK) PendingWinFail("T1 pre-window StayAwake(ws1)");

    if (!PendingWinAwaitResumeNtf("T1 RESUME NTF before opening window"))
    {
        PendingWinFail("T1: RESUME NTF never arrived after pre-window StayAwake");
    }

    PendingWinDumpState("before-window");

    if (!PendingWinOpenShutdown()) PendingWinFail("ShutdownReqAsync rejected, window never opened");

    // In-window ws2: Create + StayAwake must be accepted (window semantics: bump wsCount).
    g_pwT1Ws2 = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
        TAF_MNGDPM_WS_OPT_DEFAULT, PENDING_WIN_WS_TAG);
    if (g_pwT1Ws2 == NULL) PendingWinFail("T1 in-window CreateWakeupSource(ws2)");

    res = taf_mngdPm_StayAwake(g_pwT1Ws2);
    if (res != LE_OK)
    {
        LE_ERROR("T1 in-window StayAwake(ws2) rejected: %d", res);
        PendingWinFail("T1 in-window StayAwake(ws2) must return LE_OK");
    }
    LE_INFO("T1 in-window StayAwake(ws2) accepted (PMS StayAwake expected to be deferred)");

    // Re-invoking StayAwake on the same already-locked ws must be rejected with LE_DUPLICATE per svc
    // contract, regardless of the pending window. Anything else (LE_OK, LE_BUSY, ...) means the
    // in-window wsCount bookkeeping does not enforce single-lock semantics.
    res = taf_mngdPm_StayAwake(g_pwT1Ws2);
    if (res != LE_DUPLICATE)
    {
        LE_ERROR("T1 duplicate StayAwake(ws2) returned %d, expected LE_DUPLICATE", res);
        PendingWinFail("T1: repeated StayAwake on same ws must be LE_DUPLICATE in window");
    }
    LE_INFO("T1 duplicate StayAwake(ws2) correctly rejected with LE_DUPLICATE");

    // Window closes on the 10s prepare timeout.
    PendingWinAwaitResponse("shutdown timeout callback", PendingWinStayAwakeDone);
}

static void PendingWinStayAwakeDone(void)
{
    PendingWinExpect(TAF_MNGDPM_TIMEOUT);

    // Two wake sources are still held (ws1 and ws2), so the node must be RESUME.
    PendingWinRequireResume("node not RESUME after revert while ws1/ws2 are held");

    // Release ws2 first (in-window), then ws1 (pre-window). Both must succeed.
    le_result_t res = taf_mngdPm_Relax(g_pwT1Ws2);
    if (res != LE_OK) { LE_ERROR("T1 post-revert Relax(ws2)=%d", res); PendingWinFail("T1 Relax(ws2)"); }
    (void)taf_mngdPm_DeleteWakeupSource(g_pwT1Ws2);
    g_pwT1Ws2 = NULL;

    res = taf_mngdPm_Relax(g_pwT1Ws1);
    if (res != LE_OK) { LE_ERROR("T1 post-revert Relax(ws1)=%d", res); PendingWinFail("T1 Relax(ws1)"); }
    (void)taf_mngdPm_DeleteWakeupSource(g_pwT1Ws1);
    g_pwT1Ws1 = NULL;

    // Both wake sources gone -> node must go back to SUSPEND.
    PendingWinAwaitSuspendFinal();

    PendingWinPass("StayAwake in window + LE_DUPLICATE on repeat + timeout revert + suspend after");
}

//==================================================================================================
// T2: Relax inside the window driving wsCount to 0, closed by the 10s VHAL prepare timeout.
// Requires: pmVHalDrv.so set prepare_reason timeout
//
// Expect in tafMngdPMSvc log:
//   ReleaseWakeLock wsCount:0
//   ReleaseWakeLock wsCount=0 in pending window, defer PMS relax   <- PMS Relax suppressed
//   (no "Wake source from pms released successfully" while in the window)
//   RevertPendingWindow: Shutting down -> Resume
//   Post-revert: PMS Relax done                                    <- deferred relax applied
// then, because nothing holds a wake source any more, PMS drives the node to suspend.
//
// Note this needs the test process to own the only wake source on the node, so run it on an
// otherwise idle system: anything else holding the node awake keeps wsCount above 0 and there will be
// no suspend to observe.
//==================================================================================================
static void PendingWinRelaxDone(void);

static void PendingWinRelax(uint8_t pmNodeId)
{
    LE_INFO("---- PendingWinRelax (needs prepare_reason=timeout) ----");

    if (!PendingWinStartClient(pmNodeId)) PendingWinFail("client bring-up");

    PendingWinAssertSuspendPrepare("T2 start");

    // Pre-window: hold a wake source so wsCount can be driven to 0 from inside the window.
    wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
        TAF_MNGDPM_WS_OPT_DEFAULT, PENDING_WIN_WS_TAG);
    if (wsRef == NULL) PendingWinFail("CreateWakeupSource before window");

    le_result_t res = taf_mngdPm_StayAwake(wsRef);
    if (res != LE_OK) PendingWinFail("pre-window StayAwake");
    LE_INFO("Pre-window wake source acquired");

    if (!PendingWinAwaitResumeNtf("T2 RESUME NTF before opening window"))
    {
        PendingWinFail("T2: RESUME NTF never arrived after pre-window StayAwake");
    }

    PendingWinDumpState("before-window");

    if (!PendingWinOpenShutdown()) PendingWinFail("ShutdownReqAsync rejected, window never opened");

    // In-window: Relax must be accepted and only decrement wsCount.
    res = taf_mngdPm_Relax(wsRef);
    if (res != LE_OK)
    {
        LE_ERROR("In-window Relax rejected: %d", res);
        PendingWinFail("in-window Relax must return LE_OK");
    }
    LE_INFO("In-window Relax accepted (PMS Relax expected to be deferred)");

    PendingWinAwaitResponse("shutdown timeout callback", PendingWinRelaxDone);
}

static void PendingWinRelaxDone(void)
{
    PendingWinExpect(TAF_MNGDPM_TIMEOUT);

    PendingWinDumpState("after-revert");

    // wsCount==0 across the revert -> system must fall into SUSPEND.
    (void)taf_mngdPm_DeleteWakeupSource(wsRef);
    wsRef = NULL;

    PendingWinAwaitSuspendFinal();

    PendingWinPass("Relax in window + timeout revert + suspend afterwards");
}

//==================================================================================================
// T3: AuthorizeStayAwakeReason inside the window.
// Requires: pmVHalDrv.so set prepare_reason timeout
//
// De-authorizing NORMAL makes RefreshWakeSources decrement wsCount for the held source, then
// re-authorizing it increments wsCount again - both while PMS reference changes are deferred.
//
// Expect in tafMngdPMSvc log, twice:
//   [Refresh] Defer PMS ref change in shutdown/restart pending window
// and after the revert no "Post-revert: taf_pm_* failed", since wsCount is back where it started
// and matches the frozen isWsAcquired.
//==================================================================================================
static void PendingWinAuthorizeDone(void);

static void PendingWinAuthorize(uint8_t pmNodeId)
{
    LE_INFO("---- PendingWinAuthorize (needs prepare_reason=timeout) ----");

    if (!PendingWinStartClient(pmNodeId)) PendingWinFail("client bring-up");

    PendingWinAssertSuspendPrepare("T3 start");

    // Pre-window: authorize NORMAL first, then hold a wake source with NORMAL reason. This is the
    // ws that in-window de-authorize/re-authorize will churn.
    le_result_t res = taf_mngdPm_AuthorizeStayAwakeReason(AUTHORIZE_ALL_STAY_AWAKE_REASON);
    if (res != LE_OK) PendingWinFail("pre-window AuthorizeStayAwakeReason(ALL)");

    wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
        TAF_MNGDPM_WS_OPT_DEFAULT, PENDING_WIN_WS_TAG);
    if (wsRef == NULL) PendingWinFail("CreateWakeupSource before window");

    res = taf_mngdPm_StayAwake(wsRef);
    if (res != LE_OK) PendingWinFail("pre-window StayAwake");
    LE_INFO("Pre-window wake source acquired with an authorized reason");

    if (!PendingWinAwaitResumeNtf("T3 RESUME NTF before opening window"))
    {
        PendingWinFail("T3: RESUME NTF never arrived after pre-window StayAwake");
    }

    PendingWinDumpState("before-window");

    if (!PendingWinOpenShutdown()) PendingWinFail("ShutdownReqAsync rejected, window never opened");

    // In-window: de-authorize NORMAL -> RefreshWakeSources drops wsCount but defers PMS relax.
    res = taf_mngdPm_AuthorizeStayAwakeReason(TAF_MNGDPM_STAY_AWAKE_REASON_BIT_MASK_ECALL_ACTIVE);
    if (res != LE_OK)
    {
        LE_ERROR("In-window de-authorize rejected: %d", res);
        PendingWinFail("in-window AuthorizeStayAwakeReason must return LE_OK");
    }
    LE_INFO("In-window de-authorized NORMAL (wsCount--, PMS ref change deferred)");

    // In-window: re-authorize NORMAL -> wsCount goes back up, still deferring PMS changes.
    res = taf_mngdPm_AuthorizeStayAwakeReason(AUTHORIZE_ALL_STAY_AWAKE_REASON);
    if (res != LE_OK)
    {
        LE_ERROR("In-window re-authorize rejected: %d", res);
        PendingWinFail("in-window AuthorizeStayAwakeReason must return LE_OK");
    }
    LE_INFO("In-window re-authorized NORMAL (wsCount++, PMS ref change deferred)");

    PendingWinAwaitResponse("shutdown timeout callback", PendingWinAuthorizeDone);
}

static void PendingWinAuthorizeDone(void)
{
    PendingWinExpect(TAF_MNGDPM_TIMEOUT);

    // wsCount is back where it started, so the revert must leave the node resumed and the source
    // usable.
    PendingWinRequireResume("node not RESUME after revert while an authorized wake source is held");

    le_result_t res = taf_mngdPm_Relax(wsRef);
    if (res != LE_OK)
    {
        LE_ERROR("Post-revert Relax failed: %d", res);
        PendingWinFail("wake source accounting broken across authorize churn");
    }
    (void)taf_mngdPm_DeleteWakeupSource(wsRef);
    wsRef = NULL;

    PendingWinAwaitSuspendFinal();

    PendingWinPass("AuthorizeStayAwakeReason churn in window + timeout revert");
}

//==================================================================================================
// T4: the RESTARTING window (NAD reboot), closed by the 10s VHAL prepare timeout.
// Requires: pmVHalDrv.so set prepare_reason timeout
//
// Same in-window semantics as T1 but through RestartReqAsync, covering the RESTARTING branches of
// AcquireWakeLock/RefreshWakeSources and the restart side of VhalAckTimerHandler.
//
// Expect in tafMngdPMSvc log:
//   AcquireWakeLock wsCount = N in shutdown/restart pending window
//   VhalAckTimer expire for TAF_MNGDPM_RESTART_MODE_NAD_REBOOT
//   RevertPendingWindow: Restarting -> Resume
//==================================================================================================
static void PendingWinRestartDone(void);

static void PendingWinRestart(uint8_t pmNodeId)
{
    LE_INFO("---- PendingWinRestart (needs prepare_reason=timeout) ----");

    if (!PendingWinStartClient(pmNodeId)) PendingWinFail("client bring-up");

    PendingWinAssertSuspendPrepare("T4 start");

    // Pre-window ws + wait for RESUME NTF: same idea as T2. The ws stays held past window open so the
    // FSM does not sink back to SUSPEND between "RESUME NTF" and RestartReqAsync.
    wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
        TAF_MNGDPM_WS_OPT_DEFAULT, PENDING_WIN_WS_TAG);
    if (wsRef == NULL) PendingWinFail("CreateWakeupSource before window");

    le_result_t res = taf_mngdPm_StayAwake(wsRef);
    if (res != LE_OK) PendingWinFail("pre-window StayAwake");

    if (!PendingWinAwaitResumeNtf("T4 RESUME NTF before opening window"))
    {
        PendingWinFail("T4: RESUME NTF never arrived after pre-window StayAwake");
    }

    PendingWinDumpState("before-window");

    if (!PendingWinOpenRestart()) PendingWinFail("RestartReqAsync rejected, window never opened");

    // In-window Relax + StayAwake on the same ws: drives wsCount 1 -> 0 -> 1 with PMS ref changes
    // deferred. Both calls must return LE_OK.
    res = taf_mngdPm_Relax(wsRef);
    if (res != LE_OK)
    {
        LE_ERROR("T4 in-window Relax rejected: %d", res);
        PendingWinFail("T4 in-window Relax must return LE_OK");
    }
    LE_INFO("T4 in-window Relax accepted (wsCount 1->0, PMS ref deferred)");

    res = taf_mngdPm_StayAwake(wsRef);
    if (res != LE_OK)
    {
        LE_ERROR("T4 in-window StayAwake rejected: %d", res);
        PendingWinFail("T4 in-window StayAwake must return LE_OK");
    }
    LE_INFO("T4 in-window StayAwake accepted (wsCount 0->1, PMS ref deferred)");

    PendingWinAwaitResponse("restart timeout callback", PendingWinRestartDone);
}

static void PendingWinRestartDone(void)
{
    PendingWinExpect(TAF_MNGDPM_TIMEOUT);

    PendingWinRequireResume("node not RESUME after restart window revert");

    le_result_t res = taf_mngdPm_Relax(wsRef);
    if (res != LE_OK)
    {
        LE_ERROR("Post-revert Relax failed: %d", res);
        PendingWinFail("wake source lost across the restart window");
    }
    (void)taf_mngdPm_DeleteWakeupSource(wsRef);
    wsRef = NULL;

    PendingWinAwaitSuspendFinal();

    PendingWinPass("StayAwake in RESTARTING window + timeout revert");
}

//==================================================================================================
// T5: shutdown window closed by a VHAL NACK.
// Requires: pmVHalDrv.so set prepare_reason not_ready   (pmDriver v1: echo 1 > /data/le_fs/ack)
//
// The NACK is delivered synchronously, so the window is only one event-loop iteration wide and no
// in-window client call is attempted here. What this covers is the close path: the revert plus the
// powerMode.isForceful reset, verified by issuing a second shutdown request that must not be
// rejected with LE_BUSY.
//
// Expect in tafMngdPMSvc log:
//   RevertPendingWindow: Shutting down -> Resume
//==================================================================================================
static void PendingWinNackShutdownSecond(void);
static void PendingWinNackShutdownDone(void);

static void PendingWinNackShutdown(uint8_t pmNodeId)
{
    LE_INFO("---- PendingWinNackShutdown (needs prepare_reason=not_ready) ----");

    if (!PendingWinStartClient(pmNodeId)) PendingWinFail("client bring-up");

    PendingWinAssertSuspendPrepare("T5 start");

    // Hold a wake source so a mistakenly applied PMS relax would show up as an unexpected suspend.
    wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
        TAF_MNGDPM_WS_OPT_DEFAULT, PENDING_WIN_WS_TAG);
    if (wsRef == NULL) PendingWinFail("CreateWakeupSource before window");
    if (taf_mngdPm_StayAwake(wsRef) != LE_OK) PendingWinFail("pre-window StayAwake");

    if (!PendingWinAwaitResumeNtf("T5 RESUME NTF before opening window"))
    {
        PendingWinFail("T5: RESUME NTF never arrived after pre-window StayAwake");
    }

    PendingWinDumpState("before-window");

    if (!PendingWinOpenShutdown()) PendingWinFail("first ShutdownReqAsync");

    PendingWinAwaitResponse("shutdown NACK callback", PendingWinNackShutdownSecond);
}

static void PendingWinNackShutdownSecond(void)
{
    PendingWinExpect(TAF_MNGDPM_NOT_READY);

    PendingWinRequireResume("node not RESUME after the NACK revert");

    // The state machine left SHUTTING_DOWN, so a fresh request must be accepted rather than LE_BUSY.
    le_result_t res = taf_mngdPm_ShutdownReqAsync(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
        PendingWinShutdownCB, NULL, TAF_MNGDPM_SHUTDOWN_REASON_NORMAL);
    if (res == LE_BUSY)
    {
        PendingWinFail("second ShutdownReqAsync returned LE_BUSY: state was not reverted");
    }
    if (res != LE_OK)
    {
        LE_ERROR("Second ShutdownReqAsync failed: %d", res);
        PendingWinFail("second ShutdownReqAsync");
    }

    PendingWinAwaitResponse("second shutdown NACK callback", PendingWinNackShutdownDone);
}

static void PendingWinNackShutdownDone(void)
{
    LE_INFO("Second request answered with %s: the window is fully re-armable",
            RspModeToStr(g_pw.rspMode));

    (void)taf_mngdPm_Relax(wsRef);
    (void)taf_mngdPm_DeleteWakeupSource(wsRef);
    wsRef = NULL;

    PendingWinAwaitSuspendFinal();

    PendingWinPass("shutdown NACK revert + request re-armable");
}

//==================================================================================================
// T6: restart (NAD reboot) window closed by a VHAL NACK.
// Requires: pmVHalDrv.so set prepare_reason not_ready
//
// Expect in tafMngdPMSvc log:
//   RevertPendingWindow: Restarting -> Resume
//==================================================================================================
static void PendingWinNackRestartSecond(void);
static void PendingWinNackRestartDone(void);

static void PendingWinNackRestart(uint8_t pmNodeId)
{
    LE_INFO("---- PendingWinNackRestart (needs prepare_reason=not_ready) ----");

    if (!PendingWinStartClient(pmNodeId)) PendingWinFail("client bring-up");

    PendingWinAssertSuspendPrepare("T6 start");

    wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
        TAF_MNGDPM_WS_OPT_DEFAULT, PENDING_WIN_WS_TAG);
    if (wsRef == NULL) PendingWinFail("CreateWakeupSource before window");
    if (taf_mngdPm_StayAwake(wsRef) != LE_OK) PendingWinFail("pre-window StayAwake");

    if (!PendingWinAwaitResumeNtf("T6 RESUME NTF before opening window"))
    {
        PendingWinFail("T6: RESUME NTF never arrived after pre-window StayAwake");
    }

    PendingWinDumpState("before-window");

    if (!PendingWinOpenRestart()) PendingWinFail("first RestartReqAsync");

    PendingWinAwaitResponse("restart NACK callback", PendingWinNackRestartSecond);
}

static void PendingWinNackRestartSecond(void)
{
    PendingWinExpect(TAF_MNGDPM_NOT_READY);

    PendingWinRequireResume("node not RESUME after the restart NACK revert");

    le_result_t res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT,
        PendingWinRestartCB, NULL, TAF_MNGDPM_RESTART_REASON_NORMAL);
    if (res == LE_BUSY)
    {
        PendingWinFail("second RestartReqAsync returned LE_BUSY: state was not reverted");
    }
    if (res != LE_OK)
    {
        LE_ERROR("Second RestartReqAsync failed: %d", res);
        PendingWinFail("second RestartReqAsync");
    }

    PendingWinAwaitResponse("second restart NACK callback", PendingWinNackRestartDone);
}

static void PendingWinNackRestartDone(void)
{
    LE_INFO("Second request answered with %s: the window is fully re-armable",
            RspModeToStr(g_pw.rspMode));

    (void)taf_mngdPm_Relax(wsRef);
    (void)taf_mngdPm_DeleteWakeupSource(wsRef);
    wsRef = NULL;

    PendingWinAwaitSuspendFinal();

    PendingWinPass("restart NACK revert + request re-armable");
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
        else if(strcmp(testType, "TestAddNodePowerStateChangeHandlerStateMask") == 0)
        {
            if (testPar)
            TestAddNodePowerStateChangeHandlerStateMask(atoi(testPar));
            else {
                printf("Enter statemask value, an unsigned intergre");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "RegisterClientForPowerStateNotificationWithoutAcknowledgement") == 0)
        {
           RegisterClientForPowerStateNotificationWithoutAcknowledgement(atoi(testPar));
        }
        else if(strcmp(testType, "WsDumpClient01") == 0)
        {
            WsDumpClient01();
        }
        else if(strcmp(testType, "WsDumpClient02") == 0)
        {
            WsDumpClient02();
        }
        else if(strcmp(testType, "ImmediateNotifyClientOnCurrNodePwStateOnRegister") == 0)
        {
            if(testPar)
                ImmediateNotifyClientOnCurrNodePwStateOnRegister(atoi(testPar));
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "TestGracefulSysShutdownForNodePwStateChange") == 0)
        {
            if(testPar)
                TestGracefulSysShutdownForNodePwStateChange(atoi(testPar));
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "PendingWinStayAwake") == 0)
        {
            if(testPar)
                PendingWinStayAwake(atoi(testPar));
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "PendingWinRelax") == 0)
        {
            if(testPar)
                PendingWinRelax(atoi(testPar));
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "PendingWinAuthorize") == 0)
        {
            if(testPar)
                PendingWinAuthorize(atoi(testPar));
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "PendingWinRestart") == 0)
        {
            if(testPar)
                PendingWinRestart(atoi(testPar));
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "PendingWinNackShutdown") == 0)
        {
            if(testPar)
                PendingWinNackShutdown(atoi(testPar));
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(testType, "PendingWinNackRestart") == 0)
        {
            if(testPar)
                PendingWinNackRestart(atoi(testPar));
            else {
                printf("Enter NODE_ID");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            LE_ERROR("Error command");
            exit(EXIT_FAILURE);
        }
    }
}
