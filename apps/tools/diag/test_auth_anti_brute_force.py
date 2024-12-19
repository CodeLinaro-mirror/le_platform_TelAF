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
        self.uds_client.config['exception_on_negative_response'] = False
        self.uds_client.open()

class TestAuthentication(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        sever_ip = os.environ.get("U_REMOTE_IP", "192.168.225.1")
        phy_address = os.environ.get("U_PHY_ADDR", 0x0201)
        self.diag = DiagClient(sever_ip, phy_address)

    def test_certificate_verification_failed(self):
        self.diag.do_connect()
        #send verify_certificate_unidirectional 9 times with invalid certificate(first byte is 0x02)
        for i in range(9):
            response = self.diag.u.verify_certificate_unidirectional(communication_configuration=9, certificate_client=bytes(4096), challenge_client=bytes(1024),)
            print(response)
            self.assertTrue(response.valid)
            self.assertFalse(response.positive)
            #NRC is 0x5C from sample app
            self.assertEqual(response.code, 0x5C)

        #send verify_certificate_unidirectional the 10th time with invalid certificate(first byte is 0x02)
        response = self.diag.u.verify_certificate_unidirectional(communication_configuration=9, certificate_client=bytes(4096), challenge_client=bytes(1024),)
        print(response)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        #NRC should be 0x36
        self.assertEqual(response.code, 0x36)

        response = self.diag.u.verify_certificate_unidirectional(communication_configuration=9, certificate_client=bytes(4096), challenge_client=bytes(1024),)
        print(response)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        #NRC should be 0x37
        self.assertEqual(response.code, 0x37)

        response = self.diag.u.proof_of_ownership(proof_of_ownership_client=bytes(334))
        print(response)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        #NRC should be 0x37
        self.assertEqual(response.code, 0x37)

        response = self.diag.u.authentication_configuration()
        print(response)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        #NRC should be 0x37
        self.assertEqual(response.code, 0x37)

        #Wait authentication delay time out, for the first time, it's 60s
        time.sleep(60)

        #After authentication delay time out, positive resp
        response = self.diag.u.authentication_configuration()
        print(response)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)

        #send verify_certificate_unidirectional 9 times with invalid certificate(first byte is 0x02)
        for i in range(9):
            response = self.diag.u.verify_certificate_unidirectional(communication_configuration=9, certificate_client=bytes(4096), challenge_client=bytes(1024),)
            print(response)
            self.assertTrue(response.valid)
            self.assertFalse(response.positive)
            #NRC is 0x5C from sample app
            self.assertEqual(response.code, 0x5C)

        #send verify_certificate_unidirectional the 10th time with invalid certificate(first byte is 0x02)
        response = self.diag.u.verify_certificate_unidirectional(communication_configuration=9, certificate_client=bytes(4096), challenge_client=bytes(1024),)
        print(response)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        #NRC should be 0x36
        self.assertEqual(response.code, 0x36)

        response = self.diag.u.verify_certificate_unidirectional(communication_configuration=9, certificate_client=bytes(4096), challenge_client=bytes(1024),)
        print(response)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        #NRC should be 0x37
        self.assertEqual(response.code, 0x37)

        response = self.diag.u.proof_of_ownership(proof_of_ownership_client=bytes(334))
        print(response)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        #NRC should be 0x37
        self.assertEqual(response.code, 0x37)

        response = self.diag.u.authentication_configuration()
        print(response)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        #NRC should be 0x37
        self.assertEqual(response.code, 0x37)

        #The second time 120s, still get NRC 0x37 after 60s
        time.sleep(60)

        response = self.diag.u.authentication_configuration()
        print(response)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        #NRC should be 0x37
        self.assertEqual(response.code, 0x37)

        response = self.diag.u.authentication_configuration()
        print(response)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        #NRC should be 0x37
        self.assertEqual(response.code, 0x37)

        #The second timeout value should be 120s, get positive response after another 60s
        time.sleep(60)

        #After authentication delay time out, positive resp
        response = self.diag.u.authentication_configuration()
        print(response)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)

        # verify_certificate_unidirectional -0x01 positive response
        response = self.diag.u.verify_certificate_unidirectional(communication_configuration=0, certificate_client=bytes(4096), challenge_client=bytes(1024),)
        print(response)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)

        # proof_of_ownership -0x03 positive response
        response = self.diag.u.proof_of_ownership(proof_of_ownership_client=b'\x01' +bytes(2048))
        print(response)
        self.assertTrue(response.valid)
        self.assertTrue(response.positive)

        self.diag.d.close()

