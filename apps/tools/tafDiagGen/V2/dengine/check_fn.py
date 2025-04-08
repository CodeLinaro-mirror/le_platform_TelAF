#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import re

def Enum(*args, etype=str):
    def _Enum(value):
        if type(value) is not etype:
            return False
        if value not in args:
            return False
        return True
    return _Enum

def Int(min = None, max = None):
    def _Int(value):
        if type(value) is not int:
            return False
        if min is not None:
            if value < min:
                return False
        if max is not None:
            if value > max:
                return False
        return True
    return _Int

def Hexa():
    def _Hexa(value):
        if type(value) is not str:
            return False
        return True
    return _Hexa

def Hexa1byte():
    def _Hexa1byte(value):
        if type(value) is not str:
            return False
        return True
    return _Hexa1byte

def Hexa2bytes():
    def _Hexa2bytes(value):
        if type(value) is not str:
            return False
        return True
    return _Hexa2bytes

def Hexa3bytes():
    def _Hexa3bytes(value):
        if type(value) is not str:
            return False
        return True
    return _Hexa3bytes

def Float(min = None, max = None):
    def _Float(value):
        if type(value) is not float:
            return False
        if min is not None:
            if value < min:
                return False
        if max is not None:
            if value > max:
                return False
        return True
    return _Float

def Num(min = None, max = None):
    def _Num(value):
        if type(value) is int:
            checker = Int(min=min, max=max)
        elif type(value) is float:
            checker = Float(min=min, max=max)
        else:
            return False
        return checker(value)
    return _Num

def Bool():
    def _Bool(value):
        if type(value) is not bool:
            return False
        return True
    return _Bool

def Str(equals = None, matches = None, max = None, ignore_case = False, flags = re.DOTALL):

    if equals is not None:
        if type(equals) is not str:
            raise Exception("str 'equals' is not str-type")
        if matches is not None:
            raise Exception("str 'equals' and 'matches' can NOT appear at the same time")
        if max is not None:
            raise Exception("str 'equals' and 'max' can NOT appear at the same time")

    if max is not None:
        if type(max) is not int:
            raise Exception("str 'max' is not int-type")

    def _Str(value):
        if type(value) is not str:
            return False

        if equals is not None:
            if value != equals:
                return False
        else:
            if max is not None:
                if len(value) > max:
                    return False

            if matches is not None:
                if ignore_case is True:
                    nonlocal flags
                    flags = flags | re.IGNORECASE
                found = re.match(matches, value, flags)
                if not found:
                    return False
        return True
    return _Str

def Camelcase():
    def _Camelcase(value):
        if type(value) is not str:
            return False
        return True
    return _Camelcase

def Mnemonic():
    def _Mnemonic(value):
        return isinstance(value, str)
    return _Mnemonic

def ExecutionAuthorizationPattern():
    def _ExecutionAuthorizationPattern(value):
        return isinstance(value, str)
    return _ExecutionAuthorizationPattern

def HexaOrBin():
    def _HexaOrBin(value):
        if value.startswith('0x') or value.startswith('0X'):
            return True
        if value.startswith('0b') or value.startswith('0B'):
            return True

        return False
    return _HexaOrBin
