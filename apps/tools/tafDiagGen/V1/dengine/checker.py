#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import re, os
import importlib
import inspect
from functools import wraps

import traceback, pdb
from .exceptions import *
from .logger import checking_backtrace, trace_message, logc


def fn_origin(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        sig = inspect.signature(func)

        bound_args = sig.bind(*args, **kwargs)
        bound_args.apply_defaults()

        positionals = []
        for i, (param_name, param) in enumerate(sig.parameters.items()):
            if i < len(args):
                positionals.append(repr(args[i]))
            elif param.default is not param.empty:
                break

        keywords = []
        for param_name, param in sig.parameters.items():
            if param_name in kwargs:
                keywords.append(f"{param_name}={kwargs[param_name]}")
            elif param_name in bound_args.arguments and param.default is not param.empty:
                value = bound_args.arguments[param_name]
                if value != param.default:
                    keywords.append(f"{param_name}={value}")

        args_str = ', '.join(positionals + keywords)

        result = func(*args, **kwargs)

        result.required = getattr(result, 'required', True) # default -> True for each key

        result.signature = f"{func.__name__[:-2]}({args_str})"

        return result
    return wrapper

# --- registered checking functions ---

# For each key-value pair, the checker for the key shouldn't be anyone in this list
NON_FLAT_CHECKER_LIST = ['subset', 'map', 'include', 'any', 'list']

# 'null' is specifical, cause of the 'key' shouldn't be empty.
NON_FLAT_CHECKER_LIST.append('null')

# TBD: support: 'min' & 'max' for subset
@fn_origin
def subset__(*fn_list, key=None, required = True):
    def subset_x(pair):
        trace_message(f"{pair.node.key}/{subset_x.signature}")
        trace_message(f"(subset total [{len(pair.node.value)}] items)")
        # if pair.node.key == "did_accessibility":
        #     pdb.set_trace()
        for idx, (this_key, this_value) in enumerate(pair.node.value.items()):

            # For 'subset', if the 'key' is mismatched, we need to stop there and debugging
            if key is not None:

                if not callable(key):
                    raise Schema_SyntaxError(f"'subset': 'key' parameter should be callable, but got {type(key).__name__}/{key}")

                if key.__name__ in NON_FLAT_CHECKER_LIST:
                    raise Schema_SemanticError(f"'key' checker should be flat, but '{key.__name__}' isn't")

                pair.push(None, this_key)
                try:
                    key(pair)
                finally:
                    pair.pop()

            pair.push(this_key, this_value)

            for fn in fn_list:
                with checking_backtrace(f"(subset/item[{idx}]/key): <{this_key}/{hex(this_key) if type(this_key) is int else ''}>") as wc:
                    try:
                        trace_message(f"(subset/item[{idx}]/fn: {fn.signature}")
                        fn(pair)
                    except Exception as e:
                        # traceback.print_exc()
                        trace_message(f"|<-- (Error): {e}", tweak_indent=wc.level)
                        continue
                    else:
                        break
            else:
                trace_message(f"|<-- (Error) : SUBSET none matched: STOP")
                raise Schema_CheckingError_11("Due to SUBSET (StopIteration)")

            pair.pop()
        else:
            trace_message(f"(subset) ALL DONE")
    subset_x.__name__ = "subset"
    subset_x.required = required
    return subset_x


@fn_origin
def map__(include_ref, key = None, min = None, max = None, required = True):
    def map_x(pair):

        if not callable(include_ref):
            raise Schema_SyntaxError(f"'map': position parameter should be callable, but got {type(include_ref).__name__}/{include_ref}")

        if include_ref.__name__ != "include":
            raise Schema_SemanticError(f"'map': expected: 'include', but got: '{include_ref.__name__}'")

        # Prompt: size < 0, checked in AST stage
        if (min is not None) and (max is not None):
            if int(min) == 0:
                raise Schema_SemanticError(f"'map': 'min' value shouldn't be ZERO")

            if int(max) == 0:
                raise Schema_SemanticError(f"'map': 'max' value shouldn't be ZERO")

            if int(min) > int(max):
                raise Schema_SemanticError(f"'map': 'min' > 'max' is not allowd")

            if len(pair.node.value) < int(min) or len(pair.node.value) > int(max):
                raise Schema_CheckingError_11(f"[{pair.node.key}] -> expected: {min} < len(items) < {max}, but got: {len(pair.node.value)}")

        elif min is not None:
            if int(min) == 0:
                raise Schema_SemanticError(f"'map': 'min' value shouldn't be ZERO")

            if len(pair.node.value) < int(min):
                raise Schema_CheckingError_11(f"[{pair.node.key}] -> expected: min = {min} items, but got: {len(pair.node.value)}")

        elif max is not None:
            if int(max) == 0:
                raise Schema_SemanticError(f"'map': 'max' value shouldn't be ZERO")

            if len(pair.node.value) > int(max):
                raise Schema_CheckingError_11(f"[{pair.node.key}] -> expected: max = {max} items, but got: {len(pair.node.value)}")

        elif (min is None) and (max is None):
            pass # don't care, just base on the 'pair.node.value'

        if getattr(pair.node.value, 'values', None) is None:
            raise Schema_CheckingError_11(f"{pair.node.key}: value is not dict in 'map'")

        if not all([isinstance(val, dict) for val in pair.node.value.values()]):
            raise Schema_CheckingError_11(f"'map' only allows 'key-value-dict'")

        with checking_backtrace(f"(map/multiple: check key) {pair.node.key}"):
            for _key, _val in pair.node.value.items():
                trace_message(f"key: {_key}/{hex(_key) if type(_key) is int else ''}")

                if key is not None:

                    if not callable(key):
                        raise Schema_SyntaxError(f"'map': 'key' parameter should be callable, but got {type(key).__name__}/{key}")

                    if key.__name__ in NON_FLAT_CHECKER_LIST:
                        raise Schema_SemanticError(f"'key' checker should be flat, but '{key.__name__}' isn't")

                    pair.push(None, _key)
                    try:
                        key(pair)
                    finally: # Make sure the pair.node has been restored, the same as following
                        pair.pop()

                pair.push(_key, _val)
                try:
                    include_ref(pair)
                finally:
                    pair.pop()

    map_x.__name__ = "map"
    map_x.required = required
    return map_x


@fn_origin
def include__(to_another, key = None, required = True):
    def include_x(pair):

        # if to_another == "diag_session_ref17_item":
        #     pdb.set_trace()

        # if to_another == "did_diagnostic_session_item":
        #     pdb.set_trace()

        if key is not None:

            if not callable(key):
                raise Schema_SyntaxError(f"'include': 'key' parameter should be callable, but got {type(key).__name__}/{key}")

            if key.__name__ in NON_FLAT_CHECKER_LIST:
                raise Schema_SemanticError(f"'key' checker should be flat, but '{key.__name__}' isn't")

            with checking_backtrace(f"(include/key) {pair.node.key}"):
                trace_message(f"{pair.node.key}/{key.signature}")
                pair.push(None, pair.node.key)
                try:
                    key(pair)
                finally:
                    pair.pop()

        with checking_backtrace(f"(include/to_another) {include_x.signature}"):
            pair.leaf.top_nodes[to_another].check(pair.pending_node)

    include_x.__name__ = "include"
    include_x.required = required
    return include_x


@fn_origin
def any__(*fn_list, required = True):
    def any_x(pair):

        if len(fn_list) == 0:
            raise Schema_SyntaxError(f"any: need at least one fn-checker to be passed in")

        for fn in fn_list:

            if not callable(fn):
                raise Schema_SemanticError(f"{pair.node.key}: '{fn}' is NOT callable in {any_x.signature}")

            with checking_backtrace(f"(any/fn:{fn.__name__})") as wc:
                try:
                    trace_message(f"{fn.signature}")
                    fn(pair)
                except Exception as e:
                    # traceback.print_exc()
                    trace_message(f"|<-- (Error): {e}", tweak_indent=wc.level)
                    continue
                else: # match one handler for this node
                    trace_message(f"(any/fn:{fn.__name__}) MATCHED (SUCCESS)")
                    break

        else:
            trace_message(f"|<-- (Error) : ANY none matched: STOP")
            raise Schema_CheckingError_11("Due to ANY (StopIteration)")

    any_x.__name__ = 'any'
    any_x.required = required
    return any_x


@fn_origin
def list__(*fn_list, min = None, max = None, required = True):
    def list_x(pair):

        if min is not None and max is not None:
            if type(min) is not int or type(max) is not int:
                raise Schema_SemanticError(f"{pair.node.key}: min or max should be 'int', {list_x.signature}")
            if int(min) > int(max):
                raise Schema_CheckingError_11(f"{pair.node.key}: min ({int(min)}) > max ({int(max)})")

        if type(pair.node.value) is not list:
            raise Schema_CheckingError_11(f"{pair.node.key}: value '{pair.node.value}' is not list type")

        if min is not None:
            if type(min) is not int or int(min) < 0:
                raise Schema_SemanticError(f"{pair.node.key}: min should be 'int' & min >= 0, {list_x.signature}")
            if len(pair.node.value) < int(min):
                raise Schema_CheckingError_11(f"{pair.node.key}: expected: min = {min} items, but got: {len(pair.node.value)}, in {pair.node.value}")

        if max is not None:
            if type(max) is not int or int(max) < 0:
                raise Schema_SemanticError(f"{pair.node.key}: max should be 'int' & max >= 0, {list_x.signature}")
            if len(pair.node.value) > int(max):
                raise Schema_CheckingError_11(f"{pair.node.key}: expected: max = {max} items, but got: {len(pair.node.value)}, in {pair.node.value}")

        # Note: Support empty-list, so 'min' & 'max' can be set to '0'

        for fn in fn_list:
            if not callable(fn):
                raise Schema_SyntaxError(f"{pair.node.key}: '{fn}' is not callable, in {list_x.signature}")

        trace_message(f"(list/total[{len(pair.node.value)}])")

        for idx, val in enumerate(pair.node.value):
            pair.push(idx, val)
            with checking_backtrace(f"(list/[{idx}])") as wc:
                for fn in fn_list:
                    try:
                        fn(pair)
                    except Exception as e:
                        trace_message(f"|<-- (Error): {e}", tweak_indent=wc.level)
                        continue
                    else:
                        trace_message(f"(list/{idx}) MATCHED (SUCCESS)")
                        break
                else:
                    trace_message(f"|<-- (Error) : LIST none matched: STOP")
                    raise Schema_CheckingError_11("Due to LIST (StopIteration)")
            pair.pop()

    list_x.__name__ = 'list'
    list_x.required = required
    return list_x


@fn_origin
def int__(min = None, max = None, required = True):
    def int_x(pair):
        # Note that: the 'bool' is subclass of 'int'
        # Use 'type()' rather than 'isinstance'
        #
        # (Pdb) isinstance(True, int)
        # True
        # (Pdb) isinstance(False, int)
        # True
        #
        # (Pdb) type(True) is int
        # False
        # (Pdb) type(True) is bool
        # True

        if min is not None and max is not None:
            if type(min) is not int:
                raise Schema_SemanticError(f"{pair.node.key}: min is NOT 'int' type, in {int_x.signature}")
            if type(max) is not int:
                raise Schema_SemanticError(f"{pair.node.key}: max is NOT 'int' type, in {int_x.signature}")
            if min > max:
                raise Schema_SemanticError(f"{pair.node.key}: max < min is NOT allowed, in {int_x.signature}")

        if type(pair.node.value) is not int:
            raise Schema_CheckingError_11(f"{pair.node.key}: value {pair.node.value} is NOT 'int' type")

        if min is not None:
            if type(min) is not int:
                raise Schema_SemanticError(f"{pair.node.key}: min is NOT 'int' type, in {int_x.signature}")
            if int(pair.node.value) < int(min):
                raise Schema_CheckingError_11(f"{pair.node.key}: value {pair.node.value} less-than min value {min}, in {int_x.signature}")

        if max is not None:
            if type(max) is not int:
                raise Schema_SemanticError(f"{pair.node.key}: max is NOT 'int' type, in {int_x.signature}")
            if int(pair.node.value) > int(max):
                raise Schema_CheckingError_11(f"{pair.node.key}: value {pair.node.value} more-than max value {max}, in {int_x.signature}")

    int_x.__name__ = 'int'
    int_x.required = required
    return int_x


@fn_origin
def null__(required = True):
    def null_x(pair):
        if pair.node.value is not None:
            raise Schema_CheckingError_11(f"{pair.node.key}: expected: {null_x.signature}, but got: {pair.node.value}")
    null_x.__name__ = 'null'
    null_x.required = required
    return null_x


@fn_origin
def float__(min = None, max = None, required = True):
    def float_x(pair):

        if min is not None and max is not None:
            if type(min) is not float:
                raise Schema_SemanticError(f"{pair.node.key}: min is NOT 'float' type, in '{float_x.signature}'")
            if type(max) is not float:
                raise Schema_SemanticError(f"{pair.node.key}: max is NOT 'float' type, in '{float_x.signature}'")
            if min > max:
                raise Schema_SemanticError(f"{pair.node.key}: min {float(min)} > max {float(max)} is NOT allowed, in '{pair.leaf.line_rule}'")

        if type(pair.node.value) is not float:
            raise Schema_CheckingError_11(f"{pair.node.key}: value {pair.node.value} is NOT 'float' type")

        if min is not None:
            if type(min) is not float:
                raise Schema_SemanticError(f"{pair.node.key}: min is NOT 'float' type, in '{float_x.signature}'")
            if float(pair.node.value) < float(min):
                raise Schema_CheckingError_11(f"{pair.node.key}: value {float(pair.node.value)} < min {float(min)}")

        if max is not None:
            if type(max) is not float:
                raise Schema_SemanticError(f"{pair.node.key}: max is NOT 'float' type, in '{float_x.signature}'")
            if float(pair.node.value) > float(max):
                raise Schema_CheckingError_11(f"{pair.node.key}: value {float(pair.node.value)} > max {float(min)}")

    float_x.__name__ = "float"
    float_x.required = required
    return float_x


@fn_origin
def enum__(*enum_list, etype='str', required = True):
    def enum_x(pair):

        etype_supported = ['str', 'int', 'float']

        if etype not in etype_supported:
            raise Schema_SyntaxError(f"{pair.node.key}: 'enum' don't support {etype}, only {etype_supported}")

        for e in enum_list:
            if type(e) is not eval(etype):
                raise Schema_SemanticError(f"{pair.node.key}: 'enum' list should be ALL '{etype}' type, but got: '{e}/{type(e).__name__}'")

        if type(pair.node.value) is not eval(etype):
            raise Schema_CheckingError_11(f"{pair.node.key}: 'enum'value expected: '{etype}' type, but got: '{pair.node.value}/{type(pair.node.value).__name__}'")

        if pair.node.value not in enum_list:
            raise Schema_CheckingError_11(f"{pair.node.key}: '{pair.node.value}' is not in {enum_list}")

    enum_x.__name__ = "enum"
    enum_x.required = required
    return enum_x


@fn_origin
def bool__(required = True):
    def bool_x(pair):

        if type(pair.node.value) is not bool:
            raise Schema_CheckingError_11(f"{pair.node.key}: excepted: bool, but got: {pair.node.value}/{type(pair.node.value).__name__}, in {bool_x.signature}")

    bool_x.__name__ = "bool"
    bool_x.required = required
    return bool_x


@fn_origin
def str__(equals = None, matches = None, max = None, ignore_case = False,
          flags = re.DOTALL, required = True):
    def str_x(pair):

        if equals is not None:
            if matches is not None:
                raise Schema_SemanticError(f"{pair.node.key}: str 'equals' and 'matches' can NOT appear at the same time, in '{pair.leaf.line_rule}'")
            if max is not None:
                raise Schema_SemanticError(f"{pair.node.key}: str 'equals' and 'max' can NOT appear at the same time, in '{pair.leaf.line_rule}'")

        if type(pair.node.value) is not str:
            raise Schema_CheckingError_11(f"{pair.node.key}: value is NOT 'str' type")

        # If 'equals' present, ignore 'ignore_case' and 'flags' <--

        if equals is not None:
            if type(equals) is not str:
                raise Schema_SemanticError(f"{pair.node.key}: 'equals' -> '{type(equals).__name__}' is NOT 'str' type, in {str_x.signature}")
            if str(equals) != pair.node.value:
                raise Schema_CheckingError_11(f"{pair.node.key}: expected: {equals}, but got: {pair.node.value}")
        else:
            if max is not None:
                if type(max) is not int:
                    raise Schema_SemanticError(f"{pair.node.key}: 'max' is NOT 'int' type, in {str_x.signature}")
                if len(pair.node.value) > int(max):
                    raise Schema_CheckingError_11(f"{pair.node.key}: size of value '{pair.node.value}' is more than 'max' {max}")

            if matches is not None:
                if ignore_case is True:
                    nonlocal flags
                    flags = flags | re.IGNORECASE
                # Use 'match' to search from the first char
                found = re.match(matches, pair.node.value, flags)
                if not found:
                    raise Schema_CheckingError_11(f"{pair.node.key}: value {pair.node.value} is not matched: {matches}, ignore_case: {ignore_case}")

    str_x.__name__ = "str"
    str_x.required = required
    return str_x


class Checker():
    def __init__(self):
        self.table = {
            # -- combination --
            'subset' :  subset__ ,
            'include' : include__,
            'map' :     map__,
            'any' :     any__,
            'list' :    list__,
            # -- flat --
            'str' :     str__,
            'int' :     int__,
            'float' :   float__,
            'enum' :    enum__,
            'bool' :    bool__,
            'null' :    null__,
        }

        # subclasses = {
        #   "module_name" : [subclass_obj, ...]
        #   "module_name_" : [subclass_obj_, ...]
        # }
        self.subclass = {}

    def __getitem__(self, index):
        found_in_plugin = False
        for module_name, sub_cls_objs in self.subclass.items():
            logc.debug(f"Seaching module: {module_name}")
            for sub_cls_object in sub_cls_objs:
                logc.debug(f"Indexing object: {sub_cls_object}")
                try:
                    return sub_cls_object.table[index]
                except KeyError:
                    logc.debug(f"Not fould fn: [{index}], continue...")
                    continue
                else:
                    logc.debug(f"Found fn: [{index}] in [{module_name}/{sub_cls_object}]")
                    found_in_plugin = True
                    break # First-Matching

        if found_in_plugin is False:
            try:
                return self.table[index]
            except KeyError:
                raise NotFoundMappingFunction(index)

    def _load_plugin(self, module_name, module_path):
        spec = importlib.util.spec_from_file_location(module_name, module_path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)

        self.subclass[module_name] = []

        for attribute_name in dir(module):
            class_attribute = getattr(module, attribute_name)
            if isinstance(class_attribute, type) \
                and issubclass(class_attribute, Checker) \
                and class_attribute is not Checker:
                logc.debug(f"{module_name} <-- loading...")
                self.subclass[module_name].append(class_attribute())

    def extend(self, x_path):

        from pathlib import Path
        x_path_abs = Path(x_path).resolve()
        logc.debug(f"Extended path: {x_path_abs}")

        for filename in os.listdir(x_path_abs):
            if filename.endswith('.py') and filename != '__init__.py':
                module_name = filename[:-3]  # Remove the .py extension
                module_path = os.path.join(x_path_abs, filename)
                self._load_plugin(module_name, module_path)
