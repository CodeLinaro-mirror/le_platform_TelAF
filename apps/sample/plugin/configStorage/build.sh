#!/bin/sh
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

echo "Enter target (sa515m or sa525m):"
read TARGET

if [ $TARGET == 'sa525m' ]
then
    echo "You chose sa525m"
    mkcomp -t sa525m \
        ${MKTOOLS_X_C_FLAGS} \
        -X "-O2" -C "-O2" \
        -X "-fPIC" \
        -X "-shared" \
        -X "-std=c++17" \
        -o sa525m/tafPiCfgStor.so \
        .

elif [ $TARGET == 'sa515m' ]
then
    echo "You chose sa515m"
    mkcomp -t sa515m \
        ${MKTOOLS_X_C_FLAGS} \
        -X "-O2" -C "-O2" \
        -X "-fPIC" \
        -X "-shared" \
        -X "-std=c++17" \
        -o sa515m/tafPiCfgStor.so \
        .

else
  echo "`basename ${0}`:usage: [-t target] "
fi
