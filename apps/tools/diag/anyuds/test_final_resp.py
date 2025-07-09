#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

def algo_for_0x27(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

tcpdump_start("test-38-nrc")

uds("1081")
uds("1082")
resp = uds("2781")

key = algo_for_0x27(0x01, resp.payload[2:])
uds("27 82" + key.hex())

uds("11 82")
uds("19 81 24")
uds("31 81 02 46")
uds("29 88")
uds("86 81 08 01 19 01 01")

tcpdump_stop()
