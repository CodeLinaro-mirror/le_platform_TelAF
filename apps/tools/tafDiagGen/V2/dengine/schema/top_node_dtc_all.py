#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys
from ..check_fn import *
from ..logger import logger

def schema__dtc_all(top_node):
    Tname = schema__dtc_all.name = "dtc_all"
    need_to_stop = False

    def check_key(key):
        if not Int()(key): # Hexa3bytes
            nonlocal need_to_stop
            logger.error(f"{Tname} . {key}/{hex(key)} <-- Bad key")
            need_to_stop = True
            return False
        return True

    def check_item(value, key):
        assert isinstance(value, dict), f"{value}, {key}"
        nonlocal need_to_stop

        for required_key in {'identification', 'snapshots', 'functional_conditions'}:
            if required_key not in value.keys():
                logger.error(f"{Tname} . {key}/{hex(key)} . {required_key} <-- Required but lost")
                need_to_stop = True
                return
            if type(value[required_key]) is not dict:
                logger.error(f"{Tname} . {key}/{hex(key)} . {required_key} <-- Not a dict to include sub-nodes")
                need_to_stop = True
                return

        for required_key, check_fn in {
            'code': Int(),# Hexa2bytes
            'fault_type': Int(), # Hexa1byte
            'priority': Int(),
            'extended_data_records': lambda v: all([ Camelcase()(i) for i in v ])
            }.items():
            if required_key not in value['identification'].keys():
                logger.error(f"{Tname} . {key}/{hex(key)} . identification . {required_key} <-- Required but lost")
                need_to_stop = True
                return
            if not check_fn(value['identification'][required_key]):
                logger.error(f"{Tname} . {key}/{hex(key)} . identification . {required_key} <-- Invalid value")
                need_to_stop = True
                return

        for required_key, check_fn in {
            'snapshot_record_content': Camelcase(),
            'freeze_frames': lambda v: all([ Camelcase()(i) for i in v ])
            }.items():
            if required_key not in value['snapshots'].keys():
                logger.error(f"{Tname} . {key}/{hex(key)} . snapshots . {required_key} <-- Required but lost")
                need_to_stop = True
                return
            if not check_fn(value['snapshots'][required_key]):
                logger.error(f"{Tname} . {key}/{hex(key)} . snapshots . {required_key} <-- Invalid value")
                need_to_stop = True
                return

        for required_key in {'events'}:
            if required_key not in value['functional_conditions'].keys():
                logger.error(f"{Tname} . {key}/{hex(key)} . functional_conditions . {required_key} <-- Required but lost")
                need_to_stop = True
                return

        if type(value['functional_conditions']['events']) is not list:
            logger.error(f"{Tname} . {key}/{hex(key)} . functional_conditions . events <-- Not a LIST")
            need_to_stop = True
            return

        assert len(value['functional_conditions']['events']) >= 1

        # Check event-item
        def check_enable_condition_fn(prefix_log):
            def real_worker(condition):
                assert type(condition) is dict

                if 'and' not in condition.keys() and 'or' not in condition.keys():
                    logger.error(f"{prefix_log} . data_enable_condition <-- Not found and/or")
                    return False

                if 'and' in condition.keys() and 'or' in condition.keys():
                    logger.error(f"{prefix_log} . data_enable_condition <-- Found both and & or")
                    return False

                for logic, cond in condition.items():
                    assert type(cond) is list
                    for c in cond:
                        if not Camelcase()(c):
                            return False
                return True
            return real_worker

        events = value['functional_conditions']['events']
        for idx, event in enumerate(events):
            for required_key, check_fn in {
                'mnemonic': Mnemonic(),
                'confirmation_threshold': Int(min=1, max=255),
                'operation_cycle': Str(matches=r'^[A-Z\d_]+$'),
                'debounce_algorithm': Camelcase(),
                'data_enable_condition': check_enable_condition_fn(f"{Tname} . {key}/{hex(key)} . functional_conditions . events . {idx}"),
                }.items():
                if required_key not in event.keys():
                    logger.error(f"{Tname} . {key}/{hex(key)} . functional_conditions . events . {idx} . {required_key} <-- Required but lost")
                    need_to_stop = True
                    return
                if not check_fn(event[required_key]):
                    logger.error(f"{Tname} . {key}/{hex(key)} . functional_conditions . events . {idx} . {required_key} <-- Invalid value")
                    need_to_stop = True
                    return

    logger.info(f"Checking top-node: [{Tname}]")
    for key, value in top_node.items():
        if not check_key(key):
            continue
        check_item(value, key)

    if need_to_stop is True:
        sys.exit(1)
