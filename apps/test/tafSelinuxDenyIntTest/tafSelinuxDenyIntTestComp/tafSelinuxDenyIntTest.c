// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "legato.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define TEST_APP_NAME "tafSelinuxDenyIntTest"
#define EXPECTED_DOMAIN "telaf_tafSelinuxDenyIntTest_t"
#define ALLOW_PRIVATE_DATA_PATH "/legato/systems/current/appsWriteable/tafSelinuxAllowIntTest/selinux_allow_private_data.txt"

static int FailureCount = 0;
static int SkipCount = 0;

static void PrintStartBanner(void)
{
    LE_INFO("============================================================");
    LE_INFO("SELINUX TEST START: %s", TEST_APP_NAME);
    LE_INFO("TYPE: DENY / expected-deny validation");
    LE_INFO("Expected AVC denials from this app are normal.");
    LE_INFO("============================================================");
}

static void PrintResultBanner(void)
{
    if (FailureCount == 0)
    {
        LE_INFO("============================================================");
        LE_INFO("SELINUX TEST RESULT: PASS - %s (%d skipped)", TEST_APP_NAME, SkipCount);
        LE_INFO("============================================================");
    }
    else
    {
        LE_ERROR("============================================================");
        LE_ERROR("SELINUX TEST RESULT: FAIL - %s (%d failed, %d skipped)",
                 TEST_APP_NAME, FailureCount, SkipCount);
        LE_ERROR("============================================================");
    }
}

static void RecordFailure(const char* testName, const char* detail)
{
    FailureCount++;
    LE_ERROR("SELINUX_TEST_FAIL: %s: %s", testName, detail);
}

static void RecordSkip(const char* testName, const char* detail)
{
    SkipCount++;
    LE_WARN("SELINUX_TEST_SKIP: %s: %s", testName, detail);
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

static void ExpectOpenDenied(const char* testName, const char* path, int flags)
{
    int fd = open(path, flags, 0644);
    if (fd >= 0)
    {
        close(fd);
        char detail[256];
        snprintf(detail, sizeof(detail), "open(%s) unexpectedly succeeded", path);
        RecordFailure(testName, detail);
        return;
    }

    if ((errno == EACCES) || (errno == EPERM) || (errno == EROFS))
    {
        LE_INFO("SELINUX_TEST_PASS: %s: denied as expected: errno=%d (%s)",
                testName, errno, strerror(errno));
        return;
    }

    if (errno == ENOENT)
    {
        char detail[256];
        snprintf(detail, sizeof(detail), "path does not exist: %s", path);
        RecordSkip(testName, detail);
        return;
    }

    char detail[256];
    snprintf(detail, sizeof(detail), "expected EACCES/EPERM/EROFS but got errno=%d (%s)",
             errno, strerror(errno));
    RecordFailure(testName, detail);
}

static void ExpectAllowPrivateDataDenied(void)
{
    ExpectOpenDenied("read Allow app private data", ALLOW_PRIVATE_DATA_PATH, O_RDONLY);
}

static void ExpectUdpSocketDenied(void)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd >= 0)
    {
        close(fd);
        RecordFailure("create UDP socket without telaf_create_stream_sockets",
                      "socket(AF_INET, SOCK_DGRAM) unexpectedly succeeded");
        return;
    }

    if ((errno == EACCES) || (errno == EPERM))
    {
        LE_INFO("SELINUX_TEST_PASS: create UDP socket without telaf_create_stream_sockets: "
                "denied as expected: errno=%d (%s)", errno, strerror(errno));
        return;
    }

    char detail[160];
    snprintf(detail, sizeof(detail), "expected EACCES/EPERM but got errno=%d (%s)",
             errno, strerror(errno));
    RecordFailure("create UDP socket without telaf_create_stream_sockets", detail);
}

static void ExecShellAsFinalDenyTest(void)
{
    execl("/bin/sh",
          "sh",
          "-c",
          "echo 'SELINUX_TEST_FAIL: execute shell without telaf_exec_shell: shell unexpectedly executed successfully'; "
          "echo '============================================================'; "
          "echo 'SELINUX TEST RESULT: FAIL - tafSelinuxDenyIntTest'; "
          "echo '============================================================'; "
          "exit 1",
          (char*)NULL);

    if ((errno == EACCES) || (errno == EPERM))
    {
        LE_INFO("SELINUX_TEST_PASS: execute shell without telaf_exec_shell: denied as expected: "
                "errno=%d (%s)", errno, strerror(errno));
        return;
    }

    char detail[128];
    snprintf(detail, sizeof(detail), "expected EACCES/EPERM but got errno=%d (%s)",
             errno, strerror(errno));
    RecordFailure("execute shell without telaf_exec_shell", detail);
}

COMPONENT_INIT
{
    PrintStartBanner();
    LE_INFO("%s started, pid=%d", TEST_APP_NAME, getpid());

    ExpectCurrentDomain();
    ExpectAllowPrivateDataDenied();
    ExpectOpenDenied("read init process status without telaf_app_read_proc_state", "/proc/1/status", O_RDONLY);
    ExpectUdpSocketDenied();
    ExpectOpenDenied("read vm overcommit sysctl without telaf_read_vm_overcommit_sysctl",
                     "/proc/sys/vm/overcommit_memory", O_RDONLY);
    ExpectOpenDenied("read /etc/shadow", "/etc/shadow", O_RDONLY);
    ExpectOpenDenied("write framework bin directory",
                     "/legato/systems/current/bin/selinux_denied_test",
                     O_CREAT | O_WRONLY | O_TRUNC);
    ExpectOpenDenied("read init process memory", "/proc/1/mem", O_RDONLY);
    ExpectOpenDenied("write sysfs", "/sys/selinux_denied_test", O_CREAT | O_WRONLY | O_TRUNC);

    if (FailureCount == 0)
    {
        ExecShellAsFinalDenyTest();
    }

    PrintResultBanner();
    exit((FailureCount == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
