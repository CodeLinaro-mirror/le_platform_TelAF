# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

export SELINUX_FILE_CONTEXTS=${TELAF_ROOT}/security/selinux/sepolicy/files/file_contexts
TARGET=$1
OUTPUT=$2
NOSHIP_BUILD_DIR=$3
PROP_BUILD_DIR=$4
PA_BUILD_DIR=$5
OUTPUT_STAGE=${TELAF_ROOT}/build/${TARGET}/mkimg/

if [ "$NOSHIP_BUILD_DIR" == "" ]; then
    NOSHIP_BUILD_DIR=${TELAF_NOSHIP}
fi

if [ "$PROP_BUILD_DIR" == "" ]; then
    PROP_BUILD_DIR=${TELAF_PROP}
fi

if [ "$PA_BUILD_DIR" == "" ]; then
    PA_BUILD_DIR=${TELAF_PA}
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
        echo "stripping ${full_name_prop}"
        ${STRIP} --strip-unneeded ${full_name_prop}
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
            echo "stripping ${full_name_noship}"
            ${STRIP} --strip-unneeded ${full_name_noship}
            cp -rf ${full_name_noship} ${each_lib_name}
        done
    fi
done

echo "*** searching path: ${PA_BUILD_DIR} ***"
for full_name_pa in `find ${PA_BUILD_DIR} -maxdepth 1 -type f -name "*.so"`
do
    base_name=`basename ${full_name_pa}`
    echo "*** try to use ${base_name} replace telaf-pa stub library ***"
    full_name_telaf=`find ${TARGET_STAGE_DIR} -type f -name ${base_name}`
    if [ -n "${full_name_telaf}" ]; then
        for each_lib_name in ${full_name_telaf}
        do
            echo "stripping ${full_name_pa}"
            ${STRIP} --strip-unneeded ${full_name_pa}
            cp -rf ${full_name_pa} ${each_lib_name}
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
