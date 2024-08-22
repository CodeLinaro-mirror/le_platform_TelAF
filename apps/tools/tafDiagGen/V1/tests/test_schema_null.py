#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import unittest

import dengine as de
from dengine.exceptions import *


class Test_SchemaSyntax_null(unittest.TestCase):

    def test_null_in_abnormal_cases(self):
        schema_test =\
"""
Bar: null()
"""

        custom_test =\
"""
Bar: False
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"expected: null\(\), but got: False"):
            de.build_test_combo(schema_test, custom_test)

    def test_null_in_normal_cases(self):
        schema_test =\
"""
Foo: null()

Bar:
    Bar_A:
        Bar_AA: null(required=False)
    Bar_B: null()
    Bar_C: list(null())
    Bar_D: list(null())
"""

        custom_test =\
"""
Foo:

Bar:
    Bar_A:
        Bar_AA:
    Bar_B:
    Bar_C: []
    Bar_D:
        -
        -
"""
        self.assertIsNone(de.build_test_combo(schema_test, custom_test))

