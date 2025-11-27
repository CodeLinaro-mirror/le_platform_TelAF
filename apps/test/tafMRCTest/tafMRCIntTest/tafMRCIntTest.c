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

#define SYNC_INIT     "init"
#define SYNC_FORCED   "forced"
#define SYNC_SUCCESS  "success"
#define SYNC_FAILURE  "failure"

#define EFS_STATUS  "status"
#define EFS_PERIOD  "period"

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
        "    app runProc tafMRCIntTest -- <command> [options]\n"
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
    le_result_t result = taf_mrc_SendOtaStartMsg();
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaStartMsg - LE_OK");
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
    le_result_t result = taf_mrc_SendOtaResumeMsg();
    LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaResumeMsg - LE_OK");
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
    le_result_t result;

    if (IsCommand(status, SYNC_SUCCESS))
    {
        result = taf_mrc_SendOtaEndMsg(TAF_MRC_OTA_OP_STATUS_SUCCESS);
        LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaEndMsg - LE_OK");
    }
    else if (IsCommand(status, SYNC_FAILURE))
    {
        result = taf_mrc_SendOtaEndMsg(TAF_MRC_OTA_OP_STATUS_FAILURE);
        LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaEndMsg - LE_OK");
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
    le_result_t result;

    if (IsCommand(status, SYNC_INIT))
    {
        result = taf_mrc_SendSyncStatusMsg(TAF_MRC_SYNC_STATUS_INIT);
        LE_TEST_OK(result == LE_OK, "taf_mrc_SendSyncStatusMsg - LE_OK");
    }
    else if (IsCommand(status, SYNC_SUCCESS))
    {
        result = taf_mrc_SendSyncStatusMsg(TAF_MRC_SYNC_STATUS_SUCCESS);
        LE_TEST_OK(result == LE_OK, "taf_mrc_SendSyncStatusMsg - LE_OK");
    }
    else if (IsCommand(status, SYNC_FAILURE))
    {
        result = taf_mrc_SendSyncStatusMsg(TAF_MRC_SYNC_STATUS_FAILURE);
        LE_TEST_OK(result == LE_OK, "taf_mrc_SendSyncStatusMsg - LE_OK");
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
        le_result_t result = taf_mrc_SendOtaAbsyncMsg();
        LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaAbsyncMsg - LE_OK");
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
    taf_mrc_MetricsRef_t metrics = NULL;
    le_result_t result = taf_mrc_MeasureEfsMetrics(&metrics);
    LE_TEST_OK(result == LE_OK, "taf_mrc_MeasureEfsMetrics - LE_OK");

    uint32_t max = 0, min = 0, avg = 0, sd = 0, badblocks = 0;

    result = taf_mrc_GetEfsMaxPECount(metrics, &max);
    LE_TEST_OK(result == LE_OK, "taf_mrc_GetEfsMaxPECount - LE_OK");

    result = taf_mrc_GetEfsMinPECount(metrics, &min);
    LE_TEST_OK(result == LE_OK, "taf_mrc_GetEfsMinPECount - LE_OK");

    result = taf_mrc_GetEfsAvgPECount(metrics, &avg);
    LE_TEST_OK(result == LE_OK, "taf_mrc_GetEfsAvgPECount - LE_OK");

    result = taf_mrc_GetEfsPEStandardDeviation(metrics, &sd);
    LE_TEST_OK(result == LE_OK, "taf_mrc_GetEfsPEStandardDeviation - LE_OK");

    result = taf_mrc_GetEfsBadBlocks(metrics, &badblocks);
    LE_TEST_OK(result == LE_OK, "taf_mrc_GetEfsBadBlocks - LE_OK");

    LE_INFO("PE Max: %d", max);
    LE_INFO("PE Min: %d", min);
    LE_INFO("PE Average: %d", avg);
    LE_INFO("PE Standard Deviation: %d", sd);
    LE_INFO("Bad blocks: %d", badblocks);

    result = taf_mrc_DeleteEfsMetrics(metrics);
    LE_TEST_OK(result == LE_OK, "taf_mrc_DeleteEfsMetrics - LE_OK");
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
    le_result_t result = taf_mrc_SetEfsBackupPeriod(time);
    LE_TEST_OK(result == LE_OK, "taf_mrc_SetEfsBackupPeriod - LE_OK");
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

    LE_TEST_INFO("======== MRC Integration Test ========");

    DispatchCommand(cmd);

    exit(EXIT_SUCCESS);
}