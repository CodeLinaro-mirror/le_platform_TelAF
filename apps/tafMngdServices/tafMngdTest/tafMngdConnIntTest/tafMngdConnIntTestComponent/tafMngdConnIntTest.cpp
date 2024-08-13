/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <iostream>
#include <string>
#include "legato.h"
#include "interfaces.h"

#define MAX_DATA_ID 32
#define MAX_PATH_LEN 256
#define MCS_MAX_NAME_LEN 32

/**
 * To run unit test, use the "app" command
 * Synopsis: app runProc <appName> [<procName>]
 * Example:  app runProc tafMngdConnIntTest tafMngdConnIntTest
 *
 * For multi client tests, set AutoStart:No and start the app with different procName(s)
 */
static void PrintUsage ()
{
    std::cout << "0 -> Exit  " << std::endl
              << "1 -> StartData  " << std::endl
              << "2 -> StopData  " << std::endl
              << "3 -> GetConnState  " << std::endl
              << "4 -> GetIpAddr  " << std::endl
              << "5 -> StartDataRetry  " << std::endl
              << "6 -> CancelL1Recovery " << std::endl
              << "7 -> CancelL2Recovery " << std::endl
              << "8 -> Monitor " << std::endl
              << std::endl;
}

static std::string DataStateToString(taf_mngdConn_DataState_t state)
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
        LE_TEST_INFO("unknown data state: %d", static_cast<int>(state));
    }
    return "unknown data state";
}

static std::string RecoveryStateToString(taf_mngdConn_RecoveryState_t state)
{
    switch (state)
    {
    case TAF_MNGDCONN_RECOVERY_L1_SCHEDULED:
        return "TAF_MNGDCONN_RECOVERY_L1_SCHEDULED";
    case TAF_MNGDCONN_RECOVERY_L1_STARTED:
        return "TAF_MNGDCONN_RECOVERY_L1_STARTED";
    case TAF_MNGDCONN_RECOVERY_L1_CANCELED:
        return "TAF_MNGDCONN_RECOVERY_L1_CANCELED";
    case TAF_MNGDCONN_RECOVERY_L2_SCHEDULED:
        return "TAF_MNGDCONN_RECOVERY_L2_SCHEDULED";
    case TAF_MNGDCONN_RECOVERY_L2_STARTED:
        return "TAF_MNGDCONN_RECOVERY_L2_STARTED";
    case TAF_MNGDCONN_RECOVERY_L2_CANCELED:
        return "TAF_MNGDCONN_RECOVERY_L2_CANCELED";
    default:
        LE_TEST_INFO("unknown recovery state: %d", static_cast<int>(state));
    }
    return "unknown recovery state";
}

static le_result_t startData(taf_mngdConn_DataRef_t dataRef)
{
    LE_TEST_INFO("----startData test " );
    le_result_t result;
    result=taf_mngdConn_StartData(dataRef);
    if (result != LE_OK && result != LE_DUPLICATE)
    {
        LE_TEST_INFO("taf_mngdConn_StartData failed: %d ", result);
    }

    return result;
}

static le_result_t stopData(taf_mngdConn_DataRef_t dataRef)
{
    LE_TEST_INFO("----stopData test " );
    le_result_t result;
    result=taf_mngdConn_StopData(dataRef);
    if (LE_NOT_PERMITTED == result)
    {
        LE_TEST_INFO("taf_mngdConn_StopData not permitted as AutoStart:Yes.");
    }
    else if (LE_NOT_POSSIBLE == result)
    {
        LE_TEST_INFO("taf_mngdConn_StopData not possible at this time. Retry in 5s.");
    }
    else if(result !=LE_OK)
    {
        LE_TEST_INFO("taf_mngdConn_StopData failed: %d ", result);
    }
    return result;
}

static le_result_t cancelL1Recovery(taf_mngdConn_DataRef_t dataRef)
{
    LE_TEST_INFO("----CancelL1Recovery test ");
    le_result_t result = LE_FAULT;
    result = taf_mngdConn_CancelL1Recovery(dataRef);
    if (result != LE_OK)
    {
        LE_TEST_INFO("taf_mngdConn_CancelL1Recovery failed: %d ", result);
    }
    return result;
}

