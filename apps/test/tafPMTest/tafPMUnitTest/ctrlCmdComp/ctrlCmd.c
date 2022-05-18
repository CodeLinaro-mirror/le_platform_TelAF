/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

/// Name used to launch this program.
static const char* ProgramName;

static void PrintHelp
(
    void
)
{
    puts(
        "\n"
        "NAME:\n"
        "    pmTest - Used to perform power manager testcases.\n"
        "\n"
        "PREREQUISITES:\n"
        "    tafPMSvc is running. tafPMSvc can be started using app start tafPMSvc.\n"
        "    stop and start the tafPMUnitTest application before running each test case.\n"
        "\n"
        "DESCRIPTION:\n"
        "    pmTest test1\n"
        "       Testcase to registers for state change.\n"
        "       Send state change from telux_power_test_app to receive the callback in test app.\n"
        "\n"
        "    pmTest test2\n"
        "       Teasecase to registers and unregisters the state change callback\n"
        "\n"
        "    pmTest test3\n"
        "       Suspend testcase when wakelock is acquired, and observe device will not suspend when"
        " wakelock is acquired.\n"
        "       Send suspend request from NAD telux_power_test_app.\n"
        "\n"
        "    pmTest test4\n"
        "       Suspend test case when WL is released, device suspends when no wakelock is held.\n"
        "\n"
        "    pmTest test5\n"
        "       Suspend test case when app exits with wakelock acquired.\n"
        "\n"
        "    pmTest test6\n"
        "       Test getState.\n"
        "\n"
        "    pmTest test7\n"
        "       Test acquire and release multiple times WL with reference.\n"
        );

    exit(EXIT_SUCCESS);
}

static void CommandHandler
(
    const char* argPtr                  ///< [IN] Command-line argument.
)
{
    if (strcmp(argPtr, "test1") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlCmd_registerStateChangeListener();
    } else if (strcmp(argPtr, "test2") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlCmd_deregisterListenerTest();
    } else if (strcmp(argPtr, "test3") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlCmd_test3();
    } else if (strcmp(argPtr, "test4") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlCmd_test4();
    } else if (strcmp(argPtr, "test5") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlCmd_test5();
    } else if (strcmp(argPtr, "test6") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlCmd_test6();
    } else if (strcmp(argPtr, "test7") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlCmd_test7();
    } else
    {
        fprintf(stderr, "Unknown command.\n");
        fprintf(stderr, "Try '%s --help'.\n", ProgramName);
        exit(EXIT_FAILURE);
    }
}

COMPONENT_INIT
{
    // Read out the program name
    ProgramName = le_arg_GetProgramName();
    if (ProgramName == NULL)
    {
        ProgramName = "pmTest";
    }
    LE_INFO("pmTest ProgramName : %s", ProgramName);
    le_arg_SetFlagCallback(PrintHelp, "h", "help");

    // The first positional argument is the testcase that needs to be executed.
    le_arg_AddPositionalCallback(CommandHandler);

    // Scan the argument list.
    le_arg_Scan();

    exit(EXIT_SUCCESS);
}
