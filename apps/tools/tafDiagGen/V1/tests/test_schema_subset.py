#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import unittest
import os, sys

import dengine as de
from dengine.exceptions import *


class Test_SchemaSyntax_subset(unittest.TestCase):

    def test_subset_in_abnormal_cases(self):
        schema_subset =\
"""
Foo: subset( map(include('A'), key=str()), map(include('B'), key=str()) )

---
A:
    AA: str()
    AB:
        AAA: str()
        AAB: str()

---
B:
    BA: str()
    BB: str()
    BC: str()

"""

        custom_subset =\
"""
Foo:
    A:
        AA: s_AA
        AB:
            AAA: s_AAA
            AAB: s_AAB
    B:
        BA: s_BA
        BB: s_BB
        BC: s_BC
    C:
        CA: s_CA
        CB: s_CB
        CC: False
"""

        with self.assertRaisesRegex(Schema_CheckingError_11, r"Due to SUBSET \(StopIteration\)"):
            de.build_test_combo(schema_subset, custom_subset)


    def test_subset_in_normal_cases(self):

        schema_subset =\
"""
Foo: subset(any(include('A', key=str()), include('B', key=str()), include('C', key=str())))

---
A:
    AA: str()
    AB:
        AAA: str()
        AAB: str()

---
B:
    BA: str()
    BB: str()
    BC: str()

---
C:
    CA: str()
    CB: str()
    CC: bool()
"""

        custom_subset =\
"""
Foo:
    A:
        AA: s_AA
        AB:
            AAA: s_AAA
            AAB: s_AAB
    B:
        BA: s_BA
        BB: s_BB
        BC: s_BC
    C:
        CA: s_CA
        CB: s_CB
        CC: False
"""
        self.assertIsNone(
            de.build_test_combo(schema_subset, custom_subset)
        )

        # --- Next ---

        schema_subset =\
"""
Foo: subset( map(include('A'), key=str()), map(include('B'), key=str()) )

---
A:
    AA: str()
    AB:
        AAA: str()
        AAB: str()

---
B:
    BA: str()
    BB: str()
    BC: str()
"""

        custom_subset =\
"""
Foo:
    A_m:
        A:
            AA: s_AA
            AB:
                AAA: s_AAA
                AAB: s_AAB
        A2:
            AA: s_AA
            AB:
                AAA: s_AAA
                AAB: s_AAB
    C_m:
        B:
            BA: s_BA
            BB: s_BB
            BC: s_BC
"""
        self.assertIsNone(
            de.build_test_combo(schema_subset, custom_subset)
        )
