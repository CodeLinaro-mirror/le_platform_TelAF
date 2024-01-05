#!/bin/bash

# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

current_dir=$(dirname "$0")
project_top=$current_dir/../../../
telaf_root=$project_top/telaf

cd $telaf_root
echo -e "\n-- container information record begin --"
cat /etc/os-release
echo -e "-- container information record done --\n"
eval "$@"
