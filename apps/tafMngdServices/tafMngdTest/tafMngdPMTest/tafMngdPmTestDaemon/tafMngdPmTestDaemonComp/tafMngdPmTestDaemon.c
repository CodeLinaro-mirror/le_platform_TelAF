/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

static taf_mpms_test_MpmsTestRef_t MpmsTestInstance;
static le_ref_MapRef_t MpmsTestInstanceMap;
static taf_mngdPm_wsRef_t wsRef;

#define MAX_LOG_LENGTH 512

static void LogToConsole(const char *format, ...)
{
    int consoleFd = open("/dev/console", O_WRONLY);
    if (consoleFd == -1)
    {
        LE_ERROR("Failed to open the console: %m");
        return;
    }

    char buffer[MAX_LOG_LENGTH];
    va_list args;

    va_start(args, format);
    int formattedLength = vsnprintf(buffer, MAX_LOG_LENGTH, format, args);
    va_end(args);

    if (formattedLength >= MAX_LOG_LENGTH)
    {
        LE_ERROR("Log message truncated (max length %d)", MAX_LOG_LENGTH);
    }
    else if (formattedLength < 0)
    {
        LE_ERROR("Failed to format log message");
        close(consoleFd);
        return;
    }

    ssize_t wBytes = write(consoleFd, buffer, formattedLength);
    if (wBytes == -1)
    {
        LE_ERROR("Failed to write to console: %m");
    }
    else if ((size_t)wBytes != (size_t)formattedLength)
    {
        LE_ERROR("Incomplete write to console: wrote %zd of %d bytes",
                 wBytes, formattedLength);
    }

    if (close(consoleFd) == -1)
    {
        LE_ERROR("Failed to close console fd: %m");
    }
}

static le_result_t AcquireWakeSource()
{
    le_result_t res = taf_mngdPm_StayAwake(wsRef);
    if(res == LE_OK) {
        LE_TEST_OK(res == LE_OK, "wake source acquired successfully");
    } else {
        LE_ERROR("Failed to acquire the wake source");
    }

    return res;
}

static le_result_t ReleaseWakeSource()
{
    le_result_t res = taf_mngdPm_Relax(wsRef);
    if(res == LE_OK) {
        LE_TEST_OK(res == LE_OK, "wake source released successfully");
    } else {
        LE_ERROR("Failed to release the wake source");
    }

    return res;
}

void CallbackHandler
(
    taf_mngdPm_NodeModemAwakeEventRef_t ref,
    uint8_t pmNodeId,
    taf_mngdPm_NodeModemWsBitMask_t wsBitmask,
    void* contextPtr
)
{
    LE_UNUSED(ref);
    LE_UNUSED(contextPtr);

    le_result_t rst = AcquireWakeSource();
    LE_INFO("Acquired Wake source %s", LE_RESULT_TXT(rst));

    LogToConsole("-> [TelAF] MPMS Test Daemon Wakeup Monitor\n");
    LE_INFO("-> [TelAF] MPMS Test Daemon Wakeup Monitor\n");

    LE_INFO("node id: 0x%0x", pmNodeId);
    LE_INFO("bitmask: 0x%04x", wsBitmask);

    taf_mngdPm_NodeModemWsBitMask_t bitset;
    rst = taf_mngdPm_GetNodeModemAwakeReason(0, &bitset);

    if (rst == LE_OK)
    {
        LE_INFO("Got the wakeup reason (bitmask): 0x%04x", bitset);
        LogToConsole("--> Got the wakeup reason (bitmask): 0x%04x\n", bitset);
    }
    else
    {
        LogToConsole("--> Failed to taf_mngdPm_GetNodeModemAwakeReason\n");
        LE_ERROR("Failed to taf_mngdPm_GetNodeModemAwakeReason: %s",
                 LE_RESULT_TXT(rst));
    }
}

static taf_mngdPm_NodeModemAwakeHandlerRef_t WakeupHandlerRef;

static le_result_t GetFilter(uint32_t * bitset)
{
    return taf_mngdPm_GetNodeModemWakeupSel(0,
            (taf_mngdPm_NodeModemWsBitMask_t *) bitset);
}

static le_result_t SetFilter(uint32_t bitsetInput)
{
    le_result_t rst =
        taf_mngdPm_SetNodeModemWakeupSel(0,
            (taf_mngdPm_NodeModemWsBitMask_t) bitsetInput);

    if (rst != LE_OK)
    {
        LE_ERROR("Failed to taf_mngdPm_SetNodeModemWakeupSel");
        return rst;
    }

    uint32_t bitsetOutput = (uint32_t)(-1);

    rst = GetFilter(&bitsetOutput);

    if (rst != LE_OK)
    {
        LE_ERROR("Failed to taf_mngdPm_GetNodeModemWakeupSel");
        return rst;
    }

    LE_INFO("Set: 0x%08x, Get: 0x%08x\n",
           bitsetInput, bitsetOutput);

    return LE_OK;
}