static le_result_t cancelL2Recovery(taf_mngdConn_DataRef_t dataRef)
{
    LE_TEST_INFO("----CancelL2Recovery test ");
    le_result_t result = LE_FAULT;
    result = taf_mngdConn_CancelL2Recovery(dataRef);
    if (result != LE_OK)
    {
        LE_TEST_INFO("taf_mngdConn_CancelL2Recovery failed: %d ", result);
    }
    return result;
}

static le_result_t getConnState(taf_mngdConn_DataRef_t dataRef)
{
    LE_TEST_INFO("----GetConnState test " );
    le_result_t result = LE_FAULT;
    taf_mngdConn_DataState_t state;

    result=taf_mngdConn_GetDataConnectionState(dataRef, &state);
    if(result !=LE_OK)
        return result;

    LE_TEST_INFO("----state=%s ", DataStateToString(state).c_str());

    return result;
}

static le_result_t getConnIpAddr(taf_mngdConn_DataRef_t dataRef)
{
    LE_TEST_INFO("----GetConnIpAddr test ");
    le_result_t result = LE_FAULT;
    char ipv4Addr[TAF_DCS_IPV4_ADDR_MAX_LEN];
    char ipv6Addr[TAF_DCS_IPV6_ADDR_MAX_LEN];

    result=taf_mngdConn_GetDataConnectionIPAddresses(dataRef,
                                                     ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN,
                                                     ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN);

    LE_TEST_INFO("----result=%d" , result);

    if(result !=LE_OK)
        return result;

    LE_TEST_INFO("----IPv4Addr=%s", ipv4Addr);
    LE_TEST_INFO("----IPv6Addr=%s", ipv6Addr);

    return result;
}

static le_result_t startDataRetryAsync(taf_mngdConn_DataRef_t dataRef)
{
    LE_TEST_INFO("----StartDataRetry test ");
    le_result_t result = LE_FAULT;

    result = taf_mngdConn_StartDataRetry(dataRef);

    LE_TEST_INFO("----result=%d", result);

    return result;
}

static void RecoveryStateHandler(taf_mngdConn_RecoveryState_t recoveryState,
                                 taf_mngdConn_DataRef_t dataRef,
                                 void *contextPtr)
{
    uint8_t dataId;
    char dataName[MCS_MAX_NAME_LEN];
    size_t dataNameSize = MCS_MAX_NAME_LEN;
    LE_UNUSED(contextPtr);

    le_result_t result = taf_mngdConn_GetDataIdByRef(dataRef, &dataId);
    if (LE_OK != result)
    {
        LE_TEST_INFO("Failed to get Data ID");
    }

    result = taf_mngdConn_GetDataNameByRef(dataRef, dataName, dataNameSize);
    if (LE_OK != result)
    {
        LE_TEST_INFO("Failed to get Data Name");
    }
    LE_TEST_INFO("---data id : %d, name: %s, State : %s", dataId, dataName,
                                                RecoveryStateToString(recoveryState).c_str());

    return;
}

static void DataStateHandler
(
    taf_mngdConn_DataRef_t dataRef,
    taf_mngdConn_DataState_t dataState,
    void*  contextPtr
)
{
    uint8_t dataId;
    char dataName[MCS_MAX_NAME_LEN];
    size_t dataNameSize = MCS_MAX_NAME_LEN;
    LE_UNUSED(contextPtr);

    le_result_t result = taf_mngdConn_GetDataIdByRef(dataRef, &dataId);
    if (LE_OK != result)
    {
        LE_TEST_INFO("Failed to get Data ID");
    }
    result = taf_mngdConn_GetDataNameByRef(dataRef, dataName, dataNameSize);
    if (LE_OK != result)
    {
        LE_TEST_INFO("Failed to get Data Name");
    }

    LE_TEST_INFO("---data id : %d, name: %s, State : %s", dataId, dataName,
                                                        DataStateToString(dataState).c_str());
}

static void* HandlerThread(void* contextPtr)
{

    taf_mngdConn_DataStateHandlerRef_t     stateHandlerRef    = nullptr;
    taf_mngdConn_RecoveryStateHandlerRef_t recoveryHandlerRef = nullptr;

    taf_mngdConn_DataRef_t dataRef = (taf_mngdConn_DataRef_t)contextPtr;

    //  connect service in thread.

    taf_mngdConn_ConnectService();

    // Register data state handler
    stateHandlerRef =  taf_mngdConn_AddDataStateHandler(dataRef,
                                   (taf_mngdConn_DataStateHandlerFunc_t)DataStateHandler, NULL);
    if (nullptr == stateHandlerRef)
    {
        LE_TEST_FATAL("Unable to register for data state events");
    }

    // Register recovery state handler
    recoveryHandlerRef = taf_mngdConn_AddRecoveryStateHandler(RecoveryStateHandler, NULL);
    if (nullptr == recoveryHandlerRef)
    {
        LE_TEST_FATAL("Unable to register for recovery state events");
    }

    le_event_RunLoop();
    return NULL;
}

