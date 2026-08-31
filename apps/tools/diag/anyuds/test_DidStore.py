# Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

def algo_for_0x27(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

tcpdump_start("Test DIDStoreSvc")

#Session control
uds("10 01")
uds("10 03")

#Unlock level
resp = uds("2701")
key = algo_for_0x27(0x01, resp.payload[2:])
uds("27 02" + key.hex())

#WriteDID
uds("2E A5 A5 33 34")

#ReadDID
uds("22 A5 A5")

#WriteDID
uds("2E A5 A6 35")

#ReadDID
uds("22 A5 A6")

#ReadDID
uds("22 A5 A5 A5 A6")

#WriteDID
uds("2E A0 A0 37")

#ReadDID
uds("22 A0 A0")

#WriteDID
uds("2E A0 A2 38")

#ReadDID
uds("22 A0 A2")

#Read All DID
uds("22 A5 A5 A5 A6 A0 A0 A0 A2")

#Read All DID and F011(written by test app)
uds("22 A5 A5 A5 A6 A0 A0 A0 A2 F0 11")

tcpdump_stop()
