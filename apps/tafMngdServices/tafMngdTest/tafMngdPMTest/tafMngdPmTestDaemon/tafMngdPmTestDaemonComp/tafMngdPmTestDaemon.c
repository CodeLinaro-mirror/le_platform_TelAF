/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define PRIMARY_NODE_ID 0
#define REASON_LEN 256
#define STATE_LEN 64
#define REPORT_LEN 64
#define MONITOR_STATUS_LEN 128
#define TEST_TIMEOUT_MS 300000

typedef enum
{
    CONTROL_CONNECTION_DISCONNECTED,
    CONTROL_CONNECTION_CONNECTED
}
ControlConnectionState_t;

typedef enum
{
    CONTROL_PENDING_NONE,
    CONTROL_PENDING_SHUTDOWN,
    CONTROL_PENDING_RESTART
}
ControlPendingOperationType_t;

typedef struct
{
    ControlPendingOperationType_t type;
    taf_mngdPmTest_ServerCmdRef_t commandRef;
    le_msg_SessionRef_t clientSessionRef;
}
ControlPendingOperation_t;

typedef struct
{
    taf_mngdPm_wsRef_t wakeSourceRef;
    bool lockHeld;
}
ResumeControlContext_t;

typedef struct
{
    taf_mngdPm_NodeModemAwakeHandlerRef_t handlerRef;
    taf_mngdPm_wsRef_t wakeSourceRef;
    taf_mngdPmTest_WakeupMonitorMode_t mode;
    bool lockHeld;
}
WakeupMonitorContext_t;

typedef struct
{
    ControlConnectionState_t connectionState;
    ControlPendingOperation_t pendingOperation;
}
ControlContext_t;

typedef struct
{
    const char* id;
    const char* helpDoc;
    le_result_t (*start)(void);
    le_result_t (*teardown)(char* reason, size_t reasonSize);
    uint32_t timeoutMs;
}
TestCase_t;

typedef struct
{
    le_thread_Ref_t threadRef;
    uint32_t threadId;
}
TestThread_t;

typedef struct
{
    TestThread_t thread;
    const char* testId;
    const char* wakeSourceTag;
    uint32_t generation;
    taf_mngdPm_NodePowerStateChangeHandlerRef_t stateHandlerRef;
    taf_mngdPm_wsRef_t wakeSourceRef;
    bool wakeSourceHeld;
    bool connected;
    le_result_t cleanupResult;
    char cleanupReason[REASON_LEN];
}
NadRebootWorkerContext_t;

typedef struct
{
    enum
    {
        TEST_EVENT_READY,
        TEST_EVENT_FAILURE
    } type;
    uint32_t generation;
    uint32_t sourceId;
    le_result_t result;
    char reason[REASON_LEN];
}
TestEvent_t;

typedef struct
{
    bool connected;
    bool ownsConnection;
}
TestServiceContext_t;

typedef struct
{
    uint32_t generation;
    size_t readyWorkerCount;
    bool restartRequestStarted;
    bool restartReady;
    taf_mngdPm_NodePowerStateChangeHandlerRef_t mainStateHandlerRef;
    taf_mngdPm_NodePowerState_t lastState;
    bool resumeSeen;
    bool sawSuspendPrepare;
    bool sawRestartPrepare;
    NadRebootWorkerContext_t workers[2];
}
NadRebootScenarioContext_t;

/* Control state is accessed only from the daemon main event thread. */
static ControlContext_t ControlContext =
{
    .connectionState = CONTROL_CONNECTION_DISCONNECTED,
    .pendingOperation =
    {
        .type = CONTROL_PENDING_NONE,
        .commandRef = NULL,
        .clientSessionRef = NULL
    }
};

static bool TestRunning = false;
static bool TestCompleting = false;
static bool TestTeardownDeferred = false;
static const TestCase_t* ActiveTestCase = NULL;
static taf_mngdPmTest_ServerCmdRef_t TestCmdRef = NULL;
static le_msg_SessionRef_t TestSessionRef = NULL;
static le_timer_Ref_t TestTimer = NULL;
static uint32_t ActiveTestGeneration = 0;
static TestServiceContext_t TestService =
{
    .connected = false,
    .ownsConnection = false
};
static le_event_Id_t TestEventId = NULL;
static NadRebootScenarioContext_t NadRebootScenario;

/* Monitor state is accessed only from the daemon main event thread. */
static WakeupMonitorContext_t WakeupMonitor =
{
    .handlerRef = NULL,
    .wakeSourceRef = NULL,
    .mode = TAF_MNGDPMTEST_WAKEUP_MONITOR_OBSERVE,
    .lockHeld = false
};
/* Resume-control state is accessed only from the daemon main event thread. */
static ResumeControlContext_t ResumeControl =
{
    .wakeSourceRef = NULL,
    .lockHeld = false
};

static const char I001HelpDoc[] =
    "Preconditions:\n"
    "  - tafMngdPMSvc is running and accepts a test-owned connection.\n"
    "  - The PM VHAL is available and accepts NAD reboot requests.\n"
    "  - No control-owned wake source, wakeup monitor, or pending power request is active.\n"
    "\n"
    "Key steps:\n"
    "  - Register a main-thread state handler that acknowledges every notification.\n"
    "  - Start two worker threads with independent MPMS client sessions.\n"
    "  - Register one all-state handler and acquire one wake source in each worker session.\n"
    "  - Wait for the main handler to receive and acknowledge RESUME.\n"
    "  - Print worker state notifications; acknowledge only RESTART_PREPARE.\n"
    "  - Acknowledge RESTART_PREPARE from each worker when the notification arrives in time.\n"
    "  - Request a graceful SUSPEND transition for the primary node.\n"
    "  - Immediately request NAD_REBOOT before the SUSPEND transition completes.\n"
    "\n"
    "Expected results:\n"
    "  - The main handler observes and acknowledges RESUME before the requests are issued.\n"
    "  - No active SUSPEND_PREPARE notification is forwarded while worker wake sources are held.\n"
    "  - The graceful SUSPEND request does not prevent NAD_REBOOT from proceeding.\n"
    "  - The restart callback reports READY and LE_OK if the target remains available.\n"
    "  - Test teardown is deferred after a successful restart callback so all handlers can\n"
    "    acknowledge RESTART_PREPARE before the target reboots.\n"
    "  - RESTART_PREPARE may be visible in simulation logs but may be missed on a real target\n"
    "    because the target can reboot immediately; missing that log is not a test failure.\n";

static le_result_t Start_I001
(
    void
);

static le_result_t TeardownNadRebootScenario
(
    char* reason,
    size_t reasonSize
);

static const TestCase_t TestCases[] =
{
    {
        .id = "MPMS_I001",
        .helpDoc = I001HelpDoc,
        .start = Start_I001,
        .teardown = TeardownNadRebootScenario,
        .timeoutMs = 30000
    },
};
static const size_t TestCaseCount = sizeof(TestCases) / sizeof(TestCases[0]);

static const char* CurrentTestId
(
    void
)
{
    return ActiveTestCase != NULL ? ActiveTestCase->id : "MPMS";
}

static void SetReason
(
    char* reason,
    size_t reasonSize,
    const char* format,
    ...
)
__attribute__((format(printf, 3, 4)));

static void SetReason
(
    char* reason,
    size_t reasonSize,
    const char* format,
    ...
)
{
    va_list args;

    if (reason == NULL || reasonSize == 0)
    {
        return;
    }

    va_start(args, format);
    vsnprintf(reason, reasonSize, format, args);
    va_end(args);
}