static le_result_t monitorState(taf_mngdConn_DataRef_t dataRef)
{
    LE_TEST_INFO("----monitorState ");
    le_result_t result = LE_FAULT;
    uint8_t dataId;
    char threadName[32];

    result = taf_mngdConn_GetDataIdByRef(dataRef, &dataId);
    if (LE_OK != result)
    {
        LE_TEST_INFO("Failed to get Data ID");
    }

    snprintf(threadName, sizeof(threadName)-1, "dataThread%d", dataId);

    le_thread_Start(le_thread_Create(threadName, HandlerThread, (void*)dataRef));

    return LE_OK;
}

static taf_mngdConn_DataRef_t getDataRef(void)
{
    int option = 0;
    taf_mngdConn_DataRef_t dataRef = NULL;
    std::cout << "Do you want to run the test for dataId or dataName" << std::endl;
    std::cout << "1 -> DataId  " << std::endl
              << "2 -> DataName  " << std::endl;
    std::cin >> option;
    if (option == 1)
    {
        int dataId = 0;
        std::cout << "Enter the DataId" << std::endl;
        std::cin >> dataId;
        dataRef = taf_mngdConn_GetData(dataId);
        if (dataRef == NULL)
        {
            LE_TEST_FATAL("Unable to get data ref for data ID %d", dataId);
        }
    }
    else if (option == 2)
    {
        char dataName[MCS_MAX_NAME_LEN];
        std::cout << "Enter the DataName" << std::endl;
        std::cin >> dataName;
        dataRef = taf_mngdConn_GetDataByName(dataName);
        if (dataRef == NULL)
        {
            LE_TEST_FATAL("Unable to get data ref for data Name %s", dataName);
        }
    }
    else
    {
        std::cerr << "You entered an invalid option";
        LE_TEST_FATAL("Invalid test command %d", option);
    }
    return dataRef;
}

COMPONENT_INIT
{
    le_result_t status = LE_OK;
    int option = 0;
    LE_TEST_INIT;

    std::cout << "For testing multi client, set AutoStart:No and start the app" << std::endl
              << "with a different ProcessName"
              << std::endl;
    while(1){
        PrintUsage();
        std::cout << "Enter the option for the test" << std::endl;
        std::cin >> option;
        switch (option)
        {
            case 0:
            {
                LE_TEST_EXIT;
            }
            case 1 :
            {
                status = startData(getDataRef());
                LE_TEST_OK(LE_OK == status || LE_DUPLICATE == status, "startdata");
            }
            break;
            case 2 :
            {
                status = stopData(getDataRef());
                LE_TEST_OK(LE_OK == status, "stopData");
            }
            break;
            case 3 :
            {
                status = getConnState(getDataRef());
                LE_TEST_OK(LE_OK == status, "getconnstate");
            }
            break;
            case 4 :
            {
                status = getConnIpAddr(getDataRef());
                LE_TEST_OK(LE_OK == status, "getipaddr");
            }
            break;
            case 5:
            {
                status = startDataRetryAsync(getDataRef());
                LE_TEST_OK(LE_OK == status, "startDataRetryAsync");
            }
            break;
            case 6 :
            {
                status = cancelL1Recovery(getDataRef());
                LE_TEST_OK(LE_OK == status, "cancelL1Recovery");
            }
            break;
            case 7 :
            {
                status = cancelL2Recovery(getDataRef());
                LE_TEST_OK(LE_OK == status, "cancelL2Recovery");
            }
            break;
            case 8 :
            {
                status = monitorState(getDataRef());
                LE_TEST_OK(LE_OK == status, "MCS Test: Monitor");
            }
            break;
            default:
                std::cerr << "You entered an invalid option";
                LE_TEST_FATAL("Invalid test command %d", option);
                break;
        }
    }
    LE_TEST_EXIT;
}
