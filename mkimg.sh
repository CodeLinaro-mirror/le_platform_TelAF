#!/bin/sh

# Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted (subject to the limitations in the
# disclaimer below) provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#
#     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
# GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
# HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

export SELINUX_FILE_CONTEXTS=${TELAF_ROOT}/security/selinux/sepolicy/files/file_contexts
TARGET=$1
OUTPUT=$2
NOSHIP_BUILD_DIR=$3
PROP_BUILD_DIR=$4
OUTPUT_STAGE=${TELAF_ROOT}/build/${TARGET}/mkimg/

if [ "$NOSHIP_BUILD_DIR" == "" ]; then
    NOSHIP_BUILD_DIR=${TELAF_NOSHIP}
fi

if [ "$PROP_BUILD_DIR" == "" ]; then
    PROP_BUILD_DIR=${TELAF_PROP}
fi

cd ${LEGATO_ROOT} && source ${LEGATO_ROOT}/bin/configlegatoenv

TARGET_STAGE_DIR=${LEGATO_ROOT}/build/${TARGET}/_staging_system.${TARGET}.update_ro/

echo "*** searching path: ${PROP_BUILD_DIR} ***"
for full_name_prop in `find ${PROP_BUILD_DIR} -maxdepth 1 -type f -name "*.so"`
do
    base_name=`basename ${full_name_prop}`
    echo "*** try to use ${base_name} replace telaf-prop stub library ***"
    full_name_telaf=`find ${TARGET_STAGE_DIR} -type f -name ${base_name}`
    if [ -n "${full_name_telaf}" ]; then
        cp -rf ${full_name_prop} ${full_name_telaf}
    fi
done

echo "*** searching path: ${NOSHIP_BUILD_DIR} ***"
for full_name_noship in `find ${NOSHIP_BUILD_DIR} -maxdepth 1 -type f -name "*.so"`
do
    base_name=`basename ${full_name_noship}`
    echo "*** try to use ${base_name} replace telaf-noship stub library ***"
    full_name_telaf=`find ${TARGET_STAGE_DIR} -type f -name ${base_name}`
    if [ -n "${full_name_telaf}" ]; then
        for each_lib_name in ${full_name_telaf}
        do
            cp -rf ${full_name_noship} ${each_lib_name}
        done
    fi
done

if [ -n "${VENDOR_ROOT}" ] && [ -d "${VENDOR_ROOT}" ]; then
    echo "*** searching path: ${VENDOR_ROOT} ***"
    for so_file in `find ${VENDOR_ROOT} -type f -name "*.so"`
    do
        echo "*** installing ${so_file} to ${TARGET_STAGE_DIR}/systems/current/modules/ ***"
        cp -rf ${so_file} ${TARGET_STAGE_DIR}/systems/current/modules/
    done
fi

mklegatoimg -t "${TARGET}" \
            -d "${TARGET_STAGE_DIR}/" \
            -o "${OUTPUT_STAGE}"
if [ $? -ne 0 ]; then
    echo "mklegatoimg: Generating image failed"
    exit 1
fi

cp ${OUTPUT_STAGE}/telaf.squashfs ${OUTPUT}/telaf_ro.squashfs
cp ${OUTPUT_STAGE}/telaf.squashfs.ubi ${OUTPUT}/telaf_ro.squashfs.ubi
