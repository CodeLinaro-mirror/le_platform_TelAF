#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys
from ..check_fn import *
from ..logger import logger

def schema__data_identifier_set(top_node):
    Tname = schema__data_identifier_set.name = "data_identifier_set"

    logger.info(f"Checking top-node: [{Tname}]")

    for key, value in top_node.items():
        if not Camelcase()(key):
            logger.error(f"{Tname} . {key} <-- Bad key")
            sys.exit(1)
        assert type(value) is list
        for v in value:
            if not Int()(v): # Hexa()
                logger.error(f"{Tname} . {key} <-- Invalid value")
                sys.exit(1)
