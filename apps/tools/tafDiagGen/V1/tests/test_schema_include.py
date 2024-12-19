#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import unittest
import os, sys

import dengine as de
from dengine.exceptions import *


class Test_SchemaSyntax_include(unittest.TestCase):

    def test_include_bad_loop(self):
        # FIXME: loop-reference
        # Must: A: include('B', key=str(equals='Foo'))
        schema_include = \
"""
Foo: include('A', key=str(), required=True)

---
A: include('B', key=str(equals='Bar'))

---
B:
    AA: str()
    AB:
        AAA: str()
        AAB: str()
"""

        custom_include =\
"""
Foo:
    AA: s_AA
    AB:
        AAA: s_AAA
        AAB: s_AAB
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"None: expected: Bar, but got: Foo"):
            self.assertIsNone(
                de.build_test_combo(schema_include, custom_include)
            )


        # FIXME: Check-Bad-Key in top_node
        # Must: Foo: include('A', key=str(equals='Foo'))
        schema_include = \
"""
Foo: include('A', key=str(equals='Bar'))

---
A:
    AA: str()
    AB:
        AAA: str()
        AAB: str()
"""

        custom_include =\
"""
Foo:
    AA: s_AA
    AB:
        AAA: s_AAA
        AAB: s_AAB
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"None: expected: Bar, but got: Foo"):
            self.assertIsNone(
                de.build_test_combo(schema_include, custom_include)
            )

        schema_include = \
"""
Foo: include('A')
---
A: bool()
"""

        custom_include =\
"""
A:
    AA: s_AA
    AB:
        AAA: s_AAA
        AAB: s_AAB
"""
        with self.assertRaisesRegex(Schema_CheckingError_11, r"A: excepted: bool, but got: {'AA': 's_AA', 'AB': {'AAA': 's_AAA', 'AAB': 's_AAB'}}/dict, in bool\(\)"):
            self.assertIsNone(de.build_test_combo(schema_include, custom_include))



    def test_include_in_normal_cases(self):
        schema_include = \
"""
Foo: include('A', key=str())

---
A:
    AA: str()
    AB:
        AAA: str()
        AAB: str()
"""

        custom_include =\
"""
A:
    AA: s_AA
    AB:
        AAA: s_AAA
        AAB: s_AAB
"""
        self.assertIsNone(
            de.build_test_combo(schema_include, custom_include)
        )

        # include -> key == top_node's name
        schema_include = \
"""
Foo: include('A', key=str(equals='Foo'), required=True)

---
A:
    AA: str()
    AB:
        AAA: str()
        AAB: str()
"""

        custom_include =\
"""
Foo:
    AA: s_AA
    AB:
        AAA: s_AAA
        AAB: s_AAB
"""
        self.assertIsNone(
            de.build_test_combo(schema_include, custom_include)
        )

        # include -> another_include -> key == top_node's name
        schema_include = \
"""
Foo: include('A', key=str(), required=True)

---
A: include('B', key=str(equals='Foo'))

---
B:
    AA: str()
    AB:
        AAA: str()
        AAB: str()
"""

        custom_include =\
"""
Foo:
    AA: s_AA
    AB:
        AAA: s_AAA
        AAB: s_AAB
"""
        self.assertIsNone(
            de.build_test_combo(schema_include, custom_include)
        )

        schema_include = \
"""
Foo: include('A')
---
A: bool()
"""

        custom_include =\
"""
Foo: False
"""
        self.assertIsNone(
            de.build_test_combo(schema_include, custom_include)
        )
