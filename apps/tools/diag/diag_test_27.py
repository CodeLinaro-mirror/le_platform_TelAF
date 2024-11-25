#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import os, sys, re
import logging
import unittest
import time

import doipclient
import udsoncan
from doipclient.connectors import DoIPClientUDSConnector
from udsoncan.services import DiagnosticSessionControl
from udsoncan.client import Client
from udsoncan import DidCodec, AsciiCodec

# logging.basicConfig(level=logging.DEBUG)
logging.basicConfig(level=logging.INFO)

class DiagClient():
    def __init__(self, server_ip, phy_address):
        self.server_ip = server_ip
        self.phy_address = phy_address

    def do_connect(self):
        self.doip_client = doipclient.DoIPClient(self.server_ip, self.phy_address)
        self.d = self.doip_client
        uds_connection = DoIPClientUDSConnector(self.doip_client)
        config = dict(udsoncan.configs.default_client_config)
        self.uds_client = Client(uds_connection, config=config)
        self.u = self.uds_client
        self.uds_client.config['data_identifiers'] = { 0xA5A5: AsciiCodec(2) }
        self.uds_client.config['exception_on_negative_response'] = False
        self.uds_client.open()

SESSION_0x01 = DiagnosticSessionControl.Session.defaultSession
SESSION_0x02 = DiagnosticSessionControl.Session.programmingSession
SESSION_0x03 = DiagnosticSessionControl.Session.extendedDiagnosticSession


def dummy_send2key(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

class Test_SecurityAccessService(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        sever_ip = os.environ.get("U_REMOTE_IP", "192.168.80.2")
        phy_address = os.environ.get("U_PHY_ADDR", 0x0201)
        self.diag = DiagClient(sever_ip, phy_address)

    def test0001_default_session(self):
        self.diag.do_connect()
        # By default the session is default_session
        response = self.diag.u.request_seed(0x01)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        # Checking SID supported in active session -> NRC 7F
        self.assertEqual(response.original_payload.hex(), "7f277f")
        self.diag.d.close()

    def test0002_sunfunction_out_of_range(self):
        self.diag.do_connect()
        # To one active session
        self.diag.u.change_session(SESSION_0x03)
        # check this in general stage, so don't care the session
        response = self.diag.u.request_seed(0x43) # It's reserved
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertEqual(response.original_payload.hex(), "7f2712")
        self.diag.d.close()

    def test003_anti_brute_force(self):
        self.diag.do_connect()
        self.diag.u.change_session(SESSION_0x03)

        for i in range(1, 15):
            response = self.diag.u.request_seed(0x01)
            self.assertTrue(response.valid)
            if response.positive is True:
                print(i)
                self.assertTrue(i < 11)

                # Invalid Key
                response = self.diag.u.send_key(0x02, b'\x00\x00')

                self.assertTrue(response.valid)
                self.assertFalse(response.positive)
                if i == 10:
                    self.assertEqual(response.original_payload.hex(), "7f2736")
                else:
                    self.assertEqual(response.original_payload.hex(), "7f2735")
            else: # negative
                self.assertTrue(i >= 11)
                self.assertEqual(response.original_payload.hex(), "7f2737")

        for i in range(16):
            time.sleep(1) # Waiting for timeout
            self.diag.u.tester_present() # Keep current session to be alive

        response = self.diag.u.request_seed(0x01)
        seed = response.service_data.seed
        # Valid Key
        key = dummy_send2key(level=0x01, seed=seed)
        response = self.diag.u.send_key(0x02, key)

        self.diag.d.close()

    def test004_anti_brute_force_reboot(self):
        self.diag.do_connect()
        self.diag.u.change_session(SESSION_0x03)

        for i in range(1, 15):
            response = self.diag.u.request_seed(0x01)
            self.assertTrue(response.valid)
            if response.positive is True:
                print(i)
                self.assertTrue(i < 11)

                # Invalid Key
                response = self.diag.u.send_key(0x02, b'\x00\x00')

                self.assertTrue(response.valid)
                self.assertFalse(response.positive)
                if i == 10:
                    self.assertEqual(response.original_payload.hex(), "7f2736")
                else:
                    self.assertEqual(response.original_payload.hex(), "7f2735")
            else: # negative
                self.assertTrue(i >= 11)
                self.assertEqual(response.original_payload.hex(), "7f2737")

        self.diag.d.close()
        os.system("app stop tafDiagApp")

        # Simulate 'reboot' action
        os.system("telaf restart")

        print("waiting for [tafDiagSvc] bring-up ...")
        time.sleep(3)

        os.system("app start tafDiagApp")
        time.sleep(1)

        self.diag.do_connect()
        self.diag.u.change_session(SESSION_0x03)

        response = self.diag.u.request_seed(0x01)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertEqual(response.original_payload.hex(), "7f2737")

        for i in range(16):
            time.sleep(1) # Waiting for timeout
            self.diag.u.tester_present() # Keep current session to be alive

        response = self.diag.u.request_seed(0x01)
        seed = response.service_data.seed
        # Valid Key
        key = dummy_send2key(level=0x01, seed=seed)
        response = self.diag.u.send_key(0x02, key)

        self.diag.d.close()

    def test005_RDBI_when_locked_and_unlocked(self):
        # First:
        #  config set diag/DID/a5a5/value/data1 1 int
        #  config set diag/DID/a5a5/value/data2 2 int
        #
        self.diag.do_connect()
        self.diag.u.change_session(SESSION_0x03)
        response = self.diag.u.read_data_by_identifier(didlist=0xA5A5)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertEqual(response.original_payload.hex(), "7f2233")

        response = self.diag.u.request_seed(0x01)
        seed = response.service_data.seed
        # Valid Key
        key = dummy_send2key(level=0x01, seed=seed)
        response = self.diag.u.send_key(0x02, key)

        response = self.diag.u.read_data_by_identifier(didlist=0xA5A5)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
        self.assertEqual(response.original_payload.hex(), "62a5a50102")

        self.diag.d.close()
