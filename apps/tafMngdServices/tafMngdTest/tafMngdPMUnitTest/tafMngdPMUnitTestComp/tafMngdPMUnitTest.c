/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "legato.h"
#include "interfaces.h"

#define MAX_DATA_ID 32
#define DEFAULT_DATA_ID 1
#define GLOBAL_STATE "ALL_MACHINES"

typedef struct{
    taf_mngd_pm_Nad_t nad;
    char vmName[TAF_MNGD_PM_MACHINE_NAME_LEN];
    taf_mngd_pm_State_t state;
}requestState_t;

static le_sem_Ref_t semRef = NULL, queueSemRef = NULL;
static le_thread_Ref_t threadRef = NULL;
taf_pm_StateChangeHandlerRef_t handlerRef;
taf_pm_StateChangeExHandlerRef_t handlerExRef;
taf_mngd_pm_StateChangeHandlerRef_t mpmsHanlerRef;

le_result_t res;
int reqResult = 1;

static void PrintUsage
(
    void
)
{
    puts(
        "\n"
        "NAME:\n"
        "    tafMngdPMUnitTest - Used to perform Manged PM Service testcases.\n"
        "\n"
        "PREREQUISITES:\n"
        "    tafMngdPMSvc is running.\n"
        "    tafMngdPMSvc can be started using \"app start tafMngdPMSvc.\"\n"
        "\n"
        "DESCRIPTION:\n"
        "    To run the tafMngdPMUnitTest app :\n"
        "    app runProc tafMngdPMUnitTest tafMngdPMUnitTest -- <HOSTVM/PVM> <VM name> <VM name>\n"
        "\n"
        "    Number of VM names is not limited, can be given in any number based on availablity.\n"
        "    To know the available machines use \"pm getAvailableMachines\"\n"
        "\n"
        "    For usage:\n"
        "    app runProc tafMngdPMUnitTest tafMngdPMUnitTest -- help\n"
        );

    exit(EXIT_SUCCESS);
}

static char * StateToString(taf_mngd_pm_State_t *state)
{
    switch (*state)
    {
        case TAF_MNGD_PM_STATE_RESUME:
            return "RESUME";
        case TAF_MNGD_PM_STATE_SUSPEND:
            return "SUSPEND";
        case TAF_MNGD_PM_STATE_SHUTDOWN:
            return "SHUTDOWN";
        default:
            LE_ERROR("unknown state");
            return "UNKNOWN";
    }
}

static void* TestTriggerState(void* requestStatePtr)
{
    taf_mngd_pm_ConnectService();
    requestState_t* reqStatePtr = (requestState_t*)requestStatePtr;
    LE_TEST_INFO("Test to trigger %s State", StateToString(&reqStatePtr->state));
    if(strncmp(reqStatePtr->vmName, GLOBAL_STATE, TAF_MNGD_PM_MACHINE_NAME_LEN) == 0) {
        res = taf_mngd_pm_SetNadPowerState(reqStatePtr->nad, reqStatePtr->state);
    } else {
        res = taf_mngd_pm_SetVMPowerState(reqStatePtr->nad, reqStatePtr->vmName,
                reqStatePtr->state);
    }
    LE_TEST_OK(res==LE_OK, "Successfully triggired %s state", StateToString(&reqStatePtr->state));
    le_sem_Post(queueSemRef);
    return NULL;
}

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

void TestStateChangeHandler(taf_pm_State_t state, void* contextPtr)
{
    LE_INFO("State change triggered for %s\n", tafStateToString(state));
    printf("\nState change triggered for %s\n", tafStateToString(state));
}

//Function called on power state change
void TestStateChangeExHandler(taf_pm_PowerStateRef_t powerStateRef,
        taf_pm_NadVm_t vm_id, taf_pm_State_t state, void* contextPtr)
{
    LE_TEST_INFO("State change triggered for %s\n", tafStateToString(state));
    printf("\nState change triggered for %s\n", tafStateToString(state));
    taf_pm_SendStateChangeAck(powerStateRef,state,TAF_PM_PVM,TAF_PM_READY);
    LE_INFO("Sent state change acknowledge for %s\n", tafStateToString(state));
    printf("\n Sent state change acknowledge for %s\n", tafStateToString(state));
    if(state == TAF_PM_STATE_SHUTDOWN)
    {
        LE_INFO("Sent state change NACK for %s\n", tafStateToString(state));
        printf("\n Sent state change NACK for %s\n", tafStateToString(state));
    taf_pm_SendStateChangeAck(powerStateRef,state,TAF_PM_PVM,TAF_PM_NOT_READY);
    }
}

