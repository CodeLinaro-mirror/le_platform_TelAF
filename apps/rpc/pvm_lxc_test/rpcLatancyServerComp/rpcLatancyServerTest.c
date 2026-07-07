/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file rpcLatancyServerTest.c
 *
 * RPC latency test server component.
 * Implements ShortMessageTest, MidMessageTest and LongMessageTest APIs.
 * Each handler computes the CRC-32 checksum of the received payload using
 * le_crc_Crc32() and returns the result to the caller.
 *
 * Also implements AddTestEventHandler / RemoveTestEventHandler and fires a
 * TestEvent every 1 second with a message string "test event message <cnt>"
 * and a CLOCK_MONOTONIC timestamp (nanoseconds) so the client can measure
 * event delivery latency.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
// TestEvent handler registry
//--------------------------------------------------------------------------------------------------

typedef struct
{
    rpcLatancy_TestEventHandlerRef_t  ref;
    rpcLatancy_TestEventHandlerFunc_t handlerFunc;
    void*                             context;
} TestEventHandler_t;

static le_mem_PoolRef_t TestEventHandlerPool   = NULL;
static le_ref_MapRef_t  TestEventHandlerRefMap = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Register a TestEvent handler.
 */
//--------------------------------------------------------------------------------------------------
rpcLatancy_TestEventHandlerRef_t rpcLatancy_AddTestEventHandler
(
    rpcLatancy_TestEventHandlerFunc_t handlerPtr,
    void*                             contextPtr
)
{
    TestEventHandler_t* objPtr =
        (TestEventHandler_t*)le_mem_ForceAlloc(TestEventHandlerPool);
    memset(objPtr, 0, sizeof(TestEventHandler_t));

    objPtr->handlerFunc = handlerPtr;
    objPtr->context     = contextPtr;
    objPtr->ref         = (rpcLatancy_TestEventHandlerRef_t)
                              le_ref_CreateRef(TestEventHandlerRefMap, objPtr);

    LE_INFO("TestEventHandler registered: ref=%p", objPtr->ref);
    return objPtr->ref;
}

//--------------------------------------------------------------------------------------------------
/**
 * Unregister a TestEvent handler.
 */
//--------------------------------------------------------------------------------------------------
void rpcLatancy_RemoveTestEventHandler
(
    rpcLatancy_TestEventHandlerRef_t handlerRef
)
{
    TestEventHandler_t* objPtr =
        (TestEventHandler_t*)le_ref_Lookup(TestEventHandlerRefMap, handlerRef);

    if (objPtr != NULL)
    {
        le_ref_DeleteRef(TestEventHandlerRefMap, handlerRef);
        le_mem_Release(objPtr);
        LE_INFO("TestEventHandler unregistered: ref=%p", handlerRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Periodic timer handler – fires every 1 second.
 * Records a CLOCK_MONOTONIC timestamp, then iterates all registered
 * TestEvent handlers and calls each one with the message and timestamp.
 */
//--------------------------------------------------------------------------------------------------
static void EventTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_UNUSED(timerRef);

    static uint32_t cnt = 0;
    cnt++;

    char msg[256];
    snprintf(msg, sizeof(msg), "test event message %u", cnt);

    /* Capture send timestamp (nanoseconds, CLOCK_MONOTONIC) */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t timestamp = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;

    le_ref_IterRef_t iterRef = le_ref_GetIterator(TestEventHandlerRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        TestEventHandler_t* objPtr =
            (TestEventHandler_t*)le_ref_GetValue(iterRef);
        objPtr->handlerFunc(msg, timestamp, objPtr->context);
    }

    LE_INFO("TestEvent sent: %s", msg);
}

//--------------------------------------------------------------------------------------------------
/**
 * ShortMessageTest handler – 64-byte payload.
 */
//--------------------------------------------------------------------------------------------------
uint32_t rpcLatancy_ShortMessageTest
(
    const uint8_t* dataPtr,
    size_t         dataSize
)
{
    uint32_t crc = le_crc_Crc32((uint8_t*)dataPtr, dataSize, LE_CRC_START_CRC32);
    LE_DEBUG("ShortMessageTest: dataSize=%zu crc=0x%08X", dataSize, crc);
    return crc;
}

//--------------------------------------------------------------------------------------------------
/**
 * MidMessageTest handler – 1500-byte payload.
 */
//--------------------------------------------------------------------------------------------------
uint32_t rpcLatancy_MidMessageTest
(
    const uint8_t* dataPtr,
    size_t         dataSize
)
{
    uint32_t crc = le_crc_Crc32((uint8_t*)dataPtr, dataSize, LE_CRC_START_CRC32);
    LE_DEBUG("MidMessageTest: dataSize=%zu crc=0x%08X", dataSize, crc);
    return crc;
}

//--------------------------------------------------------------------------------------------------
/**
 * LongMessageTest handler – 16384-byte payload.
 */
//--------------------------------------------------------------------------------------------------
uint32_t rpcLatancy_LongMessageTest
(
    const uint8_t* dataPtr,
    size_t         dataSize
)
{
    uint32_t crc = le_crc_Crc32((uint8_t*)dataPtr, dataSize, LE_CRC_START_CRC32);
    LE_DEBUG("LongMessageTest: dataSize=%zu crc=0x%08X", dataSize, crc);
    return crc;
}

//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("RPC latency test server started.");

    TestEventHandlerPool   = le_mem_CreatePool("TestEventHandlerPool",
                                               sizeof(TestEventHandler_t));
    TestEventHandlerRefMap = le_ref_CreateMap("TestEventHandlerRefMap", 16);

    le_timer_Ref_t timerRef = le_timer_Create("EventTimer");
    le_timer_SetMsInterval(timerRef, 1000);
    le_timer_SetHandler(timerRef, EventTimerHandler);
    le_timer_SetRepeat(timerRef, 0);       // 0 = repeat forever
    le_timer_SetWakeup(timerRef, false);
    le_timer_Start(timerRef);
}
