
# Set global variables
export CURDIR=$(cd "$(dirname "$1")" || exit ; pwd)
if [ -d "$TELAF_ROOT" ] && [ "$TELAF_ROOT" != "$CURDIR" ]; then
    echo "Error: The TELAF_ROOT was detected as already being present in this shell environment and inconsistent with the environment to be set. Please use a clean shell when sourcing this environment script."
    return
fi
export TELAF_ROOT=${CURDIR}
export LEGATO_ROOT=${CURDIR}/../legato/legato-af
export LEGATO_VERSION=$(cat "${TELAF_ROOT}/VERSION" 2>/dev/null)

export TELAF_PROP=${CURDIR}/../telaf-prop
export TELAF_NOSHIP=${CURDIR}/../telaf-noship

# Setup toolchain
setup_toolchain() {
    local TARGET=$1

    if [ "$TARGET" == "sa415m" ]; then
        source /opt/qct/sa415m/environment-setup-armv7at2hf-neon-oe-linux-gnueabi
    elif [ "$TARGET" == "sa515m" ]; then
        source /opt/qct/sa515m/environment-setup-armv7at2hf-neon-oe-linux-gnueabi
    elif [ "$TARGET" == "sa525m" ]; then
        if [ -e /opt/qct/sa525m/environment-setup-armv7at2hf-neon-oemllib32-linux-gnueabi ]; then
            # For 32bit tool chain build
            export GCC_PREFIX="arm-oemllib32-linux-gnueabi"
            source /opt/qct/sa525m/environment-setup-armv7at2hf-neon-oemllib32-linux-gnueabi
        elif [ -e /opt/qct/sa525m/environment-setup-aarch64-oe-linux ]; then
            # For 64bit tool chain build
            export GCC_PREFIX="aarch64-oe-linux"
            source /opt/qct/sa525m/environment-setup-aarch64-oe-linux
        fi
    else
        echo " Missing target parameter!"
        echo " e.g. $0 sa415m"
        return
    fi
}

umask 002

# Function to build telaf-prop and telaf-noship
build_extras() {
    # The declare -A EXTRA_ENUM command is used to define an associative array that maps EXTRA_TYPE values to their corresponding constants
    # Later EXTRA_ENUM[${EXTRA_TYPE}] is used to retrieve the value associated with a specific EXTRA_TYPE. 
    declare -A EXTRA_ENUM=(
        [prop]="TELAF_PROP"         # Map 'prop' to 'TELAF_PROP'
        [noship]="TELAF_NOSHIP"     # Map 'noship' to 'TELAF_NOSHIP'
    )

    local EXTRA_TYPE=$1
    local TARGET=$2
    local DIST_DIR="${CURDIR}/build/${TARGET}/telaf-${EXTRA_TYPE}-prebuild"

    local TMP_VAR="${EXTRA_ENUM[${EXTRA_TYPE}]}"
    local EXTRA_SRC_PATH="${!TMP_VAR}"

    if [ -f "${EXTRA_SRC_PATH}/build.sh" ]; then
        local CODE_SYS_QMI_ROOT="${CURDIR}/../qmi/services/"
        local CODE_SYS_QMI_FRAMEWORK_ROOT="${CURDIR}/../qmi-framework/inc/"
        local CODE_SYS_DSUTIL_ROOT="${CURDIR}/../data/dsutils/inc/"
        ${EXTRA_SRC_PATH}/build.sh "${TARGET}" "${CODE_SYS_QMI_ROOT}" "${CODE_SYS_QMI_FRAMEWORK_ROOT}" "${CODE_SYS_DSUTIL_ROOT}"
        if [ $? -ne 0 ]; then
            echo "Error: when running ${EXTRA_SRC_PATH}/build.sh"
            return
        fi
    else
        # The prebuild for sa525m is located under the apps_proc directory, and it's in the tar.gz file format
        local PREBUILT_FILE=$(find "${CURDIR}/../../prebuilt_HY11" -type f -name "telaf-${EXTRA_TYPE}-build_*.tar.gz")
        # The prebuild for sa515m is in the same level directory as telaf, and it is provided in an already uncompressed format
        local PREBUILT_DIR="${CURDIR}/../prebuilt_HY33/${2}-nad/telaf-${EXTRA_TYPE}-build/telaf-${EXTRA_TYPE}"
        if [ -n "$PREBUILT_FILE" ]; then
            rm -fr "${DIST_DIR}" && mkdir -p "${DIST_DIR}"
            tar -xzvf "$PREBUILT_FILE" -C "${DIST_DIR}"
            export TELAF_${EXTRA_TYPE^^}="${DIST_DIR}/telaf-${EXTRA_TYPE}/lib"
        elif [ -d "${PREBUILT_DIR}/lib" ]; then
            rm -fr "${DIST_DIR}" && mkdir -p "${DIST_DIR}"
            cp -r "${PREBUILT_DIR}" "${DIST_DIR}/"
            export TELAF_${EXTRA_TYPE^^}="${DIST_DIR}/telaf-${EXTRA_TYPE}/lib"
        else
            echo "Error: Unable to find the required prebuilt files for telaf-${EXTRA_TYPE}."
        fi
    fi
}