// function called on MPMS power state change
void TestMPMSStateChangeHandler(taf_mngd_pm_StateInd_t* indication, void* contextPtr)
{
    switch(indication->state)
    {
        case TAF_MNGD_PM_STATE_RESUME:
            LE_TEST_INFO("MPMS state change to %s\n", "TAF_MNGD_PM_STATE_RESUME");
            printf("\nMPMS state change to %s\n", "TAF_MNGD_PM_STATE_RESUME");
            break;

        case TAF_MNGD_PM_STATE_SUSPEND:
            LE_TEST_INFO("MPMS state change to %s\n", "TAF_MNGD_PM_STATE_SUSPEND");
            printf("\nMPMS state change to %s\n", "TAF_MNGD_PM_STATE_SUSPEND");
            break;

        case TAF_MNGD_PM_STATE_SHUTDOWN:
            LE_TEST_INFO("MPMS state change to %s\n", "TAF_MNGD_PM_STATE_SHUTDOWN");
            printf("\nMPMS state change to %s\n", "TAF_MNGD_PM_STATE_SHUTDOWN");
            break;

        case TAF_MNGD_PM_STATE_RELEASING_WAKE_SOURCE:
            LE_TEST_INFO("MPMS state change to %s\n", "TAF_MNGD_PM_STATE_RELEASING_WAKE_SOURCE");
            printf("\nMPMS state change to %s\n", "TAF_MNGD_PM_STATE_RELEASING_WAKE_SOURCE");
            break;

        case TAF_MNGD_PM_STATE_SUSPENDING:
            LE_TEST_INFO("MPMS state change to %s\n", "TAF_MNGD_PM_STATE_SUSPENDING");
            printf("\nMPMS state change to %s\n", "TAF_MNGD_PM_STATE_SUSPENDING");
            break;

        case TAF_MNGD_PM_STATE_SHUTTING_DOWN:
            LE_TEST_INFO("MPMS state change to %s\n", "TAF_MNGD_PM_STATE_SHUTTING_DOWN");
            printf("\nMPMS state change to %s\n", "TAF_MNGD_PM_STATE_SHUTTING_DOWN");
            break;

        case TAF_MNGD_PM_STATE_WAKING_UP:
            LE_TEST_INFO("MPMS state change to %s\n", "TAF_MNGD_PM_STATE_WAKING_UP");
            printf("\nMPMS state change to %s\n", "TAF_MNGD_PM_STATE_WAKING_UP");
            break;

        default:
            break;
    }
}

static void* test_stateChangeHandler(void* ctxPtr)
{
    taf_pm_ConnectService();
    taf_mngd_pm_ConnectService();

    LE_TEST_INFO("Testing taf_pm_AddStateChangeHandler on valid handler reference");
    handlerRef = taf_pm_AddStateChangeHandler(TestStateChangeHandler, NULL);
    LE_TEST_OK(handlerRef != NULL,"Register state change handler is successfull");

    LE_TEST_INFO("Testing taf_pm_AddStateChangeExHandler on valid handler reference");
    handlerExRef = taf_pm_AddStateChangeExHandler(TestStateChangeExHandler, NULL);
    LE_TEST_OK(handlerExRef != NULL,"Register state change handler is successfull");

    LE_TEST_INFO("Testing taf_mngd_pm_AddStateChangeHandler on valid handler reference");
    mpmsHanlerRef = taf_mngd_pm_AddStateChangeHandler(
                    (taf_mngd_pm_StateChangeHandlerFunc_t)TestMPMSStateChangeHandler,
                    NULL);
    LE_TEST_OK(handlerExRef != NULL,"Register MPMS state change handler is successfull");

    le_sem_Post(semRef);
    le_event_RunLoop();
}

