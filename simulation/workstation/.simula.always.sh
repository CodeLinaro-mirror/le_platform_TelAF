#!/usr/bin/env bash

# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

function append_msg_to_syslogd()
{
    logger -- "-------------------------- [ $@ ] --------------------------"
}

function list_between_marks()
{
    logread | sed -n "/${1}/,/${2}/p"
}

alias mark='append_msg_to_syslogd'
alias listm='list_between_marks'

# Try to export all environment variables for Common API
if [ -e /legato/taf_rootfs/etc/vsomeip/E01HelloWorld/commonapi4someip.ini ]; then
    export COMMONAPI_CONFIG=/legato/taf_rootfs/etc/vsomeip/E01HelloWorld/commonapi4someip.ini
    export VSOMEIP_CONFIGURATION=/legato/taf_rootfs/etc/vsomeip/E01HelloWorld/vsomeip-local.json
    export LD_LIBRARY_PATH=/legato/taf_rootfs/lib:$LD_LIBRARY_PATH
fi
