#!/bin/sh
# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

function compile() {
  local target=$TARGET
  echo "You chose $target"
  mkcomp -t "$target" \
    -X "-O2" -C "-O2" \
    -X "-fPIC" \
    -X "-shared" \
    -X "-std=c++11" \
    -o "$target/pmVHalDrv.so" \
    .
}

echo "Enter target (sa515m or sa525m):"
read TARGET

compile