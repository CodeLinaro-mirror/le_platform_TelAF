/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file rpcLatancyClientTest.c
 *
 * RPC latency test client component (runs in LXC container).
 *
 * Every 1 second, sends all three message-size variants (64 B, 1500 B, 16384 B)
 * to the PVM server, verifies the returned CRC-32 against a locally computed value
 * using le_crc_Crc32(), and logs the round-trip latency (in ms) for each call.
 *
 * Also registers a TestEvent handler to receive periodic events from the server.
 * Each event carries a CLOCK_MONOTONIC timestamp (nanoseconds) recorded on the
 * server side at send time.  The handler computes the one-way event delivery
 * latency and logs both the per-event latency and the running average.
 *
 * Note: LXC and PVM share the same CLOCK_MONOTONIC reference (timens_offsets = 0),
 * so the timestamp difference is a valid measure of cross-domain event latency.
 */

#include <time.h>
#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Fill a buffer with pseudo-random bytes using a simple LCG (Numerical Recipes).
 *
 * @param bufPtr    Buffer to fill.
 * @param bufSize   Number of bytes to write.
 * @param seedPtr   Seed value (updated in place for the next call).
 */
//--------------------------------------------------------------------------------------------------
static void FillRandom
(
    uint8_t*  bufPtr,
    size_t    bufSize,
    uint32_t* seedPtr
)
{
    for (size_t i = 0; i < bufSize; i++)
    {
        *seedPtr = (*seedPtr) * 1664525U + 1013904223U;   // Numerical Recipes LCG
        bufPtr[i] = (uint8_t)((*seedPtr) >> 24);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Run one round of all three latency tests and log the results.
 */
//--------------------------------------------------------------------------------------------------
static void RunLatancyTests
(
    uint32_t* seedPtr
)
{
    static uint32_t roundCnt = 0;

    /* Accumulators for average latency (ms) per message size */
    static long     shortTotalMs = 0;
    static uint32_t shortCallCnt = 0;
    static long     midTotalMs   = 0;
    static uint32_t midCallCnt   = 0;
    static long     longTotalMs  = 0;
    static uint32_t longCallCnt  = 0;

    roundCnt++;

    /* ---- Short message test (64 bytes) ---- */
    {
        uint8_t data[64];
        FillRandom(data, sizeof(data), seedPtr);

        uint32_t localCrc = le_crc_Crc32(data, sizeof(data), LE_CRC_START_CRC32);

        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        uint32_t serverCrc = rpcLatancy_ShortMessageTest(data, sizeof(data));
        clock_gettime(CLOCK_MONOTONIC, &t1);

        long latencyMs = (t1.tv_sec - t0.tv_sec) * 1000L +
                         (t1.tv_nsec - t0.tv_nsec) / 1000000L;
        shortTotalMs += latencyMs;
        shortCallCnt++;

        if (serverCrc == localCrc)
        {
            LE_INFO("[%u] ShortMessageTest ( 64 B): latency=%ld ms  avg=%ld ms  CRC=0x%08X  OK",
                    roundCnt, latencyMs, shortTotalMs / (long)shortCallCnt, serverCrc);
        }
        else
        {
            LE_ERROR("[%u] ShortMessageTest ( 64 B): CRC MISMATCH local=0x%08X server=0x%08X",
                     roundCnt, localCrc, serverCrc);
        }
    }

    /* ---- Mid message test (1500 bytes) ---- */
    {
        uint8_t data[1500];
        FillRandom(data, sizeof(data), seedPtr);

        uint32_t localCrc = le_crc_Crc32(data, sizeof(data), LE_CRC_START_CRC32);

        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        uint32_t serverCrc = rpcLatancy_MidMessageTest(data, sizeof(data));
        clock_gettime(CLOCK_MONOTONIC, &t1);

        long latencyMs = (t1.tv_sec - t0.tv_sec) * 1000L +
                         (t1.tv_nsec - t0.tv_nsec) / 1000000L;
        midTotalMs += latencyMs;
        midCallCnt++;

        if (serverCrc == localCrc)
        {
            LE_INFO("[%u] MidMessageTest  (1500 B): latency=%ld ms  avg=%ld ms  CRC=0x%08X  OK",
                    roundCnt, latencyMs, midTotalMs / (long)midCallCnt, serverCrc);
        }
        else
        {
            LE_ERROR("[%u] MidMessageTest  (1500 B): CRC MISMATCH local=0x%08X server=0x%08X",
                     roundCnt, localCrc, serverCrc);
        }
    }

    /* ---- Long message test (16384 bytes) ---- */
    {
        uint8_t data[16384];
        FillRandom(data, sizeof(data), seedPtr);

        uint32_t localCrc = le_crc_Crc32(data, sizeof(data), LE_CRC_START_CRC32);

        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        uint32_t serverCrc = rpcLatancy_LongMessageTest(data, sizeof(data));
        clock_gettime(CLOCK_MONOTONIC, &t1);

        long latencyMs = (t1.tv_sec - t0.tv_sec) * 1000L +
                         (t1.tv_nsec - t0.tv_nsec) / 1000000L;
        longTotalMs += latencyMs;
        longCallCnt++;

        if (serverCrc == localCrc)
        {
            LE_INFO("[%u] LongMessageTest (16384 B): latency=%ld ms  avg=%ld ms  CRC=0x%08X  OK",
                    roundCnt, latencyMs, longTotalMs / (long)longCallCnt, serverCrc);
        }
        else
        {
            LE_ERROR("[%u] LongMessageTest (16384 B): CRC MISMATCH local=0x%08X server=0x%08X",
                     roundCnt, localCrc, serverCrc);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * TestEvent handler – called whenever the server fires a TestEvent.
 *
 * Computes the one-way event delivery latency by comparing the server-side
 * send timestamp (CLOCK_MONOTONIC, nanoseconds) with the current time on the
 * client side.  Logs the per-event latency and the running average.
 *
 * @param message    Null-terminated event message string from the server.
 * @param timestamp  Server send timestamp in nanoseconds (CLOCK_MONOTONIC).
 * @param contextPtr Unused context pointer.
 */
//--------------------------------------------------------------------------------------------------
static void TestEventHandler
(
    const char* message,
    uint64_t    timestamp,
    void*       contextPtr
)
{
    LE_UNUSED(contextPtr);

    static long     totalMs = 0;
    static uint32_t callCnt = 0;

    /* Compute one-way event delivery latency.
     * LXC and PVM share the same CLOCK_MONOTONIC reference (timens_offsets = 0),
     * so the difference is a valid cross-domain latency measurement. */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    long latencyMs = (long)((now - timestamp) / 1000000ULL);
    totalMs += latencyMs;
    callCnt++;

    LE_INFO("TestEvent: latency=%ld ms  avg=%ld ms  msg=%s",
            latencyMs, totalMs / (long)callCnt, message);
}

//--------------------------------------------------------------------------------------------------
/**
 * Periodic timer handler – fires every 1 second.
 * Runs one round of all three RPC latency tests (64 B, 1500 B, 16384 B).
 */
//--------------------------------------------------------------------------------------------------
static void TimerHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_UNUSED(timerRef);

    static uint32_t seed = 0x12345678U;
    RunLatancyTests(&seed);
}

//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_timer_Ref_t timerRef = le_timer_Create("LatancyTestTimer");
    le_timer_SetMsInterval(timerRef, 1000);
    le_timer_SetHandler(timerRef, TimerHandler);
    le_timer_SetRepeat(timerRef, 0);       // 0 = repeat forever
    le_timer_SetWakeup(timerRef, false);
    le_timer_Start(timerRef);

    rpcLatancy_AddTestEventHandler(TestEventHandler, NULL);

    LE_INFO("RPC latency test client started.");
}