/* Keep child-destructor registration adjacent to thread start to avoid a cancellation window. */
static le_result_t TestThread_Start
(
    TestThread_t* threadPtr,
    const char* name,
    le_thread_MainFunc_t mainFunc,
    void* contextPtr,
    le_thread_Destructor_t destructor
)
{
    if (threadPtr == NULL || mainFunc == NULL || destructor == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    threadPtr->threadRef = le_thread_Create(name, mainFunc, contextPtr);
    if (threadPtr->threadRef == NULL)
    {
        return LE_FAULT;
    }

    le_thread_AddChildDestructor(threadPtr->threadRef, destructor, contextPtr);
    le_thread_SetJoinable(threadPtr->threadRef);
    le_thread_Start(threadPtr->threadRef);
    return LE_OK;
}

static le_result_t TestThread_Stop
(
    TestThread_t* threadPtr
)
{
    if (threadPtr == NULL || threadPtr->threadRef == NULL)
    {
        return LE_OK;
    }

    le_result_t stopResult = le_thread_Cancel(threadPtr->threadRef);
    if (stopResult == LE_NOT_FOUND)
    {
        stopResult = LE_OK;
    }

    le_result_t joinResult = le_thread_Join(threadPtr->threadRef, NULL);
    threadPtr->threadRef = NULL;

    return stopResult != LE_OK ? stopResult : joinResult;
}

static const char* ResponseModeToString
(
    taf_mngdPm_ResponseMode_t responseMode
)
{
    switch (responseMode)
    {
        case TAF_MNGDPM_READY:
            return "READY";
        case TAF_MNGDPM_NOT_READY:
            return "NOT_READY";
        case TAF_MNGDPM_TIMEOUT:
            return "TIMEOUT";
        case TAF_MNGDPM_INVALID_REQUEST:
            return "INVALID_REQUEST";
        default:
            return "UNKNOWN";
    }
}

static const char* PowerStateToString
(
    taf_mngdPm_NodePowerState_t state
)
{
    switch (state)
    {
        case TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE:
            return "SHUTDOWN_PREPARE";
        case TAF_MNGDPM_NODE_STATE_RESTART_PREPARE:
            return "RESTART_PREPARE";
        case TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE:
            return "SUSPEND_PREPARE";
        case TAF_MNGDPM_NODE_STATE_RESUME:
            return "RESUME";
        default:
            return "UNKNOWN";
    }
}

static const char* BubStatusToString
(
    int32_t status
)
{
    switch (status)
    {
        case TAF_MNGDPM_BUB_STATUS_IN_USE:
            return "IN_USE";
        case TAF_MNGDPM_BUB_STATUS_NOT_IN_USE:
            return "NOT_IN_USE";
        case TAF_MNGDPM_BUB_STATUS_UNKNOWN:
            return "UNKNOWN";
        default:
            return "INVALID";
    }
}

static const TestCase_t* FindTestCase
(
    const char* testId
)
{
    if (testId == NULL)
    {
        return NULL;
    }

    for (size_t index = 0; index < TestCaseCount; index++)
    {
        if (strcmp(TestCases[index].id, testId) == 0)
        {
            return &TestCases[index];
        }
    }

    return NULL;
}

static le_result_t EnsureConnected
(
    char* reason,
    size_t reasonSize
)
{
    if (ControlContext.connectionState != CONTROL_CONNECTION_CONNECTED)
    {
        SetReason(reason, reasonSize, "MPMS service is disconnected; run connect first");
        return LE_UNAVAILABLE;
    }

    return LE_OK;
}

static le_result_t EnsureControlAvailable
(
    const char* command,
    char* reason,
    size_t reasonSize
)
{
    if (TestRunning || TestTeardownDeferred)
    {
        SetReason(reason, reasonSize,
                  TestTeardownDeferred
                      ? "%s is unavailable while restart is pending"
                      : "%s is unavailable while an integration test is running",
                  command);
        return LE_BUSY;
    }

    le_result_t result = EnsureConnected(reason, reasonSize);

    if (result != LE_OK)
    {
        return result;
    }

    if (ControlContext.pendingOperation.type != CONTROL_PENDING_NONE)
    {
        SetReason(reason, reasonSize, "%s is unavailable while a power request is pending",
                  command);
        return LE_BUSY;
    }

    return LE_OK;
}

static le_result_t EnsureTestStartAvailable
(
    char* reason,
    size_t reasonSize
)
{
    if (ControlContext.connectionState == CONTROL_CONNECTION_CONNECTED)
    {
        SetReason(reason, reasonSize,
                  "test cannot start while the control connection is active; disconnect first");
        return LE_NOT_PERMITTED;
    }

    if (ControlContext.pendingOperation.type != CONTROL_PENDING_NONE)
    {
        SetReason(reason, reasonSize,
                  "test cannot start while a control power request is pending");
        return LE_BUSY;
    }

    if (ResumeControl.wakeSourceRef != NULL || WakeupMonitor.handlerRef != NULL)
    {
        SetReason(reason, reasonSize,
                  "test cannot start while a control-owned wake source or monitor is active");
        return LE_BUSY;
    }

    return LE_OK;
}

static le_result_t EnsureTestConnected
(
    char* reason,
    size_t reasonSize
)
{
    if (ControlContext.connectionState == CONTROL_CONNECTION_CONNECTED)
    {
        SetReason(reason, reasonSize,
                  "test cannot use the control connection; disconnect first");
        return LE_NOT_PERMITTED;
    }

    if (TestService.connected)
    {
        return LE_OK;
    }

    le_result_t result = taf_mngdPm_TryConnectService();
    if (result != LE_OK)
    {
        SetReason(reason, reasonSize, "test failed to connect to tafMngdPMSvc: %s",
                  LE_RESULT_TXT(result));
        return result;
    }

    TestService.connected = true;
    TestService.ownsConnection = true;
    return LE_OK;
}

static void ReleaseTestConnection
(
    void
)
{
    if (TestService.ownsConnection && TestService.connected)
    {
        taf_mngdPm_DisconnectService();
    }

    TestService.connected = false;
    TestService.ownsConnection = false;
}

static void CleanupMonitorWakeSource
(
    void
)
{
    if (WakeupMonitor.lockHeld && WakeupMonitor.wakeSourceRef != NULL)
    {
        le_result_t result = taf_mngdPm_Relax(WakeupMonitor.wakeSourceRef);
        if (result != LE_OK)
        {
            LE_ERROR("Failed to release monitor wake source: %s", LE_RESULT_TXT(result));
        }
    }

    WakeupMonitor.lockHeld = false;

    if (WakeupMonitor.wakeSourceRef != NULL)
    {
        le_result_t result = taf_mngdPm_DeleteWakeupSource(WakeupMonitor.wakeSourceRef);
        if (result != LE_OK)
        {
            LE_ERROR("Failed to delete monitor wake source: %s", LE_RESULT_TXT(result));
        }
        WakeupMonitor.wakeSourceRef = NULL;
    }
}

static void CleanupResumeWakeSource
(
    void
)
{
    if (ResumeControl.lockHeld && ResumeControl.wakeSourceRef != NULL)
    {
        le_result_t result = taf_mngdPm_Relax(ResumeControl.wakeSourceRef);
        if (result != LE_OK)
        {
            LE_ERROR("Failed to release resume wake source: %s", LE_RESULT_TXT(result));
            return;
        }

        ResumeControl.lockHeld = false;
    }

    if (ResumeControl.wakeSourceRef != NULL)
    {
        le_result_t result = taf_mngdPm_DeleteWakeupSource(ResumeControl.wakeSourceRef);
        if (result != LE_OK)
        {
            LE_ERROR("Failed to delete resume wake source: %s", LE_RESULT_TXT(result));
        }
        else
        {
            ResumeControl.wakeSourceRef = NULL;
        }
    }
}

static le_result_t AcquireResumeWakeSource
(
    char* reason,
    size_t reasonSize
)
{
    if (ResumeControl.wakeSourceRef == NULL)
    {
        ResumeControl.wakeSourceRef = taf_mngdPm_CreateWakeupSource(
            TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
            TAF_MNGDPM_WS_OPT_DEFAULT,
            "mpmsIntgTest.resume");
        if (ResumeControl.wakeSourceRef == NULL)
        {
            SetReason(reason, reasonSize, "failed to create resume wake source");
            return LE_FAULT;
        }
    }

    if (ResumeControl.lockHeld)
    {
        return LE_OK;
    }

    le_result_t result = taf_mngdPm_StayAwake(ResumeControl.wakeSourceRef);
    if (result != LE_OK)
    {
        SetReason(reason, reasonSize, "StayAwake for resume failed: %s",
                  LE_RESULT_TXT(result));
        return result;
    }

    ResumeControl.lockHeld = true;
    return LE_OK;
}

static le_result_t ReleaseResumeWakeSource
(
    char* reason,
    size_t reasonSize
)
{
    if (ResumeControl.wakeSourceRef == NULL || !ResumeControl.lockHeld)
    {
        return LE_OK;
    }

    le_result_t result = taf_mngdPm_Relax(ResumeControl.wakeSourceRef);
    if (result != LE_OK)
    {
        SetReason(reason, reasonSize, "Relax for suspend failed: %s",
                  LE_RESULT_TXT(result));
        return result;
    }

    ResumeControl.lockHeld = false;
    result = taf_mngdPm_DeleteWakeupSource(ResumeControl.wakeSourceRef);
    if (result != LE_OK)
    {
        SetReason(reason, reasonSize, "delete resume wake source failed: %s",
                  LE_RESULT_TXT(result));
        return result;
    }

    ResumeControl.wakeSourceRef = NULL;
    return LE_OK;
}

static void StopMonitorInternal
(
    void
)
{
    if (WakeupMonitor.handlerRef != NULL)
    {
        taf_mngdPm_RemoveNodeModemAwakeHandler(WakeupMonitor.handlerRef);
        WakeupMonitor.handlerRef = NULL;
    }

    CleanupMonitorWakeSource();
    WakeupMonitor.mode = TAF_MNGDPMTEST_WAKEUP_MONITOR_OBSERVE;
}

static void WakeupMonitorCallback
(
    taf_mngdPm_NodeModemAwakeEventRef_t ref,
    uint8_t pmNodeId,
    taf_mngdPm_NodeModemWsBitMask_t wsBitmask,
    void* contextPtr
)
{
    LE_UNUSED(ref);
    LE_UNUSED(contextPtr);

    LE_INFO("MPMS wakeup event: node=%u eventMask=0x%08x",
            pmNodeId, (unsigned int)wsBitmask);

    if (WakeupMonitor.mode != TAF_MNGDPMTEST_WAKEUP_MONITOR_KEEP_RESUME ||
        WakeupMonitor.lockHeld)
    {
        return;
    }

    if (WakeupMonitor.wakeSourceRef == NULL)
    {
        WakeupMonitor.wakeSourceRef = taf_mngdPm_CreateWakeupSource(
            TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
            TAF_MNGDPM_WS_OPT_DEFAULT,
            "mpmsIntgTest.monitor");
        if (WakeupMonitor.wakeSourceRef == NULL)
        {
            LE_ERROR("Unable to create monitor wake source after wakeup event");
            return;
        }
    }

    le_result_t result = taf_mngdPm_StayAwake(WakeupMonitor.wakeSourceRef);
    if (result == LE_OK)
    {
        WakeupMonitor.lockHeld = true;
        LE_INFO("MPMS wakeup monitor acquired a wake source to keep RESUME");
    }
    else
    {
        LE_ERROR("MPMS wakeup monitor failed to keep RESUME: %s", LE_RESULT_TXT(result));
    }
}

static void CompletePendingOperation
(
    le_result_t result,
    const char* reason
)
{
    ControlPendingOperation_t operation = ControlContext.pendingOperation;

    memset(&ControlContext.pendingOperation, 0, sizeof(ControlContext.pendingOperation));
    ControlContext.pendingOperation.type = CONTROL_PENDING_NONE;

    if (operation.commandRef == NULL)
    {
        LE_WARN("Power request completed after its control client disconnected: %s",
                reason != NULL ? reason : LE_RESULT_TXT(result));
        return;
    }

    if (operation.type == CONTROL_PENDING_SHUTDOWN)
    {
        taf_mngdPmTest_shutdownRespond(operation.commandRef, result,
                                      reason != NULL ? reason : "");
    }
    else if (operation.type == CONTROL_PENDING_RESTART)
    {
        taf_mngdPmTest_restartRespond(operation.commandRef, result,
                                     reason != NULL ? reason : "");
    }
}

static void ShutdownResponseCallback
(
    taf_mngdPm_ShutdownMode_t requestedMode,
    taf_mngdPm_ResponseMode_t responseMode,
    le_result_t result,
    void* contextPtr
)
{
    LE_UNUSED(requestedMode);
    LE_UNUSED(contextPtr);

    if (ControlContext.pendingOperation.type != CONTROL_PENDING_SHUTDOWN)
    {
        LE_WARN("Unexpected shutdown response: mode=%s result=%s",
                ResponseModeToString(responseMode), LE_RESULT_TXT(result));
        return;
    }

    if (result == LE_OK && responseMode != TAF_MNGDPM_READY)
    {
        result = LE_FAULT;
    }

    char reason[REASON_LEN] = {0};
    SetReason(reason, sizeof(reason), "shutdown response=%s result=%s",
              ResponseModeToString(responseMode), LE_RESULT_TXT(result));
    CompletePendingOperation(result, reason);
}

static void RestartResponseCallback
(
    taf_mngdPm_RestartMode_t restartMode,
    taf_mngdPm_ResponseMode_t responseMode,
    le_result_t result,
    void* contextPtr
)
{
    LE_UNUSED(restartMode);
    LE_UNUSED(contextPtr);

    if (ControlContext.pendingOperation.type != CONTROL_PENDING_RESTART)
    {
        LE_WARN("Unexpected restart response: mode=%s result=%s",
                ResponseModeToString(responseMode), LE_RESULT_TXT(result));
        return;
    }

    if (result == LE_OK && responseMode != TAF_MNGDPM_READY)
    {
        result = LE_FAULT;
    }

    char reason[REASON_LEN] = {0};
    SetReason(reason, sizeof(reason), "restart response=%s result=%s",
              ResponseModeToString(responseMode), LE_RESULT_TXT(result));
    CompletePendingOperation(result, reason);
}

static void ReportTestResultAndDeferTeardown
(
    le_result_t result,
    const char* reason
)
{
    taf_mngdPmTest_ServerCmdRef_t cmdRef = TestCmdRef;

    if (!TestRunning || TestCompleting)
    {
        return;
    }

    TestCompleting = true;

    if (TestTimer != NULL)
    {
        le_timer_Stop(TestTimer);
    }

    /*
     * A successful restart response only completes the prepare phase. Keep all
     * test-owned handlers and wake sources alive until the target reboots.
     */
    TestTeardownDeferred = true;
    TestRunning = false;
    TestCmdRef = NULL;
    TestSessionRef = NULL;
    TestCompleting = false;

    if (cmdRef != NULL)
    {
        taf_mngdPmTest_RunTestRespond(cmdRef, result, reason != NULL ? reason : "");
    }
}

static void CompleteTest
(
    le_result_t result,
    const char* reason
)
{
    const TestCase_t* testCasePtr = ActiveTestCase;
    taf_mngdPmTest_ServerCmdRef_t cmdRef = TestCmdRef;
    char finalReason[REASON_LEN] = {0};
    char teardownReason[REASON_LEN] = {0};
    le_result_t teardownResult = LE_OK;

    if (!TestRunning || TestCompleting)
    {
        return;
    }

    TestCompleting = true;

    if (TestTimer != NULL)
    {
        le_timer_Stop(TestTimer);
    }

    if (testCasePtr != NULL && testCasePtr->teardown != NULL)
    {
        teardownResult = testCasePtr->teardown(teardownReason, sizeof(teardownReason));
    }

    ReleaseTestConnection();

    if (reason != NULL)
    {
        le_utf8_Copy(finalReason, reason, sizeof(finalReason), NULL);
    }

    if (teardownResult != LE_OK)
    {
        if (finalReason[0] != '\0')
        {
            size_t used = strlen(finalReason);
            if (used < sizeof(finalReason) - 1)
            {
                const char* cleanupReason = teardownReason[0] != '\0'
                    ? teardownReason : LE_RESULT_TXT(teardownResult);
                finalReason[used++] = ';';
                finalReason[used] = '\0';
                le_utf8_Copy(finalReason + used, " teardown failed: ",
                             sizeof(finalReason) - used, NULL);
                used = strlen(finalReason);
                le_utf8_Copy(finalReason + used, cleanupReason,
                             sizeof(finalReason) - used, NULL);
            }
        }
        else
        {
            SetReason(finalReason, sizeof(finalReason),
                      "teardown failed: %s",
                      teardownReason[0] != '\0'
                          ? teardownReason : LE_RESULT_TXT(teardownResult));
        }

        LE_ERROR("%s teardown failed: %s", CurrentTestId(),
                 teardownReason[0] != '\0'
                     ? teardownReason : LE_RESULT_TXT(teardownResult));
    }

    /* Keep the test busy until every test-owned resource has been cleaned up. */
    TestRunning = false;
    TestTeardownDeferred = false;
    TestCompleting = false;
    ActiveTestCase = NULL;
    TestCmdRef = NULL;
    TestSessionRef = NULL;

    if (cmdRef != NULL)
    {
        taf_mngdPmTest_RunTestRespond(cmdRef, result, finalReason);
    }
}

#define ALL_NODE_POWER_STATE_MASK \
    (TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE | \
     TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE | \
     TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE | \
     TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME)

static const char* NadRebootWakeSourceTags[2] =
{
    "mpmsIntgTest.restart.ws1",
    "mpmsIntgTest.restart.ws2"
};

static void TestEventHandler
(
    void* payloadPtr
);

static void TryStartNadReboot
(
    void
);

static void PrepareNadRebootWorker
(
    NadRebootWorkerContext_t* workerPtr
);

static void ReportTestEvent
(
    int eventType,
    NadRebootWorkerContext_t* workerPtr,
    le_result_t result,
    const char* reason
)
{
    TestEvent_t event =
    {
        .type = eventType,
        .generation = workerPtr->generation,
        .sourceId = workerPtr->thread.threadId,
        .result = result,
        .reason = {0}
    };

    if (reason != NULL)
    {
        le_utf8_Copy(event.reason, reason, sizeof(event.reason), NULL);
    }

    le_event_Report(TestEventId, &event, sizeof(event));
}

static void CleanupNadRebootWorker
(
    void* contextPtr
)
{
    NadRebootWorkerContext_t* workerPtr = contextPtr;

    if (workerPtr->stateHandlerRef != NULL)
    {
        taf_mngdPm_RemoveNodePowerStateChangeHandler(workerPtr->stateHandlerRef);
        workerPtr->stateHandlerRef = NULL;
    }

    if (workerPtr->wakeSourceRef != NULL)
    {
        if (workerPtr->wakeSourceHeld)
        {
            le_result_t result = taf_mngdPm_Relax(workerPtr->wakeSourceRef);
            if (result != LE_OK)
            {
                char reason[REASON_LEN] = {0};
                SetReason(reason, sizeof(reason), "failed to relax %s: %s",
                          workerPtr->wakeSourceTag, LE_RESULT_TXT(result));
                workerPtr->cleanupResult = result;
                le_utf8_Copy(workerPtr->cleanupReason, reason,
                             sizeof(workerPtr->cleanupReason), NULL);
                LE_ERROR("%s: %s", workerPtr->testId, reason);
            }
            workerPtr->wakeSourceHeld = false;
        }

        le_result_t result = taf_mngdPm_DeleteWakeupSource(workerPtr->wakeSourceRef);
        if (result != LE_OK)
        {
            char reason[REASON_LEN] = {0};
            SetReason(reason, sizeof(reason), "failed to delete %s: %s",
                      workerPtr->wakeSourceTag, LE_RESULT_TXT(result));
            if (workerPtr->cleanupResult == LE_OK)
            {
                workerPtr->cleanupResult = result;
                le_utf8_Copy(workerPtr->cleanupReason, reason,
                             sizeof(workerPtr->cleanupReason), NULL);
            }
            LE_ERROR("%s: %s", workerPtr->testId, reason);
        }
        workerPtr->wakeSourceRef = NULL;
    }

    if (workerPtr->connected)
    {
        taf_mngdPm_DisconnectService();
        workerPtr->connected = false;
    }
}

static void* NadRebootWorkerMain
(
    void* contextPtr
)
{
    NadRebootWorkerContext_t* workerPtr = contextPtr;

    PrepareNadRebootWorker(workerPtr);
    le_event_RunLoop();
}

static void ReportNadRebootWorkerFailure
(
    NadRebootWorkerContext_t* workerPtr,
    le_result_t result,
    const char* reason
)
{
    ReportTestEvent(TEST_EVENT_FAILURE,
                             workerPtr,
                             result,
                             reason);
}

static void FailNadRebootTest
(
    le_result_t result,
    const char* reason
)
{
    CompleteTest(result, reason);
}

static void NadRebootResponseCallback
(
    taf_mngdPm_RestartMode_t restartMode,
    taf_mngdPm_ResponseMode_t responseMode,
    le_result_t result,
    void* contextPtr
)
{
    LE_UNUSED(contextPtr);

    /*
     * This callback is only the service's restart-prepare result.  The service may invoke
     * the actual restart immediately after this callback returns, so the target can reboot
     * before this daemon sends the RunTest response or reaches teardown.  The log records
     * below are therefore the authoritative result for these reboot integration cases.
     */

    LE_INFO("%s restart response: mode=%s response=%s result=%s",
            CurrentTestId(),
            restartMode == TAF_MNGDPM_RESTART_MODE_NAD_REBOOT
                ? "NAD_REBOOT" : "UNKNOWN",
            ResponseModeToString(responseMode), LE_RESULT_TXT(result));

    if (!TestRunning)
    {
        LE_WARN("%s received a restart response after test completion", CurrentTestId());
        return;
    }

    if (restartMode != TAF_MNGDPM_RESTART_MODE_NAD_REBOOT)
    {
        char reason[REASON_LEN] = {0};
        SetReason(reason, sizeof(reason), "%s failed: unexpected restart mode", CurrentTestId());
        FailNadRebootTest(LE_FAULT, reason);
        return;
    }

    if (result != LE_OK)
    {
        char reason[REASON_LEN] = {0};
        SetReason(reason, sizeof(reason),
                  "%s failed: restart callback returned %s",
                  CurrentTestId(), LE_RESULT_TXT(result));
        FailNadRebootTest(result, reason);
        return;
    }

    if (responseMode != TAF_MNGDPM_READY)
    {
        char reason[REASON_LEN] = {0};
        SetReason(reason, sizeof(reason),
                  "%s failed: restart callback returned %s",
                  CurrentTestId(), ResponseModeToString(responseMode));
        FailNadRebootTest(LE_FAULT, reason);
        return;
    }

    NadRebootScenario.restartReady = true;
    char reason[REASON_LEN] = {0};
    SetReason(reason, sizeof(reason),
              "%s passed: graceful SUSPEND preceded NAD_REBOOT; restart callback returned "
              "READY (SUSPEND_NTF=%s, RESTART_NTF=%s)",
              CurrentTestId(),
              NadRebootScenario.sawSuspendPrepare ? "seen" : "not-seen",
              NadRebootScenario.sawRestartPrepare ? "seen" : "not-seen");
    ReportTestResultAndDeferTeardown(LE_OK, reason);
}

static void RequestNadReboot
(
    void
)
{
    if (NadRebootScenario.restartRequestStarted)
    {
        return;
    }

    NadRebootScenario.restartRequestStarted = true;

    LE_INFO("%s all workers are READY and RESUME was observed; starting power sequence",
            CurrentTestId());
    LE_INFO("%s calling SetNodeTargetedPowerMode(SUSPEND)", CurrentTestId());

    le_result_t result = taf_mngdPm_SetNodeTargetedPowerMode(
        PRIMARY_NODE_ID, TAF_MNGDPM_SUSPEND);
    if (result != LE_OK)
    {
        char reason[REASON_LEN] = {0};
        SetReason(reason, sizeof(reason),
                  "%s failed: SetNodeTargetedPowerMode(SUSPEND) returned %s",
                  CurrentTestId(), LE_RESULT_TXT(result));
        FailNadRebootTest(result, reason);
        return;
    }

    LE_INFO("%s SetNodeTargetedPowerMode(SUSPEND) accepted; calling "
            "RestartReqAsync(NAD_REBOOT)", CurrentTestId());

    result = taf_mngdPm_RestartReqAsync(
        TAF_MNGDPM_RESTART_MODE_NAD_REBOOT,
        NadRebootResponseCallback,
        NULL,
        TAF_MNGDPM_RESTART_REASON_NORMAL);
    if (result != LE_OK)
    {
        char reason[REASON_LEN] = {0};
        SetReason(reason, sizeof(reason),
                  "%s failed: RestartReqAsync(NAD_REBOOT) returned %s",
                  CurrentTestId(), LE_RESULT_TXT(result));
        FailNadRebootTest(result, reason);
        return;
    }

    LE_INFO("%s RestartReqAsync(NAD_REBOOT) accepted", CurrentTestId());
}

static void MainNadRebootStateChangeHandler
(
    uint8_t pmNodeId,
    taf_mngdPm_nodePowerStateRef_t stateRef,
    taf_mngdPm_NodePowerState_t state,
    void* contextPtr
)
{
    NadRebootScenarioContext_t* scenarioPtr = contextPtr;

    scenarioPtr->lastState = state;
    if (state == TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE)
    {
        scenarioPtr->sawSuspendPrepare = true;
    }
    else if (state == TAF_MNGDPM_NODE_STATE_RESTART_PREPARE)
    {
        scenarioPtr->sawRestartPrepare = true;
    }

    LE_INFO("%s main state notification: node=%u state=%s (ACK)",
            CurrentTestId(), pmNodeId, PowerStateToString(state));

    le_result_t result = taf_mngdPm_SendNodePowerStateChangeAck(
        pmNodeId, stateRef, TAF_MNGDPM_CLIENT_READY);
    if (result != LE_OK)
    {
        LE_WARN("%s main ACK for %s returned %s; target may already be restarting",
                 CurrentTestId(), PowerStateToString(state), LE_RESULT_TXT(result));
    }
    else
    {
        LE_INFO("%s main acknowledged %s", CurrentTestId(), PowerStateToString(state));
    }

    if (state == TAF_MNGDPM_NODE_STATE_RESUME)
    {
        scenarioPtr->resumeSeen = true;
        LE_INFO("%s RESUME gate observed; workers READY=%zu/2",
                CurrentTestId(), scenarioPtr->readyWorkerCount);
        TryStartNadReboot();
    }
}

static void NadRebootStateChangeHandler
(
    uint8_t pmNodeId,
    taf_mngdPm_nodePowerStateRef_t stateRef,
    taf_mngdPm_NodePowerState_t state,
    void* contextPtr
)
{
    NadRebootWorkerContext_t* workerPtr = contextPtr;

    LE_INFO("%s %s state notification: node=%u state=%s",
            workerPtr->testId, workerPtr->wakeSourceTag, pmNodeId,
            PowerStateToString(state));

    if (state == TAF_MNGDPM_NODE_STATE_RESTART_PREPARE)
    {
        le_result_t result = taf_mngdPm_SendNodePowerStateChangeAck(
            pmNodeId, stateRef, TAF_MNGDPM_CLIENT_READY);
        if (result != LE_OK)
        {
            LE_WARN("%s %s ACK for %s returned %s; target may already be restarting",
                    workerPtr->testId, workerPtr->wakeSourceTag,
                    PowerStateToString(state), LE_RESULT_TXT(result));
        }
        else
        {
            LE_INFO("%s %s acknowledged %s",
                    workerPtr->testId, workerPtr->wakeSourceTag, PowerStateToString(state));
        }
    }
}

static void TestEventHandler
(
    void* payloadPtr
)
{
    TestEvent_t* eventPtr = payloadPtr;

    if (!TestRunning)
    {
        return;
    }

    if (eventPtr->generation != NadRebootScenario.generation)
    {
        LE_DEBUG("Ignoring stale restart worker event from generation %u",
                 eventPtr->generation);
        return;
    }

    if (eventPtr->sourceId >= NUM_ARRAY_MEMBERS(NadRebootScenario.workers))
    {
        return;
    }

    switch (eventPtr->type)
    {
        case TEST_EVENT_READY:
            NadRebootScenario.readyWorkerCount++;
            LE_INFO("%s main observed worker %u READY (%zu/2)",
                    CurrentTestId(), eventPtr->sourceId,
                    NadRebootScenario.readyWorkerCount);
            TryStartNadReboot();
            break;

        case TEST_EVENT_FAILURE:
            CompleteTest(eventPtr->result, eventPtr->reason);
            break;
    }
}

static void PrepareNadRebootWorker
(
    NadRebootWorkerContext_t* workerPtr
)
{
    LE_INFO("%s %s connecting to tafMngdPMSvc",
            workerPtr->testId, workerPtr->wakeSourceTag);
    taf_mngdPm_ConnectService();
    workerPtr->connected = true;

    LE_INFO("%s %s registering all-state handler",
            workerPtr->testId, workerPtr->wakeSourceTag);
    workerPtr->stateHandlerRef = taf_mngdPm_AddNodePowerStateChangeHandler(
        NadRebootStateChangeHandler,
        workerPtr,
        PRIMARY_NODE_ID,
        ALL_NODE_POWER_STATE_MASK);
    if (workerPtr->stateHandlerRef == NULL)
    {
        char reason[REASON_LEN] = {0};
        SetReason(reason, sizeof(reason),
                  "%s failed: AddNodePowerStateChangeHandler returned NULL",
                  workerPtr->testId);
        ReportNadRebootWorkerFailure(workerPtr, LE_FAULT, reason);
        return;
    }

    LE_INFO("%s %s creating wake source",
            workerPtr->testId, workerPtr->wakeSourceTag);
    workerPtr->wakeSourceRef = taf_mngdPm_CreateWakeupSource(
        TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
        TAF_MNGDPM_WS_OPT_DEFAULT,
        workerPtr->wakeSourceTag);
    if (workerPtr->wakeSourceRef == NULL)
    {
        char reason[REASON_LEN] = {0};
        SetReason(reason, sizeof(reason),
                  "%s failed: CreateWakeupSource returned NULL", workerPtr->testId);
        ReportNadRebootWorkerFailure(workerPtr, LE_FAULT, reason);
        return;
    }

    LE_INFO("%s %s calling StayAwake",
            workerPtr->testId, workerPtr->wakeSourceTag);
    le_result_t result = taf_mngdPm_StayAwake(workerPtr->wakeSourceRef);
    if (result != LE_OK)
    {
        char reason[REASON_LEN] = {0};
        SetReason(reason, sizeof(reason),
                  "%s %s failed: StayAwake returned %s",
                  workerPtr->testId, workerPtr->wakeSourceTag, LE_RESULT_TXT(result));
        ReportNadRebootWorkerFailure(workerPtr, result, reason);
        return;
    }
    workerPtr->wakeSourceHeld = true;

    LE_INFO("%s %s connected, registered handler, and acquired wake source",
            workerPtr->testId, workerPtr->wakeSourceTag);

    ReportTestEvent(TEST_EVENT_READY,
                             workerPtr,
                             LE_OK,
                             NULL);
}

static void TryStartNadReboot
(
    void
)
{
    if (!TestRunning || NadRebootScenario.restartRequestStarted ||
        NadRebootScenario.readyWorkerCount < NUM_ARRAY_MEMBERS(NadRebootScenario.workers) ||
        !NadRebootScenario.resumeSeen)
    {
        return;
    }

    RequestNadReboot();
}

static le_result_t StartNadRebootScenario
(
    void
)
{
    memset(&NadRebootScenario, 0, sizeof(NadRebootScenario));
    NadRebootScenario.generation = ActiveTestGeneration;

    LE_INFO("%s registering main all-state handler", CurrentTestId());
    NadRebootScenario.mainStateHandlerRef = taf_mngdPm_AddNodePowerStateChangeHandler(
        MainNadRebootStateChangeHandler,
        &NadRebootScenario,
        PRIMARY_NODE_ID,
        ALL_NODE_POWER_STATE_MASK);
    if (NadRebootScenario.mainStateHandlerRef == NULL)
    {
        LE_ERROR("%s failed: unable to register main state handler", CurrentTestId());
        return LE_FAULT;
    }
    LE_INFO("%s main all-state handler registered", CurrentTestId());

    for (size_t index = 0;
         index < NUM_ARRAY_MEMBERS(NadRebootScenario.workers);
         index++)
    {
        NadRebootWorkerContext_t* workerPtr = &NadRebootScenario.workers[index];
        workerPtr->thread.threadId = index;
        workerPtr->testId = CurrentTestId();
        workerPtr->generation = ActiveTestGeneration;
        workerPtr->wakeSourceTag = NadRebootWakeSourceTags[index];
    }

    for (size_t index = 0;
         index < NUM_ARRAY_MEMBERS(NadRebootScenario.workers);
         index++)
    {
        char threadName[16] = {0};
        snprintf(threadName, sizeof(threadName), "MPMS_T%u", (unsigned int)(index + 1));

        NadRebootWorkerContext_t* workerPtr = &NadRebootScenario.workers[index];
        LE_INFO("%s starting worker %u", CurrentTestId(), (unsigned int)(index + 1));
        le_result_t result = TestThread_Start(
            &workerPtr->thread,
            threadName,
            NadRebootWorkerMain,
            workerPtr,
            CleanupNadRebootWorker);
        if (result != LE_OK)
        {
            char reason[REASON_LEN] = {0};
            SetReason(reason, sizeof(reason), "%s failed: unable to create worker %u",
                      CurrentTestId(), (unsigned int)(index + 1));
            CompleteTest(LE_FAULT, reason);
            return LE_OK;
        }
    }

    return LE_OK;
}

static le_result_t Start_I001
(
    void
)
{
    return StartNadRebootScenario();
}

static le_result_t TeardownNadRebootScenario
(
    char* reason,
    size_t reasonSize
)
{
    le_result_t teardownResult = LE_OK;

    if (NadRebootScenario.mainStateHandlerRef != NULL)
    {
        taf_mngdPm_RemoveNodePowerStateChangeHandler(
            NadRebootScenario.mainStateHandlerRef);
        NadRebootScenario.mainStateHandlerRef = NULL;
    }

    for (size_t index = 0;
         index < NUM_ARRAY_MEMBERS(NadRebootScenario.workers);
         index++)
    {
        NadRebootWorkerContext_t* workerPtr = &NadRebootScenario.workers[index];

        if (workerPtr->thread.threadRef != NULL)
        {
            le_result_t stopResult = TestThread_Stop(&workerPtr->thread);
            if (stopResult != LE_OK)
            {
                teardownResult = stopResult;
                SetReason(reason, reasonSize, "worker %u stop/join failed: %s",
                          (unsigned int)(index + 1), LE_RESULT_TXT(stopResult));
            }
        }

        if (workerPtr->cleanupResult != LE_OK && teardownResult == LE_OK)
        {
            teardownResult = workerPtr->cleanupResult;
            SetReason(reason, reasonSize, "worker %u cleanup failed: %s",
                      (unsigned int)(index + 1),
                      workerPtr->cleanupReason[0] != '\0'
                          ? workerPtr->cleanupReason
                          : LE_RESULT_TXT(teardownResult));
        }
    }

    LE_INFO("NAD reboot scenario teardown complete (lastState=%s, RESUME_NTF=%s, "
            "SUSPEND_NTF=%s, RESTART_NTF=%s, restartReady=%s)",
            PowerStateToString(NadRebootScenario.lastState),
            NadRebootScenario.resumeSeen ? "seen" : "not-seen",
            NadRebootScenario.sawSuspendPrepare ? "seen" : "not-seen",
            NadRebootScenario.sawRestartPrepare ? "seen" : "not-seen",
            NadRebootScenario.restartReady ? "true" : "false");

    return teardownResult;
}

static void TestTimeoutHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_UNUSED(timerRef);
    CompleteTest(LE_TIMEOUT, "integration test timeout");
}

