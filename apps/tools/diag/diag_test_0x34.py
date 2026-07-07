# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

from doipclient import DoIPClient
from doipclient.connectors import DoIPClientUDSConnector
from udsoncan.client import Client
from udsoncan.services import *
from udsoncan.exceptions import *
from udsoncan.common import *
from udsoncan import services, MemoryLocation
import udsoncan
import os
import udsoncan.configs
import logging
import time
import threading
import struct

#logging.basicConfig(level=logging.DEBUG)
# UDS config information
config = dict(udsoncan.configs.default_client_config)

key =  b"\x11\x22\x33\x44"
update_file = os.environ.get("UPDATE_FILE", "/home/ubuntu/data/update_ubi_ab.zip")

bytes_per_pack = 4000

config['request_timeout'] = None

doip_remote_ip = os.environ.get("DOIP_REMOTE_IP", "192.168.225.1")
doip_client = DoIPClient(doip_remote_ip, 0x0201)

uds_connection = DoIPClientUDSConnector(doip_client)

def dummy_send2key(level, seed):
    key = bytearray(seed)
    for i in range(len(key)):
        key[i] += (level + i)
    return bytes(key)


def TestRequestDownload():
    with Client(uds_connection, config=config) as uds_client:
        try:
            # Step1: Enter extend session(DiagnosticSessionControl). 10 03
            response = uds_client.change_session(DiagnosticSessionControl.Session.programmingSession)
            print(response)

            # Step2: Security access #1-Request seed(SecurityAccess). 27 01
            response = uds_client.request_seed(0x01)
            seed = response.service_data.seed

            # Calculate key via seed.
            key = dummy_send2key(level=0x01, seed=seed)

            # Step3: Security access #2-Send key(SecurityAccess). 27 02
            response = uds_client.send_key(0x02, key)
            print(response)

            with open(update_file, "rb") as f:
                f.seek(0, 2)    # Move to end of file
                eof = f.tell()
                hex_eof = hex(eof) # Convert to hex
                print("EOF:", eof)
                print("Hex EOF:", hex_eof)

                # Step4: RequestDownload(0x34)
                memloc = MemoryLocation(address=0x1234, memorysize=eof, address_format=16, memorysize_format=32)
                response = uds_client.request_download(memory_location=memloc)
                print(response)
                print("Max length: %d" % response.service_data.max_length)
                global bytes_per_pack
                if response.service_data.max_length < bytes_per_pack:
                    bytes_per_pack = response.service_data.max_length
                print("bytes_per_pack=%d" % bytes_per_pack)
                f.seek(0, 0)
                sq = 1
                # Step5: Transfer Data(TransferData). 36
                while f.tell() < eof:
                    bs = f.read(bytes_per_pack)
                    #response = uds_client.transfer_data(sq, bs)
                    response = uds_client.transfer_data(sequence_number=sq, data=bs)
                    print(response)
                    sq += 1
                    if sq > 0xff:
                        sq = 0
                f.close()

            # Step6: Transter Exit(RequestTransferExit). 37
            response = uds_client.request_transfer_exit()
            print(response)

        except NegativeResponseException as e:
            print('Server refused our request for service %s with code "%s" (0x%02x)' % (e.response.service.get_name(), e.response.code_name, e.response.code))
        except (InvalidResponseException, UnexpectedResponseException) as e:
            print('Server sent an invalid payload : %s' % e.response.original_payload)


TestRequestDownload()