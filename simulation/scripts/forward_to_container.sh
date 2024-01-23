#!/bin/bash

# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

# TELAF_DEV_IN_CONTAINER is from container environement args

# current_dir=$(dirname "$0")
project_top="$TELAF_DEV_IN_CONTAINER"
telaf_root=$project_top/telaf

cd $telaf_root
echo -e "\n-- container information record begin --"
cat /etc/os-release
echo -e "\nTelAF-Simulation Project : ${project_top}\n"
echo -e "-- container information record done --\n"
eval "$@"