static void ClientCloseHandler
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    LE_UNUSED(contextPtr);

    if (ControlContext.pendingOperation.clientSessionRef == sessionRef)
    {
        ControlContext.pendingOperation.commandRef = NULL;
        ControlContext.pendingOperation.clientSessionRef = NULL;
    }

    if (TestSessionRef == sessionRef)
    {
        LE_WARN("Integration test client disconnected; cancelling the active test");
        TestCmdRef = NULL;
        CompleteTest(LE_CLOSED, "integration test client disconnected");
    }
}

static const char* BuildHelpText
(
    void
)
{
    return
        "Usage:\n"
        "  mpmsIntgTest <caseId>\n"
        "  mpmsIntgTest help\n"
        "  mpmsIntgTest list [caseId]\n\n"
        "Control commands:\n"
        "  connect                          Connect to tafMngdPMSvc.\n"
        "  disconnect                       Disconnect from tafMngdPMSvc.\n"
        "  state                            Show the primary system power state.\n"
        "  resume                           Acquire a wake source to keep the system resumed.\n"
        "  suspend                          Release the resume wake source and target SUSPEND.\n"
        "  shutdown [graceful|forceful]     Shut down the system.\n"
        "  restart                          Forcefully restart the system.\n"
        "  getInfoReport                    Show the BUB information report.\n"
        "  setWakeupFilter <hex-mask>       Set the primary modem wakeup filter.\n"
        "  getWakeupFilter                  Show the primary modem wakeup filter.\n"
        "  getWakeupReason                  Show the last primary modem wakeup reason.\n"
        "  monitorWakeup start [keepResume] Monitor wakeup events.\n"
        "  monitorWakeup release            Release the monitor wake source.\n"
        "  monitorWakeup stop               Stop monitoring and release its wake source.\n"
        "  monitorWakeup status             Show the wakeup monitor status.\n\n"
        "Control commands are unavailable while an integration test is running or restart is pending.\n"
        "Each integration test establishes and cleans up its own preconditions and resources.\n\n"
        "Test cases:\n"
        "  list                             List all test case help documents.\n"
        "  list <caseId>                    Show one test case help document.\n"
        "  <caseId>                         Run one complete integration test case.\n";
}

