#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

from pathlib import Path
import logging

def create_logger():
    current_dir = Path(__file__).parent

    logger = logging.getLogger("tafDiagGen")
    logger.propagate = False
    logger.setLevel(logging.DEBUG) # defualt should be DEBUG

    # Destination to console
    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.INFO)
    console_formatter = logging.Formatter("%(message)s")
    console_handler.setFormatter(console_formatter)
    logger.addHandler(console_handler)

    # Destination to file
    log_output = Path(current_dir / '..' / 'build')
    if not log_output.exists():
        log_output = current_dir

    file_handler = logging.FileHandler(log_output / 'debug.log', mode="w")
    file_handler.setLevel(logging.DEBUG)
    file_formatter = logging.Formatter("%(message)s")
    file_handler.setFormatter(file_formatter)
    logger.addHandler(file_handler)

    logger.debug("Logger loaded <--")
    return logger

logger = create_logger()
