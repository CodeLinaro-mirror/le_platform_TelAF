#!/usr/bin/env bash

# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

MNGD_CONN_SVC_JSON=/legato/taf_rootfs/mngdConnectivity.json
if [ -e "$MNGD_CONN_SVC_JSON"  ]; then
    # Not overwrite the file that already exists.
    cp -n $MNGD_CONN_SVC_JSON /data/ManagedServices/
fi
