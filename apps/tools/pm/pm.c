/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
taf_pm_ConsolidatedAckInfoHandlerRef_t ackInfoHandlerRef = NULL;
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
                pm - Used to send commands to power manager\n\
            \n\
            SYNOPSIS:\n\
                pm --help\n\
                pm suspend\n\
                pm resume\n\
                pm shutdown\n\
                pm suspend <VM name>\n\
                pm resume <VM name>\n\
                pm getAvailableMachines\n\
                pm state\n\
            \n\
            DESCRIPTION:\n\
                pm --help\n\
                  - Print this help message and exit\n\
            \n\
                pm suspend\n\
                  - Initiates suspend of the device\n\
            \n\
                pm resume\n\
                  - Initiates resume of the device.\n\
            \n\
                pm shutdown\n\
                  - Initiates shutdown of the device.\n\
            \n\
                pm suspend <VM name>\n\
                  - Initiates suspend of the VM\n\
            \n\
                pm resume <VM name>\n\
                  - Initiates resume of the VM.\n\
            \n\
                pm getAvailableMachines\n\
                  - To get the list of machines available.\n\
            \n\
                pm state\n\
                  - To know the device state.\n\
            ");

    exit(EXIT_SUCCESS);
}

/**
 * To convert TafState to string
 */
const char* tafStateToString(taf_pm_State_t tafState)
{
    const char *state;
    switch(tafState) {
        case TAF_PM_STATE_RESUME:
            state = "Resume";
            break;
        case TAF_PM_STATE_SUSPEND:
            state = "Suspend";
            break;
        case TAF_PM_STATE_SHUTDOWN:
            state = "Shutdown";
            break;
        default :
            state = "Unknown";
            break;
    }
    return state;
}

void ConsolidatedAckInfoHandler
(
    taf_pm_ConsolidatedAckInfoRef_t infoRef,
    bool isAllAcked,
    taf_pm_State_t state,
    void* contextPtr
)
{
    LE_INFO("ConsolidatedAckInfoHandler triggered with state: %d, isAllAcked: %d",
        (int)state, (int)isAllAcked);
    le_result_t result;
    uint8_t i;

    if(isAllAcked)
    {
        printf("Slave applications successfully acknowledged the %s transition\n",
            tafStateToString(state));
        exit(EXIT_SUCCESS);
    }
    else
    {
        taf_pm_ClientInfo_t nackClients[TAF_PM_MAX_CLIENT_NUMBER] = {};
        size_t nackClientCount = 0;
        taf_pm_ClientInfo_t unresClients[TAF_PM_MAX_CLIENT_NUMBER] = {};
        size_t unresClientCount = 0;

        result = taf_pm_GetUnrespClientInfo(infoRef, unresClients, &unresClientCount);
        if(result != LE_OK)
        {
            printf("Failed to fetch unresponded clients with result %d.\n", result);
            return;
        }
        if(unresClientCount >= TAF_PM_MAX_CLIENT_NUMBER)
        {
            printf("Fetched more unresponded clients than expected with %d.\n",
                (int)unresClientCount);
            return;
        }
        if(unresClientCount > 0)
        {
            printf("Timeout occurred while waiting for acknowledgements from slave applications\n");
            printf("Number of unresponsive clients: %d\n", (int)unresClientCount);
        }

        for(i = 0; i < unresClientCount; i++)
        {
            printf("client name: %s, machine name: %s\n", unresClients[i].clientName,
                unresClients[i].machineName);
        }
        result = taf_pm_GetNackClientInfo(infoRef, nackClients, &nackClientCount);
        if(result != LE_OK)
        {
            printf("Failed to fetch nacked clients with result %d.\n", result);
            return;
        }
        if(nackClientCount >= TAF_PM_MAX_CLIENT_NUMBER)
        {
            printf("Fetched more nack clients than expected with %d.\n", (int)nackClientCount);
            return;
        }
        if(nackClientCount > 0)
        {
            printf("Number of clients responded with nack: %d\n", (int)nackClientCount);
        }

        for(i = 0; i < nackClientCount; i++)
        {
            printf("client name: %s, machine name: %s\n", nackClients[i].clientName,
                nackClients[i].machineName);
        }

    }

    exit(EXIT_SUCCESS);
}




