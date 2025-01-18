#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import unittest

import dengine as de
from dengine.exceptions import *


class Test_SchemaSyntax_bool(unittest.TestCase):
    def test_bool_in_abnormal_cases(self):
        schema_test =\
"""
Foo: bool()
"""

        custom_test =\
"""
Foo: 100
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"Foo: excepted: bool, but got: 100/int, in bool\(\)"):
            de.build_test_combo(schema_test, custom_test)


    def test_bool_in_normal_cases(self):
        schema_test =\
"""
Foo: bool()

Bar:
    Bar_A:
        Bar_AA: bool()
    Bar_B: bool()
    Bar_C: list(bool())
    Bar_D: list(bool())
"""

        custom_test =\
"""
Foo: True

Bar:
    Bar_A:
        Bar_AA: True
    Bar_B: False
    Bar_C: [False, True, False]
    Bar_D:
        - False
        - True
        - True
"""
        self.assertIsNone( de.build_test_combo(schema_test, custom_test))

