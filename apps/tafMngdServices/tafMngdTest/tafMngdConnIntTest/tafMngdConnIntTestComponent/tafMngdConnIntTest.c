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

/*
 * @file       tafMngdConnIntTest.cpp
 * @brief      This file includes integration test functions of Managed Connectivity Service.
 */

#include "legato.h"
#include "interfaces.h"

#define MAX_DATA_ID 32
#define MAX_PATH_LEN 256

taf_mngd_Conn_DataStateHandlerRef_t statHandlerRef = NULL;

static void PrintUsage ()
{
    puts("\n"
        "app start tafMngdConnIntTest\n"
        "app runProc tafMngdConnIntTest --exe=tafMngdConnIntTest -- startdata <id>\n"
        "app runProc tafMngdConnIntTest --exe=tafMngdConnIntTest -- stopdata <id>\n"
        "app runProc tafMngdConnIntTest --exe=tafMngdConnIntTest -- getconnstate <id>\n"
        "app runProc tafMngdConnIntTest --exe=tafMngdConnIntTest -- getipaddr <id>\n"
        "app runProc tafMngdConnIntTest --exe=tafMngdConnIntTest -- monitor <id>\n"
        "\n");
}

static int startData()
{
    LE_INFO("----startData test " );
    le_result_t result;

    if (le_arg_NumArgs() != 2)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    uint32_t dataId = strtol(le_arg_GetArg(1), NULL, 0);

    taf_mngd_Conn_DataRef_t dataRef = taf_mngd_Conn_GetData(dataId);

    if(dataRef == NULL)
    {
        LE_ERROR("Not initialized for data ID %d", dataId);
        return EXIT_FAILURE;
    }

    result=taf_mngd_Conn_DataStart(dataRef);

    LE_INFO("----result=%d " ,result);

    if(result !=LE_OK)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static int stopData()
{
    LE_INFO("----stopData test " );
    le_result_t result;

    if (le_arg_NumArgs() != 2)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    uint32_t dataId = strtol(le_arg_GetArg(1), NULL, 0);

    taf_mngd_Conn_DataRef_t dataRef = taf_mngd_Conn_GetData(dataId);

    if(dataRef == NULL)
    {
        LE_ERROR("Not initialized for data ID %d", dataId);
        return EXIT_FAILURE;
    }

    result=taf_mngd_Conn_DataStop(dataRef);

    LE_INFO("----result=%d " ,result);

    if(result !=LE_OK)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static char* StateToString(taf_mngd_Conn_DataState_t state)
{
    switch (state)
    {
        case TAF_MNGD_CONN_DATA_CONNECTED:
            return "TAF_MNGD_CONN_DATA_CONNECTED";
        case TAF_MNGD_CONN_DATA_DISCONNECTED:
            return "TAF_MNGD_CONN_DATA_DISCONNECTED";
        default:
            LE_ERROR("unknown status: %d", (int)state);
            return "unknow status";
    }
    return "unknow status";
}

static int getConnState()
{
    LE_INFO("----getConnState test " );
    le_result_t result;
    uint8_t retDataId;
    taf_mngd_Conn_DataState_t state;


    if (le_arg_NumArgs() != 2)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    uint8_t dataId = strtol(le_arg_GetArg(1), NULL, 0);
    LE_INFO("dataId = %d", dataId);

    taf_mngd_Conn_DataRef_t dataRef = taf_mngd_Conn_GetData(dataId);

    if(dataRef == NULL)
    {
        LE_ERROR("Not initialized for data ID %d", dataId);
        return EXIT_FAILURE;
    }

    result=taf_mngd_Conn_DataGetConnectionState(dataRef, &retDataId, &state);

    LE_INFO("----result=%d" , result);

    if(result !=LE_OK)
        return EXIT_FAILURE;

    LE_INFO("----dataId=%d ", retDataId);
    LE_INFO("----state=%s ", StateToString(state));

    return EXIT_SUCCESS;
}

static int getConnIpAddr()
{
    LE_INFO("----getConnState test " );
    le_result_t result;
    char ipv4Addr[TAF_DCS_IPV4_ADDR_MAX_LEN];
    char ipv6Addr[TAF_DCS_IPV6_ADDR_MAX_LEN];

    if (le_arg_NumArgs() != 2)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    uint8_t dataId = strtol(le_arg_GetArg(1), NULL, 0);
    LE_INFO("dataId = %d", dataId);

    taf_mngd_Conn_DataRef_t dataRef = taf_mngd_Conn_GetData(dataId);

    if(dataRef == NULL)
    {
        LE_ERROR("Not initialized for data ID %d", dataId);
        return EXIT_FAILURE;
    }

    result=taf_mngd_Conn_DataGetConnectionIPAddresses(dataRef,
                                                      ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN,
                                                      ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN);

    LE_INFO("----result=%d" , result);

    if(result !=LE_OK)
        return EXIT_FAILURE;

    LE_INFO("----dataId=%d ", dataId);
    LE_INFO("----IPv4Addr=%s", ipv4Addr);
    LE_INFO("----IPv6Addr=%s", ipv6Addr);

    return EXIT_SUCCESS;
}

static void ConnectionStateHandler
(
    taf_mngd_Conn_DataRef_t dataRef,
    taf_mngd_Conn_DataState_t dataState,
    void*  contextPtr
)
{
    taf_mngd_Conn_DataState_t state;
    uint8_t dataId;
    le_result_t result;

    LE_INFO("---data ref : %p, Connection State : %s", dataRef, StateToString(dataState));

    result=taf_mngd_Conn_DataGetConnectionState(dataRef, &dataId, &state);

    if(result == LE_OK)
        LE_INFO("---dataId=%d", dataId);

}

static void* HandlerThread(void* contextPtr)
{
    //  connect service in thread.
    taf_mngd_Conn_DataRef_t dataRef = (taf_mngd_Conn_DataRef_t)contextPtr;
    taf_mngd_Conn_ConnectService();

    statHandlerRef = taf_mngd_Conn_AddDataStateHandler(dataRef,
                        (taf_mngd_Conn_DataStateHandlerFunc_t)ConnectionStateHandler, NULL);

    le_event_RunLoop();
    return NULL;
}

static int monitorState()
{
    LE_INFO("----monitorState ");

    char threadName[32];

    if (le_arg_NumArgs() != 2)
    {
        return EXIT_FAILURE;
    }

    uint32_t dataId = strtol(le_arg_GetArg(1), NULL, 0);

    taf_mngd_Conn_DataRef_t dataRef = taf_mngd_Conn_GetData(dataId);

    if(dataRef == NULL)
    {
        LE_ERROR("Not initialized for data ID %d", dataId);
        return EXIT_FAILURE;
    }

    snprintf(threadName, sizeof(threadName)-1, "dataThread%d", dataId);

    le_thread_Start(le_thread_Create(threadName, HandlerThread, (void*)dataRef));

    return EXIT_SUCCESS;
}

COMPONENT_INIT
{
    int status = EXIT_SUCCESS;
    const char* testType = "";
    if (le_arg_NumArgs() == 0 )
    {
        PrintUsage();

    }
    if (le_arg_NumArgs() >= 1)
    {
        testType = le_arg_GetArg(0);
        LE_INFO("arg0=%s ",testType);
        if (NULL == testType) {
            LE_ERROR("testType is NULL");
            exit(EXIT_FAILURE);
        }

        if(strcmp(testType, "startdata") == 0)
        {
            status=startData();
            exit(status);
        }
        else if(strcmp(testType, "stopdata") == 0)
        {
            status=stopData();
            exit(status);
        }
        else if(strcmp(testType, "getconnstate") == 0)
        {
            status=getConnState();
            exit(status);
        }else if(strcmp(testType, "getipaddr") == 0)
        {
            status=getConnIpAddr();
            exit(status);
        }
        else if(strcmp(testType, "monitor") == 0)
        {
            status=monitorState();

            if(status == EXIT_FAILURE)
            {
                LE_ERROR("Failed to monitor state");
                exit(status);
            }
        }
        else
        {
            LE_ERROR("Error command");
            exit(EXIT_FAILURE);
        }

    }
}
