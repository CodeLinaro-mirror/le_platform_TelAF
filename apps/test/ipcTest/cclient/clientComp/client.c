/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#include <string.h>

__attribute__((unused)) static void TestServerExitHandler(void *contextPtr)
{
    LE_INFO("TestServerExitHandler called");
    LE_UNUSED(contextPtr);
    // Try connecting to the server again
    ipcTest_ConnectService();
    LE_INFO("TestServerExitHandler reconnected");
}

static void TestServerExit(void)
{
    ipcTest_SetNonExitServerDisconnectHandler(TestServerExitHandler, NULL);
    // Stop the server
    ipcTest_ExitServer();
}

COMPONENT_INIT
{
    bool skipExitTest = false;

    le_arg_SetFlagVar(&skipExitTest, NULL, "skip-exit");
    le_arg_Scan();

    LE_TEST_PLAN(1);

    ipcTest_ConnectService();
    LE_TEST_INFO("Connected to server");

    TestServerExit();
}