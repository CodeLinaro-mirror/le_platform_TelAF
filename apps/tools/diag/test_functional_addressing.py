#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

# test_functional_addressing.py
#
# UDS ReadDataByIdentifier functional addressing test.
#
# Connection parameters (fixed):
#   Target IP                     : 192.168.225.1
#   Physical target address       : 0x0201
#   Functional target address     : 0xE000
#
# The sample app can be configured to return either a negative or a positive
# response for the readable DIDs (0xA5A6, 0xF011).  Select the matching test
# case:
#
#   phy-neg   Physical addressing,   sample app returns negative response
#   phy-pos   Physical addressing,   sample app returns positive response
#   func-neg  Functional addressing, sample app returns negative response
#   func-pos  Functional addressing, sample app returns positive response
#
# Usage:
#   python3 test_functional_addressing.py phy-neg
#   python3 test_functional_addressing.py phy-pos
#   python3 test_functional_addressing.py func-neg
#   python3 test_functional_addressing.py func-pos

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
# Import AnyUds/Request/Response from anyuds.py without triggering its main().
# The module lives in the anyuds/ subdirectory.
# ---------------------------------------------------------------------------
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "anyuds"))
import anyuds as _anyuds_mod

AnyUds   = _anyuds_mod.AnyUds
Request  = _anyuds_mod.Request
Response = _anyuds_mod.Response

# ---------------------------------------------------------------------------
# Connection parameters
# ---------------------------------------------------------------------------
DOIP_REMOTE_IP               = "192.168.225.1"
DOIP_PHYSICAL_TARGET_ADDR    = 0x0201   # Physical (point-to-point) server address
DOIP_FUNCTIONAL_TARGET_ADDR  = 0xE000   # Functional (broadcast) server address

# UDS negative response code constants
NRC_SERVICE_NOT_SUPPORTED    = 0x11
NRC_SUB_FUNCTION_NOT_SUPPORTED = 0x12
NRC_REQ_OUT_OF_RANGE         = 0x31
NRC_CONDITIONS_NOT_CORRECT   = 0x22

# No-response wait time (seconds).  A request that is expected to be ignored
# must not produce anything within this window.
NO_RESPONSE_TIMEOUT          = 2

# ---------------------------------------------------------------------------
# tcpdump helpers
# ---------------------------------------------------------------------------
_stop_event     = threading.Event()
_tcpdump_thread = None

def _tcpdump_worker(file_name, stop_event):
    DUMP_IFNAME = os.environ.get("DUMP_IFNAME", "eth0")
    time_string = datetime.datetime.now().strftime("-%Y%m%d-%H%M%S")
    command = ["tcpdump", "-i", DUMP_IFNAME, "-w", file_name + time_string + ".pcap"]
    proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    while not stop_event.is_set():
        time.sleep(0.5)
    print(f"killing tcpdump PID: {proc.pid} ...")
    os.kill(proc.pid, signal.SIGTERM)
    time.sleep(1)

def tcpdump_start(file_name="test-functional-addr"):
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
def connect(target_addr):
    print(f"DoIP connecting to {DOIP_REMOTE_IP} (target=0x{target_addr:04X}) ...")
    client = doipclient.DoIPClient(DOIP_REMOTE_IP,
                                   target_addr,
                                   auto_reconnect_tcp=True)
    print("DoIP connected.")
    return AnyUds(client)

# ---------------------------------------------------------------------------
# Helpers shared by the test body
# ---------------------------------------------------------------------------
def section(title):
    print()
    print("=" * 60)
    print(f"  {title}")
    print("=" * 60)

def expect(description):
    print(f"  Exp: {description}")

def _send_and_recv(conn, req):
    """Low-level send that returns the Response, or None on timeout.

    Unlike AnyUds.send_request(), this never raises on a timeout or on an NRC,
    so the caller can assert the exact outcome (positive / NRC / no response).
    NRC 0x78 (responsePending) is transparently awaited.
    """
    request = Request.from_string(req)
    conn.empty_rxq()
    print(f"SEND --> [{request.beautify}]")
    conn.send_uds(request.payload)

    while True:
        payload = conn.recv_uds(timeout=NO_RESPONSE_TIMEOUT, exception=False)
        if payload is None:
            return None
        response = Response.from_payload(payload)
        print(f"RECV <-- [{response.beautify}]")
        # Keep waiting while the server signals responsePending.
        if (not response.positive) and response.code == 0x78:
            continue
        return response

def expect_positive(conn, req):
    """The request must produce a positive response (SID + 0x40)."""
    request  = Request.from_string(req)
    response = _send_and_recv(conn, req)
    if response is None:
        raise AssertionError(f"Expected positive response to [{request.beautify}], "
                             f"got <no response>")
    if not response.positive:
        raise AssertionError(f"Expected positive response to [{request.beautify}], "
                             f"got NRC 0x{response.code:02X}")
    if response.response_id != request.response_id:
        raise AssertionError(f"Response SID 0x{response.response_id:02X} does not match "
                             f"request 0x{request.response_id:02X}")
    return response

