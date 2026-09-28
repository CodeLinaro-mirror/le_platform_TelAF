#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""
Check that the authentication (0x29) protected IO control / DIDs / routine are
rejected with NRC 0x34 (authenticationRequired) before authentication and are
accepted once the client is authenticated.

Test steps:
   1. Switch to programming session, then IOControl 0x9006      -> NRC 0x34
   2. ReadDataByIdentifier 0xACC8                              -> NRC 0x34
   3. WriteDataByIdentifier 0xACC9                              -> NRC 0x34
   4. RoutineControl 0x024A (startRoutine)                      -> NRC 0x34
   5. VerifyCertificateUnidirectional (0x29 0x01)               -> positive
   6. ProofOfOwnership (0x29 0x03)                              -> positive
   7. IOControl 0x9006                                          -> positive
   8. ReadDataByIdentifier 0xACC8                               -> positive
   9. WriteDataByIdentifier 0xACC9                              -> positive
  10. RoutineControl 0x024A (startRoutine)                      -> positive

Configuration these steps rely on (yaml_config/default/Variant):
  * 0x9006 io  : execution_authorization_pattern_io    = pattern_s29 (programming
                 session only), execution_authentication_pattern_io    = [ b1 ]
  * 0xACC8 read: execution_authorization_pattern_read  = pattern_s29,
                 execution_authentication_pattern_read = [ b1 ]
  * 0xACC9 write: execution_authorization_pattern_write = pattern_s29,
                 execution_authentication_pattern_write = [ b1, b2 ]
  * 0x024A     : execution_authorization_pattern = pattern_s29,
                 execution_authentication_pattern = [ b1 ]
  The sample diagApp grants role 0x03 (b0|b1) in the proof of ownership handler,
  and the role list is matched with OR semantics, so b1 unlocks all four.

Environment variables:
  U_REMOTE_IP  DoIP server address (default 192.168.225.1)
  U_PHY_ADDR   DoIP logical address (default 0x0201)

Run:
  python3 test_auth_protected_services.py
