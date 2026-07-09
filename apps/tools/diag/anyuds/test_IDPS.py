#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

# test_IDPS.py
#
# Sends UDS requests for all IDPS-monitored services to exercise both the
# positive-response path and the NRC path.
#
# Services covered:
#   0x10  DiagnosticSessionControl
#   0x11  ECUReset
#   0x14  ClearDiagnosticInformation
#   0x19  ReadDTCInformation
#   0x22  ReadDataByIdentifier
#   0x23  ReadMemoryByAddress
#   0x27  SecurityAccess
#   0x29  Authentication
#   0x2C  DynamicallyDefineDataIdentifier
#   0x2E  WriteDataByIdentifier
#   0x2F  InputOutputControlByIdentifier
#   0x31  RoutineControl
#   0x34  RequestDownload
#   0x35  RequestUpload
#   0x36  TransferData
#   0x37  RequestTransferExit
#   0x38  RequestFileTransfer
#   0x3D  WriteMemoryByAddress
#   0x3E  TesterPresent
#
# Usage:
#   python3 anyuds.py test_IDPS.py
#
# Environment variables (optional):
#   DOIP_REMOTE_IP  -- target IP  (default: 192.168.225.1)
#   DOIP_PHY_ADDR   -- target addr (default: 0x0201)

# ---------------------------------------------------------------------------
# Helper: compute SecurityAccess key (same algorithm used across all tests)
# ---------------------------------------------------------------------------
def algo_for_0x27(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

# ---------------------------------------------------------------------------
# Helper: print a section banner
# ---------------------------------------------------------------------------
def section(title):
    print()
    print("=" * 60)
    print(f"  {title}")
    print("=" * 60)

# ---------------------------------------------------------------------------
# Helper: annotate expected outcome inline
# ---------------------------------------------------------------------------
def expect(description):
    print(f"  Exp: {description}")

# ===========================================================================
tcpdump_start("test-IDPS")
# ===========================================================================

# ---------------------------------------------------------------------------
# 0x10  DiagnosticSessionControl
# ---------------------------------------------------------------------------
section("0x10 DiagnosticSessionControl -- Positive")

uds("10 01")
expect("<50 01 ...>")

section("0x10 DiagnosticSessionControl -- NRC")

uds("10")
expect("<7f 10 13>")

# ---------------------------------------------------------------------------
# 0x11  ECUReset
# ---------------------------------------------------------------------------
section("0x11 ECUReset -- Positive")

uds("10 03")
uds("11 02")
expect("<51 02>  IDPS")

section("0x11 ECUReset -- NRC")

uds("11 01 02")
expect("<7f 11 13>  IDPS")

# ---------------------------------------------------------------------------
# 0x14  ClearDiagnosticInformation
# ---------------------------------------------------------------------------
section("0x14 ClearDiagnosticInformation -- Positive")

uds("10 03")
uds("14 FF FF FF")
expect("<54>")

section("0x14 ClearDiagnosticInformation -- NRC")

uds("14 AB CD EF")
expect("<7f 14 31>")

# ---------------------------------------------------------------------------
# 0x19  ReadDTCInformation
# ---------------------------------------------------------------------------
section("0x19 ReadDTCInformation -- Positive")

uds("10 03")
uds("19 01 FF")
expect("<59 01 ...>")

section("0x19 ReadDTCInformation -- NRC")

uds("19 99")
expect("<7f 19 12>")

# ---------------------------------------------------------------------------
# 0x22  ReadDataByIdentifier
# ---------------------------------------------------------------------------
section("0x22 ReadDataByIdentifier -- Positive")

uds("10 01")
uds("22 a5 a5")
expect("<22 a5 a5 ...>")

section("0x22 ReadDataByIdentifier -- NRC")

uds("22 FF FF")
expect("<7f 22 31>")

# ---------------------------------------------------------------------------
# 0x23  ReadMemoryByAddress
# ---------------------------------------------------------------------------
section("0x23 ReadMemoryByAddress -- NRC")

uds("23 12 00 00 10 00 01")
expect("<7f 23 11>  IDPS")

# ---------------------------------------------------------------------------
# 0x27  SecurityAccess
# ---------------------------------------------------------------------------
section("0x27 SecurityAccess -- Positive")

uds("10 02")
resp = uds("27 01")
expect("<67 01 ...>")

key = algo_for_0x27(0x01, resp.payload[2:])
uds("27 02" + key.hex())
expect("<67 02>  IDPS")

section("0x27 SecurityAccess -- NRC")

uds("10 02")

uds("27 09")
expect("<7f 27 12>  IDPS")

# ---------------------------------------------------------------------------
# 0x29  Authentication
# ---------------------------------------------------------------------------
section("0x29 Authentication -- Positive")

uds("10 01")
uds("29 08")
expect("<69 08 00>")

section("0x29 Authentication -- NRC")

uds("29 07 00 00 00")
expect("<7f 29 12>  IDPS")

# ---------------------------------------------------------------------------
# 0x2C  DynamicallyDefineDataIdentifier
# ---------------------------------------------------------------------------
section("0x2C DynamicallyDefineDataIdentifier -- NRC")

uds("2c 01 f3 00 a5 a5 01 01")
expect("<7f 2c 11>  IDPS")

# ---------------------------------------------------------------------------
# 0x2E  WriteDataByIdentifier
# ---------------------------------------------------------------------------
section("0x2E WriteDataByIdentifier -- Positive")

uds("10 03")
resp = uds("27 01")
key = algo_for_0x27(0x01, resp.payload[2:])
uds("27 02" + key.hex())
expect("<67 02>  IDPS")

uds("2E a5 a6 31")
expect("<6e a5 a6>  IDPS")

section("0x2E WriteDataByIdentifier -- NRC")

uds("10 01")
uds("2E A0 A0 31")
expect("<7f 2e 31>  IDPS")

# ---------------------------------------------------------------------------
# 0x2F  InputOutputControlByIdentifier
# ---------------------------------------------------------------------------
section("0x2F InputOutputControlByIdentifier -- Positive")

uds("10 03")
resp = uds("27 01")
key = algo_for_0x27(0x01, resp.payload[2:])
uds("27 02" + key.hex())
expect("<67 02>  IDPS")

uds("2f 90 07 03 30")
expect("<6f 90 07 ...>  IDPS")

section("0x2F InputOutputControlByIdentifier -- NRC")

uds("2f FF FF 03 30")
expect("<7f 2f 31>  IDPS")

# ---------------------------------------------------------------------------
# 0x31  RoutineControl
# ---------------------------------------------------------------------------
section("0x31 RoutineControl -- Positive")

uds("10 03")
resp = uds("27 01")
key = algo_for_0x27(0x01, resp.payload[2:])
uds("27 02" + key.hex())
expect("<67 02>  IDPS")

uds("31 01 02 49")
expect("<71 01 02 49>  IDPS")

section("0x31 RoutineControl -- NRC")

uds("31 04 02 49")
expect("<7f 31 12>  IDPS")

# ---------------------------------------------------------------------------
# 0x34  RequestDownload
# ---------------------------------------------------------------------------
section("0x34 RequestDownload -- NRC")

uds("34 00")
#expect NRC 0x13 since 0x34 is supported by default
expect("<7f 34 13>  IDPS")

# ---------------------------------------------------------------------------
# 0x35  RequestUpload
# ---------------------------------------------------------------------------
section("0x35 RequestUpload -- NRC")

uds("35 00 44 00 00 10 00 00 00 01 00")
expect("<7f 35 11>  IDPS")

# ---------------------------------------------------------------------------
# 0x38  RequestFileTransfer, 0x36  TransferData, 0x37  RequestTransferExit
# ---------------------------------------------------------------------------
section("0x38 RequestFileTransfer -- Positive")

uds("10 02")
resp = uds("27 01")
key = algo_for_0x27(0x01, resp.payload[2:])
uds("27 02" + key.hex())
expect("<67 02>  IDPS")

uds("38 01 0012 2F646174612f696d616765732F622E747874 00 01 05 05")
expect("<78 01 ...>  IDPS")

section("0x36 TransferData -- Positive")
uds("36 01 6262626262 0A")
expect("<76 01>")

section("0x37 RequestTransferExit -- Positive")
uds("37")
expect("<77>  IDPS")

#delete the file
section("0x38 RequestFileTransfer -- Positive")
uds("38 02 0012 2F646174612f696d616765732F622E747874")
expect("<78 02 ...>  IDPS")

section("0x38 RequestFileTransfer -- NRC")

uds("38 09 0014 2f")
expect("<7f 38 31>  IDPS")

section("0x36 TransferData -- NRC")

uds("36")
expect("<7f 36 13>")

section("0x37 RequestTransferExit -- NRC")
uds("37")
expect("<7f 37 24>  IDPS")

# ---------------------------------------------------------------------------
# 0x3D  WriteMemoryByAddress
# ---------------------------------------------------------------------------
section("0x3D WriteMemoryByAddress -- NRC")

uds("3d 12 00 00 10 00 01 00")
expect("<7f 3d 11>  IDPS")

# ---------------------------------------------------------------------------
# 0x3E  TesterPresent
# ---------------------------------------------------------------------------
section("0x3E TesterPresent -- Positive")

uds("10 01")
uds("3e 00")
expect("<7e 00>")

section("0x3E TesterPresent -- NRC")

uds("3e 01")
expect("<7f 3e 12>")

# ===========================================================================
tcpdump_stop()
print()
print("test_IDPS.py completed.")
# ===========================================================================
