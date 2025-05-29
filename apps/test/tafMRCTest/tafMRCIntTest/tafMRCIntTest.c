/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Print help menu to stdout and exit.
 */
//--------------------------------------------------------------------------------------------------
void PrintHelpMenu
(
    void
)
{
    puts(
        "NAME:\n"
        "app runProc tafMRCIntTest tafMRCIntTest - MRC Service Integration Test.\n"
        "\n"
        "SYNOPSIS:\n"
        "    app runProc tafMRCIntTest tafMRCIntTest -- help\n"
        "    app runProc tafMRCIntTest tafMRCIntTest -- start\n"
        "    app runProc tafMRCIntTest tafMRCIntTest -- resume\n"
        "    app runProc tafMRCIntTest tafMRCIntTest -- end <success|failure>\n"
        "    app runProc tafMRCIntTest tafMRCIntTest -- sync\n"
        "\n"
        "DESCRIPTION:\n"
        "    app runProc tafMRCIntTest tafMRCIntTest -- help\n"
        "       Display this help and exit.\n"
        "\n"
        "    app runProc tafMRCIntTest tafMRCIntTest -- start\n"
        "       Send OTA start message.\n"
        "\n"
        "    app runProc tafMRCIntTest tafMRCIntTest -- resume\n"
        "       Send OTA resume message.\n"
        "\n"
        "    app runProc tafMRCIntTest tafMRCIntTest -- end <success|failure>\n"
        "       Send OTA end message.\n"
        "\n"
        "    app runProc tafMRCIntTest tafMRCIntTest -- sync <init/forced/success/failure>\n"
        "       Send OTA sync message.\n"
        "\n"
    );

    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function checks the number of input parameters, if it is less than argNum, then it prints
 * the help menu.
 */
//--------------------------------------------------------------------------------------------------
void CheckArgs
(
    uint8_t argNum ///< [IN] The number of arguments.
)
{
    if (le_arg_NumArgs() < argNum)
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

    le_result_t result;
    const char* cmd = le_arg_GetArg(0);
    if (cmd == NULL)
    {
        PrintHelpMenu();
        exit(EXIT_FAILURE);
    }

    LE_TEST_INFO("======== MRC OTA Test ========");

    if (strncmp(cmd, "start", strlen("start")) == 0)
    {
        result = taf_mrc_SendOtaStartMsg();
        LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaStartMsg - LE_OK");
    }
    else if (strncmp(cmd, "resume", strlen("resume")) == 0)
    {
        result = taf_mrc_SendOtaResumeMsg();
        LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaResumeMsg - LE_OK");
    }
    else if (strncmp(cmd, "sync", strlen("sync")) == 0)
    {
        CheckArgs(2);
        const char* status = le_arg_GetArg(1);
        if (status == NULL)
        {
			PrintHelpMenu();
        }
        else if (strncmp(status, "forced", strlen("forced")) == 0)
        {
            result = taf_mrc_SendOtaAbsyncMsg();
            LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaAbsyncMsg - LE_OK");
        }
        else
        {
            if (strncmp(status, "init", strlen("init")) == 0)
            {
                result = taf_mrc_SendSyncStatusMsg(TAF_MRC_SYNC_STATUS_INIT);
                LE_TEST_OK(result == LE_OK, "taf_mrc_SendSyncStatusMsg - LE_OK");
            }
            else if (strncmp(status, "success", strlen("success")) == 0)
            {
                result = taf_mrc_SendSyncStatusMsg(TAF_MRC_SYNC_STATUS_SUCCESS);
                LE_TEST_OK(result == LE_OK, "taf_mrc_SendSyncStatusMsg - LE_OK");
            }
            else if (strncmp(status, "failure", strlen("failure")) == 0)
            {
                result = taf_mrc_SendSyncStatusMsg(TAF_MRC_SYNC_STATUS_FAILURE);
                LE_TEST_OK(result == LE_OK, "taf_mrc_SendSyncStatusMsg - LE_OK");
            }
            else
            {
                PrintHelpMenu();
            }
        }
    }
    else if (strncmp(cmd, "end", strlen("end")) == 0)
    {
        CheckArgs(2);
        const char* status = le_arg_GetArg(1);
        if (status != NULL && strncmp(status, "success", strlen("success")) == 0)
        {
            result = taf_mrc_SendOtaEndMsg(TAF_MRC_OTA_OP_STATUS_SUCCESS);
            LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaEndMsg - LE_OK");
        }
        else if (status != NULL && strncmp(status, "failure", strlen("failure")) == 0)
        {
            result = taf_mrc_SendOtaEndMsg(TAF_MRC_OTA_OP_STATUS_FAILURE);
            LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaEndMsg - LE_OK");
        }
        else
        {
            PrintHelpMenu();
        }
    }
    else
    {
        PrintHelpMenu();
    }

    exit(EXIT_SUCCESS);
}
