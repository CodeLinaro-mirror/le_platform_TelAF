#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

from pathlib import Path
import re
from pprint import pprint

# Exclude the files starting with 'ref17_'
pattern = re.compile(r'^(?!ref17_).*')

# Base on current directory
current_dir = Path(__file__).parent

checking_customer = Path(current_dir / 'customer')

which_one = 'default'

P_generic = Path( checking_customer / 'Generic')
P_ivc3_sa = Path( checking_customer / 'IVC3-SA')

if P_generic.is_dir() and P_ivc3_sa.is_dir():

    pg_files = [ f for f in P_generic.iterdir() if f.name != '.keep' ]
    pi_files = [ f for f in P_ivc3_sa.iterdir() if f.name != '.keep' ]

    if pg_files or pi_files:
        which_one = 'customer'

print(f"which_one => {which_one}")

generic = [ f for f in Path(current_dir / which_one / 'Generic').rglob("*.yaml") if f.is_file() and pattern.match(f.name) ]
ivc3_sa = [ f for f in Path(current_dir / which_one / 'IVC3-SA').rglob("*.yaml") if f.is_file() and pattern.match(f.name) ]

customer_files = generic + ivc3_sa
# print(f"Total valid files in [{current_dir}]:")
# pprint(customer_files)
