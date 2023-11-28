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
from .container import master, slavex

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

    elif args.subcommand == "slave":
        try:
            if args.shell is True:
                slavex.login_slave()
            elif args.list is True:
                slavex.list_slave()
            elif args.to is not None:
                slavex.change_def_and_login(args.to[0])
            elif args.remote is True:
                slavex.remote_do(" ".join(args.remote_todo))
        except FileNotFoundError:
            print("Oops, maybe you just run the master without any slave, right?")

    elif args.subcommand == "master":
        if args.shell is True:
            master.login_master()
        elif args.info is True:
            master.show_info()


def doit():
    parser = argparse.ArgumentParser(prog="simula", description="Simplify some simulation operations.")
    parser.add_argument("-v", "--version", action='version', version="%(prog)s " + mversion)

    subparsers = parser.add_subparsers(dest="subcommand", help="%(prog)s support some actions by sub-commands")

    subparser_slavex = subparsers.add_parser("slave", help="Support some operations for 'slave' nodes")
    subparser_slavex.add_argument("-s", "--shell", dest="shell", action='store_true', default=False,
                                  help="login remote slave container with ssh-client")
    subparser_slavex.add_argument("-l", "--list", dest="list", action='store_true', default=False,
                                  help="list all slave-x nodes and their information")
    subparser_slavex.add_argument("-t", "--to", dest="to", default=None, nargs=1,
                                  help="change current slave pointer to which you want and login")
    subparser_slavex.add_argument("-r", "--remote", dest="remote", action='store_true', default=False,
                                  help="execute commands on remote slave container and back")
    subparser_slavex.add_argument("remote_todo", nargs="*", help="any linux command, such as: telaf")

    subparser_master = subparsers.add_parser("master", help="Support some operation for the 'master' node")
    subparser_master.add_argument("-s", "--shell", dest="shell", action='store_true', default=False,
                                  help="login the master container from slave nodes")
    subparser_master.add_argument("-i", "--info", dest="info", action='store_true', default=False,
                                  help="show the master information for prompt")

    subparser_utest = subparsers.add_parser("utest", help="Run some helper scripts for unit test")
    subparser_utest.add_argument('whichone', choices=('sms', 'someip', 'dcs'), help="Unit testcase list that supported")

    args = parser.parse_args()
    dispatch_subcmd_jobs(args)