void taf_mngdPmTest_connect
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    size_t reasonSize
)
{
    char reason[REASON_LEN] = {0};

    if (reasonSize == 0)
    {
        taf_mngdPmTest_connectRespond(cmdRef, LE_BAD_PARAMETER, "reason buffer is empty");
        return;
    }

    if (TestRunning || TestTeardownDeferred)
    {
        taf_mngdPmTest_connectRespond(cmdRef, LE_BUSY,
                                      TestTeardownDeferred
                                          ? "connect is unavailable while restart is pending"
                                          : "connect is unavailable while an integration test is running");
        return;
    }

    if (ControlContext.connectionState == CONTROL_CONNECTION_CONNECTED)
    {
        SetReason(reason, sizeof(reason), "daemon is already connected to tafMngdPMSvc");
        taf_mngdPmTest_connectRespond(cmdRef, LE_DUPLICATE, reason);
        return;
    }

    le_result_t result = taf_mngdPm_TryConnectService();
    if (result == LE_OK)
    {
        ControlContext.connectionState = CONTROL_CONNECTION_CONNECTED;
        LE_INFO("MPMS integration daemon connected to tafMngdPMSvc");
    }
    else
    {
        SetReason(reason, sizeof(reason), "failed to connect to tafMngdPMSvc: %s",
                  LE_RESULT_TXT(result));
    }

    taf_mngdPmTest_connectRespond(cmdRef, result, reason);
}

