#setup toolchain - this can be optimized later with findtoolchain script

# set global variables
export CURDIR=$(cd `dirname $1` ; pwd)

if [ -d "$TELAF_ROOT" ] && [ "$TELAF_ROOT" != "$CURDIR" ]; then
    echo "Error: The TELAF_ROOT was detected as already being present in this shell environment and inconsistent with the environment to be set. Please use a clean shell when sourcing this environment script."
    return
fi

export TELAF_ROOT=${CURDIR}
export LEGATO_ROOT=${CURDIR}/../legato/legato-af
if [ -f ${TELAF_ROOT}/VERSION ]; then
    export LEGATO_VERSION=`cat ${TELAF_ROOT}/VERSION 2>/dev/null`
fi

export TELAF_PROP=${CURDIR}/../telaf-prop
if [ ! -f "${TELAF_PROP}/build.sh" ]; then
    export TELAF_PROP=${CURDIR}/../prebuilt_HY33/${1}-nad/telaf-prop-build/telaf-prop/lib
fi

export TELAF_NOSHIP=${CURDIR}/../telaf-noship
if [ ! -f "${TELAF_NOSHIP}/build.sh" ]; then
    export TELAF_NOSHIP=${CURDIR}/../prebuilt_HY33/${1}-nad/telaf-noship-build/telaf-noship/lib
fi

if [ "$1" == "sa415m" ]; then
    source /opt/qct/sa415m/environment-setup-armv7at2hf-neon-oe-linux-gnueabi
elif [ "$1" == "sa515m" ]; then
    source /opt/qct/sa515m/environment-setup-armv7at2hf-neon-oe-linux-gnueabi
elif [ "$1" == "sa525m" ]; then
    source /opt/qct/sa525m/environment-setup-aarch64-oe-linux
else
    echo " Missing target parameter!"
    echo " e.g. $0 sa415m"
    return
fi

umask 002

#build the target

function build-sa415m-af(){
    make sa415m
}

function build-clean-af(){
    make clean
}

function build-distclean-af(){
    make distclean
}

function build-sa515m-af(){
    TARGET=sa515m

    # build TelAF OSS source code
    make ${TARGET}

    # build telaf-prop source code if exists
    if [ -f "${TELAF_PROP}/build.sh" ]; then
        TELAF_SYS_QMI_ROOT=${CURDIR}/../qmi/services/
        TELAF_SYS_QMI_FRAMEWORK_ROOT=${CURDIR}/../qmi-framework/inc/
        ${TELAF_PROP}/build.sh ${TARGET} "$TELAF_SYS_QMI_ROOT" "$TELAF_SYS_QMI_FRAMEWORK_ROOT"
    fi

    ## build telaf-noship source code if exists
    if [ -f "${TELAF_NOSHIP}/build.sh" ]; then
        TELAF_SYS_QMI_ROOT=${CURDIR}/../qmi/services/
        TELAF_SYS_QMI_FRAMEWORK_ROOT=${CURDIR}/../qmi-framework/inc/
        TELAF_SYS_DSUTIL_ROOT=${CURDIR}/../data/dsutils/inc/
        ${TELAF_NOSHIP}/build.sh ${TARGET} "$TELAF_SYS_QMI_ROOT" "$TELAF_SYS_QMI_FRAMEWORK_ROOT" "$TELAF_SYS_DSUTIL_ROOT"
    fi

    ## repack TelAF image
    TELAF_REPACK_DIR=$TELAF_ROOT/build/$TARGET/
    TELAF_NOSHIP_BUILD_DIR=${TELAF_ROOT}/build/${TARGET}/telaf-noship
    TELAF_PROP_BUILD_DIR=${TELAF_ROOT}/build/${TARGET}/telaf-prop
    if [ ! -d $TELAF_NOSHIP_BUILD_DIR ]; then
        TELAF_NOSHIP_BUILD_DIR=$TELAF_NOSHIP
    fi
    if [ ! -d $TELAF_PROP_BUILD_DIR ]; then
        TELAF_PROP_BUILD_DIR=$TELAF_PROP
    fi
    ${TELAF_ROOT}/mkimg.sh ${TARGET} "$TELAF_REPACK_DIR" "$TELAF_NOSHIP_BUILD_DIR" "$TELAF_PROP_BUILD_DIR"

    # sign TelAF image
    export AVBTOOL="${OECORE_NATIVE_SYSROOT}/usr/share/avb_py_tool"
    if [ ! -d $AVBTOOL/keys ]; then
        ${AVBTOOL}/avbtool add_hashtree_footer --image ./build/sa515m/telaf_ro.squashfs --partition_name telaf --do_not_generate_fec --rollback_index 0
    else
        ${AVBTOOL}/avbtool add_hashtree_footer --image ./build/sa515m/telaf_ro.squashfs --partition_name telaf --algorithm SHA256_RSA2048 --key $AVBTOOL/keys/qpsa_attest.key --public_key_metadata $AVBTOOL/keys/qpsa_attest.der --do_not_generate_fec --rollback_index 0
    fi

    if [ $? -eq 0 ]
    then
        # create the tarball for telaf app dependencies used for sdk patch
        ${TELAF_ROOT}/bin/createsdk ${TARGET} ${TELAF_ROOT}/../
    fi
}

