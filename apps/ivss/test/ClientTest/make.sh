# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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

capicxx-core-gen -sk ../../fidl/MngdConnSvc/MngdConnSvc.fidl
capicxx-core-gen -sk ../../fidl/MngdConnSvc/MngdConnSvc.fdepl
capicxx-someip-gen ../../fidl/MngdConnSvc/MngdConnSvc.fdepl

capicxx-core-gen -sk ../../fidl/LocationSvc/LocationSvc.fidl
capicxx-core-gen -sk ../../fidl/LocationSvc/LocationSvc.fdepl
capicxx-someip-gen ../../fidl/LocationSvc/LocationSvc.fdepl

capicxx-core-gen -sk ../../fidl/SensorSvc/SensorSvc.fidl
capicxx-core-gen -sk ../../fidl/SensorSvc/SensorSvc.fdepl
capicxx-someip-gen ../../fidl/SensorSvc/SensorSvc.fdepl

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
