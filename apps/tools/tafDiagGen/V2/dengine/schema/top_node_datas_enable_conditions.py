#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys
from ..check_fn import *
from ..logger import logger

# datas_enable_conditions_nrc_map = OrderedDict()

def schema__datas_enable_conditions(top_node):
    Tname = schema__datas_enable_conditions.name = "datas_enable_conditions"
    need_to_stop = False

    def check_key(key):
        if not Camelcase()(key):
            logger.error(f"{Tname} . {key} <-- Bad key")
            nonlocal need_to_stop
            need_to_stop = True
            return False
        return True

    def check_item(value, key):
        assert isinstance(value, dict), f"{value}, {key}"
        nonlocal need_to_stop

        if "and" in value.keys() and "or" in value.keys():
            logger.error(f"{Tname} . {key} <-- 'and' & 'or' can't be present in the same time")
            need_to_stop = True
            return

        if "and" not in value.keys() and "or" not in value.keys():
            logger.error(f"{Tname} . {key} . (and | or) <-- Required but lost")
            need_to_stop = True
            return

        # either 'and' or 'or'
        for logic in value.keys():

            if logic not in ('and', 'or'): continue

            if type(value[logic]) is not list:
                logger.error(f"{Tname} . {key} . {logic} <-- Expect one LIST for the condition")
                need_to_stop = True
                return
            if len(value[logic]) < 1:
                logger.error(f"{Tname} . {key} . {logic} <-- List is empty")
                need_to_stop = True
                return
            else:
                nrc_record = set()
                # nrc = None
                for idx, item in enumerate(value[logic]):
                    assert type(item) is dict
                    for ops, body in item.items():
                        if 'nrc' not in body.keys():
                            logger.error(f"{Tname} . {key} . {logic} . {idx} . {ops} . nrc <-- Required but lost")
                            need_to_stop = True
                            return
                        else:
                            nrc_record.add(body['nrc'])
                            # nrc = body['nrc']

                if len(nrc_record) > 1:
                    logger.error(f"{Tname} . {key} . {logic} . {idx} . {ops} . nrc <-- Different NRCs {nrc_record} were set")
                    need_to_stop = True
                    return
                # global datas_enable_conditions_nrc_map
                # datas_enable_conditions_nrc_map[key] = { 'nrc' : nrc, 'id' : len(datas_enable_conditions_nrc_map) + 1}

    logger.info(f"Checking top-node: [{Tname}]")
    for key, value in top_node.items():
        if not check_key(key):
            continue
        check_item(value, key)

    if need_to_stop is True:
        sys.exit(1)
