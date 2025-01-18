#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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


    logger.info(f"Checking top-node: [{Tname}]")
    for key, value in top_node.items():
        if not check_key(key):
            continue
        check_item(value, key)

    if need_to_stop is True:
        sys.exit(1)
