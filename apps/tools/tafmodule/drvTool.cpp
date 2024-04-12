/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/** drvTool.c
 *
 * This is the telaf driver load tool.
 *
 * The general format of tafmodule commands is:
 *
 *
 *
 * The following are examples of supported commands:
 *
 *    --------------------------
 *    | cmd | commandParameter |
 *    --------------------------
 * e.g. tafmodule install /data/gpioDrv.so
 *      tafmodule remove gpioDrv
 *      tafmodule list
 *
 */

#include "legato.h"
#include "tafHalIF.hpp"
#include "devManager.hpp"
#include "limit.h"
#include <ctype.h>

//--------------------------------------------------------------------------------------------------
/**
 * Command character byte.
 **/
//--------------------------------------------------------------------------------------------------
static char Command;

//--------------------------------------------------------------------------------------------------
/**
 * Pointer to the "command parameters" string.
 *
 **/
//--------------------------------------------------------------------------------------------------
static const char* drvFullpathPtr = NULL;
static const char* drvNamePtr = NULL;
static const char* drvVerPtr = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Error response received from the deviceManager.
 **/
//--------------------------------------------------------------------------------------------------
static int ErrorFromMsg = EXIT_SUCCESS;

//--------------------------------------------------------------------------------------------------
/**
 * Prints help to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelpAndExit
(
    void
)
{
    puts(
        "NAME:\n"
        "    tafmodule - Install TelAF HAL modules.\n"
        "\n"
        "SYNOPSIS:\n"
        "    tafmodule install [module]\n"
        "    tafmodule remove [module name]\n"
        "    tafmodule list \n"
        "\n"
        "DESCRIPTION:\n"
        "    tafmodule install     install the telaf module to the telaf framework\n"
        "\n"
        "    tafmodule remove      remove the telaf module from telaf framework\n"
        "\n"
        "    tafmodule list        list all installed modules\n"
        );
        exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handles a message received from the deviceManager
 **/