static le_result_t UpMonitor(void)
{
    if (WakeupHandlerRef == NULL)
    {
        WakeupHandlerRef = taf_mngdPm_AddNodeModemAwakeHandler(CallbackHandler, NULL, 0, 0);

        if (WakeupHandlerRef == NULL)
        {
            LE_ERROR("Failed to taf_mngdPm_AddNodeModemAwakeHandler");
            return LE_FAULT;
        }
    }
    else
    {
        LE_ERROR("Can NOT register the same callback handlere\n");
        return LE_DUPLICATE;
    }

    return LE_OK;
}

static void DownMonitor(void)
{
    if (WakeupHandlerRef != NULL)
    {
        taf_mngdPm_RemoveNodeModemAwakeHandler(WakeupHandlerRef);
        WakeupHandlerRef = NULL;
        LE_INFO("Removed the callback handler");
    }
    else
    {
        LE_INFO("Do NOT need to remove callback handler, unregistered");
    }
}

static le_result_t QueryLastWakeupReason(uint32_t * outputBitset)
{
    le_result_t rst =
        taf_mngdPm_GetNodeModemAwakeReason(0,
            (taf_mngdPm_NodeModemWsBitMask_t *) outputBitset);

    if (rst != LE_OK)
    {
        LE_ERROR("Failed to taf_mngdPm_GetNodeModemAwakeReason");
    }

    return rst;
}

// ------------------- API -------------------

le_result_t taf_mpms_test_GetMpmsTestObject
(
    taf_mpms_test_MpmsTestRef_t* refPtrPtr
)
{
    if (refPtrPtr == NULL)
    {
        LE_ERROR("Bad refPtrPtr");
        return LE_BAD_PARAMETER;
    }

    *refPtrPtr = MpmsTestInstance;
    return LE_OK;
}

le_result_t taf_mpms_test_GetWakeupFilterFromMpss
(
    taf_mpms_test_MpmsTestRef_t ref,
    uint32_t* bitmaskPtr
)
{
    if (ref == NULL || ref != MpmsTestInstance)
    {
        LE_ERROR("Bad reference");
        return LE_BAD_PARAMETER;
    }

    if (bitmaskPtr == NULL)
    {
        LE_ERROR("Bad bitmaskPtr");
        return LE_BAD_PARAMETER;
    }

    return GetFilter(bitmaskPtr);
}

le_result_t taf_mpms_test_SetWakeupFitlerToMpss
(
    taf_mpms_test_MpmsTestRef_t ref,
    uint32_t bitmask
)
{
    if (ref == NULL || ref != MpmsTestInstance)
    {
        LE_ERROR("Bad reference");
        return LE_BAD_PARAMETER;
    }

    return SetFilter(bitmask);
}

le_result_t taf_mpms_test_UpMonitorForWakeup
(
    taf_mpms_test_MpmsTestRef_t ref
)
{
    if (ref == NULL || ref != MpmsTestInstance)
    {
        LE_ERROR("Bad reference");
        return LE_BAD_PARAMETER;
    }

    return UpMonitor();
}

void taf_mpms_test_DownMonitorForWakeup
(
    taf_mpms_test_MpmsTestRef_t ref
)
{
    if (ref == NULL || ref != MpmsTestInstance)
    {
        LE_ERROR("Bad reference");
        return;
    }

    DownMonitor();
}


le_result_t taf_mpms_test_QueryLastWakeupReason
(
    taf_mpms_test_MpmsTestRef_t ref,
    uint32_t * bitset
)
{
    if (ref == NULL || ref != MpmsTestInstance)
    {
        LE_ERROR("Bad reference");
        return LE_FAULT;
    }

    return QueryLastWakeupReason(bitset);
}

le_result_t taf_mpms_test_RelaxWakeSource
(
    taf_mpms_test_MpmsTestRef_t ref
)
{
    if (ref == NULL || ref != MpmsTestInstance)
    {
        LE_ERROR("Bad reference");
        return LE_FAULT;
    }

    return ReleaseWakeSource();
}

// ------------------- API -------------------

COMPONENT_INIT
{
    static int Instance;

    MpmsTestInstanceMap =
                le_ref_CreateMap("mpms-test-inst-map", 1);

    MpmsTestInstance =
        (taf_mpms_test_MpmsTestRef_t)
            le_ref_CreateRef(MpmsTestInstanceMap, &Instance);

    wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
                                            TAF_MNGDPM_WS_OPT_DEFAULT,
                                                  "testDaemonWs");
    if(wsRef != NULL) {
        LE_TEST_OK(wsRef != NULL, "wakeup source created successfully");
    } else {
        LE_ERROR("Failed to create thewakeup source");
    }

    LE_INFO("mpms test daemon up");
}
