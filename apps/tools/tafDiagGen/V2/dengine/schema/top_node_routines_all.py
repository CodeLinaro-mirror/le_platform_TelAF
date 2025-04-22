#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys
from ..check_fn import *
from ..logger import logger

def schema__routines_all(top_node):
    Tname = schema__routines_all.name = "routines_all"
    need_to_stop = False

    def check_key(key):
        if not Int()(key): # Camelcase()
            logger.error(f"{Tname} . {key} <-- Bad key")
            nonlocal need_to_stop
            need_to_stop = True
            return False
        return True

    def check_item(value, key):
        assert isinstance(value, dict), f"{value}, {key}"
        nonlocal need_to_stop

        def check_request_subnode(prefix_log):
            def _check_request_subnode(node):
                assert type(node) is dict
                if 'sub_function' not in node.keys():
                    logger.error(f"{prefix_log} . sub_function <-- Required but lost")
                    return False
                assert type(node['sub_function']) is list
                for n in node['sub_function']:
                    if not Int()(n): # Hexa()
                        logger.error(f"{prefix_log} . sub_function <-- Invalid value")
                        return False
                return True
            return _check_request_subnode

        for required_key, check_fn in {
            "identifier": Int(), # Hexa()
            "request": check_request_subnode(f"{Tname} . {key} . request"),
            "execution_authorization_pattern": Str(),
        }.items():
            if required_key not in value.keys():
                logger.error(f"{Tname} . {key} . {required_key} <-- Required but lost")
                need_to_stop = True
                return

            if not check_fn(value[required_key]):
                logger.error(f"{Tname} . {key} . {required_key} <-- Invalid value")
                need_to_stop = True
                return

        if 'access' in value.keys():
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

        # Optional -> data_enable_condition

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

        if 'execution_authentication_pattern' in value.keys():
            if type(value['execution_authentication_pattern']) is not list:
                    logger.error(f"{Tname} . execution_authentication_pattern <-- Not a LIST")
                    need_to_stop = True
                    return

            # Need to check the empty list
            if len(value['execution_authentication_pattern']) == 0:
                logger.error(f"{Tname} . execution_authentication_pattern  <-- empty attribute list")
                need_to_stop = True
                return

            # The duplicate attributes in list are not allowed
            if len(value['execution_authentication_pattern']) != len(set(value['execution_authentication_pattern'])):
                logger.error(f"{Tname} . execution_authentication_pattern  <-- duplicate attribute in list")
                need_to_stop = True
                return

            role_list = ['b0', 'b1', 'b2', 'b3', 'b4', 'b5', 'b6']
            for v in value['execution_authentication_pattern']:
                if v not in role_list:
                    logger.error(f"{Tname} . execution_authentication_pattern . {v} <-- Invalid value")
                    need_to_stop = True
                    return

    logger.info(f"Checking top-node: [{Tname}]")
    for key, value in top_node.items():
        if not check_key(key):
            continue
        check_item(value, key)

    if need_to_stop is True:
        sys.exit(1)
