#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear


payload = "56" * 1280
tail = "00 02 01CD 01CD"
command = "38 01 0500 " + payload + tail

def algo_for_0x27(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

tcpdump_start("test-38-nrc")

uds("1001")
uds("1002")
resp = uds("2701")

key = algo_for_0x27(0x01, resp.payload[2:])
uds("27 02" + key.hex())

uds("36")
print("Exp&Ind: <7f 36 13>")

uds("36 01")
print("Exp&Ind: <7f 36 24>")

uds("37")
print("Exp&Ind: <7f 37 24>")

uds("38 01")
print("Exp&Ind: <7f 38 13>")

uds("38 09 0014 2F")
print("Exp&Ind: <7f 38 31>")

uds("38 01 0000 2F")
print("Exp&Ind  <7f 38 31>")

uds("38 01 FFFF 2F6F736164732F6D616E69666573742E6A736F6E 00 02 01CD 01CD")
print("Exp&Ind  <7f 38 31>")

uds(command)
print("Exp&Ind  <7f 38 13>")

uds("38 01 0014 2F6F736164732F6D616E69666573742E6A736F6E 00 02 FFF0 FFFF")
print("Exp&Ind  <7f 38 31>")

# case 01: regardless DFI, C > UC:
uds("38 01 0014 2F6F736164732F6D616E69666573742E6A736F6E 01 02 FFF0 FFFF")
print("Exp&Ind  <7f 38 31>")

# case 02: DFI == 0x00, but UC != C:
uds("38 01 0014 2F6F736164732F6D616E69666573742E6A736F6E 00 02 01CD 01CB")
print("Exp&Ind  <7f 38 31>")

# case 03: DFI = [0x10 ~ 0xF0] & 0xF0, but UC == C:
uds("38 01 0014 2F6F736164732F6D616E69666573742E6A736F6E 10 02 01CD 01CD")
print("Exp&Ind  <7f 38 31>")

uds("38 01 0012 2F646174612f696d616765732F612E747874 10 01 06 05")
print("Exp      <Positive resp>")

uds("38 01 0012 2F646174612f696d616765732F612E747874 10 01 06 05")
print("Exp&Ind  <7f 38 22>")

uds("36 01 6161616161 0a")
print("Exp      <Positive resp>")

uds("37")
print("Exp      <Positive resp>")

uds("38 01 0012 2F646174612f696d616765732F622E747874 11 01 06 05")
print("Exp      <Positive resp>")

uds("36 01 6262626262 0a")
print("Exp      <Positive resp>")

uds("37")
print("Exp      <Positive resp>")

tcpdump_stop()
