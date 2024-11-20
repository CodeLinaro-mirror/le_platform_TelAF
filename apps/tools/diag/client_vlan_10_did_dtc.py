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
from udsoncan import DidCodec, AsciiCodec
from udsoncan import IOValues
import udsoncan.configs
import logging
import time
import threading
import struct

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
   0xEF01: AsciiCodec(10),
   0xF401: AsciiCodec(1)
}

class LedEcallCodec(DidCodec):
    def encode(self, Led_Ecall):
        return struct.pack('>B', Led_Ecall)

    def decode(self, payload):
        vals = struct.unpack('>B', payload)
        return {
            'Led_Ecall': vals[0]
        }

    def __len__(self):
        return 1

config["input_output"] = {
    0x9006: {
        'codec': LedEcallCodec,
        'mask': {
            'Led_Ecall': 0x80
        },
        'mask_size': 1
    }
}

config['request_timeout'] = None

doip_remote_ip = os.environ.get("DOIP_REMOTE_IP", "192.168.160.2")
doip_client = DoIPClient(doip_remote_ip, 0x0201, client_logical_address=0x0e10)

uds_connection = DoIPClientUDSConnector(doip_client)

def dummy_send2key(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

def update_workflow():
    with Client(uds_connection, config=config) as uds_client:
        try:
            # Step1: Enter extend session(DiagnosticSessionControl). 10 03
            response = uds_client.change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)
            print(response)

            # Step2: Security access #1-Request seed(SecurityAccess). 27 01
            response = uds_client.request_seed(0x01)
            seed = response.service_data.seed

            # Calculate key via seed.
            key = dummy_send2key(level=0x01, seed=seed)

            # Step3: Security access #2-Send key(SecurityAccess). 27 02
            response = uds_client.send_key(0x02, key)
            print(response)

            # Step4: Read data(ReadDataByIdentifier). 22 F0 11 A5 A5
            response = uds_client.read_data_by_identifier(didlist=[0xF011,0xa5a5])
            values = response.service_data.values
            print(values)

            # Step5: Read data by Id(ReadDataByIdentifier). 22 A5 A5
            response = uds_client.read_data_by_identifier(didlist=digest_did) # Only one
            print(response)

            # Step6: Read public data(ReadDataByIdentifier). 22 F0 11
            response = uds_client.read_data_by_identifier(didlist=0xF011)
            values = response.service_data.values
            print(values)

            def sendPresent():
                uds_client.tester_present()
                print(response)

            for i in range(3):
                tr = threading.Timer(2,sendPresent)
                tr.start()
                tr.join()

            # Step7: InputOutputControl--Short Term Adjustment: 2F 90 06 03 xx xx
            ioctrlvalues = {'Led_Ecall': 0x3C}
            response = uds_client.io_control(control_param=3, did=0x9006, values=ioctrlvalues)
            print('dataId:%#x'%response.service_data.did_echo)

            # Step8. InputOutputControl--returnControlToECU: 2F 90 06 00
            response = uds_client.io_control(control_param=0, did=0x9006)
            print(response)

            for i in range(3):
                tr = threading.Timer(2,sendPresent)
                tr.start()
                tr.join()

            # Step9: Write digest(WriteDataByIdentifier): 2E xx xx
            response = uds_client.write_data_by_identifier(did=digest_did2, value=digest_data)
            print(response)

            # Step10: Read data(ReadDataByIdentifier): 22 xx xx
            response = uds_client.read_data_by_identifier(didlist=digest_did2)
            values = response.service_data.values

            for i in range(3):
                tr = threading.Timer(2,sendPresent)
                tr.start()
                tr.join()

            # Step11: Entering programming session(DiagnosticSessionControl). 10 02
            response = uds_client.change_session(DiagnosticSessionControl.Session.programmingSession)
            print(response)

            # Step12.1: Read DTC(reportNumberOfDTCByStatusMask). 19 01
            response = uds_client.get_number_of_dtc_by_status_mask(status_mask)
            print(response)

            # Step12.2: Read DTC(reportDTCByStatusMask). 19 02
            response = uds_client.get_dtc_by_status_mask(status_mask)
            print(response)

            # Step12.3: Read DTC(reportDTCSnapshotIdentification). 19 03
            response = uds_client.get_dtc_snapshot_identification()
            print(response)

            # Step12.4: Read DTC(reportDTCSnapshotRecordByDTCNumber). 19 04
            response = uds_client.get_dtc_snapshot_by_dtc_number(dtc_mask, rcd_num)
            print(response)

            # Step12.5: Read DTC(reportDTCExtDataRecordByDTCNumber). 19 06
            response = uds_client.get_dtc_extended_data_by_dtc_number(dtc_mask, rcd_num, data_size)
            print(response)

            # Step12.4: Read DTC(reportSupportedDTC). 19 0A
            response = uds_client.get_supported_dtc()
            print(response)

            # Step12.5: Read DTC(reportDTCFaultDetectionCounter). 19 14
            response = uds_client.get_dtc_fault_counter()
            print(response)

            for i in range(3):
                tr = threading.Timer(2,sendPresent)
                tr.start()
                tr.join()

            # Step13: ClearDiagnosticInformation. 14
            response = uds_client.clear_dtc(grp_of_dtc)
            print(response)

            # Step14: Transmit_certificate. 29 04
            response = uds_client.transmit_certificate(certificate_evaluation_id=0x1122, certificate_data=bytes(8000),)
            print(response)

            #Step15: Verify_certificate_unidirectional. 29 01
            response = uds_client.verify_certificate_unidirectional(communication_configuration=0, certificate_client=bytes(4096), challenge_client=bytes(1024),)
            print(response)

            #Step16: Proof_of_ownership. 29 03
            response = uds_client.proof_of_ownership(proof_of_ownership_client=bytes(2048))
            print(response)

            #Step17: Authentication configuration. 29 08
            response = uds_client.authentication_configuration()
            print(response)

            #Step18: Routine control. 31 01 02 46 start the routine
            response = uds_client.routine_control(routine_id=0x0246, control_type=0x01)
            print(response)

            #Step19: Deauthenticate. 29 00
            response = uds_client.deauthenticate()
            print(response)

        except NegativeResponseException as e:
            print('Server refused our request for service %s with code "%s" (0x%02x)' % (e.response.service.get_name(), e.response.code_name, e.response.code))
        except (InvalidResponseException, UnexpectedResponseException) as e:
            print('Server sent an invalid payload : %s' % e.response.original_payload)


update_workflow()
