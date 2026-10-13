/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define HELP_TEXT_LEN 4096
#define TEST_DOC_LEN 2048
#define REASON_LEN 256
#define TEST_ID_LEN 32
#define STATE_LEN 64
#define REPORT_LEN 64
#define MONITOR_STATUS_LEN 128

static void StopCommand
(
    le_result_t result,
    const char* reason
)
{
    if (result != LE_OK)
    {
        fprintf(stderr, "%s: %s\n", LE_RESULT_TXT(result),
                reason != NULL && reason[0] != '\0' ? reason : "operation failed");
        exit(EXIT_FAILURE);
    }

    if (reason != NULL && reason[0] != '\0')
    {
        puts(reason);
    }

    exit(EXIT_SUCCESS);
}

static void FinishControl
(
    const char* command,
    le_result_t result,
    const char* reason
)
{
    if (result != LE_OK)
    {
        fprintf(stderr, "%s failed (%s): %s\n", command, LE_RESULT_TXT(result),
                reason != NULL && reason[0] != '\0' ? reason : "operation failed");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

static void PrintFormattedTestDoc
(
    const char* testId,
    const char* helpDoc
)
{
    const char* line = helpDoc;

    printf("[%s]:\n", testId);
    while (line != NULL && *line != '\0')
    {
        const char* end = strchr(line, '\n');

        fputs("  ", stdout);
        if (end == NULL)
        {
            puts(line);
            return;
        }

        fwrite(line, 1, (size_t)(end - line), stdout);
        putchar('\n');
        line = end + 1;
    }
}

static le_result_t ParseHex
(
    const char* text,
    uint32_t* value
)
{
    char* endPtr = NULL;
    unsigned long parsed;

    if (text == NULL || value == NULL || text[0] == '\0')
    {
        return LE_BAD_PARAMETER;
    }

    parsed = strtoul(text, &endPtr, 0);
    if (endPtr == text || endPtr == NULL || *endPtr != '\0' || parsed > 0xFFFFFFFFUL)
    {
        return LE_BAD_PARAMETER;
    }

    *value = (uint32_t)parsed;
    return LE_OK;
}

static void PrintHelp
(
    void
)
{
    char helpText[HELP_TEXT_LEN] = {0};

    taf_mngdPmTest_ConnectService();
    le_result_t result = taf_mngdPmTest_GetHelp(helpText, sizeof(helpText));
    StopCommand(result, helpText);
}

static void PrintTestDoc
(
    const char* testId
)
{
    char helpDoc[TEST_DOC_LEN] = {0};

    taf_mngdPmTest_ConnectService();
    le_result_t result = taf_mngdPmTest_GetTestCaseDoc(testId, helpDoc, sizeof(helpDoc));
    if (result != LE_OK)
    {
        StopCommand(result, "failed to retrieve test case help document");
    }

    PrintFormattedTestDoc(testId, helpDoc);
    exit(EXIT_SUCCESS);
}

static void PrintAllTestDocs
(
    void
)
{
    char testId[TEST_ID_LEN] = {0};
    char helpDoc[TEST_DOC_LEN] = {0};

    taf_mngdPmTest_ConnectService();

    for (uint32_t index = 0; ; index++)
    {
        memset(testId, 0, sizeof(testId));
        memset(helpDoc, 0, sizeof(helpDoc));

        le_result_t result = taf_mngdPmTest_GetTestCaseDocByIndex(index,
                                                                testId,
                                                                sizeof(testId),
                                                                helpDoc,
                                                                sizeof(helpDoc));
        if (result == LE_NOT_FOUND)
        {
            break;
        }

        if (result != LE_OK)
        {
            StopCommand(result, "failed to list integration test cases");
        }

        PrintFormattedTestDoc(testId, helpDoc);
    }

    exit(EXIT_SUCCESS);
}

static void RunTest
(
    const char* testId
)
{
    char helpDoc[TEST_DOC_LEN] = {0};
    char reason[REASON_LEN] = {0};

    taf_mngdPmTest_ConnectService();

    le_result_t result = taf_mngdPmTest_GetTestCaseDoc(testId, helpDoc, sizeof(helpDoc));
    if (result != LE_OK)
    {
        StopCommand(result, "failed to retrieve integration test help document");
    }

    PrintFormattedTestDoc(testId, helpDoc);
    putchar('\n');

    result = taf_mngdPmTest_RunTest(testId, reason, sizeof(reason));
    StopCommand(result, reason);
}

static void RunMonitorCommand
(
    void
)
{
    const char* action = le_arg_GetArg(1);
    char reason[REASON_LEN] = {0};
    le_result_t result;

    if (action == NULL)
    {
        StopCommand(LE_BAD_PARAMETER,
                    "usage: mpmsIntgTest monitorWakeup start [keepResume] | stop | release | status");
    }

    taf_mngdPmTest_ConnectService();

    if (strcmp(action, "start") == 0)
    {
        taf_mngdPmTest_WakeupMonitorMode_t mode =
            TAF_MNGDPMTEST_WAKEUP_MONITOR_OBSERVE;

        if (le_arg_NumArgs() == 3)
        {
            if (strcmp(le_arg_GetArg(2), "keepResume") != 0)
            {
                StopCommand(LE_BAD_PARAMETER,
                            "monitorWakeup start accepts only keepResume as an optional mode");
            }
            mode = TAF_MNGDPMTEST_WAKEUP_MONITOR_KEEP_RESUME;
        }
        else if (le_arg_NumArgs() != 2)
        {
            StopCommand(LE_BAD_PARAMETER,
                        "usage: mpmsIntgTest monitorWakeup start [keepResume]");
        }

        result = taf_mngdPmTest_monitorWakeup(mode, reason, sizeof(reason));
        FinishControl("monitorWakeup start", result, reason);
    }
    else if (strcmp(action, "stop") == 0 && le_arg_NumArgs() == 2)
    {
        result = taf_mngdPmTest_stopWakeupMonitor(reason, sizeof(reason));
        FinishControl("monitorWakeup stop", result, reason);
    }
    else if (strcmp(action, "release") == 0 && le_arg_NumArgs() == 2)
    {
        result = taf_mngdPmTest_releaseWakeupLock(reason, sizeof(reason));
        FinishControl("monitorWakeup release", result, reason);
    }
    else if (strcmp(action, "status") == 0 && le_arg_NumArgs() == 2)
    {
        char status[MONITOR_STATUS_LEN] = {0};

        result = taf_mngdPmTest_getWakeupMonitorStatus(status,
                                                    sizeof(status),
                                                    reason,
                                                    sizeof(reason));
        if (result != LE_OK)
        {
            FinishControl("monitorWakeup status", result, reason);
        }

        puts(status);
        exit(EXIT_SUCCESS);
    }

    StopCommand(LE_BAD_PARAMETER,
                "usage: mpmsIntgTest monitorWakeup start [keepResume] | stop | release | status");
}

static void RunControl
(
    const char* command
)
{
    char reason[REASON_LEN] = {0};
    le_result_t result;

    taf_mngdPmTest_ConnectService();

    if (strcmp(command, "connect") == 0 && le_arg_NumArgs() == 1)
    {
        result = taf_mngdPmTest_connect(reason, sizeof(reason));
        FinishControl(command, result, reason);
    }
    else if (strcmp(command, "disconnect") == 0 && le_arg_NumArgs() == 1)
    {
        result = taf_mngdPmTest_disconnect(reason, sizeof(reason));
        FinishControl(command, result, reason);
    }
    else if (strcmp(command, "state") == 0 && le_arg_NumArgs() == 1)
    {
        char powerState[STATE_LEN] = {0};

        result = taf_mngdPmTest_state(powerState,
                                   sizeof(powerState),
                                   reason,
                                   sizeof(reason));
        if (result != LE_OK)
        {
            FinishControl(command, result, reason);
        }

        puts(powerState);
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "resume") == 0 && le_arg_NumArgs() == 1)
    {
        result = taf_mngdPmTest_resume(reason, sizeof(reason));
        FinishControl(command, result, reason);
    }
    else if (strcmp(command, "suspend") == 0 && le_arg_NumArgs() == 1)
    {
        result = taf_mngdPmTest_suspend(reason, sizeof(reason));
        FinishControl(command, result, reason);
    }
    else if (strcmp(command, "shutdown") == 0 &&
             (le_arg_NumArgs() == 1 || le_arg_NumArgs() == 2))
    {
        taf_mngdPmTest_ShutdownMode_t mode = TAF_MNGDPMTEST_SHUTDOWN_GRACEFUL;

        if (le_arg_NumArgs() == 2)
        {
            if (strcmp(le_arg_GetArg(1), "graceful") == 0)
            {
                mode = TAF_MNGDPMTEST_SHUTDOWN_GRACEFUL;
            }
            else if (strcmp(le_arg_GetArg(1), "forceful") == 0)
            {
                mode = TAF_MNGDPMTEST_SHUTDOWN_FORCEFUL;
            }
            else
            {
                StopCommand(LE_BAD_PARAMETER, "shutdown mode must be graceful or forceful");
            }
        }

        result = taf_mngdPmTest_shutdown(mode, reason, sizeof(reason));
        FinishControl(command, result, reason);
    }
    else if (strcmp(command, "restart") == 0 && le_arg_NumArgs() == 1)
    {
        result = taf_mngdPmTest_restart(reason, sizeof(reason));
        FinishControl(command, result, reason);
    }
    else if (strcmp(command, "getInfoReport") == 0 && le_arg_NumArgs() == 1)
    {
        char report[REPORT_LEN] = {0};

        result = taf_mngdPmTest_getInfoReport(report,
                                           sizeof(report),
                                           reason,
                                           sizeof(reason));
        if (result != LE_OK)
        {
            FinishControl(command, result, reason);
        }

        puts(report);
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "setWakeupFilter") == 0 && le_arg_NumArgs() == 2)
    {
        uint32_t filter;

        result = ParseHex(le_arg_GetArg(1), &filter);
        if (result != LE_OK)
        {
            StopCommand(result, "wakeup filter must be a valid hexadecimal uint32");
        }

        result = taf_mngdPmTest_setWakeupFilter(filter, reason, sizeof(reason));
        FinishControl(command, result, reason);
    }
    else if (strcmp(command, "getWakeupFilter") == 0 && le_arg_NumArgs() == 1)
    {
        uint32_t filter = 0;

        result = taf_mngdPmTest_getWakeupFilter(&filter, reason, sizeof(reason));
        if (result != LE_OK)
        {
            FinishControl(command, result, reason);
        }

        printf("0x%08x\n", (unsigned int)filter);
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "getWakeupReason") == 0 && le_arg_NumArgs() == 1)
    {
        uint32_t reasonMask = 0;

        result = taf_mngdPmTest_getWakeupReason(&reasonMask, reason, sizeof(reason));
        if (result != LE_OK)
        {
            FinishControl(command, result, reason);
        }

        printf("0x%08x\n", (unsigned int)reasonMask);
        exit(EXIT_SUCCESS);
    }
    else
    {
        StopCommand(LE_BAD_PARAMETER, "invalid control command arguments");
    }
}