def expect_nrc(conn, req, expected_code):
    """The request must produce a specific negative response code."""
    request  = Request.from_string(req)
    response = _send_and_recv(conn, req)
    if response is None:
        raise AssertionError(f"Expected NRC 0x{expected_code:02X} to [{request.beautify}], "
                             f"got <no response>")
    if response.positive:
        raise AssertionError(f"Expected NRC 0x{expected_code:02X} to [{request.beautify}], "
                             f"got positive response")
    if response.code != expected_code:
        raise AssertionError(f"Expected NRC 0x{expected_code:02X} to [{request.beautify}], "
                             f"got NRC 0x{response.code:02X}")
    return response

def expect_no_response(conn, req):
    """The request must produce NO response within NO_RESPONSE_TIMEOUT."""
    request  = Request.from_string(req)
    response = _send_and_recv(conn, req)
    if response is not None:
        raise AssertionError(f"Expected <no response> to [{request.beautify}], "
                             f"got [{response.beautify}]")
    print("RECV <-- [<no response>]  (as expected)")

# ---------------------------------------------------------------------------
# Physical addressing -- sample app returns NEGATIVE response  (req #3)
# ---------------------------------------------------------------------------
def run_phy_neg(conn, label):
    section(f"[{label}] A. DiagnosticSessionControl 10 03 -- Positive")
    expect_positive(conn, "10 03")
    expect("<50 03 ...>")

    section(f"[{label}] B. DiagnosticSessionControl 10 01 -- Positive")
    expect_positive(conn, "10 01")
    expect("<50 01 ...>")

    section(f"[{label}] C. ReadDataByIdentifier F000 -- NRC 0x31")
    expect_nrc(conn, "22 F0 00", NRC_REQ_OUT_OF_RANGE)
    expect("<7F 22 31>  (DID 0xF000 not supported)")

    section(f"[{label}] D. ReadDataByIdentifier A5A6 -- NRC 0x22")
    expect_nrc(conn, "22 A5 A6", NRC_CONDITIONS_NOT_CORRECT)
    expect("<7F 22 22>  (sample app returns NRC 0x22)")

    section(f"[{label}] E. ReadDataByIdentifier F011 -- NRC 0x22")
    expect_nrc(conn, "22 F0 11", NRC_CONDITIONS_NOT_CORRECT)
    expect("<7F 22 22>  (sample app returns NRC 0x22)")

    section(f"[{label}] F. ReadDataByIdentifier F000 + F011 -- NRC 0x22")
    expect_nrc(conn, "22 F0 00 F0 11", NRC_CONDITIONS_NOT_CORRECT)
    expect("<7F 22 22>")

    section(f"[{label}] G. ReadDataByIdentifier A5A6 + F011 -- NRC 0x22")
    expect_nrc(conn, "22 A5 A6 F0 11", NRC_CONDITIONS_NOT_CORRECT)
    expect("<7F 22 22>")

    section(f"[{label}] H. ReadDataByIdentifier F000 + F001 -- NRC 0x31")
    expect_nrc(conn, "22 F0 00 F0 01", NRC_REQ_OUT_OF_RANGE)
    expect("<7F 22 31>  (both DIDs not supported)")

# ---------------------------------------------------------------------------
# Physical addressing -- sample app returns POSITIVE response  (req #4)
# ---------------------------------------------------------------------------
def run_phy_pos(conn, label):
    section(f"[{label}] A. DiagnosticSessionControl 10 03 -- Positive")
    expect_positive(conn, "10 03")
    expect("<50 03 ...>")

    section(f"[{label}] B. DiagnosticSessionControl 10 01 -- Positive")
    expect_positive(conn, "10 01")
    expect("<50 01 ...>")

    section(f"[{label}] C. ReadDataByIdentifier F000 -- NRC 0x31")
    expect_nrc(conn, "22 F0 00", NRC_REQ_OUT_OF_RANGE)
    expect("<7F 22 31>  (DID 0xF000 not supported)")

    section(f"[{label}] D. ReadDataByIdentifier A5A6 -- Positive")
    expect_positive(conn, "22 A5 A6")
    expect("<62 A5 A6 ...>")

    section(f"[{label}] E. ReadDataByIdentifier F011 -- Positive")
    expect_positive(conn, "22 F0 11")
    expect("<62 F0 11 ...>")

    section(f"[{label}] F. ReadDataByIdentifier F000 + F011 -- Positive for F011")
    resp = expect_positive(conn, "22 F0 00 F0 11")
    expect("<62 F0 11 ...>  (only F011 present)")

    section(f"[{label}] G. ReadDataByIdentifier A5A6 + F011 -- Positive for A5A6 and F011")
    resp = expect_positive(conn, "22 A5 A6 F0 11")
    expect("<62 A5 A6 ... F0 11 ...>  (both present)")

