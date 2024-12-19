#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys
from ..check_fn import *
from ..logger import logger

def schema__diagnostic_session_security_level(top_node):
    Tname = schema__diagnostic_session_security_level.name = "diagnostic_session_security_level"
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

        def check_request_seed_fn(prefix_log):
            def real_worker(req_seed):
                if Int()(req_seed) is True:
                    if req_seed % 2 != 0:
                        return True
                    else:
                        logger.error(f"{prefix_log} . request_seed_id <-- Should be (% 2 != 0)")
                return False
            return real_worker

        for required_key, check_fn in {
            "short_name": Camelcase(),
            "key_size": Int(),
            "num_failed_security_access": Int(),
            "security_delay_time": Int(),
            "seed_size": Int(),
            "request_seed_id": check_request_seed_fn(f"{Tname} . {key}"),
            "execution_authorization_pattern": ExecutionAuthorizationPattern(),
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
        check_item(value, key)

    if need_to_stop is True:
        sys.exit(1)
