#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import logging
from contextlib import contextmanager
import threading

from .constants import PREFIX_MESSAGE, DENGINE_ID

# Set the default logger for 'dengine' package --> drop all outputs by default
logging.getLogger(DENGINE_ID).addHandler(logging.NullHandler())

def get_sub_logger(subname, level):
    # Get one new logger that should inherit the 'DENGINE_ID'
    logger = logging.getLogger(DENGINE_ID + "." + subname)
    logger.propagate = False
    logger.setLevel(level)
    return logger

def setup_console_logger(subname, level=logging.INFO, console_format=None, logger = None):

    if logger is None or not isinstance(logger, logging.Logger):
        logger = get_sub_logger(subname, level)

    console_handler = logging.StreamHandler() # to console

    # We set 'logger' level and 'handler' level are same.
    console_handler.setLevel(level)

    console_formatter = logging.Formatter(console_format)
    console_handler.setFormatter(console_formatter)

    logger.addHandler(console_handler)

    return logger

def setup_file_logger(subname, file_name, level=logging.DEBUG, file_format=None, logger = None):

    if logger is None or not isinstance(logger, logging.Logger):
        logger = get_sub_logger(subname, level)

    file_handler = logging.FileHandler(file_name, mode="w")
    file_handler.setLevel(level)

    file_formatter = logging.Formatter(file_format)
    file_handler.setFormatter(file_formatter)

    logger.addHandler(file_handler)

    return logger

def setup_console_file_logger(subname, filename, level=logging.INFO, uni_format=None, logger = None):
    logger = setup_console_logger(subname, level, uni_format, logger = logger)
    logger = setup_file_logger(subname, filename, level, uni_format, logger = logger)
    return logger

# output to console
logc  = setup_console_logger('console', level = logging.INFO, console_format="%(message)s")

# output to file-stream
logf  = setup_file_logger('stream', 'backtrace.log', level = logging.DEBUG, file_format="%(message)s")

# output to console & file-stream
logcf = setup_console_file_logger('CF', 'record.log', level = logging.INFO, uni_format="%(message)s")


thread_local = threading.local()
thread_local.level = 0

def trace_message(message, increment=None, tweak_indent=None):
    if increment is True:
        thread_local.level += 1

    if increment is None:
        if tweak_indent is None:
            indent = '  ' * (thread_local.level)
        else:
            indent = '  ' * tweak_indent
    else:
        indent = '  ' * (thread_local.level - 1)

    logf.info(f"{indent}{message}")

    if increment is False:
        thread_local.level -= 1

class CheckCTX(): pass
c = CheckCTX()

@contextmanager
def checking_backtrace(level_description, first_top_node=False):
    trace_message(f"{level_description} -- BEG", increment=True)
    mark = False
    c.level = thread_local.level
    try:
        yield c
    except:
        mark = True
        raise
    finally:
        if mark is not True:
            trace_message(f"{level_description} -- END", increment=False)
        else:
            if first_top_node is True:
                trace_message(f"{level_description} -- END", increment=False)
            else:
                thread_local.level -= 1
        mark = False
