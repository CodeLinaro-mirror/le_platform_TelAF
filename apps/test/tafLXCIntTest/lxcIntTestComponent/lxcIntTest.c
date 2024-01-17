/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/*
 * @file       lxcIntTest.cpp
 * @brief      Integration test functions for LXC container
 */

#include "legato.h"
#include "interfaces.h"

#define MAX_SYSTEM_CMD_LENGTH 200

#define DEFAULT_PERMS (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)

static const char *lxcContainerNameStr = "telaflxc";
static const char *lxcContainerPathStr = "/tmp/container";
#define TELAF_LXC_CONTAINER_PATH "/tmp/container/telaflxc"
static const char *lxcConfFileStr = "/etc/lxc/lxc_telaf.conf";
static const char *lxcHostIPStr = "192.168.2.200";
static const char *lxcLogPathStr = "/tmp/container_log";
#define TELAF_LXC_RW_DATA    "/data/lxc_rw/data"
#define TELAF_LXC_RW_APP     "/data/lxc_rw/app"
#define TELAF_LXC_RW_PERSIST "/data/lxc_rw/persist"
#define TELAF_LXC_RW_TMP     "/data/lxc_rw/tmp"

static void PrintUsage(void)
{
    puts("\n"
         "app runProc tafLXCIntTest lxcTest -- create [lxc.conf]\n"
         "app runProc tafLXCIntTest lxcTest -- destroy\n"
         "app runProc tafLXCIntTest lxcTest -- start \n"
         "app runProc tafLXCIntTest lxcTest -- stop \n"
         "app runProc tafLXCIntTest lxcTest -- status \n"
         "\n");
}

/*
 * Helper function to clean and create directories
 */
static le_result_t lxcCreateDir(const char *dirPathStr)
{
    le_result_t result = LE_FAULT;
    if (le_dir_IsDir(dirPathStr))
    {
        LE_TEST_INFO("%s exists", dirPathStr);
        le_dir_RemoveRecursive(dirPathStr);
        LE_TEST_INFO("%s removed", dirPathStr);
    }

    result = le_dir_Make(dirPathStr, DEFAULT_PERMS);
    if (LE_OK != result)
    {
        LE_TEST_INFO("%s creation failed", dirPathStr);
        return result;
    }
    LE_TEST_INFO("%s created", dirPathStr);
    return result;
}

/*
 * Create telaf container. It supports user provided lxc configuration file.
 * -- create /data/lxc/mylxc.conf
 * If second parameter is not provided, the default conf file will be used
 */
static int lxcTestCreateContainer()
{
    le_result_t result = LE_FAULT;
    char systemCmd[MAX_SYSTEM_CMD_LENGTH] = {0};

    result = lxcCreateDir(lxcContainerPathStr);
    LE_TEST_ASSERT((result == LE_OK), "Create Container Path - LE_OK");

    result = lxcCreateDir(lxcLogPathStr);
    LE_TEST_ASSERT((result == LE_OK), "Create Container Log Folder - LE_OK");

    result = lxcCreateDir(TELAF_LXC_RW_DATA);
    LE_TEST_ASSERT((result == LE_OK), "Create PVM mount point data in /data/lxc_rw");

    result = lxcCreateDir(TELAF_LXC_RW_APP);
    LE_TEST_ASSERT((result == LE_OK), "Create PVM mount point app in /data/lxc_rw");

    result = lxcCreateDir(TELAF_LXC_RW_PERSIST);
    LE_TEST_ASSERT((result == LE_OK), "Create PVM mount point persist in /data/lxc_rw");

    result = lxcCreateDir(TELAF_LXC_RW_TMP);
    LE_TEST_ASSERT((result == LE_OK), "Create PVM mount point tmp in /data/lxc_rw");

    if (2 == le_arg_NumArgs())
    {
        // Use user provided configuration file
        snprintf(systemCmd, MAX_SYSTEM_CMD_LENGTH,
                 "lxc-create -n %s -f %s -t none -o %s/lxc-create.log -l TRACE",
                            lxcContainerNameStr,
                            le_arg_GetArg(1),
                            lxcLogPathStr);
    }
    else
    {
        // Use default configuration file
        snprintf(systemCmd, MAX_SYSTEM_CMD_LENGTH,
                 "lxc-create -n %s -f %s -t none -o %s/lxc-create.log -l TRACE",
                            lxcContainerNameStr,
                            lxcConfFileStr,
                            lxcLogPathStr);
    }
    LE_TEST_INFO("%s", systemCmd);
    result = system(systemCmd);
    if (0 != result)
    {
        return LE_FAULT;
    }
    return LE_OK;
}

/**
 * Destroy telaf container
 *
*/
static int lxcTestDestroyContainer()
{
    char systemCmd[MAX_SYSTEM_CMD_LENGTH] = {0};
    snprintf(systemCmd, MAX_SYSTEM_CMD_LENGTH, "lxc-destroy -n %s -o %s/lxc-destroy.log -l TRACE",
            lxcContainerNameStr, lxcLogPathStr);
    LE_TEST_INFO("%s", systemCmd);
    int result = system(systemCmd);

    // The conatiner is manually managed. So delete the container path
    if (le_dir_IsDir(TELAF_LXC_CONTAINER_PATH))
    {
        result = le_dir_RemoveRecursive(TELAF_LXC_CONTAINER_PATH);
    }

    if (0 != result)
    {
        return LE_FAULT;
    }
    return LE_OK;
}
/**
 * Start telaf container
 *
 */
