#!/bin/sh
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

echo "Enter target (sa515m or sa525m):"
read TARGET

if [ $TARGET == 'sa525m' ]
then
    mkcomp -t sa525m \
        -X "-O2" -C "-O2" \
        ${MKTOOLS_X_C_FLAGS} \
        -X "-fPIC" \
        -X "-shared" \
        -X "-std=c++11" \
        -o uaPlugin.so \
        .

elif [ $TARGET == 'sa515m' ]
then
    mkcomp -t sa515m \
        -X "-O2" -C "-O2" \
        ${MKTOOLS_X_C_FLAGS} \
        -X "-fPIC" \
        -X "-shared" \
        -X "-std=c++11" \
        -o uaPlugin.so \
        .

else
  echo "`basename ${0}`:usage: [-t target] "
fi
