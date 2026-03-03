/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

typedef enum {
    TOOL_CMD_UNKNOWN = -1,
    TOOL_CMD_SUSPEND,
    TOOL_CMD_RESUME,
    TOOL_CMD_SHUTDOWN,
    TOOL_CMD_RESTART,
    TOOL_CMD_SELFCHECK,
    TOOL_CMD_MAX,
} ToolCommand_t;

static ToolCommand_t ToolCommand = TOOL_CMD_UNKNOWN;

static void (*ActionFn)(void);

#define OK   EXIT_SUCCESS
#define NOK  EXIT_FAILURE

#define Stop(result) exit(result)

static void PrintHelp(void)
{
    puts("Usage: mpms <category> <action>");
    puts("  help:");
    puts("    mpms help -- show this page");
    puts("  tool:");
    puts("    mpms tool suspend  -- Release the wakelock then graceful SUSPEND");
    puts("    mpms tool resume   -- Acquire the wakelock then stay awake");
    puts("    mpms tool shutdown -- Release the wakelock then graceful SHUTDOWN");
    puts("    mpms tool restart  -- Release the wakelock then forceful RESTART");
    puts("    mpms tool state    -- (TBD) please leverage 'pm state' instead");
}

static void Help(void)
{
    PrintHelp();
    Stop(OK);
}

#define check_StopIfFailed(result, cmdname) \
do { \
    if ((le_result_t)(result) != LE_OK) \
    { \
        fprintf(stderr, "Err: failed to send command [%s]\n", cmdname); \
        Stop(NOK); \
    } \
    else \
    { \
        printf("[%s] command sent successfully\n", cmdname); \
    } \
} while (0)

static void ToolHandler(void)
{

    le_result_t rst = LE_OK;

    rst = taf_mpms_tool_TryConnectService();

    if (rst != LE_OK)
    {
        printf("mpms.tool.daemon is NOT ready, "
               "please check: 'app status mpmsToolDaemon'\n");
        Stop(NOK);
    }

    switch (ToolCommand)
    {
        case TOOL_CMD_SUSPEND:
            rst = taf_mpms_tool_Ctrl_Suspend();
            check_StopIfFailed(rst, "suspend");
        break;

        case TOOL_CMD_RESUME:
            rst = taf_mpms_tool_Ctrl_Resume();
            check_StopIfFailed(rst, "resume");
        break;

        case TOOL_CMD_SHUTDOWN:
            rst = taf_mpms_tool_Ctrl_Shutdown();
            check_StopIfFailed(rst, "shutdown");
        break;

        case TOOL_CMD_RESTART:
            rst = taf_mpms_tool_Ctrl_Restart();
            check_StopIfFailed(rst, "restart");
        break;

        case TOOL_CMD_SELFCHECK:
            rst = taf_mpms_tool_Ctrl_Selfcheck();
            check_StopIfFailed(rst, "selfcheck");
        break;

        default:
            printf("Err: unknown tool command: %d\n", ToolCommand);
            Stop(NOK);
    }

    Stop(OK);
}

static void TestHandler(void)
{

}

static void HandlingToolSubFunction
(
    const char *action
)
{
    if (strcmp(action, "suspend") == 0)
    {
        ToolCommand = TOOL_CMD_SUSPEND;
    }
    else if (strcmp(action, "resume") == 0)
    {
        ToolCommand = TOOL_CMD_RESUME;
    }
    else if (strcmp(action, "shutdown") == 0)
    {
        ToolCommand = TOOL_CMD_SHUTDOWN;
    }
    else if (strcmp(action, "restart") == 0)
    {
        ToolCommand = TOOL_CMD_RESTART;
    }
    else if (strcmp(action, "selfcheck") == 0)
    {
        ToolCommand = TOOL_CMD_SELFCHECK;
    }
    else
    {
        fprintf(stderr, "Unknown command '%s'.  Try --help.\n", action);
        Stop(NOK);
    }
}

static void HandlingTestSubFunction
(
    const char *action
)
{
    puts("TBD ...");
    Stop(OK);
}

static void ArgsHandler
(
    const char* command
)
{
    if (strcmp(command, "help") == 0)
    {
        PrintHelp();
        Stop(OK);
    }
    else if (strcmp(command, "tool") == 0)
    {
        ActionFn = ToolHandler;
        le_arg_AddPositionalCallback(HandlingToolSubFunction);
    }
    else if (strcmp(command, "test") == 0)
    {
        ActionFn = TestHandler;
        le_arg_AddPositionalCallback(HandlingTestSubFunction);
    }
    else
    {
        fprintf(stderr, "Unknown command '%s'.  Try --help.\n", command);
        Stop(NOK);
    }
}

COMPONENT_INIT
{
    le_arg_SetFlagCallback(Help, "h", "help");
    le_arg_AddPositionalCallback(ArgsHandler);
    le_arg_Scan();

    ActionFn();
}
