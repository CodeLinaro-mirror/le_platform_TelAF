/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"


void AsyncServer_Exit(void* cmdRef, void* anotherParam)
{
    ipcTest_ExitServerRespond(cmdRef);
    LE_INFO("Server exits from ipcTest_ExitServer");
    exit(1);
}

void ipcTest_ExitServer
(
    ipcTest_ServerCmdRef_t cmdRef
)
{
    le_event_QueueFunction(AsyncServer_Exit,
                           cmdRef, NULL);
}

COMPONENT_INIT
{}