function build-sa525m-af(){
    TARGET=sa525m

    # build TelAF OSS source code
    make ${TARGET}

    # build telaf-prop source code if exists
    if [ -f "${TELAF_PROP}/build.sh" ]; then
        TELAF_SYS_QMI_ROOT=${CURDIR}/../qmi/services/
        TELAF_SYS_QMI_FRAMEWORK_ROOT=${CURDIR}/../qmi-framework/inc/
        ${TELAF_PROP}/build.sh ${TARGET} "$TELAF_SYS_QMI_ROOT" "$TELAF_SYS_QMI_FRAMEWORK_ROOT"
    fi

    ## build telaf-noship source code if exists
    if [ -f "${TELAF_NOSHIP}/build.sh" ]; then
        TELAF_SYS_QMI_ROOT=${CURDIR}/../qmi/services/
        TELAF_SYS_QMI_FRAMEWORK_ROOT=${CURDIR}/../qmi-framework/inc/
        TELAF_SYS_DSUTIL_ROOT=${CURDIR}/../data/dsutils/inc/
        ${TELAF_NOSHIP}/build.sh ${TARGET} "$TELAF_SYS_QMI_ROOT" "$TELAF_SYS_QMI_FRAMEWORK_ROOT" "$TELAF_SYS_DSUTIL_ROOT"
    fi

    # repack TelAF image
    TELAF_REPACK_DIR=$TELAF_ROOT/build/$TARGET/
    TELAF_NOSHIP_BUILD_DIR=${TELAF_ROOT}/build/${TARGET}/telaf-noship
    TELAF_PROP_BUILD_DIR=${TELAF_ROOT}/build/${TARGET}/telaf-prop
    if [ ! -d $TELAF_NOSHIP_BUILD_DIR ]; then
        TELAF_NOSHIP_BUILD_DIR=$TELAF_NOSHIP
    fi
    if [ ! -d $TELAF_PROP_BUILD_DIR ]; then
        TELAF_PROP_BUILD_DIR=$TELAF_PROP
    fi
    ${TELAF_ROOT}/mkimg.sh ${TARGET} "$TELAF_REPACK_DIR" "$TELAF_NOSHIP_BUILD_DIR" "$TELAF_PROP_BUILD_DIR"

    # sign TelAF image
    #export AVBTOOL="${OECORE_NATIVE_SYSROOT}/usr/share/avb_py_tool"
    #if [ ! -d $AVBTOOL/keys ]; then
    #    ${AVBTOOL}/avbtool add_hashtree_footer --image ./build/sa515m/telaf_ro.squashfs --partition_name telaf --do_not_generate_fec --rollback_index 0
    #else
    #    ${AVBTOOL}/avbtool add_hashtree_footer --image ./build/sa515m/telaf_ro.squashfs --partition_name telaf --algorithm SHA256_RSA2048 --key $AVBTOOL/keys/qpsa_attest.key --public_key_metadata $AVBTOOL/keys/qpsa_attest.der --do_not_generate_fec --rollback_index 0
   #fi
}


