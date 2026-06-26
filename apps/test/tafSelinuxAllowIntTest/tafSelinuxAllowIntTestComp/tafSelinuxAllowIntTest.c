// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "legato.h"
#include "interfaces.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define TEST_APP_NAME "tafSelinuxAllowIntTest"
#define EXPECTED_DOMAIN "telaf_tafSelinuxAllowIntTest_t"
#define TEST_EXEC_PATH "/legato/systems/current/appsWriteable/tafSelinuxAllowIntTest/bin/tafSelinuxAllowIntTest"
#define TEST_CONFIG_PATH "/selinuxAllowIntTest/result"
#define PRIVATE_DATA_PATH "/legato/systems/current/appsWriteable/tafSelinuxAllowIntTest/selinux_allow_private_data.txt"

static int FailureCount = 0;

static void PrintStartBanner(void)
{
    LE_INFO("============================================================");
    LE_INFO("SELINUX TEST START: %s", TEST_APP_NAME);
    LE_INFO("TYPE: ALLOW / expected-allow validation");
    LE_INFO("============================================================");
}

static void PrintResultBanner(void)
{
    if (FailureCount == 0)
    {
        LE_INFO("============================================================");
        LE_INFO("SELINUX TEST RESULT: PASS - %s", TEST_APP_NAME);
        LE_INFO("============================================================");
    }
    else
    {
        LE_ERROR("============================================================");
        LE_ERROR("SELINUX TEST RESULT: FAIL - %s (%d failed)",
                 TEST_APP_NAME, FailureCount);
        LE_ERROR("============================================================");
    }
}

static void RecordFailure(const char* testName, const char* detail)
{
    FailureCount++;
    LE_ERROR("SELINUX_TEST_FAIL: %s: %s", testName, detail);
}

static void ExpectCurrentDomain(void)
{
    char context[256] = "";
    int fd = open("/proc/self/attr/current", O_RDONLY);
    if (fd < 0)
    {
        char detail[128];
        snprintf(detail, sizeof(detail), "open attr/current failed: errno=%d (%s)",
                 errno, strerror(errno));
        RecordFailure("process SELinux context", detail);
        return;
    }

    ssize_t bytes = read(fd, context, sizeof(context) - 1);
    close(fd);

    if (bytes < 0)
    {
        char detail[128];
        snprintf(detail, sizeof(detail), "read attr/current failed: errno=%d (%s)",
                 errno, strerror(errno));
        RecordFailure("process SELinux context", detail);
        return;
    }

    context[bytes] = '\0';
    if (strstr(context, EXPECTED_DOMAIN) == NULL)
    {
        char detail[320];
        snprintf(detail, sizeof(detail), "expected domain '%s' in context '%s'",
                 EXPECTED_DOMAIN, context);
        RecordFailure("process SELinux context", detail);
        return;
    }

    LE_INFO("SELINUX_TEST_PASS: process SELinux context: %s", context);
}

static void ExpectReadFileSuccess(const char* testName, const char* path)
{
    unsigned char byte = 0;
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        char detail[256];
        snprintf(detail, sizeof(detail), "open(%s) failed: errno=%d (%s)",
                 path, errno, strerror(errno));
        RecordFailure(testName, detail);
        return;
    }

    ssize_t bytes = read(fd, &byte, sizeof(byte));
    close(fd);

    if (bytes != sizeof(byte))
    {
        char detail[128];
        snprintf(detail, sizeof(detail), "read(%s) returned %zd", path, bytes);
        RecordFailure(testName, detail);
        return;
    }

    LE_INFO("SELINUX_TEST_PASS: %s", testName);
}

static void ExpectPrivateDataReadSuccess(void)
{
    ExpectReadFileSuccess("read own bundled private data", PRIVATE_DATA_PATH);
}

static void ExpectUdpSocketSuccess(void)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        char detail[160];
        snprintf(detail, sizeof(detail), "socket(AF_INET, SOCK_DGRAM) failed: errno=%d (%s)",
                 errno, strerror(errno));
        RecordFailure("create UDP socket", detail);
        return;
    }

    close(fd);
    LE_INFO("SELINUX_TEST_PASS: create UDP socket");
}

static void ExecShellAsFinalAllowTest(void)
{
    execl("/bin/sh",
          "sh",
          "-c",
          "echo 'SELINUX_TEST_PASS: execute shell via telaf_exec_shell'; "
          "echo '============================================================'; "
          "echo 'SELINUX TEST RESULT: PASS - tafSelinuxAllowIntTest'; "
          "echo '============================================================'; "
          "exit 0",
          (char*)NULL);

    char detail[128];
    snprintf(detail, sizeof(detail), "execl(/bin/sh) failed: errno=%d (%s)", errno, strerror(errno));
    RecordFailure("execute shell via telaf_exec_shell", detail);
}

static void ExpectConfigTreeSuccess(void)
{
    le_cfg_IteratorRef_t writeTxn = le_cfg_CreateWriteTxn(TEST_CONFIG_PATH);
    le_cfg_SetString(writeTxn, "", "pass");
    le_cfg_CommitTxn(writeTxn);

    char buffer[32] = "";
    le_cfg_IteratorRef_t readTxn = le_cfg_CreateReadTxn(TEST_CONFIG_PATH);
    le_result_t result = le_cfg_GetString(readTxn, "", buffer, sizeof(buffer), "");
    le_cfg_CancelTxn(readTxn);

    if (result != LE_OK)
    {
        char detail[128];
        snprintf(detail, sizeof(detail), "le_cfg_GetString failed: %s", LE_RESULT_TXT(result));
        RecordFailure("config tree read", detail);
        return;
    }

    if (strcmp(buffer, "pass") != 0)
    {
        char detail[128];
        snprintf(detail, sizeof(detail), "unexpected config value '%s'", buffer);
        RecordFailure("config tree readback", detail);
        return;
    }

    LE_INFO("SELINUX_TEST_PASS: config tree read/write");
}

static void ExpectUrandomRead(void)
{
    unsigned char byte = 0;
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
    {
        char detail[128];
        snprintf(detail, sizeof(detail), "open /dev/urandom failed: errno=%d (%s)",
                 errno, strerror(errno));
        RecordFailure("read /dev/urandom", detail);
        return;
    }

    ssize_t bytes = read(fd, &byte, sizeof(byte));
    close(fd);

    if (bytes != sizeof(byte))
    {
        char detail[128];
        snprintf(detail, sizeof(detail), "read /dev/urandom returned %zd", bytes);
        RecordFailure("read /dev/urandom", detail);
        return;
    }

    LE_INFO("SELINUX_TEST_PASS: read /dev/urandom");
}

COMPONENT_INIT
{
    PrintStartBanner();
    LE_INFO("%s started, pid=%d", TEST_APP_NAME, getpid());

    ExpectCurrentDomain();

    ExpectReadFileSuccess("read own executable", TEST_EXEC_PATH);
    ExpectPrivateDataReadSuccess();
    ExpectConfigTreeSuccess();
    ExpectUrandomRead();
    ExpectReadFileSuccess("read global proc file via telaf_read_proc_files", "/proc/cpuinfo");
    ExpectReadFileSuccess("read init process status via telaf_app_read_proc_state", "/proc/1/status");
    ExpectUdpSocketSuccess();
    ExpectReadFileSuccess("read vm overcommit sysctl via telaf_read_vm_overcommit_sysctl",
                          "/proc/sys/vm/overcommit_memory");

    if (FailureCount == 0)
    {
        ExecShellAsFinalAllowTest();
    }

    PrintResultBanner();
    exit(EXIT_FAILURE);
}
