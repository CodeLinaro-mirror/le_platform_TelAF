#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys
from ..check_fn import *
from ..logger import logger

def schema__did_all(top_node):
    Tname = schema__did_all.name = "did_all"
    need_to_stop = False

    def check_key(key):
        if not Int()(key): # Hexa
            nonlocal need_to_stop
            logger.error(f"{Tname} . {hex(key)} <-- Bad key")
            need_to_stop = True
            return False
        return True

    def check_authentication_pattern_value(auth_data):
        if type(auth_data) is not list:
            logger.error(f"{auth_data} <-- Not a LIST")
            need_to_stop = True
            return False

        # Need to check the empty list
        if len(auth_data) == 0:
            logger.error(f"{auth_data}  <-- empty attribute list")
            need_to_stop = True
            return False

        # The duplicate attributes in list are not allowed
        if len(auth_data) != len(set(auth_data)):
            logger.error(f"{auth_data}  <-- duplicate attribute in list")
            need_to_stop = True
            return False

        role_list = ['b0', 'b1', 'b2', 'b3', 'b4', 'b5', 'b6']
        for v in auth_data:
            if v not in role_list:
                logger.error(f"{auth_data} . {v} <-- Invalid value")
                need_to_stop = True
                return False
        return True

    def check_item(value, key):
        assert isinstance(value, dict), f"{value}, {key}"
        nonlocal need_to_stop

        key = hex(key)

        for required_key in {'identification', 'implementation', 'supported_functions'}:
            if required_key not in value.keys():
                logger.error(f"{Tname} . {key} . {required_key} <-- Required but lost")
                need_to_stop = True
                return

        for required_key, check_fn in {'code': Int()}.items():
            if required_key not in value['identification'].keys():
                logger.error(f"{Tname} . {key} . identification . {required_key} <-- Required but lost")
                need_to_stop = True
                return
            if not check_fn(value['identification'][required_key]):
                logger.error(f"{Tname} . {key} . identification . {required_key} <-- Invalid value")
                need_to_stop = True
                return

        for required_key, check_fn in {
            'did_size': Int(min=1, max=65535),
            'endianness': Enum('LSB', 'MSB', etype=str)
            }.items():
            if required_key not in value['implementation'].keys():
                logger.error(f"{Tname} . {key} . implementation . {required_key} <-- Required but lost")
                need_to_stop = True
                return
            if not check_fn(value['implementation'][required_key]):
                logger.error(f"{Tname} . {key} . implementation . {required_key} <-- Invalid value")
                need_to_stop = True
                return

        for required_key, check_fn in {
            'write_did': Bool(),
            'read_did': Bool(),
            'io_control': Bool(),
            }.items():
            if required_key not in value['supported_functions'].keys():
                logger.error(f"{Tname} . {key} . supported_functions . {required_key} <-- Required but lost")
                need_to_stop = True
                return
            if not check_fn(value['supported_functions'][required_key]):
                logger.error(f"{Tname} . {key} . supported_functions . {required_key} <-- Invalid value")
                need_to_stop = True
                return

        # Optional:
        # did_accessibility
        # access
        # data_enable_condition

        if 'did_accessibility' in value.keys():
            if value['did_accessibility'] is None:
                pass # Allow empty
            else:
                # Checking the 'diagnostic_session_and_security_level' & 'did_accessibility'
                # They can't miss in the same time !!
                if 'diagnostic_session_and_security_level' not in value['did_accessibility'].keys() \
                and 'diagnostic_session' not in value['did_accessibility'].keys():
                    logger.error(f"{Tname} . {key} . did_accessibility <-- diagnostic_session_and_security_level & diagnostic_session, neither present")
                    need_to_stop = True
                    return

                if 'diagnostic_session' in value['did_accessibility'].keys():
                    if not isinstance(value['did_accessibility']['diagnostic_session'], dict):
                        logger.error(f"{Tname} . {key} . did_accessibility . diagnostic_session <-- Not a DICT")
                        need_to_stop = True
                        return

                    for k,v in value['did_accessibility']['diagnostic_session'].items():
                        if not Str(matches=r'^[_a-z]+$')(k):
                            logger.error(f"{Tname} . {key} . did_accessibility . diagnostic_session . {k} <-- Invalid session name")
                            need_to_stop = True
                            return

                        # all([]) is True -> Need to check the empty list
                        if len(v) == 0:
                            logger.error(f"{Tname} . {key} . did_accessibility . diagnostic_session . {v} <-- empty attribute list")
                            need_to_stop = True
                            return

                        # The duplicate attributes in list are not allowed
                        if len(v) != len(set(v)):
                            logger.error(f"{Tname} . {key} . did_accessibility . diagnostic_session . {v} <-- duplicate attribute in list")
                            need_to_stop = True
                            return

                        if not all([ True if _v in ['R', 'W', 'IO'] else False for _v in v ]):
                            logger.error(f"{Tname} . {key} . did_accessibility . diagnostic_session . {v} <-- Invalid valid")
                            need_to_stop = True
                            return

                if 'diagnostic_session_and_security_level' in value['did_accessibility'].keys():

                    did_secure_node = value['did_accessibility']['diagnostic_session_and_security_level']

                    if not isinstance(did_secure_node, dict):
                        logger.error(f"{Tname} . {key} . did_accessibility . diagnostic_session_and_security_level <-- Not a DICT")
                        need_to_stop = True
                        return

                    if 'execution_authentication_pattern_read' in did_secure_node.keys():
                        if not check_authentication_pattern_value(did_secure_node['execution_authentication_pattern_read']):
                            logger.error(f"{Tname} . {key} . did_accessibility . diagnostic_session_and_security_level. execution_authentication_pattern_read")
                            need_to_stop = True
                            return

                    if 'execution_authentication_pattern_write' in did_secure_node.keys():
                        if not check_authentication_pattern_value(did_secure_node['execution_authentication_pattern_write']):
                            logger.error(f"{Tname} . {key} . did_accessibility . diagnostic_session_and_security_level . execution_authentication_pattern_write")
                            need_to_stop = True
                            return

                    if 'execution_authentication_pattern_io' in did_secure_node.keys():
                        if not check_authentication_pattern_value(did_secure_node['execution_authentication_pattern_io']):
                            logger.error(f"{Tname} . {key} . did_accessibility . diagnostic_session_and_security_level . execution_authentication_pattern_io")
                            need_to_stop = True
                            return

        if 'access' in value.keys():
            for required_key, check_fn in {
                'security_level': Int(),
                }.items():
                if required_key not in value['access'].keys():
                    logger.error(f"{Tname} . {key} . access . {required_key} <-- Required but lost")
                    need_to_stop = True
                    return
                if not check_fn(value['access'][required_key]):
                    logger.error(f"{Tname} . {key} . access . {required_key} <-- Invalid value")
                    need_to_stop = True
                    return

            if 'session' in value['access'].keys():
                if type(value['access']['session']) is not list:
                    logger.error(f"{Tname} . {key} . access . session <-- Not a LIST")
                    need_to_stop = True
                    return

            if 'role' in value['access'].keys():
                if type(value['access']['role']) is not list:
                    logger.error(f"{Tname} . {key} . access . role <-- Not a LIST")
                    need_to_stop = True
                    return

                # Need to check the empty list
                if len(value['access']['role']) == 0:
                    logger.error(f"{Tname} . {key} . access . role <-- empty attribute list")
                    need_to_stop = True
                    return

                # The duplicate attributes in list are not allowed
                if len(value['access']['role']) != len(set(value['access']['role'])):
                    logger.error(f"{Tname} . {key} . access. role <-- duplicate attribute in list")
                    need_to_stop = True
                    return

                role_list = ['b0', 'b1', 'b2', 'b3', 'b4', 'b5', 'b6']
                for v in value['access']['role']:
                    if v not in role_list:
                        logger.error(f"{Tname} . {key} . access . role . {v} <-- Invalid value")
                        need_to_stop = True
                        return

            # Check more for 'access'?

        if 'data_enable_condition' in value.keys():
            if 'and' not in value['data_enable_condition'].keys() and 'or' not in value['data_enable_condition'].keys():
                logger.error(f"{Tname} . {key} . data_enable_condition <-- Not found and/or")
                need_to_stop = True
                return

            if 'and' in value['data_enable_condition'].keys() and 'or' in value['data_enable_condition'].keys():
                logger.error(f"{Tname} . {key} . data_enable_condition <-- Found both and & or")
                need_to_stop = True
                return

            for logic, cond in value['data_enable_condition'].items():
                if type(cond) is not list:
                    logger.error(f"{Tname} . {key} . data_enable_condition . {logic}  <-- Condition should be a LIST")
                    need_to_stop = True
                    return


    logger.info(f"Checking top-node: [{Tname}]")
    for key, value in top_node.items():
        if not check_key(key):
            continue
        check_item(value, key)

    if need_to_stop is True:
        sys.exit(1)
