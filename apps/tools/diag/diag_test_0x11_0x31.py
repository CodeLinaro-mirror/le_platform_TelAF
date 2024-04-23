# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

from doipclient import DoIPClient
from doipclient.connectors import DoIPClientUDSConnector
from udsoncan.client import Client
from udsoncan.services import *
from udsoncan.exceptions import *
from udsoncan.common import *
import udsoncan
import os
from udsoncan import DidCodec, AsciiCodec, services
import udsoncan.configs
import logging
import time
import threading
import unittest
import sys

#logging.basicConfig(level=logging.DEBUG)

key =  b"\x11\x22\x33\x44"
digest_did = 0xA5A5
digest_did2=0xA5A6
status_mask=0x24
dtc_mask = 0xab0000
rcd_num = 0xFF
data_size = 1
grp_of_dtc = 0xFFFFFF
speedup = 1
digest_data = '5'

bytes_per_pack = 4000

# UDS config information
config = dict(udsoncan.configs.default_client_config)
config['data_identifiers'] = {
   0xF011: AsciiCodec(10),
   0xA5A5: AsciiCodec(2),
   0xA5A6: AsciiCodec(1),
   0xF0D0: AsciiCodec(3),
   0xF0D2: AsciiCodec(6),
   0xEF01: AsciiCodec(5),
   0xF401: AsciiCodec(1)
}

config['request_timeout'] = None

doip_client = DoIPClient("192.168.225.1", 513)

uds_connection = DoIPClientUDSConnector(doip_client)
uds_client = Client(uds_connection, config=config)

def dummy_send2key(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

# Service 0x31
class TestRoutineControl(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        unittest.TestCase.__init__(self, *args, **kwargs)
        uds_client.config['exception_on_negative_response'] = False

    def setUp(self):
        uds_client.change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)

        # Security access #1-Request seed(SecurityAccess). 27 01
        response = uds_client.request_seed(0x01)
        seed = response.service_data.seed

        # Calculate key via seed.
        key = dummy_send2key(level=0x01, seed=seed)

        # Security access #2-Send key(SecurityAccess). 27 02
        response = uds_client.send_key(0x02, key)

    def tearDown(self):
        uds_client.change_session(DiagnosticSessionControl.Session.defaultSession)

    def test_start_routine_success(self):
        response = uds_client.routine_control(routine_id=0x0246, control_type=0x01)
        self.assertTrue(response.positive)
        self.assertEqual(response.service_data.control_type_echo, 0x1)
        self.assertEqual(response.service_data.routine_id_echo, 0x0246)

    def test_stop_routine_success(self):
        response = uds_client.routine_control(routine_id=0x0246, control_type=0x02)
        self.assertTrue(response.positive)
        self.assertEqual(response.service_data.control_type_echo, 0x2)
        self.assertEqual(response.service_data.routine_id_echo, 0x0246)

    def test_get_routine_result_success(self):
        response = uds_client.routine_control(routine_id=0x0247, control_type=0x03)
        self.assertTrue(response.positive)
        self.assertEqual(response.service_data.control_type_echo, 0x3)
        self.assertEqual(response.service_data.routine_id_echo, 0x0247)

    def test_subfunction_not_supported(self):
        response = uds_client.routine_control(routine_id=0x0246, control_type=0x00)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertTrue(issubclass(response.service, services.RoutineControl))
        self.assertEqual(response.code, 0x12)

    def test_subfunction_not_supported(self):
        response = uds_client.routine_control(routine_id=0x0246, control_type=0x04)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertTrue(issubclass(response.service, services.RoutineControl))
        self.assertEqual(response.code, 0x12)

    def test_request_out_of_range(self):
        uds_client.change_session(DiagnosticSessionControl.Session.programmingSession)

        # Session change, do security access again #1-Request seed(SecurityAccess). 27 01
        response = uds_client.request_seed(0x01)
        seed = response.service_data.seed

        # Calculate key via seed.
        key = dummy_send2key(level=0x01, seed=seed)

        # Security access #2-Send key(SecurityAccess). 27 02
        response = uds_client.send_key(0x02, key)

        response = uds_client.routine_control(routine_id=0x0246, control_type=0x01)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertTrue(issubclass(response.service, services.RoutineControl))
        self.assertEqual(response.code, 0x31)

    def test_security_access_deny(self):
        uds_client.change_session(DiagnosticSessionControl.Session.defaultSession)
        uds_client.change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)

        response = uds_client.routine_control(routine_id=0x0246, control_type=0x01)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertTrue(issubclass(response.service, services.RoutineControl))
        self.assertEqual(response.code, 0x33)

    def test_rid_not_config(self):
        response = uds_client.routine_control(routine_id=0xFF11, control_type=0x01)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertTrue(issubclass(response.service, services.RoutineControl))
        self.assertEqual(response.code, 0x31)

    # Customer ask for routine control support in default session
    def test_default_session(self):
        uds_client.change_session(DiagnosticSessionControl.Session.defaultSession)

        response = uds_client.routine_control(routine_id=0x0246, control_type=0x01)
        self.assertTrue(response.positive)
        self.assertEqual(response.service_data.control_type_echo, 0x1)
        self.assertEqual(response.service_data.routine_id_echo, 0x0246)

