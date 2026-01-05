
# Set global variables
export CURDIR=$(cd "$(dirname "$1")" || exit ; pwd)
if [ -d "$TELAF_ROOT" ] && [ "$TELAF_ROOT" != "$CURDIR" ]; then
    echo "Error: The TELAF_ROOT was detected as already being present in this shell environment and inconsistent with the environment to be set. Please use a clean shell when sourcing this environment script."
    return
fi

PROG_NAME="set_af_env.sh"

export TELAF_ROOT=${CURDIR}
export LEGATO_ROOT=${CURDIR}/../legato/legato-af
export LEGATO_VERSION=$(cat "${TELAF_ROOT}/VERSION" 2>/dev/null)

export TELAF_PROP=${CURDIR}/../telaf-prop
export TELAF_NOSHIP=${CURDIR}/../telaf-noship

export TELAF_PA_DEFAULT=${CURDIR}/../telaf-pa-default
export TELAF_PA=${CURDIR}/../telaf-pa

# Setup toolchain from default location
setup_toolchain_default_location() {
    local TARGET=$1

    if [ "$TARGET" == "simulation" ];then
        local DEF_TOOLCHAIN_PATH=/tmp
    else
        local DEF_TOOLCHAIN_PATH=/opt/qct/$TARGET
    fi

    echo "Setup Toolchain for $TARGET from $DEF_TOOLCHAIN_PATH"
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
    elif [ "$TARGET" == "simulation" ]; then
        # Simulation target depends on the HOST toolchains without any prefix & pre-build packages
        export SYSROOT=/
    else
        echo " Missing target parameter!"
        echo " e.g. source $PROG_NAME sa415m"
        return
    fi

    if echo "$GCC_PREFIX" | grep -q "lib32"; then
        if echo "$CFLAGS" | grep -q "\-D_TIME_BITS=64"; then
            MKTOOLS_X_C_FLAGS="-X -D_TIME_BITS=64 -X -D_FILE_OFFSET_BITS=64"
            MKTOOLS_X_C_FLAGS+=" -C -D_TIME_BITS=64 -C -D_FILE_OFFSET_BITS=64"
            export MKTOOLS_X_C_FLAGS=$MKTOOLS_X_C_FLAGS
        fi
    fi

    # TELAF_GLOBAL_TARGET_TOOLCHAIN_PATH environment variable will be used in findtoochain script.
    export TELAF_GLOBAL_TARGET_TOOLCHAIN_PATH=$DEF_TOOLCHAIN_PATH
}

# Setup toolchain from custom location
setup_toolchain_custom_location() {
    local TARGET=$1
    # Tool chain base path
    local TC_BASE_PATH=$2
    echo "Setup Toolchain for $TARGET from $TC_BASE_PATH"

    if [ "$TARGET" == "sa415m" ]; then
        if [ -e $TC_BASE_PATH/environment-setup-armv7at2hf-neon-oe-linux-gnueabi ]; then
            source $TC_BASE_PATH/environment-setup-armv7at2hf-neon-oe-linux-gnueabi
        else
            echo "Invalid toolchain path"
            return
        fi
    elif [ "$TARGET" == "sa515m" ]; then
        if [ -e $TC_BASE_PATH/environment-setup-armv7at2hf-neon-oe-linux-gnueabi ]; then
            source $TC_BASE_PATH/environment-setup-armv7at2hf-neon-oe-linux-gnueabi
        else
            echo "Invalid toolchain path"
            return
        fi
    elif [ "$TARGET" == "sa525m" ]; then
        if [ -e $TC_BASE_PATH/environment-setup-armv7at2hf-neon-oemllib32-linux-gnueabi ]; then
            # For 32bit tool chain build
            export GCC_PREFIX="arm-oemllib32-linux-gnueabi"
            source $TC_BASE_PATH/environment-setup-armv7at2hf-neon-oemllib32-linux-gnueabi
        elif [ -e $TC_BASE_PATH/environment-setup-aarch64-oe-linux ]; then
            # For 64bit tool chain build
            export GCC_PREFIX="aarch64-oe-linux"
            source $TC_BASE_PATH/environment-setup-aarch64-oe-linux
        else
            echo "Invalid toolchain path"
            return
        fi
    elif [ "$TARGET" == "simulation" ]; then
        # Simulation target depends on the HOST toolchains without any prefix & pre-build packages
        export SYSROOT=/
        if [ ! -e "$TC_BASE_PATH" ]; then
            echo "Invalid toolchain path"
            return
        fi
    else
        echo "Invalid target"
        return
    fi

    if echo "$GCC_PREFIX" | grep -q "lib32"; then
        if echo "$CFLAGS" | grep -q "\-D_TIME_BITS=64"; then
            MKTOOLS_X_C_FLAGS="-X -D_TIME_BITS=64 -X -D_FILE_OFFSET_BITS=64"
            MKTOOLS_X_C_FLAGS+=" -C -D_TIME_BITS=64 -C -D_FILE_OFFSET_BITS=64"
            export MKTOOLS_X_C_FLAGS=$MKTOOLS_X_C_FLAGS
        fi
    fi

    # TELAF_GLOBAL_TARGET_TOOLCHAIN_PATH environment variable will be used in findtoochain script.
    export TELAF_GLOBAL_TARGET_TOOLCHAIN_PATH="$TC_BASE_PATH"
}

