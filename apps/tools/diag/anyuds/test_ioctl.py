#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

def algo_for_0x27(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

tcpdump_start("test-2f")

uds("1001")
uds("1002")
resp = uds("2701")

key = algo_for_0x27(0x01, resp.payload[2:])
uds("27 02" + key.hex())

uds("2f 90 07 03 30")
print("Exp      <Positive resp>")

uds("2f 90 07 00 05")
print("Exp      <7f 2f 13>")

uds("2f 90 07 03 30 01")
print("Exp      <7f 2f 13>")

uds("2f 90 07 03 30 01 01 03 04 05")
print("Exp      <7f 2f 13>")

tcpdump_stop()