static int lxcTestStartContainer()
{
    char systemCmd[MAX_SYSTEM_CMD_LENGTH] = {0};

    // Start the container
    snprintf(systemCmd, MAX_SYSTEM_CMD_LENGTH, "lxc-start -n %s -o %s/lxc-start.log -l TRACE",
             lxcContainerNameStr, lxcLogPathStr);
    LE_TEST_INFO("%s", systemCmd);
    int result = system(systemCmd);
    if (0 != result)
    {
        return LE_FAULT;
    }

    // Set IP address for the host interface
    LE_TEST_INFO("Container started. Setting IP address for host interface.");
    memset(systemCmd, 0, MAX_SYSTEM_CMD_LENGTH);
    snprintf(systemCmd, MAX_SYSTEM_CMD_LENGTH,
        "lxc-info -n %s | grep Link: | awk \'{print $2}\' | xargs -t -I {} /sbin/ifconfig {} %s",
        lxcContainerNameStr,
        lxcHostIPStr);
    LE_TEST_INFO("%s", systemCmd);
    result = system(systemCmd);
    if (0 != result)
    {
        return LE_FAULT;
    }
    puts("\n"
         "To attach to the container:\n"
         "   lxc-attach -n telaflxc\n"
         "To execute a command inside the container:\n"
         "   lxc-attach -n telaflxc <command>\n"
         "\n");
    return LE_OK;
}

/**
 * Stop telaf container
 *
 */
static int lxcTestStopContainer()
{
    char systemCmd[MAX_SYSTEM_CMD_LENGTH] = {0};
    snprintf(systemCmd, MAX_SYSTEM_CMD_LENGTH, "lxc-stop -n %s -o %s/lxc-stop.log -l TRACE",
             lxcContainerNameStr, lxcLogPathStr);
    LE_TEST_INFO("%s", systemCmd);
    int result = system(systemCmd);
    if (0 != result)
    {
        return LE_FAULT;
    }
    return LE_OK;
}

/**
 * Get telaf container status
 *
 */
static int lxcTestGetContainerStatus()
{
    char systemCmd[MAX_SYSTEM_CMD_LENGTH] = {0};
    snprintf(systemCmd, MAX_SYSTEM_CMD_LENGTH, "lxc-info -n %s", lxcContainerNameStr);
    // Check if lxc container exists or not. Sample error output is below
    /*
    / # lxc-info -n telaflxc
      telaflxc doesn't exist
    / # echo $?
    1
    */
    if ( 0 != system(systemCmd) )
    {
        LE_TEST_INFO("LXC Container does not exist");
        return LE_FAULT;
    }
    memset(systemCmd, 0, MAX_SYSTEM_CMD_LENGTH);
    snprintf(systemCmd, MAX_SYSTEM_CMD_LENGTH,
            "lxc-info -n %s | grep \"State\" | cut -d \":\" -f 2 | tr -d ' '", lxcContainerNameStr);
    LE_TEST_INFO("%s", systemCmd);
    char line[100] = {0};
    FILE *stream = popen(systemCmd, "r");
    if (stream)
    {
        while (fgets(line,100,stream))
        {
            LE_TEST_INFO ("LXC Container Status: %s", line);
            if ( '0' == line[0])
            {
                break;
            }
        }
    }
    pclose(stream);
    return LE_OK;
}

COMPONENT_INIT
{
    le_result_t status = LE_FAULT;

    LE_TEST_INFO("======== LXC Integration Test ========");

    if ( (1 == le_arg_NumArgs()) || (2 == le_arg_NumArgs()))
    {
        const char *testType = le_arg_GetArg(0);
        if (strncmp(testType, "create", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== LXC Create Container Test ========");
            status = lxcTestCreateContainer();
            LE_TEST_OK(LE_OK == status, "LXC Create Container test");
        }
        else if (strncmp(testType, "destroy", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== LXC Destroy Container Test ========");
            status = lxcTestDestroyContainer();
            LE_TEST_OK(LE_OK == status, "LXC Destroy Container test");
        }
        else if (strncmp(testType, "start", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== LXC Start Container Test ========");
            status = lxcTestStartContainer();
            LE_TEST_OK(LE_OK == status, "LXC Start Container test");
        }
        else if (strncmp(testType, "stop", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== LXC Stop Container Test ========");
            status = lxcTestStopContainer();
            LE_TEST_OK(LE_OK == status, "LXC Stop Container test");
        }
        else if (strncmp(testType, "status", strlen(testType)) == 0)
        {
            LE_TEST_INFO("======== LXC Container Get Status Test ========");
            status = lxcTestGetContainerStatus();
            LE_TEST_OK(LE_OK == status, "LXC Container Get Status test");
        }
        else
        {
            PrintUsage();
            LE_TEST_FATAL ("Invalid test type %s",testType);
        }
    }
    else
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }

    LE_TEST_EXIT;
}
