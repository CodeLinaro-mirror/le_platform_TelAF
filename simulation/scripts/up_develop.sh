#!/bin/bash

# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

current_dir=$(dirname "$0")
simulation_workstation=${current_dir}/../workstation

project_root=${current_dir}/../../../

function make_home
{
    LOCAL_DEV_HOME="`pwd`/$1"
    if ! [ -d ${LOCAL_DEV_HOME} ]; then
        mkdir -p ${LOCAL_DEV_HOME}
    fi
    echo ${LOCAL_DEV_HOME}
}

IMG_NAME=${IMG_NAME:="telaf_simulation_develop_$1"}
IMG_VERSION=${IMG_VERSION:="1.0.0"}
CONTAINER_OPTIONS=${CONTAINER_OPTIONS:=""}

if [ "${within}" != "" ]; then

    BUILTIN_CONTAINER_OPTIONS=${BUILTIN_CONTAINER_OPTIONS:="--rm -i -t"}
    # Caution: random name for this once command-container.
    # For 'within', it's passed from environment, like export.
    docker run ${BUILTIN_CONTAINER_OPTIONS} -u $(id -u):$(id -g) \
        -v ${project_root}:/workspace:rw \
        ${IMG_NAME}:${IMG_VERSION} "${within}"

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
            -v ${project_root}:/workspace:rw \
            ${IMG_NAME}:${IMG_VERSION} "/bin/bash" > /dev/null && eval ${attach_container}
    fi
fi
