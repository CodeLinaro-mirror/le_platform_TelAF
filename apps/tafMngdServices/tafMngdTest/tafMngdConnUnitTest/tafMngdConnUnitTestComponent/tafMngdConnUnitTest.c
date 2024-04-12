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
 * @file       tafMngdConnUnitTest.cpp
 * @brief      This file is used to do the unit test for Managed Connectivity Service.
 */

#include "legato.h"
#include "interfaces.h"

#define MAX_DATA_ID 32
#define DEFAULT_DATA_ID 1

taf_mngdConn_DataStateHandlerRef_t statHandlerRef = NULL;
static le_sem_Ref_t TestSemRef = NULL;

static char * StateToString(taf_mngdConn_DataState_t state)
{
    switch (state)
    {
        case TAF_MNGDCONN_DATA_CONNECTED:
            return "TAF_MNGDCONN_DATA_CONNECTED";
        case TAF_MNGDCONN_DATA_DISCONNECTED:
            return "TAF_MNGDCONN_DATA_DISCONNECTED";
        default:
            LE_ERROR("unknown status: %d", (int)state);
            return "unknow status";
    }
    return "unknow status";
}

static void ConnectionStateHandler
(
    taf_mngdConn_DataRef_t dataRef,
    taf_mngdConn_DataState_t dataState,
    void*  contextPtr
)
{

    LE_INFO("---:dataRef : %p, Connection State : %s", dataRef, StateToString(dataState));

}

static void* HandlerThread(void* contextPtr)
{
    //  connect service in thread.
    taf_mngdConn_ConnectService();
    taf_mngdConn_DataRef_t dataRef = (taf_mngdConn_DataRef_t)contextPtr;

    statHandlerRef = taf_mngdConn_AddDataStateHandler(dataRef,
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
    taf_mngdConn_DataRef_t dataRef = NULL;
    char ipv4Addr[TAF_DCS_IPV4_ADDR_MAX_LEN];
    char ipv6Addr[TAF_DCS_IPV6_ADDR_MAX_LEN];

    TestSemRef = le_sem_Create("testSem", 0);

    dataRef = taf_mngdConn_GetData(DEFAULT_DATA_ID);

    LE_TEST_INIT;

    LE_TEST_ASSERT(dataRef != NULL, "taf_mngdConn_GetData");

    le_thread_Ref_t mngdConnThRef = le_thread_Create("MngdConnTestTh", HandlerThread,
                                                     (void*)dataRef);

    le_thread_Start(mngdConnThRef);

    le_sem_Wait(TestSemRef);


    result = taf_mngdConn_DataStart(dataRef);
    LE_TEST_OK(result == LE_OK, "Data_Start");
    LE_TEST_INFO("Data_Start Result: %d", result);

    result=taf_mngdConn_DataGetConnectionState(dataRef, &dataId, &state);
    LE_TEST_OK(result == LE_OK, "ConnectionState");
    LE_TEST_INFO("ConnectionState Result: %d", result);

    result=taf_mngdConn_DataGetConnectionIPAddresses(dataRef,
                                                      ipv4Addr, TAF_DCS_IPV4_ADDR_MAX_LEN,
                                                      ipv6Addr, TAF_DCS_IPV6_ADDR_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "ConnectionIPAddresses");
    LE_TEST_INFO("ConnectionIPAddresses Result: %d", result);

    sleep(3);

    result=taf_mngdConn_DataStop(dataRef);
    LE_TEST_OK(result == LE_OK, "Data_Stop");
    LE_TEST_INFO("Data_Stop Result: %d", result);

    sleep(3);

    taf_mngdConn_RemoveDataStateHandler(statHandlerRef);

    LE_TEST_EXIT;
}

COMPONENT_INIT
{
    UnitTestThread(NULL);
}
