#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

# Note that: Don't use the 'set' data-structure to do the operations.
# That will break the OrderedDict items.

# output = left & right
def do_intersection(left, right):
    intersection = []
    for item in left:
        if item in right:
            intersection.append(item)
    return intersection

# output = left - right
def do_diff_left(left, right):
    diff = []
    for item in left:
        if item not in right:
            diff.append(item)
    return diff
