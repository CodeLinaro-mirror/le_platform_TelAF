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
        config = udsoncan.configs.default_client_config
        # Override the DID configurations
        config['data_identifiers'] = {
            0xA0A0: AsciiCodec(1),
            0xA0A1: AsciiCodec(1),
            0xA0A2: AsciiCodec(1)
        }
        self.uds_client = Client(uds_connection, config=config)
        self.u = self.uds_client
        self.uds_client.config['exception_on_negative_response'] = False
        self.uds_client.open()

SESSION_0x01 = DiagnosticSessionControl.Session.defaultSession
SESSION_0x02 = DiagnosticSessionControl.Session.programmingSession
SESSION_0x03 = DiagnosticSessionControl.Session.extendedDiagnosticSession

def algo_for_0x27(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

class Test_NewFormatDID(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        sever_ip = os.environ.get("U_REMOTE_IP", "192.168.80.2")
        phy_address = os.environ.get("U_PHY_ADDR", 0x0201)
        self.diag = DiagClient(sever_ip, phy_address)

    def setUp(self, ):
        self.diag.do_connect()

    def tearDown(self, ):
        self.diag.d.close()

    def test0001_DID_0xA0A0(self):
        # CFG for Test Case:
        #   0xA0A0:
        #       default session:
        #           R
        #       extended session
        #           R, W[L1]
        #       programming session:
        #           R, W[L1]

        self.diag.u.change_session(SESSION_0x03)
        response = self.diag.u.write_data_by_identifier(did=0xA0A0, value='1')
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertEqual(response.original_payload.hex(), "7f2e33")

        response = self.diag.u.request_seed(0x01)
        seed = response.service_data.seed
        key = algo_for_0x27(level=0x01, seed=seed)
        response = self.diag.u.send_key(0x02, key)

        response = self.diag.u.write_data_by_identifier(did=0xA0A0, value='1')
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
        self.assertEqual(response.original_payload.hex(), "6ea0a0")

        self.diag.u.change_session(SESSION_0x01)
        response = self.diag.u.read_data_by_identifier(didlist=0xA0A0)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
        self.assertEqual(response.data.hex(), "a0a031")

        self.diag.u.change_session(SESSION_0x03)
        response = self.diag.u.read_data_by_identifier(didlist=0xA0A0)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
        self.assertEqual(response.data.hex(), "a0a031")

        self.diag.u.change_session(SESSION_0x02)
        response = self.diag.u.read_data_by_identifier(didlist=0xA0A0)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
        self.assertEqual(response.data.hex(), "a0a031")

        response = self.diag.u.write_data_by_identifier(did=0xA0A0, value='1')
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertEqual(response.original_payload.hex(), "7f2e33")

        response = self.diag.u.request_seed(0x01)
        seed = response.service_data.seed
        key = algo_for_0x27(level=0x01, seed=seed)
        response = self.diag.u.send_key(0x02, key)

        response = self.diag.u.write_data_by_identifier(did=0xA0A0, value='1')
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
        self.assertEqual(response.original_payload.hex(), "6ea0a0")

    def test0002_DID_0xA0A1(self):
        # CFG for Test Case:
        # 0xA0A1:
        #     extended session
        #         R [L1, L61x], W[L61x] <-- L61 can't be accessed in extend_session even if you configured
        #                                   Please check security_binding, so we mark this as L61x !!
        #     programming session:
        #         R [L1, L61], W[L61]

        self.diag.u.change_session(SESSION_0x01)

        response = self.diag.u.read_data_by_identifier(didlist=0xA0A1)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertEqual(response.original_payload.hex(), "7f2231")

        response = self.diag.u.write_data_by_identifier(did=0xA0A1, value='1')
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertEqual(response.original_payload.hex(), "7f2e31")

        self.diag.u.change_session(SESSION_0x03)

        response = self.diag.u.write_data_by_identifier(did=0xA0A1, value='1')
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertEqual(response.original_payload.hex(), "7f2e33")

        response = self.diag.u.read_data_by_identifier(didlist=0xA0A1)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertEqual(response.original_payload.hex(), "7f2233")

        response = self.diag.u.request_seed(0x01)
        seed = response.service_data.seed
        key = algo_for_0x27(level=0x01, seed=seed)
        response = self.diag.u.send_key(0x02, key)

        response = self.diag.u.write_data_by_identifier(did=0xA0A1, value='1')
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertEqual(response.original_payload.hex(), "7f2e33")

        response = self.diag.u.read_data_by_identifier(didlist=0xA0A1)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
        self.assertEqual(response.data.hex(), "a0a131")

        # Try to unlock L61, but failed
        response = self.diag.u.request_seed(0x61)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)

        self.diag.u.change_session(SESSION_0x02)

        response = self.diag.u.request_seed(0x01)
        seed = response.service_data.seed
        key = algo_for_0x27(level=0x01, seed=seed)
        response = self.diag.u.send_key(0x02, key)

        response = self.diag.u.read_data_by_identifier(didlist=0xA0A1)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
        self.assertEqual(response.data.hex(), "a0a131")

        response = self.diag.u.write_data_by_identifier(did=0xA0A1, value='1')
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertEqual(response.original_payload.hex(), "7f2e33")

        response = self.diag.u.request_seed(0x61)
        seed = response.service_data.seed
        # FIXME: There is one issue in diagApp to handle the different security level!!
        # This is a workaround for different levels.
        key = algo_for_0x27(level=0x01, seed=seed)
        response = self.diag.u.send_key(0x62, key)

        response = self.diag.u.write_data_by_identifier(did=0xA0A1, value='1')
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
        self.assertEqual(response.original_payload.hex(), "6ea0a1")

        response = self.diag.u.read_data_by_identifier(didlist=0xA0A1)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
        self.assertEqual(response.data.hex(), "a0a131")

    def test0003_DID_0xA0A2(self):
        # CFG for Test Case:
        # 0xA0A2:
        #     default session:
        #         R, W
        #     extended session
        #         R, W

        self.diag.u.change_session(SESSION_0x01)

        response = self.diag.u.write_data_by_identifier(did=0xA0A2, value='1')
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
        self.assertEqual(response.original_payload.hex(), "6ea0a2")

        response = self.diag.u.read_data_by_identifier(didlist=0xA0A2)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
        self.assertEqual(response.data.hex(), "a0a231")

        self.diag.u.change_session(SESSION_0x03)

        response = self.diag.u.write_data_by_identifier(did=0xA0A2, value='1')
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
        self.assertEqual(response.original_payload.hex(), "6ea0a2")

        response = self.diag.u.read_data_by_identifier(didlist=0xA0A2)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
        self.assertEqual(response.data.hex(), "a0a231")
