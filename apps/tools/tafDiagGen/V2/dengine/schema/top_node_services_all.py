#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys
from ..check_fn import *
from ..logger import logger

def schema__services_all(top_node):
    Tname = schema__services_all.name = "services_all"
    need_to_stop = False

    def check_key(key):
        if not Int()(key): # Hexa()
            nonlocal need_to_stop
            logger.error(f"{Tname} . {key} <-- Bad key")
            need_to_stop = True
            return False
        return True

    def check_item(value, key):
        assert isinstance(value, dict), f"{value}, {key}"
        nonlocal need_to_stop

        for required_key, check_fn in {
            "supported": Bool(),
        }.items():
            if required_key not in value.keys():
                logger.error(f"{Tname} . {key} . {required_key} <-- Required but lost")
                need_to_stop = True
                return

            if not check_fn(value[required_key]):
                logger.error(f"{Tname} . {key} . {required_key} <-- Invalid value")
                need_to_stop = True
                return

        # Optional -> IDPS_supported: If not present, defaults to False
        if 'IDPS_supported' in value.keys():
            if not Bool()(value['IDPS_supported']):
                logger.error(f"{Tname} . {key} . IDPS_supported <-- Invalid value")
                need_to_stop = True
                return

        if 'sub_functions' in value.keys():
            assert isinstance(value['sub_functions'], dict)
            for k,v in value['sub_functions'].items():
                if not Int()(k):# Hexa()
                    logger.error(f"{Tname} . {key} . sub_functions . {k} <-- Bad key")
                    need_to_stop = True
                    continue

                # Optional -> sub_functions.id.supported: If not present, is equal to FALSE
                if 'supported' in v.keys():
                    if not Bool()(v['supported']):
                        logger.error(f"{Tname} . {key} . sub_functions . {k} . supported <-- Invalid value")
                        need_to_stop = True
                        break

    logger.info(f"Checking top-node: [{Tname}]")
    for key, value in top_node.items():
        if not check_key(key):
            continue
        check_item(value, key)

    if need_to_stop is True:
        sys.exit(1)
