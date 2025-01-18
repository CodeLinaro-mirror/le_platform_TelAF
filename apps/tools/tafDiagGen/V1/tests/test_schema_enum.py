#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import unittest

import dengine as de
from dengine.exceptions import *


class Test_SchemaSyntax_enum(unittest.TestCase):

    def test_enum_in_abnormal_cases(self):
        schema_test =\
"""
Foo: enum(100)
"""

        custom_test =\
"""
Foo: s_Foo
"""
        with self.assertRaisesRegex(Schema_SemanticError, r"Foo: 'enum' list should be ALL 'str' type, but got: '100/int'"):
            de.build_test_combo(schema_test, custom_test)

        schema_test =\
"""
Foo: enum('AA')
"""

        custom_test =\
"""
Foo: 100
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: 'enum'value expected: 'str' type, but got: '100/int'"):
            de.build_test_combo(schema_test, custom_test)

        schema_test =\
"""
Foo: enum('AA')
"""

        custom_test =\
"""
Foo: CC
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: 'CC' is not in \('AA',\)"):
            de.build_test_combo(schema_test, custom_test)

    def test_enum_in_normal_cases(self):
        schema_test =\
"""
Foo: enum('A', 'B', 'C', required=True)

Bar:
    Bar_A:
        Bar_AA: enum('W','C')
    Bar_B: enum('p', 'v', required=False)
    Bar_C: list(enum('t', 'z', 's'))
    Bar_D: list(enum('m','n','n'))
"""

        custom_test =\
"""
Foo: A

Bar:
    Bar_A:
        Bar_AA: C
    Bar_C: [s, z, t]
    Bar_D:
        - m
        - n
"""
        self.assertIsNone( de.build_test_combo(schema_test, custom_test))
