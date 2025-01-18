#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

class InvalidSchemaPath(Exception): pass

class InvalidCustomPath(Exception): pass

class NotFoundMappingFunction(Exception):
    def __init__(self, fn_idx):
        msg = f"Not fould fn: [{fn_idx}] in registerd checking-table"
        super().__init__(msg)

class FailToTransform(Exception):
    def __init__(self, leaf):
        msg = f"Fail to transform the [ {leaf.line_rule} ], NOT found any function in line-schema"
        super().__init__(msg)

class DuplicateKeyError(Exception):
    def __init__(self, current_fpath, dup_key, pervious_fpath):
        msg = f"In {current_fpath} already has the key [{dup_key}], " + \
              f"but got again in [{pervious_fpath}]"
        super().__init__(msg)

class CommonCheckerError(Exception):
    def __init__(self, pair):
        msg = f"Expected: [ {pair.leaf.line_label} -> {pair.leaf.line_rule} ], " + \
              f"but got: {pair.node.value}"
        super().__init__(msg)

class ConstructLeafLineSchemaError(Exception):
    def __init__(self, stage, ex, leaf):
        msg = f"In progress -> [{stage}]\n" +\
              f"SyntaxError: {ex.msg}: code[ {ex.text.strip() if ex.text else leaf.line_rule } ]\n" +\
              f"Checking [key = {leaf.line_label}], line: {ex.lineno}, offset: {ex.offset}"
        super().__init__(msg)

class BaseCommonError(Exception):
    def __init__(self, err_msg, **kwargs):
        self.err_msg = err_msg
        msg = f"({self.__class__.__name__}) {err_msg}"
        super().__init__(msg, **kwargs)

class Schema_SyntaxError(BaseCommonError): pass

class Schema_SemanticError(BaseCommonError): pass

class Schema_CheckingError_11(BaseCommonError): pass

class Schema_CheckingError_10(BaseCommonError): pass

class Schema_CheckingError_01(BaseCommonError): pass
