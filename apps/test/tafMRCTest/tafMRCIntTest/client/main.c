/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define CMD_HELP    "help"
#define CMD_START   "start"
#define CMD_RESUME  "resume"
#define CMD_END     "end"
#define CMD_SYNC    "sync"
#define CMD_EFS     "efs"
#define CMD_TOGGLE  "toggle"

#define SYNC_INIT     "init"
#define SYNC_FORCED   "forced"
#define SYNC_SUCCESS  "success"
#define SYNC_FAILURE  "failure"

#define EFS_STATUS  "status"
#define EFS_PERIOD  "period"

#define TOGGLE_REQUESTED "requested"
#define TOGGLE_SUCCEEDED "succeeded"
#define TOGGLE_FAILED    "failed"

//--------------------------------------------------------------------------------------------------
/**
 * Compare command string with expected value.
 *
 * @return true if strings match, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool IsCommand
(
    const char* cmd,        ///< [IN] Command to check
    const char* expected    ///< [IN] Expected command
)
{
    return (cmd != NULL) && (strncmp(cmd, expected, strlen(expected)) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Print help menu to stdout and exit.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelpMenu
(
    void
)
{
    puts(
        "NAME:\n"
        "    tafMRCIntTest - MRC Service Integration Test\n"
        "\n"
        "SYNOPSIS:\n"
        "    app runProc tafMRCIntTest --exe=client -- <command> [options]\n"
        "\n"
        "DESCRIPTION:\n"
        "    This application provides a command-line interface to test the MRC service.\n"
        "\n"
        "COMMANDS:\n"
        "    start\n"
        "        Sends an OTA (Over-The-Air) start message to the MRC service.\n"
        "\n"
        "    resume\n"
        "        Sends an OTA resume message to the MRC service.\n"
        "\n"
        "    end <status>\n"
        "        Sends an OTA end message with a specified status.\n"
        "        <status>: 'success' or 'failure'\n"
        "\n"
        "    sync <type>\n"
        "        Sends an OTA synchronization message.\n"
        "        <type>: 'init', 'forced', 'success', or 'failure'\n"
        "\n"
        "    efs <sub-command> [value]\n"
        "        Manages EFS (Embedded File System) operations.\n"
        "        status\n"
        "            Retrieves the status of the EFS partition, including PE (Program/Erase) "
        "counts and bad blocks.\n"
        "        period <seconds>\n"
        "            Sets the time interval for the EFS backup period.\n"
        "            <seconds>: Time in seconds for the backup period\n"
        "\n"
        "    toggle <status>\n"
        "        Sets the GPIO toggle status.\n"
        "        <status>: 'requested', 'succeeded', or 'failed'\n"
    );

    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the required number of arguments are provided.
 */
