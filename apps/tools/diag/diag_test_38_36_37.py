#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import configparser
import os, sys, re
import argparse, logging

import doipclient
import udsoncan
from doipclient.connectors import DoIPClientUDSConnector
from udsoncan.services import DiagnosticSessionControl
from udsoncan.client import Client

# logging.basicConfig(level=logging.DEBUG)

def is_valid_ip(ip_str):
    ip_regex = r'^(\d{1,3}\.){3}\d{1,3}$'
    match = re.match(ip_regex, ip_str)
    if match:
        octets = ip_str.split('.')
        if all(0 <= int(octet) <= 255 for octet in octets):
            return True
    return False

def is_valid_hex(input_str):
    return input_str.startswith("0x") \
           and all(c in "0123456789abcdefABCDEF" for c in input_str[2:])

class DiagClient():

    default_diag_config_file = os.path.join(
                               os.path.dirname(os.path.abspath(__file__)),
                               "diag_client_default.ini")

    def __init__(self, conf_file= None,
                 server_ip_address = None,
                 server_physical_address = None):

        self.conf = configparser.ConfigParser()

        if conf_file is None:
            conf_file = DiagClient.default_diag_config_file

        if not os.path.exists(conf_file):
            print("Not found Diag Client config file (.ini): [{}]".format(conf_file))
            sys.exit(-1)

        self.conf.read(conf_file)

        if server_ip_address is None:
            self.s_ip = self.conf["client.doip"]["server-ip-address"]
        else:
            if not is_valid_ip(server_ip_address):
                print("Bad server ip address [{}]".format(server_ip_address))
                sys.exit(-1)
            else:
                self.s_ip = server_ip_address

        if server_physical_address is None:
            self.s_phyaddr = int(self.conf["client.uds"]["server-physical-address"], 16)
        else:
            if not is_valid_hex(server_physical_address):
                print("Bad format for server physical address, should HEX, such as: 0x0E00, not [{}]"
                    .format(server_physical_address))
                sys.exit(-1)
            else:
                self.s_phyaddr = int(server_physical_address, 16)

    def do_connect(self):
        self.doip_client = doipclient.DoIPClient(self.s_ip, self.s_phyaddr)
        uds_connection = DoIPClientUDSConnector(self.doip_client)
        config = dict(udsoncan.configs.default_client_config)
        self.uds_client = Client(uds_connection, config=config)
        self.uds_client.open()

    def doip_alive_check(self):
        return self.doip_client.request_alive_check()

    def _uds_change_session(self, which_session):
        return self.uds_client.change_session(which_session)

    def extended_session(self):
        return self._uds_change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)

    def default_session(self):
        return self._uds_change_session(DiagnosticSessionControl.Session.defaultSession)

    def programming_session(self):
        return self._uds_change_session(DiagnosticSessionControl.Session.programmingSession)


def unlock_device(diag):
    def dummy_send2key(level, seed):
        key = bytearray(seed)
        for i in range(len(key)):
            key[i] += (level + i)
        return bytes(key)

    # Security access #1-Request seed(SecurityAccess). 27 01
    response = diag.uds_client.request_seed(0x01)

    if response.service_data.seed == bytes(len(response.service_data.seed)):
        return # The device was unlocked already

    key = dummy_send2key(level=0x01, seed=response.service_data.seed)

    # Security access #2-Send key(SecurityAccess). 27 02
    response = diag.uds_client.send_key(0x02, key)


def nack_response_handler(func):
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except udsoncan.exceptions.NegativeResponseException as e:
            pass
        except (udsoncan.exceptions.InvalidResponseException,
                udsoncan.exceptions.UnexpectedResponseException) as e:
            print('Server sent an invalid payload : {}'.format(e.response.original_payload))

    return wrapper


def download_to_target(diag, mmoop, file_in_host, file_in_target, break_in_sq = None):

    if mmoop not in (0x01, 0x03, 0x06):
        raise Exception("Bad moop for downloading type actions.")

    diag.extended_session()
    unlock_device(diag)
    diag.programming_session()
    unlock_device(diag)

    with open(file_in_host, "rb") as in_file:
        in_file.seek(0, 2)
        file_in_host_size = in_file.tell()
        in_file.seek(0, 0)

        response = diag.uds_client.request_file_transfer(moop=mmoop, path=file_in_target, filesize=file_in_host_size)
        max_pack_size = response.service_data.max_length - 2 # SID + SequenceNumber

        if (mmoop == 0x06):
            # print("filePosition = {}({:08x})".format(response.service_data.fileposition, response.service_data.fileposition))
            in_file.seek(response.service_data.fileposition, 0)

        sq = 1

        while in_file.tell() < file_in_host_size:
            bs = in_file.read(max_pack_size)
            response = diag.uds_client.transfer_data(sequence_number=sq, data=bs)

            if mmoop != 0x06 and break_in_sq:
                if break_in_sq() == sq:
                    return None # without the rest data & 37

            sq += 1
            if sq > 0xff:
                sq = 0

    diag.uds_client.request_transfer_exit()
    diag.extended_session()

