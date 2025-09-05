/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "main.h"

taf_pm_StateChangeHandlerRef_t handlerRef;
taf_pm_StateChangeExHandlerRef_t handlerExRef;

static le_sem_Ref_t semRef;

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
    if(state == TAF_PM_STATE_SHUTDOWN)
    {
        LE_INFO("Sent state change NACK for %s\n", tafStateToString(state));
        printf("\n Sent state change NACK for %s\n", tafStateToString(state));
        taf_pm_SendStateChangeAck(powerStateRef, state, TAF_PM_PVM, TAF_PM_NOT_READY);
        return;
    }
    taf_pm_SendStateChangeAck(powerStateRef, state, TAF_PM_PVM, TAF_PM_READY);
    LE_INFO("Sent state change acknowledge for %s\n", tafStateToString(state));
    printf("\n Sent state change acknowledge for %s\n", tafStateToString(state));
}

taf_pm_WakeupSourceRef_t tafPMTest_create(const char* tag)
{
    taf_pm_WakeupSourceRef_t ref = taf_pm_NewWakeupSource(0, tag);
    if(ref != NULL) {
        printf("\n created successfull\n");
        return ref;
    }
    printf("\n wakeSource Creation failed\n");
    return NULL;
}

void tafPMTest_acquire(taf_pm_WakeupSourceRef_t ref)
{
    if(taf_pm_StayAwake(ref) == LE_OK) {
        printf("\n stay awake successfull\n");
    } else {
        printf("\n stay awake failed\n");
    }
}

void tafPMTest_release(taf_pm_WakeupSourceRef_t ref)
{
    if(taf_pm_Relax(ref) == LE_OK) {
        printf("\n Release successfull\n");
    } else {
        printf("\n Release failed\n");
    }
}

