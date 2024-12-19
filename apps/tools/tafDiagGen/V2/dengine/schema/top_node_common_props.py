#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys
from ..check_fn import *
from ..logger import logger

def schema__common_props(top_node):
    Tname = schema__common_props.name = "common_props"

    logger.info(f"Checking top-node: [{Tname}]")

    for required_key, check_fn in {
        'max_number_of_request_correctly_received_response_pending': Int(),
        'occurrence_counter_processing' : Str(equals='TEST-FAILED-BIT'),
        # 's3_server_max' : Float(),
        # 'ignore_request_for_hardreset' : Bool(),
        # 'dtc_status_availability_mask' : Int() # Hexa()
    }.items():
        if required_key not in top_node.keys():
            logger.error(f"{Tname} . {required_key} <-- Required but lost")
            sys.exit(1)

        if not check_fn(top_node[required_key]):
            logger.error(f"{Tname} . {required_key} <-- Invalid value")
            sys.exit(1)