# ---------------------------------------------------------------------------
# Functional addressing -- sample app returns NEGATIVE response  (req #5)
# ---------------------------------------------------------------------------
def run_func_neg(conn, label):
    section(f"[{label}] A. DiagnosticSessionControl 10 03 -- No response")
    expect_no_response(conn, "10 03")
    expect("<no response>")

    section(f"[{label}] B. ECUReset 11 02 -- No response")
    expect_no_response(conn, "11 02")
    expect("<no response>")

    section(f"[{label}] C. ReadDataByIdentifier F000 -- No response")
    expect_no_response(conn, "22 F0 00")
    expect("<no response>  (DID 0xF000 not supported)")

    section(f"[{label}] D. ReadDataByIdentifier F011 -- NRC 0x22")
    expect_nrc(conn, "22 F0 11", NRC_CONDITIONS_NOT_CORRECT)
    expect("<7F 22 22>  (sample app returns NRC 0x22)")

    section(f"[{label}] E. ReadDataByIdentifier A5A6 -- No response")
    expect_no_response(conn, "22 A5 A6")
    expect("<no response>  (DID 0xA5A6 does not support functional address)")

    section(f"[{label}] F. ReadDataByIdentifier F000 + F011 -- NRC 0x22")
    expect_nrc(conn, "22 F0 00 F0 11", NRC_CONDITIONS_NOT_CORRECT)
    expect("<7F 22 22>")

    section(f"[{label}] G. ReadDataByIdentifier A5A6 + F011 -- NRC 0x22")
    expect_nrc(conn, "22 A5 A6 F0 11", NRC_CONDITIONS_NOT_CORRECT)
    expect("<7F 22 22>")

    section(f"[{label}] H. ReadDataByIdentifier F000 + F001 -- No response")
    expect_no_response(conn, "22 F0 00 F0 01")
    expect("<no response>  (both DIDs not supported)")

# ---------------------------------------------------------------------------
# Functional addressing -- sample app returns POSITIVE response  (req #6)
# ---------------------------------------------------------------------------
def run_func_pos(conn, label):
    section(f"[{label}] A. ReadDataByIdentifier F000 -- No response")
    expect_no_response(conn, "22 F0 00")
    expect("<no response>  (DID 0xF000 not supported)")

    section(f"[{label}] B. ReadDataByIdentifier A5A6 -- No response")
    expect_no_response(conn, "22 A5 A6")
    expect("<no response>  (DID 0xA5A6 does not support functional address)")

    section(f"[{label}] C. ReadDataByIdentifier F011 -- Positive")
    expect_positive(conn, "22 F0 11")
    expect("<62 F0 11 ...>")

    section(f"[{label}] D. ReadDataByIdentifier F000 + F011 -- Positive for F011")
    expect_positive(conn, "22 F0 00 F0 11")
    expect("<62 F0 11 ...>  (only F011 present)")

    section(f"[{label}] E. ReadDataByIdentifier A5A6 + F011 -- Positive for F011")
    expect_positive(conn, "22 A5 A6 F0 11")
    expect("<62 F0 11 ...>  (only F011 present)")

# ---------------------------------------------------------------------------
# Test-case registry
# ---------------------------------------------------------------------------
TEST_CASES = {
    "phy-neg":  (DOIP_PHYSICAL_TARGET_ADDR,   run_phy_neg,
                 "PHYSICAL ADDR / sample app NEGATIVE"),
    "phy-pos":  (DOIP_PHYSICAL_TARGET_ADDR,   run_phy_pos,
                 "PHYSICAL ADDR / sample app POSITIVE"),
    "func-neg": (DOIP_FUNCTIONAL_TARGET_ADDR, run_func_neg,
                 "FUNCTIONAL ADDR / sample app NEGATIVE"),
    "func-pos": (DOIP_FUNCTIONAL_TARGET_ADDR, run_func_pos,
                 "FUNCTIONAL ADDR / sample app POSITIVE"),
}

def run_test_case(name):
    target_addr, runner, label = TEST_CASES[name]
    print()
    print("#" * 60)
    print(f"#  {label}  --  ({name})")
    print("#" * 60)
    conn = connect(target_addr)
    runner(conn, label)
    print()
    print(f"[{label}] completed.")

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="UDS ReadDataByIdentifier addressing test (target 192.168.225.1).")

    parser.add_argument(
        "test",
        choices=list(TEST_CASES.keys()),
        help="Test case to run: phy-neg, phy-pos, func-neg, func-pos. "
             "The sample app must be configured for the matching "
             "negative/positive response behavior beforehand.")

    args = parser.parse_args()

    tcpdump_start("test-functional-addr")

    try:
        run_test_case(args.test)
    finally:
        tcpdump_stop()

if __name__ == "__main__":
    main()
