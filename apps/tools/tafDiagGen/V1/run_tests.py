#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import unittest

loader = unittest.TestLoader()
suite = loader.discover(start_dir='tests')

runner = unittest.TextTestRunner()
runner.run(suite)