@nack_response_handler
def RFT_add_file(diag, file_in_host, file_in_target):
    download_to_target(diag, 0x01, file_in_host, file_in_target, break_in_sq = None)

@nack_response_handler
def RFT_add_file_with_break(diag, file_in_host, file_in_target):
    download_to_target(diag, 0x01, file_in_host, file_in_target, break_in_sq = lambda: 3)

@nack_response_handler
def RFT_replace_file(diag, file_in_host, file_in_target):
    download_to_target(diag, 0x03, file_in_host, file_in_target, break_in_sq = None)

@nack_response_handler
def RFT_replace_file_with_break(diag, file_in_host, file_in_target):
    download_to_target(diag, 0x03, file_in_host, file_in_target, break_in_sq = lambda: 3)

@nack_response_handler
def RFT_resume_file(diag, file_in_host, file_in_target):
    download_to_target(diag, 0x06, file_in_host, file_in_target, break_in_sq = None)

@nack_response_handler
def RFT_delete_file(diag, file_in_target):
    diag.extended_session()
    unlock_device(diag)
    diag.programming_session()
    unlock_device(diag)

    diag.uds_client.request_file_transfer(moop=0x02, path=file_in_target)
    diag.extended_session()


def upload_from_target(diag, mmoop, path_in_host, path_in_target):
    diag.extended_session()
    unlock_device(diag)
    diag.programming_session()
    unlock_device(diag)

    response = diag.uds_client.request_file_transfer(moop=mmoop, path=path_in_target)

    if mmoop == 0x05:
        target_path_size = response.service_data.dirinfo_length
    elif mmoop == 0x04:
        target_path_size = response.service_data.filesize.uncompressed
        out_file = open(path_in_host, "wb")
    else:
        raise Exception("Bad moop in uploading flow.")

    total_size_received = 0

    path_list = []

    if mmoop == 0x05:
        pass # print("Content in : [{}]".format(path_in_target))

    sq = 1
    while total_size_received < target_path_size:
        response = diag.uds_client.transfer_data(sequence_number=sq)
        if mmoop == 0x05:
            # 'rstrip' is used to remove the tail '\0'
            # print("--> {}".format(response.service_data.parameter_records.decode("ascii").rstrip('\0')))
            path_list.append(response.service_data.parameter_records.decode("ascii").rstrip('\0'))
        elif mmoop == 0x04:
            out_file.write(response.service_data.parameter_records)
        total_size_received += len(response.service_data.parameter_records)
        sq += 1
        if sq > 0xff:
            sq = 0

    if mmoop == 0x04:
        out_file.close()

    diag.uds_client.request_transfer_exit()
    diag.extended_session()

    if mmoop == 0x05:
        return path_list
    else:
        return None

@nack_response_handler
def RFT_read_dir(diag, path_in_target):
    return upload_from_target(diag, 0x05, None, path_in_target)

@nack_response_handler
def RFT_read_file(diag, path_in_host, path_in_target):
    return upload_from_target(diag, 0x04, path_in_host, path_in_target)

