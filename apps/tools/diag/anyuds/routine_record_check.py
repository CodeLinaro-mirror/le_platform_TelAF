#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

def algo_for_0x27(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

tcpdump_start("Routine Control Option check")

uds("10 01")
uds("10 03")

# Authentication
uds("290411221f40"+ (bytes(8000).hex()))

uds("2901001000"+bytes(4096).hex()+"0400"+bytes(1024).hex())

uds("29030800"+bytes(2048).hex()+bytes(2).hex())

uds("29 08")

# security access
resp = uds("2701")

key = algo_for_0x27(0x01, resp.payload[2:])
uds("27 02" + key.hex())

# positive resp
uds("31 01 02 46")

# NRC 0x13
uds("31 01 02 46 08 08")

# positive resp as forbidden_characters not configured
uds("31 01 02 46 08")

# positive resp
uds("31 02 02 46")

# NRC 0x13
uds("31 02 02 46 02 02")

# NRC 0x31
uds("31 02 02 46 02")

# positive resp
uds("31 03 02 46")

# NRC 0x13
uds("31 03 02 46 02 02")

# NRC 0x31
uds("31 03 02 46 30")

# positive resp
uds("31 03 02 46 10")

# positive resp
uds("31 01 02 47")

# NRC 0x13
uds("31 01 02 47 08 08")

# NRC 0x31
uds("31 02 02 47 08")

# NRC 0x31
uds("31 03 02 47 0B")

# NRC 0x21
uds("31 01 02 48")

uds("31 01 02 48 02")

uds("31 02 02 48 01")

uds("31 03 02 48 02")

tcpdump_stop()
