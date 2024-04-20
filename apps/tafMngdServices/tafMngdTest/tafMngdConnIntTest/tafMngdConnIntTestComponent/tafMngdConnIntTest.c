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

taf_mngdConn_DataStateHandlerRef_t statHandlerRef = NULL;

static void PrintUsage ()
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

static le_result_t startData()
{
    LE_TEST_INFO("----StartData test ");
    le_result_t result = LE_FAULT;

    if (le_arg_NumArgs() != 2)
    {
        PrintUsage();
        return result;
    }

    uint32_t dataId = strtol(le_arg_GetArg(1), NULL, 0);

    taf_mngdConn_DataRef_t dataRef = taf_mngdConn_GetData(dataId);

    if(dataRef == NULL)
    {
        LE_TEST_INFO("Unable to get data ref for data ID %d", dataId);
        return result;
    }

    result=taf_mngdConn_DataStart(dataRef);
    if(result !=LE_OK)
    {
        LE_TEST_INFO("taf_mngdConn_DataStart failed: %d ", result);
    }
    return result;
}

static le_result_t stopData()
{
    LE_TEST_INFO("----StopData test ");
    le_result_t result = LE_FAULT;

    if (le_arg_NumArgs() != 2)
    {
        PrintUsage();
        return result;
    }

    uint32_t dataId = strtol(le_arg_GetArg(1), NULL, 0);

    taf_mngdConn_DataRef_t dataRef = taf_mngdConn_GetData(dataId);

    if(dataRef == NULL)
    {
        LE_TEST_INFO("Unable to get data ref for data ID %d", dataId);
        return result;
    }

    result=taf_mngdConn_DataStop(dataRef);
    if(result !=LE_OK)
    {
        LE_TEST_INFO("taf_mngdConn_DataStop failed: %d ", result);
    }
    return result;
}

static le_result_t cancelL1Recovery()
{
    LE_TEST_INFO("----CancelL1Recovery test ");
    le_result_t result = LE_FAULT;

    if (le_arg_NumArgs() != 2)
    {
        PrintUsage();
        return result;
    }

    uint32_t dataId = strtol(le_arg_GetArg(1), NULL, 0);

    taf_mngdConn_DataRef_t dataRef = taf_mngdConn_GetData(dataId);

    if (dataRef == NULL)
    {
        LE_TEST_INFO("Unable to get data ref for data ID %d", dataId);
        return result;
    }

    result = taf_mngdConn_CancelL1Recovery(dataRef);
    if (result != LE_OK)
    {
        LE_TEST_INFO("taf_mngdConn_CancelL1Recovery failed: %d ", result);
    }
    return result;
}

static char* StateToString(taf_mngdConn_DataState_t state)
{
    switch (state)
    {
        case TAF_MNGDCONN_DATA_DISCONNECTED:
            return "TAF_MNGDCONN_DATA_DISCONNECTED";
        case TAF_MNGDCONN_DATA_CONNECTED:
            return "TAF_MNGDCONN_DATA_CONNECTED";
        case TAF_MNGDCONN_DATA_CONNECTION_STALLED:
            return "TAF_MNGDCONN_DATA_CONNECTION_STALLED";
        case TAF_MNGDCONN_DATA_CONNECTION_FAILED:
            return "TAF_MNGDCONN_DATA_CONNECTION_FAILED";
    default:
        LE_TEST_INFO("unknown status: %d", (int)state);
    }
    return "unknow status";
}

static int getConnState()
{
    LE_TEST_INFO("----GetConnState test " );
    le_result_t result = LE_FAULT;
    uint8_t retDataId;
    taf_mngdConn_DataState_t state;


    if (le_arg_NumArgs() != 2)
    {
        PrintUsage();
        return result;
    }

    uint8_t dataId = strtol(le_arg_GetArg(1), NULL, 0);
    LE_TEST_INFO("dataId = %d", dataId);

    taf_mngdConn_DataRef_t dataRef = taf_mngdConn_GetData(dataId);

    if(dataRef == NULL)
    {
        LE_TEST_INFO("Unable to get data ref for data ID %d", dataId);
        return result;
    }

    result=taf_mngdConn_DataGetConnectionState(dataRef, &retDataId, &state);
    if(result !=LE_OK)
    {
        LE_TEST_INFO("taf_mngdConn_DataGetConnectionState failed: %d", result);
        return result;
    }
    LE_TEST_INFO("----dataId=%d ", retDataId);
    LE_TEST_INFO("----state=%s ", StateToString(state));

    return result;
}

static int getConnIpAddr()
{
    LE_TEST_INFO("----GetConnIpAddr test ");
    le_result_t result = LE_FAULT;
    char ipv4Addr[TAF_DCS_IPV4_ADDR_MAX_LEN];
    char ipv6Addr[TAF_DCS_IPV6_ADDR_MAX_LEN];

    if (le_arg_NumArgs() != 2)
    {
        PrintUsage();
        return result;
    }

    uint8_t dataId = strtol(le_arg_GetArg(1), NULL, 0);
    LE_TEST_INFO("dataId = %d", dataId);

    taf_mngdConn_DataRef_t dataRef = taf_mngdConn_GetData(dataId);

    if (dataRef == NULL)
    {
        LE_TEST_INFO("Unable to get data ref for data ID %d", dataId);
        return result;
    }

    result=taf_mngdConn_DataGetConnectionIPAddresses(dataRef,
                                                      ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN,
                                                      ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN);
    if (result != LE_OK)
    {
        LE_TEST_INFO("taf_mngdConn_DataGetConnectionIPAddresses failed: %d", result);
        return result;
    }

    LE_TEST_INFO("----dataId=%d ", dataId);
    LE_TEST_INFO("----IPv4Addr=%s", ipv4Addr);
    LE_TEST_INFO("----IPv6Addr=%s", ipv6Addr);

    return result;
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
        LE_TEST_INFO("arg0=%s ",testType);
        if (NULL == testType) {
            LE_TEST_FATAL("testType is NULL");
        }

        if (strncasecmp(testType, "StartData", strlen("StartData")) == 0)
        {
            status=startData();
            LE_TEST_OK(LE_OK == status, "MCS Test: StartData");
        }
        else if (strncasecmp(testType, "StopData", strlen("StopData")) == 0)
        {
            status=stopData();
            LE_TEST_OK(LE_OK == status, "MCS Test: StopData");
        }
        else if (strncasecmp(testType, "GetConnState", strlen("GetConnState")) == 0)
        {
            status=getConnState();
            LE_TEST_OK(LE_OK == status, "MCS Test: GetConnState");
        }
        else if (strncasecmp(testType, "GetIpAddr", strlen("GetIpAddr")) == 0)
        {
            status=getConnIpAddr();
            LE_TEST_OK(LE_OK == status, "MCS Test: GetIpAddr");
        }
        else if (strncasecmp(testType, "CancelL1Recovery", strlen("CancelL1Recovery")) == 0)
        {
            status = cancelL1Recovery();
            LE_TEST_OK(LE_OK == status, "MCS Test: CancelL1Recovery");
        }
        else if (strncasecmp(testType, "Monitor", strlen("Monitor")) == 0)
        {
            puts("\nMonitor not supported with this exe.\n");
            LE_TEST_INFO("----Monitor not supported with this exe.");
            PrintUsage();
        }
        else
        {
            LE_TEST_FATAL("Unknown command");
        }
    }
    LE_TEST_EXIT;
}
