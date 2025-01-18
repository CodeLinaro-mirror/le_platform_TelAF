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
        self.uds_client = Client(uds_connection, config=config)
        self.u = self.uds_client
        self.uds_client.config['exception_on_negative_response'] = False
        self.uds_client.open()

def algo_for_0x27(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

class Test_SessionSwitchBasedOnPattern(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        sever_ip = os.environ.get("U_REMOTE_IP", "192.168.80.2")
        phy_address = os.environ.get("U_PHY_ADDR", 0x0201)
        self.diag = DiagClient(sever_ip, phy_address)

    def setUp(self, ):
        self.diag.do_connect()

    def tearDown(self, ):
        self.diag.d.close()

    def test_session_switch(self):

        # to 0x01
        response = self.diag.u.change_session(0x01)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)

        # 0x01 -> 0x03
        response = self.diag.u.change_session(0x03)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)

        # 0x03 -> 0x02
        response = self.diag.u.change_session(0x02)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)

        # 0x02 -> 0x03
        response = self.diag.u.change_session(0x03)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)

        # 0x03 [L1 locked] -> 0x52
        response = self.diag.u.change_session(0x52)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertEqual(response.original_payload.hex(), "7f1022")

        # unlock L1 in 0x03 session
        response = self.diag.u.request_seed(0x01)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)

        seed = response.service_data.seed
        key = algo_for_0x27(level=0x01, seed=seed)
        response = self.diag.u.send_key(0x02, key)

        # 0x03 [L1 unlocked] -> 0x52
        response = self.diag.u.change_session(0x52)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)

        # 0x52 -> 0x03
        response = self.diag.u.change_session(0x03)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)

        # 0x03 -> 0x02
        response = self.diag.u.change_session(0x02)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)

        # 0x02 -> 0x60 [blocked]
        response = self.diag.u.change_session(0x60)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertEqual(response.original_payload.hex(), "7f1022")

        # 0x02 -> 0x02 [locked]
        self.diag.u.change_session(0x02)

        # 0x02 [locked] -> 0x60
        response = self.diag.u.change_session(0x60)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertEqual(response.original_payload.hex(), "7f1022")

        # Unlock L61 in 0x02 session
        response = self.diag.u.request_seed(0x61)
        seed = response.service_data.seed
        # FIXME: There is one issue in diagApp to handle the different security level!!
        # This is a workaround for different levels.
        key = algo_for_0x27(level=0x01, seed=seed)
        response = self.diag.u.send_key(0x62, key)

        # 0x02 [unlocked] -> 0x60
        self.diag.u.change_session(0x60)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)
