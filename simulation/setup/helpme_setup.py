#! /usr/bin/env python3

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys, re, os
import json
import subprocess

def check_returncode(result, callback=None, need_exit=True):
    if result.returncode != 0:
        print("Error: %s, %s", result.args, result.stderr)
        if callback:
            callback()
        if need_exit == True:
            sys.exit(result.returncode)

def quick_run(command):
    return subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          universal_newlines=True)

def doit(jfile):
    patch_file = open(jfile)

    root = json.load(patch_file)
    for path_to_git, repo_sections in root.items():
        if not os.path.exists(path_to_git):
            print("E: repo [{}] isn't exist, please check it".format(path_to_git))
            sys.exit(-1)

        if repo_sections["Commit-ID"] is "":
            print("> [{}] use the [latest] commit".format(path_to_git))
            continue

        check_returncode(quick_run("cd {} && git stash".format(path_to_git)))
        check_returncode(quick_run("cd {} && git checkout -b base-{} {}"
                                .format(path_to_git, repo_sections["Commit-ID"], repo_sections["Commit-ID"])))

        if not repo_sections["Patch-Cmd-List"]:
            print("> [{}] use the [{}], without any patch".format(path_to_git, repo_sections["Commit-ID"]))
            continue

        for patch in repo_sections["Patch-Cmd-List"]:
            print("+ Patch : [{}] Start".format(patch))
            check_returncode(quick_run("cd {} && {}".format(path_to_git, patch)))
            print("- Patch : [{}] Done".format(patch))

    patch_file.close()


if __name__ == "__main__":
    patch_me_json = "patch_me.json"
    doit(patch_me_json)