void taf_mngdPmTest_disconnect
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    size_t reasonSize
)
{
    char reason[REASON_LEN] = {0};

    if (reasonSize == 0)
    {
        taf_mngdPmTest_disconnectRespond(cmdRef, LE_BAD_PARAMETER, "reason buffer is empty");
        return;
    }

    if (TestRunning || TestTeardownDeferred)
    {
        taf_mngdPmTest_disconnectRespond(cmdRef, LE_BUSY,
                                         TestTeardownDeferred
                                             ? "disconnect is unavailable while restart is pending"
                                             : "disconnect is unavailable while an integration test is running");
        return;
    }

    if (ControlContext.connectionState != CONTROL_CONNECTION_CONNECTED)
    {
        taf_mngdPmTest_disconnectRespond(cmdRef, LE_UNAVAILABLE,
                                      "daemon is already disconnected from tafMngdPMSvc");
        return;
    }

    if (ControlContext.pendingOperation.type != CONTROL_PENDING_NONE)
    {
        taf_mngdPmTest_disconnectRespond(cmdRef, LE_BUSY,
                                      "cannot disconnect while an operation is running");
        return;
    }

    StopMonitorInternal();
    CleanupResumeWakeSource();
    taf_mngdPm_DisconnectService();
    ControlContext.connectionState = CONTROL_CONNECTION_DISCONNECTED;
    LE_INFO("MPMS integration daemon disconnected from tafMngdPMSvc");
    taf_mngdPmTest_disconnectRespond(cmdRef, LE_OK, reason);
}

