#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import unittest

import dengine as de
from dengine.exceptions import *


class Test_SchemaSyntax_int(unittest.TestCase):
    def test_int_in_abnormal_cases(self):
        schema_test =\
"""
Foo: int(min=2.0)
"""

        custom_test =\
"""
Foo: 3
"""
        with self.assertRaisesRegex(Schema_SemanticError, r"Foo: min is NOT 'int' type, in int\(min=2.0\)"):
            de.build_test_combo(schema_test, custom_test)

        schema_test =\
"""
Foo: int(min=4)
"""

        custom_test =\
"""
Foo: 3
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: value 3 less-than min value 4"):
            de.build_test_combo(schema_test, custom_test)

        schema_test =\
"""
Foo: int(max=4)
"""

        custom_test =\
"""
Foo: 5
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: value 5 more-than max value 4"):
            de.build_test_combo(schema_test, custom_test)

        schema_test =\
"""
Foo: int(max=4, min=5)
"""

        custom_test =\
"""
Foo: 5
"""
        with self.assertRaisesRegex(Schema_SemanticError, r"max < min is NOT allowed"):
            de.build_test_combo(schema_test, custom_test)

        schema_test =\
"""
Foo: int(max=4)
"""

        custom_test =\
"""
Foo: False
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: value False is NOT 'int' type"):
            de.build_test_combo(schema_test, custom_test)

    def test_int_in_normal_cases(self):

        schema_test =\
"""
Foo: int(min=2, max=5, required=True)

Bar:
    Bar_A:
        Bar_AA: int(min=100)
    Bar_B: int(max=200)
    Bar_C: list(int(min=1, max=3))
    Bar_D: list(int())
"""

        custom_test =\
"""
Foo: 3

Bar:
    Bar_A:
        Bar_AA: 100
    Bar_B: 199
    Bar_C: [1, 3, 2]
    Bar_D:
        - 3
        - 20000
        - -200
"""
        self.assertIsNone( de.build_test_combo(schema_test, custom_test))

