/**
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

taf_pm_WakeupSourceRef_t wsRef = NULL;
taf_pm_StateChangeHandlerRef_t pmHandlerRef;
static le_sem_Ref_t semRef;
static int pin61 = 61;
static taf_gpio_ChangeEventHandlerRef_t gpioHandlerRef;
static uint8_t wakeUpPin = 61;

//Function to convert taf pm state to string
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

// Callback function for TCU state change
void TestStateChangeHandler(taf_pm_State_t state, void* contextPtr)
{
    LE_TEST_INFO("State change triggered for %s\n", tafStateToString(state));
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
    pmHandlerRef = taf_pm_AddStateChangeHandler(TestStateChangeHandler, NULL);
    LE_TEST_OK(pmHandlerRef != NULL, "Registered successfully for StateChangeListener");
    le_sem_Post(semRef);
    le_event_RunLoop();
}

// Callback function for GPIO PIN change
static void PinChangeCallback(uint8_t pinNum, bool state, void *ctx){
    LE_INFO("State change %s pinNum %d, context pointer %d", state ? "TRUE" : "FALSE", pinNum,
            *(int *)ctx);
    if(taf_pm_GetPowerState() != TAF_PM_STATE_RESUME) {
        if(LE_OK == taf_pm_StayAwake(wsRef)) {
            LE_TEST_OK(true, "wake source acquired to keep the device awake of gpio %d trigger",
                    pinNum);
        } else {
            LE_ERROR("Failed to acquire the wake source on PIN state change");
        }
    }
}

static void AddGpioChangeCallback(uint8_t pinNum) {
    LE_INFO("AddGpioChangeCallback for PIN %d", pinNum);
    gpioHandlerRef = taf_gpio_AddChangeEventHandler(pinNum, TAF_GPIO_EDGE_FALLING, false,
            PinChangeCallback, &pin61);
    if (gpioHandlerRef != NULL) {
        LE_TEST_OK(gpioHandlerRef != NULL, "register successfully for falling trigger event"
                " pin %d", pinNum);
    } else {
        LE_ERROR("Failed to register for falling trigger event of gpio pin %d", pinNum);
    }
}

COMPONENT_INIT
{
    LE_INFO("tafPMDemoApp started");
    semRef = le_sem_Create("SemRef", 0);
    le_thread_Ref_t threadRef = le_thread_Create("taf_PM_StateHandler", test_stateChangeHandler,
            NULL);
    le_thread_Start(threadRef);
    WaitForSem_Timeout(semRef, 5);

    // Register for GPIO trigger and acquire wakeup source on GPIO trigger to resume device
    AddGpioChangeCallback(wakeUpPin);

    // Create a wakeup source
    wsRef = taf_pm_NewWakeupSource(0, "pmdemo");
     if(wsRef != NULL) {
        LE_TEST_OK(wsRef != NULL, "wakeup source created successfully");
    } else {
        LE_ERROR("Failed to create thewakeup source");
    }

    // Acquire the wakeup source
    le_result_t res = taf_pm_StayAwake(wsRef);
    if(res == LE_OK) {
        LE_TEST_OK(res == LE_OK, "wake source acquired successfully");
    } else {
        LE_ERROR("Failed to acquire the wake source");
    }
    // Delay of 5secs
    WaitForSem_Timeout(semRef, 5);

    // Release the wakeup source to suspend the device
    res = taf_pm_Relax(wsRef);
    if(res == LE_OK) {
        LE_TEST_OK(res == LE_OK, "wake source released successfully");
    } else {
        LE_ERROR("Failed to release the wake source");
    }
}
