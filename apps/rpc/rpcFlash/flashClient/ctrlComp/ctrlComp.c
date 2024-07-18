/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

ctrlFlash_Partition_t partType;
char partName[CTRLFLASH_PARTITION_NAME_BYTES];
char filePath[CTRLFLASH_FILE_PATH_BYTES];

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
                flash - Used to send flash commands\n\
            \n\
            SYNOPSIS:\n\
                flash help\n\
                flash <mtd|ubi> info <partition>\n\
                flash <mtd|ubi> read <partition> <file>\n\
                flash <mtd|ubi> write <partition> <file>\n\
            \n\
            DESCRIPTION:\n\
                flash help\n\
                  - Print this help message and exit\n\
            \n\
                flash <mtd|ubi> info <partition>\n\
                  - Show information of remote 'mtd partition' or 'ubi volume'.\n\
            \n\
                flash <local|remote> <mtd|ubi> read <partition> <file>\n\
                  - Read a remote 'mtd partition' or 'ubi volume' to file.\n\
            \n\
                flash <local|remote> <mtd|ubi> write <partition> <file>\n\
                  - Write a remote 'mtd partition' or 'ubi volume' with file.\n\
            ");

    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
* Shows MTD partition or UBI volume information.
*/
//--------------------------------------------------------------------------------------------------
static void FlashInfo
(
    const char* argPtr
)
{
    le_utf8_Copy(partName, argPtr, CTRLFLASH_PARTITION_NAME_BYTES, NULL);
    ctrlFlash_Info(partType, partName);
}

//--------------------------------------------------------------------------------------------------
/**
* Erase MTD partition.
*/
//--------------------------------------------------------------------------------------------------
static void FlashErase
(
    const char* argPtr
)
{
    le_utf8_Copy(partName, argPtr, CTRLFLASH_PARTITION_NAME_BYTES, NULL);
    ctrlFlash_Erase(partType, partName);
}

//--------------------------------------------------------------------------------------------------
/**
* Read partition data.
*/
//--------------------------------------------------------------------------------------------------
static void FlashReadPartition
(
    const char* argPtr
)
{
    le_utf8_Copy(filePath, argPtr, CTRLFLASH_FILE_PATH_BYTES, NULL);
    ctrlFlash_Read(partType, partName, filePath);
}

//--------------------------------------------------------------------------------------------------
/**
* Sets read file argument on the command-line.
*/
//--------------------------------------------------------------------------------------------------
static void FlashRead
(
    const char* argPtr
)
{
    le_utf8_Copy(partName, argPtr, CTRLFLASH_PARTITION_NAME_BYTES, NULL);
    le_arg_AddPositionalCallback(FlashReadPartition);
}

//--------------------------------------------------------------------------------------------------
/**
* Write data to partition.
*/
//--------------------------------------------------------------------------------------------------
static void FlashWriteToPartition
(
    const char* argPtr
)
{
    le_utf8_Copy(filePath, argPtr, CTRLFLASH_FILE_PATH_BYTES, NULL);
    ctrlFlash_Write(partType, partName, filePath);
}

//--------------------------------------------------------------------------------------------------
/**
* Sets write file argument on the command-line.
*/
//--------------------------------------------------------------------------------------------------
static void FlashWrite
(
    const char* argPtr
)
{
    le_utf8_Copy(partName, argPtr, CTRLFLASH_PARTITION_NAME_BYTES, NULL);
    le_arg_AddPositionalCallback(FlashWriteToPartition);
}

//--------------------------------------------------------------------------------------------------
/**
* Sets flash operation argument on the command-line.
*/
//--------------------------------------------------------------------------------------------------
static void FlashOperation
(
    const char* argPtr
)
{
    if (strcmp(argPtr, "info") == 0)
    {
        le_arg_AddPositionalCallback(FlashInfo);
    }
    else if (strcmp(argPtr, "erase") == 0)
    {
        le_arg_AddPositionalCallback(FlashErase);
    }
    else if (strcmp(argPtr, "read") == 0)
    {
        le_arg_AddPositionalCallback(FlashRead);
    }
    else if (strcmp(argPtr, "write") == 0)
    {
        le_arg_AddPositionalCallback(FlashWrite);
    }
    else
    {
        fprintf(stderr, "Unknown command. Try 'flash --help'.\n");
        exit(EXIT_FAILURE);
    }
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
    else if (strcmp(command, "mtd") == 0)
    {
        partType = CTRLFLASH_MTD;
        le_arg_AddPositionalCallback(FlashOperation);
    }
    else if (strcmp(command, "ubi") == 0)
    {
        partType = CTRLFLASH_UBI;
        le_arg_AddPositionalCallback(FlashOperation);
    }
    else
    {
        fprintf(stderr, "Unknown command. Try 'flash --help'.\n");
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