void taf_mngdPmTest_state
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    size_t powerStateSize,
    size_t reasonSize
)
{
    char powerState[STATE_LEN] = {0};
    char reason[REASON_LEN] = {0};
    taf_mngdPm_NodePowerState_t state;

    if (powerStateSize == 0 || reasonSize == 0)
    {
        taf_mngdPmTest_stateRespond(cmdRef, LE_BAD_PARAMETER, "", "");
        return;
    }

    le_result_t result = EnsureControlAvailable("state", reason, sizeof(reason));
    if (result == LE_OK)
    {
        result = taf_mngdPm_GetNodePowerState(PRIMARY_NODE_ID, &state);
        if (result == LE_OK)
        {
            le_utf8_Copy(powerState, PowerStateToString(state), sizeof(powerState), NULL);
        }
        else
        {
            SetReason(reason, sizeof(reason), "GetNodePowerState failed: %s", LE_RESULT_TXT(result));
        }
    }

    taf_mngdPmTest_stateRespond(cmdRef, result, powerState, reason);
}

void taf_mngdPmTest_resume
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    size_t reasonSize
)
{
    char reason[REASON_LEN] = {0};
    le_result_t result = EnsureControlAvailable("resume", reason, sizeof(reason));

    if (reasonSize == 0)
    {
        result = LE_BAD_PARAMETER;
        SetReason(reason, sizeof(reason), "reason buffer is empty");
    }
    else if (result == LE_OK)
    {
        result = AcquireResumeWakeSource(reason, sizeof(reason));
    }

    taf_mngdPmTest_resumeRespond(cmdRef, result, reason);
}

void taf_mngdPmTest_suspend
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    size_t reasonSize
)
{
    char reason[REASON_LEN] = {0};
    le_result_t result = EnsureControlAvailable("suspend", reason, sizeof(reason));

    if (reasonSize == 0)
    {
        result = LE_BAD_PARAMETER;
        SetReason(reason, sizeof(reason), "reason buffer is empty");
    }
    else if (result == LE_OK)
    {
        result = taf_mngdPm_SetNodeTargetedPowerMode(
            PRIMARY_NODE_ID, TAF_MNGDPM_SUSPEND);
        if (result != LE_OK)
        {
            SetReason(reason, sizeof(reason), "suspend failed: %s", LE_RESULT_TXT(result));
        }
        else
        {
            result = ReleaseResumeWakeSource(reason, sizeof(reason));
        }
    }

    taf_mngdPmTest_suspendRespond(cmdRef, result, reason);
}

