/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       wlanUnitTest.cpp
 * @brief      Unit test for WLAN device manager
 */

#include "legato.h"
#include "interfaces.h"

static le_sem_Ref_t wlanSemRef = NULL;

static void DeviceStateHandler( taf_wlan_WlanRef_t wlanRef,
                                taf_wlan_DeviceState_t state,
                                void *CtxPtr)
{
    LE_UNUSED (wlanRef);
    LE_UNUSED (CtxPtr);
    LE_TEST_INFO("DeviceStateHandler: State: %d", state);
}

static void *wlanThreadHdlr(void *contextPtr)
{
    //  connect to wlan service from the thread's context.
    taf_wlan_ConnectService();

    // Register handler
    taf_wlan_DeviceStateHandlerRef_t devHdlrRef =
                            taf_wlan_AddDeviceStateHandler(NULL, DeviceStateHandler, NULL);
    LE_UNUSED(devHdlrRef);

    // Allow main thread to proceed.
    le_sem_Post(wlanSemRef);

    // Service events
    le_event_RunLoop();

    return NULL;
}

COMPONENT_INIT
{
    le_result_t result = LE_FAULT;
    taf_wlan_DeviceState_t state;
    taf_wlan_DeviceMode_t mode=TAF_WLAN_MODE_UNSUPPORTED;

    LE_TEST_INIT;
    LE_TEST_INFO("======== WLAN Device Manager Integration Test ========");

    // 1 Register for events
    LE_TEST_INFO("======== Register for events ========");
    wlanSemRef = le_sem_Create("wlanSem", 0);
    le_thread_Ref_t wlanThreadRef = le_thread_Create("wlanThread", wlanThreadHdlr, NULL);
    le_thread_Start(wlanThreadRef);
    le_sem_Wait(wlanSemRef);

    // 2 Get State
    LE_TEST_INFO("======== GetState ========");
    result = taf_wlan_GetState(NULL, &state);
    LE_TEST_INFO("State: %d", state);
    LE_TEST_OK(LE_OK == result, "WLAN Test: taf_wlan_GetState");

    // 3 Get Mode
    LE_TEST_INFO("======== GetMode ========");
    result = taf_wlan_GetMode(NULL,&mode);
    LE_TEST_INFO("Mode: %d", mode);
    LE_TEST_OK(LE_OK == result, "WLAN Test: taf_wlan_GetMode");

    // 4 Set Mode
    LE_TEST_INFO("======== SetMode ========");
    mode = TAF_WLAN_MODE_AP;
    result = taf_wlan_SetMode(NULL,mode);
    LE_TEST_OK(LE_OK == result, "WLAN Test: taf_wlan_SetMode");

    // 5 Check Mode
    LE_TEST_INFO("======== Check GetMode ========");
    result = taf_wlan_GetMode(NULL,&mode);
    LE_TEST_INFO("Mode: %d", mode);
    LE_TEST_OK(LE_OK == result, "WLAN Test: taf_wlan_GetMode");

    // 6. Turn off WLAN. It will only take effect if it is on.
    LE_TEST_INFO("======== Turn off WLAN ========");
    result = taf_wlan_TurnOff(NULL);
    LE_TEST_OK(LE_OK == result, "WLAN Test: taf_wlan_Off");

    sleep(5);

    // 7. Turn on WLAN. It will only take effect if it is off.
    LE_TEST_INFO("======== Turn on WLAN ========");
    result = taf_wlan_TurnOn(NULL);
    LE_TEST_ASSERT(LE_OK == result, "WLAN Test: taf_wlan_On");

    sleep(5);

    // 8. Turn off WLAN. It will only take effect if it is on.
    LE_TEST_INFO("======== Turn off WLAN ========");
    result = taf_wlan_TurnOff(NULL);
    LE_TEST_OK(LE_OK == result, "WLAN Test: taf_wlan_Off");

    LE_TEST_EXIT;
}