# Service 0x11
class TestECUReset(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        unittest.TestCase.__init__(self, *args, **kwargs)
        uds_client.config['exception_on_negative_response'] = False

    def setUp(self):
        uds_client.change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)

        # Security access #1-Request seed(SecurityAccess). 27 01
        response = uds_client.request_seed(0x01)
        seed = response.service_data.seed

        # Calculate key via seed.
        key = dummy_send2key(level=0x01, seed=seed)

        # Security access #2-Send key(SecurityAccess). 27 02
        response = uds_client.send_key(0x02, key)

    def tearDown(self):
        uds_client.change_session(DiagnosticSessionControl.Session.defaultSession)

        # Session change, do security access again #1-Request seed(SecurityAccess). 27 01
        response = uds_client.request_seed(0x01)

    '''
    # Hard reset depends on update service. the testcase may fail
    def test_ecu_hard_reset(self):
        response = uds_client.ecu_reset(reset_type=1) # Hard reset
        self.assertTrue(response.positive)
        self.assertEqual(response.service_data.reset_type_echo, 0x1)
    '''
    def test_ecu_key_off_on_reset(self):
        response = uds_client.ecu_reset(reset_type=2) # key off on reset
        self.assertTrue(response.positive)
        self.assertEqual(response.service_data.reset_type_echo, 0x2)

    def test_subfunction_not_supported(self):
        response = uds_client.ecu_reset(reset_type=33)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertTrue(issubclass(response.service, services.ECUReset))
        self.assertEqual(response.code, 0x12)

    def test_subfunction_not_supported_in_active_session(self):
        uds_client.change_session(DiagnosticSessionControl.Session.programmingSession)

        # Session change, do security access again #1-Request seed(SecurityAccess). 27 01
        response = uds_client.request_seed(0x01)
        seed = response.service_data.seed

        # Calculate key via seed.
        key = dummy_send2key(level=0x01, seed=seed)

        # Security access #2-Send key(SecurityAccess). 27 02
        response = uds_client.send_key(0x02, key)

        response = uds_client.ecu_reset(reset_type=1)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertTrue(issubclass(response.service, services.ECUReset))
        self.assertEqual(response.code, 0x7E)

    def test_security_access_deny(self):
        uds_client.change_session(DiagnosticSessionControl.Session.defaultSession)
        uds_client.change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)

        response = uds_client.ecu_reset(reset_type=1)
        self.assertTrue(response.valid)
        self.assertFalse(response.positive)
        self.assertTrue(issubclass(response.service, services.ECUReset))
        self.assertEqual(response.code, 0x33)

    # Customer ask for routine control support in default session
    def test_default_session(self):
        uds_client.change_session(DiagnosticSessionControl.Session.defaultSession)

        response = uds_client.ecu_reset(reset_type=2) # Hard reset
        self.assertTrue(response.positive)
        self.assertEqual(response.service_data.reset_type_echo, 0x2)

if __name__ == "__main__":
    # Run the testcase class methots
    unittest.main()
