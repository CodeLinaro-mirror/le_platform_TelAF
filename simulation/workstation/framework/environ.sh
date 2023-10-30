#!/bin/bash

# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SDK_BIN_HOME=PATH=/legato/systems/current/sdk_rootfs/bin
export PATH=$SDK_BIN_HOME:$SCRIPT_DIR:$PATH

alias simula="python3 $SCRIPT_DIR/main.py"
