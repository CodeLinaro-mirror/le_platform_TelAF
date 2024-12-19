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

generic = [ f for f in Path(current_dir / 'customer' / 'Generic').rglob("*.yaml") if f.is_file() and pattern.match(f.name) ]
ivc3_sa = [ f for f in Path(current_dir / 'customer' / 'IVC3-SA').rglob("*.yaml") if f.is_file() and pattern.match(f.name) ]

customer_files = generic + ivc3_sa
# print(f"Total valid files in [{current_dir}]:")
# pprint(customer_files)
