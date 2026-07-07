/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

// ecall_ctrl.cpp
//
// Command-line controller for the eCall application.
// Parses arguments and calls the ctrlEcall IPC API exposed by eCallApp.
//
// Usage:
//   ecallctl start-auto
//   ecallctl start-manual
//   ecallctl start-test
//   ecallctl end
//   ecallctl cpu-start
//   ecallctl cpu-stop
//   ecallctl --help

#include "legato.h"
#include "interfaces.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static const char* ProgramName = "ecall";

//--------------------------------------------------------------------------------------------------
static void PrintHelp()
{
    puts(
        "\n"
        "NAME:\n"
        "    ecall - Control the eCall application.\n"
        "\n"
        "USAGE:\n"
        "    ecallctl <command>\n"
        "\n"
        "COMMANDS:\n"
        "    start-auto     Start an automatic eCall (crash-triggered).\n"
        "    start-manual   Start a manual eCall (driver button press).\n"
        "    start-test     Start a test eCall (dials configured test number).\n"
        "    end            End the active eCall session.\n"
        "    cpu-start      Start CPU profiling with default settings.\n"
        "    cpu-stop       Stop CPU profiling.\n"
        "\n"
        "EXAMPLES:\n"
        "    ecallctl start-manual\n"
        "    ecallctl end\n"
        "\n"
    );
    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
static void ReportResult(const char* cmd, le_result_t rc)
{
    if (rc == LE_OK)
    {
        LE_INFO("ecall %s: OK", cmd);
    }
    else
    {
        fprintf(stderr, "ecall %s failed: %s\n", cmd, LE_RESULT_TXT(rc));
        exit(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------------------
static void CommandHandler(const char* cmd)
{
    if (strcmp(cmd, "start-auto") == 0)
    {
        ReportResult(cmd, ctrlEcall_StartAutomatic());
    }
    else if (strcmp(cmd, "start-manual") == 0)
    {
        ReportResult(cmd, ctrlEcall_StartManual());
    }
    else if (strcmp(cmd, "start-test") == 0)
    {
        ReportResult(cmd, ctrlEcall_StartTest());
    }
    else if (strcmp(cmd, "end") == 0)
    {
        ReportResult(cmd, ctrlEcall_End());
    }
    else if (strcmp(cmd, "cpu-start") == 0)
    {
        ReportResult(cmd, ctrlEcall_CpuStart());
    }
    else if (strcmp(cmd, "cpu-stop") == 0)
    {
        ReportResult(cmd, ctrlEcall_CpuStop());
    }
    else
    {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        fprintf(stderr, "Try '%s --help'.\n", ProgramName);
        exit(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    ProgramName = le_arg_GetProgramName();
    if (!ProgramName) ProgramName = "ecall";

    le_arg_SetFlagCallback(PrintHelp, "h", "help");
    le_arg_AddPositionalCallback(CommandHandler);
    le_arg_Scan();

    exit(EXIT_SUCCESS);
}
