/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Send request and get response.
 */
//--------------------------------------------------------------------------------------------------
static void MySyncRequestResponse
(
    void* param1,
    void* param2
)
{
    LE_UNUSED(param1);
    LE_UNUSED(param2);

    syncSvcDemo_MyCmd(4);
    asyncSvcDemo_MyCmd(4);

    // Continuously queue requests to demonstrate async vs sync behavior.
    // In production, need to add termination condition or external control mechanism.
    le_event_QueueFunction(MySyncRequestResponse, NULL, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Client event handler.
 */
//--------------------------------------------------------------------------------------------------
static void MyEventHandler
(
    const char* evtMsg,
    void* contextPtr
)
{
    LE_UNUSED(contextPtr);

    if (evtMsg != NULL)
    {
        LE_INFO("Received: '%s'", evtMsg);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Client dedicated thread to receive events from service.
 */
//--------------------------------------------------------------------------------------------------
static void* MyEventThread
(
    void* context
)
{
    LE_UNUSED(context);

    syncSvcDemo_ConnectService();
    asyncSvcDemo_ConnectService();

    syncSvcDemo_AddMyEvtHandler(MyEventHandler, NULL);
    asyncSvcDemo_AddMyEvtHandler(MyEventHandler, NULL);
    le_event_RunLoop();
}

COMPONENT_INIT
{
    le_thread_Start(le_thread_Create("EventReceivingThread", MyEventThread, NULL));
    le_event_QueueFunction(MySyncRequestResponse, NULL, NULL);
}
