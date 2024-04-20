/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafMngdConnIntTestHdlr.c
 * @brief      This file integration test functions of Managed Connectivity Service's notifications.
 */

#include "legato.h"
#include "interfaces.h"

#define MAX_DATA_ID 32
#define MAX_PATH_LEN 256

taf_mngdConn_DataStateHandlerRef_t statHandlerRef = NULL;

static void PrintUsage()
{
    puts("\n"
         "app runProc tafMngdConnIntTest --exe=tafMngdConnIntTest -- StartData <id>\n"
         "app runProc tafMngdConnIntTest --exe=tafMngdConnIntTest -- StopData <id>\n"
         "app runProc tafMngdConnIntTest --exe=tafMngdConnIntTest -- GetConnState <id>\n"
         "app runProc tafMngdConnIntTest --exe=tafMngdConnIntTest -- GetIpAddr <id>\n"
         "app runProc tafMngdConnIntTest --exe=tafMngdConnIntTest -- CancelL1Recovery <id>\n"
         "app runProc tafMngdConnIntTest --exe=tafMngdConnIntTestHdlr -- Monitor <id>\n\n"
         "For testing multi client, set AutoStart:No and use TelAF-CM as the second client.\n"
         "For testing handlers, run test from a separate shell or put test in background.\n"
         "\n");
}

static char* StateToString(taf_mngdConn_DataState_t state)
{
    switch (state)
    {
        case TAF_MNGDCONN_DATA_CONNECTED:
            return "TAF_MNGDCONN_DATA_CONNECTED";
        case TAF_MNGDCONN_DATA_DISCONNECTED:
            return "TAF_MNGDCONN_DATA_DISCONNECTED";
        case TAF_MNGDCONN_DATA_CONNECTION_FAILED:
            return "TAF_MNGDCONN_DATA_CONNECTION_FAILED";
        case TAF_MNGDCONN_DATA_CONNECTION_STALLED:
            return "TAF_MNGDCONN_DATA_CONNECTION_STALLED";
        default:
            LE_TEST_INFO("unknown status: %d", (int)state);
            return "unknow status";
    }
    return "unknow status";
}

static void RecoveryStateHandler(taf_mngdConn_RecoveryState_t state,
                                 taf_mngdConn_DataRef_t dataRef,
                                 void *contextPtr)
{
    uint8_t dataId = 0;
    le_result_t result = taf_mngdConn_DataGetId(dataRef, &dataId);
    if (LE_OK != result)
    {
        LE_TEST_INFO("Failed to get Data ID");
    }
    else
        LE_TEST_INFO("Data ID: %d", dataId);

    switch (state)
    {
    case TAF_MNGDCONN_RECOVERY_L1_SCHEDULED:
        LE_TEST_INFO ("TAF_MNGDCONN_RECOVERY_L1_SCHEDULED");
        break;
    case TAF_MNGDCONN_RECOVERY_L1_STARTED:
        LE_TEST_INFO ("TAF_MNGDCONN_RECOVERY_L1_STARTED");
        break;
    case TAF_MNGDCONN_RECOVERY_L1_CANCELED:
        LE_TEST_INFO ("TAF_MNGDCONN_RECOVERY_L1_CANCELED");
        break;
    default:
        LE_TEST_INFO("unknown state: %d", (int)state);
        break;
    };

    LE_UNUSED(contextPtr);
    return;
}

static void ConnectionStateHandler
(
    taf_mngdConn_DataRef_t dataRef,
    taf_mngdConn_DataState_t dataState,
    void*  contextPtr
)
{
    uint8_t dataId;
    le_result_t result = taf_mngdConn_DataGetId(dataRef, &dataId);
    if (LE_OK != result)
    {
        LE_TEST_INFO("Failed to get Data ID");
    }
    LE_TEST_INFO("---data id : %d, Connection State : %s", dataId, StateToString(dataState));
}

static void* HandlerThread(void* contextPtr)
{
    //  connect service in thread.
    taf_mngdConn_DataRef_t dataRef = (taf_mngdConn_DataRef_t)contextPtr;
    taf_mngdConn_ConnectService();

    statHandlerRef = taf_mngdConn_AddDataStateHandler(dataRef,
                        (taf_mngdConn_DataStateHandlerFunc_t)ConnectionStateHandler, NULL);

    // Register recovery state handler
    taf_mngdConn_AddRecoveryStateHandler(RecoveryStateHandler, NULL);

    le_event_RunLoop();
    return NULL;
}

static int monitorState()
{
    LE_TEST_INFO("----monitorState ");
    le_result_t result = LE_FAULT;
    char threadName[32];

    uint32_t dataId = strtol(le_arg_GetArg(1), NULL, 0);

    taf_mngdConn_DataRef_t dataRef = taf_mngdConn_GetData(dataId);

    if(dataRef == NULL)
    {
        LE_TEST_INFO("Unable to get data ref for data ID %d", dataId);
        return result;
    }

    snprintf(threadName, sizeof(threadName)-1, "dataThread%d", dataId);

    le_thread_Start(le_thread_Create(threadName, HandlerThread, (void*)dataRef));

    // spin here
    do
    {
        sleep(1);
    } while (true);

    return LE_OK;
}

COMPONENT_INIT
{
    int status = EXIT_SUCCESS;
    const char* testType = "";
    LE_TEST_INFO("Num args = %zu ", le_arg_NumArgs());
    if (2 == le_arg_NumArgs() )
    {
        testType = le_arg_GetArg(0);
        LE_TEST_INFO("arg0=%s ",testType);
        if (NULL == testType) {
            LE_TEST_FATAL("testType is NULL");
        }

        if (strncasecmp(testType, "Monitor", strlen("Monitor")) == 0)
        {
            status=monitorState();
            LE_TEST_OK(LE_OK == status, "MCS Test: Monitor");
        }
        else
        {
            LE_TEST_FATAL("Unknown command");
        }
    }
    else
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }
    LE_TEST_EXIT;
}
