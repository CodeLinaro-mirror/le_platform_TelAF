#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

# This module cannot be used independently, can only be imported as a plug-in-module by checker.py
# All function symbols can override the same name in checker.py

import re

from dengine import Checker, fn_origin
from dengine.exceptions import *

PATTERN_HEXA_BINARY_OCTAL_YAML_FORMAT = r'(\d[xbo][\dA-F]+)'
PATTERN_HEXA_YAML_FORMAT = r'\dx[\dA-F]+'
PATTERN_HEXA_1_BYTE_YAML_FORMAT = r'\dx[\dA-F]{2}'
PATTERN_HEXA_2_BYTES_YAML_FORMAT = r'\dx[\dA-F]{4}'
PATTERN_HEXA_3_BYTES_YAML_FORMAT = r'\dx[\dA-F]{6}'
PATTERN_BINARY_YAML_FORMAT = r'(0b([0-1]+)'
YAML_EXTENSION = '.yaml'

unit_list_accepted = ['s', 'm', 'h', 'kph', 'V', 'km', '%']

P_DOMAIN_NAME = r'^[-\w]+$'  # Domain name pattern
P_MNEMONIC = r'^[-_\w\(\)\.]{1,70}$'  # Mnemonic name pattern
P_CAMEL_CASE = r'(^[a-z]|[A-Z0-9])[a-zA-Z]*'  # Camel case-sensitive pattern
P_SOURCE_CODE_FUNCTION_CASE = r'[a-z]+(?:[A-Z][a-z0-9]+)+'  # Source code function case-sensitive pattern
P_YAML_RANGE = r'\(\s?\'?' + PATTERN_HEXA_YAML_FORMAT + r'\'?..\'?' + PATTERN_HEXA_YAML_FORMAT + r'\'?\s?\)'
P_SW_CONTEXT_PATH = r'/?' + P_CAMEL_CASE + r'(?:/' + P_SOURCE_CODE_FUNCTION_CASE + r')+'

# https://semver.org/
PATTERN_SEMVER_FORMAT = r'^(?P<major>0|[1-9]\d*)\.(?P<minor>0|[1-9]\d*)\.(?P<patch>0|[1-9]\d*)(?:-(?P<prerelease>(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+(?P<buildmetadata>[0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$'

@fn_origin
def mnemonic__(required = True):
    def mnemonic_x(pair):
        if not ( isinstance(pair.node.value, str) and re.match(P_MNEMONIC, pair.node.value) ):
           raise CommonCheckerError(pair)
    mnemonic_x.__name__ = 'mnemonic'
    mnemonic_x.required = required
    return mnemonic_x

@fn_origin
def hexa__(required = True):
    # hex_pattern = re.compile(r'^0x[0-9a-fA-F]+$')
    def hexa_x(pair):
        # if not hex_pattern.match(pair.node.value):
        if not isinstance(pair.node.value, int):
           raise CommonCheckerError(pair)
    hexa_x.__name__ = 'hexa'
    hexa_x.required = required
    return hexa_x

@fn_origin
def camelcase__(required = True):
    def camelcase_x(pair):
        if not ( isinstance(pair.node.value, str) and re.match(P_CAMEL_CASE, pair.node.value) ):
           raise CommonCheckerError(pair)
    camelcase_x.__name__ = 'camelcase'
    camelcase_x.required = required
    return camelcase_x

@fn_origin
def domain_name__(required = True):
    def domain_name_x(pair):
        if not ( isinstance(pair.node.value, str) and re.match(P_DOMAIN_NAME, pair.node.value) ):
           raise CommonCheckerError(pair)
    domain_name_x.__name__ = "domain_name"
    domain_name_x.required = required
    return domain_name_x

@fn_origin
def FunctionCase__(required = True):
    def FunctionCase_x(pair):

        if not ( isinstance(pair.node.value, str) and re.match(P_SOURCE_CODE_FUNCTION_CASE, pair.node.value) ):
           raise CommonCheckerError(pair)
    FunctionCase_x.__name__ = "FunctionCase"
    FunctionCase_x.required = required
    return FunctionCase_x

@fn_origin
def swCtxPath__(required  = True):
    def swCtxPath_x(pair):
        if not ( isinstance(pair.node.value, str) and re.match(P_SW_CONTEXT_PATH, pair.node.value) ):
           raise CommonCheckerError(pair)
    swCtxPath_x.__name__ = "swCtxPath"
    swCtxPath_x.required = required
    return swCtxPath_x

