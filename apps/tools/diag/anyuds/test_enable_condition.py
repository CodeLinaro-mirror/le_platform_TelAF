#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

#test enable condition for V2
def algo_for_0x27(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)


uds("1001")
uds("1003")
resp = uds("2701")

key = algo_for_0x27(0x01, resp.payload[2:])
uds("27 02" + key.hex())

print("Start tafDiagApp with enableStatus=false; expect success when set to true")
uds("11 02")
print("Exp   enableStatusFalse:<7f 11 88>/enableStatusTrue:<Positive Rsp>")

uds("2E a5 a6 31")
print("Exp      enableStatusFalse:<7f 2e 22>/enableStatusTrue:<Positive Rsp>")

uds("2f 90 08 03 30")
print("Exp      enableStatusFalse:<7f 2f 83>/enableStatusTrue:<Positive Rsp>")

uds("31 01 02 4b")
print("Exp      enableStatusFalse:<7f 31 88>/enableStatusTrue:<Positive Rsp>")

