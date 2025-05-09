#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys
from ..check_fn import *
from ..logger import logger

def schema__reset_all(top_node):

    Tname = schema__reset_all.name = "reset_all"
    assert type(top_node) is dict
    need_to_stop = False

    def check_key(key):
        if not Str()(key): # Hexa()
            logger.error(f"{Tname} . {key} <-- Bad key")
            nonlocal need_to_stop
            need_to_stop = True
            return False
        return True

    def check_item(value, key):
        nonlocal need_to_stop

        # sub_function_identifier <- SubFunctionNumber (Conversion is needed)
        for required_key in ['SubFunctionNumber']:
            if required_key not in value.keys():
                logger.error(f"{Tname} . {key} . {required_key} <-- Required but lost")
                need_to_stop = True
                return False

        if not Int()(value['SubFunctionNumber']):
            logger.error(f"{Tname} . {key} . SubFunctionNumber <-- Invalid value")
            need_to_stop = True
            return False

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

        return True

    logger.info(f"Checking top-node: [{Tname}]")
    for key, value in top_node.items():
        if not check_key(key):
            continue
        check_item(value, key)

    if need_to_stop is True:
        sys.exit(1)