@fn_origin
def hexa_bool__(required  = True):
    def hexa_bool_x(pair):
        if not isinstance(pair.node.value, int) or int(pair.node.value) not in [0, 1]:
           raise CommonCheckerError(pair)
    hexa_bool_x.__name__ = "hexa_bool"
    hexa_bool_x.required = required
    return hexa_bool_x

@fn_origin
def unit__(required = True):
    unit_list_accepted = ['s', 'm', 'h', 'kph', 'V', 'km', '%']
    def unit_x(pair):
        if pair.node.value not in unit_list_accepted:
            raise CommonCheckerError(pair)
    unit_x.__name__ = "unit"
    unit_x.required = required
    return unit_x

@fn_origin
def HexaOrBin__(required = True):
    def HexaOrBin_x(pair):
        if not ( isinstance(pair.node.value, str) and re.match(PATTERN_HEXA_BINARY_OCTAL_YAML_FORMAT, pair.node.value) ):
            raise CommonCheckerError(pair)
    HexaOrBin_x.__name__ = "HexaOrBin"
    HexaOrBin_x.required = required
    return HexaOrBin_x

@fn_origin
def HexaRange__(required = True):
    def HexaRange_x(pair):
        if not ( isinstance(pair.node.value, str) and re.match(P_YAML_RANGE, pair.node.value) ):
            raise CommonCheckerError(pair)
    HexaRange_x.__name__ = "HexaRange"
    HexaRange_x.required = required
    return HexaRange_x

@fn_origin
def hexa3bytes__(required = True):
    def hexa3bytes_x(pair):
        # if not ( isinstance(pair.node.value, str) and re.match(PATTERN_HEXA_3_BYTES_YAML_FORMAT, pair.node.value) ):
        #     raise CommonCheckerError(pair)
        if not isinstance(pair.node.value, int):
           raise CommonCheckerError(pair)
    hexa3bytes_x.__name__ = "hexa3bytes"
    hexa3bytes_x.required = required
    return hexa3bytes_x

@fn_origin
def hexa2bytes__(required = True):
    def hexa2bytes_x(pair):
        # if not ( isinstance(pair.node.value, str) and re.match(PATTERN_HEXA_2_BYTES_YAML_FORMAT, pair.node.value) ):
        #     raise CommonCheckerError(pair)
        if not isinstance(pair.node.value, int):
           raise CommonCheckerError(pair)
    hexa2bytes_x.__name__ = "hexa2bytes"
    hexa2bytes_x.required = required
    return hexa2bytes_x

@fn_origin
def hexa1byte__(required = True):
    def hexa1byte_x(pair):
        # if not ( isinstance(pair.node.value, str) and re.match(PATTERN_HEXA_1_BYTE_YAML_FORMAT, pair.node.value) ):
        #     raise CommonCheckerError(pair)
        if not isinstance(pair.node.value, int):
           raise CommonCheckerError(pair)
    hexa1byte_x.__name__ = "hexa1byte"
    hexa1byte_x.required = required
    return hexa1byte_x

@fn_origin
def semver__(required = True):
    def semver_x(pair):
        if not ( isinstance(pair.node.value, str) and re.match(PATTERN_SEMVER_FORMAT, pair.node.value) ):
            raise CommonCheckerError(pair)
    semver_x.__name__ = "semver"
    semver_x.required = required
    return semver_x

@fn_origin
def ExecutionAuthorizationPattern__(required = True):
    def ExecutionAuthorizationPattern_x(pair):
        pass
    ExecutionAuthorizationPattern_x.__name__ = "ExecutionAuthorizationPattern"
    ExecutionAuthorizationPattern_x.required = required
    return ExecutionAuthorizationPattern_x

@fn_origin
def CapsNDash__(required = True):
    def CapsNDash_x(pair):
        pass
    CapsNDash_x.__name__ = "CapsNDash"
    CapsNDash_x.required = required
    return CapsNDash_x

class ExChecker(Checker):
    def __init__(self):
        self.table = \
        {
            'mnemonic' : mnemonic__,
            'hexa' : hexa__,
            'camelcase' : camelcase__,
            'domain_name' : domain_name__,
            'FunctionCase' : FunctionCase__,
            'swCtxPath' : swCtxPath__,
            'hexa_bool' : hexa_bool__,
            'unit' : unit__,
            'HexaOrBin': HexaOrBin__,
            'HexaRange': HexaRange__,
            'hexa3bytes': hexa3bytes__,
            'hexa2bytes': hexa2bytes__,
            'hexa1byte' : hexa1byte__,
            'semver': semver__,
            'ExecutionAuthorizationPattern' : ExecutionAuthorizationPattern__,
            'CapsNDash': CapsNDash__,
        }
