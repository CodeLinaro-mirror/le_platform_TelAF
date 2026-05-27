#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

# test_IDPS_vlan.py
#
# VLAN IDPS test script.  Runs the same UDS test sequence as test_IDPS.py
# but over a VLAN DoIP connection.
#
# VLAN connection parameters (fixed):
#   Target address (server) : 0x0201  (same for all VLANs)
#   VLAN 10  : remote_ip=192.168.160.2   source_addr=0x0e10
#   VLAN 110 : remote_ip=192.168.126.2   source_addr=0x0e11
#
# Usage:
#   python3 test_IDPS_vlan.py --vlan 10
#   python3 test_IDPS_vlan.py --vlan 110
#   python3 test_IDPS_vlan.py --vlan both

import os
import sys
import argparse
import datetime
import threading
import subprocess
import signal
import time

import doipclient

# ---------------------------------------------------------------------------
# Import AnyUds from anyuds.py without triggering its main() block.
# ---------------------------------------------------------------------------
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import anyuds as _anyuds_mod

AnyUds  = _anyuds_mod.AnyUds

# ---------------------------------------------------------------------------
# VLAN connection parameters
# ---------------------------------------------------------------------------
DOIP_TARGET_ADDR             = 0x0201   # Fixed server physical address

VLAN10_REMOTE_IP             = "192.168.160.2"
VLAN10_SOURCE_ADDR           = 0x0e10   # Client source address for VLAN 10

VLAN110_REMOTE_IP            = "192.168.126.2"
VLAN110_SOURCE_ADDR          = 0x0e11   # Client source address for VLAN 110

# ---------------------------------------------------------------------------
# tcpdump helpers
# ---------------------------------------------------------------------------
_stop_event     = threading.Event()
_tcpdump_thread = None

def _tcpdump_worker(file_name, stop_event):
    DUMP_IFNAME = os.environ.get("DUMP_IFNAME", "lo")
    time_string = datetime.datetime.now().strftime("-%Y%m%d-%H%M%S")
    command = ["tcpdump", "-i", DUMP_IFNAME, "-w", file_name + time_string + ".pcap"]
    proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    while not stop_event.is_set():
        time.sleep(0.5)
    print(f"killing tcpdump PID: {proc.pid} ...")
    os.kill(proc.pid, signal.SIGTERM)
    time.sleep(1)

def tcpdump_start(file_name="test-IDPS-vlan"):
    global _tcpdump_thread
    if _tcpdump_thread is None:
        _stop_event.clear()
        _tcpdump_thread = threading.Thread(
            target=_tcpdump_worker, args=(file_name, _stop_event))
        _tcpdump_thread.start()
        print("start tcpdump ...")
        time.sleep(1)
    else:
        print("tcpdump is already running ...")

def tcpdump_stop():
    global _tcpdump_thread
    if _tcpdump_thread is not None:
        print("stop tcpdump ...")
        time.sleep(1)
        _stop_event.set()
        _tcpdump_thread.join()
        _tcpdump_thread = None
    else:
        print("tcpdump is NOT running!")

# ---------------------------------------------------------------------------
# DoIP connection factory
# ---------------------------------------------------------------------------
def connect_vlan(vlan_id):
    if vlan_id == 10:
        remote_ip   = VLAN10_REMOTE_IP
        source_addr = VLAN10_SOURCE_ADDR
    elif vlan_id == 110:
        remote_ip   = VLAN110_REMOTE_IP
        source_addr = VLAN110_SOURCE_ADDR
    else:
        raise ValueError(f"Unsupported VLAN ID: {vlan_id}")
    print(f"[VLAN {vlan_id}] DoIP connecting to {remote_ip} "
          f"(target=0x{DOIP_TARGET_ADDR:04X}, source=0x{source_addr:04X}) ...")
    client = doipclient.DoIPClient(remote_ip,
                                   DOIP_TARGET_ADDR,
                                   client_logical_address=source_addr,
                                   auto_reconnect_tcp=True)
    print(f"[VLAN {vlan_id}] DoIP connected.")
    return AnyUds(client)

