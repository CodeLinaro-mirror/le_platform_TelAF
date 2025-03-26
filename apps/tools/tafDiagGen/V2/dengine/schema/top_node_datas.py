#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys
from ..check_fn import *
from ..logger import logger

def schema__datas(top_node):
    need_to_stop = False
    Tname = schema__datas.name = 'datas'

    def check_key(key):
        if not Mnemonic()(key):
            logger.error(f"{Tname} . {key} <-- Bad key")
            nonlocal need_to_stop
            need_to_stop = True
            return False
        else:
            return True

    def check_item(value, key):
        nonlocal need_to_stop

        required = ['mnemonic', 'functional_definition']

        for required_key in required:
            if required_key not in value.keys():
                logger.error(f"{Tname} . {key} . {required_key} <-- Required but lost")
                need_to_stop = True
                return False

        if not Mnemonic()(value['mnemonic']):
            logger.error(f"{Tname} . {key} . mnemonic <-- Invalid value")
            need_to_stop = True
            return False

        assert type(value['functional_definition']) is dict,\
        f"{Tname} . {key} . functional_definition <-- Should be 'dict-type'"

        for required_key in ['data_type', 'value_type', 'bit_size']:
            if required_key not in value['functional_definition'].keys():
                logger.error(f"{Tname} . {key} . functional_definition . {required_key} <-- Required but lost")
                need_to_stop = True
                return False

        functional_definition = value['functional_definition']

        if not Enum('Numeric', 'Numeric list', 'Not numeric', etype=str)(functional_definition['data_type']):
            logger.error(f"{Tname} . {key} . functional_definition . data_type <-- Invalid value")
            need_to_stop = True
            return False

        if not Enum('boolean', 'uint8', 'sint8', 'uint16', 'sint16', 'uint32', 'sint32', etype=str)(functional_definition['value_type']):
            logger.error(f"{Tname} . {key} . functional_definition . value_type <-- Invalid value")
            need_to_stop = True
            return False

        if not Int(min=1, max=9999)(functional_definition['bit_size']):
            logger.error(f"{Tname} . {key} . functional_definition . bit_size <-- Invalid value")
            need_to_stop = True
            return False

        for x,y in functional_definition.items():
            if x == "coding" and functional_definition['data_type'] == "Numeric list":
                if isinstance(y, dict):
                    for _x,_y in y.items():
                        if functional_definition['bit_size'] % 8 == 0:
                            if not HexaOrBin()(hex(_x)):
                                logger.error(f"{_x} .  functional_definition . coding <-- Invalid value")
                                need_to_stop = True
                                return False
                        if functional_definition['bit_size'] % 2 == 0:
                            if not HexaOrBin()(bin(_x)):
                                logger.error(f"{_x} .  functional_definition . coding <-- Invalid value")
                                need_to_stop = True
                                return False
            else:
                continue

        return True # No error

    logger.info(f"Checking top-node: [{Tname}]")
    for key, value in top_node.items():
        if not check_key(key):
            continue
        check_item(value, key)

    if need_to_stop is True:
        sys.exit(1)
