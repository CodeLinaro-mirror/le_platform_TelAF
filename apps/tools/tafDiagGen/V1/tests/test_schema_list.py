#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import unittest

import dengine as de
from dengine.exceptions import *


class Test_SchemaSyntax_list(unittest.TestCase):

    def test_list_in_bad_syntax(self):

        schema_test = \
"""
Foo: list(bool(), min=1.0, max=2.0)
"""

        custom_test =\
"""
Foo: [False]
"""
        with self.assertRaisesRegex(Schema_SemanticError, r"Foo: min or max should be 'int'"):
            de.build_test_combo(schema_test, custom_test)

        schema_test = \
"""
Foo: list(bool(), min=3, max=2)
"""

        custom_test =\
"""
Foo: []
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: min \(3\) > max \(2\)"):
            de.build_test_combo(schema_test, custom_test)

        schema_test = \
"""
Foo: list(bool(), min=1.0)
"""

        custom_test =\
"""
Foo: s_Foo
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: value 's_Foo' is not list type"):
            de.build_test_combo(schema_test, custom_test)


        schema_test = \
"""
Foo: list(bool(), min=1)
"""

        custom_test =\
"""
Foo: []
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: expected: min = 1 items, but got: 0"):
            de.build_test_combo(schema_test, custom_test)

        schema_test = \
"""
Foo: list(bool(), max=0)
"""

        custom_test =\
"""
Foo: [False]
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: expected: max = 0 items, but got: 1"):
            de.build_test_combo(schema_test, custom_test)

        schema_test = \
"""
Foo: list('A', 'B')
"""

        custom_test =\
"""
Foo:
       A: s_A
       B: 123
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: value '{'A': 's_A', 'B': 123}' is not list type"):
            de.build_test_combo(schema_test, custom_test)

        schema_test = \
"""
Foo: list('A')
"""

        custom_test =\
"""
Foo: []
"""
        with self.assertRaisesRegex(Schema_SyntaxError, r"Foo: 'A' is not callable, in list\('A'\)"):
            de.build_test_combo(schema_test, custom_test)

        schema_test = \
"""
Foo: list(str(), include("A"), float())
"""

        custom_test =\
"""
Foo: [True]
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Due to LIST \(StopIteration\)"):
            de.build_test_combo(schema_test, custom_test)


    def test_list_in_normal_cases(self):

        # Only match the last one
        schema_test = \
"""
Foo: any(list(), bool(), map(include("A"), key=str()), include("A", str()))

---
A:
    AA: str()
    AB:
        AAA: str()
        AAB: str()
"""

        custom_test =\
"""
Foo:
    AA: s_AA
    AB:
        AAA: s_AAA
        AAB: s_AAB
"""
        self.assertIsNone(de.build_test_combo(schema_test, custom_test))

        schema_test = \
"""
Foo: list(bool(), min=0)
"""

        custom_test =\
"""
Foo: []
"""
        self.assertIsNone(de.build_test_combo(schema_test, custom_test))

        schema_test = \
"""
Foo: list(bool(), max=0)
"""

        custom_test =\
"""
Foo: []
"""
        self.assertIsNone(de.build_test_combo(schema_test, custom_test))

        schema_test = \
"""
Foo: list(bool(), max=1)
"""

        custom_test =\
"""
Foo: []
"""
        self.assertIsNone(de.build_test_combo(schema_test, custom_test))

        schema_test = \
"""
Foo: list(bool(), min=1, max=2)
"""

        custom_test =\
"""
Foo: [False]
"""
        self.assertIsNone(de.build_test_combo(schema_test, custom_test))