static void TestMngdPMUnitTest()
{
    int NumberOfArgs = le_arg_NumArgs();
    LE_INFO("NumberOfArgs is %d", NumberOfArgs);
    const char* arg;

    arg = le_arg_GetArg(0);
    if (arg!= NULL && strncmp(arg,"help", 4) == 0)
    {
        PrintUsage();
    }

    if(threadRef == NULL)
    {
        threadRef = le_thread_Create("state_trigger_thread",
                                    test_stateChangeHandler, NULL);
        le_thread_Start(threadRef);
        le_sem_Wait(semRef);
    }
    requestState_t requestState;
    requestState.nad = TAF_MNGD_PM_NAD1;
    le_utf8_Copy(requestState.vmName, GLOBAL_STATE, 32, NULL);
    requestState.state = TAF_MNGD_PM_STATE_SUSPEND;
    le_event_QueueFunctionToThread(threadRef, (le_event_DeferredFunc_t)TestTriggerState,
                                   &requestState, NULL);
    le_sem_Wait(queueSemRef);
    sleep(5);

    taf_mngd_pm_State_t getState;

    if(NumberOfArgs >= 1 && arg != NULL){
        LE_TEST_INFO("Test taf_mngd_pm_GetVMPowerState for VM");
        res = taf_mngd_pm_GetVMPowerState(TAF_MNGD_PM_NAD1, arg, &getState);
        LE_INFO("getState is %s", StateToString(&getState));
        LE_TEST_OK(getState == TAF_MNGD_PM_STATE_SUSPEND, "Successfully get the state of VM");
    }

    LE_TEST_INFO("Test taf_pm_GetPowerState for whole NAD");
    taf_pm_State_t state = taf_pm_GetPowerState();
    LE_INFO("getState is %s", tafStateToString(state));
    LE_TEST_OK(state == TAF_PM_STATE_SUSPEND, "Successfully get the state of NAD");


    requestState.nad = TAF_MNGD_PM_NAD1;
    le_utf8_Copy(requestState.vmName, GLOBAL_STATE, 32, NULL);
    requestState.state = TAF_MNGD_PM_STATE_RESUME;
    le_event_QueueFunctionToThread(threadRef, (le_event_DeferredFunc_t)TestTriggerState,
                                   &requestState, NULL);
    le_sem_Wait(queueSemRef);
    sleep(3);

    if(NumberOfArgs >= 1 && arg != NULL){
        LE_TEST_INFO("Test taf_mngd_pm_GetVMPowerState for VM");
        res = taf_mngd_pm_GetVMPowerState(TAF_MNGD_PM_NAD1, arg, &getState);
        LE_INFO("getState is %s", StateToString(&getState));
        LE_TEST_OK(getState == TAF_MNGD_PM_STATE_RESUME, "Successfully get the state of VM");

        LE_TEST_INFO("Test taf_mngd_pm_GetVMPowerState for invalid VM");
        res = taf_mngd_pm_GetVMPowerState(TAF_MNGD_PM_NAD1, "abc", &getState);
        LE_TEST_OK(res == LE_BAD_PARAMETER, "Successfully got the BAD_PARAM on invalid VM");
    }

    LE_TEST_INFO("Test taf_pm_GetPowerState for whole NAD");
    state = taf_pm_GetPowerState();
    LE_INFO("state is %s", tafStateToString(state));
    LE_TEST_OK(state == TAF_PM_STATE_RESUME, "Successfully get the state of NAD");

    arg = le_arg_GetArg(1);
    LE_INFO("no of args is %d", NumberOfArgs);
    int argIndex = 1;
    while (arg != NULL){

        LE_TEST_INFO("Test taf_mngd_pm_GetVMPowerState for VM");
        requestState.nad = TAF_MNGD_PM_NAD1;
        le_utf8_Copy(requestState.vmName, arg, 32, NULL);
        requestState.state = TAF_MNGD_PM_STATE_UNKNOWN;
        res = taf_mngd_pm_GetVMPowerState(TAF_MNGD_PM_NAD1, requestState.vmName,
                &requestState.state);
        LE_INFO("requestState.state is %s", StateToString(&requestState.state));
        LE_TEST_OK(requestState.state == TAF_MNGD_PM_STATE_RESUME,
                "Successfully get the state of VM");

        requestState.nad = TAF_MNGD_PM_NAD1;
        le_utf8_Copy(requestState.vmName, arg, 32, NULL);
        requestState.state = TAF_MNGD_PM_STATE_SUSPEND;
        le_event_QueueFunctionToThread(threadRef, (le_event_DeferredFunc_t)TestTriggerState,
                &requestState, NULL);
        le_sem_Wait(queueSemRef);

        sleep(5);

        LE_TEST_INFO("Test taf_mngd_pm_GetVMPowerState for VM");
        requestState.state = TAF_MNGD_PM_STATE_UNKNOWN;
        res = taf_mngd_pm_GetVMPowerState(TAF_MNGD_PM_NAD1, requestState.vmName,
                &requestState.state);
        LE_INFO("requestState.state is %s", StateToString(&requestState.state));
        LE_TEST_OK(requestState.state == TAF_MNGD_PM_STATE_SUSPEND,
                "Successfully get the state of VM");

        requestState.nad = TAF_MNGD_PM_NAD1;
        le_utf8_Copy(requestState.vmName, arg, 32, NULL);
        requestState.state = TAF_MNGD_PM_STATE_RESUME;
        le_event_QueueFunctionToThread(threadRef, (le_event_DeferredFunc_t)TestTriggerState,
                &requestState, NULL);
        le_sem_Wait(queueSemRef);
        sleep(2);

        requestState.state = TAF_MNGD_PM_STATE_UNKNOWN;
        LE_TEST_INFO("Test taf_mngd_pm_GetVMPowerState for %s VM", requestState.vmName);
        res = taf_mngd_pm_GetVMPowerState(TAF_MNGD_PM_NAD1, requestState.vmName,
                &requestState.state);
        LE_INFO("requestState.state is %s", StateToString(&requestState.state));
        LE_TEST_OK(requestState.state == TAF_MNGD_PM_STATE_RESUME,
                "Successfully get the state of %s VM", requestState.vmName);

        LE_INFO("le_arg_NumArgs is %" PRIuS " argIndex is %d", le_arg_NumArgs(), argIndex);
        if(le_arg_NumArgs() > ++argIndex)
            arg = le_arg_GetArg(argIndex);
        else
            arg = NULL;
    }

    LE_INFO("====All tests are passed=====");
    LE_TEST_EXIT;
}

COMPONENT_INIT
{
    if(le_arg_NumArgs() == 0)
    {
        PrintUsage();
    }
    semRef = le_sem_Create("MngdPMSem", 0);
    queueSemRef = le_sem_Create("MngdPMQueueSem", 0);

    TestMngdPMUnitTest();
}