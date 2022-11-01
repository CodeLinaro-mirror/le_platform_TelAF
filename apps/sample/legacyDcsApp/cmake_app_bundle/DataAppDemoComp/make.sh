#!/bin/bash

# Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

cp -R "$TELAF_ROOT/apps/sample/legacyDcsApp/cmake_app_bundle/DataAppDemoComp/" DataAppDemoComp
export TARGET=sa515m

cd DataAppDemoComp
rm -rf build
if [ ! -d build ];then
    mkdir build
else
    echo build exist
fi

cd build
cmake ..
make