#!/usr/bin/env bash

# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

MNGD_CONN_SVC_JSON=/legato/taf_rootfs/mngdConnectivity.json
if [ -e "$MNGD_CONN_SVC_JSON"  ]; then
    # Not overwrite the file that already exists.
    cp -n $MNGD_CONN_SVC_JSON /data/ManagedServices/
    # Set read and write permission
    chmod 666 /data/ManagedServices/mngdConnectivity.json
fi

# Try to export all environment variables for Common API
if [ -e /legato/taf_rootfs/etc/vsomeip/E01HelloWorld/commonapi4someip.ini ]; then
    export COMMONAPI_CONFIG=/legato/taf_rootfs/etc/vsomeip/E01HelloWorld/commonapi4someip.ini
    export VSOMEIP_CONFIGURATION=/legato/taf_rootfs/etc/vsomeip/E01HelloWorld/vsomeip-local.json
    export LD_LIBRARY_PATH=/legato/taf_rootfs/lib:$LD_LIBRARY_PATH
fi
