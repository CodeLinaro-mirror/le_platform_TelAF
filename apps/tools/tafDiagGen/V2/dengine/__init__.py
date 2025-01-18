#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

__doc__ = """"""

import os

__version__ = "2.0.0"

from . import generator
from .schema import schema_mapping
from .schema.resolve_references import resolve_all_references
from . import default_value
from . import converter
from .logger import logger


def schema_check(only_yaml):
    logger.info("[Stage] --> Schema checking ...")
    for top_node_name, top_node in only_yaml.items():
        schema_mapping[top_node_name] (top_node) # Check-Fn ( top_node )
    logger.info("[Stage] --> Schema checking [OK]")

def resolve_default_values(only_yaml, def_yaml, dump_to_layer):
    logger.info("[Stage] --> Resolve default values ... (roughly)")
    with_default = default_value.resolve_default_values(
        only_yaml,
        def_yaml,
        dump_to_layer)
    logger.info("[Stage] --> Resolve default values ... (roughly) [OK]")
    return with_default

def check_all_references(only_yaml_with_default):
    logger.info("[Stage] --> Check all references of top-nodes ...")
    resolve_all_references(only_yaml_with_default)
    logger.info("[Stage] --> Check all references of top-nodes [OK]")

def convert_format(only_yaml_with_default, top_build_layer):
    logger.info("[Stage] --> Convert to specific YAML format ...")
    final_yaml = converter.convert_to_specific_format(only_yaml_with_default, top_build_layer)
    logger.info("[Stage] --> Convert to specific YAML format [OK]")
    return final_yaml

def generate(final_yaml, final_pattern, tmpls, build_dir):
    logger.info("[Stage] --> Generate source codes & Json file ...")
    generator.generate_json(final_yaml, build_dir)
    generator.generate_code(final_yaml, final_pattern, tmpls, build_dir)
    logger.info("[Stage] --> Generate source codes & Json file [OK]")

def try_to_load_ex_schema_checker(exchecker_path):

    if not os.path.exists(exchecker_path):
        logger.info("Noting to be loaded for extended purpose !")
        return

    import importlib.util
    import sys

    def load_module_from_path(module_name, file_path):
        spec = importlib.util.spec_from_file_location(module_name, file_path)
        module = importlib.util.module_from_spec(spec)
        sys.modules[module_name] = module
        spec.loader.exec_module(module)
        return module

    module_name = 'exChecker' # Fixed for now
    exchecker = load_module_from_path(module_name, exchecker_path)

    if not hasattr(exchecker, 'export'):
        logger.info(f"exChecker must implement the export() method [{exchecker_path}], Ignore exChecker ...")
    else:
        logger.info(f"Loaded exChecker from [{exchecker_path}]")
        schema_mapping.update(exchecker.export())