void taf_mngdPmTest_shutdown
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    taf_mngdPmTest_ShutdownMode_t mode,
    size_t reasonSize
)
{
    char reason[REASON_LEN] = {0};
    le_result_t result;

    if (reasonSize == 0)
    {
        taf_mngdPmTest_shutdownRespond(cmdRef, LE_BAD_PARAMETER, "reason buffer is empty");
        return;
    }

    result = EnsureControlAvailable("shutdown", reason, sizeof(reason));
    if (result != LE_OK)
    {
        taf_mngdPmTest_shutdownRespond(cmdRef, result, reason);
        return;
    }

    if (mode != TAF_MNGDPMTEST_SHUTDOWN_GRACEFUL &&
        mode != TAF_MNGDPMTEST_SHUTDOWN_FORCEFUL)
    {
        taf_mngdPmTest_shutdownRespond(cmdRef, LE_BAD_PARAMETER, "invalid shutdown mode");
        return;
    }

    if (mode == TAF_MNGDPMTEST_SHUTDOWN_GRACEFUL)
    {
        result = taf_mngdPm_SetNodeTargetedPowerMode(
            PRIMARY_NODE_ID, TAF_MNGDPM_SHUTDOWN);
        if (result != LE_OK)
        {
            SetReason(reason, sizeof(reason), "graceful shutdown failed: %s", LE_RESULT_TXT(result));
        }
        else
        {
            result = ReleaseResumeWakeSource(reason, sizeof(reason));
        }
        taf_mngdPmTest_shutdownRespond(cmdRef, result, reason);
        return;
    }

    result = ReleaseResumeWakeSource(reason, sizeof(reason));
    if (result != LE_OK)
    {
        taf_mngdPmTest_shutdownRespond(cmdRef, result, reason);
        return;
    }

    ControlContext.pendingOperation.type = CONTROL_PENDING_SHUTDOWN;
    ControlContext.pendingOperation.commandRef = cmdRef;
    ControlContext.pendingOperation.clientSessionRef = taf_mngdPmTest_GetClientSessionRef();

    result = taf_mngdPm_ShutdownReqAsync(
        TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
        ShutdownResponseCallback,
        NULL,
        TAF_MNGDPM_SHUTDOWN_REASON_NORMAL);
    if (result != LE_OK &&
        ControlContext.pendingOperation.type == CONTROL_PENDING_SHUTDOWN)
    {
        SetReason(reason, sizeof(reason), "forceful shutdown request failed: %s",
                  LE_RESULT_TXT(result));
        CompletePendingOperation(result, reason);
    }
}

void taf_mngdPmTest_restart
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    size_t reasonSize
)
{
    char reason[REASON_LEN] = {0};
    le_result_t result;

    if (reasonSize == 0)
    {
        taf_mngdPmTest_restartRespond(cmdRef, LE_BAD_PARAMETER, "reason buffer is empty");
        return;
    }

    result = EnsureControlAvailable("restart", reason, sizeof(reason));
    if (result != LE_OK)
    {
        taf_mngdPmTest_restartRespond(cmdRef, result, reason);
        return;
    }

    result = ReleaseResumeWakeSource(reason, sizeof(reason));
    if (result != LE_OK)
    {
        taf_mngdPmTest_restartRespond(cmdRef, result, reason);
        return;
    }

    ControlContext.pendingOperation.type = CONTROL_PENDING_RESTART;
    ControlContext.pendingOperation.commandRef = cmdRef;
    ControlContext.pendingOperation.clientSessionRef = taf_mngdPmTest_GetClientSessionRef();

    result = taf_mngdPm_RestartReqAsync(
        TAF_MNGDPM_RESTART_MODE_NAD_REBOOT,
        RestartResponseCallback,
        NULL,
        TAF_MNGDPM_RESTART_REASON_NORMAL);
    if (result != LE_OK &&
        ControlContext.pendingOperation.type == CONTROL_PENDING_RESTART)
    {
        SetReason(reason, sizeof(reason), "restart request failed: %s", LE_RESULT_TXT(result));
        CompletePendingOperation(result, reason);
    }
}

void taf_mngdPmTest_getInfoReport
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    size_t reportSize,
    size_t reasonSize
)
{
    char report[REPORT_LEN] = {0};
    char reason[REASON_LEN] = {0};
    int32_t bubStatus = TAF_MNGDPM_BUB_STATUS_UNKNOWN;

    if (reportSize == 0 || reasonSize == 0)
    {
        taf_mngdPmTest_getInfoReportRespond(cmdRef, LE_BAD_PARAMETER, "", "");
        return;
    }

    le_result_t result = EnsureControlAvailable("getInfoReport", reason, sizeof(reason));
    if (result == LE_OK)
    {
        result = taf_mngdPm_GetInfoReport(TAF_MNGDPM_INFO_REPORT_BUB, &bubStatus);
        if (result == LE_OK)
        {
            le_utf8_Copy(report, BubStatusToString(bubStatus), sizeof(report), NULL);
        }
        else
        {
            SetReason(reason, sizeof(reason), "GetInfoReport(BUB) failed: %s", LE_RESULT_TXT(result));
        }
    }

    taf_mngdPmTest_getInfoReportRespond(cmdRef, result, report, reason);
}

void taf_mngdPmTest_setWakeupFilter
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    uint32_t filter,
    size_t reasonSize
)
{
    char reason[REASON_LEN] = {0};
    le_result_t result;

    if (reasonSize == 0)
    {
        taf_mngdPmTest_setWakeupFilterRespond(cmdRef, LE_BAD_PARAMETER, "reason buffer is empty");
        return;
    }

    result = EnsureControlAvailable("setWakeupFilter", reason, sizeof(reason));
    if (result == LE_OK)
    {
        result = taf_mngdPm_SetNodeModemWakeupSel(
            PRIMARY_NODE_ID, (taf_mngdPm_NodeModemWsBitMask_t)filter);
        if (result != LE_OK)
        {
            SetReason(reason, sizeof(reason), "SetNodeModemWakeupSel failed: %s",
                      LE_RESULT_TXT(result));
        }
    }

    taf_mngdPmTest_setWakeupFilterRespond(cmdRef, result, reason);
}

void taf_mngdPmTest_getWakeupFilter
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    size_t reasonSize
)
{
    char reason[REASON_LEN] = {0};
    taf_mngdPm_NodeModemWsBitMask_t filter = 0;
    le_result_t result;

    if (reasonSize == 0)
    {
        taf_mngdPmTest_getWakeupFilterRespond(cmdRef, LE_BAD_PARAMETER, 0, "reason buffer is empty");
        return;
    }

    result = EnsureControlAvailable("getWakeupFilter", reason, sizeof(reason));
    if (result == LE_OK)
    {
        result = taf_mngdPm_GetNodeModemWakeupSel(PRIMARY_NODE_ID, &filter);
        if (result != LE_OK)
        {
            SetReason(reason, sizeof(reason), "GetNodeModemWakeupSel failed: %s",
                      LE_RESULT_TXT(result));
        }
    }

    taf_mngdPmTest_getWakeupFilterRespond(cmdRef, result, (uint32_t)filter, reason);
}

void taf_mngdPmTest_getWakeupReason
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    size_t reasonSize
)
{
    char reason[REASON_LEN] = {0};
    taf_mngdPm_NodeModemWsBitMask_t reasonMask = 0;
    le_result_t result;

    if (reasonSize == 0)
    {
        taf_mngdPmTest_getWakeupReasonRespond(cmdRef, LE_BAD_PARAMETER, 0, "reason buffer is empty");
        return;
    }

    result = EnsureControlAvailable("getWakeupReason", reason, sizeof(reason));
    if (result == LE_OK)
    {
        result = taf_mngdPm_GetNodeModemAwakeReason(PRIMARY_NODE_ID, &reasonMask);
        if (result != LE_OK)
        {
            SetReason(reason, sizeof(reason), "GetNodeModemAwakeReason failed: %s",
                      LE_RESULT_TXT(result));
        }
    }

    taf_mngdPmTest_getWakeupReasonRespond(cmdRef, result, (uint32_t)reasonMask, reason);
}

void taf_mngdPmTest_monitorWakeup
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    taf_mngdPmTest_WakeupMonitorMode_t mode,
    size_t reasonSize
)
{
    char reason[REASON_LEN] = {0};
    le_result_t result;

    if (reasonSize == 0)
    {
        taf_mngdPmTest_monitorWakeupRespond(cmdRef, LE_BAD_PARAMETER, "reason buffer is empty");
        return;
    }

    result = EnsureControlAvailable("monitorWakeup", reason, sizeof(reason));
    if (result != LE_OK)
    {
        taf_mngdPmTest_monitorWakeupRespond(cmdRef, result, reason);
        return;
    }

    if (mode != TAF_MNGDPMTEST_WAKEUP_MONITOR_OBSERVE &&
        mode != TAF_MNGDPMTEST_WAKEUP_MONITOR_KEEP_RESUME)
    {
        taf_mngdPmTest_monitorWakeupRespond(cmdRef, LE_BAD_PARAMETER, "invalid monitor mode");
        return;
    }

    if (WakeupMonitor.handlerRef != NULL)
    {
        taf_mngdPmTest_monitorWakeupRespond(cmdRef, LE_DUPLICATE,
                                         "wakeup monitor is already running; stop it first");
        return;
    }

    if (mode == TAF_MNGDPMTEST_WAKEUP_MONITOR_KEEP_RESUME)
    {
        WakeupMonitor.wakeSourceRef = taf_mngdPm_CreateWakeupSource(
            TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
            TAF_MNGDPM_WS_OPT_DEFAULT,
            "mpmsIntgTest.monitor");
        if (WakeupMonitor.wakeSourceRef == NULL)
        {
            taf_mngdPmTest_monitorWakeupRespond(cmdRef, LE_FAULT,
                                             "failed to create monitor wake source");
            return;
        }
    }

    WakeupMonitor.mode = mode;
    WakeupMonitor.handlerRef = taf_mngdPm_AddNodeModemAwakeHandler(
        WakeupMonitorCallback, NULL, PRIMARY_NODE_ID, 0);
    if (WakeupMonitor.handlerRef == NULL)
    {
        CleanupMonitorWakeSource();
        WakeupMonitor.mode = TAF_MNGDPMTEST_WAKEUP_MONITOR_OBSERVE;
        taf_mngdPmTest_monitorWakeupRespond(cmdRef, LE_FAULT,
                                         "failed to register wakeup monitor");
        return;
    }

    LE_INFO("MPMS wakeup monitor started in %s mode",
            mode == TAF_MNGDPMTEST_WAKEUP_MONITOR_KEEP_RESUME ? "keepResume" : "observe");
    taf_mngdPmTest_monitorWakeupRespond(cmdRef, LE_OK, reason);
}

