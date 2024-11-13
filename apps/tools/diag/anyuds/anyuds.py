#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import os, sys, re
import logging
import doipclient
import code
import threading
import subprocess
import signal
import datetime
import time

class CommonOperation:
    @property
    def beautify(self):
        msg = " ".join([ hex(bb)[2:].zfill(2) for bb in self.payload ])
        return msg

class Request(CommonOperation):
    def __init__(self):
        self.sid = None
        self.response_id = None
        self.original_string = None
        self.payload = None

    @classmethod
    def from_string(cls, req_string):
        assert type(req_string) is str

        no_spaces = "".join(req_string.split())
        request_payload = bytes.fromhex(no_spaces)

        assert len(request_payload) != 0

        request = cls()
        request.original_string = req_string
        request.payload = request_payload
        request.sid = request.payload[0]
        request.response_id = request.sid + 0x40

        return request

class Response(CommonOperation):
    def __init__(self):
        self.positive = False
        self.code = None
        self.valid = False
        self.invalid_reason = "NOT-INIT"
        self.payload = None
        self.response_id = None

    @classmethod
    def from_payload(cls, payload):

        response = cls()
        response.payload = payload

        if len(payload) < 1:
            response.valid = False
            response.invalid_reason = "EMPTY-PAYLOAD"
            return response

        if payload[0] != 0x7F:  # Positive
            response.positive = True
            response.code = 0x00
            response.response_id = payload[0]

        else:  # Negative response
            response.positive = False

            if len(payload) < 3:
                response.valid = False
                response.invalid_reason = "Bad N-ACK (len < 3)"
                return response

            response.code = int(payload[2])

        response.valid = True
        response.invalid_reason = ""

        return response



class AnyUds(object):

    def __init__(self, doip_transport):
        self.doip = doip_transport

    def send_uds(self, payload):
        self.doip.send_diagnostic(bytearray(payload))

    def recv_uds(self, timeout=2, exception=False):
        try:
            frame = self.wait_frame(timeout=timeout)
        except Exception as e:
            if exception == True:
                raise
            else:
                frame = None
        return frame

    def wait_frame(self, timeout = 2):
        return bytes(self.doip.receive_diagnostic(timeout=timeout))

    def empty_rxq(self):
        self.doip.empty_rxqueue()

    def send_request(self, req_payload_string, timeout = -1):

        request = Request.from_string(req_payload_string)

        self.empty_rxq()

        print(f"SEND --> [{request.beautify}]")
        self.send_uds(request.payload)

        done_receiving = False
        while not done_receiving:
            done_receiving = True
            try:
                payload = self.recv_uds(timeout=2, exception=True)
            except Exception as e: # Some exceptions from the under-layer (Such as: DoIP)
                raise e

            response = Response.from_payload(payload)
            print(f"RECV <-- [{response.beautify}]")

            if not response.valid:
                raise Exception(f"Invalid response received, reason: {response.invalid_reason}")

            assert response.code is not None

            if not response.positive:

                if response.code == 0x78:
                    done_receiving = False
                else:
                    pass # Nothing to do for Nack
            else:
                if response.response_id != request.response_id:
                    print(f"Err: {request.response_id}(REQ) != {response.response_id}(RESP)")

        return response

def init():
    DOIP_REMOTE_IP = os.environ.get("DOIP_REMOTE_IP", "192.168.80.2")
    DOIP_PHY_ADDR  = os.environ.get("DOIP_PHY_ADDR", 0x0201)
    print("DoIP connecting ...")
    doip_client = doipclient.DoIPClient(DOIP_REMOTE_IP,
                                        DOIP_PHY_ADDR,
                                        auto_reconnect_tcp=True)
    print("DoIP connected.")

    anyuds = AnyUds(doip_client)
    return anyuds

def tcpdump_worker(file_name, stop_event):
    DUMP_IFNAME = os.environ.get("DUMP_IFNAME", "lo")
    time_string = datetime.datetime.now().strftime("-%Y%m%d-%H%M%S")
    command = ["tcpdump", "-i", DUMP_IFNAME, "-w",  file_name + time_string + ".pcap"]
    tcpdump_proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    while not stop_event.is_set():
        time.sleep(0.5)

    print(f"killing tcpdump PID: {tcpdump_proc.pid} ...")
    os.kill(tcpdump_proc.pid, signal.SIGTERM)
    time.sleep(1)

stop_event = threading.Event()

tcpdump_thread = None
def tcpdump_start(file_name="anytest-result"):
    global tcpdump_thread
    if tcpdump_thread is None:
        stop_event.clear()
        tcpdump_thread = threading.Thread(target=tcpdump_worker, args=(file_name,stop_event))
        # tcpdump_thread.daemon = True
        tcpdump_thread.start()
        print(f"start tcpdump ...")
        time.sleep(1)
    else:
        print(f"tcpdump is running ...")

def tcpdump_stop():
    global tcpdump_thread
    if tcpdump_thread is not None:
        print(f"stop tcpdump ...")
        time.sleep(1)
        stop_event.set()
        tcpdump_thread.join()
        tcpdump_thread = None
    else:
        print(f"tcpdump is NOT running!")

anyuds = None
def uds(uds_req_string):
    global anyuds
    if anyuds is None:
        anyuds = init()
    return anyuds.send_request(uds_req_string)

def main():
    sys.ps1 = "(any uds)  > "
    sys.ps2 = "(any uds) .. "

    def execute_script(file_path):
        with open(file_path, 'r') as file:
            script = file.read()
        code = compile(script, file_path, 'exec')
        exec(code, globals())

    if len(sys.argv) > 1:
        script_file = sys.argv[1] # Only handle the first arg
        execute_script(script_file)
    else:
        variables = globals().copy()
        code.interact(local=variables)

if __name__ == "__main__": main()