//--------------------------------------------------------------------------------------------------
/**
 * Sets the command handler to call depending on which command was specified on the command-line.
 */
//--------------------------------------------------------------------------------------------------
static void SetCommandHandler
(
    const char* argPtr                  ///< [IN] Command-line argument.
)
{
    ackInfoHandlerRef =taf_pm_AddConsolidatedAckInfoHandler(
        (taf_pm_ConsolidatedAckInfoHandlerFunc_t)ConsolidatedAckInfoHandler, NULL);
    if (ackInfoHandlerRef)
        LE_INFO("Register Extended state change handler is successfull");

    LE_INFO("SetCommandHandler %s", argPtr);
    le_result_t res;
    if (strcmp(argPtr, "suspend") == 0)
    {
        if(le_arg_NumArgs() == 1) {
            res = taf_pm_SetAllVMPowerState(TAF_PM_STATE_SUSPEND);
            if (res == LE_OK) {
                LE_INFO("Successfully requested suspend");
            } else {
                fprintf(stderr, "Failed to request suspend\n");
                LE_ERROR("Failed to request suspend");
                exit(EXIT_FAILURE);
            }
        } else if(le_arg_NumArgs() == 2) {
            LE_DEBUG("setVMPowerState suspend to %s", le_arg_GetArg(1));
            taf_pm_VMListRef_t vmListRef = taf_pm_GetMachineList( );
            if(!vmListRef) {
                LE_ERROR("List is null");
                fprintf(stderr, "Machine list not available\n");
                exit(EXIT_FAILURE);
            }
            char name[32] = {0};
            const char* vmName = le_arg_GetArg(1);
            res = taf_pm_GetFirstMachineName(vmListRef, name, 32);
            while(res == LE_OK){
                if(vmName && strcmp(vmName, name) == 0){
                    res = taf_pm_SetVMPowerState(TAF_PM_STATE_SUSPEND, vmName);
                    if(res != LE_OK) {
                        LE_ERROR("Failed to suspend VM");
                        fprintf(stderr, "Failed to request suspend for %s VM\n", vmName);
                    }
                    LE_INFO("Successfully requested suspend for %s VM", vmName);
                    taf_pm_DeleteMachineList(vmListRef);
                }
                res = taf_pm_GetNextMachineName(vmListRef, name, 32);
            }
            LE_ERROR("Enter proper VM name\n");
            fprintf(stderr, "Enter proper VM name\n");
            fprintf(stderr, "Try \"pm getAvailableMachines\" to know the machines available\n");
        }
    }
    else if (strcmp(argPtr, "resume") == 0)
    {
        if(le_arg_NumArgs() == 1) {
            res = taf_pm_SetAllVMPowerState(TAF_PM_STATE_RESUME);
            if (res == LE_OK) {
                LE_INFO("Successfully requested resume");
                exit(EXIT_SUCCESS);
            } else {
                fprintf(stderr, "Failed to request resume\n");
                LE_ERROR("Failed to request resume");
                exit(EXIT_FAILURE);
            }
        } else if(le_arg_NumArgs() == 2) {
            LE_DEBUG("setVMPowerState resume to %s", le_arg_GetArg(1));
            taf_pm_VMListRef_t vmListRef = taf_pm_GetMachineList( );
            if(!vmListRef) {
                LE_ERROR("List is null");
                fprintf(stderr, "Machine list not available\n");
                exit(EXIT_FAILURE);
            }
            char name[32] = {0};
            const char* vmName = le_arg_GetArg(1);
            res = taf_pm_GetFirstMachineName(vmListRef, name, 32);
            while(res == LE_OK){
                if(vmName && strcmp(vmName, name) == 0){
                    res = taf_pm_SetVMPowerState(TAF_PM_STATE_RESUME, vmName);
                    if(res != LE_OK) {
                        LE_ERROR("Failed to resume VM");
                        fprintf(stderr, "Failed to request resume for %s VM\n", vmName);
                    }
                    LE_INFO("Successfully requested resume for %s VM", vmName);
                    taf_pm_DeleteMachineList(vmListRef);
                }
                res = taf_pm_GetNextMachineName(vmListRef, name, 32);
            }
            LE_ERROR("Enter proper VM name");
            fprintf(stderr, "Enter proper VM name\n");
            fprintf(stderr, "Try \"pm getAvailableMachines\" to know the machines available\n");
        }
    }
    else if (strcmp(argPtr, "shutdown") == 0)
    {
        if(le_arg_NumArgs() == 1) {
            res = taf_pm_SetAllVMPowerState(TAF_PM_STATE_SHUTDOWN);
            if (res == LE_OK) {
                LE_INFO("Successfully requested shutdown");
            } else {
                fprintf(stderr, "Failed to request shutdown\n");
                LE_ERROR("Failed to request shutdown");
                exit(EXIT_FAILURE);
            }
        } else if(le_arg_NumArgs() == 2) {
            LE_DEBUG("setVMPowerState shutdown to %s", le_arg_GetArg(1));
            taf_pm_VMListRef_t vmListRef = taf_pm_GetMachineList( );
            if(!vmListRef) {
                LE_ERROR("List is null");
                fprintf(stderr, "Machine list not available\n");
                exit(EXIT_FAILURE);
            }
            char name[32] = {0};
            const char* vmName = le_arg_GetArg(1);
            res = taf_pm_GetFirstMachineName(vmListRef, name, 32);
            while(res == LE_OK){
                if(vmName && strcmp(vmName, name) == 0){
                    res = taf_pm_SetVMPowerState(TAF_PM_STATE_SHUTDOWN, vmName);
                    if(res != LE_OK) {
                        LE_ERROR("Failed to shutdown VM");
                        fprintf(stderr, "Failed to request shutdown for %s VM\n", vmName);
                    }
                    LE_INFO("Successfully requested shutdown for %s VM", vmName);
                    taf_pm_DeleteMachineList(vmListRef);
                }
                res = taf_pm_GetNextMachineName(vmListRef, name, 32);
            }
            LE_ERROR("Enter proper VM name");
            fprintf(stderr, "Enter proper VM name\n");
            fprintf(stderr, "Try \"pm getAvailableMachines\" to know the machines available\n");
        }
    }
    else if (strcmp(argPtr, "getAvailableMachines") == 0)
    {
        taf_pm_VMListRef_t vmListRef = taf_pm_GetMachineList( );
        char name[32] = {0};
        if(!vmListRef) {
            LE_ERROR("List is null");
            fprintf(stderr, "Machine list not available\n");
            exit(EXIT_FAILURE);
        }
        res = taf_pm_GetFirstMachineName(vmListRef, name, 32);
        fprintf(stdout, "Available machines list :\n");
        while(res == LE_OK)
        {
            fprintf(stdout, "%s\n", name);
            LE_DEBUG("VM name %s", name);
            res = taf_pm_GetNextMachineName(vmListRef, name, 32);
        }
        res = taf_pm_DeleteMachineList(vmListRef);
        if(res == LE_OK)
            LE_DEBUG("Machine list deleted successfully");
        return;
    }
    else if (strcmp(argPtr, "state") == 0)
    {
        taf_pm_State_t state = taf_pm_GetPowerState();
        fprintf(stdout, "State is %s\n", tafStateToString(state));
        exit(EXIT_SUCCESS);
    }
    else
    {
        fprintf(stderr, "Unknown command: %s.\n", argPtr);
        fprintf(stderr, "Please try pm --help\n");
        exit(EXIT_FAILURE);
    }
}

COMPONENT_INIT
{
    LE_INFO("COMPONENT_INIT");
    // Setup command-line argument handling.
    le_arg_SetFlagCallback(PrintHelp, "h", "help");

    le_arg_AddPositionalCallback(SetCommandHandler);

    le_arg_Scan();

    LE_INFO("COMPONENT_INIT end");
}