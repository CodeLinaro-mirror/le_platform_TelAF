#!/bin/sh
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

function compile() {
  local target=$TARGET
  echo "You chose $target"
  mkcomp -t "$target" \
    -X "-O2" -C "-O2" \
    ${MKTOOLS_X_C_FLAGS} \
    -X "-fPIC" \
    -X "-shared" \
    -X "-std=c++17" \
    -o "$target/pmVHalDrvTest.so" \
    .
}

echo "Enter target (sa515m or sa525m):"
read TARGET

compile
