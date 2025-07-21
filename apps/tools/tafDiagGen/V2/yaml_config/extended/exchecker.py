#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys

from dengine.check_fn import *
from dengine import logger as logger


def schema__debounce_algorithm_type(top_node): pass
def schema_freeze_frame_trigger_type(top_node): pass

mapping = {
    # Added by default, don't check them!!
    'debounce_algorithm_type': schema__debounce_algorithm_type,
    'freeze_frame_trigger_type' : schema_freeze_frame_trigger_type,
}

def export(): return mapping
