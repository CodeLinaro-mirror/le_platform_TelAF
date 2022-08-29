/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "main.h"

taf_pm_StateChangeHandlerRef_t handlerRef;
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
    LE_TEST_INFO("State change triggered for %s\n", tafStateToString(state));
    printf("\nState change triggered for %s\n", tafStateToString(state));
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

le_result_t ctrlCmd_test3()
{
    LE_TEST_INFO("====== Suspend test case when WL is acquired ======");
    printf("\n====== Suspend test case when WL is acquired ======\n");
    semRef = le_sem_Create("SemRef", 0);
    le_thread_Ref_t threadRef = le_thread_Create("taf_PM_StateHandler",
            test_stateChangeHandler, NULL);
    le_thread_Start(threadRef);
    WaitForSem_Timeout(semRef, 5);
    taf_pm_WakeupSourceRef_t ref = tafPMTest_create("local_suspend");
    tafPMTest_acquire(ref);
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

    snprintf(tag, sizeof(tag), "testpm");
    ref = taf_pm_NewWakeupSource(1, tag);

    for(int i = 0; i < 5; i++) {
        tafPMTest_acquire(ref);
    }
    sleep(3);
    for(int i = 0; i < 5; i++) {
        tafPMTest_release(ref);
    }
    LE_TEST_OK(taf_pm_GetPowerState() == TAF_PM_STATE_SUSPEND, "Suspend test case successfull"
            " with multiple times acquire and release of wake source with reference");
    printf("\nACTION : Disconnect USB to SUSPEND the device\n");
    return LE_OK;
}

COMPONENT_INIT
{
    LE_INFO("tafPMUnitTest started");
}
