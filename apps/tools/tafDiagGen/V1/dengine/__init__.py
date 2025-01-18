#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

__doc__ = """"""

import os

__version__ = "1.0.0"

from .schema import build_schema_tree, Pair
from .custom import build_custom_tree

from .test import build_test_schema, build_test_custom
from .checker import fn_origin
from .logger import logf, logc
from . import generator

# export the 'Checker' to plugings
from .checker import Checker
from .constants import (
    PREFIX_MESSAGE,
    LOG_LEVEL_MAPPING
)

from .exceptions import (
    InvalidSchemaPath,
    InvalidCustomPath
)

__autoher__ = "Yang Zheng"

def build_schema(s_path):
    x_path = os.path.abspath(s_path)
    if not os.path.exists(x_path):
        raise InvalidSchemaPath(s_path, x_path)

    if not os.path.isdir(x_path):
        raise Exception("{} is not a dir for 'schema'".format(x_path))

    if not any(os.scandir(x_path)):
        raise Exception("{} is a empty dir for 'schema'".format(x_path))

    return build_schema_tree(x_path)

def build_custom(c_path):
    x_path = os.path.abspath(c_path)
    if not os.path.exists(x_path):
        raise InvalidCustomPath(c_path, x_path)

    if not os.path.isdir(x_path):
        raise Exception("{} is not a dir for 'config'".format(x_path))

    is_empty = True
    with os.scandir(x_path) as dirs:
        for _dir in dirs:
            if _dir.name.startswith('.'):
                continue
            is_empty = False
            break

    if is_empty:
        raise Exception("{} is a empty dir for 'config'".format(x_path))

    return build_custom_tree(x_path)

def generate(schema, custom, tmpls, build_dir):
    os.makedirs(build_dir, exist_ok=True)
    generator.generate_json(custom, build_dir)
    generator.generate_code(custom, tmpls, build_dir)
    # No return code

def build_test_combo(schema_buff, custom_buff, ex_syn_path=None):
    sc = build_test_schema(schema_buff)
    sc.transform()
    if ex_syn_path is not None:
        sc.extend(ex_syn_path)
    cu = build_test_custom(custom_buff)
    sc.check(cu)
