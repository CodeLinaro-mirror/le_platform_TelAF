/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/*
 * @file       tafMngdConnUnitTest.cpp
 * @brief      This file is used to do the unit test for Managed Connectivity Service.
 */

#include "legato.h"
#include "interfaces.h"
#include <future>

#define MAX_DATA_ID 32
#define MCS_MAX_NAME_LEN 32
#define DEFAULT_DATA_ID 1
#define DEFAULT_DATA_NAME "Data1"

taf_mngdConn_DataStateHandlerRef_t StartDataHandlerRef = NULL;
static le_sem_Ref_t TestSemRef = NULL;
std::promise<le_result_t> StartDataPromise = std::promise<le_result_t>();
std::promise<le_result_t> StartRetryPromise = std::promise<le_result_t>();

int StartDataPromiseFlag = 0;

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

static void ConnectionStateHandler
(
    taf_mngdConn_DataRef_t dataRef,
    taf_mngdConn_DataState_t dataState,
    void*  contextPtr
)
{
    le_result_t result;
    LE_INFO("---:dataRef : %p, Connection State : %s", dataRef,
                                                       DataStateToString(dataState).c_str());
    if(strcmp(DataStateToString(dataState).c_str(), "TAF_MNGDCONN_DATA_CONNECTED")==0
        && StartDataPromiseFlag != 1)
    {
        result = LE_OK;
        StartDataPromise.set_value(result);

        // Change the flag to true once StartData is successfull and promise is set
        // This helps in checking for TAF_MNGDCONN_DATA_CONNECTED state again
        // when StartDataRetry is called

        StartDataPromiseFlag = 1;
        return;
    }
    else if(strcmp(DataStateToString(dataState).c_str(), "TAF_MNGDCONN_DATA_CONNECTED")==0
            && StartDataPromiseFlag == 1)
    {
        result = LE_OK;
        StartRetryPromise.set_value(result);
        return;
    }
}

static void* HandlerThread(void* contextPtr)
{
    //  connect service in thread.
    taf_mngdConn_ConnectService();
    taf_mngdConn_DataRef_t dataRef = (taf_mngdConn_DataRef_t)contextPtr;

    StartDataHandlerRef = taf_mngdConn_AddDataStateHandler(dataRef,
                                (taf_mngdConn_DataStateHandlerFunc_t)ConnectionStateHandler, NULL);

    le_sem_Post(TestSemRef);

    le_event_RunLoop();
    return NULL;
}

static void* UnitTestThread(void* contextPtr)
{
    le_result_t result;
    taf_mngdConn_DataState_t state;
    uint8_t dataId;
    char dataName[MCS_MAX_NAME_LEN];
    size_t dataNameSize = MCS_MAX_NAME_LEN;
    taf_mngdConn_DataRef_t dataRef = NULL;
    char ipv4Addr[TAF_DCS_IPV4_ADDR_MAX_LEN];
    char ipv6Addr[TAF_DCS_IPV6_ADDR_MAX_LEN];

    TestSemRef = le_sem_Create("testSem", 0);

    dataRef = taf_mngdConn_GetData(DEFAULT_DATA_ID);

    result = taf_mngdConn_GetDataIdByRef(dataRef, &dataId);
    LE_TEST_OK(result == LE_OK && dataId == DEFAULT_DATA_ID, "Data_Get_ID");


    dataRef = taf_mngdConn_GetDataByName(DEFAULT_DATA_NAME);

    result = taf_mngdConn_GetDataNameByRef(dataRef, dataName, dataNameSize);
    LE_TEST_OK(result == LE_OK && strcmp(dataName,DEFAULT_DATA_NAME) == 0, "Data_Get_Name");
    LE_TEST_INFO("Data_Get_Name Result: %d Data Name: %s", result, dataName);


    LE_TEST_INIT;

    LE_TEST_ASSERT(dataRef != NULL, "taf_mngdConn_GetData");

    le_thread_Ref_t mngdConnThRef = le_thread_Create("MngdConnTestTh", HandlerThread,
                                                     (void*)dataRef);

    le_thread_Start(mngdConnThRef);

    le_sem_Wait(TestSemRef);


    result = taf_mngdConn_StartData(dataRef);
    LE_TEST_OK(result == LE_OK || result == LE_TIMEOUT, "StartData");
    LE_TEST_INFO("StartData Result: %d", result);
    // blocking here to get response
    std::chrono::system_clock::time_point ten_seconds_passed
        = std::chrono::system_clock::now() + std::chrono::seconds(10);
    std::future<le_result_t> futResult = StartDataPromise.get_future();
    std::future_status status = futResult.wait_until(ten_seconds_passed);
    if (status == std::future_status::ready) {
        // Result is available
        // getting and printing the result
        if (futResult.valid()) {
            result = futResult.get();
        }
        else {
        LE_TEST_INFO("Invalid state %d", result);
        }
    } else if (status == std::future_status::timeout) {
        // Timeout occurred
        LE_TEST_INFO("Timeout occurred while starting data Result: %d", result);
    }
    LE_TEST_OK(result == LE_OK, "StartData_Connected");
    LE_TEST_INFO("StartData_Connected Result: %d", result);


    result=taf_mngdConn_GetDataConnectionState(dataRef, &state);
    LE_TEST_OK(result == LE_OK && state == TAF_MNGDCONN_DATA_CONNECTED, "ConnectionState");
    LE_TEST_INFO("ConnectionState Result: %d", result);

    result=taf_mngdConn_GetDataConnectionIPAddresses(dataRef,
                                                      ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN,
                                                      ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "ConnectionIPAddresses");
    LE_TEST_INFO("ConnectionIPAddresses Result: %d", result);
    if(result == LE_OK)
    {
        LE_TEST_INFO("ConnectionIPAddresses  ---IPv4Addr=%s", ipv4Addr);
        LE_TEST_INFO("ConnectionIPAddresses  ---IPv6Addr=%s", ipv6Addr);
    }

    result = taf_mngdConn_StartDataRetry(dataRef);
    LE_TEST_OK(result == LE_OK || result == LE_TIMEOUT, "StartDataRetry");
    LE_TEST_INFO("StartDataRetry Result: %d", result);
    // blocking here to get response
    std::chrono::system_clock::time_point forty_seconds_passed
        = std::chrono::system_clock::now() + std::chrono::seconds(40);
    std::future<le_result_t> futResult1 = StartRetryPromise.get_future();
    std::future_status status1 = futResult1.wait_until(forty_seconds_passed);
    if (status1 == std::future_status::ready) {
        // Result is available
        result = futResult1.get();
        LE_TEST_OK(result == LE_OK, "StartDataRetry_Complete");
        LE_TEST_INFO("StartDataRetry_Complete Result: %d", result);
    } else if (status1 == std::future_status::timeout) {
        // Timeout occurred
        LE_TEST_INFO("Timeout occurred in StartDataRetry Result: %d", result);
    }

    result=taf_mngdConn_StopData(dataRef);
    LE_TEST_OK(result == LE_OK || result == LE_TIMEOUT, "Data_Stop");
    LE_TEST_INFO("Data_Stop Result: %d", result);

    taf_mngdConn_RemoveDataStateHandler(StartDataHandlerRef);

    LE_TEST_EXIT;
}

COMPONENT_INIT
{
    UnitTestThread(NULL);
}
