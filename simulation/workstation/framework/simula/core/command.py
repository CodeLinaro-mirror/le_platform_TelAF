#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import argparse
import subprocess
import sys
from .logger import L
from .. import __version__ as mversion
from ..utest import tafSmsUnitTest_helper as utest_sms_helper
from ..utest import tafSomeipGWTest_helper as utest_someip_helper
from ..utest import tafDataCallUnitTest_helper as utest_dcs_helper

def precondition_test():
    result = subprocess.run("telaf status", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    if "TelAF framework is running" not in result.stdout:
        L.error("TelAF framework is NOT running, please start it first")
        sys.exit(-1)

def dispatch_subcmd_jobs(args):
    if args.subcommand == "utest":
        precondition_test()
        if args.whichone == "someip":
            utest_someip_helper.to_run()
        elif args.whichone == "sms":
            utest_sms_helper.to_run()
        elif args.whichone == "dcs":
            utest_dcs_helper.to_run()

def doit():
    parser = argparse.ArgumentParser(prog="simula", description="Simplify some simulation operations.")
    parser.add_argument("-v", "--version", action='version', version="%(prog)s " + mversion)
    subparsers = parser.add_subparsers(dest="subcommand", help="%(prog)s support some actions by sub-commands")
    subparser_utest = subparsers.add_parser("utest", help="Run some helper scripts for unit test")
    subparser_utest.add_argument('whichone', choices=('sms', 'someip', 'dcs'), help="Unit testcase list that supported")
    args = parser.parse_args()
    dispatch_subcmd_jobs(args)