function build_target() {
    TARGET=$1

    # Build TelAF OSS source code
    make "${TARGET}"
    if [ $? -ne 0 ]; then
        echo "Error: when making target ${TARGET}"
        return
    fi

    # Build telaf-prop source code if exists
    build_extras "prop" "${TARGET}"

    # Build telaf-noship source code if exists
    build_extras "noship" "${TARGET}"

    # Repack TelAF image
    local TELAF_REPACK_DIR="${TELAF_ROOT}/build/${TARGET}/"
    local TELAF_NOSHIP_BUILD_DIR="${TELAF_ROOT}/build/${TARGET}/telaf-noship"
    local TELAF_PROP_BUILD_DIR="${TELAF_ROOT}/build/${TARGET}/telaf-prop"
    [[ ! -d $TELAF_NOSHIP_BUILD_DIR ]] && TELAF_NOSHIP_BUILD_DIR=$TELAF_NOSHIP
    [[ ! -d $TELAF_PROP_BUILD_DIR ]] && TELAF_PROP_BUILD_DIR=$TELAF_PROP

    echo "### telaf-noship dir: ${TELAF_NOSHIP_BUILD_DIR} ###"
    echo "### telaf-prop dir: ${TELAF_PROP_BUILD_DIR} ###"
    ${TELAF_ROOT}/mkimg.sh "${TARGET}" "$TELAF_REPACK_DIR" "$TELAF_NOSHIP_BUILD_DIR" "$TELAF_PROP_BUILD_DIR"
    if [ $? -ne 0 ]; then
        echo "Error: ${TELAF_ROOT}/mkimg.sh ${TARGET} "$TELAF_REPACK_DIR" "$TELAF_NOSHIP_BUILD_DIR" "$TELAF_PROP_BUILD_DIR""
        return
    fi

    # Sign TelAF image
    export AVBTOOL="${OECORE_NATIVE_SYSROOT}/usr/share/avb_py_tool"
    if [ -e ${AVBTOOL}/avbtool ]; then
        if [ ! -d $AVBTOOL/keys ]; then
            ${AVBTOOL}/avbtool add_hashtree_footer --image ./build/${TARGET}/telaf_ro.squashfs --partition_name telaf --do_not_generate_fec --rollback_index 0
        else
            ${AVBTOOL}/avbtool add_hashtree_footer --image ./build/${TARGET}/telaf_ro.squashfs --partition_name telaf --algorithm SHA256_RSA2048 --key $AVBTOOL/keys/qpsa_attest.key --public_key_metadata $AVBTOOL/keys/qpsa_attest.der --do_not_generate_fec --rollback_index 0
        fi
        if [ $? -ne 0 ]; then
            echo "Error: during image signing."
            return
        fi
    else
        echo "Warning: avbtool not found"
    fi

    if [ "${GCC_PREFIX}" = "arm-oemllib32-linux-gnueabi" ]; then
        mv     $TELAF_ROOT/build/${TARGET}/telaf_ro.squashfs            $TELAF_ROOT/build/${TARGET}/telaf_ro.32bit.squashfs
        mv     $TELAF_ROOT/build/${TARGET}/telaf_ro.squashfs.ubi        $TELAF_ROOT/build/${TARGET}/telaf_ro.32bit.squashfs.ubi
        ln -sf $TELAF_ROOT/build/${TARGET}/telaf_ro.32bit.squashfs      $TELAF_ROOT/build/${TARGET}/telaf_ro.squashfs
        ln -sf $TELAF_ROOT/build/${TARGET}/telaf_ro.32bit.squashfs.ubi  $TELAF_ROOT/build/${TARGET}/telaf_ro.squashfs.ubi
    elif [ "${GCC_PREFIX}" = "aarch64-oe-linux" ]; then
        mv     $TELAF_ROOT/build/${TARGET}/telaf_ro.squashfs            $TELAF_ROOT/build/${TARGET}/telaf_ro.64bit.squashfs
        mv     $TELAF_ROOT/build/${TARGET}/telaf_ro.squashfs.ubi        $TELAF_ROOT/build/${TARGET}/telaf_ro.64bit.squashfs.ubi
        ln -sf $TELAF_ROOT/build/${TARGET}/telaf_ro.64bit.squashfs      $TELAF_ROOT/build/${TARGET}/telaf_ro.squashfs
        ln -sf $TELAF_ROOT/build/${TARGET}/telaf_ro.64bit.squashfs.ubi  $TELAF_ROOT/build/${TARGET}/telaf_ro.squashfs.ubi
    fi

    if [ $? -eq 0 ]; then
        # create the tarball for telaf app dependencies used for sdk patch
        ${TELAF_ROOT}/bin/createsdk "${TARGET}" "${TELAF_ROOT}/../"
    fi
}

# Build functions for different targets
function build-sa415m-af() {
    local TARGET="sa415m"
    if [ "$TARGET" != "$TARGET_GLOBAL" ]; then
        echo "Error: Target parameter mismatch. Expected: $TARGET_GLOBAL, Actual: $TARGET"
        return 1
    fi
    build_target "${TARGET}"
}

function build-sa515m-af() {
    local TARGET="sa515m"
    if [ "$TARGET" != "$TARGET_GLOBAL" ]; then
        echo "Error: Target parameter mismatch. Expected: $TARGET_GLOBAL, Actual: $TARGET"
        return 1
    fi
    build_target "${TARGET}"
}

function build-sa525m-af() {
    local TARGET="sa525m"
    if [ "$TARGET" != "$TARGET_GLOBAL" ]; then
        echo "Error: Target parameter mismatch. Expected: $TARGET_GLOBAL, Actual: $TARGET"
        return 1
    fi
    build_target "${TARGET}"
}

function build-clean-af(){
    make clean
}

function build-distclean-af(){
    make distclean
}

export TARGET_GLOBAL="$1"
setup_toolchain "$TARGET_GLOBAL"


