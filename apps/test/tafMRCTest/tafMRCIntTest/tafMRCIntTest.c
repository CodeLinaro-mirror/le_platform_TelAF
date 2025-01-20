/*
 * Copyright (c) 2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
        "    app runProc tafMRCIntTest tafMRCIntTest -- sync\n"
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
        result = taf_mrc_SendOtaAbsyncMsg();
        LE_TEST_OK(result == LE_OK, "taf_mrc_SendOtaAbsyncMsg - LE_OK");
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
