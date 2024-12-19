#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import os, logging

DENGINE_ID = 'dengine'

INDENT = os.environ.get("DE_SCHEMA_INDENT", " " * 4)

SUFFIX = os.environ.get("DE_SCHEMA_SUFFIX", ".yaml")

LOG_LEVEL_MAPPING = {
    'DEBUG': logging.DEBUG,
    'INFO': logging.INFO,
    'WARNING': logging.WARNING,
    'ERROR': logging.ERROR,
    'CRITICAL': logging.CRITICAL
}

PREFIX_MESSAGE = os.environ.get("DE_PREFIX_MSG",
    "TAF_DIAGGEN_%(levelname)s: %(module)s/%(funcName)s +%(lineno)s: ")

JSON_FNAME = os.environ.get("DE_JSON_FNAME", "diag_template.yaml.json")
