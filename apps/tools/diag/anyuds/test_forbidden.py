#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear



"""
config set diag/DID/a5a5/value/data1 1 int
config set diag/DID/a5a5/value/data2 1 int
config set diag/DID/a5a6/value/data1 1 int
config set diag/DID/a0a0/value/data1 1 int
config set diag/DID/a0a1/value/data1 1 int
config set diag/DID/acc0/value/data1 66 int
config set diag/DID/acc1/value/data1 170 int
config set diag/DID/acc1/value/data2 170 int
config set diag/DID/acc2/value/data1 170 int
config set diag/DID/acc2/value/data2 187 int
config set diag/DID/acc3/value/data1 1 int
config set diag/DID/acc4/value/data1 1 int
"""

def algo_for_0x27(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

tcpdump_start("forbidden-check")


uds("10 01")
uds("10 03")

#0xA5A5 ---> 1 ----> Not Numeric ----> Both forbidden_values and forbidden_characters are empty

#Read DID
uds("22 A5 A5")

#WriteDID
uds("2E A5 A5 05 05")

#Read DID
uds("22 A5 A5")


#0xA5A6 ----> 1 ----> Not Numeric ----> Both forbidden_values and forbidden_characters are empty

#Read DID
uds("22 A5 A6")

#WriteDID
uds("2E A5 A6 06")

#Read DID
uds("22 A5 A6")

#Unlock level
resp = uds("2701")

key = algo_for_0x27(0x01, resp.payload[2:])
uds("27 02" + key.hex())

#0xA0A0 ---> 1  -----> Not Numeric ----> Both forbidden_values and forbidden_characters are empty

#Read DID
uds("22 A0 A0")

#WriteDID
uds("2E A0 A0 07")

#Read DID
uds("22 A0 A0")

#0xACC0 ----> 1 ----> Not Numeric ---> forbiddden_characters = {{0x00..0x2F},{0x3A..0x40},{0x5B..0x7F}}
print()

#ReadDID
uds("22 AC C0")

#WriteDID

#positive case
uds("2E AC C0 41")
uds("22 AC C0")

#NRC0x31
uds("2E AC C0 04")


#0xACC1 ---> 2 ----> Not Numeric ---> forbidden_values = {0xFFCC, 0xFFDD, 0xAACC}
print()
#ReadDID
uds("22 AC C1")

#WriteDID

#positive case
uds("2E AC C1 AA BB")
uds("22 AC C1")

#NRC0x31
uds("2E AC C1 AA CC")


#0xACC2 ---> 2 ----> Not Numeric ----> forbidden_values = {0xFFFF, 0xCCAA}
print()
#ReadDID
uds("22 AC C2")

#WriteDID

#positive case
uds("2E AC C2 AA CC")
uds("22 AC C2")

#NRC0x31
uds("2E AC C2 FF FF")

#NRC0x31
uds("2E AC C2 FF 1F")

#NRC0x31
uds("2E AC C2 03 FF")

#0xACC3 ---> 1 ---> Numeric -----> allowed = {0x00, 0x01, 0x10}
print()
#ReadDID
uds("22 AC C3")

#WriteDID

#positive case
uds("2E AC C3 28")
uds("22 AC C3")

#NRC0x31
uds("2E AC C3 c0")

#0xACC4 ---> 1 ---> Numeric ----> allowed = {min=0, max=20}
print()
#ReadDID
uds("22 AC C4")

#WriteDID

#positive case
uds("2E AC C4 10")
uds("22 AC C4")

#NRC0x31
uds("2E AC C4 28")

#NRC0x31
uds("2E AC C4 FF") # -1


#0xACC5 ---> 1 ---> Numeric list ----> allowed = {{0b00,0b01,0b10},{0b1010,0b0101},{0b00,0b01,0b10}}
print()
#WriteDID

#positive case
uds("2E AC C5 AA")

uds("22 AC C5")

#NRC0x31
uds("2E AC C5 FF")

#NRC0x31
uds("2E AC C5 3A") # 0b00111010

#NRC0x31
uds("2E AC C5 AB") # 0b10101011

#NRC0x31
uds("2E AC C5 BE") # 0b10111110


#0xACC6 ---> 2 ---> Numeric list ----> allowed = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07}
print()
#WriteDID

#positive case
uds("2E AC C6 04 04")
uds("22 AC C6")

#NRC0x31
uds("2E AC C6 08 08")

#NRC0x31
uds("2E AC C6 08 04")

#NRC0x31
uds("2E AC C6 03 08")

uds("22 AC C6")

#0xACC7 ---> 1 ---> Numeric list ----> allowed = {min=-10, max=10}
print()
#WriteDID

#positive case
uds("2E AC C7 01")
uds("22 AC C7")

#NRC0x31
uds("2E AC C7 0B") # 11

#NRC0x31
uds("2E AC C7 F5") # -11

#positive case
uds("2E AC C7 F6") # -10

tcpdump_stop()