//--------------------------------------------------------------------------------------------------
static void CheckArgs
(
    uint8_t argNum ///< [IN] The number of arguments required.
)
{
    if (le_arg_NumArgs() < argNum)
    {
        PrintHelpMenu();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get command line argument safely.
 *
 * @return Argument string or NULL if not available.
 */
//--------------------------------------------------------------------------------------------------
static const char* GetArg
(
    size_t index ///< [IN] Argument index
)
{
    const char* arg = le_arg_GetArg(index);
    if (arg == NULL)
    {
        PrintHelpMenu();
    }
    return arg;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the 'start' command.
 */
//--------------------------------------------------------------------------------------------------
static void HandleStartCommand
(
    void
)
{
    tafMRCIntTest_Start();
    LE_TEST_INFO("tafMRCIntTest_Start called");
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the 'resume' command.
 */
//--------------------------------------------------------------------------------------------------
static void HandleResumeCommand
(
    void
)
{
    tafMRCIntTest_Resume();
    LE_TEST_INFO("tafMRCIntTest_Resume called");
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the 'end' command.
 */
//--------------------------------------------------------------------------------------------------
static void HandleEndCommand
(
    void
)
{
    CheckArgs(2);
    const char* status = GetArg(1);

    if (IsCommand(status, SYNC_SUCCESS))
    {
        tafMRCIntTest_End(TAFMRCINTTEST_OTA_OP_STATUS_SUCCESS);
        LE_TEST_INFO("tafMRCIntTest_End(SUCCESS) called");
    }
    else if (IsCommand(status, SYNC_FAILURE))
    {
        tafMRCIntTest_End(TAFMRCINTTEST_OTA_OP_STATUS_FAILURE);
        LE_TEST_INFO("tafMRCIntTest_End(FAILURE) called");
    }
    else
    {
        PrintHelpMenu();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle sync status messages.
 */
//--------------------------------------------------------------------------------------------------
static void HandleSyncStatus
(
    const char* status ///< [IN] Sync status type
)
{
    if (IsCommand(status, SYNC_INIT))
    {
        tafMRCIntTest_Sync(TAFMRCINTTEST_SYNC_STATUS_INIT);
        LE_TEST_INFO("tafMRCIntTest_Sync(INIT) called");
    }
    else if (IsCommand(status, SYNC_SUCCESS))
    {
        tafMRCIntTest_Sync(TAFMRCINTTEST_SYNC_STATUS_SUCCESS);
        LE_TEST_INFO("tafMRCIntTest_Sync(SUCCESS) called");
    }
    else if (IsCommand(status, SYNC_FAILURE))
    {
        tafMRCIntTest_Sync(TAFMRCINTTEST_SYNC_STATUS_FAILURE);
        LE_TEST_INFO("tafMRCIntTest_Sync(FAILURE) called");
    }
    else
    {
        PrintHelpMenu();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the 'sync' command.
 */
//--------------------------------------------------------------------------------------------------
static void HandleSyncCommand
(
    void
)
{
    CheckArgs(2);
    const char* status = GetArg(1);

    if (IsCommand(status, SYNC_FORCED))
    {
        tafMRCIntTest_Absync();
        LE_TEST_INFO("tafMRCIntTest_Absync called");
    }
    else
    {
        HandleSyncStatus(status);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle EFS status query.
 */
//--------------------------------------------------------------------------------------------------
static void HandleEfsStatus
(
    void
)
{
    uint32_t max = 0, min = 0, avg = 0, sd = 0, badblocks = 0;

    tafMRCIntTest_GetEfsMetrics(&max, &min, &avg, &sd, &badblocks);

    LE_INFO("PE Max: %d", max);
    LE_INFO("PE Min: %d", min);
    LE_INFO("PE Average: %d", avg);
    LE_INFO("PE Standard Deviation: %d", sd);
    LE_INFO("Bad blocks: %d", badblocks);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle EFS backup period setting.
 */
//--------------------------------------------------------------------------------------------------
static void HandleEfsPeriod
(
    void
)
{
    CheckArgs(3);
    const char* period = GetArg(2);

    long time = strtol(period, NULL, 10);
    tafMRCIntTest_SetEfsBackupPeriod((uint32_t)time);
    LE_TEST_INFO("tafMRCIntTest_SetEfsBackupPeriod(%ld) called", time);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the 'efs' command.
 */
//--------------------------------------------------------------------------------------------------
static void HandleEfsCommand
(
    void
)
{
    CheckArgs(2);
    const char* option = GetArg(1);

    if (IsCommand(option, EFS_STATUS))
    {
        HandleEfsStatus();
    }
    else if (IsCommand(option, EFS_PERIOD))
    {
        HandleEfsPeriod();
    }
    else
    {
        PrintHelpMenu();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the 'toggle' command.
 */
//--------------------------------------------------------------------------------------------------
static void HandleToggleCommand
(
    void
)
{
    CheckArgs(2);
    const char* status = GetArg(1);

    if (IsCommand(status, TOGGLE_REQUESTED))
    {
        tafMRCIntTest_SetGpioToggleStatus(TAFMRCINTTEST_TOGGLE_STATUS_REQUESTED);
        LE_TEST_INFO("tafMRCIntTest_SetGpioToggleStatus(REQUESTED) called");
    }
    else if (IsCommand(status, TOGGLE_SUCCEEDED))
    {
        tafMRCIntTest_SetGpioToggleStatus(TAFMRCINTTEST_TOGGLE_STATUS_SUCCEEDED);
        LE_TEST_INFO("tafMRCIntTest_SetGpioToggleStatus(SUCCEEDED) called");
    }
    else if (IsCommand(status, TOGGLE_FAILED))
    {
        tafMRCIntTest_SetGpioToggleStatus(TAFMRCINTTEST_TOGGLE_STATUS_FAILED);
        LE_TEST_INFO("tafMRCIntTest_SetGpioToggleStatus(FAILED) called");
    }
    else
    {
        PrintHelpMenu();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Dispatch command to appropriate handler.
 */
//--------------------------------------------------------------------------------------------------
static void DispatchCommand
(
    const char* cmd ///< [IN] Command to dispatch
)
{
    if (IsCommand(cmd, CMD_START))
    {
        HandleStartCommand();
    }
    else if (IsCommand(cmd, CMD_RESUME))
    {
        HandleResumeCommand();
    }
    else if (IsCommand(cmd, CMD_END))
    {
        HandleEndCommand();
    }
    else if (IsCommand(cmd, CMD_SYNC))
    {
        HandleSyncCommand();
    }
    else if (IsCommand(cmd, CMD_EFS))
    {
        HandleEfsCommand();
    }
    else if (IsCommand(cmd, CMD_TOGGLE))
    {
        HandleToggleCommand();
    }
    else
    {
        PrintHelpMenu();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    CheckArgs(1);

    const char* cmd = le_arg_GetArg(0);
    if (cmd == NULL)
    {
        PrintHelpMenu();
        exit(EXIT_FAILURE);
    }

    LE_TEST_INFO("======== MRC Integration Test Client ========");

    DispatchCommand(cmd);

    exit(EXIT_SUCCESS);
}