//--------------------------------------------------------------------------------------------------
static void MsgReceiveHandler
(
    le_msg_MessageRef_t msgRef,
    void* contextPtr // not used.
)
{
    const char* responsePtr;

    responsePtr = (const char* )le_msg_GetPayloadPtr(msgRef);

    // Print out whatever deviceManager sent us.
    printf("%s\n", responsePtr);

    // If the first character of the response is a '*', then there has been an error.
    if (responsePtr[0] == '*')
    {
        LE_ERROR("Error from message");
        le_utf8_ParseInt(&ErrorFromMsg, &responsePtr[2]);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handles the close of the IPC session.
 **/
//--------------------------------------------------------------------------------------------------
static void SessionCloseHandler
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    exit(ErrorFromMsg);
}

//--------------------------------------------------------------------------------------------------
/**
 * Opens an IPC session to device manager.
 *
 * @return  A reference to the IPC message session.
 */
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t ConnectToDeviceManager
(
    void
)
{
    le_msg_ProtocolRef_t protocolRef;
    le_msg_SessionRef_t sessionRef;

    protocolRef = le_msg_GetProtocolRef(DEV_MANAGER_PROTOCOL_ID, DEV_MANAGER_MAX_CMD_PACKET_BYTES);
    sessionRef = le_msg_CreateSession(protocolRef, DEV_MANAGER_TOOL_NAME);

    le_msg_SetSessionRecvHandler(sessionRef, MsgReceiveHandler, NULL);
    le_msg_SetSessionCloseHandler(sessionRef, SessionCloseHandler, NULL);

    le_result_t result = le_msg_TryOpenSessionSync(sessionRef);
    if (result != LE_OK)
    {
        printf("***ERROR: Can't communicate with the device manager.\n");

        switch (result)
        {
            case LE_UNAVAILABLE:
                printf("Service not offered by device manager.\n"
                       "Check if the device manager is running with 'ps'\n");
                break;

            case LE_NOT_PERMITTED:
                printf("Missing binding to device manager\n"
                       "Please check binding info\n");
                break;

            case LE_COMM_ERROR:
                printf("Service Directory is unreachable.\n"
                       "Perhaps the Service Directory is not running?\n");
                break;

            default:
                printf("Unexpected result code %d (%s)\n", result, LE_RESULT_TXT(result));
                break;
        }
        exit(EXIT_FAILURE);
    }

    return sessionRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Print error messages and exits.
 */
//--------------------------------------------------------------------------------------------------
__attribute__ ((__noreturn__))
static void ExitWithErrorMsg
(
    const char* errorMsg
)
{
    printf("tafmodule: %s\n", errorMsg);
    printf("Try 'tafmodule --help' for more information.\n");
    exit(EXIT_FAILURE);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called by le_arg_Scan() when a tafDrive argument is seen on the command
 * line.
 **/
//--------------------------------------------------------------------------------------------------
static void DevInstallArgHandler
(
    const char* drvPtr
)
{
    drvFullpathPtr = drvPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called by le_arg_Scan() when a tafDrive argument is seen on the command
 * line.
 **/
//--------------------------------------------------------------------------------------------------
static void DevRemoveArgVerHandler
(
    const char* drvVer
)
{
    drvVerPtr = drvVer;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called by le_arg_Scan() when a tafDrive argument is seen on the command
 * line.
 **/
//--------------------------------------------------------------------------------------------------
static void DevRemoveArgHandler
(
    const char* drvName
)
{
    // Check that string is one of the level strings.
    drvNamePtr = drvName;

    // without specfiying version, use '*'
    le_arg_AllowLessPositionalArgsThanCallbacks();
    le_arg_AddPositionalCallback(DevRemoveArgVerHandler);
}

//--------------------------------------------------------------------------------------------------
/**
 * Appends text (driver information) to the command message.
 **/
//--------------------------------------------------------------------------------------------------
static void AppendToCommand
(
    le_msg_MessageRef_t msgRef,
    const char* textPtr
)
{
    if((msgRef == nullptr) || (textPtr == nullptr))
    {
        ExitWithErrorMsg("Invalid argument(s) received");
    }

    char* payloadPtr = (char*)le_msg_GetPayloadPtr(msgRef);
    size_t payloadSize = le_msg_GetMaxPayloadSize(msgRef);
    le_result_t res = le_utf8_Append(payloadPtr,
                                        textPtr,
                                        payloadSize,
                                        NULL);

    if (res ==  LE_OVERFLOW)
    {
        ExitWithErrorMsg("Overflow (Command string is too long)");
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called by le_arg_Scan() when it sees the first positional argument while
 * scanning the command line.  This should be the command name.
 **/
//--------------------------------------------------------------------------------------------------
static void CommandArgHandler
(
    const char* commandArg
)
{
    if (strcmp(commandArg, "help") == 0)
    {
        PrintHelpAndExit();
    }

    else if (strcmp(commandArg, "install") == 0)
    {
        Command = DEV_MANAGER_TOOL_INSTALL_CMD;

        // Expect full path
        le_arg_AddPositionalCallback(DevInstallArgHandler);
    }
    else if (strcmp(commandArg, "remove") == 0)
    {
        Command = DEV_MANAGER_TOOL_REMOVE_CMD;

        // Expect a trace keyword next.
        le_arg_AddPositionalCallback(DevRemoveArgHandler);
    }
    else if (strcmp(commandArg, "list") == 0)
    {
        Command = DEV_MANAGER_TOOL_LIST_DRIVERS;

        // This command has no parameters and no destination.
    }
    else
    {
        char errorMsg[100];
        snprintf(errorMsg, sizeof(errorMsg), "Invalid tafmodule command arg (%s)", commandArg);
        ExitWithErrorMsg(errorMsg);
    }
}

COMPONENT_INIT
{
    le_arg_AddPositionalCallback(CommandArgHandler);
    le_arg_SetFlagCallback(PrintHelpAndExit, "h", "help");

    le_arg_Scan();

    // Connect to deviceManager and allocate a message buffer to hold the command.
    le_msg_SessionRef_t sessionRef = ConnectToDeviceManager();
    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(sessionRef);
    char* payloadPtr = (char *)le_msg_GetPayloadPtr(msgRef);

    // Construct the message.
    payloadPtr[0] = Command;
    payloadPtr[1] = '\0';
    switch (Command)
    {
        case DEV_MANAGER_TOOL_INSTALL_CMD:

            AppendToCommand(msgRef, drvFullpathPtr);

            break;
        case DEV_MANAGER_TOOL_REMOVE_CMD:

            AppendToCommand(msgRef, drvNamePtr);
            AppendToCommand(msgRef, ":");
            if(drvVerPtr == NULL)
            {
                drvVerPtr = "*";
            }
            AppendToCommand(msgRef, drvVerPtr);
            break;

        case DEV_MANAGER_TOOL_LIST_DRIVERS:

            AppendToCommand(msgRef, "");
            break;

        default:
            break;
    }

    // Send the command and wait for messages from deviceManager.  When deviceManager has
    // finished executing the command, it will close the IPC session, resulting in a call
    // to SessionCloseHandler().
    le_msg_Send(msgRef);
}
