#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import unittest

import dengine as de
from dengine.exceptions import *

class Test_SchemaSyntax_str(unittest.TestCase):

    def test_str_in_abnormal_cases(self):
        schema_test =\
"""
Foo: str(equals="AAA", max=100)
"""

        custom_test =\
"""
Foo: BBB
"""
        with self.assertRaisesRegex(Schema_SemanticError, r"Foo: str 'equals' and 'max' can NOT appear at the same time"):
            de.build_test_combo(schema_test, custom_test)

        schema_test =\
"""
Foo: str(equals="AAA", matches="\w")
"""

        custom_test =\
"""
Foo: BBB
"""
        with self.assertRaisesRegex(Schema_SemanticError, r"Foo: str 'equals' and 'matches' can NOT appear at the same time"):
            de.build_test_combo(schema_test, custom_test)

        schema_test =\
"""
Foo: str(matches="\w")
"""

        custom_test =\
"""
Foo: 1000
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: value is NOT 'str' type"):
            de.build_test_combo(schema_test, custom_test)

        schema_test =\
"""
Foo: str(equals=False)
"""

        custom_test =\
"""
Foo: s_Foo
"""
        with self.assertRaisesRegex(Schema_SemanticError, r"Foo: 'equals' -> 'bool' is NOT 'str' type"):
            de.build_test_combo(schema_test, custom_test)

        schema_test =\
"""
Foo: str(equals="CCC")
"""

        custom_test =\
"""
Foo: s_Foo
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: expected: CCC, but got: s_Foo"):
            de.build_test_combo(schema_test, custom_test)

        schema_test =\
"""
Foo: str(matches="\W")
"""

        custom_test =\
"""
Foo: 100St
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: value 100St is not matched: \\W, ignore_case: False"):
            de.build_test_combo(schema_test, custom_test)

        schema_test =\
"""
Foo: str(matches="aaBB\w")
"""

        custom_test =\
"""
Foo: BBaa
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: value BBaa is not matched: aaBB\\w, ignore_case: False"):
            de.build_test_combo(schema_test, custom_test)


    def test_str_in_normal_cases(self):

        schema_test =\
"""
Foo: str(matches='^([\w\.]+)$', max=100, ignore_case=True, required=True)

Bar:
    Bar_A:
        Bar_AA: str(equals="hello/test")
    Bar_B: str()
    Bar_C: list(str(matches='^[A-Z\d_]+$', max=3))
    Bar_D: list(str(equals="AAA"), str(matches="BBB", ignore_case=True), str(matches="CCC", ignore_case=False))
"""

        custom_test =\
"""
Foo: echo.name

Bar:
    Bar_A:
        Bar_AA: hello/test
    Bar_B: any.string.in.here
    Bar_C: [DDD, A, WW]
    Bar_D:
        - CCC
        - AAA
        - bBb
"""
        self.assertIsNone( de.build_test_combo(schema_test, custom_test))


