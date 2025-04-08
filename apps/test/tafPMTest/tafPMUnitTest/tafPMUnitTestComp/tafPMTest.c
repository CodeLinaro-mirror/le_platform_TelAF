/*
 *  Copyright (c) 2022-2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"

taf_pm_WakeupSourceRef_t ws, wsRef, ws1;
taf_pm_StateChangeHandlerRef_t handlerRef;
taf_pm_StateChangeExHandlerRef_t handlerExRef;
static le_sem_Ref_t semRef;
le_result_t res;

//Function to convert taf state to string
char* tafStateToString(taf_pm_State_t tafState)
{
    char* state;
    switch(tafState) {
        case TAF_PM_STATE_RESUME:
            state = "Resume";
            break;
        case TAF_PM_STATE_SUSPEND:
            state = "Suspend";
            break;
        case  TAF_PM_STATE_SHUTDOWN:
            state = "Shutdown";
            break;
        default :
            state = "Unknown";
            break;
    }
    return state;
}

void Test_tafPM_createWakeupSource()
{
    LE_TEST_INFO("Testing creating of wake source with out ref");
    ws = taf_pm_NewWakeupSource(0, "pmtest1");
    LE_TEST_OK(ws != NULL, "taf_pm_NewWakeupSource is successfull");

    LE_TEST_INFO("Testing creating of wake source with ref");
    wsRef = taf_pm_NewWakeupSource(1, "pmtest_ref");
    LE_TEST_OK(wsRef != NULL, "taf_pm_NewWakeupSource as ref is successfull");
}

void Test_tafPM_StayAwake()
{
    LE_TEST_INFO("Testing taf_pm_StayAwake on wake source without reference");
    res = taf_pm_StayAwake(ws);
    LE_TEST_OK(res == LE_OK, "wake source without ref has acquired succesfully");

    LE_TEST_INFO("Testing taf_pm_StayAwake on already acquired wake source without reference");
    res = taf_pm_StayAwake(ws);
    LE_TEST_OK(res == LE_OK, "StayAwake on already acquired wake source success with warning");

    LE_TEST_INFO("Testing taf_pm_StayAwake on wake source with reference");
    res = taf_pm_StayAwake(wsRef);
    LE_TEST_OK(res == LE_OK, "wake source with ref is acquired successfully");

    LE_TEST_INFO("Testing taf_pm_StayAwake multiple times on wake source with reference");
    res = taf_pm_StayAwake(wsRef);
    LE_TEST_OK(res == LE_OK, "wake source with ref is acquired multiple times successfully");
}

void Test_tafPM_Relax()
{
    LE_TEST_INFO("Testing taf_pm_Relax on wake source without reference");
    res = taf_pm_Relax(ws);
    LE_TEST_OK(res == LE_OK, "wake source without ref has released succesfully");

    LE_TEST_INFO("Testing taf_pm_Relax on already released wake source without reference");
    res = taf_pm_Relax(ws);
    LE_TEST_OK(res == LE_OK, "Relax on already released wake source success with warning");

    LE_TEST_INFO("Testing taf_pm_Relax on wake source with reference");
    res = taf_pm_Relax(wsRef);
    LE_TEST_OK(res == LE_OK, "wake source with ref is released successfully");

    LE_TEST_INFO("Testing taf_pm_Relax multiple times on wake source with reference");
    res = taf_pm_Relax(wsRef);
    LE_TEST_OK(res == LE_OK, "wake source with ref is released multiple times successfully");
}

taf_pm_State_t Test_tafPM_GetState()
{
    return taf_pm_GetPowerState();
}

le_result_t WaitForSem_Timeout
(
    le_sem_Ref_t semRef,
    uint32_t seconds
)
{
    le_clk_Time_t timeToWait = {seconds, 0};
    return le_sem_WaitWithTimeOut(semRef, timeToWait);
}

//Function called on state change
void TestStateChangeHandler(taf_pm_State_t state, void* contextPtr)
{
    LE_INFO("State change triggered for %s\n", tafStateToString(state));
    printf("\nState change triggered for %s\n", tafStateToString(state));
}

//Function called on state change
void TestStateChangeExHandler(taf_pm_PowerStateRef_t powerStateRef,
        taf_pm_NadVm_t vm_id, taf_pm_State_t state, void* contextPtr)
{
    LE_TEST_INFO("State change triggered for %s\n", tafStateToString(state));
    printf("\nState change triggered for %s\n", tafStateToString(state));
    taf_pm_SendStateChangeAck(powerStateRef, state, TAF_PM_PVM, TAF_PM_READY);
    LE_INFO("Send state change acknowledge for %s\n", tafStateToString(state));
    printf("\n Send state change acknowledge for %s\n", tafStateToString(state));
}

static void* test_stateChangeHandler(void* ctxPtr)
{
    taf_pm_ConnectService();

    LE_TEST_INFO("Testing taf_pm_AddStateChangeHandler on invalid handler reference");
    handlerRef = taf_pm_AddStateChangeHandler(NULL, NULL);
    LE_TEST_OK(handlerRef != NULL, "Register state change handler with INVALID reference");

    LE_TEST_INFO("Testing taf_pm_RemoveStateChangeHandler handler reference");
    taf_pm_RemoveStateChangeHandler(handlerRef);
    LE_TEST_OK(true, "taf_pm_RemoveStateChangeHandler successfull");

    LE_TEST_INFO("Testing taf_pm_AddStateChangeHandler on valid handler reference");
    handlerRef = taf_pm_AddStateChangeHandler(TestStateChangeHandler, NULL);
    LE_TEST_OK(handlerRef != NULL,"Register state change handler is successfull");

    LE_TEST_INFO("Testing taf_pm_AddStateChangeExHandler on valid handler reference");
    handlerExRef = taf_pm_AddStateChangeExHandler(TestStateChangeExHandler, NULL);
    LE_TEST_OK(handlerExRef != NULL,"Register state change handler is successfull");
    le_sem_Post(semRef);
    le_event_RunLoop();
}

void Test_tafPM_registerListener()
{
    LE_INFO("Test_tafPM_registerListener");
    semRef = le_sem_Create("SemRef", 0);
    LE_TEST_INFO("Test register power state handler");
    le_thread_Ref_t threadRef = le_thread_Create("taf_PM_StateHandler", test_stateChangeHandler, NULL);
    le_thread_Start(threadRef);
    WaitForSem_Timeout(semRef, 5);
}

void Test_tafPM_deregisterStateChangeListener()
{
    LE_TEST_INFO("Test deregisterStateChangeListener");
    taf_pm_RemoveStateChangeHandler(handlerRef);
    LE_TEST_OK(true, "deregisterstate change listener is successfull");
}

void Test_tafPM_NewWakeupSourceDuplicateTag()
{
    LE_TEST_INFO("Testing creating of wake source with duplicate tag");
    ws = taf_pm_NewWakeupSource(0, "pmtest1");
    LE_TEST_OK(ws != NULL, "taf_pm_NewWakeupSource is successfull");

    LE_TEST_INFO("Testing creating of wake source with same tag, and the app will be killed");
    ws1 = taf_pm_NewWakeupSource(0, "pmtest1");
    LE_TEST_OK(ws1 == NULL, "taf_pm_NewWakeupSource with duplicate TAG failed successfull");
}

void Test_tafPM_StayAwakeInvalidRef()
{
    LE_TEST_INFO("Testing taf_pm_StayAwake on wake source with invalid reference, which will kill the app");
    res = taf_pm_StayAwake(ws);
    LE_TEST_OK(res == LE_BAD_PARAMETER, "Test_tafPM_StayAwakeInvalidRef successfull");
}

void Test_tafPM_RelaxInvalidRef()
{
    LE_TEST_INFO("Testing taf_pm_Relax on wake source with invalid reference, which will kill the app");
    res = taf_pm_Relax(ws);
    LE_TEST_OK(res == LE_BAD_PARAMETER, "Test_tafPM_RelaxInvalidRef successfull");
}

void Test_tafPM_RelaxOverlap()
{
    LE_TEST_INFO("Testing creating of wake source with ref");
    wsRef = taf_pm_NewWakeupSource(1, "pmtest_ref");
    LE_TEST_OK(wsRef != NULL, "taf_pm_NewWakeupSource as ref is successfull");

    LE_TEST_INFO("Testing taf_pm_StayAwake on wake source with reference");
    res = taf_pm_StayAwake(wsRef);
    LE_TEST_OK(res == LE_OK, "wake source with ref is acquired successfully");

    LE_TEST_INFO("Testing taf_pm_Relax on wake source with reference");
    res = taf_pm_Relax(wsRef);
    LE_TEST_OK(res == LE_OK, "wake source with ref is released successfully");

    LE_TEST_INFO("Testing overlap taf_pm_Relax on wake source with reference");
    res = taf_pm_Relax(wsRef);
    LE_TEST_OK(res == LE_OK, "wake source Relax on already released reference is successfully");
}

COMPONENT_INIT
{
    LE_INFO("tafPMUnitTest started");

    const char* procName = le_arg_GetProgramName();
    LE_INFO("procName is %s", procName);
    if (strcmp(procName, "proc1") == 0)
    {
        Test_tafPM_registerListener();
#if defined(TARGET_SA525M)
    le_result_t res;
    res = taf_pm_SetAllVMPowerState(TAF_PM_STATE_SUSPEND);
    if(res == LE_OK)
    {
       LE_INFO("suspend is initiated succesfully");
       WaitForSem_Timeout(semRef, 3);
    }

    res = taf_pm_SetAllVMPowerState(TAF_PM_STATE_RESUME);
    if(res == LE_OK)
       LE_INFO("Resume is initiated succesfully");
#endif
        Test_tafPM_createWakeupSource();

        Test_tafPM_StayAwake();

        LE_TEST_INFO("Testing taf_pm_GetState when wake source is acquired");
        taf_pm_State_t state = Test_tafPM_GetState();
        char* powerState = tafStateToString(state);
        LE_INFO("State is %s\n", powerState);
        printf("\n State : %s\n", powerState);
        LE_TEST_OK(state == TAF_PM_STATE_RESUME,
            "Get state is resume as at least one wake source is acquired successful");

        Test_tafPM_Relax();
#if defined(TARGET_SA515M)
        LE_TEST_INFO("Testing taf_pm_GetState when wake source is released");
        state = Test_tafPM_GetState();
        powerState = tafStateToString(state);
        LE_INFO("State is %s\n", powerState);
        printf("\n State : %s\n", powerState);
        LE_TEST_OK(state == TAF_PM_STATE_SUSPEND,
            "Get state is suspend if no wake source is acquired");
#endif
        Test_tafPM_deregisterStateChangeListener();
    }
    else if (strcmp(procName, "proc2") == 0)
    {
        Test_tafPM_NewWakeupSourceDuplicateTag();
    }
    else if (strcmp(procName, "proc3") == 0)
    {
        Test_tafPM_StayAwakeInvalidRef();
    }
    else if (strcmp(procName, "proc4") == 0)
    {
        Test_tafPM_RelaxInvalidRef();
    }
    else if (strcmp(procName, "proc5") == 0)
    {
        Test_tafPM_RelaxOverlap();
    }
    LE_TEST_EXIT;
}
