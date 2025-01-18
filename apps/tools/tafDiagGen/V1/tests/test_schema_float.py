#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import unittest

import dengine as de
from dengine.exceptions import *


class Test_SchemaSyntax_float(unittest.TestCase):

    def test_float_in_abnormal_cases(self):
        schema_test =\
"""
Foo: float(min=2, max=5)
"""

        custom_test =\
"""
Foo: 3.0
"""
        with self.assertRaisesRegex(Schema_SemanticError, r"Foo: min is NOT 'float' type, in 'float\(min=2, max=5\)'"):
            de.build_test_combo(schema_test, custom_test)

        schema_test =\
"""
Foo: float(min=5.1, max=3.2)
"""

        custom_test =\
"""
Foo: 4.0
"""
        with self.assertRaisesRegex(Schema_SemanticError, r"Foo: min 5.1 > max 3.2 is NOT allowed"):
            de.build_test_combo(schema_test, custom_test)


        schema_test =\
"""
Foo: float(min=3.1, max=5.0)
"""

        custom_test =\
"""
Foo: 4
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: value 4 is NOT 'float' type"):
            de.build_test_combo(schema_test, custom_test)


    def test_float_in_normal_cases(self):
        schema_test =\
"""
Foo: float(min=2.0, max=5.0, required=True)

Bar:
    Bar_A:
        Bar_AA: float(min=100.5)
    Bar_B: float(max=200.1)
    Bar_C: list(float(min=1.2, max=3.3))
    Bar_D: list(float())
"""

        custom_test =\
"""
Foo: 3.1

Bar:
    Bar_A:
        Bar_AA: 101.1
    Bar_B: 200.0
    Bar_C: [1.5, 3.0, 2.2]
    Bar_D:
        - 3.2
        - 20001.7
        - -200.2
"""
        self.assertIsNone( de.build_test_combo(schema_test, custom_test))

