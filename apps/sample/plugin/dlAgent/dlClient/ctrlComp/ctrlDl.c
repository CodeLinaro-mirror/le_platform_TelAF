/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Print the help message to stdout
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    puts("\
            NAME:\n\
                dlCtrl - Used to send download commands\n\
            \n\
            SYNOPSIS:\n\
                dlCtrl help\n\
                dlCtrl config <file>\n\
                dlCtrl start\n\
                dlCtrl pause\n\
                dlCtrl resume\n\
                dlCtrl cancel\n\
            \n\
            DESCRIPTION:\n\
                dlCtrl help\n\
                  - Print this help message and exit\n\
            \n\
                dlCtrl start\n\
                  - Starts download process.\n\
            \n\
                dlCtrl pause\n\
                  - Pauses download process.\n\
            \n\
                dlCtrl resume\n\
                  - Resumes download process.\n\
            \n\
                dlCtrl cancel\n\
                  - Cancels download process.\n\
            ");

    exit(EXIT_SUCCESS);
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
    else if (strcmp(command, "start") == 0)
    {
        ctrlDl_StartDownload();
    }
    else if (strcmp(command, "pause") == 0)
    {
        ctrlDl_PauseDownload();
    }
    else if (strcmp(command, "resume") == 0)
    {
        ctrlDl_ResumeDownload();
    }
    else if (strcmp(command, "cancel") == 0)
    {
        ctrlDl_CancelDownload();
    }
    else
    {
        fprintf(stderr, "Unknown command. Try 'dlCtrl --help'.\n");
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