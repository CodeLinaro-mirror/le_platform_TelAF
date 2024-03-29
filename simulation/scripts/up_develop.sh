#!/bin/bash

# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

current_dir=$(dirname "$0")
simulation_base=$(realpath ${current_dir}/..)
simulation_workstation=${current_dir}/../workstation

project_root=$(realpath ${current_dir}/../../../)

IMG_NAME=${IMG_NAME:="telaf_simulation_develop_$1"}
IMG_VERSION=${IMG_VERSION:="1.0.0"}
CONTAINER_OPTIONS=${CONTAINER_OPTIONS:=""}

if [ -n "${within}" ]; then

    BUILTIN_CONTAINER_OPTIONS=${BUILTIN_CONTAINER_OPTIONS:="--rm -i -t"}
    # Caution: random name for this once command-container.
    # For 'within', it's passed from environment, like export.
    # Example: make simula within="'hostname && make simula-clean && make simulac'"
    docker run ${BUILTIN_CONTAINER_OPTIONS} -u $(id -u):$(id -g) \
        -e TELAF_DEV_IN_CONTAINER=${project_root} \
        -e CPLUS_INCLUDE_PATH='/usr/include/python2.7/' \
        -v ${simulation_base}:/home/developer/simulation_ro:ro \
        -v ${project_root}:${project_root}:rw \
        -v ${current_dir}/example.gitconfig:/home/developer/.gitconfig \
        ${IMG_NAME}:${IMG_VERSION} bash -c -- "'${within}'"

else # only one parameter

    CONTAINER_NAME=${CONTAINER_NAME:="telaf_simulation_develop_$1"}

    # Caution: when we use '-d' to run container, you should check the log for aync jobs.
    BUILTIN_CONTAINER_OPTIONS=${BUILTIN_CONTAINER_OPTIONS:="-d -i -t"} # --privileged=true --net=bridge

    attach_container="docker exec -i -t ${CONTAINER_NAME} /bin/bash"

    cd ${simulation_workstation}

    container_status=$(docker inspect -f '{{.State.Status}}' "${CONTAINER_NAME}" 2>/dev/null)
    if [[ "${container_status}" == "running" ]]; then
        eval ${attach_container}
    else
        docker run --name ${CONTAINER_NAME} \
            ${BUILTIN_CONTAINER_OPTIONS} \
            ${CONTAINER_OPTIONS} -u $(id -u):$(id -g) \
            -e CPLUS_INCLUDE_PATH='/usr/include/python2.7/' \
            -e TELAF_DEV_IN_CONTAINER=${project_root} \
            -v ${simulation_base}:/home/developer/simulation_ro:ro \
            -v ${project_root}:${project_root}:rw \
            -v ${current_dir}/example.gitconfig:/home/developer/.gitconfig \
            ${IMG_NAME}:${IMG_VERSION} "/bin/bash" > /dev/null && eval ${attach_container}
    fi

    exit 0
fi
