# Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
cert =  b"\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\x00"
digest_did = 0xA5A5
digest_did2=0xA5A6
status_mask=0x24
dtc_mask = 0xab0000
rcd_num = 0xFF
data_size = 1
grp_of_dtc = 0xFFFFFF
speedup = 1
digest_data = '5'
update_file = os.environ.get("UPDATE_FILE", "/home/ubuntu/data/update_ubi_ab.zip")
restore_file = os.environ.get("RESTORE_FILE", "/data/images/update_ubi_ab.zip")

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
   0xF401: AsciiCodec(1),
   0xACC8: AsciiCodec(1),
   0xACC9: AsciiCodec(1)
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

doip_remote_ip = os.environ.get("DOIP_REMOTE_IP", "192.168.225.1")
doip_client = DoIPClient(doip_remote_ip, 0x0201)

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

            # Step7: Write digest(WriteDataByIdentifier): 2E xx xx
            response = uds_client.write_data_by_identifier(did=digest_did2, value=digest_data)
            print(response)

            # Step8: Read data(ReadDataByIdentifier): 22 xx xx
            response = uds_client.read_data_by_identifier(didlist=digest_did2)
            values = response.service_data.values

            # Step9: Entering programming session(DiagnosticSessionControl). 10 02
            response = uds_client.change_session(DiagnosticSessionControl.Session.programmingSession)
            print(response)

            # --- Unlock in current session
            response = uds_client.request_seed(0x01)
            seed = response.service_data.seed
            key = dummy_send2key(level=0x01, seed=seed)
            response = uds_client.send_key(0x02, key)
            print(response)

            # Step10.1: Read DTC(reportNumberOfDTCByStatusMask). 19 01
            response = uds_client.get_number_of_dtc_by_status_mask(status_mask)
            print(response)

            # Step10.2: Read DTC(reportDTCByStatusMask). 19 02
            response = uds_client.get_dtc_by_status_mask(status_mask)
            print(response)

            # Step10.3: Read DTC(reportDTCSnapshotIdentification). 19 03
            response = uds_client.get_dtc_snapshot_identification()
            print(response)

            # Step10.4: Read DTC(reportDTCSnapshotRecordByDTCNumber). 19 04
            response = uds_client.get_dtc_snapshot_by_dtc_number(dtc_mask, rcd_num)
            print(response)

            # Step10.5: Read DTC(reportDTCExtDataRecordByDTCNumber). 19 06
            response = uds_client.get_dtc_extended_data_by_dtc_number(dtc_mask, rcd_num, data_size)
            print(response)

            # Step10.4: Read DTC(reportSupportedDTC). 19 0A
            response = uds_client.get_supported_dtc()
            print(response)

            # Step10.5: Read DTC(reportDTCFaultDetectionCounter). 19 14
            response = uds_client.get_dtc_fault_counter()
            print(response)

            # Step11: ClearDiagnosticInformation. 14
            response = uds_client.clear_dtc(grp_of_dtc)
            print(response)

            # Step12: Transmit_certificate. 29 04
            response = uds_client.transmit_certificate(certificate_evaluation_id=0x1122, certificate_data=bytes(8000),)
            print(response)

            #Step13: Verify_certificate_unidirectional. 29 01
            response = uds_client.verify_certificate_unidirectional(communication_configuration=0, certificate_client=cert, challenge_client=b'\xFF\x07\xaa\xbb',)
            print(response)

            #Step14: Proof_of_ownership. 29 03
            response = uds_client.proof_of_ownership(proof_of_ownership_client=bytes(2048))
            print(response)

            #Step15: Authentication configuration. 29 08
            response = uds_client.authentication_configuration()
            print(response)

            # Step16: InputOutputControl--Short Term Adjustment: 2F 90 06 03 xx xx
            ioctrlvalues = {'Led_Ecall': 0x3C}
            response = uds_client.io_control(control_param=3, did=0x9006, values=ioctrlvalues)
            print('dataId:%#x'%response.service_data.did_echo)

            # Step17. InputOutputControl--returnControlToECU: 2F 90 06 00
            response = uds_client.io_control(control_param=0, did=0x9006)
            print(response)

            # Step18: Read data(ReadDataByIdentifier) with authentication check. 22 AC C8
            response = uds_client.read_data_by_identifier(didlist=0xACC8)
            values = response.service_data.values
            print(values)

            # Step19: Write data(WriteDataByIdentifier) with authentication check: 2E xx xx
            response = uds_client.write_data_by_identifier(did=0xACC9, value='9')
            print(response)

            # Step20: Read data(ReadDataByIdentifier) authentication check: 22 xx xx
            response = uds_client.read_data_by_identifier(didlist=digest_did2)
            values = response.service_data.values

            #Step21: Routine control with authentication check. 31 01 02 46 start the routine
            response = uds_client.routine_control(routine_id=0x0246, control_type=0x01, data=b'\x01')
            print(response)

            #Step22: Deauthenticate. 29 00
            response = uds_client.deauthenticate()
            print(response)

            # Step23: Security access #1-Request seed(SecurityAccess). 27 01
            response = uds_client.request_seed(0x01)
            seed = response.service_data.seed
            print("All Zero returned: ", seed)

            with open(update_file, "rb") as f:
                f.seek(0, 2)    # Move to end of file
                eof = f.tell()

                print(eof)
                # Step23.1: RequestFileTransfer(0x38)
                response = uds_client.request_file_transfer(moop=1, path = restore_file, filesize=eof)
                print(response)
                print("Max length: %d" % response.service_data.max_length)
                global bytes_per_pack
                if response.service_data.max_length < bytes_per_pack:
                    bytes_per_pack = response.service_data.max_length
                print("bytes_per_pack=%d" % bytes_per_pack)
                f.seek(0, 0)
                sq = 1
                # Step23.2: Transfer Data(TransferData). 36
                while f.tell() < eof:
                    bs = f.read(bytes_per_pack)
                    #response = uds_client.transfer_data(sq, bs)
                    response = uds_client.transfer_data(sequence_number=sq, data=bs)
                    print(response)
                    sq += 1
                    if sq > 0xff:
                        sq = 0
                f.close()

            # Step24.3: Transter Exit(RequestTransferExit). 37
            response = uds_client.request_transfer_exit()
            print(response)

            # Step25: Switch to extended session(Perform ECU Reset). 10 03
            response = uds_client.change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)
            print(response)

            # Session change, do security access again #1-Request seed(SecurityAccess). 27 01
            response = uds_client.request_seed(0x01)
            seed = response.service_data.seed

            # Calculate key via seed.
            key = dummy_send2key(level=0x01, seed=seed)

            # Security access #2-Send key(SecurityAccess). 27 02
            response = uds_client.send_key(0x02, key)
            print(response)

            #Step23: Verify_certificate_unidirectional. 29 01
            response = uds_client.verify_certificate_unidirectional(communication_configuration=0, certificate_client=bytes(4096), challenge_client=bytes(1024),)
            print(response)

            #Step24: Proof_of_ownership. 29 03
            response = uds_client.proof_of_ownership(proof_of_ownership_client=bytes(2048))
            print(response)

            # Step25: Routine Control RUNDTCTEST(RoutineControl). 31 01 02 47 start to update
            response = uds_client.routine_control(routine_id=0x0247, control_type=0x01, data=b'\x01')
            print(response)

            # Step26: Routine Control RUNDTCTEST(RoutineControl). 31 03 02 47 request update status
            for i in range(100):
                time.sleep(3)
                response = uds_client.routine_control(routine_id=0x0247, control_type=0x03, data=b'\x01')
                print(response)
                print(response.service_data.routine_status_record)
                update_state=response.service_data.routine_status_record
                print(update_state)
                if update_state == b'\x07':
                    break

            print(update_state)

            # Step27: Send tester present to maintain the current session. 3E 00
            def sendPresent():
                uds_client.tester_present()
                print(response)

            for i in range(3):
                tr = threading.Timer(3,sendPresent)
                tr.start()
                tr.join()

            # Step28: Send ECU Reset if the condition is met.
            if update_state == b'\x07':
                response = uds_client.ecu_reset(reset_type=1) # Hard reset
                print(response)

        except NegativeResponseException as e:
            print('Server refused our request for service %s with code "%s" (0x%02x)' % (e.response.service.get_name(), e.response.code_name, e.response.code))
        except (InvalidResponseException, UnexpectedResponseException) as e:
            print('Server sent an invalid payload : %s' % e.response.original_payload)


update_workflow()
