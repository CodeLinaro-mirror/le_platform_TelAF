# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

echo "CURDIR"
echo $(pwd)

echo "TELAF_ROOT"
echo $TELAF_ROOT

cp "$TELAF_ROOT/apps/ivss/common/CMakeLists.txt" ./

rm -rf build
if [ ! -d build ];then
    mkdir build
else
    echo build exist
fi

cd build
cmake -DLEGATO_TARGET="${LEGATO_TARGET}" -DLEGATO_ROOT="${LEGATO_ROOT}" -DTELAF_ROOT="${TELAF_ROOT}" -DLEGATO_SYSROOT="${LEGATO_SYSROOT}" -DPROJECT_NAME=$1 ..
make
