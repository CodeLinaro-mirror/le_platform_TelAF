/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       wlanIntTest.cpp
 * @brief      Integration test functions for WLAN device manager
 */

#include "legato.h"
#include "interfaces.h"

#define MAX_SYSTEM_CMD_LENGTH 200

static void PrintUsage(void)
{
    puts("\n"
         "app runProc tafWLANIntTest wlanTest -- On\n"
         "app runProc tafWLANIntTest wlanTest -- Off\n"
         "app runProc tafWLANIntTest wlanTest -- GetState\n"
         "app runProc tafWLANIntTest wlanTest -- GetMode\n"
         "app runProc tafWLANIntTest wlanTest -- SetMode <mode>\n"
         "\n");
}

static le_result_t wlanTestOn()
{
    le_result_t result = taf_wlan_TurnOn(NULL);
    fprintf (stderr, "taf_wlan_TurnOn Return:%d\n",result);
    return result;
}
static le_result_t wlanTestOff()
{
    le_result_t result = taf_wlan_TurnOff(NULL);
    fprintf (stderr, "taf_wlan_TurnOff Return:%d\n",result);
    return result;
}

static le_result_t wlanTestGetState()
{
    taf_wlan_DeviceState_t state;
    le_result_t result = taf_wlan_GetState(NULL,&state);
    fprintf (stderr, "taf_wlan_GetState Return:%d State: %d\n",result,state);
    LE_TEST_INFO("State: %d", state);
    return result;
}

static le_result_t wlanTestGetMode()
{
    taf_wlan_DeviceMode_t mode=TAF_WLAN_MODE_UNSUPPORTED;
    le_result_t result = taf_wlan_GetMode(NULL, &mode);
    fprintf (stderr, "taf_wlan_GetMode Return:%d Mode: %d\n",result,mode);
    LE_TEST_INFO("Mode: %d",mode);
    return result;
}

static le_result_t wlanTestSetMode()
{
    int mode  = strtol((const char *)le_arg_GetArg(1), NULL, 10);
    LE_TEST_INFO("Mode: %d", mode);
    le_result_t result = taf_wlan_SetMode(NULL,(taf_wlan_DeviceMode_t) mode);
    fprintf (stderr, "taf_wlan_SetMode Return:%d\n",result);
    return result;
}

COMPONENT_INIT
{
    le_result_t status = LE_FAULT;

    LE_TEST_INIT;

    LE_TEST_INFO("======== WLAN Device Manager Integration Test ========");
    size_t numArgs = le_arg_NumArgs();

    // Number of arguments should be 1 or 2.
    if ((1 == numArgs) || (2 == numArgs))
    {
        const char *testType = le_arg_GetArg(0);
        if (strncmp(testType, "On", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== WLAN Test: On ========");
            status = wlanTestOn();
            LE_TEST_OK(LE_OK == status, "WLAN Test: On");
        }
        else if (strncmp(testType, "Off", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== WLAN Test: Off ========");
            status = wlanTestOff();
            LE_TEST_OK(LE_OK == status, "WLAN Test: Off");
        }
        else if (strncmp(testType, "GetState", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== WLAN Test: GetState ========");
            status = wlanTestGetState();
            LE_TEST_OK(LE_OK == status, "WLAN Test: GetState");
        }
        else if (strncmp(testType, "GetMode", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== WLAN Test: GetMode========");
            status = wlanTestGetMode();
            LE_TEST_OK(LE_OK == status, "WLAN Test: GetMode");
        }
        else if (strncmp(testType, "SetMode", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== WLAN Test: SetMode========");
            if (numArgs != 2)
            {
                PrintUsage();
                LE_TEST_FATAL("Invalid number of arguments");
            }
            status = wlanTestSetMode();
            LE_TEST_OK(LE_OK == status, "WLAN Test: SetMode");
        }
        else
        {
            PrintUsage();
            LE_TEST_FATAL("Invalid test type %s", testType);
        }
    }
    else
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }

    LE_TEST_EXIT;
}