"""

import os
import logging
import struct
import unittest

import doipclient
import udsoncan
import udsoncan.configs
from doipclient.connectors import DoIPClientUDSConnector
from udsoncan.client import Client
from udsoncan.services import DiagnosticSessionControl
from udsoncan import DidCodec, AsciiCodec

# logging.basicConfig(level=logging.DEBUG)
logging.basicConfig(level=logging.INFO)

# NRC authenticationRequired
NRC_AUTHENTICATION_REQUIRED = 0x34

IO_CTRL_DID = 0x9006
READ_DID = 0xACC8
WRITE_DID = 0xACC9
ROUTINE_ID = 0x024A

# Short term adjustment value for 0x9006, same value as update_client.py uses.
IO_CTRL_VALUES = {'Led_Ecall': 0x3C}
# Value written to 0xACC9, the DID is 1 byte long.
WRITE_DID_VALUE = '9'
# controlOptionRecord of routine 0x024A startRoutine (Base1 -> Green_Light, 1 byte).
ROUTINE_START_DATA = b'\x01'

# Authentication payloads. The sample diagApp only rejects a communication
# configuration of 0x09, any other value with a non empty certificate passes.
AUTH_COMM_CONF = 0
AUTH_CERT = bytes(4096)
AUTH_CHALLENGE = bytes(1024)
AUTH_POWN = bytes(2048)

# Security level unlocked before the test. Service 0x11 is configured with the
# secured_configuration pattern in drc_services.yaml, and that pattern maps to
# SecAcc_Level_01 (request seed 0x01), so the level has to be unlocked.
SEC_ACCESS_LEVEL = 0x01


class LedEcallCodec(DidCodec):
    """Codec of the 0x9006 IO control data record."""

    def encode(self, Led_Ecall):
        return struct.pack('>B', Led_Ecall)

    def decode(self, payload):
        vals = struct.unpack('>B', payload)
        return {
            'Led_Ecall': vals[0]
        }

    def __len__(self):
        return 1


def build_client_config():
    config = dict(udsoncan.configs.default_client_config)
    config['data_identifiers'] = {
        READ_DID: AsciiCodec(1),
        WRITE_DID: AsciiCodec(1)
    }
    config['input_output'] = {
        IO_CTRL_DID: {
            'codec': LedEcallCodec,
            'mask': {
                'Led_Ecall': 0x80
            },
            'mask_size': 1
        }
    }
    # Negative responses are an expected outcome here, assert on them instead.
    config['exception_on_negative_response'] = False
    return config


def algo_for_0x27(level, seed):
    """Seed to key algorithm implemented by the sample diagApp."""
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)


class DiagClient():
    def __init__(self, server_ip, phy_address):
        self.server_ip = server_ip
        self.phy_address = phy_address

    def do_connect(self):
        self.doip_client = doipclient.DoIPClient(self.server_ip, self.phy_address)
        self.d = self.doip_client
        uds_connection = DoIPClientUDSConnector(self.doip_client)
        self.uds_client = Client(uds_connection, config=build_client_config())
        self.u = self.uds_client
        self.uds_client.open()


class TestAuthProtectedServices(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        server_ip = os.environ.get("U_REMOTE_IP", "192.168.225.1")
        phy_address = os.environ.get("U_PHY_ADDR", None)
        phy_address = 0x0201 if phy_address is None else int(phy_address, 0)
        self.diag = DiagClient(server_ip, phy_address)

    def setUp(self):
        self.diag.do_connect()

    def tearDown(self):
        self.diag.d.close()

    def check_nrc(self, response, expected_nrc, what):
        print("%s -> %s" % (what, response))
        self.assertTrue(response.valid, "%s: invalid response" % what)
        self.assertFalse(response.positive, "%s: expected a negative response" % what)
        self.assertEqual(response.code, expected_nrc,
                         "%s: expected NRC 0x%02X, got 0x%02X"
                         % (what, expected_nrc, response.code))

    def check_positive(self, response, what):
        print("%s -> %s" % (what, response))
        self.assertTrue(response.valid, "%s: invalid response" % what)
        self.assertTrue(response.positive,
                        "%s: expected a positive response" % what)

    def unlock_security_access(self, level):
        """Unlock the given security level in the current session (0x27)."""
        response = self.diag.u.request_seed(level)
        self.check_positive(response, "Security access request seed 0x%02X" % level)
        seed = response.service_data.seed

        key = algo_for_0x27(level=level, seed=seed)
        response = self.diag.u.send_key(level + 1, key)
        self.check_positive(response, "Security access send key 0x%02X" % (level + 1))

    def test_auth_protected_services(self):
        # Step 1: Switch to programming session (0x10 0x02), then send IO control
        # for 0x9006. The DID is authentication protected, so NRC 0x34 is
        # expected. All four requests below run in the programming session, which
        # is the only session the pattern_s29 authorization pattern allows, so the
        # authentication check is the only one that can reject them.
        response = self.diag.u.change_session(
                DiagnosticSessionControl.Session.programmingSession)
        self.check_positive(response, "Step 1a: change to programming session")

        # Service 0x11 is configured with the secured_configuration pattern, which
        # maps to SecAcc_Level_01, so unlock the level here. This does not affect
        # the NRC 0x34 expectations below, because for every service used here the
        # authentication check runs before the security access check.
        self.unlock_security_access(SEC_ACCESS_LEVEL)

        response = self.diag.u.io_control(control_param=3, did=IO_CTRL_DID,
                                         values=IO_CTRL_VALUES)
        self.check_nrc(response, NRC_AUTHENTICATION_REQUIRED,
                       "Step 1b: IOControl 0x%04X before authentication" % IO_CTRL_DID)

        # Step 2: Read data by identifier 0xACC8 (0x22), expect NRC 0x34.
        response = self.diag.u.read_data_by_identifier(didlist=READ_DID)
        self.check_nrc(response, NRC_AUTHENTICATION_REQUIRED,
                       "Step 2: RDBI 0x%04X before authentication" % READ_DID)

        # Step 3: Write data by identifier 0xACC9 (0x2E), expect NRC 0x34.
        response = self.diag.u.write_data_by_identifier(did=WRITE_DID,
                                                       value=WRITE_DID_VALUE)
        self.check_nrc(response, NRC_AUTHENTICATION_REQUIRED,
                       "Step 3: WDBI 0x%04X before authentication" % WRITE_DID)

        # Step 4: Routine control 0x024A startRoutine (0x31 0x01), expect NRC 0x34.
        response = self.diag.u.routine_control(routine_id=ROUTINE_ID, control_type=0x01,
                                              data=ROUTINE_START_DATA)
        self.check_nrc(response, NRC_AUTHENTICATION_REQUIRED,
                       "Step 4: RoutineControl 0x%04X before authentication" % ROUTINE_ID)

        # Step 5: Verify certificate unidirectional (0x29 0x01), expect positive.
        response = self.diag.u.verify_certificate_unidirectional(
                communication_configuration=AUTH_COMM_CONF,
                certificate_client=AUTH_CERT,
                challenge_client=AUTH_CHALLENGE,)
        self.check_positive(response, "Step 5: verify certificate unidirectional")

        # Step 6: Proof of ownership (0x29 0x03), expect positive. The sample
        # diagApp grants role 0x03 here, which covers b1.
        response = self.diag.u.proof_of_ownership(proof_of_ownership_client=AUTH_POWN)
        self.check_positive(response, "Step 6: proof of ownership")

        # The authentication of role b1 times out after 30s, so run the remaining
        # steps back to back without any delay.

        # Step 7: IO control for 0x9006 (0x2F), now expect a positive response.
        response = self.diag.u.io_control(control_param=3, did=IO_CTRL_DID,
                                         values=IO_CTRL_VALUES)
        self.check_positive(response,
                            "Step 7: IOControl 0x%04X after authentication" % IO_CTRL_DID)
        self.assertEqual(response.service_data.did_echo, IO_CTRL_DID)

        # Step 8: Read data by identifier 0xACC8 (0x22), now expect a positive
        # response.
        response = self.diag.u.read_data_by_identifier(didlist=READ_DID)
        self.check_positive(response,
                            "Step 8: RDBI 0x%04X after authentication" % READ_DID)
        print("Step 8: values = %s" % (response.service_data.values,))

        # Step 9: Write data by identifier 0xACC9 (0x2E), now expect a positive
        # response. The write role list is [ b1, b2 ] and is matched with OR
        # semantics, so the granted b1 is enough.
        response = self.diag.u.write_data_by_identifier(did=WRITE_DID,
                                                       value=WRITE_DID_VALUE)
        self.check_positive(response,
                            "Step 9: WDBI 0x%04X after authentication" % WRITE_DID)

        # Step 10: Routine control 0x024A startRoutine (0x31 0x01), now expect a
        # positive response.
        response = self.diag.u.routine_control(routine_id=ROUTINE_ID, control_type=0x01,
                                              data=ROUTINE_START_DATA)
        self.check_positive(response,
                            "Step 10: RoutineControl 0x%04X after authentication"
                            % ROUTINE_ID)

        # Cleanup, hand the 0x9006 IO control back to the ECU (0x2F 0x00).
        response = self.diag.u.io_control(control_param=0, did=IO_CTRL_DID)
        self.check_positive(response,
                            "Cleanup: return control of 0x%04X to ECU" % IO_CTRL_DID)


if __name__ == '__main__':
    unittest.main()