# ---------------------------------------------------------------------------
# Helpers shared by the test body
# ---------------------------------------------------------------------------
def algo_for_0x27(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

def section(title):
    print()
    print("=" * 60)
    print(f"  {title}")
    print("=" * 60)

def expect(description):
    print(f"  Exp: {description}")

# ---------------------------------------------------------------------------
# Full IDPS test body
# ---------------------------------------------------------------------------
def run_idps_tests(conn, label):
    def uds(req):
        return conn.send_request(req)

    print()
    print("#" * 60)
    print(f"#  IDPS TEST  --  {label}")
    print("#" * 60)

    # -----------------------------------------------------------------------
    # 0x10  DiagnosticSessionControl
    # -----------------------------------------------------------------------
    section(f"[{label}] 0x10 DiagnosticSessionControl -- Positive")

    uds("10 01")
    expect("<50 01 ...>")

    section(f"[{label}] 0x10 DiagnosticSessionControl -- NRC")

    uds("10")
    expect("<7f 10 13>")

    # -----------------------------------------------------------------------
    # 0x11  ECUReset
    # -----------------------------------------------------------------------
    section(f"[{label}] 0x11 ECUReset -- Positive")

    uds("10 03")
    uds("11 02")
    expect("<51 02>  IDPS")

    section(f"[{label}] 0x11 ECUReset -- NRC")

    uds("11 01 02")
    expect("<7f 11 13>  IDPS")

    # -----------------------------------------------------------------------
    # 0x14  ClearDiagnosticInformation
    # -----------------------------------------------------------------------
    section(f"[{label}] 0x14 ClearDiagnosticInformation -- Positive")

    uds("10 03")
    uds("14 FF FF FF")
    expect("<54>")

    section(f"[{label}] 0x14 ClearDiagnosticInformation -- NRC")

    uds("14 AB CD EF")
    expect("<7f 14 31>")

    # -----------------------------------------------------------------------
    # 0x19  ReadDTCInformation
    # -----------------------------------------------------------------------
    section(f"[{label}] 0x19 ReadDTCInformation -- Positive")

    uds("10 03")
    uds("19 01 FF")
    expect("<59 01 ...>")

    section(f"[{label}] 0x19 ReadDTCInformation -- NRC")

    uds("19 99")
    expect("<7f 19 12>")

    # -----------------------------------------------------------------------
    # 0x22  ReadDataByIdentifier
    # -----------------------------------------------------------------------
    section(f"[{label}] 0x22 ReadDataByIdentifier -- Positive")

    uds("10 01")
    uds("22 A5 A5")
    expect("<62 a5 a5 ...>")

    section(f"[{label}] 0x22 ReadDataByIdentifier -- NRC")

    uds("22 FF FF")
    expect("<7f 22 31>")

    # -----------------------------------------------------------------------
    # 0x27  SecurityAccess
    # -----------------------------------------------------------------------
    section(f"[{label}] 0x27 SecurityAccess -- Positive")

    uds("10 02")
    resp = uds("27 01")
    expect("<67 01 <seed>>")

    key = algo_for_0x27(0x01, resp.payload[2:])
    uds("27 02" + key.hex())
    expect("<67 02>  IDPS")

    section(f"[{label}] 0x27 SecurityAccess -- NRC")

    uds("10 02")
    uds("27 09")
    expect("<7f 27 12>  IDPS")

    # -----------------------------------------------------------------------
    # 0x29  Authentication
    # -----------------------------------------------------------------------
    section(f"[{label}] 0x29 Authentication -- Positive")

    uds("10 01")
    uds("29 08")
    expect("<69 08 00>")

    section(f"[{label}] 0x29 Authentication -- NRC")

    uds("29 07 00 00 00")
    expect("<7f 29 12>  IDPS")

    # -----------------------------------------------------------------------
    # 0x2E  WriteDataByIdentifier
    # -----------------------------------------------------------------------
    section(f"[{label}] 0x2E WriteDataByIdentifier -- Positive")

    uds("10 03")
    resp = uds("27 01")
    key = algo_for_0x27(0x01, resp.payload[2:])
    uds("27 02" + key.hex())
    expect("<67 02>  IDPS")

    uds("2E A5 A6 31")
    expect("<6e a5 a6>  IDPS")

    section(f"[{label}] 0x2E WriteDataByIdentifier -- NRC")

    uds("10 01")
    uds("2E A0 A0 31")
    expect("<7f 2e 31>  IDPS")

    # -----------------------------------------------------------------------
    # 0x2F  InputOutputControlByIdentifier
    # -----------------------------------------------------------------------
    section(f"[{label}] 0x2F InputOutputControlByIdentifier -- Positive")

    uds("10 03")
    resp = uds("27 01")
    key = algo_for_0x27(0x01, resp.payload[2:])
    uds("27 02" + key.hex())
    expect("<67 02>  IDPS")

    uds("2F 90 07 03 30")
    expect("<6f 90 07 ...>  IDPS")

    section(f"[{label}] 0x2F InputOutputControlByIdentifier -- NRC")

    uds("2F FF FF 03 30")
    expect("<7f 2f 31>  IDPS")

    # -----------------------------------------------------------------------
    # 0x31  RoutineControl
    # -----------------------------------------------------------------------
    section(f"[{label}] 0x31 RoutineControl -- Positive")

    uds("10 03")
    resp = uds("27 01")
    key = algo_for_0x27(0x01, resp.payload[2:])
    uds("27 02" + key.hex())
    expect("<67 02>  IDPS")

    uds("31 01 02 46 08")
    expect("<71 01 02 46>  IDPS")

    section(f"[{label}] 0x31 RoutineControl -- NRC")

    uds("31 04 02 49")
    expect("<7f 31 12>  IDPS")

    # -----------------------------------------------------------------------
    # 0x38 / 0x36 / 0x37  FileTransfer sequence
    # -----------------------------------------------------------------------
    section(f"[{label}] 0x38 RequestFileTransfer -- Positive")

    uds("10 02")
    resp = uds("27 01")
    key = algo_for_0x27(0x01, resp.payload[2:])
    uds("27 02" + key.hex())
    expect("<67 02>  IDPS")

    uds("38 01 0012 2F646174612f696d616765732F622E747874 00 01 05 05")
    expect("<78 01 ...>  IDPS")

    section(f"[{label}] 0x36 TransferData -- Positive")
    uds("36 01 6262626262 0A")
    expect("<76 01>")

    section(f"[{label}] 0x37 RequestTransferExit -- Positive")
    uds("37")
    expect("<77>  IDPS")

    # delete the file
    section(f"[{label}] 0x38 RequestFileTransfer (delete) -- Positive")
    uds("38 02 0012 2F646174612f696d616765732F622E747874")
    expect("<78 02 ...>  IDPS")

    section(f"[{label}] 0x38 RequestFileTransfer -- NRC")
    uds("38 09 0014 2F")
    expect("<7f 38 31>  IDPS")

    section(f"[{label}] 0x36 TransferData -- NRC")
    uds("36")
    expect("<7f 36 13>")

    section(f"[{label}] 0x37 RequestTransferExit -- NRC")
    uds("37")
    expect("<7f 37 24>  IDPS")

    # -----------------------------------------------------------------------
    # 0x3E  TesterPresent
    # -----------------------------------------------------------------------
    section(f"[{label}] 0x3E TesterPresent -- Positive")

    uds("10 01")
    uds("3e 00")
    expect("<7e 00>")

    section(f"[{label}] 0x3E TesterPresent -- NRC")

    uds("3e 01")
    expect("<7f 3e 12>")

    print()
    print(f"[{label}] test_IDPS_vlan completed.")

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="IDPS VLAN UDS test script.")

    parser.add_argument(
        "--vlan",
        choices=["10", "110", "both"],
        required=True,
        help="VLAN ID to test: 10, 110, or both.")

    args = parser.parse_args()

    tcpdump_start("test-IDPS-vlan")

    try:
        if args.vlan == "10":
            conn = connect_vlan(10)
            run_idps_tests(conn, "VLAN10")

        elif args.vlan == "110":
            conn = connect_vlan(110)
            run_idps_tests(conn, "VLAN110")

        elif args.vlan == "both":
            conn10 = connect_vlan(10)
            run_idps_tests(conn10, "VLAN10")

            conn110 = connect_vlan(110)
            run_idps_tests(conn110, "VLAN110")

    finally:
        tcpdump_stop()

if __name__ == "__main__":
    main()
