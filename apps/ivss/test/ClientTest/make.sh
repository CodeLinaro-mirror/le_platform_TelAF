# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

echo "CURDIR"
echo $(pwd)

# CommonAPI tools, change to commands on your own environment
capicxx-core-gen -sk ../../fidl/RadioSvc/RadioSvc.fidl
capicxx-core-gen -sk ../../fidl/RadioSvc/RadioSvc.fdepl
capicxx-someip-gen ../../fidl/RadioSvc/RadioSvc.fdepl

capicxx-core-gen -sk ../../fidl/SimSvc/SimSvc.fidl
capicxx-core-gen -sk ../../fidl/SimSvc/SimSvc.fdepl
capicxx-someip-gen ../../fidl/SimSvc/SimSvc.fdepl

capicxx-core-gen -sk ../../fidl/InfoSvc/InfoSvc.fidl
capicxx-core-gen -sk ../../fidl/InfoSvc/InfoSvc.fdepl
capicxx-someip-gen ../../fidl/InfoSvc/InfoSvc.fdepl

rm -rf build
if [ ! -d build ];then
    mkdir build
else
    echo build exist
fi

cd build
cmake -DCMAKE_INSTALL_PREFIX=../install-soa/ ..
make
make install
