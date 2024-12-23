#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

tcpdump_start("test-86-onDTCStatusChange")

#NRC 0x13
uds("8601")

#NRC 0x12
uds("860A01")

#NRC 0x31
uds("86410801190101")

#Positive response
uds("86010801190101")


tcpdump_stop()
