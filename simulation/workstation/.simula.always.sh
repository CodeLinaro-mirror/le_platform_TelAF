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
