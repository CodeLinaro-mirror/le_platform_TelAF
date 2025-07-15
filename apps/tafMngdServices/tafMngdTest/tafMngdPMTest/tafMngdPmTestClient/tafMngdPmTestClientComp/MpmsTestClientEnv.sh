#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

function try_to_run_mpms_test_daemon()
{
    if ! [ -e /tmp/legato/devManager/drivers/pmVHalDrvTest.so ]; then
        echo "Plugin was not found"
        return
    fi

    # Anyway, try to install again
    tafmodule install pmVHalDrvTest.so

    app restart tafMngdPMSvc

    app start tafMngdPmTestDaemon
}

alias mpms.test.d="try_to_run_mpms_test_daemon"
alias mpms.test="app runProc tafMngdPmTestClient tafMngdPmTestClient --"