def testcases(cfile, s_ip, s_phyaddr):
    print("-- [Start testing diag 38/36/37 flow] --")

    diag = DiagClient(cfile, s_ip, s_phyaddr)
    diag.do_connect()
    diag.doip_alive_check()
    diag.default_session()

    try:
        # READ-DIR
        path_list = RFT_read_dir(diag, "/data")
        assert "le_fs" in path_list, "RFT: Read Dir (0x05) is [NOK]"
        print("RFT: Read Dir (0x05) is [OK]")

        # READ-FILE
        RFT_read_file(diag, "/tmp/liblegato.so", "/legato/systems/current/lib/liblegato.so")
        result = os.system("strings /tmp/liblegato.so | grep le_msg_OpenSession > /dev/null 2>&1 ")
        assert result == 0, "RFT: Read File (0x04) is [NOK]"
        print("RFT: Read File (0x04) is [OK]")

        # ADD-FILE
        RFT_add_file(diag, "/etc/os-release", "/data/os-release")
        path_list = RFT_read_dir(diag, "/data")
        assert "os-release" in path_list, "RFT: Add File (0x01) is [NOK]"
        RFT_read_file(diag, "/tmp/os-release", "/data/os-release")
        result = os.system("diff /tmp/os-release /etc/os-release > /dev/null 2>&1 ")
        assert result == 0, "RFT: Add File (0x01) is [NOK]"
        os.system("rm -f /tmp/os-release")
        print("RFT: Add File (0x01) is [OK]")

        # REPLACE-FILE
        RFT_replace_file(diag, "/etc/passwd", "/data/os-release")
        RFT_read_file(diag, "/tmp/passwd", "/data/os-release")
        result = os.system("diff /etc/passwd /tmp/passwd > /dev/null 2>&1 ")
        assert result == 0, "RFT: Replace File (0x03) is [NOK]"
        os.system("rm -f /tmp/passwd")
        print("RFT: Replace File (0x03) is [OK]")

        # DELETE-FILE
        RFT_delete_file(diag, "/data/os-release")
        path_list = RFT_read_dir(diag, "/data")
        assert "os-release" not in path_list, "RFT: Delete File (0x02) is [NOK]"
        print("RFT: Delete File (0x02) is [OK]")

        # RESUME-FILE after ADD-FILE
        RFT_add_file_with_break(diag, "/tmp/liblegato.so", "/data/liblegato.so")
        diag.doip_client.close()
        diag.doip_client.reconnect()
        path_list = RFT_read_dir(diag, "/data")
        assert "liblegato.so.incomplete" in path_list, "RFT: Add file with break is [NOK]"
        RFT_resume_file(diag, "/tmp/liblegato.so", "/data/liblegato.so")
        path_list = RFT_read_dir(diag, "/data")
        assert "liblegato.so.incomplete" not in path_list, "RFT: Resume File (0x06) after Add-File is [NOK]"
        assert "liblegato.so" in path_list, "RFT: Resume File (0x06) after Add-File is [NOK]"
        RFT_read_file(diag, "/tmp/liblegato.so.bak", "/data/liblegato.so")
        result = os.system("diff /tmp/liblegato.so.bak /tmp/liblegato.so > /dev/null 2>&1 ")
        assert result == 0, "RFT: Resume File (0x06) after Add-File is [NOK]"
        os.system("rm -f /tmp/liblegato.so.bak /tmp/liblegato.so")
        print("RFT: Resume File (0x06) after Add-File is [OK]")

        # RESUME-FILE after REPLACE-FILE
        RFT_read_file(diag, "/tmp/libComponent_supervisor.so", "/legato/systems/current/lib/libComponent_supervisor.so")
        RFT_replace_file_with_break(diag, "/tmp/libComponent_supervisor.so", "/data/liblegato.so")
        diag.doip_client.close()
        diag.doip_client.reconnect()
        path_list = RFT_read_dir(diag, "/data")
        assert "liblegato.so.incomplete" in path_list, "RFT: Replace file with break is [NOK]"
        RFT_resume_file(diag, "/tmp/libComponent_supervisor.so", "/data/liblegato.so")
        path_list = RFT_read_dir(diag, "/data")
        assert "liblegato.so.incomplete" not in path_list, "RFT: Resume File (0x06) after Replace-File is [NOK]"
        assert "liblegato.so" in path_list, "RFT: Resume File (0x06) after Replace-File is [NOK]"
        RFT_read_file(diag, "/tmp/libComponent_supervisor.so.bak", "/data/liblegato.so")
        result = os.system("diff /tmp/libComponent_supervisor.so.bak /tmp/libComponent_supervisor.so > /dev/null 2>&1 ")
        assert result == 0, "RFT: Resume File (0x06) after Replace-File is [NOK]"
        print("RFT: Resume File (0x06) after Replace-File is [OK]")

        RFT_delete_file(diag, "/data/liblegato.so")
        os.system("rm -f /tmp/libComponent_supervisor.so /tmp/libComponent_supervisor.so.bak")

        print("-- [Done testing diag 38/36/37 flow] --")
    except:
        os.system("rm -f /tmp/liblegato.so.bak \
                         /tmp/liblegato.so \
                         /tmp/passwd /tmp/os-release \
                         /tmp/libComponent_supervisor.so \
                         /tmp/libComponent_supervisor.so.bak")
        print("\n--> For recover on target, please: rm -f /data/os-release /data/liblegato.so\n")
        raise

def main():
    parser = argparse.ArgumentParser(prog=__file__, description="Diag test in simulation")
    parser.add_argument("-v", "--version", action='version', version="%(prog)s " + "1.0.0")
    parser.add_argument("-c", "--cfile", dest="cfile", metavar='<uds-doip-config-file>', help="a config-file for uds & doip")
    parser.add_argument("-p", "--server-ip", dest="s_ip", metavar='<server-ip-address>', help="doip server IP address")
    parser.add_argument("-a", "--server-physical-address", dest="s_phyaddr", metavar='<server-physical-address>', help="uds server physical address")
    args = parser.parse_args()
    testcases(args.cfile, args.s_ip, args.s_phyaddr)

if __name__ == "__main__":
    main()
