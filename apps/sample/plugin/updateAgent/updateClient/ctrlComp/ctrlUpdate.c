/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Print the help message to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    puts("\
            NAME:\n\
                updateCtrl - Used to send update commands\n\
            \n\
            SYNOPSIS:\n\
                updateCtrl help\n\
                updateCtrl install <file>\n\
            \n\
            DESCRIPTION:\n\
                updateCtrl help\n\
                  - Print this help message and exit\n\
            \n\
                updateCtrl install\n\
                  - Starts update process.\n\
            ");

    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
* Sets the update package specified on the command-line.
*/
//--------------------------------------------------------------------------------------------------
static void SetFile
(
    const char* argPtr
)
{
    ctrlUpdate_StartInstall(argPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called by le_arg_Scan() when it sees the first positional argument while
 * scanning the command line.  This should be the command name.
 **/
//--------------------------------------------------------------------------------------------------
static void CommandHandler
(
    const char* command
)
{
    if (strcmp(command, "help") == 0)
    {
        PrintHelp();
    }
    else if (strcmp(command, "install") == 0)
    {
        le_arg_AddPositionalCallback(SetFile);
    }
    else
    {
        fprintf(stderr, "Unknown command. Try 'updateCtrl --help'.\n");
        exit(EXIT_FAILURE);
    }
}

COMPONENT_INIT
{
    // Setup command-line argument handling.
    le_arg_SetFlagCallback(PrintHelp, "h", "help");

    le_arg_AddPositionalCallback(CommandHandler);

    le_arg_Scan();

    exit(EXIT_SUCCESS);
}