#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys
from ..check_fn import *
from ..logger import logger

def schema__debounce_algorithm(top_node):
    Tname = schema__debounce_algorithm.name = "debounce_algorithm"
    need_to_stop = False

    def check_key(key):
        if not Str()(key):
            logger.error(f"{Tname} . {key} <-- Bad key")
            nonlocal need_to_stop
            need_to_stop = True
            return False
        return True

    def check_counter_based_item(value, key):
       nonlocal need_to_stop
       for required_key, check_fn in {
            "short_name": Camelcase(),
            "counter_based": Bool(),
            "debounce_counter_storage": Bool(),
            "debounce_behavior": Camelcase(),
            "counter_decrement_step_size": Int(),
            "counter_passed_threshold": Int(),
            "counter_increment_step_size": Int(),
            "counter_failed_threshold": Int(),
            "counter_jump_down_value": Int(),
            "counter_jump_up_value": Int(),
            "counter_jump_up": Bool(),
            "counter_jump_down": Bool(),
        }.items():
            if required_key not in value.keys():
                logger.error(f"{Tname} . {key} . {required_key} <-- Required but lost")
                need_to_stop = True
                return

            if not check_fn(value[required_key]):
                logger.error(f"{Tname} . {key} . {required_key} <-- Invalid value")
                need_to_stop = True
                return

    def check_time_based_item(value, key):
       nonlocal need_to_stop
       for required_key, check_fn in {
            "short_name": Camelcase(),
            "time_based": Bool(),
            "time_failed_threshold": Num(min=0.00),
            "time_passed_threshold": Num(min=0.00),
        }.items():
            if required_key not in value.keys():
                logger.error(f"{Tname} . {key} . {required_key} <-- Required but lost")
                need_to_stop = True
                return

            if not check_fn(value[required_key]):
                logger.error(f"{Tname} . {key} . {required_key} <-- Invalid value")
                need_to_stop = True
                return

    def check_monitor_internal_item(value, key):
       nonlocal need_to_stop
       for required_key, check_fn in {
            "short_name": Camelcase(),
            "monitor_internal": Bool(),
        }.items():
            if required_key not in value.keys():
                logger.error(f"{Tname} . {key} . {required_key} <-- Required but lost")
                need_to_stop = True
                return

            if not check_fn(value[required_key]):
                logger.error(f"{Tname} . {key} . {required_key} <-- Invalid value")
                need_to_stop = True
                return

    logger.info(f"Checking top-node: [{Tname}]")
    for key, value in top_node.items():
        if not check_key(key):
            continue

        assert isinstance(value, dict), f"{value}, {key}"

        if 'counter_based' in value.keys():
            check_counter_based_item(value, key)
        if 'time_based' in value.keys():
            check_time_based_item(value, key)
        if 'monitor_internal' in value.keys():
            check_monitor_internal_item(value, key)

    if need_to_stop is True:
        sys.exit(1)

