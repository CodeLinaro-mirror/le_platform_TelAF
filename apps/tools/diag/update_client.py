 # Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted (subject to the limitations in the
 # disclaimer below) provided that the following conditions are met:
 #
 #     * Redistributions of source code must retain the above copyright
 #       notice, this list of conditions and the following disclaimer.
 #
 #     * Redistributions in binary form must reproduce the above
 #       copyright notice, this list of conditions and the following
 #       disclaimer in the documentation and/or other materials provided
 #       with the distribution.
 #
 #     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 #       contributors may be used to endorse or promote products derived
 #       from this software without specific prior written permission.
 #
 # NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 # GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 # HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 # WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 # MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 # IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 # ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 # DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 # GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 # INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 # IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 # OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 # IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from doipclient import DoIPClient
from doipclient.connectors import DoIPClientUDSConnector
from udsoncan.client import Client
from udsoncan.services import *
from udsoncan.exceptions import *
from udsoncan.common import *
import udsoncan
import os
from udsoncan import DidCodec, AsciiCodec
import udsoncan.configs
import logging
import time
import threading

#logging.basicConfig(level=logging.DEBUG)

key =  b"\x11\x22\x33\x44"
digest_did = 0xA5A5
digest_did2=0xA5A6
status_mask=0x24
speedup = 1
digest_data = '5'
update_file = "/home/ubuntu/data/update_ubi_ab.zip"
restore_file = "/data/images/update_ubi_ab.zip"

bytes_per_pack = 4000

# UDS config information
config = dict(udsoncan.configs.default_client_config)
config['data_identifiers'] = {
   0xF011: AsciiCodec(10),
   0xA5A5: AsciiCodec(2),
   0xA5A6: AsciiCodec(1)
}

doip_client = DoIPClient("192.168.225.1", 513)

power_mode = doip_client.request_diagnostic_power_mode()
print(power_mode)

uds_connection = DoIPClientUDSConnector(doip_client)

alivecheck = doip_client.request_alive_check()
print(alivecheck)

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

            # Step2: Read data(ReadDataByIdentifier). 22 F0 11 A5 A5
            response = uds_client.read_data_by_identifier(didlist=[0xF011,0xa5a5])
            values = response.service_data.values
            print(values)

            # Step3: Entering programming session(DiagnosticSessionControl). 10 02
            response = uds_client.change_session(DiagnosticSessionControl.Session.programmingSession)
            print(response)

            # Step4: Read DTC(ReadDTCInformation). 19 02
            response = uds_client.get_dtc_by_status_mask(status_mask)
            print(response)

            # Step5: Read data by Id(ReadDataByIdentifier). 22 A5 A5
            response = uds_client.read_data_by_identifier(didlist=digest_did) # Only one
            print(response)

            # Step6: Read public data(ReadDataByIdentifier). 22 F0 11
            response = uds_client.read_data_by_identifier(didlist=0xF011)
            values = response.service_data.values
            print(values)

            # Step7: Security access #1-Request seed(SecurityAccess). 27 01
            response = uds_client.request_seed(0x01)
            seed = response.service_data.seed

            # Calculate key via seed.
            key = dummy_send2key(level=0x01, seed=seed)

            # Step8: Security access #2-Send key(SecurityAccess). 27 02
            response = uds_client.send_key(0x02, key)
            print(response)

            # Step9: Write digest(WriteDataByIdentifier): 2E xx xx
            response = uds_client.write_data_by_identifier(did=digest_did2, value=digest_data)
            print(response)

            # Step10: Read data(ReadDataByIdentifier): 22 xx xx
            response = uds_client.read_data_by_identifier(didlist=digest_did2)
            values = response.service_data.values

            with open(update_file, "rb") as f:
                f.seek(0, 2)    # Move to end of file
                eof = f.tell()

                print(eof)
                # Step11: RequestFileTransfer(0x38)
                response = uds_client.request_file_transfer(moop=1, path = restore_file, filesize=eof)
                print(response)
                print("Max length: %d" % response.service_data.max_length)
                global bytes_per_pack
                if response.service_data.max_length < bytes_per_pack:
                    bytes_per_pack = response.service_data.max_length
                print("bytes_per_pack=%d" % bytes_per_pack)
                f.seek(0, 0)
                sq = 1
                # Step12: Transfer Data(TransferData). 36
                while f.tell() < eof:
                    bs = f.read(bytes_per_pack)
                    #response = uds_client.transfer_data(sq, bs)
                    response = uds_client.transfer_data(sequence_number=sq, data=bs)
                    print(response)
                    sq += 1
                    if sq > 0xff:
                        sq = 0
                f.close()

            # Step13: Transter Exit(RequestTransferExit). 37
            response = uds_client.request_transfer_exit()
            print(response)

            # Step14: Routine Control RUNDTCTEST(RoutineControl). 31 01 02 47 start to update
            response = uds_client.routine_control(routine_id=0x0247, control_type=0x01)
            print(response)

            # Step15: Routine Control RUNDTCTEST(RoutineControl). 31 03 02 47 request update status
            for i in range(100):
                time.sleep(3)
                response = uds_client.routine_control(routine_id=0x0247, control_type=0x03)
                print(response)
                print(response.service_data.routine_status_record)
                update_state=response.service_data.routine_status_record
                print(update_state)
                if update_state == b'\x06':
                    break

            print(update_state)

            # Step16: Switch to extended session(Perform ECU Reset). 10 03
            response = uds_client.change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)
            print(response)

            # Step17: Send tester present to maintain the current session. 3E 00
            def sendPresent():
                uds_client.tester_present()
                print(response)

            for i in range(3):
                tr = threading.Timer(3,sendPresent)
                tr.start()
                tr.join()

            # Step18: Send ECU Reset if the condition is met.
            if update_state == b'\x06':
                response = uds_client.ecu_reset(reset_type=1) # Hard reset
                print(response)

        except NegativeResponseException as e:
            print('Server refused our request for service %s with code "%s" (0x%02x)' % (e.response.service.get_name(), e.response.code_name, e.response.code))
        except (InvalidResponseException, UnexpectedResponseException) as e:
            print('Server sent an invalid payload : %s' % e.response.original_payload)


update_workflow()
