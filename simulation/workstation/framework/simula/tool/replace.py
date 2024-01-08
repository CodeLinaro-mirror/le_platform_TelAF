#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import fileinput
import re, sys

def replace(file_path, from_pattern, to_pattern):
    def replace_callback(match):
        captured_groups = match.groups()
        return match.expand(to_pattern.format(*captured_groups))

    with fileinput.FileInput(file_path, inplace=True, backup='.bak') as file:
        for line in file:
            modified_line = re.sub(from_pattern, replace_callback, line)
            sys.stdout.write(modified_line)
