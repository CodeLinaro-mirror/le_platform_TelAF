#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import os, yaml
import fnmatch
from .constants import SUFFIX
from .exceptions import DuplicateKeyError
from .logger import logc

class Custom:
    def __init__(self, x_path):
        self.dpath = x_path
        self.data = {}
        self.merged_data = {}

        logc.info(f"Specific custom parsing path: {self.dpath}")

        if os.path.isdir(self.dpath):
            for root, _, files in os.walk(self.dpath):
                for file in fnmatch.filter(files, "*" + SUFFIX):
                    logc.info(f"Collecting custom file -> {os.path.join(root, file)}")
                    self._merge_custom_files(os.path.join(root, file))
        else: # file
            self._merge_custom_files(self.dpath)

    def _merge_custom_files(self, fpath):
        with open(fpath, 'r') as f:
            docs = yaml.safe_load_all(f)
            update_me = {}
            for doc in docs:
                for top_key in doc.keys():
                    for fname, root in self.data.items():
                        if top_key in root:
                            raise DuplicateKeyError(fname, top_key, fpath)
                update_me.update(doc)
            self.data.update({fpath: update_me})
            self.merged_data.update(update_me)

def build_custom_tree(x_path):
    return Custom(x_path)