static void CommandHandler
(
    const char* command
)
{
    if (command == NULL)
    {
        StopCommand(LE_BAD_PARAMETER, "missing command");
    }

    if (strcmp(command, "help") == 0 && le_arg_NumArgs() == 1)
    {
        PrintHelp();
    }

    if (strcmp(command, "list") == 0 && le_arg_NumArgs() == 1)
    {
        PrintAllTestDocs();
    }

    if (strcmp(command, "list") == 0 && le_arg_NumArgs() == 2)
    {
        PrintTestDoc(le_arg_GetArg(1));
    }

    if (strcmp(command, "help") == 0)
    {
        StopCommand(LE_BAD_PARAMETER, "usage: mpmsIntgTest help");
    }

    if (strcmp(command, "list") == 0)
    {
        StopCommand(LE_BAD_PARAMETER, "usage: mpmsIntgTest list [caseId]");
    }

    if (strcmp(command, "monitorWakeup") == 0)
    {
        RunMonitorCommand();
    }

    if (strcmp(command, "connect") == 0 ||
        strcmp(command, "disconnect") == 0 ||
        strcmp(command, "state") == 0 ||
        strcmp(command, "resume") == 0 ||
        strcmp(command, "suspend") == 0 ||
        strcmp(command, "shutdown") == 0 ||
        strcmp(command, "restart") == 0 ||
        strcmp(command, "getInfoReport") == 0 ||
        strcmp(command, "setWakeupFilter") == 0 ||
        strcmp(command, "getWakeupFilter") == 0 ||
        strcmp(command, "getWakeupReason") == 0)
    {
        RunControl(command);
    }

    if (le_arg_NumArgs() == 1)
    {
        RunTest(command);
    }

    StopCommand(LE_BAD_PARAMETER, "unknown command");
}

COMPONENT_INIT
{
    LE_INFO("mpmsIntgTest CLI started");
    le_arg_AddPositionalCallback(CommandHandler);
    le_arg_Scan();

    if (le_arg_NumArgs() == 0)
    {
        StopCommand(LE_BAD_PARAMETER,
                    "usage: mpmsIntgTest help | list [caseId] | <caseId>");
    }
}
