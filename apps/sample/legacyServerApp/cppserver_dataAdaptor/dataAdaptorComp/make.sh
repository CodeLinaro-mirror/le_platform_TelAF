#!/bin/bash

# Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

cp -R "$TELAF_ROOT/apps/sample/legacyServerApp/cppserver_dataAdaptor/dataAdaptorComp/" dataAdaptorComp
export TARGET=$TARGET_GLOBAL

cd dataAdaptorComp
rm -rf build
if [ ! -d build ];then
    mkdir build
else
    echo build exist
fi

cd build
cmake ..
make