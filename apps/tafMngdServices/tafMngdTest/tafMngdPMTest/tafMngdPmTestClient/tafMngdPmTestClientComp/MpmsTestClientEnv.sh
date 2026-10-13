#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

function run_mpms_test_daemon()
{
    local plugin_path=/tmp/legato/devManager/drivers/pmVHalDrvTest.so

    if [ ! -f "$plugin_path" ]; then
        printf 'PM VHAL plugin was not found: %s\n' "$plugin_path"
        return 1
    fi

    if ! tafmodule install "$plugin_path"; then
        printf 'Failed to install PM VHAL plugin: %s\n' "$plugin_path"
        return 1
    fi

    if ! app restart tafMngdPMSvc; then
        printf 'Failed to restart tafMngdPMSvc\n'
        return 1
    fi

    if ! app start tafMngdPmTestDaemon; then
        printf 'Failed to start tafMngdPmTestDaemon\n'
        return 1
    fi
}

function run_mpms_test()
{
    app runProc tafMngdPmTestClient mpmsIntgTest -- "$@"
}

alias mpms.test.d='run_mpms_test_daemon'
alias mpms.test='run_mpms_test'
