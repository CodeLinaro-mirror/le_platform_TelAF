# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

echo "CURDIR"
echo $(pwd)

echo "TELAF_ROOT"
echo $TELAF_ROOT

cp "$TELAF_ROOT/apps/sample/etsSvc/common/CMakeLists.txt" ./

rm -rf build
if [ ! -d build ];then
    mkdir build
else
    echo build exist
fi

cd build
cmake -DPROJECT_NAME=$1 ..
make
