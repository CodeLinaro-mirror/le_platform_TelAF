#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

def algo_for_0x27(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

tcpdump_start("Routine Control Option check for V2")

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

# Record length is configured 1, and no record in request.
# NRC 0x13 since the received length(4) mismatches the configured length(4+1)
uds("31 01 02 46")
print("Exp      <7f 31 13>")

# Record length is configured 1, and there is 2 in request.
# NRC 0x13 since the received length(6) mismatches the configured length(4+1)
uds("31 01 02 46 08 08")
print("Exp      <7f 31 13>")

# Record length is configured 1, and there is 1 in request.
# positive resp as forbidden_characters not configured
uds("31 01 02 46 08")
print("Exp      <71 01 02 46>")

# Record length is configured 1, and no record in request.
# NRC 0x13 since the received length(4) mismatches the configured length(4+1)
uds("31 02 02 46")
print("Exp      <7f 31 13>")

# Record length is configured 1, and there is 2 in request.
# NRC 0x13 since the received length(6) mismatches the configured length(4+1)
uds("31 02 02 46 02 02")
print("Exp      <7f 31 13>")

# Record length is configured 1, and there is 1 in request.
# NRC 0x31 since the record data 02 is forbidden
uds("31 02 02 46 02")
print("Exp      <7f 31 31>")

# Record length is configured 1, and no record in request.
# NRC 0x13 since the received length(4) mismatches the configured length(4+1)
uds("31 03 02 46")
print("Exp      <7f 31 13>")

# Record length is configured 1, and there is 2 in request.
# NRC 0x13 since the received length(6) mismatches the configured length(4+1)
uds("31 03 02 46 02 02")
print("Exp      <7f 31 13>")

# Record length is configured 1, and there is 1 in request.
# NRC 0x31 since the max value is 20
uds("31 03 02 46 30")
print("Exp      <7f 31 31>")

# Record length is configured 1, and there is 1 in request.
# positive resp
uds("31 03 02 46 10")
print("Exp      <71 03 02 46>")

# Record length is configured 1, and no record in request.
# NRC 0x13 since the received length(4) mismatches the configured length(4+1)
uds("31 01 02 47")
print("Exp      <7f 31 13>")

# Record length is configured 1, and there is 1 in request.
# NRC 0x72 since the sample app returns 0x72 if diag/routine/targetFile is not configured
uds("31 01 02 47 08")
print("Exp      <7f 31 72>")

# Record length is configured 1, and there is 2 in request.
# NRC 0x13 since the received length(6) mismatches the configured length(4+1)
uds("31 01 02 47 08 08")
print("Exp      <7f 31 13>")

# Record length is configured 1, and there is 1 in request.
# NRC 0x31 since record data is not in the coding list
uds("31 02 02 47 08")
print("Exp      <7f 31 31>")

# Record length is configured 1, and there is 1 in request.
# NRC 0x31 since record data is not in the range(-10, 10)
uds("31 03 02 47 0B")
print("Exp      <7f 31 31>")

# Record length is configured 1, and there is 1 in request.
# Positvie response since record data is in the range(-10, 10)
uds("31 03 02 47 01")
print("Exp      positive resp")

# Record length is configured 1, and no record in request.
# NRC 0x13 since the received length(4) mismatches the configured length(4+1)
uds("31 01 02 48")
print("Exp      <7f 31 13>")

# Record length is configured 1, and there is 1 in request.
# NRC 0x21 since the handler is not registered by the app
uds("31 01 02 48 02")
print("Exp      <7f 31 21>")

# Record is null, and there is 1 in request.
# NRC 0x13 since the item stop of control_option_record is configured to empty but data record is 01
uds("31 02 02 48 01")
print("Exp      <7f 31 13>")

# Record is empty, and there is 1 in request.
# NRC 0x13 since the item result of control_option_record doesn't exist but data record is 03
uds("31 03 02 48 03")
print("Exp      <7f 31 13>")

# Record is null, and no record in request.
# Positive resp since the item start of control_option_record is configured to empty
uds("31 01 02 49")
print("Exp      <71 01 02 49>")

# Record is null, and there is 3 in request.
# NRC 0x13 since the item start of control_option_record is empty but data record is 000000
uds("31 01 02 49 00 00 00")
print("Exp      <7f 31 13>")

# Record is null, and there is 5 in request.
# NRC 0x13 since the item start of control_option_record is empty but data record is ffffffff
uds("31 02 02 49 FF FF FF FF FF")
print("Exp      <7f 31 13>")

# Record is null, and there is 7 in request.
# NRC 0x13 since the item result of control_option_record doesn't exist but data record has 7 bytes
uds("31 03 02 49 FF FF FF FF FF FF FF")
print("Exp      <7f 31 13>")

tcpdump_stop()