umask 002

# Function to build telaf-prop and telaf-noship
build_extras() {
    # The declare -A EXTRA_ENUM command is used to define an associative array that maps EXTRA_TYPE values to their corresponding constants
    # Later EXTRA_ENUM[${EXTRA_TYPE}] is used to retrieve the value associated with a specific EXTRA_TYPE.
    declare -A EXTRA_ENUM=(
        [prop]="TELAF_PROP"         # Map 'prop' to 'TELAF_PROP'
        [noship]="TELAF_NOSHIP"     # Map 'noship' to 'TELAF_NOSHIP'
		[pa]="TELAF_PA"
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
            echo "Warning: Unable to find the required prebuilt files for telaf-${EXTRA_TYPE}."
        fi
    fi
}

build_extras_pa() {
    local EXTRA_SRC_PATH=$1
    local INSTALL_DIR=$2
    local DLT_LOGGING="false"

    if [ "${ENABLE_DLT_LOGGING:-}" = "1" ]; then
        echo "ENABLE_DLT_LOGGING is set for building PA."
        DLT_LOGGING="true"
    fi

    if [ -z "${EXTRA_SRC_PATH}" ]; then
        echo "Error: Missing EXTRA_SRC_PATH argument"
        return 1
    fi

    if [ -f "${EXTRA_SRC_PATH}/build_pa.sh" ]; then
        echo ">>> Running ${EXTRA_SRC_PATH}/build_pa.sh"
        (cd "${EXTRA_SRC_PATH}" && ./build_pa.sh "${INSTALL_DIR}" "${DLT_LOGGING}")
        if [ $? -ne 0 ]; then
            echo "Error: when running ${EXTRA_SRC_PATH}/build_pa.sh"
            return 1
        fi
        echo ">>> Build completed. Output: ${INSTALL_DIR}"
    fi
}

clean_extra_build() {
    local DIRS=("$TELAF_PROP" "$TELAF_NOSHIP")
    local TARGETS=("build" "staging")

    for base in "${DIRS[@]}"; do
        if [[ -d "$base" ]]; then
            for sub in "${TARGETS[@]}"; do
                local target="$base/$sub"
                if [[ -d "$target" ]]; then
                    echo "Cleaning $target"
                    rm -rf "$target"
                fi
            done
        fi
    done
}

function build_target() {
    local TARGET=$1

    export TELAF_TARGET_PA_LIB_DIR="${TELAF_PA}/staging/"
    build_extras_pa "${TELAF_PA}" "${TELAF_TARGET_PA_LIB_DIR}/"
    if [ $? -ne 0 ]; then
        echo "Error: when building target PA for target ${TARGET}"
        return
    fi

    export TELAF_DEFAULT_PA_LIB_DIR="${TELAF_PA_DEFAULT}/staging/"
    build_extras_pa "${TELAF_PA_DEFAULT}" "${TELAF_DEFAULT_PA_LIB_DIR}/"
    if [ $? -ne 0 ]; then
        echo "Error: when building default PA for target ${TARGET}"
        return
    fi

    # Build TelAF OSS source code
    make "${TARGET}"
    if [ $? -ne 0 ]; then
        echo "Error: when making target ${TARGET}"
        return
    fi

    # Repack TelAF image
    local TELAF_REPACK_DIR="${TELAF_ROOT}/build/${TARGET}/"
    local TELAF_NOSHIP_BUILD_DIR="${TELAF_ROOT}/build/${TARGET}/telaf-noship"
    local TELAF_PROP_BUILD_DIR="${TELAF_ROOT}/build/${TARGET}/telaf-prop"
    local TELAF_PA_BUILD_DIR="${TELAF_ROOT}/build/${TARGET}/telaf-pa"

    build_extras_pa "${TELAF_PROP}" "${TELAF_PROP_BUILD_DIR}/"
    if [ $? -ne 0 ]; then
        echo "Error: when building target PROP for target ${TARGET}"
        return
    fi

    build_extras_pa "${TELAF_NOSHIP}" "${TELAF_NOSHIP_BUILD_DIR}/"
    if [ $? -ne 0 ]; then
        echo "Error: when building target NOSHIP for target ${TARGET}"
        return
    fi

    echo "### telaf-noship dir: ${TELAF_NOSHIP_BUILD_DIR} ###"
    echo "### telaf-prop dir: ${TELAF_PROP_BUILD_DIR} ###"

    ${TELAF_ROOT}/mkimg.sh \
      -t "${TARGET}" \
      -o "${TELAF_REPACK_DIR}" \
      -s "${TELAF_REPACK_DIR}/_staging_system.${TARGET}.update_ro" \
      -n "${TELAF_NOSHIP_BUILD_DIR}" \
      -p "${TELAF_PROP_BUILD_DIR}" \
      -a "${TELAF_PA_BUILD_DIR}" \
      -r "${TELAF_ROOT}" \
      -w "${TELAF_TARGET_PA_LIB_DIR}" \
      -d "${TELAF_DEFAULT_PA_LIB_DIR}"
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

function build-sa525m-lxc-af() {
    local TARGET="sa525m"
    if [ "$TARGET" != "$TARGET_GLOBAL" ]; then
        echo "Error: Target parameter mismatch. Expected: $TARGET_GLOBAL, Actual: $TARGET"
        return 1
    fi
    # Set the flavor to lxc to build TelAF system for the container.
    export BUILD_FLAVOR=lxc
    build_target "${TARGET}"
}

function build-simulation-af() {
    local TARGET="simulation"
    if [ "$TARGET" != "$TARGET_GLOBAL" ]; then
        echo "Error: Target parameter mismatch. Expected: $TARGET_GLOBAL, Actual: $TARGET"
        return 1
    fi

    # Ex: build-simulation-af IMPORT_SDK_SIMULATION=y TELAF_SIMULATION_ENABLE_SMS=y sdk_rootfs=/path/to/sdk_rootfs
    make simulac $@

    if [ $? -eq 0 ]; then
        # create the tarball for telaf app dependencies used for sdk patch
        ${TELAF_ROOT}/bin/createsdk "${TARGET}" "${TELAF_ROOT}/../"
    fi
}

function build-clean-af(){
    make clean
}

function build-distclean-af(){
    make distclean
    clean_extra_build
}

export TARGET_GLOBAL="$1"
if [ $# -eq 1 ]; then
    # only target is provided. Setup toolchain from default location
    setup_toolchain_default_location "$TARGET_GLOBAL"
elif [ $# -eq 2 ]; then
    # target and toolchain locations are provided. Setup toolchain from provided location
    setup_toolchain_custom_location "$TARGET_GLOBAL" $2
else
    echo "Correct Usage: source $PROG_NAME <target> <toolchain location(optional)>"
    echo "Valid targets: sa415m, sa515m, sa525m, simulation"
fi
