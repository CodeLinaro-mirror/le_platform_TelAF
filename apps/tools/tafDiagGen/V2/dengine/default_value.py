#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import copy, os
import oyaml as yaml
from collections import OrderedDict
from .logger import logger
from .utils import *

def recursive_diff(already, default, result=None):
    if result is None:
        result = OrderedDict()

    # Handled in ordered case
    intersection = do_intersection(already.keys(), default.keys())
    left_only = do_diff_left(already, default)
    right_only = do_diff_left(default, already)

    for key in left_only:
        result[key] = already[key]
    for key in right_only:
        result[key] = default[key]
    for key in intersection:
        v_already = already[key]
        v_default = default[key]

        if isinstance(v_already, dict) and isinstance(v_default, dict):
            nested_diff_result = recursive_diff(v_already, v_default)
            if nested_diff_result:
                result[key] = nested_diff_result
        else:
            result[key] = v_default # Override
    return result

def resolve_default_values(only_yaml, def_yaml, dump_to_layer):
    only_yaml_with_default = copy.deepcopy(only_yaml)

    with open(def_yaml, 'r') as file:
        default_yaml = yaml.safe_load(file)

        with_default = recursive_diff(only_yaml, default_yaml, only_yaml_with_default)

        with open(os.path.join(dump_to_layer, 'merged_with_default.yaml'), 'w') as f:
            yaml.dump(with_default, f, default_flow_style=False, sort_keys=False)

        return copy.deepcopy(with_default)