void taf_mngdPmTest_releaseWakeupLock
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    size_t reasonSize
)
{
    char reason[REASON_LEN] = {0};

    if (reasonSize == 0)
    {
        taf_mngdPmTest_releaseWakeupLockRespond(cmdRef, LE_BAD_PARAMETER, "reason buffer is empty");
        return;
    }

    le_result_t result = EnsureControlAvailable("releaseWakeupLock", reason, sizeof(reason));
    if (result == LE_OK && !WakeupMonitor.lockHeld)
    {
        result = LE_UNAVAILABLE;
        SetReason(reason, sizeof(reason), "monitor does not hold a wake source");
    }
    else if (result == LE_OK)
    {
        result = taf_mngdPm_Relax(WakeupMonitor.wakeSourceRef);
        if (result == LE_OK)
        {
            WakeupMonitor.lockHeld = false;
        }
        else
        {
            SetReason(reason, sizeof(reason), "Relax monitor wake source failed: %s",
                      LE_RESULT_TXT(result));
        }
    }

    taf_mngdPmTest_releaseWakeupLockRespond(cmdRef, result, reason);
}

void taf_mngdPmTest_stopWakeupMonitor
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    size_t reasonSize
)
{
    char reason[REASON_LEN] = {0};

    if (reasonSize == 0)
    {
        taf_mngdPmTest_stopWakeupMonitorRespond(cmdRef, LE_BAD_PARAMETER, "reason buffer is empty");
        return;
    }

    le_result_t result = EnsureControlAvailable("stopWakeupMonitor", reason, sizeof(reason));
    if (result == LE_OK && WakeupMonitor.handlerRef == NULL)
    {
        result = LE_NOT_FOUND;
        SetReason(reason, sizeof(reason), "wakeup monitor is not running");
    }
    else if (result == LE_OK)
    {
        StopMonitorInternal();
        LE_INFO("MPMS wakeup monitor stopped");
    }

    taf_mngdPmTest_stopWakeupMonitorRespond(cmdRef, result, reason);
}

void taf_mngdPmTest_getWakeupMonitorStatus
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    size_t statusSize,
    size_t reasonSize
)
{
    char status[MONITOR_STATUS_LEN] = {0};
    char reason[REASON_LEN] = {0};

    if (statusSize == 0 || reasonSize == 0)
    {
        taf_mngdPmTest_getWakeupMonitorStatusRespond(cmdRef, LE_BAD_PARAMETER, "", "");
        return;
    }

    le_result_t result = EnsureControlAvailable("getWakeupMonitorStatus", reason, sizeof(reason));
    if (result != LE_OK)
    {
        taf_mngdPmTest_getWakeupMonitorStatusRespond(cmdRef, result, "", reason);
        return;
    }

    const char* mode =
        WakeupMonitor.mode == TAF_MNGDPMTEST_WAKEUP_MONITOR_KEEP_RESUME
            ? "keepResume"
            : "observe";

    if (WakeupMonitor.handlerRef == NULL)
    {
        le_utf8_Copy(status, "stopped", sizeof(status), NULL);
    }
    else
    {
        snprintf(status, sizeof(status), "running mode=%s lock=%s", mode,
                 WakeupMonitor.lockHeld ? "held" : "released");
    }

    taf_mngdPmTest_getWakeupMonitorStatusRespond(cmdRef, LE_OK, status, reason);
}

void taf_mngdPmTest_RunTest
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    const char* testId,
    size_t reasonSize
)
{
    char reason[REASON_LEN] = {0};

    if (reasonSize == 0 || testId == NULL || testId[0] == '\0')
    {
        taf_mngdPmTest_RunTestRespond(cmdRef, LE_BAD_PARAMETER, "invalid test ID");
        return;
    }

    if (TestRunning || TestTeardownDeferred)
    {
        taf_mngdPmTest_RunTestRespond(cmdRef, LE_BUSY,
                                   TestTeardownDeferred
                                       ? "previous integration test is waiting for restart"
                                       : "another integration test is running");
        return;
    }

    const TestCase_t* testCasePtr = FindTestCase(testId);
    if (testCasePtr == NULL)
    {
        taf_mngdPmTest_RunTestRespond(cmdRef, LE_NOT_FOUND, "unknown integration test case");
        return;
    }

    le_result_t result = EnsureTestStartAvailable(reason, sizeof(reason));
    if (result != LE_OK)
    {
        taf_mngdPmTest_RunTestRespond(cmdRef, result, reason);
        return;
    }

    result = EnsureTestConnected(reason, sizeof(reason));
    if (result != LE_OK)
    {
        taf_mngdPmTest_RunTestRespond(cmdRef, result, reason);
        return;
    }

    ActiveTestGeneration++;
    if (ActiveTestGeneration == 0)
    {
        ActiveTestGeneration = 1;
    }

    TestRunning = true;
    TestTeardownDeferred = false;
    ActiveTestCase = testCasePtr;
    TestCmdRef = cmdRef;
    TestSessionRef = taf_mngdPmTest_GetClientSessionRef();

    if (TestTimer == NULL)
    {
        TestTimer = le_timer_Create("MpmsIntgTestTimeout");
        le_timer_SetHandler(TestTimer, TestTimeoutHandler);
    }
    le_timer_SetMsInterval(TestTimer, testCasePtr->timeoutMs > 0
                                        ? testCasePtr->timeoutMs
                                        : TEST_TIMEOUT_MS);
    le_timer_Start(TestTimer);

    LE_INFO("Starting integration test %s", testCasePtr->id);
    result = testCasePtr->start != NULL ? testCasePtr->start() : LE_FAULT;
    if (result != LE_OK)
    {
        CompleteTest(result, "integration test setup failed");
    }
}

void taf_mngdPmTest_GetHelp
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    size_t helpTextSize
)
{
    if (helpTextSize == 0)
    {
        taf_mngdPmTest_GetHelpRespond(cmdRef, LE_BAD_PARAMETER, "");
        return;
    }

    taf_mngdPmTest_GetHelpRespond(cmdRef, LE_OK, BuildHelpText());
}

void taf_mngdPmTest_GetTestCaseDoc
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    const char* testId,
    size_t helpDocSize
)
{
    const TestCase_t* testCasePtr = FindTestCase(testId);

    if (helpDocSize == 0 || testId == NULL)
    {
        taf_mngdPmTest_GetTestCaseDocRespond(cmdRef, LE_BAD_PARAMETER, "");
    }
    else if (testCasePtr == NULL)
    {
        taf_mngdPmTest_GetTestCaseDocRespond(cmdRef, LE_NOT_FOUND, "");
    }
    else
    {
        taf_mngdPmTest_GetTestCaseDocRespond(cmdRef, LE_OK, testCasePtr->helpDoc);
    }
}

void taf_mngdPmTest_GetTestCaseDocByIndex
(
    taf_mngdPmTest_ServerCmdRef_t cmdRef,
    uint32_t index,
    size_t testIdSize,
    size_t helpDocSize
)
{
    if (testIdSize == 0 || helpDocSize == 0)
    {
        taf_mngdPmTest_GetTestCaseDocByIndexRespond(cmdRef, LE_BAD_PARAMETER, "", "");
    }
    else if (index >= TestCaseCount)
    {
        taf_mngdPmTest_GetTestCaseDocByIndexRespond(cmdRef, LE_NOT_FOUND, "", "");
    }
    else
    {
        taf_mngdPmTest_GetTestCaseDocByIndexRespond(cmdRef,
                                                 LE_OK,
                                                 TestCases[index].id,
                                                 TestCases[index].helpDoc);
    }
}

COMPONENT_INIT
{
    TestEventId = le_event_CreateId("TestEvent", sizeof(TestEvent_t));
    le_event_AddHandler("TestEventHandler", TestEventId, TestEventHandler);
    le_msg_AddServiceCloseHandler(taf_mngdPmTest_GetServiceRef(), ClientCloseHandler, NULL);
    LE_INFO("MPMS integration test daemon started (disconnected)");
}
