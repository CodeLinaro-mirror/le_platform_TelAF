#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import unittest
import os, sys

import dengine as de
from dengine.exceptions import *


class Test_SchemaSyntax_map(unittest.TestCase):

    schema_map_tmpl =\
"""
Foo: map({include_fn}, key={key_fn}, min={min_val}, max={max_val})
---
Foo_item:
    A:
        AA: str()
        AB: bool()
    B: str()
    C: bool()
"""

    custom_map_tmpl =\
"""
Foo:
    Foo_A:
        A:
            AA: s_AA
            AB: True
        B: s_B
        C: False
    Foo_B:
        A:
            AA: s_B_AA
            AB: False
        B: s_B_B
        C: True
"""

    def test_map_with_bad_parameters_include(self):
        schema_map = Test_SchemaSyntax_map.schema_map_tmpl.format(
            include_fn = "{include_fn}",
            key_fn = "str()",
            min_val = "1",
            max_val = "2"
        )

        self.assertRaisesRegex(
            Schema_SemanticError,
            "'map': expected: 'include', but got:",
            de.build_test_combo,
            schema_map.format(include_fn='any(str())'), # <--
            Test_SchemaSyntax_map.custom_map_tmpl
        )

        self.assertRaisesRegex(
            Schema_SyntaxError,
            "In schema rule line, should be --> NameOnly",
            de.build_test_combo,
            schema_map.format(include_fn='NameOnly'), # <--
            Test_SchemaSyntax_map.custom_map_tmpl
        )

        self.assertRaisesRegex(
            Schema_SyntaxError,
            "'map': position parameter should be callable",
            de.build_test_combo,
            schema_map.format(include_fn='123'), # <--
            Test_SchemaSyntax_map.custom_map_tmpl
        )

    def test_map_with_bad_parameters_key(self):
        schema_map = Test_SchemaSyntax_map.schema_map_tmpl.format(
            include_fn = "include('Foo_item')",
            key_fn = "{key_fn}",
            min_val = "1",
            max_val = "2"
        )

        for case in [schema_map.format(key_fn="subset(str())"),
                     schema_map.format(key_fn="map(include('bar'))"),
                     schema_map.format(key_fn="include('bar')"),
                     schema_map.format(key_fn="any(int())"),
                     schema_map.format(key_fn="list(float())"),
                     schema_map.format(key_fn="null()"),]:
            with self.subTest(case = case):
                self.assertRaisesRegex(
                    Schema_SemanticError,
                    "'key' checker should be flat",
                    de.build_test_combo,
                    case,
                    Test_SchemaSyntax_map.custom_map_tmpl
                )

        self.assertRaisesRegex(
            Schema_SyntaxError,
            "ast.Name/OnlyName is not allowed in schema-line",
            de.build_test_combo,
            schema_map.format(key_fn="OnlyName"),
            Test_SchemaSyntax_map.custom_map_tmpl
        )

        self.assertRaisesRegex(
            Schema_SyntaxError,
            "'key' parameter should be callable",
            de.build_test_combo,
            schema_map.format(key_fn="123"),
            Test_SchemaSyntax_map.custom_map_tmpl
        )

    def test_map_with_bad_parameters_min_max(self):
        schema_map = Test_SchemaSyntax_map.schema_map_tmpl.format(
             include_fn = "include('Foo_item')",
             key_fn = "str()",
             min_val = "{min_val}",
             max_val = "{max_val}"
        )

        for case in [schema_map.format(min_val=-1, max_val=1),
                     schema_map.format(min_val=1,  max_val=-1),
                     schema_map.format(min_val=-1, max_val=-1)]:
            with self.subTest(case = case):
                self.assertRaisesRegex(
                    Schema_SyntaxError,
                    "'UnaryOp' is not supported",
                    de.build_test_combo,
                    case,
                    Test_SchemaSyntax_map.custom_map_tmpl
                )

        for case in [schema_map.format(min_val=0, max_val=1),
                     schema_map.format(min_val=1, max_val=0),
                     schema_map.format(min_val=0, max_val=0)]:
            with self.subTest(case = case):
                self.assertRaisesRegex(
                    Schema_SemanticError,
                    "value shouldn't be ZERO",
                    de.build_test_combo,
                    case,
                    Test_SchemaSyntax_map.custom_map_tmpl
                )

        self.assertRaisesRegex(
            Schema_SemanticError,
            "'min' > 'max' is not allowd",
            de.build_test_combo,
            schema_map.format(min_val=2, max_val=1),
            Test_SchemaSyntax_map.custom_map_tmpl
        )

    def test_map_with_invalid_value(self):
        schema_map = Test_SchemaSyntax_map.schema_map_tmpl.format(
            include_fn = "include('Foo_item')",
            key_fn = "str()",
            min_val = "2",
            max_val = "3"
        )

        custom_map =\
