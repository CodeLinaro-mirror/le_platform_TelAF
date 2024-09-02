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
update_file = os.environ.get("UPDATE_FILE", "/home/ubuntu/data/update_ubi_ab.zip")
restore_file = os.environ.get("RESTORE_FILE", "/data/images/update_ubi_ab.zip")

bytes_per_pack = 4000

# UDS config information
config = dict(udsoncan.configs.default_client_config)

config['request_timeout'] = None

doip_remote_ip = os.environ.get("DOIP_REMOTE_IP", "192.168.126.2")
doip_client = DoIPClient(doip_remote_ip, 0x0201, client_logical_address=0x0e11)

uds_connection = DoIPClientUDSConnector(doip_client)

def dummy_send2key(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)

def update_workflow():
    with Client(uds_connection, config=config) as uds_client:
        try:
            # Step11: Entering programming session(DiagnosticSessionControl). 10 02
            response = uds_client.change_session(DiagnosticSessionControl.Session.programmingSession)
            print(response)

            # Step14: Security access #1-Request seed(SecurityAccess). 27 01
            response = uds_client.request_seed(0x01)
            seed = response.service_data.seed

            # Calculate key via seed.
            key = dummy_send2key(level=0x01, seed=seed)

            # Step15: Security access #2-Send key(SecurityAccess). 27 02
            response = uds_client.send_key(0x02, key)
            print(response)

            with open(update_file, "rb") as f:
                f.seek(0, 2)    # Move to end of file
                eof = f.tell()

                print(eof)
                # Step16.1: RequestFileTransfer(0x38)
                response = uds_client.request_file_transfer(moop=1, path = restore_file, filesize=eof)
                print(response)
                print("Max length: %d" % response.service_data.max_length)
                global bytes_per_pack
                if response.service_data.max_length < bytes_per_pack:
                    bytes_per_pack = response.service_data.max_length
                print("bytes_per_pack=%d" % bytes_per_pack)
                f.seek(0, 0)
                sq = 1
                # Step16.2: Transfer Data(TransferData). 36
                while f.tell() < eof:
                    bs = f.read(bytes_per_pack)
                    #response = uds_client.transfer_data(sq, bs)
                    response = uds_client.transfer_data(sequence_number=sq, data=bs)
                    print(response)
                    sq += 1
                    if sq > 0xff:
                        sq = 0
                f.close()

            # Step16.3: Transter Exit(RequestTransferExit). 37
            response = uds_client.request_transfer_exit()
            print(response)

            # Step17: Switch to extended session(Perform ECU Reset). 10 03
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

            # Step18: Routine Control RUNDTCTEST(RoutineControl). 31 01 02 47 start to update
            response = uds_client.routine_control(routine_id=0x0247, control_type=0x01)
            print(response)

        except NegativeResponseException as e:
            print('Server refused our request for service %s with code "%s" (0x%02x)' % (e.response.service.get_name(), e.response.code_name, e.response.code))
        except (InvalidResponseException, UnexpectedResponseException) as e:
            print('Server sent an invalid payload : %s' % e.response.original_payload)


update_workflow()

