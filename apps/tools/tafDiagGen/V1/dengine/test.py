#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import tempfile

def build_test_schema(test_buffer):
    with tempfile.TemporaryDirectory() as tdir:
        print(f"(schema) Creating-Dir: {tdir}")
        with tempfile.NamedTemporaryFile(dir=tdir,
                                         mode='w+',
                                         prefix="eg_schema_syntax",
                                         suffix=".yaml",
                                         delete=False) as tfile:
            print(f"  New-file: {tfile.name}")
            tfile.write(test_buffer)
            for line_no, line_txt in enumerate(test_buffer.split('\n'), start=1):
                print(f"  {line_no}: {line_txt}")
        from . import build_schema
        return build_schema(tdir)

def build_test_custom(test_buffer):
    with tempfile.TemporaryDirectory() as tdir:
        print(f"(custom) Creating-Dir: {tdir}")
        with tempfile.NamedTemporaryFile(dir=tdir,
                                         mode='w+',
                                         prefix="eg_custom_syntax",
                                         suffix=".yaml",
                                         delete=False) as tfile:
            print(f"  New-file: {tfile.name}")
            tfile.write(test_buffer)
            for line_no, line_txt in enumerate(test_buffer.split('\n'), start=1):
                print(f"  {line_no}: {line_txt}")
        from . import build_custom
        return build_custom(tdir)
