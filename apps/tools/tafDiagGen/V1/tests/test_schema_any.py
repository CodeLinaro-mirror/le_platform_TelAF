#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import unittest

import dengine as de
from dengine.exceptions import *


class Test_SchemaSyntax_any(unittest.TestCase):

    def test_any_in_bad_syntax(self):
        schema_test = \
"""
Foo: any('A', key=str())
"""

        custom_test =\
"""
Foo: s_AA
"""
        # sc.transform() <-- syntax error
        with self.assertRaisesRegex(TypeError, r"got an unexpected keyword argument 'key'"):
            de.build_test_combo(schema_test, custom_test)

        schema_test = \
"""
Foo: any()
"""

        custom_test =\
"""
Foo: s_AA
"""
        with self.assertRaisesRegex(Schema_SyntaxError, r"any: need at least one fn-checker to be passed in"):
            de.build_test_combo(schema_test, custom_test)

        schema_test = \
"""
Foo: any('A')
"""

        custom_test =\
"""
Foo: s_AA
"""
        with self.assertRaisesRegex(Schema_SemanticError, r"Foo: 'A' is NOT callable"):
            de.build_test_combo(schema_test, custom_test)

        schema_test = \
"""
Foo: any(list(), map(include('A'), key=str()), int())
---
A:
    AA:
        A: str()
"""

        custom_test =\
"""
Foo: True
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Due to ANY \(StopIteration\)"):
            de.build_test_combo(schema_test, custom_test)

    def test_any_in_normal_cases(self):

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