"""
Foo:
    Foo_A:
        A:
            AA: s_AA
            AB: True
        B: s_B
        C: False

"""
        self.assertRaisesRegex(
            Schema_CheckingError_11,
            r"expected: 2 < len(.*?) < 3, but got: 1",
            de.build_test_combo,
            schema_map,
            custom_map
        )

        custom_map =\
"""
Foo:
    Foo_A:
        A:
            AA: s_AA
            AB: True
        C: False
    Foo_B:
        A:
            AA: s_AA
            AB: True
        D: s_B
"""
        self.assertRaisesRegex(
            Schema_CheckingError_10,
            "'B' was NOT found in 'Foo_A/Foo_item'",
            de.build_test_combo,
            schema_map,
            custom_map
        )

        schema_map = \
"""
Foo: map(include('Foo_item'), key=str(), min=1, max=2)
---
Foo_item:
    A:
        AA: str(required=False)
        AB: bool(required=False)
    B: str(required=False)
    C: bool(required=False)
"""

        custom_map =\
"""
Foo:
    Foo_A: s_AA
    Foo_B: s_BB
"""
        self.assertRaisesRegex(
            Schema_CheckingError_11,
            "'map' only allows 'key-value-dict'",
            de.build_test_combo,
            schema_map,
            custom_map
        )

    def test_map_bad_required_in_first_top_node(self):
        schema_map = \
"""
Foo: map(include('Foo_item'), key=str(), min=1, max=2, required=False)
---
Foo_item:
    A:
        AA: str()
        AB: bool()
    B: str()
    C: bool()
"""

        custom_map = \
"""
Foo:
    Foo_A:
        A:
            AA: s_AA
            AB: True
        B: s_B
        C: False
"""
        self.assertRaisesRegex(
            Schema_SemanticError,
            "The first top node 'Foo' shouldn't be marked as 'required=False'",
            de.build_test_combo,
            schema_map,
            custom_map
        )

    def test_map_bad_required_others(self):
        schema_map = \
"""
Foo: map(include('Foo_item', required=False), key=str(required=False), min=1, max=2)
---
Foo_item:
    A:
        AA: str()
        AB: bool()
    B: str()
    C: bool()
"""

        custom_map = \
"""
Foo:
    Foo_A:
        A:
            AA: s_AA
            AB: True
        B: s_B
        C: False
"""
        # required=False in 'include' & it's 'key' can be handled by: CASE_03
        self.assertIsNone(de.build_test_combo(schema_map, custom_map))

        schema_map = \
"""
Foo: map(include('Foo_item', required=False), key=str(required=False), min=1, max=2)
---
Foo_item:
    A:
        AA: str()
        AB: bool()
    B: str()
    C: bool()
"""

        custom_map = \
"""
Foo:
    Foo_A:
        A:
            AB: True
        B: s_B
        C: False
"""
        self.assertRaisesRegex(
            Schema_CheckingError_10,
            r"'AA' was NOT found in 'A/A', that's required",
            de.build_test_combo,
            schema_map,
            custom_map
        )

        schema_map = \
"""
Foo: map(include('Foo_item', required=False), key=str(required=False), min=1, max=2)
---
Foo_item:
    A:
        AA: str(required=False)
        AB: bool()
    B: str()
    C: bool(required=True)
"""

        custom_map = \
"""
Foo:
    Foo_A:
        A:
            AB: True
        B: s_B
"""
        self.assertRaisesRegex(
            Schema_CheckingError_10,
            r"'C' was NOT found in 'Foo_A/Foo_item', that's required",
            de.build_test_combo,
            schema_map,
            custom_map
        )


    def test_map_with_normal_value(self):
        schema_map = Test_SchemaSyntax_map.schema_map_tmpl.format(
            include_fn = "include('Foo_item')",
            key_fn = "str()",
            min_val = "1",
            max_val = "3"
        )

        self.assertIsNone(
            de.build_test_combo(schema_map,
                                Test_SchemaSyntax_map.custom_map_tmpl))
