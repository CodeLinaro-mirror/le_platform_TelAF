#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

def algo_for_0x27(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

tcpdump_start("test-38-nrc")

uds("1001")
uds("1003")
resp = uds("2701")

key = algo_for_0x27(0x01, resp.payload[2:])
uds("27 02" + key.hex())

uds("38 01 FFFF 2F6F736164732F6D616E69666573742E6A736F6E 00 02 01CD 01CD")
uds("38 01 0014 2F6F736164732F6D616E69666573742E6A736F6E 00 02 FFFF 01CD")
uds("38 01 0014 2F6F736164732F6D616E69666573742E6A736F6E 00 02 01CD FFFF")

tcpdump_stop()
