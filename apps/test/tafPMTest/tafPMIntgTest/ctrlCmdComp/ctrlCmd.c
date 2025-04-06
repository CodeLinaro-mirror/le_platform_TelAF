/*
 *  Copyright (c) 2022-2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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
        "    Please do (logread -f |grep -i \"tafPMIntgTest\" &) to check state before this cmd.\n"
        "\n"
        "    pmTest test7\n"
        "       Test acquire and release multiple times WL with reference.\n"
        "\n"
        "    pmTest test8\n"
        "       Testcase to registers for Extend state change.\n"
        "       Send state change to receive the callback in test app.\n"
        "       This test case is supported only for 525 target.\n"
        "\n"
        "    pmTest test9\n"
        "       Teasecase to unregisters the Extend state change callback\n"
        "       This test case is supported only for 525 target.\n"
        "\n"
        "    pmTest resume\n"
        "       Resume whole device.\n"
        "\n"
#if LE_CONFIG_TARGET_SA525M
        "    pmTest getAllMachines\n"
        "       To get all the machines available.\n"
        "    Please do (logread -f |grep -i \"tafPMIntgTest\" &) to check machine names before this cmd.\n"
        "\n"
        "    pmTest resume <vm_name>\n"
        "       Resume the particular virtual machine.\n"
        "\n"
        "    pmTest suspend\n"
        "       suspend whole device.\n"
        "\n"
        "    pmTest suspend <vm_name>\n"
        "       suspend the particular virtual machine.\n"
        "\n"
        "    pmTest shutdown\n"
        "       suspend whole device.\n"
        "\n"
        "    pmTest setPowerMode <powerMode>\n"
        "       sets PowerMode to device.\n"

#endif
        );

    exit(EXIT_SUCCESS);
}

static void CommandHandler
(
    const char* argPtr                  ///< [IN] Command-line argument.
)
{
    le_result_t res = LE_OK;
    const char* arg;
    arg = le_arg_GetArg(1);
    if(le_arg_NumArgs() == 2 && arg == NULL)
    {
        arg = le_arg_GetArg(1);
        LE_ERROR("tafPMIntgTest: NULL argument received, Line %d", __LINE__);
        exit(EXIT_FAILURE);
    }
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
    } else if (strcmp(argPtr, "getAllMachines") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlCmd_getAllMachines();
    } else if (strcmp(argPtr, "test8") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlCmd_registerStateChangeExListener();
    } else if (strcmp(argPtr, "test9") == 0 && le_arg_NumArgs() == 1)
    {
        ctrlCmd_deregisterStateChangeExListener();
    }else if (strcmp(argPtr, "resume") == 0 && le_arg_NumArgs() >= 1)
    {
        if(le_arg_NumArgs() == 2 && arg != NULL) {
            res = ctrlCmd_resumeVM(arg);
        } else if(le_arg_NumArgs() == 1){
            res = ctrlCmd_resume();
        }
        if(res != LE_OK)
            LE_ERROR("Failed to resume the device");
    } else if (strcmp(argPtr, "suspend") == 0 && le_arg_NumArgs() >= 1)
    {
        if(le_arg_NumArgs() == 2 && arg != NULL) {
            res = ctrlCmd_suspendVM(arg);
        } else if(le_arg_NumArgs() == 1) {
            res = ctrlCmd_suspend();
        }
        if(res != LE_OK)
            LE_ERROR("Failed to suspend the device");
    } else if (strcmp(argPtr, "shutdown") == 0 && le_arg_NumArgs() >= 1)
    {
        if(le_arg_NumArgs() == 2 && arg != NULL) {
            res = ctrlCmd_shutdownVM(arg);
        } else if(le_arg_NumArgs() == 1) {
            res = ctrlCmd_shutdown();
        }
        if(res != LE_OK)
            LE_ERROR("Failed to shutdown the device");
    }else if (strcmp(argPtr, "setPowerMode") == 0 && le_arg_NumArgs() >= 1)
    {
        if(le_arg_NumArgs() == 2 && arg != NULL) {
            res = ctrlCmd_SetPowerMode(atoi(arg));
        } else if(le_arg_NumArgs() == 1) {
            printf("power mode is empty!");
        }
        if(res != LE_OK)
            LE_ERROR("Failed to set power mode to the system");
    } else
    {
        fprintf(stderr, "Unknown command.\n");
        fprintf(stderr, "Try '%s --help'.\n", ProgramName);
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
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