//Function to get the State
le_result_t ctrlCmd_test6()
{
    taf_pm_State_t state = taf_pm_GetPowerState();
    char* powerState = tafStateToString(state);
    LE_TEST_INFO("Test get power State %s\n", powerState);
    printf("\n State : %s\n", powerState);
    LE_TEST_OK(state <= TAF_PM_STATE_SHUTDOWN && state >= TAF_PM_STATE_UNKNOWN,
            "Get power state is successfull");
    return LE_OK;
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

static void* test_stateChangeHandler(void* ctxPtr)
{
    taf_pm_ConnectService();
    handlerRef = taf_pm_AddStateChangeHandler(TestStateChangeHandler, NULL);
    printf("\nRegistered successfully for StateChangeListener\n");
    LE_ASSERT(handlerRef != NULL);

    handlerExRef = taf_pm_AddStateChangeExHandler(TestStateChangeExHandler, NULL);
    printf("\nRegistered successfully for StateChangeExListener\n");
    LE_ASSERT(handlerExRef != NULL);

    le_sem_Post(semRef);
    le_event_RunLoop();
}

le_result_t ctrlCmd_registerStateChangeListener()
{
    LE_TEST_INFO("Test registerStateChangeListener");
    handlerRef = taf_pm_AddStateChangeHandler(TestStateChangeHandler, NULL);
    printf("\n====== Register listener for state change Test ======\n"
            "ACTION : Trigger state change from telux_power_test_app and observe\n");
    LE_TEST_OK(NULL != handlerRef,"Registered successfully to listener for state change");
    return LE_OK;
}

le_result_t ctrlCmd_registerStateChangeExListener()
{
    LE_TEST_INFO("Test registerStateChangeExListener");
    handlerExRef = taf_pm_AddStateChangeExHandler(TestStateChangeExHandler, NULL);
    printf("\n====== Register listener for state change extended handler Test ======\n"
            "ACTION : Trigger state change from telux_power_test_app and observe\n");
    LE_TEST_OK(NULL != handlerExRef,"Registered successfully to listener for Ex state change");
    return LE_OK;
}

void ctrlCmd_deregisterStateChangeExListener()
{
    LE_INFO("tafPMTest_deregisterStateChangeExListener");
    taf_pm_RemoveStateChangeExHandler(handlerExRef);
    printf("\n====== Extended StateChangeListener removed successfully ======\n");
}

le_result_t ctrlCmd_deregisterListenerTest()
{
    semRef = le_sem_Create("SemRef", 0);
    LE_TEST_INFO("Test Remove power state handler");
    le_thread_Ref_t threadRef = le_thread_Create("taf_PM_StateHandler", test_stateChangeHandler, NULL);
    le_thread_Start(threadRef);
    WaitForSem_Timeout(semRef, 5);
    tafPMTest_deregisterStateChangeListener();
    LE_TEST_OK(true, "Deregistered successfully");
    return LE_OK;
}

void tafPMTest_deregisterStateChangeListener()
{
    LE_INFO("tafPMTest_deregisterStateChangeListener");
    taf_pm_RemoveStateChangeHandler(handlerRef);
    printf("\n====== StateChangeListener removed successfully ======\n");
}

static taf_pm_WakeupSourceRef_t LocalSuspendwsRef;

le_result_t ctrlCmd_test3()
{
    LE_TEST_INFO("====== Suspend test case when WL is acquired ======");
    printf("\n====== Suspend test case when WL is acquired ======\n");
    semRef = le_sem_Create("SemRef", 0);
    le_thread_Ref_t threadRef = le_thread_Create("taf_PM_StateHandler",
            test_stateChangeHandler, NULL);
    le_thread_Start(threadRef);
    WaitForSem_Timeout(semRef, 5);
    LocalSuspendwsRef = tafPMTest_create("local_suspend");
    tafPMTest_acquire(LocalSuspendwsRef);
    printf("\nACTION : Trigger suspend from telux_power_test_app and observe device will not suspend\n");
    return LE_OK;
}

le_result_t ctrlCmd_test4()
{
    LE_TEST_INFO("====== Suspend test case when WL is not acquired ======");
    printf("\n====== Suspend test case when WL is not acquired ======\n");
    semRef = le_sem_Create("SemRef", 0);
    le_thread_Ref_t threadRef = le_thread_Create("taf_PM_StateHandler",
            test_stateChangeHandler, NULL);
    le_thread_Start(threadRef);
    WaitForSem_Timeout(semRef, 5);
    taf_pm_WakeupSourceRef_t ref = tafPMTest_create("no_wl_suspend");
    tafPMTest_acquire(ref);
    sleep(3);
    printf("\nWL releases and device suspends as no WL is acquired\n");
    printf("\nACTION : Disconnect the USB to SUSPEND the device \n");
    tafPMTest_release(ref);
    return LE_OK;
}

le_result_t ctrlCmd_test5()
{
    LE_TEST_INFO("====== Suspend test case when WL is acquired from app and app exits ======");
    printf("\n====== Suspend test case when WL is acquired from app and app exits ======\n");
    taf_pm_WakeupSourceRef_t ref = tafPMTest_create("app_exit_suspend");
    tafPMTest_acquire(ref);
    sleep(2);
    printf("\nApp exits and device suspends\n");
    printf("\nACTION : Disconnect USB to SUSPEND the device\n");
    exit(EXIT_SUCCESS);
    return LE_OK;
}

le_result_t ctrlCmd_test7()
{
    LE_TEST_INFO("====== Test acquire and release multiple times WL with reference ======");
    printf("\n====== Test acquire and release multiple times WL with reference ======\n");
    taf_pm_WakeupSourceRef_t ref;
    char tag[10];

    snprintf(tag, sizeof(tag), "testWLs");
    ref = taf_pm_NewWakeupSource(1, tag);

    for(int i = 0; i < 5; i++) {
        tafPMTest_acquire(ref);
    }
    sleep(3);
    for(int i = 0; i < 5; i++) {
        tafPMTest_release(ref);
    }

    tafPMTest_release(LocalSuspendwsRef);

    LE_TEST_OK(taf_pm_GetPowerState() == TAF_PM_STATE_SUSPEND, "Suspend test case successfull"
            " with multiple times acquire and release of wake source with reference");
    printf("\nACTION : Disconnect USB to SUSPEND the device\n");
    return LE_OK;
}

le_result_t ctrlCmd_getAllMachines()
{
    taf_pm_VMListRef_t vmListRef = taf_pm_GetMachineList( );
    char name[32] = {0};
    if(!vmListRef) {
        LE_ERROR("List is null");
        return LE_FAULT;
    }
    le_result_t res = taf_pm_GetFirstMachineName(vmListRef, name, 32);
    while(res == LE_OK)
    {
        LE_INFO("vm name : %s",name);
        res = taf_pm_GetNextMachineName(vmListRef, name, 32);
    }
    res = taf_pm_DeleteMachineList(vmListRef);
    if(res == LE_OK)
        LE_INFO("Machine list deleted successfully");
    return LE_OK;
}
le_result_t ctrlCmd_suspendVM(const char* vmName)
{
    char name[32] = {0};
    taf_pm_VMListRef_t vmListRef = taf_pm_GetMachineList( );
    le_result_t res = taf_pm_GetFirstMachineName(vmListRef, name, 32);
    while(res == LE_OK){
        if(strcmp(vmName, name) == 0){
            res = taf_pm_SetVMPowerState(TAF_PM_STATE_SUSPEND, vmName);
            if(res != LE_OK)
                LE_ERROR("Failed to suspend VM");
            return res;
        }
        res = taf_pm_GetNextMachineName(vmListRef, name, 32);
    }
    LE_ERROR("Enter proper VM name");
    return LE_FAULT;
}

le_result_t ctrlCmd_suspend()
{
    LE_INFO("Send suspend request");
    le_result_t res = taf_pm_SetAllVMPowerState(TAF_PM_STATE_SUSPEND);
    if(res != LE_OK)
        LE_ERROR("Failed to suspend");
    return res;
}

le_result_t ctrlCmd_resumeVM(const char* vmName)
{
    char name[32] = {0};
    taf_pm_VMListRef_t vmListRef = taf_pm_GetMachineList( );
    le_result_t res = taf_pm_GetFirstMachineName(vmListRef, name, 32);
    while(res == LE_OK){
        if(strcmp(vmName, name) == 0){
            res = taf_pm_SetVMPowerState(TAF_PM_STATE_RESUME, vmName);;
            if(res != LE_OK)
                LE_ERROR("Failed to resume VM");
            return res;
        }
        res = taf_pm_GetNextMachineName(vmListRef, name, 32);
    }
    LE_ERROR("Enter proper VM name");
    return LE_FAULT;
}

le_result_t ctrlCmd_resume()
{
    LE_INFO("Send resume request");
    le_result_t res = taf_pm_SetAllVMPowerState(TAF_PM_STATE_RESUME);
    if(res != LE_OK)
        LE_ERROR("Failed to resume");
    return res;
}

le_result_t ctrlCmd_shutdownVM(const char* vmName)
{
    char name[32] = {0};
    taf_pm_VMListRef_t vmListRef = taf_pm_GetMachineList( );
    le_result_t res = taf_pm_GetFirstMachineName(vmListRef, name, 32);
    while(res == LE_OK){
        if(strcmp(vmName, name) == 0){
            res = taf_pm_SetVMPowerState(TAF_PM_STATE_SHUTDOWN, vmName);
            if(res != LE_OK)
                LE_ERROR("Failed to shutdown VM");
            return res;
        }
        res = taf_pm_GetNextMachineName(vmListRef, name, 32);
    }
    LE_ERROR("Enter proper VM name");
    return LE_FAULT;
}

le_result_t ctrlCmd_shutdown()
{
    LE_INFO("Send shutdown request");
    le_result_t res = taf_pm_SetAllVMPowerState(TAF_PM_STATE_SHUTDOWN);
    if(res != LE_OK)
        LE_ERROR("Failed to shutdown");
    return res;
}

le_result_t ctrlCmd_SetPowerMode(ctrlCmd_PowerMode_t powerMode)
{
    LE_INFO("ctrlCmd_SetPowerMode");
    le_result_t res = taf_pm_SetPowerMode(powerMode);
    if(res != LE_OK)
        LE_ERROR("Failed to SetPowerMode");
    return res;
}

COMPONENT_INIT
{
    LE_INFO("tafPMUnitTest started");
}
