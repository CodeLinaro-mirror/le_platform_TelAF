/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

taf_pm_WakeupSourceRef_t ws, wsRef, ws1, wsBad;
taf_pm_StateChangeHandlerRef_t HandlerRef;
taf_pm_StateChangeExHandlerRef_t HandlerExRef;
le_result_t rst = LE_FAULT;
static le_event_Id_t evt;

#define stop_IF(condition, fmt, ...) \
    if ( (condition) ) \
    { \
        LE_ERROR(fmt, ##__VA_ARGS__); \
        return LE_FAULT; \
    }

#define TEST_PASS \
do { \
    LE_INFO("%s - [OK]", __FUNCTION__); \
    return LE_OK; \
} while (0)


typedef enum {
    sig_TestAddHandlers,
    sig_TestSwitchPowerState,
    sig_TestCreateWakeupSource,
    sig_TestStayAwake,
    sig_TestRelax,
    sig_TestRemoveHandlers,
    sig_Terminal,
    sig_TestDone,
} Signal_t;

typedef struct {
    Signal_t id;
} Event_t;

static inline const char *to_StateText
(
    taf_pm_State_t state
)
{
    switch (state)
    {
        case TAF_PM_STATE_RESTART: return "st(RESTART)";
        case TAF_PM_STATE_RESUME: return "st(RESUME)";
        case TAF_PM_STATE_SUSPEND: return "st(SUSPEND)";
        case TAF_PM_STATE_SHUTDOWN: return "st(SHUTDOWN)";

        case TAF_PM_STATE_ALL_ACKED:
            return "st(ALL_ACKED/internal)";
        case TAF_PM_STATE_ALL_WAKELOCKS_RELEASED:
            return "st(ALL_WAKELOCKS_RELEASED/internal)";

        case TAF_PM_STATE_UNKNOWN:
        default:
            return "st(UNKNOWN)";
    }
}

static void StateChangeHandler
(
    taf_pm_State_t state,
    void* unused
)
{
    LE_INFO("APP: state: %s", to_StateText(state));
}

static void StateChangeExHandler
(
    taf_pm_PowerStateRef_t powerStateRef,
    taf_pm_NadVm_t vm_id,
    taf_pm_State_t state,
    void* unused
)
{
    LE_INFO("APP: ex-state: %s", to_StateText(state));
}

static inline void to_Next
(
    Signal_t sig
)
{
    Event_t event = { sig };
    le_event_Report(evt, &event, sizeof(event));
}

static inline bool OK
(
    le_result_t result
)
{
    if (result != LE_OK)
    {
        Event_t event = { sig_Terminal };
        le_event_Report(evt, &event, sizeof(event));
        return false;
    }
    else
    {
        return true;
    }
}

static le_result_t Test_AddHandlers()
{
    HandlerRef = taf_pm_AddStateChangeHandler(StateChangeHandler, NULL);
    stop_IF(
        HandlerRef == NULL,
        "Failed to test taf_pm_AddStateChangeHandler");

    HandlerExRef = taf_pm_AddStateChangeExHandler(StateChangeExHandler, NULL);
    stop_IF(
        HandlerExRef == NULL,
        "Failed to test taf_pm_AddStateChangeExHandler");

    TEST_PASS;
}

static le_result_t Test_SwitchPowerState()
{
    taf_pm_State_t state;

    rst = taf_pm_SetAllVMPowerState(TAF_PM_STATE_SUSPEND);
    stop_IF(
        rst != LE_OK,
        "Failed to set power state to 'SUSPEND'"
    );

    state = taf_pm_GetPowerState();
    stop_IF(
        state != TAF_PM_STATE_SUSPEND,
        "Current state is not SUSPEND"
    );

    le_thread_Sleep(2);

    rst = taf_pm_SetAllVMPowerState(TAF_PM_STATE_RESUME);
    stop_IF(
        rst != LE_OK,
        "Failed to set power state to 'RESUME'"
    );

    state = taf_pm_GetPowerState();
    stop_IF(
        state != TAF_PM_STATE_RESUME,
        "Current state is not RESUME"
    );

    TEST_PASS;
}

static le_result_t Test_CreateWakeupSource()
{
    ws = taf_pm_NewWakeupSource(0, "pmtest1");
    stop_IF(
        ws == NULL,
        "Failed to create new ws for pmtest1"
    );

    ws1 = taf_pm_NewWakeupSource(0, "pmtest1");
    stop_IF(
        ws1 != NULL,
        "Failed to test taf_pm_NewWakeupSource"
    );

    wsRef = taf_pm_NewWakeupSource(1, "pmtest_ref");
    stop_IF(
        wsRef == NULL,
        "Failed to create new ref-ws for pmtest_ref"
    );

    TEST_PASS;
}

static le_result_t Test_StayAwake()
{
    rst = taf_pm_StayAwake(ws);
    stop_IF(
        rst != LE_OK,
        "Failed to stay ws"
    );

    rst = taf_pm_StayAwake(ws);
    stop_IF(
        rst != LE_OK,
        "Failed to stay ws again"
    );

    rst = taf_pm_StayAwake(wsRef);
    stop_IF(
        rst != LE_OK,
        "Failed to stay ref-ws"
    );

    rst = taf_pm_StayAwake(wsRef);
    stop_IF(
        rst != LE_OK,
        "Failed to stay ref-ws again"
    );

    rst = taf_pm_StayAwake(wsBad);
    stop_IF(
        rst != LE_BAD_PARAMETER,
        "Failed to test taf_pm_StayAwake with bad ws"
    );

    TEST_PASS;
}

static le_result_t Test_Relax()
{
    rst = taf_pm_Relax(ws);
    stop_IF(
        rst != LE_OK,
        "Failed to relax ref-ws"
    );

    rst = taf_pm_Relax(ws);
    stop_IF(
        rst != LE_OK,
        "Failed to relax ref-ws again "
    );

    rst = taf_pm_Relax(wsRef);
    stop_IF(
        rst != LE_OK,
        "Failed to relax ref-ws"
    );

    rst = taf_pm_Relax(wsRef);
    stop_IF(
        rst != LE_OK,
        "Failed to relax ref-ws again"
    );

    rst = taf_pm_Relax(wsBad);
    stop_IF(
        rst != LE_BAD_PARAMETER,
        "Failed to test taf_pm_Relax with bad ws"
    );

    TEST_PASS;
}

static le_result_t Test_RemoveHandlers()
{
    taf_pm_RemoveStateChangeHandler(HandlerRef);

    taf_pm_RemoveStateChangeExHandler(HandlerExRef);

    TEST_PASS;
}

static void Dispatcher
(
    void * reportPtr
)
{
    Event_t * evp = (Event_t *) reportPtr;

    switch (evp->id)
    {
        case sig_TestAddHandlers:
        {
            if (OK (Test_AddHandlers()))
            {
                to_Next(sig_TestSwitchPowerState);
            }
            // else: -> Stop to check...
        }
        break;

        case sig_TestSwitchPowerState:
        {
            if (OK (Test_SwitchPowerState()))
            {
                to_Next(sig_TestCreateWakeupSource);
            }
        }
        break;

        case sig_TestCreateWakeupSource:
        {
            if (OK (Test_CreateWakeupSource()))
            {
                to_Next(sig_TestStayAwake);
            }
        }
        break;

        case sig_TestStayAwake:
        {
            if (OK (Test_StayAwake()))
            {
                to_Next(sig_TestRelax);
            }
        }
        break;

        case sig_TestRelax:
        {
            if (OK ( Test_Relax()))
            {
                to_Next(sig_TestRemoveHandlers);
            }
        }
        break;

        case sig_TestRemoveHandlers:
        {
            if (OK (Test_RemoveHandlers()))
            {
                to_Next(sig_TestDone);
            }
        }
        break;

        case sig_TestDone:
        {
            LE_INFO("-- PMS Unit Test (SUCCESS) --");
            exit(EXIT_SUCCESS);
        }
        break;

        case sig_Terminal:
        {
            LE_ERROR("-- PMS Unit Test (FAILURE) --");
            exit(EXIT_FAILURE);
        }
    }
}

static void * Tester
(
    void * sync
)
{
    // New thread, sync-connect to PMS svc...
    taf_pm_ConnectService();

    le_event_AddHandler("evt-hdlr", evt, Dispatcher);

    Event_t event = { sig_TestAddHandlers };
    le_event_Report(evt, &event, sizeof(event));

    le_sem_Post((le_sem_Ref_t )sync);
    le_event_RunLoop();
}


COMPONENT_INIT
{
    LE_INFO(" -- PMS Unit Test (start) --");

    le_sem_Ref_t sync = le_sem_Create("sync", 0);

    evt = le_event_CreateId("event", sizeof(Event_t));

    le_thread_Ref_t thread =
        le_thread_Create(
            "pm-unit-test",
            Tester,
            sync);

    le_thread_Start(thread);

    le_clk_Time_t timeToWait = {5, 0};

    if (le_sem_WaitWithTimeOut(sync, timeToWait) == LE_TIMEOUT)
    {
        LE_ERROR("[Failed] Timeout for test init");
        exit(EXIT_FAILURE);
    }
}
